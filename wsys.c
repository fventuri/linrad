// Copyright (c) <2012> <Leif Asbrink>
//
// Permission is hereby granted, free of charge, to any person 
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
#include "globdef.h"
#ifndef _MSC_VER
  #include <malloc.h>
#endif
#include <ctype.h>
#include "uidef.h"
#include "thrdef.h"
#include "wdef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "fft3def.h"
#include "screendef.h"
#include "sigdef.h"
#include "seldef.h"
#include "blnkdef.h"
#include "caldef.h"
#include "txdef.h"
#include "vernr.h"
#include "keyboard_def.h"
#include "hwaredef.h"
#include "sdrdef.h"
#include "wscreen.h"

void thread_rx_raw_netinput(void);
void thread_rx_fft1_netinput(void);
void thread_lir_server(void);

static DCB old_serDCB;
char serport_name[]="\\\\.\\COM\0\0\0\0";
int first_mempix_copy;
int last_mempix_copy;
char first_mempix_flag;
char last_mempix_flag;
int screen_refresh_count;

// Parameters to generate quasi-random numbers using the
// The Linear Congruential Method, first introduced by D. Lehmer in 1951.
#define RANDOM_MOD 2147483647
#define RANDOM_FACT 16807
int win_random_seed=4711;


void getbox_error(int x, int y, int w, int h)
{
fprintf( stderr, 
      "\nTrying to get box outside screen x=%d, y=%d, width=%d height=%d",
                                                                  x, y, w, h);
}

void flipbox_error(int x, int y, int w, int h)
{
fprintf( stderr, 
      "\nTrying to flip box outside screen x=%d, y=%d, width=%d height=%d",
                                                                  x, y, w, h);
}

void lir_mutex_lock(int no)
{
EnterCriticalSection(&windows_mutex[no]);
}

void lir_mutex_unlock(int no)
{
LeaveCriticalSection(&windows_mutex[no]);
}

void lir_mutex_init(void)
{
int i;
for(i=0; i<MAX_LIRMUTEX; i++)
  {
  InitializeCriticalSection(&windows_mutex[i]);
  }
}

void lir_mutex_destroy(void)
{
int i;
for(i=0; i<MAX_LIRMUTEX; i++)
  {
  DeleteCriticalSection(&windows_mutex[i]);
  }
}

int open_USB2LPT(void);

void lir_set_title(char *s)
{
(void) s;
}

void lir_init_event(int i)
{
lir_event[i]=CreateEvent( NULL, FALSE, FALSE, NULL);
if(lir_event[i] == NULL)
  {
  lirerr(744521);
  return;
  }
}

void lir_close_event(int i)
{
CloseHandle(lir_event[i]);
}  

void lir_set_event(int no)
{
SetEvent(lir_event[no]);
}

void lir_await_event(int no)
{ 
WaitForSingleObject(lir_event[no], INFINITE);
// The event has been initialized with the auto reset flag,
// therefore it does not need to be reset manually.
}

void lir_sync(void)
{
// This routine is called to force a write to the hard disk under Linux.
// Under Windows, some of the files are opened with the 'c' flag to ensure
// that data is really put to the disk when fflush is called.
}

float lir_random(void)
{
__int64 n;
n=(__int64)(RANDOM_FACT)*(__int64)(win_random_seed);
n=n%(__int64)(RANDOM_MOD);
win_random_seed=n;
return (float)(n)/RANDOM_MOD;
}

float lir_noisegen(int level) 
{
double x;
x=lir_random()*2*PI_L;
return sin(x)*sqrt(-2*log(lir_random()))*pow(2.,0.5*(double)level);
}

double lir_get_thread_time(int no)
{
double t1, t2;
FILETIME t_create, t_exit, t_kernel, t_user;
GetThreadTimes(thread_identifier[no], &t_create, &t_exit, &t_kernel, &t_user);
t1=t_kernel.dwLowDateTime*(0.5/0x80000000)+t_kernel.dwHighDateTime;
t2=t_user.dwLowDateTime*(0.5/0x80000000)+t_user.dwHighDateTime;
// Convert filetime to seconds with the factor 429.5 
return 429.5*(t1+t2);
}

void lir_system_times(double *cpu_time, double *total_time)
{
__int64 *ipu, *ipk;
FILETIME crea, iexit, pkern, puser;
int res;
res=GetProcessTimes(CurrentProcess, &crea, &iexit, &pkern, &puser);
if(res==0)
  {
  total_time[0]=0;
  cpu_time[0]=0;
  return;
  }
ipu=(void*)&puser;
ipk=(void*)&pkern;
total_time[0]=no_of_processors*current_time();
cpu_time[0]=0.0000001*((double)((__int64)ipu[0])+(double)(__int64)ipk[0]);
}

int lir_get_epoch_seconds(void)
{
// Here we have to add a calendar to add the number
// of seconds from todays (year, month, day) to Jan 1 1970.
// The epoch time is needed for moon position computations.
SYSTEMTIME tim;
int k, days, secs;
unsigned char days_per_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
GetSystemTime(&tim);
secs=3600.*tim.wHour+60.*tim.wMinute+tim.wSecond;
days=tim.wDay-1;
k=tim.wMonth-2;
while( k >= 0 )
  {
  days+=days_per_month[k];
  k--;
  }
days+=(tim.wYear-1970)*365;  
// Leap years are every 4th year. Add the number of completed leap years.
days+=(tim.wYear-1969)/4;
// Do not worry about year 2100. This code will presumably
// not live until then......
// Add one more day if the current year is a leap year and if date is above
// February 28. 
if(tim.wYear%4 == 0)
  {
  if(tim.wMonth >= 3)
    {
    days++;
    }
  }  
return days*86400+secs;
}


