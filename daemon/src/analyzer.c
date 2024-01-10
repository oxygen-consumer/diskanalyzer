#include <analyzer.h>
#include <constants.h>
#include <path_stack.h>
#include <task.h>
#include <utils.h>

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

void check_or_exit_thread(int ok, struct task_details *task, const char *msg)
{
    if (ok != 1)
    {
        syslog(LOG_ERR, "%s", msg);
        if (set_task_status(task, ERROR) != 0)
        {
            syslog(LOG_ERR, "Error setting task status to ERROR");
        }
        pthread_exit(NULL);
    }
}

void cleanup_handler(void *arg)
{
    free_path_stack(arg);
    free(arg);
}

long long analyzing(const char *path, struct task_details *task, double sub_progress)
{
    // Wait until the thread is allowed to continue
    permission_to_continue(task);

    /*
     * Check if the thread should continue or die before opening the directory so that the file descriptor is not
     * leaked.
     */
    pthread_testcancel();

    char sub_path[MAX_PATH_SIZE];
    struct stat file_stat;
    struct dirent *sub_directory;
    long long size = 0;
    DIR *directory = opendir(path);

    if (directory == NULL)
    {
        task->progress += sub_progress;
        return 0;
    }

    int dirs_count = 0;
    PathStack *stack = (PathStack *)malloc(sizeof(PathStack));
    initialize_path_stack(stack);

    pthread_cleanup_push(cleanup_handler, stack);

    while ((sub_directory = readdir(directory)) != NULL)
    {
        if (special_directory(sub_directory->d_name))
            continue;
        add_to_path(path, sub_directory->d_name, sub_path);
        if (stat(sub_path, &file_stat) != 0)
            continue;

        if (S_ISREG(file_stat.st_mode))
        {
            size += fsize(sub_path);
            task->files += 1;
        }
        else if (S_ISDIR(file_stat.st_mode))
        {
            // size += fsize(sub_path);
            push(stack, sub_path);
            dirs_count += 1;
            task->dirs += 1;
        }
    }

    check_or_exit_thread(closedir(directory) == 0, task, "Error closing directory");

    while (!is_empty(stack))
    {
        char *sub_path = pop(stack);
        size += analyzing(sub_path, task, (double)sub_progress / dirs_count);
        free(sub_path);
    }
    pthread_cleanup_pop(1);

    if (dirs_count == 0)
    {
        task->progress += sub_progress;
    }

    write_report_info(path, size, task);
    return size;
}

void *start_analyses_thread(void *arg)
{
    // Enable thread cancellation
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    struct task_details *current_task = (struct task_details *)arg;
    check_or_exit_thread(set_task_status(current_task, RUNNING) == 0, current_task,
                         "Error setting task status to RUNNING");

    check_or_exit_thread(current_task->path != NULL, current_task, "NULL path");
    check_or_exit_thread(current_task->path[0] != '\0', current_task, "Empty path");
    check_or_exit_thread(directory_exists(current_task->path) != 0, current_task, "Directory does not exist");

    long long analyzing_size = analyzing(current_task->path, current_task, 100.0);

    check_or_exit_thread(set_task_status(current_task, FINISHED) == 0, current_task,
                         "Error setting task status to FINISHED");
    // just in case
    current_task->progress = 100.0;
    return NULL;
}

void write_report_info(const char *path, long long size, struct task_details *task)
{
    int depth = get_depth_of_subpath(task->path, path);
    if (depth == 1)
    {
        fprintf(task->output_fd, "|-%s/ | %lld bytes | %d files | %d dirs\n", relative_path(path, depth),
                size, task->files, task->dirs);
    }

    if (depth == 0)
    {
        fprintf(task->output_fd, "%s/ | %lld bytes | %d files | %d dirs\n", path, size, task->files, task->dirs);
    }
}
