#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <math.h>
#include <dlfcn.h>
#include <pthread.h>
#include "info_header.h"
#include "list.h"
#include "client_queue.h"
#include "sock_queue.h"
#include "ride_list.h"
#include "taxi_list.h"


FILE *stat_file = NULL;
FILE *log_file = NULL;
pthread_t tid[NUM_THREADS];
struct Ride_info_list *cur_rides_list = NULL;
void *lib_handler;
int end_server = FALSE;

pthread_cond_t cond = PTHREAD_COND_INITIALIZER; 

pthread_mutex_t cond_lock;
pthread_mutex_t fds_lock;
pthread_mutex_t get_sock_lock;
pthread_mutex_t ride_list_lock; 
pthread_mutex_t client_queue_lock;
pthread_mutex_t reg_list_lock;

void handle_sigusr1(int sig)
{
	Show_curent_rides(cur_rides_list);
}

void handle_sigtint(int sig)
{
	int i, ret_val;
	void *status;

	end_server = TRUE;
	if(stat_file != NULL)
		fclose(stat_file);
	if(log_file != NULL)
		fclose(log_file);
	dlclose(lib_handler);

	for(i = 0; i < NUM_THREADS; i++)
	{
		ret_val = pthread_cancel(tid[i]);
		if(ret_val != 0 && ret_val != 3)
		{
			pthread_cond_signal(&cond);
			printf("Can`t cancel thread by sig, err = %i\n", ret_val);
			exit(EXIT_FAILURE);
		}
	}

	for(i = 0; i < NUM_THREADS; i++)
	{
		ret_val = pthread_join(tid[i], &status);
		if(ret_val != 0 && ret_val != 3)
		{
			printf("Can`t join thread by sig, err = %i\n", ret_val);
			exit(EXIT_FAILURE);
		}
	}
 
	pthread_mutex_destroy(&fds_lock); 
	pthread_mutex_destroy(&get_sock_lock); 
	pthread_mutex_destroy(&ride_list_lock); 
	pthread_mutex_destroy(&client_queue_lock);
	pthread_mutex_destroy(&reg_list_lock); 
	exit(EXIT_SUCCESS);
}

void Print_in_stat_file(struct Ride_info *info)
{
	if(stat_file == NULL)
	{
		printf("Can`t print, stat file is not open!\n");
		return;
	}

	fprintf(stat_file, "clint fd:%i\n", info->client_fd);
	fprintf(stat_file, "price:%i\n", info->price);
	fprintf(stat_file, "taxi id:%i\n", info->car.id);
	fprintf(stat_file, "car num:%i\n", info->car.car_num);
	fprintf(stat_file, "start position: (%i, %i)\n", info->start_pos.x, info->start_pos.y);
	fprintf(stat_file, "dest position: (%i, %i)\n", info->dest_pos.x, info->dest_pos.y);
	fprintf(stat_file, "status: DONE\n");
	fprintf(stat_file, " - - - - - - - - - - - - - -\n\n");
}

void Send_info_to_taxi(struct taxi_unit *taxi, struct Ride_info *ride_info)
{
	printf("taxi fd = %i\n", taxi->fd);
	int taxi_sock = taxi->fd;
	struct Taxi_info_msg send_buf;

	send_buf.dest_x = ride_info->dest_pos.x;
	send_buf.dest_y = ride_info->dest_pos.y;
	send_buf.time_of_ride = ride_info->time_of_ride;
	send_buf.price = ride_info->price;
	send_buf.dist_to_client = ride_info->dist_to_client;
	send_buf.dist_to_dest = ride_info->dist_to_dest;
	send_buf.status = ride_info->status;

	send(taxi_sock, &send_buf, sizeof(send_buf), 0);
}

float Calc_dist(struct Point *start_pos, struct Point *dest_pos)
{
	sleep(3); // долго считаем
	return sqrt(pow((dest_pos->x - start_pos->x), 2) + pow((dest_pos->y - start_pos->y), 2));
}

