#ifndef RIDE_LIST_HEADER_H
#define RIDE_LIST_HEADER_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "list.h"
#include "taxi_list.h"
#include "info_header.h"


struct Ride_info_list
{
	struct Ride_info_list *next;
	struct Ride_info *info;
};

struct Ride_info
{
	int ride_id;
	int client_fd; // ВРЕМЕННО, ЗАМЕНИТЬ НА СТРУКТУРУ КЛИЕНТА
	struct taxi_unit *car;
	int status;
	int start_pos[2];
	int dest_pos[2];
	int price;
	int dist_to_dest;
	int dist_to_client;
};

struct taxi_unit *Get_taxi_by_client_fd(struct Ride_info_list *head, int fd);

int Get_ride_info_by_taxi_id(struct Ride_info_list *head, struct Ride_info *info_buf, int ride_id);

void Show_curent_rides(struct Ride_info_list *head);

void Set_ride_is_executing(struct Ride_info_list *head, int client_fd);

void Delete_ride_by_taxi_id(struct Ride_info_list **head, int taxi_id);

#endif