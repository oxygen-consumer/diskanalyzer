#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <task.h>

void permission_to_continue(struct task_details *task)
{
    pthread_mutex_lock(task->permission_mutex);
    pthread_mutex_unlock(task->permission_mutex);
}

struct task_details *init_task(int id, char *path, enum Priority priority)
{
    struct task_details *task = (struct task_details *)malloc(sizeof(struct task_details));
    task->task_id = id;
    strcpy(task->path, path);
    task->priority = priority;
    task->status = NOT_STARTED;
    task->dirs = 0;
    task->files = 0;
    task->total_size = 0;

    task->permission_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    task->status_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));

    if (task->permission_mutex == NULL || task->status_mutex == NULL)
    {
        perror("Mutex allocation failed");
        return NULL;
    }

    if (pthread_mutex_init(task->permission_mutex, NULL) != 0)
    {
        perror("Mutex init failed");
        return NULL;
    }

    if (pthread_mutex_init(task->status_mutex, NULL) != 0)
    {
        perror("Mutex init failed");
        return NULL;
    }
    syslog(LOG_INFO, "Task %d initialized.", task->task_id);
    return task;
}

void destroy_task(struct task_details *task)
{
    syslog(LOG_INFO, "Task %d to be destroyed.", task->task_id);
    pthread_mutex_destroy(task->permission_mutex);
    pthread_mutex_destroy(task->status_mutex);
    free(task->permission_mutex);
    free(task->status_mutex);
    free(task);
}

void suspend_task(struct task_details *task)
{
    pthread_mutex_lock(task->permission_mutex);
    syslog(LOG_INFO, "Task %d suspended.", task->task_id);
}

void resume_task(struct task_details *task)
{
    pthread_mutex_unlock(task->permission_mutex);
    syslog(LOG_INFO, "Task %d resumed.", task->task_id);
}
