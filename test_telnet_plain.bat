START "平文のまま転送" CPortFwd\Release\CPortFwd.exe /P 65001 /FP 65002 /FD localhost
START C:\Factory\Labo\Socket\telnet\Client.exe localhost 65001
START C:\Factory\Labo\Socket\telnet\Server.exe 65002
