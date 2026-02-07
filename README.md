# 🛡️ Evault Pro: Advanced Banking & Security Vault

![Evault Pro Preview](https://via.placeholder.com/800x400.png?text=Evault+Pro+Interface)

Evault Pro is a high-fidelity, native Windows desktop application designed to simulate a premium banking environment with a focus on "Glassmorphism" aesthetics and secure data persistence. Built entirely in C++ using the Win32 API and GDI+, it provides a seamless and visually stunning user experience.

## ✨ Core Features

- **💎 Glassmorphism UI**: A futuristic design featuring semi-transparent panels, dynamic blurs, and smooth animations using GDI+.
- **🗄️ SQLite Persistence**: Full integration with a local SQLite database for permanent storage of accounts, balances, and security credentials.
- **🔐 Multi-Profile Security**: A secure 4-profile selection system with individual 4-digit PIN verification for every user.
- **💰 Dynamic Banking Portal**: Real-time deposit and withdrawal operations with "Insufficient Funds" protection and balance revealing tools.
- **📈 Stock Market Integration**: A dedicated portal for tracking market assets (expandable).
- **🆕 Account Generator**: Built-in registration system that automatically generates unique account numbers and validates security PINs.

## 🛠️ Technology Stack

- **Language**: C++11
- **Graphics Engine**: GDI+ (Windows Graphics Device Interface)
- **Database**: SQLite3 (Embedded)
- **API**: Win32 API (Native Windows Development)
- **Compiler**: MinGW-w64 (g++)

## 🚀 Getting Started

### Prerequisites
- Windows OS (7/10/11)
- MinGW-w64 (if compiling from source)

### Running the Application
1. Download the repository.
2. Navigate to the folder.
3. Run `Evault_Pro.exe`.

### Compiling from Source
If you wish to modify and recompile the vault:
```powershell
# Compile SQLite object
gcc -c sqlite3.c -o sqlite3.o

# Compile Main Application
g++ -std=c++11 -c Evault.cpp -o Evault.o -DUNICODE -D_UNICODE

# Link Executable
g++ Evault.o sqlite3.o -o Evault_Pro.exe -mwindows -lgdiplus -lgdi32 -lcomctl32 -lole32 -luuid -static
```

## 🔑 Default Accounts
| Account Holder | Account Number | Default PIN |
| :--- | :--- | :--- |
| **jashwanth oggu** | 77367438 | 1985 |
| **chinni jaswanth** | 48528372 | 4066 |
| **muni charan teja** | 57422441 | 1028 |
| **jane SQLite** | 84536252 | 4321 |

---
*Created with ❤️ for Advanced Banking Simulations.*

## 💡 Expert Notes & Best Practices

To maintain a professional-grade repository, this project follows industry-standard "Clean Repository" practices:

- **Git Ignore Implementation**: A `.gitignore` file is included to ensure that large binary files (`.exe`), temporary object files (`.o`), and local databases (`.db`) are not tracked. This keeps the repository lightweight and prevents merge conflicts.
- **Log Management**: Build logs and error reports are excluded from the main branch. In a production environment, debugging information is generated locally and should never be part of the codebase.
- **SQLite Isolation**: The `evault.db` is handled as a local asset. While the code initializes it automatically, the data itself is private and unique to the local machine, ensuring no sensitive test data is leaked to public platforms.
- **GDI+ Resource Handling**: The application uses proactive memory management for GDI+ brushes and paths to ensure zero memory leaks during prolonged vault sessions.
