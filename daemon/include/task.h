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
    enum Priority priority;
    enum Status status;
    double progress;
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
 * Returns 0 when succes.
 * Please call with set_task_status(task, PAUSED).
 */
int suspend_task(struct task_details *task);

/*
 * Resume a thread by unlocking the mutex.
 * Returns 0 when succes.
 * Please call with set_task_status(task, RUNNING).
 */
int resume_task(struct task_details *task);

/*
 * Close the outputfd and set the task status to finished
 * Please call with set_task_status(task, FINISHED).
 * Returns 0 when succes.
 */
int finish_task(struct task_details *task);

/*
 * Change task status if the flow is ok.
 * Returns 0 when succes.
 */
int set_task_status(struct task_details *task, enum Status status);

/*
 * Cancel thread if working and free memory. Returns 0 when succes.
 */
int remove_task(struct task_details *task, pthread_t *thread);

#endif // TASK_H
