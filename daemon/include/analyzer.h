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
#define MAX_TASKS 5

struct task_details
{
    int task_id, status, priority;
    int files, dirs;
    char path[MAX_PATH_LENGTH];
    long long total_size;
};

void output_task(struct task_details *task);

struct task_details *task[MAX_TASKS];
pthread_mutex_t permission_mutex[MAX_TASKS];
pthread_mutex_t status_mutex[MAX_TASKS];
pthread_t threads[MAX_TASKS];

void permission_to_continue(int id);

void set_task_status(int id, int status);

/* Returns the size of a directory (including subdirectories).
 */
long long get_size_dir(const char *path, int task_id);

/* Writes the analyze report to the file descriptor.
 */
long long analyzing(const char *path, int task_id, int depth, FILE *output_fd);

/* Start the thread report.
 */
void *start_analyses_thread(void *arg);

#endif // ANALYZER_H
