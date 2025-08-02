// Программа "Крестики-нолики" с улучшенным графическим интерфейсом на WinAPI
// Поддержка кириллицы и оконное приложение без консоли

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <tchar.h>
#include <stdbool.h>
#include <math.h>

#define CELL_SIZE 120
#define GRID_SIZE 3
#define WIN_WIDTH (CELL_SIZE * GRID_SIZE)
#define WIN_HEIGHT (CELL_SIZE * GRID_SIZE + 60)
#define ANIMATION_STEPS 20
#define PI 3.14159265359

typedef enum { EMPTY, X, O } Cell;

typedef struct {
    Cell type;
    float animation;
    bool animating;
    int animDirection;
} CellData;

typedef struct {
    float r, g, b;
} Color;

CellData board[GRID_SIZE][GRID_SIZE];
bool xTurn = true;
bool gameOver = false;
int animationTimer = 0;
int hoverX = -1, hoverY = -1;
bool showHover = false;
bool needsRedraw = false;

// Прототипы функций
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
bool CheckWin(Cell who);
bool CheckDraw();
void ResetGame();
void ShowEndMessage(HWND hwnd, const TCHAR* msg);
void DrawGradientBackground(HDC hdc, RECT rect);
void DrawCell(HDC hdc, int x, int y, CellData cell, bool isHover);
void DrawX(HDC hdc, int x, int y, float animation);
void DrawO(HDC hdc, int x, int y, float animation);
void DrawShadow(HDC hdc, int x, int y, int width, int height);
Color LerpColor(Color a, Color b, float t);
void SetTimerForAnimation(HWND hwnd);
void DrawBoardDoubleBuffered(HWND hwnd, HDC hdc);

// Цветовая схема
Color bgColor1 = {0.1f, 0.1f, 0.2f};
Color bgColor2 = {0.2f, 0.2f, 0.4f};
Color cellColor = {0.15f, 0.15f, 0.25f};
Color hoverColor = {0.25f, 0.25f, 0.35f};
Color xColor = {0.9f, 0.3f, 0.3f};
Color oColor = {0.3f, 0.7f, 0.9f};
Color gridColor = {0.4f, 0.4f, 0.6f};

void DrawGradientBackground(HDC hdc, RECT rect) {
    int height = rect.bottom - rect.top;
    for (int y = 0; y < height; y++) {
        float t = (float)y / height;
        Color color = LerpColor(bgColor1, bgColor2, t);
        HBRUSH brush = CreateSolidBrush(RGB(color.r * 255, color.g * 255, color.b * 255));
        RECT lineRect = {rect.left, rect.top + y, rect.right, rect.top + y + 1};
        FillRect(hdc, &lineRect, brush);
        DeleteObject(brush);
    }
}

Color LerpColor(Color a, Color b, float t) {
    Color result;
    result.r = a.r + (b.r - a.r) * t;
    result.g = a.g + (b.g - a.g) * t;
    result.b = a.b + (b.b - a.b) * t;
    return result;
}

void DrawShadow(HDC hdc, int x, int y, int width, int height) {
    HBRUSH shadowBrush = CreateSolidBrush(RGB(0, 0, 0));
    for (int i = 0; i < 5; i++) {
        RECT shadowRect = {x + i, y + i, x + width + i, y + height + i};
        SetBkMode(hdc, TRANSPARENT);
        FillRect(hdc, &shadowRect, shadowBrush);
    }
    DeleteObject(shadowBrush);
}

