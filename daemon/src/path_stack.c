#include <path_stack.h>
#include <constants.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void initialize_path_stack(PathStack *stack)
{
    stack->top = NULL;
}

int is_empty(PathStack *stack)
{
    return stack->top == NULL;
}

void push(PathStack *stack, const char *value)
{
    Node *new_node = (Node *)malloc(sizeof(Node));
    if (new_node == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    strncpy(new_node->data, value, MAX_PATH_SIZE - 1);
    new_node->data[MAX_PATH_SIZE - 1] = '\0';

    new_node->next = stack->top;
    stack->top = new_node;
}

char *pop(PathStack *stack)
{
    if (is_empty(stack))
    {
        fprintf(stderr, "Stack underflow\n");
        exit(EXIT_FAILURE);
    }

    Node *popped_node = stack->top;
    stack->top = popped_node->next;

    char *popped_value = (char *)malloc(MAX_PATH_SIZE * sizeof(char));
    if (popped_value == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    strncpy(popped_value, popped_node->data, MAX_PATH_SIZE - 1);
    popped_value[MAX_PATH_SIZE - 1] = '\0';

    free(popped_node);

    return popped_value;
}

void free_path_stack(PathStack *stack)
{
    while (!is_empty(stack))
    {
        pop(stack);
    }
}
