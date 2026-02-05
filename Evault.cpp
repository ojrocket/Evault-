/*
 * ╔══════════════════════════════════════════════════════════════════════════╗
 * ║                         EVAULT BANKING PORTAL (GUI)                      ║
 * ║                      Secure Banking Application                          ║
 * ║                                                                          ║
 * ║  Features: Account Creation, Login, Deposit, Withdraw, Transfer          ║
 * ║  Tech Stack: C++, Win32 API, SQLite                                      ║
 * ╚══════════════════════════════════════════════════════════════════════════╝
 */

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#define _WIN32_WINNT 0x0600
#define _WIN32_IE 0x0900

#include "sqlite3.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>


using namespace std;

// Global variables for UI
HINSTANCE hInst;
HWND hMainWindow;
HFONT hFont;

// Control IDs
#define ID_BTN_LOGIN 101
#define ID_BTN_REGISTER 102
#define ID_BTN_SUBMIT_LOGIN 103
#define ID_BTN_SUBMIT_REGISTER 104
#define ID_BTN_LOGOUT 105
#define ID_BTN_DEPOSIT 106
#define ID_BTN_WITHDRAW 107
#define ID_BTN_TRANSFER 108
#define ID_BTN_BACK 110

#define ID_EDIT_USER 201
#define ID_EDIT_PIN 202
#define ID_EDIT_NAME 203
#define ID_EDIT_AMOUNT 204
#define ID_EDIT_TARGET 205

#define ID_LIST_HISTORY 303

// Helper for strings
wstring toWString(const string &str) {
  if (str.empty())
    return wstring();
  int size_needed =
      MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
  wstring wstrTo(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0],
                      size_needed);
  return wstrTo;
}

string toString(const wstring &wstr) {
  if (wstr.empty())
    return string();
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(),
                                        NULL, 0, NULL, NULL);
  string strTo(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0],
                      size_needed, NULL, NULL);
  return strTo;
}

namespace EvaultApp {

enum class TransactionType { DEPOSIT, WITHDRAW, TRANSFER_IN, TRANSFER_OUT };

class Transaction {
public:
  int transactionId;
  string accountNumber;
  TransactionType type;
  double amount;
  time_t timestamp;
  string targetAccount;
  string targetName; // Added field

  Transaction(int id, const string &accNum, TransactionType t, double amt,
              const string &target = "", const string &tName = "")
      : transactionId(id), accountNumber(accNum), type(t), amount(amt),
        timestamp(time(nullptr)), targetAccount(target), targetName(tName) {}

  string getTypeString() const {
    switch (type) {
    case TransactionType::DEPOSIT:
      return "DEPOSIT";
    case TransactionType::WITHDRAW:
      return "WITHDRAW";
    case TransactionType::TRANSFER_IN:
      return "TRANSFER IN";
    case TransactionType::TRANSFER_OUT:
      return "TRANSFER OUT";
    default:
      return "UNKNOWN";
    }
  }
};

class Account {
private:
  string accountNumber;
  string holderName;
  string pin;
  double balance;

public:
  Account() : balance(0.0) {}
  Account(const string &accNum, const string &name, const string &pinCode,
          double initialBalance)
      : accountNumber(accNum), holderName(name), pin(pinCode),
        balance(initialBalance) {}

  string getAccountNumber() const { return accountNumber; }
  string getHolderName() const { return holderName; }
  string getPin() const { return pin; }
  double getBalance() const { return balance; }

  bool deposit(double amount) {
    if (amount <= 0)
      return false;
    balance += amount;
    return true;
  }

  bool withdraw(double amount) {
    if (amount <= 0 || amount > balance)
      return false;
    balance -= amount;
    return true;
  }

  bool verifyPin(const string &inputPin) const { return pin == inputPin; }
};

class Database {
private:
  sqlite3 *db;
  map<string, Account> accountsCache;
  int nextTransactionId;

