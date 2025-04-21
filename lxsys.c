// Copyright (c) <2012> <Leif Asbrink>
//
// Permission is hereby granted, free of charge, to any pers5~on 
// obtaining a copy of this software and associated documentation 
// files (the "Software"), to deal in the Software without restriction, 
// including without limitation the rights to use, copy, modify, 
// merge, publish, distribute, sublicense, and/or sell copies of 
// the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be 
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
// OR OTHER DEALINGS IN THE SOFTWARE.

#include "osnum.h"
#if DARWIN == 1
#include <mach/machine.h>
#include <sys/dtrace.h>
#define TRUE_BSD FALSE
#else
#if (defined(__unix__) || defined(unix)) && !defined(USG)
#include <sys/param.h>
#endif
#ifdef BSD
#define TRUE_BSD TRUE
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#ifndef __NetBSD__
#include <machine/cpufunc.h>
#endif
#include <machine/sysarch.h>
#else
#define TRUE_BSD FALSE
#if HAVE_PARPORT == 1
#include <sys/io.h>
#endif
#endif
#endif

#if HAVE_SVGALIB == 1
#include <vgamouse.h>
#endif

#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <pthread.h>
#include <sched.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <ctype.h>
#include <sys/syscall.h>
#include <semaphore.h>
#if HAVE_OSS == 1
  #if OSSD == 1
  #include <sys/soundcard.h>
  #else
  #include <soundcard.h>
  #endif
#include <sys/ioctl.h>
#endif

#include "globdef.h"
#include "thrdef.h"
#include "uidef.h"
#include "xdef.h"
#include "hwaredef.h"
#include "options.h"
#include "sdrdef.h"
#include "lscreen.h"
#include "screendef.h"
#include "keyboard_def.h"

#if HAVE_SHM == 1
XShmSegmentInfo *shminfo;
#endif

p_clear_screen clear_screen;
p_lir_refresh_entire_screen lir_refresh_entire_screen;
p_lir_refresh_screen lir_refresh_screen;
p_lir_line lir_line;
p_lir_hline lir_hline;
p_lir_putbox lir_putbox;
p_lir_getbox lir_getbox;
p_lir_fillbox lir_fillbox;
p_lir_fix_bug lir_fix_bug;
p_lir_getpalettecolor lir_getpalettecolor;
p_lir_setpixel lir_setpixel;
p_lir_getpixel lir_getpixel;


void init_llir(void)
{
clear_screen=lclear_screen;
lir_refresh_entire_screen=llir_refresh_entire_screen;
lir_refresh_screen=llir_refresh_screen;
lir_line=llir_line;
lir_hline=llir_hline;
lir_putbox=llir_putbox;
lir_getbox=llir_getbox;
lir_fillbox=llir_fillbox;
lir_fix_bug=llir_fix_bug;
lir_getpalettecolor=llir_getpalettecolor;
lir_setpixel=llir_setpixel;
lir_getpixel=llir_getpixel;
}

void init_xlir(void)
{
clear_screen=xclear_screen;
lir_refresh_entire_screen=xlir_refresh_entire_screen;
lir_refresh_screen=xlir_refresh_screen;
lir_line=xlir_line;
lir_hline=xlir_hline;
lir_putbox=xlir_putbox;
lir_getbox=xlir_getbox;
lir_fillbox=xlir_fillbox;
lir_fix_bug=xlir_fix_bug;
lir_getpalettecolor=xlir_getpalettecolor;
lir_setpixel=xlir_setpixel;
}

void init_flir(void)
{
clear_screen=fclear_screen;
lir_refresh_entire_screen=llir_refresh_entire_screen;
lir_refresh_screen=llir_refresh_screen;
lir_line=flir_line;
lir_hline=flir_hline;
lir_putbox=flir_putbox;
lir_getbox=flir_getbox;
lir_fillbox=flir_fillbox;
lir_fix_bug=llir_fix_bug;
lir_getpalettecolor=flir_getpalettecolor;
lir_setpixel=flir_setpixel;
lir_getpixel=flir_getpixel;
}

int open_USB2LPT(void);

struct termios old_options;
#if TRUE_BSD == TRUE
char serport_name[]="/dev/ttyu....";
#else
char serport_name[]="/dev/ttyS....?";
#endif

void print_procerr(int xxprint)
{
int i;
if( (xxprint&1) != 0)
  {
  i=0;
  if( (xxprint&2)==0)
    {
    i=1;
    printf("\nMMX supported by hardware");
    }
  if( (xxprint&4)==0 && simd_present != 0)
    {
    i=1;
    printf("\nSIMD (=sse) supported by hardware");
    }
  if(i!=0)
    {
    printf("\n/proc/cpuinfo says LINUX core is not compatible.");
    printf("\n\nLinrad will allow you to select routines that may be illegal");
    printf("\non your system.");
    printf("\nAny illegal instruction will cause a program crasch!!");
    printf("\nSeems just the sse flag in /proc/cpuinfo is missing.");
    printf("\nTo use fast routines with an older core, recompile LINUX");
    printf("\nwith appropriate patches.\n\n");
    }
  }
}

