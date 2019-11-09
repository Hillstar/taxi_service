#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


struct List
{
	struct List *next;
	void *data;
};

struct taxi_list
{
	struct taxi_list *next;
	int *id;
};


int Is_list_empty(struct List *head)
{
	if(head == NULL)
		return 1;
	else
		return 0;
}

void Show_list(struct taxi_list *head)
{
	if(Is_list_empty((struct List*)head) == 1)
		return;

	struct taxi_list *temp = head;

	printf("available taxi(id): ");
	while(temp != NULL)
	{
		printf("car %i, ", *(temp->id));
		temp = temp->next;
	}
	printf("\n");
}


void Delete_list(struct List **head)
{
	if(Is_list_empty(*head) == 1)
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
    new_node->data = malloc(data_size);
    memcpy(new_node->data, data, data_size);
    new_node->next = (*head);
    (*head) = new_node;
}

int main()
{
	struct taxi_list *list = NULL;
	int data = 10000;
	for (int i = 0; i < 7; ++i)
	{
		Push_in_list((struct List**)&list, &data, sizeof(data));
		data++;
	}

	Show_list(list);
	return 0;
}