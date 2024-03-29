#include <stdbool.h>

#include <shared.h>

int get_id(const char *str);

bool is_valid_directory(const char *path);

enum Priority get_priority(const char *str);

char *priority_to_string(enum Priority priority);

char *response_code_to_string(enum ResponseCode code);

char *read_from_path(const char *path);

void print_deamon_report(const char *path);

long long get_first_number(char *str);

long long get_nth_number(char *str, int n);

int get_last_slash_index(const char *str);
