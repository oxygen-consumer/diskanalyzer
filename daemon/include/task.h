#ifndef TASK_H
#define TASK_H

#include <constants.h>
#include <pthread.h>
#include <shared.h>
#include <stdio.h>
#include <stdlib.h>

struct task_details
{
    int task_id;
    int files, dirs;
    char path[MAX_PATH_SIZE];
    long long total_size;
    enum Priority priority;
    enum Status status;
    pthread_mutex_t *permission_mutex;
    pthread_mutex_t *status_mutex;
    FILE *output_fd;
};

void output_task(struct task_details *task);

/*
 * Check the permission mutex.
 */
void permission_to_continue(struct task_details *task);

/*
 *Return a new task with the given id, path and priority.
 */
struct task_details *init_task(int id, char *path, enum Priority priority);

/*
 * Free the memory allocated for the task.
 */
void destroy_task(struct task_details *task);

/*
 * Suspend a task by locking the mutex.
 * Please call with set_task_status(task, PAUSED).
 */
void suspend_task(struct task_details *task);

/*
 * Resume a thread by unlocking the mutex.
 * Please call with set_task_status(task, RUNNING).
 */
void resume_task(struct task_details *task);

/*
 * Close the outputfd and set the task status to finished
 * Please call with set_task_status(task, FINISHED).
 */
void finish_task(struct task_details *task);

/*
 * Change task status.
 * Lock the status mutex and change the status if the flow is ok.
 */
void set_task_status(struct task_details *task, enum Status status);

/*
 * ONLY IF IT IS FINISHED!
 * we cannot remove task that are not finished because we cannot clean the opened directories in the recursive function
 * to write the report at this moment.
 * Cancel thread if working and free memory
 */
int remove_task(struct task_details *task, pthread_t *thread);

#endif // TASK_H
