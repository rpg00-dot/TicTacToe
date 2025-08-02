@echo off
echo Компиляция приложения "Крестики-нолики"...
"D:\Compiler\MinGW-7.3.0\bin\gcc.exe" -o TicTacToe.exe Untitled-3.c -lgdi32 -luser32 -lkernel32 -mwindows -municode
if %ERRORLEVEL% EQU 0 (
    echo Компиляция успешна!
    echo Запуск приложения...
    start TicTacToe.exe
) else (
    echo Ошибка компиляции!
    pause
) 