#include <analyzer.h>

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
#include <utils.h>

void output_task(struct task_details *task)
{
    printf("Task id: %d\n", task->task_id);
    printf("Task status: %d\n", task->status);
    printf("Task priority: %d\n", task->priority);
    printf("Task files: %d\n", task->files);
    printf("Task dirs: %d\n", task->dirs);
    printf("Task path: %s\n", task->path);
    printf("Task total_size: %lld\n", task->total_size);
}

void permission_to_continue(struct task_details *task)
{
    pthread_mutex_lock(task->permission_mutex);
    pthread_mutex_unlock(task->permission_mutex);
}

void set_task_status(struct task_details *task, int status)
{
    pthread_mutex_lock(task->status_mutex);
    printf("Task %d with status %d.\n", task->task_id, status);
    task->status = status;
    pthread_mutex_unlock(task->status_mutex);
}

// returns the size of a directory (including subdirectories)
long long get_size_dir(const char *path, struct task_details *task)
{
    permission_to_continue(task);

    DIR *directory = opendir(path);
    if (directory == NULL)
        return 0;

    char sub_path[MAX_PATH_LENGTH] = "";

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
    closedir(directory);
    return size;
}

long long analyzing(const char *path, struct task_details *task, FILE *output_fd)
{
    permission_to_continue(task);

    char sub_path[MAX_PATH_LENGTH];
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
            task->files += 1;
            size += fsize(sub_path);
        }
        else if (S_ISDIR(file_stat.st_mode))
        {
            task->dirs += 1;
            size += fsize(sub_path);
            size += analyzing(sub_path, task, output_fd);
        }
    }
    closedir(directory);
    write_report_info(output_fd, path, size, task);
    return size;
}

void *start_analyses_thread(void *arg)
{
    struct task_details *current_task = (struct task_details *)arg;
    char thread_output[MAX_PATH_LENGTH];

    snprintf(thread_output, MAX_PATH_LENGTH, "/tmp/aux%d.txt", current_task->task_id);
    FILE *output_fd = fopen(thread_output, "w");
    if (output_fd == NULL)
    {
        perror("Error opening output file");
        return NULL;
    }

    assert(current_task->path != NULL && current_task->path[0] != '\0');
    current_task->total_size = get_size_dir(current_task->path, current_task);
    assert(current_task->total_size > 0);

    long long analyzing_size = analyzing(current_task->path, current_task, output_fd);

    assert(current_task->total_size == analyzing_size);
    set_task_status(current_task, 1);
    return NULL;
}
struct task_details *init_task(int id)
{
    struct task_details *task = (struct task_details *)malloc(sizeof(struct task_details));
    task->task_id = id;
    task->permission_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    task->status_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));

    if (task->permission_mutex == NULL || task->status_mutex == NULL)
    {
        perror("Mutex allocation failed");
        exit(1);
    }

    if (pthread_mutex_init(task->permission_mutex, NULL) != 0)
    {
        perror("Mutex init failed");
        exit(1);
    }

    if (pthread_mutex_init(task->status_mutex, NULL) != 0)
    {
        perror("Mutex init failed");
        exit(1);
    }
    return task;
}

void destroy_task(struct task_details *task)
{
    pthread_mutex_destroy(task->permission_mutex);
    pthread_mutex_destroy(task->status_mutex);
    free(task->permission_mutex);
    free(task->status_mutex);
    free(task);
}

void suspend_task(struct task_details *task)
{
    pthread_mutex_lock(task->permission_mutex);
}

void resume_task(struct task_details *task)
{
    pthread_mutex_unlock(task->permission_mutex);
}

void write_report_info(FILE *output_fd, const char *path, long long size, struct task_details *task)
{
    int depth = get_depth(task->path, path);
    if (depth > 0 && depth <= 2)
    {
        fprintf(output_fd, "|-%s %0.2lf%% %lld\n", relative_path(path, depth), ((double)size / task->total_size) * 100,
                size);
    }
    if (depth == 1)
    {
        fprintf(output_fd, "|\n");
    }
    if (depth == 0)
    {
        fprintf(output_fd, "%s %0.2lf%% %lld\n", path, ((double)size / task->total_size) * 100, size);
    }
}
