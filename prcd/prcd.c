/***************************************************

device starts
  if low space show alarm  (low space is not enough to record and encode maxduration)
pre recording
  if no overwrite adjust time to new adjusted max
during recording
  if low space and overwrite then sacrifice
  if low space and no overwrite stop recording  

Stream:
raspivid -o - -t 0 -w 1280 -h 720 -fps 25 -b 600000 -g 50 | ./ffmpeg -re -ar 44100 -ac 2 -acodec pcm_s16le -f s16le -ac 2 -i /dev/zero -f h264 -i - -vcodec copy -acodec aac -ab 128k -g 50 -strict experimental -f flv rtmp://a.rtmp.youtube.com/live2/XXXXXXX 

***************************************************/

#include "system_includes.h"
#include "mcp23s17.h"
#include "pifacedigital.h"
#include "upUnixTools.h"


#define	DEBUG 


void LogConfig(void);
int LoadParms(void);
void LedTrix(void);
int startRecording(void);
void stopRecording(void);
void WiFiCtrl(int);
int startEncoding(char *filename,char *srcsuffix);
void ManageSignals(int);
void ManageDeath(int);
void ManageChildren(int);
fsblkcnt_t GetFSSizeFromPathname(char *,struct statfs *);
int GetConfOptionNumEx(FILE *in, char *optstr,long *value,long minvalue,long maxvalue,long defaultvalue);
int GetConfOptionStrEx(FILE *in,char *optstr,char *value,size_t valuelen,char *defaultvalue);
void FatalError(void);
void BuildVCCmd(char* temp,size_t tempsize);
int CheckRawPath(const char * dir,const char *arg,char *filename,size_t fsize);
void EncodingPrep(char *filename,char *srcsuffix,char *dstsuffix);
int FindOldestFile(const char *dir,char *filename,size_t fsize);
int SacrificeOldestFile(void);

#define ONETENTH			(100000)
#define NEWRECSUFFIX		(".new")
#define PREENCSUFFIX		(".h264")
#define DONESUFFIX		(".mp4")
#define WIFION				(1)
#define WIFIOFF			(0)
#define ENCODINGSPACEMULTIPLIER   (2.1)
#define MINMINUTESAVAIL	(2) 	// why bother with less then 2 minutes available

typedef struct globals_t {
	// settings
	char userdatauser[512]; // change userdata files user ownership to 
	char prcdsigfile[512]; // file that is written by piracecam.py to signal prcd to do something
	long housekeepingfreq; // how often to perform housekeeping tasks
	long housekeepingsync; // sync fs during housekeeping?
	char vfpath[512];	// completed mp4 file path (intended to be separate from raw path
	char wificmd[512]; // cmd to turn wifi on and off
	long wififlag;	// turn wifi on and off?
	char vccmd[512]; // cmd to start raspivid
	char enccmd[512]; // cmd to encode h264 to mp4
	char vcrawpath[512]; // raw path where h264 files are created
	long sizeof1minvidinblocks; // sizeof 1 minute of raw video in 1K blocks (as reported by ls -ls)
	long vcmaxduration; // max duration of video recorder operation in minutes
	char vcargs[512]; // extra arguments for raspivid
	long recstatled; // the piface led bit for the recording status LED
	long errstatled; // the piface led bit for the error status LED
	char vfnamelocation[512]; // used to create the name of the vid file as in "HPR, MAM, etc..."
	char vfnamesponsor[512]; // name of the sponsor
	char vfnameyear[512]; // until we get a realtime clock on the pi we need to at least set the year 
	char vfnamecar[512]; // name of the car
	char vfnamedriver[512]; // name of the driver of the car
	char vfsponsorfile[512];  // name of the jpg file to overlay on the mp4 (not implemented yet)
	long vfserial; // the serial number of this video
	char vcexposure[512]; // the exposure setting of raspivid
	char vccamkillcmd[512]; // the command to kill the camera process
	long vcrotation; // the rotation of the camera (180 is upside down)
	long vcstopdelay; // how long in minutes after the car is shut off (not sure what input to use yet) 
	long vcvidstab; // raspivid video stabilization on or off
	long vfoverwrite; // if we run out of space can we canibalize the oldest (by serial) mp4 files one at a time until we have enough space?
	char codeccmd[512]; // cmd to encode to mp4
	char codecargs[512]; // extra args for encoding
	char codecnicelevel[512]; // level to nice the encoding process
	char encfilename[512]; // name of file that is being encoded
	char encdonefile[512]; // completed mp4 file in the raw path
	char encdoneuserdestfile[512]; // completed mp4 file in the userpath
	long codecpause; // pause codec while recording
	long daemon; // go daemon or not
	long vcautostart;  // start the camera on startup
	// internal variables
	char recfilename[512];
	unsigned char sigtoggle;
	long recpid;
	long encpid;
	long debug;
	struct statfs sfs;
	long housekeepingticker;
	long sigfileticker;
	long sigfilefreq;
	long minsavail;
	int hw_addr;
	int ledcounter;
	int ledcounteroff;
	int sizewarning;
	int uduid;
	int udgid;
} GLOBALS;

