@ECHO OFF
CLS
ECHO Rebuild all projects about SUSI 4.0(CE)
ECHO.

REM Controls the visibility of environment variables and enables cmd extensions
setlocal enableextensions
cd /d "%~dp0"

SET SRC_PATH=..\..

REM ================================================================
REM = Select build type
REM ================================================================
SET BUILD_TYPE=rebuild
IF %1==1 SET BUILD_TYPE=Build

REM ================================================================
set DEVENV_EXE="%VS80COMNTOOLS%\..\IDE\devenv.com"
set SUSI_CE_DRV_SLN="%SRC_PATH%\Drivers\SUSIDrvCE.sln"
set SUSI_CE_LIB_SLN="%SRC_PATH%\Library\Susi4CE.sln"

%DEVENV_EXE% /%BUILD_TYPE% "Release|STANDARDSDK_500 (x86)" %SUSI_CE_DRV_SLN%
	CALL "Fail_Check.bat" %ERRORLEVEL%

%DEVENV_EXE% /%BUILD_TYPE% "Release|STANDARDSDK_500 (x86)" %SUSI_CE_LIB_SLN%
	CALL "Fail_Check.bat" %ERRORLEVEL%

@pause
