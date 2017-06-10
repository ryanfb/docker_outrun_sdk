@echo off
setlocal enabledelayedexpansion

rem for now we'll just pushd the folder in which make.bat resides. useful for visual studio.
pushd %~dp0

rem Check for the SDK.
if not defined OUTRUN_SDK_PATH ( 
  rem Backup plan: check whether we can find setupenv.bat ourselves, and use it for the time being.
  if exist "..\..\setupenv.bat" ( 
    call ..\..\setupenv.bat
    if errorlevel 1 goto error
  ) else (
    echo OUTRUN_SDK_PATH environment variable not set. Please run setupenv.bat!
    exit /b 1
  )
)

set OUTRUN_SDK_INCLUDE=%OUTRUN_SDK_PATH%/include
set OUTRUN_SDK_LDSCRIPT=%OUTRUN_SDK_PATH%/ldscript
set OUTRUN_SDK_LIB=%OUTRUN_SDK_PATH%/lib
set OUTPUT_PATH=output

if "%1"=="clean" goto clean

if not defined OUTRUN_GCC_PATH ( 
  echo OUTRUN_GCC_PATH environment variable not set. Please run setenv.bat!
  exit /b 1
)
set OUTRUN_GCC_PREFIX=m68k-elf-

if not exist !OUTPUT_PATH! mkdir !OUTPUT_PATH!

rem clean out linker scripts.
if exist "!OUTPUT_PATH!\main.link.in" del "!OUTPUT_PATH!\main.link.in"
if exist "!OUTPUT_PATH!\sub.link.in" del "!OUTPUT_PATH!\sub.link.in"

rem compile our files.
echo Compiling...

for %%i in (*.c *.cpp *.s ..\common\*.c) do (
  set inputfile=%%i
  set substr=!inputfile:sub=!
  set cpudef=CPU0
  if not "x!substr!"=="x!inputfile!" set cpudef=CPU1
  
  echo %%i

  if %%~xi? == .c? (
    %OUTRUN_GCC_PREFIX%gcc -c %%i -std=gnu11 -m68000 -o !OUTPUT_PATH!/%%~ni.o -Os -D!CPUDEF! -I. -I!OUTRUN_SDK_INCLUDE! -I!OUTRUN_SDK_INCLUDE!\!cpudef! -I../common
  )
  if %%~xi? == .cpp? (
    %OUTRUN_GCC_PREFIX%g++ -c %%i --no-rtti -m68000 -o !OUTPUT_PATH!/%%~ni.o -Os -D!CPUDEF! -I. -I!OUTRUN_SDK_INCLUDE! -I!OUTRUN_SDK_INCLUDE!\!cpudef! -I../common
  )
  if %%~xi? == .s? (
    %OUTRUN_GCC_PREFIX%as %%i -m68000 -o !OUTPUT_PATH!/%%~ni.o --defsym !CPUDEF!=1
  )

  if ERRORLEVEL 1 goto error

  rem append to linker input list
  if !cpudef!==CPU1 echo !OUTPUT_PATH!/%%~ni.o >> "!OUTPUT_PATH!\sub.link.in"
  if !cpudef!==CPU0 echo !OUTPUT_PATH!/%%~ni.o >> "!OUTPUT_PATH!\main.link.in"
)

rem link
echo Linking...
echo maincpu_rom.bin
rem crti.o crtbegin.o ... -lgcc crtend.o crtn.o
%OUTRUN_GCC_PREFIX%ld "%OUTRUN_GCC_PATH%/lib/gcc/m68k-elf/4.9.0/m68000/crtbegin.o" @!OUTPUT_PATH!/main.link.in !OUTRUN_SDK_LIB!/outrun_sdk_cpu0.lib -lc -lgcc "%OUTRUN_GCC_PATH%/lib/gcc/m68k-elf/4.9.0/m68000/crtend.o" -L"%OUTRUN_GCC_PATH%/lib/gcc/m68k-elf/4.9.0/m68000" -L"%OUTRUN_GCC_PATH%/m68k-elf/lib/m68000" --script=%OUTRUN_SDK_LDSCRIPT%/outrun_main_rom.ld -o !OUTPUT_PATH!/maincpu_rom.bin --Map=!OUTPUT_PATH!/maincpu_rom.map
if ERRORLEVEL 1 goto error
echo maincpu_ram.bin
%OUTRUN_GCC_PREFIX%ld "%OUTRUN_GCC_PATH%/lib/gcc/m68k-elf/4.9.0/m68000/crtbegin.o" @!OUTPUT_PATH!/main.link.in !OUTRUN_SDK_LIB!/outrun_sdk_cpu0.lib -lc -lgcc "%OUTRUN_GCC_PATH%/lib/gcc/m68k-elf/4.9.0/m68000/crtend.o" -L"%OUTRUN_GCC_PATH%/lib/gcc/m68k-elf/4.9.0/m68000" -L"%OUTRUN_GCC_PATH%/m68k-elf/lib/m68000" --script=%OUTRUN_SDK_LDSCRIPT%/outrun_main_ram.ld -o !OUTPUT_PATH!/maincpu_ram.bin --Map=!OUTPUT_PATH!/maincpu_ram.map
if ERRORLEVEL 1 goto error

