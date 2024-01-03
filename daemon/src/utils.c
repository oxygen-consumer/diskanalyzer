#include <utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

void die(bool ok, const char *msg, ...)
{
    syslog(LOG_USER | (ok ? LOG_INFO : LOG_ERR), "%s", msg);

    // Remove the socket file
    char SV_SOCK_PATH[37];
    sprintf(SV_SOCK_PATH, "/var/run/user/%d/diskanalyzer.sock", getuid());
    if (remove(SV_SOCK_PATH) == -1)
    {
        syslog(LOG_USER | LOG_WARNING, "Unable to remove the socket file located at %s.", SV_SOCK_PATH);
    }

    closelog();
    exit(ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
