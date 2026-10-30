/**
 * ZenSpend — Personal Expense & Daily Streak Tracker
 * Native Windows Win32 GUI Application in Pure C
 * 
 * Features:
 * - Full Graphical User Interface (Windows API)
 * - User Input Form (Amount, Type, Category, Date, Payment, Note)
 * - Gamified Daily Habit Streak Engine (Consecutive Day Tracking)
 * - Real-time KPI summaries (Balance, Income, Expenses)
 * - Transaction ListView with sorting and row selection
 * - Persistent storage (transactions.csv)
 */

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef LVS_EX_FULLROWSELECT
#define LVS_EX_FULLROWSELECT 0x00000020
#endif
#ifndef LVS_EX_GRIDLINES
#define LVS_EX_GRIDLINES 0x00000001
#endif
#ifndef ListView_SetExtendedListViewStyle
#define ListView_SetExtendedListViewStyle(w, s) SendMessage((w), 0x1000 + 54, 0, (LPARAM)(s))
#endif

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")

#define MAX_TRANSACTIONS 1000
#define MAX_NOTE_LEN 128
#define MAX_CAT_LEN 48
#define MAX_PAYMENT_LEN 32
#define MAX_DATE_LEN 16

/* Control IDs */
#define ID_AMOUNT_EDIT      101
#define ID_TYPE_COMBO       102
#define ID_CAT_COMBO        103
#define ID_DATE_EDIT        104
#define ID_PAYMENT_COMBO    105
#define ID_NOTE_EDIT        106
#define ID_ADD_BTN          107
#define ID_LISTVIEW         108
#define ID_DELETE_BTN       109
#define ID_EXPORT_BTN       110
#define ID_DEMO_BTN         111
#define ID_CLEAR_BTN        112

/* Transaction Data Model */
typedef struct {
    char id[32];
    char date[MAX_DATE_LEN]; /* YYYY-MM-DD */
    char type[16];          /* "Expense" or "Income" */
    char category[MAX_CAT_LEN];
    double amount;
    char payment[MAX_PAYMENT_LEN];
    char note[MAX_NOTE_LEN];
} Transaction;

/* Streak Statistics */
typedef struct {
    int current_streak;
    int longest_streak;
    int total_days;
    int logged_today;
    int logged_yesterday;
} StreakInfo;

/* Global State */
static Transaction g_transactions[MAX_TRANSACTIONS];
static int g_trans_count = 0;
static StreakInfo g_streak;
static const char* CSV_FILENAME = "transactions.csv";

/* GUI Window & Control Handles */
static HWND g_hwndMain;
static HWND g_hLblStreakTitle;
static HWND g_hLblStreakStatus;
static HWND g_hLblStreakStats;
static HWND g_hLblBalance;
static HWND g_hLblIncome;
static HWND g_hLblExpense;

static HWND g_hEditAmount;
static HWND g_hComboType;
static HWND g_hComboCategory;
static HWND g_hEditDate;
static HWND g_hComboPayment;
static HWND g_hEditNote;
static HWND g_hBtnAdd;

static HWND g_hListView;
static HWND g_hBtnDelete;
static HWND g_hBtnExport;
static HWND g_hBtnDemo;
static HWND g_hBtnClear;

static HFONT g_hFontNormal;
static HFONT g_hFontBold;
static HFONT g_hFontTitle;
static HFONT g_hFontLarge;

/* Forward Declarations */
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void InitControls(HWND hwnd);
void RefreshAllUI(void);
void ComputeStreak(void);
void LoadTransactionsFromCSV(void);
void SaveTransactionsToCSV(void);
void LoadDemoData(void);
void GetTodayString(char* buffer);
void GetYesterdayString(char* buffer);
int DateToDays(int year, int month, int day);
int ParseDateToDays(const char* dateStr);

/* Categories */
static const char* EXPENSE_CATEGORIES[] = {
    "Food & Dining",
    "Groceries",
    "Transportation",
    "Housing & Rent",
    "Bills & Utilities",
    "Entertainment",
    "Shopping",
    "Health & Medical",
    "Education",
    "Miscellaneous"
};
#define NUM_EXPENSE_CATS (sizeof(EXPENSE_CATEGORIES) / sizeof(EXPENSE_CATEGORIES[0]))

static const char* INCOME_CATEGORIES[] = {
    "Salary / Wages",
    "Freelance / Gig",
    "Investments",
    "Gifts / Grants",
    "Other Income"
};
#define NUM_INCOME_CATS (sizeof(INCOME_CATEGORIES) / sizeof(INCOME_CATEGORIES[0]))

