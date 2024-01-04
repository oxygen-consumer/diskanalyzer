#ifndef ANALYZER_H
#define ANALYZER_H

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>

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

/* Returns the size of a directory (including subdirectories).
 */
long long get_size_dir(const char *path, struct task_details *task);

/* Writes the analyze report to the file descriptor.
 */
long long analyzing(const char *path, struct task_details *task, FILE *output_fd);

/* Start the thread report.
 */
void *start_analyses_thread(void *arg);

void suspend_task(struct task_details *task);

void resume_task(struct task_details *task);

void write_report_info(FILE *output_fd, const char *path, long long size, struct task_details *task);

/*  Return a new task with the given id.
 */
struct task_details *init_task(int id);

/*  Free the memory allocated for the task.
 */
void destroy_task(struct task_details *task);

#endif // ANALYZER_H
