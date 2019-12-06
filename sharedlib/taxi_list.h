#ifndef TAXI_HEADER_H
#define TAXI_HEADER_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "list.h"
#include "info_header.h"

struct Taxi_unit
{
	int id;
	int fd;
	Point pos;
	int status;
	//int connection_status;
};

struct Taxi_list
{
	struct Taxi_list *next;
	struct Taxi_unit *car;
};

int Check_taxi_by_id(struct Taxi_list *head, int id);

int Set_taxi_fd(struct Taxi_list *head, int id, int fd);

int Set_taxi_status(struct Taxi_list *head, int id, int status);

int Set_taxi_ride(struct Taxi_list *head, int id, int ride_id);

int Set_taxi_pos(struct Taxi_list *head, int id, struct Point new_pos);

int Get_taxi_status(struct Taxi_list *head, int id);

int Get_taxi_fd(struct Taxi_list *head, int fd);

int Get_taxi_by_id(struct Taxi_list *head, struct Taxi_unit *taxi, int car_num);

int Delete_taxi_by_id(struct Taxi_list **head, int id);

int Delete_taxi_by_fd(struct Taxi_list **head, int fd);

void Show_taxi_list(struct Taxi_list *head);

void Push_taxi_in_list(struct Taxi_list **head, struct Taxi_unit *taxi, int data_size) ;

#endif