
<div align="center">

# 🏦 EVAULT BANKING PORTAL
### Secure. Native. Fast.

![C++](https://img.shields.io/badge/Language-C++11-blue?style=for-the-badge&logo=c%2B%2B)
![Platform](https://img.shields.io/badge/Platform-Windows_Win32-0078D6?style=for-the-badge&logo=windows)
![Database](https://img.shields.io/badge/Database-SQLite3-003B57?style=for-the-badge&logo=sqlite)
![Status](https://img.shields.io/badge/Status-Active-success?style=for-the-badge)

</div>

---

## 📖 About The Project

**Evault** is a high-performance, native Windows desktop banking application designed for speed and security. Built entirely in **C++** using the raw **Win32 API**, it offers a lightweight alternative to bloated web-based banking apps. It features a robust **SQLite** database backend to ensure your financial data is persistent, secure, and always available.

### ✨ Key Features

*   **🔐 Secure Authentication**: PIN-based login system protecting user accounts.
*   **👤 Account Management**: Seamless account creation with auto-generated unique IDs.
*   **💸 Real-time Transactions**: Deposit and Withdraw funds instantly.
*   **🔄 Smart Transfers**: Transfer money between accounts with recipient name verification.
*   **📜 Transaction History**: View detailed history of all your activities in a clear list format.
*   **🖥️ Native GUI**: A responsive, non-web interface using standard Windows controls.

---

## 🛠️ Technology Stack

| Component | Technology | Description |
| :--- | :--- | :--- |
| **Core Logic** | C++ (Standard 11) | High-performance, object-oriented business logic. |
| **GUI Framework** | Win32 API | Native Windows interface without heavy external frameworks. |
| **Database** | SQLite 3 | Serverless, self-contained SQL database engine. |
| **Build System** | MinGW (G++) | Compiled natively for Windows. |

---

## 🚀 Getting Started

### Prerequisites

*   Windows 10 or 11
*   MinGW (GCC/G++) Compiler
*   Git

### Installation & Build

1.  **Clone the Repository**
    ```bash
    git clone https://github.com/your-username/evault-banking.git
    cd evault-banking
    ```

2.  **Compile the Application**
    Run the following command in your terminal/PowerShell:
    ```powershell
    g++ -std=c++11 -o Evault.exe Evault.cpp sqlite3.o -lgdi32 -mwindows -static-libgcc -static-libstdc++
    ```
    *Note: Ensure `sqlite3.o` is present in the directory or compile it from source.*

3.  **Run Evault**
    ```powershell
    .\Evault.exe
    ```

---

## 📸 Usage Guide

1.  **Register**: Click **"Create New Account"**, enter your details, and set a 4-digit PIN.
2.  **Login**: Use your generated Account Number and PIN to access the dashboard.
3.  **Transact**:
    *   **Deposit**: Add funds to your own account.
    *   **Transfer**: Send money to another user (Enter their Account Number). *The app will verify and show their name in history.*
4.  **Logout**: Securely sign out when finished.

---

## 📂 Project Structure

```
Evault/
├── Evault.cpp        # 🧠 Main Application Source & Entry Point
├── sqlite3.h         # 🗄️ Database Header
├── sqlite3.o         # 📦 Database Object File
├── evault.db         # 💾 Persistent Database File (Auto-created)
└── README.md         # 📄 Project Documentation
```

---

<div align="center">
    <b>Star ⭐ this repo if you find it useful!</b><br>
    Made with ❤️ by Evault Team
</div>
