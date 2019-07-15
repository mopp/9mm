CFLAGS = -std=c11 -Wall -g -static
SRCS   = src/main.c src/tokenize.c src/parse.c src/codegen.c src/container.c src/lib.c
OBJS   = $(SRCS:.c=.o)

9mm: $(OBJS)
	$(CC) -Isrc -o 9mm $(OBJS) $(LDFLAGS)

$(OBJS): src/9mm.h

selfhost: 9mm
	cat src/main.c src/tokenize.c src/parse.c src/codegen.c src/container.c > src/self.c
	./9mm ./src/self.c > ./src/self.s
	gcc -g -no-pie ./src/self.s -o 9mms

test: 9mm
	./9mm --test
	./test.sh

clean:
	rm -f 9mm src/*.o *~ tmp tmp.s src/self.c src/self.s 9mms
