# Output executable name
TARGET = CStatsDProxy

# Installation directory
INSTALL_DIR = /srv/CStatsDProxy

# Source files and object files
SRC = main.c lib/logger.c lib/config_reader.c lib/queue.c lib/worker.c
OBJ = $(SRC:.c=.o)

# Compiler and linker
CC = gcc
LD = gcc

# Compiler and linker flags
CFLAGS = -Wall -c
LDFLAGS = -lpthread

# Build rules
all: $(TARGET)

$(TARGET): $(OBJ)
	$(LD) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

install: $(TARGET)
	mkdir -p $(INSTALL_DIR)
	cp $(TARGET) $(INSTALL_DIR)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean install
