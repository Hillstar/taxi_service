#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "info_header.h"


void Generate_info(struct Init_msg *init_buf, Point *dest_buf)
{
	// место нахождения
	init_buf->x = rand() % MAP_SIZE;
	init_buf->y = rand() % MAP_SIZE;

	// тип отправителя 
	init_buf->type = Client_init; 

	// место назначения
	dest_buf->x = rand() % MAP_SIZE;
	dest_buf->y = rand() % MAP_SIZE;
}

int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		printf("too few arguments\n");
		exit(EXIT_FAILURE);
	}

	int connect_sock = 0;
	int ret_val;
	char choise;
	struct Init_msg info_buf;
	struct Client_info_msg recv_buf;
	Point dest_buf;
	struct sockaddr_in serv_addr;
	srand(getpid()); 
	Generate_info(&info_buf, &dest_buf);

	socklen_t r_buf_size = sizeof(recv_buf);
	socklen_t s_buf_size = sizeof(info_buf);
	socklen_t dest_buf_size = sizeof(dest_buf);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(PORT);

	socklen_t addr_size = sizeof(serv_addr);

	connect_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(connect_sock == -1)
	{
		perror("cant`t create socket");
		exit(EXIT_FAILURE);
	}
	
	ret_val = connect(connect_sock, (struct sockaddr*)&serv_addr, addr_size);
	if(ret_val == -1)
	{
		perror("cant`t connect");
		exit(EXIT_FAILURE);
	}
	
	send(connect_sock, &info_buf, s_buf_size, 0);
	send(connect_sock, &dest_buf, dest_buf_size, 0);
	printf("my position: (%i, %i)\n", info_buf.x, info_buf.y);	
	printf("my dest: (%i, %i)\n", dest_buf.x, dest_buf.y);

	recv(connect_sock, &recv_buf, r_buf_size, 0);

	if(recv_buf.status != Taxi_found)
	{
		printf("there`s no taxi now, please wait....\n");
		recv(connect_sock, &recv_buf, r_buf_size, 0);
	}

	printf("price: %i, waiting time: %i, time of ride: %i\n", recv_buf.price, recv_buf.time_of_waiting, recv_buf.time_of_ride);
	printf("will you ride?(y - yes, n - any other key): ");
	scanf("%c", &choise);
	info_buf.type = Client_choise;
	send(connect_sock, &info_buf, s_buf_size, 0);
	send(connect_sock, &choise, sizeof(choise), 0);

	if(choise == 'y')
	{
		printf("Taxi is coming\n");
		recv(connect_sock, &recv_buf, r_buf_size, 0);
		if(recv_buf.status == Car_waiting_for_client)
		{
			printf("Taxi is waiting for you\n");
		}

		recv(connect_sock, &recv_buf, r_buf_size, 0);
		if(recv_buf.status == Executing)
		{
			printf("The ride has begun\n");
		}

		recv(connect_sock, &recv_buf, r_buf_size, 0);
		if(recv_buf.status == Done)
		{
			printf("You are at destonation\n");
		}
	}
	
	close(connect_sock);

	return 0;
}
