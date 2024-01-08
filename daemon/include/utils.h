#ifndef DAEMON_UTILS_H
#define DAEMON_UTILS_H

#include <task.h>

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <shared.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>

#define MAX_TASKS 100

/*
 * Kill the daemon and print the message to syslog.
 * If ok is false, the message will be printed as an error,
 * or info otherwise.
 */
void die(bool ok, const char *msg, ...);

/*
 * Merge current_path and add_path in ans
 */
void add_to_path(const char *current_path, const char *add_path, char *ans);

/*
 * Get size of an file/directory
 * returns 0 if something went wrong
 */
long long fsize(const char *filename);

/*
 * Return a substring of the input path, starting from the depth-th '/' character from the end of the string.
 */
char *relative_path(const char *path, int depth);

/*
 * Directories "." and ".." are typically special entries in a
 * directory, and we want to skip them to avoid infinite loops.
 */
int special_directory(char *d_name);

/*
 * Returns the depth of the subpath in the path or -1 if the subpath is not in the path.
 */
int get_depth_of_subpath(const char *path, const char *subpath);

int get_unused_task(int used_tasks[MAX_TASKS]);

int get_thread_id(int task_id, struct task_details *task[MAX_TASKS]);

void send_error_response(int client_fd, enum ResponseCode);

char *status_to_string(enum Status status);

/*
 * Check if the directory exists, returns 1 if exits.
 */
int directory_exists(const char *path);

/*
 * Check if str starts with pre.
 * Return true if it does, false otherwise.
 */
bool starts_with(const char *pre, const char *str);

#endif
