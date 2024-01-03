#include <daemonize.h>
#include <shared.h>
#include <utils.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <syslog.h>
#include <unistd.h>

int main(void)
{
    // Connect to system log
    const char *LOG_NAME = "diskanalyzer";
    openlog(LOG_NAME, LOG_PID, LOG_USER);

    // Make the process a daemon
    if (daemonize())
    {
        die(false, "Unable to start daemon. Exiting.");
    }

    // Socket preparation
    const int BACKLOG = 16, BUF_SIZE = 4096;

    char SV_SOCK_PATH[37];
    sprintf(SV_SOCK_PATH, "/var/run/user/%d/diskanalyzer.sock", getuid());

    struct sockaddr_un addr;
    char buf[BUF_SIZE];
    int sfd, cfd; // server fd, client fd
    ssize_t nread;

    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1)
    {
        die(false, "Unable to open socket. Exiting.");
    }

    // Open the socket and bind it
    if (remove(SV_SOCK_PATH) == -1 && errno != ENOENT)
    {
        die(false, "Could not open socket at %s. Exiting.", SV_SOCK_PATH);
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SV_SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1)
    {
        die(false, "Could not bind socket. Exiting.");
    }

    if (listen(sfd, BACKLOG) == -1)
    {
        die(false, "Unable to listen on socket. Exiting.");
    }

    // Everything ready
    syslog(LOG_USER | LOG_INFO, "Diskanalyzer daemon ready.");

    struct message msg;
    ssize_t bytes_received;

    while (true)
    {
        // Handle every incomming connection
        for (;;)
        {
            cfd = accept(sfd, NULL, NULL);
            if (cfd == -1)
            {
                syslog(LOG_USER | LOG_WARNING, "Could not accept connection.");
                continue;
            }

            /*
             * FIXME: For now we print everything to syslog,
             * for test purposes.
             * We will need a function to process them accordingly.
             */
            bytes_received = recv(sfd, &msg, sizeof(msg), 0);

            if (bytes_received <= 0)
            {
                syslog(LOG_USER | LOG_WARNING, "Failed to read from socket. bytes_received = %ld", bytes_received);
            }
            else
            {
                syslog(LOG_USER | LOG_INFO, "Received: %s", msg.path);
            }
            // while ((nread = read(cfd, buf, BUF_SIZE)) > 0)
            // {
            //     syslog(LOG_USER | LOG_INFO, "Received: %s", buf);
            // }

            // if (nread == -1)
            // {
            //     syslog(LOG_USER | LOG_WARNING, "Failed to read from socket.");
            // }

            // if (close(cfd) == -1)
            // {
            //     syslog(LOG_USER | LOG_WARNING, "Failed to close connetion.");
            // }
        }
    }

    /*
     * We should never reach this point.
     */

    die(false, "Something really bad happened. Exiting.");
}
