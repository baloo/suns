
CFLAGS += -D_GNU_SOURCE
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Werror
CFLAGS += -Wmissing-declarations
CFLAGS += -std=c99

OBJS=suns.o

.PHONY: all
all: suns

suns: $(OBJS)

.PHONY: clean
clean:
	rm -f suns $(OBJS)

.PHONY: format
format:
	clang-format-3.7 -i *.c
