#include <windows.h>
#include <dwmapi.h>
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

struct TargetInfo {
    std::wstring processName;
    HWND foundHwnd = NULL;
};

std::wstring ToLower(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), ::towlower);
    return s;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    TargetInfo* target = (TargetInfo*)lParam;
    if (!IsWindowVisible(hwnd)) return TRUE;

    int cloaked = 0;
    DwmGetWindowAttribute(hwnd, 14, &cloaked, sizeof(int));
    if (cloaked != 0) return TRUE;

    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProcess) {
        wchar_t imagePath[MAX_PATH];
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProcess, 0, imagePath, &size)) {
            std::wstring path = ToLower(imagePath);
            // Check if this window belongs to our target process
            if (path.find(L"\\" + target->processName) != std::wstring::npos) {
                wchar_t title[256];
                if (GetWindowTextW(hwnd, title, 256) > 0) {
                    target->foundHwnd = hwnd;
                    CloseHandle(hProcess);
                    return FALSE; 
                }
            }
        }
        CloseHandle(hProcess);
    }
    return TRUE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    
    // Minimum 3 args: [0]Launcher, [1]ExePath, [2]NewWindowFlag
    if (argc < 3) {
        if (argv) LocalFree(argv);
        return 1;
    }

    AllowSetForegroundWindow(ASFW_ANY);

    std::wstring fullExePath = argv[1];
    std::wstring newWindowFlag = argv[2];
    
    // Extract just the filename (e.g., chrome.exe) for the window search
    size_t lastSlash = fullExePath.find_last_of(L"\\");
    TargetInfo target;
    target.processName = ToLower((lastSlash == std::wstring::npos) ? fullExePath : fullExePath.substr(lastSlash + 1));

    // 1. Scan for a window belonging to this process on the current desktop
    EnumWindows(EnumWindowsProc, (LPARAM)&target);

    // 2. Build the command line
    std::wstring finalCmd = L"\"" + fullExePath + L"\" ";

    if (target.foundHwnd != NULL) {
        SetForegroundWindow(target.foundHwnd);
    } else {
        // If flag is "NONE", skip adding a flag
        if (newWindowFlag != L"NONE") {
            finalCmd += newWindowFlag + L" ";
        }
    }

    // 3. Append all remaining arguments
    for (int i = 3; i < argc; i++) {
        finalCmd += L"\"" + std::wstring(argv[i]) + L"\" ";
    }

    // 4. Launch
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (CreateProcessW(NULL, (LPWSTR)finalCmd.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    if (argv) LocalFree(argv);
    return 0;
}
