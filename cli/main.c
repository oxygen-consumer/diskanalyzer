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
    fprintf(stderr, "Usage: ./program [options]\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -a, --add <path>\tAnalyze directory\n");
    fprintf(stderr, "  -P, --priority <low/normal/high>\tSet priority\n");
    fprintf(stderr, "  -S, --suspend <id>\tSuspend task with ID\n");
    fprintf(stderr, "  -R, --resume <id>\tResume task with ID\n");
    fprintf(stderr, "  -r, --remove <id>\tRemove analysis with ID\n");
    fprintf(stderr, "  -i, --info <id>\tPrint status for analysis with ID\n");
    fprintf(stderr, "  -l, --list\t\tList all analysis tasks\n");
    fprintf(stderr, "  -p, --print <id>\tPrint analysis report for task with ID\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    /*
     * FIXME: Send a test message to the server to test the connection.
     */

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

        switch (option)
        {
        case 'a': {
            msg.task_code = ADD;

            if (strlen(optarg) > 255)
            {
                usage("Path too long");
            }

            strcpy(msg.path, optarg);
            msg.priority = MEDIUM;
            break;
        }
        case 'P': {
            if (msg.task_code != ADD)
            {
                usage("Option -P requires option -a first");
            }

            if (strcmp(optarg, "low") == 0)
            {
                msg.priority = LOW;
            }
            else if (strcmp(optarg, "normal") == 0)
            {
                msg.priority = MEDIUM;
            }
            else if (strcmp(optarg, "high") == 0)
            {
                msg.priority = HIGH;
            }
            else
            {
                usage("Invalid priority");
            }
            break;
        }
        case 'S': {
            msg.task_code = SUSPEND;

            msg.id = stringToInt(optarg);
            if (msg.id < 0)
            {
                usage("Invalid ID");
            }
            break;
        }
        case 'R': {
            msg.task_code = RESUME;

            msg.id = stringToInt(optarg);
            if (msg.id < 0)
            {
                usage("Invalid ID");
            }
            break;
        }
        case 'r': {
            msg.task_code = REMOVE;

            msg.id = stringToInt(optarg);
            if (msg.id < 0)
            {
                usage("Invalid ID");
            }
            break;
        }
        case 'i': {
            msg.task_code = INFO;

            msg.id = stringToInt(optarg);
            if (msg.id < 0)
            {
                usage("Invalid ID");
            }
            break;
        }
        case 'l': {
            msg.task_code = LIST;
            break;
        }
        case 'p': {
            msg.task_code = PRINT;

            msg.id = stringToInt(optarg);
            if (msg.id < 0)
            {
                usage("Invalid ID");
            }
            break;
        }
        default: {
            usage("Unknown option");
            break;
        }
        }
    }

    return 0;
}
