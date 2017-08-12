START "E" CPortFwd\Release\CPortFwd.exe /P 65001 /FP 65002 /FD localhost /K myon
START "D" CPortFwd\Release\CPortFwd.exe /P 65002 /FP 65003 /FD localhost /K myon /D
START C:\Factory\Labo\Socket\Test\Echo\Server.exe 65003
START C:\Factory\Labo\Socket\Test\Echo\Client.exe localhost 65001
