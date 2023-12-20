CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c17 -I./include
LDFLAGS = -lpthread

SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))
EXEC = $(BUILD_DIR)/da

all: mkdir_build $(EXEC)

mkdir_build:
	@mkdir -p $(BUILD_DIR)

$(EXEC): $(OBJ) main.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

install:
	cp $(EXEC) /usr/local/bin/da-daemon
