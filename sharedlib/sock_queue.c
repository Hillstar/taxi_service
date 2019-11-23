#include "sock_queue.h"

int Pop_sock_from_queue(struct Sock_queue **head, int *sock)
{
	if(Is_list_empty((List*)*head) == 1)
		return -1;

	struct Sock_queue *temp = (*head);
	memcpy(sock, temp->sock, sizeof(int));
	(*head) = (*head)->next;
	free(temp);

	return 0;
}

void Show_sock_queue(struct Sock_queue *head)
{
	if(Is_list_empty((List*)head) == TRUE)
		return;

	struct Sock_queue *temp = head;

	printf("Sockets in queue: ");
	while(temp != NULL)
	{
		printf("%i, ", *(temp->sock));
		temp = temp->next;
	}
	printf("\n");
}


void Push_sock_in_queue(struct Sock_queue **head, int *sock) 
{
	struct Sock_queue *new_node;
	new_node = malloc(sizeof(struct List));
	if(new_node == NULL)
	{
		printf("can`t malloc\n");
		exit(EXIT_FAILURE);
	}
	new_node->sock = malloc(sizeof(int));
	memcpy(new_node->sock, sock, sizeof(int));
	if(Is_list_empty((List *)*head) == TRUE)
	{
		new_node->next = NULL;
		(*head) = new_node;
		return;
	}

	else
	{
		struct Sock_queue *temp = *head;
		while(temp->next != NULL)
			temp = temp->next;

		temp->next = new_node;
		temp->next->next = NULL;
	}
}

int Check_sock_in_queue(struct Sock_queue *head, int sock)
{
	if(Is_list_empty((List*)head) == TRUE)
		return -1;

	struct Sock_queue *temp = head;

	while(temp != NULL)
	{
		if(*(temp->sock) == sock)
			return 0;
		temp = temp->next;
	}
	return -1;
}