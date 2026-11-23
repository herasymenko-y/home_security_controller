
CC ?= gcc

CFLAGS = -Wall -Wextra -pthread -Iinclude
LIBS = -lgpiod -pthread

SRC = $(wildcard src/*.c)

OBJ = $(SRC:.c=.o)
TARGET = sys_daemon

.PHONY: all clean format

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

format:
	clang-format -i $(SRC) include/*.h

clean:
	rm -f $(OBJ) $(TARGET)

