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


void Generate_position(struct Init_msg *info_buf)
{
	info_buf->x = rand() % MAP_SIZE;
	info_buf->y = rand() % MAP_SIZE;
	info_buf->type = Taxi_init; // тип отправителя 
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
	struct Init_msg send_buf;
	struct Taxi_info_msg recv_buf; 
	int id = 0;
	int car_num;
	int taxi_stop = False;
	int cur_state = Not_initialized;
	struct sockaddr_in serv_addr;

	srand(getpid()); 
	Generate_position(&send_buf);
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
	serv_addr.sin_port = htons(PORT);
	socklen_t s_addr_size = sizeof(serv_addr);

	ret_val = connect(connect_sock, (struct sockaddr*)&serv_addr, s_addr_size);
	if(ret_val == -1)
	{
		perror("cant`t connect");
		exit(EXIT_FAILURE);
	}

	send(connect_sock, &send_buf, s_buf_size, 0);;
	printf("my position: (%i, %i), ", send_buf.x, send_buf.y);
	printf("my car number: %i\n", car_num);
		
	while(taxi_stop == False)
	{
		if(cur_state == Not_initialized)
		{
			send(connect_sock, &car_num, sizeof(car_num), 0);
			ret_val = recv(connect_sock, &id, sizeof(id), 0);
			if(ret_val == -1)
			{
				perror("can`t recv");
				exit(EXIT_FAILURE);
			}
			printf("my id:%i\n", id);
			cur_state = Waiting_for_order;
		}

		if(cur_state == Waiting_for_order)
		{
			ret_val = recv(connect_sock, &recv_buf, r_buf_size, 0);
			printf("status = %i\n", recv_buf.status);
			switch(recv_buf.status)
			{
				case Car_waiting_for_client:
					cur_state = Waiting_for_client;
					break;

				case Executing:
					cur_state = Going_to_dest;
					break;
					
				case Car_going_to_client:
					cur_state = Going_to_client;
					break;

				case Car_waiting_for_answer:
					cur_state = Waiting_for_client_answer;
					break;

				default:
					printf("oops, something wrong\n");
					exit(EXIT_FAILURE);
			}	
		}

		if(cur_state == Waiting_for_client_answer)
		{
			cur_state = Waiting_for_order;
			continue;
		}
		

		if(cur_state == Going_to_client)
		{
			printf("Destonation: (%i, %i), time of ride: %i, price: %i\n", recv_buf.dest_x, recv_buf.dest_y, recv_buf.time_of_ride, recv_buf.price);
			// едем до клиента
			sleep(recv_buf.dist_to_client / 2);

			// на месте, ждет клиента
			cur_state = Waiting_for_client;
			send_buf.type = Taxi_waiting_for_client;
			send(connect_sock, &send_buf, s_buf_size, 0);
		}
		
		if(cur_state == Waiting_for_client)
		{
			printf("Enter any key to start ride");
			getchar();
			cur_state = Going_to_dest;
			send_buf.type = Taxi_going_to_dest;
			send(connect_sock, &send_buf, s_buf_size, 0);
		}

		if(cur_state == Going_to_dest)
		{
			sleep(recv_buf.dist_to_dest / 2);
			send_buf.type = Taxi_ride_done;
			send_buf.x = recv_buf.dest_x;
			send_buf.y = recv_buf.dest_y;
			cur_state = Waiting_for_order;
			send(connect_sock, &send_buf, s_buf_size, 0);;
			printf("my position: (%i, %i), ", send_buf.x, send_buf.y);
			printf("my car number: %i\n", car_num);
		}
	}

	close(connect_sock);
	return 0;
}