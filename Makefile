# Compiler and Linker
CC = gcc
LD = gcc

# Target executable
TARGET = bin/CStatsDProxy

# Directories
INCLUDES = -I./ -I./lib -I./include -I./third_party/Unity/src
BASE_DIR = ./
DEFINES = -DBASE_DIR=\"$(BASE_DIR)\"

# Installation directory
INSTALL_DIR = /usr/sbin

# Source files and object files
SRC = src/main.c lib/logger.c lib/config_reader.c lib/queue.c lib/worker.c lib/global.c lib/requeue.c lib/http.c
OBJ = $(SRC:.c=.o)

# Test files and objects
TEST_SRC =  test/test_http.c test/test_runner.c test/test_logger.c test/test_global.c test/test_queue.c test/test_requeue.c test/test_config_reader.c third_party/Unity/src/unity.c
TEST_OBJ = $(TEST_SRC:.c=.o)

# Compiler flags
CFLAGS = -Wall -c

# Linker flags
LDFLAGS = -lpthread

# Build the main target (your application)
all: $(TARGET)

$(TARGET): $(OBJ)
	$(LD) $(OBJ) -o $(TARGET) $(LDFLAGS)

# Build object files from source
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $< -o $@

# Run tests
test: CFLAGS += -DTESTING -g
test: $(TEST_OBJ) $(OBJ)
	$(LD) $(TEST_OBJ) $(OBJ) -o test_runner $(LDFLAGS)
	./test_runner $(ARGS)

# Install the executable and set up necessary directories and permissions
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

# Clean all build and test files
clean:
	rm -f $(OBJ) $(TEST_OBJ) $(TARGET) test_runner

# Phony targets (to avoid naming conflicts with files)
.PHONY: all clean test install
