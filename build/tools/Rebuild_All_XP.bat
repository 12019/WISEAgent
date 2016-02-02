@ECHO OFF
CLS
ECHO Rebuild all projects about SUSIAccess Agent
ECHO.

REM Controls the visibility of environment variables and enables cmd extensions
setlocal enableextensions
cd /d "%~dp0"

SET SRC_PATH=..\..
SET gSOAP_PATH=..\..\..\gSOAP
SET BUILD_PATH=%SRC_PATH%\Release-XP
SET REL_PATH=%SRC_PATH%\Release

REM ================================================================
REM = Select build type
REM ================================================================
IF "%1"=="1" (
SET BUILD_TYPE=Build
) ELSE (
SET BUILD_TYPE=rebuild
)

REM ================================================================
REM = User's configurations
REM ================================================================
REM VS2005 => Microsoft Visual Studio 8	(VS80COMNTOOLS)
REM VS2008 => Microsoft Visual Studio 9.0 (VS90COMNTOOLS)
REM VS2010 => Microsoft Visual Studio 10.0 (VS100COMNTOOLS)
set DEVENV_EXE="%VS90COMNTOOLS%\..\IDE\devenv.com"

set SUSIACCESSAGENT_SLN="%SRC_PATH%\CAgent.sln"
REM ================================================================
REM = Start rebuild
REM ================================================================
%DEVENV_EXE% /%BUILD_TYPE% "Release-XP|Win32" %SUSIACCESSAGENT_SLN%
	CALL "Fail_Check.bat" %ERRORLEVEL%

MOVE /Y "%BUILD_PATH%" "%REL_PATH%"
@pause