GLOBALS g;


/*********************************************************/
int main(int argc, char * argv[])
{
	uint8_t input=0xff,oldinput=0xff;
	struct sigaction new,old;
	char buf[64];
	FILE *fp;
	
	bzero(&g,sizeof(GLOBALS));

	if(LoadParms())
		FatalError();

	if(!g.debug)
	{
		bzero(&new,sizeof( struct sigaction));
		new.sa_handler = SIG_IGN;
		sigaction( SIGINT, &new, &old );
		sigaction( SIGTRAP, &new, &old );
		sigaction( SIGABRT, &new, &old );
		sigaction( SIGPIPE, &new, &old );
 	} 
   bzero( &new,sizeof( struct sigaction ));
   new.sa_handler = ManageChildren;
   sigaction( SIGCHLD, &new, &old );

   bzero( &new,sizeof( struct sigaction ));
   new.sa_handler = ManageSignals;
	sigaction( SIGHUP, &new, &old );
   sigaction( SIGALRM, &new, &old );
   sigaction( SIGUSR1, &new, &old );

	if(g.daemon)
		GoDaemon(0); // ***** don't close stdin, out, and err OR MP4Box encoding FAILS. *****
	
   // self sigs are: 1) if max recording time lapses(***DONE***), 2) traqmate line drops
	if(pifacedigital_open(g.hw_addr)==-1)
	{
		syslog(LOG_DEBUG,"Fatal error: Unable to open piface interface. Please Enable SPI in raspi-config")
		FatalError();
	}
	LedTrix();
	if(g.wififlag&&(!g.vcautostart))
		WiFiCtrl(WIFION);
	if(g.vcautostart)
		g.sigtoggle = g.vcautostart;

	syslog(LOG_DEBUG,"Entering operational state.");
	g.housekeepingticker=g.housekeepingfreq;
	g.sigfilefreq=5;
	g.sigfileticker=g.sigfilefreq;
	while(1)
	{
      /*****************/
		/* HOUSEKEEPING  */
      /*****************/
		if(++g.housekeepingticker>g.housekeepingfreq)
		{
#ifdef DEBUG
			if(g.debug)syslog(LOG_DEBUG,"In housekeeping\n");
#endif
			g.housekeepingticker=0;
			if(g.housekeepingsync)
				sync();
			// check FS space
			if((GetFSSizeFromPathname(g.vfpath,&g.sfs))==-1)
			{
				if(g.debug)syslog(LOG_DEBUG,"can't get fs size\n");
			}
			else
			{
				g.minsavail=(g.sfs.f_bavail*(g.sfs.f_bsize/1024))/(g.sizeof1minvidinblocks*ENCODINGSPACEMULTIPLIER);
				SetConfOptionNum(PRCSPACEFILENAME,"MinsAvailable",g.minsavail);
				if(g.minsavail<g.vcmaxduration)
				{
					g.sizewarning=1;
					syslog(LOG_DEBUG,"Calculated that we are going to run out of room for new videos. Minutes Available = [%ld]",g.minsavail);
				}
				else 
					g.sizewarning=0;
            if(g.recpid&&g.sizewarning)
				{
					syslog(LOG_DEBUG,"Calculated that we are going to run out of room for CURRENT video. Minutes Available = [%ld]",g.minsavail);
					if(g.vfoverwrite)
					{
						if(!(SacrificeOldestFile()))
						{
							syslog(LOG_DEBUG,"Nothing left to sacrifice! Emergency shunt of recording because of low space\n");
							kill(g.recpid,SIGINT);
						}
					}
					else
					{
						syslog(LOG_DEBUG,"Emergency shunt of recording because of low space and no VFOverWrite\n");
						kill(g.recpid,SIGINT);
					}
				}
			}
			// check for orphaned videos (powered off while recording)
			if(!g.recpid&&!g.encpid)
				if(CheckRawPath(g.vcrawpath,NEWRECSUFFIX,g.encfilename,sizeof(g.encfilename)))
					EncodingPrep(g.encfilename,NEWRECSUFFIX,PREENCSUFFIX);
			// check for videos to encode to mp4 in the raw path
			if(!g.encpid)
				if(CheckRawPath(g.vcrawpath,PREENCSUFFIX,g.encfilename,sizeof(g.encfilename)))
					startEncoding(g.encfilename,PREENCSUFFIX);
		}
		/*******************/
		/* SIGS FROM WEB   */
		/*******************/
		if(++g.sigfileticker>g.sigfilefreq)
		{
			g.sigfileticker=0;
			if((fp=fopen(g.prcdsigfile,"r")))
			{
				bzero(buf,sizeof(buf));
				fgets(buf,sizeof(buf)-1,fp);
				if(strstr(buf,"recalc"))
				{
					g.housekeepingticker=g.housekeepingfreq;
				}
				if(strstr(buf,"reload"))
				{
					if((!g.recpid)&&(!g.encpid))
					{
						syslog(LOG_DEBUG,"Reload triggered from [%s].",g.prcdsigfile);
						LoadParms();
					}
					else
						syslog(LOG_DEBUG,"WARN: Can't reload while recording or encoding.");
					LogConfig();
				}
				if(strstr(buf,"toggle"))
				{
					g.sigtoggle=1;
					syslog(LOG_DEBUG,"Record Toggle triggered from [%s].",g.prcdsigfile);
				}
				fclose(fp);
				unlink(g.prcdsigfile);
			}
		}
      /*****************/
		/* Read Statuses */
      /*****************/
		input=pifacedigital_read_reg(INPUT,g.hw_addr);
		if(g.sigtoggle) // signal start/stop via unix signal
		{
			g.sigtoggle=0;
			input=0xfe;
		}
		if(input!=oldinput)
		{
#ifdef DEBUG
			if(g.debug) syslog(LOG_DEBUG,"Inputs: 0x%x\n", input);
#endif
			oldinput=input;
			if(input==0xfe) // button 1 was pressed
         {
				// toggle recording
				if(g.recpid>0)
				{
					kill(g.recpid,SIGINT);
					pifacedigital_write_bit(0,g.recstatled,OUTPUT,g.hw_addr);
				}
				else
				{
					if(startRecording())
						pifacedigital_write_bit(1,g.recstatled,OUTPUT,g.hw_addr);
				}
			}
		}
		/***************/
		/* Blink Leds  */
		/***************/
		if(!g.sizewarning&&!g.encpid&&!g.recpid) // slow blink
		{
			if(++g.ledcounteroff>2)
			{
				if((pifacedigital_read_bit(g.recstatled,OUTPUT,g.hw_addr)))
					pifacedigital_write_bit(0,g.recstatled,OUTPUT,g.hw_addr);
			}
			if((pifacedigital_read_bit(g.errstatled,OUTPUT,g.hw_addr)))
				pifacedigital_write_bit(0,g.errstatled,OUTPUT,g.hw_addr);
			if(++g.ledcounter>30)
			{
				g.ledcounter=0;
				g.ledcounteroff=0;
				pifacedigital_write_bit(1,g.recstatled,OUTPUT,g.hw_addr);
			}
		}
		if(g.sizewarning) // error led on solid
		{
			if(!(pifacedigital_read_bit(g.errstatled,OUTPUT,g.hw_addr)))
				pifacedigital_write_bit(1,g.errstatled,OUTPUT,g.hw_addr);
		}
		if(g.recpid>0) // on solid
		{
			if(!(pifacedigital_read_bit(g.recstatled,OUTPUT,g.hw_addr)))
				pifacedigital_write_bit(1,g.recstatled,OUTPUT,g.hw_addr);
		}
		if(g.encpid>0) // fast blink
		{
			if(++g.ledcounter>2)
			{
				g.ledcounter=0;
				if((pifacedigital_read_bit(g.recstatled,OUTPUT,g.hw_addr)))
					pifacedigital_write_bit(0,g.recstatled,OUTPUT,g.hw_addr);
				else
					pifacedigital_write_bit(1,g.recstatled,OUTPUT,g.hw_addr);
			}
		}
		usleep(ONETENTH);
	}

   pifacedigital_write_reg(0xfc,OUTPUT,g.hw_addr);
   pifacedigital_close(g.hw_addr);

	return(0);	
}
/*********************************************************/
void ManageChildren(int signal)
{
	pid_t pid=0;
	int status=0;
	char temp[512];

#ifdef DEBUG
	if(g.debug) syslog(LOG_DEBUG,"In ManageChildren(%d)\n",signal);
#endif
	pid=wait(&status);
	if(pid<=0) return; // huh? why are we here?
#ifdef DEBUG
	if(g.debug) syslog(LOG_DEBUG,"pid=[%d]\n",pid);
#endif
	if(pid==g.recpid)
	{
#ifdef DEBUG
		if(g.debug) syslog(LOG_DEBUG,"raspivid exited with [%d].\n",status);
#endif
		g.recpid=0;
		pifacedigital_write_bit(0,g.recstatled,OUTPUT,g.hw_addr);
		stopRecording();
	}
	else if(pid==g.encpid)
	{
#ifdef DEBUG
		if(g.debug) syslog(LOG_DEBUG,"encoding exited with [%d].\n",status);
#endif
		// turn off led
		g.encpid=0;
		pifacedigital_write_bit(0,g.recstatled,OUTPUT,g.hw_addr);
		// remove intermediate h264 file
		if(g.encfilename[0])
		{
			snprintf(temp,sizeof(temp),"%s%s",g.vcrawpath,g.encfilename);
#ifdef DEBUG
			if(g.debug) syslog(LOG_DEBUG,"Removing %s\n",temp);
#endif
			unlink(temp);
			bzero(g.encfilename,sizeof(g.encfilename));
			g.housekeepingticker=g.housekeepingfreq;
		}
		// move mp4 to vfpath
		if(g.encdonefile[0]&&g.encdoneuserdestfile[0])
		{
#ifdef DEBUG
			if(g.debug) syslog(LOG_DEBUG,"Moving %s to %s\n",g.encdonefile,g.encdoneuserdestfile);
#endif
			rename(g.encdonefile,g.encdoneuserdestfile);	
		}
	}
	return;
}
/*********************************************************/
void ManageSignals(int signal)
{
#ifdef DEBUG
	syslog(LOG_DEBUG,"In ManageSignals(%d)\n",signal);
#endif
	if(signal==SIGHUP)
	{
		if((!g.recpid)&&(!g.encpid))
			LoadParms();
		LogConfig();
	}
	if(signal==SIGUSR1)
		g.sigtoggle=1;
	return;
}
/*********************************************************/
void LogConfig(void)
{
	syslog(LOG_DEBUG,"HouseKeepingFreq=[%ld]\n",g.housekeepingfreq);
	syslog(LOG_DEBUG,"HouseKeepingSync=[%ld]\n",g.codecpause);
	syslog(LOG_DEBUG,"VCMaxDuration=[%ld]\n",g.vcmaxduration);
	syslog(LOG_DEBUG,"PRCDDebug=[%ld]\n",g.debug);
	syslog(LOG_DEBUG,"PRCDDaemon=[%ld]\n",g.debug);
	syslog(LOG_DEBUG,"Sizeof1MinVidInBlocks=[%ld]\n",g.sizeof1minvidinblocks);
	syslog(LOG_DEBUG,"VFSerial=[%ld]\n",g.vfserial);
	syslog(LOG_DEBUG,"RecordStatusLedBit=[%ld]\n",g.recstatled);
	syslog(LOG_DEBUG,"ErrorStatusLedBit=[%ld]\n",g.errstatled);
	syslog(LOG_DEBUG,"VCVidStab=[%ld]\n",g.vcvidstab);
	syslog(LOG_DEBUG,"VCAutoStart=[%ld]\n",g.vcautostart);
	syslog(LOG_DEBUG,"VCRotation=[%ld]\n",g.vcrotation);
	syslog(LOG_DEBUG,"VFOverwrite=[%ld]\n",g.vfoverwrite);
	syslog(LOG_DEBUG,"VCStopDelay=[%ld]\n",g.vcstopdelay);
	syslog(LOG_DEBUG,"CodecPause=[%ld]\n",g.codecpause);
	syslog(LOG_DEBUG,"WiFiCtrlCommand=[%s]\n",g.wificmd);
	syslog(LOG_DEBUG,"WiFiFlag=[%ld]\n",g.wififlag);
	syslog(LOG_DEBUG,"VFPath=[%s]\n",g.vfpath);
	syslog(LOG_DEBUG,"VFYear=[%s]\n",g.vfnameyear);
	syslog(LOG_DEBUG,"VFLocation=[%s]\n",g.vfnamelocation);
	syslog(LOG_DEBUG,"VFCar=[%s]\n",g.vfnamecar);
	syslog(LOG_DEBUG,"VFSponsor=[%s]\n",g.vfnamesponsor);
	syslog(LOG_DEBUG,"VFSponsorFile=[%s]\n",g.vfsponsorfile);
	syslog(LOG_DEBUG,"VFDriver=[%s]\n",g.vfnamedriver);
	syslog(LOG_DEBUG,"VCExposure=[%s]\n",g.vcexposure);
	syslog(LOG_DEBUG,"VCCamKillCommand=[%s]\n",g.vccamkillcmd);
	syslog(LOG_DEBUG,"VidCamArguments=[%s]\n",g.vcargs);
	syslog(LOG_DEBUG,"VidCamCommand=[%s]\n",g.vccmd);
	syslog(LOG_DEBUG,"VidCamRawPath=[%s]\n",g.vcrawpath);
	syslog(LOG_DEBUG,"CodecCommand=[%s]\n",g.codeccmd);
	syslog(LOG_DEBUG,"CodecArguments=[%s]\n",g.codecargs);
	syslog(LOG_DEBUG,"CodecNiceLevel=[%s]\n",g.codecnicelevel);
	syslog(LOG_DEBUG,"UserDataUser=[%s]\n",g.userdatauser);
	syslog(LOG_DEBUG,"PrcdSigFile=[%s]\n",g.prcdsigfile);
	syslog(LOG_DEBUG,"MinsAvailable=[%ld]\n",g.minsavail);
	syslog(LOG_DEBUG,"UDUID=[%d]\n",g.uduid);
	syslog(LOG_DEBUG,"UDGID=[%d]\n",g.udgid);
	syslog(LOG_DEBUG,"g.sfs.f_bavail=[%ld]\n",g.sfs.f_bavail);
	syslog(LOG_DEBUG,"g.sfs.f_bsize=[%d]\n",g.sfs.f_bsize);
	syslog(LOG_DEBUG,"I can fit [%ld] minutes of video on the device.\n",g.minsavail);
	return;
}
/*********************************************************/
void ManageDeath(int signal)
{
#ifdef DEBUG
	syslog(LOG_DEBUG,"In ManageDeath()\n");
#endif
	if(g.recpid) kill(g.recpid,SIGINT);
	if(g.encpid) kill(g.encpid,SIGINT);
   pifacedigital_write_reg(0x00,OUTPUT,g.hw_addr);
	if(g.wififlag)
		WiFiCtrl(WIFION);
	exit(0);
}
/*********************************************************/
void stopRecording(void)
{
	g.housekeepingticker=g.housekeepingfreq;
#ifdef DEBUG
	if(g.debug) syslog(LOG_DEBUG,"*** recording stopped ***\n");
#endif
	if(g.wififlag) 
		WiFiCtrl(WIFION);
	return;
}
/*********************************************************/
void WiFiCtrl(int toggle)
{
	char wificmd[512];
	int ret;

	bzero(wificmd,sizeof(wificmd));
	sprintf(wificmd,"%s %d",g.wificmd,toggle);
#ifdef DEBUG
	if(g.debug) syslog(LOG_DEBUG,"Exec: [%s]\n",wificmd);
#endif
   ret=system(wificmd);
#ifdef DEBUG
	if(g.debug) syslog(LOG_DEBUG,"Back from startin wifi. ret=%d\n",ret);
#endif
	return;
}
/*********************************************************/
int startRecording(void)
{
	char temp[512];
	char *argv[64];
	int pid=0;

	if(g.minsavail<MINMINUTESAVAIL)
		return(0);
	bzero(temp,sizeof(temp));
	system(g.vccamkillcmd);
	BuildVCCmd(temp,sizeof(temp));
	if(g.wififlag)
		WiFiCtrl(WIFIOFF);
#ifdef DEBUG
	if(g.debug) syslog(LOG_DEBUG,"About to Execv(): [%s]\n",temp);
#endif
	bzero(argv,sizeof(argv));
  	ParseLine(temp,argv); 
	pid=fork();
	if(!pid)
	{
#ifdef DEBUG
		if(g.debug) syslog(LOG_DEBUG,"Execv()ing\n");
#endif
		execv(argv[0],argv);
#ifdef DEBUG
		if(g.debug) syslog(LOG_DEBUG,"Error: We shouldn't be here.\n");
#endif
      exit(1);
	}
#ifdef DEBUG
	if(pid==-1&&g.debug)
		syslog(LOG_DEBUG,"Failure to fork() for record process\n");
#endif
	g.recpid=pid;
	return(pid);
}
/*********************************************************/
void EncodingPrep(char *filename,char *srcsuffix,char *dstsuffix)
{
	char src[512];
	char dst[512];

	bzero(src,sizeof(src));
	bzero(dst,sizeof(dst));

	snprintf(src,sizeof(src)-1,"%s%s",g.vcrawpath,filename);	
	strncpy(dst,g.vcrawpath,sizeof(g.vcrawpath)-1);
	strncat(dst,filename,strlen(filename)-strlen(srcsuffix));
	strncat(dst,dstsuffix,sizeof(dst));
#ifdef DEBUG
	if(g.debug) syslog(LOG_DEBUG,"EncodePrep rename %s %s\n",src,dst);
#endif
	rename(src,dst);
	return;
}
/*********************************************************/
int startEncoding(char *filename,char *srcsuffix)
{
	char temp[512];
	char newfilename[512];
	char *argv[64];
	int pid=0;
	char *p;

	bzero(argv,sizeof(argv));
	bzero(temp,sizeof(temp));
	// blow away any previous .mp4 container with same name
	snprintf(temp,sizeof(temp),"%s%s",g.vcrawpath,filename);
	temp[strlen(temp)-sizeof(DONESUFFIX)]=0;
	strncat(temp,DONESUFFIX,sizeof(temp));
#ifdef DEBUG
	if(g.debug) syslog(LOG_DEBUG,"Trying to remove (old and possibly bad) .mp4 file: [%s]",temp);
#endif
	unlink(temp);
	// create filename for the .mp4
	bzero(temp,sizeof(temp));
	bzero(newfilename,sizeof(newfilename));
	strncpy(newfilename,filename,sizeof(newfilename));
	p=strstr(newfilename,srcsuffix);
	if(!*p)
	{
#ifdef DEBUG
		syslog(LOG_DEBUG,"have weird filename =[%s]\n",newfilename);
#endif
		return(-1);
	}
	*p=0;
	bzero(g.encdonefile,sizeof(g.encdonefile));
	bzero(g.encdoneuserdestfile,sizeof(g.encdoneuserdestfile));
	snprintf(g.encdonefile,sizeof(g.encdonefile),"%s%s%s",g.vcrawpath,newfilename,DONESUFFIX);
	snprintf(g.encdoneuserdestfile,sizeof(g.encdoneuserdestfile),"%s%s%s",g.vfpath,newfilename,DONESUFFIX);
	snprintf(temp,sizeof(temp),"%s %s %s%s %s%s%s",g.codeccmd,g.codecargs,g.vcrawpath,filename,g.vcrawpath,newfilename,DONESUFFIX);
#ifdef DEBUG
	if(g.debug) syslog(LOG_DEBUG,"About to Execv(): [%s]\n",temp);
#endif
  	ParseLine(temp,argv); 
	sync();
	pid=fork();
	if(!pid)
	{
#ifdef DEBUG
		if(g.debug) syslog(LOG_DEBUG,"Execv()ing\n");
#endif
		nice(atoi(g.codecnicelevel));
		if(g.udgid)
			setgid(g.udgid);
		if(g.uduid)
			setuid(g.uduid);
		execv(argv[0],argv);
#ifdef DEBUG
		if(g.debug) syslog(LOG_DEBUG,"Error: We shouldn't be here.\n");
#endif
      exit(1);
	}
#ifdef DEBUG
	if(pid==-1)
	{
		if(g.debug)
			syslog(LOG_DEBUG,"Failure to fork() for encoding process\n");
	}
#endif
	g.encpid=pid;
	return(pid);
}
/*********************************************************/
int CheckRawPath(const char *dir,const char *arg,char *filename,size_t fsize)
{
	DIR *dirp;
	struct dirent *dp;

	if(!(dirp=opendir(dir)))
   {
		perror(dir);
		return(0);
	}
	bzero(filename,fsize);
	while((dp=readdir(dirp)))
	{
		if(dp->d_type!=DT_REG)continue;
#ifdef DEBUG
		if(g.debug) syslog(LOG_DEBUG,"CheckRawPath found [%s]\n",dp->d_name);
#endif
		if (strstr(dp->d_name,arg)) {
			strncpy(filename,dp->d_name,fsize-1);
			break;
		}
	}
	closedir(dirp);

	return(filename[0]?1:0);
}
/*********************************************************/
int SacrificeOldestFile(void)
{
	char victim[512];

	if(FindOldestFile(g.vfpath,victim,sizeof(victim)))
	{
		syslog(LOG_DEBUG,"Sacrificing [%s]\n",victim);
		unlink(victim);
	}
	return(victim[0]?1:0);
}
/*********************************************************/
int FindOldestFile(const char *dir,char *filename,size_t fsize)
{
	DIR *dirp;
	struct dirent *dp;
	struct stat st;
	time_t mtime;

	if(!(dirp=opendir(dir)))
	{
		perror(dir);
		return(0);
	}
	bzero(filename,fsize);
   time(&mtime);
	while((dp=readdir(dirp)))
   {
		if(dp->d_type!=DT_REG)continue;
#ifdef DEBUG
		if(g.debug) syslog(LOG_DEBUG,"FindOldestFile found [%s]\n",dp->d_name);
#endif
		stat(dp->d_name,&st);
		if(strstr(dp->d_name,DONESUFFIX)&&st.st_mtime<mtime)
		{
			mtime=st.st_mtime;
			strncpy(filename,dp->d_name,fsize-1);
		}
	}
	closedir(dirp);

	return(filename[0]?1:0);
}
/*********************************************************/
void BuildVCCmd(char* temp,size_t tempsize)
{
	int ret;
	char filename[512];
	char dynargs[512];
	char rot[16];
	char exp[32];
	char vs[6];
	long duration=0;

	bzero(&filename,sizeof(filename));
	bzero(&dynargs,sizeof(dynargs));
	bzero(&rot,sizeof(rot));
	bzero(&exp,sizeof(exp));
	bzero(&vs,sizeof(vs));
	ret=SetConfOptionNum(PRCSERIALFILENAME,"VFSerial",++g.vfserial);
#ifdef DEBUG
	if(ret==-1)
		if(g.debug) syslog(LOG_DEBUG,"could not write new serial number to config file\n");
#endif
	snprintf(filename,sizeof(filename)-1,"%s-%03ld-%s-%s-%s%s",g.vfnameyear,g.vfserial,g.vfnamelocation,g.vfnamecar,g.vfnamedriver,NEWRECSUFFIX);
	if(g.vcrotation)
		snprintf(rot,sizeof(rot)-1,"-rot %ld",g.vcrotation);
	if(g.vcexposure)
		snprintf(exp,sizeof(exp)-1,"-ex %s",g.vcexposure);
	if(g.vcvidstab)
		snprintf(vs,sizeof(vs)-1,"-vs");
	snprintf(dynargs,sizeof(dynargs)-1,"%s %s %s",vs,exp,rot);
	if(g.vfoverwrite)
		duration=g.vcmaxduration;
	else
	{
		syslog(LOG_DEBUG,"Overriding max length of video to [%ld]\n",g.minsavail);
		duration=g.minsavail;
	}
	duration*=60000;
	snprintf(temp,tempsize-1,"%s %s %s -t %ld -o %s%s",g.vccmd,g.vcargs,dynargs,duration,g.vcrawpath,filename);
	strncpy(g.recfilename,g.vcrawpath,sizeof(g.recfilename));
	strncat(g.recfilename,filename,sizeof(g.recfilename));
}
/*********************************************************/
void LedTrix(void)
{
	int x;
	for(x=50;x>1;x--)
	{
		usleep(250000/x);
		pifacedigital_write_bit(1,g.recstatled,OUTPUT,g.hw_addr);
		pifacedigital_write_bit(1,g.errstatled,OUTPUT,g.hw_addr);
		usleep(250000/x);
		pifacedigital_write_bit(0,g.recstatled,OUTPUT,g.hw_addr);
		pifacedigital_write_bit(0,g.errstatled,OUTPUT,g.hw_addr);
	}
}
/*********************************************************/
fsblkcnt_t GetFSSizeFromPathname(char *pathname,struct statfs *sfs)
{
	if(statfs(pathname,sfs)!=-1)
		return(sfs->f_bavail);
	return(-1);
}
/*********************************************************/
void FatalError(void)
{
	pifacedigital_open(g.hw_addr);
   pifacedigital_write_reg(0xfc,OUTPUT,g.hw_addr);
	while(1)
	{
		sync();
		sleep(60);
	}
}
/*********************************************************/
int GetConfOptionNumEx(FILE *in, char *optstr,long *value,long minvalue,long maxvalue,long defaultvalue)
{
   *value=-1;
   if(GetConfOptionNum(in,optstr,value)>=0)
   {
      if((*value<minvalue)||(*value>maxvalue))
      {
			if(*value<minvalue)
         	*value=minvalue;
			else
				*value=maxvalue;
#ifdef DEBUG
			if(g.debug) syslog(LOG_DEBUG,"Warning: Invalid value. %s reset to [%ld].\n",optstr,*value);
#endif
			return(-1);
      }
   }
   else
   {
      *value=defaultvalue;
#ifdef DEBUG
      if(g.debug) syslog(LOG_DEBUG,"Warning: Invalid value. %s reset to [%ld].\n",optstr,*value);
#endif
		return(-1);
   }
	return(0);
}
/*********************************************************/
int GetConfOptionStrEx(FILE *in,char *optstr,char *value,size_t valuelen,char *defaultvalue)
{
   if(GetConfOptionStr(in,optstr,value,valuelen)<0)
   {
      syslog(LOG_DEBUG,"Warning: Did not find %s in config file. Using [%s].\n",optstr,defaultvalue);
      strncpy(value,defaultvalue,valuelen-2);
		return(0);
   }
	return(-1);
}
/*********************************************************/
int LoadParms(void)
{
   FILE *fp;
	struct passwd *pp;


   fp=fopen(PRCCONFIGFILENAME,"r");
   if(!fp)
   {
      syslog(LOG_DEBUG,"Error: Unable to open config file [%s]!\n",PRCCONFIGFILENAME);
      return(-1);
   }
   GetConfOptionNumEx(fp,"VCAutoStart",&g.vcautostart,0,1,0);
   GetConfOptionNumEx(fp,"VCVidStab",&g.vcvidstab,0,1,0);
	GetConfOptionNumEx(fp,"WiFiFlag",&g.wififlag,0,1,1);
   GetConfOptionNumEx(fp,"VFOverwrite",&g.vfoverwrite,0,1,1);
	GetConfOptionNumEx(fp,"VCMaxDuration",&g.vcmaxduration,1,1000,60);
   GetConfOptionNumEx(fp,"VCStopDelay",&g.vcstopdelay,0,30,2);
   GetConfOptionStrEx(fp,"VFYear",g.vfnameyear,sizeof(g.vfnameyear),"2014");
   GetConfOptionStrEx(fp,"VFLocation",g.vfnamelocation,sizeof(g.vfnamelocation),"HomeTrack");
   GetConfOptionStrEx(fp,"VFCar",g.vfnamecar,sizeof(g.vfnamecar),"MyCar");
   GetConfOptionStrEx(fp,"VFSponsor",g.vfnamesponsor,sizeof(g.vfnamesponsor),"Self");
   GetConfOptionStrEx(fp,"VFSponsorFile",g.vfsponsorfile,sizeof(g.vfsponsorfile),"");
   GetConfOptionStrEx(fp,"VFDriver",g.vfnamedriver,sizeof(g.vfnamedriver),"Me");
   GetConfOptionStrEx(fp,"VCExposure",g.vcexposure,sizeof(g.vcexposure),"auto");
	fclose(fp);

   fp=fopen(PRCSERIALFILENAME,"r");
   if(!fp)
   {
      syslog(LOG_DEBUG,"Error: Unable to open config file [%s]!\n",PRCSERIALFILENAME);
      return(-1);
   }
   GetConfOptionNumEx(fp,"VFSerial",&g.vfserial,0,99999,1);
	fclose(fp);

   fp=fopen(PRCSPACEFILENAME,"r");
   if(!fp)
   {
      syslog(LOG_DEBUG,"Error: Unable to open config file [%s]!\n",PRCSPACEFILENAME);
      return(-1);
   }
	GetConfOptionNumEx(fp,"MinsAvailable",&g.daemon,0,9999,60);
	fclose(fp);

   fp=fopen(PRCDCONFIGFILENAME,"r");
   if(!fp)
   {
      syslog(LOG_DEBUG,"Error: Unable to open config file [%s]!\n",PRCDCONFIGFILENAME);
      return(-1);
   }
	GetConfOptionNumEx(fp,"HouseKeepingSync",&g.codecpause,0,1,0);
	GetConfOptionNumEx(fp,"HouseKeepingFreq",&g.housekeepingfreq,10,120,30);
	g.housekeepingfreq*=10; // multiply by to to get to seconds
   GetConfOptionStrEx(fp,"WiFiCtrlCommand",g.wificmd,sizeof(g.wificmd),"/opt/piracecam/bin/wireless-ctrl");
   GetConfOptionStrEx(fp,"UserDataUser",g.userdatauser,sizeof(g.userdatauser),"www-data");
   GetConfOptionNumEx(fp,"PRCDDebug",&g.debug,0,1,0);
   GetConfOptionNumEx(fp,"Sizeof1MinVidInBlocks",&g.sizeof1minvidinblocks,1000,100000,36000);
   GetConfOptionNumEx(fp,"RecordStatusLedBit",&g.recstatled,0,8,2);
   GetConfOptionNumEx(fp,"ErrorStatusLedBit",&g.errstatled,0,8,3);
	GetConfOptionNumEx(fp,"CodecPause",&g.codecpause,0,1,0);
	GetConfOptionNumEx(fp,"PRCDDaemon",&g.daemon,0,1,1);
   GetConfOptionStrEx(fp,"VidCamArguments",g.vcargs,sizeof(g.vcargs),"-n -pf main -b 5000000 -fps 30");
   GetConfOptionStrEx(fp,"VidCamCommand",g.vccmd,sizeof(g.vccmd),"/usr/bin/raspivid");
   GetConfOptionStrEx(fp,"VidCamKillCommand",g.vccamkillcmd,sizeof(g.vccamkillcmd),"/usr/bin/killall -9 raspivid");
   GetConfOptionStrEx(fp,"VidCamRawPath",g.vcrawpath,sizeof(g.vcrawpath),"/opt/piracecam/tmp/");
	GetConfOptionStrEx(fp,"CodecCommand",g.codeccmd,sizeof(g.codeccmd),"/usr/bin/MP4Box");
	GetConfOptionStrEx(fp,"CodecArguments",g.codecargs,sizeof(g.codecargs),"-brand mp42:0 -no-iod -par 1=1:1 -add");
	GetConfOptionStrEx(fp,"CodecNiceLevel",g.codecnicelevel,sizeof(g.codecnicelevel),"19");
	GetConfOptionStrEx(fp,"PrcdSigFile",g.prcdsigfile,sizeof(g.prcdsigfile),"/opt/piracecam/bin/piracecam.sig");
   GetConfOptionStrEx(fp,"VFPath",g.vfpath,sizeof(g.vfpath),"/opt/piracecam/userdata/");
	fclose(fp);

	pp=getpwnam(g.userdatauser);
	if(pp)
	{
		g.uduid=pp->pw_uid;
		g.udgid=pp->pw_gid;
	}

	syslog(LOG_DEBUG,"Loaded config file parmeteres.");
	return(0);
}