int lir_open_serport(int serport_number, int baudrate,int stopbit_flag, int rts_mode)
{
static COMSTAT Status;
static unsigned long lpErrors;
static DCB serDCB;
int rc;
COMMTIMEOUTS timeouts;
rc=0;
if(serport != INVALID_HANDLE_VALUE)return rc;
// Get a file handle to the comm port
if(serport_number < 0 || serport_number > 999)
  {
  rc= 1279;
  return rc;
  }
sprintf(&serport_name[7],"%d",serport_number);
serport=CreateFile(serport_name,GENERIC_READ | GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
// Check if valid
if (serport == INVALID_HANDLE_VALUE) 
  {
//  lirerr(1244);
  rc=1244;
  return rc;
  }
// Get serial port parameters - will overwrite baud rate, number of bits,
// parity and stop bits
if (!GetCommState(serport,&serDCB))
  {
//  lirerr(1277);
  rc=1277;
  goto close_x;
  }
switch ( baudrate )
  {
  case 110:
  serDCB.BaudRate= CBR_110; 
  break;
  
  case 300: 
  serDCB.BaudRate= CBR_300;
  break;
  
  case 600: 
  serDCB.BaudRate= CBR_600; 
  break; 
  
  case 1200: 
  serDCB.BaudRate= CBR_1200; 
  break;
  
  case 2400: 
  serDCB.BaudRate= CBR_2400; 
  break;
  
  case 4800: 
  serDCB.BaudRate= CBR_4800; 
  break;
  
  case 9600: 
  serDCB.BaudRate= CBR_9600; 
  break;
  
  case 19200: 
  serDCB.BaudRate= CBR_19200; 
  break;
  
  case 38400: 
  serDCB.BaudRate= CBR_38400; 
  break;
  
  case 57600: 
  serDCB.BaudRate= CBR_57600; 
  break;
  
  default: 
//  lirerr(1280);
  rc=1280;
  goto close_x;
  }
old_serDCB=serDCB;
serDCB.ByteSize=8;
serDCB.Parity=NOPARITY;
serDCB.StopBits=TWOSTOPBITS;
if(stopbit_flag)
  {
  serDCB.StopBits=TWOSTOPBITS;
  }
else
  {  
  serDCB.StopBits=ONESTOPBIT; 
  }
// Tell the device we want 4096 buffers in both directions.
SetupComm(serport,4096,4096);
// Write changes back
if (!SetCommState(serport,&serDCB))
  {
//  lirerr(1278);
  rc=1278;
  goto close_x;
  }
//Set timeouts
//how long (in mseconds) to wait between receiving characters before timing out.
timeouts.ReadIntervalTimeout=50;
//how long to wait (in mseconds) before returning.
timeouts.ReadTotalTimeoutConstant=50;
//how much additional time to wait (in mseconds) before
//returning for each byte that was requested in the read operation
timeouts.ReadTotalTimeoutMultiplier=10;
timeouts.WriteTotalTimeoutConstant=50;
timeouts.WriteTotalTimeoutMultiplier=10;
if(!SetCommTimeouts(serport, &timeouts))
  {
//  lirerr(1278);
  rc=1278;
  goto close_x;
  }
switch (rts_mode)
  {
  case 0:
  if (!EscapeCommFunction(serport, SETRTS))rc=1278;
  break;
  
  case 1:
  if (!EscapeCommFunction(serport, CLRRTS))rc=1278;  
  break;

  case 2:
  if (!EscapeCommFunction(serport, CLRRTS))rc=1278;  
  break;
  }  
close_x:
if(rc != 0)
  {
  CloseHandle(serport);
  serport=INVALID_HANDLE_VALUE;
  return rc ;
  }

ClearCommError(serport, &lpErrors, &Status);
return rc;
}

void lir_close_serport(void)
{
if(serport == INVALID_HANDLE_VALUE)return;
if(!SetCommState(serport,&old_serDCB))lirerr(1278);
CloseHandle(serport);
serport=INVALID_HANDLE_VALUE;
return ;
}

int lir_write_serport(void *s, int bytes)
{
unsigned long iBytesWritten;
if (WriteFile(serport,s,bytes,&iBytesWritten,NULL))
  {
  return 0;
  }
else
  {
  return -1;
  }
}

int lir_read_serport(void *s, int bytes)
{
unsigned long iBytesRead;
int nread = 0;
while (bytes > 0) {
if (ReadFile(serport,((char*)s + nread),bytes,&iBytesRead,NULL))
  {
 if (iBytesRead == 0) return -1;
 nread += iBytesRead;
 bytes -= iBytesRead;
  }
else
  {
  return -1;
  }
}
return nread;
}

int ms_since_midnight(void)
{
int i;
SYSTEMTIME tim;
GetSystemTime(&tim);
i=3600000.*tim.wHour+60000.*tim.wMinute+1000.*tim.wSecond+tim.wMilliseconds;
return i;
}

void win_semaphores(void)
{
long old;
if(rxin1_bufready != NULL)
  {
  ReleaseSemaphore(rxin1_bufready,1,&old);
  }
}

void lir_sched_yield(void)
{
Sleep(0);
};


int lir_parport_permission(int portno)
{
(void) portno;
// Using usb2lpt is always permitted if libusb is installed
// but always forbidden if not.
if (wse.parport==USB2LPT16_PORT_NUMBER) 
  {
  if(!usb2lpt_flag)usb2lpt_flag=open_USB2LPT();
  return usb2lpt_flag;
  }
else
  {
  if(parport_installed)return TRUE;
  }  
return FALSE;
}

void lir_sleep(int us)
{
Sleep(us/1000);
}

void linrad_thread_create(int no)
{
thread_status_flag[no]=THRFLAG_INIT;
thread_command_flag[no]=THRFLAG_ACTIVE;
lir_sleep(20000);
thread_identifier[no]=CreateThread(NULL, 0, 
                                      thread_routines[no], NULL, 0, NULL);
if(thread_identifier[no] == NULL)lirerr(9871201);
}

double current_time(void)
{
LARGE_INTEGER lrgi;
double dt1;
if(QueryPerformanceCounter(&lrgi)==0)
  {
  lirerr(10002);
  return 0;
  }
dt1=lrgi.HighPart-internal_clock_offset;
dt1*=(float)(0x10000)*(float)(0x10000);
dt1+=lrgi.LowPart;
recent_time=dt1/internal_clock_frequency;
return recent_time;
}

void lirerr(int errcod)
{
if(kill_all_flag) return;
DEB"\nlirerr(%d)",errcod);
if(dmp != 0)fflush( dmp);
lir_errcod=errcod;
lir_set_event(EVENT_KILL_ALL);
while(!kill_all_flag)lir_sleep(10000);
}

char lir_inb(int port)
{
if(wse.parport==USB2LPT16_PORT_NUMBER)
  {
  return in_USB2LPT((port-wse.parport));
  }
return (inp32)(port);
}

void lir_outb(char bytedat, int port)
{
if(wse.parport==USB2LPT16_PORT_NUMBER)
  {
  out_USB2LPT(bytedat,(port-wse.parport));
  }
else
  {
  (oup32)(port,bytedat);
  }
}

void lir_join(int no)
{
if(thread_command_flag[no]==THRFLAG_NOT_ACTIVE)return;
WaitForSingleObject(thread_identifier[no],INFINITE);
thread_status_flag[no]=THRFLAG_NOT_ACTIVE;
}

// *************************************************************
// Set global parameters and the corresponding dummy routines.
// *************************************************************

bool fonts_by_GDI = false;
int  font_color_GDI = 0;

static const char *wfConfigFileName	= "par_userfont";

#define wfCfgKeyName			"Face name"
#define wfCfgKeyHeight			"Height"
#define wfCfgKeyWidth			"Width"
#define wfCfgKeyWeight			"Weight"
#define wfCfgKeyItalic			"Italic"
#define wfCfgKeyCharSet			"Char set"
#define wfCfgKeyPitchAndFamily  "Pitch and family"

enum {
	WF_CFG_KEY_UNKNOWN,
	WF_CFG_KEY_NAME,
	WF_CFG_KEY_HEIGHT,
	WF_CFG_KEY_WIDTH,
	WF_CFG_KEY_WEIGHT,
	WF_CFG_KEY_ITALIC,
	WF_CFG_KEY_CHAR_SET,
	WF_CFG_KEY_PITCH_AND_FAMILY
};

