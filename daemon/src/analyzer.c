#include <analyzer.h>
#include <constants.h>
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
        perror(msg);
        set_task_status(task, ERROR);
        pthread_exit(NULL);
    }
}

void set_task_status(struct task_details *task, enum Status status)
{
    pthread_mutex_lock(task->status_mutex);
    task->status = status;
    syslog(LOG_INFO, "Task %d changed status to %d.", task->task_id, status);
    pthread_mutex_unlock(task->status_mutex);
}

long long get_size_dir(const char *path, struct task_details *task)
{
    permission_to_continue(task);
    DIR *directory = opendir(path);

    if (directory == NULL)
        return 0;

    char sub_path[MAX_PATH_SIZE] = "";

    struct dirent *sub_directory;
    long long size = 0;
    while ((sub_directory = readdir(directory)) != NULL)
    {
        if (special_directory(sub_directory->d_name))
            continue;

        add_to_path(path, sub_directory->d_name, sub_path);
        size += fsize(sub_path);
        size += get_size_dir(sub_path, task);
    }
    check_or_exit_thread(closedir(directory) == 0, task, "Error closing directory");
    return size;
}

long long analyzing(const char *path, struct task_details *task, FILE *output_fd)
{
    permission_to_continue(task);

    char sub_path[MAX_PATH_SIZE];
    struct stat file_stat;
    struct dirent *sub_directory;
    long long size = 0;
    DIR *directory = opendir(path);

    if (directory == NULL)
        return 0;

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
            size += fsize(sub_path);
            size += analyzing(sub_path, task, output_fd);
            task->dirs += 1;
        }
    }
    check_or_exit_thread(closedir(directory) == 0, task, "Error closing directory");
    write_report_info(output_fd, path, size, task);
    return size;
}

void *start_analyses_thread(void *arg)
{
    struct task_details *current_task = (struct task_details *)arg;
    set_task_status(current_task, RUNNING);

    check_or_exit_thread(current_task->path != NULL, current_task, "NULL path");
    check_or_exit_thread(current_task->path[0] != '\0', current_task, "Empty path");
    check_or_exit_thread(directory_exists(current_task->path) != 0, current_task, "Directory does not exist");

    current_task->total_size = get_size_dir(current_task->path, current_task);
    check_or_exit_thread(current_task->total_size != 0, current_task, "Total size = 0");

    char thread_output[MAX_PATH_SIZE];
    snprintf(thread_output, MAX_PATH_SIZE, "/var/run/user/%d/da_tasks/tree_%d", getuid(), current_task->task_id);
    FILE *output_fd = fopen(thread_output, "w");
    check_or_exit_thread(output_fd != NULL, current_task, "Error opening output file");

    long long analyzing_size = analyzing(current_task->path, current_task, output_fd);
    fclose(output_fd);
    check_or_exit_thread(analyzing_size == current_task->total_size, current_task, "Different sizes");
    set_task_status(current_task, FINISHED);
    return NULL;
}

int directory_exists(const char *path)
{
    struct stat statbuf;

    if (stat(path, &statbuf) != 0)
    {
        // Check if the error was ENOENT ("No such file or directory")
        if (errno == ENOENT)
        {
            return 0; // The directory does not exist
        }
        else
        {
            perror("stat");
            exit(1); // An error occurred
        }
    }

    // Check if the path is a directory
    return S_ISDIR(statbuf.st_mode);
}

void write_report_info(FILE *output_fd, const char *path, long long size, struct task_details *task)
{
    int depth = get_depth(task->path, path);
    if (depth > 0 && depth <= 2)
    {
        fprintf(output_fd, "|-%s | %0.2lf%% | %lld bytes | %d files | %d dirs\n", relative_path(path, depth),
                ((double)size / task->total_size) * 100, size, task->files, task->dirs);
    }
    if (depth == 1)
    {
        fprintf(output_fd, "|\n");
    }
    if (depth == 0)
    {
        fprintf(output_fd, "%s | %0.2lf%% | %lld bytes | %d files | %d dirs\n", path,
                ((double)size / task->total_size) * 100, size, task->files, task->dirs);
    }
}
