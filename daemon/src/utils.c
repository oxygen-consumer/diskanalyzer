#include <utils.h>

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
#include <unistd.h>

void die(bool ok, const char *msg, ...)
{
    printf("%s\n", msg);
    syslog(LOG_USER | (ok ? LOG_INFO : LOG_ERR), "%s", msg);

    // Remove the socket file
    char SV_SOCK_PATH[37];
    sprintf(SV_SOCK_PATH, "/var/run/user/%d/diskanalyzer.sock", getuid());
    if (remove(SV_SOCK_PATH) == -1)
    {
        syslog(LOG_USER | LOG_WARNING, "Unable to remove the socket file located at %s.", SV_SOCK_PATH);
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

    syslog(LOG_INFO, "Received - Task Code: %s, Path: %s, ID: %d, Priority: %s", taskCode, msg->path, msg->id, priority);
}
