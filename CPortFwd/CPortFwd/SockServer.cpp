#include "all.h"

static int FwrdPortNo;
static char *FwrdDomain;
static int ReverseMode;
static camellia_t *Keys[2];
static int StopServerEvent;
static int StopServerFlag;
static critical_t CritConnectWait;
static autoList<int> *ConnectThList;

typedef struct ChannelInfo_st
{
	int RecvSock;
	int SendSock;
	uint64 Counter2C[2];
	uint64 Counter2S[2];
	int DecMode;
	int Death;
	struct ChannelInfo_st *OtherSide;
}
ChannelInfo_t;

static __inline void MakeCounter2(uint64 counter2[2])
{
	do
	{
		getCryptoBytes((uchar *)counter2, sizeof(uint64) * 2);
	}
	while(counter2[0] == _UI64_MAX); // negotiation でカンストしないように。

	cout(
		"MKC2: %020I64u %020I64u\n"
		,counter2[0]
		,counter2[1]
		);
}
static __inline void AddCounter2(autoList<uchar> *block, uint64 counter2[2])
{
	addValue(block, counter2[0]);
	addValue(block, counter2[1]);

	cout(
		"ADC2: %020I64u %020I64u\n"
		,counter2[0]
		,counter2[1]
		);
}
static __inline void UnaddCounter2(autoList<uchar> *block, uint64 counter2[2])
{
	counter2[1] = unaddValue64(block);
	counter2[0] = unaddValue64(block);

	cout(
		"UAC2: %020I64u %020I64u\n"
		,counter2[0]
		,counter2[1]
		);
}
static __inline void CopyCounter2(uint64 dest[2], uint64 src[2])
{
	dest[0] = src[0];
	dest[1] = src[1];

	cout(
		"CPC2: %020I64u %020I64u\n"
		,dest[0]
		,dest[1]
		);
}
static __inline void IncrementCounter4(ChannelInfo_t *i)
{
	if(
		++i->Counter2C[0] == 0 &&
		++i->Counter2C[1] == 0 &&
		++i->Counter2S[0] == 0
		)
		i->Counter2S[1]++;

	// test
	{
		thread_tls static int callCount = 100; // 接続毎に同じように振る舞わなければならないので TLS じゃないとだめ。

		if(callCount)
		{
			switch(callCount)
			{
			case 90:
				i->Counter2C[1]++;
				LOGPOS();
				break;

			case 50:
				i->Counter2S[0]++;
				LOGPOS();
				break;

			case 10:
				i->Counter2S[1]++;
				LOGPOS();
				break;
			}
			callCount--;
		}
	}
}
static int DecryptStreamBuffer(autoList<uchar> *recvBuffer, autoList<uchar> *sendBuffer, ChannelInfo_t *i) // ret: ? 異常アリ
{
	for(; ; )
	{
		if(recvBuffer->GetCount() < 2)
			break;

		int segSize = recvBuffer->GetElement(0) | recvBuffer->GetElement(1) << 8;

		if(recvBuffer->GetCount() < 2 + segSize)
			break;

		autoList<uchar> *buffer = recvBuffer->CopyRange(2, segSize);
		recvBuffer->RemoveElements(0, 2 + segSize);

		if(!camellia_crypt_extend(Keys, buffer, 1)) // ? 失敗
		{
			cout("復号エラー(鍵の不一致の可能性)\n");
			delete buffer;
			return 1;
		}
		// 同期
		{
			uint64 cc2[2];
			uint64 sc2[2];

			UnaddCounter2(buffer, sc2);
			UnaddCounter2(buffer, cc2);

			IncrementCounter4(i);

			if(
				cc2[0] != i->Counter2C[0] ||
				cc2[1] != i->Counter2C[1] ||
				sc2[0] != i->Counter2S[0] ||
				sc2[1] != i->Counter2S[1]
			)
			{
				cout("同期エラー(通信データ破損の可能性)\n");
				delete buffer;
				return 1;
			}
		}
		sendBuffer->AddElements(buffer);
		delete buffer;
	}
	return 0;
}
static void EncryptStreamBuffer(autoList<uchar> *recvBuffer, autoList<uchar> *sendBuffer, ChannelInfo_t *i)
{
	int segSize = m_min(50000, recvBuffer->GetCount());

#if 0 // 重くなる割に意味が無い。
	{
		const int SEGSZMIN = 100;

		if(SEGSZMIN < segSize)
		{
			segSize -= SEGSZMIN;
			segSize *= getCryptoByte();
			segSize /= 255;
			segSize += SEGSZMIN;

			errorCase(!m_isRange(segSize, SEGSZMIN, m_min(50000, recvBuffer->GetCount()))); // test
		}
	}
#endif
	autoList<uchar> *buffer = recvBuffer->DesertRange(0, segSize);

	IncrementCounter4(i);

	AddCounter2(buffer, i->Counter2C);
	AddCounter2(buffer, i->Counter2S);

	camellia_crypt_extend(Keys, buffer, 0);

	sendBuffer->AddElement(buffer->GetCount() >> 0 & 0xff);
	sendBuffer->AddElement(buffer->GetCount() >> 8 & 0xff);
	sendBuffer->AddElements(buffer);

	delete buffer;
}
static void PrintChannelStatus(autoList<uchar> *recvBuffer, autoList<uchar> *sendBuffer, ChannelInfo_t *i, char *location)
{
#if 1 // 重くなる疑惑
	if(recvBuffer->GetCount() || sendBuffer->GetCount())
	{
		cout(
			"%d -> %d %s [%d] -> [%d]\n"
			,i->RecvSock
			,i->SendSock
			,location
			,recvBuffer->GetCount()
			,sendBuffer->GetCount()
			);
	}
#endif
}
static void ChannelTh(uint vi)
{
	ChannelInfo_t *i = (ChannelInfo_t *)vi;

	critical();
	{
//		errorCase(i->SendSock == -1);
//		errorCase(i->RecvSock == -1);

		cout(
			"CC2C: %020I64u %020I64u\n"
			,i->Counter2C[0]
			,i->Counter2C[1]
			);

		cout(
			"CC2S: %020I64u %020I64u\n"
			,i->Counter2S[0]
			,i->Counter2S[1]
			);

		autoList<uchar> *recvBuffer = new autoList<uchar>();
		autoList<uchar> *sendBuffer = new autoList<uchar>();
		time_t deathTime = -1;

		while(!StopServerFlag)
		{
			int delaied = 0;

			if(recvBuffer->GetCount() < 66000) // segSize max == 2^16-1
			{
				PrintChannelStatus(recvBuffer, sendBuffer, i, "R1");

				int recvSizeMax = 50000;

				if(Keys[0] && i->DecMode)
				{
					recvSizeMax = 100;

					if(2 <= recvBuffer->GetCount())
					{
						recvSizeMax = 2 + (recvBuffer->GetElement(0) | recvBuffer->GetElement(1) << 8) - recvBuffer->GetCount();

						// test
						{
							static int entryCount = 100;

							if(entryCount)
							{
								recvSizeMax -= recvSizeMax % 100 + 150;
								entryCount--;
							}
						}
						m_range(recvSizeMax, 100, 50000);
					}
				}

				// ? エラー || 切断
				if(SockRecvBuffer(
					i->RecvSock,
					recvBuffer,
					sendBuffer->GetCount() ? 0 : (delaied = 1, 100),
					recvSizeMax
					) == -1
					)
				{
					LOGPOS();
					cout("recvSizeMax: %d\n", recvSizeMax);
					
					if(2 <= recvBuffer->GetCount())
						cout("recvBuffer[0-1]: %02x %02x\n", recvBuffer->GetElement(0), recvBuffer->GetElement(1));

					i->Death = 1;
					delaied = 0;
				}
				PrintChannelStatus(recvBuffer, sendBuffer, i, "R2");
			}
			if(recvBuffer->GetCount() && sendBuffer->GetCount() < 50000)
			{
				PrintChannelStatus(recvBuffer, sendBuffer, i, "C1");

				if(Keys[0]) // ? 暗号モード
				{
					if(i->DecMode) // ? 復号モード
					{
						if(DecryptStreamBuffer(recvBuffer, sendBuffer, i)) // ? 異常アリ
						{
							LOGPOS();
							i->OtherSide->Death = 1;
							i->Death = 1;
							break;
						}
					}
					else // ? 暗号化モード
					{
						EncryptStreamBuffer(recvBuffer, sendBuffer, i);
					}
				}
				else // ? 平文モード
				{
					sendBuffer->AddElements(recvBuffer);
					recvBuffer->Clear();
				}
				PrintChannelStatus(recvBuffer, sendBuffer, i, "C2");
			}
			if(sendBuffer->GetCount())
			{
				PrintChannelStatus(recvBuffer, sendBuffer, i, "S1");

				if(SockSendBuffer(i->SendSock, sendBuffer, (delaied = 1, 100)) == -1) // ? エラー || 切断
				{
					LOGPOS();
					i->Death = 1;
					delaied = 0;
					sendBuffer->Clear();
				}
				PrintChannelStatus(recvBuffer, sendBuffer, i, "S2");
			}

			if(i->Death)
			{
				i->OtherSide->Death = 1;

				if(deathTime != -1 && !recvBuffer->GetCount() && !sendBuffer->GetCount())
					break;

				if(deathTime == -1)
					deathTime = now() + 2;

				if(deathTime < now())
					break;
			}

			// zantei
			{
				uncritical();
				{
					Sleep(delaied ? 0 : 100);
				}
				critical();
			}
		}
		delete recvBuffer;
		delete sendBuffer;
	}
	uncritical();
}
static void ConnectTh(int sock)
{
	int fwrdSock;
	autoList<uchar> *buffer;

	critical();
	{
		buffer = new autoList<uchar>(48);

		if(ReverseMode)
		{
			fwrdSock = -1; // dummy
			cout("接続: %d -> ?\n", sock);
		}
		else
		{
			fwrdSock = SockConnect(FwrdDomain, FwrdPortNo);
			cout("接続: %d -> %d\n", sock, fwrdSock);

			if(fwrdSock == -1)
				goto connectFault;
		}
		ChannelInfo_t channels[2];
		ChannelInfo_t *encChannel;
		ChannelInfo_t *decChannel;

		memset(channels, 0x00, sizeof(channels));

		channels[0].RecvSock = sock;
		channels[0].SendSock = fwrdSock;
		channels[1].RecvSock = fwrdSock;
		channels[1].SendSock = sock;

		if(!Keys[0]) // ? 平文モード
		{
			encChannel = channels + 0;
			decChannel = channels + 1;
			encChannel->OtherSide = decChannel;
			decChannel->OtherSide = encChannel;
			goto startChannels;
		}

		// negotiation >

		int negoSock;

		if(ReverseMode)
		{
			uncritical();
			{
				enterCritical(&CritConnectWait);
				{
					Sleep(100);
				}
				leaveCritical(&CritConnectWait);
			}
			critical();

			negoSock = sock;
		}
		else
			negoSock = fwrdSock;

		uint64 uc2[2];
		uint64 dc2[2];

		MakeCounter2(uc2);
		MakeCounter2(dc2);

		AddCounter2(buffer, uc2);
		AddCounter2(buffer, dc2);

		camellia_crypt_extend(Keys, buffer, 0, 1);

		int retval = SockSendTime(negoSock, buffer, 60);

		if(retval != 1)
		{
			LOGPOS();
			goto connectFault;
		}
		buffer->Clear(); // 2bs
		retval = SockRecvTime(negoSock, buffer, 64, 60);

		if(retval != 1)
		{
			LOGPOS();
			goto connectFault;
		}
		if(!camellia_crypt_extend(Keys, buffer, 1))
		{
			LOGPOS();
			goto connectFault;
		}
		uint64 ruc2[2];
		uint64 rdc2[2];

		UnaddCounter2(buffer, rdc2);
		UnaddCounter2(buffer, ruc2);

		if(ruc2[0] == _UI64_MAX)
		{
			LOGPOS();
			goto connectFault;
		}
		uc2[0]++;
		ruc2[0]++;

		buffer->Clear(); // 2bs
		AddCounter2(buffer, ruc2);
		AddCounter2(buffer, rdc2);

		camellia_crypt_extend(Keys, buffer, 0, 1);

		retval = SockSendTime(negoSock, buffer, 60);

		if(retval != 1)
		{
			LOGPOS();
			goto connectFault;
		}
		buffer->Clear(); // 2bs
		retval = SockRecvTime(negoSock, buffer, 64, 60);

		if(retval != 1)
		{
			LOGPOS();
			goto connectFault;
		}
		if(!camellia_crypt_extend(Keys, buffer, 1))
		{
			LOGPOS();
			goto connectFault;
		}
		uint64 rruc2[2];
		uint64 rrdc2[2];

		UnaddCounter2(buffer, rrdc2);
		UnaddCounter2(buffer, rruc2);

		if(
			uc2[0] != rruc2[0] ||
			uc2[1] != rruc2[1] ||
			dc2[0] != rrdc2[0] ||
			dc2[1] != rrdc2[1]
			)
		{
			LOGPOS();
			goto connectFault;
		}
		if(ReverseMode)
		{
			encChannel = channels + 1;
			decChannel = channels + 0;

			CopyCounter2(encChannel->Counter2C, rdc2);
			CopyCounter2(encChannel->Counter2S, dc2);
			CopyCounter2(decChannel->Counter2C, ruc2);
			CopyCounter2(decChannel->Counter2S, uc2);
		}
		else
		{
			encChannel = channels + 0;
			decChannel = channels + 1;

			CopyCounter2(encChannel->Counter2C, uc2);
			CopyCounter2(encChannel->Counter2S, ruc2);
			CopyCounter2(decChannel->Counter2C, dc2);
			CopyCounter2(decChannel->Counter2S, rdc2);
		}

		// < negotiation

		if(ReverseMode)
		{
			fwrdSock = SockConnect(FwrdDomain, FwrdPortNo);
			cout("接続: %d -> %d\n", sock, fwrdSock);

			if(fwrdSock == -1)
				goto connectFault;

			channels[0].SendSock = fwrdSock;
			channels[1].RecvSock = fwrdSock;
		}
		encChannel->OtherSide = decChannel;
		decChannel->OtherSide = encChannel;
		decChannel->DecMode = 1;

startChannels:
		int encChTh = runThread(ChannelTh, (uint)encChannel);
		int decChTh = runThread(ChannelTh, (uint)decChannel);

		uncritical();
		{
			waitForMillis(encChTh, INFINITE);
			waitForMillis(decChTh, INFINITE);
		}
		critical();

		handleClose(encChTh);
		handleClose(decChTh);

connectFault:
		if(fwrdSock != -1)
		{
			SockDisconnect(fwrdSock);
		}
		delete buffer;

		SockDisconnect(sock);

		cout("切断: %d -> %d\n", sock, fwrdSock);
	}
	uncritical();
}

