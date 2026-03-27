# SLAU C Programming Project — Year 1, Semester 1

A command-line shop management system written in C, developed as a Year 1, Semester 1 programming project at **SLAU (St. Lawrence University — Africa)**. The system manages product inventory, customers, sales, and users for a computer accessories sales and repair shop, using CSV files for data persistence.

---

## Table of Contents

- [Prerequisites](#prerequisites)
- [Project Structure](#project-structure)
- [How to Build and Run](#how-to-build-and-run)
- [Usage Instructions](#usage-instructions)
- [Example Run](#example-run)
- [Limitations / TODOs](#limitations--todos)
- [Contributing / Academic Integrity](#contributing--academic-integrity)
- [License](#license)
- [Contact](#contact)

---

## Prerequisites

| Requirement | Details |
|---|---|
| C compiler | `gcc` (Linux/macOS) or `gcc` via [MinGW/MSYS2](https://www.mingw-w64.org/) on Windows |
| Standard C library | Included with any standard C compiler |
| `make` | *Optional* — no Makefile is provided; a single `gcc` command is sufficient |
| Operating system | Linux, macOS, or Windows (a pre-compiled `.exe` is included for Windows) |

---

## Project Structure

```
slau-c-programming-project-yr1-sem1/
├── IT-REPAIR-SHOT/
│   ├── SHOP-MGT.c          # Main source file (enhanced version)
│   ├── SHOP-MGT.OLD.c      # Original/earlier version of the program
│   ├── SHOP-MGT.exe        # Pre-compiled Windows executable
│   ├── products.csv        # Persistent product inventory data
│   ├── customers.csv       # Persistent customer records
│   ├── sales.csv           # Persistent sales transaction records
│   └── users.csv           # User accounts and permissions
└── README.md
```

---

## How to Build and Run

### Linux / macOS

```bash
cd IT-REPAIR-SHOT
gcc -o shop_manager SHOP-MGT.c
./shop_manager
```

### Windows (MinGW / MSYS2)

```bash
cd IT-REPAIR-SHOT
gcc -o shop_manager.exe SHOP-MGT.c
shop_manager.exe
```

### Windows (pre-compiled executable)

A pre-compiled `SHOP-MGT.exe` is included. Double-click it or run it from the command prompt:

```cmd
cd IT-REPAIR-SHOT
SHOP-MGT.exe
```

---

## Usage Instructions

When the program starts it prompts for a **username and password**.

**Default credentials:**

| Username | Password |
|---|---|
| `admin` | `admin` |

After logging in, a text menu is displayed. Use the numbered options to navigate:

- **Product management** — add, list, search, and update stock for products
- **Customer management** — add, list, and search customer records
- **Sales management** — record a new sale (reduces stock automatically), view sales history
- **User management** *(admin only)* — add/disable users, assign per-module permissions
- **Reports** — low-stock alerts, sales summaries
- **Logout / Exit**

All data is read from and written back to the CSV files in the same directory as the executable. The CSV files are created automatically on first run if they do not exist.

---

## Example Run

```
=== IT REPAIR SHOP MANAGEMENT SYSTEM ===
Username: admin
Password: ****

Login successful. Welcome, admin!

======= MAIN MENU =======
1. Product Management
2. Customer Management
3. Sales Management
4. User Management
5. Reports
6. Logout
Enter choice: 5

--- Sales Summary ---
Total sales recorded: 1
Total revenue       : UGX 180,000.00
```

---

## Limitations / TODOs

- **Input validation** — some fields accept arbitrary strings; stricter validation (e.g., numeric-only phone numbers, email format checks) would improve robustness.
- **Password security** — passwords are stored as simple hashes; a proper cryptographic hashing library (e.g., bcrypt) is not used.
- **No network/multi-user support** — the system is single-process, single-user at a time; concurrent access is not safe.
- **CSV fragility** — records containing commas or quotes in field values may break CSV parsing.
- **No data backup** — although a `backups/` directory reference exists in the source, automated backup logic may be incomplete.
- **Repairs / Assemblies** — the original version (`SHOP-MGT.OLD.c`) includes repair and assembly modules that are not present in the current enhanced version.
- **Windows paths** — the program uses relative paths for CSV files, so it must be run from inside the `IT-REPAIR-SHOT/` directory.

---

## Contributing / Academic Integrity

Contributions and suggestions are welcome via pull requests or issues.

> **Academic integrity notice:** This project was submitted as coursework at SLAU. If you are a student at the same institution, please do **not** submit this code as your own work. Doing so would violate your institution's academic integrity policy.

---

## License

No license specified. All rights reserved by the author unless otherwise stated.

---

## Contact

**Elijah Musiitwa Newton**
📧 [musiitwaelijah@gmail.com](mailto:musiitwaelijah@gmail.com)
