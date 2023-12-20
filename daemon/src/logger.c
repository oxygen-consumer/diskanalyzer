#include <logger.h>
#include <stdlib.h>
#include <stdio.h>

void write_log(const char *msg)
{
    // Open a pipe to the logger program
    FILE *logger = popen("/usr/bin/logger -t diskanalyzer", "w");

    if (logger == NULL)
    {
        perror("Failed to open logger pipe");
        exit(EXIT_FAILURE);
    }

    // Write the message to the logger program
    fprintf(logger, "%s\n", msg);

    // Close the pipe
    if (pclose(logger) == -1)
    {
        perror("Failed to close logger pipe");
        exit(EXIT_FAILURE);
    }
}
