cmake_minimum_required(VERSION 3.15)
project(StockAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-only backend
if(NOT WIN32)
    message(FATAL_ERROR "This backend targets Windows (WinSock2 + WinHTTP).")
endif()

set(SOURCES
    src/main.cpp
    src/server.cpp
    src/yahoo_finance.cpp
    src/user.cpp
    src/stock.cpp
    src/watchlist.cpp
    src/auth.cpp
    src/json_utils.cpp
)

add_executable(StockAnalyzer ${SOURCES})

target_include_directories(StockAnalyzer PRIVATE include)

# Windows system libraries
target_link_libraries(StockAnalyzer PRIVATE ws2_32 winhttp)

# Enable warnings (MSVC + GCC/Clang)
if(MSVC)
    target_compile_options(StockAnalyzer PRIVATE /W4 /WX)
else()
    target_compile_options(StockAnalyzer PRIVATE -Wall -Wextra -Wpedantic)
endif()
