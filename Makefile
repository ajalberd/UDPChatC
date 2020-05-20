CC=gcc
CFLAGS=-Wall.
client: client.c DieWithError.c
	$(CC) -g -pthread -o client client.c DieWithError.c -Wall
