CC ?= gcc

CFLAGS = -Wall -Wextra -pthread -Iinclude
LIBS = -lgpiod -pthread

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
TARGET = home_security
SERVICE = home_security.service

PREFIX ?= /usr/local
BINDIR = $(DESTDIR)$(PREFIX)/bin
SYSTEMD_DIR = $(DESTDIR)/etc/systemd/system

.PHONY: all clean format install uninstall

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

install: $(TARGET)
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/$(TARGET)
	install -d $(SYSTEMD_DIR)
	install -m 644 $(SERVICE) $(SYSTEMD_DIR)/$(SERVICE)

uninstall:
	rm -f $(BINDIR)/$(TARGET)
	rm -f $(SYSTEMD_DIR)/$(SERVICE)
format:
	clang-format -i $(SRC) include/*.h

clean:
	rm -f $(OBJ) $(TARGET)

