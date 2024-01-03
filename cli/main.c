#include <shared.h>

#include <getopt.h>
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
    fprintf(stderr, "  -a, --add <arg>\tAnalyze directory\n");
    fprintf(stderr, "  -p, --priority <arg>\tSet priority\n");
    fprintf(stderr, "  -S, --suspend <arg>\tSuspend task with ID\n");
    fprintf(stderr, "  -R, --resume <arg>\tResume task with ID\n");
    fprintf(stderr, "  -r, --remove <arg>\tRemove analysis with ID\n");
    fprintf(stderr, "  -i, --info <arg>\tPrint status for analysis with ID\n");
    fprintf(stderr, "  -l, --list\t\tList all analysis tasks\n");
    fprintf(stderr, "  -p, --print <arg>\tPrint analysis report for task with ID\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    /*
     * FIXME: Send a test message to the server to test the connection.
     */

    // Socket preparation
    const int BUF_SIZE = 4096;

    char SV_SOCK_PATH[37];
    sprintf(SV_SOCK_PATH, "/var/run/user/%d/diskanalyzer.sock", getuid());

    struct sockaddr_un addr;
    char buf[BUF_SIZE];
    int sfd; // server fd
    ssize_t nread;

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
                                           {"priority", required_argument, NULL, 'p'},
                                           {"suspend", required_argument, NULL, 'S'},
                                           {"resume", required_argument, NULL, 'R'},
                                           {"remove", required_argument, NULL, 'r'},
                                           {"info", required_argument, NULL, 'i'},
                                           {"list", no_argument, NULL, 'l'},
                                           {"print", required_argument, NULL, 'p'},
                                           {0, 0, 0, 0}};

    while ((option = getopt_long(argc, argv, "a:p:S:R:r:i:l", long_options, &option_index)) != -1)
    {
        switch (option)
        {
        case 'a': {
            printf("Analyze directory: %s\n", optarg);
            break;
        }
        case 'p': {
            printf("Set priority: %s\n", optarg);
            break;
        }
        case 'S': {
            printf("Suspend task with ID: %s\n", optarg);
            break;
        }
        case 'R': {
            printf("Resume task with ID: %s\n", optarg);
            break;
        }
        case 'r': {
            printf("Remove analysis with ID: %s\n", optarg);
            break;
        }
        case 'i': {
            printf("Print status for analysis with ID: %s\n", optarg);
            break;
        }
        case 'l': {
            printf("List all analysis tasks\n");
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
