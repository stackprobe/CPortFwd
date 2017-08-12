int mutexOpen(char *mutexName);
void mutexRelease(int h);

int eventOpen(char *eventName);
void eventSet(int h);

void waitSignal(int h, int millis = INFINITE);
int waitForMillis(int h, int millis);
void waitForever(int h);
void handleClose(int h);
