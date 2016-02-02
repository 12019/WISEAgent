@echo off
setlocal enableextensions

REM =================================================
REM Parameters:
REM  %1: [IN]  Target path which get revision form
REM Return:
REM  Revision
REM =================================================

set cmd=svnversion -n %1

FOR /F %%i IN ('%cmd%') DO SET SVNVER=%%i

REM Remove M/S/P characters at last
set SVNVER=%SVNVER:M=%
set SVNVER=%SVNVER:S=%
set SVNVER=%SVNVER:P=%

REM Parse a mixed-revision, Ex: 4123:4168
for /f "tokens=1-2 delims=: " %%a in ("%SVNVER%") do (
	SET SVNVER=%%a
	IF NOT "%%b"=="" SET SVNVER=%%b
)

exit /b %SVNVER%