/* Date Conversion to Julian Days (Gregorian Calendar) */
int DateToDays(int y, int m, int d) {
    int a, b, c, e, f;
    if (m <= 2) {
        y -= 1;
        m += 12;
    }
    a = y / 100;
    b = a / 4;
    c = 2 - a + b;
    e = (int)(365.25 * (y + 4716));
    f = (int)(30.6001 * (m + 1));
    return (int)(c + d + e + f - 1524);
}

int ParseDateToDays(const char* dateStr) {
    int y = 0, m = 0, d = 0;
    if (sscanf(dateStr, "%d-%d-%d", &y, &m, &d) == 3) {
        return DateToDays(y, m, d);
    }
    return 0;
}

void GetTodayString(char* buffer) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    sprintf(buffer, "%04d-%02d-%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
}

void GetYesterdayString(char* buffer) {
    time_t now = time(NULL) - 86400;
    struct tm* t = localtime(&now);
    sprintf(buffer, "%04d-%02d-%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
}

/* Compare helper for qsort of integers */
static int CompareInts(const void* a, const void* b) {
    return (*(const int*)a - *(const int*)b);
}

/* Streak Engine */
void ComputeStreak(void) {
    int uniqueDays[MAX_TRANSACTIONS];
    int uniqueCount = 0;
    int i, j, already;
    char todayStr[16];
    char yestStr[16];
    int todayDays, yestDays;

    GetTodayString(todayStr);
    GetYesterdayString(yestStr);

    todayDays = ParseDateToDays(todayStr);
    yestDays = ParseDateToDays(yestStr);

    g_streak.logged_today = 0;
    g_streak.logged_yesterday = 0;

    for (i = 0; i < g_trans_count; i++) {
        int d = ParseDateToDays(g_transactions[i].date);
        if (d <= 0) continue;

        if (d == todayDays) g_streak.logged_today = 1;
        if (d == yestDays) g_streak.logged_yesterday = 1;

        already = 0;
        for (j = 0; j < uniqueCount; j++) {
            if (uniqueDays[j] == d) {
                already = 1;
                break;
            }
        }
        if (!already && uniqueCount < MAX_TRANSACTIONS) {
            uniqueDays[uniqueCount++] = d;
        }
    }

    g_streak.total_days = uniqueCount;

    if (uniqueCount == 0) {
        g_streak.current_streak = 0;
        g_streak.longest_streak = 0;
        return;
    }

    qsort(uniqueDays, uniqueCount, sizeof(int), CompareInts);

    /* Calculate Current Streak */
    g_streak.current_streak = 0;
    if (g_streak.logged_today) {
        int target = todayDays;
        while (1) {
            int found = 0;
            for (i = 0; i < uniqueCount; i++) {
                if (uniqueDays[i] == target) {
                    found = 1;
                    break;
                }
            }
            if (found) {
                g_streak.current_streak++;
                target--;
            } else {
                break;
            }
        }
    } else if (g_streak.logged_yesterday) {
        int target = yestDays;
        while (1) {
            int found = 0;
            for (i = 0; i < uniqueCount; i++) {
                if (uniqueDays[i] == target) {
                    found = 1;
                    break;
                }
            }
            if (found) {
                g_streak.current_streak++;
                target--;
            } else {
                break;
            }
        }
    } else {
        g_streak.current_streak = 0;
    }

    /* Calculate Longest Consecutive Streak across history */
    {
        int maxStreak = 1;
        int curStreak = 1;
        for (i = 1; i < uniqueCount; i++) {
            if (uniqueDays[i] == uniqueDays[i - 1] + 1) {
                curStreak++;
                if (curStreak > maxStreak) {
                    maxStreak = curStreak;
                }
            } else if (uniqueDays[i] > uniqueDays[i - 1] + 1) {
                curStreak = 1;
            }
        }
        g_streak.longest_streak = (maxStreak > g_streak.current_streak) ? maxStreak : g_streak.current_streak;
    }
}

/* File Persistence */
void SaveTransactionsToCSV(void) {
    FILE* fp = fopen(CSV_FILENAME, "w");
    int i;
    if (!fp) return;

    fprintf(fp, "ID,Date,Type,Category,Amount,Payment,Note\n");
    for (i = 0; i < g_trans_count; i++) {
        fprintf(fp, "%s,%s,%s,\"%s\",%.2f,\"%s\",\"%s\"\n",
            g_transactions[i].id,
            g_transactions[i].date,
            g_transactions[i].type,
            g_transactions[i].category,
            g_transactions[i].amount,
            g_transactions[i].payment,
            g_transactions[i].note
        );
    }
    fclose(fp);
}

void LoadTransactionsFromCSV(void) {
    FILE* fp = fopen(CSV_FILENAME, "r");
    char line[512];
    if (!fp) {
        LoadDemoData();
        return;
    }

    /* Skip header */
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return;
    }

    g_trans_count = 0;
    while (fgets(line, sizeof(line), fp) && g_trans_count < MAX_TRANSACTIONS) {
        Transaction* t = &g_transactions[g_trans_count];
        char* token;
        char* p = line;

        /* Remove trailing newline */
        p[strcspn(p, "\r\n")] = 0;

        /* ID */
        token = strtok(p, ",");
        if (!token) continue;
        strncpy(t->id, token, sizeof(t->id) - 1);

        /* Date */
        token = strtok(NULL, ",");
        if (!token) continue;
        strncpy(t->date, token, sizeof(t->date) - 1);

        /* Type */
        token = strtok(NULL, ",");
        if (!token) continue;
        strncpy(t->type, token, sizeof(t->type) - 1);

        /* Category */
        token = strtok(NULL, ",");
        if (!token) continue;
        if (token[0] == '"') token++;
        if (token[strlen(token) - 1] == '"') token[strlen(token) - 1] = 0;
        strncpy(t->category, token, sizeof(t->category) - 1);

        /* Amount */
        token = strtok(NULL, ",");
        if (!token) continue;
        t->amount = atof(token);

        /* Payment */
        token = strtok(NULL, ",");
        if (!token) continue;
        if (token[0] == '"') token++;
        if (token[strlen(token) - 1] == '"') token[strlen(token) - 1] = 0;
        strncpy(t->payment, token, sizeof(t->payment) - 1);

        /* Note */
        token = strtok(NULL, "");
        if (!token) token = "";
        if (token[0] == '"') token++;
        if (strlen(token) > 0 && token[strlen(token) - 1] == '"') token[strlen(token) - 1] = 0;
        strncpy(t->note, token, sizeof(t->note) - 1);

        g_trans_count++;
    }
    fclose(fp);
}

