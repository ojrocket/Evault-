#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#define _WIN32_WINNT 0x0600
#define NOMINMAX

#include <windows.h>
#include <objbase.h>
#include <objidl.h>
#include <gdiplus.h>
#include <commctrl.h>
#include <algorithm>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>

extern "C" {
#include "sqlite3.h"
}

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")

using namespace Gdiplus;
using namespace std;

// ==========================================
// Theme Configuration
// ==========================================
namespace Theme {
const COLORREF Bg = RGB(13, 14, 16);
const COLORREF Sidebar = RGB(18, 19, 21);
const COLORREF Card = RGB(24, 25, 28);
const COLORREF Accent = RGB(138, 180, 248);
const COLORREF Border = RGB(45, 46, 48);
const COLORREF Text = RGB(232, 234, 237);
const COLORREF TextDim = RGB(154, 160, 166);
}

// ==========================================
// Logic Classes
// ==========================================
namespace Core {
struct Account {
    wstring accNum;
    wstring name;
    wstring pin;
    double balance;
};

class VaultDB {
private:
    sqlite3* db;
public:
    VaultDB() : db(nullptr) {}
    ~VaultDB() { if(db) sqlite3_close(db); }

    bool Init() {
        if (sqlite3_open("evault.db", &db) != SQLITE_OK) return false;
        sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS accounts (acc_num TEXT PRIMARY KEY, name TEXT, pin TEXT, balance REAL);", 0, 0, 0);
        
        sqlite3_stmt* check;
        if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM accounts;", -1, &check, 0) == SQLITE_OK) {
            if (sqlite3_step(check) == SQLITE_ROW && sqlite3_column_int(check, 0) == 0) {
                sqlite3_finalize(check);
                CreateAccount(L"77367438", L"jashwanth oggu", L"1985", 50000);
                CreateAccount(L"48528372", L"chinni jaswanth", L"4066", 25000);
                CreateAccount(L"57422441", L"muni charan teja", L"1028", 15000);
                CreateAccount(L"84536252", L"jane SQLite", L"4321", 100000);
            } else {
                sqlite3_finalize(check);
            }
        }
        return true;
    }

