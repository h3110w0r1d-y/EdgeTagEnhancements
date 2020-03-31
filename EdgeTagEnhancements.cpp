#include <Windows.h>
#include <iostream>
#include <tlhelp32.h>
#include <strsafe.h>
#include <comutil.h>
#include <atltypes.h>
#include <string>
using namespace std;

HHOOK mouseHook;
LPPOINT curpoint1 = new POINT;
LPPOINT curpoint2 = new POINT;
BOOL first = true;
int data1;
int data2;
HDC desktopDc = GetDC(NULL);
int horizontalDPI = GetDeviceCaps(desktopDc, LOGPIXELSX);
int verticalDPI = GetDeviceCaps(desktopDc, LOGPIXELSY);

LPCWSTR stringToLPCWSTR(std::string orig)
{
    size_t origsize = orig.length() + 1;
    const size_t newsize = 100;
    size_t convertedChars = 0;
    wchar_t* wcstring = (wchar_t*)malloc(sizeof(wchar_t) * (orig.length() - 1));
    mbstowcs_s(&convertedChars, wcstring, origsize, orig.c_str(), _TRUNCATE);
    return wcstring;
}

PROCESSENTRY32 GetNameByPID(DWORD dwProcessID)
{
    HANDLE	hprocessSnap = NULL;
    PROCESSENTRY32 pe32 = { 0 };

    hprocessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hprocessSnap == (HANDLE)-1)
    {
        return { 0 };
    }
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hprocessSnap, &pe32))
    {
        do
        {
            if (pe32.th32ProcessID == dwProcessID)
            {
                return pe32;
            }
        } while (Process32Next(hprocessSnap, &pe32));
    }
    else
    {
        CloseHandle(hprocessSnap);
    }
    return { 0 };
}

BOOL IsEdgeWindow()
{
    DWORD* lpdwProcessId = new DWORD;
    HWND hwnd = GetForegroundWindow();
    GetWindowThreadProcessId(hwnd, lpdwProcessId);
    PROCESSENTRY32W pt = GetNameByPID(*lpdwProcessId);
    if (lstrcmp((LPCWSTR)pt.szExeFile, stringToLPCWSTR("msedge.exe")) == 0)
    {
        RECT r1;
        IsZoomed(hwnd);
        GetWindowRect(GetForegroundWindow(), &r1);
        LPPOINT m = new POINT;
        GetCursorPos(m);
        if ( (IsZoomed(hwnd) && m->y < (r1.top + 32 * (horizontalDPI / 96.0)) && m->x > r1.left && m->x < r1.right) ||
            (!IsZoomed(hwnd) && m->y < (r1.top + 40 * (horizontalDPI / 96.0)) && m->y > (r1.top + 14 * (horizontalDPI / 96.0)) && m->x - r1.left > 0 && m->x - r1.right < 0))
        {
            return true;
        }
    }
    return false;
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        switch (wParam)
        {
        case WM_LBUTTONUP:
            if (IsEdgeWindow())
            {
                if (first)
                {
                    GetCursorPos(curpoint1);
                    data1 = GetTickCount64();
                    first = false;
                }
                else
                {
                    GetCursorPos(curpoint2);
                    data2 = GetTickCount64();
                    if (data2 - data1 < 200)
                    {
                        mouse_event(MOUSEEVENTF_MIDDLEDOWN, curpoint2->x, curpoint2->y, 0, 0);
                        Sleep(10);
                        mouse_event(MOUSEEVENTF_MIDDLEUP, curpoint2->x, curpoint2->y, 0, 0);
                        first = true;
                    }
                    else
                    {
                        data1 = data2;
                        curpoint1 = curpoint2;
                    }
                }
            }
            break;
        case WM_MOUSEWHEEL:
            if (IsEdgeWindow())
            {
                PMSLLHOOKSTRUCT pmll = (PMSLLHOOKSTRUCT)lParam;
                int zDelta = (short)HIWORD(pmll->mouseData);
                if (zDelta > 0)
                {
                    keybd_event(0x11, 0, 0, 0); //按下Ctrl键
                    keybd_event(0x10, 0, 0, 0); //按下Shift键
                    keybd_event(0x09, 0, 0, 0); //按下Tab键
                    keybd_event(0x09, 0, KEYEVENTF_KEYUP, 0);
                    keybd_event(0x10, 0, KEYEVENTF_KEYUP, 0);
                    keybd_event(0x11, 0, KEYEVENTF_KEYUP, 0);
                }
                else
                {
                    keybd_event(0x11, 0, 0, 0);//按下Ctrl键
                    keybd_event(0x09, 0, 0, 0);//按下Tab键
                    keybd_event(0x09, 0, KEYEVENTF_KEYUP, 0);
                    keybd_event(0x11, 0, KEYEVENTF_KEYUP, 0);
                }
            }
            
        }
    }
    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

void SetHook()
{
    if (!(mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0)))
    {
        //cout << "Failed to install mouse hook!" << endl;
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    //SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);
    SetHook();
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}



