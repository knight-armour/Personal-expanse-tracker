/**
 * ZenSpend — Personal Expense & Daily Streak Tracker
 * Interactive Terminal / Console TUI Version in Pure C
 * 
 * Features:
 * - Interactive Terminal GUI/TUI with ANSI styling & ASCII Fire Art
 * - User Input Form for adding expenses & income
 * - Daily Habit Streak Tracking Engine (Consecutive Day Tracking)
 * - Category spending breakdown & financial summaries
 * - CSV File Persistence (shared with GUI version)
 */

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_TRANSACTIONS 1000
#define MAX_NOTE_LEN 128
#define MAX_CAT_LEN 48
#define MAX_PAYMENT_LEN 32
#define MAX_DATE_LEN 16

/* ANSI Colors */
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_ORANGE  "\033[38;5;208m"

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

typedef struct {
    int current_streak;
    int longest_streak;
    int total_days;
    int logged_today;
    int logged_yesterday;
} StreakInfo;

static Transaction g_transactions[MAX_TRANSACTIONS];
static int g_trans_count = 0;
static StreakInfo g_streak;
static const char* CSV_FILENAME = "transactions.csv";

static const char* EXPENSE_CATEGORIES[] = {
    "Food & Dining", "Groceries", "Transportation", "Housing & Rent",
    "Bills & Utilities", "Entertainment", "Shopping", "Health & Medical",
    "Education", "Miscellaneous"
};
#define NUM_EXPENSE_CATS (sizeof(EXPENSE_CATEGORIES) / sizeof(EXPENSE_CATEGORIES[0]))

static const char* INCOME_CATEGORIES[] = {
    "Salary / Wages", "Freelance / Gig", "Investments", "Gifts / Grants", "Other Income"
};
#define NUM_INCOME_CATS (sizeof(INCOME_CATEGORIES) / sizeof(INCOME_CATEGORIES[0]))

/* Julian Day Calculation for Exact Date Differences */
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

    /* Current streak */
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

    /* Longest streak */
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

void LoadDemoData(void);

void LoadTransactionsFromCSV(void) {
    FILE* fp = fopen(CSV_FILENAME, "r");
    char line[512];
    if (!fp) {
        LoadDemoData();
        return;
    }

    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return;
    }

    g_trans_count = 0;
    while (fgets(line, sizeof(line), fp) && g_trans_count < MAX_TRANSACTIONS) {
        Transaction* t = &g_transactions[g_trans_count];
        char* token;
        char* p = line;

        p[strcspn(p, "\r\n")] = 0;

        token = strtok(p, ",");
        if (!token) continue;
        strncpy(t->id, token, sizeof(t->id) - 1);

        token = strtok(NULL, ",");
        if (!token) continue;
        strncpy(t->date, token, sizeof(t->date) - 1);

        token = strtok(NULL, ",");
        if (!token) continue;
        strncpy(t->type, token, sizeof(t->type) - 1);

        token = strtok(NULL, ",");
        if (!token) continue;
        if (token[0] == '"') token++;
        if (token[strlen(token) - 1] == '"') token[strlen(token) - 1] = 0;
        strncpy(t->category, token, sizeof(t->category) - 1);

        token = strtok(NULL, ",");
        if (!token) continue;
        t->amount = atof(token);

        token = strtok(NULL, ",");
        if (!token) continue;
        if (token[0] == '"') token++;
        if (token[strlen(token) - 1] == '"') token[strlen(token) - 1] = 0;
        strncpy(t->payment, token, sizeof(t->payment) - 1);

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
    strcpy(g_transactions[g_trans_count].date, d3);
    strcpy(g_transactions[g_trans_count].type, "Expense");
    strcpy(g_transactions[g_trans_count].category, "Bills & Utilities");
    g_transactions[g_trans_count].amount = 65.00;
    strcpy(g_transactions[g_trans_count].payment, "UPI / Online");
    strcpy(g_transactions[g_trans_count].note, "Fiber optical internet");
    g_trans_count++;

    strcpy(g_transactions[g_trans_count].id, "tx_6");
    strcpy(g_transactions[g_trans_count].date, d4);
    strcpy(g_transactions[g_trans_count].type, "Expense");
    strcpy(g_transactions[g_trans_count].category, "Shopping");
    g_transactions[g_trans_count].amount = 38.40;
    strcpy(g_transactions[g_trans_count].payment, "Credit/Debit Card");
    strcpy(g_transactions[g_trans_count].note, "Desk lamp & organizer");
    g_trans_count++;

    SaveTransactionsToCSV();
}

