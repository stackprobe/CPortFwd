START "E" CPortFwd\Release\CPortFwd.exe /P 65001 /FP 65002 /FD localhost /K �A�C�h���}�X�^�[CINDERELLA_GIRLS
START "D" CPortFwd\Release\CPortFwd.exe /P 65002 /FP 65003 /FD localhost /K �A�C�h���}�X�^�[CINDERELLA_GIRLS /D
START C:\Factory\Labo\Socket\telnet\Client.exe localhost 65001
START C:\Factory\Labo\Socket\telnet\Server.exe 65003
