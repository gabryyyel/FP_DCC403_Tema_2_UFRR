CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -g
TARGET = edge-engine

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c

clean:
	rm -f $(TARGET)

valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)