/* Print Header with Flame Art & Streak Info */
void PrintHeader(void) {
    double totalIncome = 0.0, totalExpense = 0.0, netBalance = 0.0;
    int i;

    ComputeStreak();

    for (i = 0; i < g_trans_count; i++) {
        if (strcmp(g_transactions[i].type, "Income") == 0) {
            totalIncome += g_transactions[i].amount;
        } else {
            totalExpense += g_transactions[i].amount;
        }
    }
    netBalance = totalIncome - totalExpense;

    printf("\n");
    printf(COLOR_YELLOW "     (  .      )" COLOR_RESET "\n");
    printf(COLOR_ORANGE "      )           (     " COLOR_BOLD COLOR_WHITE "ZENSPEND - PERSONAL EXPENSE & STREAK TRACKER" COLOR_RESET "\n");
    printf(COLOR_RED    "     (  .     '    )    " COLOR_CYAN "Habit-Driven Personal Finance Engine in Pure C" COLOR_RESET "\n");
    printf(COLOR_YELLOW "       '        )       " COLOR_RESET "\n");
    printf("=================================================================================\n");

    /* Streak Status Display */
    printf(COLOR_BOLD "🔥 DAILY STREAK: " COLOR_RESET);
    if (g_streak.current_streak > 0) {
        printf(COLOR_ORANGE COLOR_BOLD "%d CONSECUTIVE DAYS! " COLOR_RESET, g_streak.current_streak);
    } else {
        printf(COLOR_WHITE "0 Days (Ready to ignite!) " COLOR_RESET);
    }

    if (g_streak.logged_today) {
        printf(COLOR_GREEN "[STATUS: ✅ Logged Today - Streak Saved!]" COLOR_RESET "\n");
    } else if (g_streak.logged_yesterday && g_streak.current_streak > 0) {
        printf(COLOR_YELLOW "[STATUS: ⏳ Pending Today - Log to keep streak alive!]" COLOR_RESET "\n");
    } else {
        printf(COLOR_RED "[STATUS: ⚠️ Inactive - Log an entry today to ignite streak!]" COLOR_RESET "\n");
    }

    printf("Best Streak Record: " COLOR_CYAN "%d Days" COLOR_RESET "  |  Total Logged Days: " COLOR_CYAN "%d" COLOR_RESET "  |  Total Transactions: " COLOR_CYAN "%d" COLOR_RESET "\n",
        g_streak.longest_streak, g_streak.total_days, g_trans_count);

    printf("---------------------------------------------------------------------------------\n");
    printf("Net Balance: ");
    if (netBalance >= 0) {
        printf(COLOR_GREEN COLOR_BOLD "$%.2f" COLOR_RESET, netBalance);
    } else {
        printf(COLOR_RED COLOR_BOLD "-$%.2f" COLOR_RESET, -netBalance);
    }
    printf("   |   Income: " COLOR_GREEN "+$%.2f" COLOR_RESET "   |   Expenses: " COLOR_RED "-$%.2f" COLOR_RESET "\n",
        totalIncome, totalExpense);
    printf("=================================================================================\n");
}

