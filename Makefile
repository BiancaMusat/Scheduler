CC=gcc
CFLAGS=-Wall -g
LINK=-lpthread

build: libscheduler.so

libscheduler.so: so_scheduler.o queue.o
	$(CC) $(CFLAGS) so_scheduler.o queue.o -shared -o libscheduler.so

so_scheduler.o: so_scheduler.c
	$(CC) $(CFLAGS) so_scheduler.c -fPIC -c -o so_scheduler.o $(LINK)

queue.o: queue.c
	$(CC) $(CFLAGS) queue.c -fPIC -c -o queue.o $(LINK)

clean:
	rm -rf so_scheduler.o queue.o libscheduler.so