#ifndef SOCK_QUEUE_HEADER_H
#define SOCK_QUEUE_HEADER_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "list.h"
#include "info_header.h"

struct Sock_queue
{
	struct Sock_queue *next;
	int *sock;
};

int Pop_sock_from_queue(struct Sock_queue **head, int *sock);

void Push_sock_in_queue(struct Sock_queue **head, int *sock);

void Show_sock_queue(struct Sock_queue *head);

int Check_sock_in_queue(struct Sock_queue *head, int sock);

#endif
