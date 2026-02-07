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
#include <cwctype>

extern "C" {
#include "sqlite3.h"
}

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")

using namespace Gdiplus;
using namespace std;

// ==========================================
// PREMIUM DESIGN SYSTEM
// ==========================================
namespace Theme {
    const Color Bg(255, 5, 5, 6);
    const Color Sidebar(255, 10, 10, 15);
    const Color CardBg(180, 20, 22, 28);
    const Color Accent(255, 0, 240, 255);
    const Color Border(80, 255, 255, 255);
    const Color TextTitle(255, 255, 255, 255);
    const Color TextDim(255, 150, 150, 160);
    const Color GridLines(40, 255, 255, 255);
}

// ==========================================
// CORE DATA
// ==========================================
namespace Core {
struct Account { wstring accNum, name, pin; double balance; };
struct Stock { wstring symbol, name; double price; vector<float> history; };

class VaultDB {
private:
    sqlite3* db;
public:
    VaultDB() : db(nullptr) {}
    ~VaultDB() { if(db) sqlite3_close(db); }

    bool Init() {
        if (sqlite3_open("evault.db", &db) != SQLITE_OK) return false;
        sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS accounts (acc_num TEXT PRIMARY KEY, name TEXT, pin TEXT, balance REAL);", 0, 0, 0);
        sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS portfolio (acc_num TEXT, symbol TEXT, quantity INTEGER, avg_price REAL, PRIMARY KEY(acc_num, symbol));", 0, 0, 0);
        
        sqlite3_stmt* check;
        if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM accounts;", -1, &check, 0) == SQLITE_OK) {
            if (sqlite3_step(check) == SQLITE_ROW && sqlite3_column_int(check, 0) == 0) {
                sqlite3_finalize(check);
                CreateAccount(L"77367438", L"jashwanth oggu", L"1985", 100000);
                CreateAccount(L"48528372", L"chinni jaswanth", L"4066", 75000);
                CreateAccount(L"57422441", L"muni charan teja", L"1028", 50000);
            } else sqlite3_finalize(check);
        }
        return true;
    }

