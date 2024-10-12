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
#include "padef.h"
#include "thrdef.h"
#include "sigdef.h"
#include "keyboard_def.h"
#include "fft1def.h"
#include "screendef.h"
#include "txdef.h"
#include "options.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

#define MAX_LATENCY 20

typedef void PaStream;
typedef int PaDeviceIndex;
typedef int PaHostApiIndex;
typedef int PaError;
typedef double PaTime;
typedef unsigned long PaSampleFormat;
typedef unsigned long PaStreamCallbackFlags;
typedef unsigned long PaStreamFlags;

#define paNoFlag         ((PaStreamFlags) 0)
#define paClipOff        ((PaStreamFlags) 0x00000001)
#define paDitherOff      ((PaStreamFlags) 0x00000002)
#define paInputOverflow   ((PaStreamCallbackFlags) 0x00000002)
#define paOutputUnderflow ((PaStreamCallbackFlags) 0x00000004)
#define paFloat32        ((PaSampleFormat) 0x00000001) /**< @see PaSampleFormat */
#define paInt32          ((PaSampleFormat) 0x00000002) /**< @see PaSampleFormat */
#define paInt24          ((PaSampleFormat) 0x00000004) /**< Packed 24 bit format. @see PaSampleFormat */
#define paInt16          ((PaSampleFormat) 0x00000008) /**< @see PaSampleFormat */
#define paInt8           ((PaSampleFormat) 0x00000010) /**< @see PaSampleFormat */
#define paUInt8          ((PaSampleFormat) 0x00000020) /**< @see PaSampleFormat */
#define paCustomFormat   ((PaSampleFormat) 0x00010000)/**< @see PaSampleFormat */
#define paNonInterleaved ((PaSampleFormat) 0x80000000)
#define paFormatIsSupported (0)


typedef enum PaErrorCode
  {
  paNoError = 0,
  paNotInitialized = -10000,
  paUnanticipatedHostError,
  paInvalidChannelCount,
  paInvalidSampleRate,
  paInvalidDevice,
  paInvalidFlag,
  paSampleFormatNotSupported,
  paBadIODeviceCombination,
  paInsufficientMemory,
  paBufferTooBig,
  paBufferTooSmall,
  paNullCallback,
  paBadStreamPtr,
  paTimedOut,
  paInternalError,
  paDeviceUnavailable,
  paIncompatibleHostApiSpecificStreamInfo,
  paStreamIsStopped,
  paStreamIsNotStopped,
  paInputOverflowed,
  paOutputUnderflowed,
  paHostApiNotFound,
  paInvalidHostApi,
  paCanNotReadFromACallbackStream,
  paCanNotWriteToACallbackStream,
  paCanNotReadFromAnOutputOnlyStream,
  paCanNotWriteToAnInputOnlyStream,
  paIncompatibleStreamHostApi,
  paBadBufferPtr
  }
PaErrorCode;

typedef enum PaHostApiTypeId
  {
  paInDevelopment=0,
  paDirectSound=1,
  paMME=2,
  paASIO=3,
  paSoundManager=4,
  paCoreAudio=5,
  paOSS=7,
  paALSA=8,
  paAL=9,
  paBeOS=10,
  paWDMKS=11,
  paJACK=12,
  paWASAPI=13,
  paAudioScienceHPI=14
  }
PaHostApiTypeId;

typedef enum PaStreamCallbackResult
  {
  paContinue=0,
  paComplete=1,
  paAbort=2
  }
PaStreamCallbackResult;

typedef struct PaStreamCallbackTimeInfo
  {
  PaTime inputBufferAdcTime;
  PaTime currentTime;
  PaTime outputBufferDacTime;
  }
PaStreamCallbackTimeInfo;

typedef struct PaHostApiInfo
  {
  int structVersion;
  PaHostApiTypeId type;
  const char *name;
  int deviceCount;
  PaDeviceIndex defaultInputDevice;
  PaDeviceIndex defaultOutputDevice;
  }
PaHostApiInfo;

typedef struct PaHostErrorInfo
  {
  PaHostApiTypeId hostApiType;
  long errorCode;
  const char *errorText;
  }
PaHostErrorInfo;

typedef struct PaDeviceInfo
  {
  int structVersion;
  const char *name;
  PaHostApiIndex hostApi;
  int maxInputChannels;
  int maxOutputChannels;
  PaTime defaultLowInputLatency;
  PaTime defaultLowOutputLatency;
  PaTime defaultHighInputLatency;
  PaTime defaultHighOutputLatency;
  double defaultSampleRate;
  }
PaDeviceInfo;

typedef struct PaStreamParameters
  {
  PaDeviceIndex device;
  int channelCount;
  PaSampleFormat sampleFormat;
  PaTime suggestedLatency;
  void *hostApiSpecificStreamInfo;
  } 
PaStreamParameters;

typedef struct PaStreamInfo
  {
  int structVersion;
  PaTime inputLatency;
  PaTime outputLatency;
  double sampleRate;
  }
PaStreamInfo;

typedef int PaStreamCallback(const void *input, void *output,
           unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo,
              PaStreamCallbackFlags statusFlags, void *userData );

typedef int (*p_Pa_GetVersion)(void);
typedef const char* (*p_Pa_GetVersionText)(void);
typedef const char* (*p_Pa_GetErrorText)(PaError errorCode);
typedef PaError (*p_Pa_Initialize)(void);
typedef PaError (*p_Pa_Terminate)(void);
typedef const PaHostApiInfo* (*p_Pa_GetHostApiInfo)(PaHostApiIndex hostApi);
typedef PaDeviceIndex (*p_Pa_GetDeviceCount)(void);
typedef const PaDeviceInfo* (*p_Pa_GetDeviceInfo)(PaDeviceIndex device);
typedef PaError (*p_Pa_IsFormatSupported)( const PaStreamParameters *inputParameters,
                              const PaStreamParameters *outputParameters,
                              double sampleRate );
typedef PaError (*p_Pa_OpenStream)(PaStream** stream, 
           const PaStreamParameters *inputParameters,
           const PaStreamParameters *outputParameters,
           double sampleRate, unsigned long framesPerBuffer,
           PaStreamFlags streamFlags, PaStreamCallback *streamCallback,
           void *userData );
typedef PaError (*p_Pa_CloseStream)(PaStream *stream);
typedef PaError (*p_Pa_IsStreamStopped)(PaStream *stream);
typedef PaError (*p_Pa_AbortStream)(PaStream *stream);
typedef PaError (*p_Pa_StartStream)(PaStream *stream);
typedef const PaStreamInfo* (*p_Pa_GetStreamInfo)(PaStream *stream);
typedef void (*p_PaAlsa_EnableRealtimeScheduling)(PaStream *s, int enable);
typedef PaError (*p_PaJack_SetClientName)(const char *name);


p_Pa_GetVersion Pa_GetVersion;
p_Pa_GetVersionText Pa_GetVersionText;
p_Pa_GetErrorText Pa_GetErrorText;
p_Pa_Initialize Pa_Initialize;
p_Pa_Terminate Pa_Terminate;
p_Pa_GetHostApiInfo Pa_GetHostApiInfo;
p_Pa_GetDeviceCount Pa_GetDeviceCount;
p_Pa_GetDeviceInfo Pa_GetDeviceInfo;
p_Pa_IsFormatSupported Pa_IsFormatSupported;
p_Pa_OpenStream Pa_OpenStream;
p_Pa_CloseStream Pa_CloseStream;
p_Pa_IsStreamStopped Pa_IsStreamStopped;
p_Pa_AbortStream Pa_AbortStream;
p_Pa_StartStream Pa_StartStream;
p_Pa_GetStreamInfo Pa_GetStreamInfo;
p_PaAlsa_EnableRealtimeScheduling PaAlsa_EnableRealtimeScheduling;
p_PaJack_SetClientName PaJack_SetClientName;

PaStream *rxda_stream;
PaStream *rxad_stream;
PaStream *txad_stream;
PaStream *txda_stream;
int daout_workload_reset;
int pa_version;


#if OSNUM == OSNUM_WINDOWS
HANDLE pa_libhandle=NULL;
// Host processor. Allows to skip internal PA processing completely.
// You must set paWinWasapiRedirectHostProcessor flag to PaWasapiStreamInfo::flags member
// in order to have host processor redirected to your callback.
// Use with caution! inputFrames and outputFrames depend solely on final device setup (buffer
// size is just recommendation) but are not changing during run-time once stream is started.
typedef void (*PaWasapiHostProcessorCallback) (void *inputBuffer,  long inputFrames,
                                               void *outputBuffer, long outputFrames,
                                               void *userData);

typedef unsigned long PaWinWaveFormatChannelMask;

typedef enum PaWasapiFlags
  {
  paWinWasapiExclusive                = (1 << 0),
  paWinWasapiRedirectHostProcessor    = (1 << 1),
  paWinWasapiUseChannelMask           = (1 << 2),
  paWinWasapiPolling                  = (1 << 3),
  paWinWasapiThreadPriority           = (1 << 4)
 }
PaWasapiFlags;

typedef enum PaWasapiThreadPriority
  {
  eThreadPriorityNone = 0,
  eThreadPriorityAudio,
  eThreadPriorityCapture,
  eThreadPriorityDistribution,
  eThreadPriorityGames,
  eThreadPriorityPlayback,
  eThreadPriorityProAudio,
  eThreadPriorityWindowManager
  }
PaWasapiThreadPriority;

typedef struct PaWasapiStreamInfo
  {
  unsigned long size;
  PaHostApiTypeId hostApiType;
  unsigned long version;
  unsigned long flags;
  PaWinWaveFormatChannelMask channelMask;
  PaWasapiHostProcessorCallback hostProcessorOutput;
  PaWasapiHostProcessorCallback hostProcessorInput;
  PaWasapiThreadPriority threadPriority;
  }
PaWasapiStreamInfo;

#define NO_OF_PADLL 5
char* portaudio_dll_files[NO_OF_PADLL]={"palir-04",
                                        "palir-03",
                                        "palir-02",
                                        "portaudio",
                                        "libportaudio"};

