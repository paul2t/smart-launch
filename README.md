Windows program that will detect if a program is alreay open on the current virtual desktop. \
The first argument is the program to look for in the current virtual desktop \
If it is not found, then it will add the 2nd argument specified (`--new-window` for chrome), to force it to open as a new window in the current virtual desktop. \
If it is found, it will not add this argument, give focus to the window, and launch it. That way, it can open as a new tab in the existing window of the current virtual desktop. \
The remaining arguments are passed through to the target executable.

```
smart-launch.exe PROGRAM_PATH NEW_WINDOW_FLAG OTHER_ARGUMENTS
```

# How to setup
Put the smart-launch.exe somewhere. For example: `C:\tools\bin\smart-launch.exe`

Look for `C:\Program Files\Google\Chrome\Application\chrome.exe` in the registry.

Replace it with:
```
C:\tools\bin\smart-launch.exe "C:\Program Files\Google\Chrome\Application\chrome.exe" --new-window "%1"
```

You can also use the .reg files provided for `chrome` and `sublime_text`. \
Make sure it doesn't change your default program.
