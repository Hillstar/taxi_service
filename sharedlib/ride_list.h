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
	int client_fd;
	struct taxi_unit car;
	int status;
	Point start_pos;
	Point dest_pos;
	int price;
	int dist_to_dest;
	int dist_to_client;
	int time_of_ride;
	int taxi_state;
};

int Get_ride_info_by_ride_id(struct Ride_info_list *head, struct Ride_info *info_buf, int ride_id);

int Get_ride_info_by_taxi_id(struct Ride_info_list *head, struct Ride_info *info_buf, int taxi_id);

int Get_ride_info_by_client_fd(struct Ride_info_list *head, struct Ride_info *info_buf, int client_fd);

void Show_curent_rides(struct Ride_info_list *head);

int Set_ride_status(struct Ride_info_list *head, int client_fd, int status);

void Delete_ride_by_taxi_id(struct Ride_info_list **head, int taxi_id);

#endif