# I am not very good at Makefiles.

CFLAGS += -Wall -g
INCLUDES = -I.

all: bin/CuTestTest

bin:
	mkdir -p $@

test: bin/CuTestTest
	@bin/CuTestTest

bin/CuTestTest: test/AllTests.c test/CuTestTest.c CuTest.c | bin
	$(CC) $(CFLAGS) $(INCLUDES) -Wno-address -lm -o $@ $^

clean:
	@rm -rf *~ bin
