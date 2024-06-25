
#include "osnum.h"
#include "globdef.h"
#include <string.h>
#include "uidef.h"
#include "screendef.h"
#include "thrdef.h"
#include "vernr.h"
#include "sdrdef.h"
#include "usbdefs.h"
#include "fft1def.h"
#include "options.h"

#ifdef __FreeBSD__
#include <sys/types.h>
#endif


int halfbuf;
int callback_blocksize;
short int card;
int pcie9842_samples;

struct {
  /* One large memory chunk will be allocated for two buffers. */
  short *data_unaligned;
  short *data_aligned;
  /* The buffers are fille by the callback function in double-buffering mode. */
  short *buffer[2];
  unsigned short bufferId[2];
} digitizer_globals;

// Structure for PCIE9842 parameters
typedef struct {
int rate_dividend;
int resampling_factor;
int freq_adjust;
int check;
}P_PCIE9842;
#define MAX_PCIE9842_PARM 4

P_PCIE9842 pcie9842;
char *pcie9842_parm_text[MAX_PCIE9842_PARM]={"Rate dividend",
                                     "Resampling factor",
                                     "Freq adjust",
                                     "Check"};

char *pcie9842_parfil_name="par_pcie9842";
#define PCIe_9842       0x30
#define NoError 0
#define ASYNCH_OP       2
// Notify when the next buffer is ready for transfer.
//DAQ Event type for the event message  
#define DBEvent  1

#define MIN_PCIE9842_DIVIDEND 1
#define MAX_PCIE9842_DIVIDEND 20000

#if(OSNUM == OSNUM_WINDOWS)
HANDLE pcie_libhandle;
typedef short (WINAPI *pRegister_Card)(u_int16_t CardType, u_int16_t card_num);
typedef short (WINAPI *pAI_Config)(u_int16_t wCardNumber, u_int16_t TimeBase, 
         unsigned int adDutyRestore, u_int16_t ConvSrc, unsigned int doubleEdged, 
            unsigned int AutoResetBuf);
typedef short (WINAPI *pRelease_Card )(u_int16_t CardNumber);
typedef short (WINAPI *pAI_ContBufferSetup)(u_int16_t wCardNumber, void *pwBuffer, 
                             u_int32_t dwReadCount, u_int16_t *BufferId);
typedef short (WINAPI *pAI_EventCallBack)(u_int16_t wCardNumber, short mode, 
                            short EventType, void(*callbackAddr)(int));
typedef short (WINAPI *pAI_ContReadChannel)(u_int16_t CardNumber, 
          u_int16_t Channel, u_int16_t BufId, u_int32_t ReadScans, 
               u_int32_t ScanIntrv, u_int32_t SampIntrv, u_int16_t SyncMode);
typedef short (WINAPI *pAI_ContReadMultiChannels)(u_int16_t CardNumber, 
          u_int16_t NumChans, u_int16_t *Chans, u_int16_t BufId, 
            u_int32_t ReadScans, u_int32_t ScanIntrv, u_int32_t SampIntrv, 
                                                         u_int16_t SyncMode);
typedef short (WINAPI *pAI_AsyncClear)(u_int16_t CardNumber, u_int32_t *StartPos, 
                                                     u_int32_t *AccessCnt);               
typedef short (WINAPI *pAI_ContBufferReset)(u_int16_t wCardNumber);
typedef short (WINAPI *pAI_Set_Mode)(u_int16_t  wCardNumber, 
                                        u_int16_t  modeCtrl, u_int16_t  wIter);
typedef short (WINAPI *pAI_AsyncDblBufferMode)(u_int16_t CardNumber, unsigned int Enable);
typedef short (WINAPI *pAI_AsyncDblBufferHandled)(u_int16_t wCardNumber);
typedef short (WINAPI *pAI_InitialMemoryAllocated)(u_int16_t CardNumber, 
                                                     u_int32_t  *MemSize);
typedef short (WINAPI *pAI_AsyncDblBufferHalfReady)(u_int16_t CardNumber, 
                               unsigned int *HalfReady, unsigned int *StopFlag);
#endif


#if(OSNUM == OSNUM_LINUX)
#include <dlfcn.h>
void *pcie_libhandle;
typedef short (*pRegister_Card)(u_int16_t CardType, u_int16_t card_num);
typedef short (*pAI_Config)(u_int16_t wCardNumber, u_int16_t TimeBase, 
                           unsigned int adDutyRestore, u_int16_t ConvSrc, 
                        unsigned int doubleEdged, unsigned int AutoResetBuf);
