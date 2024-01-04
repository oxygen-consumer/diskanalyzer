#ifndef DAEMON_UTILS_H
#define DAEMON_UTILS_H

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>

/*
 * Kill the daemon and print the message to syslog.
 * If ok is false, the message will be printed as an error,
 * or info otherwise.
 */
void die(bool ok, const char *msg, ...);

/* Merge current_path and add_path in ans
 */
void add_to_path(const char *current_path, const char *add_path, char *ans);

// need to send signal to daemon to start low priority task
// void notify_task_done(int task_id)
// {
//     syslog(LOG_NOTICE, "Job %d done\n", task_id);
//     // set_task_status(task_id,DONE);
//     // priority_compute(); // check if we need to work on less important tasks
// }

/* get size of an file/directory
 * returns 0 if something went wrong
 */
long long fsize(const char *filename);

/* Return a substring of the input path, starting from the depth-th '/' character from the end of the string.
 */
char *relative_path(const char *path, int depth);

/*  Directories "." and ".." are typically special entries in a
 * directory, and we want to skip them to avoid infinite loops.
 */
int special_directory(char *d_name);

int get_depth(const char *path, const char *subpath);

#endif
