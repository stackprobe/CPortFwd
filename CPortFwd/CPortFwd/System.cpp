#include "all.h"

int SilentMode;
int QuietMode;

static oneObject(autoList<void (*)(void)>, new autoList<void (*)(void)>(), GetFinalizers)

void addFinalizer(void (*func)(void))
{
	GetFinalizers()->AddElement(func);
}
void (*unaddFinalizer(void))(void)
{
	return GetFinalizers()->UnaddElement();
}

void termination(int errorlevel)
{
	while(GetFinalizers()->GetCount())
	{
		GetFinalizers()->UnaddElement()();
	}
	exit(errorlevel);
}
void error2(char *source, int lineno, char *function, char *message)
{
	{
		static int once;

		if(once)
			exit(2);

		once = 1;
	}

	cout("ERROR %s %d %s\n", source, lineno, function);

	if(message)
		cout("message: %s\n", message);

	if(!QuietMode)
		MessageBox(
			NULL,
			message ? message : xcout("An error has occurred @ %s %d %s", source, lineno, function),
			"CPortFwd Error",
			MB_OK | MB_ICONSTOP | MB_TOPMOST
			);

	termination(EXITCODE_ERROR);
}

void cout(char *format, ...)
{
	if(SilentMode)
		return;

	va_list marker;

	va_start(marker, format);
	errorCase(vprintf(format, marker) < 0);
	va_end(marker);
}
char *xcout(char *format, ...)
{
	char *buffer;
	int size;
	va_list marker;

	va_start(marker, format);

	for(size = strlen(format) + 128; ; size *= 2)
	{
		buffer = (char *)memAlloc(size + 20);
		int retval = _vsnprintf(buffer, size + 10, format, marker);
		buffer[size + 10] = '\0'; // �����I�ɕ���B

		if(0 <= retval && retval <= size)
			break;

		memFree(buffer);
		errorCase(128 * 1024 * 1024 < size); // anti-overflow
	}
	va_end(marker);

	return buffer;
}

static int ArgIndex = 1;

int hasArgs(int count)
{
	return count <= __argc - ArgIndex;
}
int argIs(char *spell)
{
	if(ArgIndex < __argc)
	{
		if(!_stricmp(__argv[ArgIndex], spell))
		{
			ArgIndex++;
			return 1;
		}
	}
	return 0;
}
char *getArg(int index)
{
	errorCase(index < 0 || __argc - ArgIndex <= index);
	return __argv[ArgIndex + index];
}
char *nextArg(void)
{
	char *arg = getArg(0);

	ArgIndex++;
	return arg;
}
int getArgIndex(void)
{
	return ArgIndex;
}
void setArgIndex(int index)
{
	errorCase(index < 0 || __argc < index); // index == __argc �͑S���ǂݏI��������
	ArgIndex = index;
}

char *getSelfFile(void)
{
	static char *fileBuff;

	if(!fileBuff)
	{
		const int BUFFSIZE = 1000;
		const int MARGINSIZE = 10;

		fileBuff = (char *)memAlloc(BUFFSIZE + MARGINSIZE);
		errorCase(!GetModuleFileName(NULL, fileBuff, BUFFSIZE)); // ? ���s
	} 
	return fileBuff;
}
char *getSelfDir(void)
{
	static char *dirBuff;

	if(!dirBuff)
		dirBuff = getDir(getSelfFile());

	return dirBuff;
}

char *getTempRtDir(void)
{
	static char *dir;
	
	if(!dir)
	{
		dir = getenv("TMP");

		if(!dir)
			dir = getenv("TEMP");

		errorCase(!dir);
		errorCase(!existDir(dir));
		dir = getFullPath(dir, getSelfDir()); // TMP, TEMP �̓t���p�X���Ǝv�����ǁA�O�̂��߁B���� dir �����̂��߁B
	}
	return dir;
}
char *makeTempPath(char *suffix)
{
	char *TMPDIR_UUID = "{7b6b94e7-6a7c-4974-9369-f510d89944f4}";

	for(; ; )
	{
		char *pw = makePw36();
		char *path = combine_cx(getTempRtDir(), xcout("%s_%s%s", c_makeUpper(TMPDIR_UUID), pw, suffix));
		memFree(pw);

		if(!existPath(path))
			return path;

		memFree(path);
	}
}
char *makeTempFile(char *suffix)
{
	char *out = makeTempPath(suffix);
	createFile(out);
	return out;
}
char *makeTempDir(char *suffix)
{
	char *out = makeTempPath(suffix);
	createDir(out);
	return out;
}

static uint64 NowTick(void)
{
	uint currTick = GetTickCount();
	static uint lastTick;
	static uint64 baseTick;
	uint64 retTick;
	static uint64 lastRetTick;

	if(currTick < lastTick) // ? �J�E���^���߂��� -> �I�[�o�[�t���[�����H
	{
		uint diffTick = lastTick - currTick;

		if(UINT_MAX / 2 < diffTick) // �I�[�o�[�t���[���낤�B
		{
			LOGPOS();
			baseTick += (uint64)UINT_MAX + 1;
		}
		else // �I�[�o�[�t���[���H
		{
			LOGPOS();
			baseTick += diffTick; // �O��Ɠ����߂�l�ɂȂ�悤�ɒ�������B
		}
	}
	lastTick = currTick;
	retTick = baseTick + currTick;
	errorCase(retTick < lastRetTick); // 2bs
	lastRetTick = retTick;
	return retTick;
}
time_t now(void)
{
	return (time_t)(NowTick() / 1000);
}
char *getTimeStamp(time_t t) // t: 0 == ������
{
	static char timeStamp[25];
	char *p;

	if(!t)
		t = time(NULL);

	p = ctime(&t); // "Wed Jan 02 02:03:55 1980\n"

	if(!p) // ? invalid t
		p = "Thu Jan 01 00:00:00 1970";

	memcpy(timeStamp, p, 24);
	return timeStamp; // "Wed Jan 02 02:03:55 1980"
}