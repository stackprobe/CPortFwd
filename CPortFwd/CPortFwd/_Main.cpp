#include "all.h"

int main(int argc, char **argv)
{
	int recvPortNo = 8080;
	int fwrdPortNo = 80;
	char *fwrdDomain = "localhost";
	int connectMax = 10;
	int decMode = 0;
	uchar *rawKey = NULL;

readArgs:
	if(argIs("/P"))
	{
		recvPortNo = s2i(nextArg());
		goto readArgs;
	}
	if(argIs("/T"))
	{
		int h = eventOpen(GetStopServerEventName(recvPortNo));
		eventSet(h);
		handleClose(h);
		goto endProc;
	}
	if(argIs("/FP"))
	{
		fwrdPortNo = s2i(nextArg());
		goto readArgs;
	}
	if(argIs("/FD"))
	{
		fwrdDomain = nextArg();
		goto readArgs;
	}
	if(argIs("/C"))
	{
		connectMax = s2i(nextArg());
		goto readArgs;
	}
	if(argIs("/K"))
	{
		rawKey = StrToRawKey(nextArg());
		goto readArgs;
	}
	if(argIs("/D"))
	{
		decMode = 1;
		goto readArgs;
	}
	if(argIs("/S"))
	{
		cout("silent mode...\n");
		SilentMode = 1;
		goto readArgs;
	}
	if(argIs("/Q"))
	{
		cout("quiet mode...\n");
		QuietMode = 1;
		goto readArgs;
	}
	errorCase_m(hasArgs(1), "不明なコマンド引数が指定されました。\n(コマンド引数の順序は入れ替えできません)");

	errorCase_m(recvPortNo < 1 || 65535 < recvPortNo, "待ち受けポート番号が正しくありません。");
	errorCase_m(fwrdPortNo < 1 || 65535 < fwrdPortNo, "転送先ポート番号が正しくありません。");
	errorCase_m(m_isEmpty(fwrdDomain), "転送先ホスト名が正しくありません。");
	errorCase_m(connectMax < 1 || 500 < connectMax, "最大接続数が正しくありません。");

	cout("recvPortNo: %d\n", recvPortNo);
	cout("fwrdPortNo: %d\n", fwrdPortNo);
	cout("fwrdDomain: %s\n", fwrdDomain);
	cout("connectMax: %d\n", connectMax);
	cout("decMode: %d\n", decMode);

	SockServer(recvPortNo, fwrdPortNo, fwrdDomain, connectMax, decMode, rawKey);

endProc:
	termination(EXITCODE_NORMAL);
	return 0; // dummy
}
