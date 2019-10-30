#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

#define MAP_SIZE 20

//int Calc_distance();
//int Get_nearest_driver();

void Init_map(char map[MAP_SIZE][MAP_SIZE])
{
	for(int i = 0; i < MAP_SIZE; i++)
	{
		for(int j = 0; j < MAP_SIZE; j++)
		{
			map[i][j] = '-';
		}
	}
}

void Print_map(char map[MAP_SIZE][MAP_SIZE])
{
	for(int i = 0; i < MAP_SIZE; i++)
	{
		for(int j = 0; j < MAP_SIZE; j++)
		{
			printf("%c ", map[i][j]);
		}
		printf("\n");
	}
}

int main(int argc, char *argv[])
{
	int listen_sock = 0, accept_sock = 0;
	int ret_val;
	char map[MAP_SIZE][MAP_SIZE];
	struct sockaddr_in serv_addr, client_addr;
	//char recv_buf[10];
	int recv_buf[3];
	int server_stop = 0;

	Init_map(map);

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

	listen(listen_sock, 5);
	while(server_stop == 0)
	{
		accept_sock = accept(listen_sock, (struct sockaddr*)&client_addr, &cli_addr_len);
		if(accept_sock == -1)
		{
			perror("Can`t accept");
			exit(EXIT_FAILURE);
		}
		
		recv(accept_sock, recv_buf, buf_size, 0);

		map[recv_buf[1]][recv_buf[2]] = recv_buf[0] + '0';

		system("clear");
		printf("recv buf: type: %i, point:(%i, %i)\n", recv_buf[0], recv_buf[1], recv_buf[2]);

		Print_map(map);
	}
	
	close(listen_sock);

	return 0;
}