struct KeyNumPair {
  const char *key;
  const int   num;
};

const struct KeyNumPair wfCfgKeys[] = {
  { wfCfgKeyName,           WF_CFG_KEY_NAME },
  { wfCfgKeyHeight,         WF_CFG_KEY_HEIGHT },
  { wfCfgKeyWidth,          WF_CFG_KEY_WIDTH },
  { wfCfgKeyWeight,         WF_CFG_KEY_WEIGHT },
  { wfCfgKeyItalic,         WF_CFG_KEY_ITALIC },
  { wfCfgKeyCharSet,        WF_CFG_KEY_CHAR_SET },
  { wfCfgKeyPitchAndFamily, WF_CFG_KEY_PITCH_AND_FAMILY }
};

static bool load_font(const char *path, LOGFONT *lf)
{
memset(lf, 0, sizeof(LOGFONT));
FILE *file = fopen(path, "rt");
if (file == NULL) return false;
char buf[2048];
char *c, *end;
int i;
int key;
while (!feof(file)) {
  if (fgets(buf, 2048, file) == NULL)
	  break;
  c = buf;
  while (*c != 0 && (*c == ' ' || *c == '\t'))
    ++ c;
  if (*c == '#')
	// comment
	continue;
  // Find the key.
  key = WF_CFG_KEY_UNKNOWN;
  for (i = 0; i < (int)(sizeof(wfCfgKeys)/sizeof(struct KeyNumPair)); ++ i) {
    if (strncmp(c, wfCfgKeys[i].key, strlen(wfCfgKeys[i].key)) == 0) {
      c += strlen(wfCfgKeys[i].key);
	  key = wfCfgKeys[i].num;
	  break;
    }
  }
  if (key == WF_CFG_KEY_UNKNOWN)
     goto err;
  // Skip whitespaces
  while (*c != 0 && (*c == ' ' || *c == '\t'))
	++c;
  if (key == WF_CFG_KEY_NAME) {
	// Parse a string enclosed in the brackets.
	if (*c == '[') ++c;
	else goto err;
	end = strchr(c, ']');
	if (end == NULL || *end != ']') goto err;
	*end = 0;
	if (strlen(c) > LF_FACESIZE)
      goto err;
	strcpy(lf->lfFaceName, c);
	continue;
  }
  // The other keys shall have numeric value. Parse it.
  if (sscanf(c, "[%d]", &i) != 1) goto err;
  switch (key) {
  case WF_CFG_KEY_HEIGHT: lf->lfHeight = i; break;
  case WF_CFG_KEY_WIDTH:  lf->lfWidth  = i; break;
  case WF_CFG_KEY_WEIGHT: lf->lfWeight = i; break;
  case WF_CFG_KEY_ITALIC: lf->lfItalic = i; break;
  case WF_CFG_KEY_CHAR_SET: lf->lfCharSet = i; break;
  case WF_CFG_KEY_PITCH_AND_FAMILY: lf->lfPitchAndFamily = i; break;
  }
}
fclose(file);
return true;

err:
fclose(file);
memset(lf, 0, sizeof(LOGFONT));
return false;
}

static bool save_font(const char *path, LOGFONT *lf)
{
FILE *file = fopen(path, "wt");
if (file == NULL)
  return false;
fprintf(file, "%s [%s]\n",  wfCfgKeyName,    lf->lfFaceName);
fprintf(file, "%s [%ld]\n", wfCfgKeyHeight,  lf->lfHeight);
fprintf(file, "%s [%ld]\n", wfCfgKeyWidth,   lf->lfWidth);
fprintf(file, "%s [%ld]\n", wfCfgKeyWeight,  lf->lfWeight);
fprintf(file, "%s [%d]\n",  wfCfgKeyItalic,  lf->lfItalic);
fprintf(file, "%s [%d]\n",  wfCfgKeyCharSet, lf->lfCharSet);
fprintf(file, "%s [%d]\n",  wfCfgKeyPitchAndFamily, lf->lfPitchAndFamily);
fclose(file);
return true;
}

static bool choose_font(HWND hwnd, LOGFONT *lf)
{
CHOOSEFONT cf;
ZeroMemory(&cf, sizeof(cf));
cf.lStructSize = sizeof(cf);
cf.hwndOwner = hwnd;
cf.lpLogFont = lf;
cf.Flags = CF_SCREENFONTS | CF_FIXEDPITCHONLY | CF_FORCEFONTEXIST | CF_NOVERTFONTS | CF_INITTOLOGFONTSTRUCT | CF_LIMITSIZE;
cf.nSizeMin = 7;
cf.nSizeMax = 32;
if (ChooseFont(&cf) == TRUE) return true;
return false;
}

void win_init_font_GDI(LOGFONT *logfont, bool forceDialog)
{
bool loaded = false;
fonts_by_GDI = true;
// Create a font object, select it into the GDI device context of the bitmap.
if (load_font(wfConfigFileName, logfont)) {
  loaded = true;
} else {
  // A sensible default, which is always a part of the Windows installation.
  memset(logfont, 0, sizeof(LOGFONT));
  strcpy(logfont->lfFaceName, "Terminal");
  logfont->lfHeight = -15;
  logfont->lfWeight = FW_BOLD;
}
if (! loaded || forceDialog) {
  LOGFONT lf2;
  memcpy(&lf2, logfont, sizeof(lf2));
  if (choose_font(linrad_hwnd, &lf2))
	// Accept the changes.
	memcpy(logfont, &lf2, sizeof(lf2));
}
if (! save_font(wfConfigFileName, logfont)) {
  // Report errror.
}
}

void win_load_font_GDI(LOGFONT *logfont)
{
RECT rect;
POINT point;
OUTLINETEXTMETRIC otm;
SelectObject(memory_hdc, CreateFontIndirect(logfont));
SetTextColor(memory_hdc, RGB(255, 255, 255));
SetBkColor(memory_hdc, RGB(0, 0, 0));
// Measure the font.
rect.left = 0;
rect.top = 0;
rect.right = 1000;
rect.bottom = 1000; 
DrawTextEx(memory_hdc, "X", 1, &rect, DT_LEFT | DT_TOP | DT_CALCRECT, NULL);
GetOutlineTextMetrics(memory_hdc, sizeof(otm), &otm);
point.x = rect.right - rect.left;
point.y = rect.bottom - rect.top;
LPtoDP(memory_hdc, &point, 1);
//text_width = rint((float)(rect.right - rect.left)*1.2f);
text_width = point.x;
text_height = point.y + 1;
//	text_heigth = otm.otmAscent + otm.otmDescent + otm.otmLineGap;
screen_last_line = (screen_height - 1) / text_height - 1;
screen_last_xpixel = screen_width - 1;
screen_last_col = screen_width / text_width - 1;
}

void win_selectfont(void)
{
if (ui.font_scale == 0) {
  LOGFONT logfont;
  win_init_font_GDI(&logfont, true);
  // Initialize a Windows font, select it into memory_hdc.
  win_load_font_GDI(&logfont);
}
}