int Get_nearest_driver(float *dist, struct taxi_unit *nearest_driver, struct taxi_list *reg_list, struct Point *client_pos)
{
	if(reg_list == NULL)
		return -1;
	int taxi_found = FALSE;
	struct taxi_list *temp = reg_list;
	float new_dist, min_dist = 10000;
	struct taxi_unit *taxi;

	// находим ближайшего водителя
	while(temp != NULL)
	{
		if(temp->car->status == Waiting_for_order)
		{
			new_dist = Calc_dist(&(temp->car->pos), client_pos);
			if(new_dist < min_dist)
			{
				taxi_found = TRUE;
				min_dist = new_dist;
				taxi = temp->car;
			}
		}
		temp = temp->next;
	}

	if(taxi_found == FALSE)
		return -1;
	*dist = min_dist;
	(*nearest_driver) = (*taxi);
	return 0;
}

void Close_connection(int cur_sock, struct pollfd fds[MAX_FD], struct Fd_info *fd_info, struct taxi_list **reg_list, struct Client_queue **client_queue, int *num_of_av_taxi)
{
	int ret_val, k = 0;
	struct Ride_info info_buf;
	struct taxi_unit taxi;
	pthread_mutex_lock(&fds_lock);

	// ищем в fds поле с этим сокетом
	while(fds[k].fd != cur_sock && k < MAX_FD)
		k++;

	if(fd_info->type == Taxi)
	{
		printf("Taxi disconnected\n");
		pthread_mutex_lock(&reg_list_lock);
		ret_val = Get_ride_info_by_taxi_id(cur_rides_list, &info_buf, fd_info->unit_id);
		if(ret_val == -1)	// если текущей поездки нет
		{
			ret_val = Delete_taxi_by_id(reg_list, fd_info->unit_id);
			printf("ret_val = %i\n", ret_val);
			if(ret_val == 0)
				(*num_of_av_taxi)--;
			Show_reg_list(*reg_list);
		}
		pthread_mutex_unlock(&reg_list_lock);
	}
			
	else
	{
		pthread_mutex_lock(&client_queue_lock);
		ret_val = Get_ride_info_by_client_fd(cur_rides_list, &info_buf, cur_sock);
		if(ret_val == 0 && info_buf.status == Car_waiting_for_answer)
		{
			taxi = info_buf.car;
			pthread_mutex_lock(&reg_list_lock);
			Set_taxi_status(*reg_list, taxi.id, Waiting_for_order);
			if(Is_list_empty((List*)*reg_list) == FALSE)
			{
				Show_reg_list(*reg_list);
				(*num_of_av_taxi)++;
			}
			pthread_mutex_unlock(&reg_list_lock);

			pthread_mutex_lock(&ride_list_lock);
			Delete_ride_by_taxi_id(&cur_rides_list, taxi.id);
			pthread_mutex_unlock(&ride_list_lock);
			fd_info->cur_ride_id = -1;
		}

		Delete_client_by_fd(client_queue, fds[k].fd);

		printf("Client disconnected\n");
		pthread_mutex_unlock(&client_queue_lock);
	}	

	close(fds[k].fd);
	fds[k].fd = -1;
	pthread_mutex_unlock(&fds_lock);
}

void Init_server(int *listen_sock, struct sockaddr_in *serv_addr)
{
	int ret_val;

	lib_handler = dlopen("libshared.so", RTLD_LAZY);
	if(lib_handler == NULL)
	{
		fprintf(stderr,"dlopen() error: %s\n", dlerror());
		exit(EXIT_FAILURE);	// в случае ошибки можно, например, закончить работу программы
	};

	stat_file = fopen("stat", "w");
	if(stat_file == NULL)
	{
		printf("Can`t open stat file\n");
		exit(EXIT_FAILURE);
	}

	serv_addr->sin_family = AF_INET;
	serv_addr->sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr->sin_port = htons(PORT);

	*listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(*listen_sock == -1)
	{
		perror("cant`t create socket");
		exit(EXIT_FAILURE);
	}

	ret_val = bind(*listen_sock, (struct sockaddr*)serv_addr, sizeof(*serv_addr));
	if(ret_val != 0)
	{
		printf("Bind error\n");
		exit(EXIT_FAILURE);
	}

	ret_val = listen(*listen_sock, 30);
	if(ret_val == -1)
	{
		perror("cant`t listen socket");
		exit(EXIT_FAILURE);
	}

	signal(SIGINT, handle_sigtint); 
	signal(SIGUSR1, handle_sigusr1);

	ret_val = pthread_mutex_init(&fds_lock, NULL);
	if (ret_val != 0) 
	{
		printf("\n mutex init has failed\n"); 
		exit(EXIT_FAILURE); 
	}

	ret_val = pthread_mutex_init(&get_sock_lock, NULL);
	if (ret_val != 0) 
	{
		printf("\n mutex init has failed\n"); 
		exit(EXIT_FAILURE); 
	}

	ret_val = pthread_mutex_init(&ride_list_lock, NULL);
	if (ret_val != 0) 
	{
		printf("\n mutex init has failed\n"); 
		exit(EXIT_FAILURE); 
	}

	ret_val = pthread_mutex_init(&client_queue_lock, NULL);
	if (ret_val != 0) 
	{
		printf("\n mutex init has failed\n"); 
		exit(EXIT_FAILURE); 
	}

	ret_val = pthread_mutex_init(&reg_list_lock, NULL);
	if (ret_val != 0) 
	{
		printf("\n mutex init has failed\n"); 
		exit(EXIT_FAILURE); 
	}
}

