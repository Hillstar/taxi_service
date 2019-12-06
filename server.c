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
pthread_mutex_t taxi_list_lock;

int Check_client_in_queue(int taxi_id, struct Client_queue **client_queue, struct Taxi_list **taxi_list, 
			struct pollfd fds[MAX_FD], struct Fd_info fd_info[MAX_FD], int *nfds);

void handle_sigusr1(int sig)
{
	Show_curent_rides(cur_rides_list);
}

void handle_sigtint(int sig)
{
	int ret_val;
	
	end_server = TRUE;
	if(stat_file != NULL)
		fclose(stat_file);
	if(log_file != NULL)
		fclose(log_file);
	dlclose(lib_handler);

	int i;
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

	void *status;
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
	pthread_mutex_destroy(&taxi_list_lock); 
	exit(EXIT_SUCCESS);
}

void Print_in_stat_file(struct Ride_info *info)
{
	if(stat_file == NULL)
	{
		printf("Can`t print, stat file is not open!\n");
		return;
	}

	fprintf(stat_file, "clint id:%i\n", info->client_id);
	fprintf(stat_file, "taxi id:%i\n", info->taxi_id);
	fprintf(stat_file, "start position: (%i, %i)\n", info->start_pos.x, info->start_pos.y);
	fprintf(stat_file, "dest position: (%i, %i)\n", info->dest_pos.x, info->dest_pos.y);
	fprintf(stat_file, "price:%i\n", info->price);
	fprintf(stat_file, "status: DONE\n");
	fprintf(stat_file, " - - - - - - - - - - - - - -\n\n");
}

int Send_info_to_taxi(int taxi_id, struct Ride_info *ride_info, struct Taxi_list **taxi_list)
{
	int ret_val;

	struct Taxi_unit taxi;
	pthread_mutex_lock(&taxi_list_lock);
	ret_val = Get_taxi_by_id(*taxi_list, &taxi, taxi_id);
	pthread_mutex_unlock(&taxi_list_lock);

	if(ret_val == -1 || taxi.status == Offline)
		return -1;

	printf("send to taxi(id: %i)\n", taxi.id);
	struct Taxi_info_msg send_buf;
	send_buf.dest = ride_info->dest_pos;
	send_buf.time_of_ride = ride_info->time_of_ride;
	send_buf.price = ride_info->price;
	send_buf.dist_to_client = ride_info->dist_to_client;
	send_buf.dist_to_dest = ride_info->dist_to_dest;
	send_buf.status = ride_info->status;

	ret_val = send(taxi.fd, &send_buf, sizeof(send_buf), 0);
	return ret_val;
}

int Get_sock_by_id(struct Fd_info fd_info[MAX_FD], int sock_type, int id)
{
	int sockIndex = 0;

	while(sockIndex < MAX_FD)
	{
		if(fd_info[sockIndex].type == sock_type && fd_info[sockIndex].unit_id == id)
		{
			return sockIndex;
		}
		sockIndex++;
	}

	return -1;
}

float Calc_dist(struct Point *start_pos, struct Point *dest_pos)
{
	sleep(3);	// долго считаем
	return sqrt(pow((dest_pos->x - start_pos->x), 2) + pow((dest_pos->y - start_pos->y), 2));
}

int Get_nearest_driver(float *dist, struct Taxi_unit *nearest_driver, struct Taxi_list *taxi_list, struct Point *client_pos)
{
	int taxi_found = FALSE;
	struct Taxi_list *temp;
	float new_dist, min_dist = 10000;
	struct Taxi_unit taxi, new_taxi;

	if(taxi_list == NULL)
		return -1;
	pthread_mutex_lock(&taxi_list_lock);
	temp = taxi_list;
	if(temp != NULL)
	{
		new_taxi = *(temp->car);
	}
	pthread_mutex_unlock(&taxi_list_lock);

	// находим ближайшего водителя
	while(temp != NULL)
	{
		if(new_taxi.status == Waiting_for_order)
		{
			new_dist = Calc_dist(&(new_taxi.pos), client_pos);
			if(new_dist < min_dist)
			{
				taxi_found = TRUE;
				min_dist = new_dist;
				taxi = new_taxi;
			}
		}
		pthread_mutex_lock(&taxi_list_lock);
		temp = temp->next;
		if(temp != NULL)
		{
			new_taxi = *(temp->car);
		}
		pthread_mutex_unlock(&taxi_list_lock);
	}

	if(taxi_found == FALSE)
		return -1;
	*dist = min_dist;
	(*nearest_driver) = taxi;
	return 0;
}