    bool CreateAccount(wstring num, wstring name, wstring pin, double bal) {
        sqlite3_stmt* s;
        sqlite3_prepare_v2(db, "INSERT INTO accounts VALUES(?,?,?,?);", -1, &s, 0);
        string sNum(num.begin(), num.end()), sName(name.begin(), name.end()), sPin(pin.begin(), pin.end());
        sqlite3_bind_text(s, 1, sNum.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(s, 2, sName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(s, 3, sPin.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(s, 4, bal);
        bool ok = (sqlite3_step(s) == SQLITE_DONE); sqlite3_finalize(s); return ok;
    }

    vector<Account> LoadAccounts() {
        vector<Account> list; sqlite3_stmt* s;
        if (sqlite3_prepare_v2(db, "SELECT * FROM accounts;", -1, &s, 0) == SQLITE_OK) {
            while(sqlite3_step(s) == SQLITE_ROW) {
                string n = (const char*)sqlite3_column_text(s, 0), m = (const char*)sqlite3_column_text(s, 1), p = (const char*)sqlite3_column_text(s, 2);
                list.push_back({wstring(n.begin(), n.end()), wstring(m.begin(), m.end()), wstring(p.begin(), p.end()), sqlite3_column_double(s, 3)});
            }
            sqlite3_finalize(s);
        }
        return list;
    }

    bool UpdateBalance(wstring num, double newBal) {
        sqlite3_stmt* s; sqlite3_prepare_v2(db, "UPDATE accounts SET balance=? WHERE acc_num=?;", -1, &s, 0);
        string sNum(num.begin(), num.end()); sqlite3_bind_double(s, 1, newBal);
        sqlite3_bind_text(s, 2, sNum.c_str(), -1, SQLITE_TRANSIENT);
        bool ok = (sqlite3_step(s) == SQLITE_DONE); sqlite3_finalize(s); return ok;
    }

    int Transfer(wstring from, wstring toName, double amount) {
        if (amount <= 0) return 5;
        string sFrom(from.begin(), from.end());
        string sToName(toName.begin(), toName.end());
        
        sqlite3_stmt* find; string targetID;
        sqlite3_prepare_v2(db, "SELECT acc_num FROM accounts WHERE name = ? COLLATE NOCASE;", -1, &find, 0);
        sqlite3_bind_text(find, 1, sToName.c_str(), -1, SQLITE_TRANSIENT);
        if(sqlite3_step(find) == SQLITE_ROW) targetID = (const char*)sqlite3_column_text(find, 0);
        sqlite3_finalize(find);
        
        if(targetID.empty()) return 4;
        if(targetID == sFrom) return 2;

        sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);
        sqlite3_stmt *s1, *s2;
        sqlite3_prepare_v2(db, "UPDATE accounts SET balance = balance - ? WHERE acc_num = ? AND balance >= ?;", -1, &s1, 0);
        sqlite3_bind_double(s1, 1, amount); sqlite3_bind_text(s1, 2, sFrom.c_str(), -1, SQLITE_TRANSIENT); sqlite3_bind_double(s1, 3, amount);
        bool dOk = (sqlite3_step(s1) == SQLITE_DONE && sqlite3_changes(db) > 0); sqlite3_finalize(s1);
        if (!dOk) { sqlite3_exec(db, "ROLLBACK;", 0, 0, 0); return 3; }
        
        sqlite3_prepare_v2(db, "UPDATE accounts SET balance = balance + ? WHERE acc_num = ?;", -1, &s2, 0);
        sqlite3_bind_double(s2, 1, amount); sqlite3_bind_text(s2, 2, targetID.c_str(), -1, SQLITE_TRANSIENT);
        bool cOk = (sqlite3_step(s2) == SQLITE_DONE && sqlite3_changes(db) > 0); sqlite3_finalize(s2);
        if (!cOk) { sqlite3_exec(db, "ROLLBACK;", 0, 0, 0); return 4; }
        
        sqlite3_exec(db, "COMMIT;", 0, 0, 0); return 0;
    }

    int GetOwnedStocks(wstring accNum, wstring symbol, double* avgPrice = nullptr) {
        sqlite3_stmt* s; string sNum(accNum.begin(), accNum.end()), sSym(symbol.begin(), symbol.end());
        sqlite3_prepare_v2(db, "SELECT quantity, avg_price FROM portfolio WHERE acc_num=? AND symbol=?;", -1, &s, 0);
        sqlite3_bind_text(s, 1, sNum.c_str(), -1, SQLITE_TRANSIENT); sqlite3_bind_text(s, 2, sSym.c_str(), -1, SQLITE_TRANSIENT);
        int qty = 0; if(sqlite3_step(s) == SQLITE_ROW) {
            qty = sqlite3_column_int(s, 0);
            if(avgPrice) *avgPrice = sqlite3_column_double(s, 1);
        }
        sqlite3_finalize(s); return qty;
    }

    bool UpdateStocks(wstring accNum, wstring symbol, int delta, double price) {
        double currentAvg = 0;
        int currentQty = GetOwnedStocks(accNum, symbol, &currentAvg);
        int nextQty = currentQty + delta;
        if (nextQty < 0) return false;
        double nextAvg = currentAvg;
        if (delta > 0) nextAvg = ((currentAvg * currentQty) + (price * delta)) / nextQty;

        sqlite3_stmt* s; string sNum(accNum.begin(), accNum.end()), sSym(symbol.begin(), symbol.end());
        if (currentQty == 0 && delta > 0) {
            sqlite3_prepare_v2(db, "INSERT INTO portfolio VALUES(?,?,?,?);", -1, &s, 0);
            sqlite3_bind_text(s, 1, sNum.c_str(), -1, SQLITE_TRANSIENT); sqlite3_bind_text(s, 2, sSym.c_str(), -1, SQLITE_TRANSIENT); 
            sqlite3_bind_int(s, 3, nextQty); sqlite3_bind_double(s, 4, nextAvg);
        } else {
            sqlite3_prepare_v2(db, "UPDATE portfolio SET quantity=?, avg_price=? WHERE acc_num=? AND symbol=?;", -1, &s, 0);
            sqlite3_bind_int(s, 1, nextQty); sqlite3_bind_double(s, 2, nextAvg);
            sqlite3_bind_text(s, 3, sNum.c_str(), -1, SQLITE_TRANSIENT); sqlite3_bind_text(s, 4, sSym.c_str(), -1, SQLITE_TRANSIENT);
        }
        bool ok = (sqlite3_step(s) == SQLITE_DONE); sqlite3_finalize(s); return ok;
    }
};
}

// ==========================================
// STATE
// ==========================================
enum ViewID { PRELOAD, ACCOUNTS, REGISTER, LOGIN, PORTALS, BANKING, STOCKS };
ViewID activeView = PRELOAD;
HWND hMain, hSide, hCont;
ULONG_PTR gdiplusToken;
vector<HWND> controls;
wstring uName, uID, uPIN;
double uBal = 0;
int preloadPct = 0, stockSelIdx = 0;
Core::VaultDB dbInstance;

vector<Core::Stock> marketStocks = {
    {L"NVDA", L"NVIDIA Corp", 880.50, {850, 860, 875, 870, 890, 885, 880}},
    {L"AAPL", L"Apple Inc", 172.10, {170, 171, 175, 173, 174, 172, 172}},
    {L"TSLA", L"Tesla Inc", 165.40, {180, 175, 170, 168, 160, 162, 165}},
    {L"BTC", L"Bitcoin", 65400.0, {62000, 63000, 66000, 64000, 68000, 67000, 65400}},
    {L"ETH", L"Ethereum", 3500.2, {3200, 3300, 3600, 3400, 3700, 3600, 3500}}
};

// ==========================================
// UI HELPERS
// ==========================================
void DrawPremiumRect(Graphics &g, RectF r, REAL rad, Color c, bool border = true) {
    GraphicsPath path; path.AddArc(r.X, r.Y, rad, rad, 180, 90); path.AddArc(r.X + r.Width - rad, r.Y, rad, rad, 270, 90); path.AddArc(r.X + r.Width - rad, r.Y + r.Height - rad, rad, rad, 0, 90); path.AddArc(r.X, r.Y + r.Height - rad, rad, rad, 90, 90); path.CloseFigure();
    SolidBrush br(c); g.FillPath(&br, &path); if (border) { Pen p(Theme::Border, 1.2f); g.DrawPath(&p, &path); }
}

void DrawGlass(Graphics &g, RectF r) {
    DrawPremiumRect(g, r, 12, Theme::CardBg, true);
    LinearGradientBrush shine(PointF(r.X, r.Y), PointF(r.X+r.Width, r.Y+r.Height), Color(40, 255, 255, 255), Color(0, 0, 0, 0)); g.FillRectangle(&shine, r);
}

void RepositionControls() {
    RECT rc; GetClientRect(hCont, &rc); int w = rc.right, h = rc.bottom;
    if (activeView == ACCOUNTS) {
        HWND b = GetDlgItem(hCont, 6001); if(b) MoveWindow(b, w/2 - 120, h - 80, 240, 45, TRUE);
    } else if (activeView == REGISTER) {
        MoveWindow(GetDlgItem(hCont, 7002), w/2-150, 280, 300, 35, TRUE);
        MoveWindow(GetDlgItem(hCont, 7003), w/2-150, 340, 300, 35, TRUE);
        MoveWindow(GetDlgItem(hCont, 7004), w/2-150, 400, 300, 35, TRUE);
        MoveWindow(GetDlgItem(hCont, 7005), w/2-150, 460, 300, 45, TRUE);
    } else if (activeView == LOGIN) {
        MoveWindow(GetDlgItem(hCont, 5001), w/2-100, h/2, 200, 40, TRUE);
        MoveWindow(GetDlgItem(hCont, 5002), w/2-100, h/2+60, 200, 45, TRUE);
    } else if (activeView == BANKING || activeView == STOCKS) {
        MoveWindow(GetDlgItem(hCont, 4000), w-180, 30, 150, 40, TRUE);
    }
}

void RequestView(ViewID vid) {
    activeView = vid; for(auto c : controls) { ShowWindow(c, SW_HIDE); DestroyWindow(c); } controls.clear();
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hMain, GWLP_HINSTANCE);
    RECT rc; GetClientRect(hMain, &rc); int sw = (vid >= BANKING) ? 240 : 0;
    ShowWindow(hSide, sw ? SW_SHOW : SW_HIDE); SetWindowPos(hCont, NULL, sw, 0, rc.right - sw, rc.bottom, SWP_NOZORDER);
    RECT cc; GetClientRect(hCont, &cc); int cw = cc.right, ch = cc.bottom;
    if (vid == ACCOUNTS) controls.push_back(CreateWindowW(L"BUTTON", L"+ NEW VAULT", WS_VISIBLE|WS_CHILD, cw/2-120, ch-80, 240, 45, hCont, (HMENU)6001, hInst, NULL));
    else if (vid == REGISTER) {
        controls.push_back(CreateWindowExW(0, L"EDIT", L"", WS_VISIBLE|WS_CHILD|WS_BORDER, cw/2-150, 280, 300, 35, hCont, (HMENU)7002, hInst, NULL));
        controls.push_back(CreateWindowExW(0, L"EDIT", L"", WS_VISIBLE|WS_CHILD|WS_BORDER|ES_PASSWORD|ES_NUMBER, cw/2-150, 340, 300, 35, hCont, (HMENU)7003, hInst, NULL));
        controls.push_back(CreateWindowExW(0, L"EDIT", L"1000", WS_VISIBLE|WS_CHILD|WS_BORDER|ES_NUMBER, cw/2-150, 400, 300, 35, hCont, (HMENU)7004, hInst, NULL));
        controls.push_back(CreateWindowW(L"BUTTON", L"CREATE SECURE VAULT", WS_VISIBLE|WS_CHILD, cw/2-150, 460, 300, 45, hCont, (HMENU)7005, hInst, NULL));
        controls.push_back(CreateWindowW(L"BUTTON", L"BACK", WS_VISIBLE|WS_CHILD, 50, 50, 100, 40, hCont, (HMENU)7006, hInst, NULL));
    } else if (vid == LOGIN) {
        controls.push_back(CreateWindowExW(0, L"EDIT", L"", WS_VISIBLE|WS_CHILD|WS_BORDER|ES_PASSWORD|ES_NUMBER|ES_CENTER, cw/2-100, ch/2, 200, 40, hCont, (HMENU)5001, hInst, NULL));
        controls.push_back(CreateWindowW(L"BUTTON", L"UNLOCK VAULT", WS_VISIBLE|WS_CHILD, cw/2-100, ch/2+60, 200, 45, hCont, (HMENU)5002, hInst, NULL));
        controls.push_back(CreateWindowW(L"BUTTON", L"BACK", WS_VISIBLE|WS_CHILD, 50, 50, 100, 40, hCont, (HMENU)7006, hInst, NULL));
    } else if (vid == PORTALS) {
        controls.push_back(CreateWindowW(L"BUTTON", L"CENTRAL BANKING", WS_VISIBLE|WS_CHILD, cw/2-350, ch/2-100, 300, 200, hCont, (HMENU)4001, hInst, NULL));
        controls.push_back(CreateWindowW(L"BUTTON", L"GLOBAL MARKETS", WS_VISIBLE|WS_CHILD, cw/2+50, ch/2-100, 300, 200, hCont, (HMENU)4002, hInst, NULL));
        controls.push_back(CreateWindowW(L"BUTTON", L"LOCK SESSION", WS_VISIBLE|WS_CHILD, 50, 50, 150, 40, hCont, (HMENU)5003, hInst, NULL));
    } else if (vid == BANKING) {
        controls.push_back(CreateWindowExW(0, L"EDIT", L"0", WS_VISIBLE|WS_CHILD|WS_BORDER|ES_NUMBER, 70, 480, 200, 40, hCont, (HMENU)3001, hInst, NULL));
        controls.push_back(CreateWindowW(L"BUTTON", L"DEPOSIT", WS_VISIBLE|WS_CHILD, 70, 535, 120, 45, hCont, (HMENU)3003, hInst, NULL));
        controls.push_back(CreateWindowW(L"BUTTON", L"WITHDRAW", WS_VISIBLE|WS_CHILD, 200, 535, 120, 45, hCont, (HMENU)3004, hInst, NULL));
        controls.push_back(CreateWindowExW(0, L"EDIT", L"", WS_VISIBLE|WS_CHILD|WS_BORDER, 350, 480, 240, 40, hCont, (HMENU)3002, hInst, NULL));
        controls.push_back(CreateWindowW(L"BUTTON", L"INSTANT TRANSFER", WS_VISIBLE|WS_CHILD, 350, 535, 240, 45, hCont, (HMENU)3005, hInst, NULL));
        controls.push_back(CreateWindowW(L"BUTTON", L"SWITCH PORTAL", WS_VISIBLE|WS_CHILD, cw-180, 30, 150, 40, hCont, (HMENU)4000, hInst, NULL));
    } else if (vid == STOCKS) {
        for(int i=0; i<5; i++) {
            controls.push_back(CreateWindowW(L"BUTTON", L"BUY", WS_VISIBLE|WS_CHILD, 520, 158+i*80, 80, 35, hCont, (HMENU)(8000+i), hInst, NULL));
            controls.push_back(CreateWindowW(L"BUTTON", L"SELL", WS_VISIBLE|WS_CHILD, 610, 158+i*80, 80, 35, hCont, (HMENU)(9000+i), hInst, NULL));
        }
        controls.push_back(CreateWindowW(L"BUTTON", L"SWITCH PORTAL", WS_VISIBLE|WS_CHILD, cw-180, 30, 150, 40, hCont, (HMENU)4000, hInst, NULL));
    }
    RepositionControls(); InvalidateRect(hCont, NULL, TRUE);
}