void mmxerr(void)
{
printf("\n\nCould not read file /proc/cpuinfo (flags:)");
printf("\nSetting MMX and SIMD flags from hardware");
printf("\nProgram may fail if kernel support is missing and modes"); 
printf("\nneeding MMX or SIMD are selected.");
printf("\n\n%s",press_enter);
fflush(stdout);
getchar();
}

int investigate_cpu(void)
{
#if CPU == CPU_INTEL
  int i;
#else
  #if TRUE_BSD == FALSE
    int i;
  #endif
#endif
#if TRUE_BSD == FALSE
int k, maxproc, maxproc_flag;
FILE *file;
char s[256];
char *flags="flags";
char *fmmx=" mmx";
char *fsse=" sse";
char *fht=" ht";
char *fprocessor="processor";
int no_of_ht;
#endif
int xxprint;
// If there is no mmx, do not use simd either.
tickspersec = sysconf(_SC_CLK_TCK);
xxprint=0;
#if CPU == CPU_INTEL
#if IA64 == 0
i=check_mmx();
#else
// We do not use assembly on 64 bit systems (yet??)
i=0;
i=2; // simd is OK, but not MMX.
#endif
mmx_present=i&1;
simd_present=i>>1;
#else
mmx_present=0;
simd_present=0;
#endif
#if TRUE_BSD == TRUE
size_t len=sizeof(no_of_processors);
int err;
if((err=sysctlbyname("hw.ncpu", &no_of_processors, 
                               &len, NULL, 0)) < 0)no_of_processors=1;
#else
no_of_processors=1;
no_of_ht=0;
maxproc=0;
maxproc_flag=0;
file = fopen("/proc/cpuinfo", "r");
if (file == NULL)
  {
  mmxerr();
  return 0;
  }
else
  {
  no_of_processors=0;
nxline:;
  if(fgets(s,256,file)==NULL)
    {
    if(no_of_processors>0)goto cpuinfo_ok;
    fclose(file);
#if CPU == CPU_INTEL
    mmxerr();
#endif
    no_of_processors=1;
    return 0;
    }
  else
    {
    for(i=0; i<9; i++) if(s[i]!=fprocessor[i])goto nxt; 
    k=8;
    while(s[k+1] == ' ' || s[k+1] == ':' || s[k+1] == 9)k++;
    i=atoi(&s[k]);
    if(i != no_of_processors)maxproc_flag++;
    no_of_processors++;
    if(i > maxproc)maxproc=i;
nxt:;
    k=0;
    while(s[k] != flags[0] && k < 10)k++;
    for(i=0; i<5; i++) if(s[i+k]!=flags[i])goto nxline; 
    k+=5;  
nxbln:      
    while(s[k]!=0 && s[k]!=' ')k++;
    while(s[k]!=0 && s[k]==' ')k++;
    if(s[k]!=0)
      {
      k--;
      for(i=0; i<4; i++)if(s[i+k]!=fmmx[i])goto notmmx;
      if(s[k+4] != ' ' && s[k+4] != 10)goto notmmx;
      xxprint|=2;
notmmx:;      
      for(i=0; i<4; i++)if(s[i+k]!=fsse[i])goto notsse;
      if(s[k+4] != ' ' && s[k+4] != 10)goto notsse;
      xxprint|=4;
notsse:;
      for(i=0; i<3; i++)if(s[i+k]!=fht[i])goto notht;
      if(s[k+3] != ' ' && s[k+3] != 10)goto notht;
      no_of_ht++;
notht:;
      k++;
      goto nxbln;
      }      
    xxprint|=1;
    }
  goto nxline;
cpuinfo_ok:;      
  fclose(file);
  }
if(maxproc_flag != 0)
  {
// If something went wrong, better underestimate the number of CPUs.
  maxproc++;
  if(no_of_processors > maxproc)no_of_processors=maxproc;
  }
if(no_of_processors == no_of_ht)
  {
// This is incorrect. In e.g. Ubuntu Studio 13.10 the hyperthreaded
// cores are not listed, but nevertheless the ht flag is set.
// In case it turns out to be interesting to control cpu affinities
// it will be necessary do something else here.
// Look for 'core id' and 'cpu cores'
  hyperthread_flag=TRUE;
  }
else
  {
  hyperthread_flag=FALSE;
  }  
#endif
return xxprint;
}


void lir_system_times(double *cpu_time, double *total_time)
{
cpu_time[0]=lir_get_cpu_time();
total_time[0]=current_time();
}

void clear_thread_times(int no)
{
#if TRUE_BSD == TRUE 
thread_pid[no]=no;
#else
#if DARWIN == 0
thread_pid[no]=syscall(SYS_gettid);
#else
uint64_t thread_id;
pthread_threadid_np(NULL, &thread_id);
thread_pid[no]=(int)thread_id;
#endif
#endif
thread_tottim1[no]=current_time();
thread_cputim1[no]=lir_get_cpu_time();
thread_workload[no]=0;
}

void make_thread_times(int no)
{
float t1,t2;
thread_tottim2[no]=current_time();
thread_cputim2[no]=lir_get_thread_time(no);
t1=100*(thread_cputim2[no]-thread_cputim1[no]);
t2=thread_tottim2[no]-thread_tottim1[no];
if(t1>0 && t2>0)
  {
  thread_workload[no]=t1/t2;
  }
else
  {
  thread_workload[no]=0;
  }
}

