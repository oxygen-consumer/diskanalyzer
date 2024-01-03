#include <utils.h>

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
