CC = gcc
CFLAGS = -Wall -Wextra

all: CStatsDProxy

CStatsDProxy: main.o queue.o
	$(CC) $(CFLAGS) -o CStatsDProxy main.o queue.o -lpthread

main.o: main.c queue.h
	$(CC) $(CFLAGS) -c main.c

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c queue.c

clean:
	rm -f *.o CStatsDProxy