double lir_get_cpu_time(void)
{
double tm;
struct rusage rudat;
getrusage(RUSAGE_SELF,&rudat);
tm=0.000001*(rudat.ru_utime.tv_usec + rudat.ru_stime.tv_usec)+ 
                    rudat.ru_utime.tv_sec + rudat.ru_stime.tv_sec;
return tm;
}

double lir_get_thread_time(int no)
{
#if TRUE_BSD == TRUE 
(void) no;
return 0.0;
#else
char fnam[80], info[512];
FILE *pidstat;
int j,k,m;
long long int ii1, ii2;
sprintf(fnam,"/proc/%d/task/%d/stat",thread_pid[no],thread_pid[no]);
pidstat=fopen(fnam,"r");
if(pidstat==NULL)return 0;
#if VALGRIND == TRUE
memset(info, 0, 512);
#endif
m=fread(info,1,512,pidstat);  
fclose(pidstat);  
j=0;
k=0;
while(k<13)
  {
  while(info[j]!= ' ' && j<m)j++;
  while(info[j]== ' ' && j<m)j++;
  k++;
  }
if(j>=m)return 0;
sscanf(&info[j], "%lld %lld", &ii1, &ii2);
return (double)(ii1+ii2)/tickspersec;
#endif
}

void lir_init_event(int i)
{
pthread_mutex_init(&lir_event_mutex[i],NULL);
lir_event_flag[i]=FALSE;
pthread_cond_init(&lir_event_cond[i], NULL);
}

void lir_close_event(int i)
{
pthread_mutex_destroy(&lir_event_mutex[i]);
pthread_cond_destroy(&lir_event_cond[i]);
}  


void lir_set_event(int no)
{
while(pthread_mutex_lock(&lir_event_mutex[no]) != 0)lir_sleep(100);
lir_event_flag[no]=TRUE;
pthread_cond_signal(&lir_event_cond[no]);
pthread_mutex_unlock(&lir_event_mutex[no]);
//öö lir_event_flag[no]=FALSE;
}    

void lir_await_event(int no)
{
while(pthread_mutex_lock(&lir_event_mutex[no]) != 0)lir_sleep(10);
if(lir_event_flag[no] != TRUE)
  {
  pthread_cond_wait(&lir_event_cond[no], &lir_event_mutex[no]);
  }
lir_event_flag[no]=FALSE;
while(pthread_mutex_unlock(&lir_event_mutex[no]) != 0)lir_sleep(10);
}

float lir_noisegen(int level) 
{
// Return a number distributed following a gaussian
// Mean value is 0 and sigma pow(2,level)
double y, z;
int64_t k;
k=random();
y = (double)k/(double)RAND_MAX; 
k=random();
z = (double)k/(double)RAND_MAX; 
return sin(z*2*PI_L)*sqrt(-2*log(y))*pow(2.0,0.5*(double)level);
}

void lir_mutex_init(void)
{
int i;
for(i=0; i<MAX_LIRMUTEX; i++)
  {
  pthread_mutex_init(&linux_mutex[i],NULL);
  }
}

void lir_mutex_destroy(void)
{
int i;
for(i=0; i<MAX_LIRMUTEX; i++)
  {
  pthread_mutex_destroy(&linux_mutex[i]);
  }
}

void lir_mutex_lock(int no)
{
lir_sched_yield();

pthread_mutex_lock(&linux_mutex[no]);
lir_sched_yield();
}

void lir_mutex_unlock(int no)
{
lir_sched_yield();
pthread_mutex_unlock(&linux_mutex[no]);
lir_sched_yield();
}

void lirerr(int errcod)
{
fprintf(STDERR,"\nlir error %d",errcod);
if(kill_all_flag) return;
DEB"\nlirerr(%d)",errcod);
if(dmp != 0)fflush( dmp);
lir_errcod=errcod;
lir_set_event(EVENT_KILL_ALL);
fflush(NULL);
while(!kill_all_flag)lir_sleep(10000);
}

