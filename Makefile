CC = gcc
CFLAGS = -Wall -Wextra -g

TARGET = build/main
SRC = src/main.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
