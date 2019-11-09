#ifndef INFO_HEADER_H
#define INFO_HEADER_H

#define MAP_SIZE 20

enum Init_msg_fields { X, Y, TYPE };

enum Init_msg_types
	{ Client_init, Client_choise, Taxi_init, Taxi_ride_done };

enum Is_there_a_av_taxi 
	{ Taxi_found, No_taxi_now };

enum Ride_status
	{ Done, Executing, Waiting_for_answer };

#endif