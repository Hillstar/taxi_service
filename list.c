#include "list.h"

int Is_list_empty(struct List *head)
{
    if(head == NULL)
        return 1;
    else
        return 0;
}

void Delete_list(struct List **head)
{
    if(Is_list_empty(*head) == TRUE)
        return;

    struct List *cur = *head;
    struct List *next;

    while(cur != NULL)
    {
        next = cur->next;
        free(cur->data);
        free(cur);
        cur = next;
    }
}

void Push_in_list(struct List **head, void *data, int data_size) 
{
    struct List *new_node;
    new_node = malloc(sizeof(struct List));
    if(new_node == NULL)
    {
        printf("can`t malloc\n");
        exit(EXIT_FAILURE);
    }
    new_node->data = malloc(data_size);
    memcpy(new_node->data, data, data_size);
    if(Is_list_empty(*head) == TRUE)
    {
        new_node->next = NULL;
        (*head) = new_node;
        return;
    }

    else
    {
        struct List *temp = *head;
        while(temp->next != NULL)
            temp = temp->next;

        temp->next = new_node;
        temp->next->next = NULL;
    }
}