void Compress_fds_array(struct pollfd fds[MAX_FD], int *nfds)
{
	int i, j;
	pthread_mutex_lock(&fds_lock);
	for(i = 0; i < *nfds; i++)
	{
		if(fds[i].fd == -1)
		{
			for(j = i; j < *nfds; j++)
			{
				fds[j].fd = fds[j+1].fd;
			}
		i--;
		(*nfds)--;
		}
	}
	pthread_mutex_unlock(&fds_lock);
}

int Handle_socket(int cur_sock, struct pollfd fds[MAX_FD], struct Fd_info fd_info[MAX_FD], struct Client_queue **client_queue, struct taxi_list **reg_list, int *num_of_av_taxi, int *nfds)
{
	int ret_val;
	struct Init_msg recv_buf;
	struct Client_info_msg c_send_buf;
	struct Client_info client_info;
	struct taxi_unit taxi;
	struct Ride_info ride_info;

	socklen_t c_buf_size = sizeof(c_send_buf);
	socklen_t buf_size = sizeof(recv_buf);

	int car_num;
	static int id_taxi_counter = 0;
	static int id_ride_counter = 0;
	float dist_to_client = 0;
	float dist_to_dest = 0;
	Point client_pos;
	Point client_dest;
	char client_choise;


	ret_val = recv(cur_sock, &recv_buf, buf_size, 0);
	if(ret_val < 0)
	{
		perror("recv() failed");
		return Connection_closed;
	}

	if(ret_val == 0)
	{
		return Connection_closed;
	}

	switch(recv_buf.type)
	{
		case Client_init:
			fd_info[cur_sock].type = Client;
			ret_val = recv(cur_sock, &client_dest, (socklen_t)sizeof(client_dest), 0);
			if(ret_val <= 0)
			{
				if(ret_val < 0)
				perror("recv() failed");
				return Connection_closed;
			}

			client_pos.x = recv_buf.x;
			client_pos.y = recv_buf.y;

			printf("type: Client_init, position: (%i, %i), destonation: (%i, %i)\n", recv_buf.x, recv_buf.y, client_dest.x, client_dest.y);
			while(TRUE)
			{
				ret_val = Get_nearest_driver(&dist_to_client, &taxi, *reg_list, &client_pos);
				if(ret_val == -1)
					break;
				else
				{
					pthread_mutex_lock(&reg_list_lock);
					ret_val = Get_taxi_status(*reg_list, taxi.id);
					if(ret_val == Waiting_for_order)
					{
						Set_taxi_status(*reg_list, taxi.id, Waiting_for_client_answer);
						(*num_of_av_taxi)--;
						pthread_mutex_unlock(&reg_list_lock);
						break;
					}
					pthread_mutex_unlock(&reg_list_lock);
				}
			}

			if(ret_val == 0)
			{
				dist_to_dest = Calc_dist(&client_pos, &client_dest);
				printf("nearest taxi(id):%i\n", taxi.id);
				printf("dist to client: %f\n", dist_to_client);
				printf("dist from client to dest: %f\n", dist_to_dest);

				// собираем всю инфу о поездке
				ride_info.ride_id = id_ride_counter;
				ride_info.client_fd = cur_sock;
				ride_info.price = (int)(round(dist_to_dest * 2) - *num_of_av_taxi);
				ride_info.car = taxi;
				ride_info.status = Car_waiting_for_answer;
				ride_info.start_pos = client_pos;
				ride_info.dest_pos = client_dest;
				ride_info.dist_to_client = dist_to_client;
				ride_info.dist_to_dest = dist_to_dest;
				ride_info.time_of_ride = (int)round(dist_to_dest);

				// привязываем клиенту и таксисту текущую поездку
				fd_info[cur_sock].cur_ride_id = ride_info.ride_id;
				fd_info[taxi.fd].cur_ride_id = ride_info.ride_id;

				// отправляем инфу клиенту
				c_send_buf.status = Taxi_found;
				c_send_buf.price = ride_info.price;
				c_send_buf.time_of_waiting = (int)round(dist_to_client);
				c_send_buf.time_of_ride = (int)round(dist_to_dest);
				ret_val = send(cur_sock, &c_send_buf, c_buf_size, 0);
				if(ret_val <= 0)
				{
					pthread_mutex_lock(&reg_list_lock);
					Set_taxi_status(*reg_list, taxi.id, Waiting_for_order);
					pthread_mutex_unlock(&reg_list_lock);
					perror("send() failed");
					return Connection_closed;
				}
				taxi.cur_ride_id = id_ride_counter;
				id_ride_counter++;

				// добавляем инфу о поездке
				pthread_mutex_lock(&ride_list_lock); 
				Push_in_list((List**)&cur_rides_list, &ride_info, sizeof(ride_info));
				pthread_mutex_unlock(&ride_list_lock);
			}

			else
			{
				// говорим клиенту подождать
				client_info.fd = cur_sock;
				client_info.start_pos.x = recv_buf.x;
				client_info.start_pos.y = recv_buf.y;
				client_info.dest_pos = client_dest;

				pthread_mutex_lock(&client_queue_lock);
				Push_in_queue(client_queue, &client_info, sizeof(client_info));
				pthread_mutex_unlock(&client_queue_lock);
				c_send_buf.status = No_taxi_now;
				ret_val = send(cur_sock, &c_send_buf, c_buf_size, 0);
				if(ret_val <= 0)
				{
					if(ret_val < 0)
						perror("send() failed");
					return Connection_closed;
				}
			}
			break;

		case Client_choise:
			ret_val = recv(cur_sock, &client_choise, (socklen_t)sizeof(char), 0);
			if(ret_val <= 0)
			{
				if(ret_val < 0)
					perror("recv() failed");
				return Connection_closed;
			}

			printf("client choise: %c\n", client_choise);
			if(client_choise == 'y') // если клиент согласен
			{
				Set_ride_status(cur_rides_list, cur_sock, Car_going_to_client);
				Get_ride_info_by_ride_id(cur_rides_list, &ride_info, fd_info[cur_sock].cur_ride_id);
				taxi = ride_info.car;
				pthread_mutex_lock(&reg_list_lock);
				Set_taxi_status(*reg_list, taxi.id, Going_to_client);
				pthread_mutex_unlock(&reg_list_lock);
				Send_info_to_taxi(&taxi, &ride_info);
			}

			else	// клиент отказался от поездки
			{
				printf("ride rejected\n");
				Get_ride_info_by_ride_id(cur_rides_list, &ride_info, fd_info[cur_sock].cur_ride_id);
				taxi = ride_info.car;
				pthread_mutex_lock(&reg_list_lock);
				Set_taxi_status(*reg_list, taxi.id, Waiting_for_order);
				(*num_of_av_taxi)++;
				pthread_mutex_unlock(&reg_list_lock);
			}
			break;

		case Taxi_waiting_for_client:
			Get_ride_info_by_taxi_id(cur_rides_list, &ride_info, fd_info[cur_sock].unit_id);
			Set_ride_status(cur_rides_list, ride_info.client_fd, Car_waiting_for_client);
			printf("taxi with id: %i waiting for client\n", ride_info.car.id);
			c_send_buf.status = Car_waiting_for_client;
			taxi = ride_info.car;
			ret_val = send(ride_info.client_fd, &c_send_buf, sizeof(c_send_buf), 0);
			pthread_mutex_lock(&reg_list_lock);
			Set_taxi_status(*reg_list, taxi.id, Waiting_for_client);
			pthread_mutex_unlock(&reg_list_lock);
			break;


		case Taxi_going_to_dest:
			Get_ride_info_by_taxi_id(cur_rides_list, &ride_info, fd_info[cur_sock].unit_id);
			Set_ride_status(cur_rides_list, ride_info.client_fd, Executing);
			c_send_buf.status = Executing;
			taxi = ride_info.car;
			printf("taxi with id: %i executes the order\n", ride_info.car.id);
			ret_val = send(ride_info.client_fd, &c_send_buf, sizeof(c_send_buf), 0);
			pthread_mutex_lock(&reg_list_lock);
			Set_taxi_status(*reg_list, taxi.id, Going_to_dest);
			pthread_mutex_unlock(&reg_list_lock);
			break;

		case Taxi_init:
		case Taxi_ride_done:
			(*num_of_av_taxi)++;
			if(recv_buf.type == Taxi_init)
			{
				ret_val = recv(cur_sock, &car_num, (socklen_t)sizeof(car_num), 0);
				if(ret_val <= 0)
				{
					if(ret_val < 0)
						perror("recv() failed");
					return Connection_closed;
				}

				ret_val = Get_taxi_by_car_num(*reg_list, &taxi, car_num);
				if(ret_val == -1)
				{
					printf("new unregistered car\n");
					taxi.id = id_taxi_counter;	// выдаем новый id
					taxi.car_num = car_num;
					taxi.fd = cur_sock;		// записываем номер сокета
					pthread_mutex_lock(&reg_list_lock);
					Push_in_list((List**)reg_list, &taxi, sizeof(taxi));
					pthread_mutex_unlock(&reg_list_lock);
					Show_reg_list(*reg_list);
					id_taxi_counter++;
					ret_val = send(cur_sock, &(taxi.id), sizeof(int), 0);
					if(ret_val <= 0)
					{
						if(ret_val < 0)
							perror("send() failed");
						return Connection_closed;
					}
				}

				else
				{

					printf("new registered car\n");
					Show_reg_list(*reg_list);
					taxi.fd = cur_sock;	// записываем номер сокета
					ret_val = send(cur_sock, &(taxi.id), sizeof(int), 0);
					if(ret_val <= 0)
					{
						if(ret_val < 0)
							perror("send() failed");
						return Connection_closed;
					}
					ret_val = Get_taxi_status(*reg_list, taxi.id);
					if(ret_val != Waiting_for_order)
						(*num_of_av_taxi)--;
				}

				fd_info[cur_sock].type = Taxi;
				fd_info[cur_sock].unit_id = taxi.id;
			}

			else if(recv_buf.type == Taxi_ride_done) 
			{
				printf("taxi with id: %i is back\n", fd_info[cur_sock].unit_id);
				pthread_mutex_lock(&reg_list_lock);
				Set_taxi_status(*reg_list, taxi.id, Waiting_for_order);
				pthread_mutex_unlock(&reg_list_lock);
				Get_ride_info_by_taxi_id(cur_rides_list, &ride_info, fd_info[cur_sock].unit_id);
				fd_info[cur_sock].cur_ride_id = -1;
				taxi = ride_info.car;
				c_send_buf.status = Done;
				ret_val = send(ride_info.client_fd, &c_send_buf, sizeof(c_send_buf), 0);
				if(ret_val <= 0)
				{
					Close_connection(ride_info.client_fd, fds, &fd_info[ride_info.client_fd], reg_list, client_queue, num_of_av_taxi);
					Compress_fds_array(fds, nfds);	
				}
				Print_in_stat_file(&ride_info);
				pthread_mutex_lock(&ride_list_lock);
				Delete_ride_by_taxi_id(&cur_rides_list, taxi.id);
				pthread_mutex_unlock(&ride_list_lock);
			}

			taxi.pos.x = recv_buf.x;
			taxi.pos.y = recv_buf.y;
			Set_taxi_pos(*reg_list, taxi.id, taxi.pos);

			printf("type: Taxi, id:%i, position:(%i, %i)\n", taxi.id, recv_buf.x, recv_buf.y);
			ret_val = Get_ride_info_by_taxi_id(cur_rides_list, &ride_info, taxi.id);

			// если таксист уже в поездке
			if(ret_val == 0) 
			{
				printf(" - TAXI IS ALREADY ON THE RIDE\n");
				printf("position: (%i, %i), destonation: (%i, %i)\n", ride_info.start_pos.x, ride_info.start_pos.y, ride_info.dest_pos.x, ride_info.dest_pos.y);
				printf("taxi_id = %i, t_id_ride = %i\n", taxi.id, ride_info.car.id);

				// берем инфу из записи о поездке и снова отправляем таксисту
				taxi = ride_info.car;

				// заново отправляем инфу таксисту
				Send_info_to_taxi(&taxi, &ride_info);
				(*num_of_av_taxi)--;
			}

			else
			{
				fd_info[cur_sock].cur_ride_id = -1;
				pthread_mutex_lock(&client_queue_lock);
				ret_val = Pop_client_info_from_queue(client_queue, &client_info);
				pthread_mutex_unlock(&client_queue_lock);
				// если есть клиенты в очереди ожидания
				if(ret_val != -1)
				{
					printf(" - GOT CLIENT IN QUEUE\n");
					// убираем такси из списка свободных
					pthread_mutex_lock(&reg_list_lock);
					Set_taxi_status(*reg_list, taxi.id, Waiting_for_client_answer);
					pthread_mutex_unlock(&reg_list_lock);
					(*num_of_av_taxi)--;
					dist_to_client = Calc_dist(&(client_info.start_pos), &(taxi.pos));
					dist_to_dest = Calc_dist(&(client_info.start_pos), &(client_info.dest_pos));
					printf("nearest taxi(id):%i\n", taxi.id);
					printf("dist to client: %f\n", dist_to_client);
					printf("dist from client to dest: %f\n", dist_to_dest);
					printf("taxi_id = %i\n", taxi.id);

					// собираем всю инфу о поездке
					ride_info.ride_id = id_ride_counter;
					ride_info.client_fd = client_info.fd;
					ride_info.price = (int)(round(dist_to_dest * 2) - *num_of_av_taxi);
					ride_info.car = taxi;
					ride_info.status = Car_waiting_for_answer;
					ride_info.start_pos = client_info.start_pos;
					ride_info.dest_pos = client_info.dest_pos;
					ride_info.dist_to_client = dist_to_client;
					ride_info.dist_to_dest = dist_to_dest;
					ride_info.time_of_ride = (int)round(dist_to_dest);

					printf("n_taxi_id = %i\n", taxi.id);

					// привязываем таксисту и клиенту текущую поездку
					fd_info[cur_sock].cur_ride_id = ride_info.ride_id;
					fd_info[client_info.fd].cur_ride_id = ride_info.ride_id;

					printf("position: (%i, %i), destonation: (%i, %i)\n", ride_info.start_pos.x, ride_info.start_pos.y, ride_info.dest_pos.x, ride_info.dest_pos.y);

					// отправляем инфу клиенту
					c_send_buf.status = Done;
					c_send_buf.price = ride_info.price;
					c_send_buf.time_of_waiting = (int)round(dist_to_client);
					c_send_buf.time_of_ride = (int)round(dist_to_dest);
					ret_val = send(client_info.fd, &c_send_buf, c_buf_size, 0);
					if(ret_val <= 0)
					{
						pthread_mutex_lock(&reg_list_lock);
						Set_taxi_status(*reg_list, taxi.id, Waiting_for_order);
						pthread_mutex_unlock(&reg_list_lock);
						return Connection_closed;
					}
					taxi.cur_ride_id = id_ride_counter;
					id_ride_counter++;

					// добавляем инфу о поездке
					pthread_mutex_lock(&ride_list_lock);
					Push_in_list((List**)&cur_rides_list, &ride_info, sizeof(ride_info));
					pthread_mutex_unlock(&ride_list_lock);
				}

				else
				{
					pthread_mutex_lock(&reg_list_lock);
					Set_taxi_status(*reg_list, taxi.id, Waiting_for_order);
					pthread_mutex_unlock(&reg_list_lock);
				}
			}
			break;
		}
	return Connection_is_active;
}

