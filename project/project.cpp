#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <string>

std::atomic<bool> running(true);
std::atomic<bool> countdownRunning(false);
std::atomic<bool> isAlwaysOnTop(false);
HWND timeLabel, timeLeftLabel, debugLabel, hwndMain;

void ShowCurrentTime(HWND hwnd) {
    auto update = [hwnd]() {
        while (running) {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            tm localTime;
            localtime_s(&localTime, &time);
            char buffer[100];
            strftime(buffer, sizeof(buffer), "%H:%M:%S", &localTime);

            std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
            std::string timeStr = std::string(buffer) + "." + std::to_string(ms.count());

            // Convert std::string to std::wstring
            std::wstring timeWStr(timeStr.begin(), timeStr.end());
            SetWindowTextW(timeLabel, timeWStr.c_str());

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        };
    std::thread(update).detach();
}

void SimulateMouseClick() {
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));

    ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));

    // Add timestamp here
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    tm localTime;
    localtime_s(&localTime, &time);
    char buffer[100];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &localTime);

    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::string timestamp(buffer);
    timestamp += "." + std::to_string(ms.count());

    // Add "Click at: " text in front of the timestamp
    std::string clickText = "Click at: " + timestamp;
    std::wstring clickTextWStr(clickText.begin(), clickText.end());
    SetWindowTextW(debugLabel, clickTextWStr.c_str());
}