void win_global_uiparms(int wn)
{
TIMECAPS timer_limits;
char s[80], ss[80], st[80], sr[80], su[80];
char chr;
int maxprio;
if(wn != 0)
  {
  if( ui.operator_skil == OPERATOR_SKIL_NEWCOMER)
    {
    clear_screen();
    lir_text(0,4,"You are now in NEWCOMER mode.");
    lir_text(0,6,"Press 'Y' to change to NORMAL mode or 'N' to");
    lir_text(0,7,"stay in newcomer mode.");
ask_newco:;
    await_processed_keyboard();
    if(lir_inkey == 'N')goto stay_newco;
    if(lir_inkey != 'Y')goto ask_newco;
    ui.operator_skil=OPERATOR_SKIL_NORMAL;
    }
stay_newco:;
  if( ui.operator_skil == OPERATOR_SKIL_NORMAL)
    {
    clear_screen();
    lir_text(0,7,"You are now in NORMAL mode.");
    lir_text(0,9,"Press 'Y' to change to EXPERT mode or 'N' to");
    lir_text(0,10,"stay in normal mode.");
ask_normal:;
    await_processed_keyboard();
    if(lir_inkey == 'N')goto stay_normal;
    if(lir_inkey != 'Y')goto ask_normal;
    ui.operator_skil=OPERATOR_SKIL_EXPERT;
    }
stay_normal:;
  }
else
  {
  ui.vga_mode=12;
  ui.mouse_speed=8;
  }
if(ui.operator_skil == OPERATOR_SKIL_NEWCOMER)
  {
  ui.font_scale=2;
  ui.process_priority=1;
  wse.parport=0;
  wse.parport_pin=0;
  ui.max_blocked_cpus=0;
  }
else
  {    
  sprintf(s,"Enter font scale (1 to 5), 0 for Windows fonts"); 
fntc:;
  if(wn==0)
    {
    printf("\n%s, then press Enter: ",s); 
    while(fgets(ss,8,stdin)==NULL);
    lir_inkey=to_upper(ss[0]);
    }
  else
    {
    clear_screen();
    lir_text(4,5,s);
    to_upper_await_keyboard();
    if(kill_all_flag) return;
    }
  if(lir_inkey < '0' || lir_inkey > '5')goto fntc;
  ui.font_scale=lir_inkey-'0';
  if (ui.font_scale == 0) win_selectfont();
  else init_font(ui.font_scale);
prio:;
  sprintf(s,"Set process priority (0=NORMAL to "); 
  if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
    {
    strcat(s,"3=REALTIME)");
    maxprio=3;
    }
  else  
    {
    strcat(s,"2=HIGH)");
    maxprio=2;
    }
  if(wn==0)
    {
    printf("\n%s, then press Enter: ",s); 
    while(fgets(ss,8,stdin)==NULL);
    lir_inkey=to_upper(ss[0]);
    }  
  else
    {
    clear_screen();
    lir_text(0,7,s);
    to_upper_await_keyboard();
    }
  if(lir_inkey >='0' && lir_inkey <= maxprio+'0')
    {
    ui.process_priority=lir_inkey-'0';
    }
  else
    {
    goto prio;
    }
  if(ui.process_priority==2)
    {
    sprintf(s,"Are you sure? (Y/N)");
    if(wn==0)
      {
      printf("\n%s\n=>",s); 
      while(fgets(ss,8,stdin)==NULL);
      if(to_upper(ss[0])=='Y')goto prio_x;
      goto prio;
      }
    else
      {
      clear_screen();
      lir_text(0,3,s);
      to_upper_await_keyboard();
      if(lir_inkey=='Y')goto prio_x;
      goto prio;
      }
    }
  if(ui.process_priority==3)
    {
    sprintf(s,"Hmmm, you are the expert so you know what might happen....");
    sprintf(ss,"Are you really sure? (Y/N)");
    if(wn==0)
      {
      printf("\n%s\n=>",s); 
      printf("\n%s\n=>",ss); 
      while(fgets(ss,8,stdin)==NULL);
      if(to_upper(ss[0])=='Y')goto prio_x;
      goto prio;
      }
    else
      {
      clear_screen();
      lir_text(0,3,s);
      lir_text(0,5,ss);
      to_upper_await_keyboard();
      if(lir_inkey=='Y')goto prio_x;
      goto prio;
      }
    }
prio_x:;
  if(no_of_processors > 1)
    {
    sprintf(s,"This system has % d processors.",no_of_processors);
    sprintf(ss,"How many do you allow Linrad to block?");
    sprintf(sr,
        "If you run several instances of Linrad on one multiprocessor");
    sprintf(st,"platform it may be a bad idea to allow the total number");
    sprintf(su,"of blocked CPUs to be more that the total number less one.");        
    if(wn==0)
      {
      printf("\n%s",s); 
      printf("\n%s",ss); 
      printf("\n%s",sr); 
      printf("\n%s",st); 
      printf("\n%s\n\n=>",su); 
      while(fgets(ss,8,stdin)==NULL);
      sscanf(ss,"%d", &ui.max_blocked_cpus);
      if(ui.max_blocked_cpus <0)ui.max_blocked_cpus=0;
      if(ui.max_blocked_cpus >=no_of_processors)
                                          ui.max_blocked_cpus=no_of_processors;
      }
    else
      {
      clear_screen();
      lir_text(0,2,s);
      lir_text(0,3,ss);
      lir_text(0,4,sr);
      lir_text(0,5,st);
      lir_text(0,6,su);
      lir_text(0,8,"=>");
      ui.max_blocked_cpus=lir_get_integer(3,8,2,0,no_of_processors-1);
      }
    }    
  else
    {
    ui.max_blocked_cpus=0;
    }  
  if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
    {
    timeGetDevCaps(&timer_limits,sizeof(timer_limits));
    if(timer_limits.wPeriodMax > 999)timer_limits.wPeriodMax=999;
    sprintf(s,"Set timer resolution in %d to %d ms (0 to use default)",
                        timer_limits.wPeriodMin, timer_limits.wPeriodMax);
get_timer_resolution:;    
    if(wn==0)
      {
      printf("\n%s\n=>",s); 
      while(fgets(ss,8,stdin)==NULL);
      sscanf(ss,"%d", &ui.timer_resolution);
      }         
    else
      {
      clear_screen();
      lir_text(0,6,s);
      ui.timer_resolution=lir_get_integer(56,6,4,0,timer_limits.wPeriodMax);
      clear_screen();
      }
    if(ui.timer_resolution != 0)
      {
      if(ui.timer_resolution < timer_limits.wPeriodMin ||
         ui.timer_resolution > timer_limits.wPeriodMax)
        {
        goto get_timer_resolution;
        }
      }
    sprintf(s,"Set autostart: Z=none,A=WCW,B=CW,C=MS,D=SSB,E=FM,F=AM,G=QRSS");
get_autostart:;    
    if(wn==0)
      {
      printf("\n%s\n=>",s); 
      while(fgets(ss,8,stdin)==NULL);
      lir_inkey=to_upper(ss[0]);
      }         
    else
      {
      lir_text(0,6,s);
      lir_text(0,8,"=>");
      to_upper_await_keyboard();
      sprintf(ss,"%c",lir_inkey);
      lir_text(3,8,ss);
      }
    lir_inkey=to_upper(ss[0]);
    if(lir_inkey == 'Z')
      {
      ui.autostart=0;
      }
    else
      {
      if(lir_inkey < 'A' || lir_inkey > 'G')goto get_autostart;
      ui.autostart=lir_inkey;
      }
    }
  }
sprintf(s,"You can specify the screen size in pixels or as a percentage");
sprintf(sr,"of the entire area of all your screens. Enter Y for sizes in"); 
sprintf(st,"pixels or N for sizes as %% (Y/N)=>");
screen_sel:;
if(wn == 0)
  {
  printf("\n%s",s); 
  printf("\n%s",sr); 
  printf("\n%s",st); 
  while(fgets(ss,8,stdin)==NULL);
  sscanf(ss,"%c", &chr);
  }
else
  {
  clear_screen();
  lir_text(0,2,s);
  lir_text(0,3,sr);
  lir_text(0,4,st);
  await_keyboard();
  chr=lir_inkey;
  if(kill_all_flag) return;
  }
chr=toupper(chr);  
switch (chr)
  {
  case 'N':    
  sprintf(s,"Percentage of screen width to use(25 to 100):"); 
parport_wfac:;
  if(wn==0)
    {
    printf("\n%s\n=>",s);
    while(fgets(ss,8,stdin)==NULL);
    sscanf(ss,"%d", &ui.screen_width_factor);
    if(ui.screen_width_factor < 25 ||
       ui.screen_width_factor > 100)goto parport_wfac;
    }
  else
    {
    clear_screen();
    lir_text(0,6,s);
    ui.screen_width_factor=lir_get_integer(46,6,3,0,100);
    }
  sprintf(s,"Percentage of screen height to use(25 to 100):"); 
parport_hfac:;
  if(wn==0)
    {
    printf("\n%s\n=>",s); 
    while(fgets(ss,8,stdin)==NULL);
    sscanf(ss,"%d", &ui.screen_height_factor);
    if(ui.screen_height_factor < 25 ||
       ui.screen_height_factor > 100)goto parport_hfac;
    }
  else
    {
    clear_screen();
    lir_text(0,7,s);
    ui.screen_height_factor=lir_get_integer(47,7,3,0,100);
    }
  break;
  
  case 'Y':
  sprintf(s,"Screen width in pixels (500 to 10000):"); 
parport_wpix:;
  if(wn==0)
    {
    printf("\n%s\n=>",s);
    while(fgets(ss,8,stdin)==NULL);
    sscanf(ss,"%d", &ui.screen_width_factor);
    if(ui.screen_width_factor < 500 ||
       ui.screen_width_factor > 10000)goto parport_wpix;
    }
  else
    {
    clear_screen();
    lir_text(0,6,s);
    ui.screen_width_factor=lir_get_integer(46,6,5,500,10000);
    }
  ui.screen_width_factor=-ui.screen_width_factor;
  sprintf(s,"Screen height in pixels (400 to 10000):"); 
parport_hpix:;
  if(wn==0)
    {
    printf("\n%s\n=>",s); 
    while(fgets(ss,8,stdin)==NULL);
    sscanf(ss,"%d", &ui.screen_height_factor);
    if(ui.screen_height_factor < 400 ||
       ui.screen_height_factor > 10000)goto parport_hpix;
    }
  else
    {
    clear_screen();
    lir_text(0,7,s);
    ui.screen_height_factor=lir_get_integer(47,7,5,400,10000);
    }
  ui.screen_height_factor=-ui.screen_height_factor;
  break;
  
  default:
  goto screen_sel;
  } 
if(wn == 0)
  {
  printf("\n\nLinrad will now open another window.");
  printf("\nMinimize this window and click on the new window to continue.");
  printf(
      "\n\nDo not forget to save your parameters with 'W' in the main menu");
  fflush(stdout);
  }
else
  {
  clear_screen();
  settextcolor(15);
  lir_text(0,3,"Save the new parameters with 'W' in the main menu.");
  lir_text(0,5,"Then exit and restart Linrad for any new");
  lir_text(0,6,"screen parameter to take effect.");
  lir_text(5,8,press_any_key);
  await_keyboard();
  clear_screen();
  settextcolor(7);
  }
}