void *Handle_fds_array(void *t_args)
{
	int cur_sock;
	int ret_val;

	struct Thread_args *args = (struct Thread_args *)t_args;

	int *nfds = args->nfds;
	struct pollfd *fds = args->fds;
	struct Fd_info *fd_info = args->fd_info;

	static struct Client_queue *client_queue = NULL;
	static struct taxi_list *reg_list = NULL;

	static int num_of_av_taxi = 0;

	while(end_server == FALSE)
	{

		pthread_mutex_lock(&get_sock_lock);
		ret_val = Pop_sock_from_queue((args->sock_queue), &cur_sock);
		pthread_mutex_unlock(&get_sock_lock); 
		if(ret_val == -1)
		{
			continue;
			usleep(10000);
		}
		
		/*
		pthread_mutex_lock(&get_sock_lock);
		while(Pop_sock_from_queue((args->sock_queue), &cur_sock) == -1)
			pthread_cond_wait(&cond, &get_sock_lock);
		pthread_mutex_unlock(&get_sock_lock);
		*/

		ret_val = Handle_socket(cur_sock, fds, fd_info, &client_queue, &reg_list, &num_of_av_taxi, nfds);
		if(ret_val == Connection_closed)
		{
			Close_connection(cur_sock, fds, &fd_info[cur_sock], &reg_list, &client_queue, &num_of_av_taxi);
			Compress_fds_array(fds, nfds);
		}

		printf("Current num of taxi = %i\n", num_of_av_taxi);
		Show_client_queue(client_queue);
		printf(" - - - - - - - - - - - - - - - - - - - \n");
		fd_info[cur_sock].is_waititng = TRUE;
	}
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	printf("mypid is %i\n", getpid());

	if(argc > 1)
	{
		if(strcmp(argv[1], "-d") == 0)
			daemon(1, 0);

		log_file = fopen("log", "w");
		if(log_file == NULL)
		{
			printf("Can`t open log file\n");
			exit(EXIT_FAILURE);
		}

		dup2(fileno(log_file), 1);
	}

	int i;
	int listen_sock = 0, accept_sock = 0;
	int ret_val;
	struct sockaddr_in serv_addr, client_addr;

	int timeout;
	int nfds = 1;
	struct pollfd fds[MAX_FD];
	struct Fd_info fd_info[MAX_FD];

	socklen_t cli_addr_len = sizeof(client_addr);

	struct Sock_queue *sock_queue = NULL;

	struct Thread_args t_args;
	t_args.fds = fds;
	t_args.fd_info = fd_info;
	t_args.sock_queue = &sock_queue;
	t_args.nfds = &nfds;

	void *status;

	Init_server(&listen_sock, &serv_addr);

	fds[0].fd = listen_sock;
	fds[0].events = POLLIN;
	fd_info[listen_sock].type = Listen_sock;
	timeout = -1;	// бесконечный таймер ожидания

	for(i = 0; i < NUM_THREADS; i++)
	{
		ret_val = pthread_create(&tid[i], NULL, Handle_fds_array, (void *)&t_args);
		if(ret_val != 0) 
		{
			printf("Can`t create thread\n");
			exit(EXIT_FAILURE);
		}
	}

	while(end_server == FALSE)
	{
		ret_val = poll(fds, nfds, timeout);
		if(ret_val < 0)
		{
			if(errno == EINTR)	// если poll заблочился сигналом, пробуем снова
				ret_val = poll(fds, nfds, timeout);
			else
			{
				perror("poll() failed");
				exit(EXIT_FAILURE);
			}
		}

		for(i = 0; i < nfds; i++)
		{
			if(fds[i].revents == POLLHUP || fds[i].revents == POLLERR)
			{
				Compress_fds_array(fds, &nfds);
				if(fds[i].revents == POLLERR)
					printf("Error! revents = %d\n", fds[i].revents);
			}

			else if(fds[i].revents != 0)
			{
				if(i == 0)	// номер слушающего сокета
				{	
					accept_sock = accept(fds[i].fd, (struct sockaddr*)&client_addr, &cli_addr_len);
					if(accept_sock == -1)
					{
						perror("Can`t accept");
						exit(EXIT_FAILURE);
					}
					fd_info[accept_sock].is_waititng = TRUE;
					fds[nfds].fd = accept_sock;
					fds[nfds].events = POLLIN;
					nfds++;
				}

				else
				{
					if(fd_info[fds[i].fd].is_waititng == TRUE && Check_sock_in_queue(sock_queue, fds[i].fd) == -1)
					{
						fd_info[fds[i].fd].is_waititng = FALSE;
						pthread_mutex_lock(&get_sock_lock);
						Push_sock_in_queue(&sock_queue, &fds[i].fd);
						pthread_mutex_unlock(&get_sock_lock);
						//pthread_cond_signal(&cond);
					}
				}
			}
		}
		usleep(10000);
	}


	for (i = 0; i < NUM_THREADS; i++)
	{
		ret_val = pthread_join(tid[i], &status);
		if(ret_val != 0 && ret_val != 3)
		{
			printf("Can`t join thread\n");
			exit(EXIT_FAILURE);
		}
	}
	
	// закрываем все сокеты
	for(i = 0; i < nfds; i++) 
	{
		if(fds[i].fd >= 0)
			close(fds[i].fd);
	}

	close(listen_sock);
	fclose(stat_file);
	return 0;
}