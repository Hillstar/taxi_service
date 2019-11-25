#ifndef TAXI_HEADER_H
#define TAXI_HEADER_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "list.h"
#include "info_header.h"

struct taxi_unit
{
	int id;
	int car_num;
	int fd;
	Point pos;
	int cur_ride_id;
	int status;
};

struct taxi_list
{
	struct taxi_list *next;
	struct taxi_unit *car;
};

int Set_taxi_status(struct taxi_list *head, int id, int status);

int Set_taxi_pos(struct taxi_list *head, int id, struct Point new_pos);

int Get_taxi_status(struct taxi_list *head, int id);

int Get_taxi_by_car_num(struct taxi_list *head, struct taxi_unit *taxi, int car_num);

int Delete_taxi_by_id(struct taxi_list **head, int id);

int Delete_taxi_by_fd(struct taxi_list **head, int fd);

void Show_reg_list(struct taxi_list *head);

int Get_id_by_num(struct taxi_list *head, int car_num);

#endif