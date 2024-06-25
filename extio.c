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
#include <string.h>
#include "uidef.h"
#include "screendef.h"
#include "hwaredef.h"
#include "seldef.h"
#include "thrdef.h"
#include "fft1def.h"
#include "keyboard_def.h"
#include "options.h"

void show_hardware_gain(void);

#define MAX_EXTIO_FILES 40
#define BUFFER_SIZE 256
char extio_name[BUFFER_SIZE], extio_model[BUFFER_SIZE];
double extio_start_time;

#if(OSNUM == OSNUM_WINDOWS)
#include <windows.h>
#include "wscreen.h"
#ifdef _MSC_VER
  #include <stdbool.h>
#endif
typedef bool (WINAPI *pOpenHW)(void);
typedef int (WINAPI *pSetHWLO)(long freq);
typedef void (WINAPI *pCloseHW)(void);
typedef long (WINAPI *pGetHWLO)(void);
typedef void (WINAPI *pSetCallback)(void (* Callback)(int,int,float,short*));
typedef long (WINAPI *pTuneChanged)(long);
typedef void (WINAPI *pSetRFGain)(int);
typedef int (WINAPI *pGetRFGain)(void);
typedef bool (WINAPI *pInitHW)(char *name, char *model,int *type);
typedef void (WINAPI *pShowGUI)(void);
typedef void (WINAPI *pHideGUI)(void);
typedef int (WINAPI *pStartHW)(long freq);
typedef void (WINAPI *pStopHW)(void);
typedef long (WINAPI *pGetHWSR)(void);
typedef long (WINAPI *pSetHWSR)(int rate);
#endif

#if(OSNUM == OSNUM_LINUX)
#include <pthread.h> 
#include "lscreen.h"
typedef bool (*pOpenHW)(void);
typedef int (*pSetHWLO)(long freq);
typedef void (*pCloseHW)(void);
typedef long (*pGetHWLO)(void);
typedef void (*pSetCallback)(void (* Callback)(int,int,float,short*));
typedef long (*pTuneChanged)(long);
typedef void (*pSetRFGain)(int);
typedef int (*pGetRFGain)(void);
typedef bool (*pInitHW)(char *name, char *model,int *type);
typedef void (*pShowGUI)(void);
typedef void (*pHideGUI)(void);
typedef int (*pStartHW)(long freq);
typedef void (*pStopHW)(void);
typedef long (*pGetHWSR)(void);
typedef int (*pSetHWSR)(int rate);
#endif

int extio_read_bytes;
int extio_cnt;

pOpenHW OpenHW;
pSetHWLO SetHWLO;
pCloseHW CloseHW;
pGetHWLO GetHWLO;
pSetCallback SetCallback;
pTuneChanged TuneChanged;
pSetRFGain SetRFGain;
pGetRFGain GetRFGain;
pInitHW InitHW;
pShowGUI ShowGUI;
pHideGUI HideGUI;
pStartHW StartHW;
pStopHW StopHW;
pGetHWSR GetHWSR;
pSetHWSR SetHWSR;

#define EXTIO_FREQ_CHANGED  101
#define EXTIO_RF_GAIN_CHANGED  210
#define EXTIO_SAMPRATE_CHANGED 100


#define RF_GAIN_UP_LIMIT 44 //dB
#define RF_GAIN_LOW_LIMIT -6 //dB

#define EXTIO_LIB_FILENAME "par_extio_libname" 

void write_extio_parfile(char *fnam)
{
FILE *file;
file=fopen(EXTIO_LIB_FILENAME,"w");
if(file == NULL)
  {
  lirerr(1236);
  return;
  }
clear_screen();
fprintf(file,"%s\n",fnam);
fclose(file);
lir_text(0,0,fnam);
lir_text(0,1,"Inverted spectrum? (Y/N):");
invspk:;
to_upper_await_keyboard();
if(lir_inkey == 'Y')
  {
  ui.use_extio=EXTIO_INVERTED;
  }
else
  {
  if(lir_inkey != 'N')goto invspk;
  ui.use_extio=EXTIO_NON_INVERTED;
  }
}

