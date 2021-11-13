# the compiler: gcc for C program, define as g++ for C++
CC=gcc
CFLAGS=-g -pedantic -std=gnu17 -Wall -Wextra -pthread #-Werror
LIBS=-lm

# the build target executable:
TARGET=nyuenc

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c $(LIBS)

clean:
	$(RM) $(TARGET)