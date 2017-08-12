#include "all.h"

static int SockStartupDepth;
static int SockStartupOnce;

void SockStartup(void)
{
	WORD ver;
	WSADATA wsd;
	int retval;

	if(SockStartupDepth++)
		return;

	errorCase(SockStartupOnce);
	SockStartupOnce = 1;

	ver = MAKEWORD(2, 2);
	retval = WSAStartup(ver, &wsd);
	errorCase(retval);
}
void SockCleanup(void)
{
	errorCase(!SockStartupDepth);

	if(--SockStartupDepth)
		return;

	WSACleanup();
}

int SockWait(int sock, int millis, int forWrite)
{
	fd_set fs;
	struct timeval tv;
	int retval;

	FD_ZERO(&fs);
	FD_SET(sock, &fs);
	tv.tv_sec = millis / 1000;
	tv.tv_usec = (millis % 1000) * 1000;

	uncritical();
	{
		retval = select(sock + 1,
			forWrite ? NULL : &fs,
			forWrite ? &fs : NULL,
			NULL,
			&tv
			);
	}
	critical();

	errorCase(retval < -1 || 1 < retval);
	return retval;
}
int SockSend(int sock, void *buffData, int dataSize)
{
	return send(sock, (const char *)buffData, dataSize, 0);
}
int SockRecv(int sock, void *buffData, int dataSize)
{
	return recv(sock, (char *)buffData, dataSize, 0);
}
int SockTransmit(int sock, void **pBuffData, int dataSize, int waitMillis, int forWrite) // ret: 読み込んだバイト数, -1 == エラー
{
	int retval;

	if(dataSize == 0)
		return 0;

	retval = SockWait(sock, waitMillis, forWrite);

	if(retval <= 0) // ? timeout || エラー
		goto endfunc;

	retval = (forWrite ? SockSend : SockRecv)(sock, *pBuffData, dataSize);
	errorCase(retval < -1 || dataSize < retval);

	if(retval == 0) // ? select() が 1 を返したのに、0 バイト -> sock が相手側で閉じられたときの挙動
		retval = -1;

endfunc:
	return retval;
}

void StrIp2Ip(uchar ip[4], char *strip)
{
	autoList<char *> *tokens = tokenize(strip, ".");
	int notBroken = 0;

	if(tokens->GetCount() != 4)
		goto endFunc;

	for(int index = 0; index < 4; index++)
	{
		int n = s2i(tokens->GetElement(index));

		if(!m_isRange(n, 0, 255))
			goto endFunc;

		ip[index] = n;
	}
	notBroken = 1;

endFunc:
	if(!notBroken)
		*(int *)ip = 0;

	releaseList(tokens, (void (*)(char *))memFree);
}
char *Ip2StrIp(uchar ip[4])
{
	static char strip[16];
	sprintf(strip, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
	return strip;
}
autoList<uint> *SockLookupList(char *domain)
{
	autoList<uint> *ipList = new autoList<uint>();
	struct hostent *host;
	struct in_addr addr;
	uint ip;

	SockStartup();
	host = gethostbyname(domain); // domain == "" -> 自分のIP, 複数のNICを挿していると複数返る。

	if(host)
	{
		for(int index = 0; host->h_addr_list[index]; index++)
		{
			addr.s_addr = *(u_long *)host->h_addr_list[index];
			StrIp2Ip((uchar *)&ip, inet_ntoa(addr));
			ipList->AddElement(ip);
		}
	}
	SockCleanup();
	return ipList;
}
void SockLookup(uchar ip[4], char *domain)
{
	autoList<uint> *ipList = SockLookupList(domain);

	*(uint *)ip = ipList->RefElement(0, 0);
	delete ipList;
}
int SockConnect(char *domain, uint portno) // ret: -1 == 失敗
{
	errorCase(!domain);

	uchar ip[4];
	SockLookup(ip, domain);
	char *strip = Ip2StrIp(ip);

	if(!*(int *)ip) // ? not found
		return -1;

	int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	errorCase(sock == -1);

	struct sockaddr_in sa;
	memset(&sa, 0x00, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(strip);
	errorCase(sa.sin_addr.s_addr == INADDR_NONE);
	sa.sin_port = htons((unsigned short)portno);

	int retval = connect(sock, (struct sockaddr *)&sa, sizeof(sa));

	if(retval == -1) // ? 失敗
	{
		closesocket(sock);
		return -1;
	}
	return sock;
}
void SockDisconnect(int sock)
{
	shutdown(sock, SD_BOTH);
	closesocket(sock);
}

int SockSendBuffer(int sock, autoList<uchar> *buffer, int millis)
{
	void *buffData = buffer->ElementAt(0);
	int retval = SockTransmit(sock, &buffData, buffer->GetCount(), millis, 1);

	if(retval == -1)
		return -1;

	if(retval)
	{
		buffer->RemoveElements(0, retval);
		return 1;
	}
	return 0;
}
int SockRecvBuffer(int sock, autoList<uchar> *buffer, int millis, int recvSizeMax)
{
	static int recvBuffSize;
	static void *recvBuff;

	if(!recvBuff || recvBuffSize < recvSizeMax)
	{
		recvBuffSize = recvSizeMax;
		recvBuff = memRealloc(recvBuff, recvBuffSize);
	}
	int retval = SockTransmit(sock, &recvBuff, recvSizeMax, millis, 0);

	if(retval == -1)
		return -1;

	if(retval)
	{
		buffer->AddElements((uchar *)recvBuff, retval);
		return 1;
	}
	return 0;
}

int SSTSRT_ForceShortMode;

int SockSendTime(int sock, autoList<uchar> *buffer, int timeout)
{
	time_t startTime = now();
	int nextPos = 0;

	while(nextPos < buffer->GetCount())
	{
		if(SSTSRT_ForceShortMode && 2 < timeout)
		{
			LOGPOS();
			timeout = 2;
		}
		if(startTime + timeout < now())
		{
			LOGPOS();
			return 0;
		}
		void *buffData = buffer->ElementAt(nextPos);
		int retval = SockTransmit(sock, &buffData, buffer->GetCount() - nextPos, 100, 1);

		if(retval == -1)
		{
			LOGPOS();
			return -1;
		}
		nextPos += retval;
	}
	buffer->RemoveElements(0, nextPos);
	return 1;
}
int SockRecvTime(int sock, autoList<uchar> *buffer, int size, int timeout)
{
	static int recvBuffSize;
	static void *recvBuff;

	if(!recvBuff || recvBuffSize < size)
	{
		recvBuffSize = size;
		recvBuff = memRealloc(recvBuff, recvBuffSize);
	}
	time_t startTime = now();

	while(buffer->GetCount() < size)
	{
		if(SSTSRT_ForceShortMode && 2 < timeout)
		{
			LOGPOS();
			timeout = 2;
		}
		if(startTime + timeout < now())
		{
			LOGPOS();
			return 0;
		}
		int retval = SockTransmit(sock, &recvBuff, size - buffer->GetCount(), 100, 0);

		if(retval == -1)
		{
			LOGPOS();
			return -1;
		}
		buffer->AddElements((uchar *)recvBuff, retval);
	}
	return 1;
}
