all: client pthread-server select-server

client: client.c
	gcc -Wall -g client.c -o client

pthread-server: pthread-server.c
	gcc -Wall -g pthread-server.c -o pthread-server -lpthread

select-server: select-server.c
	gcc -Wall -g select-server.c -o select-server

clean:
	@rm -f client pthread-server select-server
