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
#include <task.h>

/*
 * Returns the size of a directory (including subdirectories).
 */
long long get_size_dir(const char *path, struct task_details *task);

/*
 * Writes the analyze report to the file descriptor.
 */
long long analyzing(const char *path, struct task_details *task);

/*
 * Start the thread report.
 */
void *start_analyses_thread(void *arg);

/* Write the report info to the file descriptor of the task.
 */
void write_report_info(const char *path, long long size, struct task_details *task);

/* Check if the thread should continue or die.
 */
void check_or_exit_thread(int ok, struct task_details *task, const char *msg);


void write_report_info(FILE *output_fd, const char *path, long long size, struct task_details *task);

#endif // ANALYZER_H
