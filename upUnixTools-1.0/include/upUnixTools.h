/*
 * upUnixTools.h
 * Dale Scott Laushman
 * 2014
 *
 *
 *
 */
#ifndef UNIXTOOLS
#define UNIXTOOLS
#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <strings.h>
#endif
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>

#ifndef _WIN32
#ifndef  IPC_WAIT
#define  IPC_WAIT       0
#endif
#endif

union semun {
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
	struct seminfo *__buf;
};

int ParseLine(char *buf, char **argv);
int GetConfOptionNum(FILE *infile,char *option,long *value);
int GetConfOptionStr(FILE *infile,char *option,char *newvalue,size_t len);
int SetConfOptionNum(char *configfilename,char *option,long value);
int SetConfOptionStr(char *configfilename,char *option,char *newvalue);
int GetConfigFileNum(char *filename,long *value);
int GetConfigFileStr(char *filename,char *value,size_t valuelen);

#ifndef _WIN32
int CreateSem(long key,int numsems);
int RemoveSem(int semid);
int GetSem(long key,int numsems);
int LockSem(int semid,int semnumber,unsigned waitflag);
int UnlockSem(int semid,int semnumber,unsigned waitflag);

int CreateShmem(long key,size_t segsize);
int RemoveShmem(int shmid);
int GetShmid(long key,size_t segsize);

int CreateMsgQ(long key);
int RemoveMsgQ(int qid);

void GoDaemon(unsigned char CloseStdStreamsFlag);

#endif
int TokenizeIt(int fieldnum,char delim,char *src,char *dest,size_t destsize);
void RemoveTrailingBlanks(char *src);
void StripNonNumbers(char *string,size_t stringsize);

#endif
/*
 * end of file
 */
