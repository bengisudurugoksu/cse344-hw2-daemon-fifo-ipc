CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99

TARGETS = hw2 child1 child2

all: $(TARGETS)

hw2: hw2.c
	$(CC) $(CFLAGS) -o hw2 hw2.c

child1: child1.c
	$(CC) $(CFLAGS) -o child1 child1.c

child2: child2.c
	$(CC) $(CFLAGS) -o child2 child2.c

clean:
	rm -f $(TARGETS) *.o *.log fifo1 fifo2 /tmp/temp_result.txt
	killall hw2 child1 child2 2>/dev/null || true