int select_portaudio_dll(void)
{
char pa_dllname[80];
char s[100];
int i, j, k, line;
int valid_files[NO_OF_PADLL];
clear_screen();
line=3;
lir_text(5,line,"Select what dll file to use for Portaudio");
line++;
lir_text(5,line,"Only high-lighted files are available on this system");
line+=2;
j=0;
k=-1;
for(i=0; i<NO_OF_PADLL; i++)
  {
  sprintf(pa_dllname,"%s%s",DLLDIR,portaudio_dll_files[i]);
  pa_libhandle=LoadLibraryEx(pa_dllname, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
  if(pa_libhandle == NULL)
    {
    settextcolor(22);
    valid_files[i]=FALSE;
    }
  else   
    {
    settextcolor(15);
    valid_files[i]=TRUE;
    k=i;
    j++;
    }
  sprintf(s,"%d  %s",i,pa_dllname); 
  lir_text(8,line,s);
  line++;
  }
line++;  
settextcolor(7);
if(j==0)
  {
  lir_text(5,line,"No library for Portaudio found on this system");
  line+=2;
  lir_text(7,line,press_any_key);
  await_processed_keyboard();
  return-1;
  }
if(j==1)
  {
  sprintf(s,"Only one alternative. %s autoselected",portaudio_dll_files[k]);  
  lir_text(5,line,s);
  line+=2;
  lir_text(7,line,press_any_key);
  return k;
  }
lir_text(5,line,"Select dll file by line number =>");
new:;
k=lir_get_integer(40,line,1,0,NO_OF_PADLL);
if(valid_files[k] == FALSE)goto new;
clear_screen();
return k;
}

int load_pa_library(void) 
{
char pa_dllname[80];
int i, info;
i=((ui.use_alsa&PORTAUDIO_DLL_VERSION)>>5)-1;
info=0;
sprintf(pa_dllname,"%s%s.dll",DLLDIR,portaudio_dll_files[i]);
pa_libhandle=LoadLibraryEx(pa_dllname, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
if(pa_libhandle == NULL)goto load_error; 
info=1;
Pa_GetErrorText=(p_Pa_GetErrorText)(void*)GetProcAddress(pa_libhandle,"Pa_GetErrorText");
if(!Pa_GetErrorText)goto sym_error;
Pa_Initialize=(p_Pa_Initialize)(void*)GetProcAddress(pa_libhandle,"Pa_Initialize");
if(!Pa_Initialize)goto sym_error;
Pa_Terminate=(p_Pa_Terminate)(void*)GetProcAddress(pa_libhandle,"Pa_Terminate");
if(!Pa_Terminate)goto sym_error;
Pa_GetHostApiInfo=(p_Pa_GetHostApiInfo)(void*)GetProcAddress(pa_libhandle,"Pa_GetHostApiInfo");
if(!Pa_GetHostApiInfo)goto sym_error;
Pa_GetDeviceCount=(p_Pa_GetDeviceCount)(void*)GetProcAddress(pa_libhandle,"Pa_GetDeviceCount");
if(!Pa_GetDeviceCount)goto sym_error;
Pa_GetDeviceInfo=(p_Pa_GetDeviceInfo)(void*)GetProcAddress(pa_libhandle,"Pa_GetDeviceInfo");
if(!Pa_GetDeviceInfo)goto sym_error;
Pa_IsFormatSupported=(p_Pa_IsFormatSupported)(void*)GetProcAddress(pa_libhandle,"Pa_IsFormatSupported");
if(!Pa_IsFormatSupported)goto sym_error;
Pa_OpenStream=(p_Pa_OpenStream)(void*)GetProcAddress(pa_libhandle,"Pa_OpenStream");
if(!Pa_OpenStream)goto sym_error;
Pa_CloseStream=(p_Pa_CloseStream)(void*)GetProcAddress(pa_libhandle,"Pa_CloseStream");
if(!Pa_CloseStream)goto sym_error;
Pa_IsStreamStopped=(p_Pa_IsStreamStopped)(void*)GetProcAddress(pa_libhandle,"Pa_IsStreamStopped");
if(!Pa_IsStreamStopped)goto sym_error;
Pa_AbortStream=(p_Pa_AbortStream)(void*)GetProcAddress(pa_libhandle,"Pa_AbortStream");
if(!Pa_AbortStream)goto sym_error;
Pa_StartStream=(p_Pa_StartStream)(void*)GetProcAddress(pa_libhandle,"Pa_StartStream");
if(!Pa_StartStream)goto sym_error;
Pa_GetStreamInfo=(p_Pa_GetStreamInfo)(void*)GetProcAddress(pa_libhandle,"Pa_GetStreamInfo");
if(!Pa_GetStreamInfo)goto sym_error;
pa_version=1;
Pa_GetVersionText=(p_Pa_GetVersionText)(void*)GetProcAddress(pa_libhandle,"Pa_GetVersionText");
if(!Pa_GetVersionText)pa_version=0;
Pa_GetVersion=(p_Pa_GetVersion)(void*)GetProcAddress(pa_libhandle, "Pa_GetVersion");
if(!Pa_GetVersion)pa_version=0;
return 0;
sym_error:;
FreeLibrary(pa_libhandle);
load_error:;
pa_libhandle=NULL;
if(i >= 0 && i<NO_OF_PADLL)
  {
  library_error_screen(pa_dllname,info);
  }
else
  {
  library_error_screen("for Portaudio",info);  
  }
return -1;
}

void unload_pa_library(void)
{
if(pa_libhandle != NULL)
  {
  FreeLibrary(pa_libhandle);
  pa_libhandle=NULL;
  }
}
#endif



#if(OSNUM == OSNUM_LINUX)
#include <dlfcn.h>
void *pa_libhandle;
const char linrad_name[8] ="linrad";
int pa_linux_realtime;
int pa_linux_jack;

int load_pa_library(void) 
{
int info;
info=0;
pa_libhandle=dlopen(PA_LIBNAME, RTLD_NOW);  
if(pa_libhandle == NULL)goto load_error; 
info=1;
Pa_GetErrorText=(p_Pa_GetErrorText)dlsym(pa_libhandle,"Pa_GetErrorText");
if(Pa_GetErrorText == NULL)goto sym_error;
Pa_Initialize=(p_Pa_Initialize)dlsym(pa_libhandle,"Pa_Initialize");
if(Pa_Initialize == NULL)goto sym_error;
Pa_Terminate=(p_Pa_Terminate)dlsym(pa_libhandle,"Pa_Terminate");
if(Pa_Terminate == NULL)goto sym_error;
Pa_GetHostApiInfo=(p_Pa_GetHostApiInfo)dlsym(pa_libhandle,"Pa_GetHostApiInfo");
if(Pa_GetHostApiInfo == NULL)goto sym_error;
Pa_GetDeviceCount=(p_Pa_GetDeviceCount)dlsym(pa_libhandle,"Pa_GetDeviceCount");
if(Pa_GetDeviceCount == NULL)goto sym_error;
Pa_GetDeviceInfo=(p_Pa_GetDeviceInfo)dlsym(pa_libhandle,"Pa_GetDeviceInfo");
if(Pa_GetDeviceInfo == NULL)goto sym_error;
Pa_IsFormatSupported=(p_Pa_IsFormatSupported)dlsym(pa_libhandle,"Pa_IsFormatSupported");
if(Pa_IsFormatSupported == NULL)goto sym_error;
Pa_OpenStream=(p_Pa_OpenStream)dlsym(pa_libhandle,"Pa_OpenStream");
if(Pa_OpenStream == NULL)goto sym_error;
Pa_CloseStream=(p_Pa_CloseStream)dlsym(pa_libhandle,"Pa_CloseStream");
if(Pa_CloseStream == NULL)goto sym_error;
Pa_IsStreamStopped=(p_Pa_IsStreamStopped)dlsym(pa_libhandle,"Pa_IsStreamStopped");
if(Pa_IsStreamStopped == NULL)goto sym_error;
Pa_AbortStream=(p_Pa_AbortStream)dlsym(pa_libhandle,"Pa_AbortStream");
if(Pa_AbortStream == NULL)goto sym_error;
Pa_StartStream=(p_Pa_StartStream)dlsym(pa_libhandle,"Pa_StartStream");
if(Pa_StartStream == NULL)goto sym_error;
Pa_GetStreamInfo=(p_Pa_GetStreamInfo)dlsym(pa_libhandle,"Pa_GetStreamInfo");
if(Pa_GetStreamInfo == NULL)goto sym_error;
PaAlsa_EnableRealtimeScheduling=(p_PaAlsa_EnableRealtimeScheduling)
                      dlsym(pa_libhandle, "PaAlsa_EnableRealtimeScheduling");
if(PaAlsa_EnableRealtimeScheduling != NULL)
  {
  pa_linux_realtime=1;
  }
else
  {
  pa_linux_realtime=0;
  }
PaJack_SetClientName=(p_PaJack_SetClientName)dlsym(pa_libhandle,
                                                    "PaJack_SetClientName");
if(PaJack_SetClientName != NULL)
  {
  pa_linux_jack=1;
  }
else
  {
  pa_linux_jack=0;
  }
pa_version=1;
Pa_GetVersionText=(p_Pa_GetVersionText)dlsym(pa_libhandle,"Pa_GetVersionText");
if(Pa_GetVersionText == NULL)pa_version=0;
Pa_GetVersion=(p_Pa_GetVersion)dlsym(pa_libhandle, "Pa_GetVersion");
if(Pa_GetVersion == NULL)pa_version=0;
return 0;
sym_error:;
dlclose(pa_libhandle);
load_error:;
pa_libhandle=NULL;
library_error_screen(PA_LIBNAME,info);
return -1;
}

void unload_pa_library(void)
{
if(pa_libhandle != NULL)
  {
  dlclose(pa_libhandle);
  pa_libhandle=NULL;
  }
}
#endif

#if(OSNUM == OSNUM_WINDOWS)
int pa_linux_realtime=0;
#endif



/// This routine will be called by the PortAudio engine when audio is needed.
/// It may be called at interrupt level on some machines so don't do anything
/// that could mess up the system like calling malloc() or free().

extern int rxadCallback( const void *inputBuffer, void *outputBuffer,
                         unsigned long framesToProcess,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData )
{
char s[80];
// Prevent unused variable warnings.
(void) outputBuffer;
(void) timeInfo;
(void) userData;
if(snd[RXAD].open_flag != CALLBACK_CMD_ACTIVE)
  {
  if(snd[RXAD].open_flag == CALLBACK_CMD_START)
    {
    snd[RXAD].open_flag=CALLBACK_ANSWER_AWAKE;
    }
  if(snd[RXAD].open_flag == CALLBACK_ANSWER_AWAKE)
    {
    return paContinue;
    }
  snd[RXAD].open_flag=CALLBACK_ANSWER_CLOSED;
  return paAbort;
  }
if(portaudio_active_flag == FALSE)
  {
  snd[RXAD].open_flag=CALLBACK_ERROR;
  return paAbort;
  }
if(snd[RXAD].block_frames != (int)framesToProcess)
  {
  snd[RXAD].open_flag=CALLBACK_ERROR;
  return paAbort;
  }
if( (statusFlags&paInputOverflow) != 0)
  {
  no_of_rx_overrun_errors++;
  sprintf(s,"RX%s%d",overrun_error_msg,no_of_rx_overrun_errors);
  wg_error(s,WGERR_RXIN);
  }
memcpy(&timf1_char[timf1p_sdr],inputBuffer,snd[RXAD].block_bytes);
timf1p_sdr=(timf1p_sdr+snd[RXAD].block_bytes)&timf1_bytemask;
lir_set_event(EVENT_PORTAUDIO_RXAD_READY);
return paContinue;
}

extern int txadCallback( const void *inputBuffer, void *outputBuffer,
                         unsigned long framesToProcess,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData )
{
char s[80];
// Prevent unused variable warnings.
(void) outputBuffer;
(void) timeInfo;
(void) userData;
short int *bufs;
int *bufi;
int i, n;
if(snd[TXAD].open_flag != CALLBACK_CMD_ACTIVE)
  {
  if(snd[TXAD].open_flag == CALLBACK_CMD_START)
    {
    snd[TXAD].open_flag=CALLBACK_ANSWER_AWAKE;
    }
  return paContinue;
  }  
if(portaudio_active_flag == FALSE)
  {
  snd[TXAD].open_flag=CALLBACK_ERROR;
  return paAbort;
  }
if(snd[TXAD].block_frames != (int)framesToProcess)
  {
  snd[TXAD].open_flag=CALLBACK_ERROR;
  return paAbort;
  }
if( (statusFlags&paInputOverflow) != 0)
  {
  no_of_rx_overrun_errors++;
  sprintf(s,"TX%s%d",overrun_error_msg,no_of_rx_overrun_errors);
  wg_error(s,WGERR_TXIN);
  }
n=mictimf_adread_block;
if(ui.tx_ad_bytes == 2)
  {  
  bufs=(short int*)inputBuffer;
  for(i=0; i<n; i++)
    {
    mictimf_shi[mictimf_pc]=bufs[i];
    mictimf_pc=(mictimf_pc+1)&mictimf_mask;
    }
  }
else  
  {  
  bufi=(int*)inputBuffer;
  for(i=0; i<n; i++)
    {
    mictimf_int[mictimf_pc]=bufi[i];
    mictimf_pc=(mictimf_pc+1)&mictimf_mask;
    }
  }
lir_set_event(EVENT_PORTAUDIO_TXAD_READY);
return paContinue;
}

extern int rxdaCallback( const void *inputBuffer, void *outputBuffer,
                         unsigned long framesToProcess,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData )
{
char s[80];
// Prevent unused variable warnings.
(void) inputBuffer;
(void) timeInfo;
(void) userData;
int samps;
if(snd[RXDA].open_flag != CALLBACK_CMD_ACTIVE)
  {
  if(snd[RXDA].open_flag == CALLBACK_CMD_START)
    {
    snd[RXDA].open_flag=CALLBACK_ANSWER_AWAKE;
    }
  memset(outputBuffer,0,snd[RXDA].block_bytes);    
  return paContinue;
  }  
if(portaudio_active_flag == FALSE)
  {
  snd[RXDA].open_flag=CALLBACK_ERROR;
  return paAbort;
  }
if(snd[RXDA].block_frames != (int)framesToProcess)
  {
  snd[RXDA].open_flag=CALLBACK_ERROR;
  return paAbort;
  }
if(thread_command_flag[THREAD_RX_OUTPUT]==THRFLAG_IDLE ||
           thread_status_flag[THREAD_RX_OUTPUT]==THRFLAG_IDLE)goto idle;
if(daout_workload_reset!=workload_reset_flag)
  {
  min_daout_samps=baseband_size;
  daout_workload_reset=workload_reset_flag;
  }
if(audio_dump_flag == 1)goto idle;
if( (statusFlags&paOutputUnderflow) != 0)
  {
  if(count_rx_underrun_flag)
    {
    no_of_rx_underrun_errors++;
    sprintf(s,"RX%s%d",underrun_error_msg,no_of_rx_underrun_errors);
    wg_error(s,WGERR_RXOUT);
    }
  }
samps=(daout_pa-daout_px+daout_size)&daout_bufmask;
if( samps > snd[RXDA].block_bytes)
  {
  memcpy(outputBuffer,&daout[daout_px],snd[RXDA].block_bytes);
  daout_px=(daout_px+snd[RXDA].block_bytes)&daout_bufmask;
  samps-=snd[RXDA].block_bytes;
  if(min_daout_samps > samps)min_daout_samps=samps;
  }
else
  {
  min_daout_samps=0;
idle:;  
  memset(outputBuffer, 0, snd[RXDA].block_bytes);
  }
return paContinue;
}

extern int txdaCallback( const void *inputBuffer, void *outputBuffer,
                         unsigned long framesToProcess,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData )
{
char s[80];
// Prevent unused variable warnings.
(void) inputBuffer;
(void) timeInfo;
(void) userData;
int *bufi;
short int *bufs;
int i;
bufi=(int*)outputBuffer;
bufs=(short int*)outputBuffer;
if(snd[TXDA].open_flag != CALLBACK_CMD_ACTIVE)
  {
  if(snd[TXDA].open_flag == CALLBACK_CMD_START)
    {
    snd[TXDA].open_flag=CALLBACK_ANSWER_AWAKE;
    }
  memset(outputBuffer, 0, snd[TXDA].block_bytes);
  if(snd[TXDA].open_flag == CALLBACK_ANSWER_AWAKE)
    {
    return paContinue;
    }
  snd[TXDA].open_flag=CALLBACK_ANSWER_CLOSED;
  return paAbort;
  }
if(portaudio_active_flag == FALSE)
  {
  snd[TXDA].open_flag=CALLBACK_ERROR;
  return paAbort;
  }
if(snd[TXDA].block_frames != (int)framesToProcess)
  {
  snd[TXDA].open_flag=CALLBACK_ERROR;
  return paAbort;
  }
if( (statusFlags&paOutputUnderflow) != 0)
  {
  if(count_rx_underrun_flag)
    {
    no_of_rx_underrun_errors++;
    sprintf(s,"TX%s%d",underrun_error_msg,no_of_rx_underrun_errors);
    wg_error(s,WGERR_TXOUT);
    }
  }
if( ((txout_pa-txout_px+txout_bufsiz)&txout_mask) > 2*snd[TXDA].block_frames)
  {
  switch(tx_output_mode)
    {
    case 0:
    for(i=0; i<snd[TXDA].block_frames; i++)
      {
      bufs[i]=TX_DA_MARGIN*0x7fff*txout[txout_px];
      txout_px=(txout_px+2)&txout_mask;  
      }
    break;
  
    case 1:
    for(i=0; i<snd[TXDA].block_frames; i++)
      {
      bufs[2*i  ]=TX_DA_MARGIN*0x7fff*txout[txout_px+1];
      bufs[2*i+1]=TX_DA_MARGIN*0x7fff*txout[txout_px  ];
      txout_px=(txout_px+2)&txout_mask;  
      }
    break;
 
    case 2:
    for(i=0; i<snd[TXDA].block_frames; i++)
      {
      bufi[i]=TX_DA_MARGIN*0x7fff0000*txout[txout_px];
/*
//---------------------------------------------------------
// This is to test a bug. The generated squarewave should be
// 32 times below the Nyquist frequency.
// The Delta44 runs bursts at a 10 times too high speed. 
// Useless for 32 bit data!!
if(i&32)
  {
  bufi[i]=TX_DA_MARGIN*0x7fff0000;
  }
else
  {
  bufi[i]=-TX_DA_MARGIN*0x7fff0000;
  }
//---------------------------------------------------------
*/
      txout_px=(txout_px+2)&txout_mask;  
      }
    break;
  
    case 3:
    for(i=0; i<snd[TXDA].block_frames; i++)
      {
      bufi[2*i  ]=TX_DA_MARGIN*0x7fff0000*txout[txout_px+1];
      bufi[2*i+1]=TX_DA_MARGIN*0x7fff0000*txout[txout_px  ];

/*
//---------------------------------------------------------
// This is to test a bug. The generated squarewave should be
// 32 times below the Nyquist frequency.
// The Delta44 runs bursts at a 10 times too high speed. 
// Useless for 32 bit data!!
if(i&32)
  {
  bufi[2*i]=TX_DA_MARGIN*0x7fff0000;
  bufi[2*i+1]=TX_DA_MARGIN*0x7fff0000;
  }
else
  {
  bufi[2*i]=-TX_DA_MARGIN*0x7fff0000;
  bufi[2*i+1]=0;
  }
//---------------------------------------------------------
*/

      txout_px=(txout_px+2)&txout_mask;  
      }
    break;
    }
  }
else
  {
  memset(outputBuffer, 0, snd[TXDA].block_bytes);
  }
return paContinue;
}

void portaudio_stop(void)
{
if(portaudio_active_flag == FALSE)
  {
  lirerr(711401);
  return;
  }
Pa_Terminate();
}

void open_portaudio_rxad(void)
{
PaStreamParameters  rxadParameters;
const PaStreamInfo *streamInfo;
PaError pa_err;
#if (OSNUM == OSNUM_WINDOWS)
const PaDeviceInfo *deviceInfo;
int i;
PaWasapiStreamInfo LinradWasapiStreaminfo;
CoInitialize(NULL);
#endif
snd[RXAD].open_flag=CALLBACK_CMD_START;
if(portaudio_active_flag == FALSE)
  {
  rxad_status=LIR_PA_FLAG_ERROR;
  lirerr(701401);
  return;
  }
// Set sample format
if( (ui.rx_input_mode&DWORD_INPUT) == 0)
  {
  rxadParameters.sampleFormat = paInt16; // 2 byte samples
  }
else
  {  
  rxadParameters.sampleFormat = paInt32; // 4 byte samples
  }
rxadParameters.device       = ui.rx_addev_no;
rxadParameters.channelCount = ui.rx_ad_channels;
rxadParameters.suggestedLatency = ui.rx_ad_latency/snd[RXAD].interrupt_rate;
// ******************************************************************************
#if (OSNUM == OSNUM_WINDOWS)
//TO GET BEST LATENCY WITH WASAPI: SELECT EXCLUSIVE MODE INSTEAD OF THE DEFAULT SHARED MODE 
//EXCLUSIVE MODE WORKS FOR VISTAX64, VISTAX86, WINDOWS7
//EXCLUSIVE MODE DOES NOT WORK FOR WOW64 (32-bit applications running on VISTAX64)
deviceInfo = Pa_GetDeviceInfo(ui.rx_addev_no);
if(deviceInfo == NULL)
  {
  rxad_status=LIR_PA_FORMAT_NOT_SUPPORTED;
  goto start_pa_end;
  }
i = strncmp(Pa_GetHostApiInfo( deviceInfo->hostApi )->name,"Windows WASAPI", 14);
if (i==0)
  {
  LinradWasapiStreaminfo.size=sizeof(PaWasapiStreamInfo);
  LinradWasapiStreaminfo.hostApiType=paWASAPI;
  LinradWasapiStreaminfo.version=1;
  LinradWasapiStreaminfo.flags=paWinWasapiExclusive;
  rxadParameters.hostApiSpecificStreamInfo = &LinradWasapiStreaminfo;
  }
else
#endif
  {
  rxadParameters.hostApiSpecificStreamInfo = NULL;
  }
// ********************************************************************************
lir_sched_yield();
pa_err = Pa_IsFormatSupported( &rxadParameters, NULL, ui.rx_ad_speed);
if( pa_err != paFormatIsSupported )
  {
  DEB"\npa_err=%s\n",Pa_GetErrorText(pa_err));
  rxad_status=LIR_PA_FORMAT_NOT_SUPPORTED;
  goto start_pa_end;
  }
timf1p_sdr=timf1p_pa;  
lir_init_event(EVENT_PORTAUDIO_RXAD_READY);
// Open and Start audio I/O stream.
pa_err = Pa_OpenStream( 
              &rxad_stream,
              &rxadParameters,
              NULL,
              (double)(ui.rx_ad_speed),           //SAMPLE_RATE,
              snd[RXAD].block_frames,             //frames per buffer
              paClipOff|paDitherOff,              //no clipping, no dither
              rxadCallback,
              NULL );                             //no userdata
if( pa_err != paNoError )
  {
  DEB"\npa_err=%s\n",Pa_GetErrorText(pa_err));
  lir_close_event(EVENT_PORTAUDIO_RXAD_READY);
  rxad_status=LIR_PA_OPENSTREAM_FAILED;
  goto start_pa_end;
  }
if(pa_linux_realtime == 1)
  {
  PaAlsa_EnableRealtimeScheduling ( rxad_stream, 1);
  }
pa_err = Pa_StartStream( rxad_stream );
if( pa_err != paNoError )
  {
  DEB"\npa_err=%s\n",Pa_GetErrorText(pa_err));
  rxad_status=LIR_PA_START_STREAM_FAILED;
  goto start_pa_end;
  }
