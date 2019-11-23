#include "taxi_list.h"

int Get_taxi_by_car_num(struct taxi_list *head, struct taxi_unit *taxi, int car_num)
{
	struct taxi_list *temp = head;

	while(temp != NULL)
	{
		if(temp->car->car_num == car_num)
		{
			memcpy(taxi, temp->car, sizeof(struct taxi_unit));
			return 0;
		}

		temp = temp->next;
	}

	return -1;
}

void Delete_taxi_by_id(struct taxi_list **head, int id)
{
	if(Is_list_empty((List*)*head) == TRUE)
		return;

	struct taxi_list *temp = (*head)->next;
	struct taxi_list *prev = (*head);

	// проверка первого элемента списка
	if((*head)->car->id == id)
	{
		free(*head);
		(*head) = temp;
		return;
	}

	// проверка остальных элементов списка
	while(temp != NULL)
	{
		if(temp->car->id == id)
		{
			prev->next = temp->next;
			free(temp);
			return;
		}
		prev = temp;
		temp = temp->next;
	}
}

int Delete_taxi_by_fd(struct taxi_list **head, int fd)
{
	if(Is_list_empty((List*)*head) == TRUE)
		return -1;

	struct taxi_list *temp = (*head)->next;
	struct taxi_list *prev = (*head);

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

void Show_reg_list(struct taxi_list *head)
{
	if(Is_list_empty((List*)head) == TRUE)
		return;

	struct taxi_list *temp = head;

	printf("registered taxi:\n");
	while(temp != NULL)
	{
		printf(" - car №%i (id: %i), status: %i, pos(%i, %i)\n", temp->car->car_num, temp->car->id, temp->car->id, temp->car->pos.x, temp->car->pos.y);
		temp = temp->next;
	}
	printf("\n");
}

int Get_id_by_num(struct taxi_list *head, int car_num)
{
	struct taxi_list *temp = head;

	while(temp != NULL)
	{
		if(temp->car->car_num == car_num)
			return temp->car->id;

		temp = temp->next;
	}

	return -1;
}

int Set_taxi_status(struct taxi_list *head, int id, int status)
{
	struct taxi_list *temp = head;

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

int Get_taxi_status(struct taxi_list *head, int id)
{
	struct taxi_list *temp = head;

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

int Set_taxi_pos(struct taxi_list *head, int id, struct Point new_pos)
{
	struct taxi_list *temp = head;

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