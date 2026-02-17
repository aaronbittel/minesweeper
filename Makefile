CC = gcc
CFLAGS = -Wall -Wextra -pedantic -g
RAYLIB = $(shell pkg-config --cflags --libs raylib)

build: main.c
	$(CC) $(CFLAGS) main.c -o main $(RAYLIB)

run: build
	./main
