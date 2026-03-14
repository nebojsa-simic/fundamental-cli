@ECHO OFF
setlocal enabledelayedexpansion

echo ========================================
echo fundamental-cli smoke test (Windows)
echo ========================================
echo.

set ORIG_DIR=%~dp0
set SMOKE_DIR=%ORIG_DIR%..\fundamental-cli-smoke
set FUNDAMENTAL_DIR=%ORIG_DIR%..\fundamental
cd /d "%ORIG_DIR%"
set PASS=0
set FAIL=0

REM -----------------------------------------------------------------------
echo [SETUP] Clean and create smoke directory
if exist "%SMOKE_DIR%" rmdir /s /q "%SMOKE_DIR%"
mkdir "%SMOKE_DIR%"
echo [PASS] smoke dir created
set /a PASS+=1

REM -----------------------------------------------------------------------
echo.
echo [BUILD] Build fundamental-cli
call "%ORIG_DIR%build-windows-amd64.bat"
if %ERRORLEVEL% neq 0 (
    echo [FAIL] build-windows-amd64.bat failed
    exit /b 1
)
echo [PASS] fun.exe built
set /a PASS+=1

REM -----------------------------------------------------------------------
echo.
echo [SETUP] Copy fun.exe to smoke dir
copy /Y fun.exe "%SMOKE_DIR%\fun.exe" >nul
if %ERRORLEVEL% neq 0 (
    echo [FAIL] copy fun.exe failed
    exit /b 1
)
echo [PASS] fun.exe copied
set /a PASS+=1

REM -----------------------------------------------------------------------
cd /d "%SMOKE_DIR%"

echo.
echo [INIT] Run: fun init
"%SMOKE_DIR%\fun.exe" init
if %ERRORLEVEL% neq 0 (
    echo [FAIL] fun init returned non-zero
    exit /b 1
)
echo [PASS] fun init exited 0
set /a PASS+=1

REM -----------------------------------------------------------------------
echo.
echo [CHECK] Files created by fun init

for %%f in (
    src\main.c
    src\cli.c
    src\cli.h
    src\commands\cmd_version.c
    src\commands\cmd_version.h
    src\commands\cmd_help.c
    src\commands\cmd_help.h
    build-windows-amd64.bat
    fun.ini
    README.md
) do (
    if exist "%%f" (
        echo [PASS] %%f exists
        set /a PASS+=1
    ) else (
        echo [FAIL] %%f missing
        set /a FAIL+=1
    )
)

if %FAIL% neq 0 exit /b 1

REM -----------------------------------------------------------------------
echo.
echo [VENDOR] Copy fundamental into vendor\fundamental
xcopy /E /I /Y "%FUNDAMENTAL_DIR%" vendor\fundamental\ >nul
if %ERRORLEVEL% neq 0 (
    echo [FAIL] xcopy fundamental failed
    exit /b 1
)
echo [PASS] fundamental vendored
set /a PASS+=1

if not exist "vendor\fundamental\include\string\string.h" (
    echo [FAIL] vendor\fundamental\include\string\string.h missing
    exit /b 1
)
echo [PASS] vendor sanity check passed
set /a PASS+=1

REM -----------------------------------------------------------------------
echo.
echo [BUILD] Run: fun build
"%SMOKE_DIR%\fun.exe" build
if %ERRORLEVEL% neq 0 (
    echo [FAIL] fun build returned non-zero
    exit /b 1
)
echo [PASS] fun build exited 0
set /a PASS+=1

REM -----------------------------------------------------------------------
echo.
echo [CHECK] Build output

if not exist "app.exe" (
    echo [FAIL] app.exe not found
    exit /b 1
)
echo [PASS] app.exe exists
set /a PASS+=1

REM -----------------------------------------------------------------------
echo.
echo [RUN] Execute app.exe
"%SMOKE_DIR%\app.exe"
if %ERRORLEVEL% neq 0 (
    echo [FAIL] app.exe exited non-zero
    exit /b 1
)
echo [PASS] app.exe ran successfully
set /a PASS+=1

REM -----------------------------------------------------------------------
echo.
echo ========================================
echo Smoke test complete: %PASS% passed, %FAIL% failed
echo ========================================
exit /b 0
