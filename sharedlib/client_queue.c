#include "client_queue.h"

int Pop_client_info_from_queue(struct Client_queue **head, struct Client_info *client_info)
{
	if(Is_list_empty((List*)*head) == 1)
		return -1;

	struct Client_queue *temp = (*head);
	memcpy(client_info, temp->client_info, sizeof(struct Client_info));
	(*head) = (*head)->next;
	free(temp);

	return 0;
}

void Show_client_queue(struct Client_queue *head)
{
	if(Is_list_empty((List*)head) == TRUE)
		return;

	struct Client_queue *temp = head;

	printf("Clients in queue: ");
	while(temp != NULL)
	{
		printf("client_fd:%i, ", temp->client_info->fd);
		temp = temp->next;
	}
	printf("\n");
}

void Delete_client_by_fd(struct Client_queue **head, int fd)
{
	if(Is_list_empty((List*)*head) == TRUE)
		return;

	struct Client_queue *temp = (*head)->next;
	struct Client_queue *prev = (*head);

	// проверка первого элемента списка
	if((*head)->client_info->fd == fd)
	{
		free(*head);
		(*head) = temp;
		return;
	}

	// проверка остальных элементов списка
	while(temp != NULL)
	{
		if(temp->client_info->fd == fd)
		{
			prev->next = temp->next;
			free(temp);
			return;
		}
		prev = temp;
		temp = temp->next;
	}
}

void Push_in_queue(struct Client_queue **head, struct Client_info *client_info, int data_size) 
{
	struct Client_queue *new_node;
	new_node = malloc(sizeof(struct List));
	if(new_node == NULL)
	{
		printf("can`t malloc\n");
		exit(EXIT_FAILURE);
	}
	new_node->client_info = malloc(sizeof(struct Client_info));
	memcpy(new_node->client_info, client_info, data_size);
	if(Is_list_empty((List *)*head) == TRUE)
	{
		new_node->next = NULL;
		(*head) = new_node;
		return;
	}

	else
	{
		struct Client_queue *temp = *head;
		while(temp->next != NULL)
			temp = temp->next;

		temp->next = new_node;
		temp->next->next = NULL;
	}
}