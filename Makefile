CFLAGS = -std=c11 -Wall -g -static
SRCS   = $(wildcard src/*.c)
OBJS   = $(SRCS:.c=.o)

9mm: $(OBJS)
	$(CC) -Isrc -o 9mm $(OBJS) $(LDFLAGS)

$(OBJS): src/9mm.h

test: 9mm
	./9mm --test
	./test.sh

clean:
	rm -f 9mm src/*.o *~ tmp tmp.s
