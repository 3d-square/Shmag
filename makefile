CC=gcc
CFLAGS=-Wall -Wextra
INC=-I./src/

SRCS=$(wildcard src/*)

OBJS=$(SRCS:.c=.o)
DEPS=src/shmag.h

all: shmag

shmag: $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) $(INC) $^ -o shmag -lm

src/%.o: src/%.c $(DEPS)
	$(CC) $(CFLAGS) $(INC) -c  $< -o $@ 

clean:
	rm src/*.o
	rm shmag
