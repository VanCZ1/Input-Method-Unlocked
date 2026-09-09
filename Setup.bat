@echo off
setlocal

set "LIB_URL_1=https://github.com/alandtse/CommonLibSSE-NG.git"
set "LIB_BRANCH_1=ng"
set "LIB_PATH_1=extern\CommonLibSSE-NG"

echo Setting up SKSE Plugin...
echo Target1 Submodule: %LIB_URL_1%
echo Target1 Branch: %LIB_BRANCH_1%
echo Target1 Path: %LIB_PATH_1%
echo.

REM Check if Git is available
git --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Git is not installed or not in PATH!
    echo Please install git and try again!
    pause
    exit /b 1
)

REM Initialize git repo if missing
if not exist ".git" (
    echo No .git folder found. Initializing new git repository...
    git init
)

REM Add the submodule
if not exist "extern" (
    mkdir extern
)

REM Add the target1
if exist "%LIB_PATH_1%" (
    echo Removing existing submodule...
    rmdir /s /q "%LIB_PATH_1%"
)
git submodule add -b %LIB_BRANCH_1% %LIB_URL_1% %LIB_PATH_1%
if errorlevel 1 (
    echo ERROR: Target1 add failed, attempting update...
)

REM Update the submodule
git submodule update --init --recursive

REM Clean build cache if it exists
if exist "build" (
    echo Cleaning existing build cache...
    rmdir /s /q "build"
)

echo.
echo Setup complete!
echo Press any key to continue...

endlocal
pause >nul
