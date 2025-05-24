CC = cc
CFLAGS = -Wall -Wextra -g

SRC = src/main.c src/menu/system.c src/auth.c src/menu/init_menu.c src/menu/main_menu.c \
       src/validation/get_valid_date.c src/validation/get_valid_country.c src/validation/get_valid_phone.c \
       src/validation/get_valid_amount.c src/validation/get_valid_account.c src/database/database.c \
       src/utils/terminal_utils.c src/utils/hashing_utils.c \
	    src/utils/file_utils.c src/ui_helper.c src/utils/ipc_utils.c

OBJ = $(SRC:.c=.o)
TARGET = atm

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) -lcrypto -lsqlite3

src/%.o: src/%.c src/header.h
	$(CC) $(CFLAGS) -c $< -o $@

# Specific rules for object files with unique dependencies
src/utils/terminal_utils.o: src/utils/terminal_utils.c src/utils/terminal_utils.h
	$(CC) $(CFLAGS) -c $< -o $@

src/utils/hashing_utils.o: src/utils/hashing_utils.c src/utils/hashing_utils.h
	$(CC) $(CFLAGS) -c $< -o $@

src/auth.o: src/auth.c src/auth.h src/header.h src/utils/terminal_utils.h src/utils/hashing_utils.h
	$(CC) $(CFLAGS) -c $< -o $@


src/database/database.o: src/database/database.c src/database/database.h
	$(CC) $(CFLAGS) -c $< -o $@

src/utils/ipc_utils.o: src/utils/ipc_utils.c src/utils/ipc_utils.h
	$(CC) $(CFLAGS) -c src/utils/ipc_utils.c -o src/utils/ipc_utils.o

clean:
	rm -f $(OBJ) $(TARGET)