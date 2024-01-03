enum TaskCode
{
    ADD,
    PRIORITY,
    SUSPEND,
    RESUME,
    REMOVE,
    INFO,
    LIST,
    PRINT,
};

struct error
{
    int error_code;
    char *error_msg;
};

struct message
{
    enum TaskCode task_code; 
    char *path;
    int id, priority;
};


