#ifndef TASK_H
#define TASK_H

#include <pthread.h>
#include <stdio.h>

#define MAX_PATH_LENGTH 100
#define MAX_TASKS 2

struct task_details
{
    int task_id, status, priority;
    int files, dirs;
    char path[MAX_PATH_LENGTH];
    long long total_size;
    pthread_mutex_t *permission_mutex;
    pthread_mutex_t *status_mutex;
};

void output_task(struct task_details *task);

/* Check the permission mutex.
 */
void permission_to_continue(struct task_details *task);

/* Change task status.
 */
void set_task_status(struct task_details *task, int status);

/*  Return a new task with the given id, path and priority.
 */
struct task_details *init_task(int id, char *path, int priority);

/*  Free the memory allocated for the task.
 */
void destroy_task(struct task_details *task);

void suspend_task(struct task_details *task);

void resume_task(struct task_details *task);

#endif // TASK_H
