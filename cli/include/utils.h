#include <stdbool.h>

#include <shared.h>

int get_id(const char *str);

bool is_valid_directory(const char *path);

enum Priority get_priority(const char *str);

char *priority_to_string(enum Priority priority);

char *response_code_to_string(enum ResponseCode code);

char *read_from_path(const char *path);
