@echo off
setlocal enabledelayedexpansion

rem Global settings.
set OUTRUN_GCC_PREFIX=m68k-elf-
set OUTRUN_SDK_INCLUDE=include

Rem Check for the compiler.
where %OUTRUN_GCC_PREFIX%gcc >>NUL 2>&1
if %ERRORLEVEL% NEQ 0 ( 
  echo Could not find compiler in path. Please make sure gcc/bin has been added to the environment.
  goto error
)

rem For now we'll just pushd the folder in which build_sdk.bat resides, so we can use relative paths.
pushd %~dp0

if not "%1"=="clean" echo Building the Sega Out Run SDK...

for %%j in (0 1) do (
  set inputdir=cpu%%j
  set outputdir=obj\cpu%%j
  set cpudef=CPU%%j

  rem CPU specific
  rem Clean out archive listing.
  if exist !outputdir!\lib.in del !outputdir!\lib.in
  
  for %%f in (src\!inputdir!\*.c !inputdir!\*.cpp src\!inputdir!\*.s) do (

    if "%1"=="clean" (
	  if exist "!outputdir!\%%~nf.o" del "!outputdir!\%%~nf.o"
	) else (
  
      echo %%f
      if not exist !outputdir! mkdir !outputdir!

      if %%~xf? == .c? (
        %OUTRUN_GCC_PREFIX%gcc -c %%f -std=gnu11 -m68000 -o !outputdir!\%%~nf.o -Os -D!CPUDEF! -I!OUTRUN_SDK_INCLUDE! -I!OUTRUN_SDK_INCLUDE!/cpu%%j
      )
      if %%~xf? == .cpp? (     
	    %OUTRUN_GCC_PREFIX%g++ -c %%f --no-rtti -m68000 -o !outputdir!\%%~nf.o -Os -D!CPUDEF! --no-exceptions -I!OUTRUN_SDK_INCLUDE! -I!OUTRUN_SDK_INCLUDE!/cpu%%j
      )
      if %%~xf? == .s? (
        %OUTRUN_GCC_PREFIX%as %%f -m68000 -o !outputdir!\%%~nf.o --defsym !CPUDEF!=1
      )

      if ERRORLEVEL 1 goto error
  
      rem append to library input list
      echo !outputdir:\=/!/%%~nf.o >> !outputdir!\lib.in
	)
  )
  
  rem remove old libraries
  set libname=outrun_sdk_cpu%%j.lib
  if exist lib\!libname! del lib\!libname!

  if not "%1"=="clean" (
    if exist !outputdir!\lib.in (
      echo Creating library...
      if not exist lib mkdir lib
      %OUTRUN_GCC_PREFIX%ar --target=elf32-m68k lib/!libname! -c -r @!outputdir!\lib.in
      if ERRORLEVEL 1 goto error
	)
  )
)

goto end

:error
echo Build aborted.
popd
exit /b 1

:end
echo Done.
popd
