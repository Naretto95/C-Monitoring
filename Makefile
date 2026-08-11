srcdir  = src/
bindir  = bin/

SRC  = $(wildcard $(srcdir)*.c)
HEAD = $(wildcard $(srcdir)*.h)
OBJ  = $(patsubst $(srcdir)%.c,$(bindir)%.o,$(SRC))

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -pthread
LDFLAGS = -pthread
LDLIBS  = -lm

PROG = executable

.PHONY: all clean

all: $(bindir)$(PROG)

# relink only when an object file actually changed
$(bindir)$(PROG): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# rebuild an object file if its source or any header changed
$(bindir)%.o: $(srcdir)%.c $(HEAD) | $(bindir)
	$(CC) $(CFLAGS) -c $< -o $@

$(bindir):
	mkdir -p $(bindir)

clean:
	rm -f $(bindir)*.o $(bindir)$(PROG)
