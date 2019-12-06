#ifndef INFO_HEADER_H
#define INFO_HEADER_H

#define MAP_SIZE 20
#define MAX_FD 200
#define NUM_THREADS 4
#define PORT 3007

enum e_Fd_type { Client, Taxi, Listen_sock };

enum e_Bool { False, True };

enum e_Init_msg_field { X, Y, TYPE };
	
enum e_Init_msg_type
	{ Client_init, Client_choise, Taxi_init, Taxi_ride_done, Taxi_waiting_for_client, Taxi_going_to_dest, Taxi_going_to_client };

enum e_Client_statement
	{ Waiting_for_offer, Waiting_in_queue, Answering, Waiting_for_taxi, C_going_to_taxi, C_going_to_dest, C_not_initialized };

enum e_Taxi_statement
	{ Waiting_for_order, Waiting_for_client_answer, Going_to_client, Waiting_for_client, Going_to_dest, Not_initialized, Offline };

enum e_Ride_status
	{ Car_waiting_for_answer, Car_going_to_client, Car_waiting_for_client, Executing, Done, Rejected, No_taxi_now, Taxi_found };

enum e_Connectio_status
	{ Connection_is_active, Connection_closed, Connection_is_duplicate };


typedef struct Point
{
	int x;
	int y;
} Point;

struct Fd_info
{
	int type;
	int unit_id;
	int is_waititng;
};

struct Init_msg
{
	int x;
	int y;
	int id;
	int type;
};

struct Taxi_info_msg
{
	Point dest;
	int time_of_ride;
	int price;
	float dist_to_client;
	float dist_to_dest;
	int status;
};

struct Client_info_msg
{
	int status;
	int price;
	int time_of_waiting;
	int time_of_ride;
};

struct Thread_args
{
	struct pollfd *fds;
	struct Fd_info *fd_info;
	int *nfds;
	struct Sock_queue **sock_queue;
};

#endif