CC = cc
CFLAGS = -Wall -Wextra -g

SRC = src/main.c src/system.c src/auth.c src/init_menu.c src/main_menu.c src/get_valid_date.c src/get_valid_country.c src/get_valid_phone.c src/get_valid_amount.c src/get_valid_account.c
OBJ = $(SRC:.c=.o)
TARGET = atm

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) -lcrypto -lsqlite3

src/%.o: src/%.c src/header.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
