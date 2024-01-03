#include <utils.h>

#include <dirent.h>
#include <stdlib.h>

int stringToInt(const char *str)
{
    char *endptr;
    int result = (int)strtol(str, &endptr, 10);
    if (*endptr != '\0')
    {
        return -1;
    }
    return result;
}

int isValidDirectory(const char *path)
{
    DIR *dir = opendir(path);
    if (dir)
    {
        closedir(dir);
        return 1; // Valid directory
    }
    else
    {
        return 0; // Not a directory or doesn't exist
    }
}