int lir_open_serport(int serport_number, int baudrate,int stopbit_flag, int rts_mode)
{
int rte;
int rc;
struct termios options;
rc=0;
if(serport != -1)return rc ;
if(serport_number < 1 || serport_number > 8)
  {
  rc=1279;
  return rc;
  }
if(serport_number <= 4)
  {
  sprintf(&serport_name[0],"%s","/dev/ttyS");
  sprintf(&serport_name[9],"%d",serport_number-1);
  }
else
  {
  sprintf(&serport_name[0],"%s","/dev/ttyUSB");
  sprintf(&serport_name[11],"%d",serport_number-5);
  }
serport=open(serport_name,O_RDWR  | O_NOCTTY | O_NDELAY);
if (serport == -1)
  {
  rc=1244;
  return rc;
  }
fcntl(serport,F_SETFL,0);           //blocking I/O
//fcntl(serport, F_SETFL, FNDELAY);     //non blocking I/O
if(tcgetattr(serport,&options) != 0)
  {
  rc=1277;
  goto close_x;
  }
switch ( baudrate )
  {
  case 110: 
  rte=B110;
  break;
  
  case 300: 
  rte=B300;
  break;
  
  case 600: 
  rte=B600;
  break;
  
  case 1200: 
  rte=B1200;
  break;
  
  case 2400: 
  rte=B2400;
  break;
  
  case 4800: 
  rte=B4800;
  break;
  
  case 9600: 
  rte=B9600;
  break;
  
  case 19200: 
  rte=B19200;
  break;
  
  case 38400: 
  rte=B38400;
  break;
  
  case 57600: 
  rte=B57600;
  break;

  default: 
  rc=1280;
  goto close_x;
  }
old_options=options;
cfsetispeed(&options,rte);
cfsetospeed(&options,rte);
//CLOCAL means don’t allow
//control of the port to be changed
//CREAD says to enable the receiver
options.c_cflag|= (CLOCAL | CREAD);
// no parity, 2 or 1 stopbits, 8 bits per word
options.c_cflag&= ~PARENB; //  no parity=>disable the "enable parity bit" PARENB
if(stopbit_flag)
  {
  options.c_cflag|= CSTOPB;// =>2 stopbits
  }
else
  {
  options.c_cflag&= ~CSTOPB;//=> 1 stopbit   
  }
options.c_cflag&= ~CSIZE;   // clear size-bit by anding with negation
options.c_cflag|= CS8;      // =>set size to 8 bits per word
// raw input /output
options.c_oflag &= ~OPOST; 
options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
//Set the timeouts:
//VMIN is the minimum amount characters to read
options.c_cc[VMIN]=0;
//The amount of time to wait
//for the amount of data
//specified by VMIN in tenths
//of a second.
options.c_cc[VTIME] = 1;
//  Select  flow control  mode
switch (rts_mode)
  {
  case 0:
  options.c_cflag &= ~CRTSCTS;       // no handshaking
  options.c_iflag &= ~IXON;
  break;
  
  case 1:
  options.c_cflag &= ~CRTSCTS;       // Enable Xon/Xoff software handshaking 
  options.c_iflag |= IXON;		
  break;
  
  case 2:
  options.c_cflag |= CRTSCTS;        // Enable Hardware RTS handshaking 
  options.c_iflag &= ~IXON;
  break;
  }
//flush I/O  buffers and apply the settings
if(tcsetattr(serport, TCSAFLUSH,&options) != 0) 
  {
  rc=1278;
close_x:  
  close(serport);
  serport=-1;
  }
return rc;
}

void lir_close_serport(void)
{
if(serport == -1)return;
if(tcsetattr(serport, TCSAFLUSH,&old_options) != 0)lirerr(1278);
close(serport);
serport=-1;
}

int lir_write_serport(void *s, int bytes)
{
return write(serport,s,bytes);
}

int lir_read_serport(void *s, int bytes)
{
int retnum, nread = 0;
while (bytes > 0) {
	retnum = read (serport, (char *)(s + nread), bytes);
	if (retnum <= 0) return -1;
	nread += retnum;
	bytes -= retnum;
	}	
return nread;
}

int lir_parport_permission(int portno)
{
int i;
// If the user selected USB2LPT16_PORT_NUMBER we will use USB for
// the 25 pin parallel port. It may not be permitted in case
// the user does not have privileges.
// verify that we can open the device.
if (wse.parport == USB2LPT16_PORT_NUMBER)
  {
  if(!usb2lpt_flag)usb2lpt_flag=open_USB2LPT();
  return usb2lpt_flag;
  }
#if DARWIN == 0
// Get permission to write to the parallel port
#ifdef BSD
#ifdef i386_set_ioperm
i=i386_set_ioperm(portno,4,1);
#else
i=portno;
return FALSE;
#endif
#else
i=portno;
#if HAVE_PARPORT == 1
if(portno < 0x400-4)
  {
  i=ioperm(portno,4,1);
  }
else
  {
  i=iopl(3);
  }
#endif
#endif  
if(i != 0)
  {
  lir_text(2,2,"Access to parallel port denied.");
  lir_text(2,4,"Use sudo or run as root");
  lir_text(2,6,press_any_key);
  await_keyboard();
  clear_screen();
  return FALSE;
  }
return TRUE;
#else
i=portno;
i=FALSE;
return i;
#endif
}

void lir_sched_yield(void)
{
sched_yield();
}

void win_global_uiparms(int n)
{
(void) n;
// Dummy routine. Not used under Linux.
}

void linrad_thread_create(int no)
{
thread_status_flag[no]=THRFLAG_INIT;
thread_command_flag[no]=THRFLAG_ACTIVE;
lir_sched_yield();
lir_sleep(200000);
pthread_create(&thread_identifier[no],NULL,(void*)thread_routine[no], NULL);
threads_running=TRUE;
lir_sched_yield();
}

void fix_prio(int thread)
{
#if OSNUM == OSNUM_LINUX
int policy;
int i, k;
if(ui.process_priority != 0)
  {
  struct sched_param parms;
  pthread_getschedparam(thread_identifier[thread],&policy,&parms); 
  policy=SCHED_FIFO;
  i=sched_get_priority_max(policy);
  switch (ui.process_priority)
    {
    case 3:
    k=i-1;
    break;
    
    case 2:
    k=(i+parms.sched_priority)/2;
    break;
    
    default:
    k=parms.sched_priority+1;
    if(k > i) k=i;
    }
  parms.sched_priority=k;
  pthread_setschedparam(thread_identifier[thread],policy,&parms); 
  }
#endif
}


