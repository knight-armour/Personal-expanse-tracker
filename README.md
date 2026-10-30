# ZenSpend — Personal Expense & Daily Streak Tracker (C Language) 💳🔥

A personal expense tracker written in **Pure C** featuring user input forms, financial calculations, CSV data persistence, and a gamified **Daily Habit Tracking Streak** engine.

---

## 📦 What's Included

The project provides two C implementations:

1. **[expense_tracker_gui.c](file:///C:/Users/hp/.gemini/antigravity-ide/scratch/expense-tracker-c/expense_tracker_gui.c)**:
   - **Native Windows GUI Desktop App** (`expense_tracker_gui.exe`)
   - Built with the native Win32 API (`<windows.h>`, `<commctrl.h>`)
   - Requires **zero external dependencies or DLLs**
   - Features input edit boxes, category dropdowns, status indicators, and a multi-column transaction ListView
   - Displays real-time streak banners and celebratory dialogs when your streak increases!

2. **[expense_tracker_cli.c](file:///C:/Users/hp/.gemini/antigravity-ide/scratch/expense-tracker-c/expense_tracker_cli.c)**:
   - **Interactive Terminal / Console TUI**
   - Features ASCII flame art, ANSI color formatting, interactive numbered menus, and horizontal category distribution bars

3. **[build.bat](file:///C:/Users/hp/.gemini/antigravity-ide/scratch/expense-tracker-c/build.bat)**:
   - One-click Windows batch compilation script using MinGW GCC

---

## 🚀 How to Run and Compile

### Running the Desktop GUI App
The Windows executable has already been compiled:
- Double-click **[expense_tracker_gui.exe](file:///C:/Users/hp/.gemini/antigravity-ide/scratch/expense-tracker-c/expense_tracker_gui.exe)** or run:
  ```powershell
  .\expense_tracker_gui.exe
  ```

### Compiling with MinGW GCC
To recompile either version manually:

- **Compile Desktop GUI:**
  ```bash
  gcc -O2 -mwindows -o expense_tracker_gui.exe expense_tracker_gui.c -lcomctl32 -lgdi32
  ```

- **Compile Terminal CLI:**
  ```bash
  gcc -O2 -o expense_tracker_cli.exe expense_tracker_cli.c
  ```

- **Compile Both (One-Click):**
  Double-click **[build.bat](file:///C:/Users/hp/.gemini/antigravity-ide/scratch/expense-tracker-c/build.bat)**.

---

## 🔥 How the Daily Streak Engine Works in C

The streak engine tracks consecutive daily habits:
1. **Julian Day Arithmetic:** Dates (`YYYY-MM-DD`) are converted into integer day values using standard Gregorian calendar calculations (`DateToDays()`), guaranteeing accurate day difference math across months, leap years, and year-ends.
2. **Consecutive Day Verification:**
   - Extracts all unique logging dates.
   - If the user logs an entry today, it walks backward from today to count unbroken consecutive days.
   - If the user logged yesterday, the streak is marked as **"Pending Today"** (at risk, but alive!).
   - If neither today nor yesterday was logged, the streak resets to 0.
3. **Streak Celebration:** When the user enters their first expense for today, the app detects the streak extension and presents a celebratory dialog box!
4. **Data Storage:** All transactions automatically persist in `transactions.csv` so your streak continues every day.