void SetTargetTime(int hour, int minute, int second, int millisecond, HWND hwnd) {
    auto now = std::chrono::system_clock::now();
    std::time_t current_time = std::chrono::system_clock::to_time_t(now);
    tm local_time;
    localtime_s(&local_time, &current_time);

    local_time.tm_hour = hour;
    local_time.tm_min = minute;
    local_time.tm_sec = second;

    auto target_time = std::chrono::system_clock::from_time_t(std::mktime(&local_time));
    target_time += std::chrono::milliseconds(millisecond);

    auto updateCountdown = [target_time]() {
        while (countdownRunning) {
            auto now = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(target_time - now);
            if (duration.count() <= 0) {
                countdownRunning = false;
                SimulateMouseClick();  // Simulate mouse click
                SetWindowTextW(timeLeftLabel, L"Time's up!");
                break;
            }

            int hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();
            duration -= std::chrono::hours(hours);
            int minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
            duration -= std::chrono::minutes(minutes);
            int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
            duration -= std::chrono::seconds(seconds);
            int milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

            wchar_t buffer[100];
            swprintf(buffer, sizeof(buffer) / sizeof(wchar_t), L"%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);
            SetWindowTextW(timeLeftLabel, buffer);

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        };
    std::thread(updateCountdown).detach();
}

void StartClick(HWND hwnd) {
    char hourBuffer[3], minuteBuffer[3], secondBuffer[3], millisecondBuffer[4];

    GetWindowTextA(GetDlgItem(hwnd, 1001), hourBuffer, 3);
    GetWindowTextA(GetDlgItem(hwnd, 1002), minuteBuffer, 3);
    GetWindowTextA(GetDlgItem(hwnd, 1003), secondBuffer, 3);
    GetWindowTextA(GetDlgItem(hwnd, 1004), millisecondBuffer, 4);

    int hour = atoi(hourBuffer);
    int minute = atoi(minuteBuffer);
    int second = atoi(secondBuffer);
    int millisecond = atoi(millisecondBuffer);

    countdownRunning = true;
    SetTargetTime(hour, minute, second, millisecond, hwnd);
}

void SetCurrentTime(HWND hwnd) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    tm localTime;
    localtime_s(&localTime, &time);

    wchar_t hourBuffer[3], minuteBuffer[3], secondBuffer[3], millisecondBuffer[4];
    swprintf(hourBuffer, 3, L"%02d", localTime.tm_hour);
    swprintf(minuteBuffer, 3, L"%02d", localTime.tm_min);
    swprintf(secondBuffer, 3, L"%02d", localTime.tm_sec);

    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    swprintf(millisecondBuffer, 4, L"%03d", (int)ms.count());

    SetWindowTextW(GetDlgItem(hwnd, 1001), hourBuffer);
    SetWindowTextW(GetDlgItem(hwnd, 1002), minuteBuffer);
    SetWindowTextW(GetDlgItem(hwnd, 1003), secondBuffer);
    SetWindowTextW(GetDlgItem(hwnd, 1004), millisecondBuffer);
}

void ToggleAlwaysOnTop(HWND hwnd) {
    isAlwaysOnTop = !isAlwaysOnTop;
    SetWindowPos(hwnd, isAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    InvalidateRect(hwnd, NULL, TRUE); // Trigger a repaint to update the dot color
}

void HighlightInputOnFocus(HWND hwnd, int id) {
    HWND hEdit = GetDlgItem(hwnd, id);
    SendMessage(hEdit, EM_SETSEL, 0, -1);  // Select all text
    SetFocus(hEdit);
}

void SetUIFont(HWND hControl, int fontSize) {
    LOGFONT lf = { 0 };
    lf.lfHeight = fontSize;
    lf.lfWeight = FW_NORMAL;
    wcscpy_s(lf.lfFaceName, L"Arial");

    HFONT hFont = CreateFontIndirect(&lf);
    SendMessage(hControl, WM_SETFONT, (WPARAM)hFont, TRUE);
}

void SetUIFont(HWND hwnd, int id, int fontSize) {
    HWND hControl = GetDlgItem(hwnd, id);
    SetUIFont(hControl, fontSize);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        timeLabel = CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD, 10, 10, 300, 50, hwnd, NULL, NULL, NULL);
        SetUIFont(timeLabel, 50); //set font size
        timeLeftLabel = CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD, 10, 70, 300, 50, hwnd, NULL, NULL, NULL);
        SetUIFont(timeLeftLabel, 40); //set font size
        debugLabel = CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD, 10, 130, 300, 50, hwnd, NULL, NULL, NULL);
        SetUIFont(debugLabel, 30); //set font size

        CreateWindowW(L"STATIC", L"Hour:", WS_VISIBLE | WS_CHILD, 10, 190, 40, 20, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 50, 190, 50, 20, hwnd, (HMENU)1001, NULL, NULL);

        CreateWindowW(L"STATIC", L"Minute:", WS_VISIBLE | WS_CHILD, 110, 190, 60, 20, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 170, 190, 50, 20, hwnd, (HMENU)1002, NULL, NULL);

        CreateWindowW(L"STATIC", L"Second:", WS_VISIBLE | WS_CHILD, 230, 190, 60, 20, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 290, 190, 50, 20, hwnd, (HMENU)1003, NULL, NULL);

        CreateWindowW(L"STATIC", L"Millisecond:", WS_VISIBLE | WS_CHILD, 350, 190, 90, 20, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 440, 190, 70, 20, hwnd, (HMENU)1004, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Start Click", WS_VISIBLE | WS_CHILD, 10, 220, 80, 30, hwnd, (HMENU)2001, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Set Current Time", WS_VISIBLE | WS_CHILD, 100, 220, 130, 30, hwnd, (HMENU)2002, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Toggle Always on Top", WS_VISIBLE | WS_CHILD, 240, 220, 150, 30, hwnd, (HMENU)2003, NULL, NULL);

        ShowCurrentTime(hwnd);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 2001) {
            StartClick(hwnd);
        }
        else if (LOWORD(wParam) == 2002) {
            SetCurrentTime(hwnd);
        }
        else if (LOWORD(wParam) == 2003) {
            ToggleAlwaysOnTop(hwnd);
        }
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Get the dimensions of the "Toggle Always on Top" button
        HWND buttonHwnd = GetDlgItem(hwnd, 2003);
        RECT rect;
        GetWindowRect(buttonHwnd, &rect);
        MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rect, 2);

        // Set the color based on the isAlwaysOnTop status
        HBRUSH brush = CreateSolidBrush(isAlwaysOnTop ? RGB(0, 255, 0) : RGB(255, 0, 0));
        SelectObject(hdc, brush);

        // Adjust the position of the dot here
        int dotSize = 10;

        // Retrieve the button text
        wchar_t buttonText[256];
        GetWindowTextW(buttonHwnd, buttonText, sizeof(buttonText) / sizeof(buttonText[0]));

        // Calculate the size of the text
        RECT textRect = { 0 };
        DrawTextW(hdc, buttonText, -1, &textRect, DT_CALCRECT);

        // Calculate the position of the dot relative to the button text size
        int x = rect.left + textRect.right + 10;  // 5 pixels padding from the right edge of the text
        int y = rect.top + ((rect.bottom - rect.top - dotSize) / 2);  // Center vertically

        // Draw the dot
        Ellipse(hdc, x, y, x + dotSize, y + dotSize);

        DeleteObject(brush);
        EndPaint(hwnd, &ps);

        break;

    }
    case WM_LBUTTONDOWN:
    {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);

        RECT rect;
        GetWindowRect(debugLabel, &rect);
        MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rect, 2);

        if (PtInRect(&rect, pt)) {
            // Copy text to clipboard
            int len = GetWindowTextLengthW(debugLabel);
            if (len > 0) {
                wchar_t* text = new wchar_t[len + 1];
                GetWindowTextW(debugLabel, text, len + 1);

                OpenClipboard(hwnd);
                EmptyClipboard();
                HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(wchar_t));
                if (!hg) {
                    CloseClipboard();
                    delete[] text;
                    break;
                }
                memcpy(GlobalLock(hg), text, (len + 1) * sizeof(wchar_t));
                GlobalUnlock(hg);
                SetClipboardData(CF_UNICODETEXT, hg);
                CloseClipboard();

                delete[] text;
            }
        }
    }
    break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"Sample Window Class";

    WNDCLASS wc = { };
    wc.style = CS_HREDRAW | CS_VREDRAW; // Add these styles
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    hwndMain = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Timed Click",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 300,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwndMain == NULL) {
        return 0;
    }

    ShowWindow(hwndMain, nCmdShow);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
