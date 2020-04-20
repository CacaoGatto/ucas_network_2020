all: client server

client: client.c
	gcc -Wall -g client.c -o client

server: server.c
	gcc -Wall -g server.c -o server

test-server: test-server.c
	gcc -Wall -g test-server.c -o test-server

clean:
	@rm -f client server test-server