streamInfo = Pa_GetStreamInfo(rxad_stream);
DEB"\nLATENCY %f\n",streamInfo->inputLatency);
DEB"\nRXout interrupt_rate: Actual %f",snd[RXAD].interrupt_rate);
while(!kill_all_flag && snd[RXAD].open_flag != CALLBACK_ANSWER_AWAKE)
  {
  lir_sleep(5000);
  }
start_pa_end:;
snd[RXAD].tot_bytes=timf1_bytes;
snd[RXAD].tot_frames=timf1_bytes/snd[RXAD].framesize;
snd[RXAD].no_of_blocks=timf1_bytes/snd[RXAD].block_bytes;
}

void open_portaudio_rxda(void)
{
PaStreamParameters  rxdaParameters;
const PaStreamInfo *streamInfo;
PaError pa_err;
#if (OSNUM == OSNUM_WINDOWS)
const PaDeviceInfo *deviceInfo;
int i;
PaWasapiStreamInfo LinradWasapiStreaminfo;
CoInitialize(NULL);
#endif
snd[RXDA].open_flag=CALLBACK_CMD_START;
if(portaudio_active_flag == FALSE)
  {
  rxda_status=LIR_PA_FLAG_ERROR;
  lirerr(721401);
  return;
  }
// Set sample format
if(rx_daout_bytes == 1)
  {
  rxdaParameters.sampleFormat = paUInt8; // 1 byte samples
  }
else
  {  
  rxdaParameters.sampleFormat = paInt16; // 2 byte samples
  }
rxdaParameters.device       = ui.rx_dadev_no;
rxdaParameters.channelCount = rx_daout_channels;
rxdaParameters.suggestedLatency = ui.rx_da_latency/snd[RXDA].interrupt_rate;
// ******************************************************************************
#if (OSNUM == OSNUM_WINDOWS)
//TO GET BEST LATENCY WITH WASAPI: SELECT EXCLUSIVE MODE INSTEAD OF THE DEFAULT SHARED MODE 
//EXCLUSIVE MODE WORKS FOR VISTAX64, VISTAX86, WINDOWS7
//EXCLUSIVE MODE DOES NOT WORK FOR WOW64 (32-bit applications running on VISTAX64)
deviceInfo = Pa_GetDeviceInfo(ui.rx_dadev_no);
if(deviceInfo == NULL)
  {
  rxad_status=LIR_PA_FORMAT_NOT_SUPPORTED;
  goto start_pa_end;
  }
i = strncmp(Pa_GetHostApiInfo( deviceInfo->hostApi )->name,"Windows WASAPI", 14);
if (i==0)
  {
  LinradWasapiStreaminfo.size=sizeof(PaWasapiStreamInfo);
  LinradWasapiStreaminfo.hostApiType=paWASAPI;
  LinradWasapiStreaminfo.version=1;
  LinradWasapiStreaminfo.flags=paWinWasapiExclusive;
  rxdaParameters.hostApiSpecificStreamInfo = &LinradWasapiStreaminfo;
  }
else
#endif
  {
  rxdaParameters.hostApiSpecificStreamInfo = NULL;
  }
// ********************************************************************************
pa_err = Pa_IsFormatSupported( NULL,&rxdaParameters, genparm[DA_OUTPUT_SPEED] );
if( pa_err != paFormatIsSupported )
  {
  rxda_status=LIR_PA_FORMAT_NOT_SUPPORTED;
  goto start_pa_end;
  }
// Open and Start audio I/O stream.
pa_err = Pa_OpenStream( 
              &rxda_stream,
              NULL,                               //no input
              &rxdaParameters,
              (double)(genparm[DA_OUTPUT_SPEED]), //SAMPLE_RATE,
              snd[RXDA].block_frames,             //frames per buffer
              paClipOff|paDitherOff,              //no clipping, no dither
              rxdaCallback,
              NULL );                             //no userdata
if( pa_err != paNoError )
  {
  rxda_status=LIR_PA_OPENSTREAM_FAILED;
  goto start_pa_end;
  }
if(pa_linux_realtime == 1)
  {
  PaAlsa_EnableRealtimeScheduling ( rxda_stream, 1);
  }
pa_err = Pa_StartStream( rxda_stream );
if( pa_err != paNoError )
  {
  rxda_status=LIR_PA_START_STREAM_FAILED;
  goto start_pa_end;
  }
streamInfo = Pa_GetStreamInfo(rxda_stream);
DEB"  Actual %.1f",snd[RXDA].interrupt_rate);
DEB"  Latency %.4f",streamInfo->outputLatency);
while(!kill_all_flag && snd[RXDA].open_flag != CALLBACK_ANSWER_AWAKE)
  {
  lir_sleep(5000);
  }
