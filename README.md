# 🛡️ Evault Pro: Advanced Banking & Security Vault

![Evault Pro Banner](https://via.placeholder.com/1200x400.png?text=Evault+Pro:+The+Future+of+Secure+Banking)

**Evault Pro** is a high-fidelity, native Windows desktop application that redefines the digital vault experience. Built with a "Security-First" philosophy, it combines the power of C++ with a stunning **Glassmorphism** interface, providing a premium, high-performance environment for managing assets, trading stocks, and securing private data.

---

## 📸 Visual Showcase

Evault Pro features a state-of-the-art interface designed for clarity and elegance.

| **Glassmorphism UI** | **P2P Banking** | **Stock Engine** |
| :--- | :--- | :--- |
| ![UI Preview](https://via.placeholder.com/400x250.png?text=Glass+Interface) | ![Banking Preview](https://via.placeholder.com/400x250.png?text=Secure+Transfers) | ![Stock Preview](https://via.placeholder.com/400x250.png?text=Market+Simulation) |
| *Semi-transparent panels with dynamic blurs.* | *Real-time account validation and fund isolation.* | *Dynamic price tracking and portfolio management.* |

---

## 🚀 Key Features

### 💎 Elite Design System
- **Next-Gen Aesthetics**: Multi-layered Glassmorphism panels utilizing GDI+ Alpha-blending.
- **Micro-Animations**: Smooth view transitions and hover effects for a responsive desktop feel.
- **Double-Buffered Rendering**: Ensures a flicker-free experience even during intensive UI updates.

### 💰 Comprehensive Banking Suite
- **Dynamic Account Management**: Instantly generate unique account numbers with secure 4-digit PINs.
- **Peer-to-Peer (P2P) Transfers**: Securely transfer funds between profiles with real-time balance updates.
- **Persistent Ledger**: Every transaction is recorded in an embedded SQLite database, ensuring zero data loss.

### 📈 High-Fidelity Stock Market
- **Real-Time Simulation**: Trade volatile assets including APPL, TSLA, GOOG, and high-performance cryptocurrencies like BTC and ETH.
- **Portfolio Analytics**: Track your quantity, average purchase price, and total equity in real-time.
- **Price History Visualization**: Conceptualized trend visualization for market monitoring.

### 🔐 Advanced Security Protocols
- **4-Profile Isolation**: Support for multiple users on a single machine, each with a private vault.
- **PIN-Gate Verification**: Every critical operation requires biometric-style 4-digit PIN confirmation.
- **Secure Data Persistence**: Industry-standard SQLite integration for encrypted-style local storage.

---

## 🛠️ Technical Deep Dive

Evault Pro is engineered for performance and reliability using a native Windows stack.

### Engineering Stack
*   **Core**: C++11 (Object-Oriented Architecture)
*   **UI Engine**: GDI+ (Windows Graphics Device Interface)
*   **Storage**: SQLite3 (C-Compatible SQL Engine)
*   **Graphics**: Custom Win32 Message Loop with Double Buffering

### Database Schema
The application utilizes a relational database structure designed for speed and integrity:
```sql
CREATE TABLE accounts (
    acc_num TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    pin TEXT NOT NULL,
    balance REAL DEFAULT 0
);

CREATE TABLE stocks (
    acc_num TEXT,
    symbol TEXT,
    quantity INTEGER,
    avg_price REAL,
    FOREIGN KEY(acc_num) REFERENCES accounts(acc_num)
);
```

### Performance Optimization
- **Resource Management**: Proactive cleanup of GDI+ objects (Brushes, Paths, Fonts) to maintain a minimal memory footprint.
- **Event-Driven UI**: Efficient Win32 message handling reduces CPU overhead during idle states.

---

## 🏗️ Getting Started

### Prerequisites
- **OS**: Windows 7/10/11
- **Compiler**: MinGW-w64 (if building from source)

### Installation
1.  Clone the repository.
2.  Navigate to the project root.
3.  Execute `Evault_Pro.exe`.

### Build from Source
```powershell
# Compile SQLite object
gcc -c sqlite3.c -o sqlite3.o

# Compile Main Application
g++ -std=c++11 -c Evault.cpp -o Evault.o -DUNICODE -D_UNICODE

# Link Executable
g++ Evault.o sqlite3.o -o Evault_Pro.exe -mwindows -lgdiplus -lgdi32 -lcomctl32 -lole32 -luuid -static
```

---

## 🗺️ Roadmap
- [ ] **Transaction History**: Comprehensive log of all deposit/withdrawal/transfer events.
- [ ] **Advanced Charts**: Interactive candlestick charts for the Stock Market portal.
- [ ] **Dark/Light Mode**: Dynamic theme switching for personalized aesthetics.
- [ ] **Data Export**: Export your banking report as a PDF or CSV.

---
*Developed with a commitment to Visual Excellence and Software Integrity.*
