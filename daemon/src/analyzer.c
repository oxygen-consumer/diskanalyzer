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
    pthread_mutex_lock(&permission_mutex[id]);
    pthread_mutex_unlock(&permission_mutex[id]);
}

void set_task_status(int id, int status)
{
    pthread_mutex_lock(&status_mutex[id]);
    printf("Task %d with status %d.\n", id, status);
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

long long analyzing(const char *path, int task_id, FILE *output_fd)
{
    permission_to_continue(task_id);

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
            task[task_id]->files += 1;
            size += fsize(sub_path);
        }
        else if (S_ISDIR(file_stat.st_mode))
        {
            task[task_id]->dirs += 1;
            size += fsize(sub_path);
            size += analyzing(sub_path, task_id, output_fd);
        }
    }
    closedir(directory);
    write_report_info(output_fd, path, size, task_id);
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

    long long analyzing_size = analyzing(current_task->path, current_task->task_id, output_fd);

    assert(current_task->total_size == analyzing_size);
    set_task_status(current_task->task_id, 1);
    return NULL;
}
void init_taks()
{
    for (int i = 0; i < MAX_TASKS; i++)
    {
        task[i] = (struct task_details *)malloc(sizeof(struct task_details));
        task[i]->task_id = i;

        if (pthread_mutex_init(&permission_mutex[i], NULL) != 0)
        {
            perror("Mutex init failed");
            exit(1);
        }

        if (pthread_mutex_init(&status_mutex[i], NULL) != 0)
        {
            perror("Mutex init failed");
            exit(1);
        }
    }
}

void destroy_tasks()
{
    for (int i = 0; i < MAX_TASKS; i++)
    {
        free(task[i]);
        pthread_mutex_destroy(&permission_mutex[i]);
        pthread_mutex_destroy(&status_mutex[i]);
    }
}

void suspend_task(int id)
{
    pthread_mutex_lock(&permission_mutex[id]);
}

void resume_task(int id)
{
    pthread_mutex_unlock(&permission_mutex[id]);
}