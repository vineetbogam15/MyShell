CC = gcc
CFLAGS = -Wall -std=c99 -g

mysh: mysh.c
	$(CC) $(CFLAGS) $^ -o mysh

clean:
	rm -f *.o mysh