  void initializeDatabase() {
    if (!db)
      return;
    sqlite3_exec(db,
                 "CREATE TABLE IF NOT EXISTS accounts (account_number TEXT "
                 "PRIMARY KEY, holder_name TEXT, pin TEXT, balance REAL, "
                 "created_at DATETIME DEFAULT CURRENT_TIMESTAMP);",
                 0, 0, 0);
    sqlite3_exec(
        db,
        "CREATE TABLE IF NOT EXISTS transactions (id INTEGER PRIMARY KEY "
        "AUTOINCREMENT, account_number TEXT, type TEXT, amount REAL, "
        "target_account TEXT, timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);",
        0, 0, 0);

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db,
                           "SELECT COALESCE(MAX(id), 0) + 1 FROM transactions;",
                           -1, &stmt, 0) == SQLITE_OK) {
      if (sqlite3_step(stmt) == SQLITE_ROW)
        nextTransactionId = sqlite3_column_int(stmt, 0);
      sqlite3_finalize(stmt);
    }
  }

  string generateAccountNumber() {
    static bool seeded = false;
    if (!seeded) {
      srand(time(NULL));
      seeded = true;
    }
    string accNum;
    do {
      accNum = "";
      for (int i = 0; i < 8; i++)
        accNum += to_string(rand() % 10);
    } while (accountsCache.count(accNum));
    return accNum;
  }

public:
  Database() : db(nullptr), nextTransactionId(1) {}

  bool init(const string &filename) {
    if (sqlite3_open(filename.c_str(), &db) == SQLITE_OK) {
      initializeDatabase();
      reloadAccounts();
      return true;
    }
    return false;
  }

  ~Database() {
    if (db)
      sqlite3_close(db);
  }

  string createNewAccountNumber() { return generateAccountNumber(); }

  bool saveAccount(const Account &acc) {
    if (!db)
      return false;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(
        db, "INSERT INTO accounts VALUES(?,?,?,?, CURRENT_TIMESTAMP);", -1,
        &stmt, 0);
    sqlite3_bind_text(stmt, 1, acc.getAccountNumber().c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, acc.getHolderName().c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, acc.getPin().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, acc.getBalance());
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    if (ok)
      accountsCache[acc.getAccountNumber()] = acc;
    return ok;
  }

  bool updateAccount(const Account &acc) {
    if (!db)
      return false;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
                       "UPDATE accounts SET balance=? WHERE account_number=?;",
                       -1, &stmt, 0);
    sqlite3_bind_double(stmt, 1, acc.getBalance());
    sqlite3_bind_text(stmt, 2, acc.getAccountNumber().c_str(), -1,
                      SQLITE_TRANSIENT);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    if (ok)
      accountsCache[acc.getAccountNumber()] = acc;
    return ok;
  }

  Account *findAccount(const string &accNum) {
    if (accountsCache.count(accNum))
      return &accountsCache[accNum];
    return nullptr;
  }

  void reloadAccounts() {
    accountsCache.clear();
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT * FROM accounts;", -1, &stmt, 0);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      string num = (const char *)sqlite3_column_text(stmt, 0);
      string name = (const char *)sqlite3_column_text(stmt, 1);
      string pin = (const char *)sqlite3_column_text(stmt, 2);
      double bal = sqlite3_column_double(stmt, 3);
      accountsCache[num] = Account(num, name, pin, bal);
    }
    sqlite3_finalize(stmt);
  }

  bool saveTransaction(const Transaction &t) {
    if (!db)
      return false;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
                       "INSERT INTO transactions (account_number, type, "
                       "amount, target_account) VALUES (?,?,?,?);",
                       -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, t.accountNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, t.getTypeString().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, t.amount);
    sqlite3_bind_text(stmt, 4, t.targetAccount.c_str(), -1, SQLITE_TRANSIENT);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    if (ok)
      nextTransactionId++;
    return ok;
  }

  vector<Transaction> getHistory(const string &accNum) {
    vector<Transaction> h;
    if (!db)
      return h;
    sqlite3_stmt *stmt;
    // LEFT JOIN to get target name
    string sql =
        "SELECT t.id, t.type, t.amount, t.target_account, a.holder_name "
        "FROM transactions t "
        "LEFT JOIN accounts a ON t.target_account = a.account_number "
        "WHERE t.account_number = ? ORDER BY t.id DESC;";

    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, accNum.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
      int id = sqlite3_column_int(stmt, 0);
      string typeStr = (const char *)sqlite3_column_text(stmt, 1);
      double amt = sqlite3_column_double(stmt, 2);
      const char *tgt = (const char *)sqlite3_column_text(stmt, 3);
      const char *tgtName = (const char *)sqlite3_column_text(stmt, 4);

      TransactionType tType = TransactionType::DEPOSIT;
      if (typeStr == "WITHDRAW")
        tType = TransactionType::WITHDRAW;
      else if (typeStr == "TRANSFER IN")
        tType = TransactionType::TRANSFER_IN;
      else if (typeStr == "TRANSFER OUT")
        tType = TransactionType::TRANSFER_OUT;

      h.emplace_back(id, accNum, tType, amt, tgt ? tgt : "",
                     tgtName ? tgtName : "Unknown");
    }
    sqlite3_finalize(stmt);
    return h;
  }

  int getNextTId() { return nextTransactionId; }
};

