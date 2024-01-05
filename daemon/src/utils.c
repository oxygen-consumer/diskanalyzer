#include <utils.h>

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
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

/* Merge current_path and add_path in ans
 */
void add_to_path(const char *current_path, const char *add_path, char *ans)
{
    strcpy(ans, current_path);
    if (ans[strlen(ans) - 1] != '/')
    {
        strcat(ans, "/");
    }
    strcat(ans, add_path);
}

/* Get size of an file/directory.
 * Returns 0 if something went wrong.
 */
long long fsize(const char *filename)
{
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    return 0;
}

char *relative_path(const char *path, int depth)
{
    int len = strlen(path);
    for (int i = len - 1; i >= 0; i--)
    {
        if (path[i] == '/')
        {
            depth--;
            if (depth <= 0)
            {
                return (char *)path + i;
            }
        }
    }
    return (char *)path;
}

int special_directory(char *d_name)
{
    return strcmp(d_name, ".") == 0 || strcmp(d_name, "..") == 0 || d_name[0] == '.'; // also hidden files atm
}

int get_depth(const char *path, const char *subpath)
{
    // Check if subpath starts with path
    if (strncmp(path, subpath, strlen(path)) != 0)
    {
        return -1; // subpath is not a subpath of path
    }

    // Get the part of subpath after path
    const char *relative_subpath = subpath + strlen(path);

    // Count the number of '/' characters in relative_subpath
    int depth = 0;
    for (const char *p = relative_subpath; *p != '\0'; p++)
    {
        if (*p == '/')
        {
            depth++;
        }
    }

    return depth;
}
