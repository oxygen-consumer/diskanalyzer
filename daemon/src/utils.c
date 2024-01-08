#include <utils.h>

#define __USE_XOPEN_EXTENDED

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int rv = remove(fpath);

    if (rv)
        perror(fpath);

    return rv;
}

int rmrf(const char *path)
{
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

void die(bool ok, const char *msg, ...)
{
    printf("%s\n", msg);
    syslog(LOG_USER | (ok ? LOG_INFO : LOG_ERR), "%s", msg);

    // TODO: Destroy all tasks

    // Remove the socket file
    char SV_SOCK_PATH[37];
    sprintf(SV_SOCK_PATH, "/var/run/user/%d/diskanalyzer.sock", getuid());
    if (remove(SV_SOCK_PATH) == -1)
    {
        syslog(LOG_USER | LOG_WARNING, "Unable to remove the socket file located at %s.", SV_SOCK_PATH);
    }

    char path[50];
    sprintf(path, "/var/run/user/%d/da_tasks", getuid());
    if (rmrf(path))
    {
        syslog(LOG_WARNING, "Failed to delete tasks directory.");
    }

    closelog();
    exit(ok ? EXIT_SUCCESS : EXIT_FAILURE);
}

void add_to_path(const char *current_path, const char *add_path, char *ans)
{
    strcpy(ans, current_path);
    if (ans[strlen(ans) - 1] != '/')
    {
        strcat(ans, "/");
    }
    strcat(ans, add_path);
}

long long fsize(const char *filename)
{
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    return 0;
}

char *relative_path(const char *path, int depth)
{
    int len = strlen(path);
    for (int i = len - 1; i >= 0; i--)
    {
        if (path[i] == '/')
        {
            depth--;
            if (depth <= 0)
            {
                return (char *)path + i;
            }
        }
    }
    return (char *)path;
}

int special_directory(char *d_name)
{
    return strcmp(d_name, ".") == 0 || strcmp(d_name, "..") == 0 || d_name[0] == '.'; // also hidden files atm
}

void syslog_message(const struct message *msg)
{
    char taskCode[20];
    switch (msg->task_code)
    {
    case NO_TASK:
        strcpy(taskCode, "NO_TASK");
        break;
    case ADD:
        strcpy(taskCode, "ADD");
        break;
    case PRIORITY:
        strcpy(taskCode, "PRIORITY");
        break;
    case SUSPEND:
        strcpy(taskCode, "SUSPEND");
        break;
    case RESUME:
        strcpy(taskCode, "RESUME");
        break;
    case REMOVE:
        strcpy(taskCode, "REMOVE");
        break;
    case INFO:
        strcpy(taskCode, "INFO");
        break;
    case LIST:
        strcpy(taskCode, "LIST");
        break;
    case PRINT:
        strcpy(taskCode, "PRINT");
        break;
    }

    char priority[20];
    switch (msg->priority)
    {
    case NO_PRIORITY:
        strcpy(priority, "NO_PRIORITY");
        break;
    case LOW:
        strcpy(priority, "LOW");
        break;
    case MEDIUM:
        strcpy(priority, "MEDIUM");
        break;
    case HIGH:
        strcpy(priority, "HIGH");
        break;
    }

    syslog(LOG_INFO, "Received - Task Code: %s, Path: %s, ID: %d, Priority: %s", taskCode, msg->path, msg->id,
           priority);
}

int get_depth_of_subpath(const char *path, const char *subpath)
{
    // Check if subpath starts with path
    if (strncmp(path, subpath, strlen(path)) != 0)
    {
        return -1; // subpath is not a subpath of path
    }

    // Get the part of subpath after path
    const char *relative_subpath = subpath + strlen(path);

    // Count the number of '/' characters in relative_subpath
    int depth = 0;
    for (const char *p = relative_subpath; *p != '\0'; p++)
    {
        if (*p == '/')
        {
            depth++;
        }
    }

    return depth;
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
            return -1;
        }
    }

    // Check if the path is a directory
    return S_ISDIR(statbuf.st_mode);
}

int get_unused_task(int used_tasks[MAX_TASKS])
{
    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (used_tasks[i] == 0)
        {
            return i;
        }
    }
    return -1;
}

int get_thread_id(int task_id, struct task_details *tasks[MAX_TASKS])
{
    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (tasks[i] != NULL && tasks[i]->task_id == task_id)
        {
            return i;
        }
    }
    return -1;
}

void send_error_response(int client_fd, enum ResponseCode response_code)
{
    struct Response response;
    response.response_code = response_code;
    response.message[0] = '\0';
    ssize_t bytes_sent = 0;

    bytes_sent = send(client_fd, &response, sizeof(response), 0);

    if (bytes_sent == -1)
    {
        syslog(LOG_ERR, "Failed to send response to client.");
        return;
    }

    syslog(LOG_INFO, "Sent response to client.");
    close(client_fd);
}

char *status_to_string(enum Status status)
{
    switch (status)
    {
    case NOT_STARTED:
        return "NOT_STARTED";
    case RUNNING:
        return "RUNNING";
    case PAUSED:
        return "SUSPENDED";
    case FINISHED:
        return "FINISHED";
    case ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

bool starts_with(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre), lenstr = strlen(str);
    return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}