start_pa_end:;
// set an arbitrary positive value.
if(rxda_status == LIR_OK)rx_audio_out=1;
snd[RXDA].tot_bytes=daout_size;
snd[RXDA].tot_frames=daout_size/snd[RXDA].framesize;
snd[RXDA].no_of_blocks=daout_size/snd[RXDA].block_bytes;
daout_workload_reset=workload_reset_flag;
}

void close_portaudio_rxda(void)
{
PaError pa_err;
Pa_AbortStream( rxda_stream );
pa_err=Pa_IsStreamStopped( rxda_stream );
while(pa_err == 0)
  {
  pa_err = Pa_IsStreamStopped( rxda_stream );
  lir_sleep(1000);
  } 
pa_err = Pa_CloseStream( rxda_stream );
if( pa_err != paNoError )
  {
  lirerr(1168);
  }  
}

void close_portaudio_rxad(void)
{
PaError pa_err;
Pa_AbortStream( rxad_stream );
pa_err=Pa_IsStreamStopped( rxad_stream );
while(pa_err == 0)
  {
  pa_err = Pa_IsStreamStopped( rxad_stream );
  lir_sleep(1000);
  } 
pa_err = Pa_CloseStream( rxad_stream );
if( pa_err != paNoError )
  {
  lirerr(1229);
  }  
}

int pa_get_device_info (int  n,
                        int sound_type,
                        void *pa_device_name,
                        void *pa_device_hostapi, 
			double *pa_device_max_speed,
			double *pa_device_min_speed,
			int *pa_device_max_bytes,
			int *pa_device_min_bytes,
			int *pa_device_max_channels,
			int *pa_device_min_channels )

