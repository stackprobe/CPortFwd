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

// sync > @ cout

void cout(char *format, ...)
{
	va_list marker;

	va_start(marker, format);

	if(!strcmp(format, "%s"))
	{
		coutLongText(va_arg(marker, char *));
	}
	else if(!strcmp(format, "%s\n"))
	{
		coutLongText(va_arg(marker, char *));
		coutLongText("\n");
	}
	else
		coutLongText_x(vxcout(format, marker));

	va_end(marker);
}
char *xcout(char *format, ...)
{
	char *ret;
	va_list marker;

	va_start(marker, format);
	ret = vxcout(format, marker);
	va_end(marker);

	return ret;
}
char *vxcout(char *format, va_list marker)
{
	char *buffer;

	for(int size = strlen(format) + 100; ; size *= 2)
	{
		int ret;

#define MARGIN 10

		buffer = (char *)memAlloc(size + MARGIN * 2);
		ret = _vsnprintf(buffer, size + MARGIN, format, marker);

#undef MARGIN

		if(0 <= ret && ret <= size)
			break;

		memFree(buffer);
		errorCase(128 * 1024 * 1024 < size); // ANTI OVER-FLOW
	}
	return strr(buffer);
}
void coutLongText(char *text)
{
	coutLongText_x(strx(text));
}
void coutLongText_x(char *text)
{
	char *p;
	char *q;

	for(p = text; *p; p = q)
	{
		int bkc;

#define FPUTS_TEXT_LMT 100

		for(q = p; *q && ((uint)q - (uint)p) < FPUTS_TEXT_LMT; q += _ismbblead(*q) ? 2 : 1)
		{}

#undef FPUTS_TEXT_LMT

		bkc = *q;
		*q = '\0';
		errorCase(fputs(p, stdout) < 0);
		*q = bkc;
	}
	memFree(text);
}

// < sync

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
	errorCase(index < 0 || __argc < index); // index == __argc は全部読み終わった状態
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
		errorCase(!GetModuleFileName(NULL, fileBuff, BUFFSIZE)); // ? 失敗
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
		dir = getFullPath(dir, getSelfDir()); // TMP, TEMP はフルパスだと思うけど、念のため。あと dir 複製のため。
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
#if 0 // not using
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
#endif

static uint64 NowTick(void)
{
	uint currTick = GetTickCount();
	static uint lastTick;
	static uint64 baseTick;
	uint64 retTick;
	static uint64 lastRetTick;

	if(currTick < lastTick) // ? カウンタが戻った -> オーバーフローした？
	{
		uint diffTick = lastTick - currTick;

		if(UINT_MAX / 2 < diffTick) // オーバーフローだろう。
		{
			LOGPOS();
			baseTick += (uint64)UINT_MAX + 1;
		}
		else // オーバーフローか？
		{
			LOGPOS();
			baseTick += diffTick; // 前回と同じ戻り値になるように調整する。
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
char *getTimeStamp(time_t t) // t: 0 == 現時刻
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
