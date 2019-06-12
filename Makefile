CFLAGS = -std=c11 -Wall -g -static
SRCS   = $(wildcard *.c)
OBJS   = $(SRCS:.c=.o)

9mm: $(OBJS)
	$(CC) -o 9mm $(OBJS) $(LDFLAGS)

$(OBJS): 9mm.h

test: 9mm
	./9mm -test
	./test.sh

clean:
	rm -f 9mm *.o *~
