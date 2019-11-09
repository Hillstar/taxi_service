#include "taxi_list.h"

void Show_taxi_list(struct taxi_list *head)
{
	if(Is_list_empty((List*)head) == TRUE)
		return;

	struct taxi_list *temp = head;

	printf("available taxi(id): ");
	while(temp != NULL)
	{
		printf("car %i, ", temp->car->id);
		temp = temp->next;
	}
	printf("\n");
}

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

void Show_reg_list(struct taxi_list *head)
{
	if(Is_list_empty((List*)head) == TRUE)
		return;

	struct taxi_list *temp = head;

	printf("registered taxi: ");
	while(temp != NULL)
	{
		printf("car №%i (id: %i), ", temp->car->car_num, temp->car->id);
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