void LoadDemoData(void) {
    time_t now = time(NULL);
    struct tm* tm_info;
    char d0[16], d1[16], d2[16], d3[16], d4[16];
    time_t t;

    t = now; tm_info = localtime(&t);
    sprintf(d0, "%04d-%02d-%02d", tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);

    t = now - 86400; tm_info = localtime(&t);
    sprintf(d1, "%04d-%02d-%02d", tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);

    t = now - 86400 * 2; tm_info = localtime(&t);
    sprintf(d2, "%04d-%02d-%02d", tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);

    t = now - 86400 * 3; tm_info = localtime(&t);
    sprintf(d3, "%04d-%02d-%02d", tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);

    t = now - 86400 * 4; tm_info = localtime(&t);
    sprintf(d4, "%04d-%02d-%02d", tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);

    g_trans_count = 0;

    /* 5 consecutive days of activity */
    strcpy(g_transactions[g_trans_count].id, "tx_1");
    strcpy(g_transactions[g_trans_count].date, d0);
    strcpy(g_transactions[g_trans_count].type, "Expense");
    strcpy(g_transactions[g_trans_count].category, "Food & Dining");
    g_transactions[g_trans_count].amount = 14.50;
    strcpy(g_transactions[g_trans_count].payment, "Credit/Debit Card");
    strcpy(g_transactions[g_trans_count].note, "Morning artisan coffee & bagel");
    g_trans_count++;

    strcpy(g_transactions[g_trans_count].id, "tx_2");
    strcpy(g_transactions[g_trans_count].date, d0);
    strcpy(g_transactions[g_trans_count].type, "Expense");
    strcpy(g_transactions[g_trans_count].category, "Groceries");
    g_transactions[g_trans_count].amount = 45.20;
    strcpy(g_transactions[g_trans_count].payment, "Credit/Debit Card");
    strcpy(g_transactions[g_trans_count].note, "Supermarket weekly fruits");
    g_trans_count++;

    strcpy(g_transactions[g_trans_count].id, "tx_3");
    strcpy(g_transactions[g_trans_count].date, d1);
    strcpy(g_transactions[g_trans_count].type, "Expense");
    strcpy(g_transactions[g_trans_count].category, "Transportation");
    g_transactions[g_trans_count].amount = 22.00;
    strcpy(g_transactions[g_trans_count].payment, "UPI / Online");
    strcpy(g_transactions[g_trans_count].note, "Metro transit card reload");
    g_trans_count++;

    strcpy(g_transactions[g_trans_count].id, "tx_4");
    strcpy(g_transactions[g_trans_count].date, d2);
    strcpy(g_transactions[g_trans_count].type, "Income");
    strcpy(g_transactions[g_trans_count].category, "Salary / Wages");
    g_transactions[g_trans_count].amount = 1950.00;
    strcpy(g_transactions[g_trans_count].payment, "Bank Transfer");
    strcpy(g_transactions[g_trans_count].note, "Monthly engineering payroll");
    g_trans_count++;

    strcpy(g_transactions[g_trans_count].id, "tx_5");
    strcpy(g_transactions[g_trans_count].date, d2);
    strcpy(g_transactions[g_trans_count].type, "Expense");
    strcpy(g_transactions[g_trans_count].category, "Bills & Utilities");
    g_transactions[g_trans_count].amount = 65.00;
    strcpy(g_transactions[g_trans_count].payment, "UPI / Online");
    strcpy(g_transactions[g_trans_count].note, "High-speed broadband internet");
    g_trans_count++;

    strcpy(g_transactions[g_trans_count].id, "tx_6");
    strcpy(g_transactions[g_trans_count].date, d3);
    strcpy(g_transactions[g_trans_count].type, "Expense");
    strcpy(g_transactions[g_trans_count].category, "Entertainment");
    g_transactions[g_trans_count].amount = 18.99;
    strcpy(g_transactions[g_trans_count].payment, "Credit/Debit Card");
    strcpy(g_transactions[g_trans_count].note, "Streaming service renewal");
    g_trans_count++;

    strcpy(g_transactions[g_trans_count].id, "tx_7");
    strcpy(g_transactions[g_trans_count].date, d4);
    strcpy(g_transactions[g_trans_count].type, "Expense");
    strcpy(g_transactions[g_trans_count].category, "Shopping");
    g_transactions[g_trans_count].amount = 38.40;
    strcpy(g_transactions[g_trans_count].payment, "Credit/Debit Card");
    strcpy(g_transactions[g_trans_count].note, "Desk lamp & organizer");
    g_trans_count++;

    SaveTransactionsToCSV();
}

