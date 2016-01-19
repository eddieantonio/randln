BIN = randln
CFLAGS = -std=c99 -Wall -pedantic -O2

all: $(BIN)

test: $(BIN)
	./$< LICENSE

clean:
	$(RM) $(BIN)

.PHONY: all clean test
