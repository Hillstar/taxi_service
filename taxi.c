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

enum Bool { FALSE, TRUE };

void Generate_position(int info_buf[3])
{
	info_buf[0] = rand() % MAP_SIZE;
	info_buf[1] = rand() % MAP_SIZE;
	info_buf[2] = Taxi_init; // тип отправителя 
}

int main(int argc, char *argv[])
{
	if(argc < 3)
	{
		printf("too few arguments\n");
		exit(EXIT_FAILURE);
	}

	int connect_sock = 0;
	int ret_val;
	int send_buf[3]; // где [0] - тип отправителя, [1] - координата x, [0] - координата y  
	int recv_buf[4]; 
	int id = 0;
	int car_num;
	int it_first_connect = TRUE;
	int taxi_stop = FALSE;
	struct sockaddr_in serv_addr;

	srand(getpid()); 
	Generate_position(send_buf);
	car_num = atoi(argv[2]);

	socklen_t r_buf_size = sizeof(recv_buf);
	socklen_t s_buf_size = sizeof(send_buf);

	// init connect sock
	connect_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(connect_sock == -1)
	{
		perror("cant`t create socket");
		exit(EXIT_FAILURE);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(3007);
	socklen_t s_addr_size = sizeof(serv_addr);

	ret_val = connect(connect_sock, (struct sockaddr*)&serv_addr, s_addr_size);
	if(ret_val == -1)
	{
		perror("cant`t connect");
		exit(EXIT_FAILURE);
	}
		
	while(taxi_stop == FALSE)
	{
		send(connect_sock, send_buf, s_buf_size, 0);
		send(connect_sock, &car_num, sizeof(car_num), 0);
		printf("my position: (%i, %i), ", send_buf[0], send_buf[1]);
		printf("my car number: %i\n", car_num);

		if(it_first_connect == TRUE)
		{
			recv(connect_sock, &id, sizeof(id), 0);
			if(ret_val != 0)
			{
				printf("Bind error\n");
				exit(EXIT_FAILURE);
			}
			it_first_connect = FALSE;
			send_buf[2] = Taxi_ride_done;
		}

		printf("my id:%i\n", id);

		ret_val = recv(connect_sock, recv_buf, r_buf_size, 0);
		printf("Destonation: (%i, %i), time of ride: %i, price: %i\n", recv_buf[0], recv_buf[1], recv_buf[2], recv_buf[3]);
		
		sleep(recv_buf[2] / 2);

		send_buf[0] = recv_buf[0]; // x
		send_buf[1] = recv_buf[1]; // y
	}

	close(connect_sock);
	return 0;
}