void SockServer(int recvPortNo, int fwrdPortNo, char *fwrdDomain, int connectMax, int decMode, uchar *rawKey)
{
	FwrdPortNo = fwrdPortNo;
	FwrdDomain = fwrdDomain;
	ReverseMode = decMode;

	if(rawKey)
	{
		Keys[0] = camellia_create(rawKey + 32 * 0, 32);
		Keys[1] = camellia_create(rawKey + 32 * 1, 32);
	}
	else
	{
		Keys[0] = NULL;
		Keys[1] = NULL;
	}
	StopServerEvent = eventOpen(GetStopServerEventName(recvPortNo));
	StopServerFlag = 0;
	initCritical(&CritConnectWait);
	ConnectThList = new autoList<int>();

	/*
		SockWait() で uncritical() ～ critical() する。
	*/
	critical();
	{
		SockStartup();

		int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		errorCase(sock == -1); // ? 失敗

		struct sockaddr_in sa;
		memset(&sa, 0x00, sizeof(sa));
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = htonl(INADDR_ANY);
		sa.sin_port = htons((unsigned short)recvPortNo);

		int retval = bind(sock, (struct sockaddr *)&sa, sizeof(sa));
#if 0
		if(retval != 0)
		{
			cout("ERROR bind() %d\n", retval);

			if(!QuietMode)
				MessageBox(
					NULL,
					xcout("Can't bind to local port %d.", recvPortNo),
					"CPortFwd Error",
					MB_OK | MB_ICONSTOP | MB_TOPMOST
					);

			termination(EXITCODE_ERROR);
		}
#else
		errorCase_m(retval != 0, xcout("バインドに失敗しました。\nポート %d 番は使用中かもしれません。", recvPortNo)); // ? 失敗
#endif

		retval = listen(sock, SOMAXCONN);
		errorCase(retval != 0); // ? 失敗

		while(!StopServerFlag || ConnectThList->GetCount())
		{
			if(waitForMillis(StopServerEvent, 0) || isHitKey(0x1b)) // ? 停止
			{
				cout("停止しています...\n");
				StopServerFlag = 1;
			}
			SSTSRT_ForceShortMode = connectMax - 1 <= ConnectThList->GetCount() || StopServerFlag;

			if(!StopServerFlag && ConnectThList->GetCount() < connectMax)
			{
				retval = SockWait(sock, 2000, 0);
				errorCase(retval == -1); // ? 失敗

				if(retval) // ? 接続あり
				{
					struct sockaddr_in clsa;
					int sasz = sizeof(clsa);
					int clSock = accept(sock, (struct sockaddr *)&clsa, &sasz);
					errorCase(clSock == -1); // ? 失敗

					int th = runThread((void (*)(uint))ConnectTh, (uint)clSock);
					ConnectThList->AddElement(th);
				}
			}
			else
			{
				uncritical();
				{
					Sleep(2000);
				}
				critical();
			}
			for(int index = 0; index < ConnectThList->GetCount(); index++)
			{
				if(waitForMillis(ConnectThList->GetElement(index), 0))
				{
					ConnectThList->FastDesertElement(index);
					index--;
				}
			}
		}
		retval = closesocket(sock);
		errorCase(retval != 0);

		SockCleanup();
	}
	uncritical();

	if(Keys[0])
	{
		camellia_release(Keys[0]);
		camellia_release(Keys[1]);
	}
	handleClose(StopServerEvent);
	uninitCritical(&CritConnectWait);
	delete ConnectThList;
}
char *GetStopServerEventName(int portNo)
{
	static char *name;
	memFree(name);
	name = xcout("cerulean.charlotte CryptPortForward server termination PORT %d", portNo);
	return name;
}
