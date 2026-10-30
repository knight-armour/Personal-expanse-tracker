@echo off
echo ========================================================
echo     Compiling ZenSpend C Applications (MinGW GCC)
echo ========================================================
echo.

echo [1/2] Compiling Windows Native GUI App (expense_tracker_gui.exe)...
gcc -O2 -mwindows -o expense_tracker_gui.exe expense_tracker_gui.c -lcomctl32 -lgdi32
if %ERRORLEVEL% EQU 0 (
    echo [OK] expense_tracker_gui.exe compiled successfully!
) else (
    echo [ERROR] Failed to compile expense_tracker_gui.exe
)

echo.
echo [2/2] Compiling Terminal CLI/TUI App (expense_tracker_cli.exe)...
gcc -O2 -o expense_tracker_cli.exe expense_tracker_cli.c
if %ERRORLEVEL% EQU 0 (
    echo [OK] expense_tracker_cli.exe compiled successfully!
) else (
    echo [ERROR] Failed to compile expense_tracker_cli.exe
)

echo.
echo ========================================================
echo Compilation complete!
echo - Double click expense_tracker_gui.exe for the Desktop GUI
echo - Double click expense_tracker_cli.exe for the Terminal Version
echo ========================================================
pause
