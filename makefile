# Compilador
CC = gcc

# Flags de compilación
CFLAGS = -Wall -pedantic -g -O2 -Iinclude
LDFLAGS = -lm

# Directorios
SRCDIR = src
INCDIR = include
BINDIR = bin
OBJDIR = obj

# Programas
PROGRAMS = master player view
P_LIBS = game-logic-master.c parameters-master.c processes-master.c sync-lib.c

# Archivos fuente
SOURCES_master = $(SRCDIR)/master.c $(addprefix $(SRCDIR)/libs/, shm.c sync-lib.c random.c game-logic-master.c parameters-master.c processes-master.c)
SOURCES_player = $(SRCDIR)/player.c $(addprefix $(SRCDIR)/libs/, shm.c sync-lib.c)
SOURCES_view = $(SRCDIR)/view.c $(addprefix $(SRCDIR)/libs/, shm.c sync-lib.c)

# Archivos objeto
OBJS_master = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES_master))
OBJS_player = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES_player))
OBJS_view = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES_view))

# Binarios finales
BIN_master = $(BINDIR)/master
BIN_player = $(BINDIR)/player
BIN_view = $(BINDIR)/view

# Reglas de construcción
.PHONY: all
all: $(PROGRAMS)
	make clean

# Target del master
master: $(OBJS_master)
	@mkdir -p $(BINDIR)
	$(CC) -o $(BIN_master) $^ $(LDFLAGS)

# Target del player
player: $(OBJS_player)
	@mkdir -p $(BINDIR)
	$(CC) -o $(BIN_player) $^ $(LDFLAGS)

# Target del view
view: $(OBJS_view)
	@mkdir -p $(BINDIR)
	$(CC) -o $(BIN_view) $^ $(LDFLAGS)

# Compilación de archivos objeto
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)/libs
	$(CC) $(CFLAGS) -c -o $@ $<

# Target de limpieza
.PHONY: clean
clean:
	rm -rfd $(OBJDIR)