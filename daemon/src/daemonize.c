#include <daemonize.h>

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

unsigned short daemonize(void)
{
    // Become background process
    switch (fork())
    {
    case -1:
        return 1;

    case 0:
        break;

    default:
        exit(EXIT_SUCCESS);
    }

    // Become leader of new session
    if (setsid() == -1)
    {
        return 1;
    }

    // Transfer parentship to INIT
    switch (fork())
    {
    case -1:
        return 1;

    case 0:
        break;

    default:
        exit(EXIT_SUCCESS);
    }

    // Clear file creation mask
    umask(0);

    // Change to root directory
    chdir("/");

    // Close all open files
    {
        size_t maxfd = sysconf(_SC_OPEN_MAX);
        for (size_t fd = 0; fd < maxfd; ++fd)
        {
            close(fd);
        }
    }

    // Bravo six, going dark
    {
        close(STDIN_FILENO);

        size_t fd = open("/dev/null", O_RDWR);
        if (fd != STDIN_FILENO)
            return 1;
        if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
            return 2;
        if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
            return 3;
    }

    // Bind signals
    signal(SIGTERM, signal_handler);

    return 0;
}

void signal_handler(int signum)
{
    if (signum == SIGTERM)
    {
        syslog(LOG_USER | LOG_INFO, "Received SIGTERM. Exiting.");
        closelog();
        exit(EXIT_SUCCESS);
    }
}
