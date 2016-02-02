@echo off
setlocal enableextensions
cd /d "%~dp0"

SET TOOLS_ROOT=%CD%\..\tools
SET SRC_PATH=%CD%\..\..
SET REL_PATH=%SRC_PATH%\Release
SET WRAP_PATH=%SRC_PATH%\Wrapper
SET OUT_PATH=%SRC_PATH%\Output\WISEAgent

set MAIN_VERSION=3
set SUB_VERSIOM=0
set BUILD_VERSION=0
set REVISION=0
set argC=0
for %%x in (%*) do Set /A argC+=1

if %argC% LSS  1 goto result

set version=%1
if defined version goto nextVar
goto result

:nextVar
	for /F "tokens=1-4 delims=." %%a in ("%version%") do (
		set MAIN_VERSION=%%a
		set SUB_VERSIOM=%%b
		set BUILD_VERSION=%%c
		set REVISION=%%d
		if [%%d] EQU [] goto getsvnrevision
		goto result
	)	
:getsvnrevision
	CALL "%TOOLS_ROOT%\GetSVNVersion.bat" "%SRC_PATH%"
	set REVISION=%errorlevel%
	
:result

SET version=%MAIN_VERSION%.%SUB_VERSIOM%.%BUILD_VERSION%.%REVISION%


del /s /q %WRAP_PATH%\*.zip
del /s /q %REL_PATH%\*.pdb
del /s /q %REL_PATH%\*.lib
del /s /q %REL_PATH%\*.exp

REM ================================================================
REM = Start package
REM ================================================================
REM "%TOOLS_ROOT%\7zip\7z" a -tzip "%WRAP_PATH%\SUSIAccessAgent-%version%.zip" "%REL_PATH%/*"
"%TOOLS_ROOT%\7zip\7z" a -tzip "%WRAP_PATH%\WISEAgent-%version%.zip" "%OUT_PATH%/*"