void DrawX(HDC hdc, int x, int y, float animation) {
    int centerX = x + CELL_SIZE / 2;
    int centerY = y + CELL_SIZE / 2;
    int size = (int)(30 * animation);
    
    HPEN pen = CreatePen(PS_SOLID, 4, RGB(xColor.r * 255, xColor.g * 255, xColor.b * 255));
    HPEN oldPen = SelectObject(hdc, pen);
    
    MoveToEx(hdc, centerX - size, centerY - size, NULL);
    LineTo(hdc, centerX + size, centerY + size);
    MoveToEx(hdc, centerX + size, centerY - size, NULL);
    LineTo(hdc, centerX - size, centerY + size);
    
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

void DrawO(HDC hdc, int x, int y, float animation) {
    int centerX = x + CELL_SIZE / 2;
    int centerY = y + CELL_SIZE / 2;
    int radius = (int)(25 * animation);
    
    HPEN pen = CreatePen(PS_SOLID, 4, RGB(oColor.r * 255, oColor.g * 255, oColor.b * 255));
    HPEN oldPen = SelectObject(hdc, pen);
    HBRUSH oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    
    Ellipse(hdc, centerX - radius, centerY - radius, centerX + radius, centerY + radius);
    
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
}

void DrawCell(HDC hdc, int x, int y, CellData cell, bool isHover) {
    int cellX = x * CELL_SIZE;
    int cellY = y * CELL_SIZE;
    
    // Рисуем тень
    DrawShadow(hdc, cellX + 2, cellY + 2, CELL_SIZE - 4, CELL_SIZE - 4);
    
    // Рисуем фон ячейки
    Color cellBgColor = isHover ? hoverColor : cellColor;
    HBRUSH cellBrush = CreateSolidBrush(RGB(cellBgColor.r * 255, cellBgColor.g * 255, cellBgColor.b * 255));
    RECT cellRect = {cellX + 5, cellY + 5, cellX + CELL_SIZE - 5, cellY + CELL_SIZE - 5};
    FillRect(hdc, &cellRect, cellBrush);
    DeleteObject(cellBrush);
    
    // Рисуем содержимое ячейки
    if (cell.type == X) {
        DrawX(hdc, cellX, cellY, cell.animation);
    } else if (cell.type == O) {
        DrawO(hdc, cellX, cellY, cell.animation);
    }
}

void DrawBoard(HDC hdc) {
    RECT clientRect;
    GetClientRect(GetActiveWindow(), &clientRect);
    
    // Рисуем градиентный фон
    DrawGradientBackground(hdc, clientRect);
    
    // Рисуем ячейки
    for (int y = 0; y < GRID_SIZE; ++y) {
        for (int x = 0; x < GRID_SIZE; ++x) {
            bool isHover = (x == hoverX && y == hoverY && showHover);
            DrawCell(hdc, x, y, board[y][x], isHover);
        }
    }
    
    // Рисуем сетку
    HPEN gridPen = CreatePen(PS_SOLID, 2, RGB(gridColor.r * 255, gridColor.g * 255, gridColor.b * 255));
    HPEN oldPen = SelectObject(hdc, gridPen);
    
    for (int i = 1; i < GRID_SIZE; ++i) {
        MoveToEx(hdc, i * CELL_SIZE, 0, NULL);
        LineTo(hdc, i * CELL_SIZE, WIN_HEIGHT - 60);
        MoveToEx(hdc, 0, i * CELL_SIZE, NULL);
        LineTo(hdc, WIN_WIDTH, i * CELL_SIZE);
    }
    
    SelectObject(hdc, oldPen);
    DeleteObject(gridPen);
    
    // Рисуем статус игры с поддержкой кириллицы
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    
    // Используем шрифт с поддержкой кириллицы
    HFONT hFont = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                             RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT oldFont = SelectObject(hdc, hFont);
    
    TCHAR statusText[100];
    if (gameOver) {
        _tcscpy(statusText, L"Кликните для новой игры");
    } else {
        _tcscpy(statusText, xTurn ? L"Ход крестиков" : L"Ход ноликов");
    }
    
    RECT textRect = {0, WIN_HEIGHT - 50, WIN_WIDTH, WIN_HEIGHT};
    DrawTextW(hdc, statusText, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    SelectObject(hdc, oldFont);
    DeleteObject(hFont);
}

void DrawBoardDoubleBuffered(HWND hwnd, HDC hdc) {
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    
    // Создаем совместимый DC для двойной буферизации
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, clientRect.right - clientRect.left, 
                                              clientRect.bottom - clientRect.top);
    HBITMAP oldBitmap = SelectObject(memDC, memBitmap);
    
    // Рисуем в память
    DrawBoard(memDC);
    
    // Копируем на экран
    BitBlt(hdc, 0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top,
           memDC, 0, 0, SRCCOPY);
    
    // Очищаем ресурсы
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

bool CheckWin(Cell who) {
    // Проверка строк и столбцов
    for (int i = 0; i < GRID_SIZE; ++i) {
        if ((board[i][0].type == who && board[i][1].type == who && board[i][2].type == who) ||
            (board[0][i].type == who && board[1][i].type == who && board[2][i].type == who))
            return true;
    }
    // Проверка диагоналей
    if ((board[0][0].type == who && board[1][1].type == who && board[2][2].type == who) ||
        (board[0][2].type == who && board[1][1].type == who && board[2][0].type == who))
        return true;
    return false;
}

bool CheckDraw() {
    for (int y = 0; y < GRID_SIZE; ++y)
        for (int x = 0; x < GRID_SIZE; ++x)
            if (board[y][x].type == EMPTY)
                return false;
    return true;
}

void ResetGame() {
    for (int y = 0; y < GRID_SIZE; ++y) {
        for (int x = 0; x < GRID_SIZE; ++x) {
            board[y][x].type = EMPTY;
            board[y][x].animation = 0.0f;
            board[y][x].animating = false;
            board[y][x].animDirection = 1;
        }
    }
    xTurn = true;
    gameOver = false;
    hoverX = -1;
    hoverY = -1;
    showHover = false;
    needsRedraw = true;
}

void SetTimerForAnimation(HWND hwnd) {
    SetTimer(hwnd, 1, 16, NULL); // ~60 FPS
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    const TCHAR szClassName[] = L"TicTacToeWindow";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = szClassName;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(szClassName, L"Крестики-нолики - Улучшенная версия",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, WIN_WIDTH + 16, WIN_HEIGHT + 39,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    ResetGame();
    SetTimerForAnimation(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

void ShowEndMessage(HWND hwnd, const TCHAR* msg) {
    MessageBoxW(hwnd, msg, L"Игра окончена", MB_OK | MB_ICONINFORMATION);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TIMER:
        {
            bool animationNeedsRedraw = false;
            
            // Обновляем анимации
            for (int y = 0; y < GRID_SIZE; ++y) {
                for (int x = 0; x < GRID_SIZE; ++x) {
                    if (board[y][x].animating) {
                        board[y][x].animation += 0.1f * board[y][x].animDirection;
                        if (board[y][x].animation >= 1.0f) {
                            board[y][x].animation = 1.0f;
                            board[y][x].animating = false;
                        } else if (board[y][x].animation <= 0.0f) {
                            board[y][x].animation = 0.0f;
                            board[y][x].animating = false;
                        }
                        animationNeedsRedraw = true;
                    }
                }
            }
            
            if (animationNeedsRedraw || needsRedraw) {
                needsRedraw = false;
                InvalidateRect(hwnd, NULL, FALSE);
                UpdateWindow(hwnd);
            }
        }
        break;
        
    case WM_MOUSEMOVE:
        {
            int x = LOWORD(lParam) / CELL_SIZE;
            int y = HIWORD(lParam) / CELL_SIZE;
            
            if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE && !gameOver) {
                if (hoverX != x || hoverY != y) {
                    hoverX = x;
                    hoverY = y;
                    showHover = true;
                    needsRedraw = true;
                }
            } else {
                if (showHover) {
                    showHover = false;
                    needsRedraw = true;
                }
            }
        }
        break;
        
    case WM_LBUTTONDOWN:
        if (gameOver) {
            ResetGame();
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
            break;
        }
        {
            int x = LOWORD(lParam) / CELL_SIZE;
            int y = HIWORD(lParam) / CELL_SIZE;
            if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE && 
                board[y][x].type == EMPTY && !board[y][x].animating) {
                
                board[y][x].type = xTurn ? X : O;
                board[y][x].animation = 0.0f;
                board[y][x].animating = true;
                board[y][x].animDirection = 1;
                
                if (CheckWin(board[y][x].type)) {
                    gameOver = true;
                    needsRedraw = true;
                    ShowEndMessage(hwnd, xTurn ? L"Победили крестики! 🎉" : L"Победили нолики! 🎉");
                } else if (CheckDraw()) {
                    gameOver = true;
                    needsRedraw = true;
                    ShowEndMessage(hwnd, L"Ничья! 🤝");
                } else {
                    xTurn = !xTurn;
                    needsRedraw = true;
                }
            }
        }
        break;
        
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            DrawBoardDoubleBuffered(hwnd, hdc);
            EndPaint(hwnd, &ps);
        }
        break;
        
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        break;
        
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}