void x_global_uiparms(int wn)
{
lir_inkey=wn;
}

void lin_global_uiparms(int wn)
{
lir_inkey=wn;
}

// *********************************************************
// Graphics for Windows
// *********************************************************

void lir_getpalettecolor(int j, int *r, int *g, int *b)
{
r[0]=(int)svga_palette[3*j]>>2;
g[0]=(int)svga_palette[3*j+1]>>2;
b[0]=(int)svga_palette[3*j+2]>>2;
}

// Invalidate a rectangle.
// This region will be bitblted to the screen at the next screen update.
// Why is this function static ?
// Because modern compilers following C11 standard section 6.7.4 cause a problem.
// We have to take care that the linker can see this function.
// https://runtimeverification.com/blog/undefined-c-common-mistakes/
static __inline void lir_screen_invalidate(int x, int y, int w, int h)
{
(void)w;
int k = x + (screen_height - y)*screen_width;
if (last_mempix < k + h - 1)
{
  last_mempix_copy = k + h - 1;
  last_mempix = k + h - 1;
  last_mempix_flag = 1;
}
k -= screen_width*(h - 1);
if (first_mempix > k)
{
  first_mempix_copy = k;
  first_mempix = k;
  first_mempix_flag = 1;
}
}

void lir_fillbox(int x, int y,int w, int h, unsigned char c)
{
int i, j, k;
k=x+(screen_height-y)*screen_width;
if(k-h*screen_width<0 || k+w > screen_totpix)
  {
  fillbox_error(x,y,w,h);
  return;
  }
for(j=0; j<h; j++)
  {
  for(i=0; i<w; i++)mempix[k+i]=c;
  k-=screen_width;
  }
lir_screen_invalidate(x, y, w, h);
}

// Flip a box horizontally.
void lir_flipbox_horiz(int x, int y, int w, int h)
{
int i, j, k;
char c;
k = x + (screen_height - y)*screen_width;
if (k - h*screen_width<0 || k + w > screen_totpix)
  {
  flipbox_error(x,y,w,h);
  return;
  }
for (j = 0; j<h; j++, k -= screen_width) {
  int i1 = k;
  int i2 = k + w - 1;
  for (i = 0; i < w / 2; ++ i, ++ i1, -- i2) {
    c = mempix[i1];
    mempix[i1] = mempix[i2];
    mempix[i2] = c;
  }
}
lir_screen_invalidate(x, y, w, h);
}