void get_library_filename(char *s)
{
FILE *file;
int i, k;
file=fopen(EXTIO_LIB_FILENAME,"r");
if(file == NULL)
  {
  ui.use_extio=0;
  sprintf(s,"Unknown file in %s",EXTIO_LIB_FILENAME);
  return;
  }
k=fread(s,1,BUFFER_SIZE,file);
if(k==BUFFER_SIZE)lirerr(1317);
fclose(file);
for(i=0; i<BUFFER_SIZE; i++)
  {
  if( s[i]==0 ||
      s[i]==10 ||
      s[i]==13)
    {
    s[i]=0;
    return;
    }    
  }
}

void get_extio_name(char *name)
{
int i,j;
char s[BUFFER_SIZE];
get_library_filename(&s[0]);
i=0;
while(s[i] != 0)
  {
  if(strncmp("ExtIO",&s[i],5) == 0)
    {
    j=0;
    while(s[i]!='.' && s[i]!=0 && j<40)
      {
      name[j]=s[i];
      i++;
      j++;
      }
    name[j]=0;  
    return;
    }
  i++;
  }
name[0]=0;
}      
    
void set_extio_uiparm(void)
{
char s[2*BUFFER_SIZE], ss[3*BUFFER_SIZE];
int line;
get_library_filename(&s[0]);
lir_text(28,3,s);
line=10;  
sprintf(s,"Name:  %s",extio_name);
lir_text(0,6,s);
sprintf(s,"Model: %s",extio_model);
lir_text(0,7,s);
sprintf(s,"Type:");
lir_text(0,8,s);
if( (ui.use_extio&EXTIO_INVERTED) != 0)
  {
  s[0]=0;
  }
else
  {
  sprintf(s,"non-");
  }
sprintf(ss,"Spectrum: %sinverted",s);
lir_text(2,line,ss);
line+=2;
switch(ui.extio_type)
  {
  case 1:
  case 3:
  lir_text(7,8,"int 16bit dll callback");
  ui.rx_input_mode=IQ_DATA;
  goto prt_callb;

  case 4:
  lir_text(7,8,"Soundcard input");
  lir_text(2,line,"Select the appropriate sound device");
  break;
  
  case 5:
  case 8:
  lir_text(7,8,"int 24bit dll callback");
  ui.rx_input_mode=IQ_DATA+BYTE_INPUT+DWORD_INPUT;
  goto prt_callb;
 
  case 6:
  lir_text(7,8,"int 32bit dll callback");
  ui.rx_input_mode=IQ_DATA+DWORD_INPUT+QWORD_INPUT;
  goto prt_callb;
  
  case 7:
  lir_text(7,8,"float 32bit dll callback");
  ui.rx_input_mode=IQ_DATA+DWORD_INPUT+FLOAT_INPUT;
prt_callb:;
  lir_text(2,line,
     "Set sampling speed and other parameters in the ExtIO window");
  ui.rx_addev_no=EXTIO_DEVICE_CODE;
  break;
  
  default:
  lirerr(1341);
  return; 
  }
line+=2;
lir_text(7,line,press_any_key);
await_processed_keyboard();  
}


