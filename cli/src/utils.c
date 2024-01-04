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

bool isValidDirectory(const char *path)
{
    DIR *dir = opendir(path);
    if (dir)
    {
        closedir(dir);
        return true; // Valid directory
    }
    else
    {
        return false; // Not a directory or doesn't exist
    }
}