{
(void) n ;
(void) sound_type;
(void) pa_device_name;
(void) pa_device_hostapi; 
(void) pa_device_max_speed;
(void) pa_device_min_speed;
(void) pa_device_max_bytes;
(void) pa_device_min_bytes;
(void) pa_device_max_channels;
(void) pa_device_min_channels;
const PaDeviceInfo *deviceInfo;
PaError pa_err;
PaStreamParameters inputParameters;
PaStreamParameters outputParameters;
int i,j, speed_warning;
int minBytes, maxBytes;
int maxInputChannels, maxOutputChannels;
int minInputChannels, minOutputChannels;
double maxStandardSampleRate;
double minStandardSampleRate;
// negative terminated  list
static double standardSampleRates[] = {8000.0, 9600.0, 
        11025.0, 12000.0, 16000.0, 22050.0, 24000.0, 32000.0,
        44100.0, 48000.0, 88200.0, 96000.0, 192000.0, -1};
#define MAX_STANDARD_RATES 13
// *******************************************************
sprintf((char*)(pa_device_name),"%s"," ");
sprintf((char*)(pa_device_hostapi),"%s"," ");
*pa_device_max_speed=0;
*pa_device_min_speed=0;
*pa_device_max_bytes=0;
*pa_device_min_bytes=0;
*pa_device_max_channels=0;
*pa_device_min_channels=0;
minInputChannels=0;
minOutputChannels=0;



#if OSNUM == OSNUM_LINUX
#if DUMPFILE == TRUE
if(sound_type == RXAD || sound_type == TXAD)
  {
/*  
  if(n == 8 ||
     n == 9 ||
     n == 10 ||
     n == 11 ||
     n == 12 ||
     n == 13 ||
     n == 14 ||
     n == 15 ||
     n == 16 ||
     n == 17 ||
     n == 18)
    {
    fprintf( stderr,"\nWARNING Portaudio input device %d disabled in pa.c",n);
    fprintf( dmp,"\nWARNING Portaudio input device %d disabled in pa.c",n);
    return -1;
    }
*/
  }
else
  {
  if(n == 7  ||
     n == 8 ||
     n == 9 ||
     n == 10 ||
     n == 11 ||
     n == 12 )
    {
    fprintf( stderr,"\nWARNING Portaudio output device %d disabled in pa.c",n);
    fprintf( dmp,"\nWARNING Portaudio output device %d disabled in pa.c",n);
    return -1;
    }
  }
#endif
#endif


if(n >= Pa_GetDeviceCount() ) return -1;
deviceInfo = Pa_GetDeviceInfo(n);
if (deviceInfo->maxOutputChannels==0 && 
         (sound_type==RXDA || sound_type==TXDA))return -1;
if (deviceInfo->maxInputChannels==0 && 
         (sound_type==RXAD || sound_type==TXAD))return -1; 
sprintf((char*)(pa_device_name),"%s",deviceInfo->name);
sprintf((char*)(pa_device_hostapi),"%s",Pa_GetHostApiInfo( deviceInfo->hostApi )->name);
speed_warning=0;
#if (OSNUM == OSNUM_WINDOWS)
// bypass bug in Juli@ ASIO driver: 
// this driver hangs after a Pa_IsFormatSupported call 
i = strncmp(deviceInfo->name, "ASIO 2.0 - ESI Juli@", 19);
if (i == 0)
    {
    minStandardSampleRate=44100;
    maxStandardSampleRate=192000;
    minBytes=1;
    maxBytes=4;
    maxInputChannels= deviceInfo->maxInputChannels;
    minInputChannels= 1;
    maxOutputChannels= deviceInfo->maxOutputChannels;
    minOutputChannels= 1;
    goto end_pa_get_device_info;
    }
#endif
// Investigate device capabilities.
// Check min and max samplerates  with 16 bit data.
maxStandardSampleRate=0;
minStandardSampleRate=0;
inputParameters.device = n;
inputParameters.channelCount = deviceInfo->maxInputChannels;
inputParameters.sampleFormat = paInt16;
inputParameters.suggestedLatency = 0;
inputParameters.hostApiSpecificStreamInfo = NULL;
outputParameters.device = n;
outputParameters.channelCount = deviceInfo->maxOutputChannels;
outputParameters.sampleFormat = paInt16; /*paInt24;*/
outputParameters.suggestedLatency = 0;
outputParameters.hostApiSpecificStreamInfo = NULL;
SNDLOG"\n\nTESTING PORTAUDIO DEVICE %i FOR %s\n",n, sndtype[sound_type]);
SNDLOG"\nid=%d device=%s hostapi=%s max_input_chan=%d max_output_chan=%d",
          n, 
          deviceInfo->name,
          Pa_GetHostApiInfo( deviceInfo->hostApi )->name,
          deviceInfo->maxInputChannels,
          deviceInfo->maxOutputChannels);
fflush(sndlog); 
// ************************************************************************
//filter for portaudio Windows hostapi's with non experts.
#if (OSNUM == OSNUM_WINDOWS)
//only allow ASIO or WASAPI or WDM-KS
i = strncmp(Pa_GetHostApiInfo(deviceInfo->hostApi)->name, "ASIO", 4);
if (i==0 ) goto end_filter_hostapi;
i = strncmp(Pa_GetHostApiInfo(deviceInfo->hostApi)->name, "Windows WASAPI", 14);
if (i==0 ) goto end_filter_hostapi;
i = strncmp(Pa_GetHostApiInfo(deviceInfo->hostApi)->name, "Windows WDM-KS", 14);
if (i==0 ) goto end_filter_hostapi;
if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
  {
  speed_warning=1;
  }
else  
  { 
  return -1;
  }
end_filter_hostapi:;
#endif
// ************************************************************************
i=0;
while(standardSampleRates[i] > 0 && minStandardSampleRate==0)
   {
    SNDLOG"\nTesting %d %f",i,standardSampleRates[i]);
    if(sound_type==RXDA || sound_type==TXDA)
      {
      pa_err=Pa_IsFormatSupported(
                          NULL, &outputParameters, standardSampleRates[i] );
      }
    else
      {  
      pa_err=Pa_IsFormatSupported(
                  &inputParameters, NULL, standardSampleRates[i] );
      }
    SNDLOG"  err=%d  %s",pa_err, Pa_GetErrorText(pa_err));
    fflush(sndlog);  
    if(pa_err == paDeviceUnavailable)return -1;
    if(pa_err == paInvalidDevice)    return -1;
    if(pa_err == paFormatIsSupported )
      { 
      minStandardSampleRate=standardSampleRates[i];
      }
    i++;
   }
if(minStandardSampleRate != 0)
  {
  j=i;
  i=MAX_STANDARD_RATES-1;
  while(i >= j && maxStandardSampleRate==0)
    {
    SNDLOG"\nTesting %d %f",i,standardSampleRates[i]);
    if(sound_type==RXDA || sound_type==TXDA)
      {
      pa_err=Pa_IsFormatSupported(
                          NULL, &outputParameters, standardSampleRates[i] );
      }
    else
      {  
      pa_err=Pa_IsFormatSupported(
                &inputParameters, NULL, standardSampleRates[i] );
      }
    SNDLOG"  err=%d  %s",pa_err, Pa_GetErrorText(pa_err));
    fflush(sndlog);   
    if(pa_err == paDeviceUnavailable)return -1;
    if(pa_err == paInvalidDevice)    return -1;
    if( pa_err == paFormatIsSupported )
      { 
      maxStandardSampleRate=standardSampleRates[i];
      }
    i--;
    }
// check if min SampleRate  = max SampleRate 
  if(maxStandardSampleRate==0)
    {
    maxStandardSampleRate=minStandardSampleRate;
    }
  }
if(minStandardSampleRate==0)
  {
  minStandardSampleRate=deviceInfo->defaultSampleRate;
  maxStandardSampleRate=deviceInfo->defaultSampleRate;
  }
// check min and max bytes
minBytes=2;
maxBytes=2;
if(sound_type==RXDA || sound_type==TXDA)
    {
    outputParameters.sampleFormat = paUInt8;
    pa_err=Pa_IsFormatSupported(
                          NULL, &outputParameters, maxStandardSampleRate );
    }
else
    {  
    inputParameters.sampleFormat = paUInt8;
    pa_err=Pa_IsFormatSupported(
                  &inputParameters, NULL, maxStandardSampleRate );
    }
if( pa_err == paFormatIsSupported )
    { 
    minBytes=1;
    }
SNDLOG"\nminBytes=%i", minBytes); 
fflush(sndlog);
if(sound_type==RXDA || sound_type==TXDA)
    {
    outputParameters.sampleFormat = paInt32;
    pa_err=Pa_IsFormatSupported(
                          NULL, &outputParameters, maxStandardSampleRate );
    }
else
    {  
    inputParameters.sampleFormat = paInt32;
    pa_err=Pa_IsFormatSupported(
                  &inputParameters, NULL, maxStandardSampleRate );
    }
if( pa_err == paFormatIsSupported )
    { 
    maxBytes=4;
    }
SNDLOG"\nmaxBytes=%i", maxBytes); 
fflush(sndlog);
// check min channel count
maxInputChannels= deviceInfo->maxInputChannels;
maxOutputChannels= deviceInfo->maxOutputChannels;
if(sound_type==RXDA || sound_type==TXDA)
    {
      outputParameters.channelCount = 1;
      outputParameters.sampleFormat = paInt16;
      pa_err=paFormatIsSupported+32000;
      while(pa_err != paFormatIsSupported &&
           ( outputParameters.channelCount < (maxOutputChannels+1)) )
        {
        pa_err=Pa_IsFormatSupported(
                          NULL, &outputParameters, maxStandardSampleRate );
        outputParameters.channelCount++;
        }
      if( pa_err == paFormatIsSupported )
        {
        minOutputChannels=outputParameters.channelCount-1;
        SNDLOG"\nminOutputChannels=%i", minOutputChannels); 
        SNDLOG"\nmaxOutputChannels=%i\n\n", maxOutputChannels); 
        fflush(sndlog); 
        }
      else
        {
        SNDLOG"\nminOutputChannels=%i samplerate=%6.0f err=%d  %s"
           , (outputParameters.channelCount-1), maxStandardSampleRate, pa_err, Pa_GetErrorText(pa_err));
        fflush(sndlog);
        return -1;
        }
    }
else
    { 
      inputParameters.channelCount = 1;
      inputParameters.sampleFormat = paInt16;
      pa_err=paFormatIsSupported+32000;
      while(pa_err != paFormatIsSupported &&
           ( inputParameters.channelCount < (maxInputChannels+1)) )
        {
        pa_err=Pa_IsFormatSupported(
                  &inputParameters, NULL, maxStandardSampleRate );
        inputParameters.channelCount++;
        }
      if( pa_err == paFormatIsSupported )
        {
        minInputChannels=inputParameters.channelCount-1;
        SNDLOG"\nminInputChannels=%i", minInputChannels); 
        SNDLOG"\nmaxInputChannels=%i\n\n", maxInputChannels); 
        fflush(sndlog);
        }
      else
        {
        SNDLOG"\nminInputChannels=%i samplerate=%6.0f err=%d  %s"
            ,( inputParameters.channelCount-1), maxStandardSampleRate, pa_err, Pa_GetErrorText(pa_err));
        fflush(sndlog);
        return -1;
        }
     }
