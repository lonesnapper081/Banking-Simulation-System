# Banking Simulation System

**Student:** Shivansh Roy  
**Section:** 5B  
**Subject:** Computer Programming (26CAH-101)  
**Submitted to:** Kamaldeep Kaur

## Project overview

A console-based banking simulation written in C. It demonstrates account management and banking operations using structures, functions, control statements, dynamic memory allocation, and file handling.

## Features

- Create an account with a four-digit PIN and an opening deposit of at least Rs. 500
- Log in using an account number and PIN
- Lock an account after three incorrect PIN attempts
- Deposit and withdraw money
- Transfer money to another account
- Check account balance
- Change PIN
- View the last ten dated transactions
- Save account records in the binary file `accounts.dat`

## Requirements

- GCC C compiler
- macOS, Linux, or Windows with GCC/MinGW

## Compile

```bash
gcc -std=c11 -Wall -Wextra banking_management.c -o banking
```

## Run

On macOS or Linux:

```bash
./banking
```

On Windows PowerShell:

```powershell
.\banking.exe
```

## Data files

The program creates `accounts.dat` in the current working directory. This file stores account records between runs. Keep it in the same folder when reopening the application.

## Important note

This is an educational simulation only. It is not connected to a real bank and must not be used for real customer or financial data.
