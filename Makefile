CC = cc
CFLAGS = -Wall -Wextra -O2

snake: main.c libs/math.c libs/string.c libs/memory.c libs/screen.c libs/keyboard.c libs/sound.c libs/entity.c
	$(CC) $(CFLAGS) -o snake $^

clean:
	rm -f snake
