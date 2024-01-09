#include <utils.h>

#include <shared.h>

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINES 500

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
    case NOT_FINISHED_ERROR:
        return "NOT_FINISHED_ERROR";
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

int get_last_slash_index(const char *str)
{
    size_t i = strlen(str) - 1;
    while (str[i] != '/')
        i--;
    return i;
}

long long get_first_number(char *str)
{
    long long ans = 0;
    size_t i = 0;
    while (!(str[i] >= '0' && str[i] <= '9'))
        i++;
    while (str[i] >= '0' && str[i] <= '9')
    {
        ans = ans * 10 + (str[i] - '0');
        i++;
    }
    return ans;
}

long long get_nth_number(char *str, int n)
{
    if (n == 1)
    {
        return get_first_number(str);
    }
    while (!(str[0] >= '0' && str[0] <= '9'))
        str++;
    while (str[0] >= '0' && str[0] <= '9')
        str++;
    return get_nth_number(str, n - 1);
}

void print_deamon_report(const char *path)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    fp = fopen(path, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Failed to open daemon response file:  %s\n", path);
        exit(EXIT_FAILURE);
    }

    char **lines = malloc(sizeof(char *) * MAX_LINES);
    int line_count = 0;
    int pre_dirs = 0;
    int pre_files = 0;

    int a_dirs[MAX_LINES];
    int a_files[MAX_LINES];
    int a_bytes[MAX_LINES];
    int last_dir;
    int last_file;

    while ((read = getline(&line, &len, fp)) != -1)
    {
        lines[line_count] = malloc(sizeof(char) * (strlen(line) + 1));
        strcpy(lines[line_count], line);
        int bytes = get_nth_number(lines[line_count], 1);
        last_file = get_nth_number(lines[line_count], 2);
        last_dir = get_nth_number(lines[line_count], 3);
        int files = last_file - pre_files;
        int dirs = last_dir - pre_dirs;
        a_bytes[line_count] = bytes;
        a_dirs[line_count] = dirs;
        a_files[line_count] = files;
        pre_dirs += dirs;
        pre_files += files;
        line_count++;
    }
    a_files[line_count - 1] = last_file;
    a_dirs[line_count - 1] = last_dir;
    fclose(fp);
    if (line)
        free(line);

    for (int i = line_count - 1; i >= 0; i--)
    {
        int last_slash_index = get_last_slash_index(lines[i]);
        for (int j = 0; j < last_slash_index; j++)
            printf("%c", lines[i][j]);

        printf("\t\t\t | %d bytes (%0.3lf%%)\t | %d files \t| %d dirs\n", a_bytes[i],
               (double)a_bytes[i] / a_bytes[line_count - 1] * 100.0, a_files[i], a_dirs[i]);
        free(lines[i]);
    }
    free(lines);
}
