#ifndef LIST_HEADER_H
#define LIST_HEADER_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define FALSE 0
#define TRUE 1

//static  enum Bool { FALSE, TRUE };

typedef struct List
{
	struct List *next;
	void *data;
} List;

int Is_list_empty(struct List *head);

void Delete_list(struct List **head);

void Push_in_list(struct List **head, void *data, int data_size);

#endif