void callback_extio(int cnt, int status, float IQoffs, short *IQdata)
{
char *ch, *dh;
int *iz, *ix;
long int lf;
float *z;
int i, j;
(void) IQoffs;
if(kill_all_flag || timf1p_sdr == -1)goto skip;
switch(status)
  {
  case EXTIO_FREQ_CHANGED:
  if(GetHWLO)
    {
    lf=GetHWLO();
    fg.passband_center = (double)(lf) / 1e6;
    set_hardware_rx_frequency();
    }
  break;

  case EXTIO_RF_GAIN_CHANGED:
  if(GetRFGain)
    {
    fg.gain = GetRFGain();
    if(fg.gain > RF_GAIN_UP_LIMIT) fg.gain = RF_GAIN_UP_LIMIT;
    if(fg.gain < RF_GAIN_LOW_LIMIT) fg.gain = RF_GAIN_LOW_LIMIT;
    fg.gain_increment = 1;
    show_hardware_gain();
    }
  break;
  
  case EXTIO_SAMPRATE_CHANGED:
  if(ui.extio_type != 4)
    {
    if(!GetHWSR)
      {
      lirerr(131600);
      }
    if(ui.rx_ad_speed!=GetHWSR())extio_speed_changed=TRUE;
    }
  break;
  }
if(ui.extio_type == 4 || cnt < 0)return;
if(cnt >= 0 && thread_status_flag[THREAD_EXTIO_INPUT] == THRFLAG_ACTIVE)
  {
  extio_cnt=cnt;
  switch(ui.extio_type)
    {
    case 1:
    case 3:
    case 6:
    ix=(int*)IQdata;
    j=extio_read_bytes/4;
    for(i=0; i<j; i++)
      {
      iz=(int*)&timf1_char[timf1p_sdr];
      iz[0]=ix[i];
      timf1p_sdr=(timf1p_sdr+4)&timf1_bytemask;
      }
    break;

    case 5:
    case 8:
    ch=(char*)IQdata;
    j=extio_read_bytes>>2;
    for(i=0; i<j; i+=2)
      {
      dh=&timf1_char[timf1p_sdr];
      dh[3]=ch[3*i+2];
      dh[2]=ch[3*i+1];
      dh[1]=ch[3*i  ];
      dh[0]=0;
      dh[7]=ch[3*i+5];
      dh[6]=ch[3*i+4];
      dh[5]=ch[3*i+3];
      dh[4]=0;
      timf1p_sdr=(timf1p_sdr+8)&timf1_bytemask;
      } 
    break;

    case 7:
    z=(float*)IQdata;
    j=extio_read_bytes>>2;
    for(i=0; i<j; i++)
      {
      iz=(int*)&timf1_char[timf1p_sdr];
      iz[0]=(int)(0x7fffffff*z[i]);
      timf1p_sdr=(timf1p_sdr+4)&timf1_bytemask;
      }
    break;
    }
  if( ((timf1p_sdr-timf1p_pa+timf1_bytes)&timf1_bytemask) >= 
                                                     snd[RXAD].block_bytes)
    {
skip:;
    lir_set_event(EVENT_EXTIO_RXREADY);
    }
  }
}

  
#if(OSNUM == OSNUM_LINUX)

#include <dirent.h>
#include <sys/types.h>
#include <sys/param.h>
#include <dlfcn.h>
#include <semaphore.h>

#include "thrdef.h"

#if(IA64 == 0)
char *dirs[]={"./",
              "/lib",
              "/lib32",
              "/usr/lib",
              "/usr/lib32",
              "/usr/lib32/lib",
              "/usr/local/lib",
              "/usr/local32/lib",
              "/emul/ia32-linux/lib",
              "/emul/ia32-linux/usr/lib",              
              "/usr/lib/i386-linux-gnu",
              "X"};
#else
char *dirs[]={"./",
              "/lib",
              "/lib64",
              "/usr/lib",
              "/usr/lib64",
              "/usr/local/lib",
              "/usr/lib/x86_64-linux-gnu",
              "X"};
#endif