    bool CreateAccount(wstring num, wstring name, wstring pin, double bal) {
        sqlite3_stmt* s;
        if (sqlite3_prepare_v2(db, "INSERT INTO accounts VALUES(?,?,?,?);", -1, &s, 0) != SQLITE_OK) return false;
        string sNum(num.begin(), num.end());
        string sName(name.begin(), name.end());
        string sPin(pin.begin(), pin.end());
        sqlite3_bind_text(s, 1, sNum.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(s, 2, sName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(s, 3, sPin.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(s, 4, bal);
        bool ok = (sqlite3_step(s) == SQLITE_DONE);
        sqlite3_finalize(s);
        return ok;
    }

    vector<Account> LoadAccounts() {
        vector<Account> list;
        sqlite3_stmt* s;
        if (sqlite3_prepare_v2(db, "SELECT * FROM accounts;", -1, &s, 0) == SQLITE_OK) {
            while(sqlite3_step(s) == SQLITE_ROW) {
                string n = (const char*)sqlite3_column_text(s, 0);
                string m = (const char*)sqlite3_column_text(s, 1);
                string p = (const char*)sqlite3_column_text(s, 2);
                double b = sqlite3_column_double(s, 3);
                list.push_back({wstring(n.begin(), n.end()), wstring(m.begin(), m.end()), wstring(p.begin(), p.end()), b});
            }
            sqlite3_finalize(s);
        }
        return list;
    }

    bool UpdateBalance(wstring num, double newBal) {
        sqlite3_stmt* s;
        if (sqlite3_prepare_v2(db, "UPDATE accounts SET balance=? WHERE acc_num=?;", -1, &s, 0) != SQLITE_OK) return false;
        string sNum(num.begin(), num.end());
        sqlite3_bind_double(s, 1, newBal);
        sqlite3_bind_text(s, 2, sNum.c_str(), -1, SQLITE_TRANSIENT);
        bool ok = (sqlite3_step(s) == SQLITE_DONE);
        sqlite3_finalize(s);
        return ok;
    }
};
}

// ==========================================
// Global State
// ==========================================
enum ViewID { VIEW_PRELOAD, VIEW_ACCOUNT_SELECT, VIEW_REGISTER, VIEW_LOGIN, VIEW_PORTAL_CHOICE, VIEW_DASHBOARD, VIEW_STOCKS };
ViewID currentView = VIEW_PRELOAD;
HWND hMain, hSidebar, hContent;
ULONG_PTR gdiplusToken;
vector<HWND> viewControls;
int preloadProgress = 0;
wstring activeName, activeNum, activePin;
double activeBalance = 0;
Core::VaultDB dbInstance;

#define ID_NAV_BANKING 1001
#define ID_NAV_STOCKS 1002
#define ID_EDIT_AMOUNT 3001
#define ID_BTN_DEPOSIT 2001
#define ID_BTN_WITHDRAW 2002
#define ID_BTN_CHECK_BAL 2004
#define ID_BTN_PORTAL_BANKING 4001
#define ID_BTN_PORTAL_STOCKS 4002
#define ID_EDIT_PIN 5001
#define ID_BTN_LOGIN 5002
#define ID_BTN_LOGOUT 5003
#define ID_BTN_CREATE_ACC 6001
#define ID_EDIT_REG_NAME 6002
#define ID_EDIT_REG_PIN 6003
#define ID_EDIT_REG_DEPOSIT 6004
#define ID_BTN_SUBMIT_REG 6005
#define ID_BTN_REG_BACK 6006
#define ID_TIMER_PRELOAD 7001

void SwitchView(ViewID newView, bool force = false);

// ==========================================
// UI Helpers
// ==========================================
void DrawRoundedRect(Graphics &g, RectF rect, REAL r, Color color, REAL borderSize = 0, Color borderColor = Color(0,0,0,0)) {
  GraphicsPath path;
  path.AddArc(rect.X, rect.Y, r, r, 180, 90);
  path.AddArc(rect.X + rect.Width - r, rect.Y, r, r, 270, 90);
  path.AddArc(rect.X + rect.Width - r, rect.Y + rect.Height - r, r, r, 0, 90);
  path.AddArc(rect.X, rect.Y + rect.Height - r, r, r, 90, 90);
  path.CloseFigure();
  SolidBrush brush(color); g.FillPath(&brush, &path);
  if (borderSize > 0) { Pen pen(borderColor, borderSize); g.DrawPath(&pen, &path); }
}

void DrawGlassPanel(Graphics &g, RectF rect, REAL alpha = 40) {
  Color base(alpha, 24, 25, 28);
  DrawRoundedRect(g, rect, 15, base, 1.2f, Color(60, 255, 255, 255));
  LinearGradientBrush shine(PointF(rect.X, rect.Y), PointF(rect.X + rect.Width, rect.Y + rect.Height), Color(30, 255, 255, 255), Color(0, 255, 255, 255));
  g.FillRectangle(&shine, rect.X, rect.Y, rect.Width, rect.Height);
}

void ClearView() {
  for (HWND ctrl : viewControls) ShowWindow(ctrl, SW_HIDE);
  InvalidateRect(hContent, NULL, TRUE);
  UpdateWindow(hContent);
  for (HWND ctrl : viewControls) DestroyWindow(ctrl);
  viewControls.clear();
}

void SwitchView(ViewID newView, bool force) {
  if (currentView == newView && !force) return;
  currentView = newView; ClearView();
  HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hMain, GWLP_HINSTANCE);
  RECT rc; GetClientRect(hMain, &rc);
  if (newView <= VIEW_PORTAL_CHOICE) { ShowWindow(hSidebar, SW_HIDE); SetWindowPos(hContent, NULL, 0, 0, rc.right, rc.bottom, SWP_NOZORDER); }
  else { ShowWindow(hSidebar, SW_SHOW); SetWindowPos(hContent, NULL, 240, 0, rc.right - 240, rc.bottom, SWP_NOZORDER); }

  if (newView == VIEW_ACCOUNT_SELECT) {
      viewControls.push_back(CreateWindowW(L"BUTTON", L"+ CREATE NEW ACCOUNT", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, rc.right/2 - 125, rc.bottom - 100, 250, 45, hContent, (HMENU)ID_BTN_CREATE_ACC, hInst, NULL));
  } else if (newView == VIEW_REGISTER) {
      viewControls.push_back(CreateWindowExW(0, L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, rc.right/2 - 150, 280, 300, 35, hContent, (HMENU)ID_EDIT_REG_NAME, hInst, NULL));
      viewControls.push_back(CreateWindowExW(0, L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD | ES_NUMBER, rc.right/2 - 150, 340, 300, 35, hContent, (HMENU)ID_EDIT_REG_PIN, hInst, NULL));
      viewControls.push_back(CreateWindowExW(0, L"EDIT", L"1000", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER, rc.right/2 - 150, 400, 300, 35, hContent, (HMENU)ID_EDIT_REG_DEPOSIT, hInst, NULL));
      viewControls.push_back(CreateWindowW(L"BUTTON", L"INITIALIZE ACCOUNT", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, rc.right/2 - 150, 460, 300, 45, hContent, (HMENU)ID_BTN_SUBMIT_REG, hInst, NULL));
      viewControls.push_back(CreateWindowW(L"BUTTON", L"BACK", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 50, 50, 100, 40, hContent, (HMENU)ID_BTN_REG_BACK, hInst, NULL));
  } else if (newView == VIEW_LOGIN) {
      HWND hPin = CreateWindowExW(0, L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD | ES_NUMBER | ES_CENTER, rc.right/2 - 100, rc.bottom/2, 200, 40, hContent, (HMENU)ID_EDIT_PIN, hInst, NULL);
      viewControls.push_back(hPin);
      viewControls.push_back(CreateWindowW(L"BUTTON", L"ENTER VAULT", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, rc.right/2 - 100, rc.bottom/2 + 60, 200, 45, hContent, (HMENU)ID_BTN_LOGIN, hInst, NULL));
      viewControls.push_back(CreateWindowW(L"BUTTON", L"BACK", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 50, 50, 100, 40, hContent, (HMENU)ID_BTN_REG_BACK, hInst, NULL));
      SetFocus(hPin);
  } else if (newView == VIEW_PORTAL_CHOICE) {
      viewControls.push_back(CreateWindowW(L"BUTTON", L"BANKING PORTAL", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, rc.right/2 - 350, 300, 300, 200, hContent, (HMENU)ID_BTN_PORTAL_BANKING, hInst, NULL));
      viewControls.push_back(CreateWindowW(L"BUTTON", L"STOCK MARKET", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, rc.right/2 + 50, 300, 300, 200, hContent, (HMENU)ID_BTN_PORTAL_STOCKS, hInst, NULL));
      viewControls.push_back(CreateWindowW(L"BUTTON", L"BACK TO PROFILES", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 50, 50, 150, 40, hContent, (HMENU)ID_BTN_LOGOUT, hInst, NULL));
  } else if (newView == VIEW_DASHBOARD) {
    viewControls.push_back(CreateWindowExW(0, L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER, 70, 480, 200, 40, hContent, (HMENU)ID_EDIT_AMOUNT, hInst, NULL));
    viewControls.push_back(CreateWindowW(L"BUTTON", L"DEPOSIT", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 70, 535, 120, 45, hContent, (HMENU)ID_BTN_DEPOSIT, hInst, NULL));
    viewControls.push_back(CreateWindowW(L"BUTTON", L"WITHDRAW", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 200, 535, 120, 45, hContent, (HMENU)ID_BTN_WITHDRAW, hInst, NULL));
    viewControls.push_back(CreateWindowW(L"BUTTON", L"CHECK BALANCE", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 40, 310, 320, 40, hContent, (HMENU)ID_BTN_CHECK_BAL, hInst, NULL));
    viewControls.push_back(CreateWindowW(L"BUTTON", L"LOGOUT", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, rc.right - 240 - 120, 40, 100, 40, hContent, (HMENU)ID_BTN_LOGOUT, hInst, NULL));
  }
}

LRESULT CALLBACK ContentProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  if (msg == WM_COMMAND) { SendMessage(GetParent(hwnd), WM_COMMAND, wp, lp); return 0; }
  if (msg == WM_PAINT) {
    PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps); RECT rc; GetClientRect(hwnd, &rc);
    HDC mdc = CreateCompatibleDC(hdc); HBITMAP mbm = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    SelectObject(mdc, mbm); Graphics g(mdc); g.SetSmoothingMode(SmoothingModeAntiAlias); g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    SolidBrush bg(Color(255, 13, 14, 16)); g.FillRectangle(&bg, 0, 0, rc.right, rc.bottom);
    FontFamily ff(L"Segoe UI"); Font tFont(&ff, 24, FontStyleBold, UnitPixel); Font sFont(&ff, 14, FontStyleRegular, UnitPixel);
    SolidBrush wr(Color(255, 255, 255, 255)); SolidBrush dr(Color(255, 154, 160, 166));

