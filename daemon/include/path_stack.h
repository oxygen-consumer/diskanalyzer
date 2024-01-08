#ifndef DAEMON_PATH_STACK_H
#define DAEMON_PATH_STACK_H

#include <constants.h>

typedef struct Node
{
    char data[MAX_PATH_SIZE];
    struct Node *next;
} Node;

typedef struct
{
    Node *top;
} PathStack;

void initialize_path_stack(PathStack *stack);
int is_empty(PathStack *stack);
void push(PathStack *stack, const char *value);
char *pop(PathStack *stack);
void free_path_stack(PathStack *stack);

#endif
