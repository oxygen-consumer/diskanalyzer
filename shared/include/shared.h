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
    char error_msg[256];
};

struct message
{
    enum TaskCode task_code;
    char path[256];
    int id;
    enum Priority priority;
};
