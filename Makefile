# Makefile for the IPv4 extraction assignment.
#
# Targets:
#   make        - build the executable (default target, same as "make all")
#   make run    - build (if needed) and run the executable
#   make clean  - remove build artifacts

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g
TARGET  = ipv4extract
SRC     = ipv4extract.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe

.PHONY: all run clean
