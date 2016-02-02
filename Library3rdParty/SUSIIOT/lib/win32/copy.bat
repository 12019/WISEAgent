cd /d "%~dp0"
REM ================================================================
SET KEY_NAME=HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment
SET VALUE_NAME=PROCESSOR_ARCHITECTURE
for /F "usebackq tokens=3" %%A IN (`reg query "%KEY_NAME%" /v "%VALUE_NAME%" 2^>nul ^| find "%VALUE_NAME%"`) do (
  If %%A==AMD64 SET OS_TYPE=AMD64
  If %%A==x86 SET OS_TYPE=X86
)
if %OS_TYPE%==X86 (
	xcopy /Y *.dll "%WINDIR%\System32\"
) else (
	xcopy /Y *.dll "%WINDIR%\SysWOW64\"
)
xcopy /Y modules\*.dll "C:\Windows\SUSI\DLL\"

