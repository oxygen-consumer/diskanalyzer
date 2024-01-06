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
    for (int i = 0; i < MAX_TASKS; ++i)
    {
        task[i] = NULL;
    }
    int id_next = 0;

    // used for print, to be deleted after
    char thread_output[MAX_PATH_SIZE];
    char buffer[1024];
    //

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

            /*
             * FIXME: For now we print everything to syslog,
             * for test purposes.
             * We will need a function to process them accordingly.
             */
            struct message msg;
            while ((nread = read(cfd, &msg, sizeof(msg))) > 0)
            {
                syslog_message(&msg);
            }

            if (nread == -1)
            {
                syslog(LOG_USER | LOG_WARNING, "Failed to read from socket.");
                continue;
            }

            if (close(cfd) == -1)
            {
                syslog(LOG_USER | LOG_WARNING, "Failed to close connetion.");
                continue;
            }
            syslog(LOG_USER | LOG_WARNING, "start switch");
            switch (msg.task_code)
            {
            case ADD:
                task[id_next] = init_task(id_next, msg.path, msg.priority);
                if (task[id_next] == NULL)
                {
                    syslog(LOG_USER | LOG_WARNING, "Failed to create task.");
                    break;
                }
                if (pthread_create(&threads[id_next], NULL, start_analyses_thread, task[id_next]) != 0)
                {
                    syslog(LOG_USER | LOG_WARNING, "Failed to create thread.");
                    break;
                }
                syslog(LOG_USER | LOG_WARNING, "Created thread.");
                ++id_next;
                break;
            case PRIORITY:
                syslog(LOG_USER | LOG_WARNING, "Not yet implemented. Task for id %d rejected.", msg.id);
                break;
            case SUSPEND:
                if (msg.id < 0 || msg.id >= MAX_TASKS || task[msg.id] == NULL)
                {
                    syslog(LOG_USER | LOG_WARNING, "Task %d does not exist.", msg.id);
                }
                else
                {
                    set_task_status(task[msg.id], PAUSED);
                }
                break;
            case RESUME:
                if (msg.id < 0 || msg.id >= MAX_TASKS || task[msg.id] == NULL)
                {
                    syslog(LOG_USER | LOG_WARNING, "Task %d does not exist.", msg.id);
                }
                else
                {
                    set_task_status(task[msg.id], RUNNING);
                }
                break;
            case REMOVE:
                if (msg.id < 0 || msg.id >= MAX_TASKS || task[msg.id] == NULL)
                {
                    syslog(LOG_USER | LOG_WARNING, "Invalid task id.");
                    break;
                }
                else
                {
                    remove_task(task[msg.id], &threads[msg.id]);
                    task[msg.id] = NULL;
                }
                break;

            // after this is just response
            case INFO:
                if (task[msg.id] != NULL)
                {
                    output_task(task[msg.id]);
                }
                else
                {
                    syslog(LOG_USER | LOG_WARNING, "Task %d does not exist.", msg.id);
                }
                break;
            case LIST:
                for (int i = 0; i < id_next; ++i)
                {
                    if (task[i] != NULL)
                    {
                        output_task(task[i]);
                    }
                }
                break;
            case PRINT:
                snprintf(thread_output, MAX_PATH_SIZE, "/tmp/diskanalyzer_%d.txt", msg.id);
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

// int main()
// {
//     pthread_t threads[MAX_TASKS];
//     struct task_details *task[MAX_TASKS];
//     for (int i = 0; i < MAX_TASKS; ++i)
//     {
//         task[i] = init_task(i);
//     }

//     int result;
//     for (int i = 0; i < MAX_TASKS; i++)
//     {
//         strcpy(task[i]->path, "/home/");
//         result = pthread_create(&threads[i], NULL, start_analyses_thread, task[i]);
//         if (result != 0)
//         {
//             perror("Thread creation failed");
//             return 1;
//         }
//     }

//     int id_selectat = 0;
//     suspend_task(task[id_selectat]);
//     sleep(1);
//     printf("id_selectat = %d\n", id_selectat);
//     resume_task(task[id_selectat]);

//     for (int i = 0; i < MAX_TASKS; i++)
//     {
//         result = pthread_join(threads[i], NULL);
//         if (result != 0)
//         {
//             perror("Thread join failed");
//             return 1;
//         }
//         // printf("Main thread %d finished.\n", i);
//         destroy_task(task[i]);
//     }

//     return 0;
// }