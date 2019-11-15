#include "server.h"

FILE *stat_file = NULL;
FILE *log_file = NULL;
struct Ride_info_list *cur_rides_list = NULL;
void *lib_handler;

void handle_sigusr1(int sig)
{
	fp_Show_curent_rides(cur_rides_list);
}

void handle_sigtint(int sig)
{
	if(stat_file != NULL)
		fclose(stat_file);
	if(log_file != NULL)
		fclose(log_file);
	dlclose(lib_handler);
	exit(EXIT_SUCCESS);
}

void Load_func()
{
	fp_Pop_client_info_from_queue = dlsym(lib_handler, "Pop_client_info_from_queue");
	if(dlerror() != NULL) 
	{
		fprintf(stderr, "Could not get function pointer, error is: %s\n\n", dlerror());
		exit(EXIT_FAILURE);
	}

	fp_Push_in_queue = dlsym(lib_handler, "Push_in_queue");
	if(dlerror() != NULL) 
	{
		fprintf(stderr, "Could not get function pointer, error is: %s\n\n", dlerror());
		exit(EXIT_FAILURE);
	}

	fp_Show_client_queue = dlsym(lib_handler, "Show_client_queue");
	if(dlerror() != NULL) 
	{
		printf("Func load failed\n");
		exit(EXIT_FAILURE);
	}

	fp_Delete_client_by_fd = dlsym(lib_handler, "Delete_client_by_fd");
	if(dlerror() != NULL) 
	{
		printf("Func load failed\n");
		exit(EXIT_FAILURE);
	}

	fp_Get_ride_info_by_ride_id = dlsym(lib_handler, "Get_ride_info_by_ride_id");
	if(dlerror() != NULL) 
	{
		printf("Func load failed\n");
		exit(EXIT_FAILURE);
	}

	fp_Get_ride_info_by_taxi_id = dlsym(lib_handler, "Get_ride_info_by_taxi_id");
	if(dlerror() != NULL) 
	{
		printf("Func load failed\n");
		exit(EXIT_FAILURE);
	}

	fp_Get_ride_info_by_client_fd = dlsym(lib_handler, "Get_ride_info_by_client_fd");
	if(dlerror() != NULL) 
	{
		printf("Func load failed\n");
		exit(EXIT_FAILURE);
	}

	fp_Show_curent_rides = dlsym(lib_handler, "Show_curent_rides");
	if(dlerror() != NULL) 
	{
		printf("Func load failed\n");
		exit(EXIT_FAILURE);
	}

	fp_Set_ride_status = dlsym(lib_handler, "Set_ride_status");
	if(dlerror() != NULL) 
	{
		printf("Func load failed\n");
		exit(EXIT_FAILURE);
	}

	fp_Delete_ride_by_taxi_id = dlsym(lib_handler, "Delete_ride_by_taxi_id");
	if(dlerror() != NULL) 
	{
		printf("Func load failed\n");
		exit(EXIT_FAILURE);
	}

	fp_Show_taxi_list = dlsym(lib_handler, "Show_taxi_list");
	if(dlerror() != NULL) 
	{
		printf("Func load failed\n");
		exit(EXIT_FAILURE);
	}

	fp_Get_taxi_by_car_num = dlsym(lib_handler, "Get_taxi_by_car_num");
	if(dlerror() != NULL) 
	{
		printf("Func load failed\n");
		exit(EXIT_FAILURE);
	}

	fp_Delete_taxi_by_id = dlsym(lib_handler, "Delete_taxi_by_id");
	if(dlerror() != NULL) 
	{
		printf("Func load failed\n");
		exit(EXIT_FAILURE);
	}

	fp_Delete_taxi_by_fd = dlsym(lib_handler, "Delete_taxi_by_fd");
	if(dlerror() != NULL) 
	{
		printf("Func load failed\n");
		exit(EXIT_FAILURE);
	}

	fp_Show_reg_list = dlsym(lib_handler, "Show_reg_list");
	if(dlerror() != NULL) 
	{
		printf("Func load failed\n");
		exit(EXIT_FAILURE);
	}

	fp_Get_id_by_num = dlsym(lib_handler, "Get_id_by_num");
	if(dlerror() != NULL) 
	{
		printf("Func load failed\n");
		exit(EXIT_FAILURE);
	}
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
	return sqrt(pow((dest_pos->x - start_pos->x), 2) + pow((dest_pos->y - start_pos->y), 2));
}

