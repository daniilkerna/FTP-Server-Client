all: server client 

clean: 
	rm server client

server: server.c
	gcc -std=gnu99 -Wall -pedantic -o server server.c

client: client.c 
	gcc -std=gnu99 -Wall -pedantic -o client client.c