/* User Input: Add Transaction */
void AddTransactionUI(void) {
    char typeChoice;
    char typeStr[16];
    char catStr[MAX_CAT_LEN];
    char dateStr[MAX_DATE_LEN];
    char payStr[MAX_PAYMENT_LEN];
    char noteStr[MAX_NOTE_LEN];
    char todayStr[MAX_DATE_LEN];
    char inputBuf[128];
    double amount = 0.0;
    int catIdx = 0, payIdx = 0;
    int i;
    int prevStreak, prevLoggedToday;

    GetTodayString(todayStr);

    printf("\n" COLOR_BOLD COLOR_CYAN ">>> FEED NEW TRANSACTION <<<" COLOR_RESET "\n");

    /* 1. Type */
    printf("1. Expense (Outflow)\n");
    printf("2. Income (Inflow)\n");
    printf("Choose Type (1/2, default 1): ");
    if (fgets(inputBuf, sizeof(inputBuf), stdin)) {
        if (inputBuf[0] == '2') {
            strcpy(typeStr, "Income");
        } else {
            strcpy(typeStr, "Expense");
        }
    } else {
        strcpy(typeStr, "Expense");
    }

    /* 2. Amount */
    while (1) {
        printf("Enter Amount ($): ");
        if (fgets(inputBuf, sizeof(inputBuf), stdin)) {
            amount = atof(inputBuf);
            if (amount > 0.0) break;
        }
        printf(COLOR_RED "Invalid amount. Please enter a value greater than 0.\n" COLOR_RESET);
    }

    /* 3. Category */
    printf("\nSelect Category:\n");
    if (strcmp(typeStr, "Income") == 0) {
        for (i = 0; i < NUM_INCOME_CATS; i++) {
            printf("  [%d] %s\n", i + 1, INCOME_CATEGORIES[i]);
        }
        printf("Choose Category (1-%d): ", (int)NUM_INCOME_CATS);
        if (fgets(inputBuf, sizeof(inputBuf), stdin)) {
            catIdx = atoi(inputBuf) - 1;
        }
        if (catIdx < 0 || catIdx >= NUM_INCOME_CATS) catIdx = 0;
        strcpy(catStr, INCOME_CATEGORIES[catIdx]);
    } else {
        for (i = 0; i < NUM_EXPENSE_CATS; i++) {
            printf("  [%d] %s\n", i + 1, EXPENSE_CATEGORIES[i]);
        }
        printf("Choose Category (1-%d): ", (int)NUM_EXPENSE_CATS);
        if (fgets(inputBuf, sizeof(inputBuf), stdin)) {
            catIdx = atoi(inputBuf) - 1;
        }
        if (catIdx < 0 || catIdx >= NUM_EXPENSE_CATS) catIdx = 0;
        strcpy(catStr, EXPENSE_CATEGORIES[catIdx]);
    }

    /* 4. Date */
    printf("Enter Date [YYYY-MM-DD] (Press Enter for today: %s): ", todayStr);
    if (fgets(inputBuf, sizeof(inputBuf), stdin)) {
        inputBuf[strcspn(inputBuf, "\r\n")] = 0;
        if (strlen(inputBuf) >= 8) {
            strncpy(dateStr, inputBuf, sizeof(dateStr) - 1);
        } else {
            strcpy(dateStr, todayStr);
        }
    } else {
        strcpy(dateStr, todayStr);
    }

    /* 5. Payment Method */
    printf("\nPayment Method:\n");
    printf("  [1] Credit/Debit Card\n");
    printf("  [2] Cash\n");
    printf("  [3] UPI / Online Transfer\n");
    printf("  [4] Bank Account\n");
    printf("Choose Payment (1-4, default 1): ");
    if (fgets(inputBuf, sizeof(inputBuf), stdin)) {
        payIdx = atoi(inputBuf);
    }
    switch (payIdx) {
        case 2: strcpy(payStr, "Cash"); break;
        case 3: strcpy(payStr, "UPI / Online"); break;
        case 4: strcpy(payStr, "Bank Transfer"); break;
        default: strcpy(payStr, "Credit/Debit Card"); break;
    }

    /* 6. Note */
    printf("Enter Description / Note: ");
    if (fgets(inputBuf, sizeof(inputBuf), stdin)) {
        inputBuf[strcspn(inputBuf, "\r\n")] = 0;
        strncpy(noteStr, inputBuf, sizeof(noteStr) - 1);
    } else {
        strcpy(noteStr, "General expense");
    }

    if (g_trans_count >= MAX_TRANSACTIONS) {
        printf(COLOR_RED "Error: Max transactions reached!\n" COLOR_RESET);
        return;
    }

    prevStreak = g_streak.current_streak;
    prevLoggedToday = g_streak.logged_today;

    /* Save to array */
    sprintf(g_transactions[g_trans_count].id, "tx_%ld", (long)time(NULL));
    strncpy(g_transactions[g_trans_count].date, dateStr, sizeof(g_transactions[g_trans_count].date) - 1);
    strcpy(g_transactions[g_trans_count].type, typeStr);
    strncpy(g_transactions[g_trans_count].category, catStr, sizeof(g_transactions[g_trans_count].category) - 1);
    g_transactions[g_trans_count].amount = amount;
    strncpy(g_transactions[g_trans_count].payment, payStr, sizeof(g_transactions[g_trans_count].payment) - 1);
    strncpy(g_transactions[g_trans_count].note, noteStr, sizeof(g_transactions[g_trans_count].note) - 1);
    g_trans_count++;

    SaveTransactionsToCSV();
    ComputeStreak();

    printf("\n" COLOR_GREEN "✔ Transaction logged successfully!" COLOR_RESET "\n");

    /* Streak Celebration Feedback */
    if (!prevLoggedToday && g_streak.logged_today) {
        printf("\n==================================================================\n");
        printf(COLOR_ORANGE COLOR_BOLD "🔥🔥🔥 STREAK EXTENDED! YOU'RE ON A %d-DAY STREAK! 🔥🔥🔥\n" COLOR_RESET, g_streak.current_streak);
        printf(COLOR_YELLOW "Congratulations! You logged today's expense and kept your habit alive!\n" COLOR_RESET);
        printf("==================================================================\n");
    }
}