// Flip a box horizontally, center the flip at the center of a nonzero pixels.
void lir_flipbox_horiz_symmetric(int x, int y, int w, int h)
{
int i, j, k, k0, l, r;
char c;
k0 = x + (screen_height - y)*screen_width;
k = k0;
if (k - h*screen_width<0 || k + w > screen_totpix)
  {
  flipbox_error(x,y,w,h);
  return;
  }
// Find a left / right span of a character inside the box.
l = screen_width;
r = 0;
for (j = 0; j<h; j++, k -= screen_width) {
  for (i = 0; i < w; ++i)
	// Pixel is nonzero, extend the left / right span.
    if (mempix[i+k] != 0) {
      if (i < l) l = i;
	  if (i > r) r = i;
	}
}
// Flip the box horizontally, but only in the left / right span.
k = k0;
for (j = 0; j<h; j++, k -= screen_width) {
  int i1 = k+l;
  int i2 = k+r;
  for (; i1 < i2; ++ i1, -- i2) {
    c = mempix[i1];
    mempix[i1] = mempix[i2];
    mempix[i2] = c;
  }
}
lir_screen_invalidate(x, y, w, h);
}

// Flip a box vertically.
void lir_flipbox_vert(int x, int y, int w, int h)
{
int i, k1, k2;
char c;
k1 = x + (screen_height - y - h + 1)*screen_width;
k2 = x + (screen_height - y)*screen_width;
if (k1<0 || k2 + w > screen_totpix)
  {
  flipbox_error(x,y,w,h);
  return;
  }
for (; k1 < k2; k1 += screen_width, k2 -= screen_width) {
  int i1 = k1;
  int i2 = k2;
  for (i = 0; i < w; ++i, ++i1, ++i2) {
    c = mempix[i1];
    mempix[i1] = mempix[i2];
    mempix[i2] = c;
  }
}
lir_screen_invalidate(x, y, w, h);
}

// Flip a box vertically, center the flip at the center of a nonzero pixels.
void lir_flipbox_vert_symmetric(int x, int y, int w, int h)
{
int i, k1, k2;
char c;
k1 = x + (screen_height - y - h + 1)*screen_width;
k2 = x + (screen_height - y)*screen_width;
if (k1<0 || k2 + w > screen_totpix)
  {
  flipbox_error(x,y,w,h);
  return;
  }
// Find the first nonzero line.
for (; k1 < k2; k1 += screen_width) {
	for (i = 0; i < w; ++i)
		if (mempix[k1 + i] != 0)
			goto end_k1;
	}
end_k1:
	// Find the last nonzero line.
	for (; k1 < k2; k2 -= screen_width) {
		for (i = 0; i < w; ++i)
			if (mempix[k2 + i] != 0)
				goto end_k2;
	}
end_k2:
	// Flip the box between k1 and k2.
	for (; k1 < k2; k1 += screen_width, k2 -= screen_width) {
		int i1 = k1;
		int i2 = k2;
		for (i = 0; i < w; ++i, ++i1, ++i2) {
			c = mempix[i1];
			mempix[i1] = mempix[i2];
			mempix[i2] = c;
		}
	}
	lir_screen_invalidate(x, y, w, h);
}

void lir_getbox(int x, int y, int w, int h, size_t* dp)
{
unsigned char *savmem;
int i, j, k, m;
k=x+(screen_height-y)*screen_width;
if(k-h*screen_width<0 || k+w > screen_totpix)
  {
  getbox_error(x,y,w,h);
  return;
  } 
m=0;
savmem=(unsigned char*)dp;
for(j=0; j<h; j++)
  {
  for(i=0; i<w; i++)
    {
    savmem[m]=mempix[k+i];
    m++;
    }
  k-=screen_width;
  }
}

void lir_putbox(int x, int y, int w, int h, size_t* dp)
{
unsigned char *savmem;
int i, j, k, m;
k=x+(screen_height-y)*screen_width;
if(k-h*screen_width<0 || k+w > screen_totpix)
  {
  putbox_error(x,y,w,h);
  return;
  }
m=0;
savmem=(unsigned char*)dp;
for(j=0; j<h; j++)
  {
  for(i=0; i<w; i++)
    {
    mempix[k+i]=savmem[m];
    m++;
    }
  k-=screen_width;
  }
lir_screen_invalidate(x, y, w, h);
}

void lir_hline(int x1, int y, int x2, unsigned char c)
{
int i, ia, ib;
ia=(screen_height-y)*screen_width;
if(x1 <= x2)
  {
  ib=ia+x2;
  ia+=x1;
  }
else
  {
  ib=ia+x1;
  ia+=x2;
  }
if(ia < 0 || ib >= screen_totpix)
  {
  h_line_error(x1, y, x2);
  return;
  }
for(i=ia; i<=ib; i++)mempix[i]=c;
if(first_mempix > ia)
  {
  first_mempix_copy=ia;
  first_mempix=ia;
  first_mempix_flag=1;
  }
if(last_mempix < ib)
  {
  last_mempix_copy=ib;
  last_mempix=ib;
  last_mempix_flag=1;
  }
}

void lir_line(int x1, int yy1, int x2,int y2, unsigned char c)
{
int ia;
int i,j,k;
int xd, yd;
float t1,t2,delt;
xd=x2-x1;
yd=y2-yy1;
if(yd==0)
  {
  if(xd==0)
    {
    lir_setpixel(x1,yy1,c);
    }
  else
    {
    lir_hline(x1,yy1,x2,c);
    }
  return;  
  }  
if(abs(xd)>=abs(yd))
  {
  if(xd>=0)
    {
    k=1;
    }
  else
    {
    k=-1;
    }  
  if(yd >= 0)
    {
    delt=0.5;
    }
  else
    {  
    delt=-0.5;
    }
  t1=yy1;
  t2=(float)(yd)/abs(xd);
  i=x1-k;
  while(i!=x2)
    {
    i+=k;
    j=t1+delt;
    ia=i+(screen_height-j)*(screen_width);
    if(ia < 0 || ia >= screen_totpix)
      {
      lirerr(10008);
      return;
      }
    mempix[ia]=c;
    if(first_mempix > ia)
      {
      first_mempix_copy=ia;
      first_mempix=ia;
      first_mempix_flag=1;
      }
    if(last_mempix < ia)
      {
      last_mempix_copy=ia;
      last_mempix=ia;
      last_mempix_flag=1;
      }
    t1+=t2;
    }
  }  
else
  {
  if(yd>=0)
    {
    k=1;
    }
  else
    {
    k=-1;
    } 
  if(xd >= 0)
    {
    delt=0.5;
    }
  else
    {  
    delt=-0.5;
    }
  t1=x1;
  t2=(float)(xd)/abs(yd);
  i=yy1-k;
  while(i!=y2)
    {
    i+=k;
    j=t1+delt;
    ia=j+(screen_height-i)*(screen_width);
    if(ia < 0 || ia >= screen_totpix)
      {
      lirerr(10007);
      return;
      }
    mempix[ia]=c;
    if(first_mempix > ia)
      {
      first_mempix_copy=ia;
      first_mempix=ia;
      first_mempix_flag=1;
      }
    if(last_mempix < ia)
      {
      last_mempix_copy=ia;
      last_mempix=ia;
      last_mempix_flag=1;
      }
    t1+=t2;
    }
  }  
}

