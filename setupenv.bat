@echo off
if defined OUTRUN_SDK_PATH (
  echo Environment already set.
  exit /b 0
)

echo Setting up Outrun SDK environment
set path=%path%;%~dp0gcc\bin;%~dp0bin
set OUTRUN_SDK_PATH=%~dp0sdk
set OUTRUN_GCC_PATH=%~dp0gcc

echo.