if exist "!OUTPUT_PATH!\sub.link.in" (
  echo subcpu_rom.bin
  %OUTRUN_GCC_PREFIX%ld "%OUTRUN_GCC_PATH%/lib/gcc/m68k-elf/4.9.0/m68000/crtbegin.o" @!OUTPUT_PATH!/sub.link.in !OUTRUN_SDK_LIB!/outrun_sdk_cpu1.lib -lc -lgcc "%OUTRUN_GCC_PATH%/lib/gcc/m68k-elf/4.9.0/m68000/crtend.o" -L"%OUTRUN_GCC_PATH%/lib/gcc/m68k-elf/4.9.0/m68000" --script=%OUTRUN_SDK_LDSCRIPT%/outrun_sub_rom.ld -o !OUTPUT_PATH!/subcpu_rom.bin --Map=!OUTPUT_PATH!/subcpu_rom.map
  if ERRORLEVEL 1 goto error
  echo subcpu_ram.bin
  %OUTRUN_GCC_PREFIX%ld "%OUTRUN_GCC_PATH%/lib/gcc/m68k-elf/4.9.0/m68000/crtbegin.o" @!OUTPUT_PATH!/sub.link.in !OUTRUN_SDK_LIB!/outrun_sdk_cpu1.lib -lc -lgcc "%OUTRUN_GCC_PATH%/lib/gcc/m68k-elf/4.9.0/m68000/crtend.o" -L"%OUTRUN_GCC_PATH%/lib/gcc/m68k-elf/4.9.0/m68000" --script=%OUTRUN_SDK_LDSCRIPT%/outrun_sub_ram.ld -o !OUTPUT_PATH!/subcpu_ram.bin --Map=!OUTPUT_PATH!/subcpu_ram.map
  if ERRORLEVEL 1 goto error
)

rem delete linker input lists
if exist "!OUTPUT_PATH!\main.link.in" del "!OUTPUT_PATH!\main.link.in"
if exist "!OUTPUT_PATH!\sub.link.in" del "!OUTPUT_PATH!\sub.link.in"

rem build rom images.
echo Building ROM images...

splitbin.exe "!OUTPUT_PATH!\maincpu_rom.bin" 65536 2 "!OUTPUT_PATH!\epr-10380b.133" "!OUTPUT_PATH!\epr-10382b.118" "!OUTPUT_PATH!\epr-10381b.132" "!OUTPUT_PATH!\epr-10383b.117"
if ERRORLEVEL 1 goto error

if exist "!OUTPUT_PATH!\subcpu_rom.bin" (
  splitbin.exe "!OUTPUT_PATH!\subcpu_rom.bin" 65536 2 "!OUTPUT_PATH!\epr-10327a.76" "!OUTPUT_PATH!\epr-10329a.58" "!OUTPUT_PATH!\epr-10328a.75" "!OUTPUT_PATH!\epr-10330a.57"
  if ERRORLEVEL 1 goto error
)

echo Done.
goto end

:clean
rem object files
for %%i in (*.c *.cpp *.s ..\common\*.c) do (
  if exist "!OUTPUT_PATH!\%%~ni.o" del "!OUTPUT_PATH!\%%~ni.o"
)

rem rom files
for %%i in (epr-10380b.133 epr-10382b.118 epr-10381b.132 epr-10383b.117) do (
  if exist "!OUTPUT_PATH!\%%i" del "!OUTPUT_PATH!\%%i"
)

for %%i in (epr-10327a.76 epr-10329a.58 epr-10328a.75 epr-10330a.57) do (
  if exist "!OUTPUT_PATH!\%%i" del "!OUTPUT_PATH!\%%i"
)

for %%i in (main.link.in sub.link.in maincpu_ram.bin maincpu_ram.map maincpu_rom.bin maincpu_rom.map subcpu_ram.bin subcpu_ram.map subcpu_rom.bin subcpu_rom.map) do (
  if exist "!OUTPUT_PATH!\%%i" del "!OUTPUT_PATH!\%%i"
)
goto end

:error
echo Build aborted.
echo.
exit /b 1

:end

popd
