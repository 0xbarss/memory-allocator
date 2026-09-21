CC = gcc
CFLAGS = -Wall -Wextra -g -Isrc

BUILD_DIR = build
TARGET = $(BUILD_DIR)/main
TEST_TARGET = $(BUILD_DIR)/unit_test
BENCH_TARGET = $(BUILD_DIR)/compare_glibc

LIB_SRC = src/heap.c src/allocator.c src/strategy.c
MAIN_SRC = src/main.c
TEST_SRC = tests/unit_test.c
BENCH_SRC = bench/compare_glibc.c

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(LIB_SRC) $(MAIN_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LIB_SRC) $(MAIN_SRC) -o $(TARGET)

$(TEST_TARGET): $(LIB_SRC) $(TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LIB_SRC) $(TEST_SRC) -o $(TEST_TARGET)

$(BENCH_TARGET): $(LIB_SRC) $(BENCH_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -O2 $(LIB_SRC) $(BENCH_SRC) -o $(BENCH_TARGET)

run: $(TARGET)
	./$(TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

bench: $(BENCH_TARGET)
	./$(BENCH_TARGET)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run test bench clean
