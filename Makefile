# Output executable name
TARGET = bin/CStatsDProxy
INCLUDES = -I./ -I./lib
BASE_DIR = ./
DEFINES = -DBASE_DIR=\"$(BASE_DIR)\"

# Installation directory
INSTALL_DIR = /usr/sbin

# Source files and object files
SRC = src/main.c lib/logger.c lib/config_reader.c lib/worker.c lib/atomic.c
OBJ = $(SRC:.c=.o)

# Compiler and linker
CC = gcc
LD = gcc

# Compiler and linker flags
CFLAGS = -Wall -c -std=gnu99 -D_GNU_SOURCE
LDFLAGS = -lpthread

# Build rules
all: $(TARGET)

$(TARGET): $(OBJ)
	$(LD) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $< -o $@

install: all
	@if [ "$$(id -u)" -ne 0 ]; then \
		echo "You must be root to install."; \
		exit 1; \
	fi
	mv bin/CStatsDProxy $(INSTALL_DIR)
	useradd -r -s /bin/false CStatsDProxy
	mkdir -p /var/log/CStatsDProxy
	chown CStatsDProxy:CStatsDProxy /var/log/CStatsDProxy
	cp CStatsDProxy.service /etc/systemd/system/
	systemctl daemon-reload
	systemctl enable CStatsDProxy

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean install
