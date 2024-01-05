#include <shared.h>

#include <utils.h>

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

void usage(const char *message)
{
    if (message != NULL)
    {
        fprintf(stderr, "%s\n", message);
    }
    fprintf(stderr, "Usage: ./program [options]\n"
                    "Options:\n"
                    "  -a, --add <path>\t\t\tAnalyze directory\n"
                    "  -P, --priority <low/normal/high>\tSet priority\n"
                    "  -S, --suspend <id>\t\t\tSuspend task\n"
                    "  -R, --resume <id>\t\t\tResume task\n"
                    "  -r, --remove <id>\t\t\tRemove task\n"
                    "  -i, --info <id>\t\t\tPrint task status\n"
                    "  -l, --list\t\t\t\tList all analysis tasks\n"
                    "  -p, --print <id>\t\t\tPrint task analysis report \n");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    // Socket preparation
    // const int BUF_SIZE = 4096;

    char SV_SOCK_PATH[37];
    sprintf(SV_SOCK_PATH, "/var/run/user/%d/diskanalyzer.sock", getuid());

    struct sockaddr_un addr;
    // char buf[BUF_SIZE];
    int sfd; // server fd
    // ssize_t nread;

    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SV_SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    int option;
    int option_index = 0;
    static struct option long_options[] = {{"add", required_argument, NULL, 'a'},
                                           {"priority", required_argument, NULL, 'P'},
                                           {"suspend", required_argument, NULL, 'S'},
                                           {"resume", required_argument, NULL, 'R'},
                                           {"remove", required_argument, NULL, 'r'},
                                           {"info", required_argument, NULL, 'i'},
                                           {"list", no_argument, NULL, 'l'},
                                           {"print", required_argument, NULL, 'p'},
                                           {0, 0, 0, 0}};

    struct message msg;
    msg.task_code = NO_TASK;
    msg.path[0] = '\0';
    msg.id = -1;
    msg.priority = NO_PRIORITY;

    while ((option = getopt_long(argc, argv, "a:P:S:R:r:i:lp:", long_options, &option_index)) != -1)
    {
        if (msg.task_code != NO_TASK && option != 'P')
        {
            usage("Cannot add more than one task");
        }

        if (option == 'S' || option == 'R' || option == 'r' || option == 'i' || option == 'p') // option requires id
        {
            msg.id = get_id(optarg);
            if (msg.id < 0)
            {
                usage("Invalid ID");
            }
        }

        switch (option)
        {
        case 'a': {
            msg.task_code = ADD;

            if (strlen(optarg) > MAX_PATH_SIZE)
            {
                usage("Path too long");
            }

            if (!is_valid_directory(optarg))
            {
                usage("Invalid path");
            }

            realpath(optarg, msg.path); // convert to absolute path

            msg.priority = MEDIUM; // default priority
            break;
        }
        case 'P': {
            if (msg.task_code != ADD)
            {
                usage("Option -P requires option -a first");
            }

            msg.priority = get_priority(optarg);
            if (msg.priority == NO_PRIORITY)
            {
                usage("Invalid priority");
            }
            break;
        }
        case 'S': {
            msg.task_code = SUSPEND;

            break;
        }
        case 'R': {
            msg.task_code = RESUME;

            break;
        }
        case 'r': {
            msg.task_code = REMOVE;

            break;
        }
        case 'i': {
            msg.task_code = INFO;

            break;
        }
        case 'l': {
            msg.task_code = LIST;
            break;
        }
        case 'p': {
            msg.task_code = PRINT;

            break;
        }
        default: {
            usage("Unknown option");
            break;
        }
        }
    }

    if (msg.task_code == NO_TASK)
    {
        usage("No task specified");
    }
    ssize_t bytes_sent;
    bytes_sent = send(sfd, &msg, sizeof(msg), 0);

    if (bytes_sent == -1)
    {
        fprintf(stderr, "Failed to send message to server.\n");
        exit(EXIT_FAILURE);
    }

    /* FIXME:

     */

    struct Response response;
    response.response_code = NO_RESPONSE;
    response.message[0] = '\0';
    ssize_t bytes_received;

    // JUST FOR TESTING
    bytes_received = recv(sfd, &msg, sizeof(response), 0);
    // bytes_received = 1;
    // response.response_code = OK;
    // strcpy(response.message, "/home/sebi/Desktop/temaa/ghizi.cpp");
    // END OF TESTING
    //
    if (bytes_received <= 0)
    {
        fprintf(stderr, "Failed to receive response from server.\n");
        exit(EXIT_FAILURE);
    }

    if (response.response_code != OK)
    {
        fprintf(stderr, "Server responded with error code %s.\n", response_code_to_string(response.response_code));
        exit(EXIT_FAILURE);
    }

    switch (msg.task_code)
    {
    case ADD: {
        int id = get_id(response.message);
        if (id < 0)
        {
            fprintf(stderr, "Received invalid task ID from server\n");
            exit(EXIT_FAILURE);
        }
        printf("Created analysis task with ID \'%d\' for \'%s\' and priority \'%s\'.\n", id, msg.path,
               priority_to_string(msg.priority));

        break;
    }
    case SUSPEND: {
        printf("Suspended analysis task with ID \'%d\'.\n", msg.id);
        break;
    }
    case RESUME: {
        printf("Resumed analysis task with ID \'%d\'.\n", msg.id);
        break;
    }
    case REMOVE: {
        printf("Removed analysis task with ID \'%d\'.\n", msg.id);
        break;
    }
    case INFO: {
        printf("%s\n", response.message);
        break;
    }
    case LIST: {
        char *message = read_from_path(response.message);
        printf("%s\n", message);
        free(message);
        break;
    }
    case PRINT: {
        char *message = read_from_path(response.message);
        printf("%s\n", message);
        free(message);
        break;
    }
    default: {
        fprintf(stderr, "Something went wrong\n");
        exit(EXIT_FAILURE);
        break;
    }
    }

    return 0;
}
