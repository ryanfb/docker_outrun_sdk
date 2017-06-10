@echo off

setlocal enableDelayedExpansion

rem Add possible MAME paths here. Don't be too picky.

for %%i in (%MAME_PATH% c:\outrun\mame c:\mame) do (
  for %%j in (mame64.exe mame.exe) do (
    if exist "%%i\%%j" ( 
      set MAME_EXE=%%j
      set MAME_PATH=%%i
      goto found_mame
    )
  )
)

echo MAME executable not found. Aborting.
echo You can set the MAME_PATH variable to your MAME directory, or add it directly to run.bat
exit /b 1

:found_mame

echo Running %MAME_EXE% from %MAME_PATH%.

rem check whether the binaries are present.

if not exist output\epr-10380b.133 ( 
	echo EPR-10380b.133 not found! 
	echo.
	goto error
)

if not exist output\epr-10382b.118 (
	echo EPR-10382b.118 not found!
	echo.
	goto error
)

rem copy binaries to MAME roms folder.

if not exist "%MAME_PATH%\roms\outrun" mkdir "%MAME_PATH%\roms\outrun"

copy /Y output\epr-10380b.133 "%MAME_PATH%\roms\outrun\epr-10380b.133" > NUL
copy /Y output\epr-10382b.118 "%MAME_PATH%\roms\outrun\epr-10382b.118" > NUL

if exist output\epr-10381b.132 copy /Y output\epr-10381b.132 "%MAME_PATH%\roms\outrun\epr-10381b.132" > NUL
if exist output\epr-10383b.117 copy /Y output\epr-10383b.117 "%MAME_PATH%\roms\outrun\epr-10383b.117" > NUL

rem should also check for errors.
if exist output\epr-10327a.76 copy /Y output\epr-10327a.76 "%MAME_PATH%\roms\outrun\epr-10327a.76" > NUL
if exist output\epr-10329a.58 copy /Y output\epr-10329a.58 "%MAME_PATH%\roms\outrun\epr-10329a.58" > NUL

pushd %MAME_PATH%
if "%1"=="debug" (
	%MAME_EXE% outrun -debug -window -skip_gameinfo
) else (
	%MAME_EXE% outrun -skip_gameinfo
)
popd

:error
:end
