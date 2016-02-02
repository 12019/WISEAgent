@echo off
setlocal enableextensions
cd /d "%~dp0"

::-------------------------------------
:: Reference
::-------------------------------------

::-------------------------------------
:: Configurations
::-------------------------------------
SET TOOLS_ROOT=..\tools

::-------------------------------------
:: Generate SVN Version
::-------------------------------------
CALL "%TOOLS_ROOT%\GenVersionInfo.bat" %1
