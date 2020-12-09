CC = gcc
CFLAGS = -g -Wall -fsanitize=address

all: CShell

CShell: cshell.c
	$(CC) $(CFLAGS) cshell.c -o CShell

clean:
	rm -rf CShell