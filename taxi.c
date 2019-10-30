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
}

int main(int argc, char *argv[])
{
	int connect_sock = 0;
	int addr_size, ret_val;
	int info_buf[3]; // где [0] - тип отправителя, [1] - координата x, [0] - координата y  
	struct sockaddr_in serv_addr;
	//char send_buf[] = "Hello\n";
	srand(getpid()); 
	Generate_position(info_buf);

	socklen_t buf_size = sizeof(info_buf);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("10.25.32.140");
	serv_addr.sin_port = htons(3007);

	addr_size = sizeof(serv_addr);

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
	
	send(connect_sock, info_buf, buf_size, 0);
	//printf("send buf: %s", send_buf);	
	printf("my position: (%i, %i)\n", info_buf[1], info_buf[2]);	
	
	close(connect_sock);

	return 0;
}