typedef short (*pRelease_Card )(u_int16_t CardNumber);
typedef short (*pAI_ContBufferSetup)(u_int16_t wCardNumber, void *pwBuffer, 
                                u_int32_t dwReadCount, u_int16_t *BufferId);
typedef short (*pAI_EventCallBack)(u_int16_t wCardNumber, short mode, 
                            short EventType, void(*callbackAddr)(int));
typedef short (*pAI_ContReadChannel)(u_int16_t CardNumber, u_int16_t Channel,
                   u_int16_t BufId, u_int32_t ReadScans, u_int32_t ScanIntrv, 
                                      u_int32_t SampIntrv, u_int16_t SyncMode);
typedef short (*pAI_ContReadMultiChannels)(u_int16_t CardNumber, 
          u_int16_t NumChans, u_int16_t *Chans, u_int16_t BufId, 
            u_int32_t ReadScans, u_int32_t ScanIntrv, u_int32_t SampIntrv, 
                                                         u_int16_t SyncMode);
typedef short (*pAI_AsyncClear)(u_int16_t CardNumber, u_int32_t *StartPos, 
                                                        u_int32_t *AccessCnt);               
typedef short (*pAI_ContBufferReset)(u_int16_t wCardNumber);
typedef short (*pAI_Set_Mode)(u_int16_t  wCardNumber, 
                                     u_int16_t  modeCtrl, u_int16_t  wIter);
typedef short (*pAI_AsyncDblBufferMode)(u_int16_t CardNumber, 
                                                        unsigned int Enable);
typedef short (*pAI_AsyncDblBufferHandled)(u_int16_t wCardNumber);
typedef short (*pAI_InitialMemoryAllocated)(u_int16_t CardNumber, 
                                                      u_int32_t  *MemSize);
typedef short (*pAI_AsyncDblBufferHalfReady)(u_int16_t CardNumber, 
                            unsigned int *HalfReady, unsigned int *StopFlag);
#endif


pRegister_Card WD_Register_Card;
pAI_Config WD_AI_Config;
pRelease_Card WD_Release_Card;
pAI_ContBufferSetup WD_AI_ContBufferSetup;
pAI_EventCallBack WD_AI_EventCallBack;
pAI_ContReadChannel WD_AI_ContReadChannel;
pAI_ContReadMultiChannels WD_AI_ContReadMultiChannels;
pAI_AsyncClear WD_AI_AsyncClear;
pAI_ContBufferReset WD_AI_ContBufferReset;
pAI_Set_Mode WD_AI_Set_Mode;
pAI_AsyncDblBufferMode WD_AI_AsyncDblBufferMode;
pAI_AsyncDblBufferHandled WD_AI_AsyncDblBufferHandled;
pAI_InitialMemoryAllocated WD_AI_InitialMemoryAllocated;
pAI_AsyncDblBufferHalfReady WD_AI_AsyncDblBufferHalfReady;



