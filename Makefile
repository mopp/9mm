CFLAGS      := -std=c11 -Wall -g -static
AFLAGS      := -g -no-pie
SRCS        := src/main.c src/tokenize.c src/parse.c src/codegen.c src/container.c
OBJS        := $(SRCS:.c=.o)
HEADERS     := $(wildcard src/*.h)
TESTS_IN    := $(wildcard test/*.c)
TESTS_DIFFS := $(TESTS_IN:.c=.diff)
MM          := ./9mm
MMS         := ./9mms
TEST_MM     ?= $(MM)


$(MM): $(OBJS)
	$(CC) -Isrc -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): $(HEADERS)

.PHONY: selfhost
selfhost: $(MMS)

$(MMS): $(MM)
	cat $(SRCS) > src/self.c
	$(MM) ./src/self.c > ./src/self.s
	$(CC) $(AFLAGS) ./src/self.s -o $@

.PHONY: diffs
diffs: $(TESTS_DIFFS)

%.diff: %.c %.ans $(TEST_MM) src/lib.o
	$(TEST_MM) $< > $*.s
	$(CC) $(AFLAGS) -o $*.bin $*.s src/lib.o
	./$*.bin > $*.out
	diff $*.ans $*.out | tee $*.diff

.PHONY: test
test: $(MM) src/lib.o
	$(MM) --test
	./test.sh
	make -s $(TESTS_DIFFS)

.PHONY: clean
clean:
	rm -f $(MM) $(OBJS) tmp tmp.s src/self.* $(MMS) test/*.s test/*.bin test/*.out $(TESTS_DIFFS)
