

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <task.h>
#include <utils.h>

void output_task(struct task_details *task)
{
    syslog(LOG_INFO, "-----------------");
    syslog(LOG_INFO, "Task id: %d", task->task_id);
    syslog(LOG_INFO, "Task status: %s", status_to_string(task->status));
    syslog(LOG_INFO, "Task priority: %d", task->priority);
    syslog(LOG_INFO, "Task files: %d", task->files);
    syslog(LOG_INFO, "Task dirs: %d", task->dirs);
    syslog(LOG_INFO, "Task path: %s", task->path);
    syslog(LOG_INFO, "Task total_size: %lld", task->total_size);
    syslog(LOG_INFO, "-----------------");
}

void permission_to_continue(struct task_details *task)
{
    pthread_mutex_lock(task->permission_mutex);
    pthread_mutex_unlock(task->permission_mutex);
}

FILE *get_output_fd(int id)
{
    char thread_output[MAX_PATH_SIZE];
    snprintf(thread_output, MAX_PATH_SIZE, PATH_TO_ANALYZE, getuid(), id);
    FILE *output_fd = fopen(thread_output, "w");
    if (output_fd == NULL)
    {
        syslog(LOG_ERR, "Error opening file %s: %s", thread_output, strerror(errno));
        return NULL;
    }
    return output_fd;
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

    task->output_fd = get_output_fd(id);

    if (task->output_fd == NULL)
    {
        perror("Error opening file");
        return NULL;
    }

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
    task = NULL;
}

void suspend_task(struct task_details *task)
{
    pthread_mutex_lock(task->permission_mutex);
    task->status = PAUSED;

    fclose(task->output_fd);
    syslog(LOG_INFO, "Task %d suspended.", task->task_id);
}

void resume_task(struct task_details *task)
{
    task->output_fd = get_output_fd(task->task_id);
    if (task->output_fd == NULL)
    {
        syslog("Error opening file when resume");
        exit(errno); // if we couldn't open the file, we can't continue
    }

    task->status = RUNNING;
    pthread_mutex_unlock(task->permission_mutex);
    syslog(LOG_INFO, "Task %d resumed.", task->task_id);
}

void finish_task(struct task_details *task)
{
    task->status = FINISHED;
    fclose(task->output_fd);

    syslog(LOG_INFO, "Task %d finished.", task->task_id);
}

void set_task_status(struct task_details *task, enum Status status)
{
    pthread_mutex_lock(task->status_mutex);
    syslog(LOG_INFO, "Task %d with status %s.", task->task_id, status_to_string(status));
    // output_task(task);

    // TODO: check if status change is valid
    switch (status)
    {
    case NOT_STARTED:
        syslog(LOG_ERR, "Cannot go back in time.");
        break;
    case RUNNING:
        if (task->status == PAUSED)
        {
            resume_task(task);
        }
        else if (task->status == NOT_STARTED)
        {
            task->status = RUNNING;
            syslog(LOG_ERR, "Changed status to RUNNING");
        }
        else
        {
            syslog(LOG_ERR, "Cannot resume a task that is not paused or not started.");
        }
        break;
    case PAUSED:
        if (task->status == RUNNING)
        {
            suspend_task(task);
        }
        else
        {
            syslog(LOG_ERR, "Cannot suspend a task that is not running.");
        }
        break;
    case FINISHED:
        if (task->status == RUNNING)
        {
            finish_task(task);
        }
        else
        {
            syslog(LOG_ERR, "Cannot finish a task that is not running.");
        }
        break;
    case ERROR:
        task->status = ERROR;
        syslog(LOG_ERR, "Task %d with status ERROR.", task->task_id);
        break;
    default:
        syslog(LOG_ERR, "Invalid status change.");
        break;
    }
    output_task(task);
    pthread_mutex_unlock(task->status_mutex);
}

int remove_task(struct task_details *task, pthread_t *thread)
{
    if (task == NULL)
        return -1;
    int ok = 0;
    pthread_mutex_lock(task->status_mutex);
    if (task->status == FINISHED)
    {
        if (pthread_join(*thread, NULL) != 0)
        {
            syslog(LOG_ERR, "Error joining thread.");
            ok = -1;
        }
    }
    else
    {
        syslog(LOG_ERR, "Cannot remove a task that is not finished.");
        ok = -1;
    }
    pthread_mutex_unlock(task->status_mutex);

    if (ok == 0)
        destroy_task(task);
    return ok;
}