void init_extio(void)
{
char extio_lib_names[MAX_EXTIO_FILES][BUFFER_SIZE];
DIR *dirp;
char s[4*BUFFER_SIZE];
int i, m, num_extio;
int lineno;
struct dirent *dp;
i=0;  
lineno=0;
lir_text(10,1,"Seaching libraries for ExtIO files");
lir_refresh_screen();
while(dirs[i][0] != 'X' && lineno < MAX_EXTIO_FILES)
  {
  dirp=opendir(dirs[i]);
  if(dirp != NULL)
    {
next_file:;
    dp=readdir(dirp);
    if(dp != NULL)
      {
      m=strncmp("libExtIO",dp->d_name,8);
      if(m == 0)
        {
        sprintf(extio_lib_names[lineno],"%s/%s", dirs[i], dp->d_name);
        sprintf(s,"%2d %s",lineno,extio_lib_names[lineno]);
        lir_text(3,lineno+4,s);
        lineno++;
        if(lineno >= MAX_EXTIO_FILES || lineno+4 > screen_last_line)
          {
          settextcolor(12);
          lir_text(3,lineno+4,"Too many ExtIO libraries");
          settextcolor(7);
          goto skip;
          }
        }
      goto next_file;
      }
    closedir(dirp);  
    }
  i++;  
  }
skip:;
if(lineno == 0)
  {
  lir_text(8,lineno+6,"No libExtIO_xxx.so file found");
  lir_text(5,lineno+8,press_any_key);
  await_keyboard();
  return;
  }  
else
  {
  lir_text(8,lineno+6,"Select libExtIO_xxx.so file by line number:");
  num_extio=lir_get_integer(51,lineno+6,2,0,lineno-1);  
  if(kill_all_flag)return;
  }
write_extio_parfile(extio_lib_names[num_extio]);
if(ui.use_extio != 0)
  {
  command_extio_library(EXTIO_COMMAND_LOAD_DLL);
  if(!extio_handle)
    {
    lir_text(8,lineno+6,"Loading libExtIO_xxx.so failed.");
    lir_text(5,lineno+8,press_any_key);
    await_keyboard();
    return;
    }
  set_extio_uiparm();
  }  
lir_sleep(10000);
if(extio_handle)
  {
// Start and stop so we get all the parameters into the ui structure.  
  timf1p_pa=-1;
  extio_show_gui=FALSE;
  command_extio_library(EXTIO_COMMAND_START);
  command_extio_library(EXTIO_COMMAND_STOP);
  command_extio_library(EXTIO_COMMAND_UNLOAD_DLL);
  }    
}

void command_extio_library(int load)
{
extio_command_flag=load;
lir_set_event(EVENT_MANAGE_EXTIO);
if(load==EXTIO_COMMAND_KILL_ALL)return;
while(extio_command_flag != EXTIO_COMMAND_DONE)
  {
  lir_sleep(20000);
  }
}  

int load_extio_library(void)
{
char s[BUFFER_SIZE];
int errcod;
if(extio_handle != NULL)
  {
  lirerr(675433);
  return 99;
  }
clear_screen();
lir_text(3,3,"LOADING ExtIO LIBRARY");   
lir_refresh_screen();
get_library_filename(&s[0]);
if(ui.use_extio==0)return 1;
extio_handle=dlopen(s, RTLD_LAZY);
if( extio_handle==NULL) return 2;
OpenHW = (pOpenHW)dlsym(extio_handle, "OpenHW");
if(OpenHW == NULL)
  {
  errcod=3;
  OpenHW=NULL;
  goto errexit;
  }
SetHWLO = (pSetHWLO) dlsym(extio_handle,"SetHWLO");
if(SetHWLO == NULL)
  {
  errcod=4;
  goto errexit;
  }
CloseHW = (pCloseHW) dlsym(extio_handle,"CloseHW");
if(CloseHW == NULL)
  {
  errcod=5;
  goto errexit;
  }
SetCallback = (pSetCallback) dlsym(extio_handle, "SetCallback");
if(SetCallback == NULL)
  {
  errcod=6;
  goto errexit;
  }
InitHW = (pInitHW) dlsym(extio_handle, "InitHW");
if(InitHW == NULL)
  {
  errcod=7;
  goto errexit;
  }
StopHW = (pStopHW) dlsym(extio_handle, "StopHW");
if(StopHW == NULL)
  {
  errcod=8;
  goto errexit;
  }
ShowGUI = (pShowGUI) dlsym(extio_handle, "ShowGUI");
if(ShowGUI == NULL)ShowGUI=NULL;
HideGUI = (pHideGUI) dlsym(extio_handle, "HideGUI");
StartHW = (pStartHW) dlsym(extio_handle, "StartHW");
TuneChanged = (pTuneChanged) dlsym(extio_handle, "TuneChanged");
GetHWLO = (pGetHWLO) dlsym(extio_handle, "GetHWLO");
GetHWSR = (pGetHWSR) dlsym(extio_handle, "GetHWSR");
SetHWSR = (pSetHWSR) dlsym(extio_handle, "SetHWSR");
SetRFGain = (pSetRFGain) dlsym(extio_handle, "SetRFGain");
GetRFGain = (pGetRFGain) dlsym(extio_handle, "GetRFGain");
errcod=InitHW(extio_name, extio_model, &ui.extio_type);
if(ui.extio_type == 2)ui.extio_type=4;
if(!errcod)
  {
  errcod=9;
  goto errexit;
  }
if(kill_all_flag)return 99;
extio_running=1;
return 0;
errexit:;
unload_extio_library();
return errcod;
}