#if (OSNUM == OSNUM_WINDOWS)
end_pa_get_device_info:;
#endif
*pa_device_max_speed=maxStandardSampleRate;
*pa_device_min_speed=minStandardSampleRate;
*pa_device_max_bytes=maxBytes;
*pa_device_min_bytes=minBytes;
if(sound_type==RXDA || sound_type==TXDA)
    {
    *pa_device_max_channels= maxOutputChannels;
    *pa_device_min_channels= minOutputChannels;
    }
else
    {
    *pa_device_max_channels= maxInputChannels;
    *pa_device_min_channels= minInputChannels;
    }
// Change the limits for RXDA if they are too generous or seem incorrect.
if(sound_type==RXDA )
    {
    if(*pa_device_max_bytes > 2)*pa_device_max_bytes=2;
    if(*pa_device_min_channels >=  2)*pa_device_min_channels=2;
           else *pa_device_min_channels=1;
    if(*pa_device_max_channels <  2 )*pa_device_max_channels=1;
           else *pa_device_max_channels=2;
    }
return speed_warning;
}

void set_portaudio(int sound_type, int *line)
{
(void) sound_type;
(void) line;
char s[256];
int i, j, k;
char devflag[MAX_PA_DEVICES];
double devmaxStandardSampleRate[MAX_PA_DEVICES];
double devminStandardSampleRate[MAX_PA_DEVICES];
int devmaxBytes[MAX_PA_DEVICES];
int devminBytes[MAX_PA_DEVICES];
int devmaxInputChannels[MAX_PA_DEVICES];
int devminInputChannels[MAX_PA_DEVICES];
int devmaxOutputChannels[MAX_PA_DEVICES];
int devminOutputChannels[MAX_PA_DEVICES];
int id, err, numDevices, valid_dev_cnt;
char pa_device_name[128];     
char pa_device_hostapi[128]; 
double pa_device_max_speed;
double pa_device_min_speed;
int pa_device_max_bytes;
int pa_device_min_bytes;
int pa_device_max_channels;
int pa_device_min_channels;
SNDLOG"\nTESTING ALL PORTAUDIO DEVICES FOR %s\n",sndtype[sound_type]);
if(pa_version != 0)
  {
  SNDLOG"\nPortAudio ver %d \n%s\n", Pa_GetVersion(),Pa_GetVersionText());
  }
else
  {
  SNDLOG"\nOld PortAudio no support for version info\n");
  }
valid_dev_cnt=0;
clear_screen();
line[0]=4;
numDevices=Pa_GetDeviceCount();
if(numDevices < 0)
  {
  lirerr(1166);
  return;
  }
if(numDevices == 0)
  {
  lir_text(5,2,"No portaudio device detected for testing.");
  SNDLOG"\nNo portaudio device detected for testing.");
  lir_text(5,4,press_any_key);
  await_keyboard();
  goto end_portaudiotest;
  }
else
  {
  SNDLOG"\nNumber of portaudio devices to be tested=%i\n",numDevices);
  }
if(numDevices > MAX_PA_DEVICES)
  {
  sprintf(s,"Too many portaudio devices %i Max is %d",
                                                    numDevices,MAX_PA_DEVICES);
  lir_text(5,2,s);
  SNDLOG"\n%s",s);
  lir_text(5,3,"The list will be truncated.");
  lir_text(5,4,"Disconnect/uninstall devices that you do not need.");
  lir_text(5,6,press_any_key);
  await_keyboard();
  numDevices=MAX_PA_DEVICES;  
  }
fflush(sndlog);
settextcolor(14);
sprintf(s,"LIST OF VALID PORTAUDIO DEVICES FOR %s :",sndtype[sound_type]);
lir_text(20,0,s);
settextcolor(13);
lir_text(28,1,"This may take a very long time.");
settextcolor(7);
// headings
lir_text(75,line[0],"SAMPLE RATE");
lir_text(90,line[0],"CHANNELS");
lir_text(102,line[0],"BYTES");
line[0]++;
lir_text(2,line[0],"ID         NAME");
lir_text(54,line[0],"HOST-API");
lir_text(77,line[0],"MIN/MAX");
lir_text(91,line[0],"MIN/MAX");
lir_text(101,line[0],"MIN/MAX");
line[0]++;
lir_text(2,line[0],"--  ------------------------");
lir_text(54,line[0],"-------");
lir_text(74,line[0],"-------------");
lir_text(91,line[0],"-------");
lir_text(101,line[0],"-------");
line[0]++;
lir_refresh_screen();
// look for valid devices
err=0;
for(id=0; id < numDevices; id++)
  {
  devflag[id]=0;
  sprintf(s,"--- Testing device %d ---",id);
  lir_text(5,line[0],s);
  err=pa_get_device_info (id,
                          sound_type,
                          &pa_device_name,
                          &pa_device_hostapi,
			  &pa_device_max_speed,
			  &pa_device_min_speed,
			  &pa_device_max_bytes,
			  &pa_device_min_bytes,
			  &pa_device_max_channels,
			  &pa_device_min_channels);
  if (err >= 0 )
    {
    clear_lines(line[0],line[0]);
    if(strcmp( pa_device_hostapi,"ASIO")!=0 ||asio_flag==FALSE)
      {
      devflag[id]=1+err;
      valid_dev_cnt++;
//display result on screen
      sprintf(s,"%2d/%2d",pa_device_min_channels,pa_device_max_channels);
      lir_text(91,line[0],s);
      sprintf(s,"%03d  %s", id, pa_device_name);
      lir_text(1,line[0],s);
      sprintf(s," %s", pa_device_hostapi );
      lir_text(53,line[0],s);
      sprintf(s,"%6.0f/%6.0f",pa_device_min_speed, pa_device_max_speed);
      lir_text(73,line[0],s);
      sprintf(s,"%i/%i",pa_device_min_bytes, pa_device_max_bytes);
      lir_text(103,line[0],s);
      line[0]++;
      if(line[0] >= screen_last_line)screenerr(line);
      lir_refresh_screen();
//save values
      devmaxStandardSampleRate[id]=pa_device_max_speed;
      devminStandardSampleRate[id]=pa_device_min_speed;
      devmaxBytes[id]=pa_device_max_bytes;
      devminBytes[id]=pa_device_min_bytes;
      if(sound_type==RXDA || sound_type==TXDA)
        {
        devmaxOutputChannels[id]=pa_device_max_channels;
        devminOutputChannels[id]=pa_device_min_channels;
        }
      else
        {
        devmaxInputChannels[id]=pa_device_max_channels;
        devminInputChannels[id]=pa_device_min_channels;
        }
      }   
    }
  }
if(err < 0)clear_lines(line[0],line[0]);

SNDLOG"\n\nEND TESTING PORTAUDIO DEVICES\n");
fflush(sndlog);
if (valid_dev_cnt !=0)
  {
  line[0]+=3;
  if(line[0] >= screen_last_line)screenerr(line);
  sprintf(s,"Select portaudio device ID for %s from list >",sndtype[sound_type]);
  SNDLOG"\n%s",s);
  gtdev:;
  settextcolor(14);
  clear_lines(line[0],line[0]+1);
  lir_text(2,line[0],s);
  settextcolor(7);
  id=lir_get_integer(strlen(s)+4,line[0], 3, 0, numDevices-1);
  SNDLOG"\nUser selected %i\n",id);
  if(kill_all_flag)return;
  if(devflag[id] == 0)goto gtdev;
  if(devflag[id] != 1)
    {
    lir_text(2,line[0]+1,"This is a very slow device. Really use it? (Y/N)");
really:;
    to_upper_await_keyboard();
    if(lir_inkey == 'N')goto gtdev;
    if(lir_inkey != 'Y')goto really;
    line[0]++;
    if(line[0] >= screen_last_line)screenerr(line);
    }
  line[0]++;
  if(line[0] >= screen_last_line-6)
    {
    line[0]=5;
    clear_screen();
    }
  switch (sound_type)
    {
    case RXDA:
    ui.rx_dadev_no=id;
    ui.rx_max_da_speed=devmaxStandardSampleRate[id];
    ui.rx_min_da_speed=devminStandardSampleRate[id];
    ui.rx_min_da_bytes=devminBytes[id];
    ui.rx_max_da_bytes=devmaxBytes[id];
    ui.rx_min_da_channels= devminOutputChannels[id];
    ui.rx_max_da_channels=devmaxOutputChannels[id];
    if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)       
      {
      line[0]+=2;
      if(line[0] >= screen_last_line)screenerr(line);
      sprintf(s,"Latency Factor (1 to %d)",MAX_LATENCY);
      lir_text(0,line[0],s);
      ui.rx_da_latency=lir_get_integer(26,line[0], 2, 1, MAX_LATENCY);
      }
    else
      {
      ui.rx_da_latency=2;
      }  
    break;

    case RXAD:
    ui.rx_addev_no=id;
    line[0]+=2;
    if(line[0] >= screen_last_line)screenerr(line);
    lir_text(10,line[0],"Sampling speed (Hz)>");
    ui.rx_ad_speed=lir_get_integer(31,line[0], 6, 
                                         devminStandardSampleRate[id], 
                                                devmaxStandardSampleRate[id]);
    if(kill_all_flag) return;
    if(devmaxBytes[id] < 2)
      {
      lirerr(1217);
      return;
      }  
    if(devminBytes[id] < 2)devminBytes[id]=2;
    if(devmaxBytes[id] == 3)
      {
      lirerr(366132);
      return;
      }
    ui.rx_input_mode=0;
    if(devmaxBytes[id] >= 4)
      {  
get_rxbits:;
      lir_text(40,line[0],"No of bits (16/32):");
      i=lir_get_integer(60,line[0], 2, 16, 32);
      if(kill_all_flag) return;
      clear_screen();
      if(i==32)
        {
        ui.rx_input_mode=DWORD_INPUT;
        }
      else
        {
        if(i != 16)goto get_rxbits;
        }
      }
// Ask for the desired processing mode.
sel_radio:;
    clear_screen();
    lir_text(10,0,"Select radio interface>");
    j=1;
    if(devmaxInputChannels[id]>=2)j=3;
    if(devmaxInputChannels[id]>=4)j=4;
    if(devminInputChannels[id]==1)
      {
      k=0;
      }
    else
      {
      k=1;
      }  
    for(i=k; i<j; i++)
      {
      sprintf(s,"%d: %s",i+1,audiomode_text[i]);
      lir_text(10,2+i,s);
      }
    lir_text(10,3+i,"F1 for help/info");
    line[0]=5+i;
