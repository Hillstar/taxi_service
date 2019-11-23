CC = gcc
INC = -I../ -I../listlib
LDIR = -L../ -L../listlib
CFLAGS = -Wall -Werror -fPIC
LIBS = -llist -ldl
SRCS = taxi_list.c client_queue.c ride_list.c sock_queue.c
OBJS = $(patsubst %.c, %.o, $(SRCS))
DEPS = $(patsubst %.c, %.h, $(SRCS))
TARGET = libshared.so

shared_lib: $(OBJS) $(TARGET) move_lib

%.o: %.c $(DEPS)
	$(CC) $(INC) $(LDIR) $(LIBS) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJS)
	$(CC) $(LDIR) $(LIBS) -shared -o $@ $^ $(CFLAGS)
	rm $(OBJS) -f

move_lib:
	$(shell sudo mv $(TARGET) /usr/lib/$(TARGET))
	$(shell sudo chmod 755 /usr/lib/$(TARGET))

clean:
	rm /usr/lib/$(TARGET) -f
	rm $(OBJS) -f
	rm $(TARGET) -f
