#ifndef DAEMON_DAEMONIZE_H
#define DAEMON_DAEMONIZE_H

/*
 * Make the process a daemon using double fork. See TIP chapter 37.
 * Returns 0 on success, 1 on failure.
 */
unsigned short daemonize(void);

/*
 * Define various signals behaviour
 */
void signal_handler(int signum);

#endif
