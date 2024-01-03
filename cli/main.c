#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main(void)
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

    while ((nread = read(STDIN_FILENO, buf, BUF_SIZE)) > 0)
    {
        if (write(sfd, buf, nread) != nread)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
