# Compilador
CC = gcc

# Flags de compilacion
CFLAGS = -Wall -pedantic -std=c99 -g -O2 -Iinclude


SRCDIR = code
INCDIR = include
BINDIR = bin

PROGRAMS = master player view

SOURCES_master = $(SRCDIR)/master.c $(SRCDIR)/shm.c
SOURCES_player = $(SRCDIR)/player.c $(SRCDIR)/shm.c
SOURCES_view = $(SRCDIR)/view.c $(SRCDIR)/shm.c

OBJS_master = $(SOURCES_master:.c=.o)
OBJS_player = $(SOURCES_player:.c=.o)
OBJS_view = $(SOURCES_view:.c=.o)

BIN_master = $(BINDIR)/master
BIN_player = $(BINDIR)/player
BIN_view = $(BINDIR)/view


.PHONY: all
all: $(PROGRAMS)
	make clean

master: $(OBJS_master)
	@mkdir -p $(BINDIR)
	$(CC) $(LDFLAGS) -o $(BIN_master) $^

player: $(OBJS_player)
	@mkdir -p $(BINDIR)
	$(CC) $(LDFLAGS) -o $(BIN_player) $^

view: $(OBJS_view)
	@mkdir -p $(BINDIR)
	$(CC) $(LDFLAGS) -o $(BIN_view) $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<


.PHONY: clean
clean:
	rm -rf $(SRCDIR)/*.o


.PHONY: rebuild
rebuild: clean all
