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
	errorCase_m(hasArgs(1), "�s���ȃR�}���h�������w�肳��܂����B\n(�R�}���h�����̏����͓���ւ��ł��܂���)");

	errorCase_m(recvPortNo < 1 || 65535 < recvPortNo, "�҂��󂯃|�[�g�ԍ�������������܂���B");
	errorCase_m(fwrdPortNo < 1 || 65535 < fwrdPortNo, "�]����|�[�g�ԍ�������������܂���B");
	errorCase_m(m_isEmpty(fwrdDomain), "�]����z�X�g��������������܂���B");
	errorCase_m(connectMax < 1 || 500 < connectMax, "�ő�ڑ���������������܂���B");

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