void Close_connection(int cur_sock, struct pollfd fds[MAX_FD], struct Fd_info fd_info[MAX_FD], int *nfds, 
			struct Taxi_list **taxi_list, struct Client_queue **client_queue)
{
	int ret_val;
	struct Ride_info info_buf;
	struct Taxi_unit taxi;
	int need_to_check_queue = False;
	pthread_mutex_lock(&fds_lock);

	// ищем в fds поле с этим сокетом
	int k = 0;
	while(fds[k].fd != cur_sock && k < MAX_FD)
		k++;

	if(k == MAX_FD)
	{
		printf("Can`t close\n");
		pthread_mutex_unlock(&fds_lock);
		return;
	}

	if(fd_info[cur_sock].type == Taxi)
	{
		printf("Taxi (id:%i) fd:%i disconnected\n", fd_info[cur_sock].unit_id, cur_sock);
		//Set_taxi_fd(*taxi_list, fd_info[cur_sock].unit_id, -1);
		ret_val = Get_ride_info_by_taxi_id(cur_rides_list, &info_buf, fd_info[cur_sock].unit_id);
		if(ret_val == -1 || info_buf.status == Car_waiting_for_answer)	// если текущей поездки нет
		{
			//Delete_taxi_by_fd(taxi_list, cur_sock);
			Set_taxi_status(*taxi_list, fd_info[cur_sock].unit_id, Offline);
		}
	}

	else
	{
		ret_val = Get_ride_info_by_client_fd(cur_rides_list, &info_buf, cur_sock);
		if(ret_val == 0)
		{
			if(info_buf.status == Car_waiting_for_answer)
			{
				pthread_mutex_lock(&taxi_list_lock);
				ret_val = Get_taxi_by_id(*taxi_list, &taxi, info_buf.taxi_id);
				pthread_mutex_unlock(&taxi_list_lock);
				if(ret_val != -1 && taxi.status != Offline)
				{
					pthread_mutex_lock(&taxi_list_lock);
					Set_taxi_status(*taxi_list, taxi.id, Waiting_for_order);
					pthread_mutex_unlock(&taxi_list_lock);

					pthread_mutex_lock(&ride_list_lock);
					Delete_ride_by_client_id(&cur_rides_list, fd_info[cur_sock].unit_id);
					pthread_mutex_unlock(&ride_list_lock);

					need_to_check_queue = True;
					//Check_client_in_queue(taxi.id, client_queue, taxi_list, fds, fd_info, nfds);
				}

				else
				{
					pthread_mutex_lock(&ride_list_lock);
					Delete_ride_by_client_id(&cur_rides_list, fd_info[cur_sock].unit_id);
					pthread_mutex_unlock(&ride_list_lock);
				}
			}

			else
			{
				pthread_mutex_lock(&ride_list_lock);
				Set_ride_client_fd(cur_rides_list, fd_info[cur_sock].unit_id, -1);
				pthread_mutex_unlock(&ride_list_lock);
			}
		}

		pthread_mutex_lock(&client_queue_lock);
		Delete_client_by_id(client_queue, fd_info[cur_sock].unit_id);
		pthread_mutex_unlock(&client_queue_lock);
		printf("Client(id: %i) fd:%i disconnected\n", fd_info[cur_sock].unit_id, cur_sock);
	}	

	close(fds[k].fd);
	fd_info[cur_sock].type = -1;
	fd_info[cur_sock].unit_id = -1;
	fd_info[cur_sock].is_waititng = TRUE;
	fds[k].fd = -1;
	pthread_mutex_unlock(&fds_lock);

	if(need_to_check_queue == True)
		Check_client_in_queue(taxi.id, client_queue, taxi_list, fds, fd_info, nfds);
}

