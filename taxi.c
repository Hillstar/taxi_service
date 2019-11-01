#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#define MAP_SIZE 20

void Generate_position(int info_buf[3])
{
	info_buf[0] = 2; // тип отправителя 
	info_buf[1] = rand() % MAP_SIZE;
	info_buf[2] = rand() % MAP_SIZE;
	info_buf[3] = info_buf[4] = 0;
}

int main(int argc, char *argv[])
{
	int recv_sock = 0, connect_sock = 0;
	int ret_val;
	int info_buf[5]; // где [0] - тип отправителя, [1] - координата x, [0] - координата y  
	int recv_buf[3];
	int id = 0;
	int taxi_stop = 0;
	struct sockaddr_in serv_addr, taxi_addr;
	srand(getpid()); 
	Generate_position(info_buf);

	socklen_t r_buf_size = sizeof(recv_buf);
	socklen_t s_buf_size = sizeof(info_buf);

	// init connect sock
	/*
	connect_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(connect_sock == -1)
	{
		perror("cant`t create socket");
		exit(EXIT_FAILURE);
	}
	*/

	//serv_addr.sin_family = AF_INET;
	//serv_addr.sin_addr.s_addr = inet_addr("10.25.32.140");
	//serv_addr.sin_port = htons(3007);
	//socklen_t s_addr_size = sizeof(serv_addr);

	while(taxi_stop == 0)
	{
		connect_sock = socket(AF_INET, SOCK_STREAM, 0);
		if(connect_sock == -1)
		{
			perror("cant`t create socket");
			exit(EXIT_FAILURE);
		}
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = inet_addr("10.25.32.140");
		serv_addr.sin_port = htons(3007);
		socklen_t s_addr_size = sizeof(serv_addr);

		ret_val = connect(connect_sock, (struct sockaddr*)&serv_addr, s_addr_size);
		if(ret_val == -1)
		{
			perror("cant`t connect");
			exit(EXIT_FAILURE);
		}
		
		send(connect_sock, info_buf, s_buf_size, 0);
		printf("my position: (%i, %i)\n", info_buf[1], info_buf[2]);

		// init recv sock
		recv(connect_sock, &id, sizeof(int), 0);
		recv_sock = socket(AF_INET, SOCK_DGRAM, 0);
		taxi_addr.sin_family = AF_INET;
		taxi_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		taxi_addr.sin_port = htons(3100 + id);
		socklen_t t_addr_size = sizeof(taxi_addr);
		ret_val = bind(recv_sock, (struct sockaddr*)&taxi_addr, sizeof(taxi_addr));
		if(ret_val != 0)
		{
			printf("Bind error\n");
			exit(EXIT_FAILURE);
		}

		close(connect_sock); // потому что один поток на сервере

		recvfrom(recv_sock, recv_buf, r_buf_size, 0, (struct sockaddr*)&serv_addr, &t_addr_size);
		printf("Destonation is (%i, %i) (time of ride is %i)\n", recv_buf[0], recv_buf[1], recv_buf[2]);
		
		//close(connect_sock);
		close(recv_sock);
		sleep(recv_buf[2] / 2);
		info_buf[1] = recv_buf[0]; // x
		info_buf[2] = recv_buf[1]; // y
	}

	return 0;
}