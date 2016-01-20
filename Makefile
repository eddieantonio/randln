BIN = randln
MANSECTION = 1
MANPAGE = $(BIN).$(MANSECTION)
CFLAGS = -std=c99 -Wall -pedantic -O2

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/share/man/man$(MANSECTION)

all: $(BIN) $(MANPAGE)

test: $(BIN)
	./$< LICENSE

install: $(BIN) $(MANPAGE)
	install -d $(BINDIR) $(MANDIR)
	install -s $(BIN) $(BINDIR)/
	cp $(MANPAGE) $(MANDIR)/

clean:
	$(RM) $(BIN)
	-$(RM) -r *.dSYM/
	-$(RM) -r *.o

%.$(MANSECTION): %.$(MANSECTION).md
	pandoc --standalone --to man $< -o $@

.PHONY: all clean install test
