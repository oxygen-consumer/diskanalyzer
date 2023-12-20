#include <daemonize.h>

#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

int main(void)
{
    // Connect to system log
    openlog("diskanalyzer", LOG_PID, LOG_USER);

    // Make the process a daemon
    if (daemonize())
    {
        syslog(LOG_USER | LOG_ERR, "Unable to start the daemon. Exiting.");
        closelog();
        exit(EXIT_FAILURE);
    }

    syslog(LOG_USER | LOG_INFO, "Diskanalyzer daemon ready.");

    while (true)
    {
        // TODO
        sleep(60);
    }

    /*
     * We should never reach this point.
     */

    syslog(LOG_USER | LOG_ERR, "Something really bad happened. Exiting.");
    closelog();
    return EXIT_FAILURE;
}
