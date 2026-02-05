# Evault Banking Application - Technical Report

## 1. Overview
This report provides a quick explanation of the key variables and important library functions used in the **Evault Banking Portal** source code (`Evault.cpp`). The application is built using **C++**, the **Win32 API** for the GUI, and **SQLite** for the database backend.

## 2. Key Global Variables

| Variable | Type | Description |
| :--- | :--- | :--- |
| `hInst` | `HINSTANCE` | Handle to the current application instance. Used by Windows to identify the running executable. |
| `hMainWindow` | `HWND` | Handle to the main application window. Used to manipulate the window (show, hide, destroy). |
| `hFont` | `HFONT` | Handle to the custom font ("Segoe UI") used for all UI controls to give a modern look. |
| `currentUser` | `Account*` | Pointer to the currently logged-in user's `Account` object. `nullptr` if no user is logged in. |
| `db` | `Database` | Global instance of the `Database` class, managing all SQLite interactions. |
| `currentView` | `ViewState` | Enum tracking the current screen being displayed (`VIEW_HOME`, `VIEW_LOGIN`, `VIEW_REGISTER`, `VIEW_DASHBOARD`). |

## 3. Important Library Functions

### 3.1 Win32 API (GUI management)
These functions are from `<windows.h>` and handle the graphical interface.

- **`WinMain(...)`**: The entry point of the Windows GUI application (replaces `main()`).
- **`RegisterClass(...)`**: Registers the window class definition (style, cursor, background color, WndProc) with the OS.
- **`CreateWindow(...)`**: Creates a window. Used for both the main application window and child controls (Buttons, Text Labels, Edit Fields).
  - *Note*: We use "STATIC" for labels, "BUTTON" for buttons, "EDIT" for inputs, and "LISTBOX" for history.
- **`GetMessage(...)` / `DispatchMessage(...)`**: The core "Message Loop". It retrieves input events (clicks, typing) from the OS and sends them to the window procedure.
- **`WndProc(...)`**: The "Window Procedure" callback. It handles all events sent to the window (e.g., `WM_COMMAND` when a button is clicked, `WM_CREATE` on startup).
- **`SendMessage(...)`**: Sends a message to a window/control. We use it to set fonts (`WM_SETFONT`) or add items to the listbox (`LB_ADDSTRING`).
- **`GetWindowText(...)`**: Retrieves text entered by the user in an Input Box (`EDIT` control).
- **`MessageBox(...)`**: Displays a pop-up modal dialog for alerts, errors, or success messages.

### 3.2 SQLite API (Database Management)
These functions are from `"sqlite3.h"` and manage the `evault.db` file.

- **`sqlite3_open(...)`**: Opens the database file connection.
- **`sqlite3_exec(...)`**: Executes a raw SQL query (used here for creating tables `accounts` and `transactions` if they don't exist).
- **`sqlite3_prepare_v2(...)`**: Compiles an SQL statement into a byte-code object (`sqlite3_stmt`). Used for parameterized queries (preventing SQL Injection).
- **`sqlite3_bind_...(...)`**: Binds values (text, double, int) to the `?` placeholders in prepared statements.
  - `sqlite3_bind_text`: Binds string data.
  - `sqlite3_bind_double`: Binds floating-point numbers (amounts).
- **`sqlite3_step(...)`**: Runs the prepared statement. Returns `SQLITE_ROW` if data is returned (SELECT), or `SQLITE_DONE` if finished (INSERT/UPDATE).
- **`sqlite3_column_...(...)`**: Extracts data from a result row (e.g., `sqlite3_column_text` for strings).
- **`sqlite3_finalize(...)`**: Destroys the prepared statement object to free memory.

### 3.3 C++ Standard Library
- **`std::stod(...)`**: Converts a string to a `double`. Used to parse transaction amounts from user input.
- **`std::to_string(...)`**: Converts numbers to string format.
- **`rand()` / `srand()`**: Generates pseudo-random numbers, used for creating unique account numbers.
- **`time()`**: Gets the current system time, used for timestamping transactions.
