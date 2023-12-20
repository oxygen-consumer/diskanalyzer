#ifndef DAEMON_DAEMONIZE_H
#define DAEMON_DAEMONIZE_H

/*
 * Make the process a daemon using double fork. See daemon(7).
 * Returns 0 on success, 1 on failure.
 */
unsigned short daemonize(void);

/*
 * Define various signals behaviour
 */
void signal_handler(int signum);

#endif
