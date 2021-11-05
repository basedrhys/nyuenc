CC=gcc
CFLAGS=-g -pedantic -std=gnu17 -Wall -Wextra -pthread
# -Werror

.PHONY: all
all: nyuenc

nyuenc: nyuenc.o

nyuenc.o: nyuenc.c
# nyush: nyush.o builtins/builtins.o externals/externals.o logging/logging.o redirection/redirection.o

# nyush.o: nyush.c builtins/builtins.h externals/externals.h logging/logging.h redirection/redirection.h

# builtins.o: builtins/builtins.c builtins/builtins.h

# externals.o: externals/externals.c externals/externals.h

# logging.o: logging/logging.c logging/logging.h

# redirection.o: redirection/redirection.c redirection/redirection.h

.PHONY: clean
clean:
	rm -f *.o nyush