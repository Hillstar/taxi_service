#ifndef QUEUE_HEADER_H
#define QUEUE_HEADER_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "list.h"
#include "info_header.h"


struct Client_info
{
	int fd;
	int id;
	Point start_pos;
	Point dest_pos;
};

struct Client_queue
{
	struct Client_queue *next;
	struct Client_info *client_info;
};

int Pop_client_info_from_queue(struct Client_queue **head, struct Client_info *client_info);

void Push_client_in_queue(struct Client_queue **head, struct Client_info *client_info, int data_size);

void Show_client_queue(struct Client_queue *head);

void Delete_client_by_id(struct Client_queue **head, int id);

#endif