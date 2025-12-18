# Strix - Native Windows Strings Utility

Strix is a standalone command-line tool for Windows, designed to be a drop-in replacement for the GNU/Linux `strings` utility. It extracts human-readable strings from binary files, supporting both ASCII and UTF-16LE (Unicode) formats simultaneously.

## Features

- **Native Windows Executable**: No dependencies, no runtime required.
- **Dual Mode Scanning**: Automatically detects and extracts both ASCII and UTF-16LE strings.
- **Full `strings` Functionality**:
  - Configurable minimum string length (`-n`).
  - Offset printing in hex, decimal, or octal (`-t`).
  - Filename printing (`-f`).

## Installation

To use `strix` as a global command in PowerShell (e.g., `strix malware.exe`), run the included installation script:

```powershell
.\install.ps1
```

After running the script, restart your PowerShell window. You can then use `strix` from any directory.

## Usage

```powershell
strix [options] <file...>
```

### Options

| Option | Description |
|--------|-------------|
| `-n <number>` | Specify minimum string length (default: 4). |
| `-t {o,d,x}` | Print the offset of the string in octal, decimal, or hexadecimal. |
| `-f` | Print the name of the file before each string. |
| `-h` | Display help message. |

### Examples

**Basic usage:**
```powershell
strix C:\Windows\System32\notepad.exe
```

**Find strings longer than 10 characters:**
```powershell
strix -n 10 target.bin
```

**Print offsets in hexadecimal:**
```powershell
strix -t x firmware.bin
```

## Building

### On Linux (Cross-compilation)
Requires `mingw-w64`.

```bash
x86_64-w64-mingw32-gcc -o strix.exe strix.c -static
```

### On Windows (MinGW/GCC)
```cmd
gcc -o strix.exe strix.c -static
```