/* UI Refresh Functions */
void RefreshAllUI(void) {
    char buf[256];
    double totalIncome = 0.0;
    double totalExpense = 0.0;
    double netBalance = 0.0;
    int i;

    ComputeStreak();

    /* 1. Update Streak Banner */
    sprintf(buf, "[STREAK: %d DAYS ACTIVE]", g_streak.current_streak);
    SetWindowText(g_hLblStreakTitle, buf);

    if (g_streak.logged_today) {
        sprintf(buf, "Status: Logged Today! Streak Protected! (Habit is burning hot)");
    } else if (g_streak.logged_yesterday && g_streak.current_streak > 0) {
        sprintf(buf, "Status: Streak Pending! Log today to keep your %d-day streak alive!", g_streak.current_streak);
    } else {
        sprintf(buf, "Status: Ready to start! Log an expense today to ignite your streak!");
    }
    SetWindowText(g_hLblStreakStatus, buf);

    sprintf(buf, "Best Record: %d Days  |  Total Logged Days: %d  |  Records: %d",
        g_streak.longest_streak, g_streak.total_days, g_trans_count);
    SetWindowText(g_hLblStreakStats, buf);

    /* 2. Calculate Financial Totals */
    for (i = 0; i < g_trans_count; i++) {
        if (strcmp(g_transactions[i].type, "Income") == 0) {
            totalIncome += g_transactions[i].amount;
        } else {
            totalExpense += g_transactions[i].amount;
        }
    }
    netBalance = totalIncome - totalExpense;

    sprintf(buf, "Net Balance: $%.2f", netBalance);
    SetWindowText(g_hLblBalance, buf);

    sprintf(buf, "Total Income: +$%.2f", totalIncome);
    SetWindowText(g_hLblIncome, buf);

    sprintf(buf, "Total Expenses: -$%.2f", totalExpense);
    SetWindowText(g_hLblExpense, buf);

    /* 3. Populate ListView */
    ListView_DeleteAllItems(g_hListView);

    for (i = g_trans_count - 1; i >= 0; i--) {
        LVITEM lvi;
        int itemIndex;
        char amtBuf[32];

        memset(&lvi, 0, sizeof(lvi));
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = g_trans_count - 1 - i;
        lvi.iSubItem = 0;
        lvi.pszText = g_transactions[i].date;
        lvi.lParam = (LPARAM)i;

        itemIndex = ListView_InsertItem(g_hListView, &lvi);

        /* Subitem 1: Type */
        ListView_SetItemText(g_hListView, itemIndex, 1, g_transactions[i].type);

        /* Subitem 2: Category */
        ListView_SetItemText(g_hListView, itemIndex, 2, g_transactions[i].category);

        /* Subitem 3: Amount */
        sprintf(amtBuf, "%s$%.2f",
            (strcmp(g_transactions[i].type, "Income") == 0) ? "+" : "-",
            g_transactions[i].amount);
        ListView_SetItemText(g_hListView, itemIndex, 3, amtBuf);

        /* Subitem 4: Payment */
        ListView_SetItemText(g_hListView, itemIndex, 4, g_transactions[i].payment);

        /* Subitem 5: Note */
        ListView_SetItemText(g_hListView, itemIndex, 5, g_transactions[i].note);
    }
}