chsel:;
    await_processed_keyboard();
    if(lir_inkey == F1_KEY || lir_inkey == '!')
      {
      if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
        {
        help_message(89);
        }
      else
        {  
        help_message(109);
        }
      goto sel_radio;
      }
    if(lir_inkey-'0'>j)goto chsel;
    if(lir_inkey-'0'<k)goto chsel;
    switch (lir_inkey)
      {
      case '1':
      ui.rx_rf_channels=1;
      ui.rx_ad_channels=1;
      break;

      case '2':
      ui.rx_input_mode|=IQ_DATA;
      ui.rx_rf_channels=1;
      ui.rx_ad_channels=2;
      break;

      case '3':
      ui.rx_input_mode|=TWO_CHANNELS;
      ui.rx_rf_channels=2;
      ui.rx_ad_channels=2;
      break;

      case '4':
      ui.rx_input_mode|=TWO_CHANNELS+IQ_DATA;
      ui.rx_rf_channels=2;
      ui.rx_ad_channels=4;
      break;

      default:
      goto chsel;
      }
// Input seems OK.
    if(ui.rx_rf_channels == 1 && ui.rx_ad_channels == 2)
      {
      line[0]+=2;
      lir_text(0,line[0],
                "Number of points to time shift between I and Q? (-4 to +4)");
      ui.sample_shift=lir_get_integer(60,line[0], 2, -4, 4);
      }
    if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)       
      {
      line[0]+=2;
      sprintf(s,"Latency Factor (1 to %d)",MAX_LATENCY);
      lir_text(0,line[0],s);
      ui.rx_ad_latency=lir_get_integer(26,line[0], 2, 1, MAX_LATENCY);
      }
    else
      {
      ui.rx_ad_latency=2;
      }
    snd[RXAD].framesize=2*ui.rx_ad_channels;  
    if( (ui.rx_input_mode&DWORD_INPUT) != 0)snd[RXAD].framesize*=2;
    snd[RXAD].block_frames=0.04*ui.rx_ad_speed;
    snd[RXAD].block_bytes=snd[RXAD].block_frames*snd[RXAD].framesize;
    break;

    case TXAD:
    ui.tx_addev_no=id;
    lir_text(10,line[0],"Sampling speed (Hz)>");
    ui.tx_ad_speed=lir_get_integer(31,line[0], 6, 
                                         devminStandardSampleRate[id], 
                                                devmaxStandardSampleRate[id]);
    if(kill_all_flag) return;
    if(devmaxBytes[id] < 2)
      {
      lirerr(1217);
      return;
      }  
    if(devminBytes[id] < 2)devminBytes[id]=2;
    if(devmaxBytes[id] == 3)
      {
      lirerr(366132);
      return;
      }
    ui.tx_ad_bytes=2;
    if(devmaxBytes[id] >= 4)
      {
get_txad_bits:;
      lir_text(40,line[0],"No of bits (16/32):");
      i=lir_get_integer(60,line[0], 2, 16, 32);
      if(kill_all_flag) return;
      if(i==32)
        {
        ui.tx_ad_bytes=4;
        }
      else
        {
        if(i != 16)goto get_txad_bits;
        }
      }  
    if(devminInputChannels[id]==1)
      {
      ui.tx_ad_channels=1;
      }
    else
      {
      ui.tx_ad_channels=2;
      }
    line[0]+=2;
    sprintf(s,"Latency Factor (1 to %d)",MAX_LATENCY);
    lir_text(0,line[0],s);
    ui.tx_ad_latency=lir_get_integer(26,line[0], 2, 1, MAX_LATENCY);
    break;
    
    case TXDA:
    ui.tx_dadev_no=id;
    lir_text(10,line[0],"Sampling speed (Hz)>");
    ui.tx_da_speed=lir_get_integer(31,line[0], 6, 
                                         devminStandardSampleRate[id], 
                                                devmaxStandardSampleRate[id]);
    if(kill_all_flag) return;
    if(devmaxBytes[id] < 2)
      {
      lirerr(1217);
      return;
      }  
    if(devminBytes[id] < 2)devminBytes[id]=2;
    if(devmaxBytes[id] == 3)
      {
      lirerr(366132);
      return;
      }
    ui.tx_da_bytes=2;
    if(devmaxBytes[id] >= 4)
      {
get_txda_bits:;
      lir_text(40,line[0],"No of bits (16/32):");
      i=lir_get_integer(60,line[0], 2, 16, 32);
      if(kill_all_flag) return;
      if(i==32)
        {
        ui.tx_da_bytes=4;
        }
      else
        {
        if(i != 16)goto get_txda_bits;
        }
      }  
    line[0]++;
    if(devminOutputChannels[id]==1)
      {
      if(devmaxOutputChannels[id]>=2)
        {
        select_txout_format(line);
        }
      else
        {
        ui.tx_da_channels=1;
        }
      }
    else
      {
      if(devmaxOutputChannels[id]>=2)      
        {
        ui.tx_da_channels=2;
        }
      else
        {
        lirerr(850332);
        return;
        }
      }
    line[0]+=2;
    sprintf(s,"Latency Factor (1 to %d)",MAX_LATENCY);
    lir_text(0,line[0],s);
    ui.tx_da_latency=lir_get_integer(26,line[0], 2, 1, MAX_LATENCY);
    break;
    } 
  }
else
  {
  line[0]++;
  settextcolor(14);
  lir_text(2,line[0],"No valid portaudio device detected ");
  SNDLOG"\nNo valid portaudio device detected ");
  line[0]++;
  lir_text(2,line[0],press_any_key);
  settextcolor(7);
  await_keyboard();
  } 
end_portaudiotest:;
}

int pa_get_valid_samplerate (int n,
                             int mode,
                             int *line,
                             unsigned int *returned_sampling_rate)
{
(void) n;
(void) mode;
(void) line;
(void) returned_sampling_rate;
int i,k,id;
unsigned int val;
char s[80];
unsigned int valid_sampling_rate[16];
// negative terminated  list
static double standardSampleRates[] = {8000.0, 9600.0, 
        11025.0, 12000.0, 16000.0, 22050.0, 24000.0, 32000.0,
        44100.0, 48000.0, 88200.0, 96000.0, 192000.0, -1};
const PaDeviceInfo *deviceInfo;
PaError pa_err;
PaStreamParameters inputParameters;
PaStreamParameters outputParameters;
deviceInfo = Pa_GetDeviceInfo(n);
inputParameters.device = n;
inputParameters.channelCount = deviceInfo->maxInputChannels;
inputParameters.sampleFormat = paInt16;
inputParameters.suggestedLatency = 0;
inputParameters.hostApiSpecificStreamInfo = NULL;
outputParameters.device = n;
outputParameters.channelCount = deviceInfo->maxOutputChannels;
outputParameters.sampleFormat = paInt16; 
outputParameters.suggestedLatency = 0;
outputParameters.hostApiSpecificStreamInfo = NULL;
line[0]+=2;
sprintf(s,"This is a portaudio device.");
lir_text(1,line[0], s);
line[0]+=1;
sprintf(s,"Select sampling rate by line number from list :");
lir_text(1,line[0], s);
line[0]+=2;
#if (OSNUM == OSNUM_WINDOWS)
// bypass bug in Juli@ ASIO driver: 
// this driver hangs after a Pa_IsFormatSupported call 
static double JuliSampleRates[] = {
       44100.0, 48000.0, 88200.0, 96000.0, 176400., 192000.0, -1};
i = strncmp(deviceInfo->name, "ASIO 2.0 - ESI Juli@", 19);
k=0;
if (i == 0)
    {
    while(JuliSampleRates[k] > 0 )
      {
      valid_sampling_rate[k]= JuliSampleRates[k];
      sprintf(s,"%2d -> %6d ",k,valid_sampling_rate[k]);
      lir_text(1,line[0],s);
      line[0]++;
      k++;
      }
    goto end_pa_get_valid_samplerate;
    }
#endif
i=0;
k=0;
while(standardSampleRates[i] > 0 )
   {
    val= standardSampleRates[i];
    if(mode==RXDA || mode==TXDA)
      {
      pa_err=Pa_IsFormatSupported(
                          NULL, &outputParameters, standardSampleRates[i] );
      }
    else
      {  
      pa_err=Pa_IsFormatSupported(
                  &inputParameters, NULL, standardSampleRates[i] );
      }
    if(pa_err == paFormatIsSupported )
      { 
      sprintf(s,"%2d -> %6d ",k,val);
      lir_text(1,line[0],s);
      valid_sampling_rate[k]= val;
      line[0]++;
      k++;      
      }
    i++;
   }
#if (OSNUM == OSNUM_WINDOWS)
end_pa_get_valid_samplerate:;
#endif
line[0]++;
sprintf(s,"Enter line number >");
lir_text(1,line[0],s);
id=lir_get_integer(21,line[0],2, 0, k-1);
*returned_sampling_rate=valid_sampling_rate[id];
return 0;
}

int portaudio_startstop(void)
{
PaErrorCode err;
if( (ui.use_alsa&PORTAUDIO_RX_IN) != 0 ||
    (ui.use_alsa&PORTAUDIO_RX_OUT) != 0 ||
    (ui.use_alsa&PORTAUDIO_TX_IN) != 0 ||
    (ui.use_alsa&PORTAUDIO_TX_OUT) != 0 )
  {
  if(portaudio_active_flag == FALSE)
    {
    if(load_pa_library() != 0)
      {
      return FALSE;
      }
#if(OSNUM == OSNUM_LINUX)
    if(pa_linux_jack == 1)
      {
      PaJack_SetClientName(linrad_name);
      }
#endif
    err=Pa_Initialize();
    if(err != paNoError )
      {
#if(OSNUM == OSNUM_WINDOWS)
      lirerr(1415);
#endif
#if(OSNUM == OSNUM_LINUX)
      lirerr(1416);
#endif
      return FALSE;
      }
    if(pa_version != 0)
      {
      DEB"\nPortAudio ver %d \n%s\n", Pa_GetVersion(),Pa_GetVersionText());
      }
    else
      {
      DEB"\nOld PortAudio no support for version info\n");
      }
    portaudio_active_flag=TRUE;
    }
  else
    {
    err=Pa_Terminate();
    if(err != paNoError)
      {
      lirerr(847298);
      return FALSE;
      }
    portaudio_active_flag=FALSE;
    unload_pa_library();
    }
  }
