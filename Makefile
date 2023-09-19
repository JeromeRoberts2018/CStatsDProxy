# Output executable name
TARGET = CStatsDProxy

# Installation directory
INSTALL_DIR = /usr/sbin

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

install: all
	@if [ "$$(id -u)" -ne 0 ]; then \
		echo "You must be root to install."; \
		exit 1; \
	fi
	mv CStatsDProxy $(INSTALL_DIR)
	useradd -r -s /bin/false CStatsDProxy
	mkdir -p /var/log/CStatsDProxy
	chown CStatsDProxy:CStatsDProxy /var/log/CStatsDProxy
	cp CStatsDProxy.service /etc/systemd/system/
	systemctl daemon-reload
	systemctl enable CStatsDProxy

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean install
