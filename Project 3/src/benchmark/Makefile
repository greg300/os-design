all: test

test:
	gcc test.c -L../ -lmy_vm -lm -lpthread -m32 -o test

test2:
	gcc test2.c -L../ -lmy_vm -lm -lpthread -m32 -o test2

clean:
	rm -rf test test2
