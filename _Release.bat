C:\Factory\Tools\RDMD.exe /RC out

COPY /B CPortFwd\Release\CPortFwd.exe out
COPY /B WCPortFwd\WCPortFwd\bin\Release\WCPortFwd.exe out

C:\Factory\Tools\xcp.exe doc out

C:\Factory\SubTools\zip.exe /O out CPortFwd

IF NOT "%1" == "/-P" PAUSE