Database db;
Account *currentUser = nullptr;

bool Login(string u, string p) {
  db.reloadAccounts();
  Account *acc = db.findAccount(u);
  if (acc && acc->verifyPin(p)) {
    currentUser = acc;
    return true;
  }
  return false;
}
} // namespace EvaultApp

// ==========================================
//               UI Logic
// ==========================================

// Views
enum ViewState { VIEW_HOME, VIEW_LOGIN, VIEW_REGISTER, VIEW_DASHBOARD };
ViewState currentView = VIEW_HOME;

// Helper to Create Controls
HWND CreateLabel(LPCWSTR text, int x, int y, int w, int h, HWND parent) {
  HWND hCtrl = CreateWindow(L"STATIC", text, WS_CHILD | WS_VISIBLE | SS_LEFT, x,
                            y, w, h, parent, NULL, hInst, NULL);
  SendMessage(hCtrl, WM_SETFONT, (WPARAM)hFont, TRUE);
  return hCtrl;
}

HWND CreateButton(LPCWSTR text, int x, int y, int w, int h, HWND parent,
                  int id) {
  HWND hCtrl = CreateWindow(
      L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT, x, y, w,
      h, parent, (HMENU) static_cast<UINT_PTR>(id), hInst, NULL);
  SendMessage(hCtrl, WM_SETFONT, (WPARAM)hFont, TRUE);
  return hCtrl;
}

HWND CreateEdit(int x, int y, int w, int h, HWND parent, int id,
                bool password = false) {
  DWORD style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
  if (password)
    style |= ES_PASSWORD;
  HWND hCtrl = CreateWindow(L"EDIT", L"", style, x, y, w, h, parent,
                            (HMENU) static_cast<UINT_PTR>(id), hInst, NULL);
  SendMessage(hCtrl, WM_SETFONT, (WPARAM)hFont, TRUE);
  return hCtrl;
}

// Clear all child windows (reset view)
void ClearView(HWND hwnd) {
  HWND hChild = GetWindow(hwnd, GW_CHILD);
  while (hChild) {
    HWND hNext = GetWindow(hChild, GW_HWNDNEXT);
    DestroyWindow(hChild);
    hChild = hNext;
  }
}

void ShowHome(HWND hwnd);
void ShowLogin(HWND hwnd);
void ShowRegister(HWND hwnd);
void ShowDashboard(HWND hwnd);

void ShowHome(HWND hwnd) {
  ClearView(hwnd);
  currentView = VIEW_HOME;
  CreateLabel(L"Welcome to Evault Banking Portal", 250, 50, 300, 30, hwnd);
  CreateButton(L"Login", 300, 150, 200, 40, hwnd, ID_BTN_LOGIN);
  CreateButton(L"Create New Account", 300, 210, 200, 40, hwnd, ID_BTN_REGISTER);
}

