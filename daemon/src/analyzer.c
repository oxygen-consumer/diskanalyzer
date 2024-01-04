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

void permission_to_continue(int id)
{
    return;
    // pthread_mutex_lock(&permission_mutex[id]);
    // pthread_mutex_unlock(&permission_mutex[id]);
}

void set_task_status(int id, int status)
{
    pthread_mutex_lock(&status_mutex[id]);
    task[id]->status = status;
    pthread_mutex_unlock(&status_mutex[id]);
}

// returns the size of a directory (including subdirectories)
long long get_size_dir(const char *path, int task_id)
{
    permission_to_continue(task_id);

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
        size += get_size_dir(sub_path, task_id);
    }
    closedir(directory);
    return size;
}

long long analyzing(const char *path, int task_id, int depth, FILE *output_fd)
{
    permission_to_continue(task_id);

    char sub_path[MAX_PATH_LENGTH];
    struct stat file_stat;
    struct dirent *sub_directory;
    long long size = 0;
    DIR *directory = opendir(path);

    if (directory == NULL)
        return 0;

    printf("Analyzing %s\n", path); // debug

    while ((sub_directory = readdir(directory)) != NULL)
    {
        if (special_directory(sub_directory->d_name))
            continue;
        add_to_path(path, sub_directory->d_name, sub_path);
        if (stat(sub_path, &file_stat) != 0)
            continue;

        if (S_ISREG(file_stat.st_mode))
        {
            task[task_id]->files += 1;
            size += fsize(sub_path);
        }
        else if (S_ISDIR(file_stat.st_mode))
        {
            size += fsize(sub_path);
            size += analyzing(sub_path, task_id, depth + 1, output_fd);
        }
    }
    closedir(directory);
    print_path_info(depth, path, task[task_id]->total_size, size, output_fd);
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
    current_task->total_size = get_size_dir(current_task->path, current_task->task_id);
    assert(current_task->total_size > 0);

    long long analyzing_size = analyzing(current_task->path, current_task->task_id, 0, output_fd);

    assert(current_task->total_size == analyzing_size);

    fprintf(output_fd, "|-%s       100%%         %lld\n", current_task->path, analyzing_size);
    set_task_status(current_task->task_id, 1);
    return NULL;
}