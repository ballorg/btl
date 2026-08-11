@echo off
setlocal

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 exit /b %errorlevel%

set CMAKE_BIN=C:\Users\Wend4r\AppData\Local\Programs\CLion\bin\cmake\win\x64\bin\cmake.exe
set CTEST_BIN=C:\Users\Wend4r\AppData\Local\Programs\CLion\bin\cmake\win\x64\bin\ctest.exe

if "%~1"=="" (
	"%CMAKE_BIN%" --preset Debug
	if errorlevel 1 exit /b %errorlevel%
	"%CMAKE_BIN%" --build --preset Debug
	exit /b %errorlevel%
)

if /I "%~1"=="configure" (
	"%CMAKE_BIN%" --preset Debug
	exit /b %errorlevel%
)

if /I "%~1"=="build" (
	"%CMAKE_BIN%" --build --preset Debug
	exit /b %errorlevel%
)

if /I "%~1"=="test" (
	"%CMAKE_BIN%" --build --preset Debug
	if errorlevel 1 exit /b %errorlevel%
	"%CTEST_BIN%" --preset Debug
	exit /b %errorlevel%
)

if /I "%~1"=="target" (
	if "%~2"=="" exit /b 2
	"%CMAKE_BIN%" --build --preset Debug --target %2
	exit /b %errorlevel%
)

echo Usage:
echo   scripts\cmake-msvc-x64.cmd
echo   scripts\cmake-msvc-x64.cmd configure
echo   scripts\cmake-msvc-x64.cmd build
echo   scripts\cmake-msvc-x64.cmd test
echo   scripts\cmake-msvc-x64.cmd target ball-tests
exit /b 2