/* View Transactions Table */
void ViewTransactionsUI(void) {
    int i;
    printf("\n" COLOR_BOLD COLOR_WHITE "=== TRANSACTION HISTORY (%d RECORDS) ===" COLOR_RESET "\n", g_trans_count);
    printf("%-4s | %-10s | %-8s | %-18s | %-10s | %-14s | %-20s\n",
        "#", "Date", "Type", "Category", "Amount", "Payment", "Note");
    printf("---------------------------------------------------------------------------------------------\n");

    for (i = g_trans_count - 1; i >= 0; i--) {
        char amtStr[32];
        if (strcmp(g_transactions[i].type, "Income") == 0) {
            sprintf(amtStr, "+$%.2f", g_transactions[i].amount);
            printf("%-4d | %-10s | " COLOR_GREEN "%-8s" COLOR_RESET " | %-18s | " COLOR_GREEN "%-10s" COLOR_RESET " | %-14s | %-20s\n",
                g_trans_count - i,
                g_transactions[i].date,
                g_transactions[i].type,
                g_transactions[i].category,
                amtStr,
                g_transactions[i].payment,
                g_transactions[i].note
            );
        } else {
            sprintf(amtStr, "-$%.2f", g_transactions[i].amount);
            printf("%-4d | %-10s | " COLOR_RED "%-8s" COLOR_RESET " | %-18s | " COLOR_RED "%-10s" COLOR_RESET " | %-14s | %-20s\n",
                g_trans_count - i,
                g_transactions[i].date,
                g_transactions[i].type,
                g_transactions[i].category,
                amtStr,
                g_transactions[i].payment,
                g_transactions[i].note
            );
        }
    }
}