#if(OSNUM == OSNUM_LINUX)
int load_pcie9842_library(void)
{
int info;
info=0;
pcie_libhandle=dlopen(PCI_LIBNAME, RTLD_LAZY);
if(pcie_libhandle == NULL)goto load_error; 
info=1;
WD_Register_Card=(pRegister_Card)dlsym(pcie_libhandle, "WD_Register_Card");
if(WD_Register_Card == NULL)goto sym_error;
WD_AI_Config=(pAI_Config)dlsym(pcie_libhandle, "WD_AI_Config");
if(WD_AI_Config == NULL)goto sym_error;
WD_Release_Card=(pRelease_Card)dlsym(pcie_libhandle, "WD_Release_Card");
if(WD_Release_Card == NULL)goto sym_error;
WD_AI_ContBufferSetup=(pAI_ContBufferSetup)
                        dlsym(pcie_libhandle, "WD_AI_ContBufferSetup");
if(WD_AI_ContBufferSetup == NULL)goto sym_error;
WD_AI_EventCallBack=(pAI_EventCallBack)
                          dlsym(pcie_libhandle, "WD_AI_EventCallBack");
if(WD_AI_EventCallBack == NULL)goto sym_error;
WD_AI_ContReadChannel=(pAI_ContReadChannel)
                           dlsym(pcie_libhandle, "WD_AI_ContReadChannel");
if(WD_AI_ContReadChannel == NULL)goto sym_error;
WD_AI_ContReadMultiChannels=(pAI_ContReadMultiChannels)
                           dlsym(pcie_libhandle, "WD_AI_ContReadMultiChannels");
if(WD_AI_ContReadMultiChannels == NULL)goto sym_error;
WD_AI_AsyncClear=(pAI_AsyncClear)
                                 dlsym(pcie_libhandle, "WD_AI_AsyncClear");
if(WD_AI_AsyncClear == NULL)goto sym_error;
WD_AI_ContBufferReset=(pAI_ContBufferReset)
                               dlsym(pcie_libhandle, "WD_AI_ContBufferReset");
if(WD_AI_ContBufferReset == NULL)goto sym_error;
WD_AI_Set_Mode=(pAI_Set_Mode)dlsym(pcie_libhandle, "WD_AI_Set_Mode");
if(WD_AI_Set_Mode == NULL)goto sym_error;
WD_AI_AsyncDblBufferMode=(pAI_AsyncDblBufferMode)
                          dlsym(pcie_libhandle, "WD_AI_AsyncDblBufferMode");
if(WD_AI_AsyncDblBufferMode == NULL)goto sym_error;
WD_AI_AsyncDblBufferHandled=(pAI_AsyncDblBufferHandled)
                         dlsym(pcie_libhandle, "WD_AI_AsyncDblBufferHandled");
if(WD_AI_AsyncDblBufferHandled == NULL)goto sym_error;
WD_AI_InitialMemoryAllocated=(pAI_InitialMemoryAllocated)
                        dlsym(pcie_libhandle, "WD_AI_InitialMemoryAllocated");
if(WD_AI_InitialMemoryAllocated == NULL)goto sym_error;
WD_AI_AsyncDblBufferHalfReady=(pAI_AsyncDblBufferHalfReady)
                       dlsym(pcie_libhandle, "WD_AI_AsyncDblBufferHalfReady");
if(WD_AI_AsyncDblBufferHalfReady == NULL)goto sym_error;
return 0;
sym_error:;
dlclose(pcie_libhandle);
load_error:;
library_error_screen(PCI_LIBNAME,info);
return -1;
}

void unload_pcie9842_library(void)
{
dlclose(pcie_libhandle);
}
#endif


#if(OSNUM == OSNUM_WINDOWS)