void ShowLogin(HWND hwnd) {
  ClearView(hwnd);
  currentView = VIEW_LOGIN;
  CreateLabel(L"Login to Evault", 350, 50, 200, 30, hwnd);
  CreateLabel(L"Account Number:", 250, 120, 150, 20, hwnd);
  CreateEdit(400, 115, 200, 25, hwnd, ID_EDIT_USER);
  CreateLabel(L"PIN Code:", 250, 160, 150, 20, hwnd);
  CreateEdit(400, 155, 200, 25, hwnd, ID_EDIT_PIN, true);
  CreateButton(L"Login Securely", 300, 220, 200, 40, hwnd, ID_BTN_SUBMIT_LOGIN);
  CreateButton(L"Back", 30, 500, 100, 30, hwnd, ID_BTN_BACK);
}

void ShowRegister(HWND hwnd) {
  ClearView(hwnd);
  currentView = VIEW_REGISTER;
  CreateLabel(L"Create New Account", 330, 50, 200, 30, hwnd);
  CreateLabel(L"Full Name:", 250, 120, 150, 20, hwnd);
  CreateEdit(400, 115, 200, 25, hwnd, ID_EDIT_NAME);
  CreateLabel(L"Set PIN (4-Digits):", 250, 160, 150, 20, hwnd);
  CreateEdit(400, 155, 200, 25, hwnd, ID_EDIT_PIN, true);
  CreateLabel(L"Initial Deposit ($):", 250, 200, 150, 20, hwnd);
  CreateEdit(400, 195, 200, 25, hwnd, ID_EDIT_AMOUNT);
  CreateButton(L"Register Account", 300, 260, 200, 40, hwnd,
               ID_BTN_SUBMIT_REGISTER);
  CreateButton(L"Back", 30, 500, 100, 30, hwnd, ID_BTN_BACK);
}

void ShowDashboard(HWND hwnd) {
  ClearView(hwnd);
  currentView = VIEW_DASHBOARD;
  using namespace EvaultApp;

  if (!currentUser) {
    ShowHome(hwnd);
    return;
  }

  wstring welcome = L"Welcome, " + toWString(currentUser->getHolderName());
  wstring accNum = L"Account #" + toWString(currentUser->getAccountNumber());

  CreateLabel(welcome.c_str(), 50, 30, 400, 30, hwnd);
  CreateLabel(accNum.c_str(), 50, 60, 400, 30, hwnd);

  wstringstream ss;
  ss << L"Current Balance: $" << fixed << setprecision(2)
     << currentUser->getBalance();
  CreateLabel(ss.str().c_str(), 50, 100, 400, 30, hwnd);

  // Transaction Section
  CreateLabel(L"Transaction Controls:", 50, 150, 200, 20, hwnd);

  CreateLabel(L"Amount ($):", 50, 180, 100, 20, hwnd);
  CreateEdit(150, 175, 100, 25, hwnd, ID_EDIT_AMOUNT);

  CreateLabel(L"Target Account (To):", 270, 180, 220, 20, hwnd);
  CreateEdit(500, 175, 150, 25, hwnd, ID_EDIT_TARGET);

  CreateButton(L"Deposit", 50, 220, 100, 30, hwnd, ID_BTN_DEPOSIT);
  CreateButton(L"Withdraw", 170, 220, 100, 30, hwnd, ID_BTN_WITHDRAW);
  CreateButton(L"Transfer", 290, 220, 100, 30, hwnd, ID_BTN_TRANSFER);

  CreateButton(L"Logout", 650, 30, 100, 30, hwnd, ID_BTN_LOGOUT);

  // History
  CreateLabel(L"Recent Transactions:", 50, 270, 200, 20, hwnd);
  HWND hList = CreateWindow(
      L"LISTBOX", NULL,
      WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY, 50, 300, 700,
      220, hwnd, (HMENU)ID_LIST_HISTORY, hInst, NULL);
  SendMessage(hList, WM_SETFONT, (WPARAM)hFont, TRUE);

  auto hist = db.getHistory(currentUser->getAccountNumber());
  for (const auto &t : hist) {
    wstringstream line;
    line << L"[" << t.transactionId << L"] " << toWString(t.getTypeString())
         << L" $" << fixed << setprecision(2) << t.amount;

    if (!t.targetAccount.empty()) {
      if (t.type == TransactionType::TRANSFER_IN) {
        line << L" (From: "
             << (t.targetName.empty() || t.targetName == "Unknown"
                     ? toWString(t.targetAccount)
                     : toWString(t.targetName))
             << L")";
      } else if (t.type == TransactionType::TRANSFER_OUT) {
        line << L" (To: "
             << (t.targetName.empty() || t.targetName == "Unknown"
                     ? toWString(t.targetAccount)
                     : toWString(t.targetName))
             << L")";
      } else {
        line << L" -> " << toWString(t.targetName);
      }
    }

    SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)line.str().c_str());
  }
}