/* Category Breakdown */
void ViewCategoryBreakdownUI(void) {
    double catTotals[32] = {0};
    int i, j;
    double totalExpense = 0.0;

    for (i = 0; i < g_trans_count; i++) {
        if (strcmp(g_transactions[i].type, "Expense") == 0) {
            totalExpense += g_transactions[i].amount;
            for (j = 0; j < NUM_EXPENSE_CATS; j++) {
                if (strcmp(g_transactions[i].category, EXPENSE_CATEGORIES[j]) == 0) {
                    catTotals[j] += g_transactions[i].amount;
                    break;
                }
            }
        }
    }

    printf("\n" COLOR_BOLD COLOR_CYAN "=== EXPENSES BY CATEGORY ===" COLOR_RESET "\n");
    if (totalExpense <= 0.0) {
        printf("No expenses recorded yet.\n");
        return;
    }

    for (j = 0; j < NUM_EXPENSE_CATS; j++) {
        if (catTotals[j] > 0.0) {
            double pct = (catTotals[j] / totalExpense) * 100.0;
            int barLen = (int)(pct / 2.5);
            int k;

            printf("%-18s | $%-8.2f (%5.1f%%) | ", EXPENSE_CATEGORIES[j], catTotals[j], pct);
            printf(COLOR_ORANGE);
            for (k = 0; k < barLen; k++) printf("█");
            printf(COLOR_RESET "\n");
        }
    }
}

/* Delete Transaction */
void DeleteTransactionUI(void) {
    char inputBuf[64];
    int idx, realIdx, i;

    ViewTransactionsUI();
    printf("\nEnter transaction # to delete (0 to cancel): ");
    if (fgets(inputBuf, sizeof(inputBuf), stdin)) {
        idx = atoi(inputBuf);
        if (idx <= 0 || idx > g_trans_count) {
            printf("Cancelled.\n");
            return;
        }

        realIdx = g_trans_count - idx;
        printf("Are you sure you want to delete \"%s\" ($%.2f)? (y/n): ",
            g_transactions[realIdx].note, g_transactions[realIdx].amount);
        if (fgets(inputBuf, sizeof(inputBuf), stdin) && (inputBuf[0] == 'y' || inputBuf[0] == 'Y')) {
            for (i = realIdx; i < g_trans_count - 1; i++) {
                g_transactions[i] = g_transactions[i + 1];
            }
            g_trans_count--;
            SaveTransactionsToCSV();
            ComputeStreak();
            printf(COLOR_GREEN "Transaction deleted.\n" COLOR_RESET);
        } else {
            printf("Cancelled.\n");
        }
    }
}

/* Main Loop */
int main(void) {
    char choiceBuf[32];
    int choice = 0;

    LoadTransactionsFromCSV();

    while (1) {
        PrintHeader();
        printf("\n" COLOR_BOLD "MAIN MENU:" COLOR_RESET "\n");
        printf("  [1] 📝 Feed New Transaction\n");
        printf("  [2] 📑 View All Transactions\n");
        printf("  [3] 📊 View Category Spending Breakdown\n");
        printf("  [4] 🗑️ Delete a Transaction\n");
        printf("  [5] ✨ Load Demo Data (5-Day Streak)\n");
        printf("  [6] 💾 Force Save to CSV\n");
        printf("  [7] ❌ Exit\n");
        printf("\nEnter Choice (1-7): ");

        if (fgets(choiceBuf, sizeof(choiceBuf), stdin)) {
            choice = atoi(choiceBuf);
        } else {
            break;
        }

        switch (choice) {
            case 1: AddTransactionUI(); break;
            case 2: ViewTransactionsUI(); break;
            case 3: ViewCategoryBreakdownUI(); break;
            case 4: DeleteTransactionUI(); break;
            case 5:
                LoadDemoData();
                printf(COLOR_GREEN "✔ Demo data reloaded! 5-day active streak restored.\n" COLOR_RESET);
                break;
            case 6:
                SaveTransactionsToCSV();
                printf(COLOR_GREEN "✔ Saved to %s (%d records).\n" COLOR_RESET, CSV_FILENAME, g_trans_count);
                break;
            case 7:
                printf("\nKeep your streak alive tomorrow! Goodbye!\n");
                return 0;
            default:
                printf(COLOR_RED "Invalid selection.\n" COLOR_RESET);
                break;
        }

        printf("\nPress Enter to continue...");
        getchar();
    }

    return 0;
}