int load_pcie9842_library(void)
{
char pcie_dllname[80];
int info;
info=0;
sprintf(pcie_dllname,"%sWD-Dask.dll",DLLDIR);
pcie_libhandle=LoadLibraryEx(pcie_dllname, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
if(pcie_libhandle == NULL)goto load_error; 
info=1;
WD_Register_Card=(pRegister_Card)(void*)GetProcAddress(pcie_libhandle, "WD_Register_Card");
if(!WD_Register_Card)goto sym_error;
WD_AI_Config=(pAI_Config)(void*)GetProcAddress(pcie_libhandle, "WD_AI_Config");
if(!WD_AI_Config)goto sym_error;
WD_Release_Card=(pRelease_Card)(void*)GetProcAddress(pcie_libhandle, "WD_Release_Card");
if(!WD_Release_Card)goto sym_error;
WD_AI_ContBufferSetup=(pAI_ContBufferSetup)
                        (void*)GetProcAddress(pcie_libhandle, "WD_AI_ContBufferSetup");
if(!WD_AI_ContBufferSetup)goto sym_error;
WD_AI_EventCallBack=(pAI_EventCallBack)
                          (void*)GetProcAddress(pcie_libhandle, "WD_AI_EventCallBack");
if(!WD_AI_EventCallBack)goto sym_error;
WD_AI_ContReadChannel=(pAI_ContReadChannel)
                           (void*)GetProcAddress(pcie_libhandle, "WD_AI_ContReadChannel");
if(!WD_AI_ContReadChannel)goto sym_error;
WD_AI_ContReadMultiChannels=(pAI_ContReadMultiChannels)
                           (void*)GetProcAddress(pcie_libhandle, "WD_AI_ContReadMultiChannels");
if(!WD_AI_ContReadMultiChannels)goto sym_error;
WD_AI_AsyncClear=(pAI_AsyncClear)
                                 (void*)GetProcAddress(pcie_libhandle, "WD_AI_AsyncClear");
if(!WD_AI_AsyncClear)goto sym_error;
WD_AI_ContBufferReset=(pAI_ContBufferReset)
                               (void*)GetProcAddress(pcie_libhandle, "WD_AI_ContBufferReset");
if(!WD_AI_ContBufferReset)goto sym_error;
WD_AI_Set_Mode=(pAI_Set_Mode)(void*)GetProcAddress(pcie_libhandle, "WD_AI_Set_Mode");
if(!WD_AI_Set_Mode)goto sym_error;
WD_AI_AsyncDblBufferMode=(pAI_AsyncDblBufferMode)
                          (void*)GetProcAddress(pcie_libhandle, "WD_AI_AsyncDblBufferMode");
if(!WD_AI_AsyncDblBufferMode)goto sym_error;
WD_AI_AsyncDblBufferHandled=(pAI_AsyncDblBufferHandled)
                         (void*)GetProcAddress(pcie_libhandle, "WD_AI_AsyncDblBufferHandled");
if(!WD_AI_AsyncDblBufferHandled)goto sym_error;
WD_AI_InitialMemoryAllocated=(pAI_InitialMemoryAllocated)
                        (void*)GetProcAddress(pcie_libhandle, "WD_AI_InitialMemoryAllocated");
if(!WD_AI_InitialMemoryAllocated)goto sym_error;
WD_AI_AsyncDblBufferHalfReady=(pAI_AsyncDblBufferHalfReady)
                       (void*)GetProcAddress(pcie_libhandle, "WD_AI_AsyncDblBufferHalfReady");

CoInitialize(NULL);
return 0;
sym_error:;
FreeLibrary(pcie_libhandle);
load_error:;
library_error_screen(pcie_dllname,info);
return -1;
}

void unload_pcie9842_library(void)
{
FreeLibrary(pcie_libhandle);
CoUninitialize();
}
#endif

void osciCallback (int sig_no)
{
(void)sig_no;
// This is the callback function that will be invoked by the
// driver whenever a buffer is ready for transfer.
int i, j, k, m, n;
short int *iz;
int *iw;
WD_AI_AsyncDblBufferHandled(card);
switch(pcie9842.resampling_factor)
  {
  case 0:
  iz=(short int*)&timf1_char[timf1p_sdr];
  for(i=0; i<pcie9842_samples; i++)
    {
    iz[i]=digitizer_globals.buffer[halfbuf][i];
    }
  timf1p_sdr=(timf1p_sdr+sizeof(short int)*pcie9842_samples)&timf1_bytemask;
  break;

  case 1:
  m=pcie9842_samples/2;
  iz=(short int*)&timf1_char[timf1p_sdr];
  for(i=0; i<m; i++)
    {
    iz[i]=digitizer_globals.buffer[halfbuf][2*i  ]/2+
          digitizer_globals.buffer[halfbuf][2*i+1]/2;
    }
  timf1p_sdr=(timf1p_sdr+sizeof(short int)*m)&timf1_bytemask;
  break;

  case 2:
  m=pcie9842_samples/4;
  iz=(short int*)&timf1_char[timf1p_sdr];
  for(i=0; i<m; i++)
    {
    iz[i]=(digitizer_globals.buffer[halfbuf][4*i  ]>>2)+
          (digitizer_globals.buffer[halfbuf][4*i+1]>>2)+
          (digitizer_globals.buffer[halfbuf][4*i+2]>>2)+
          (digitizer_globals.buffer[halfbuf][4*i+3]>>2);
    }
  timf1p_sdr=(timf1p_sdr+sizeof(short int)*m)&timf1_bytemask;
  break;

  default:
  n=16-pcie9842.resampling_factor;
  m=pcie9842_samples/callback_blocksize;
  iw=(int*)&timf1_char[timf1p_sdr];
  for(i=0; i<m; i++)
    {
    k=digitizer_globals.buffer[halfbuf][i*callback_blocksize];
    for(j=1; j<callback_blocksize; j++)
      {
      k+=digitizer_globals.buffer[halfbuf][i*callback_blocksize+j];
      }
    iw[i]=k<<n;
    }
  timf1p_sdr=(timf1p_sdr+sizeof(int)*m)&timf1_bytemask;
  break;
  }
lir_set_event(EVENT_HWARE1_RXREADY);
halfbuf=1&(halfbuf+1);
}

void pcie9842_input(void)
{
int i;
int rxin_local_workload_reset;
double read_start_time, total_reads;
int timing_loop_counter,timing_loop_counter_max,initial_skip_flag;
short int err;
u_int32_t totmem;
lir_init_event(EVENT_HWARE1_RXREADY);
load_pcie9842_library();
if(lir_status != LIR_OK)goto await_exit;
halfbuf=0;
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_PCIE9842_INPUT);
#endif
i=read_sdrpar(pcie9842_parfil_name, MAX_PCIE9842_PARM, 
                                     pcie9842_parm_text, (int*)((void*)&pcie9842));
if(i != 0 || pcie9842.check != PCIE9842PAR_VERNR)
  {
  lirerr(1359);
  goto pcie9842_init_error_exit;
  }   
if( (ui.rx_ad_speed<<pcie9842.resampling_factor) != 
                           (int)((rint)((double)200000000L/pcie9842.rate_dividend*
                                 (100000000L+(double)pcie9842.freq_adjust))/100000000L))
  {
  lirerr(1359);
  goto pcie9842_init_error_exit;
  }
callback_blocksize=1<<pcie9842.resampling_factor;
card=WD_Register_Card(PCIe_9842, 0);
if (card <0 ) 
  {
  lirerr(1361);
  goto pcie9842_init_error_exit;
  }
err=WD_AI_AsyncDblBufferMode (card, 1);
if(err != NoError) 
  {
  lirerr(1366);
  goto pcie9842_error_exit;
  }  
err=WD_AI_InitialMemoryAllocated (card, &totmem);
if(err != NoError) 
  {
  lirerr(1367);
  goto pcie9842_error_exit;
  }
pcie9842_samples=snd[RXAD].block_bytes/2; 
pcie9842_samples<<=pcie9842.resampling_factor;
if(pcie9842.resampling_factor >=3)pcie9842_samples>>=1;
make_power_of_two(&pcie9842_samples);
while((u_int32_t)pcie9842_samples/512 > totmem)pcie9842_samples/=2;
digitizer_globals.data_unaligned =(short int *)malloc(2*
                      pcie9842_samples*sizeof(short int) + 256);
if(digitizer_globals.data_unaligned == NULL) 
  {
  lirerr(1360);
  goto pcie9842_error_exit1;
  }
digitizer_globals.data_aligned=(short int *) ((size_t) ((size_t) 
       digitizer_globals.data_unaligned + (size_t) 255) & (size_t)(-256));
// Save pointers to the two aligned buffers.
digitizer_globals.buffer[0] = digitizer_globals.data_aligned;
digitizer_globals.buffer[1] = &digitizer_globals.data_aligned[pcie9842_samples];
digitizer_globals.bufferId[0] = 0;
digitizer_globals.bufferId[1] = 0;
err=WD_AI_ContBufferSetup(card,
                          digitizer_globals.buffer[0],
                          pcie9842_samples,
                          &digitizer_globals.bufferId[0]);
if(err != NoError) 
  {
  lirerr(1363);
  goto pcie9842_error_exit;
  }  
err=WD_AI_ContBufferSetup (card,
                           digitizer_globals.buffer[1],
                           pcie9842_samples,
                           &digitizer_globals.bufferId[1]);
if(err != NoError) 
  {
  lirerr(1363);
  goto pcie9842_error_exit;
  }  
fft1_block_timing();
if(thread_command_flag[THREAD_SCREEN]!=THRFLAG_NOT_ACTIVE)
  {
  while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
    {
    if(thread_command_flag[THREAD_PCIE9842_INPUT] ==
                                       THRFLAG_KILL)goto pcie9842_error_exit;
    lir_sleep(10000);
    }
  }

err=WD_AI_EventCallBack (card, 1, DBEvent, osciCallback);
if(err != NoError) 
  {
  lirerr(1364);
  goto pcie9842_error_exit;
  }
for(i=0;i<timf1_bytes;i++)timf1_char[i]=0;
err=WD_AI_ContReadChannel(card,                          
                          0,
                          digitizer_globals.bufferId[0],
                          pcie9842_samples,
                          pcie9842.rate_dividend,
                          pcie9842.rate_dividend,
                          ASYNCH_OP);
if(err != NoError) 
  {
  lirerr(1364);
  goto pcie9842_error_exit;
  }
#include "timing_setup.c"
thread_status_flag[THREAD_PCIE9842_INPUT]=THRFLAG_ACTIVE;
timf1p_sdr=timf1p_pa;  
lir_status=LIR_OK;
while(!kill_all_flag && 
            thread_command_flag[THREAD_PCIE9842_INPUT] == THRFLAG_ACTIVE)
  {
  lir_await_event(EVENT_HWARE1_RXREADY);
  if(kill_all_flag)goto pcie9842_exit;
  while (!kill_all_flag && 
                  ((timf1p_sdr-timf1p_pa+timf1_bytes)&timf1_bytemask) >=
                                   snd[RXAD].block_bytes)
    {
#include "input_speed.c"
    finish_rx_read();
    }
  }
pcie9842_exit:;  
// Tell digitizer to generate no more triggers.
u_int32_t count=0, startPos=0;
WD_AI_AsyncClear(card, &startPos, &count);
lir_sleep(50000);
WD_AI_ContBufferReset(card);
WD_AI_EventCallBack (card, 0, DBEvent, NULL);
pcie9842_error_exit:;
free(digitizer_globals.data_unaligned);
pcie9842_error_exit1:;
WD_Release_Card(card);
pcie9842_init_error_exit:;
unload_pcie9842_library();
await_exit:;
sdr=-1;
thread_status_flag[THREAD_PCIE9842_INPUT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_PCIE9842_INPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
lir_close_event(EVENT_HWARE1_RXREADY);
}

int display_pcie9842_parm_info(int *line)
{
char s[80];
int errcod;
errcod=read_sdrpar(pcie9842_parfil_name, MAX_PCIE9842_PARM, 
                               pcie9842_parm_text, 
                               (int*)((void*)&pcie9842));
if(errcod == 0)
  {
  settextcolor(7);
  sprintf(s,"Rate dividend = %i, Clk adjust = %.0f ppb  sampling rate %d Resampling %d",  
         pcie9842.rate_dividend,
         10.*pcie9842.freq_adjust,
         (int)((rint)((double)200000000L/pcie9842.rate_dividend*
                     (100000000L+(double)pcie9842.freq_adjust))/100000000L),
                     pcie9842.resampling_factor);
  lir_text(24,line[0],s);
  SNDLOG"\n%s",s);
  }
return (errcod);
}