void thread_rx_output(void)
{
fix_prio(THREAD_RX_OUTPUT);
rx_output();
//öÖthread_status_flag[THREAD_RX_OUTPUT]=THRFLAG_RETURNED;
}

void thread_kill_all(void)
{
// Wait until the event is set.
// Then stop all processing threads so main can write any
// error code/message and exit.
thread_status_flag[THREAD_KILL_ALL]=THRFLAG_ACTIVE;
lir_await_event(EVENT_KILL_ALL);
thread_status_flag[THREAD_KILL_ALL]=THRFLAG_NOT_ACTIVE;
kill_all();
}

double current_time(void)
{
struct timeval t;
gettimeofday(&t,NULL);
recent_time=0.000001*t.tv_usec+t.tv_sec;
return recent_time;
}

int ms_since_midnight(void)
{
int i;
double dt1;
dt1=current_time();
i=dt1/(24*3600);
dt1-=24*3600*i;
i=1000*dt1;
return i%(24*3600000);
}

void lir_sync(void)
{
// This routine is called to force a write to the hard disk
sync();
}


int lir_get_epoch_seconds(void)
{
struct timeval tim;
gettimeofday(&tim,NULL);
return tim.tv_sec;
}

void lir_join(int no)
{
lir_sched_yield();
if(thread_command_flag[no]==THRFLAG_NOT_ACTIVE)return;
pthread_join(thread_identifier[no],0);
thread_status_flag[no]=THRFLAG_NOT_ACTIVE;
}

void lir_sleep(int us)
{
usleep(us);
}

void lir_outb(char byte, int port)
{    
int i;
if(libusb1_library_flag == TRUE || libusb0_library_flag == TRUE) 
  {
  if(wse.parport==USB2LPT16_PORT_NUMBER)
    {
    out_USB2LPT(byte,(port-wse.parport));
    }
  }
else
  {
#if DARWIN == 0 
#ifdef BSD
#ifdef i386_set_ioperm
  i=i386_set_ioperm(port,4,1);
#else
  return;
#endif  
#else
i=0;
#if HAVE_PARPORT == 1
  if(port < 0x400-4)
    {
    i=ioperm(port,4,1);
    }
  else
    {
    i=iopl(3);
    }
#endif
#endif  
  if(i!=0)lirerr(764921);
#if TRUE_BSD == TRUE
#ifndef __NetBSD__
    {
    int k;
    k=byte;
    outb(k,port);
    }
#endif
#else
#if HAVE_PARPORT == 1
  outb(byte,port);
#endif
#endif
#else
i=byte+port;
#endif
  }
i=1000;
while(i>0)i--;
}

char lir_inb(int port)
{
#if DARWIN == 0
int i;
#endif
if(wse.parport==USB2LPT16_PORT_NUMBER)
  {
  if(libusb1_library_flag == TRUE || libusb0_library_flag == TRUE) 
    {
    return in_USB2LPT((port-wse.parport));
    }
  else
    {
    lirerr(7211332);
    return 0;
    }
  }
else
  {
#if TRUE_BSD == TRUE
#ifdef i386_set_ioperm
  i=i386_set_ioperm(port,4,1);
#else
  return 0;
#endif
#else
#if DARWIN == 0
i=port;
#if HAVE_PARPORT == 1
  if(port < 0x400-4)
    {
    i=ioperm(port,4,1);
    }
  else
    {
    i=iopl(3);
    }
#endif
#endif   
#endif
#if DARWIN == 0
if(i!=0)lirerr(764921);
#if HAVE_PARPORT == 1
return inb(port);
#endif
#endif
  }  
return port;
}

void restore_behind_mouse(void)
{
int i,imax;
imax=screen_width-mouse_x;
if(imax > mouse_cursize) imax=mouse_cursize;
for(i=1; i<imax; i++)
  {
  lir_setpixel(mouse_x+i,mouse_y,behind_mouse[4*i  ]);
  }
imax=mouse_x;
if(imax > mouse_cursize) imax=mouse_cursize;
for(i=1; i<imax; i++)
  {
  lir_setpixel(mouse_x-i,mouse_y,behind_mouse[4*i+1]);
  }
imax=screen_height-mouse_y;
if(imax > mouse_cursize) imax=mouse_cursize;
for(i=1; i<imax; i++)
  {
  lir_setpixel(mouse_x,mouse_y+i,behind_mouse[4*i+2]);
  }
imax=mouse_y;
if(imax > mouse_cursize) imax=mouse_cursize;
for(i=1; i<imax; i++)
  {
  lir_setpixel(mouse_x,mouse_y-i,behind_mouse[4*i+3]);
  }
lir_setpixel(mouse_x,mouse_y,behind_mouse[0]);
mouse_hide_flag=0;
}


void hide_mouse(int x1,int x2,int iy1,int y2)
{
if( (screen_type&(SCREEN_TYPE_X11+SCREEN_TYPE_X11_SHM)))return;
if(mouse_hide_flag ==0)return;
if(mouse_xmax < x1)return;
if(mouse_xmin > x2)return;
if(mouse_ymax < iy1)return;
if(mouse_ymin > y2)return;
restore_behind_mouse();
}

