CC = gcc
INC = -I../ -I../listlib
LDIR = -L../ -L../listlib
CFLAGS = -Wall -Werror -fPIC
LIBS = -llist -ldl
SRCS = taxi_list.c client_queue.c ride_list.c
OBJS = $(patsubst %.c, %.o, $(SRCS))
DEPS = $(patsubst %.c, %.h, $(SRCS))
TARGET = libshared.so

shared_lib: export_path $(OBJS) $(TARGET)

export_path:
	$(shell (export LD_LIBRARY_PATH=$(pwd)))

%.o: %.c $(DEPS)
	$(CC) $(INC) $(LDIR) $(LIBS) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJS)
	$(CC) $(LDIR) $(LIBS) -shared -o $@ $^ $(CFLAGS)
	$(shell sudo mv libshared.so /usr/lib/libshared.so)
	$(shell sudo chmod 755 /usr/lib/libshared.so)
	#ldconfig
	rm $(OBJS)

clean:
	rm /usr/lib/$(TARGET)
	rm $(OBJS) -f
