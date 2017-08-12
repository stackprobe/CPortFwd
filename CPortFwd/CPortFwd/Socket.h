void SockStartup(void);
void SockCleanup(void);

int SockWait(int sock, int millis, int forWrite);
int SockSend(int sock, void *buffData, int dataSize);
int SockRecv(int sock, void *buffData, int dataSize);
int SockTransmit(int sock, void **pBuffData, int dataSize, int waitMillis, int forWrite);

#define DOMAIN_MYSELF ""

void StrIp2Ip(uchar ip[4], char *strip);
char *Ip2StrIp(uchar ip[4]);
autoList<uint> *SockLookupList(char *domain = DOMAIN_MYSELF);
void SockLookup(uchar ip[4], char *domain = DOMAIN_MYSELF);
int SockConnect(char *domain, uint portno);
void SockDisconnect(int sock);

int SockSendBuffer(int sock, autoList<uchar> *buffer, int millis);
int SockRecvBuffer(int sock, autoList<uchar> *buffer, int millis, int recvSizeMax);

extern int SSTSRT_ForceShortMode;

int SockSendTime(int sock, autoList<uchar> *buffer, int timeout);
int SockRecvTime(int sock, autoList<uchar> *buffer, int size, int timeout);