void lir_setpixel(int x, int y, unsigned char c)
{
int ia;
ia=x+(screen_height-y)*screen_width;
if(ia < 0 || ia >= screen_totpix)
  {
  setpixel_error(x, y);
  return;
  }
mempix[ia]=c;
if(first_mempix > ia)
  {
  first_mempix_copy=ia;
  first_mempix=ia;
  first_mempix_flag=1;
  }
if(last_mempix < ia)
  {
  last_mempix_copy=ia;
  last_mempix=ia;
  last_mempix_flag=1;
  }
}

void clear_screen(void)
{
memset(mempix, 0, screen_width * screen_height);
lir_refresh_entire_screen();
lir_refresh_screen();
}

void lir_refresh_entire_screen(void)
{
lir_screen_invalidate(0, 0, screen_width, screen_height);
}

bool lir_pixwrite_GDI(int x, int y, char *txt)
{
char *txtcpy;
int len, i;
RECT rect;
bool cooked = false;
if (!fonts_by_GDI) return false;
len = strlen(txt);
if (len == 0) return true;
// Substitute arrows and cursors in the font.
txtcpy = (char*)
#ifdef _MSC_VER
	_alloca(len + 1);
#else
	alloca(len + 1);
#endif
for (i = 0; i <= len; ++i) {
  char c = txt[i];
  switch (c) {
    // Block character will be painted from scratch.
    case 0x01: txtcpy[i] = ' '; cooked = true; break;
	// Up arrow will be taken from the font.
    case 0x18: txtcpy[i] = 0x18; break;
    // Down arrow will be replaced by the up arrow and flipped later.
    case 0x19: txtcpy[i] = 0x18; cooked = true; break;
    // Left arrow will be taken from the font.
    case 0x1b: txtcpy[i] = 0x1b; break;
    // Right arrow will be replaced by the left arrow and flipped later.
    case 0x1a: txtcpy[i] = 0x1b; cooked = true; break;
    default: txtcpy[i] = c;
  }
}
EnterCriticalSection(&screen_update_mutex);
SetTextColor(memory_hdc, RGB(svga_palette[font_color_GDI * 3], svga_palette[font_color_GDI * 3 + 1], svga_palette[font_color_GDI * 3 + 2]));
rect.left = x;
rect.top = y - 1;
rect.right = rect.left + text_width*strlen(txt); // -1;
rect.bottom = rect.top + text_height - 1;
//DrawTextEx(memory_hdc, txt, strlen(txt), &rect, DT_LEFT | DT_TOP | DT_CALCRECT, NULL);
DrawTextExA(memory_hdc, txtcpy, len, &rect, DT_LEFT | DT_TOP, NULL);
GdiFlush();
lir_screen_invalidate(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
LeaveCriticalSection(&screen_update_mutex);
if (!cooked) return true;
for (i = 0; i < len; ++ i) {
  char c = txt[i];
  switch (c) {
  // Block character will be painted from scratch.
  case 0x01:
	lir_fillbox(x+i*text_width, y, text_width-1, text_height-2, font_color_GDI);
    break;
  // Down arrow will be replaced by the up arrow and flipped later.
  case 0x19:
	lir_flipbox_vert_symmetric(x + i*text_width, y+1, text_width, text_height-3);
	break;
  // Right arrow will be replaced by the left arrow and flipped later.
  case 0x1a:
	lir_flipbox_horiz_symmetric(x + i*text_width, y, text_width-1, text_height);
	break;
  }
}
return true;
}

void lir_refresh_screen(void)
{
HDC hdc;
EnterCriticalSection(&screen_update_mutex);
if(last_mempix >= first_mempix) {
  int l1=screen_height-last_mempix/screen_width;
  int l2=screen_height-first_mempix/screen_width+1;
  first_mempix_flag=0;
  last_mempix_flag=0;    
  first_mempix=0x7fffffff;
  last_mempix=0;
  if(first_mempix_flag != 0)first_mempix=first_mempix_copy;
  if(last_mempix_flag != 0)last_mempix=last_mempix_copy;
  hdc = GetDC(linrad_hwnd);
  BitBlt(hdc, 0, l1, screen_width, l2-l1, memory_hdc, 0, l1, SRCCOPY);
  ReleaseDC(linrad_hwnd, hdc);
}
LeaveCriticalSection(&screen_update_mutex);
}

int mbutton_state = 0;

void wmouse(void)
{
MSG    msg;
int    m;
int	   mx;
int    my;
while (GetMessage(&msg, NULL, 0, 0) > 0) {
switch (msg.message) {
  case WM_MOUSEWHEEL:
  m=GET_WHEEL_DELTA_WPARAM(msg.wParam);
  if(mbutton_state==0)
  {

    if(m<0)
      {
      step_rx_frequency(1);
      }
    else
      {
      step_rx_frequency(-1);
      }
    }
  else
    {
    m=bg.wheel_stepn;
    if (m<0)
      {
      m++;
      if(m>30)m=30;
      }
    else
      {
      m=bg.wheel_stepn;
      m--;
      if(m<-30)m=-30;
      if((genparm[AFC_ENABLE]==0 || genparm[AFC_LOCK_RANGE] == 0) && m<0)m=0;
      }
    bg.wheel_stepn=m;
    sc[SC_SHOW_WHEEL]++;
    make_modepar_file(GRAPHTYPE_BG);
    }
  break;

  case WM_MBUTTONDOWN:
  mbutton_state=1;
  break;

  case WM_MBUTTONUP:
  mbutton_state=0;
  break;

  case WM_LBUTTONDOWN:
  new_lbutton_state=1;
  goto mousepost; 

  case WM_LBUTTONUP:
  new_lbutton_state=0;
  goto mousepost;

  case WM_RBUTTONDOWN:
  new_rbutton_state=1;
  goto mousepost;

  case WM_RBUTTONUP:
  new_rbutton_state=0;
  goto mousepost;

  case WM_MOUSEMOVE:
  mx=new_mouse_x;
  my=new_mouse_y;
  new_mouse_x= LOWORD (msg.lParam);
  new_mouse_y= HIWORD (msg.lParam);
  if(new_mouse_x >= screen_width)new_mouse_x=screen_width-1;
  if(new_mouse_y >= screen_height)new_mouse_y=screen_height-1;
  if(new_mouse_x < 0)new_mouse_x=0;
  if(new_mouse_y < 0)new_mouse_y=0;
  if(  mx == new_mouse_x &&   my==new_mouse_y)break;
  if( (mx == new_mouse_x && new_mouse_x == screen_width-1) || 
      (my == new_mouse_y && new_mouse_y == screen_height-1) ||
      (mx == new_mouse_x && new_mouse_x == 0) || 
      (my == new_mouse_y && new_mouse_y == 0))break;
mousepost:
  if( thread_status_flag[THREAD_SCREEN] == THRFLAG_SEM_WAIT ||
      thread_status_flag[THREAD_SCREEN] == THRFLAG_ACTIVE ||
      thread_status_flag[THREAD_SCREEN] == THRFLAG_IDLE) 
    {
    lir_move_mouse_cursor();
    check_mouse_actions();
    }
}
}
}

// *********************************************************
// Mouse routines for Windows. Draw nothing on screen.
// *********************************************************

void unconditional_hide_mouse(void)
{
// Do nothing, Windows takes care of the mouse cursor
}

void show_mouse(void)
{
// Do nothing, Windows takes care of the mouse cursor
}


void hide_mouse(int x1, int x2, int iy1, int y2)
{
(void) x1;
(void) x2;
(void) iy1;
(void) y2;
// Do nothing, Windows takes care of the mouse cursor
}

void lir_move_mouse_cursor(void)
{
set_button_coordinates();
// We do not have a cursor of our own. Do nothing.
}

// *********************************************************************
// Thread entries for Windows
// *********************************************************************

DWORD WINAPI winthread_lir_server(PVOID arg)
{
(void) arg;
thread_lir_server();
return 100;
}

DWORD WINAPI winthread_blocking_rxout(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
blocking_rxout();
return 100;
}

DWORD WINAPI winthread_mix2(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
do_mix2();
return 100;
}

DWORD WINAPI winthread_fft3(PVOID arg)
{ 
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
do_fft3();
return 100;
}

DWORD WINAPI winthread_write_raw_file(PVOID arg)
{
(void) arg;
write_raw_file();
return 100;
}

DWORD WINAPI winthread_main_menu(PVOID arg)
{
(void) arg;
main_menu();
return 100;
}

DWORD WINAPI winthread_kill_all(PVOID arg)
{
(void) arg;
lir_await_event(EVENT_KILL_ALL);
kill_all();
clear_keyboard();
return 100;
}

DWORD WINAPI winthread_tune(PVOID arg)
{
(void) arg;
tune();
return 100;
}

DWORD WINAPI winthread_rx_output(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
rx_output();
return 100;
}


DWORD WINAPI winthread_sdr14_input(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
sdr14_input();
return 100;
}

DWORD WINAPI winthread_perseus_input(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
perseus_input();
return 100;
}

DWORD WINAPI winthread_hware_command(PVOID arg)
{
(void) arg;
hware_command();
return 100;
}

DWORD WINAPI winthread_sdrip_input(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
sdrip_input();
return 100;
}

DWORD WINAPI winthread_cloudiq_input(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
cloudiq_input();
return 100;
}

DWORD WINAPI winthread_netafedri_input(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
netafedri_input();
return 100;
}

DWORD WINAPI winthread_excalibur_input(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
excalibur_input();
return 100;
}

DWORD WINAPI winthread_radar(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST);
run_radar();
return 100;
}

