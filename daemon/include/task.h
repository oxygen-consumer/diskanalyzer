#ifndef TASK_H
#define TASK_H

#include <constants.h>

#include <pthread.h>
#include <shared.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_TASKS 2

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
};

void output_task(struct task_details *task);

/*
 * Check the permission mutex.
 */
void permission_to_continue(struct task_details *task);

/*
 * Change task status.
 */
void set_task_status(struct task_details *task, enum Status status);

/*
 * Return a new task with the given id, path and priority.
 */
struct task_details *init_task(int id, char *path, enum Priority priority);

/*
 * Free the memory allocated for the task.
 */
void destroy_task(struct task_details *task);

void suspend_task(struct task_details *task);

void resume_task(struct task_details *task);

#endif // TASK_H
