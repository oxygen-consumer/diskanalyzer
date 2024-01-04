#ifndef SHARED_H
#define SHARED_H

#include <constants.h>

enum TaskCode
{
    NO_TASK,
    ADD,
    PRIORITY,
    SUSPEND,
    RESUME,
    REMOVE,
    INFO,
    LIST,
    PRINT,
};

enum Priority
{
    NO_PRIORITY,
    LOW,
    MEDIUM,
    HIGH,
};

struct error
{
    int error_code;
    char error_msg[MAX_ERROR_MSG_SIZE];
};

struct message
{
    enum TaskCode task_code;
    char path[MAX_PATH_SIZE];
    int id;
    enum Priority priority;
};

enum ResponseCode
{
    OK,
    DIRECTOR_ALREADY_TRACKED_ERROR, // cannot add task for a directory that is already tracked by another task
    INVALID_ID_ERROR,               // there is not task with such id
    TASK_ALREADY_FINISHED_ERROR,    // if task is finished, it can't be suspended/resumed
    TASK_ALREADY_SUSPENDED_ERROR,   // cannot suspend task that is already suspended
    TASK_ALREADY_RUNNING_ERROR,     // cannot resume task that is already running
    NO_TASK_DONE_ERROR,             // cannot print analysis report for a task that is not done
    GENERAL_ERROR,                  // really bad things happened
};

struct response
{
    enum ResponseCode response_code;
    char message[MAX_PATH_SIZE];
    // -a => (message=to_string(task_id), OK) | DIRECTOR_ALREADY_TRACKED_ERROR | GENERAL_ERROR
    // -S => OK | INVALID_ID_ERROR | TASK_ALREADY_FINISHED_ERROR | TASK_ALREADY_SUSPENDED_ERROR | GENERAL_ERROR
    // -R => OK | INVALID_ID_ERROR | TASK_ALREADY_FINISHED_ERROR | TASK_ALREADY_RUNNING_ERROR | GENERAL_ERROR
    // -r => OK | INVALID_ID_ERROR | GENERAL_ERROR
    // -i => (message=info_message, OK) | INVALID_ID_ERROR | GENERAL_ERROR
    // -l => (message=PATH_TO_message,OK) | GENERAL_ERROR
    // -p => (message=PATH_TO_message, OK) | NO_TASK_DONE_ERROR | GENERAL_ERROR
};

#endif