void unload_extio_library(void)
{
if(extio_running == 2)stop_extio();
dlclose(extio_handle);
extio_handle=NULL;
lir_sched_yield();
}  



#endif

#if(OSNUM == OSNUM_WINDOWS)
#include "wdef.h"

void init_extio(void)
{
char s[BUFFER_SIZE];
char dirnam[BUFFER_SIZE];
WIN32_FIND_DATA file_data;
UINT charnum;
HANDLE find_handle;
char extio_lib_names[MAX_EXTIO_FILES][BUFFER_SIZE];
int i, num_extio;
int lineno;
i=0;
lineno=0;
lir_text(10,1,"Seaching libraries for ExtIO files");
lir_refresh_screen();
charnum=0;
while(i < 2)
  {
  if(i == 0) 
    {
    charnum=GetCurrentDirectory(BUFFER_SIZE, dirnam);
    }
  if(i == 1)
    {
    sprintf(dirnam,"%s",DLLDIR);
// Do not include the last backslash.
    charnum=strlen(dirnam)-1;
    }
  sprintf(&dirnam[charnum],"\\Extio*.dll");
  find_handle=FindFirstFile(dirnam, &file_data);
  if(find_handle == INVALID_HANDLE_VALUE)goto nomore;
more:;
  if(i==0)
    {
    sprintf(extio_lib_names[lineno],"%s",file_data.cFileName);
    sprintf(s,"%2d %s",lineno,extio_lib_names[lineno]);
    lir_text(3,lineno+4,s);
    }
  else
    {  
    sprintf(extio_lib_names[lineno],"%s", dirnam); 
    sprintf(&extio_lib_names[lineno][charnum+1],"%s",file_data.cFileName);
    sprintf(s,"%2d %s",lineno,extio_lib_names[lineno]);
    lir_text(3,lineno+4,s);
    }
  lineno++;
  if(lineno >= MAX_EXTIO_FILES || lineno+4 > screen_last_line)goto max;
  if(FindNextFile(find_handle, &file_data))goto more;
nomore:;
  if(find_handle != INVALID_HANDLE_VALUE)FindClose(find_handle);
  i++;  
  if(lineno >= MAX_EXTIO_FILES || lineno+4 > screen_last_line)
    {
max:;
    settextcolor(12);
    lir_text(3,lineno+4,"Too many ExtIO libraries");
    settextcolor(7);
    goto skip;
    }
  }
skip:;
if(lineno == 0)
  {
  lir_text(8,lineno+6,"No ExtIOxxx.dll file found");
  lir_text(5,lineno+8,press_any_key);
  await_keyboard();
  return;
  }  
else
  {
  lir_text(8,lineno+6,"Select ExtIO_xxx.dll file by line number:");
  num_extio=lir_get_integer(51,lineno+6,2,0,lineno-1);  
  if(kill_all_flag)return;
  }
write_extio_parfile(extio_lib_names[num_extio]);
if(ui.use_extio != 0)
  {
  command_extio_library(EXTIO_COMMAND_LOAD_DLL);
  if(!extio_handle)
    {
    lir_text(8,lineno+6,"Loading libExtIO_xxx.so failed.");
    lir_text(5,lineno+8,press_any_key);
    await_keyboard();
    return;
    }
  set_extio_uiparm();
  }  
lir_sleep(10000);
if(extio_handle)
  {
// Start and stop so we get all the parameters into the ui structure.  
  timf1p_pa=-1;
  extio_show_gui=FALSE;
  command_extio_library(EXTIO_COMMAND_START);
  command_extio_library(EXTIO_COMMAND_STOP);
  command_extio_library(EXTIO_COMMAND_UNLOAD_DLL);
  }    
}

