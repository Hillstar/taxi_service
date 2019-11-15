#ifndef INFO_HEADER_H
#define INFO_HEADER_H

#define MAP_SIZE 20
#define MAX_FD 200
#define PORT 3007

enum Fd_types {	Client, Taxi, Listen_sock };

enum Bool { False, True };

enum Init_msg_fields { X, Y, TYPE };

enum Init_msg_types
	{ Client_init, Client_choise, Taxi_init, Taxi_ride_done, Taxi_waiting_for_client, Taxi_going_to_dest, Taxi_going_to_client };

enum Taxi_statements
	{ Waiting_for_order, Going_to_client, Waiting_for_client, Going_to_dest, Not_initialized, Waiting_for_client_answer };

enum Is_there_a_av_taxi 
	{ Taxi_found, No_taxi_now };

enum Ride_status
	{ Done, Executing, Car_waiting_for_answer, Car_waiting_for_client, Car_going_to_client };


typedef struct Point
{
	int x;
	int y;
} Point;

struct Fd_info
{
	int type;
	int unit_id;
	int cur_ride_id;
};

struct Init_msg
{
	int x;
	int y;
	int type;
};

struct Taxi_info_msg
{
	int dest_x;
	int dest_y;
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

#endif