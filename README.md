# Stock Analyzer
---

## What you need to install

### 1. CMake
Download and install from https://cmake.org/download/  
Pick the Windows x64 installer. During install, choose **"Add CMake to system PATH"**.

Verify it worked:
```
cmake --version
```

### 2. A C++ compiler — pick one

**Option A: Visual Studio 2019 or 2022 (recommended)**  
Download from https://visualstudio.microsoft.com/  
During install, check **"Desktop development with C++"**.  
You do not need the full IDE — the Build Tools alone are enough:  
https://visualstudio.microsoft.com/visual-cpp-build-tools/

**Option B: MinGW-w64 (lighter)**  
Download the MSYS2 installer from https://www.msys2.org/  
After install, open the MSYS2 terminal and run:
```
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake
```
Then add `C:\msys64\ucrt64\bin` to your Windows PATH.

---

## How to build

Open a **Developer Command Prompt** (Visual Studio) or a normal `cmd`/PowerShell if you used MinGW.

Navigate to the backend folder:
```
cd path\to\stock2\backend
```

### With Visual Studio
```
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```
The executable will be at `build\Release\StockAnalyzer.exe`.

### With MinGW
```
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
The executable will be at `build\StockAnalyzer.exe`.

---

## How to start

From inside the `backend` folder, run the executable:

### Visual Studio build
```
build\Release\StockAnalyzer.exe
```

### MinGW build
```
build\StockAnalyzer.exe
```

You should see:
```
[Main] Initialising Yahoo Finance client...
[Yahoo] Initialised. Crumb obtained.
[Server] Listening on http://localhost:8081
```

Once it says "Listening", open `frontend\login.html` in your browser (or serve the frontend folder with any static file server).

> **Note:** The server creates a `backend\data\` folder automatically on first run.
> This is where `users.bin`, `stocks.bin`, and `watchlist.bin` are stored.
> Do not delete this folder while the server is running.

---

## How to stop

Press **Ctrl + C** in the terminal window where the server is running.

The data files are written to disk immediately on every change, so it is safe to stop at any time without losing user accounts or cached stock data.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `cmake` not found | Re-run the CMake installer and choose "Add to PATH", then restart your terminal |
| `bind() failed on port 8081` | Something else is using port 8081. Find and close it, or change the port number in `src/main.cpp` and rebuild |
| `[Yahoo] Warning: could not obtain crumb` | No internet connection, or Yahoo Finance is temporarily down. The frontend will fall back to cached stock data from the last successful fetch |
| Frontend shows "Connection error" | The server is not running. Start it first, then refresh the page |
| Blank stock list after login | The server started but Yahoo Finance returned no data. Wait 30 seconds and click Refresh Data |