DWORD WINAPI winthread_cal_filtercorr(PVOID arg)
{
(void) arg;
cal_filtercorr();
return 100;
}

DWORD WINAPI winthread_cal_interval(PVOID arg)
{
(void) arg;
do_cal_interval();
return 100;
}

DWORD WINAPI winthread_user_command(PVOID arg)
{
(void) arg;
user_command();
return 100;
}

DWORD WINAPI winthread_narrowband_dsp(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST);
narrowband_dsp();
return 100;
}

DWORD WINAPI winthread_wideband_dsp(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST);
wideband_dsp();
return 100;
}

DWORD WINAPI winthread_do_fft1c(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST);
do_fft1c();
return 100;
}

DWORD WINAPI winthread_fft1b(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST);
do_fft1b();
return 100;
}

DWORD WINAPI winthread_second_fft(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST);
second_fft();
return 100;
}

DWORD WINAPI winthread_timf2(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST);
timf2_routine();
return 100;
}

DWORD WINAPI winthread_tx_input(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
tx_input();
return 100;
}

DWORD WINAPI winthread_tx_output(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
run_tx_output();
return 100;
}

DWORD WINAPI winthread_screen(PVOID arg)
{
(void) arg;
screen_routine();
return 100;
}

DWORD WINAPI winthread_rx_file_input(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
rx_file_input();
return 100;
}

DWORD WINAPI winthread_rx_fft1_netinput(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
thread_rx_fft1_netinput();
return 100;
}

DWORD WINAPI winthread_rx_raw_netinput(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
thread_rx_raw_netinput();
return 100;
}

DWORD WINAPI winthread_cal_iqbalance(PVOID arg)
{
(void) arg;
cal_iqbalance();
return 100;
}

DWORD WINAPI winthread_rx_adtest(PVOID arg)
{
(void) arg;
rx_adtest();
return 100;
}

DWORD WINAPI winthread_powtim(PVOID arg)
{
(void) arg;
powtim();
return 100;
}

DWORD WINAPI winthread_txtest(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST);
txtest();
return 100;
}

DWORD WINAPI winthread_mouse(PVOID arg)
{
(void) arg;
wmouse();
return 100;
}

DWORD WINAPI winthread_syscall(PVOID arg)
{
(void) arg;
do_syscall();
return 100;
}

DWORD WINAPI winthread_extio_input(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
extio_input();
return 100;
}

DWORD WINAPI winthread_rtl2832_input(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
rtl2832_input();
return 100;
}

DWORD WINAPI winthread_airspy_input(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
airspy_input();
return 100;
}

DWORD WINAPI winthread_airspyhf_input(PVOID arg)
{
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
airspyhf_input();
return 100;
}

DWORD WINAPI winthread_mirics_input(PVOID arg)
{ 
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
mirics_input();
return 100;
}

DWORD WINAPI winthread_sdrplay2_input(PVOID arg)
{
  (void) arg;
  SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
  sdrplay2_input();
  return 100;
}

DWORD WINAPI winthread_sdrplay3_input(PVOID arg)
{
  (void) arg;
  SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
  sdrplay3_input();
  return 100;
}

DWORD WINAPI winthread_bladerf_input(PVOID arg)
{ 
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
bladerf_input();
return 100;
}

DWORD WINAPI winthread_pcie9842_input(PVOID arg)
{ 
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
pcie9842_input();
return 100;
}

DWORD WINAPI winthread_openhpsdr_input(PVOID arg)
{ 
(void) arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
openhpsdr_input();
return 100;
}

DWORD WINAPI winthread_rtl_starter(PVOID arg)
{
(void) arg;
rtl_starter();
return 100;
}

DWORD WINAPI winthread_mirisdr_starter(PVOID arg)
{
(void) arg;
mirisdr_starter();
return 100;
}

DWORD WINAPI winthread_bladerf_starter(PVOID arg)
{
(void) arg;
bladerf_starter();
return 100;
}

DWORD WINAPI winthread_html_server(PVOID arg)
{
(void) arg;
#if SERVER == 1
html_server();
#endif
return 100;
}

DWORD WINAPI winthread_network_send(PVOID arg)
{
(void) arg;
network_send();
return 100;
}

