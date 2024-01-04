#include <utils.h>

#include <analyzer.h>
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
/* Return a substring of the input path, starting from the depth-th '/' character from the end of the string.
 */
char *get_relative_path_with_depth(const char *path, int depth)
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

/*  Directories "." and ".." are typically special entries in a
 * directory, and we want to skip them to avoid infinite loops.
 */
int special_directory(char *d_name)
{
    return strcmp(d_name, ".") == 0 || strcmp(d_name, "..") == 0;
}

void print_path_info(int depth, const char *path, long long total_size, long long size, FILE *output_fd)
{
    if (depth > 0 && depth <= 2)
    {
        printf("|-%s       %0.2lf%%          %lld\n", get_relative_path_with_depth(path, depth),
               ((double)size / total_size) * 100, size);
        fprintf(output_fd, "|-%s       %0.2lf%%          %lld\n", get_relative_path_with_depth(path, depth),
                ((double)size / total_size) * 100, size);
    }
    if (depth == 1)
    {
        fprintf(output_fd, "|\n");
    }
}