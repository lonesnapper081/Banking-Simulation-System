/*
 * BANKING SIMULATION SYSTEM
 * C Programming Mini Project
 * Student: Shivansh Roy | Section: 5B
 * Submitted to: Kamaldeep Kaur
 *
 * Features:
 * - Create account with a 4-digit PIN and opening deposit
 * - Login with account number and PIN
 * - Lock account after 3 incorrect PIN attempts
 * - Deposit, withdraw, transfer, balance inquiry and change PIN
 * - View the last 10 dated transactions
 * - Binary file storage in accounts.dat
 *
 * Educational simulation only. Not connected to a real bank.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define DATA_FILE "accounts.dat"
#define MAX_HISTORY 10
#define NAME_LEN 50
#define TYPE_LEN 20
#define DATE_LEN 26

typedef struct {
    char date[DATE_LEN];
    char type[TYPE_LEN];
    double amount;
    double balanceAfter;
} Transaction;

typedef struct {
    long accountNo;
    char name[NAME_LEN];
    int pin;
    double balance;
    int txCount;
    int failedAttempts;
    int locked;
    Transaction history[MAX_HISTORY];
} Account;

static Account *accounts = NULL;
static size_t accountCount = 0;

static void readLine(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    if (fgets(buffer, (int)size, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }
    buffer[strcspn(buffer, "\n")] = '\0';
}

static long readLong(const char *prompt) {
    char input[100], *end;
    long value;
    for (;;) {
        readLine(prompt, input, sizeof input);
        errno = 0;
        value = strtol(input, &end, 10);
        if (errno == 0 && end != input && *end == '\0' && value > 0)
            return value;
        puts("Please enter a valid positive number.");
    }
}

static double readPositiveAmount(const char *prompt) {
    char input[100], *end;
    double value;
    for (;;) {
        readLine(prompt, input, sizeof input);
        errno = 0;
        value = strtod(input, &end);
        if (errno == 0 && end != input && *end == '\0' &&
            value > 0.0)
            return value;
        puts("Please enter a valid amount greater than zero.");
    }
}

static int readPin(const char *prompt) {
    char input[100], *end;
    long pin;
    for (;;) {
        readLine(prompt, input, sizeof input);
        errno = 0;
        pin = strtol(input, &end, 10);
        if (errno == 0 && end != input && *end == '\0' &&
            strlen(input) == 4 && pin >= 1000 && pin <= 9999)
            return (int)pin;
        puts("PIN must contain exactly 4 digits.");
    }
}

static int saveAccounts(void) {
    FILE *fp = fopen(DATA_FILE, "wb");
    if (fp == NULL) {
        puts("Error: could not save account data.");
        return 0;
    }
    if (accountCount > 0 &&
        fwrite(accounts, sizeof(Account), accountCount, fp) != accountCount) {
        puts("Error: account data could not be written completely.");
        fclose(fp);
        return 0;
    }
    fclose(fp);
    return 1;
}

static int loadAccounts(void) {
    FILE *fp = fopen(DATA_FILE, "rb");
    long fileSize;
    size_t count;
    if (fp == NULL)
        return 1;

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return 0;
    }
    fileSize = ftell(fp);
    if (fileSize < 0 || fileSize % (long)sizeof(Account) != 0) {
        puts("Warning: accounts.dat has an invalid format.");
        fclose(fp);
        return 0;
    }
    count = (size_t)(fileSize / (long)sizeof(Account));
    rewind(fp);

    if (count > 0) {
        accounts = (Account *)malloc(count * sizeof(Account));
        if (accounts == NULL) {
            puts("Error: not enough memory to load accounts.");
            fclose(fp);
            return 0;
        }
        if (fread(accounts, sizeof(Account), count, fp) != count) {
            puts("Error: could not read account data.");
            free(accounts);
            accounts = NULL;
            fclose(fp);
            return 0;
        }
    }
    accountCount = count;
    fclose(fp);
    return 1;
}

static int findAccountIndex(long accountNo) {
    size_t i;
    for (i = 0; i < accountCount; i++)
        if (accounts[i].accountNo == accountNo)
            return (int)i;
    return -1;
}

static long generateAccountNumber(void) {
    long highest = 1000;
    size_t i;
    for (i = 0; i < accountCount; i++)
        if (accounts[i].accountNo > highest)
            highest = accounts[i].accountNo;
    return highest + 1;
}

static void addTransaction(Account *account, const char *type, double amount) {
    time_t now = time(NULL);
    struct tm *timeInfo = localtime(&now);
    char dateText[DATE_LEN];

    if (timeInfo != NULL)
        strftime(dateText, sizeof dateText, "%d-%m-%Y %H:%M:%S", timeInfo);
    else
        snprintf(dateText, sizeof dateText, "Time unavailable");

    if (account->txCount < MAX_HISTORY) {
        int index = account->txCount;
        snprintf(account->history[index].date, DATE_LEN, "%s", dateText);
        snprintf(account->history[index].type, TYPE_LEN, "%s", type);
        account->history[index].amount = amount;
        account->history[index].balanceAfter = account->balance;
        account->txCount++;
    } else {
        int i;
        for (i = 1; i < MAX_HISTORY; i++)
            account->history[i - 1] = account->history[i];
        snprintf(account->history[MAX_HISTORY - 1].date, DATE_LEN, "%s", dateText);
        snprintf(account->history[MAX_HISTORY - 1].type, TYPE_LEN, "%s", type);
        account->history[MAX_HISTORY - 1].amount = amount;
        account->history[MAX_HISTORY - 1].balanceAfter = account->balance;
    }
}

static void createAccount(void) {
    Account newAccount;
    Account *temporary;
    char nameInput[NAME_LEN];

    puts("\n========== CREATE NEW ACCOUNT ==========");
    readLine("Enter full name: ", nameInput, sizeof nameInput);
    if (nameInput[0] == '\0') {
        puts("Name cannot be empty.");
        return;
    }
    memset(&newAccount, 0, sizeof newAccount);
    snprintf(newAccount.name, sizeof newAccount.name, "%s", nameInput);
    newAccount.pin = readPin("Set a 4-digit PIN: ");

    {
        char answer[20];
        double openingDeposit;
        readLine("Enter opening deposit (minimum Rs. 500): Rs. ", answer, sizeof answer);
        {
            char *end;
            errno = 0;
            openingDeposit = strtod(answer, &end);
            if (errno != 0 || end == answer || *end != '\0' || openingDeposit < 500.0) {
                puts("Opening deposit must be at least Rs. 500.");
                return;
            }
        }
        newAccount.balance = openingDeposit;
    }

    newAccount.accountNo = generateAccountNumber();
    newAccount.txCount = 0;
    newAccount.failedAttempts = 0;
    newAccount.locked = 0;
    addTransaction(&newAccount, "OPENING", newAccount.balance);

    temporary = (Account *)realloc(accounts, (accountCount + 1) * sizeof(Account));
    if (temporary == NULL) {
        puts("Error: could not allocate memory for the new account.");
        return;
    }
    accounts = temporary;
    accounts[accountCount++] = newAccount;

    if (saveAccounts()) {
        puts("\nAccount created successfully!");
        printf("Your account number is: %ld\n", newAccount.accountNo);
        puts("Please remember your account number and PIN.");
    }
}

static int login(void) {
    long accountNo = readLong("\nEnter account number: ");
    int index = findAccountIndex(accountNo);
    int enteredPin;

    if (index < 0) {
        puts("Account not found.");
        return -1;
    }
    if (accounts[index].locked) {
        puts("This account is locked after 3 incorrect PIN attempts.");
        return -1;
    }

    while (accounts[index].failedAttempts < 3) {
        enteredPin = readPin("Enter 4-digit PIN: ");
        if (enteredPin == accounts[index].pin) {
            accounts[index].failedAttempts = 0;
            saveAccounts();
            printf("\nWelcome, %s!\n", accounts[index].name);
            return index;
        }
        accounts[index].failedAttempts++;
        printf("Incorrect PIN. Attempts remaining: %d\n",
               3 - accounts[index].failedAttempts);
        if (accounts[index].failedAttempts >= 3) {
            accounts[index].locked = 1;
            puts("Account locked after 3 incorrect PIN attempts.");
            saveAccounts();
            return -1;
        }
        saveAccounts();
    }
    return -1;
}

static void deposit(int index) {
    double amount = readPositiveAmount("Enter amount to deposit: Rs. ");
    accounts[index].balance += amount;
    addTransaction(&accounts[index], "DEPOSIT", amount);
    if (saveAccounts())
        printf("Deposit successful. New balance: Rs. %.2f\n", accounts[index].balance);
}

static void withdraw(int index) {
    double amount = readPositiveAmount("Enter amount to withdraw: Rs. ");
    if (amount > accounts[index].balance) {
        puts("Insufficient balance. Withdrawal cancelled.");
        return;
    }
    accounts[index].balance -= amount;
    addTransaction(&accounts[index], "WITHDRAWAL", amount);
    if (saveAccounts())
        printf("Withdrawal successful. New balance: Rs. %.2f\n", accounts[index].balance);
}

static void transferMoney(int senderIndex) {
    long recipientNumber = readLong("Enter recipient account number: ");
    int recipientIndex = findAccountIndex(recipientNumber);
    double amount;

    if (recipientIndex < 0) {
        puts("Recipient account not found.");
        return;
    }
    if (recipientIndex == senderIndex) {
        puts("You cannot transfer money to your own account.");
        return;
    }
    if (accounts[recipientIndex].locked) {
        puts("Transfer rejected: recipient account is locked.");
        return;
    }
    printf("Recipient name: %s\n", accounts[recipientIndex].name);
    amount = readPositiveAmount("Enter amount to transfer: Rs. ");
    if (amount > accounts[senderIndex].balance) {
        puts("Insufficient balance. Transfer cancelled.");
        return;
    }

    accounts[senderIndex].balance -= amount;
    accounts[recipientIndex].balance += amount;
    addTransaction(&accounts[senderIndex], "TRANSFER OUT", amount);
    addTransaction(&accounts[recipientIndex], "TRANSFER IN", amount);
    if (saveAccounts()) {
        puts("Transfer successful.");
        printf("Your new balance: Rs. %.2f\n", accounts[senderIndex].balance);
    }
}

static void checkBalance(int index) {
    printf("\nAccount holder : %s\n", accounts[index].name);
    printf("Account number : %ld\n", accounts[index].accountNo);
    printf("Current balance: Rs. %.2f\n", accounts[index].balance);
}

static void changePin(int index) {
    int currentPin = readPin("Enter current PIN: ");
    int newPin;
    if (currentPin != accounts[index].pin) {
        puts("Incorrect current PIN. PIN was not changed.");
        return;
    }
    newPin = readPin("Enter new 4-digit PIN: ");
    if (newPin == currentPin) {
        puts("New PIN must be different from the current PIN.");
        return;
    }
    accounts[index].pin = newPin;
    if (saveAccounts())
        puts("PIN changed successfully.");
}

static void showHistory(int index) {
    int i;
    printf("\n========== LAST %d TRANSACTIONS ==========\n", MAX_HISTORY);
    if (accounts[index].txCount == 0) {
        puts("No transactions found.");
        return;
    }
    printf("%-20s %-15s %12s %14s\n",
           "DATE & TIME", "TYPE", "AMOUNT", "BALANCE");
    puts("--------------------------------------------------------------------------");
    for (i = 0; i < accounts[index].txCount; i++) {
        Transaction *t = &accounts[index].history[i];
        printf("%-20s %-15s Rs.%9.2f Rs.%11.2f\n",
               t->date, t->type, t->amount, t->balanceAfter);
    }
}

static void customerMenu(int index) {
    char input[20];
    int choice;
    do {
        puts("\n============= ACCOUNT MENU =============");
        puts("1. Deposit Money");
        puts("2. Withdraw Money");
        puts("3. Transfer Money");
        puts("4. Balance Inquiry");
        puts("5. Change PIN");
        puts("6. Transaction History");
        puts("0. Logout");
        readLine("Enter your choice: ", input, sizeof input);
        choice = atoi(input);
        switch (choice) {
            case 1: deposit(index); break;
            case 2: withdraw(index); break;
            case 3: transferMoney(index); break;
            case 4: checkBalance(index); break;
            case 5: changePin(index); break;
            case 6: showHistory(index); break;
            case 0: puts("Logged out successfully."); break;
            default: puts("Invalid choice. Please select 0 to 6.");
        }
    } while (choice != 0);
}

int main(void) {
    char input[20];
    int choice;

    if (!loadAccounts()) {
        puts("The program could not load account data. Exiting.");
        free(accounts);
        return 1;
    }

    puts("========================================");
    puts("        BANKING SIMULATION SYSTEM");
    puts("========================================");
    puts("Student: Shivansh Roy | Section: 5B");

    do {
        puts("\n=============== MAIN MENU ===============");
        puts("1. Create New Account");
        puts("2. Login");
        puts("0. Exit");
        readLine("Enter your choice: ", input, sizeof input);
        choice = atoi(input);
        switch (choice) {
            case 1: createAccount(); break;
            case 2: {
                int index = login();
                if (index >= 0)
                    customerMenu(index);
                break;
            }
            case 0:
                saveAccounts();
                puts("Thank you for using the Banking Simulation System.");
                break;
            default:
                puts("Invalid choice. Please select 0, 1 or 2.");
        }
    } while (choice != 0);

    free(accounts);
    return 0;
}
