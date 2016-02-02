@echo off
set "AgentPath=%~dp0" 
set pwd=%1 
set StdaPath=null
set CAPath=null
set MMSPath=null

echo AgentPath:%AgentPath%
echo pwd:%pwd%

if exist "%AgentPath%CAgent.exe" (
    set CAPath="%AgentPath%CAgent.exe"
)
echo CAPath:%CAPath%
if not CAPath==null (
    sadmin solidify -z %pwd% %CAPath%
    sadmin updaters add -z %pwd% %CAPath%
)

if exist "%AgentPath%Standalone Agent.exe" (
    set StdaPath="%AgentPath%Standalone Agent.exe"
)
echo StdaPath:%StdaPath%
if not StdaPath==null (
    sadmin solidify -z %pwd% %StdaPath%
    sadmin updaters add -z %pwd% %StdaPath%
)

if exist "%ProgramFiles%\Acronis\BackupAndRecovery\mms.exe" (
    set MMSPath="%ProgramFiles%\Acronis\BackupAndRecovery\mms.exe"
) else if exist "%ProgramFiles(x86)%\Acronis\BackupAndRecovery\mms.exe" (
    set MMSPath="%ProgramFiles(x86)%\Acronis\BackupAndRecovery\mms.exe"
)
echo MMSPath:%MMSPath%
if not MMSPath==null (
    sadmin solidify -z %pwd% %MMSPath%
    sadmin updaters add -z %pwd% %MMSPath%
)

sadmin features disable -z %pwd% pkg-ctrl-allow-uninstall