// ==========================================
// PROCS
// ==========================================
LRESULT CALLBACK ContentProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_COMMAND) { SendMessage(hMain, WM_COMMAND, wp, lp); return 0; }
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps); RECT rc; GetClientRect(hwnd, &rc);
        HDC mdc = CreateCompatibleDC(hdc); HBITMAP mbm = CreateCompatibleBitmap(hdc, rc.right, rc.bottom); SelectObject(mdc, mbm);
        Graphics g(mdc); g.SetSmoothingMode(SmoothingModeAntiAlias); g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
        SolidBrush bg(Theme::Bg); g.FillRectangle(&bg, 0, 0, rc.right, rc.bottom);
        FontFamily ff(L"Segoe UI"); Font fT(&ff, 28, FontStyleBold, UnitPixel); Font fS(&ff, 15, FontStyleRegular, UnitPixel); Font fP(&ff, 20, FontStyleBold, UnitPixel);
        SolidBrush w(Theme::TextTitle), d(Theme::TextDim), a(Theme::Accent);
        
        if (activeView == PRELOAD) {
            DrawPremiumRect(g, RectF(rc.right/2.0f-150, rc.bottom/2.0f+50, 300, 8), 4, Color(50,50,50,50));
            DrawPremiumRect(g, RectF(rc.right/2.0f-150, rc.bottom/2.0f+50, preloadPct*3.0f, 8), 4, Theme::Accent, false);
        } else if (activeView == ACCOUNTS) {
            g.DrawString(L"Select Secure Profile", -1, &fT, PointF(rc.right/2.0f-140, 60), &w);
            auto accs = dbInstance.LoadAccounts();
            for(int i=0; i<(int)accs.size(); i++){
                int col=i%3, row=i/3; RectF card(rc.right/2.0f-400+col*280, 140+row*240, 240, 180); DrawGlass(g, card);
                g.DrawString(accs[i].name.substr(0,1).c_str(), 1, &fT, PointF(card.X+100, card.Y+40), &a);
                RectF nr(card.X, card.Y+110, 240, 30); StringFormat sf; sf.SetAlignment(StringAlignmentCenter); 
                g.DrawString(accs[i].name.c_str(), -1, &fS, nr, &sf, &w);
            }
        } else if (activeView == BANKING) {
            g.DrawString(L"FINANCIAL OVERVIEW", -1, &fT, PointF(40, 40), &w); DrawGlass(g, RectF(40, 120, 380, 200));
            g.DrawString(L"LIQUID BALANCE", -1, &fS, PointF(70, 150), &d); wstringstream ss; ss << L"Rs. " << fixed << setprecision(2) << uBal; g.DrawString(ss.str().c_str(), -1, &fT, PointF(70, 185), &a);
            g.DrawString(L"SEARCH BY HOLDER NAME", -1, &fS, PointF(350, 455), &d);
            g.DrawString(L"REGISTERED PEERS", -1, &fS, PointF(660, 50), &d); auto list = dbInstance.LoadAccounts(); int k=0;
            for(auto& ac : list) if(ac.accNum != uID) { g.DrawString(ac.name.c_str(), -1, &fS, PointF(660, (REAL)(80+k*30)), &w); k++; }
        } else if (activeView == STOCKS) {
            g.DrawString(L"MARKET INTELLIGENCE", -1, &fT, PointF(40, 40), &w);
            RectF balBox(rc.right-360.0f, 80, 300, 60); DrawGlass(g, balBox);
            wstringstream bss; bss << L"BAL: Rs. " << fixed << setprecision(2) << uBal;
            g.DrawString(bss.str().c_str(), -1, &fP, PointF(balBox.X+20, balBox.Y+15), &a);

            for(int i=0; i<5; i++){
                DrawGlass(g, RectF(40, (REAL)(140+i*80), 720, 70)); if(i==stockSelIdx) { Pen sp(Theme::Accent, 1.5f); g.DrawRectangle(&sp, 40.0f, (REAL)(140+i*80), 720.0f, 70.0f); }
                g.DrawString(marketStocks[i].symbol.c_str(), -1, &fP, PointF(70, (REAL)(160+i*80)), &w);
                double avg = 0; int qty = dbInstance.GetOwnedStocks(uID, marketStocks[i].symbol, &avg);
                float pnl = (marketStocks[i].price - avg) * qty;
                wstringstream ps; ps << L"Rs." << fixed << setprecision(2) << marketStocks[i].price << L" | QTY: " << qty;
                g.DrawString(ps.str().c_str(), -1, &fS, PointF(220, (REAL)(165+i*80)), &a);
                if(qty > 0) {
                    wstringstream pss; pss << (pnl >= 0 ? L"+" : L"") << fixed << setprecision(2) << pnl;
                    SolidBrush pnlB(pnl >= 0 ? Color(255, 0, 255, 0) : Color(255, 255, 0, 0));
                    g.DrawString(pss.str().c_str(), -1, &fS, PointF(420, (REAL)(165+i*80)), &pnlB);
                }
            }
            RectF gBox(780, 160, max(200.0f, rc.right-820.0f), 400); DrawGlass(g, gBox);
            Pen gridP(Theme::GridLines, 1);
            for(int i=0; i<5; i++) { float gy = gBox.Y + 40 + i*(gBox.Height-80)/4; g.DrawLine(&gridP, gBox.X+40, gy, gBox.X+gBox.Width-40, gy); }
            for(int i=0; i<7; i++) { float gx = gBox.X + 40 + i*(gBox.Width-80)/6; g.DrawLine(&gridP, gx, gBox.Y+40, gx, gBox.Y+gBox.Height-40); }

            auto& h = marketStocks[stockSelIdx].history;
            if(h.size()>1){
                float minP=h[0], maxP=h[0]; for(float v:h){ minP=min(minP,v); maxP=max(maxP,v); }
                float r=max(maxP-minP,1.0f); Pen p(Theme::Accent, 2.5f);
                for(size_t i=0; i<h.size()-1; i++){
                    float x1=gBox.X+40+(i*(gBox.Width-80)/(h.size()-1)), y1=gBox.Y+gBox.Height-40-((h[i]-minP)*(gBox.Height-80)/r);
                    float x2=gBox.X+40+((i+1)*(gBox.Width-80)/(h.size()-1)), y2=gBox.Y+gBox.Height-40-((h[i+1]-minP)*(gBox.Height-80)/r);
                    g.DrawLine(&p,x1,y1,x2,y2); 
                }
            }
        }
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, mdc, 0, 0, SRCCOPY); DeleteObject(mbm); DeleteDC(mdc); EndPaint(hwnd, &ps); return 0;
    }
    if (msg == WM_LBUTTONDOWN) {
        int x=LOWORD(lp), y=HIWORD(lp); RECT rc; GetClientRect(hwnd, &rc);
        if (activeView == ACCOUNTS) {
            auto accs = dbInstance.LoadAccounts(); 
            for(int i=0; i<(int)accs.size(); i++){
                int col=i%3, row=i/3;
                if(x>rc.right/2-400+col*280 && x<rc.right/2-400+col*280+240 && y>140+row*240 && y<140+row*240+180) {
                    uName=accs[i].name; uID=accs[i].accNum; uPIN=accs[i].pin; uBal=accs[i].balance; RequestView(LOGIN); break;
                }
            }
        } else if (activeView == STOCKS) { 
            for(int i=0; i<5; i++) if(y>140+i*80 && y<210+i*80) { stockSelIdx=i; InvalidateRect(hwnd, NULL, TRUE); break; }
        } else if (activeView == BANKING && x > 660) {
            auto list = dbInstance.LoadAccounts(); int k=0;
            for(auto& ac : list) if(ac.accNum!=uID) { if(y>80+k*30 && y<105+k*30) { SetWindowTextW(GetDlgItem(hCont, 3002), ac.name.c_str()); break; } k++; } 
        }
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

LRESULT CALLBACK SideProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_COMMAND) { SendMessage(hMain, WM_COMMAND, wp, lp); return 0; }
    if (msg == WM_PAINT) { PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps); RECT rc; GetClientRect(hwnd, &rc); Graphics g(hdc); SolidBrush bg(Theme::Sidebar); g.FillRectangle(&bg, 0, 0, rc.right, rc.bottom); FontFamily ff(L"Segoe UI"); Font f(&ff, 18, FontStyleBold, UnitPixel); SolidBrush a(Theme::Accent); g.DrawString(L"EVAULT PRO", -1, &f, PointF(30, 40), &a); EndPaint(hwnd, &ps); return 0; }
    return DefWindowProc(hwnd, msg, wp, lp);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: hMain = hwnd; hSide = CreateWindowExW(0, L"Side", NULL, WS_VISIBLE|WS_CHILD, 0,0, 240, 800, hwnd, NULL, NULL, NULL); hCont = CreateWindowExW(0, L"Cont", NULL, WS_VISIBLE|WS_CHILD, 240,0, 1040, 800, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"BUTTON", L"BANKING", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 10, 120, 220, 50, hSide, (HMENU)101, NULL, NULL); CreateWindowW(L"BUTTON", L"STOCKS", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 10, 180, 220, 50, hSide, (HMENU)102, NULL, NULL); dbInstance.Init(); SetTimer(hwnd, 1, 50, NULL); SetTimer(hwnd, 2, 2000, NULL); break;
    case WM_TIMER: 
        if(wp == 1) { preloadPct += 5; if(preloadPct >= 100) { KillTimer(hwnd,1); RequestView(ACCOUNTS); } InvalidateRect(hCont, NULL, TRUE); }
        if(wp == 2 && activeView == STOCKS) {
            for(auto& s : marketStocks) { float dev = ((rand()%20)-10)/100.0f; s.price *= (1.0f + dev); s.history.push_back(s.price); if(s.history.size()>15) s.history.erase(s.history.begin()); }
            InvalidateRect(hCont, NULL, TRUE);
        }
        break;
    case WM_DRAWITEM: { DRAWITEMSTRUCT* p = (DRAWITEMSTRUCT*)lp; Graphics g(p->hDC); RectF r(0,0,(REAL)p->rcItem.right,(REAL)p->rcItem.bottom); bool sel = (activeView == BANKING && p->CtlID == 101) || (activeView == STOCKS && p->CtlID == 102); DrawPremiumRect(g, r, 12, sel ? Color(60, 0, 240, 255) : Color(0,0,0,0), false);
        WCHAR txt[64]; GetWindowTextW(p->hwndItem, txt, 64); FontFamily ff(L"Segoe UI"); Font f(&ff, 13, FontStyleBold, UnitPixel); SolidBrush w(Theme::TextTitle); StringFormat sf; sf.SetAlignment(StringAlignmentNear); sf.SetLineAlignment(StringAlignmentCenter); RectF tr(25, 0, r.Width-25, r.Height); g.DrawString(txt, -1, &f, tr, &sf, &w); return TRUE; }
    case WM_COMMAND: { int id = LOWORD(wp);
        if (id == 101) RequestView(BANKING); else if (id == 102) RequestView(STOCKS); else if (id == 4000) RequestView(PORTALS); else if (id == 5003) RequestView(ACCOUNTS); else if (id == 6001) RequestView(REGISTER); else if (id == 7006) RequestView(ACCOUNTS); else if (id == 4001) RequestView(BANKING); else if (id == 4002) RequestView(STOCKS);
        else if (id == 5002) { WCHAR p[16]; GetWindowTextW(GetDlgItem(hCont, 5001), p, 16); if(wstring(p) == uPIN) RequestView(PORTALS); else MessageBoxW(hwnd, L"DENIED", L"SEC", MB_ICONERROR); }
        else if (id == 3003 || id == 3004) { WCHAR a[32]; GetWindowTextW(GetDlgItem(hCont, 3001), a, 32); double amt = _wtof(a); if(amt > 0 && (id == 3003 || amt <= uBal)){ uBal += (id == 3003 ? amt : -amt); dbInstance.UpdateBalance(uID, uBal); InvalidateRect(hCont, NULL, TRUE); } }
        else if (id == 3005) { WCHAR a[32], t[32]; GetWindowTextW(GetDlgItem(hCont, 3001), a, 32); GetWindowTextW(GetDlgItem(hCont, 3002), t, 32); double amt = _wtof(a); 
            int res = dbInstance.Transfer(uID, t, amt); 
            if(res == 0) { uBal -= amt; dbInstance.UpdateBalance(uID, uBal); InvalidateRect(hCont, NULL, TRUE); MessageBoxW(hwnd, L"SUCCESS", L"SEC", MB_OK); } 
            else if(res == 4) MessageBoxW(hwnd, L"RECIPIENT NOT FOUND", L"SEC", MB_ICONERROR);
            else MessageBoxW(hwnd, L"FAILED", L"SEC", MB_ICONERROR); }
        else if (id >= 8000 && id < 8005) { int i=id-8000; if(uBal>=marketStocks[i].price && dbInstance.UpdateStocks(uID, marketStocks[i].symbol, 1, marketStocks[i].price)){ uBal-=marketStocks[i].price; dbInstance.UpdateBalance(uID, uBal); InvalidateRect(hCont, NULL, TRUE); } }
        else if (id >= 9000 && id < 9005) { int i=id-9000; if(dbInstance.UpdateStocks(uID, marketStocks[i].symbol, -1, marketStocks[i].price)){ uBal+=marketStocks[i].price; dbInstance.UpdateBalance(uID, uBal); InvalidateRect(hCont, NULL, TRUE); } }
        else if (id == 7005) { WCHAR n[64], p[16], d[16]; GetWindowTextW(GetDlgItem(hCont, 7002), n, 64); GetWindowTextW(GetDlgItem(hCont, 7003), p, 16); GetWindowTextW(GetDlgItem(hCont, 7004), d, 16); if(wcslen(n)<2 || wcslen(p)!=4) return 0; srand((unsigned)time(0)); wstringstream ac; for(int i=0;i<8;i++) ac << rand()%10; if(dbInstance.CreateAccount(ac.str(), n, p, _wtof(d))) RequestView(ACCOUNTS); }
    } break;
    case WM_SIZE: { int w=LOWORD(lp), h=HIWORD(lp); int sw=(activeView >= BANKING)?240:0; MoveWindow(hSide, 0,0, sw, h, TRUE); MoveWindow(hCont, sw,0, w-sw, h, TRUE); RepositionControls(); } break;
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProc(hwnd, msg, wp, lp);
    } return 0;
}

int WINAPI WinMain(HINSTANCE h, HINSTANCE hp, LPSTR c, int s) {
    GdiplusStartupInput gsi; GdiplusStartup(&gdiplusToken, &gsi, NULL);
    WNDCLASSW wc = {0}; wc.lpfnWndProc = WndProc; wc.hInstance = h; wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.lpszClassName = L"EV"; RegisterClassW(&wc);
    wc.lpfnWndProc = SideProc; wc.lpszClassName = L"Side"; RegisterClassW(&wc);
    wc.lpfnWndProc = ContentProc; wc.lpszClassName = L"Cont"; RegisterClassW(&wc);
    hMain = CreateWindowExW(0, L"EV", L"EVAULT PRO", WS_OVERLAPPEDWINDOW|WS_VISIBLE, 100, 100, 1280, 800, NULL, NULL, h, NULL);
    MSG m; while(GetMessage(&m, NULL, 0, 0)) { TranslateMessage(&m); DispatchMessage(&m); }
    GdiplusShutdown(gdiplusToken); return (int)m.wParam;
}
