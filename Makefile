CC = gcc
CFLAGS = -Wall -Werror -g
INC =  -I. -I./sharedlib -I./listlib 
LDIR = -L./sharedlib -L./listlib
LIBS = -lm -lpthread -llist -ldl -lshared
SRCS = server.c client.c taxi.c
OBJS = $(patsubst %.c, %.o, $(SRCS))
TARGET = server client taxi
DEPS = info_header.h
EXP_PATH = LD_LIBRARY_PATH=$(shell pwd)/sharedlib

all: llist lshared export_path $(TARGET) 

$(TARGET): %: %.c $(DEPS)
	$(CC) $(CFLAGS) $(INC) $< -o $@ $(LIBS) $(LDIR)

export_path:
	echo $(shell $(EXP_PATH))

llist:
	make list_lib -C listlib -f listlib.mk

lshared:
	make shared_lib -C sharedlib -f sharedlib.mk

clean:
	rm $(TARGET) -f
	rm log -f
	rm stat -f
	make clean -C sharedlib -f sharedlib.mk
	make clean -C listlib -f listlib.mk