void unconditional_hide_mouse(void)
{
if( (screen_type&(SCREEN_TYPE_X11+SCREEN_TYPE_X11_SHM)))return;
if(mouse_hide_flag ==0)return;
restore_behind_mouse();
}

void show_mouse(void)
{
int i,imax;
unsigned char color;
if( (screen_type&(SCREEN_TYPE_X11+SCREEN_TYPE_X11_SHM)))return;
if(mouse_hide_flag == 1)return;
mouse_hide_flag=1;
color=15-leftpressed;
imax=screen_width-mouse_x;
if(imax > mouse_cursize) imax=mouse_cursize;
for(i=1; i<imax; i++)
  {
  behind_mouse[4*i  ]=lir_getpixel(mouse_x+i,mouse_y);
  lir_setpixel(mouse_x+i,mouse_y,color);
  }
imax=mouse_x;
if(imax > mouse_cursize) imax=mouse_cursize;
for(i=1; i<imax; i++)
  {
  behind_mouse[4*i+1]=lir_getpixel(mouse_x-i,mouse_y);
  lir_setpixel(mouse_x-i,mouse_y,color);
  }
imax=screen_height-mouse_y;
if(imax > mouse_cursize) imax=mouse_cursize;
for(i=1; i<imax; i++)
  {
  behind_mouse[4*i+2]=lir_getpixel(mouse_x,mouse_y+i);
  lir_setpixel(mouse_x,mouse_y+i,color);
  }
imax=mouse_y;
if(imax > mouse_cursize) imax=mouse_cursize;
for(i=1; i<imax; i++)
  {
  behind_mouse[4*i+3]=lir_getpixel(mouse_x,mouse_y-i);
  lir_setpixel(mouse_x,mouse_y-i,color);
  }
behind_mouse[0]=lir_getpixel(mouse_x,mouse_y);
lir_setpixel(mouse_x,mouse_y,color);
}

void lir_move_mouse_cursor(void)
{
if( (screen_type&(SCREEN_TYPE_X11+SCREEN_TYPE_X11_SHM)))
  {
  set_button_coordinates();
  return;
  }
if(mouse_hide_flag !=0)restore_behind_mouse();
set_button_coordinates();
mouse_xmax=mouse_x+mouse_cursize;
mouse_xmin=mouse_x-mouse_cursize;
mouse_ymax=mouse_y+mouse_cursize;
mouse_ymin=mouse_y-mouse_cursize;
}

int force_getchar(void)
{
// Get a character from keyboard, but set a 0.1 s timeout
// and return -1 if no key was pressed.
struct termios term, oterm;
int c = 0;
// get the terminal settings 
tcgetattr(0, &oterm);
// get a copy of the settings, which we modify 
memcpy(&term, &oterm, sizeof(term));
// put the terminal in non-canonical mode, any 
// reads timeout after 0.1 seconds or when a 
// single character is read
term.c_lflag = term.c_lflag & (!ICANON);
term.c_cc[VMIN] = 0;
term.c_cc[VTIME] = 1;
tcsetattr(0, TCSANOW, &term);
// get input - timeout after 0.1 seconds or
// when one character is read. If timed out
// getchar() returns -1, otherwise it returns
// the character
c=getchar();
//   reset the terminal to original state 
tcsetattr(0, TCSANOW, &oterm);
return c;
}


