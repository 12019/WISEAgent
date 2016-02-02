@ECHO OFF
setlocal enableextensions
cd /d "%~dp0"
CLS

::----------------------------------------------------------------
:: Configurations
::----------------------------------------------------------------
SET SUSI_ROOT=..\..\..
SET TOOLS_PATH=..\tools

::----------------------------------------------------------------
:: Menu
::----------------------------------------------------------------
SET version=%1
if [%version%] EQU [] goto Menu
GOTO PassMenu

:Menu
ECHO.
ECHO ======================================================
ECHO = Input build version: 3.0.0 or 3.0.0.0
ECHO ======================================================
ECHO.
SET /P version=Input version and press ENTER: 
:PassMenu
::----------------------------------------------------------------
:: Start
::----------------------------------------------------------------
@echo | CALL Prebuilt.bat %version% newversion
@echo | CALL %TOOLS_PATH%\Rebuild_All.bat 2
@echo | CALL %TOOLS_PATH%\Collecting.bat %version%
@echo | CALL Postbuilt.bat %version%
REM @echo | CALL Package.bat

REM @pause
