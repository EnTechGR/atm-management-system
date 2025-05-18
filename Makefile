CC = cc
CFLAGS = -Wall -Wextra -g

SRC = src/main.c src/system.c src/auth.c src/init_menu.c src/main_menu.c src/get_valid_date.c src/get_valid_country.c src/get_valid_phone.c src/get_valid_amount.c src/get_valid_account.c src/database.c src/terminal_utils.c src/hashing_utils.c
OBJ = $(SRC:.c=.o)
TARGET = atm

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) -lcrypto -lsqlite3

src/%.o: src/%.c src/header.h
	$(CC) $(CFLAGS) -c $< -o $@

# Specific rules for object files with unique dependencies
src/terminal_utils.o: src/terminal_utils.c src/terminal_utils.h
	$(CC) $(CFLAGS) -c $< -o $@

src/hashing_utils.o: src/hashing_utils.c src/hashing_utils.h
	$(CC) $(CFLAGS) -c $< -o $@

src/auth.o: src/auth.c src/auth.h src/header.h src/terminal_utils.h src/hashing_utils.h
	$(CC) $(CFLAGS) -c $< -o $@


src/database.o: src/database.c src/database.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)