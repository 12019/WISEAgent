@ECHO OFF
setlocal enableextensions
cd /d "%~dp0"

IF %1==Ant (
	TYPE "status.txt"
	FIND "BUILD SUCCESSFUL" /i "status.txt" |FIND "BUILD SUCCESSFUL" /i >"temp.txt"
	< "temp.txt" SET /p status=
	IF NOT DEFINED status (
		PAUSE
		EXIT %1
	)
	SET status=
	GOTO END
)

IF NOT %1==0 (
	COLOR 0C
	PAUSE
	EXIT %1
)

:END
