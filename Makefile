all:
	gcc server.c list.h -o server -lm -Wall -Werror
	gcc client.c -o client -Wall -Werror
	gcc taxi.c -o taxi -Wall -Werror

clean:
	rm server
	rm client
	rm taxi
