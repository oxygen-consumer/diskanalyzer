#include <utils.h>

#include <shared.h>

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int get_id(const char *str)
{
    char *endptr;
    int result = (int)strtol(str, &endptr, 10);
    if (*endptr != '\0')
    {
        return -1;
    }
    return result;
}

bool is_valid_directory(const char *path)
{
    DIR *dir = opendir(path);
    if (dir)
    {
        closedir(dir);
        return true; // Valid directory
    }
    else
    {
        return false; // Not a directory or doesn't exist
    }
}

enum Priority get_priority(const char *str)
{
    if (strcmp(str, "low") == 0)
    {
        return LOW;
    }
    else if (strcmp(str, "normal") == 0)
    {
        return MEDIUM;
    }
    else if (strcmp(str, "high") == 0)
    {
        return HIGH;
    }
    else
    {
        return NO_PRIORITY;
    }
}

char *priority_to_string(enum Priority priority)
{
    switch (priority)
    {
    case LOW:
        return "low";
    case MEDIUM:
        return "normal";
    case HIGH:
        return "high";
    default:
        return "unknown";
    }
}

char *response_code_to_string(enum ResponseCode code)
{
    switch (code)
    {
    case OK:
        return "OK";
    case DIRECTOR_ALREADY_TRACKED_ERROR:
        return "DIRECTOR_ALREADY_TRACKED_ERROR";
    case INVALID_ID_ERROR:
        return "INVALID_ID_ERROR";
    case TASK_ALREADY_FINISHED_ERROR:
        return "TASK_ALREADY_FINISHED_ERROR";
    case TASK_ALREADY_SUSPENDED_ERROR:
        return "TASK_ALREADY_SUSPENDED_ERROR";
    case TASK_ALREADY_RUNNING_ERROR:
        return "TASK_ALREADY_RUNNING_ERROR";
    case NO_TASK_DONE_ERROR:
        return "NO_TASK_DONE_ERROR";
    case GENERAL_ERROR:
        return "GENERAL_ERROR";
    case NO_RESPONSE:
        return "NO_RESPONSE";
    default:
        return "UNKNOWN";
    }
}

char *read_from_path(const char *path)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int total_read = 0;
    fp = fopen(path, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Failed to open file %s\n", path);
        exit(EXIT_FAILURE);
    }

    char *result = malloc(sizeof(char) * MAX_MESSAGE_SIZE);
    result[0] = '\0';
    while ((read = getline(&line, &len, fp)) != -1)
    {
        total_read += read;
        if (total_read > MAX_MESSAGE_SIZE)
        {
            fprintf(stderr, "Message at %s is too big\n", path);
            exit(EXIT_FAILURE);
        }
        strcat(result, line);
    }

    fclose(fp);
    if (line)
        free(line);

    return result;
}

void print_deamon_report(const char *path)
{
}