#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <math.h>
#include "list.h"
#include "info_header.h"
#include "taxi_list.h"
#include "client_queue.h"
#include "ride_list.h"


int listen_sock = 0;
FILE *log_file;
struct Ride_info_list *cur_rides_list = NULL;

void handle_sigusr1(int sig)
{ 
    Show_curent_rides(cur_rides_list);
} 

void handle_sigtint(int sig) 
{ 
    fclose(log_file); 
    close(listen_sock);
    exit(EXIT_SUCCESS);
} 

void Print_in_log(struct Ride_info *info)
{
	fprintf(log_file, "clint fd:%i\n", info->client_fd);
	fprintf(log_file, "price:%i\n", info->price);
	fprintf(log_file, "taxi id:%i\n", info->car->id);
	fprintf(log_file, "car num:%i\n", info->car->car_num);
	fprintf(log_file, "start position: (%i, %i)\n", info->start_pos[0], info->start_pos[1]);
	fprintf(log_file, "dest position: (%i, %i)\n", info->dest_pos[0], info->dest_pos[1]);
	fprintf(log_file, "status: DONE\n");
	fprintf(log_file, " - - - - - - - - - - - - - -\n\n");
}

void Send_info_to_taxi(struct taxi_unit *taxi, int client_dest[2], int time, int price)
{
	printf("taxi fd = %i\n", taxi->fd);
	int taxi_sock = taxi->fd;
	int send_buf[4]; // [0][1] = координаты места назначения, [2] - время поездки

	send_buf[0] = client_dest[0];
	send_buf[1] = client_dest[1];
	send_buf[2] = time;
	send_buf[3] = price;

	send(taxi_sock, send_buf, sizeof(send_buf), 0);
}

float Calc_dist(int start_pos[2], int dest_pos[2])
{
	return sqrt(pow((dest_pos[0] - start_pos[0]), 2) + pow((dest_pos[1] - start_pos[1]), 2));
}

struct taxi_unit *Get_nearest_driver(float *dist, struct taxi_list *list, int client_pos[2])
{
	struct taxi_list *temp = list;
	int min_dist = Calc_dist(list->car->pos, client_pos);
	struct taxi_unit *nearest_driver = list->car;

	// находим ближайшего водителя
	while(temp != NULL)
	{
		if(min_dist > Calc_dist(temp->car->pos, client_pos))
			nearest_driver = temp->car;
		temp = temp->next;
	}

	*dist = Calc_dist(nearest_driver->pos, client_pos);
	return nearest_driver;
}

