CFLAGS      := -std=c11 -Wall -g -static
AFLAGS      := -g -no-pie
SRCS        := src/main.c src/preprocessor.c src/tokenize.c src/parse.c src/codegen.c src/container.c
OBJS        := $(SRCS:.c=.o)
HEADERS     := $(wildcard src/*.h)
TESTS_IN    := $(filter-out test/lib.c, $(wildcard test/*.c))
TESTS_DIFFS := $(TESTS_IN:.c=.diff)
MM          := ./9mm
MMS         := ./9mms
TEST_MM     ?= $(MM)
TEST_LIB	:= test/lib.o


$(MM): $(OBJS)
	$(CC) -Isrc -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): $(HEADERS)

$(MMS): $(MM)
	cat $(SRCS) > src/self.c
	$(MM) ./src/self.c > ./src/self.s
	$(CC) $(AFLAGS) ./src/self.s -o $@

.PHONY: test
test: $(TEST_MM) $(TEST_LIB)
	$(TEST_MM) --test
	./test.sh $(TEST_MM)
	make -s $(TESTS_DIFFS) TEST_MM=$(TEST_MM)

.PHONY: diffs
diffs: $(TESTS_DIFFS)

%.diff: %.c %.ans $(TEST_MM) $(TEST_LIB)
	$(TEST_MM) $< > $*.s
	$(CC) $(AFLAGS) -o $*.bin $*.s $(TEST_LIB)
	./$*.bin > $*.out
	diff $*.ans $*.out | tee $*.diff

.PHONY: selfhost
selfhost: $(MMS)

.PHONY: test_selfhost
test_selfhost: $(MMS)
	make -s test TEST_MM=$(MMS)

.PHONY: clean
clean:
	rm -f $(MM) $(OBJS) tmp tmp.s src/self.* $(MMS) test/*.s test/*.bin test/*.out $(test/lib.o) $(TESTS_DIFFS)