    if (currentView == VIEW_PRELOAD) {
        g.DrawString(L"V", -1, &tFont, PointF(rc.right/2.0f-10, rc.bottom/2.0f-50), &wr);
        DrawRoundedRect(g, RectF(rc.right/2.0f-150, rc.bottom/2.0f+50, 300, 6), 3, Color(255, 45, 46, 48));
        DrawRoundedRect(g, RectF(rc.right/2.0f-150, rc.bottom/2.0f+50, (REAL)(preloadProgress*3), 6), 3, Color(255, 138, 180, 248));
    } else if (currentView == VIEW_ACCOUNT_SELECT) {
        g.DrawString(L"Who's using Evault?", -1, &tFont, PointF(rc.right/2.0f-100, 60), &wr);
        vector<Core::Account> accounts = dbInstance.LoadAccounts();
        for(size_t i=0; i<accounts.size() && i<8; i++) {
            int row = (int)(i / 4); int col = (int)(i % 4);
            int startX = (int)(rc.right/2 - 560);
            if(accounts.size() < 4) startX = (int)(rc.right/2 - (accounts.size()*280)/2);
            RectF card((REAL)(startX + col*280), (REAL)(150 + row*300), 220.0f, 260.0f); DrawGlassPanel(g, card, 20);
            Font initFont(&ff, 32, FontStyleBold, UnitPixel); 
            g.DrawString(accounts[i].name.substr(0,1).c_str(), 1, &initFont, PointF(card.X+90, card.Y+65), &wr);
            RectF nr(card.X+10, card.Y+160, 200, 30); StringFormat sf; sf.SetAlignment(StringAlignmentCenter); 
            g.DrawString(accounts[i].name.c_str(), -1, &sFont, nr, &sf, &wr);
            RectF ar(card.X+10, card.Y+190, 200, 30); g.DrawString((L"#" + accounts[i].accNum).c_str(), -1, &sFont, ar, &sf, &dr);
        }
    } else if (currentView == VIEW_REGISTER) {
        g.DrawString(L"Join the Core", -1, &tFont, PointF(rc.right/2.0f-60, 150), &wr);
        DrawGlassPanel(g, RectF(rc.right/2.0f-200, 220, 400, 320), 30);
        g.DrawString(L"Full Name", -1, &sFont, PointF(rc.right/2.0f-150, 255), &dr);
        g.DrawString(L"Create PIN (4 Digits)", -1, &sFont, PointF(rc.right/2.0f-150, 315), &dr);
        g.DrawString(L"Initial Deposit (Rs.)", -1, &sFont, PointF(rc.right/2.0f-150, 375), &dr);
    } else if (currentView == VIEW_LOGIN) {
        g.DrawString(L"Security Verification", -1, &tFont, PointF(rc.right/2.0f-100, rc.bottom/2.0f-150), &wr);
        DrawGlassPanel(g, RectF(rc.right/2.0f-150, rc.bottom/2.0f-130, 300, 280), 15);
        g.DrawString((L"Logging into " + activeName).c_str(), -1, &sFont, PointF(rc.right/2.0f-100, rc.bottom/2.0f-100), &dr);
        g.DrawString((L"Account: #" + activeNum).c_str(), -1, &sFont, PointF(rc.right/2.0f-100, rc.bottom/2.0f-75), &dr);
    } else if (currentView == VIEW_DASHBOARD) {
        g.DrawString(L"Banking Dashboard", -1, &tFont, PointF(40, 40), &wr);
        DrawGlassPanel(g, RectF(40, 100, 320, 200), 80);
        g.DrawString(L"Protected Balance", -1, &sFont, PointF(70, 130), &dr);
        g.DrawString(L"Rs. ********", -1, &tFont, PointF(70, 160), &wr);
        g.DrawString(L"Vault Operations", -1, &tFont, PointF(40, 380), &wr);
        DrawGlassPanel(g, RectF(40, 430, 560, 300), 30);
        g.DrawString(L"Amount (Rs.)", -1, &sFont, PointF(70, 455), &dr);
        DrawGlassPanel(g, RectF(640, 40, (REAL)(rc.right - 680), (REAL)(rc.bottom - 80)), 10);
        g.DrawString(L"Recent Activity", -1, &sFont, PointF(660, 60), &wr);
    }
    BitBlt(hdc, 0, 0, rc.right, rc.bottom, mdc, 0, 0, SRCCOPY); DeleteObject(mbm); DeleteDC(mdc); EndPaint(hwnd, &ps); return 0;
  }
  if (msg == WM_LBUTTONDOWN) {
      if (currentView == VIEW_ACCOUNT_SELECT) {
          int x = LOWORD(lp), y = HIWORD(lp); RECT rc; GetClientRect(hwnd, &rc);
          vector<Core::Account> accounts = dbInstance.LoadAccounts();
          for(size_t i=0; i<accounts.size() && i<8; i++) {
              int row = (int)(i / 4); int col = (int)(i % 4);
              int startX = (int)(rc.right/2 - 560);
              if(accounts.size() < 4) startX = (int)(rc.right/2 - (accounts.size()*280)/2);
              if (x > startX+col*280 && x < startX+col*280+220 && y > 150+row*300 && y < 150+row*300+260) {
                  activeName = accounts[i].name; activeNum = accounts[i].accNum; activePin = accounts[i].pin; activeBalance = accounts[i].balance;
                  SwitchView(VIEW_LOGIN, false);
              }
          }
      }
  }
  return DefWindowProc(hwnd, msg, wp, lp);
}

