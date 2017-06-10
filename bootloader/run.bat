@echo off

SET MAME_PATH=c:\outrun\mame

rem check whether the binaries are present.

if not exist epr-10380b.133 ( 
	echo EPR-10380b.133 not found! 
	goto error
)

if not exist epr-10382b.118 (
	echo EPR-10382b.118 not found!
	goto error
)

rem copy binaries to MAME roms folder.

if not exist "%MAME_PATH%" (
	echo Mame not installed!
	goto error
)

if not exist "%MAME_PATH%\roms" mkdir "%MAME_PATH%\roms"
if not exist "%MAME_PATH%\roms\outrun" mkdir "%MAME_PATH%\roms\outrun"

copy /Y epr-10380b.133 "%MAME_PATH%\roms\outrun\epr-10380b.133" > NUL
copy /Y epr-10382b.118 "%MAME_PATH%\roms\outrun\epr-10382b.118" > NUL

rem should also check for errors.
if exist epr-10327a.76 copy /Y epr-10327a.76 "%MAME_PATH%\roms\outrun\epr-10327a.76" > NUL
if exist epr-10329a.58 copy /Y epr-10329a.58 "%MAME_PATH%\roms\outrun\epr-10329a.58" > NUL

pushd %MAME_PATH%
set mame_exe=
if exist mame.exe mame_exe=mame.exe
if exist mame64.exe set mame_exe=mame64.exe
if %mame_exe%?==? (
  echo Neither mame.exe nor mame64.exe found; aborting.
  popd
  goto error
)

if "%1"=="debug" (
	%mame_exe% outrun -debug -window -skip_gameinfo
) else (
	%mame_exe% outrun -skip_gameinfo
)
popd

:error
echo.
:end
set mame_exe=
set mame_path=
