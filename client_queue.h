#include <stdio.h>
#include <stdlib.h>
#ifndef QUEUE_HEADER_H
#define QUEUE_HEADER_H
#include <unistd.h>
#include <string.h>
#include "list.h"


struct Client_info
{
	int fd;
	int start_pos[2];
	int dest_pos[2];
	int ride_id;
};

struct Client_queue
{
	struct Client_queue *next;
	struct Client_info *client_info;
};

int Pop_client_info_from_queue(struct Client_queue **head, struct Client_info *client_info);

void Show_client_queue(struct Client_queue *head);

#endif