#include "taxi_list.h"

int Check_taxi_by_id(struct Taxi_list *head, int id)
{
	struct Taxi_list *temp = head;

	while(temp != NULL)
	{
		if(temp->car->id == id)
		{
			return 0;
		}

		temp = temp->next;
	}

	return -1;
}

int Get_taxi_by_id(struct Taxi_list *head, struct Taxi_unit *taxi, int id)
{
	struct Taxi_list *temp = head;

	while(temp != NULL)
	{
		if(temp->car->id == id)
		{
			memcpy(taxi, temp->car, sizeof(struct Taxi_unit));
			return 0;
		}

		temp = temp->next;
	}

	return -1;
}

int Delete_taxi_by_id(struct Taxi_list **head, int id)
{
	if(Is_list_empty((List*)*head) == TRUE)
		return -1;

	struct Taxi_list *temp = (*head)->next;
	struct Taxi_list *prev = (*head);

	// проверка первого элемента списка
	if((*head)->car->id == id)
	{
		free(*head);
		(*head) = temp;
		return 0;
	}

	// проверка остальных элементов списка
	while(temp != NULL)
	{
		if(temp->car->id == id)
		{
			prev->next = temp->next;
			free(temp);
			return 0;
		}
		prev = temp;
		temp = temp->next;
	}

	return -1;
}

int Delete_taxi_by_fd(struct Taxi_list **head, int fd)
{
	if(Is_list_empty((List*)*head) == TRUE)
		return -1;

	struct Taxi_list *temp = (*head)->next;
	struct Taxi_list *prev = (*head);

	// проверка первого элемента списка
	if((*head)->car->fd == fd)
	{
		free(*head);
		(*head) = temp;
		return 0;
	}

	// проверка остальных элементов списка
	while(temp != NULL)
	{
		if(temp->car->fd == fd)
		{
			prev->next = temp->next;
			free(temp);
			return 0;
		}
		prev = temp;
		temp = temp->next;
	}

	return -1;
}

void Show_taxi_list(struct Taxi_list *head)
{
	if(Is_list_empty((List*)head) == TRUE)
		return;

	struct Taxi_list *temp = head;

	printf("registered taxi:\n");
	while(temp != NULL)
	{
		printf(" - car(id: %i), fd: %i, status: %i\n", temp->car->id, temp->car->fd, temp->car->status);
		temp = temp->next;
	}
	printf("\n");
}

int Set_taxi_fd(struct Taxi_list *head, int id, int fd)
{
	struct Taxi_list *temp = head;

	while(temp != NULL)
	{
		if(temp->car->id == id)
		{
			temp->car->fd = fd;
			return 0;
		}

		temp = temp->next;
	}

	return -1;
}

int Set_taxi_status(struct Taxi_list *head, int id, int status)
{
	struct Taxi_list *temp = head;

	while(temp != NULL)
	{
		if(temp->car->id == id)
		{
			temp->car->status = status;
			return 0;
		}

		temp = temp->next;
	}

	return -1;
}

int Get_taxi_status(struct Taxi_list *head, int id)
{
	struct Taxi_list *temp = head;

	while(temp != NULL)
	{
		if(temp->car->id == id)
		{
			return temp->car->status;
		}

		temp = temp->next;
	}

	return -1;
}

int Get_taxi_fd(struct Taxi_list *head, int id)
{
	struct Taxi_list *temp = head;

	while(temp != NULL)
	{
		if(temp->car->id == id)
		{
			return temp->car->fd;
		}

		temp = temp->next;
	}

	return -1;
}

int Set_taxi_pos(struct Taxi_list *head, int id, struct Point new_pos)
{
	struct Taxi_list *temp = head;

	while(temp != NULL)
	{
		if(temp->car->id == id)
		{
			temp->car->pos = new_pos;
			return 0;
		}

		temp = temp->next;
	}

	return -1;
}

void Push_taxi_in_list(struct Taxi_list **head, struct Taxi_unit *taxi, int data_size) 
{
	struct Taxi_list *new_node;
	new_node = malloc(sizeof(struct List));
	if(new_node == NULL)
	{
		printf("can`t malloc\n");
		exit(EXIT_FAILURE);
	}
	new_node->car = malloc(sizeof(struct Taxi_unit));
	memcpy(new_node->car, taxi, data_size);
	if(Is_list_empty((List *)*head) == TRUE)
	{
		new_node->next = NULL;
		(*head) = new_node;
		return;
	}

	else
	{
		struct Taxi_list *temp = *head;
		while(temp->next != NULL)
			temp = temp->next;

		temp->next = new_node;
		temp->next->next = NULL;
	}
}