int main(int argc, char *argv[])
{
	int i, j;
	int accept_sock = 0;
	int ret_val;
	struct sockaddr_in serv_addr, client_addr;
	int recv_buf[3];  // 
	int c_send_buf[4]; // [0] = -1 если таксист не найден, 1 если найден, [1] = стоимость, [2] = время ожидания, [3] = время поездки 

  	int end_server = FALSE, compress_array = FALSE;
  	int close_conn;
  	int timeout;
  	int nfds = 1, current_size = 0;
  	struct pollfd fds[200];

  	struct Ride_info ride_info;

  	struct Client_queue *client_queue = NULL;
  	struct Client_info client_info;

	struct taxi_unit taxi;
	struct taxi_unit *nearest_taxi;
	struct taxi_list *av_taxi_list = NULL;
	struct taxi_list *reg_list = NULL;
	int car_num;
	int num_of_taxi = 0;
	int id_taxi_counter = 0;
	int id_ride_counter = 0;
	float dist_to_client = 0;
	float dist_to_dest = 0;
	int client_dest[2];
	char client_choise;
	int price = 0;

	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);

	socklen_t c_buf_size = sizeof(c_send_buf);
	socklen_t buf_size = sizeof(recv_buf);
	socklen_t cli_addr_len = sizeof(client_addr);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(3007);

	signal(SIGINT, handle_sigtint); 
	signal(SIGUSR1, handle_sigusr1);

	printf("mypid is %i\n", getpid());

	log_file = fopen("log", "w");
	if(log_file == NULL)
	{
		printf("Can`t open log file\n");
		exit(EXIT_FAILURE);
	}

	fprintf(log_file, "server start working...\n");
	fprintf(log_file, " - - - - - - - - - - - - - -\n\n");

	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_sock == -1)
	{
		perror("cant`t create socket");
		exit(EXIT_FAILURE);
	}

	ret_val = bind(listen_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	if(ret_val != 0)
	{
		printf("Bind error\n");
		exit(EXIT_FAILURE);
	}

	ret_val = listen(listen_sock, 30);
	if(ret_val == -1)
	{
		perror("cant`t listen socket");
		exit(EXIT_FAILURE);
	}

  	fds[0].fd = listen_sock;
  	fds[0].events = POLLIN;
  	timeout = -1; // бесконечный таймер ожидания

	while(end_server == FALSE)
	{
		//sigprocmask(SIG_SETMASK, &mask, NULL);
		ret_val = poll(fds, nfds, timeout);
		//sigprocmask(SIG_UNBLOCK, &mask, NULL);
    	if(ret_val < 0)
    	{
    		if(errno == EINTR)
    			ret_val = poll(fds, nfds, timeout);
    		else
      		{
      			perror("poll() failed");
      			exit(EXIT_FAILURE);
      		}
    	}

    	current_size = nfds;

		for(i = 0; i < current_size; i++)
    	{
      		if(fds[i].revents == 0)
      		{
       			continue;
       			printf("revents = 0\n");
      		}

      		if(fds[i].revents != POLLIN)
      		{
        		printf("Error! revents = %d\n", fds[i].revents);
        		end_server = TRUE;
        		break;
      		}

	    	if(fds[i].fd == listen_sock)
	    	{	
		        accept_sock = accept(listen_sock, (struct sockaddr*)&client_addr, &cli_addr_len);
				if(accept_sock == -1)
				{
					perror("Can`t accept");
					exit(EXIT_FAILURE);
				}

		        fds[nfds].fd = accept_sock;
		        fds[nfds].events = POLLIN;
		        nfds++;
	    	}

		    else
		    {
		        close_conn = FALSE;

		        ret_val = recv(fds[i].fd, recv_buf, buf_size, 0);
		        if(ret_val < 0)
		        {
		            perror("recv() failed");
		            close_conn = TRUE;
		           	exit(EXIT_FAILURE);
		        }

		        if(ret_val == 0)
		        {
		        	printf("Connection closed\n");
		           	close_conn = TRUE;
		        }

		        if(close_conn == TRUE)
		        {
		          	close(fds[i].fd);
		          	fds[i].fd = -1;
		          	compress_array = TRUE;
		        }

		        else
		        {
		        	if(recv_buf[2] == Client_init)
					{
						recv(fds[i].fd, client_dest, (socklen_t)sizeof(client_dest), 0);
						printf("type: Client_init, position: (%i, %i), destonation: (%i, %i)\n", recv_buf[0], recv_buf[1], client_dest[0], client_dest[1]);
						if(Is_list_empty((List*)av_taxi_list) != 1)
						{
							nearest_taxi = Get_nearest_driver(&dist_to_client, av_taxi_list, recv_buf);
							dist_to_dest = Calc_dist(recv_buf, client_dest);
							price = (int)(round(dist_to_dest * 2) - num_of_taxi);
							printf("nearest taxi(id):%i\n", nearest_taxi->id);
							printf("dist to client: %f\n", dist_to_client);
							printf("dist from client to dest: %f\n", dist_to_dest);

							// собираем всю инфу о поездке
							ride_info.client_fd = fds[i].fd;
							ride_info.price = price;
							ride_info.car = nearest_taxi;
							ride_info.status = Waiting_for_answer;
							ride_info.start_pos[0] = recv_buf[0];
							ride_info.start_pos[1] = recv_buf[1];
							ride_info.dest_pos[0] = client_dest[0];
							ride_info.dest_pos[1] = client_dest[1];
							ride_info.dist_to_client = dist_to_client;
							ride_info.dist_to_dest = dist_to_dest;
							
							// отправляем инфу клиенту
							c_send_buf[0] = Taxi_found;
							c_send_buf[1] = price;
							c_send_buf[2] = (int)round(dist_to_client);
							c_send_buf[3] = (int)round(dist_to_dest);
							send(fds[i].fd, c_send_buf, c_buf_size, 0);
							nearest_taxi->cur_ride_id = id_ride_counter;
							id_ride_counter++;

							// добавляем инфу о поездке
							Push_in_list((List**)&cur_rides_list, &ride_info, sizeof(ride_info));

							// убираем такси из списка свободных
							Delete_taxi_by_id(&av_taxi_list, nearest_taxi->id); 
							num_of_taxi--;
						}

						else
						{
							// говорим клиенту подождать
							client_info.fd = fds[i].fd;
							client_info.start_pos[0] = recv_buf[0];
							client_info.start_pos[1] = recv_buf[1];
							client_info.dest_pos[0] = client_dest[0];
							client_info.dest_pos[1] = client_dest[1];

							Push_in_list((List**)&client_queue, &client_info, sizeof(client_info));
							c_send_buf[0] = No_taxi_now;
							c_send_buf[1] = c_send_buf[2] = c_send_buf[3] = 0;
							ret_val = send(fds[i].fd, c_send_buf, c_buf_size, 0);
						}
					}

					else if(recv_buf[2] == Client_choise)
					{
						recv(fds[i].fd, &client_choise, (socklen_t)sizeof(char), 0);
						printf("client choise: %c\n", client_choise);
						if(client_choise == 'y') // если клиент согласен
						{
							Send_info_to_taxi(nearest_taxi, client_dest, (dist_to_client + dist_to_dest), price);
							Set_ride_is_executing(cur_rides_list, fds[i].fd);
						}

						else // клиент отказался от поездки
						{
							printf("ride rejected\n");
							taxi = *Get_taxi_by_client_fd(cur_rides_list, fds[i].fd);
							Push_in_list((List**)&av_taxi_list, &taxi, sizeof(taxi));
							num_of_taxi++;
						}
					}

					else if(recv_buf[2] == Taxi_init || recv_buf[2] == Taxi_ride_done)
					{
						recv(fds[i].fd, &car_num, (socklen_t)sizeof(car_num), 0);
						if(recv_buf[2] == Taxi_init)
						{
							taxi.id = Get_id_by_num(reg_list, car_num);
							if(taxi.id == -1)
							{
								printf("new unregistered car\n");
								taxi.id = id_taxi_counter; // выдаем новый id
								taxi.car_num = car_num;
								taxi.fd = fds[i].fd;  // записываем номер сокета
 								Push_in_list((List**)&reg_list, &taxi, sizeof(taxi));
								Show_reg_list(reg_list);
								id_taxi_counter++;
								send(fds[i].fd, &(taxi.id), sizeof(int), 0);
							}

							else
							{
								printf("new registered car\n");
								Show_reg_list(reg_list);
								send(fds[i].fd, &(taxi.id), sizeof(int), 0);
							}
						}

						else if(recv_buf[2] == Taxi_ride_done) 
						{
							printf("taxi with num: %i is back\n", car_num);
							Get_taxi_by_car_num(reg_list, &taxi, car_num);
							Get_ride_info_by_taxi_id(cur_rides_list, &ride_info, taxi.id);
							c_send_buf[0] = Done;
							send(ride_info.client_fd, c_send_buf, sizeof(c_send_buf), 0);
							Print_in_log(&ride_info);
							Delete_ride_by_taxi_id(&cur_rides_list, taxi.id);
						}

						taxi.pos[0] = recv_buf[0];
						taxi.pos[1] = recv_buf[1];
						Push_in_list((List**)&av_taxi_list, &taxi, sizeof(taxi));
						printf("type: Taxi, id:%i, position:(%i, %i)\n", taxi.id, recv_buf[0], recv_buf[1]);

						ret_val = Get_ride_info_by_taxi_id(cur_rides_list, &ride_info, taxi.id);

						// если таксист уже в поездке
						if(ret_val == 0) 
						{
							printf(" - TAXI IS ALREADY ON THE RIDE\n");
							printf("position: (%i, %i), destonation: (%i, %i)\n", ride_info.start_pos[0], ride_info.start_pos[1], ride_info.dest_pos[0], ride_info.dest_pos[1]);

							// берем инфу из записи о поездке и снова отправляем таксисту
							nearest_taxi = ride_info.car;
							dist_to_client = ride_info.dist_to_client;
							dist_to_dest = ride_info.dist_to_dest;
							price = ride_info.price;

							// заново отправляем инфу таксисту
							Send_info_to_taxi(nearest_taxi, ride_info.dest_pos, (dist_to_client + dist_to_dest), price);

							// убираем такси из списка свободных
							Delete_taxi_by_id(&av_taxi_list, nearest_taxi->id); 
								num_of_taxi--;
						}

						else
						{
							ret_val = Pop_client_info_from_queue(&client_queue, &client_info);
							// если есть клиенты в очереди ожидания
							if(ret_val != -1)
							{
								printf(" - GOT CLIENT IN QUEUE\n");
								nearest_taxi = &taxi;
								dist_to_client = Calc_dist(client_info.start_pos, taxi.pos);
								dist_to_dest = Calc_dist(client_info.start_pos, client_info.dest_pos);
								price = (int)(round(dist_to_dest * 2) - num_of_taxi);
								printf("nearest taxi(id):%i\n", nearest_taxi->id);
								printf("dist to client: %f\n", dist_to_client);
								printf("dist from client to dest: %f\n", dist_to_dest);

								// собираем всю инфу о поездке
								ride_info.client_fd = client_info.fd;
								ride_info.price = price;
								ride_info.car = nearest_taxi;
								ride_info.status = Waiting_for_answer;
								ride_info.start_pos[0] = client_info.start_pos[0];
								ride_info.start_pos[1] = client_info.start_pos[1];
								ride_info.dest_pos[0] = client_info.dest_pos[0];
								ride_info.dest_pos[1] = client_info.dest_pos[1];

								printf("position: (%i, %i), destonation: (%i, %i)\n", ride_info.start_pos[0], ride_info.start_pos[1], ride_info.dest_pos[0], ride_info.dest_pos[1]);

								// отправляем инфу клиенту
								c_send_buf[0] = Done;
								c_send_buf[1] = price;
								c_send_buf[2] = (int)round(dist_to_client);
								c_send_buf[3] = (int)round(dist_to_dest);
								send(client_info.fd, c_send_buf, c_buf_size, 0);
								nearest_taxi->cur_ride_id = id_ride_counter;
								id_ride_counter++;

								// добавляем инфу о поездке
								Push_in_list((List**)&cur_rides_list, &ride_info, sizeof(ride_info));

								// убираем такси из списка свободных
								Delete_taxi_by_id(&av_taxi_list, nearest_taxi->id); 
								num_of_taxi--;
							}
						}

						num_of_taxi++;
					}
				}
			} 

			printf("Current num of taxi = %i\n", num_of_taxi);
			Show_taxi_list(av_taxi_list);
			Show_client_queue(client_queue);
			printf(" - - - - - - - - - - - - - - - - - - - \n");
		} 

		// сжимаем массив сокетов, если необходимо
		if(compress_array == TRUE)
    	{
        	compress_array = FALSE;
      		for(i = 0; i < nfds; i++)
      		{
        		if(fds[i].fd == -1)
        		{
          			for(j = i; j < nfds; j++)
          			{
            			fds[j].fd = fds[j+1].fd;
          			}
          		i--;
          		nfds--;
        		}
      		}
    	}
	}
	
	// закрываем все сокеты
	for(i = 0; i < nfds; i++) 
  	{
    	if(fds[i].fd >= 0)
      		close(fds[i].fd);
  	}

  	fclose(log_file);
	return 0;
}