void thread_keyboard(void)
{
int c, i, k;
char s[80];
pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,&i);
while(1)
  {
// It is silly to use force_getchar here. 
// We should set up stdin so it will return on each char of an ESC sequence.
// so we would not have to loop around at 10 Hz here.
  c=getchar();
  keyboard_buffer[keyboard_buffer_ptr]=c;
  if(c == 27)
    {
// Set the flag and wait a while to make it unlikely
// it will not be visible for the kill_all thread.
// We must not kill the keyboard thread while force_getchar() is
// executed because the changed terminal setting will survive
// after Linrad is finished and make the command line behave
// incorrectly.
      {
      c=force_getchar();
      if(c == -1)
        {
// The ESC key was pressed. Exit from Linrad NOW!    
        DEB"\nESC pressed");
        if(ui.operator_skil == OPERATOR_SKIL_NEWCOMER && !kill_all_flag)
          {
          newcomer_escpress(0);
newco_esc:;          
          while(c != 'Y' && c != 'N')c=to_upper(getchar());
          if(c=='N')
            {
            while(c!=-1)c=force_getchar();
            newcomer_escpress(1);
            goto next;
            }
          if(c != 'Y')goto newco_esc;  
          }
        lir_set_event(EVENT_KILL_ALL);
        while(!kill_all_flag)lir_sleep(3000);
check_threads:;
        k=0; 
        for(i=0; i<THREAD_MAX; i++)    
          {
          if(i != THREAD_SYSCALL && 
             thread_command_flag[i] != THRFLAG_NOT_ACTIVE &&
             thread_waitsem[i] == -2)
            {
            k++;
            }   
          }
        if(k != 0)
          {
          lir_sleep(30000);
          goto check_threads;
          }  
        keyboard_buffer[keyboard_buffer_ptr]=0;
        keyboard_buffer_ptr=(keyboard_buffer_ptr+1)&
                                                  (KEYBOARD_BUFFER_SIZE-1);
        lir_sleep(10000);  
        lir_set_event(EVENT_KEYBOARD);
        while(1)
          {
          lir_sleep(30000);
          }
        return;
        }    
// The ESC sequences are all [27],[91],.....
      if(c == 91)
        {
        c=force_getchar();
// Arrow keys are:
// ARROW_UP=65
// ARROW_DOWN=66
// ARROW_RIGHT=67
// ARROW_LEFT=68
        if(c >= 65 && c <= 68)
          {
          c+=ARROW_UP_KEY-65;
          goto fkn_key_ok;
          }
        if(c >= 49 && c <= 54)
          {
          i=force_getchar();
// These keys have to be followed by 126
// HOME=49
// INSERT=50
// DELETE=51
// END=52
// PAGE_UP=53
// PAGE_DOWN=54
          if(i == 126)
            {
            c+=HOME_KEY-49;
            goto fkn_key_ok;
            }
          k=force_getchar();
          if(k == 126)
            {  
            if(c == 49)
              {
// F6=55
// F7=56
// F8=57
              if(i >= 55 && i <= 57)
                {
                c=i+F6_KEY-55;
                goto fkn_key_ok;
                }
              }  
            if(c == 50)
              {
// F9=48
// F10=49
// F10_PAD=50
// F11=51
// F12=52
// SHIFT_F1=53
// SHIFT_F2=54
// SHIFT_F2_PAD=55
// SHIFT_F3=56
// SHIFT_F4=57
              if(i >= 48 && i <= 57)
                { 
                c=i+F9_KEY-48;
                goto fkn_key_ok;
                }
              }    
            if(c == 51)
              {
// SHIFT_F5=49
// SHIFT_F6=50
// SHIFT_F7=51
// SHIFT_F8=52
             if(i >= 49 && i <= 52)
                { 
                c=i+SHIFT_F5_KEY-49;
                goto fkn_key_ok;
                }
              }
            }    
          sprintf(s,"ESC sequence):[27][91][%d][%d][%d]",c,i,k);
          goto skip;
          }
// PAUSE = 80
        if(c == 80)
          {
          c=PAUSE_KEY;
          goto fkn_key_ok;
          }
        if(c == 91)
          {

          c=force_getchar();
// F1=65
// F2=66
// F3=67
// F4=68
// F5=69
          if(c >= 65 && c <= 69)
            {
            c+=F1_KEY-65;
            goto fkn_key_ok;
            }
          sprintf(s,"ESC sequence:[27][91][91][%d]",c);
          goto skip;
          }
        sprintf(s,"ESC sequence:[27][91][%d]",c);
        goto skip;
        }
      if(c >= 'a' && c <= 'z')c=toupper(c);  
      if(c >= 'A' && c <= 'Z')
        {
        c=c-'A'+ALT_A_KEY;
        goto fkn_key_ok;
        }        
// ESC followed by something else than 91 or ALT keys)
      sprintf(s,"ESC sequence:[27][%d]",c);
skip:;
      lir_text(1,5,s);
      c=force_getchar();
      if(c != -1)
        {
        i=0;
        while(s[i]!=0 && i<70)i++;
        sprintf(&s[i],"[%d]",c);
        goto skip;
        }
      if(c == -1) goto next;
fkn_key_ok:;
      keyboard_buffer[keyboard_buffer_ptr]=c;
      }
    }
  keyboard_buffer_ptr=(keyboard_buffer_ptr+1)&(KEYBOARD_BUFFER_SIZE-1);
  lir_sleep(10000);  
  lir_set_event(EVENT_KEYBOARD);
next:;
  }
}

void lthread_mouse(void)
{
#if HAVE_SVGALIB == 1
int i, m, button, rx;
pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,&i);
while(TRUE)
  {
// **********************************************
// Wait for svgalib to report a mouse event
  mouse_waitforupdate();
// We do nothing unless mouse_flag is set.
  if(mouse_flag != 0)
    {
    button= mouse_getbutton();
    mouse_getposition_6d(&new_mouse_x, &new_mouse_y, NULL, &rx, NULL, NULL);
    if(rx!=0)
      {
      if (button & MOUSE_MIDDLEBUTTON) 
        {
        m=bg.wheel_stepn;
        if(rx > 0)
          {
          m++;
          if(m>30)m=30;
          }
        else
          {
          m--;
          if(m<-30)m=-30;
          if((genparm[AFC_ENABLE]==0 || 
                                   genparm[AFC_LOCK_RANGE] == 0) && m<0)m=0;
          }
        bg.wheel_stepn=m;
        sc[SC_SHOW_WHEEL]++;
        make_modepar_file(GRAPHTYPE_BG);
        }
      else
        {
        if(rx > 0)
          {
          step_rx_frequency(1);
          }
        else
          {
          step_rx_frequency(-1);
          }  
        }
      }
    if (button & MOUSE_LEFTBUTTON) 
      {
      new_lbutton_state=1;
      }
    else
      {
      new_lbutton_state=0;
      }
    if (button & MOUSE_RIGHTBUTTON) 
      {
      new_rbutton_state=1;
      }
    else
      {
      new_rbutton_state=0;
      }
    if( thread_status_flag[THREAD_SCREEN] == THRFLAG_SEM_WAIT ||
        thread_status_flag[THREAD_SCREEN] == THRFLAG_ACTIVE ||
        thread_status_flag[THREAD_SCREEN] == THRFLAG_IDLE) 
      {
      if( new_mouse_x!=mouse_x || new_mouse_y!=mouse_y)
        {
        awake_screen();
        }
      check_mouse_actions();
      }
    }
  }