struct taxi_unit *Get_nearest_driver(float *dist, struct taxi_list *list, struct Point *client_pos)
{
	struct taxi_list *temp = list;
	int min_dist = Calc_dist(&(list->car->pos), client_pos);
	struct taxi_unit *nearest_driver = list->car;

	// находим ближайшего водителя
	while(temp != NULL)
	{
		if(min_dist > Calc_dist(&(list->car->pos), client_pos))
			nearest_driver = temp->car;
		temp = temp->next;
	}

	*dist = Calc_dist(&(nearest_driver->pos), client_pos);
	return nearest_driver;
}

void Close_connection(struct pollfd *poll_fd, struct Fd_info *fd_info, struct taxi_list **av_taxi_list, struct Client_queue **client_queue, int *num_of_av_taxi)
{
	if(fd_info->type == Taxi)
	{
		fp_Delete_taxi_by_fd(av_taxi_list, poll_fd->fd);
		printf("Taxi disconnected\n");
		if(fd_info->cur_ride_id == -1) // если текущей поездки нет
			(*num_of_av_taxi)--;
	}
			
	else
	{
		fp_Delete_client_by_fd(client_queue, poll_fd->fd);
		printf("Client disconnected\n");
	}	

	close(poll_fd->fd);
	poll_fd->fd = -1;
}

