@ECHO OFF
CLS
ECHO Package Susi 4.0 to EXE
ECHO.

REM Controls the visibility of environment variables and enables cmd extensions
setlocal enableextensions
cd /d "%~dp0"
cd 

REM ================================================================
REM = User's configurations
REM ================================================================
set AIP_PATH=Setup
set AIP_NAME=SUSI4

REM ================================================================
REM = Local variables
REM ================================================================
SET TOOLS_PATH=..\tools

REM ================================================================
REM = Find Advanced Installer Path
REM ================================================================
SET AI_VALUE_NAME=Advanced Installer Path
IF %PROCESSOR_ARCHITECTURE%==AMD64 SET AI_KEY_NAME=HKLM\SOFTWARE\Wow6432Node\Caphyon\Advanced Installer
IF %PROCESSOR_ARCHITECTURE%==x86 SET AI_KEY_NAME=HKLM\SOFTWARE\Caphyon\Advanced Installer
for /F "usebackq skip=2 tokens=4*" %%A IN (`reg query "%AI_KEY_NAME%" /v "%AI_VALUE_NAME%" 2^>nul`) do (
  SET ADVANCEDINSTALLER_PATH=%%B
)
echo Advanced install Path: %ADVANCEDINSTALLER_PATH%
IF "%ADVANCEDINSTALLER_PATH%"=="" (
	ECHO *error* Cannot find Advanced Installer
	CALL "%TOOLS_PATH%\Fail_Check.bat" 2
)

set ADVANCEDINSTALLER_COM="%ADVANCEDINSTALLER_PATH%\bin\x86\AdvancedInstaller.com"

REM ================================================================
REM Get SVN revision
REM ================================================================
SET SRC_PATH=%CD%
setlocal enabledelayedexpansion
CALL "%TOOLS_PATH%\GetSVNVersion.bat" "%SRC_PATH%"
SET SVNREVISOIN=%errorlevel%
setlocal disabledelayedexpansion
echo SVN revision: %SVNREVISOIN%

REM ================================================================
REM = Start package
REM ================================================================
SET RELEASE_AIP=%AIP_PATH%\%AIP_NAME%_Release.aip

COPY /Y "%AIP_PATH%\%AIP_NAME%.aip" "%RELEASE_AIP%"

%ADVANCEDINSTALLER_COM% /edit %RELEASE_AIP% /SetVersion 4.0.%SVNREVISOIN%.0
	CALL "%TOOLS_PATH%\Fail_Check.bat" %ERRORLEVEL%
	
%ADVANCEDINSTALLER_COM% /rebuild %RELEASE_AIP% -buildslist Standard
	CALL "%TOOLS_PATH%\Fail_Check.bat" %ERRORLEVEL%

%ADVANCEDINSTALLER_COM% /rebuild %RELEASE_AIP% -buildslist Light
	CALL "%TOOLS_PATH%\Fail_Check.bat" %ERRORLEVEL%

@pause
