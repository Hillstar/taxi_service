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
#include "info_header.h"
#include "list.h"
#include "client_queue.h"
#include "ride_list.h"
#include "taxi_list.h"


// указатели на ссылки из client_queue
int (*fp_Pop_client_info_from_queue)(struct Client_queue **head, struct Client_info *client_info);
void (*fp_Push_in_queue)(struct Client_queue **head, struct Client_info *client_info, int data_size);
void (*fp_Show_client_queue)(struct Client_queue *head);
void (*fp_Delete_client_by_fd)(struct Client_queue **head, int fd);

// указатели на ссылки из ride_list
int (*fp_Get_ride_info_by_ride_id)(struct Ride_info_list *head, struct Ride_info *info_buf, int ride_id);
int (*fp_Get_ride_info_by_taxi_id)(struct Ride_info_list *head, struct Ride_info *info_buf, int taxi_id);
int (*fp_Get_ride_info_by_client_fd)(struct Ride_info_list *head, struct Ride_info *info_buf, int client_fd);
void (*fp_Show_curent_rides)(struct Ride_info_list *head);
int (*fp_Set_ride_status)(struct Ride_info_list *head, int client_fd, int status);
void (*fp_Delete_ride_by_taxi_id)(struct Ride_info_list **head, int taxi_id);

// указатели на ссылки из taxi_list
void (*fp_Show_taxi_list)(struct taxi_list *head);
int (*fp_Get_taxi_by_car_num)(struct taxi_list *head, struct taxi_unit *taxi, int car_num);
void (*fp_Delete_taxi_by_id)(struct taxi_list **head, int id);
int (*fp_Delete_taxi_by_fd)(struct taxi_list **head, int fd);
void (*fp_Show_reg_list)(struct taxi_list *head);
int (*fp_Get_id_by_num)(struct taxi_list *head, int car_num);