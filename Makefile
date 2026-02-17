CC     = cc
CFLAGS = -Wall -Wextra -g

# ── Sources ──────────────────────────────────────────────────────────────────
SRC = src/main.c \
      src/menu/system.c \
      src/auth.c \
      src/menu/init_menu.c \
      src/menu/main_menu.c \
      src/validation/get_valid_date.c \
      src/validation/get_valid_country.c \
      src/validation/get_valid_phone.c \
      src/validation/get_valid_amount.c \
      src/validation/get_valid_account.c \
      src/database/database.c \
      src/utils/terminal_utils.c \
      src/utils/hashing_utils.c \
      src/utils/file_utils.c \
      src/ui_helper.c \
      src/utils/ipc_utils.c \
      src/ui/tui.c

OBJ    = $(SRC:.c=.o)
TARGET = atm

# ── Link ─────────────────────────────────────────────────────────────────────
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) \
	    -lncurses -lmenu -lform \
	    -lcrypto -lsqlite3 -lpthread

# ── Compile: general rule ─────────────────────────────────────────────────────
src/%.o: src/%.c src/header.h
	$(CC) $(CFLAGS) -c $< -o $@

# ── Compile: files with unique dependency sets ────────────────────────────────
src/ui/tui.o: src/ui/tui.c src/ui/tui.h
	$(CC) $(CFLAGS) -c $< -o $@

src/utils/terminal_utils.o: src/utils/terminal_utils.c src/utils/terminal_utils.h
	$(CC) $(CFLAGS) -c $< -o $@

src/utils/hashing_utils.o: src/utils/hashing_utils.c src/utils/hashing_utils.h
	$(CC) $(CFLAGS) -c $< -o $@

src/auth.o: src/auth.c src/auth.h src/header.h \
            src/utils/terminal_utils.h src/utils/hashing_utils.h
	$(CC) $(CFLAGS) -c $< -o $@

src/database/database.o: src/database/database.c src/database/database.h
	$(CC) $(CFLAGS) -c $< -o $@

src/utils/ipc_utils.o: src/utils/ipc_utils.c src/utils/ipc_utils.h
	$(CC) $(CFLAGS) -c $< -o $@

# ── Utility ───────────────────────────────────────────────────────────────────
.PHONY: clean install-deps

clean:
	rm -f $(OBJ) $(TARGET)

# Install required system libraries (Debian/Ubuntu)
install-deps:
	sudo apt-get update
	sudo apt-get install -y \
	    build-essential \
	    libncurses-dev \
	    libsqlite3-dev \
	    libssl-dev