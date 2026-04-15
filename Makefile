CC     = gcc
CFLAGS = -Wall -O0

OBJS = main.o math.o string.o memory.o screen.o keyboard.o

snake: $(OBJS)
	$(CC) $(CFLAGS) -o snake $(OBJS)

main.o:     main.c     snake.h
	$(CC) $(CFLAGS) -c main.c
math.o:     math.c     snake.h
	$(CC) $(CFLAGS) -c math.c
string.o:   string.c   snake.h
	$(CC) $(CFLAGS) -c string.c
memory.o:   memory.c   snake.h
	$(CC) $(CFLAGS) -c memory.c
screen.o:   screen.c   snake.h
	$(CC) $(CFLAGS) -c screen.c
keyboard.o: keyboard.c snake.h
	$(CC) $(CFLAGS) -c keyboard.c

clean:
	rm -f *.o snake
