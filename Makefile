CFLAGS=-std=c11 -g -static -Wall -Wextra -fno-common
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

y3c: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): y3c.h

test: y3c
	./test.sh

clean:
	rm -f y3c *.o *~ tmp*

.PHONY: test clean