LRESULT CALLBACK SidebarProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  if (msg == WM_COMMAND) { SendMessage(GetParent(hwnd), WM_COMMAND, wp, lp); return 0; }
  if (msg == WM_PAINT) {
    PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps); Graphics g(hdc);
    SolidBrush bg(Color(255, 18, 19, 21)); RECT rc; GetClientRect(hwnd, &rc); g.FillRectangle(&bg, 0, 0, rc.right, rc.bottom);
    FontFamily ff(L"Segoe UI"); Font f(&ff, 18, FontStyleBold, UnitPixel); SolidBrush b(Color(255, 138, 180, 248));
    g.DrawString(L"EVAULT PRO", -1, &f, PointF(25, 30), &b); EndPaint(hwnd, &ps); return 0;
  }
  return DefWindowProc(hwnd, msg, wp, lp);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
  case WM_CREATE: hMain = hwnd;
    hSidebar = CreateWindowExW(0, L"SidebarClass", NULL, WS_VISIBLE|WS_CHILD|WS_CLIPCHILDREN, 0, 0, 240, 800, hwnd, NULL, NULL, NULL);
    hContent = CreateWindowExW(0, L"ContentClass", NULL, WS_VISIBLE|WS_CHILD|WS_CLIPCHILDREN, 240, 0, 1040, 800, hwnd, NULL, NULL, NULL);
    for(int i=0; i<2; i++) CreateWindowW(L"BUTTON", i==0?L"Banking":L"Stocks", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 10, 100+i*55, 220, 45, hSidebar, (HMENU)(1001+i), NULL, NULL);
    dbInstance.Init(); SetTimer(hwnd, ID_TIMER_PRELOAD, 30, NULL); SwitchView(VIEW_PRELOAD, true); break;
  case WM_TIMER: preloadProgress += 2; if (preloadProgress >= 100) { KillTimer(hwnd, ID_TIMER_PRELOAD); SwitchView(VIEW_ACCOUNT_SELECT, true); } InvalidateRect(hContent, NULL, TRUE); break;
  case WM_DRAWITEM: {
    LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lp; Graphics g(pdis->hDC); RectF r(0,0,(REAL)pdis->rcItem.right,(REAL)pdis->rcItem.bottom);
    bool sel = (pdis->CtlID == 1001 && currentView == VIEW_DASHBOARD);
    DrawRoundedRect(g, r, 10, sel?Color(50, 138, 180, 248):Color(0,0,0,0));
    WCHAR txt[64]; GetWindowTextW(pdis->hwndItem, txt, 64); FontFamily ff(L"Segoe UI"); Font f(&ff, 14, FontStyleRegular, UnitPixel);
    SolidBrush b(Color(255,255,255,255)); StringFormat sf; sf.SetAlignment(StringAlignmentNear); sf.SetLineAlignment(StringAlignmentCenter);
    r.X+=20.0f; g.DrawString(txt, -1, &f, r, &sf, &b); return TRUE;
  }
  case WM_COMMAND: {
    int id = LOWORD(wp);
    if (id == 1001 || id == ID_BTN_PORTAL_BANKING) SwitchView(VIEW_DASHBOARD, false);
    else if (id == 1002 || id == ID_BTN_PORTAL_STOCKS) SwitchView(VIEW_STOCKS, false);
    else if (id == ID_BTN_CREATE_ACC) SwitchView(VIEW_REGISTER, false);
    else if (id == ID_BTN_REG_BACK) SwitchView(VIEW_ACCOUNT_SELECT, false);
    else if (id == ID_BTN_LOGOUT) SwitchView(VIEW_ACCOUNT_SELECT, false);
    else if (id == ID_BTN_CHECK_BAL) {
        wstringstream ss; ss << L"Account #" << activeNum << L"\nHolder: " << activeName << L"\nBalance: Rs. " << fixed << setprecision(2) << activeBalance;
        MessageBoxW(hwnd, ss.str().c_str(), L"Vault Balance", MB_ICONINFORMATION);
    }
    else if (id == ID_BTN_LOGIN) {
        WCHAR p[16]; GetWindowTextW(GetDlgItem(hContent, ID_EDIT_PIN), p, 16);
        if (activePin == wstring(p)) SwitchView(VIEW_PORTAL_CHOICE, false);
        else MessageBoxW(hwnd, L"Access Denied: Invalid PIN.", L"Security", MB_ICONERROR);
    }
    else if (id == ID_BTN_SUBMIT_REG) {
        WCHAR n[64], p[16], d[16];
        HWND hName = GetDlgItem(hContent, ID_EDIT_REG_NAME);
        HWND hPinField = GetDlgItem(hContent, ID_EDIT_REG_PIN);
        HWND hDep = GetDlgItem(hContent, ID_EDIT_REG_DEPOSIT);

        if (!hName || !hPinField || !hDep) {
             MessageBoxW(hwnd, L"Internal Error: Input fields not found.", L"Error", MB_ICONERROR);
             break;
        }

        GetWindowTextW(hName, n, 64);
        GetWindowTextW(hPinField, p, 16);
        GetWindowTextW(hDep, d, 16);
        
        if(wcslen(n) < 3 || wcslen(p) != 4) { 
            MessageBoxW(hwnd, L"Invalid Name (min 3 chars) or PIN (must be 4 digits).", L"Validation Error", MB_ICONERROR); 
            break; 
        }
        
        srand((unsigned int)time(NULL)); 
        wstringstream acc; for(int i=0; i<8; i++) acc << rand()%10;
        wstring newAccNum = acc.str();
        wstring newPin = p;

        if(dbInstance.CreateAccount(newAccNum, n, newPin, _wtof(d))) {
            wstringstream successMsg;
            successMsg << L"Account Initialized Successfully!\n\n"
                       << L"Holder: " << n << L"\n"
                       << L"Account #: " << newAccNum << L"\n"
                       << L"Security PIN: " << newPin << L"\n\n"
                       << L"Note these credentials securely.";
            
            MessageBoxW(hwnd, successMsg.str().c_str(), L"Success", MB_ICONINFORMATION);
            SwitchView(VIEW_ACCOUNT_SELECT, false);
        } else {
            MessageBoxW(hwnd, L"Failed to save account to database.", L"Database Error", MB_ICONERROR);
        }
    }
    else if (id == ID_BTN_DEPOSIT || id == ID_BTN_WITHDRAW) {
        WCHAR amtStr[32]; GetWindowTextW(GetDlgItem(hContent, ID_EDIT_AMOUNT), amtStr, 32);
        double amount = _wtof(amtStr);
        if (amount <= 0) { MessageBoxW(hwnd, L"Enter valid amount.", L"Invalid", MB_ICONWARNING); break; }
        if (id == ID_BTN_DEPOSIT) { activeBalance += amount; }
        else {
            if (amount > activeBalance) { MessageBoxW(hwnd, L"Insufficient funds.", L"Error", MB_ICONERROR); break; }
            activeBalance -= amount;
        }
        dbInstance.UpdateBalance(activeNum, activeBalance);
        MessageBoxW(hwnd, L"Transaction success!", L"Success", MB_ICONINFORMATION);
        SetWindowTextW(GetDlgItem(hContent, ID_EDIT_AMOUNT), L"");
    }
    break;
  }
  case WM_SIZE: { int w=LOWORD(lp), h=HIWORD(lp); if (currentView <= VIEW_PORTAL_CHOICE) { MoveWindow(hSidebar,0,0,0,h,TRUE); MoveWindow(hContent,0,0,w,h,TRUE); } else { MoveWindow(hSidebar,0,0,240,h,TRUE); MoveWindow(hContent,240,0,w-240,h,TRUE); } } break;
  case WM_DESTROY: PostQuitMessage(0); break;
  default: return DefWindowProc(hwnd, msg, wp, lp);
  } return 0;
}

int WINAPI WinMain(HINSTANCE h, HINSTANCE hp, LPSTR c, int s) {
  GdiplusStartupInput gsi; GdiplusStartup(&gdiplusToken, &gsi, NULL);
  WNDCLASSW wc = {0}; wc.lpfnWndProc = WndProc; wc.hInstance = h; wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.lpszClassName = L"EvaultProClass"; wc.hbrBackground = CreateSolidBrush(Theme::Bg); RegisterClassW(&wc);
  wc.lpfnWndProc = SidebarProc; wc.lpszClassName = L"SidebarClass"; RegisterClassW(&wc);
  wc.lpfnWndProc = ContentProc; wc.lpszClassName = L"ContentClass"; RegisterClassW(&wc);
  hMain = CreateWindowExW(0, L"EvaultProClass", L"Evault Pro", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN, 100, 100, 1280, 800, NULL, NULL, h, NULL);
  MSG m; while (GetMessage(&m, NULL, 0, 0)) { TranslateMessage(&m); DispatchMessage(&m); }
  GdiplusShutdown(gdiplusToken); return (int)m.wParam;
}