/* Update Category Combo based on Type */
void UpdateCategoryCombo(void) {
    int typeIdx = (int)SendMessage(g_hComboType, CB_GETCURSEL, 0, 0);
    int i;
    SendMessage(g_hComboCategory, CB_RESETCONTENT, 0, 0);

    if (typeIdx == 1) { /* Income */
        for (i = 0; i < NUM_INCOME_CATS; i++) {
            SendMessage(g_hComboCategory, CB_ADDSTRING, 0, (LPARAM)INCOME_CATEGORIES[i]);
        }
    } else { /* Expense */
        for (i = 0; i < NUM_EXPENSE_CATS; i++) {
            SendMessage(g_hComboCategory, CB_ADDSTRING, 0, (LPARAM)EXPENSE_CATEGORIES[i]);
        }
    }
    SendMessage(g_hComboCategory, CB_SETCURSEL, 0, 0);
}

/* User Input Handler: Add Transaction */
void HandleAddTransaction(void) {
    char amtStr[32];
    char dateStr[MAX_DATE_LEN];
    char catStr[MAX_CAT_LEN];
    char payStr[MAX_PAYMENT_LEN];
    char noteStr[MAX_NOTE_LEN];
    double amount;
    int typeIdx;
    int prevStreak;
    int prevLoggedToday;

    GetWindowText(g_hEditAmount, amtStr, sizeof(amtStr));
    amount = atof(amtStr);
    if (amount <= 0.0) {
        MessageBox(g_hwndMain, "Please enter a valid amount greater than 0.", "Input Error", MB_OK | MB_ICONWARNING);
        SetFocus(g_hEditAmount);
        return;
    }

    GetWindowText(g_hEditNote, noteStr, sizeof(noteStr));
    if (strlen(noteStr) == 0) {
        MessageBox(g_hwndMain, "Please enter a description or note.", "Input Error", MB_OK | MB_ICONWARNING);
        SetFocus(g_hEditNote);
        return;
    }

    GetWindowText(g_hEditDate, dateStr, sizeof(dateStr));
    if (strlen(dateStr) < 8) {
        MessageBox(g_hwndMain, "Please enter a valid date in YYYY-MM-DD format.", "Input Error", MB_OK | MB_ICONWARNING);
        SetFocus(g_hEditDate);
        return;
    }

    if (g_trans_count >= MAX_TRANSACTIONS) {
        MessageBox(g_hwndMain, "Transaction storage limit reached (1000 items).", "Limit Reached", MB_OK | MB_ICONERROR);
        return;
    }

    typeIdx = (int)SendMessage(g_hComboType, CB_GETCURSEL, 0, 0);
    GetWindowText(g_hComboCategory, catStr, sizeof(catStr));
    GetWindowText(g_hComboPayment, payStr, sizeof(payStr));

    prevStreak = g_streak.current_streak;
    prevLoggedToday = g_streak.logged_today;

    /* Append Transaction */
    sprintf(g_transactions[g_trans_count].id, "tx_%ld", (long)time(NULL));
    strncpy(g_transactions[g_trans_count].date, dateStr, sizeof(g_transactions[g_trans_count].date) - 1);
    strcpy(g_transactions[g_trans_count].type, (typeIdx == 1) ? "Income" : "Expense");
    strncpy(g_transactions[g_trans_count].category, catStr, sizeof(g_transactions[g_trans_count].category) - 1);
    g_transactions[g_trans_count].amount = amount;
    strncpy(g_transactions[g_trans_count].payment, payStr, sizeof(g_transactions[g_trans_count].payment) - 1);
    strncpy(g_transactions[g_trans_count].note, noteStr, sizeof(g_transactions[g_trans_count].note) - 1);

    g_trans_count++;

    SaveTransactionsToCSV();
    RefreshAllUI();

    /* Clear Amount & Note Inputs */
    SetWindowText(g_hEditAmount, "");
    SetWindowText(g_hEditNote, "");
    SetFocus(g_hEditAmount);

    /* Check if streak increased */
    if (!prevLoggedToday && g_streak.logged_today) {
        char msg[256];
        sprintf(msg,
            "*** STREAK EXTENDED! ***\n\n"
            "Congratulations! You've logged your daily expense and kept your habit burning for %d consecutive days!\n\n"
            "Consistency in tracking builds financial freedom!",
            g_streak.current_streak
        );
        MessageBox(g_hwndMain, msg, "Streak Level Up!", MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBeep(MB_OK);
    }
}

/* Delete Selected Transaction */
void HandleDeleteTransaction(void) {
    int selIndex = ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
    LVITEM lvi;
    int realIndex;
    int i;

    if (selIndex == -1) {
        MessageBox(g_hwndMain, "Please select a transaction from the list to delete.", "Notice", MB_OK | MB_ICONINFORMATION);
        return;
    }

    memset(&lvi, 0, sizeof(lvi));
    lvi.mask = LVIF_PARAM;
    lvi.iItem = selIndex;
    ListView_GetItem(g_hListView, &lvi);
    realIndex = (int)lvi.lParam;

    if (realIndex >= 0 && realIndex < g_trans_count) {
        char msg[256];
        sprintf(msg, "Are you sure you want to delete transaction:\n\n\"%s\" ($%.2f)?",
            g_transactions[realIndex].note, g_transactions[realIndex].amount);
        if (MessageBox(g_hwndMain, msg, "Confirm Delete", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            for (i = realIndex; i < g_trans_count - 1; i++) {
                g_transactions[i] = g_transactions[i + 1];
            }
            g_trans_count--;
            SaveTransactionsToCSV();
            RefreshAllUI();
        }
    }
}

/* Export CSV Notice */
void HandleExportCSV(void) {
    char msg[256];
    SaveTransactionsToCSV();
    sprintf(msg, "Transactions successfully saved and exported to:\n\n%s\n\n(Total records: %d)", CSV_FILENAME, g_trans_count);
    MessageBox(g_hwndMain, msg, "Export Complete", MB_OK | MB_ICONINFORMATION);
}

/* Clear All */
void HandleClearAll(void) {
    if (MessageBox(g_hwndMain, "Are you sure you want to reset and delete ALL transactions?\nThis will also reset your streak history.",
        "Warning: Clear All", MB_YESNO | MB_ICONWARNING) == IDYES) {
        g_trans_count = 0;
        SaveTransactionsToCSV();
        RefreshAllUI();
    }
}

/* Initialize Controls */
void InitControls(HWND hwnd) {
    char todayStr[16];
    LVCOLUMN lvc;

    GetTodayString(todayStr);

    /* Fonts */
    g_hFontNormal = CreateFont(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

    g_hFontBold = CreateFont(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

    g_hFontTitle = CreateFont(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

    g_hFontLarge = CreateFont(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

    /* ================= Top Streak Banner ================= */
    CreateWindow("BUTTON", "Daily Tracking Streak & Habit Gamification",
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        15, 10, 935, 95, hwnd, NULL, NULL, NULL);

    g_hLblStreakTitle = CreateWindow("STATIC", "[STREAK: 0 DAYS]",
        WS_VISIBLE | WS_CHILD,
        30, 32, 400, 26, hwnd, NULL, NULL, NULL);
    SendMessage(g_hLblStreakTitle, WM_SETFONT, (WPARAM)g_hFontTitle, TRUE);

    g_hLblStreakStatus = CreateWindow("STATIC", "Status: Initializing...",
        WS_VISIBLE | WS_CHILD,
        30, 60, 520, 20, hwnd, NULL, NULL, NULL);
    SendMessage(g_hLblStreakStatus, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

    g_hLblStreakStats = CreateWindow("STATIC", "Best Record: 0 Days  |  Total Logged Days: 0",
        WS_VISIBLE | WS_CHILD | SS_RIGHT,
        560, 45, 370, 20, hwnd, NULL, NULL, NULL);
    SendMessage(g_hLblStreakStats, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    /* ================= Financial Summary Group ================= */
    CreateWindow("BUTTON", "Financial Overview",
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        15, 115, 935, 65, hwnd, NULL, NULL, NULL);

    g_hLblBalance = CreateWindow("STATIC", "Net Balance: $0.00",
        WS_VISIBLE | WS_CHILD,
        35, 140, 280, 25, hwnd, NULL, NULL, NULL);
    SendMessage(g_hLblBalance, WM_SETFONT, (WPARAM)g_hFontLarge, TRUE);

    g_hLblIncome = CreateWindow("STATIC", "Total Income: +$0.00",
        WS_VISIBLE | WS_CHILD,
        350, 140, 260, 25, hwnd, NULL, NULL, NULL);
    SendMessage(g_hLblIncome, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

    g_hLblExpense = CreateWindow("STATIC", "Total Expenses: -$0.00",
        WS_VISIBLE | WS_CHILD,
        640, 140, 280, 25, hwnd, NULL, NULL, NULL);
    SendMessage(g_hLblExpense, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

    /* ================= Left Form: Feed New Transaction ================= */
    CreateWindow("BUTTON", "Feed New Transaction",
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        15, 190, 335, 435, hwnd, NULL, NULL, NULL);

    /* Amount */
    CreateWindow("STATIC", "Amount ($):", WS_VISIBLE | WS_CHILD, 30, 220, 100, 18, hwnd, NULL, NULL, NULL);
    g_hEditAmount = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        30, 240, 305, 26, hwnd, (HMENU)ID_AMOUNT_EDIT, NULL, NULL);
    SendMessage(g_hEditAmount, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    /* Type */
    CreateWindow("STATIC", "Transaction Type:", WS_VISIBLE | WS_CHILD, 30, 275, 120, 18, hwnd, NULL, NULL, NULL);
    g_hComboType = CreateWindow("COMBOBOX", "",
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        30, 295, 305, 120, hwnd, (HMENU)ID_TYPE_COMBO, NULL, NULL);
    SendMessage(g_hComboType, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    SendMessage(g_hComboType, CB_ADDSTRING, 0, (LPARAM)"Expense (Outflow)");
    SendMessage(g_hComboType, CB_ADDSTRING, 0, (LPARAM)"Income (Inflow)");
    SendMessage(g_hComboType, CB_SETCURSEL, 0, 0);

    /* Category */
    CreateWindow("STATIC", "Category:", WS_VISIBLE | WS_CHILD, 30, 330, 100, 18, hwnd, NULL, NULL, NULL);
    g_hComboCategory = CreateWindow("COMBOBOX", "",
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        30, 350, 305, 180, hwnd, (HMENU)ID_CAT_COMBO, NULL, NULL);
    SendMessage(g_hComboCategory, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    UpdateCategoryCombo();

    /* Date */
    CreateWindow("STATIC", "Date (YYYY-MM-DD):", WS_VISIBLE | WS_CHILD, 30, 385, 150, 18, hwnd, NULL, NULL, NULL);
    g_hEditDate = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", todayStr,
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        30, 405, 305, 26, hwnd, (HMENU)ID_DATE_EDIT, NULL, NULL);
    SendMessage(g_hEditDate, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    /* Payment Method */
    CreateWindow("STATIC", "Payment Method:", WS_VISIBLE | WS_CHILD, 30, 440, 120, 18, hwnd, NULL, NULL, NULL);
    g_hComboPayment = CreateWindow("COMBOBOX", "",
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        30, 460, 305, 120, hwnd, (HMENU)ID_PAYMENT_COMBO, NULL, NULL);
    SendMessage(g_hComboPayment, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    SendMessage(g_hComboPayment, CB_ADDSTRING, 0, (LPARAM)"Credit/Debit Card");
    SendMessage(g_hComboPayment, CB_ADDSTRING, 0, (LPARAM)"Cash");
    SendMessage(g_hComboPayment, CB_ADDSTRING, 0, (LPARAM)"UPI / Online");
    SendMessage(g_hComboPayment, CB_ADDSTRING, 0, (LPARAM)"Bank Transfer");
    SendMessage(g_hComboPayment, CB_SETCURSEL, 0, 0);

    /* Note */
    CreateWindow("STATIC", "Description / Note:", WS_VISIBLE | WS_CHILD, 30, 495, 150, 18, hwnd, NULL, NULL, NULL);
    g_hEditNote = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        30, 515, 305, 26, hwnd, (HMENU)ID_NOTE_EDIT, NULL, NULL);
    SendMessage(g_hEditNote, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    /* Submit Button */
    g_hBtnAdd = CreateWindow("BUTTON", "Log Transaction & Boost Streak (+1)",
        WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        30, 560, 305, 42, hwnd, (HMENU)ID_ADD_BTN, NULL, NULL);
    SendMessage(g_hBtnAdd, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

    /* ================= Right Section: Transaction ListView ================= */
    CreateWindow("BUTTON", "Transaction Log & History",
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        365, 190, 585, 435, hwnd, NULL, NULL, NULL);

    g_hListView = CreateWindowEx(0, WC_LISTVIEW, "",
        WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SINGLESEL | WS_BORDER,
        380, 220, 555, 340, hwnd, (HMENU)ID_LISTVIEW, NULL, NULL);
    SendMessage(g_hListView, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    ListView_SetExtendedListViewStyle(g_hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    /* Add Columns */
    memset(&lvc, 0, sizeof(lvc));
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    lvc.iSubItem = 0; lvc.cx = 90;  lvc.pszText = "Date";
    ListView_InsertColumn(g_hListView, 0, &lvc);

    lvc.iSubItem = 1; lvc.cx = 70;  lvc.pszText = "Type";
    ListView_InsertColumn(g_hListView, 1, &lvc);

    lvc.iSubItem = 2; lvc.cx = 110; lvc.pszText = "Category";
    ListView_InsertColumn(g_hListView, 2, &lvc);

    lvc.iSubItem = 3; lvc.cx = 85;  lvc.pszText = "Amount";
    ListView_InsertColumn(g_hListView, 3, &lvc);

    lvc.iSubItem = 4; lvc.cx = 95;  lvc.pszText = "Payment";
    ListView_InsertColumn(g_hListView, 4, &lvc);

    lvc.iSubItem = 5; lvc.cx = 175; lvc.pszText = "Description";
    ListView_InsertColumn(g_hListView, 5, &lvc);

    /* ListView Action Buttons */
    g_hBtnDelete = CreateWindow("BUTTON", "Delete Selected",
        WS_VISIBLE | WS_CHILD,
        380, 575, 120, 32, hwnd, (HMENU)ID_DELETE_BTN, NULL, NULL);
    SendMessage(g_hBtnDelete, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    g_hBtnExport = CreateWindow("BUTTON", "Export CSV",
        WS_VISIBLE | WS_CHILD,
        510, 575, 110, 32, hwnd, (HMENU)ID_EXPORT_BTN, NULL, NULL);
    SendMessage(g_hBtnExport, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    g_hBtnDemo = CreateWindow("BUTTON", "Load Demo Data",
        WS_VISIBLE | WS_CHILD,
        630, 575, 130, 32, hwnd, (HMENU)ID_DEMO_BTN, NULL, NULL);
    SendMessage(g_hBtnDemo, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    g_hBtnClear = CreateWindow("BUTTON", "Clear All",
        WS_VISIBLE | WS_CHILD,
        770, 575, 100, 32, hwnd, (HMENU)ID_CLEAR_BTN, NULL, NULL);
    SendMessage(g_hBtnClear, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
}

/* Window Procedure */
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        InitControls(hwnd);
        LoadTransactionsFromCSV();
        RefreshAllUI();
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_TYPE_COMBO:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                UpdateCategoryCombo();
            }
            break;

        case ID_ADD_BTN:
            HandleAddTransaction();
            break;

        case ID_DELETE_BTN:
            HandleDeleteTransaction();
            break;

        case ID_EXPORT_BTN:
            HandleExportCSV();
            break;

        case ID_DEMO_BTN:
            LoadDemoData();
            RefreshAllUI();
            MessageBox(hwnd, "Demo transactions loaded! Consecutive 5-day streak activated.", "Demo Loaded", MB_OK | MB_ICONINFORMATION);
            break;

        case ID_CLEAR_BTN:
            HandleClearAll();
            break;
        }
        break;

    case WM_DESTROY:
        DeleteObject(g_hFontNormal);
        DeleteObject(g_hFontBold);
        DeleteObject(g_hFontTitle);
        DeleteObject(g_hFontLarge);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

/* WinMain Application Entry Point */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc;
    HWND hwnd;
    MSG msg;

    /* Initialize Common Controls for Modern ListView */
    InitCommonControls();

    /* Register Window Class */
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "ZenSpendGuiClass";
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    /* Create Main Window */
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        "ZenSpendGuiClass",
        "ZenSpend - Personal Expense & Daily Streak Tracker (C / Win32 GUI)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 980, 680,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    g_hwndMain = hwnd;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    /* Message Loop */
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
