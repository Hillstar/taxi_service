#include "ride_list.h"

int Check_ride_by_taxi_id(struct Ride_info_list *head, int taxi_id)
{
	struct Ride_info_list *temp = head;

	while(temp != NULL)
	{
		if(temp->info->taxi_id == taxi_id)
		{
			return 0;
		}

		temp = temp->next;
	}

	return -1;
}

int Get_ride_info_by_taxi_id(struct Ride_info_list *head, struct Ride_info *info_buf, int taxi_id)
{
	struct Ride_info_list *temp = head;

	while(temp != NULL)
	{
		if(temp->info->taxi_id == taxi_id)
		{
			memcpy(info_buf, temp->info, sizeof(struct Ride_info));
			return 0;
		}

		temp = temp->next;
	}

	return -1;
}

int Get_ride_info_by_client_id(struct Ride_info_list *head, struct Ride_info *info_buf, int client_id)
{
	struct Ride_info_list *temp = head;

	while(temp != NULL)
	{
		if(temp->info->client_id == client_id)
		{
			memcpy(info_buf, temp->info, sizeof(struct Ride_info));
			return 0;
		}

		temp = temp->next;
	}

	return -1;
}

int Get_ride_info_by_client_fd(struct Ride_info_list *head, struct Ride_info *info_buf, int client_fd)
{
	struct Ride_info_list *temp = head;

	while(temp != NULL)
	{
		if(temp->info->client_fd == client_fd)
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
		printf("client id:%i, \n", temp->info->client_id);
		printf("car id:%i, \n", temp->info->taxi_id);
		printf("start pos:(%i, %i), \n", temp->info->start_pos.x, temp->info->start_pos.y);
		printf("dest pos:(%i, %i), \n", temp->info->dest_pos.x, temp->info->dest_pos.y);
		printf("price:%i, \n", temp->info->price);
		switch(temp->info->status)
		{
			case Executing:
				printf("status: EXECUTING\n");
				break;

			case Car_waiting_for_answer:
				printf("status: CAR WAITING FOR CLIENT ANSWER\n");
				break;

			case Car_waiting_for_client:
				printf("status: CAR WAITING FOR CLIENT\n");
				break;

			case Car_going_to_client:
				printf("status: CAR GOING TO CLIENT\n");
				break;
		}

		printf(" - - - - - - - - - - - - - - - - - - \n\n");
		temp = temp->next;
	}
	printf("\n");
}

int Set_ride_status(struct Ride_info_list *head, int client_id, int status)
{
	struct Ride_info_list *temp = head;

	while(temp != NULL)
	{
		if(temp->info->client_id == client_id)
		{
			temp->info->status = status;
			return 0;
		}

		temp = temp->next;
	}

	return -1;
}

int Set_ride_client_fd(struct Ride_info_list *head, int client_id, int client_fd)
{
	struct Ride_info_list *temp = head;

	while(temp != NULL)
	{
		if(temp->info->client_id == client_id)
		{
			temp->info->client_fd = client_fd;
			return 0;
		}

		temp = temp->next;
	}

	return -1;
}

void Delete_ride_by_taxi_id(struct Ride_info_list **head, int taxi_id)
{
	if(Is_list_empty((List*)*head) == TRUE)
		return;

	struct Ride_info_list *temp = (*head)->next;
	struct Ride_info_list *prev = (*head);

	// проверка первого элемента списка
	if((*head)->info->taxi_id == taxi_id)
	{
		free(*head);
		(*head) = temp;
		return;
	}

	// проверка остальных элементов списка
	while(temp != NULL)
	{
		if(temp->info->taxi_id == taxi_id)
		{
			prev->next = temp->next;
			free(temp);
			return;
		}
		prev = temp;
		temp = temp->next;
	}
}

void Delete_ride_by_client_id(struct Ride_info_list **head, int client_id)
{
	if(Is_list_empty((List*)*head) == TRUE)
		return;

	struct Ride_info_list *temp = (*head)->next;
	struct Ride_info_list *prev = (*head);

	// проверка первого элемента списка
	if((*head)->info->client_id == client_id)
	{
		free(*head);
		(*head) = temp;
		return;
	}

	// проверка остальных элементов списка
	while(temp != NULL)
	{
		if(temp->info->client_id == client_id)
		{
			prev->next = temp->next;
			free(temp);
			return;
		}
		prev = temp;
		temp = temp->next;
	}
}