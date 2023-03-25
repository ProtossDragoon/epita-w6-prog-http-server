# Compiler
CC = gcc

# Compiler flags
# pkg-config --cflags x: find compile dependency of x
# -std=c99: C standard version
# -g: make debuggable
# -fsanitize=address: help to find memory leak
# -Wall: show all warnings
# -Wextra: show extra warnings
CFLAGS = `pkg-config --cflags glib-2.0` -std=c99 -g -fsanitize=address -Wall -Wextra -pthread -D_GNU_SOURCE

# Linker flags
# pkg-config --libs x: find linker dependency of x
LDLIBS = `pkg-config --libs glib-2.0` -fsanitize=address -pthread

# File variables
EXE = print_request print_resource single_threaded multithreaded

all: $(EXE)

$(foreach f, $(EXE), $(eval $(f):))

.PHONY: clean

clean:
	${RM} $(EXE)
