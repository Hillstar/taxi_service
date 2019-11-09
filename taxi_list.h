#ifndef TAXI_HEADER_H
#define TAXI_HEADER_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "list.h"


struct taxi_unit
{
	int id;
	int car_num;
	int fd;
	int pos[2];
	int cur_ride_id;
};

struct taxi_list
{
	struct taxi_list *next;
	struct taxi_unit *car;
};

void Show_taxi_list(struct taxi_list *head);

int Get_taxi_by_car_num(struct taxi_list *head, struct taxi_unit *taxi, int car_num);

void Delete_taxi_by_id(struct taxi_list **head, int id);

void Show_reg_list(struct taxi_list *head);

int Get_id_by_num(struct taxi_list *head, int car_num);

#endif