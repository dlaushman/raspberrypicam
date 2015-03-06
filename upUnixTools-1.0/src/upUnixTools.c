/*
 * upUnixTools.c
 * Dale Scott Laushman
 * 2014
 * 
 * - Functions to read config files
 * - Shared memory wrapper funcs
 * - Semaphore wrapper funcs
 * - Message Queue wrapper funcs
 * - Misc text funcs
 * 
 */
#ifndef _WIN32
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <strings.h>
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "upUnixTools.h"
#ifndef _WIN32

/*
 *
 */
int ParseLine(char *buf, char **argv) 
{
	char *delim;
	int argc=0;

	buf[strlen(buf)+1]=0;
	buf[strlen(buf)]=' ';

	while((delim=strchr(buf,' ')))
	{
		argv[argc++]=buf;
		*delim=0;
		buf=delim+1;
		while(*buf&&(*buf==' '))buf++; // go to next argument
	}
	if (!argc)  /* Ignore blank line */
		return 1;

	return argc;
}

/*
 *
 */
int CreateMsgQ(long key)
{
	int qid;
	
	qid=msgget(key,(IPC_CREAT|0660));
	return(qid);
}

/*
 *
 */
int RemoveMsgQ( int qid )
{
	int ret = 0;
	
	if((ret=msgctl(qid,IPC_RMID,NULL))==-1)return(ret);
	return(0);
}
#endif

/*
 *
 */
int SetConfOptionNum(char *ConfigFileName,char *optstr,long value)
{
	char temp[40];
	
	snprintf(temp,sizeof(temp)-2,"%ld",value);
	return(SetConfOptionStr(ConfigFileName,optstr,temp));
}
/*
 *
 */
int SetConfOptionStr(char *ConfigFileName,char *optstr,char *value)
{
	char temp[256];
	char outline[256];
	FILE *ifp,*ofp;
	int ofd;
	struct stat mystat;
	char tempfile[256];

	strncpy(temp,ConfigFileName,sizeof(temp)-2);
	ofd=strlen(temp);
	for(;ofd;ofd--)
	{
		if(temp[ofd]=='/')
		{
			temp[ofd]=0;
			snprintf(tempfile,sizeof(tempfile)-2,"%s/setconfoption.XXXXXX",temp);
			ofd=-2;
			break;
		}
	}

	if(ofd!=-2)
		snprintf(tempfile,sizeof(tempfile)-2,"setconfoption.XXXXXX");

	stat(ConfigFileName,&mystat);

	if(!(ifp=fopen(ConfigFileName,"r")))
		return(-1);

	ofd=mkstemp(tempfile);
	if(!(ofp=fopen(tempfile,"w")))
 	{
		fclose(ifp);
		close(ofd);
		return(-1);
	}
	
   while(fgets(temp,sizeof(temp)-2,ifp))
	{
		if(strstr(temp,optstr))
			snprintf(outline,sizeof(outline)-2,"%s = %s\n",optstr,value);
		else
			strcpy(outline,temp);
		fprintf(ofp,"%s",outline);
   }               
	fclose(ifp);
	fclose(ofp);
	close(ofd);
	chmod(tempfile,mystat.st_mode);
	chown(tempfile,mystat.st_uid,mystat.st_gid);
	rename(tempfile,ConfigFileName);
	chmod(ConfigFileName,mystat.st_mode);
	chown(ConfigFileName,mystat.st_uid,mystat.st_gid);
	return(0);
}

int GetConfOptionNum(FILE *in,char *optstr,long *value)
{
	char temp[256];
	char *p;
	char *k;
	
	rewind(in);
	while(1)
	{
		if(!fgets(temp,sizeof(temp)-2,in))return(-1);
		if(temp[0] == '#'||temp[0]==';'||temp[0]=='[')continue;
		p=strstr(temp," = ");
		if(p)
		{
			*p=0;
			for(k=temp;*k&&isspace(*k);++k);
			if(!strcmp(k,optstr))
			{
				p+=3;
				*value=atoi(p);
				return(0);
			}
		}
	}
}

/*
 *
 */
int GetConfigFileStr(char *filename,char *value,size_t valuelen)
{
	char temp[256];
	FILE *fp;
	
	if(!(fp=fopen(filename,"r")))
		return(-1);
	memset(temp,0,sizeof(temp));
 	memset(value,0,valuelen);
	if(!fgets(temp,sizeof(temp)-2,fp))
	{
		fclose(fp);
		return(-1);
	}
	if(temp[strlen(temp)-1]=='\n')
		temp[strlen(temp)-1]=0;
	strncpy(value,temp,valuelen-1);
	fclose(fp);
	return(0);
}
/*
 *
 */