#endif
}




// *********************************************************************
// Thread entries for Linux
// *********************************************************************

void thread_main_menu(void)
{
main_menu();
}

void thread_tune(void)
{
tune();
}

void thread_sdr14_input(void)
{
fix_prio(THREAD_SDR14_INPUT);
sdr14_input();
}

void thread_radar(void)
{
fix_prio(THREAD_RADAR);
run_radar();
}

void thread_blocking_rxout(void)
{
fix_prio(THREAD_BLOCKING_RXOUT);
blocking_rxout();
thread_status_flag[THREAD_BLOCKING_RXOUT]=THRFLAG_RETURNED;
}

void thread_mix2(void)
{
fix_prio(THREAD_MIX2);
do_mix2();
}

void thread_fft3(void)
{
fix_prio(THREAD_FFT3);
do_fft3();
}

void thread_write_raw_file(void)
{
write_raw_file();
}

void thread_perseus_input(void)
{
input_wait_flag=TRUE;
lir_sched_yield();
fix_prio(THREAD_PERSEUS_INPUT);
perseus_input();
}

void thread_rtl2832_input(void)
{
fix_prio(THREAD_RTL2832_INPUT);
rtl2832_input();
}

void thread_mirics_input(void)
{
fix_prio(THREAD_MIRISDR_INPUT);
mirics_input();
}

void thread_sdrplay2_input(void)
{
  fix_prio(THREAD_SDRPLAY2_INPUT);
  sdrplay2_input();
}

void thread_sdrplay3_input(void)
{
  fix_prio(THREAD_SDRPLAY3_INPUT);
  sdrplay3_input();
}

void thread_bladerf_input(void)
{
fix_prio(THREAD_BLADERF_INPUT);
bladerf_input();
}

void thread_pcie9842_input(void)
{
fix_prio(THREAD_PCIE9842_INPUT);
pcie9842_input();
}

void thread_openhpsdr_input(void)
{
fix_prio(THREAD_OPENHPSDR_INPUT);
openhpsdr_input();
}

void thread_hware_command(void)
{
hware_command();
}

void thread_sdrip_input(void)
{
fix_prio(THREAD_SDRIP_INPUT);
sdrip_input();
}

void thread_cloudiq_input(void)
{
fix_prio(THREAD_CLOUDIQ_INPUT);
cloudiq_input();
}

void thread_netafedri_input(void)
{
fix_prio(THREAD_NETAFEDRI_INPUT);
netafedri_input();
}

void thread_excalibur_input(void)
{
fix_prio(THREAD_EXCALIBUR_INPUT);
excalibur_input();
}

void thread_cal_filtercorr(void)
{
cal_filtercorr();
}

void thread_cal_interval(void)
{
do_cal_interval();
}

void thread_user_command(void)
{
user_command();
}

void thread_narrowband_dsp(void)
{
fix_prio(THREAD_NARROWBAND_DSP);
narrowband_dsp();
}

void thread_do_fft1c(void)
{
fix_prio(THREAD_DO_FFT1C);
do_fft1c();
}

void thread_fft1b(void)
{
do_fft1b();
}

void thread_wideband_dsp(void)
{
fix_prio(THREAD_WIDEBAND_DSP);
wideband_dsp();
}

void thread_second_fft(void)
{
fix_prio(THREAD_SECOND_FFT);
second_fft();
}

void thread_timf2(void)
{
fix_prio(THREAD_TIMF2);
timf2_routine();
}

void thread_tx_input(void)
{
fix_prio(THREAD_TX_INPUT);
tx_input();
}

void thread_tx_output(void)
{
fix_prio(THREAD_TX_OUTPUT);
run_tx_output();
}

void thread_screen(void)
{
screen_routine();
}

void thread_rx_file_input(void)
{
fix_prio(THREAD_RX_FILE_INPUT);
rx_file_input();
}

void thread_cal_iqbalance(void)
{
cal_iqbalance();
}

void thread_rx_adtest(void)
{
rx_adtest();
}

void thread_powtim(void)
{
powtim();
}

void thread_txtest(void)
{
txtest();
}

void thread_syscall(void)
{
do_syscall();
}

void thread_extio_input(void)
{
fix_prio(THREAD_EXTIO_INPUT);
extio_input();
}

void thread_rtl_starter(void)
{
rtl_starter();
}

void thread_fdms1_input(void)
{
fix_prio(THREAD_FDMS1_INPUT);
fdms1_input();
}

void thread_fdms1_starter(void)
{
fdms1_starter();
}

void thread_airspy_input(void)
{
fix_prio(THREAD_AIRSPY_INPUT);
airspy_input();
}

void thread_airspyhf_input(void)
{
fix_prio(THREAD_AIRSPYHF_INPUT);
airspyhf_input();
}

void thread_mirisdr_starter(void)
{
mirisdr_starter();
}

void thread_bladerf_starter(void)
{
bladerf_starter();
}

void thread_network_send(void)
{
network_send();
}

void thread_html_server(void)
{
#if SERVER == 1
html_server();
#endif
}