void init_pcie9842(void)
{
FILE *pcie9842_file;
int i, line;
char s[80];
int *sdr_pi;
short handle;
if(load_pcie9842_library() != 0)return;
if(kill_all_flag)return;
// One or two cards.
handle=WD_Register_Card(PCIe_9842, 0);
if(handle < 0 ) 
  {
  lir_text(5,5,"PCIe 9842 not found. (Is driver in place?)");
  lir_text(5,7,press_any_key);
  await_keyboard();
  unload_pcie9842_library();
  return;
  }
line=2;  
// Look for more cards and allow the user to choose
// one or two cards.
sprintf(s,"Set rate dividend (%d to %d) =>",
                                    MIN_PCIE9842_DIVIDEND,
                                    MAX_PCIE9842_DIVIDEND);
lir_text(3,line,s);
pcie9842.rate_dividend=lir_get_integer(4+strlen(s),line,7,
                                     MIN_PCIE9842_DIVIDEND,
                                     MAX_PCIE9842_DIVIDEND);
line+=2;
if(kill_all_flag)
  {
  WD_Release_Card(handle);
  unload_pcie9842_library();
  return;
  }
lir_text(3,line,"Enter sampling clock error in ppb =>");
pcie9842.freq_adjust=0.1*lir_get_float(40,line,9,-300000.,300000.);
line+=2;
lir_text(3,line,"Enter resampling factor in powers of two (0-4) =>");
pcie9842.resampling_factor=lir_get_integer(53,line,1,0,4);
// set up other parameters and verify that the card(s) 
// are OK. Store all we neet to know in the pcie9842 structure.
pcie9842_file=fopen(pcie9842_parfil_name,"w");
if(pcie9842_file == NULL)
  {
  unload_pcie9842_library();
  lirerr(382268);
  return;
  }
pcie9842.check=PCIE9842PAR_VERNR;
WD_Release_Card(handle);
unload_pcie9842_library();
sdr_pi=(int*)(&pcie9842);
for(i=0; i<MAX_PCIE9842_PARM; i++)
  {
  fprintf(pcie9842_file,"%s [%d]\n",pcie9842_parm_text[i],sdr_pi[i]);
  }
parfile_end(pcie9842_file);
ui.rx_addev_no=PCIE9842_DEVICE_CODE;
ui.rx_ad_speed=(int)((rint)((double)200000000L/pcie9842.rate_dividend*
                     (100000000L+(double)pcie9842.freq_adjust))/100000000L);
ui.rx_ad_speed>>=pcie9842.resampling_factor;
if(pcie9842.resampling_factor <=2)
  {
  ui.rx_input_mode=0;
  }
else
  {
  ui.rx_input_mode=DWORD_INPUT;
  }
ui.rx_rf_channels=1;
ui.rx_ad_channels=1;
ui.rx_admode=0;
}
