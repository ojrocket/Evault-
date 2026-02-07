#include <ctime>
#include <iomanip>
#include <map>
#include <sqlite3.h>
#include <sstream>
#include <string>
#include <vector>


using namespace std;

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
  string targetName;

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
    reloadAccounts();
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