void command_extio_library(int load)
{
extio_command_flag=load;
PostMessage(linrad_hwnd, WM_USER, load, 0);
if(load==EXTIO_COMMAND_KILL_ALL)return;
while(extio_command_flag != EXTIO_COMMAND_DONE)
  {
  lir_sleep(20000);
  }
}  

int load_extio_library(void)
{
char s[BUFFER_SIZE];
int errcod;
if(extio_handle != NULL)
  {
  lirerr(675433);
  return 99;
  }
clear_screen();
lir_text(3,3,"LOADING ExtIO LIBRARY");   
lir_refresh_screen();
get_library_filename(&s[0]);
if(ui.use_extio==0)return 1;
extio_handle=LoadLibraryEx(s, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
if(extio_handle == NULL)return 2;
OpenHW = (pOpenHW)(void*)GetProcAddress(extio_handle, "OpenHW");
if(!OpenHW)
  {
  errcod=3;
  goto errexit;
  }
SetHWLO = (pSetHWLO) (void*)GetProcAddress(extio_handle, "SetHWLO");
if(!SetHWLO)
  {
  errcod=4;
  goto errexit;
  }
CloseHW = (pCloseHW) (void*)GetProcAddress(extio_handle, "CloseHW");
if(!CloseHW)
  {
  errcod=5;
  goto errexit;
  }
SetCallback = (pSetCallback) (void*)GetProcAddress(extio_handle, "SetCallback");
if(!SetCallback)
  {
  errcod=6;
  goto errexit;
  }
InitHW = (pInitHW) (void*)GetProcAddress(extio_handle, "InitHW");
if(!InitHW)
  {
  errcod=7;
  goto errexit;
  }
StopHW = (pStopHW) (void*)GetProcAddress(extio_handle, "StopHW");
if(!StopHW)
  {
  errcod=8;
  goto errexit;
  }
ShowGUI = (pShowGUI) (void*)GetProcAddress(extio_handle, "ShowGUI");
HideGUI = (pHideGUI) (void*)GetProcAddress(extio_handle, "HideGUI");
StartHW = (pStartHW) (void*)GetProcAddress(extio_handle, "StartHW");
TuneChanged = (pTuneChanged) (void*)GetProcAddress(extio_handle, "TuneChanged");
GetHWLO = (pGetHWLO) (void*)GetProcAddress(extio_handle, "GetHWLO");
SetRFGain = (pSetRFGain) (void*)GetProcAddress(extio_handle, "SetRFGain");
GetRFGain = (pGetRFGain) (void*)GetProcAddress(extio_handle, "GetRFGain");
GetHWSR = (pGetHWSR) (void*)GetProcAddress(extio_handle, "GetHWSR");
SetHWSR = (pSetHWSR) (void*)GetProcAddress(extio_handle, "SetHWSR");
CoInitialize(NULL);
errcod=InitHW(extio_name, extio_model, &ui.extio_type);
extio_running=1;
if(ui.extio_type == 2)ui.extio_type=4;
if(errcod <= 0)
  {
  errcod=9;
  CoUninitialize();
  goto errexit;
  }
if(kill_all_flag)return 99;
return 0;
errexit:;
unload_extio_library();
return errcod;
}

void unload_extio_library(void)
{
if(extio_running == 2)
  {
  stop_extio();
  }
FreeLibrary(extio_handle);
extio_handle=NULL;
CoUninitialize();
lir_sched_yield();
}  
#endif


void first_check_extio(void)
{
char s[BUFFER_SIZE];
if(extio_handle == NULL)
  {
  clear_screen();
  get_library_filename(&s[0]);
  lir_text(3,3,"ERROR: dynamic library for ExtIO failed.");
  lir_text(3,5,s);
  lir_text(3,7,"Use of this library is disabled. Press W on the main menu");
  lir_text(3,8,"if you want to disable it permanently.");
  lir_text(8,11,press_any_key);
  await_processed_keyboard();
  ui.use_extio=0;
  return;
  }
}

void start_extio(void)
{
char s[80];
int i, errcod, siz;
long TuneFreq;
if( extio_handle == NULL)
  {
  lirerr(522156);
  return;
  }
clear_screen();
lir_text(3,3,"Starting input hardware using ExtIO library.");
lir_text(3,4,"Note that it may take 10 seconds until the Extio GUI appears");
settextcolor(15);
lir_text(3,7,"Move mouse into ExtIO window if Linrad hangs here.");
settextcolor(7);
lir_init_event(EVENT_EXTIO_RXREADY);
lir_text(15,13,"Calling SetCallback");
lir_refresh_screen();
SetCallback(callback_extio);
lir_text(15,13,"SetCallback OK     ");
lir_refresh_screen();
lir_text(15,14,"Calling OpenHW");
lir_refresh_screen();
errcod=OpenHW();
if(errcod < 1)
  {
  lir_text(15,14,"OpenHW returned an ERROR");
  lir_refresh_screen();
  unload_extio_library();
  lirerr(1346);
  return;
  }
lir_text(15,14,"OpenHW OK     ");
lir_refresh_screen();
fg.gain_increment = 1;
if(abs(fg.passband_direction) != 1)fg.passband_direction=1; 
if(fg.passband_increment < 0.0001 || fg.passband_increment > 1.5)
  {
  fg.passband_increment=.01;
  }
if(fg.passband_center == 0)fg.passband_center=7;

fg.passband_center=144;
TuneFreq = (long)(1000000L*fg.passband_center);
lir_text(15,15,"Calling StartHW");
lir_refresh_screen();
extio_cnt=0;
siz=StartHW(TuneFreq);
sprintf(s,"StartHW returned size = %d   type = %d",siz,ui.extio_type);
lir_text(15,15,s);
lir_refresh_screen();
switch(ui.extio_type)
  {
  case 1:
  case 3:
// 16 bit integer    
  ui.rx_input_mode=IQ_DATA;
  snd[RXAD].framesize=4;
  break;

  case 4:
  break;
    
  case 5:
  case 8:
// 24 bit integer    
  ui.rx_input_mode=IQ_DATA+BYTE_INPUT+DWORD_INPUT;
  snd[RXAD].framesize=8;
  break;
    
  case 6:
// 32 bit integer    
  ui.rx_input_mode=IQ_DATA+DWORD_INPUT+QWORD_INPUT;
  snd[RXAD].framesize=8;
  break;
    
  case 7:
// 32 bit float
  ui.rx_input_mode=IQ_DATA+DWORD_INPUT+FLOAT_INPUT;
  snd[RXAD].framesize=8;
  break;
   
  default:
  lirerr(984546);
  break;
  }
if(ui.extio_type < 1 || ui.extio_type > 8)
  {
  lirerr(1239);
  return;
  }
if(ui.extio_type != 4)
  {
  if(siz <= 0)
    {
    i=30;
    while(extio_cnt == 0 && i>0)
      {
      lir_sleep(100000);
      i--;
      }
    siz=extio_cnt;
    }  
  if(siz == 0)
    {
    lirerr(1344);
    return;
    }
  if(siz < 0)
    {
    lirerr(1347);
    return;
    }
  extio_read_bytes=siz*snd[RXAD].framesize;
  if(!GetHWSR)
    {
    lirerr(1316);
    return;
    }
  if(ui.rx_ad_speed!=GetHWSR())extio_speed_changed=TRUE;
  timf1p_sdr=timf1p_pa;  
  make_power_of_two(&siz);
  snd[RXAD].block_frames=siz;
  snd[RXAD].block_bytes=snd[RXAD].framesize*siz;
  snd[RXAD].interrupt_rate=ui.rx_ad_speed/siz;
  }    
lir_refresh_screen();
if(ui.extio_type != 4)
  {
  if(!GetHWSR)
    {
    lirerr(1316);
    return;
    }
  if(SetHWSR)
    {
    SetHWSR(ui.rx_ad_speed);
    }  
  i=ui.rx_ad_speed;  
  ui.rx_ad_speed=GetHWSR();
  if(i == ui.rx_ad_speed)
    {
    extio_speed_changed=FALSE;
    }
  else
    {
    extio_speed_changed=TRUE;
    }
  ui.rx_rf_channels=1;
  ui.rx_ad_channels=2;
  }
extio_start_time=current_time();
extio_running=2;
clear_screen();
if(ShowGUI != NULL)
  {
  if(extio_show_gui)ShowGUI();
  }
else
  {
  extio_show_gui=FALSE;
  }
}
  
void stop_extio(void)
{
char s[80];
if(extio_running != 2)
  {
  lirerr(659473);
  return;
  }
clear_screen();  
if(HideGUI != NULL)
  {
  if(extio_show_gui)HideGUI();
  lir_sleep(30000);
  }
while(current_time()-extio_start_time < 4)
  {
  sprintf(s,"Waiting before stop %.2f",4-current_time()+extio_start_time);
  lir_text(3,8,s);
  lir_refresh_screen();
  lir_sleep(200000);
  }
StopHW();
lir_close_event(EVENT_EXTIO_RXREADY);
CloseHW();
extio_running=1;
}      

void update_extio_rx_freq(void)
{
long TuneFreq;
if(extio_running != 2)return;
TuneFreq = (long)(1000000L*fg.passband_center);
SetHWLO(TuneFreq);
}

void update_extio_rx_gain(void)
{
if(extio_running == 2 && SetRFGain)
  {
  if(fg.gain > RF_GAIN_UP_LIMIT) fg.gain = RF_GAIN_UP_LIMIT;
  if(fg.gain < RF_GAIN_LOW_LIMIT) fg.gain = RF_GAIN_LOW_LIMIT;	
  SetRFGain(fg.gain);
  }
}

void extio_input(void)
{
int errcod;
int rxin_local_workload_reset;
double read_start_time, total_reads;
int timing_loop_counter,timing_loop_counter_max,initial_skip_flag;
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_EXTIO_INPUT);
#endif
errcod=0;
fft1_block_timing();
if(thread_command_flag[THREAD_SCREEN]!=THRFLAG_NOT_ACTIVE)
  {
  while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
    {
    if(thread_command_flag[THREAD_EXTIO_INPUT] == THRFLAG_KILL)goto extio_exit;
    lir_sleep(10000);
    }
  }
thread_status_flag[THREAD_EXTIO_INPUT]=THRFLAG_ACTIVE;
update_extio_rx_freq();
update_extio_rx_gain();
#include "timing_setup.c"
lir_status=LIR_OK;
while(thread_command_flag[THREAD_EXTIO_INPUT] == THRFLAG_ACTIVE)
  {
  while( thread_command_flag[THREAD_EXTIO_INPUT] == THRFLAG_ACTIVE &&
         !kill_all_flag && 
         ((timf1p_sdr-timf1p_pa+timf1_bytes)&timf1_bytemask) < 
                                                     (int)snd[RXAD].block_bytes) 
    {
    lir_await_event(EVENT_EXTIO_RXREADY);
    }
  while (!kill_all_flag && ((timf1p_sdr-timf1p_pa+timf1_bytes)&
                        timf1_bytemask) >= (int)snd[RXAD].block_bytes)
    {
#include "input_speed.c"
    finish_rx_read();
    }                       
  if(kill_all_flag)goto extio_exit;
  if(extio_speed_changed)
    {    
    while(keyboard_buffer_ptr != keyboard_buffer_used)lir_sleep(1000);
    keyboard_buffer[keyboard_buffer_used]='X';
    keyboard_buffer_ptr=(keyboard_buffer_ptr+1)&(KEYBOARD_BUFFER_SIZE-1);
    lir_sleep(10000);
    lir_set_event(EVENT_KEYBOARD);
    lir_sleep(100000);
    if(lir_inkey == 'X')
      {
      ui.rx_ad_speed=GetHWSR();
      extio_speed_changed=FALSE;
      }
    if(kill_all_flag)goto extio_exit;
    }    
  }
extio_exit:;
if(errcod != 0)lirerr(errcod);
thread_status_flag[THREAD_EXTIO_INPUT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_EXTIO_INPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

