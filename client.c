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


void Generate_info(struct Init_msg *init_msg_buf, Point *dest_buf)
{
	// место нахождения
	init_msg_buf->x = rand() % MAP_SIZE;
	init_msg_buf->y = rand() % MAP_SIZE;

	// место назначения
	dest_buf->x = rand() % MAP_SIZE;
	dest_buf->y = rand() % MAP_SIZE;
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
	int id = atoi(argv[2]);
	
	struct Client_info_msg recv_buf;
	struct Init_msg init_msg_buf;
	init_msg_buf.id = id;
	init_msg_buf.type = Client_init;
	
	Point dest_buf;
	Generate_info(&init_msg_buf, &dest_buf);

	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(PORT);

	socklen_t addr_size = sizeof(serv_addr);
	socklen_t s_buf_size = sizeof(init_msg_buf);
	socklen_t r_buf_size = sizeof(recv_buf);
	socklen_t dest_buf_size = sizeof(dest_buf);

	int connect_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(connect_sock == -1)
	{
		perror("cant`t create socket");
		exit(EXIT_FAILURE);
	}
	
	ret_val = connect(connect_sock, (struct sockaddr*)&serv_addr, addr_size);
	if(ret_val < 0)
	{
		perror("cant`t connect");
		exit(EXIT_FAILURE);
	}

	char choise;
	int cur_state = C_not_initialized;
	int client_stop = False;
	int connection_duplicate;
	while(client_stop == False)
	{
		switch(cur_state)
		{
			case C_not_initialized:
				send(connect_sock, &init_msg_buf, s_buf_size, 0);	
				send(connect_sock, &dest_buf, dest_buf_size, 0);

				printf("my position: (%i, %i)\n", init_msg_buf.x, init_msg_buf.y);	
				printf("my dest: (%i, %i)\n", dest_buf.x, dest_buf.y);
				printf("my id: %i\n", id);

				ret_val = recv(connect_sock, &connection_duplicate, sizeof(connection_duplicate), 0);
				if(ret_val < 0)
				{
					perror("can`t recv");
					exit(EXIT_FAILURE);
				}

				if(connection_duplicate == True)
				{
					printf("Your last session wasn`t end. Pls, try to reconnect later\n");
					client_stop = True;
				}

				else
					cur_state = Waiting_for_offer;
				break;

			case Waiting_for_offer:
				ret_val = recv(connect_sock, &recv_buf, r_buf_size, 0);
				if(ret_val < 0)
				{
					perror("can`t recv");
					exit(EXIT_FAILURE);
				}
				printf("status: %i\n", recv_buf.status);
				switch(recv_buf.status)
				{
					case Taxi_found:
						cur_state = Answering;
						break;

					case Car_waiting_for_client:
						printf("Taxi is waiting for you\n");
						cur_state = C_going_to_taxi;
						break;

					case Rejected:
						cur_state = Waiting_for_offer;
						break;

					case No_taxi_now:
						printf("there`s no taxi now, please wait....\n");
						cur_state = Waiting_in_queue;
						break;

					case Car_going_to_client:
						printf("Taxi is coming\n");
						cur_state = Waiting_for_taxi;
						break;

					case Car_waiting_for_answer:
						cur_state = Answering;
						break;

					case Executing:
						printf("The ride has begun\n");
						cur_state = C_going_to_dest;
						break;
				}
				break;

			case Waiting_in_queue: /// ДОДЕЛАТЬ
				ret_val = recv(connect_sock, &recv_buf, r_buf_size, 0);
				if(ret_val < 0)
				{
						perror("can`t recv");
						exit(EXIT_FAILURE);
				}
				cur_state = Answering;
				break;

			case Waiting_for_taxi:
				ret_val = recv(connect_sock, &recv_buf, r_buf_size, 0);
				if(ret_val < 0)
				{
						perror("can`t recv");
						exit(EXIT_FAILURE);
				}

				if(recv_buf.status == Car_waiting_for_client)
				{
					printf("Taxi is waiting for you\n");
					cur_state = C_going_to_taxi;
				}
				else if(recv_buf.status == Rejected)
				{
					printf("Ride rejected, looking for new taxi\n");
					send(connect_sock, &dest_buf, dest_buf_size, 0);
					cur_state = Waiting_for_offer;
				}
				break;

			case Answering:
				printf("price: %i, waiting time: %i, time of ride: %i\n", recv_buf.price, recv_buf.time_of_waiting, recv_buf.time_of_ride);
				printf("will you ride?(y - yes, n - any other key): ");
				fflush(stdin);
				scanf(" %c", &choise);
				init_msg_buf.type = Client_choise;
				send(connect_sock, &init_msg_buf, s_buf_size, 0);
				send(connect_sock, &choise, sizeof(choise), 0);
				if(choise != 'y')
				{
					printf("ride rejected\n");
					client_stop = True;
				}
				else
				{
					printf("Taxi is coming\n");
					cur_state = Waiting_for_taxi; 
				}

				break;

			case C_going_to_taxi:
					ret_val = recv(connect_sock, &recv_buf, r_buf_size, 0);
					if(ret_val < 0)
					{
						perror("can`t recv");
						exit(EXIT_FAILURE);
					}

					if(recv_buf.status == Executing)
					{
						printf("The ride has begun\n");
						cur_state = C_going_to_dest;
					}
				break;

			case C_going_to_dest:
					ret_val = recv(connect_sock, &recv_buf, r_buf_size, 0);
					if(ret_val < 0)
					{
						perror("can`t recv");
						exit(EXIT_FAILURE);
					}

					if(recv_buf.status == Done)
					{
						printf("You are at destonation\n");
					}
					client_stop = True;
				break;

			default:
				printf("oops, something wrong\n");
				exit(EXIT_FAILURE);
		}
	}
	
	close(connect_sock);
	return 0;
}