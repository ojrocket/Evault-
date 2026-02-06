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

#pragma comment(                                                               \
    linker,                                                                    \
    "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

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

#define ID_BTN_ENTER 111
#define ID_LIST_HISTORY 206
#define ID_BTN_PROFILE_CARD 300
#define ID_CARD_BALANCE 4000 // Base for profile cards

// Selection Tracking
string selectedProfileAccNum = "";

// Global Gray/Dark Colors
COLORREF colBlue = RGB(138, 180, 248);    // Soft Blue for dark mode
COLORREF colBg = RGB(32, 33, 36);         // Dark Gray Bg (Chrome Style)
COLORREF colCard = RGB(41, 42, 45);       // Slightly lighter card
COLORREF colWhite = RGB(232, 234, 237);   // Light Gray text
COLORREF colText = RGB(232, 234, 237);    // Dark Text
COLORREF colTextDim = RGB(154, 160, 166); // Dimmer text

HBRUSH hBrushBg;
HBRUSH hBrushCard;
HBRUSH hBrushBlue;

HFONT hFontTitle, hFontSmall, hFontBig;

// Helper for strings
wstring toWString(const string &s) {
  if (s.empty())
    return L"";
  int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
  wstring w(n, 0);
  MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
  return w;
}
string toString(const wstring &w) {
  if (w.empty())
    return "";
  int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), NULL, 0,
                              NULL, NULL);
  string s(n, 0);
  WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, NULL,
                      NULL);
  return s;
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
    sqlite3_stmt *s;
    sqlite3_prepare_v2(
        db, "INSERT INTO accounts VALUES(?,?,?,?, CURRENT_TIMESTAMP);", -1, &s,
        0);
    sqlite3_bind_text(s, 1, acc.getAccountNumber().c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(s, 2, acc.getHolderName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s, 3, acc.getPin().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(s, 4, acc.getBalance());
    bool ok = (sqlite3_step(s) == SQLITE_DONE);
    sqlite3_finalize(s);
    if (ok)
      accountsCache[acc.getAccountNumber()] = acc;
    return ok;
  }

  bool updateAccount(const Account &acc) {
    if (!db)
      return false;
    sqlite3_stmt *s;
    sqlite3_prepare_v2(
        db, "UPDATE accounts SET balance=? WHERE account_number=?;", -1, &s, 0);
    sqlite3_bind_double(s, 1, acc.getBalance());
    sqlite3_bind_text(s, 2, acc.getAccountNumber().c_str(), -1,
                      SQLITE_TRANSIENT);
    bool ok = (sqlite3_step(s) == SQLITE_DONE);
    sqlite3_finalize(s);
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

  map<string, Account> getAllAccounts() {
    reloadAccounts(); // Ensure cache is up-to-date
    return accountsCache;
  }

  bool saveTransaction(const Transaction &t) {
    if (!db)
      return false;
    sqlite3_stmt *s;
    sqlite3_prepare_v2(db,
                       "INSERT INTO transactions (account_number, type, "
                       "amount, target_account) VALUES (?,?,?,?);",
                       -1, &s, 0);
    sqlite3_bind_text(s, 1, t.accountNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s, 2, t.getTypeString().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(s, 3, t.amount);
    sqlite3_bind_text(s, 4, t.targetAccount.c_str(), -1, SQLITE_TRANSIENT);
    bool ok = (sqlite3_step(s) == SQLITE_DONE);
    sqlite3_finalize(s);
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

  // Transaction Support
  bool beginTransaction() {
    return sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0) == SQLITE_OK;
  }

  bool commitTransaction() {
    return sqlite3_exec(db, "COMMIT;", 0, 0, 0) == SQLITE_OK;
  }

  bool rollbackTransaction() {
    return sqlite3_exec(db, "ROLLBACK;", 0, 0, 0) == SQLITE_OK;
  }
};

Database db;
Account *currentUser = nullptr;

bool Login(string u, string p) {
  Account *acc = db.findAccount(u);
  if (acc && acc->verifyPin(p))
    return currentUser = acc, true;
  return false;
}
} // namespace EvaultApp

// ==========================================
//               UI Logic
// ==========================================

// View States
enum ViewState {
  VIEW_HOME,
  VIEW_LOGIN,
  VIEW_REGISTER,
  VIEW_DASHBOARD,
  VIEW_PROFILES,
  VIEW_PIN_ENTRY
};
ViewState currentView = VIEW_HOME;

// Unified Control Creator
HWND InitCtrl(LPCWSTR cls, LPCWSTR txt, int x, int y, int w, int h, HWND p,
              int id = 0, DWORD extra = 0) {
  DWORD st = WS_CHILD | WS_VISIBLE | extra;
  HWND ctrl =
      CreateWindow(cls, txt, st, x, y, w, h, p, (HMENU)(UINT_PTR)id, hInst, 0);
  SendMessage(ctrl, WM_SETFONT, (WPARAM)hFont, 1);
  return ctrl;
}
#define AddLabel(t, x, y, w, h, p)                                             \
  InitCtrl(L"STATIC", t, x, y, w, h, p, 0, SS_LEFT)
#define AddBtn(t, x, y, w, h, p, id)                                           \
  InitCtrl(L"BUTTON", t, x, y, w, h, p, id, BS_PUSHBUTTON | BS_FLAT)
#define AddEdit(x, y, w, h, p, id, pwd)                                        \
  InitCtrl(L"EDIT", L"", x, y, w, h, p, id,                                    \
           WS_BORDER | ES_AUTOHSCROLL | (pwd ? ES_PASSWORD : 0))

void LabelCenter(HWND hLabel) {
  RECT rcParent, rcLabel;
  GetClientRect(GetParent(hLabel), &rcParent);
  GetWindowRect(hLabel, &rcLabel);
  MapWindowPoints(HWND_DESKTOP, GetParent(hLabel), (LPPOINT)&rcLabel, 2);

  int labelWidth = rcLabel.right - rcLabel.left;
  int newX = (rcParent.right - labelWidth) / 2;
  SetWindowPos(hLabel, NULL, newX, rcLabel.top, 0, 0,
               SWP_NOSIZE | SWP_NOZORDER);
}

// Clear all child windows (reset view)
BOOL CALLBACK ClearChildProc(HWND child, LPARAM) {
  DestroyWindow(child);
  return TRUE;
}

void ClearView(HWND hwnd) { EnumChildWindows(hwnd, ClearChildProc, 0); }

void ShowHome(HWND hwnd);
void ShowRegister(HWND hwnd);
void ShowDashboard(HWND hwnd);
void ShowProfileSelection(HWND hwnd);
void ShowPinEntry(HWND hwnd);

void ShowHome(HWND hwnd) {
  ClearView(hwnd);
  currentView = VIEW_HOME;

  HWND hTitle = AddLabel(L"Evault", 300, 150, 200, 60, hwnd);
  SendMessage(hTitle, WM_SETFONT, (WPARAM)hFontBig, TRUE);
  LabelCenter(hTitle);

  HWND hSub = AddLabel(L"Secure. Simple. Banking.", 300, 210, 200, 25, hwnd);
  SendMessage(hSub, WM_SETFONT, (WPARAM)hFontSmall, TRUE);
  LabelCenter(hSub);

  AddBtn(L"Enter Vault", 300, 300, 200, 45, hwnd, ID_BTN_ENTER);
  AddBtn(L"Create Account", 300, 355, 200, 30, hwnd, ID_BTN_REGISTER);
}

void ShowProfileSelection(HWND hwnd) {
  ClearView(hwnd);
  currentView = VIEW_PROFILES;
  EvaultApp::db.reloadAccounts();
  auto accounts = EvaultApp::db.getAllAccounts();

  HWND hTitle = AddLabel(L"Who's using Evault?", 200, 100, 400, 40, hwnd);
  SendMessage(hTitle, WM_SETFONT, (WPARAM)hFontTitle, TRUE);
  LabelCenter(hTitle);

  int x = 100, y = 200;
  int i = 0;
  for (auto const &pair : accounts) {
    const string &accNum = pair.first;
    const EvaultApp::Account &acc = pair.second;
    HWND hCard = CreateWindow(
        L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, x, y, 140, 180,
        hwnd, (HMENU)(ID_BTN_PROFILE_CARD + i), hInst, (LPVOID)accNum.c_str());
    SetWindowLongPtr(hCard, GWLP_USERDATA, (LONG_PTR) new string(accNum));
    x += 160;
    if (x > 600) {
      x = 100;
      y += 200;
    }
    i++;
  }

  LabelCenter(
      AddBtn(L"+ Add Account", 350, 500, 120, 35, hwnd, ID_BTN_REGISTER));
}

void ShowPinEntry(HWND hwnd) {
  ClearView(hwnd);
  currentView = VIEW_PIN_ENTRY;
  auto acc = EvaultApp::db.findAccount(selectedProfileAccNum);
  if (!acc) {
    ShowProfileSelection(hwnd);
    return;
  }

  LabelCenter(
      InitCtrl(L"STATIC", L"", 350, 100, 100, 100, hwnd, 0, SS_OWNERDRAW));
  HWND hName = AddLabel(toWString(acc->getHolderName()).c_str(), 200, 210, 400,
                        30, hwnd);
  SendMessage(hName, WM_SETFONT, (WPARAM)hFontTitle, TRUE);
  LabelCenter(hName);

  LabelCenter(AddLabel(L"Enter your 4-digit PIN", 300, 260, 200, 20, hwnd));
  HWND hPin = AddEdit(325, 290, 150, 35, hwnd, ID_EDIT_PIN, true);
  SendMessage(hPin, WM_SETFONT, (WPARAM)hFontTitle, TRUE);
  LabelCenter(hPin);

  LabelCenter(AddBtn(L"Login", 350, 350, 100, 40, hwnd, ID_BTN_SUBMIT_LOGIN));
  LabelCenter(AddBtn(L"Back", 360, 400, 80, 25, hwnd, ID_BTN_BACK));
}

void ShowRegister(HWND hwnd) {
  ClearView(hwnd);
  currentView = VIEW_REGISTER;
  LabelCenter(AddLabel(L"Join Evault", 300, 50, 200, 40, hwnd));
  LabelCenter(AddLabel(L"Full Name", 300, 120, 200, 20, hwnd));
  LabelCenter(AddEdit(300, 145, 200, 30, hwnd, ID_EDIT_NAME, false));
  LabelCenter(AddLabel(L"Set 4-Digit PIN", 300, 195, 200, 20, hwnd));
  LabelCenter(AddEdit(300, 220, 200, 30, hwnd, ID_EDIT_PIN, true));
  LabelCenter(AddLabel(L"Initial Deposit ($)", 300, 270, 200, 20, hwnd));
  LabelCenter(AddEdit(300, 295, 200, 30, hwnd, ID_EDIT_AMOUNT, false));
  LabelCenter(AddBtn(L"Create Account", 300, 360, 200, 40, hwnd,
                     ID_BTN_SUBMIT_REGISTER));
  LabelCenter(AddBtn(L"Cancel", 350, 410, 100, 30, hwnd, ID_BTN_BACK));
}

void ShowDashboard(HWND hwnd) {
  ClearView(hwnd);
  currentView = VIEW_DASHBOARD;
  using namespace EvaultApp;
  if (!currentUser) {
    ShowHome(hwnd);
    return;
  }

  HWND hWelcome =
      AddLabel((L"Hi, " + toWString(currentUser->getHolderName())).c_str(), 50,
               30, 400, 40, hwnd);
  SendMessage(hWelcome, WM_SETFONT, (WPARAM)hFontTitle, TRUE);
  AddLabel((L"Account #" + toWString(currentUser->getAccountNumber())).c_str(),
           50, 75, 400, 20, hwnd);

  InitCtrl(L"STATIC", L"", 50, 110, 700, 100, hwnd, ID_CARD_BALANCE,
           SS_OWNERDRAW);

  AddLabel(L"Quick Actions", 50, 230, 200, 20, hwnd);
  AddBtn(L"Deposit", 50, 260, 120, 40, hwnd, ID_BTN_DEPOSIT);
  AddBtn(L"Withdraw", 185, 260, 120, 40, hwnd, ID_BTN_WITHDRAW);
  AddBtn(L"Transfer", 320, 260, 120, 40, hwnd, ID_BTN_TRANSFER);

  AddLabel(L"Amount ($)", 50, 320, 100, 20, hwnd);
  AddEdit(50, 345, 120, 30, hwnd, ID_EDIT_AMOUNT, false);
  AddLabel(L"Target Account", 185, 320, 120, 20, hwnd);
  AddEdit(185, 345, 255, 30, hwnd, ID_EDIT_TARGET, false);
  AddBtn(L"Logout", 650, 30, 100, 30, hwnd, ID_BTN_LOGOUT);

  AddLabel(L"Recent Activity", 50, 400, 200, 20, hwnd);
  HWND hList =
      InitCtrl(L"LISTBOX", 0, 50, 430, 700, 120, hwnd, ID_LIST_HISTORY,
               WS_VSCROLL | LBS_NOTIFY | LBS_OWNERDRAWFIXED | WS_BORDER);

  auto hist = db.getHistory(currentUser->getAccountNumber());
  for (const auto &t : hist) {
    wstringstream line;
    line << L"[" << t.transactionId << L"] " << toWString(t.getTypeString())
         << L" $" << fixed << setprecision(2) << t.amount;
    if (!t.targetAccount.empty()) {
      if (t.type == TransactionType::TRANSFER_IN)
        line << L" (From: "
             << (t.targetName == "Unknown" ? toWString(t.targetAccount)
                                           : toWString(t.targetName))
             << L")";
      else if (t.type == TransactionType::TRANSFER_OUT)
        line << L" (To: "
             << (t.targetName == "Unknown" ? toWString(t.targetAccount)
                                           : toWString(t.targetName))
             << L")";
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
    hFontTitle =
        CreateFont(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                   DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    hFontBig =
        CreateFont(64, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                   DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    hFontSmall =
        CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                   DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    hBrushBg = CreateSolidBrush(colBg);
    hBrushCard = CreateSolidBrush(colCard);
    hBrushBlue = CreateSolidBrush(colBlue);

    ShowHome(hwnd);
    break;

  case WM_CTLCOLORSTATIC: {
    HDC hdc = (HDC)wParam;
    HWND ctrl = (HWND)lParam;
    SetTextColor(hdc, colWhite);
    SetBkMode(hdc, TRANSPARENT);
    // If it's a dashboard label, it might need hBrushBg unless inside a card
    return (INT_PTR)hBrushBg;
  }
  case WM_CTLCOLORDLG:
    return (INT_PTR)hBrushBg;
  case WM_CTLCOLOREDIT: {
    HDC hdc = (HDC)wParam;
    SetTextColor(hdc, colWhite);
    SetBkColor(hdc, colCard);
    return (INT_PTR)hBrushCard;
  }

  case WM_DRAWITEM: {
    LPDRAWITEMSTRUCT pds = (LPDRAWITEMSTRUCT)lParam;

    // Profile Cards (Who's using Chrome style)
    if (pds->CtlID >= ID_BTN_PROFILE_CARD && pds->CtlType == ODT_BUTTON) {
      string *pAccNum =
          (string *)GetWindowLongPtr(pds->hwndItem, GWLP_USERDATA);
      if (!pAccNum)
        return TRUE; // Should not happen
      auto acc = EvaultApp::db.findAccount(*pAccNum);
      if (!acc)
        return TRUE; // Should not happen

      // Background Hover
      FillRect(pds->hDC, &pds->rcItem, hBrushBg);
      if (pds->itemState & ODS_SELECTED) {
        // Darker feedback
        HBRUSH hHoverBrush = CreateSolidBrush(RGB(50, 50, 50));
        FillRect(pds->hDC, &pds->rcItem, hHoverBrush);
        DeleteObject(hHoverBrush);
      }

      // Circular Avatar
      HBRUSH hCircleBrush = CreateSolidBrush(RGB(200, 30, 90)); // Vibrant pink
      HGDIOBJ oldBrush = SelectObject(pds->hDC, hCircleBrush);
      SelectObject(pds->hDC, GetStockObject(NULL_PEN));
      Ellipse(pds->hDC, pds->rcItem.left + 30, pds->rcItem.top + 20,
              pds->rcItem.left + 110, pds->rcItem.top + 100);

      // Initials
      SetTextColor(pds->hDC, RGB(255, 255, 255));
      SetBkMode(pds->hDC, TRANSPARENT);
      wstring initial = L"";
      if (!acc->getHolderName().empty()) {
        initial = toWString(acc->getHolderName()).substr(0, 1);
      }
      RECT rcInit = {pds->rcItem.left + 30, pds->rcItem.top + 20,
                     pds->rcItem.left + 110, pds->rcItem.top + 100};
      DrawText(pds->hDC, initial.c_str(), -1, &rcInit,
               DT_CENTER | DT_VCENTER | DT_SINGLELINE);

      // Name
      SetTextColor(pds->hDC, colWhite);
      RECT rcName = pds->rcItem;
      rcName.top += 110;
      DrawText(pds->hDC, toWString(acc->getHolderName()).c_str(), -1, &rcName,
               DT_CENTER | DT_SINGLELINE);

      // Account Number
      SetTextColor(pds->hDC, colTextDim);
      HGDIOBJ oldFont = SelectObject(pds->hDC, hFontSmall);
      RECT rcAcc = rcName;
      rcAcc.top += 25;
      DrawText(pds->hDC, (L"#" + toWString(acc->getAccountNumber())).c_str(),
               -1, &rcAcc, DT_CENTER | DT_SINGLELINE);
      SelectObject(pds->hDC, oldFont);

      SelectObject(pds->hDC, oldBrush);
      DeleteObject(hCircleBrush);
      return TRUE;
    }
    // Avatar for Pin Entry
    if (pds->CtlType == ODT_STATIC && pds->CtlID == 0 &&
        currentView == VIEW_PIN_ENTRY) {
      auto acc = EvaultApp::db.findAccount(selectedProfileAccNum);
      if (!acc)
        return TRUE;

      HBRUSH hCircleBrush = CreateSolidBrush(RGB(200, 30, 90)); // Vibrant pink
      HGDIOBJ oldBrush = SelectObject(pds->hDC, hCircleBrush);
      SelectObject(pds->hDC, GetStockObject(NULL_PEN));
      Ellipse(pds->hDC, pds->rcItem.left, pds->rcItem.top, pds->rcItem.right,
              pds->rcItem.bottom);

      SetTextColor(pds->hDC, RGB(255, 255, 255));
      SetBkMode(pds->hDC, TRANSPARENT);
      wstring initial = L"";
      if (!acc->getHolderName().empty()) {
        initial = toWString(acc->getHolderName()).substr(0, 1);
      }
      DrawText(pds->hDC, initial.c_str(), -1, &pds->rcItem,
               DT_CENTER | DT_VCENTER | DT_SINGLELINE);

      SelectObject(pds->hDC, oldBrush);
      DeleteObject(hCircleBrush);
      return TRUE;
    }

    if (pds->CtlType == ODT_STATIC) {
      // Drawing the Balance Card
      FillRect(pds->hDC, &pds->rcItem, hBrushCard);
      HPEN hPen = CreatePen(PS_SOLID, 1, RGB(218, 220, 224));
      HGDIOBJ oldPen = SelectObject(pds->hDC, hPen);
      HGDIOBJ oldBrush = SelectObject(pds->hDC, GetStockObject(NULL_BRUSH));
      RoundRect(pds->hDC, pds->rcItem.left, pds->rcItem.top, pds->rcItem.right,
                pds->rcItem.bottom, 15, 15);

      if (pds->CtlID == ID_CARD_BALANCE && EvaultApp::currentUser) {
        SetTextColor(pds->hDC, colWhite);
        SetBkMode(pds->hDC, TRANSPARENT);
        SelectObject(pds->hDC, hFontSmall);
        RECT rcLabel = pds->rcItem;
        rcLabel.left += 20;
        rcLabel.top += 10;
        DrawText(pds->hDC, L"Total Balance", -1, &rcLabel, DT_SINGLELINE);

        SelectObject(pds->hDC, hFontTitle);
        wstringstream ss;
        ss << L"$" << fixed << setprecision(2)
           << EvaultApp::currentUser->getBalance();
        RECT rcBal = pds->rcItem;
        rcBal.left += 20;
        rcBal.top += 35;
        DrawText(pds->hDC, ss.str().c_str(), -1, &rcBal, DT_SINGLELINE);
      }

      SelectObject(pds->hDC, oldPen);
      SelectObject(pds->hDC, oldBrush);
      DeleteObject(hPen);
      return TRUE;
    }
    if (pds->CtlID == ID_LIST_HISTORY && pds->CtlType == ODT_LISTBOX) {
      if (pds->itemID == -1)
        return TRUE;

      COLORREF txtCol = colText;
      HBRUSH bgFill = hBrushCard;

      if (pds->itemState & ODS_SELECTED) {
        bgFill = CreateSolidBrush(RGB(232, 240, 254));
        txtCol = colBlue;
      }

      FillRect(pds->hDC, &pds->rcItem, bgFill);
      if (pds->itemState & ODS_SELECTED)
        DeleteObject(bgFill);

      wchar_t text[256];
      SendMessage(pds->hwndItem, LB_GETTEXT, pds->itemID, (LPARAM)text);

      SetTextColor(pds->hDC, txtCol);
      SetBkMode(pds->hDC, TRANSPARENT);

      RECT textRect = pds->rcItem;
      textRect.left += 15;
      DrawText(pds->hDC, text, -1, &textRect, DT_SINGLELINE | DT_VCENTER);
      return TRUE;
    }
  } break;

  case WM_COMMAND: {
    int id = LOWORD(wParam);

    // Navigation
    if (id == ID_BTN_ENTER)
      ShowProfileSelection(hwnd);
    if (id >= ID_BTN_PROFILE_CARD && id < ID_BTN_PROFILE_CARD + 100) {
      string *pAccNum =
          (string *)GetWindowLongPtr(GetDlgItem(hwnd, id), GWLP_USERDATA);
      if (pAccNum) {
        selectedProfileAccNum = *pAccNum;
        ShowPinEntry(hwnd);
      }
    }
    if (id == ID_BTN_REGISTER)
      ShowRegister(hwnd);
    if (id == ID_BTN_BACK) {
      if (currentView == VIEW_PROFILES || currentView == VIEW_REGISTER) {
        ShowHome(hwnd);
      } else if (currentView == VIEW_PIN_ENTRY) {
        ShowProfileSelection(hwnd);
      } else {
        ShowHome(hwnd); // Default fallback
      }
    }
    if (id == ID_BTN_LOGOUT) {
      EvaultApp::currentUser = nullptr;
      ShowProfileSelection(hwnd);
      MessageBox(hwnd, L"Logged out successfully.", L"Evault",
                 MB_OK | MB_ICONINFORMATION);
    }

    // Login Logic
    if (id == ID_BTN_SUBMIT_LOGIN) {
      wchar_t p[50];
      GetWindowText(GetDlgItem(hwnd, ID_EDIT_PIN), p, 50);

      if (EvaultApp::Login(selectedProfileAccNum, toString(p))) {
        ShowDashboard(hwnd);
      } else {
        MessageBox(hwnd, L"Invalid PIN!", L"Login Failed",
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
        ShowProfileSelection(hwnd);
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

        // Capture current user account number before reload invalidates the
        // pointer
        string currentUserAccNum = currentUser->getAccountNumber();

        db.reloadAccounts();

        // Restore currentUser pointer
        currentUser = db.findAccount(currentUserAccNum);
        if (!currentUser) {
          MessageBox(hwnd, L"Session Error. Please login again.", L"Error",
                     MB_OK | MB_ICONERROR);
          ShowHome(hwnd);
          break;
        }

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
          // Atomicity: Begin Transaction
          if (!db.beginTransaction()) {
            MessageBox(hwnd, L"Database Error: Could not start transaction.",
                       L"Error", MB_OK | MB_ICONERROR);
            // Revert local memory change since DB failed immediately
            currentUser->deposit(amount);
            break;
          }

          bool success = true;

          target->deposit(amount);

          if (!db.updateAccount(*currentUser))
            success = false;
          if (success && !db.updateAccount(*target))
            success = false;

          // Save transactions
          if (success && !db.saveTransaction(Transaction(
                             db.getNextTId(), currentUser->getAccountNumber(),
                             TransactionType::TRANSFER_OUT, amount, targetAcc)))
            success = false;

          if (success &&
              !db.saveTransaction(Transaction(
                  db.getNextTId(), targetAcc, TransactionType::TRANSFER_IN,
                  amount, currentUser->getAccountNumber())))
            success = false;

          if (success) {
            db.commitTransaction();
            MessageBox(hwnd, L"Transfer Successful!", L"Success",
                       MB_OK | MB_ICONINFORMATION);
            ShowDashboard(hwnd);
          } else {
            db.rollbackTransaction();
            // Revert local memory state
            currentUser->deposit(amount);
            target->withdraw(amount); // Revert target (though if pointer is
                                      // fresh it might be fine, but safe to do)

            MessageBox(hwnd, L"Transfer Failed! Transaction Rolled Back.",
                       L"Error", MB_OK | MB_ICONERROR);
          }
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
