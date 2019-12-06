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


void Generate_position(struct Init_msg *init_msg_buf)
{
	init_msg_buf->x = rand() % MAP_SIZE;
	init_msg_buf->y = rand() % MAP_SIZE;
}

int main(int argc, char *argv[])
{
	if(argc < 3)
	{
		printf("too few arguments\n");
		exit(EXIT_FAILURE);
	}

	srand(getpid());

	int ret_val;
	
	// init connect sock
	int connect_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(connect_sock == -1)
	{
		perror("cant`t create socket");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(PORT);
	socklen_t s_addr_size = sizeof(serv_addr);

	ret_val = connect(connect_sock, (struct sockaddr*)&serv_addr, s_addr_size);
	if(ret_val < 0)
	{
		perror("cant`t connect");
		exit(EXIT_FAILURE);
	}

	int id = atoi(argv[2]);

	struct Init_msg init_msg_buf;
	init_msg_buf.id = id;
	init_msg_buf.type = Taxi_init;
	socklen_t s_buf_size = sizeof(init_msg_buf);
	Generate_position(&init_msg_buf);

	struct Taxi_info_msg recv_buf; 
	socklen_t r_buf_size = sizeof(recv_buf);
		
	int connection_duplicate = False;
	int cur_state = Not_initialized;
	int taxi_stop = False;

	while(taxi_stop == False)
	{
		switch(cur_state)
		{
			case Not_initialized:
				send(connect_sock, &init_msg_buf, s_buf_size, 0);
				printf("my position: (%i, %i), ", init_msg_buf.x, init_msg_buf.y);
				printf("my car number: %i\n", id);
				printf("my id:%i\n", id);
				
				ret_val = recv(connect_sock, &connection_duplicate, sizeof(connection_duplicate), 0);
				if(ret_val < 0)
				{
					perror("can`t recv");
					exit(EXIT_FAILURE);
				}

				if(connection_duplicate == True)
				{
					printf("Your last session wasn`t end. Pls, try to reconnect later\n");
					taxi_stop = True;
				}

				else
					cur_state = Waiting_for_order;

				break;

			case Waiting_for_order:
				printf("Waiting for order\n");
				ret_val = recv(connect_sock, &recv_buf, r_buf_size, 0);
				if(ret_val < 0)
				{
					perror("can`t recv");
					exit(EXIT_FAILURE);
				}
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
						sleep(1);
						cur_state = Waiting_for_client_answer;
						break;

					default:
						printf("oops, something wrong\n");
						exit(EXIT_FAILURE);
				}
				break;

			case Waiting_for_client_answer:
				cur_state = Waiting_for_order;
				break;

			case Going_to_client:
				printf("Going to client\n");
				printf("Destonation: (%i, %i), time of ride: %i, price: %i\n", recv_buf.dest.x, recv_buf.dest.y, recv_buf.time_of_ride, recv_buf.price);
				// едем до клиента
				sleep((int)recv_buf.dist_to_client / 2);

				// на месте, ждет клиента
				cur_state = Waiting_for_client;
				init_msg_buf.type = Taxi_waiting_for_client;
				send(connect_sock, &init_msg_buf, s_buf_size, 0);
				break;

			case Waiting_for_client:
				printf("waiting for client\n");
				printf("Enter any key to start ride\n");
				getchar();
				cur_state = Going_to_dest;
				init_msg_buf.type = Taxi_going_to_dest;
				send(connect_sock, &init_msg_buf, s_buf_size, 0);
				break;

			case Going_to_dest:
				printf("going to dest\n");
				sleep((int)recv_buf.dist_to_dest / 2);
				init_msg_buf.type = Taxi_ride_done;
				init_msg_buf.x = recv_buf.dest.x;
				init_msg_buf.y = recv_buf.dest.y;
				send(connect_sock, &init_msg_buf, s_buf_size, 0);
				printf("ride is done\n");
				printf("my position: (%i, %i), ", init_msg_buf.x, init_msg_buf.y);
				printf("my car number: %i\n", id);
				cur_state = Waiting_for_order;
				break;

			default:
				printf("oops, something wrong\n");
				exit(EXIT_FAILURE);
		}	
	}

	close(connect_sock);
	return 0;
}