int main(int argc, char *argv[])
{
	printf("mypid is %i\n", getpid());

	lib_handler = dlopen("libshared.so", RTLD_LAZY);
	if(lib_handler == NULL)
	{
		fprintf(stderr,"dlopen() error: %s\n", dlerror());
		exit(EXIT_FAILURE); // в случае ошибки можно, например, закончить работу программы
	};

	Load_func();

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

	int i, j, k;
	int listen_sock = 0;
	int accept_sock = 0;
	int ret_val;
	struct sockaddr_in serv_addr, client_addr;
	struct Init_msg recv_buf;
	struct Client_info_msg c_send_buf;

	int end_server = FALSE, compress_array = FALSE;
	int close_conn;

	int timeout;
	int nfds = 1, current_size = 0;
	struct pollfd fds[MAX_FD];
	struct Fd_info fd_info[MAX_FD];

	struct Ride_info ride_info;

	struct Client_queue *client_queue = NULL;
	struct Client_info client_info;

	struct taxi_unit taxi;
	struct taxi_unit *nearest_taxi;
	struct taxi_list *av_taxi_list = NULL;
	struct taxi_list *reg_list = NULL;
	int car_num;
	int num_of_av_taxi = 0;
	int id_taxi_counter = 0;
	int id_ride_counter = 0;
	float dist_to_client = 0;
	float dist_to_dest = 0;
	Point client_dest;
	Point client_pos;
	char client_choise;
	int price = 0;

	socklen_t c_buf_size = sizeof(c_send_buf);
	socklen_t buf_size = sizeof(recv_buf);
	socklen_t cli_addr_len = sizeof(client_addr);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(PORT);

	signal(SIGINT, handle_sigtint); 
	signal(SIGUSR1, handle_sigusr1);

	stat_file = fopen("stat", "w");
	if(stat_file == NULL)
	{
		printf("Can`t open stat file\n");
		exit(EXIT_FAILURE);
	}

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
	fd_info[0].type = Listen_sock;
	timeout = -1; // бесконечный таймер ожидания

	while(end_server == FALSE)
	{
		ret_val = poll(fds, nfds, timeout);
		if(ret_val < 0)
		{
			if(errno == EINTR) // если poll заблочился сигналом, пробуем снова
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
				continue;
			
			if(fds[i].revents == POLLHUP)
			{
				close_conn = TRUE;
			}

			if(fds[i].revents == POLLERR)
			{
				printf("Error! revents = %d\n", fds[i].revents);
				close_conn = TRUE;
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

				ret_val = recv(fds[i].fd, &recv_buf, buf_size, 0);
				if(ret_val < 0)
				{
					perror("recv() failed");
					close_conn = TRUE;
				}

				if(ret_val == 0)
				{
					close_conn = TRUE;
				}

				if(close_conn == TRUE)
				{
					Close_connection(&fds[i], &fd_info[i], &av_taxi_list, &client_queue, &num_of_av_taxi);
					compress_array = TRUE;
					/*
					if(fd_info[i].type == Taxi)
					{
						fp_Delete_taxi_by_fd(&av_taxi_list, fds[i].fd);
						printf("Taxi disconnected\n");
						//num_of_av_taxi--;
					}
			
					else
					{
						fp_Delete_client_by_fd(&client_queue, fds[i].fd);
						printf("Client disconnected\n");
					}	

					close(fds[i].fd);
					fds[i].fd = -1;
					*/
				}

				else
				{
					if(recv_buf.type == Client_init)
					{
						fd_info[i].type = Client;
						ret_val = recv(fds[i].fd, &client_dest, (socklen_t)sizeof(client_dest), 0);
						if(ret_val <= 0)
						{
							if(ret_val < 0)
								perror("recv() failed");
							Close_connection(&fds[i], &fd_info[i], &av_taxi_list, &client_queue, &num_of_av_taxi);
							compress_array = TRUE;
							break;
						}
						printf("type: Client_init, position: (%i, %i), destonation: (%i, %i)\n", recv_buf.x, recv_buf.y, client_dest.x, client_dest.y);
						if(Is_list_empty((List*)av_taxi_list) != 1)
						{
							client_pos.x = recv_buf.x;
							client_pos.y = recv_buf.y;
							nearest_taxi = Get_nearest_driver(&dist_to_client, av_taxi_list, &client_pos);
							dist_to_dest = Calc_dist(&client_pos, &client_dest);
							price = (int)(round(dist_to_dest * 2) - num_of_av_taxi);
							printf("nearest taxi(id):%i\n", nearest_taxi->id);
							printf("dist to client: %f\n", dist_to_client);
							printf("dist from client to dest: %f\n", dist_to_dest);

							// собираем всю инфу о поездке
							ride_info.ride_id = id_ride_counter;
							ride_info.client_fd = fds[i].fd;
							ride_info.price = price;
							ride_info.car = *(nearest_taxi);
							ride_info.status = Car_waiting_for_answer;
							ride_info.start_pos.x = recv_buf.x;
							ride_info.start_pos.y = recv_buf.y;
							ride_info.dest_pos.x = client_dest.x;
							ride_info.dest_pos.y = client_dest.y;
							ride_info.dist_to_client = dist_to_client;
							ride_info.dist_to_dest = dist_to_dest;
							ride_info.time_of_ride = (int)round(dist_to_dest);

							// закрепляем клиента за этой поездкой
							fd_info[i].cur_ride_id = ride_info.ride_id;

							// ищем поле с инфой по сокету клиента и привзязываем ему поездку
							k = 0;
							while(fds[k].fd != nearest_taxi->fd && k < MAX_FD)
								k++;
							if(k < MAX_FD)
								fd_info[k].cur_ride_id = ride_info.ride_id;

							// отправляем инфу клиенту
							c_send_buf.status = Taxi_found;
							c_send_buf.price = price;
							c_send_buf.time_of_waiting = (int)round(dist_to_client);
							c_send_buf.time_of_ride = (int)round(dist_to_dest);
							ret_val = send(fds[i].fd, &c_send_buf, c_buf_size, 0);
							if(ret_val <= 0)
							{
								if(ret_val < 0)
									perror("send() failed");
								Close_connection(&fds[i], &fd_info[i], &av_taxi_list, &client_queue, &num_of_av_taxi);
								compress_array = TRUE;
								break;
							}
							nearest_taxi->cur_ride_id = id_ride_counter;
							id_ride_counter++;

							// добавляем инфу о поездке
							Push_in_list((List**)&cur_rides_list, &ride_info, sizeof(ride_info));

							// убираем такси из списка свободных
							fp_Delete_taxi_by_id(&av_taxi_list, nearest_taxi->id); 
							num_of_av_taxi--;
						}

						else
						{
							// говорим клиенту подождать
							client_info.fd = fds[i].fd;
							client_info.start_pos.x = recv_buf.x;
							client_info.start_pos.y = recv_buf.y;
							client_info.dest_pos.x = client_dest.x;
							client_info.dest_pos.y = client_dest.y;

							fp_Push_in_queue(&client_queue, &client_info, sizeof(client_info));
							c_send_buf.status = No_taxi_now;
							ret_val = send(fds[i].fd, &c_send_buf, c_buf_size, 0);
							if(ret_val <= 0)
							{
								if(ret_val < 0)
									perror("send() failed");
								Close_connection(&fds[i], &fd_info[i], &av_taxi_list, &client_queue, &num_of_av_taxi);
								compress_array = TRUE;
								break;
							}
						}
					}

					else if(recv_buf.type == Client_choise)
					{
						ret_val = recv(fds[i].fd, &client_choise, (socklen_t)sizeof(char), 0);
						if(ret_val <= 0)
						{
							if(ret_val < 0)
								perror("recv() failed");
							Close_connection(&fds[i], &fd_info[i], &av_taxi_list, &client_queue, &num_of_av_taxi);
							compress_array = TRUE;
							break;
						}
						printf("client choise: %c\n", client_choise);
						if(client_choise == 'y') // если клиент согласен
						{
							fp_Set_ride_status(cur_rides_list, fds[i].fd, Car_going_to_client);
							fp_Get_ride_info_by_ride_id(cur_rides_list, &ride_info, fd_info[i].cur_ride_id);
							nearest_taxi = &(ride_info.car);
							Send_info_to_taxi(nearest_taxi, &ride_info);
						}

						else // клиент отказался от поездки
						{
							printf("ride rejected\n");
							fp_Get_ride_info_by_ride_id(cur_rides_list, &ride_info, fd_info[i].cur_ride_id);
							taxi = ride_info.car;
							Push_in_list((List**)&av_taxi_list, &taxi, sizeof(taxi));
							num_of_av_taxi++;
						}
					}

					else if(recv_buf.type == Taxi_waiting_for_client)
					{
						fp_Get_ride_info_by_taxi_id(cur_rides_list, &ride_info, fd_info[i].unit_id);
						fp_Set_ride_status(cur_rides_list, ride_info.client_fd, Car_waiting_for_client);
						printf("taxi with id: %i waiting for client\n", ride_info.car.id);
						c_send_buf.status = Car_waiting_for_client;
						ret_val = send(ride_info.client_fd, &c_send_buf, sizeof(c_send_buf), 0);
						if(ret_val <= 0)
						{
							if(ret_val < 0)
								perror("send() failed");
							Close_connection(&fds[i], &fd_info[i], &av_taxi_list, &client_queue, &num_of_av_taxi);
							compress_array = TRUE;
							break;
						}
					}

					else if(recv_buf.type == Taxi_going_to_dest)
					{
						
						fp_Get_ride_info_by_taxi_id(cur_rides_list, &ride_info, fd_info[i].unit_id);
						fp_Set_ride_status(cur_rides_list, ride_info.client_fd, Executing);
						c_send_buf.status = Executing;
						printf("taxi with id: %i executes the order\n", ride_info.car.id);
						ret_val = send(ride_info.client_fd, &c_send_buf, sizeof(c_send_buf), 0);
						if(ret_val <= 0)
						{
							if(ret_val < 0)
								perror("send() failed");
							Close_connection(&fds[i], &fd_info[i], &av_taxi_list, &client_queue, &num_of_av_taxi);
							compress_array = TRUE;
							break;
						}
					}

					else if(recv_buf.type == Taxi_init || recv_buf.type == Taxi_ride_done) // сообщение от такси
					{
						fd_info[i].cur_ride_id = -1;
						if(recv_buf.type == Taxi_init)
						{
							ret_val = recv(fds[i].fd, &car_num, (socklen_t)sizeof(car_num), 0);
							if(ret_val <= 0)
							{
								if(ret_val < 0)
									perror("recv() failed");
								Close_connection(&fds[i], &fd_info[i], &av_taxi_list, &client_queue, &num_of_av_taxi);
								compress_array = TRUE;
								break;
							}
							ret_val = fp_Get_taxi_by_car_num(reg_list, &taxi, car_num);
							if(ret_val == -1)
							{
								printf("new unregistered car\n");
								taxi.id = id_taxi_counter; // выдаем новый id
								taxi.car_num = car_num;
								taxi.fd = fds[i].fd;  // записываем номер сокета
 								Push_in_list((List**)&reg_list, &taxi, sizeof(taxi));
								fp_Show_reg_list(reg_list);
								id_taxi_counter++;
								ret_val = send(fds[i].fd, &(taxi.id), sizeof(int), 0);
								if(ret_val <= 0)
								{
									if(ret_val < 0)
										perror("send() failed");
									Close_connection(&fds[i], &fd_info[i], &av_taxi_list, &client_queue, &num_of_av_taxi);
									compress_array = TRUE;
									break;
								}
							}

							else
							{
								printf("new registered car\n");
								fp_Show_reg_list(reg_list);
								taxi.fd = fds[i].fd;  // записываем номер сокета
								ret_val = send(fds[i].fd, &(taxi.id), sizeof(int), 0);
								if(ret_val <= 0)
								{
									if(ret_val < 0)
										perror("send() failed");
									Close_connection(&fds[i], &fd_info[i], &av_taxi_list, &client_queue, &num_of_av_taxi);
									compress_array = TRUE;
									break;
								}
							}

							fd_info[i].type = Taxi;
							fd_info[i].unit_id = taxi.id;
						}



						else if(recv_buf.type == Taxi_ride_done) 
						{
							printf("taxi with id: %i is back\n", fd_info[i].unit_id);
							fp_Get_ride_info_by_taxi_id(cur_rides_list, &ride_info, fd_info[i].unit_id);
							fd_info[i].cur_ride_id = -1;
							taxi = ride_info.car;
							c_send_buf.status = Done;
							ret_val = send(ride_info.client_fd, &c_send_buf, sizeof(c_send_buf), 0);
							if(ret_val <= 0)
							{
								if(ret_val < 0)
									perror("send() failed");
								Close_connection(&fds[i], &fd_info[i], &av_taxi_list, &client_queue, &num_of_av_taxi);
								compress_array = TRUE;
								break;
							}
							Print_in_stat_file(&ride_info);
							fp_Delete_ride_by_taxi_id(&cur_rides_list, taxi.id);
						}

						taxi.pos.x = recv_buf.x;
						taxi.pos.y = recv_buf.y;
						Push_in_list((List**)&av_taxi_list, &taxi, sizeof(taxi));
						num_of_av_taxi++;
						printf("type: Taxi, id:%i, position:(%i, %i)\n", taxi.id, recv_buf.x, recv_buf.y);
						ret_val = fp_Get_ride_info_by_taxi_id(cur_rides_list, &ride_info, taxi.id);

						// если таксист уже в поездке
						if(ret_val == 0) 
						{
							printf(" - TAXI IS ALREADY ON THE RIDE\n");
							printf("position: (%i, %i), destonation: (%i, %i)\n", ride_info.start_pos.x, ride_info.start_pos.y, ride_info.dest_pos.x, ride_info.dest_pos.y);
							printf("taxi_id = %i, t_id_ride = %i\n", taxi.id, ride_info.car.id);

							// берем инфу из записи о поездке и снова отправляем таксисту
							nearest_taxi = &(ride_info.car);

							// заново отправляем инфу таксисту
							Send_info_to_taxi(nearest_taxi, &ride_info);

							// убираем такси из списка свободных
							fp_Delete_taxi_by_id(&av_taxi_list, nearest_taxi->id); 
								num_of_av_taxi--;
						}

						else
						{
							ret_val = fp_Pop_client_info_from_queue(&client_queue, &client_info);
							// если есть клиенты в очереди ожидания
							if(ret_val != -1)
							{
								printf(" - GOT CLIENT IN QUEUE\n");
								nearest_taxi = &taxi;
								dist_to_client = Calc_dist(&(client_info.start_pos), &(taxi.pos));
								dist_to_dest = Calc_dist(&(client_info.start_pos), &(client_info.dest_pos));
								price = (int)(round(dist_to_dest * 2) - num_of_av_taxi);
								printf("nearest taxi(id):%i\n", nearest_taxi->id);
								printf("dist to client: %f\n", dist_to_client);
								printf("dist from client to dest: %f\n", dist_to_dest);
								printf("taxi_id = %i\n", taxi.id);

								// собираем всю инфу о поездке
								ride_info.ride_id = id_ride_counter;
								ride_info.client_fd = client_info.fd;
								ride_info.price = price;
								ride_info.car = taxi;
								ride_info.status = Car_waiting_for_answer;
								ride_info.start_pos.x = client_info.start_pos.x;
								ride_info.start_pos.y = client_info.start_pos.y;
								ride_info.dest_pos.x = client_info.dest_pos.x;
								ride_info.dest_pos.y = client_info.dest_pos.y;
								ride_info.time_of_ride = (int)round(dist_to_dest);

								printf("n_taxi_id = %i\n", nearest_taxi->id);

								// привязываем таксисту текущую поездку
								fd_info[i].cur_ride_id = ride_info.ride_id;

								// ищем поле с инфой по сокету клиента и привзязываем ему поездку
								k = 0;
								while(fds[k].fd != client_info.fd && k < MAX_FD)
									k++;
								fd_info[k].cur_ride_id = ride_info.ride_id;

								printf("position: (%i, %i), destonation: (%i, %i)\n", ride_info.start_pos.x, ride_info.start_pos.y, ride_info.dest_pos.x, ride_info.dest_pos.y);

								// отправляем инфу клиенту
								c_send_buf.status = Done;
								c_send_buf.price = price;
								c_send_buf.time_of_waiting = (int)round(dist_to_client);
								c_send_buf.time_of_ride = (int)round(dist_to_dest);
								ret_val = send(client_info.fd, &c_send_buf, c_buf_size, 0);
								if(ret_val <= 0)
								{
									if(ret_val < 0)
										perror("send() failed");
									Close_connection(&fds[i], &fd_info[i], &av_taxi_list, &client_queue, &num_of_av_taxi);
									compress_array = TRUE;
									break;
								}
								nearest_taxi->cur_ride_id = id_ride_counter;
								id_ride_counter++;

								// добавляем инфу о поездке
								Push_in_list((List**)&cur_rides_list, &ride_info, sizeof(ride_info));

								// убираем такси из списка свободных
								Delete_taxi_by_id(&av_taxi_list, nearest_taxi->id); 
								num_of_av_taxi--;
							}
						}
					}
				}
			}

			for (k = 0; k < nfds; k++)
				printf("type = %i, cur_ride_id = %i\n", fd_info[k].type, fd_info[k].cur_ride_id);

			printf("Current num of taxi = %i\n", num_of_av_taxi);
			fp_Show_taxi_list(av_taxi_list);
			fp_Show_client_queue(client_queue);
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
						fd_info[j].type = fd_info[j+1].type;
						fd_info[j].cur_ride_id = fd_info[j+1].cur_ride_id;
						fd_info[j].unit_id = fd_info[j+1].unit_id;
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

	fclose(stat_file);
	return 0;
}