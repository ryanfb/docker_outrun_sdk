@echo off

rem for now we'll just pushd the folder in which make.bat resides. useful for visual studio.
pushd %~dp0

if "%1"=="clean" goto clean

rem compile our files.
echo Assembling...
echo boot.s
m68k-elf-as boot.s -m68000 -o boot.o
if ERRORLEVEL 1 goto error

if exist boot-sub.s ( 
  echo boot-sub.s
  m68k-elf-as boot-sub.s -m68000 -o boot-sub.o
  if ERRORLEVEL 1 goto error
)

echo Linking...
m68k-elf-ld boot.o --script=boot.ld -o bootloader.bin --Map=bootloader.map
if ERRORLEVEL 1 goto error

if exist boot-sub.o (
  m68k-elf-ld boot-sub.o --script=boot-sub.ld -o bootloader-sub.bin --Map=bootloader-sub.map
  if ERRORLEVEL 1 goto error
)

rem build rom images.
echo Building ROM images...

m68k-elf-objcopy bootloader.bin epr-10380b.133 --input-target binary -i 2 --byte 0 --pad-to 0x10000
if ERRORLEVEL 1 goto error
m68k-elf-objcopy bootloader.bin epr-10382b.118 --input-target binary -i 2 --byte 1 --pad-to 0x10000
if ERRORLEVEL 1 goto error

if exist bootloader-sub.bin (
  m68k-elf-objcopy bootloader-sub.bin epr-10327a.76 --input-target binary -i 2 --byte 0 --pad-to 0x10000
  if ERRORLEVEL 1 goto error
  m68k-elf-objcopy bootloader-sub.bin epr-10329a.58 --input-target binary -i 2 --byte 1 --pad-to 0x10000
  if ERRORLEVEL 1 goto error
)

echo Done.
goto end

:clean
if exist boot.o del del boot.o
if exist bootloader.bin del del bootloader.bin
if exist epr-10380b.133 del epr-10380b.133
if exist epr-10382b.118 del epr-10382b.118
if exist bootloader.map del bootloader.map
if exist boot-sub.o del del boot-sub.o
if exist bootloader-sub.bin del del bootloader-sub.bin
if exist epr-10327a.76 del epr-10327a.76
if exist epr-10329a.58 del epr-10329a.58
if exist bootloader-sub.map del bootloader-sub.map
goto end

:error
echo Build aborted.
echo.
exit /b 1

:end

popd
