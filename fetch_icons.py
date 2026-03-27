"""
fetch_icons.py  –  Extract .ico files from Chrome and Sublime Text executables.

Run this on your Windows machine BEFORE building:
    python fetch_icons.py

Requires: Pillow   ->  pip install Pillow
          icoextract is NOT needed; we use Windows shell APIs via ctypes.

Outputs:
    chrome.ico
    sublime_text.ico

Both files are written next to this script and will be picked up by the Makefile.
"""

import ctypes
import ctypes.wintypes
import os
import struct
import sys
from pathlib import Path

# ── locate executables ──────────────────────────────────────────────────────

TARGETS = {
    "chrome.ico": [
        r"C:\Program Files\Google\Chrome\Application\chrome.exe",
        r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe",
    ],
    "sublime_text.ico": [
        r"C:\Program Files\Sublime Text\sublime_text.exe",
        r"C:\Program Files\Sublime Text 4\sublime_text.exe",
        r"C:\Program Files\Sublime Text 3\sublime_text.exe",
    ],
}

# ── Win32 helpers ───────────────────────────────────────────────────────────

shell32 = ctypes.windll.shell32
user32  = ctypes.windll.user32
gdi32   = ctypes.windll.gdi32

def extract_icon_to_file(exe_path: str, out_ico: str) -> bool:
    """
    Extract the first large icon from exe_path and save it as a proper
    multi-resolution .ico file (256x256 + 48x48 + 32x32 + 16x16).
    Returns True on success.
    """
    try:
        from PIL import Image
        import io
    except ImportError:
        print("ERROR: Pillow not installed. Run:  pip install Pillow")
        sys.exit(1)

    # ExtractIconEx gives us HICON handles
    large = ctypes.wintypes.HICON()
    small = ctypes.wintypes.HICON()
    n = shell32.ExtractIconExW(exe_path, 0,
                               ctypes.byref(large), ctypes.byref(small), 1)
    if n == 0 or not large:
        print(f"  ! Could not extract icon from {exe_path}")
        return False

    sizes = [256, 128, 64, 48, 32, 16]
    images = []

    for size in sizes:
        # LoadImage with LR_DEFAULTCOLOR and desired size
        hicon = user32.LoadImageW(
            None, exe_path,
            1,          # IMAGE_ICON
            size, size,
            0x00000010  # LR_LOADFROMFILE
        )
        if not hicon:
            # fall back to the already-extracted large icon rescaled
            hicon = large

        # Convert HICON → PIL Image via BMP
        hdc       = user32.GetDC(None)
        hdc_mem   = gdi32.CreateCompatibleDC(hdc)
        hbmp      = gdi32.CreateCompatibleBitmap(hdc, size, size)
        old_bmp   = gdi32.SelectObject(hdc_mem, hbmp)

        # Fill with transparent background (black; we'll handle alpha below)
        user32.DrawIconEx(hdc_mem, 0, 0, hicon, size, size, 0, None, 3)  # DI_NORMAL

        # BITMAPINFOHEADER
        bmi = (ctypes.c_byte * 40)()
        struct.pack_into('<IiiHHIIiiII', bmi, 0,
                         40, size, -size, 1, 32,
                         0, size*size*4, 0, 0, 0, 0)

        buf = (ctypes.c_byte * (size * size * 4))()
        gdi32.GetDIBits(hdc_mem, hbmp, 0, size, buf, bmi, 0)

        # Wrap as RGBA PIL image
        img = Image.frombuffer('RGBA', (size, size), bytes(buf), 'raw', 'BGRA', 0, 1)
        images.append(img)

        gdi32.SelectObject(hdc_mem, old_bmp)
        gdi32.DeleteObject(hbmp)
        gdi32.DeleteDC(hdc_mem)
        user32.ReleaseDC(None, hdc)

    if large: user32.DestroyIcon(large)
    if small: user32.DestroyIcon(small)

    # Save as .ico with all sizes
    base = images[0]  # largest
    base.save(out_ico, format='ICO',
              sizes=[(img.width, img.height) for img in images],
              append_images=images[1:])
    print(f"  ✓ Saved {out_ico}  ({len(images)} sizes)")
    return True


def main():
    script_dir = Path(__file__).parent
    all_ok = True

    for out_name, candidates in TARGETS.items():
        out_path = str(script_dir / out_name)
        found = False
        for exe in candidates:
            if os.path.isfile(exe):
                print(f"Extracting {out_name} from:\n  {exe}")
                if extract_icon_to_file(exe, out_path):
                    found = True
                    break
                else:
                    print(f"  Trying next candidate…")
        if not found:
            print(f"WARNING: Could not produce {out_name}. "
                  f"Place it manually next to the Makefile.")
            all_ok = False

    if all_ok:
        print("\nAll icons ready. You can now run `make` (or `build.bat`).")
    else:
        print("\nSome icons are missing. Provide them manually before building.")


if __name__ == "__main__":
    if sys.platform != "win32":
        print("This script must be run on Windows to access Win32 shell APIs.")
        sys.exit(1)
    main()