// Main Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
  case WM_CREATE:
    hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    ShowHome(hwnd);
    break;

  case WM_COMMAND: {
    int id = LOWORD(wParam);

    // Navigation
    if (id == ID_BTN_LOGIN)
      ShowLogin(hwnd);
    if (id == ID_BTN_REGISTER)
      ShowRegister(hwnd);
    if (id == ID_BTN_BACK)
      ShowHome(hwnd);
    if (id == ID_BTN_LOGOUT) {
      EvaultApp::currentUser = nullptr;
      ShowHome(hwnd);
      MessageBox(hwnd, L"Logged out successfully.", L"Evault",
                 MB_OK | MB_ICONINFORMATION);
    }

    // Login Logic
    if (id == ID_BTN_SUBMIT_LOGIN) {
      wchar_t u[50], p[50];
      GetWindowText(GetDlgItem(hwnd, ID_EDIT_USER), u, 50);
      GetWindowText(GetDlgItem(hwnd, ID_EDIT_PIN), p, 50);

      if (EvaultApp::Login(toString(u), toString(p))) {
        ShowDashboard(hwnd);
      } else {
        MessageBox(hwnd, L"Invalid Account or PIN!", L"Login Failed",
                   MB_OK | MB_ICONERROR);
      }
    }

    // Register Logic
    if (id == ID_BTN_SUBMIT_REGISTER) {
      wchar_t n[100], p[50], a[50];
      GetWindowText(GetDlgItem(hwnd, ID_EDIT_NAME), n, 100);
      GetWindowText(GetDlgItem(hwnd, ID_EDIT_PIN), p, 50);
      GetWindowText(GetDlgItem(hwnd, ID_EDIT_AMOUNT), a, 50);

      string name = toString(n);
      string pin = toString(p);
      double amt = 0.0;
      try {
        if (wcslen(a) > 0)
          amt = stod(a);
      } catch (...) {
        amt = 0.0;
      }

      if (name.empty() || pin.length() != 4) {
        MessageBox(hwnd, L"Invalid Input. Name required, PIN must be 4 digits.",
                   L"Error", MB_OK | MB_ICONERROR);
        break;
      }

      string accNum = EvaultApp::db.createNewAccountNumber();
      EvaultApp::Account newAcc(accNum, name, pin, amt);
      if (EvaultApp::db.saveAccount(newAcc)) {
        if (amt > 0)
          EvaultApp::db.saveTransaction(
              EvaultApp::Transaction(EvaultApp::db.getNextTId(), accNum,
                                     EvaultApp::TransactionType::DEPOSIT, amt));

        wstring msg = L"Account Created! Your Number: " + toWString(accNum) +
                      L"\nPlease remember it to login.";
        MessageBox(hwnd, msg.c_str(), L"Success", MB_OK | MB_ICONINFORMATION);
        ShowLogin(hwnd);
      }
    }

    // Transaction Logic
    using namespace EvaultApp;
    if (id == ID_BTN_DEPOSIT || id == ID_BTN_WITHDRAW ||
        id == ID_BTN_TRANSFER) {
      wchar_t amtStr[50], tgtStr[50];
      GetWindowText(GetDlgItem(hwnd, ID_EDIT_AMOUNT), amtStr, 50);

      double amount = 0.0;
      try {
        if (wcslen(amtStr) > 0)
          amount = stod(amtStr);
      } catch (...) {
        amount = 0.0;
      }

      if (amount <= 0) {
        MessageBox(hwnd, L"Please enter a valid amount greater than 0.",
                   L"Invalid Amount", MB_OK | MB_ICONWARNING);
        break;
      }

      if (id == ID_BTN_DEPOSIT) {
        currentUser->deposit(amount);
        db.updateAccount(*currentUser);
        db.saveTransaction(Transaction(db.getNextTId(),
                                       currentUser->getAccountNumber(),
                                       TransactionType::DEPOSIT, amount));
        MessageBox(hwnd, L"Deposit Successful!", L"Success",
                   MB_OK | MB_ICONINFORMATION);
        ShowDashboard(hwnd); // Refresh
      } else if (id == ID_BTN_WITHDRAW) {
        if (currentUser->withdraw(amount)) {
          db.updateAccount(*currentUser);
          db.saveTransaction(Transaction(db.getNextTId(),
                                         currentUser->getAccountNumber(),
                                         TransactionType::WITHDRAW, amount));
          MessageBox(hwnd, L"Withdrawal Successful!", L"Success",
                     MB_OK | MB_ICONINFORMATION);
          ShowDashboard(hwnd);
        } else {
          MessageBox(hwnd, L"Insufficient Funds!", L"Error",
                     MB_OK | MB_ICONERROR);
        }
      } else if (id == ID_BTN_TRANSFER) {
        GetWindowText(GetDlgItem(hwnd, ID_EDIT_TARGET), tgtStr, 50);
        string targetAcc = toString(tgtStr);

        if (targetAcc.empty()) {
          MessageBox(hwnd,
                     L"Please enter a target account number for transfer.",
                     L"Missing Info", MB_OK | MB_ICONWARNING);
          break;
        }

        db.reloadAccounts();

        Account *target = db.findAccount(targetAcc);
        if (!target) {
          wstring err =
              L"Target account '" + toWString(targetAcc) + L"' not found!";
          MessageBox(hwnd, err.c_str(), L"Error", MB_OK | MB_ICONERROR);
          break;
        }

        if (targetAcc == currentUser->getAccountNumber()) {
          MessageBox(hwnd, L"Cannot transfer to yourself!", L"Error",
                     MB_OK | MB_ICONWARNING);
          break;
        }

        if (currentUser->withdraw(amount)) {
          target->deposit(amount);

          db.updateAccount(*currentUser);
          db.updateAccount(*target);

          // Save transactions with target info
          db.saveTransaction(
              Transaction(db.getNextTId(), currentUser->getAccountNumber(),
                          TransactionType::TRANSFER_OUT, amount, targetAcc));
          db.saveTransaction(Transaction(db.getNextTId(), targetAcc,
                                         TransactionType::TRANSFER_IN, amount,
                                         currentUser->getAccountNumber()));

          MessageBox(hwnd, L"Transfer Successful!", L"Success",
                     MB_OK | MB_ICONINFORMATION);
          ShowDashboard(hwnd);
        } else {
          MessageBox(hwnd, L"Insufficient Funds!", L"Error",
                     MB_OK | MB_ICONERROR);
        }
      }
    }
    break;
  }

  case WM_DESTROY:
    PostQuitMessage(0);
    break;

  default:
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  hInst = hInstance;

  // Initialize Database Lazy
  if (!EvaultApp::db.init("evault.db")) {
    MessageBox(NULL, L"Failed to connect to database!", L"Critical Error",
               MB_OK | MB_ICONERROR);
    return -1;
  }

  // Register Window Class
  WNDCLASS wc = {0};
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInstance;
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.lpszClassName = L"EvaultClass";
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);

  if (!RegisterClass(&wc))
    return -1;

  // Create Window
  hMainWindow = CreateWindow(L"EvaultClass", L"Evault Banking Portal",
                             WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                                 WS_MINIMIZEBOX | WS_VISIBLE,
                             100, 100, 800, 600, NULL, NULL, hInstance, NULL);

  if (!hMainWindow)
    return -1;

  // Message Loop
  MSG msg = {0};
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return (int)msg.wParam;
}
