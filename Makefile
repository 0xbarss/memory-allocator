CC = gcc
CFLAGS = -Wall -Wextra -g -Isrc

BUILD_DIR = build
TARGET = $(BUILD_DIR)/main
TEST_TARGET = $(BUILD_DIR)/unit_test

LIB_SRC = src/heap.c src/allocator.c src/strategy.c
MAIN_SRC = src/main.c
TEST_SRC = tests/unit_test.c

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(LIB_SRC) $(MAIN_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LIB_SRC) $(MAIN_SRC) -o $(TARGET)

$(TEST_TARGET): $(LIB_SRC) $(TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LIB_SRC) $(TEST_SRC) -o $(TEST_TARGET)

run: $(TARGET)
	./$(TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run test clean
