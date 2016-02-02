@echo off
setlocal enableextensions
REM =================================================
REM Parameters:
REM  %1: [IN]  version
REM Return:
REM  Revision
REM =================================================
SET TOOLS_ROOT=..\tools
SET INCLUDE_ROOT=..\..\Include
SET SRC_PATH=%CD%\..\..

set version=3.0.0.0
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

SET INC_SVN_VER=%INCLUDE_ROOT%\svnversion.h
SET MAKE_SO_VER=%SRC_PATH%\common_version.mk

echo #ifndef __SVN_REVISION_H__ > "%INC_SVN_VER%"
echo #define __SVN_REVISION_H__ >> "%INC_SVN_VER%"
echo #define MAIN_VERSION %MAIN_VERSION% >> "%INC_SVN_VER%"
echo #define SUB_VERSION %SUB_VERSIOM% >> "%INC_SVN_VER%"
echo #define BUILD_VERSION %BUILD_VERSION% >> "%INC_SVN_VER%"
echo #define SVN_REVISION %REVISION% >> "%INC_SVN_VER%"
echo #endif /* __SVN_REVISION_H__ */ >> "%INC_SVN_VER%"

echo core_lib_SOVERSION := %version% > "%MAKE_SO_VER%"
echo SOVERSION := %version% >> "%MAKE_SO_VER%"

SET XML_AGENT_VER=%SRC_PATH%\build\Standard\config\agent_config.xml
type "%XML_AGENT_VER%"|%TOOLS_ROOT%\repl "(<SWVersion>).*(</SWVersion>)" "$1%version%$2" >"%XML_AGENT_VER%.new"

echo %version%
