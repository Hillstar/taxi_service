CC = gcc
CFLAGS = -Wall -Werror
LIBS = -lm
TARGET = server client taxi
DEPS = info_header.h

all: $(TARGET)

server: $(DEPS)	server.c taxi_list.c client_queue.c ride_list.c
	$(CC) $(CFLAGS) $(DEPS) server.c info_header.h taxi_list.c client_queue.c ride_list.c list.c -o server $(LIBS)

client: client.c $(DEPS)
	$(CC) $(CFLAGS) $(DEPS) client.c -o client

taxi: taxi.c $(DEPS)
	$(CC) $(CFLAGS) $(DEPS) taxi.c -o taxi

clean:
	rm $(TARGET)