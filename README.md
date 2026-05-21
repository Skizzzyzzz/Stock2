# Stock Analyzer
---

### List of Content

1. Structure
2. Requirements
3. How to build
4. How to start
5. Troubleshooting and known issues 
---

### 1. Structure

#### Backend: C++ | Frontend: HTML/Css/js | API used: Yahoo Finance

```
StockAnalyzer/
├── backend/
│   ├── data/
│   ├── include/
│   │   ├── auth.h
│   │   ├── json_utils.h
│   │   ├── server.h
│   │   ├── stock.h
│   │   ├── user.h
│   │   ├── watchlist.h
│   │   └── yahoo_finance.h
│   ├── src/
│   │   ├── auth.cpp
│   │   ├── json_utils.cpp
│   │   ├── main.cpp
│   │   ├── server.cpp
│   │   ├── stock.cpp
│   │   ├── user.cpp
│   │   ├── watchlist.cpp
│   │   └── yahoo_finance.cpp
│   └── CMakeLists.txt
├── frontend/
│   ├── css/
│   │   └── style.css
│   ├── js/
│   │   ├── app.js
│   │   └── auth.js
│   ├── index.html
│   ├── login.html
│   └── signup.html
├── .gitignore
└── readme.md
```

### 2. Requirements

1. **CMake** (avalible on https://cmake.org/download/)

During install, choose "Add CMake to system PATH".

To verify:
```
camake --version
```

2. **A C++ compiler** (Visual Studio avalible on https://visualstudio.microsoft.com/)

During install, check "Desktop development with C++"

### 3. How to build

**In powershell**

Navigate to backend:
```
cd path_to_project\stockanalyzer\backend
```

Building the whole thing:
```
cmake -B build
cmake --build build -- config Release
```

The executable will be at `build\Release\StockAnalyzer.exe`

### 4. How to start

**In PowerShell**

From inside the `backend`, run the executable:
```
./build\Release\StockAnalyzer.exe
```

The output should be:
```
[Main] Initialising Yahoo Finance client...
[Yahoo] Initialised. Crumb obtained.
[Main] Starting server on port 8081...
[Server] Listening on http://localhost:8081
```

Once it says "Listening", open the second PowerShell

**In 2nd PowerShell**

```
Start-Process " path_to_project\frontend\login.html"
```

It will open the Login page in Browser

**How to stop**
In 1st PowerShell "Ctrl + C", and just close the window in browser

> **Note:** The server creates a `backend/data` folder automatically on first run. This is where `users.bin` and `stocks.bin` are stored. Do not delete this folder while the server is running.

---

## TroubleShooting

| Problem | Fix |
| :--- | :--- |
| `cmake` not found | Re-run the CMake installer and choose "Add to PATH", then restart your terminal |
| `bind() failed on port 8081` | Something else is using port 8081. Find and close it, or change the port number in `src/main.cpp` and rebuild |
| `[Yahoo] Warning: could not obtain crumb` | No internet connection, or Yahoo Finance is temporarily down. The frontend will fall back to cached stock data from the last successful fetch |
| Frontend shows "Connection error" | The server is not running. Start it first, then refresh the page |
| Blank stock list after login | The server started but Yahoo Finance returned no data. Wait 30 seconds and click Refresh Data |