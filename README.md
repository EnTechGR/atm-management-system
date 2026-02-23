<div align="center">

```
    _  _____ __  __
   / \|_   _|  \/  |
  / _ \ | | | |\/| |
 / ___ \| | | |  | |
/_/   \_|_| |_|  |_|
```

# Bank Management System

**A terminal-based ATM application written in C**

[![Language](https://img.shields.io/badge/Language-C%20%28C11%29-blue.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Database](https://img.shields.io/badge/Database-SQLite3-lightgrey.svg)](https://www.sqlite.org/)
[![UI](https://img.shields.io/badge/UI-ncurses-green.svg)](https://invisible-island.net/ncurses/)
[![Security](https://img.shields.io/badge/Security-Salted%20SHA--256-orange.svg)](https://www.openssl.org/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](#license)

A fully-featured, multi-user banking terminal application with a rich TUI, salted password hashing, embedded SQLite persistence, real-time inter-user notifications via POSIX named FIFOs, and a thread-safe asynchronous notification engine.

</div>

---

## Table of Contents

- [Features](#features)
- [Screenshots](#screenshots)
- [Architecture](#architecture)
- [Project Structure](#project-structure)
- [Prerequisites](#prerequisites)
- [Building](#building)
- [Running](#running)
- [Usage Guide](#usage-guide)
- [Security Model](#security-model)
- [Database Schema](#database-schema)
- [Account Types & Interest](#account-types--interest)
- [IPC Notification System](#ipc-notification-system)
- [Module Reference](#module-reference)
- [Contributing](#contributing)
- [License](#license)

---

## Features

| Category | Details |
|----------|---------|
| **Authentication** | User registration & login with salted SHA-256 password hashing |
| **Account Management** | Create, view, update, and delete bank accounts |
| **Transactions** | Deposits and withdrawals with overdraft protection |
| **Ownership Transfer** | Transfer any account to another registered user |
| **Real-time Notifications** | Recipient is notified instantly when an account is transferred to them |
| **Multi-account Support** | Each user can hold unlimited accounts across all types |
| **Interest Projections** | Live interest calculations displayed per account type |
| **Input Validation** | All user input is validated client-side with contextual error messages |
| **Persistent Storage** | All data survives restarts via an embedded SQLite3 database |
| **Rich TUI** | Full-colour ncurses interface with modal dialogs, scrollable menus, and live status bars |

---

## Screenshots

```
┌──────────────────────────────────────────────────────────────┐
│                    BANK MANAGEMENT SYSTEM                     │  ← Header bar
├──────────────────────────────────────────────────────────────┤
│                                                              │
│        _  _____ __  __                                       │
│       / \|_   _|  \/  |                                      │
│      / _ \ | | | |\/| |       ATM ASCII logo                 │
│     / ___ \| | | |  | |                                      │
│    /_/   \_|_| |_|  |_|                                      │
│                                                              │
│          B A N K   M A N A G E M E N T   S Y S T E M        │
│                                                              │
│   ┌─────────────── WELCOME ──────────────────┐              │
│   │  > Login                                 │              │
│   │    Register                              │              │
│   │    Exit                                  │              │
│   └──────────────────────────────────────────┘              │
├──────────────────────────────────────────────────────────────┤
│  Up/Down or 1-3 to navigate   ENTER to select               │  ← Footer bar
└──────────────────────────────────────────────────────────────┘
```

```
┌──────────────────────┐   ┌─────────────── OVERVIEW ──────────┐
│  ── MAIN MENU ──     │   │                                   │
│  > 1  Create Account │   │  Welcome back,                    │
│    2  Update Account │   │    Alice                          │
│    3  Check Details  │   │  ─────────────────────────────    │
│    4  View All       │   │  Accounts     : 3                 │
│    5  Transaction    │   │  Total Balance: $12,450.00        │
│    6  Remove Account │   │  ─────────────────────────────    │
│    7  Transfer       │   │  Monday, 23 Feb 2026              │
│    8  Exit           │   │  14:32                            │
└──────────────────────┘   └───────────────────────────────────┘
```

---

## Architecture

The application is structured into four horizontal layers with clean separation of concerns:

```
┌─────────────────────────────────────────────────────┐
│               PRESENTATION LAYER                    │
│   tui.c · main_menu.c · init_menu.c · ui_helper.c  │
├─────────────────────────────────────────────────────┤
│              BUSINESS LOGIC LAYER                   │
│                    system.c                         │
├────────────────────────┬────────────────────────────┤
│   AUTHENTICATION       │      VALIDATION            │
│   auth.c · init_menu.c │   validators.c             │
│                        │   get_valid_*.c            │
├────────────────────────┴────────────────────────────┤
│               PERSISTENCE LAYER                     │
│         database.c · database_oper.c                │
├─────────────────────────────────────────────────────┤
│                UTILITIES LAYER                      │
│  hashing_utils.c · ipc_utils.c · string_utils.c    │
│  file_utils.c · terminal_utils.c                   │
└─────────────────────────────────────────────────────┘
```

### Startup Sequence

```
main()
 ├── initialize_database("./data/atm.db")   # Create tables if absent
 ├── tui_init()                             # Start ncurses
 ├── initMenu(&u)                           # Login / Register screen
 │    └── getUserById()                     # Hydrate struct User from DB
 ├── pthread_create(notificationListener)  # Spawn FIFO listener thread
 └── mainMenu(u)                            # Enter main event loop
      └── [user selects operation]
           └── system.c function → DB → modal → success()
```

---

## Project Structure

```
.
├── main.c                      # Entry point — initialisation & thread management
├── header.h                    # Shared struct definitions (User, Record, Date)
├── auth.c / auth.h             # Authentication helpers (getPassword, isUsernameTaken)
├── init_menu.c                 # Active login & registration TUI screen
├── main_menu.c                 # Main navigation menu with account overview panel
├── system.c                    # All six banking operations
├── ui_helper.c                 # success() and stayOrReturn() flow helpers
│
├── ui/
│   ├── tui.c                   # Full ncurses TUI engine (modals, input, layout)
│   └── tui.h                   # TUI public API
│
├── utils/
│   ├── hashing_utils.c / .h    # Salt generation, SHA-256 hashing, verification
│   ├── ipc_utils.c / .h        # POSIX FIFO creation, notification send/receive
│   ├── string_utils.c / .h     # String helpers (toLower)
│   ├── file_utils.c / .h       # Legacy flat-file record I/O
│   └── terminal_utils.c / .h   # getch() implementation (non-ncurses contexts)
│
├── validation/
│   ├── validators.c / .h       # Pure I/O-free validator functions
│   ├── get_valid_account.c     # Account number & type input loop
│   ├── get_valid_amount.c      # Monetary amount input loop
│   ├── get_valid_country.c     # Country lookup with ISO code support
│   ├── get_valid_date.c        # Date input with today-as-default
│   └── get_valid_phone.c       # Phone number input loop
│
├── database/
│   ├── database.c / .h         # Schema initialisation (CREATE TABLE IF NOT EXISTS)
│   └── database_oper.c         # Connection open/close helpers
│
└── data/
    ├── atm.db                  # SQLite database (auto-created on first run)
    └── countries.txt           # Country name / ISO code lookup table
```

---

## Prerequisites

### Linux (Ubuntu / Debian)

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    libsqlite3-dev \
    libncurses-dev \
    libssl-dev
```

> `pthread` is included with glibc — no separate package is needed.

### macOS (Homebrew)

```bash
brew install sqlite ncurses openssl

# Make headers discoverable by the compiler
export CFLAGS="-I$(brew --prefix)/include"
export LDFLAGS="-L$(brew --prefix)/lib"
```

### Arch Linux

```bash
sudo pacman -S base-devel sqlite ncurses openssl
```

---

## Building

### Using Make (recommended)

```bash
# Clone the repository
git clone https://github.com/<your-username>/bank-management-system.git
cd bank-management-system

# Create the data directory
mkdir -p data

# Build
make

# Binary is placed at ./atm
```

### Manual GCC Command

```bash
gcc -Wall -Wextra -g -std=c11 \
    main.c auth.c system.c init_menu.c main_menu.c ui_helper.c \
    ui/tui.c \
    utils/hashing_utils.c utils/ipc_utils.c utils/terminal_utils.c \
    utils/string_utils.c utils/file_utils.c \
    validation/validators.c validation/get_valid_account.c \
    validation/get_valid_amount.c validation/get_valid_country.c \
    validation/get_valid_date.c validation/get_valid_phone.c \
    database/database.c database/database_oper.c \
    -o atm \
    -lsqlite3 -lncurses -lmenu -lpthread -lssl -lcrypto
```

### Clean Build

```bash
make clean      # Remove object files and binary
make clean all  # Full rebuild from scratch
```

---

## Running

> **Important:** Always launch the binary from the project root directory. The application resolves `./data/atm.db` and `data/countries.txt` relative to your working directory.

```bash
# From the project root:
./atm
```

The database (`data/atm.db`) is created automatically on the first run. No manual setup is required.

---

## Usage Guide

### First Run — Register an Account

1. At the welcome screen, select **Register**
2. Enter a username (letters `a-z` / `A-Z` only)
3. Enter a password (8–12 alphanumeric characters, confirmed by re-entry)
4. Select **Login** and enter your credentials

### Creating a Bank Account

1. From the main menu, choose **Create New Account**
2. Fill in all fields in sequence:
   - **Account Number** — a positive integer unique to your profile
   - **Date** — `MM/DD/YYYY`, or press `Enter` to use today's date
   - **Country** — full name, 2-letter ISO (`US`) or 3-letter ISO (`DEU`)
   - **Phone** — 8 to 10 digits
   - **Initial Deposit** — positive amount, up to 2 decimal places
   - **Account Type** — select from the type menu (see [Account Types](#account-types--interest))

### Making a Transaction

1. Select **Make a Transaction**
2. Pick an account from the scrollable list
3. Choose **Deposit** or **Withdrawal**
4. Enter the amount

> Fixed-term accounts (`fixed01`, `fixed02`, `fixed03`) are locked for the full term — transactions are blocked until maturity.

### Transferring Account Ownership

1. Select **Transfer Ownership**
2. Pick the account you want to transfer
3. Enter the recipient's username
4. Confirm the transfer

The recipient will see a real-time notification modal the next time their `tui_getch()` loop polls — within 300 ms if they are actively using the application.

### Navigation

| Key | Action |
|-----|--------|
| `↑` / `↓` or `j` / `k` | Move selection |
| `Enter` | Confirm selection |
| `1`–`9` | Jump directly to item |
| `Esc` or `q` | Cancel / go back |
| `Y` / `N` | Yes / No in confirm dialogs |
| `←` / `→` | Toggle Yes/No in confirm dialogs |

---

## Security Model

### Password Storage

Passwords are never stored in plain text. The storage format is:

```
<salt_hex>$<hash_hex>
```

| Component | Detail |
|-----------|--------|
| Salt | 16 bytes of cryptographically random data via `RAND_bytes()` (OpenSSL CSPRNG) |
| Hash | SHA-256 of `password ‖ salt` using the OpenSSL EVP API |
| Encoding | Both components stored as lowercase hexadecimal strings |
| Separator | `$` character — the salt is always the first 32 hex chars |

### SQL Injection Prevention

Every database query uses **parameterised statements** (`sqlite3_prepare_v2` + `sqlite3_bind_*`). There is no string concatenation into SQL anywhere in the codebase, making SQL injection architecturally impossible.

### Input Masking

Password fields in the TUI render each character as `*` via the `password` flag in `tui_input()`. The raw password string is never written to any log file or the database.

---

## Database Schema

```sql
CREATE TABLE users (
    id       INTEGER PRIMARY KEY AUTOINCREMENT,
    name     TEXT    NOT NULL,
    password TEXT    NOT NULL   -- format: salt_hex$hash_hex
);

CREATE TABLE accounts (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id       INTEGER NOT NULL,          -- FK → users.id
    account_id    INTEGER NOT NULL,          -- user-chosen number
    creation_date TEXT    NOT NULL,          -- ISO 8601: YYYY-MM-DD
    country       TEXT,
    phone         TEXT,
    balance       REAL    DEFAULT 0.0,
    account_type  TEXT,                      -- see account types below
    FOREIGN KEY (user_id) REFERENCES users(id)
);
```

The schema is initialised with `CREATE TABLE IF NOT EXISTS` on every startup, making the first-run setup fully automatic and subsequent runs a no-op.

---

## Account Types & Interest

| Type | Rate | Term | Interest Calculation |
|------|------|------|----------------------|
| `savings` | 7% / year | Rolling | `balance × 0.07 ÷ 12` paid monthly on the deposit day |
| `current` | 0% | N/A | No interest |
| `fixed01` | 4% / year | 1 year | `balance × 0.04 × 1` paid at maturity |
| `fixed02` | 5% / year | 2 years | `balance × 0.05 × 2` paid at maturity |
| `fixed03` | 8% / year | 3 years | `balance × 0.08 × 3` paid at maturity |

Interest projections are displayed live in the **Check Account Details** screen. Fixed-term accounts block all deposits and withdrawals for the duration of the term.

---

## IPC Notification System

The application implements a real-time inter-user notification system using **POSIX named FIFOs**.

### How It Works

```
Sender (transferOwnership)          Recipient (logged-in session)
─────────────────────────           ──────────────────────────────
notifyUser("alice", msg)            notificationListener thread
  └── open(/tmp/atm_fifo_alice,       └── read() polls every 1s
          O_WRONLY|O_NONBLOCK)            └── tui_set_notification(msg)
      └── write(msg)                          └── mutex write to
                                                  g_pending_notif[]

                                    Main thread (tui_getch)
                                      └── wtimeout(win, 300ms)
                                          └── polls g_pending_notif
                                              └── tui_modal_notification()
                                                  shows modal, waits
                                                  for keypress, flushes
                                                  input both sides
```

### Thread Safety

The notification thread **never calls any ncurses function**. It only writes to a `pthread_mutex_t`-protected global string buffer. The main thread polls this buffer every 300 ms via `wtimeout()` — the only correct approach, since ncurses is single-threaded by design.

The previous approach of calling `ungetch(KEY_F(12))` from a background thread was removed: `ungetch` is not thread-safe and caused corrupted input or dropped notifications.

### FIFO Locations

```
/tmp/atm_fifo_<username_lowercase>
```

FIFOs are created on login and persist across sessions. The writer uses `O_NONBLOCK` so a transfer to an offline user never stalls the sender.

---

## Module Reference

| File | Exports | Description |
|------|---------|-------------|
| `main.c` | `main()` | Subsystem init, thread lifecycle |
| `auth.c` | `getPassword()`, `isUsernameTaken()` | DB-backed credential helpers |
| `init_menu.c` | `initMenu()`, `getUserById()` | ncurses login/register screens |
| `main_menu.c` | `mainMenu()` | Navigation event loop with overview panel |
| `system.c` | `createNewAcc()` … `transferOwnership()` | All banking operations |
| `ui_helper.c` | `success()`, `stayOrReturn()` | Post-operation flow control |
| `ui/tui.c` | Full TUI API | ncurses engine: modals, input, layout, notifications |
| `utils/hashing_utils.c` | `generateSalt()`, `hashPassword()`, `verifyPassword()` | OpenSSL EVP password security |
| `utils/ipc_utils.c` | `createUserPipe()`, `notifyUser()`, `notificationListener()` | POSIX FIFO IPC |
| `validation/validators.c` | `val_*()` functions | Pure, I/O-free input validators |
| `database/database.c` | `initialize_database()` | Idempotent schema initialisation |

---

## Contributing

Contributions are welcome. Please follow the conventions already established in the codebase.

### Code Style

- **Naming:** `snake_case` for local variables, `camelCase` for public functions, `_camelCase` for `static` private functions, `ALL_CAPS` for macros
- **Headers:** every `.c` file that exposes a public API has a matching `.h` with an include guard
- **Validators:** new validators go in `validation/validators.c` / `.h` and must follow the `int val_*(const char *input, ..., char err_out[VALIDATOR_ERR_BUF])` contract — no I/O, no side effects
- **Database:** all queries must use parameterised statements — no string concatenation into SQL

### Pull Request Process

1. Fork the repository and create a feature branch: `git checkout -b feature/my-feature`
2. Build with `make clean all` and confirm zero warnings under `-Wall -Wextra`
3. Run under `valgrind --leak-check=full ./atm` and confirm no memory errors
4. Open a pull request with a clear description of what changed and why

---

## License

This project is licensed under the MIT License.

```
MIT License

Copyright (c) 2026

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

<div align="center">

Built with C · ncurses · SQLite3 · OpenSSL · pthreads

</div>