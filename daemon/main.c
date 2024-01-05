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

int get_unused_task(int used_tasks[MAX_TASKS])
{
    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (used_tasks[i] == 0)
        {
            return i;
        }
    }
    return -1;
}

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
    int i = 0;

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
            // remove finished tasks (status == error and status = finished)
            // idk if we need to delete them or just keep them in memory for list and print
            for (int i = 0; i < MAX_TASKS; ++i)
            {
                if (task[i] != NULL && (task[i]->status == FINISHED || task[i]->status == ERROR))
                {
                    pthread_join(threads[i], NULL);
                    // destroy_task(task[i]);
                    task[i] = NULL;
                    used_tasks[i] = 0;
                }
            }

            cfd = accept(sfd, NULL, NULL);
            if (cfd == -1)
            {
                syslog(LOG_USER | LOG_WARNING, "Could not accept connection.");
                continue;
            }

            struct message msg;
            while ((nread = read(cfd, &msg, sizeof(msg))) > 0)
            {
                syslog_message(&msg);
            }

            if (nread == -1)
            {
                syslog(LOG_USER | LOG_WARNING, "Failed to read from socket.");
                close(cfd);
                continue;
            }

            if (close(cfd) == -1)
            {
                syslog(LOG_USER | LOG_WARNING, "Failed to close connetion.");
                continue;
            }

            switch (msg.task_code)
            {
            case ADD:
                i = get_unused_task(used_tasks);
                if (i == -1)
                {
                    syslog(LOG_USER | LOG_WARNING, "No more tasks available.");
                    break;
                }
                task[i] = init_task(i, msg.path, msg.priority);
                if (task[i] == NULL)
                {
                    syslog(LOG_USER | LOG_WARNING, "Failed to create task.");
                    break;
                }
                if (pthread_create(&threads[i], NULL, start_analyses_thread, task[i]) != 0)
                {
                    syslog(LOG_USER | LOG_WARNING, "Failed to create thread.");
                    break;
                }
                used_tasks[i] = 1;
                syslog(LOG_USER | LOG_WARNING, "Created thread.");
                break;

            case PRIORITY:
                if (task[msg.id] != NULL)
                {
                    task[msg.id]->priority = msg.priority;
                }
                else
                {
                    syslog(LOG_USER | LOG_WARNING, "Task %d does not exist.", msg.id);
                }
                break;

            case SUSPEND:
                if (task[msg.id] != NULL)
                {
                    suspend_task(task[msg.id]);
                }
                else
                {
                    syslog(LOG_USER | LOG_WARNING, "Task %d does not exist.", msg.id);
                }
                break;

            case RESUME:
                if (task[msg.id] != NULL)
                {
                    resume_task(task[msg.id]);
                }
                else
                {
                    syslog(LOG_USER | LOG_WARNING, "Task %d does not exist.", msg.id);
                }
                break;

            case REMOVE:
                if (task[msg.id] != NULL)
                {
                    if (task[msg.id]->status == 0)
                    {
                        if (pthread_cancel(threads[msg.id]) != 0)
                        {
                            syslog(LOG_USER | LOG_WARNING, "Failed to cancel thread.");
                        }
                        else
                        {
                            void *res;
                            if (pthread_join(threads[msg.id], &res) != 0)
                            {
                                syslog(LOG_USER | LOG_WARNING, "Failed to join thread.");
                            }
                            else if (res == PTHREAD_CANCELED)
                            {
                                syslog(LOG_USER | LOG_INFO, "Thread was cancelled.");
                                destroy_task(task[msg.id]);
                                task[msg.id] = NULL;
                                used_tasks[msg.id] = 0;
                            }
                            else
                            {
                                syslog(LOG_USER | LOG_INFO, "Thread was not cancelled.");
                            }
                        }
                    }
                }
                else
                {
                    syslog(LOG_USER | LOG_WARNING, "Task %d does not exist.", msg.id);
                }
                break;

            // after this is just response
            case INFO:
                if (task[msg.id] != NULL)
                {
                    // TODO:
                }
                else
                {
                    syslog(LOG_USER | LOG_WARNING, "Task %d does not exist.", msg.id);
                }
                break;

            case LIST:
                // TODO:
                break;

            case PRINT:
                snprintf(thread_output, MAX_OUTPUT_SIZE, "/var/run/user/%d/da_tasks/task_%d.info", getuid(), msg.id);
                FILE *output_fd = fopen(thread_output, "w");
                if (output_fd == NULL)
                {
                    syslog(LOG_USER | LOG_WARNING, "Error opening file for id %d.", msg.id);
                    break;
                }

                while (fgets(buffer, sizeof(buffer), output_fd) != NULL)
                {
                    syslog(LOG_INFO, "%s", buffer);
                }

                fclose(output_fd);
                break;

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
