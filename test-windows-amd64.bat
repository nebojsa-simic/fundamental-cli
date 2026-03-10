@echo off
setlocal

set TESTDIR=test-windows-amd64

if exist %TESTDIR% rmdir /s /q %TESTDIR%
mkdir %TESTDIR%
pushd %TESTDIR%

echo Testing Fundamental CLI on Windows AMD64...

echo Running cmd_init test...
..\fun.exe init
IF %ERRORLEVEL% NEQ 0 ( echo FAILED: init & popd & exit /b 1 )

echo Running cmd_version test...
..\fun.exe version
IF %ERRORLEVEL% NEQ 0 ( echo FAILED: version & popd & exit /b 1 )

echo Running cmd_help test...
..\fun.exe help
IF %ERRORLEVEL% NEQ 0 ( echo FAILED: help & popd & exit /b 1 )

echo All tests completed successfully.
popd
rmdir /s /q %TESTDIR%
endlocal
