CC = cc
CFLAGS = -Wall -Wextra -g

SRC = src/main.c src/system.c src/auth.c src/init_menu.c src/main_menu.c
OBJ = $(SRC:.c=.o)
TARGET = atm

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) -lcrypto

src/%.o: src/%.c src/header.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
