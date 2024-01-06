#include <analyzer.h>
#include <constants.h>
#include <daemonize.h>
#include <shared.h>
#include <utils.h>

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <syslog.h>
#include <unistd.h>

/*
`SCHED_OTHER (0):** This is the default scheduling policy for threads. Threads with this policy are scheduled in a
round-robin fashion, with higher priority threads getting more CPU time. The priority range for this policy is from 0
to 99.

`SCHED_RR (1):** This is a real-time scheduling policy that provides a guaranteed amount of CPU time for threads.
Threads with this policy are scheduled round-robin fashion, but they are not preempted unless they have used up their
timeslice. The priority range for this policy is from 1 to 99.

SCHED_FIFO (2):** This is another real-time scheduling policy that provides an even higher priority thanSCHED_RR`.
Threads with this policy are scheduled in a first-in, first-out (FIFO) fashion, and they are not preempted unless they
voluntarily give up the CPU or they have been stopped by the operating system. The priority range for this policy is
from 1 to 99.

`SCHED_IDLE (3):** This is a scheduling policy that is specifically designed for idle threads. Threads with this policy
are not scheduled for CPU time unless there are no other threads that can run. The priority range for this policy is
from 0 to 1.
*/

int main(void)
{

    // Connect to system log
    const char *LOG_NAME = "diskanalyzer";
    openlog(LOG_NAME, LOG_PID, LOG_USER);

    // Make the process a daemon
    if (daemonize())
    {
        die(false, "Unable to start daemon. Exiting.");
    }

    // Create tasks directory
    char TASKS_DIR[50];
    sprintf(TASKS_DIR, "/var/run/user/%d/da_tasks", getuid());
    if (mkdir(TASKS_DIR, 0777) == -1)
    {
        die(false, "Unable to create tasks directory. Exiting.");
    }

    // Socket preparation
    const int BACKLOG = 16, BUF_SIZE = 4096;

    char SV_SOCK_PATH[37];
    sprintf(SV_SOCK_PATH, "/var/run/user/%d/diskanalyzer.sock", getuid());

    struct sockaddr_un addr;
    char buf[BUF_SIZE];
    int sfd, cfd; // server fd, client fd
    ssize_t nread;
    pthread_t threads[MAX_TASKS];
    struct task_details *task[MAX_TASKS];
    int used_tasks[MAX_TASKS];
    for (int i = 0; i < MAX_TASKS; ++i)
    {
        threads[i] = 0;
        task[i] = NULL;
        used_tasks[i] = 0;
    }
    int next_id = 1;

    // used for print, to be deleted after
    char thread_output[MAX_OUTPUT_SIZE];
    char buffer[1024];

    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1)
    {
        die(false, "Unable to open socket. Exiting.");
    }

    // Open the socket and bind it
    if (remove(SV_SOCK_PATH) == -1 && errno != ENOENT)
    {
        die(false, "Could not open socket at %s. Exiting.", SV_SOCK_PATH);
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SV_SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1)
    {
        die(false, "Could not bind socket. Exiting.");
    }

    if (listen(sfd, BACKLOG) == -1)
    {
        die(false, "Unable to listen on socket. Exiting.");
    }

    // Everything ready
    syslog(LOG_USER | LOG_INFO, "Diskanalyzer daemon ready.");

    while (true)
    {
        // Handle every incomming connection
        for (;;)
        {

            cfd = accept(sfd, NULL, NULL);
            if (cfd == -1)
            {
                syslog(LOG_USER | LOG_WARNING, "Could not accept connection.");
                continue;
            }

            struct message msg;

            int bytes_received = recv(cfd, &msg, sizeof(msg), 0);
            if (bytes_received == -1)
            {
                syslog(LOG_USER | LOG_WARNING, "Failed to receive message.");
                close(cfd);
                continue;
            }

            syslog_message(&msg);

            ssize_t bytes_sent = -1;
            struct Response response;
            response.response_code = NO_RESPONSE;
            response.message[0] = '\0';

            switch (msg.task_code)
            {
            case ADD: {
                int thread_id = get_unused_task(used_tasks);
                if (thread_id == -1)
                {
                    syslog(LOG_USER | LOG_WARNING, "No more tasks can be added.");
                    send_error_response(cfd, GENERAL_ERROR);
                    break;
                }
                task[thread_id] = init_task(next_id++, msg.path, msg.priority);
                if (task[thread_id] == NULL)
                {
                    syslog(LOG_USER | LOG_WARNING, "Failed to initialize task.");
                    send_error_response(cfd, GENERAL_ERROR);
                    break;
                }

                // Assign priority to thread
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                struct sched_param param;
                switch (task[thread_id]->priority)
                {
                case LOW:
                    param.sched_priority = 0;
                    break;
                case MEDIUM:
                    param.sched_priority = 50;
                    break;
                case HIGH:
                    param.sched_priority = 99;
                    break;
                default:
                    break;
                }
                pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
                pthread_attr_setschedparam(&attr, &param);

                if (pthread_create(&threads[thread_id], &attr, start_analyses_thread, task[thread_id]) != 0)
                {
                    syslog(LOG_USER | LOG_WARNING, "Failed to create thread.");
                    send_error_response(cfd, GENERAL_ERROR);
                    break;
                }
                used_tasks[thread_id] = 1;
                response.response_code = OK;
                snprintf(response.message, MAX_PATH_SIZE, "%d", task[thread_id]->task_id);
                send(cfd, &response, sizeof(response), 0);
                syslog(LOG_USER | LOG_WARNING, "Created thread.");
                break;
            }

            case SUSPEND: {
                int thread_id = get_thread_id(msg.id, task);
                if (thread_id == -1)
                {
                    syslog(LOG_USER | LOG_WARNING, "Task %d does not exist.", msg.id);
                    send_error_response(cfd, INVALID_ID_ERROR);
                }
                else
                {
                    syslog(LOG_USER | LOG_WARNING, "Task %d status: %d", msg.id, task[thread_id]->status);
                    if (task[thread_id]->status == PAUSED)
                    {
                        syslog(LOG_USER | LOG_WARNING, "Task %d is already suspended.", msg.id);
                        send_error_response(cfd, TASK_ALREADY_SUSPENDED_ERROR);
                        break;
                    }
                    else if (task[thread_id]->status == FINISHED)
                    {
                        syslog(LOG_USER | LOG_WARNING, "Task %d is already finished.", msg.id);
                        send_error_response(cfd, TASK_ALREADY_FINISHED_ERROR);
                        break;
                    }

                    set_task_status(task[thread_id], PAUSED);
                    syslog(LOG_USER | LOG_WARNING, "Task %d suspended.", msg.id);
                    response.response_code = OK;
                    send(cfd, &response, sizeof(response), 0);
                }

                break;
            }
            case RESUME: {
                int thread_id = get_thread_id(msg.id, task);
                if (thread_id == -1)
                {
                    syslog(LOG_USER | LOG_WARNING, "Task %d does not exist.", msg.id);
                    send_error_response(cfd, INVALID_ID_ERROR);
                    break;
                }
                else
                {
                    if (task[thread_id]->status == RUNNING)
                    {
                        syslog(LOG_USER | LOG_WARNING, "Task %d is already running.", msg.id);
                        send_error_response(cfd, TASK_ALREADY_RUNNING_ERROR);
                        break;
                    }
                    else if (task[thread_id]->status == FINISHED)
                    {
                        syslog(LOG_USER | LOG_WARNING, "Task %d is already finished.", msg.id);
                        send_error_response(cfd, TASK_ALREADY_FINISHED_ERROR);
                        break;
                    }

                    set_task_status(task[thread_id], RUNNING);
                    syslog(LOG_USER | LOG_WARNING, "Task %d resumed.", msg.id);
                    response.response_code = OK;
                    send(cfd, &response, sizeof(response), 0);
                }
                break;
            }

            case REMOVE: {
                int thread_id = get_thread_id(msg.id, task);
                if (remove_task(task[thread_id], &threads[thread_id]) != 0)
                {
                    if (task[thread_id] != NULL)
                        output_task(task[thread_id]);
                    syslog(LOG_USER | LOG_WARNING, "Removed faild %d.", msg.id);
                    send_error_response(cfd, GENERAL_ERROR);
                }
                else
                {
                    task[thread_id] = NULL;
                }
                response.response_code = OK;
                send(cfd, &response, sizeof(response), 0);
                break;
            }
            case INFO: {
                int thread_id = get_thread_id(msg.id, task);
                if (thread_id == -1)
                {
                    send_error_response(cfd, INVALID_ID_ERROR);
                    syslog(LOG_USER | LOG_WARNING, "Task %d does not exist.", msg.id);
                    break;
                }
                else
                {
                    response.response_code = OK;
                    // TO DO !!!!
                    //  PUT INFO ABOUT TASK IN RESPONSE.MESSAGE
                    snprintf(response.message, MAX_PATH_SIZE, "ID: %d, Priority: %d, Path: %s, Status: %s",
                             task[thread_id]->task_id, task[thread_id]->priority, task[thread_id]->path,
                             status_to_string(task[thread_id]->status));
                    send(cfd, &response, sizeof(response), 0);
                    syslog(LOG_USER | LOG_WARNING, "Task %d info sent.", msg.id);
                }
                break;
            }

            case LIST:
                // TODO
                for (int i = 0; i < MAX_TASKS; i++)
                {
                    if (task[i] != NULL)
                    {
                        output_task(task[i]);
                    }
                }
                send_error_response(cfd, INVALID_ID_ERROR);
                break;

            case PRINT: {
                int thread_id = get_thread_id(msg.id, task);
                if (thread_id == -1)
                {
                    response.response_code = INVALID_ID_ERROR;
                    send(cfd, &response, sizeof(response), 0);
                    syslog(LOG_USER | LOG_WARNING, "Task %d does not exist.", msg.id);
                    break;
                }
                if (task[thread_id]->status != FINISHED)
                {
                    response.response_code = NOT_FINISHED_ERROR;
                    send(cfd, &response, sizeof(response), 0);
                    syslog(LOG_USER | LOG_WARNING, "Task %d is not finished.", msg.id);
                    break;
                }
                snprintf(thread_output, MAX_OUTPUT_SIZE, "/var/run/user/%d/da_tasks/task_%d.info", getuid(), msg.id);
                FILE *output_fd = fopen(thread_output, "w");
                if (output_fd == NULL)
                {
                    response.response_code = GENERAL_ERROR;
                    snprintf(response.message, MAX_PATH_SIZE, "Failed to open file.");
                    send(cfd, &response, sizeof(response), 0);
                    syslog(LOG_USER | LOG_WARNING, "Error opening file for id %d.", msg.id);
                    break;
                }

                while (fgets(buffer, sizeof(buffer), output_fd) != NULL)
                {
                    syslog(LOG_INFO, "%s", buffer);
                }
                response.response_code = OK;
                snprintf(response.message, MAX_PATH_SIZE, "/var/run/user/%d/da_tasks/task_%d.info", getuid(),
                         msg.id); // temporary

                fclose(output_fd);
                break;
            }

            default:
                syslog(LOG_USER | LOG_WARNING, "Invalid task code.");
                break;
            }
        }
    }

    /*
     * We should never reach this point.
     */

    die(false, "Something really bad happened. Exiting.");
}
