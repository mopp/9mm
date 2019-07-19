CFLAGS      = -std=c11 -Wall -g -static
SRCS        = src/main.c src/tokenize.c src/parse.c src/codegen.c src/container.c src/lib.c
OBJS        = $(SRCS:.c=.o)
TESTS_IN    = $(wildcard test/*.c)
TESTS_DIFFS = $(TESTS_IN:.c=.diff)

9mm: $(OBJS)
	$(CC) -Isrc -o 9mm $(OBJS) $(LDFLAGS)

$(OBJS): src/9mm.h

.PHONY: selfhost
selfhost: 9mms

9mms: 9mm
	cat src/main.c src/tokenize.c src/parse.c src/codegen.c src/container.c > src/self.c
	./9mm ./src/self.c > ./src/self.s
	gcc -g -no-pie ./src/self.s -o 9mms

%.diff: %.c %.ans src/lib.o
	./9mm $< > $*.s
	gcc -no-pie -g -o $*.bin $*.s src/lib.o
	./$*.bin > $*.out
	diff $*.ans $*.out > $*.diff

.PHONY: test
test: 9mm
	./9mm --test
	./test.sh
	make -s $(TESTS_DIFFS)
	cat $(TESTS_DIFFS)

.PHONY: clean
clean:
	rm -f 9mm src/*.o *~ tmp tmp.s src/self.c src/self.s 9mms test/*.s test/*.bin test/*.out test/*.diff
