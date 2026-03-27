# smart-launch build instructions

## Files in this package

| File | Purpose |
|---|---|
| `smart-launch.cpp` | Shared launcher source (unchanged) |
| `smart-chrome.rc` | Windows resource script for Chrome build |
| `smart-sublime_text.rc` | Windows resource script for Sublime Text build |
| `fetch_icons.py` | Extracts `.ico` files from your installed apps |
| `Makefile` | For Linux / WSL / MSYS2 with MinGW-w64 |
| `build.bat` | For native Windows with MSYS2/MinGW in PATH |

---

## Step 1 — Extract icons

Run **on your Windows machine**:

```
pip install Pillow
python fetch_icons.py
```

This produces `chrome.ico` and `sublime_text.ico` next to the script.  
If the script can't find an executable (non-standard install path), drop the
`.ico` files in manually — you can extract them with
[Resource Hacker](https://www.angusj.com/resourcehacker/) or
[IcoFX](https://icofx.ro/) as a fallback.

---

## Step 2 — Build

### Option A: Linux / WSL

```bash
sudo apt install g++-mingw-w64-x86-64 mingw-w64-tools
make
```

### Option B: Windows with MSYS2

Open the **MSYS2 MinGW 64-bit** shell:

```bash
pacman -S mingw-w64-x86_64-gcc
make          # if make is available
# or just:
build.bat
```

### Option C: native Windows cmd (MSYS2 in PATH)

```
build.bat
```

---

## How the launchers are invoked

Both executables share the same `smart-launch.cpp` logic.  
The intended call convention is:

```
smart-chrome.exe      <full-exe-path>  <new-window-flag>  [extra args...]
smart-sublime_text.exe <full-exe-path> <new-window-flag>  [extra args...]
```

Example shortcuts / aliases:

```batch
:: Open URL in Chrome, reuse existing window if already running
smart-chrome.exe "C:\Program Files\Google\Chrome\Application\chrome.exe" NONE https://example.com

:: Open file in Sublime, pass --new-window only when not already open
smart-sublime_text.exe "C:\Program Files\Sublime Text\sublime_text.exe" --new-window C:\work\notes.txt
```

`NONE` as the flag means "don't add any flag when launching fresh" — useful
for apps that open a new window by default anyway.
