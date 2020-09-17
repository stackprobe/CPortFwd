/*
	ex. thread_tls static int TlsInt;
*/
#define thread_tls \
	__declspec(thread)

int runThread(void (*userFunc)(uint), uint userInfo);

typedef struct critical_st
{
	CRITICAL_SECTION Csec;
}
critical_t;

void initCritical(critical_t *i);
void uninitCritical(critical_t *i);
void enterCritical(critical_t *i);
void leaveCritical(critical_t *i);

void critical(void);
void uncritical(void);
