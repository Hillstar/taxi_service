CC = gcc
CFLAGS = -Wall -Werror -fPIC
SRCS = list.c
OBJS = list.o
DEPS = list.h
TARGET = liblist.a 

list_lib: $(OBJS) $(TARGET)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJS) 
	ar rcs $@ $^
	rm $(OBJS) -f

clean:
	rm $(TARGET) -f
	rm $(OBJS) -f