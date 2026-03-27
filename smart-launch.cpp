#include <windows.h>
#include <dwmapi.h>
#include <string>
#include <algorithm>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

// ---------------------------------------------------------------------------
// Per-target configuration — defined via /D on the compiler command line.
//
//   TARGET_EXE_PATH    Full path to the target executable  (wide string literal)
//   TARGET_NEW_WINDOW  Flag passed when no window is found (wide string literal)
//                      Define as L"" to pass no flag at all.
//
// Example (chrome):
//   /DTARGET_EXE_PATH=L"C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe"
//   /DTARGET_NEW_WINDOW=L"--new-window"
// ---------------------------------------------------------------------------
#if !defined(TARGET_EXE_PATH) || !defined(TARGET_NEW_WINDOW)
#  error "Define TARGET_EXE_PATH and TARGET_NEW_WINDOW on the compiler command line."
#endif

struct TargetInfo {
    std::wstring processName;
    HWND foundHwnd = NULL;
};

std::wstring ToLower(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), ::towlower);
    return s;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    TargetInfo* target = reinterpret_cast<TargetInfo*>(lParam);
    if (!IsWindowVisible(hwnd)) return TRUE;

    int cloaked = 0;
    DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(int));
    if (cloaked != 0) return TRUE;

    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProcess) {
        wchar_t imagePath[MAX_PATH];
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProcess, 0, imagePath, &size)) {
            std::wstring path = ToLower(imagePath);
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

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    // The only accepted argument is an optional path/URL to open.
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    const std::wstring fullExePath   = TARGET_EXE_PATH;
    const std::wstring newWindowFlag = TARGET_NEW_WINDOW;

    // Derive process name (e.g. "chrome.exe") for the window search
    size_t lastSlash = fullExePath.find_last_of(L"\\");
    TargetInfo target;
    target.processName = ToLower(
        (lastSlash == std::wstring::npos) ? fullExePath : fullExePath.substr(lastSlash + 1)
    );

    // 1. Check whether the app already has a visible window
    AllowSetForegroundWindow(ASFW_ANY);
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&target));

    // 2. Build command line
    std::wstring finalCmd = L"\"" + fullExePath + L"\"";

    if (target.foundHwnd != NULL) {
        // App is already running — bring it to the foreground
        SetForegroundWindow(target.foundHwnd);
    } else {
        // App is not running — add the new-window flag if one is configured
        if (!newWindowFlag.empty()) {
            finalCmd += L" " + newWindowFlag;
        }
    }

    // 3. Forward the optional path/URL argument (argv[1])
    if (argc >= 2) {
        finalCmd += L" \"" + std::wstring(argv[1]) + L"\"";
    }

    // 4. Launch
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    if (CreateProcessW(NULL, &finalCmd[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    if (argv) LocalFree(argv);
    return 0;
}
