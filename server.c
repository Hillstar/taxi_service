#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include "list.h"

#define MAP_SIZE 20

void Send_info_to_taxi(struct taxi_unit *taxi, int client_dest[2], int time, int price)
{
	int temp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in taxi_addr;
	int send_buf[4]; // [0][1] = координаты места назначения, [2] - время поездки
	int addr_size = sizeof(taxi_addr);

	send_buf[0] = client_dest[0];
	send_buf[1] = client_dest[1];
	send_buf[2] = time;
	send_buf[3] = price;

	taxi_addr.sin_family = AF_INET;
	taxi_addr.sin_addr.s_addr = inet_addr("10.25.32.140");
	taxi_addr.sin_port = htons(3100 + taxi->id);

	sendto(temp_sock, send_buf, sizeof(send_buf), 0, (struct sockaddr*)&taxi_addr, addr_size);
	close(temp_sock);
}

float Calc_dist(int start_pos[2], int dest_pos[2])
{
	return sqrt(pow((dest_pos[0] - start_pos[0]), 2) + pow((dest_pos[1] - start_pos[1]), 2));
}

struct taxi_unit *Get_nearest_driver(float *dist, struct taxi_list *list, int client_pos[2])
{
	struct taxi_list *temp = list;
	int min_dist = Calc_dist(list->car.pos, client_pos);
	struct taxi_unit *nearest_driver = &(list->car);
	while(temp != NULL)
	{
		if(min_dist > Calc_dist(temp->car.pos, client_pos))
			nearest_driver = &(temp->car);
		temp = temp->next;
	}

	*dist = Calc_dist(nearest_driver->pos, client_pos);
	return nearest_driver;
}

int main(int argc, char *argv[])
{
	int listen_sock = 0, accept_sock = 0;
	int ret_val;
	struct sockaddr_in serv_addr, client_addr;
	int recv_buf[3];  // 
	int c_send_buf[4]; // [0] = -1 если таксист не найден, 1 если найден, [1] = стоимость, [2] = время ожидания, [3] = время поездки 
	int server_stop = 0;

	struct taxi_unit taxi;
	struct taxi_unit *nearest_taxi;
	struct taxi_list *av_taxi_list = NULL;
	int num_of_taxi = 0;
	int id_counter = 0;
	float dist_to_client = 0;
	float dist_to_dest = 0;
	int client_pos[2];
	int client_dest[2];
	int client_choise;
	int price = 0;

	socklen_t c_buf_size = sizeof(c_send_buf);
	socklen_t buf_size = sizeof(recv_buf);
	socklen_t cli_addr_len = sizeof(client_addr);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(3007);

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

	while(server_stop == 0)
	{
		accept_sock = accept(listen_sock, (struct sockaddr*)&client_addr, &cli_addr_len);
		if(accept_sock == -1)
		{
			perror("Can`t accept");
			exit(EXIT_FAILURE);
		}
		
		recv(accept_sock, recv_buf, buf_size, 0);
		if(recv_buf[0] == 1)
		{
			recv(accept_sock, client_dest, (socklen_t)sizeof(client_dest), 0);
			printf("type: Client, position: (%i, %i), destonation: (%i, %i)\n", recv_buf[1], recv_buf[2], client_dest[0], client_dest[1]);
			if(Is_list_empty(av_taxi_list) != 1)
			{
				client_pos[0] = recv_buf[1];
				client_pos[1] = recv_buf[2];
				nearest_taxi = Get_nearest_driver(&dist_to_client, av_taxi_list, client_pos);
				dist_to_dest = Calc_dist(client_pos, client_dest);
				price = (int)(round(dist_to_dest * 2) - num_of_taxi);
				printf("nearest taxi(id):%i\n", nearest_taxi->id);
				printf("dist to client: %f\n", dist_to_client);
				printf("dist from client to dest: %f\n", dist_to_dest);

				// отправить инфу клиенту
				c_send_buf[0] = 1;
				c_send_buf[1] = price;
				c_send_buf[2] = (int)round(dist_to_client);
				c_send_buf[3] = (int)round(dist_to_dest);
				send(accept_sock, c_send_buf, c_buf_size, 0);

				recv(accept_sock, &client_choise, (socklen_t)sizeof(char), 0);
				printf("client choise: %c\n", client_choise);
				if(client_choise == 'y')
				{
					Send_info_to_taxi(nearest_taxi, client_dest, (dist_to_client + dist_to_dest), price);
					Delete_by_id(&av_taxi_list, nearest_taxi->id);
					num_of_taxi--;
				}
			}

			else
			{
				c_send_buf[0] = -1;
				c_send_buf[1] = 0;
				send(accept_sock, c_send_buf, c_buf_size, 0);
			}
		}

		else if(recv_buf[0] == 2 || recv_buf[0] == 20)
		{
			if(recv_buf[0] == 2)
				taxi.id = id_counter++; // выдаем новый id
			else
				recv(accept_sock, &(taxi.id), sizeof(int), 0); // берем старый id
			taxi.pos[0] = recv_buf[1];
			taxi.pos[1] = recv_buf[2];
			Push(&av_taxi_list, taxi);
			printf("type: Taxi, id:%i, position:(%i, %i)\n", taxi.id, recv_buf[1], recv_buf[2]);

			send(accept_sock, &(taxi.id), sizeof(int), 0);
			num_of_taxi++;
		}

		printf("Current num of taxi = %i\n", num_of_taxi);
		Show_list(av_taxi_list);
		printf(" - - - - - - - - - - - - - - - - - - - \n");
	}
	
	close(accept_sock);
	close(listen_sock);
	return 0;
}