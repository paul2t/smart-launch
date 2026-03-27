@echo off
:: build.bat  –  Build smart-chrome.exe and smart-sublime_text.exe using MSVC
::
:: Run from a "Developer Command Prompt for VS", or from any cmd —
:: the script will locate MSVC automatically via vswhere.
::
:: Prerequisites:
::   python fetch_icons.py   <- run once to produce chrome.ico + sublime_text.ico

setlocal EnableDelayedExpansion

:: ── Bootstrap MSVC environment if needed ─────────────────────────────────
where /q cl.exe
if errorlevel 1 (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "!VSWHERE!" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "!VSWHERE!" (
        echo ERROR: cl.exe not found and vswhere.exe is missing.
        echo        Open a "Developer Command Prompt for VS" and retry.
        exit /b 1
    )
    for /f "usebackq delims=" %%i in (
        `"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`
    ) do set "VS_PATH=%%i"
    call "!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
)

:: ── Verify icons ──────────────────────────────────────────────────────────
if not exist chrome.ico (
    echo ERROR: chrome.ico not found. Run:  python fetch_icons.py
    exit /b 1
)
if not exist sublime_text.ico (
    echo ERROR: sublime_text.ico not found. Run:  python fetch_icons.py
    exit /b 1
)

set SRC=smart-launch.cpp
set CFLAGS=/nologo /O2 /W3 /std:c++17 /EHsc
set LIBS=dwmapi.lib user32.lib shell32.lib
set LFLAGS=/nologo /subsystem:windows

:: ── smart-chrome ──────────────────────────────────────────────────────────
echo [1/4] Compiling resources for smart-chrome...
rc /nologo /fo smart-chrome.res smart-chrome.rc
if errorlevel 1 goto :fail

echo [2/4] Linking smart-chrome.exe...
cl %CFLAGS% ^
    /DTARGET_EXE_PATH=L"\"C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe\"" ^
    /DTARGET_NEW_WINDOW=L"\"--new-window\"" ^
    /Fe:smart-chrome.exe %SRC% smart-chrome.res /link %LFLAGS% %LIBS%
if errorlevel 1 goto :fail

:: ── smart-sublime_text ────────────────────────────────────────────────────
echo [3/4] Compiling resources for smart-sublime_text...
rc /nologo /fo smart-sublime_text.res smart-sublime_text.rc
if errorlevel 1 goto :fail

echo [4/4] Linking smart-sublime_text.exe...
cl %CFLAGS% ^
    /DTARGET_EXE_PATH=L"\"C:\\Program Files\\Sublime Text\\sublime_text.exe\"" ^
    /DTARGET_NEW_WINDOW=L"\"-n\"" ^
    /Fe:smart-sublime_text.exe %SRC% smart-sublime_text.res /link %LFLAGS% %LIBS%
if errorlevel 1 goto :fail

echo.
echo Done!
echo   smart-chrome.exe
echo   smart-sublime_text.exe
del /q *.res *.obj 2>nul
exit /b 0

:fail
echo.
echo BUILD FAILED (see error above).
del /q *.res *.obj 2>nul
exit /b 1
