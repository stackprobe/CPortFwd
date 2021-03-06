int existPath(char *path);
int existFile(char *file);
int existDir(char *dir);

void trimPath(char *path);
char *combine(char *dir, char *file);
char *combine_cx(char *dir, char *file);
char *combine_xc(char *dir, char *file);
char *getDir(char *path);
char *getDir_x(char *path);

void setCwd(char *dir);
void setCwd_x(char *dir);
char *getCwd(void);
void addCwd(char *dir);
void addCwd_x(char *dir);
void unaddCwd(void);

#if 0 // not using
void createFile(char *file);
void createDir(char *dir);
#endif

void removeFile(char *file);
void removeDir(char *dir);
void clearDir(char *dir);
void forceRemoveDir(char *dir);

char *getFullPath(char *path, char *baseDir = ".");