return TRUE;
}

void open_portaudio_txad(void)
{
PaStreamParameters txadParameters;
const PaStreamInfo *streamInfo;
PaError pa_err;
#if (OSNUM == OSNUM_WINDOWS)
const PaDeviceInfo *deviceInfo;
int i;
PaWasapiStreamInfo LinradWasapiStreaminfo;
CoInitialize(NULL);
#endif
txad_status=LIR_OK;
snd[TXAD].open_flag=CALLBACK_CMD_START;
if(portaudio_active_flag == FALSE)
  {
  txad_status=LIR_PA_FLAG_ERROR;
  lirerr(701402);
  return;
  }
// Set sample format
if( (ui.tx_ad_bytes) == 2)
  {
  txadParameters.sampleFormat = paInt16; // 2 byte samples
  }
else
  {  
  txadParameters.sampleFormat = paInt32; // 4 byte samples
  }
txadParameters.device       = ui.tx_addev_no;
txadParameters.channelCount = ui.tx_ad_channels;
txadParameters.suggestedLatency = ui.tx_ad_latency/snd[TXAD].interrupt_rate;
// ******************************************************************************
#if (OSNUM == OSNUM_WINDOWS)
//TO GET BEST LATENCY WITH WASAPI: SELECT EXCLUSIVE MODE INSTEAD OF THE DEFAULT SHARED MODE 
//EXCLUSIVE MODE WORKS FOR VISTAX64, VISTAX86, WINDOWS7
//EXCLUSIVE MODE DOES NOT WORK FOR WOW64 (32-bit applications running on VISTAX64)
deviceInfo = Pa_GetDeviceInfo(ui.tx_addev_no);
if(deviceInfo == NULL)
  {
  rxad_status=LIR_PA_FORMAT_NOT_SUPPORTED;
  goto start_pa_end;
  }
i = strncmp(Pa_GetHostApiInfo( deviceInfo->hostApi )->name,"Windows WASAPI", 14);
if (i==0)
  {
  LinradWasapiStreaminfo.size=sizeof(PaWasapiStreamInfo);
  LinradWasapiStreaminfo.hostApiType=paWASAPI;
  LinradWasapiStreaminfo.version=1;
  LinradWasapiStreaminfo.flags=paWinWasapiExclusive;
  txadParameters.hostApiSpecificStreamInfo = &LinradWasapiStreaminfo;
  }
else
#endif
  {
  txadParameters.hostApiSpecificStreamInfo = NULL;
  }
// ********************************************************************************
pa_err = Pa_IsFormatSupported( &txadParameters, NULL, ui.tx_ad_speed);
  DEB"\nTXAD pa_err=%s\n",Pa_GetErrorText(pa_err));
if( pa_err != paFormatIsSupported )
  {
  DEB"\npa_err=%s\n",Pa_GetErrorText(pa_err));
  txad_status=LIR_PA_FORMAT_NOT_SUPPORTED;
  goto start_pa_end;
  }
mictimf_pc=mictimf_pa;  
lir_init_event(EVENT_PORTAUDIO_TXAD_READY);
// Open and Start audio I/O stream.
pa_err = Pa_OpenStream( 
              &txad_stream,
              &txadParameters,
              NULL,
              (double)(ui.tx_ad_speed),           //SAMPLE_RATE,
              snd[TXAD].block_frames,             //frames per buffer
              paClipOff|paDitherOff,              //no clipping, no dither
              txadCallback,
              NULL );                             //no userdata
if( pa_err != paNoError )
  {
  DEB"\npa_err=%s\n",Pa_GetErrorText(pa_err));
  lir_close_event(EVENT_PORTAUDIO_TXAD_READY);
  txad_status=LIR_PA_OPENSTREAM_FAILED;
  goto start_pa_end;
  }
if(pa_linux_realtime == 1)
  {
  PaAlsa_EnableRealtimeScheduling ( txad_stream, 1);
  }
pa_err = Pa_StartStream( txad_stream );
if( pa_err != paNoError )
  {
  DEB"\npa_err=%s\n",Pa_GetErrorText(pa_err));
  txad_status=LIR_PA_START_STREAM_FAILED;
  goto start_pa_end;
  }
streamInfo = Pa_GetStreamInfo(txad_stream);
DEB"  Actual %.1f",snd[TXAD].interrupt_rate);
DEB"  Latency %.4f\n",streamInfo->inputLatency);
while(!kill_all_flag && snd[TXAD].open_flag != CALLBACK_ANSWER_AWAKE)
  {
  lir_sleep(5000);
  }
tx_audio_in=0;
start_pa_end:;
snd[TXAD].tot_bytes=mictimf_bufsiz;
snd[TXAD].tot_frames=mictimf_bufsiz/snd[TXAD].framesize;
snd[TXAD].no_of_blocks=mictimf_bufsiz/snd[TXAD].block_bytes;
}

void close_portaudio_txad(void)
{
PaError pa_err;
Pa_AbortStream( txad_stream );
pa_err=Pa_IsStreamStopped( txad_stream );
while(pa_err == 0)
  {
  pa_err = Pa_IsStreamStopped( txad_stream );
  lir_sleep(1000);
  } 
pa_err = Pa_CloseStream( txad_stream );
if( pa_err != paNoError )
  {
  lirerr(1229);
  }  
}

void open_portaudio_txda(void)
{
PaStreamParameters  txdaParameters;
const PaStreamInfo *streamInfo;
PaError pa_err;
#if (OSNUM == OSNUM_WINDOWS)
const PaDeviceInfo *deviceInfo;
int i;
PaWasapiStreamInfo LinradWasapiStreaminfo;
CoInitialize(NULL);
#endif
snd[TXDA].open_flag=CALLBACK_CMD_START;
txda_status=LIR_OK;
if(portaudio_active_flag == FALSE)
  {
  txda_status=LIR_PA_FLAG_ERROR;
  lirerr(784401);
  return;
  }
// Set sample format
if(ui.tx_da_bytes == 2)
  {
  txdaParameters.sampleFormat = paInt16; // 2 byte samples
  }
else
  {  
  txdaParameters.sampleFormat = paInt32; // 4 byte samples
  }
txdaParameters.device       = ui.tx_dadev_no;
txdaParameters.channelCount = ui.tx_da_channels;
txdaParameters.suggestedLatency = ui.tx_da_latency/snd[TXDA].interrupt_rate;
// ******************************************************************************
#if (OSNUM == OSNUM_WINDOWS)
//TO GET BEST LATENCY WITH WASAPI: SELECT EXCLUSIVE MODE INSTEAD OF THE DEFAULT SHARED MODE 
//EXCLUSIVE MODE WORKS FOR VISTAX64, VISTAX86, WINDOWS7
//EXCLUSIVE MODE DOES NOT WORK FOR WOW64 (32-bit applications running on VISTAX64)
deviceInfo = Pa_GetDeviceInfo(ui.tx_dadev_no);
if(deviceInfo == NULL)
  {
  rxad_status=LIR_PA_FORMAT_NOT_SUPPORTED;
  goto start_pa_end;
  }
i = strncmp(Pa_GetHostApiInfo( deviceInfo->hostApi )->name,"Windows WASAPI", 14);
if (i==0)
  {
  LinradWasapiStreaminfo.size=sizeof(PaWasapiStreamInfo);
  LinradWasapiStreaminfo.hostApiType=paWASAPI;
  LinradWasapiStreaminfo.version=1;
  LinradWasapiStreaminfo.flags=paWinWasapiExclusive;
  txdaParameters.hostApiSpecificStreamInfo = &LinradWasapiStreaminfo;
  }
else
#endif
  {
  txdaParameters.hostApiSpecificStreamInfo = NULL;
  }
// ********************************************************************************
pa_err = Pa_IsFormatSupported( NULL,&txdaParameters, ui.tx_da_speed);
if( pa_err != paFormatIsSupported )
  {
  txda_status=LIR_PA_FORMAT_NOT_SUPPORTED;
  goto start_pa_end;
  }
// Open and Start audio I/O stream.
pa_err = Pa_OpenStream( 
              &txda_stream,
              NULL,                     //no input
              &txdaParameters,
              ui.tx_da_speed,           //SAMPLE_RATE,
              snd[TXDA].block_frames,   //frames per buffer
              paClipOff|paDitherOff,    //no clipping, no dither
              txdaCallback,
              NULL );                   //no userdata
if( pa_err != paNoError )
  {
  txda_status=LIR_PA_OPENSTREAM_FAILED;
  goto start_pa_end;
  }
if(pa_linux_realtime == 1)
  {
  PaAlsa_EnableRealtimeScheduling ( txda_stream, 1);
  }
pa_err = Pa_StartStream( txda_stream );
if( pa_err != paNoError )
  {
  txda_status=LIR_PA_START_STREAM_FAILED;
  goto start_pa_end;
  }
streamInfo = Pa_GetStreamInfo(txda_stream);
snd[TXDA].interrupt_rate=ui.tx_da_speed/snd[TXDA].block_frames;
DEB"  Actual %.1f",snd[TXDA].interrupt_rate);
DEB"  Latency %.4f\n",streamInfo->outputLatency);
while(!kill_all_flag && snd[TXDA].open_flag != CALLBACK_ANSWER_AWAKE)
  {
  lir_sleep(5000);
  }
start_pa_end:;
// set an arbitrary positive value.
if(txda_status == LIR_OK)tx_audio_out=1;
snd[TXDA].tot_bytes=txout_bufsiz;
snd[TXDA].tot_frames=txout_bufsiz/snd[TXDA].framesize;
snd[TXDA].no_of_blocks=txout_bufsiz/snd[TXDA].block_bytes;
}

void close_portaudio_txda(void)
{
PaError pa_err;
Pa_AbortStream( txda_stream );
pa_err=Pa_IsStreamStopped( txda_stream );
while(pa_err == 0)
  {
  pa_err = Pa_IsStreamStopped( txda_stream );
  lir_sleep(1000);
  } 
pa_err = Pa_CloseStream( txda_stream );
if( pa_err != paNoError )
  {
  lirerr(1229);
  }  
}