void Init_server(int *listen_sock, struct sockaddr_in *serv_addr, struct Fd_info fd_info[MAX_FD])
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

	ret_val = pthread_mutex_init(&taxi_list_lock, NULL);
	if (ret_val != 0) 
	{
		printf("\n mutex init has failed\n"); 
		exit(EXIT_FAILURE); 
	}

	int i;
	for(i = 0; i < MAX_FD; i++)
	{
		fd_info[i].type = -1;
		fd_info[i].unit_id = -1;
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

int Lookup_for_taxi(int cur_sock, struct Client_queue **client_queue, struct Taxi_list **taxi_list, 
			Point client_pos, Point client_dest, int client_id)
{
	int ret_val;
	struct Taxi_unit taxi;
	float dist_to_client = 0;

	while(TRUE)
	{
		ret_val = Get_nearest_driver(&dist_to_client, &taxi, *taxi_list, &client_pos);
		if(ret_val == -1)
			break;
		else
		{
			pthread_mutex_lock(&taxi_list_lock);
			ret_val = Get_taxi_status(*taxi_list, taxi.id);
			if(ret_val == Waiting_for_order)
			{
				ret_val = Set_taxi_status(*taxi_list, taxi.id, Waiting_for_client_answer);
				pthread_mutex_unlock(&taxi_list_lock);
				if(ret_val == -1)
				{
					printf("taxi is busy, trying get new one\n");
					continue;
				}

				else
					break;
			}
			pthread_mutex_unlock(&taxi_list_lock);
		}
	}

	if(ret_val == 0)
	{
		float dist_to_dest = Calc_dist(&client_pos, &client_dest);
		printf("client(id:%i, fd:%i)\n", client_id, cur_sock);
		printf("nearest taxi(id:%i, fd:%i)\n", taxi.id, taxi.fd);
		printf("dist to client: %f\n", dist_to_client);
		printf("dist from client to dest: %f\n", dist_to_dest);

		// собираем всю инфу о поездке
		struct Ride_info ride_info;
		ride_info.client_fd = cur_sock;
		ride_info.client_id = client_id;
		ride_info.price = (int)(round(dist_to_dest * 2));
		ride_info.taxi_id = taxi.id;
		ride_info.status = Car_waiting_for_answer;
		ride_info.start_pos = client_pos;
		ride_info.dest_pos = client_dest;
		ride_info.dist_to_client = dist_to_client;
		ride_info.dist_to_dest = dist_to_dest;
		ride_info.time_of_ride = (int)round(dist_to_dest);

		// отправляем инфу клиенту
		struct Client_info_msg c_send_buf;
		c_send_buf.status = Taxi_found;
		c_send_buf.price = ride_info.price;
		c_send_buf.time_of_waiting = (int)round(dist_to_client);
		c_send_buf.time_of_ride = (int)round(dist_to_dest);
		socklen_t c_buf_size = sizeof(c_send_buf);
		ret_val = send(cur_sock, &c_send_buf, c_buf_size, 0);
		if(ret_val <= 0)
		{
			pthread_mutex_lock(&taxi_list_lock);
			Set_taxi_status(*taxi_list, taxi.id, Waiting_for_order);
			pthread_mutex_unlock(&taxi_list_lock);
			perror("send() failed");
			return Connection_closed;
		}

		// добавляем инфу о поездке
		pthread_mutex_lock(&ride_list_lock); 
		Push_in_list((List**)&cur_rides_list, &ride_info, sizeof(ride_info));
		pthread_mutex_unlock(&ride_list_lock);
	}

	else
	{
		printf("Push client in queue\n");
		// заносим клиента в очередь ожидания
		pthread_mutex_lock(&client_queue_lock);
		struct Client_info client_info;
		client_info.fd = cur_sock;
		client_info.id = client_id;
		client_info.start_pos.x = client_pos.x;
		client_info.start_pos.y = client_pos.y;
		client_info.dest_pos = client_dest;
		Push_client_in_queue(client_queue, &client_info, sizeof(client_info));
		pthread_mutex_unlock(&client_queue_lock);

		// говорим клиенту подождать
		struct Client_info_msg c_send_buf;
		c_send_buf.status = No_taxi_now;
		socklen_t c_buf_size = sizeof(c_send_buf);
		ret_val = send(cur_sock, &c_send_buf, c_buf_size, 0);
		if(ret_val <= 0)
		{
			perror("send() failed");
			return Connection_closed;
		}
	}
	return Connection_is_active;
}

int Check_client_in_queue(int taxi_id, struct Client_queue **client_queue, struct Taxi_list **taxi_list, 
			struct pollfd fds[MAX_FD], struct Fd_info fd_info[MAX_FD], int *nfds)
{
	int ret_val;

	struct Taxi_unit taxi;
	pthread_mutex_lock(&taxi_list_lock);
	ret_val = Get_taxi_by_id(*taxi_list, &taxi, taxi_id);
	pthread_mutex_unlock(&taxi_list_lock);
	if(ret_val == -1 || taxi.status != Waiting_for_order)
		return -1;

	struct Client_info client_info;
	while(TRUE)
	{
		pthread_mutex_lock(&client_queue_lock);
		ret_val = Pop_client_info_from_queue(client_queue, &client_info);
		pthread_mutex_unlock(&client_queue_lock);
		// если есть клиенты в очереди ожидания
		if(ret_val != -1) 
		{
			pthread_mutex_lock(&taxi_list_lock);
			Set_taxi_status(*taxi_list, taxi.id, Waiting_for_client_answer);
			pthread_mutex_unlock(&taxi_list_lock);
			printf(" - GOT CLIENT IN QUEUE\n");

			float dist_to_client = Calc_dist(&(client_info.start_pos), &(taxi.pos));
			float dist_to_dest = Calc_dist(&(client_info.start_pos), &(client_info.dest_pos));

			printf("client(id:%i, fd:%i)\n", client_info.id, client_info.fd);
			printf("nearest taxi(id:%i, fd:%i)\n", taxi.id, taxi.fd);
			printf("dist to client: %f\n", dist_to_client);
			printf("dist from client to dest: %f\n", dist_to_dest);
			printf("taxi_id = %i\n", taxi.id);

			// собираем всю инфу о поездке
			struct Ride_info ride_info;
			ride_info.client_fd = client_info.fd;
			ride_info.client_id = client_info.id;
			ride_info.price = (int)(round(dist_to_dest * 2));
			ride_info.taxi_id = taxi.id;
			ride_info.status = Car_waiting_for_answer;
			ride_info.start_pos = client_info.start_pos;
			ride_info.dest_pos = client_info.dest_pos;
			ride_info.dist_to_client = dist_to_client;
			ride_info.dist_to_dest = dist_to_dest;
			ride_info.time_of_ride = (int)round(dist_to_dest);

			printf("position: (%i, %i), destonation: (%i, %i)\n", ride_info.start_pos.x, ride_info.start_pos.y, ride_info.dest_pos.x, ride_info.dest_pos.y);

			// отправляем инфу клиенту
			struct Client_info_msg c_send_buf;
			socklen_t c_buf_size = sizeof(c_send_buf);
			c_send_buf.status = Taxi_found;
			c_send_buf.price = ride_info.price;
			c_send_buf.time_of_waiting = (int)round(dist_to_client);
			c_send_buf.time_of_ride = (int)round(dist_to_dest);

			// если пока мы считали, кто-то занял этот сокет
			if(fd_info[client_info.fd].type != Client || fd_info[client_info.fd].unit_id != client_info.id) 
			{
				printf("Client(id:%i) already closed\n", client_info.id);
				pthread_mutex_lock(&taxi_list_lock);
				Set_taxi_status(*taxi_list, taxi.id, Waiting_for_order);
				pthread_mutex_unlock(&taxi_list_lock);
				continue;
			}

			ret_val = Check_ride_by_taxi_id(cur_rides_list, taxi.id); 
			if(ret_val == 0) // если кто-то уже занял это такси, возвращаем клиента в очередь
			{
				pthread_mutex_lock(&taxi_list_lock);
				Set_taxi_status(*taxi_list, taxi_id, Waiting_for_order);
				pthread_mutex_unlock(&taxi_list_lock);

				pthread_mutex_lock(&client_queue_lock);
				Push_client_in_queue(client_queue, &client_info, sizeof(client_info));
				pthread_mutex_unlock(&client_queue_lock);
				return 0;
			}

			ret_val = send(client_info.fd, &c_send_buf, c_buf_size, 0);
			if(ret_val <= 0)
			{
				pthread_mutex_lock(&taxi_list_lock);
				Set_taxi_status(*taxi_list, taxi.id, Waiting_for_order);
				pthread_mutex_unlock(&taxi_list_lock);
				Close_connection(client_info.fd, fds, fd_info, nfds, taxi_list, client_queue);
				Compress_fds_array(fds, nfds);
			}

			else
			{
				// добавляем инфу о поездке
				pthread_mutex_lock(&ride_list_lock);
				Push_in_list((List**)&cur_rides_list, &ride_info, sizeof(ride_info));
				pthread_mutex_unlock(&ride_list_lock);
				return 0;
			}
		}

		else
			break;
	}

	return -1;
}

int Handle_socket(int cur_sock, struct pollfd fds[MAX_FD], struct Fd_info fd_info[MAX_FD], 
			struct Client_queue **client_queue, struct Taxi_list **taxi_list, int *nfds)
{
	int ret_val;

	struct Init_msg recv_buf;
	socklen_t buf_size = sizeof(recv_buf);

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

	struct Taxi_unit taxi;
	struct Ride_info ride_info;
	
	Point client_pos;
	Point client_dest;

	int client_id;
	int taxi_id;
	char client_choise;
	int connection_duplicate = False;

	struct Client_info_msg c_send_buf;
	socklen_t c_buf_size = sizeof(c_send_buf);

	switch(recv_buf.type)
	{
		case Client_init:
			client_id = recv_buf.id;
			pthread_mutex_lock(&fds_lock);
			ret_val = Get_sock_by_id(fd_info, Client, client_id);
			pthread_mutex_unlock(&fds_lock);
			printf("ret_val = %i\n", ret_val);
			if(ret_val != -1)
			{
				connection_duplicate = True;
				send(cur_sock, &connection_duplicate, sizeof(connection_duplicate), 0);
				return Connection_is_duplicate;
			}
			else
			{
				ret_val = send(cur_sock, &connection_duplicate, sizeof(connection_duplicate), 0);
				if(ret_val <= 0)
				{
					perror("send() failed");
					return Connection_closed;
				}
			}
			
			ret_val = recv(cur_sock, &client_dest, sizeof(client_dest), 0);
			if(ret_val <= 0)
			{
				perror("recv() failed");
				return Connection_closed;
			}

			fd_info[cur_sock].type = Client;
			fd_info[cur_sock].unit_id = client_id;

			client_pos.x = recv_buf.x;
			client_pos.y = recv_buf.y;

			printf("INIT client(id: %i), fd:%i, position: (%i, %i), destonation: (%i, %i)\n", client_id, cur_sock, recv_buf.x, recv_buf.y, client_dest.x, client_dest.y);

			ret_val = Get_ride_info_by_client_id(cur_rides_list, &ride_info, client_id);
			if(ret_val != -1)
			{
				// снова отправляем инфу клиенту
				printf(" - CLIENT IS ALREADY ON THE RIDE\n");
				Set_ride_client_fd(cur_rides_list, client_id, cur_sock);
				c_send_buf.status = ride_info.status;
				c_send_buf.price = ride_info.price;
				c_send_buf.time_of_waiting = (int)round(ride_info.dist_to_client);
				c_send_buf.time_of_ride = (int)round(ride_info.dist_to_dest);

				ret_val = send(cur_sock, &c_send_buf, c_buf_size, 0);
				if(ret_val <= 0)
				{
					perror("send() failed");
					return Connection_closed;
				}
			}

			else
			{
				ret_val = Lookup_for_taxi(cur_sock, client_queue, taxi_list, client_pos, client_dest, client_id);
				if(ret_val == Connection_closed)
					return Connection_closed;
			}
			break;

		case Client_choise:
			client_id = recv_buf.id;
			ret_val = recv(cur_sock, &client_choise, sizeof(client_choise), 0);
			if(ret_val <= 0)
			{
				perror("recv() failed");
				return Connection_closed;
			}

			fd_info[cur_sock].type = Client;
			fd_info[cur_sock].unit_id = client_id;

			printf("client(id:%i), fd:%i, choise: %c\n", fd_info[cur_sock].unit_id, cur_sock, client_choise);
			if(client_choise == 'y')	// если клиент согласен
			{
				Set_ride_status(cur_rides_list, fd_info[cur_sock].unit_id, Car_going_to_client);
				Set_ride_client_fd(cur_rides_list, client_id, cur_sock);
				Get_ride_info_by_client_id(cur_rides_list, &ride_info, fd_info[cur_sock].unit_id);

				pthread_mutex_lock(&taxi_list_lock);
				ret_val = Get_taxi_by_id(*taxi_list, &taxi, ride_info.taxi_id);
				pthread_mutex_unlock(&taxi_list_lock);
				if(ret_val != -1 && taxi.status != Offline)
				{
					ret_val = Send_info_to_taxi(ride_info.taxi_id, &ride_info, taxi_list);
				}

				if(ret_val <= 0)
				{
					printf("ride rejected\n");
					pthread_mutex_lock(&ride_list_lock);
					Delete_ride_by_client_id(&cur_rides_list, client_id);
					pthread_mutex_unlock(&ride_list_lock);

					// отправляем инфу клиенту
					c_send_buf.status = Rejected;
					ret_val = send(cur_sock, &c_send_buf, c_buf_size, 0);
					if(ret_val <= 0)
					{
						perror("send() failed");
						return Connection_closed;
					}

					ret_val = recv(cur_sock, &client_dest, sizeof(client_dest), 0);
					if(ret_val <= 0)
					{
						perror("recv() failed");
						return Connection_closed;
					}

					else
					{
						client_pos.x = recv_buf.x;
						client_pos.y = recv_buf.y;
						client_id = fd_info[cur_sock].unit_id;
						ret_val = Lookup_for_taxi(cur_sock, client_queue, taxi_list, client_pos, client_dest, client_id);
						if(ret_val == Connection_closed)
							return Connection_closed;
					}
				}

				else
				{
					pthread_mutex_lock(&taxi_list_lock);
					Set_taxi_status(*taxi_list, taxi.id, Going_to_client);
					pthread_mutex_unlock(&taxi_list_lock);
				}
			}

			else	// клиент отказался от поездки
			{
				printf("ride rejected by client\n");
				pthread_mutex_lock(&ride_list_lock);
				Get_ride_info_by_client_id(cur_rides_list, &ride_info, fd_info[cur_sock].unit_id);
				pthread_mutex_unlock(&ride_list_lock);

				pthread_mutex_lock(&taxi_list_lock);
				ret_val = Get_taxi_status(*taxi_list, ride_info.taxi_id);
				if(ret_val != Offline && ret_val != -1)
					Set_taxi_status(*taxi_list, ride_info.taxi_id, Waiting_for_order);
				pthread_mutex_unlock(&taxi_list_lock);

				pthread_mutex_lock(&ride_list_lock);
				Delete_ride_by_client_id(&cur_rides_list, client_id);
				pthread_mutex_unlock(&ride_list_lock);

				pthread_mutex_lock(&taxi_list_lock);
				ret_val = Get_taxi_by_id(*taxi_list, &taxi, ride_info.taxi_id);
				pthread_mutex_unlock(&taxi_list_lock);
				if(ret_val != -1 && taxi.status != Offline)
				{
					Check_client_in_queue(taxi.id, client_queue, taxi_list, fds, fd_info, nfds);
				}
			}
			break;

		case Taxi_waiting_for_client:
			taxi_id = recv_buf.id;
			Get_ride_info_by_taxi_id(cur_rides_list, &ride_info, fd_info[cur_sock].unit_id);
			Set_ride_status(cur_rides_list, ride_info.client_id, Car_waiting_for_client);
			printf("taxi (id:%i, fd:%i) waiting for client (id:%i, fd:%i)\n", ride_info.taxi_id, cur_sock, ride_info.client_id, ride_info.client_fd);
			c_send_buf.status = Car_waiting_for_client;
			ret_val = send(ride_info.client_fd, &c_send_buf, sizeof(c_send_buf), 0);
			if(ret_val <= 0)
			{
				Close_connection(ride_info.client_fd, fds, fd_info, nfds, taxi_list, client_queue);
				Compress_fds_array(fds, nfds);	
			}
			pthread_mutex_lock(&taxi_list_lock);
			Set_taxi_status(*taxi_list, taxi_id, Waiting_for_client);
			pthread_mutex_unlock(&taxi_list_lock);
			break;

		case Taxi_going_to_dest:
			taxi_id = recv_buf.id;
			Get_ride_info_by_taxi_id(cur_rides_list, &ride_info, fd_info[cur_sock].unit_id);
			Set_ride_status(cur_rides_list, ride_info.client_id, Executing);
			printf("taxi (id:%i, fd:%i) executes for client (id:%i, fd:%i)\n", ride_info.taxi_id, cur_sock, ride_info.client_id, ride_info.client_fd);
			c_send_buf.status = Executing;
			ret_val = send(ride_info.client_fd, &c_send_buf, sizeof(c_send_buf), 0);
			if(ret_val <= 0)
			{
				Close_connection(ride_info.client_fd, fds, fd_info, nfds, taxi_list, client_queue);
				Compress_fds_array(fds, nfds);	
			}
			pthread_mutex_lock(&taxi_list_lock);
			Set_taxi_status(*taxi_list, taxi_id, Going_to_dest);
			pthread_mutex_unlock(&taxi_list_lock);
			break;

		case Taxi_init:
		case Taxi_ride_done:
			taxi_id = recv_buf.id;
			if(recv_buf.type == Taxi_init)
			{
				pthread_mutex_lock(&fds_lock);
				ret_val = Get_sock_by_id(fd_info, Taxi, taxi_id);
				pthread_mutex_unlock(&fds_lock);
				if(ret_val != -1)
				{
					connection_duplicate = True;
					send(cur_sock, &connection_duplicate, sizeof(connection_duplicate), 0);
					return Connection_is_duplicate;
				}
				else
				{
					ret_val = send(cur_sock, &connection_duplicate, sizeof(connection_duplicate), 0);
					if(ret_val <= 0)
					{
						perror("send() failed");
						return Connection_closed;
					}
				}

				pthread_mutex_lock(&taxi_list_lock);
				ret_val = Get_taxi_by_id(*taxi_list, &taxi, taxi_id);
				pthread_mutex_unlock(&taxi_list_lock);
				if(ret_val == -1)
				{
					taxi.id = taxi_id;
					taxi.fd = cur_sock;
					taxi.status = Waiting_for_order;
					pthread_mutex_lock(&taxi_list_lock);
					//Push_in_list((List**)taxi_list, &taxi, sizeof(taxi));
					Push_taxi_in_list(taxi_list, &taxi, sizeof(taxi));
					pthread_mutex_unlock(&taxi_list_lock);
					printf("INIT unregistered car(id:%i) fd:%i position:(%i, %i)\n", taxi.id, cur_sock, recv_buf.x, recv_buf.y);
				}

				else
				{
					taxi.fd = cur_sock;	// записываем номер сокета
					pthread_mutex_lock(&taxi_list_lock);
					Set_taxi_fd(*taxi_list, taxi_id, cur_sock);
					if(taxi.status == Offline)
						Set_taxi_status(*taxi_list, taxi_id, Waiting_for_order);
					pthread_mutex_unlock(&taxi_list_lock);
					printf("INIT registered car(id:%i) fd:%i position:(%i, %i)\n", taxi.id, cur_sock, recv_buf.x, recv_buf.y);
				}

				fd_info[cur_sock].type = Taxi;
				fd_info[cur_sock].unit_id = taxi.id;
			}

			else if(recv_buf.type == Taxi_ride_done) 
			{
				Set_taxi_fd(*taxi_list, taxi_id, cur_sock);
				pthread_mutex_lock(&taxi_list_lock);
				ret_val = Get_taxi_by_id(*taxi_list, &taxi, taxi_id);
				pthread_mutex_unlock(&taxi_list_lock);
				fd_info[cur_sock].type = Taxi;
				fd_info[cur_sock].unit_id = taxi.id;
				printf("DONE: taxi with id: %i is back\n", fd_info[cur_sock].unit_id);
				pthread_mutex_lock(&taxi_list_lock);
				Set_taxi_status(*taxi_list, taxi.id, Waiting_for_order);
				pthread_mutex_unlock(&taxi_list_lock);
				pthread_mutex_lock(&ride_list_lock);
				Get_ride_info_by_taxi_id(cur_rides_list, &ride_info, fd_info[cur_sock].unit_id);
				pthread_mutex_unlock(&ride_list_lock);
				c_send_buf.status = Done;
				pthread_mutex_lock(&fds_lock);
				int client_sock = Get_sock_by_id(fd_info, Client, ride_info.client_id);
				pthread_mutex_unlock(&fds_lock);
				if(client_sock != -1)
				{
					/// ОПАСНО если клиент отрубится и другой успеет занять сокет клиента ему полетит инфа, надо обнести мьютексом
					ret_val = send(client_sock, &c_send_buf, sizeof(c_send_buf), 0);
					if(ret_val <= 0)
					{
						Close_connection(client_sock, fds, fd_info, nfds, taxi_list, client_queue);
						Compress_fds_array(fds, nfds);
					}
				}
				Print_in_stat_file(&ride_info);
				pthread_mutex_lock(&ride_list_lock);
				Delete_ride_by_taxi_id(&cur_rides_list, taxi.id);
				pthread_mutex_unlock(&ride_list_lock);
			}

			taxi.pos.x = recv_buf.x;
			taxi.pos.y = recv_buf.y;
			pthread_mutex_lock(&taxi_list_lock);
			Set_taxi_pos(*taxi_list, taxi.id, taxi.pos);
			pthread_mutex_unlock(&taxi_list_lock);

			ret_val = Get_ride_info_by_taxi_id(cur_rides_list, &ride_info, taxi.id);
			if(ret_val == 0) 
			{
				// если таксист уже в поездке
				printf(" - TAXI IS ALREADY ON THE RIDE\n");
				printf("position: (%i, %i), destonation: (%i, %i)\n", ride_info.start_pos.x, ride_info.start_pos.y, ride_info.dest_pos.x, ride_info.dest_pos.y);
				printf("client_id: %i\n", ride_info.client_id);
				printf("taxi_id: %i\n", taxi.id);
				printf("status: %i\n", ride_info.status);

				// заново отправляем инфу таксисту
				ret_val = Send_info_to_taxi(taxi_id, &ride_info, taxi_list);
				if(ret_val <= 0)
				{
					perror("recv() failed");
					return Connection_closed;
				}
			}

			else
			{
				Check_client_in_queue(taxi_id, client_queue, taxi_list, fds, fd_info, nfds);
			}
			break;
		}
	return Connection_is_active;
}

void *Handle_fds_array(void *t_args)
{
	int ret_val;
	int cur_sock;

	struct Thread_args *args = (struct Thread_args *)t_args;

	int *nfds = args->nfds;
	struct pollfd *fds = args->fds;
	struct Fd_info *fd_info = args->fd_info;

	static struct Client_queue *client_queue = NULL;
	static struct Taxi_list *taxi_list = NULL;

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

		ret_val = Handle_socket(cur_sock, fds, fd_info, &client_queue, &taxi_list, nfds);
		if(ret_val == Connection_closed)
		{
			Close_connection(cur_sock, fds, fd_info, nfds, &taxi_list, &client_queue);
			Compress_fds_array(fds, nfds);
		}

		else if(ret_val == Connection_is_duplicate)
		{
			pthread_mutex_lock(&fds_lock);
			// ищем в fds поле с этим сокетом
			int k = 0;
			while(fds[k].fd != cur_sock && k < MAX_FD)
				k++;
			close(fds[k].fd);
			//fd_info[cur_sock].is_waititng = TRUE;
			fds[k].fd = -1;
			pthread_mutex_unlock(&fds_lock);
		}

		pthread_mutex_lock(&client_queue_lock);
		Show_client_queue(client_queue);
		pthread_mutex_unlock(&client_queue_lock);
		pthread_mutex_lock(&taxi_list_lock);
		Show_taxi_list(taxi_list);
		pthread_mutex_unlock(&taxi_list_lock);
		printf(" - - - - - - - - - - - - - - - - - - - \n");
		fd_info[cur_sock].is_waititng = TRUE;
	}
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	printf("mypid is %i\n", getpid());

	if(argc > 1 && strcmp(argv[1], "-d") == 0)
	{
		log_file = fopen("log", "w");
		if(log_file == NULL)
		{
			printf("Can`t open log file\n");
			exit(EXIT_FAILURE);
		}
		dup2(fileno(log_file), 1);
		daemon(1, 0);
	}

	int ret_val;
	
	int timeout;
	int nfds = 1;
	struct pollfd fds[MAX_FD];
	struct Fd_info fd_info[MAX_FD];
	struct Sock_queue *sock_queue = NULL;

	struct Thread_args t_args;
	t_args.fds = fds;
	t_args.fd_info = fd_info;
	t_args.sock_queue = &sock_queue;
	t_args.nfds = &nfds;

	int listen_sock = 0, accept_sock = 0;
	struct sockaddr_in serv_addr, client_addr;
	socklen_t cli_addr_len = sizeof(client_addr);
	Init_server(&listen_sock, &serv_addr, fd_info);

	int i;
	for(i = 0; i < NUM_THREADS; i++)
	{
		ret_val = pthread_create(&tid[i], NULL, Handle_fds_array, (void *)&t_args);
		if(ret_val != 0) 
		{
			printf("Can`t create thread\n");
			exit(EXIT_FAILURE);
		}
	}

	fds[0].fd = listen_sock;
	fds[0].events = POLLIN;
	fd_info[listen_sock].type = Listen_sock;
	timeout = -1;	// бесконечный таймер ожидания

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

	void *status;
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