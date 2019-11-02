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

void Generate_info(int info_buf[3], int dest[2])
{
	info_buf[0] = 1; // тип отправителя 

	// место нахождения
	info_buf[1] = rand() % MAP_SIZE;
	info_buf[2] = rand() % MAP_SIZE;

	// место назначения
	dest[0] = rand() % MAP_SIZE;
	dest[1] = rand() % MAP_SIZE;
}

int main(int argc, char *argv[])
{
	int connect_sock = 0;
	int ret_val;
	char choise;
	int info_buf[3];
	int recv_buf[4];
	int dest_buf[2];
	struct sockaddr_in serv_addr;
	srand(getpid()); 
	Generate_info(info_buf, dest_buf);

	socklen_t r_buf_size = sizeof(recv_buf);
	socklen_t s_buf_size = sizeof(info_buf);
	socklen_t dest_buf_size = sizeof(dest_buf);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("10.25.32.140");
	serv_addr.sin_port = htons(3007);

	socklen_t addr_size = sizeof(serv_addr);

	connect_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(connect_sock == -1)
	{
		perror("cant`t create socket");
		exit(EXIT_FAILURE);
	}
	
	ret_val = connect(connect_sock, (struct sockaddr*)&serv_addr, addr_size);
	if(ret_val == -1)
	{
		perror("cant`t connect");
		exit(EXIT_FAILURE);
	}
	
	send(connect_sock, info_buf, s_buf_size, 0);
	send(connect_sock, dest_buf, dest_buf_size, 0);
	printf("my position: (%i, %i)\n", info_buf[1], info_buf[2]);	
	printf("my dest: (%i, %i)\n", dest_buf[0], dest_buf[1]);

	recv(connect_sock, recv_buf, r_buf_size, 0);
	if(recv_buf[0] == 1)
	{
		printf("price: %i, waiting time: %i, time of ride: %i\n", recv_buf[1], recv_buf[2], recv_buf[3]);
		printf("will you ride?(y - yes, n - any other key): ");
		scanf("%c", &choise);
		send(connect_sock, &choise, sizeof(char), 0);
	}
	else
		printf("there`s no taxi\n");
	
	close(connect_sock);

	return 0;
}
