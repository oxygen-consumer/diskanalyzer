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
    NO_TASK_DONE_ERROR,             // cannot list analysis reports if no task is finished
    GENERAL_ERROR,                  // really bad things happened
    NOT_FINISHED_ERROR,             // cannot print analysis report for a task that is not finished
    NO_RESPONSE,                    // used as default value
};

struct Response
{
    enum ResponseCode response_code;
    char message[MAX_PATH_SIZE];
    // -a => (message=to_string(task_id), OK) | DIRECTOR_ALREADY_TRACKED_ERROR | GENERAL_ERROR
    //    => e.g. message = "1\0" ; please note the null terminator
    // -S => OK | INVALID_ID_ERROR | TASK_ALREADY_FINISHED_ERROR | TASK_ALREADY_SUSPENDED_ERROR | GENERAL_ERROR
    // -R => OK | INVALID_ID_ERROR | TASK_ALREADY_FINISHED_ERROR | TASK_ALREADY_RUNNING_ERROR | GENERAL_ERROR
    // -r => OK | INVALID_ID_ERROR | GENERAL_ERROR
    // -i => (message=info_message, OK) | INVALID_ID_ERROR | GENERAL_ERROR
    //    => e.g. message = "ID: 1\nPath: /home/user/Downloads\nPriority: normal\nStatus: running\nProgress: 75%: 69
    //    dirs, 420 files\0" ; please note the null terminator
    // -l => (message=PATH_TO_message,OK) | GENERAL_ERROR
    //    => e.g. message = "/tmp/analysis_report_1.txt\0" ; please note the null terminator
    // -p => (message=PATH_TO_message, OK) | NOT_FINISHED_ERROR | GENERAL_ERROR
};

enum Status
{
    NOT_STARTED,
    RUNNING,
    PAUSED,
    FINISHED,
    ERROR,
};

#endif