int GetConfigFileNum(char *filename,long *value)
{
	char temp[256];
	FILE *fp;
	
	if(!(fp=fopen(filename,"r")))
		return(-1);
	memset(temp,0,sizeof(temp));
	if(!fgets(temp,sizeof(temp)-2,fp))
	{
		fclose(fp);
		return(-1);
	}
	*value=atoi(temp);
	fclose(fp);
	return(0);
}

/*
 *
 */
int GetConfOptionStr(FILE *in,char *optstr,char *value,size_t valuelen)
{
	char temp[256];
	char *p;
	char *k;
	
	rewind(in);
	while(1)
	{
		if(!fgets(temp,sizeof(temp)-2,in))return(-1);
		if(temp[0]=='#'||temp[0]==';'||temp[0]=='[')continue;
		p=strstr(temp," = ");
		if(p)
		{
			*p=0;
			p+=3;
			for(k=temp;*k&&isspace(*k);++k);
			if(!strcmp(k,optstr))
			{
				strncpy(value,p,valuelen);
				p=strchr(value,'\n');
				*p=0;
				if(strstr(value,"\"\""))
					*value=0;
				return(0);
			}
		}
	}
}
#ifndef _WIN32

/*
 *
 */
int CreateShmem(long key,size_t segsize)
{
	unsigned flags;
	int shmid;
	
	flags=(IPC_CREAT|IPC_EXCL|0660);
	if((shmid=shmget(key,segsize,flags))==-1)
	{
		shmctl(shmid,IPC_RMID,0);
		return(-1);
	}
	return(shmid);
}

/*
 *
 */
int RemoveShmem(int shmid)
{
	if(shmid!=-1)
		return(shmctl(shmid,IPC_RMID,0));
	return(-1);
}

/*
 *
 */
int GetShmid(long key,size_t segsize)
{
	return(shmget(key,segsize,0));
}

/* attach */
/*if((s=(<struct here>*)shmat(shmid,(char *)'\0',0))==-1)*/


/*
 *
 */
int CreateSem(long key,int numsems)
{
	union semun arg;
	int semid;
	
	if((semid=semget(key,numsems,(IPC_CREAT|0660)))<0)
		return(-1);
	
	arg.val=1;
	semctl(semid,0,SETVAL,arg);
	return(semid);
}

/*
 *
 */
int RemoveSem(semid)
{
	return(semctl(semid,0,IPC_RMID,0));
}

/*
 *
 */
int GetSem(long key,int numsems)
{
	return(semget(key,numsems,0));
}

/*
 *
 */
int LockSem(int semid,int semnumber,unsigned waitflag)
{
	struct sembuf sb;
	
	sb.sem_num=semnumber;
	sb.sem_op=-1;
	if(!waitflag)
		sb.sem_flg=SEM_UNDO;
	else
		sb.sem_flg=SEM_UNDO|IPC_NOWAIT;
	
	return(semop(semid,&sb,1));
}

/*
 *
 */
int UnlockSem(int semid,int semnumber,unsigned waitflag)
{
	struct sembuf sb;
	
	sb.sem_num=semnumber;
	sb.sem_op=1;
	if(!waitflag)
		sb.sem_flg=SEM_UNDO;
	else
		sb.sem_flg=SEM_UNDO|IPC_NOWAIT;
	
	return(semop(semid,&sb,1));
}

/*
 *
 */
int TokenizeIt(int fieldnum,char delim,char *src,char *dest,size_t destsize)
{
	int x=0;
	char *p1,*srcp;
	
	srcp=src;
	if(fieldnum>0)
	{
		while(1)
		{
			if((srcp=strchr(srcp,delim))!=NULL)
			{
				srcp++;
				if(++x==fieldnum)break;
			}
			else
				return(-1);
		}
	}
	if((p1=strchr(srcp,delim))!=NULL)
		*p1=(char)NULL;
	strncpy(dest,srcp,destsize);
	return(0);
}

/*
 *
 */
void RemoveTrailingBlanks(char *str)
{
	int x,len;
	
	len=strlen(str);
	for(x=(--len);x>-1;x--)
	{
		if(str[x]!=' '||str[x]==0)
			return;
		if(str[x]==' ')
			str[x]=0;
	}
}

void StripNonNumbers(char *string,size_t stringsize)
{
	char *pbuf;
	int x,y;

	pbuf=calloc(1,stringsize*2);
	if(!pbuf)
		return;
	
	for(x=0,y=0;x<stringsize-1;x++)
		if(isdigit((int)string[x]))
			pbuf[y++]=string[x];	

	strncpy(string,pbuf,stringsize);
	free(pbuf);
	return;	
}

void GoDaemon(unsigned char CloseStdStreamsFlag)
{
	if(fork())
		exit(0);
	setsid();
	if(CloseStdStreamsFlag)
	{
		fclose(stdin);
		fclose(stdout);
		fclose(stderr);
	}
	chdir("/");
	return;
}


#endif
/*
 * end of file
 */
