CFLAGS      := -std=c11 -Wall -g -static
AFLAGS      := -g -no-pie
SRCS        := src/main.c src/preprocessor.c src/tokenize.c src/parse.c src/codegen.c src/container.c
OBJS        := $(SRCS:.c=.o)
HEADERS     := $(wildcard src/*.h)
TESTS_IN    := $(filter-out test/lib.c, $(wildcard test/*.c))
TESTS_DIFFS := $(TESTS_IN:.c=.diff)
MM          := ./9mm
TEST_LIB    := test/lib.o
TEST_9MM    ?= $(MM)
PREV        ?= $(MM)
NEXT        ?= ./9mms


$(MM): $(OBJS)
	$(CC) -Isrc -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): $(HEADERS)

.PHONY: selfcompile
selfcompile: $(PREV)
	cat $(SRCS) > src/self.c
	$(PREV) ./src/self.c > ./src/self.s
	$(CC) $(AFLAGS) ./src/self.s -o $(NEXT)

.PHONY: test
test: $(TEST_9MM) $(TEST_LIB)
	$(TEST_9MM) --test
	./test.sh $(TEST_9MM)
	make -s $(TESTS_DIFFS) TEST_9MM=$(TEST_9MM)

.PHONY: diffs
diffs: $(TESTS_DIFFS)

%.diff: %.c %.ans $(TEST_9MM) $(TEST_LIB)
	$(TEST_9MM) $< > $*.s
	$(CC) $(AFLAGS) -o $*.bin $*.s $(TEST_LIB)
	./$*.bin > $*.out
	diff $*.ans $*.out | tee $*.diff

.PHONY: clean
clean:
	rm -f $(MM)* $(OBJS) tmp tmp.s src/self.* test/*.s test/*.bin test/*.out $(TEST_LIB) $(TESTS_DIFFS)
