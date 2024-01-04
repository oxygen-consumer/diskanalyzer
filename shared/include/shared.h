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
