#include "ride_list.h"

struct taxi_unit *Get_taxi_by_client_fd(struct Ride_info_list *head, int fd)
{
	struct Ride_info_list *temp = head;

	while(temp != NULL)
	{
		if(temp->info->client_fd == fd)
			return temp->info->car;

		temp = temp->next;
	}

	return NULL;
}

int Get_ride_info_by_taxi_id(struct Ride_info_list *head, struct Ride_info *info_buf, int taxi_id)
{
	struct Ride_info_list *temp = head;

	while(temp != NULL)
	{
		if(temp->info->car->id == taxi_id)
		{
			memcpy(info_buf, temp->info, sizeof(struct Ride_info));
			return 0;
		}

		temp = temp->next;
	}

	return -1;
}

void Show_curent_rides(struct Ride_info_list *head)
{
	if(Is_list_empty((List*)head) == TRUE)
		return;

	struct Ride_info_list *temp = head;

	printf("Current rides: \n");
	while(temp != NULL)
	{
		printf("ride id:%i, \n", temp->info->ride_id);
		printf("client fd:%i, \n", temp->info->client_fd);
		printf("price:%i, \n", temp->info->price);
		printf("car id:%i, \n", temp->info->car->id);
		printf("start pos:(%i, %i), \n", temp->info->start_pos[0], temp->info->start_pos[1]);
		printf("dest pos:(%i, %i), \n", temp->info->dest_pos[0], temp->info->dest_pos[1]);
		if(temp->info->status == Executing)
			printf("status: EXECUTING\n");
		else if(temp->info->status == Waiting_for_answer)
			printf("status: WAITING FOR CLIENT ANSWER\n");
		printf(" - - - - - - - - - - - - - - - - - - \n\n");
		temp = temp->next;
	}
	printf("\n");
}

void Set_ride_is_executing(struct Ride_info_list *head, int client_fd)
{
	struct Ride_info_list *temp = head;

	while(temp != NULL)
	{
		if(temp->info->client_fd == client_fd)
		{
			temp->info->status = Executing;
			return;
		}

		temp = temp->next;
	}

	return;
}

void Delete_ride_by_taxi_id(struct Ride_info_list **head, int taxi_id)
{
	if(Is_list_empty((List*)*head) == TRUE)
		return;

	struct Ride_info_list *temp = (*head)->next;
	struct Ride_info_list *prev = (*head);

	// проверка первого элемента списка
	if((*head)->info->car->id == taxi_id)
	{
		free(*head);
		(*head) = temp;
		return;
	}

	// проверка остальных элементов списка
	while(temp != NULL)
	{
		if(temp->info->car->id == taxi_id)
		{
			prev->next = temp->next;
			free(temp);
			return;
		}
		prev = temp;
		temp = temp->next;
	}
}