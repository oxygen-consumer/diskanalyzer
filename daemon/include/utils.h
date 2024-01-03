#ifndef DAEMON_UTILS_H
#define DAEMON_UTILS_H

#include <stdbool.h>

/*
 * Kill the daemon and print the message to syslog.
 * If ok is false, the message will be printed as an error,
 * or info otherwise.
 */
void die(bool ok, const char *msg, ...);

#endif
