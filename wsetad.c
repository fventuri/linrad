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
#include "sdrdef.h"
#include "txdef.h"
#include "vernr.h"
#include "keyboard_def.h"
#include "hwaredef.h"
#include "options.h"
#include "padef.h"
#include "wscreen.h"

WAVEFORMATEXTENSIBLE fmt;
int select_portaudio_dll(void);

size_t local_txadin_newbuf_ptr;
DWORD WINAPI winthread_test_rxad(PVOID arg);
DWORD WINAPI winthread_test_rxda(PVOID arg);
DWORD WINAPI winthread_test_txda(PVOID arg);
HANDLE thread_iotest_id[MAX_IOTEST];

LPTHREAD_START_ROUTINE iotest_routines[MAX_IOTEST]=
                          {winthread_test_rxad,     //0
                           winthread_test_rxda,     //1
                           winthread_test_txda};    //2

DWORD WINAPI  winthread_test_rxda(PVOID arg)
{
(void) arg;
test_rxda();
return 0;
}

DWORD WINAPI  winthread_test_txda(PVOID arg)
{
(void) arg;
test_txda();
return 0;
}

DWORD WINAPI  winthread_test_rxad(PVOID arg)
{
(void) arg;
test_rxad();
return 0;
}

int start_iotest(int *line, int mode)
{
char *mode_text[3]={"RX INPUT", "RX OUTPUT", "TX OUTPUT"};
char s[80];
int i;
// To make sure that the devices we select for tx or for rx output can
// be used while the rx input is running we start a separate thread that
// uses the rx input so we will fail on devices that do not
// allow simultaneous read and write.
soundcard_test_block_count[mode]=-1;
soundcard_test_cmd_flag[mode]=THRFLAG_ACTIVE;
lir_sleep(20000);
thread_iotest_id[mode] = CreateThread(NULL, 0, 
                                      iotest_routines[mode], NULL, 0, NULL);
if(thread_iotest_id[mode] == NULL)return FALSE;
i=0;
while(i < 200 && soundcard_test_block_count[mode] == -1)
  {
  lir_sleep(10000);
  if(kill_all_flag)return FALSE;
  i++;
  }
if(i >= 200)
  {
  sprintf(s,"COULD NOT START THE %s",mode_text[mode]);
  goto msg;
  }
i=0;
while(i < 100 && soundcard_test_block_count[mode] < 8)
  {
  lir_sleep(10000);
  if(kill_all_flag)return FALSE;
  i++;
  }
if(i >= 100)
  {
  sprintf(s,"FAILED TO RUN THE %s",mode_text[mode]);
msg:;
  line[0]+=2;
  settextcolor(12);
  lir_text(0,line[0],s);
  line[0]++;
  if(mode == RXAD)
    {
    lir_text(0,line[0],"Set a working input or force no soundcard.");
    line[0]++;
    }
  settextcolor(7);
  lir_text(0,line[0],press_any_key);
  await_keyboard();
  line[0]+=2;
  return FALSE;
  }
settextcolor(7);
return TRUE;
}

void stop_iotest(int mode)
{
soundcard_test_cmd_flag[mode]=THRFLAG_KILL;
WaitForSingleObject(thread_iotest_id[mode],INFINITE);
}

DWORD WINAPI winthread_rx_adinput(PVOID arg)
{
char s[40];
int rxin_local_workload_reset;
int timing_loop_counter,timing_loop_counter_max,initial_skip_flag;
size_t local_rxadin1_newbuf_ptr;
size_t local_rxadin2_newbuf_ptr;
double read_start_time, total_reads;
MMRESULT mmrs;
WAVEHDR *whdr1, *whdr2;
char *c1;
short int *c1_isho, *c2_isho;
int *c1_int, *c2_int;
int i, k;
(void)arg;
sys_func(THRFLAG_OPEN_RX_SNDIN);
lir_sched_yield();
if(kill_all_flag) goto rxadin_init_error;
if(rxad_status != LIR_OK)
  {
  lirerr(1419);
  goto rxadin_init_error;
  }
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST);
thread_status_flag[THREAD_RX_ADINPUT]=THRFLAG_ACTIVE;
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
if ( (ui.use_alsa&PORTAUDIO_RX_IN) != 0)
  {
// Do nothing, but clear some pointers to satisfy the compiler.
  local_rxadin1_newbuf_ptr=0;
  local_rxadin2_newbuf_ptr=0;
  }
else
  {    
  EnterCriticalSection(&protect_rxin1);
  local_rxadin1_newbuf_ptr=rxadin1_newbuf_ptr;
  LeaveCriticalSection(&protect_rxin1);
  if(ui.rx_addev_no >= 256)
    {
    EnterCriticalSection(&protect_rxin2);
    local_rxadin2_newbuf_ptr=rxadin2_newbuf_ptr;
    LeaveCriticalSection(&protect_rxin2);
    }
  else
    {
    local_rxadin2_newbuf_ptr=0;
    }
  }
snd[RXAD].open_flag=CALLBACK_CMD_ACTIVE;
if(thread_command_flag[THREAD_SCREEN]!=THRFLAG_NOT_ACTIVE)
  {
  while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
    {
    if(thread_command_flag[THREAD_RX_ADINPUT] ==
                                        THRFLAG_KILL)goto rxadin_init_error;
    lir_sleep(10000);
    }
  }
if( allow_wse_parport != 0)
  {
  linrad_thread_create(THREAD_HWARE_COMMAND);
  }
#include "timing_setup.c"
lir_status=LIR_OK;
while(thread_command_flag[THREAD_RX_ADINPUT] == THRFLAG_ACTIVE)
  {
#include "input_speed.c"
  if ( (ui.use_alsa&PORTAUDIO_RX_IN) != 0)
    {
    while (!kill_all_flag && timf1p_sdr == timf1p_pa)
      {
      lir_await_event(EVENT_PORTAUDIO_RXAD_READY);
      }
    }
  else
    {
    EnterCriticalSection(&protect_rxin1);
    k=rxadin1_newbuf_ptr-local_rxadin1_newbuf_ptr;
    LeaveCriticalSection(&protect_rxin1);
    if(k<0)k+=snd[RXAD].no_of_blocks;
    if(k>=snd[RXAD].no_of_blocks-2)
      {
      no_of_rx_overrun_errors++;
      sprintf(s,"RX%s%d",overrun_error_msg,no_of_rx_overrun_errors);
      wg_error(s,WGERR_RXIN);
      }
    if(k<=1)
      {
next_event1:;    
      WaitForSingleObject(rxin1_bufready, INFINITE);
      }
    whdr1=(WAVEHDR*)rxadin1_newbuf[local_rxadin1_newbuf_ptr];
    if( (whdr1[0].dwFlags & WHDR_DONE) == 0)
      {
      wg_error("SOUND ERROR. PLEASE REPORT!!!!",WGERR_RXIN+1);
      goto next_event1;
      }
    c1=whdr1[0].lpData;
    if(ui.rx_addev_no < 256)
      {
      memcpy(rxin_isho,c1,snd[RXAD].block_bytes);
      if(local_rxadin1_newbuf_ptr==rxadin1_newbuf_ptr)
        {
        no_of_rx_overrun_errors++;
        sprintf(s,"RX%s%d",overrun_error_msg,no_of_rx_overrun_errors);
        wg_error(s,WGERR_RXIN);
        }
      local_rxadin1_newbuf_ptr++;
      if(local_rxadin1_newbuf_ptr>=(unsigned)snd[RXAD].no_of_blocks)local_rxadin1_newbuf_ptr=0;
      mmrs=waveInAddBuffer(hwav_rxadin1,whdr1, sizeof(WAVEHDR));
      if(mmrs != MMSYSERR_NOERROR)
        {
        lirerr(1226);
        goto rxadin_error;
        }
      }
    else
      {
      EnterCriticalSection(&protect_rxin2);
      k=rxadin2_newbuf_ptr-local_rxadin2_newbuf_ptr;
      LeaveCriticalSection(&protect_rxin2);
      if(k<0)k+=snd[RXAD].no_of_blocks;
      if(k>=snd[RXAD].no_of_blocks-2)
        {
        no_of_rx_overrun_errors++;
        sprintf(s,"RX%s%d",overrun_error_msg,no_of_rx_overrun_errors);
        wg_error(s,WGERR_RXIN);
        }
      if(k<=1)
        {
next_event2:;      
        WaitForSingleObject(rxin2_bufready, INFINITE);
        }
      whdr2=(WAVEHDR*)rxadin2_newbuf[local_rxadin2_newbuf_ptr];
      if( (whdr2[0].dwFlags & WHDR_DONE) == 0)
        {
        wg_error("SOUND ERROR. PLEASE REPORT!!!!",WGERR_RXIN+1);
        goto next_event2;
        }
      if( (ui.rx_input_mode&DWORD_INPUT) == 0)
        {
        k=snd[RXAD].block_bytes>>3;
        c1_isho=(short int*)whdr1[0].lpData;
        c2_isho=(short int*)whdr2[0].lpData;
        for(i=0; i<k; i++)
          {
          rxin_isho[4*i  ]=c1_isho[2*i  ];
          rxin_isho[4*i+1]=c1_isho[2*i+1];
          rxin_isho[4*i+2]=c2_isho[2*i  ];
          rxin_isho[4*i+3]=c2_isho[2*i+1];
          }
        }
      else
        {
        k=snd[RXAD].block_bytes>>4;
        c1_int=(int*)whdr1[0].lpData;
        c2_int=(int*)whdr2[0].lpData;
        for(i=0; i<k; i++)
          {
          rxin_int[4*i  ]=c1_int[2*i  ];
          rxin_int[4*i+1]=c1_int[2*i+1];
          rxin_int[4*i+2]=c2_int[2*i  ];
          rxin_int[4*i+3]=c2_int[2*i+1];
          }
        }
      local_rxadin1_newbuf_ptr++;
      local_rxadin2_newbuf_ptr++;
      if(local_rxadin1_newbuf_ptr>=(unsigned)snd[RXAD].no_of_blocks)local_rxadin1_newbuf_ptr=0;
      if(local_rxadin2_newbuf_ptr>=(unsigned)snd[RXAD].no_of_blocks)local_rxadin2_newbuf_ptr=0;
      mmrs=waveInAddBuffer(hwav_rxadin1,whdr1, sizeof(WAVEHDR));
      if(mmrs != MMSYSERR_NOERROR)
        {
        lirerr(1232);
        goto rxadin_error;
        }
      mmrs=waveInAddBuffer(hwav_rxadin2,whdr2, sizeof(WAVEHDR));
      if(mmrs != MMSYSERR_NOERROR)
        {
        lirerr(1233);
        goto rxadin_error;
        }
      }
    }
  finish_rx_read();
  }
rxadin_error:;
close_rx_sndin();
rxadin_init_error:;
thread_status_flag[THREAD_RX_ADINPUT]=THRFLAG_RETURNED;
lir_sched_yield();
if(!kill_all_flag)
  {
  while(thread_command_flag[THREAD_RX_ADINPUT] != THRFLAG_NOT_ACTIVE)
    {
    lir_sleep(1000);
    }
  }
if( allow_wse_parport != 0)
  {
  linrad_thread_stop_and_join(THREAD_HWARE_COMMAND);
  }
return 100;
}

VOID CALLBACK mme_callback_rxad1(HWAVEIN hwi, UINT iMsg, DWORD dwInst,
                         DWORD dwParam1, DWORD dwParam2)
{
(void) hwi;
(void) dwInst;
(void) dwParam2;
if(snd[RXAD].open_flag != CALLBACK_CMD_ACTIVE)
  {
  if(snd[RXAD].open_flag == CALLBACK_CMD_START)
    {
    snd[RXAD].open_flag=CALLBACK_ANSWER_AWAKE;
    }
  if(snd[RXAD].open_flag == CALLBACK_ANSWER_AWAKE)return;
  snd[RXAD].open_flag=CALLBACK_ANSWER_CLOSED;
  return;
  }
if(iMsg == WIM_DATA)
  {
  EnterCriticalSection(&protect_rxin1);
  rxadin1_newbuf[rxadin1_newbuf_ptr]=dwParam1;
  rxadin1_newbuf_ptr++;
  if(rxadin1_newbuf_ptr >= (unsigned)snd[RXAD].no_of_blocks)rxadin1_newbuf_ptr=0;
  LeaveCriticalSection(&protect_rxin1);
  SetEvent(rxin1_bufready);
  }
else
  {
  if(iMsg == WIM_CLOSE)
    {
    rxad_mme_open_flag1=0;
    }
  }
}

VOID CALLBACK mme_callback_rxad2(HWAVEIN hwi, UINT iMsg, DWORD dwInst,
                         DWORD dwParam1, DWORD dwParam2)
{
(void) hwi;
(void) dwInst;
(void) dwParam2;
if(snd[RXAD].open_flag != CALLBACK_CMD_ACTIVE)
  {
  if(snd[RXAD].open_flag == CALLBACK_CMD_START)
    {
    snd[RXAD].open_flag=CALLBACK_ANSWER_AWAKE;
    }
  if(snd[RXAD].open_flag == CALLBACK_ANSWER_AWAKE)return;
  snd[RXAD].open_flag=CALLBACK_ANSWER_CLOSED;
  return;
  }
if(iMsg == WIM_DATA)
  {
  EnterCriticalSection(&protect_rxin2);
  rxadin2_newbuf[rxadin2_newbuf_ptr]=dwParam1;
  rxadin2_newbuf_ptr++;
  if(rxadin2_newbuf_ptr >= (unsigned)snd[RXAD].no_of_blocks)rxadin2_newbuf_ptr=0;
  LeaveCriticalSection(&protect_rxin2);
  SetEvent(rxin2_bufready);
  }
else
  {
  if(iMsg == WIM_CLOSE)
    {
    rxad_mme_open_flag2=0;
    }
  }
}

int lir_tx_output_samples(void)
{
if ( (ui.use_alsa&PORTAUDIO_TX_OUT) != 0)
  {
  return ((txout_pa-txout_px+txout_bufsiz)&txout_mask)/2;
  }
return 0;
}

int lir_tx_input_samples(void)
{
return 0;
}

VOID CALLBACK mme_callback_txad(HWAVEIN hwi, UINT iMsg, DWORD dwInst,
                         size_t dwParam1, DWORD dwParam2)
{
(void) hwi;
(void) dwInst;
(void) dwParam2;
if(snd[TXAD].open_flag != CALLBACK_CMD_ACTIVE)
  {
  if(snd[TXAD].open_flag == CALLBACK_CMD_START)
    {
    snd[TXAD].open_flag=CALLBACK_ANSWER_AWAKE;
    }
  if(snd[TXAD].open_flag == CALLBACK_ANSWER_AWAKE)return;
  snd[TXAD].open_flag=CALLBACK_ANSWER_CLOSED;
  return;
  }
if(iMsg == WIM_DATA)
  {
  EnterCriticalSection(&protect_txin);
  txadin_newbuf[txadin_newbuf_ptr]=dwParam1;
  txadin_newbuf_ptr++;
  if(txadin_newbuf_ptr >= (unsigned)snd[TXAD].no_of_blocks)txadin_newbuf_ptr=0;
  LeaveCriticalSection(&protect_txin);
  SetEvent(txin_bufready);
  }
else
  {
  if(iMsg == WIM_CLOSE)
    {
    txad_mme_open_flag=0;
    }
  }
}

void lir_tx_adread(char *buf)
{
MMRESULT mmrs;
char s[80];
int k;
WAVEHDR *whdr;
if ( (ui.use_alsa&PORTAUDIO_TX_IN) != 0)
  {
  return;
  }
EnterCriticalSection(&protect_txin);
k=txadin_newbuf_ptr-local_txadin_newbuf_ptr;
LeaveCriticalSection(&protect_txin);
if(k<0)k+=snd[TXAD].no_of_blocks;
if(k>=snd[TXAD].no_of_blocks-2)
  {
  no_of_tx_overrun_errors++;
  sprintf(s,"TX%s%d",overrun_error_msg,no_of_tx_overrun_errors);
  wg_error(s,WGERR_TXIN);
  }
if(k<=1)
  {
next_event:;    
  WaitForSingleObject(txin_bufready, INFINITE);
  }
whdr=(WAVEHDR*)txadin_newbuf[local_txadin_newbuf_ptr];
if( (whdr[0].dwFlags & WHDR_DONE) == 0)
  {
  wg_error("SOUND ERROR. PLEASE REPORT!!!!",WGERR_TXIN+1);
  goto next_event;
  }
//memcpy(buf,whdr[0].lpData,mictimf_adread_block);
memcpy(buf,whdr[0].lpData,snd[TXAD].block_bytes);
if(local_txadin_newbuf_ptr==txadin_newbuf_ptr)
  {
  no_of_tx_overrun_errors++;
  sprintf(s,"TX%s%d",overrun_error_msg,no_of_tx_overrun_errors);
  wg_error(s,WGERR_TXIN);
  }
local_txadin_newbuf_ptr++;
if(local_txadin_newbuf_ptr>=(unsigned)snd[TXAD].no_of_blocks)local_txadin_newbuf_ptr=0;
mmrs=waveInAddBuffer(hwav_txadin,whdr, sizeof(WAVEHDR));
if(mmrs != MMSYSERR_NOERROR)
  {
  lirerr(1226);
  }
}

VOID CALLBACK rxda_mme_callback(HWAVEOUT hwi, UINT iMsg, DWORD dwInst,
                         DWORD dwParam1, DWORD dwParam2)
{
(void) hwi;
(void) dwInst;
(void) dwParam2;
(void) dwParam1;
if(iMsg == WIM_DATA)
  {
  }
else
  {
  if(iMsg == WIM_CLOSE)
    {
    rx_audio_in=-1;
    }
  else
    {
    if(iMsg == WIM_OPEN)
      {
      rx_audio_in=0;
      }
    }
  }
}

VOID CALLBACK txda_mme_callback(HWAVEOUT hwi, UINT iMsg, DWORD dwInst,
                         DWORD dwParam1, DWORD dwParam2)
{
(void)hwi;
(void)dwInst;
(void)dwParam1;
(void)dwParam2;
if(iMsg == WOM_DONE)
  {
  }
else
  {
  if(iMsg == WOM_CLOSE)
    {
    tx_audio_out=-1;
    }
  else
    {
    if(iMsg == WOM_OPEN)
      {
      tx_audio_out=0;
      }
    }
  }
}

void fill_wavefmt(int channels, int bits, int flag, int speed)
{
fmt.Format.nSamplesPerSec=speed;
if(bits==16)
  {
  fmt.Samples.wValidBitsPerSample=16;
  fmt.Format.wBitsPerSample=16;
  if( flag == 0)
    {
    fmt.Format.wFormatTag=WAVE_FORMAT_PCM;
    fmt.Format.cbSize=0;
    }
  else
    {
    fmt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    fmt.Format.cbSize=sizeof(WAVEFORMATEXTENSIBLE);
    fmt.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    }
  }
else
  {
  if( flag == 0)
    {
    lirerr(2654844);
    return;
    }
  fmt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
  fmt.Samples.wValidBitsPerSample=24;
  fmt.Format.wBitsPerSample=32;
  fmt.Format.cbSize=sizeof(WAVEFORMATEXTENSIBLE);
  fmt.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
  }
fmt.Format.nChannels=channels;
switch (channels)
  {
  case 1:
  fmt.dwChannelMask = 1;
  break;
  
  case 2:
  fmt.dwChannelMask = 3;
  break;
  
  case 4:
  fmt.dwChannelMask = 0x33;
  break;
  
  default:
  lirerr(2963432);
  return;
  }
fmt.Format.nBlockAlign=fmt.Format.nChannels*fmt.Format.wBitsPerSample/8;
fmt.Format.nAvgBytesPerSec=fmt.Format.nBlockAlign*fmt.Format.nSamplesPerSec;
}

void open_tx_sndout(void)
{
int errcod;
WAVEFORMATEXTENSIBLE wfmt;
WAVEOUTCAPS pwoc;
MMRESULT mmrs;
ssize_t ptr;
int i;
if(tx_audio_out != -1)return;
if(ui.tx_dadev_no <0 || ui.tx_dadev_no >= SPECIFIC_DEVICE_CODES)
  {
  lirerr(1408);
  return;
  }
snd[TXDA].block_frames=snd[TXDA].block_bytes/snd[TXDA].framesize;
if(snd[TXDA].block_frames<8)snd[TXDA].block_frames=8;
snd[TXDA].interrupt_rate=(float)(ui.tx_da_speed)/(float)snd[TXDA].block_frames;
while(snd[TXDA].interrupt_rate < ui.min_dma_rate && snd[TXDA].block_frames>8)
  {
  snd[TXDA].interrupt_rate*=2;
  snd[TXDA].block_bytes/=2;
  snd[TXDA].block_frames/=2;
  }
while(snd[TXDA].interrupt_rate > ui.max_dma_rate)
  {
  snd[TXDA].interrupt_rate/=2;
  snd[TXDA].block_bytes*=2;
  snd[TXDA].block_frames*=2;
  }
snd[TXDA].no_of_blocks=1.F*snd[TXDA].interrupt_rate;
make_power_of_two(&snd[TXDA].no_of_blocks);
if(snd[TXDA].no_of_blocks < 8)snd[TXDA].no_of_blocks=8; 
DEB"\nTXout interrupt_rate: Desired %.1f",snd[TXDA].interrupt_rate);
if( (ui.use_alsa&PORTAUDIO_TX_OUT) != 0)
  {
  open_portaudio_txda();
  return;
  }
// Currently we use EXTENSIBLE always!!  
wfmt.Format.nSamplesPerSec=ui.tx_da_speed;
wfmt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
if(ui.tx_da_bytes == 2)
  {
  wfmt.Samples.wValidBitsPerSample=16;
  wfmt.Format.wBitsPerSample=16;
  }
else
  {
  wfmt.Samples.wValidBitsPerSample=24;
  wfmt.Format.wBitsPerSample=32;
  }
wfmt.Format.cbSize=sizeof(WAVEFORMATEXTENSIBLE);;
wfmt.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
wfmt.Format.nChannels=ui.tx_da_channels;
switch(ui.tx_da_channels)
  {
  case 1:
  wfmt.dwChannelMask = 1;
  break;
  
  case 2:
  wfmt.dwChannelMask = 3;
  break;
  
  default:
  lirerr(2962431);
  return;
  }
wfmt.Format.nBlockAlign=(unsigned int)((wfmt.Format.nChannels *
                                         wfmt.Format.wBitsPerSample)/8);
wfmt.Format.nAvgBytesPerSec=wfmt.Format.nBlockAlign*wfmt.Format.nSamplesPerSec;
mmrs=waveOutGetDevCaps(ui.tx_dadev_no, &pwoc, sizeof(WAVEOUTCAPS));
if(mmrs != MMSYSERR_NOERROR)
  {
  lirerr(1425);
  return;
  }
mmrs=waveOutOpen(&hwav_txdaout, ui.tx_dadev_no, (WAVEFORMATEX*)&wfmt, 
                              (size_t)txda_mme_callback, 0, CALLBACK_FUNCTION);
if(mmrs != MMSYSERR_NOERROR)
  {
  lirerr(1424);
  return;
  }
errcod=1331;
tx_waveout_buf=malloc(snd[TXDA].no_of_blocks*snd[TXDA].block_bytes+
                                                  wfmt.Format.nBlockAlign);
if(tx_waveout_buf==NULL)goto wave_close;
tx_wave_outhdr=malloc(snd[TXDA].no_of_blocks*sizeof(WAVEHDR));
if(tx_wave_outhdr==NULL)goto free_waveout;
txdaout_newbuf=malloc(snd[TXDA].no_of_blocks*sizeof(WAVEHDR));
if(txdaout_newbuf==NULL)goto free_outhdr;
txdaout_newbuf_ptr=0;
for(i=0; i<snd[TXDA].no_of_blocks; i++)
  {
  ptr=(ssize_t)(tx_waveout_buf+i*snd[TXDA].block_bytes+wfmt.Format.nBlockAlign);
  ptr&=(-1-wfmt.Format.nBlockAlign);
  tx_wave_outhdr[i].lpData=(char*)ptr;
  tx_wave_outhdr[i].dwBufferLength=snd[TXDA].block_bytes;
  tx_wave_outhdr[i].dwBytesRecorded=0;
  tx_wave_outhdr[i].dwUser=1;
  tx_wave_outhdr[i].dwFlags=0;
  tx_wave_outhdr[i].dwLoops=0;
  tx_wave_outhdr[i].lpNext=NULL;
  tx_wave_outhdr[i].reserved=0;
  mmrs=waveOutPrepareHeader(hwav_txdaout, &tx_wave_outhdr[i], sizeof(WAVEHDR));
  txdaout_newbuf[i]=&tx_wave_outhdr[i];
  if(mmrs != MMSYSERR_NOERROR)goto errfree;
  }
snd[TXDA].tot_bytes=snd[TXDA].no_of_blocks*snd[TXDA].block_bytes;
snd[TXDA].tot_frames=snd[TXDA].no_of_blocks*snd[TXDA].block_frames;
tx_audio_out=0;
return;
errfree:;
free(txdaout_newbuf);
free_outhdr:;
free(tx_wave_outhdr);
free_waveout:;
free(tx_waveout_buf);
wave_close:;
waveOutClose(hwav_txdaout);
tx_audio_out=-1;
lirerr(errcod);
}

void close_tx_sndout(void)
{
MMRESULT mmrs;
WAVEHDR *whdr;
int i;
if( (ui.use_alsa&PORTAUDIO_TX_OUT) != 0)
  {
  close_portaudio_txda();
  goto freebuf;
  }
//if(close(tx_audio_out)==-1)lirerr(231037);
mmrs=waveOutReset(hwav_txdaout);
if(mmrs != MMSYSERR_NOERROR)lirerr(25344);
for(i=0; i<snd[TXDA].no_of_blocks; i++)
  {
  whdr=txdaout_newbuf[i];
  while( (whdr[0].dwFlags&WHDR_INQUEUE) != 0)lir_sleep(3000);
  mmrs=waveOutUnprepareHeader(hwav_txdaout, whdr, sizeof(WAVEHDR));
  if(mmrs != MMSYSERR_NOERROR)lirerr(25144);
  }
mmrs=waveOutClose(hwav_txdaout);
if(mmrs != MMSYSERR_NOERROR)lirerr(25344);
free(tx_wave_outhdr);
free(tx_waveout_buf);
free(txdaout_newbuf);
freebuf:;
tx_audio_out=-1;
}

void open_tx_sndin(void)
{
MMRESULT mmrs;
WAVEINCAPS pwic;
int errcod;
int i, bits;
ssize_t ptr;
if(ui.tx_addev_no <0 || ui.tx_addev_no >= SPECIFIC_DEVICE_CODES)
  {
  lirerr(1409);
  return;
  }
snd[TXAD].block_frames=snd[TXAD].block_bytes/snd[TXAD].framesize;
if(snd[TXAD].block_frames<8)snd[TXAD].block_frames=8;
snd[TXAD].interrupt_rate=(float)(ui.tx_ad_speed)/(float)snd[TXAD].block_frames;
while(snd[TXAD].interrupt_rate < ui.min_dma_rate && snd[TXAD].block_frames>8)
  {
  snd[TXAD].interrupt_rate*=2;
  snd[TXAD].block_bytes/=2;
  snd[TXAD].block_frames/=2;
  }
while(snd[TXAD].interrupt_rate > ui.max_dma_rate &&
      snd[TXAD].block_frames > 8)
  {
  snd[TXAD].interrupt_rate/=2;
  snd[TXAD].block_bytes*=2;
  snd[TXAD].block_frames*=2;
  }
DEB"\nTXin interrupt_rate: Desired %.1f",snd[TXAD].interrupt_rate);
if( (ui.use_alsa&PORTAUDIO_TX_IN) != 0)
  {
  open_portaudio_txad();
  return;
  }
if( ui.tx_ad_bytes == 2)
  {
  bits=16;
  }
else
  {
  bits=24;
  if( (ui.use_alsa&TXAD_USE_EXTENSIBLE) == 0)
    {
    lirerr(1422);
    return;
    }
  }
mmrs=waveInGetDevCaps(ui.tx_addev_no, &pwic, sizeof(WAVEINCAPS));
if(mmrs != MMSYSERR_NOERROR)
  {
  lirerr(1421);
  return;
  }
fill_wavefmt(ui.tx_ad_channels, bits,
                           (ui.use_alsa&TXAD_USE_EXTENSIBLE),ui.tx_ad_speed);
if(fmt.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
  {
  mmrs=waveInOpen(&hwav_txadin, ui.tx_addev_no, (WAVEFORMATEX*)&fmt,
                           (size_t)mme_callback_txad, 0, CALLBACK_FUNCTION);
  }
else
  {
  mmrs=waveInOpen(&hwav_txadin, ui.tx_addev_no, &fmt.Format,
                                    (size_t)mme_callback_txad, 0, CALLBACK_FUNCTION);
  }
if(mmrs != MMSYSERR_NOERROR)
  {
  switch (mmrs)
    {
    case MMSYSERR_ALLOCATED:
    lirerr(1192);
    break;

    case MMSYSERR_BADDEVICEID:
    lirerr(1146);
    break;

    case MMSYSERR_NODRIVER:
    lirerr(1147);
    break;

    case MMSYSERR_NOMEM:
    lirerr(1155);
    break;

    case WAVERR_BADFORMAT:
    lirerr(1156);
    break;

    default:
    lirerr(1227);
    break;
    }
  return;
  }
txad_mme_open_flag=1;
txin_bufready=CreateEvent( NULL, FALSE, FALSE, NULL);
if(txin_bufready== NULL)
  {
  errcod=1223;
  goto errclose;
  }
InitializeCriticalSection(&protect_txin);
snd[TXAD].no_of_blocks=.1F*(float)(ui.tx_ad_speed*
                                 snd[TXAD].framesize)/snd[TXAD].block_bytes;
if(snd[TXAD].no_of_blocks<12)snd[TXAD].no_of_blocks=12;
tx_wavein_buf=malloc(
              snd[TXAD].no_of_blocks*snd[TXAD].block_bytes+fmt.Format.nBlockAlign);
if(tx_wavein_buf == NULL)
  {
  errcod=1236;
  goto errclose;
  }
snd[TXAD].tot_bytes=snd[TXAD].no_of_blocks*snd[TXAD].block_bytes;
snd[TXAD].tot_frames=snd[TXAD].tot_bytes/snd[TXAD].framesize;
tx_wave_inhdr=malloc(snd[TXAD].no_of_blocks*(sizeof(WAVEHDR)+sizeof(size_t)));
if(tx_wave_inhdr == NULL)
  {
  errcod=1236;
  goto errfree_inbuf;
  }
txadin_newbuf=(size_t*)&tx_wave_inhdr[snd[TXAD].no_of_blocks];
txadin_newbuf_ptr=0;
local_txadin_newbuf_ptr=0;
for(i=0; i<snd[TXAD].no_of_blocks; i++)
  {
  ptr=(ssize_t)(tx_wavein_buf+i*snd[TXAD].block_bytes+fmt.Format.nBlockAlign);
  ptr&=(-1-fmt.Format.nBlockAlign);
  tx_wave_inhdr[i].lpData=(char*)(ptr);
  tx_wave_inhdr[i].dwBufferLength=snd[TXAD].block_bytes;
  tx_wave_inhdr[i].dwBytesRecorded=0;
  tx_wave_inhdr[i].dwUser=0;
  tx_wave_inhdr[i].dwFlags=0;
  tx_wave_inhdr[i].dwLoops=0;
  tx_wave_inhdr[i].lpNext=NULL;
  tx_wave_inhdr[i].reserved=0;
  mmrs=waveInPrepareHeader(hwav_txadin, &tx_wave_inhdr[i], sizeof(WAVEHDR));
  if(mmrs != MMSYSERR_NOERROR)
    {
    errcod=1245;
    goto errfree;
    }
  mmrs=waveInAddBuffer(hwav_txadin,&tx_wave_inhdr[i], sizeof(WAVEHDR));
  if(mmrs != MMSYSERR_NOERROR)
    {
    errcod=1247;
    goto errfree;
    }
  }
DEB"  Actual %.1f",snd[TXAD].interrupt_rate);  
waveInStart(hwav_txadin);
return;
errfree:;
free(tx_wave_inhdr);
errfree_inbuf:;
free(tx_wavein_buf);
errclose:;
waveInClose(hwav_txadin);
lirerr(errcod);
}

void close_tx_sndin(void)
{
if( (ui.use_alsa&PORTAUDIO_TX_IN) != 0)
  {
  close_portaudio_txad();
  goto close_tx_sndin_x;
  }
waveInReset(hwav_txadin);
waveInClose(hwav_txadin);
while(txad_mme_open_flag != 0)
  {
  lir_sleep(1000);
  }
free(tx_wave_inhdr);
free(tx_wavein_buf);
CloseHandle(txin_bufready);
DeleteCriticalSection(&protect_txin);
txin_bufready=NULL;
close_tx_sndin_x:;
tx_audio_in=-1;
}

void lir_empty_da_device_buffer(void)
{
if( (ui.use_alsa&PORTAUDIO_RX_OUT) != 0)
  {
  return;
  }
if(rx_audio_out == 0)
  {
  waveOutReset(hwav_rxdaout);
  }
else
  {
  lirerr(745251);
  }
}

void close_rx_sndout(void)
{
MMRESULT mmrs;
WAVEHDR *whdr;
int i;
if(rx_audio_out == -1)return;
if( (ui.use_alsa&PORTAUDIO_RX_OUT) != 0)
  {
  close_portaudio_rxda();
  goto freebuf;
  }
mmrs=waveOutReset(hwav_rxdaout);
if(mmrs != MMSYSERR_NOERROR)lirerr(25344);
for(i=0; i<snd[RXDA].no_of_blocks; i++)
  {
  whdr=rxdaout_newbuf[i];
  while( (whdr[0].dwFlags&WHDR_INQUEUE) != 0)lir_sleep(3000);
  mmrs=waveOutUnprepareHeader(hwav_rxdaout, whdr, sizeof(WAVEHDR));
  if(mmrs != MMSYSERR_NOERROR)lirerr(25144);
  }
mmrs=waveOutClose(hwav_rxdaout);
if(mmrs != MMSYSERR_NOERROR)lirerr(25344);
free(rx_wave_outhdr);
free(rx_waveout_buf);
free(rxdaout_newbuf);
freebuf:;
rx_audio_out=-1;
}

void lir_tx_dawrite(char * buf)
{
char *c;
WAVEHDR *whdr;
if( (ui.use_alsa&PORTAUDIO_TX_OUT) != 0)
  {
// We assume Portaudio is always OK.  
  lir_sched_yield();
  return;
  }
// Wait until there is a buffer ready for us.
whdr=txdaout_newbuf[txdaout_newbuf_ptr];
while( (whdr[0].dwFlags&WHDR_INQUEUE) != 0)
  {
  if(snd[TXDA].interrupt_rate < 150)
    {
    lir_sleep(10000);
    }
  else
    {
    lir_sleep(1000);
    }
  }
c=whdr[0].lpData;
memcpy(c,buf,snd[TXDA].block_bytes);
waveOutWrite(hwav_txdaout,whdr,sizeof(WAVEHDR));
txdaout_newbuf_ptr++;
if(txdaout_newbuf_ptr >= (unsigned)snd[TXDA].no_of_blocks)txdaout_newbuf_ptr=0;
daout_px=(daout_px+snd[TXDA].block_bytes)&daout_bufmask;
}

void lir_rx_dawrite(void)
{
char *c;
WAVEHDR *whdr;
if( (ui.use_alsa&PORTAUDIO_RX_OUT) != 0)
  {
// We assume Portaudio is always OK.  
  lir_sched_yield();
  return;
  }
// Wait until there is a buffer ready for us.
whdr=rxdaout_newbuf[rxdaout_newbuf_ptr];
while( (whdr[0].dwFlags&WHDR_INQUEUE) != 0)
  {
  if(snd[RXDA].interrupt_rate < 150)
    {
    lir_sleep(10000);
    }
  else
    {
    lir_sleep(1000);
    }
  }
c=whdr[0].lpData;
memcpy(c,&daout[daout_px],snd[RXDA].block_bytes);
waveOutWrite(hwav_rxdaout,whdr,sizeof(WAVEHDR));
rxdaout_newbuf_ptr++;
if(rxdaout_newbuf_ptr >= (unsigned)snd[RXDA].no_of_blocks)rxdaout_newbuf_ptr=0;
daout_px=(daout_px+snd[RXDA].block_bytes)&daout_bufmask;
}

void close_rx_sndin(void)
{
if(ui.use_extio != 0 && ui.extio_type != 4)return;
if( (ui.use_alsa&PORTAUDIO_RX_IN) != 0)
  {
  close_portaudio_rxad();
  return;
  }
if(rxad_mme_open_flag1 == 0)return;
if(ui.rx_addev_no > 255)
  {
  waveInReset(hwav_rxadin2);
  waveInClose(hwav_rxadin2);
  }
waveInReset(hwav_rxadin1);
waveInClose(hwav_rxadin1);
while(rxad_mme_open_flag1!=0||rxad_mme_open_flag2!=0)
  {
  lir_sleep(1000);
  }
free(rx_wave_inhdr);
free(rx_wavein_buf);
CloseHandle(rxin1_bufready);
DeleteCriticalSection(&protect_rxin1);
if(ui.rx_addev_no > 255)
  {
  CloseHandle(rxin2_bufready);
  DeleteCriticalSection(&protect_rxin2);
  }
rxin1_bufready=NULL;
}

int open_rx_sndin(int return_errors)
{
int ch, bits, errcod;
MMRESULT mmrs;
WAVEINCAPS pwic;
int i, dev, devno;
ssize_t ptr;
rxad_status=LIR_OK;
if(rxad_mme_open_flag1 == 1)
  {
  lirerr(869233);
  return 0;
  }
if(ui.use_extio != 0 && ui.extio_type != 4)return 0;  
if( (ui.use_alsa&PORTAUDIO_RX_IN) != 0)
  {
  open_portaudio_rxad();
  return 0;
  }
ch=ui.rx_ad_channels;
if(ui.rx_addev_no > 255)
  {
  devno=2;
  ch/=2;
  }
else
  {
  devno=1;
  }
if( (ui.rx_input_mode&DWORD_INPUT) == 0)
  {
  bits=16;
  }
else
  {
  bits=24;
  if( (ui.use_alsa&RXAD_USE_EXTENSIBLE) == 0)
    {
    lirerr(1157);
    return 0;
    }
  }
if(devno==2)
  {
  dev=(ui.rx_addev_no>>8)-1;
  mmrs=waveInGetDevCaps(dev, &pwic, sizeof(WAVEINCAPS));
  if(mmrs != MMSYSERR_NOERROR)
    {
    lirerr(1420);
    return 0;
    }
  }  
dev=ui.rx_addev_no&255;
mmrs=waveInGetDevCaps(dev, &pwic, sizeof(WAVEINCAPS));
if(mmrs != MMSYSERR_NOERROR)
  {
  lirerr(1420);
  return 0;
  }
fill_wavefmt(ch,bits,(ui.use_alsa&RXAD_USE_EXTENSIBLE),ui.rx_ad_speed);
if(fmt.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
  {
  mmrs=waveInOpen(&hwav_rxadin1, dev, (WAVEFORMATEX*)&fmt,
                           (size_t)mme_callback_rxad1, 0, CALLBACK_FUNCTION);
  }
else
  {
  mmrs=waveInOpen(&hwav_rxadin1, dev, &fmt.Format,
                                    (size_t)mme_callback_rxad1, 0, CALLBACK_FUNCTION);
  }
if(mmrs != MMSYSERR_NOERROR)
  {
  if(return_errors == TRUE) return (int)mmrs;  
  switch (mmrs)
    {
    case MMSYSERR_ALLOCATED:
    lirerr(1192);
    break;

    case MMSYSERR_BADDEVICEID:
    lirerr(1146);
    break;

    case MMSYSERR_NODRIVER:
    lirerr(1147);
    break;

    case MMSYSERR_NOMEM:
    lirerr(1155);
    break;

    case WAVERR_BADFORMAT:
    lirerr(1156);
    break;

    default:
    lirerr(1227);
    break;
    }
  return 0;
  }
rxad_mme_open_flag1=1;
rxin1_bufready=CreateEvent( NULL, FALSE, FALSE, NULL);
if(rxin1_bufready== NULL)
  {
  errcod=1223;
  goto errclose;
  }
InitializeCriticalSection(&protect_rxin1);
if(devno==2)
  {
  dev=(ui.rx_addev_no>>8)-1;
  mmrs=waveInOpen(&hwav_rxadin2, dev, (WAVEFORMATEX*)&fmt,
                                     (size_t)mme_callback_rxad2, 0, CALLBACK_FUNCTION);
  if(mmrs != MMSYSERR_NOERROR)
    {
    if(return_errors == TRUE) return (int)mmrs;  
    errcod=1228;
    goto errclose;
    }
  rxad_mme_open_flag2=1;
  rxin2_bufready=CreateEvent( NULL, FALSE, FALSE, NULL);
  if(rxin2_bufready== NULL)
    {
    errcod=1223;
    goto errclose;
    }
  InitializeCriticalSection(&protect_rxin2);
  }
// We read ui.rx_ad_speed*snd[RXAD].framesize bytes per second from hardware.
// The callback routine will give us the data in a block size of
// snd[RXAD].block_bytes/devno.
// Make the number of buffers large enough to hold 1 second of data.
snd[RXAD].no_of_blocks=1.F*(float)(ui.rx_ad_speed*snd[RXAD].framesize)/snd[RXAD].block_bytes;
if(snd[RXAD].no_of_blocks<12)snd[RXAD].no_of_blocks=12;
rx_wavein_buf=malloc(snd[RXAD].no_of_blocks*snd[RXAD].block_bytes+fmt.Format.nBlockAlign);
if(rx_wavein_buf == NULL)
  {
  errcod=1236;
  goto errclose;
  }
snd[RXAD].tot_bytes=snd[RXAD].no_of_blocks*snd[RXAD].block_bytes;
snd[RXAD].tot_frames=snd[RXAD].tot_bytes/snd[RXAD].framesize;
rx_wave_inhdr=malloc(devno*snd[RXAD].no_of_blocks*(sizeof(WAVEHDR)+sizeof(size_t)));
if(rx_wave_inhdr == NULL)
  {
  errcod=1236;
  goto errfree_inbuf;
  }
rxadin1_newbuf=(size_t*)&rx_wave_inhdr[devno*snd[RXAD].no_of_blocks];
rxadin2_newbuf=&rxadin1_newbuf[snd[RXAD].no_of_blocks];
rxadin1_newbuf_ptr=0;
rxadin2_newbuf_ptr=0;
for(i=0; i<snd[RXAD].no_of_blocks; i++)
  {
  if(devno==1)
    {
    ptr=(ssize_t)rx_wavein_buf+(ssize_t)(i*snd[RXAD].block_bytes)+
                                                    fmt.Format.nBlockAlign;
    ptr&=(-1-fmt.Format.nBlockAlign);
    rx_wave_inhdr[i].lpData=(char*)(ptr);
    rx_wave_inhdr[i].dwBufferLength=snd[RXAD].block_bytes;
    rx_wave_inhdr[i].dwBytesRecorded=0;
    rx_wave_inhdr[i].dwUser=0;
    rx_wave_inhdr[i].dwFlags=0;
    rx_wave_inhdr[i].dwLoops=0;
    rx_wave_inhdr[i].lpNext=NULL;
    rx_wave_inhdr[i].reserved=0;
    mmrs=waveInPrepareHeader(hwav_rxadin1, &rx_wave_inhdr[i], sizeof(WAVEHDR));
    if(mmrs != MMSYSERR_NOERROR)
      {
      errcod=1245;
      goto errfree;
      }
    mmrs=waveInAddBuffer(hwav_rxadin1,&rx_wave_inhdr[i], sizeof(WAVEHDR));
    if(mmrs != MMSYSERR_NOERROR)
      {
      errcod=1247;
      goto errfree;
      }
    }
  else
    {
    ptr=(ssize_t)(rx_wavein_buf+(2*i  )*(snd[RXAD].block_bytes>>1)+fmt.Format.nBlockAlign);
    ptr&=(-1-fmt.Format.nBlockAlign);
    rx_wave_inhdr[2*i  ].lpData=(char*)(ptr);
    rx_wave_inhdr[2*i  ].dwBufferLength=snd[RXAD].block_bytes>>1;
    rx_wave_inhdr[2*i  ].dwBytesRecorded=0;
    rx_wave_inhdr[2*i  ].dwUser=0;
    rx_wave_inhdr[2*i  ].dwFlags=0;
    rx_wave_inhdr[2*i  ].dwLoops=0;
    rx_wave_inhdr[2*i  ].lpNext=NULL;
    rx_wave_inhdr[2*i  ].reserved=0;
    mmrs=waveInPrepareHeader(hwav_rxadin1, &rx_wave_inhdr[2*i],
                                                          sizeof(WAVEHDR));
    if(mmrs != MMSYSERR_NOERROR)
      {
      errcod=1245;
      goto errfree;
      }
    mmrs=waveInAddBuffer(hwav_rxadin1,&rx_wave_inhdr[2*i  ], sizeof(WAVEHDR));
    if(mmrs != MMSYSERR_NOERROR)
      {
      errcod=1247;
      goto errfree;
      }
    ptr=(ssize_t)(rx_wavein_buf+(2*i+1)*(snd[RXAD].block_bytes>>1)+fmt.Format.nBlockAlign);
    ptr&=(-1-fmt.Format.nBlockAlign);
    rx_wave_inhdr[2*i+1].lpData=(char*)(ptr);
    rx_wave_inhdr[2*i+1].dwBufferLength=snd[RXAD].block_bytes>>1;
    rx_wave_inhdr[2*i+1].dwBytesRecorded=0;
    rx_wave_inhdr[2*i+1].dwUser=0;
    rx_wave_inhdr[2*i+1].dwFlags=0;
    rx_wave_inhdr[2*i+1].dwLoops=0;
    rx_wave_inhdr[2*i+1].lpNext=NULL;
    rx_wave_inhdr[2*i+1].reserved=0;
    mmrs=waveInPrepareHeader(hwav_rxadin2, &rx_wave_inhdr[2*i+1],
                                                            sizeof(WAVEHDR));
    if(mmrs != MMSYSERR_NOERROR)
      {
      errcod=1245;
      goto errfree;
      }
    mmrs=waveInAddBuffer(hwav_rxadin2,&rx_wave_inhdr[2*i+1], sizeof(WAVEHDR));
    if(mmrs != MMSYSERR_NOERROR)
      {
      errcod=1247;
      goto errfree;
      }
    }
  }
mmrs=waveInStart(hwav_rxadin1);
if(mmrs != MMSYSERR_NOERROR)
  {
  errcod=1426;
  goto errfree;
  }
if(devno==2)
  {
  mmrs=waveInStart(hwav_rxadin2);
  if(mmrs != MMSYSERR_NOERROR)
    {
    waveInReset(hwav_rxadin1);
    errcod=1427;
    goto errfree;
    }
  }
return mmrs;
errfree:;
free(rx_wave_inhdr);
errfree_inbuf:;
free(rx_wavein_buf);
errclose:;
if(devno==2)
  {
  waveInClose(hwav_rxadin2);
  }
waveInClose(hwav_rxadin1);
lirerr(errcod);
return mmrs;
}


void set_rx_io(void)
{
char s[256];
char s1[512];
char *tmptxt;
char *tmptxt1;
int latency, dword_bytes;
char *ss;
int i, j, k, bits, dev, line;
int n1, n2, m1, m2;
unsigned int device_id, no_of_addev, no_of_dadev;
MMRESULT mmrs;
WAVEINCAPS pwic;
WAVEOUTCAPS pwoc;
sndlog = fopen(rx_logfile_name, "w");
if(sndlog == NULL)
  {
  lirerr(1016);
  goto set_rx_io_errexit;
  }
write_log=TRUE;
if(ui.use_alsa & 
  (PORTAUDIO_RX_IN+PORTAUDIO_RX_OUT+PORTAUDIO_TX_IN+PORTAUDIO_TX_OUT))
  {
  i=1;
  if(portaudio_active_flag==FALSE)i=portaudio_start(200);
  lir_sched_yield();
  if(kill_all_flag)goto set_rx_io_errexit;
  if(!i)
    {
    clear_screen();
    lir_text(5,5,"Portaudio is selected but can not start.");
    lir_text(5,6,"Maybe a missing dll file?");
    lir_text(5,7,"Try to re-select RX input or press ESC");
    lir_text(10,10,press_any_key);
    await_processed_keyboard();
    if(kill_all_flag)goto set_rx_io_errexit;
    ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
    ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
    ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
    ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
    ui.use_alsa&=~(PORTAUDIO_RX_IN+PORTAUDIO_RX_OUT+
                                        PORTAUDIO_TX_IN+PORTAUDIO_TX_OUT);
    ui.use_alsa&=~PORTAUDIO_DLL_VERSION;
    }
  }    
if(ui.rx_addev_no != DISABLED_DEVICE_CODE)
  {
  if(ui.rx_ad_channels == 0 && ui.rx_dadev_no != DISABLED_DEVICE_CODE)
    {
    ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
    }
  if(ui.rx_max_da_channels == 0)
    {
    ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
    }
  }  
begin_rx_set_io:;
latency=0;
// ******************************************************************
// Show the current RX settings on the screen and do some validation
// ******************************************************************
clear_screen();
line=0;
settextcolor(14);
lir_text(2,line,"CURRENT A/D and D/A SETUP FOR RX");
if(ui.use_extio != 0)
  {
  get_extio_name(&s[0]);
  lir_text(35,line,"using");
  lir_text(41,line,s);
  if(ui.extio_type == 4)
    {
    lir_text(42+strlen(s),line,"with soundcard input.");
    }
  else
    {
    if(ui.extio_type >= 1 && ui.extio_type <= 8)
      {
      goto display_rx_out;
      }
    else
      {
      lirerr(1237);
      goto set_rx_io_errexit;
      }  
    }        
  }  
line+=2;
settextcolor(7);
if( show_rx_input_settings(&line) )goto display_rx_out;
// check for input soundcards
if(ui.rx_addev_no == UNDEFINED_DEVICE_CODE)
  {
  settextcolor(12);
  sprintf(s,"NOT YET SELECTED (Select Menu Option 'A')");
  lir_text(24,line,s);
  SNDLOG"\n%s",s);
  goto display_rx_out;
  }
dword_bytes=4;  
if ( (ui.use_alsa&PORTAUDIO_RX_IN) != 0)
  {
  int err;
  char pa_device_name[128];
  char pa_device_hostapi  [128];
  double pa_device_max_speed;
  double pa_device_min_speed;
  int pa_device_max_bytes;
  int pa_device_min_bytes;
  int pa_device_max_channels;
  int pa_device_min_channels;
  asio_flag=FALSE;
  if(portaudio_active_flag==FALSE)portaudio_start(200);
  err=pa_get_device_info (ui.rx_addev_no,
                          RXAD,
                          &pa_device_name,
                          &pa_device_hostapi,
			  &pa_device_max_speed,
			  &pa_device_min_speed,
			  &pa_device_max_bytes,
			  &pa_device_min_bytes,
			  &pa_device_max_channels,
			  &pa_device_min_channels );
  if (err >= 0)
    {
    sprintf(s,"SOUNDCARD device   = %s ", pa_device_name);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    line+=1;
    sprintf(s,"device number      = %d", ui.rx_addev_no);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    line+=1;
    sprintf(s,"hostapi            = Portaudio (%s)", pa_device_hostapi);
    if(strcmp( pa_device_hostapi,"ASIO")==0)asio_flag=TRUE;
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    latency=ui.rx_ad_latency;
    }
  else
    {
    sprintf (s,"PORTAUDIO device number %d : INVALID DEVICE ",ui.rx_addev_no );
    settextcolor(12);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
    goto  set_rx_io_ask;
    }
  }
else
if (ui.rx_addev_no > 255)
  {
  device_id=(ui.rx_addev_no&255);
  mmrs=waveInGetDevCaps(device_id, &pwic, sizeof(WAVEINCAPS));
  if(mmrs==MMSYSERR_NOERROR)
    {
    settextcolor(10);
    sprintf(s,"SOUNDCARD device 1 = %s ", pwic.szPname);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    line+=1;
    sprintf(s,"device number      = %d", device_id);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    line+=1;
    }
  else
    {
    sprintf(s,"SOUNDCARD device number %d : INVALID DEVICE ", device_id);
    settextcolor(12);
    lir_text(24,line,s);
    SNDLOG"\n%s\n",s);
    line+=2;
    ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
    goto display_rx_out;
    }
  device_id=(ui.rx_addev_no>>8)-1;
  mmrs=waveInGetDevCaps(device_id, &pwic, sizeof(WAVEINCAPS));
  if(mmrs==MMSYSERR_NOERROR)
    {
    settextcolor(10);
    sprintf(s,"SOUNDCARD device 2 = %s ", pwic.szPname);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    line+=1;
    sprintf(s,"device number      = %d", device_id);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    }
  else
    {
    sprintf(s,"SOUNDCARD device number %d : INVALID DEVICE ", device_id);
    settextcolor(12);
    lir_text(24,line,s);
    SNDLOG"\n%s\n",s);
    ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
    line+=2;
    goto display_rx_out;
    }
  }
else
  {
  device_id=ui.rx_addev_no;
  mmrs=waveInGetDevCaps(device_id, &pwic, sizeof(WAVEINCAPS));
  if(mmrs==MMSYSERR_NOERROR)
    {
    settextcolor(10);
    sprintf(s,"SOUNDCARD device   = %s ", pwic.szPname);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    line+=1;
    sprintf(s,"device number      = %d", device_id);
    sprintf(&s[strlen(s)],", native MME");
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    dword_bytes=3;  
    }
  else
    {
    sprintf(s,"SOUNDCARD device number %d : INVALID DEVICE ", device_id);
    settextcolor(12);
    lir_text(24,line,s);
    SNDLOG"\n%s\n",s);
    ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
    line+=2;
    goto display_rx_out;
    }
  }
line++;
sprintf(s,"associated radio   = %s",rx_soundcard_radio_names[ui.rx_soundcard_radio]);    
settextcolor(7);
if(ui.rx_soundcard_radio == RX_SOUNDCARD_RADIO_WSE)
  {
  sprintf(&s[strlen(s)],"  Port=%d Pin=%d",wse.parport,wse.parport_pin);
  if (wse.parport == USB2LPT16_PORT_NUMBER)
    {
    sprintf(&s[strlen(s)],"  USB2LPT 1.6 SELECTED");
    }
  }
lir_text(24,line,s);
line++;
sprintf(s,"sample rate        = %i" ,ui.rx_ad_speed);
if(latency)sprintf(&s[strlen(s)],", Latency %d",latency);
lir_text(24,line,s);
SNDLOG"\n%s",s);
line++;
sprintf(s,"no of input bytes  = 2 (16 bits)");
if( (ui.rx_input_mode&DWORD_INPUT) != 0)
  {
  if(dword_bytes == 3)
    {  
    sprintf(&s[21],"3 (24 bits)");
    }
  else
    {  
    sprintf(&s[21],"4 (32 bits)");
    }
  }
lir_text(24,line,s);
SNDLOG"\n%s",s);
line++;
tmptxt=" ";
tmptxt1=" ";
if( ((ui.rx_input_mode&TWO_CHANNELS) == 0)&&((ui.rx_input_mode&IQ_DATA) == 0))
  {
  tmptxt="radio interface    = One Rx channel, one audio channels ";
  tmptxt1="                     (normal audio)";
  }
if( ((ui.rx_input_mode&TWO_CHANNELS) == 0)&&((ui.rx_input_mode&IQ_DATA) != 0))
  {
  tmptxt="radio interface    = One Rx channel, two audio channels ";
  sprintf(s,"                     (direct conversion, time shift=%d)",
                      ui.sample_shift );
  tmptxt1=s;
  }
if( ((ui.rx_input_mode&TWO_CHANNELS) != 0)&&((ui.rx_input_mode&IQ_DATA) != 0))
  {
  tmptxt="radio interface    = Two Rx channels, four audio channels ";
  tmptxt1="                     (direct conversion, adaptive polarization)";
  }
if( ((ui.rx_input_mode&TWO_CHANNELS) != 0)&&((ui.rx_input_mode&IQ_DATA) == 0))
  {
  tmptxt="radio interface    = Two Rx channels, two audio channels ";
  tmptxt1="                     (normal audio, adaptive polarization)";
  }
lir_text(24,line,tmptxt);
SNDLOG"\n%s",tmptxt);
line+=1;
lir_text(24,line,tmptxt1);
SNDLOG"\n%s\n",tmptxt1);
// ************************************************************************
// Show the current RX OUTPUT settings on the screen and do some validation
// ************************************************************************
display_rx_out:;
latency=0;
settextcolor(7);
line+=2;
sprintf(s,"Linrad RX output to:");
lir_text(2,line,s);
SNDLOG"\nSETUP FOR RX OUTPUT IS NOW:\n");
settextcolor(10);
//check if network output mode
s[0]='\0';
if((ui.network_flag & NET_RXOUT_RAW16) != 0)  strcat ( s,"RAW16 ");
if((ui.network_flag & NET_RXOUT_RAW18) != 0)  strcat ( s,"RAW18 ");
if((ui.network_flag & NET_RXOUT_RAW24) != 0)  strcat ( s,"RAW24 ");
if((ui.network_flag & NET_RXOUT_FFT1)  != 0)  strcat ( s,"FFT1 ");
if((ui.network_flag & NET_RXOUT_TIMF2) != 0)  strcat ( s,"TIMF2 ");
if((ui.network_flag & NET_RXOUT_FFT2)  != 0)  strcat ( s,"FFT2 ");
if((ui.network_flag & NET_RXOUT_BASEB)  != 0)  strcat ( s,"BASEB16 ");
if((ui.network_flag & NET_RXOUT_BASEBRAW) != 0)  strcat ( s,"BASEB24 ");
if (strlen(s) != 0)
  {
  sprintf(s1,"NETWORK");
  lir_text(24,line,s1);
  SNDLOG"\n%s",s1);
  settextcolor(7);
  line+=1;
  sprintf(s1,"send format        = %s",s);
  lir_text(24,line,s1);
  SNDLOG"\n%s\n",s1);
  line+=2;
  settextcolor(10);
//disable output soundcard if not yet selected
  if(ui.rx_dadev_no == UNDEFINED_DEVICE_CODE)  
    {
    ui.rx_dadev_no=DISABLED_DEVICE_CODE;
    }
  }
//check for output soundcards
if(ui.rx_dadev_no < 0)
  {
  if(ui.rx_dadev_no == UNDEFINED_DEVICE_CODE)
    {
    settextcolor(12);
    sprintf(s,"NOT YET SELECTED:  ");
    if(ui.rx_addev_no == UNDEFINED_DEVICE_CODE)
      {
      sprintf(&s[strlen(s)],"(Select input first.)");
      } 
    else
      { 
      sprintf(&s[strlen(s)]," (Select Menu Option 'B')");
      }
    lir_text(24,line,s);
    SNDLOG"\n%s\n",s);
    goto set_rx_io_ask;
    }
  else
    {
    ui.rx_dadev_no=DISABLED_DEVICE_CODE;
    sprintf(s,"DISABLED");
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    goto set_rx_io_ask;
    }
  }
else
  {
  if ( (ui.use_alsa&PORTAUDIO_RX_OUT) != 0)
    {
    int err;
    char pa_device_name[128];
    char pa_device_hostapi  [128];
    double pa_device_max_speed;
    double pa_device_min_speed;
    int pa_device_max_bytes;
    int pa_device_min_bytes;
    int pa_device_max_channels;
    int pa_device_min_channels;
    err=pa_get_device_info (ui.rx_dadev_no,
                          RXDA,
                          &pa_device_name,
                          &pa_device_hostapi,
			  &pa_device_max_speed,
			  &pa_device_min_speed,
			  &pa_device_max_bytes,
			  &pa_device_min_bytes,
			  &pa_device_max_channels,
			  &pa_device_min_channels );
    if (err >= 0)
      {
      sprintf(s,"SOUNDCARD device   = %s ", pa_device_name);
      lir_text(24,line,s);
      SNDLOG"\n%s",s);
      line+=1;
      sprintf(s,"device number      = %d", ui.rx_dadev_no);
      lir_text(24,line,s);
      SNDLOG"\n%s",s);
      line+=1;
      sprintf(s,"hostapi            = Portaudio (%s)", pa_device_hostapi);
      lir_text(24,line,s);
      SNDLOG"\n%s",s);
      ui.rx_min_da_speed=pa_device_min_speed;
      ui.rx_max_da_speed=pa_device_max_speed;
      ui.rx_min_da_bytes=pa_device_min_bytes;
      ui.rx_max_da_bytes=pa_device_max_bytes;
      ui.rx_min_da_channels=pa_device_min_channels;
      ui.rx_max_da_channels=pa_device_max_channels;
      latency=ui.rx_da_latency;
      }
    else
      {
      sprintf (s,"PORTAUDIO device number %d : INVALID DEVICE ",ui.rx_dadev_no );
      settextcolor(12);
      lir_text(24,line,s);
      SNDLOG"\n%s",s);
      ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
      goto  set_rx_io_ask;
      }
    }
  else
    {
    device_id=ui.rx_dadev_no;
    mmrs=waveOutGetDevCaps(device_id, &pwoc, sizeof(WAVEOUTCAPS));
    if(mmrs==MMSYSERR_NOERROR)
      {
      sprintf(s,"SOUNDCARD device   = %s ", pwoc.szPname);
      lir_text(24,line,s);
      SNDLOG"\n%s",s);
      line+=1;
      sprintf(s,"device number      = %d", device_id);
      sprintf(&s[strlen(s)],", native MME");
      lir_text(24,line,s);
      SNDLOG"\n%s",s);
      }
    else
      {
      sprintf(s,"SOUNDCARD device number %d : INVALID DEVICE ", device_id);
      settextcolor(12);
      lir_text(24,line,s);
      ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
      SNDLOG"\n%s\n",s);
      line+=2;
      goto  set_rx_io_ask;
      }
    }
  settextcolor(7);
  line++;
  sprintf(s,"D/A sample rate    = %i to %i",
                                    ui.rx_min_da_speed, ui.rx_max_da_speed);
  if(latency)sprintf(&s[strlen(s)],", latency factor %d",latency);
  lir_text(24,line,s);
  SNDLOG"\n%s",s);
  line++;
  sprintf(s,"D/A bytes          = %i or %i",
                                     ui.rx_min_da_bytes, ui.rx_max_da_bytes);
  lir_text(24,line,s);
  SNDLOG"\n%s",s);
  line++;
  sprintf(s,"D/A channels       = %i or %i",
                               ui.rx_min_da_channels, ui.rx_max_da_channels);
  lir_text(24,line,s);
  SNDLOG"\n%s",s);
  }
set_rx_io_ask:;
line+=2;
if(ui.converter_mode != 0)
  {
  compute_converter_parameters();
  settextcolor(10);
  line+=2;
  if( (ui.converter_mode&CONVERTER_UP) !=0 )
    {
    lir_text(2,line,"UP");
    ss="IF";
    j=4;
    }
  else
    {
    lir_text(2,line,"DOWN");
    j=6;
    ss="signal";
    }   
  lir_text(j,line,"CONVERTER:");
  settextcolor(7);
  sprintf(s,"LO frequency %.6f",converter_offset_mhz);
  i=0;
  while(s[i] != 0)i++;
  while(s[i-1] == '0')i--;
  if(s[i-1] == '.')i--;
  sprintf(&s[i]," MHz.  LO is ");
  if( (ui.converter_mode&CONVERTER_LO_BELOW) != 0)
    {
    strcat(s,"below ");
    }
  else
    {
    strcat(s,"above ");
    }
  strcat(s,ss);
  strcat(s," frequency.");
  lir_text(j+11,line,s);    
  }
line+=2;
settextcolor(7);
sprintf(s,"DMA rate   min=%i    max=%i" ,ui.min_dma_rate, ui.max_dma_rate);
lir_text(2,line,s);
SNDLOG"\n%s\n",s);
// ***************************************************************************
// Ask whether the user wants to change settings and act accordingly:
// ***************************************************************************
line+=2;
lir_text(2,line,"A = Change input settings and reset all other soundcard settings");
line++;
lir_text(7,line,"if a soundcard is selected as input.");
line++;
lir_text(2,line,"B = Change the output soundcard settings.");
line++;
if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
  {
  lir_text(2,line,"C = Change min/max dma rate.");
  line++;
  }
if(ui.rx_soundcard_radio != RX_SOUNDCARD_RADIO_UNDEFINED &&
   ui.rx_soundcard_radio != RX_SOUNDCARD_RADIO_UNDEFINED_REVERSED)
  {
  lir_text(2,line,"D = Set parameters for soundcard radio hardware.");
  line++;
  }
if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
  {
  lir_text(2,line,"E = Enable/Disable frequency converter and set LO.");
  line++;
  }
lir_text(2,line,"Z = Disable the output soundcard.");
line++;
check_line(&line);
lir_text(2,line,"X = To main menu.");
await_processed_keyboard();
if(kill_all_flag)goto set_rx_io_errexit;
clear_screen();
switch(lir_inkey)
  {
  case 'A':
// ************************** CHANGE INPUT SETTINGS ************************************ //
  SNDLOG"\nUSER STARTED THE SELECTION OF A NEW INPUT DEVICE \n");
  ui.sample_shift=0;
  if(ui.use_extio != 0)
    {
    ui.use_extio=0;
    }
  ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
  clear_screen();
  line=2;
  lir_text(10,line,"SELECT HARDWARE FOR RX");
  line+=2;
  lir_text(4,line,"A = Soundcard");
  line++;
  lir_text(4,line,"B = SDR-14 or SDR-IQ");
  line++;
  lir_text(4,line,"C = Perseus");
  line++;
  lir_text(4,line,"D = SDR-IP");
  line++;
  lir_text(4,line,"E = Excalibur");
  line++;
  lir_text(4,line,"F = libExtIO hardwares");
  line++;
  lir_text(4,line,"H = RTL2832 USB dongle");
  line++;
  lir_text(4,line,"I = SDRplay or Mirics USB dongle");
  line++;
  lir_text(4,line,"J = bladeRF");
  line++;
  lir_text(4,line,"K = PCIe 9842");
  line++;
  lir_text(4,line,"L = OpenHPSDR");
  line++;
  lir_text(4,line,"M = Afedri-NET");
  line++;
  lir_text(4,line,"O = Airspy");
  line++;
  lir_text(4,line,"P = CloudIQ");
  line++;
  lir_text(4,line,"Q = Airspy HF+");
  line++;
  lir_text(4,line,"S = SDRplay ver2 (RSP1a/RSPII/RSPduo)");
  line++;
  lir_text(4,line,"T = SDRplay ver3 (RSP1/RSP1A/RSP2/RSPduo/RSPdx)");
  line++;
  if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
    {
    lir_text(4,line,"Y = Network");
    line++;
    }
  lir_text(4,line,"Z = Disable (Disk input allowed)");
  await_processed_keyboard();
  if(kill_all_flag)goto set_rx_io_errexit;
  clear_screen();
  ui.use_extio=0;
  ui.rx_soundcard_radio=0;
// reset the RX output soundcard and the tx soundcards.
  ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
  ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
  ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
  ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
  ui.use_alsa&=~(PORTAUDIO_RX_IN+PORTAUDIO_RX_OUT+
                                        PORTAUDIO_TX_IN+PORTAUDIO_TX_OUT);
  switch (lir_inkey)
    {
    case 'A':
use_rx_sndcard:;
    clear_screen();
// Find out how many audio input devices there are on this computer.
// Allow the user to select one or two for the rx input.
    lir_text(10,5,"Use Portaudio for rx input? (Y/N) =>");
    await_processed_keyboard();
    if(kill_all_flag)goto set_rx_io_errexit;
    if(lir_inkey == 'Y')
      { 
      if( ((ui.use_alsa&PORTAUDIO_DLL_VERSION)>>5) == 0)
        {
        i=select_portaudio_dll();
        if(kill_all_flag)goto set_rx_io_errexit;
        if(i<0)
          {
          ui.use_alsa&=~PORTAUDIO_DLL_VERSION;
          break;
          }
        ui.use_alsa|=(i+1)<<5;
        }
      ui.use_alsa|=PORTAUDIO_RX_IN;
      }
    else
      {
      if(lir_inkey != 'N')
        {
        goto use_rx_sndcard;
        }
      }
    i=1;
    if(portaudio_active_flag==FALSE)i=portaudio_start(201);
    if(kill_all_flag)goto set_rx_io_errexit;
    if(!i)goto use_rx_sndcard;
//get a 'portaudio' selection list
    clear_screen();
    if( (ui.use_alsa&PORTAUDIO_RX_IN) != 0)
      {
      asio_flag=FALSE;
      set_portaudio(RXAD, &line);
      if(kill_all_flag)goto set_rx_io_errexit;
// Allocate a big buffer. The open routine might change the block size.
      timf1_bytes=16*snd[RXAD].block_frames;
      timf1_bytemask=timf1_bytes-1;
      timf1_char=malloc((size_t)timf1_bytes);
      if(timf1_char==NULL)
        {
        lirerr(23628);
        goto set_rx_io_errexit;
        }
      open_rx_sndin(TRUE);
      snd[RXAD].open_flag=CALLBACK_CMD_ACTIVE;
      if(rxad_status != LIR_OK)
        {
        line+=3;
        settextcolor(12);
        lir_text(0,line,
          "Drive routine: Open failed unexpectedly");
        line++;
        lir_text(0,line,"Try another driver for INPUT.");
        if(kill_all_flag)goto set_rx_io_errexit;
        settextcolor(7);
        line+=2;
        lir_text(15,line,press_any_key);
        await_processed_keyboard();
        clear_screen();
        goto begin_rx_set_io;
        }
      close_rx_sndin();
      free(timf1_char);
      goto set_rxin_exit;
      }
get_dev2_q:;
    settextcolor(14);
    lir_text(10,0,"Select SOUNDCARD device for RX input from list ");
    settextcolor(7);
    no_of_addev=waveInGetNumDevs();
    if(no_of_addev == 0)
      {
      lir_text(10,2,"No soundcard detected.");
      lir_text(10,3,press_any_key);
      await_keyboard();
      ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
      break;
      }
    line=2;
    k=0;
    if(no_of_addev > MAX_WAV_DEVICES)no_of_addev=MAX_WAV_DEVICES;
    for(device_id=0; device_id<no_of_addev; device_id++)
      {
      mmrs=waveInGetDevCaps(device_id, &pwic, sizeof(WAVEINCAPS));
      if(mmrs==MMSYSERR_NOERROR)
        {
        sprintf(s,"%d  %s", device_id, pwic.szPname);
        settextcolor(7);
        k++;
        }
      else
        {
        sprintf(s,"%d ERROR (mmrs=%d) %s", device_id, mmrs, pwic.szPname);
        settextcolor(3);
        }
      lir_text(10,line,s);
      line++;
      if(line >= screen_last_line)screenerr(&line);
      }
    line+=2;
    if(line >= screen_last_line)screenerr(&line);
    settextcolor(7);
    if(k == 0)
      {
      lir_text(10,line,"No usable input device found.");
      line+=2;
      if(line >= screen_last_line)screenerr(&line);
      lir_text(14,line,press_any_key);
      await_keyboard();
      ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
      break;
      }
    if(no_of_addev == 1)
      {
      ui.rx_addev_no=0;
      lir_text(10,line,"Device autoselected.");
      line++;
      }
    else
      {
      if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
        {
        ss="(first)";
        }
      else
        {
        ss="";
        }
      sprintf(s,"Select %s device for Rx input by line number>",ss);
      lir_text(10,line,s);
      ui.rx_addev_no=lir_get_integer(61,line, 2, 0, no_of_addev-1);
      if(kill_all_flag)goto set_rx_io_errexit;
      }
    line++;
    if(line >= screen_last_line)screenerr(&line);
    if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
      {
      if(line >= screen_last_line-1)screenerr(&line);
      lir_text(10,line,
                     "Do you need more channels from the same soundcard ? (Y/N)");
      lir_text(10,line+1,"F1 for info/help");
get_dev2:;
      to_upper_await_keyboard();
      if(lir_inkey == F1_KEY || lir_inkey == '!')
        {
        help_message(339);
        clear_screen();
        goto get_dev2_q;
        }
      if(kill_all_flag)goto set_rx_io_errexit;
      if(lir_inkey == 'Y')
        {
        line++;
        if(line >= screen_last_line)screenerr(&line);
        lir_text(10,line,"Select a second device for Rx input>");
dev2_n:;
        i=lir_get_integer(47,line, 2, 0, no_of_addev-1);
        if(kill_all_flag)goto set_rx_io_errexit;
        if(i==ui.rx_addev_no)goto dev2_n;
        ui.rx_addev_no+=256*(i+1);
        }
      else
        {
        if(lir_inkey != 'N')goto get_dev2;
        }
      }
// Windows does not allow us to query the device for its capabilities.
// Ask the user to specify what he wants.
    line+=2;
    if(line >= screen_last_line)screenerr(&line);
    lir_text(10,line,"Linrad can not query hardware because Windows will report that");
    line++;
    if(line >= screen_last_line)screenerr(&line);
    lir_text(10,line,"everything is possible. Windows will silently resample and provide");
    line++;
    if(line >= screen_last_line)screenerr(&line);
    lir_text(10,line,"data that would be meaningless in an SDR context.");
    line++;
    if(line >= screen_last_line)screenerr(&line);
    lir_text(10,line,"Therefore, make sure you enter data that is compatible with the");
    line++;
    if(line >= screen_last_line)screenerr(&line);
    lir_text(10,line,"native capabilities of your soundcardhardware. (And make sure that");
    line++;
    if(line >= screen_last_line)screenerr(&line);
    lir_text(10,line,"the soundcard really is set to the speed you have selected.)");
    line+=2;
    if(line >= screen_last_line)screenerr(&line);
    if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
      {
get_wavfmt:;
      if(line >= screen_last_line-1)screenerr(&line);
      lir_text(10,line,"Use extended format (WAVEFORMATEXTENSIBLE) ? (Y/N)");
      lir_text(10,line+1,"F1 for info/help");
getfmt:;
      to_upper_await_keyboard();
      if(lir_inkey == F1_KEY || lir_inkey == '!')
        {
        help_message(108);
        clear_screen();
        goto get_wavfmt;
        }
      if(lir_inkey == 'N')
        {
        ui.use_alsa&=~RXAD_USE_EXTENSIBLE;
        }
      else
        {
        if(lir_inkey != 'Y')goto getfmt;
        ui.use_alsa|=RXAD_USE_EXTENSIBLE;
        }
      line++;
      if(line >= screen_last_line)screenerr(&line);
      clear_lines(line,line);
      line++;
      if(line >= screen_last_line)screenerr(&line);
      }
    else
      {
      ui.use_alsa&=~RXAD_USE_EXTENSIBLE;
      }
get_rxinrate:;
    lir_text(10,line,"Sampling speed (Hz)>");
    ui.rx_ad_speed=lir_get_integer(31,line, 6, 5000, 999999);
    if(kill_all_flag)goto set_rx_io_errexit;
    bits=16;
    if((ui.use_alsa&RXAD_USE_EXTENSIBLE) == RXAD_USE_EXTENSIBLE)
      {
get_rxbits:;
      lir_text(40,line,"No of bits (16/24):");
      i=lir_get_integer(60,line, 2, 16, 24);
      if(kill_all_flag)goto set_rx_io_errexit;
      clear_screen();
      if(i==16)
        {
        ui.rx_input_mode=0;
        }
      else
        {
        if(i==24)
          {
          ui.rx_input_mode=DWORD_INPUT;
          bits=24;
          }
        else
          {
          goto get_rxbits;
          }
        }
      }
    else
      {
      ui.rx_input_mode=0;
      }
    clear_screen();
// Check whether the device(s) can do whatever the user asked for.
// Try to open with four channels.
// Try again with two channels on error.
    dev=ui.rx_addev_no&255;
    j=8;
test_channels:;
    j/=2;
    fill_wavefmt(j,bits,(ui.use_alsa&RXAD_USE_EXTENSIBLE),ui.rx_ad_speed);
// Make the buffer 40 milliseconds for this test.
    snd[RXAD].framesize=j*bits/8;
    snd[RXAD].block_bytes=0.04*ui.rx_ad_speed*snd[RXAD].framesize;
    mmrs=waveInOpen(&hwav_rxadin1, dev, (WAVEFORMATEX*)&fmt, 0, 0,
                                     WAVE_FORMAT_QUERY|WAVE_FORMAT_DIRECT);
    if(mmrs != MMSYSERR_NOERROR)
      {
      if(j==4)goto test_channels;
      if(mmrs == MMSYSERR_ALLOCATED)
        {
        lirerr(1218);
        goto set_rx_io_errexit;
        }
      if(mmrs == MMSYSERR_BADDEVICEID)
        {
        lirerr(1219);
        goto set_rx_io_errexit;
        }
      if(mmrs == MMSYSERR_NODRIVER)
        {
        lirerr(1220);
        goto set_rx_io_errexit;
        }
      if(mmrs == MMSYSERR_NOMEM)
        {
        lirerr(1221);
        goto set_rx_io_errexit;
        }
      if(mmrs == WAVERR_BADFORMAT)
        {
        lirerr(1222);
        goto set_rx_io_errexit;
        }
      lirerr(1238);
      goto set_rx_io_errexit;
      }
    if(ui.rx_addev_no>255)
      {
      dev=(ui.rx_addev_no>>8)-1;
      mmrs=waveInOpen(&hwav_rxadin2, dev, (WAVEFORMATEX*)&fmt, 0, 0,
                  WAVE_FORMAT_QUERY|WAVE_FORMAT_DIRECT);
      if(mmrs != MMSYSERR_NOERROR)
        {
        lir_text(0,line+2,
             "ERROR  Format not supported by second device. (Press any key)");
        await_keyboard();
        clear_screen();
        if(kill_all_flag)goto set_rx_io_errexit;
        goto get_rxinrate;
        }
      }
    if(ui.rx_input_mode==DWORD_INPUT)
      {
      if( (ui.use_alsa&RXAD_USE_EXTENSIBLE) == 0)lirerr(5823198);
      fill_wavefmt(j,24,(ui.use_alsa&RXAD_USE_EXTENSIBLE),ui.rx_ad_speed);
      mmrs=waveInOpen(&hwav_rxadin1, dev, (WAVEFORMATEX*)&fmt, 0, 0,
                                     WAVE_FORMAT_QUERY|WAVE_FORMAT_DIRECT);
      if(mmrs != MMSYSERR_NOERROR)
        {
        clear_screen();
        lir_text(5,5,"Can not use 24 bit A/D. Will use 16 bits instead.");
        lir_text(5,7,press_any_key);
        await_keyboard();
        ui.rx_input_mode=0;
        }
      }
    if(ui.rx_addev_no>255 && j<4)j*=2;
// If we have reached this far, the user has given correct info about
// speed, channels and no of bits.
// Ask for the desired processing mode.
sel_radio:;
    clear_screen();
    line=0;
    lir_text(10,line,"Select radio interface>");
    if(j == 2)j=3;
    line+=2;
    for(i=0; i<j; i++)
      {
      sprintf(s,"%d: %s",i+1,audiomode_text[i]);
      lir_text(10,line,s);
      line++;
      }
    lir_text(10,line,"F1 for help/info");
    line+=2;
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
      line+=2;
      if(line >= screen_last_line)screenerr(&line);
      lir_text(0,line,"Number of points to time shift between I and Q? (-4 to +4)");
      ui.sample_shift=lir_get_integer(60,line, 2, -4, 4);
      }
    mmrs=(MMRESULT)open_rx_sndin(TRUE);
    snd[RXAD].open_flag=CALLBACK_CMD_ACTIVE;
    if(mmrs != MMSYSERR_NOERROR)
      {
      line+=3;
      if(line >= screen_last_line-1)screenerr(&line);
      settextcolor(12);
      lir_text(0,line,
        "Drive routine: Query with WAVE_FORMAT_QUERY reports format is OK");
      line++;
      lir_text(0,line,
                  "but actually opening the device fails with MMSYSERR_");
      switch (mmrs)
        {
        case MMSYSERR_ALLOCATED:
        break;

        case MMSYSERR_BADDEVICEID:
        break;

        case MMSYSERR_NODRIVER:
        break;

        case MMSYSERR_NOMEM:
        break;

        case WAVERR_BADFORMAT:
        lir_text(52,line,"BADFORMAT");
        break;

        default:
        break;
        }
      if(kill_all_flag)goto set_rx_io_errexit;
      settextcolor(7);
      line+=2;
      if(line >= screen_last_line)screenerr(&line);
      lir_text(15,line,press_any_key);
      await_processed_keyboard();
      clear_screen();
      goto begin_rx_set_io;
      }
    close_rx_sndin();
set_rxin_exit:;
    clear_screen();
    line=0;
    lir_text(9,line,"Select receiver hardware to use with soundcard.");
    line+=2;
    for(i=0; i<MAX_RX_SOUNDCARD_RADIO; i++)
      {
      sprintf(s,"%d   %s",i,rx_soundcard_radio_names[i]);
      lir_text(5,line,s);
      line++;
      if(line >= screen_last_line)screenerr(&line);
      }
    line++;
    if(line >= screen_last_line)screenerr(&line);
    lir_text(0,line,"Select by line number=>");
    ui.rx_soundcard_radio=lir_get_integer(25,line,1,0,MAX_RX_SOUNDCARD_RADIO);
    break;
    
    case 'B':
    timf1_char=malloc(0x8000);
    if(timf1_char == NULL)
      {
      lirerr(1231231);
      goto set_rx_io_errexit;
      }
    init_sdr14();
    free(timf1_char);
    break;
    
    case 'C':
    init_perseus();
    sdr=-1;
    break;
    
    case 'D':
    init_sdrip();
    break;

    case 'E':
    init_excalibur();
    break;

    case 'F':
    init_extio();
    if(kill_all_flag)goto set_rx_io_errexit;
    if(ui.extio_type == 4)goto use_rx_sndcard;
    break;

    case 'H':
    init_rtl2832();
    break;

    case 'I':
    init_mirics();
    break;

    case 'J':
    init_bladerf();
    break;

    case 'K':
    init_pcie9842();
    break;

    case 'L':
    init_openhpsdr();
    break;

    case 'M':
    init_netafedri();
    break;

    case 'O':
    init_airspy();
    break;

    case 'P':
    init_cloudiq();
    break;

    case 'Q':
    init_airspyhf();
    break;

    case 'S':
      init_sdrplay2();
      break;

    case 'T':
      init_sdrplay3();
      break;

    case 'Y':
    if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
      {
      ui.rx_addev_no=NETWORK_DEVICE_CODE;
      ui.network_flag|=NET_RX_INPUT;
      verify_network(TRUE);
      }
    break;

    case 'Z':
    ui.rx_addev_no=DISABLED_DEVICE_CODE;
    break;
    }
  if(kill_all_flag)goto set_rx_io_errexit;
  if(ui.rx_addev_no == UNDEFINED_DEVICE_CODE)goto begin_rx_set_io;
  break;

  case 'B':
// ************************** CHANGE OUTPUT SETTINGS ************************************ //
  clear_screen();
  line=2;
  j=0;
  if(ui.rx_addev_no == UNDEFINED_DEVICE_CODE ) 
    {
    settextcolor(11);
    line+=2;
    lir_text(2,line,"Option B selected, but RX input must be set first:");
    line++;
    lir_text(2,line,"Hit any key to go back to this menu and select option A");
    settextcolor(7);
    await_processed_keyboard();
    if (kill_all_flag) goto set_rx_io_errexit;
    break;
    }
  SNDLOG"\nUSER STARTED THE SELECTION OF A NEW OUTPUT DEVICE \n");
  clear_screen();
  if((ui.rx_addev_no >=0 && ui.rx_addev_no < SPECIFIC_DEVICE_CODES) &&
                                        (ui.network_flag&NET_RX_INPUT) == 0)
    {
    if(!start_iotest(&line, RXAD))
      {
      if(kill_all_flag)goto set_rx_io_errexit;
      goto set_rxout_exit;
      }
    }
pa_rxda:;
  ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
  ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
  ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
  ui.use_alsa&=~(PORTAUDIO_RX_OUT+PORTAUDIO_TX_IN+PORTAUDIO_TX_OUT);
  lir_text(10,5,"Use Portaudio for rx output? (Y/N) =>");
  await_processed_keyboard();
  if (kill_all_flag) goto set_rx_io_errexit;
  if(lir_inkey == 'Y')
    {
    if( ((ui.use_alsa&PORTAUDIO_DLL_VERSION) >> 5) == 0)
      {
      i=select_portaudio_dll();
      if(kill_all_flag)goto set_rx_io_errexit;
      if(i<0)
        {
        ui.use_alsa&=~PORTAUDIO_DLL_VERSION;
go_pa_rxda:;
        clear_screen();
        goto pa_rxda;
        }
      ui.use_alsa|=(i+1)<<5;
      }
    ui.use_alsa|=PORTAUDIO_RX_OUT;
    }
  else
    {
    if(lir_inkey != 'N')
      {
      goto go_pa_rxda;
      }
    }
  i=1;
  if(portaudio_active_flag==FALSE)i=portaudio_start(202);
  if(kill_all_flag)goto set_rx_io_errexit;
  if(!i)goto go_pa_rxda;
//get a 'portaudio' selection list
  clear_screen();
  if( (ui.use_alsa&PORTAUDIO_RX_OUT) != 0)
    {
    set_portaudio(RXDA, &line);
    goto set_rxout_exit;
    }
//get a 'native MME' selection list
  no_of_dadev=waveOutGetNumDevs();
  if(no_of_dadev == 0)
    {
    lir_text(5,2,"No output device detected");
    if(ui.rx_addev_no < SPECIFIC_DEVICE_CODES)
                                   lir_text(5,28,"while input is open.");
    lir_text(5,3,"PRESS ANY KEY");
    await_keyboard();
    if (kill_all_flag) goto set_rx_io_errexit;
    goto set_rxout_exit;
    }
  settextcolor(14);
  lir_text(10,0,"Select SOUNDCARD device for Rx output");
  settextcolor(7);
  line=2;
  if(no_of_dadev > MAX_WAV_DEVICES)no_of_dadev=MAX_WAV_DEVICES;
  for(device_id=0; device_id<no_of_dadev; device_id++)
    {
    mmrs=waveOutGetDevCaps(device_id, &pwoc, sizeof(WAVEOUTCAPS));
    if(mmrs==MMSYSERR_NOERROR)
      {
      sprintf(s,"%d  %s", device_id, pwoc.szPname);
      lir_text(10,line,s);
      line++;
      if(line >= screen_last_line)screenerr(&line);
      }
    }
  line+=2;
  if(line >= screen_last_line)screenerr(&line);
  if(no_of_dadev == 1)
    {
    ui.rx_dadev_no=0;
    }
  else
    {
    lir_text(10,line,"Select device for Rx output by line number>");
    ui.rx_dadev_no=lir_get_integer(54,line, 2, 0, no_of_dadev-1);
    if(kill_all_flag)goto set_rxout_exit;
    }
  ui.rx_max_da_channels=2;
  ui.rx_min_da_channels=1;
  mmrs=waveOutGetDevCaps(ui.rx_dadev_no, &pwoc, sizeof(WAVEOUTCAPS));
  i=0;
  if( (pwoc.dwFormats &
           (WAVE_FORMAT_1M08|WAVE_FORMAT_2M08|WAVE_FORMAT_4M08|
            WAVE_FORMAT_1S08|WAVE_FORMAT_2S08|WAVE_FORMAT_4S08)) != 0)i=1;
  if( (pwoc.dwFormats &
           (WAVE_FORMAT_1M16|WAVE_FORMAT_2M16|WAVE_FORMAT_4M16|
            WAVE_FORMAT_1S16|WAVE_FORMAT_2S16|WAVE_FORMAT_4S16)) != 0)i|=2;
  if(i==0 || i==3)
    {
    ui.rx_max_da_bytes=2;
    ui.rx_min_da_bytes=1;
    }
  else
    {
    ui.rx_max_da_bytes=i;
    ui.rx_min_da_bytes=i;
    }
  ui.rx_min_da_speed=8000;
  ui.rx_max_da_speed=96000;
set_rxout_exit:;
  if(ui.rx_dadev_no == ui.tx_dadev_no)ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
  if(ui.rx_dadev_no == ui.tx_addev_no)ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
  if((ui.rx_addev_no >=0 && ui.rx_addev_no < SPECIFIC_DEVICE_CODES) &&
                                        (ui.network_flag&NET_RX_INPUT) == 0)
    {
    stop_iotest(RXAD);
    }
  if(kill_all_flag)goto set_rx_io_errexit;
  lir_sleep(10000);
  break; 
  
  case 'C':
// ************************** CHANGE DMA RATE ************************************ //
  if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
    {
    clear_screen();
    line=5;
    SNDLOG"\nUSER STARTED A CHANGE OF MIN/MAX DMA RATE\n");
    sprintf(s,"Current  min, max DMA rates in Hz       : %i, %i ",
                                               ui.min_dma_rate, ui.max_dma_rate);
    lir_text(10,line,s);
    SNDLOG"%s\n",s);
    line+=2;
    if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
      {
      n1=MIN_DMA_RATE_EXP;
      m1=DMA_MIN_DIGITS_EXP;
      n2=MAX_DMA_RATE_EXP;
      m2=DMA_MAX_DIGITS_EXP;
      }
    else
      {
      n1=MIN_DMA_RATE;
      m1=DMA_MIN_DIGITS;
      n2=MAX_DMA_RATE;
      m2=DMA_MAX_DIGITS;
      }
    sprintf(s,"Enter new min DMA rate in Hz (%d-%d) > ",n1,n2);
    SNDLOG"%s\n",s);
    lir_text(10,line,s);
    i=lir_get_integer(45+m1+m2, line, m2,n1,n2);
    if(kill_all_flag)goto set_rx_io_errexit;
    sprintf(s,"User selected  %i",i);
    SNDLOG"%s\n",s);
    ui.min_dma_rate=i;
    line+=4;
    sprintf(s,"%i",i);
    m1=strlen(s);
    sprintf(s,"Enter new max DMA rate in Hz (%d-%d) > ",ui.min_dma_rate,n2);
    SNDLOG"%s\n",s);
    lir_text(10,line,s);
    i=lir_get_integer(43+m1+m2, line, m2, ui.min_dma_rate, n2);
    if(kill_all_flag)goto set_rx_io_errexit;
    sprintf(s,"User selected  %i",i);
    SNDLOG"%s\n",s);
    ui.max_dma_rate=i;
    }
  break;

  case 'D':
  switch (ui.rx_soundcard_radio)
    {
    case RX_SOUNDCARD_RADIO_WSE:
    wse_setup();
    break;
    
    case RX_SOUNDCARD_RADIO_SI570:
    si570_rx_setup();
    break;

    case RX_SOUNDCARD_RADIO_SOFT66:
    soft66_setup();
    break;

    case RX_SOUNDCARD_RADIO_ELEKTOR:
    elektor_setup();
    break;

    case RX_SOUNDCARD_RADIO_FCDPROPLUS:
    fcdproplus_setup();
    break;

    case RX_SOUNDCARD_RADIO_AFEDRI_USB:
    afedriusb_setup();  
    }
  break;  

  case 'E':
  clear_screen();
// *************************  CONVERTER SETUP FOR RX ****************************
  lir_text(5,3,"Do you wish to use a converter in front of your receiver (Y/N)?");
  ss=NULL;
get_convuse:;
  await_processed_keyboard();
  if (kill_all_flag) goto set_rx_io_errexit;
  ui.converter_mode=0;
  if(lir_inkey=='N')
    {
    ui.converter_hz=0;  
    ui.converter_mhz=0;  
    compute_converter_parameters();
    break;
    }
  else
    {
    if(lir_inkey != 'Y')goto get_convuse;
    }
  ui.converter_mode=CONVERTER_USE;  
  lir_text(5,5,"Enter converter LO frequency in MHz:");
  converter_offset_mhz=(float)lir_get_float(42,5,14,0.001F,200000.);
  ui.converter_mhz=(unsigned int)converter_offset_mhz;
  ui.converter_hz=(int)(round(1000000*(converter_offset_mhz-ui.converter_mhz)));
get_updown:;
  lir_text(5,7,"Up or down conversion? (U/D)?");  
  await_processed_keyboard();
  if(kill_all_flag) goto set_rx_io_errexit;
  if(lir_inkey=='U')
    {
    ui.converter_mode|=CONVERTER_UP;  
    ss="IF";
    }
  else
    {
    if(lir_inkey != 'D')goto get_updown;
    ss="signal";
    }
get_losign:;
  sprintf(s,"Is LO below %s frequency (Y/N)?",ss);
  lir_text(5,9,s);
  await_processed_keyboard();
  if(kill_all_flag) goto set_rx_io_errexit;
  if(lir_inkey=='Y')
    {
    ui.converter_mode|=CONVERTER_LO_BELOW;  
    }
  else
    {
    if(lir_inkey != 'N')goto get_losign;
    }
  compute_converter_parameters();
  break;

  case 'Z':
// ************************** DISABLE OUTPUT ************************************ //
  SNDLOG"\nUSER DISABLED THE OUPUT SOUNDCARD \n");
  ui.rx_dadev_no=DISABLED_DEVICE_CODE;
  ui.rx_max_da_bytes=2;
  ui.rx_max_da_channels=2;
  ui.rx_min_da_bytes=1;
  ui.rx_min_da_channels=1;
  ui.rx_min_da_speed=5000;
  ui.rx_max_da_speed=1000000;
  break;

  case 'X':
// ************************** EXIT TO MAIN MENU ********************************** //
//validate input settings
  clear_screen();
  line=0;
  if ((ui.rx_addev_no==UNDEFINED_DEVICE_CODE) && 
                                   ((ui.network_flag&NET_RX_INPUT)==0))
    {
    settextcolor(12);
    line+=2;
    lir_text(2,line,"Invalid input settings: NOT YET SELECTED");
    line+=1;
    lir_text(2,line,"Hit any key to go back to this menu and select option A");
    settextcolor(7);
    await_processed_keyboard();
    if (kill_all_flag) goto set_rx_io_errexit;
    break;
    } 
//validate output settings
  if (ui.rx_dadev_no==UNDEFINED_DEVICE_CODE)
    {
    settextcolor(12);
    line+=2;
    lir_text(2,line,"Invalid output settings: NOT YET SELECTED");
    line+=1;
    lir_text(2,line,"Hit any key to go back to this menu and select option B or Z");
    settextcolor(7);
    await_processed_keyboard();
    if (kill_all_flag)goto set_rx_io_errexit;
    break;
    } 
//reurn to main menu
  line+=2;
  lir_text(2,line,remind_parsave);
  line++;
  lir_text(2,line,press_any_key);
  await_keyboard();
  goto set_rx_io_exit;

  default:
  break;
  }
// ******************************************************************************** //
goto begin_rx_set_io;
set_rx_io_exit:;
SNDLOG"\nNormal end\n");
set_rx_io_errexit:;
if(portaudio_active_flag==TRUE)portaudio_stop(200);
fclose(sndlog);
write_log=FALSE;
return;
}

void open_rx_sndout(void)
{
int errcod;
WAVEFORMATEX wfmt;
MMRESULT mmrs;
WAVEOUTCAPS pwoc;
int i;
ssize_t ptr;
float t1;
if(rx_audio_out == 0)return;
snd[RXDA].framesize=rx_daout_channels*rx_daout_bytes;
t1=min_delay_time;
i=4;
if(diskread_flag >= 2)i=1;
if(t1 < fft3_blocktime/i)t1=fft3_blocktime/i;
if(t1 > 1)t1=1;
snd[RXDA].block_frames=0.75*t1*genparm[DA_OUTPUT_SPEED];
make_power_of_two(&snd[RXDA].block_frames);
// make the minimum block 8 frames
if(snd[RXDA].block_frames < 8)snd[RXDA].block_frames=8;
make_power_of_two(&snd[RXDA].block_frames);
snd[RXDA].interrupt_rate=(float)(genparm[DA_OUTPUT_SPEED])/
                                                     snd[RXDA].block_frames;
while(snd[RXDA].interrupt_rate < ui.min_dma_rate && snd[RXDA].block_frames>8)
  {
  snd[RXDA].interrupt_rate*=2;
  snd[RXDA].block_frames/=2;
  }
while(snd[RXDA].interrupt_rate > ui.max_dma_rate)
  {
  snd[RXDA].interrupt_rate/=2;
  snd[RXDA].block_frames*=2;
  }
DEB"\nRXout interrupt_rate: Desired %.1f",snd[RXDA].interrupt_rate);
// Allocate buffers for minimum 1 second.
snd[RXDA].block_bytes=snd[RXDA].block_frames*snd[RXDA].framesize;
snd[RXDA].no_of_blocks=1.F*snd[RXDA].interrupt_rate;
make_power_of_two(&snd[RXDA].no_of_blocks);
if(snd[RXDA].no_of_blocks < 8)snd[RXDA].no_of_blocks=8;
snd[RXDA].tot_bytes=snd[RXDA].no_of_blocks*snd[RXDA].block_bytes;
snd[RXDA].tot_frames=snd[RXDA].no_of_blocks*snd[RXDA].block_frames;
if(ui.rx_dadev_no == DISABLED_DEVICE_CODE)return;
if( (ui.use_alsa&PORTAUDIO_RX_OUT) != 0)
  {
  open_portaudio_rxda();
  return;
  }
mmrs=waveOutGetDevCaps(ui.rx_dadev_no, &pwoc, sizeof(WAVEOUTCAPS));
if(mmrs != MMSYSERR_NOERROR)
  {
  lirerr(1423);
  return;
  }
wfmt.wFormatTag=WAVE_FORMAT_PCM;
wfmt.nSamplesPerSec=genparm[DA_OUTPUT_SPEED];
wfmt.wBitsPerSample=8*rx_daout_bytes;
wfmt.cbSize=0;
wfmt.nChannels=rx_daout_channels;
wfmt.nBlockAlign=wfmt.nChannels*wfmt.wBitsPerSample/8;
wfmt.nAvgBytesPerSec=wfmt.nBlockAlign*wfmt.nSamplesPerSec;
mmrs=waveOutOpen(&hwav_rxdaout, ui.rx_dadev_no, &wfmt, 
                          (size_t)rxda_mme_callback, 0, CALLBACK_FUNCTION);
if(mmrs != MMSYSERR_NOERROR)
  {
  lirerr(1234);
  return;
  }
errcod=1331;
rx_waveout_buf=malloc(snd[RXDA].no_of_blocks*snd[RXDA].block_bytes+wfmt.nBlockAlign);
if(rx_waveout_buf==NULL)goto wave_close;
rx_wave_outhdr=malloc(snd[RXDA].no_of_blocks*sizeof(WAVEHDR));
if(rx_wave_outhdr==NULL)goto free_waveout;
rxdaout_newbuf=malloc(snd[RXDA].no_of_blocks*sizeof(WAVEHDR));
if(rxdaout_newbuf==NULL)goto free_outhdr;
rxdaout_newbuf_ptr=0;
for(i=0; i<snd[RXDA].no_of_blocks; i++)
  {
  ptr=(ssize_t)(rx_waveout_buf+i*snd[RXDA].block_bytes+wfmt.nBlockAlign);
  ptr&=(-1-wfmt.nBlockAlign);
  rx_wave_outhdr[i].lpData=(char*)(ptr);
  rx_wave_outhdr[i].dwBufferLength=snd[RXDA].block_bytes;
  rx_wave_outhdr[i].dwBytesRecorded=0;
  rx_wave_outhdr[i].dwUser=1;
  rx_wave_outhdr[i].dwFlags=0;
  rx_wave_outhdr[i].dwLoops=0;
  rx_wave_outhdr[i].lpNext=NULL;
  rx_wave_outhdr[i].reserved=0;
  mmrs=waveOutPrepareHeader(hwav_rxdaout, &rx_wave_outhdr[i], sizeof(WAVEHDR));
  rxdaout_newbuf[i]=&rx_wave_outhdr[i];
  if(mmrs != MMSYSERR_NOERROR)goto errfree;
  }
rx_audio_out=0;
return;
errfree:;
free(rxdaout_newbuf);
free_outhdr:;
free(rx_wave_outhdr);
free_waveout:;
free(rx_waveout_buf);
wave_close:;
waveOutClose(hwav_rxdaout);
rx_audio_out=-1;
lirerr(errcod);
}

void set_tx_io(void)
{
char * tmptxt;
unsigned int no_of_dadev, no_of_addev;
char s[256];
int i, j, k, line, bits;
int latency;
int dev_valid;
char *tx_logfile_name="soundboard_tx_init.log";
unsigned int device_id;
MMRESULT mmrs;
WAVEINCAPS pwic;
WAVEOUTCAPS pwoc;
int err;
int soundcard_error_flag;
char pa_device_name[128];
char pa_device_hostapi  [128];
double pa_device_max_speed;
double pa_device_min_speed;
int pa_device_max_bytes;
int pa_device_min_bytes;
int pa_device_max_channels;
int pa_device_min_channels;
clear_screen();
if(((unsigned int)ui.network_flag&NET_RX_INPUT) != 0)
  {
  lir_text(0,0,"Input from the network is currently selected.");
  goto not_now;
  }
if( ((ui.rx_ad_channels == 0 || ui.rx_addev_no == UNDEFINED_DEVICE_CODE)
                               && ui.rx_addev_no != DISABLED_DEVICE_CODE) ||
    ((ui.rx_max_da_channels == 0 || ui.rx_dadev_no == UNDEFINED_DEVICE_CODE)
                               && ui.rx_dadev_no != DISABLED_DEVICE_CODE))
  {
  lir_text(0,0,"The RX input and output must be set up before the TX.");
not_now:;
  lir_text(0,1,"TX is now disabled.");
  ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
  ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
  lir_text(10,3,press_any_key);
  await_processed_keyboard();
  return;
  }
if(ui.tx_ad_channels == 0)ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
if(ui.tx_da_channels == 0)ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
sndlog=fopen(tx_logfile_name, "w");
if(sndlog == NULL)
  {
  lirerr(1016);
  return;
  }
write_log=TRUE;
latency=0;
if(ui.use_alsa & 
  (PORTAUDIO_RX_IN+PORTAUDIO_RX_OUT+PORTAUDIO_TX_IN+PORTAUDIO_TX_OUT))
  {
  i=1;
  if(portaudio_active_flag==FALSE)i=portaudio_start(300);
  if(kill_all_flag)goto set_tx_io_errexit;
  if(!i)
    {
    clear_screen();
    lir_text(5,5,"Portaudio is selected but can not start.");
    lir_text(5,6,"Maybe a missing dll file?");
    lir_text(5,7,"Try to re-select RX input or press ESC");
    lir_text(10,10,press_any_key);
    await_processed_keyboard();
    if(kill_all_flag)goto set_tx_io_errexit;
    ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
    ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
    ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
    ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
    ui.use_alsa&=~(PORTAUDIO_RX_IN+PORTAUDIO_RX_OUT+
                                        PORTAUDIO_TX_IN+PORTAUDIO_TX_OUT);
    ui.use_alsa&=~PORTAUDIO_DLL_VERSION;
    }
  }    
begin_tx_set_io:;
clear_screen();
if(kill_all_flag)goto set_tx_io_errexit;
line=2;
// **************************************************************
// Show the current TX output settings on the screen.
// **************************************************************
lir_text(0,line,"Linrad TX output from:");
dev_valid=FALSE;
settextcolor(10);
latency=0;  
if(ui.tx_dadev_no == UNDEFINED_DEVICE_CODE)
  {
  sprintf(s,"NOT YET SELECTED");
  goto show_txda;
  }
if ( (ui.use_alsa&PORTAUDIO_TX_OUT) != 0)
  {
  err=pa_get_device_info (ui.tx_dadev_no,
                          TXDA,
                          &pa_device_name,
                          &pa_device_hostapi,
			  &pa_device_max_speed,
			  &pa_device_min_speed,
			  &pa_device_max_bytes,
			  &pa_device_min_bytes,
			  &pa_device_max_channels,
			  &pa_device_min_channels );
  if (err >= 0)
    {
    sprintf(s,"SOUNDCARD device   = %s ", pa_device_name);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    line+=1;
    sprintf(s,"device number      = %d", ui.tx_dadev_no);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    line+=1;
    sprintf(s,"hostapi            = Portaudio (%s)", pa_device_hostapi);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    latency=ui.tx_da_latency;
    dev_valid=TRUE;
    }
  else
    {
    sprintf (s,"PORTAUDIO device number %d : INVALID DEVICE ",ui.tx_dadev_no );
    settextcolor(12);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
    line+=2;
    goto  set_tx_io_ask;
    }
  }
else
  {
  device_id=ui.tx_dadev_no;
  mmrs=waveOutGetDevCaps(device_id, &pwoc, sizeof(WAVEOUTCAPS));
  if(mmrs==MMSYSERR_NOERROR)
    {
    sprintf(s,"SOUNDCARD device   = %s ", pwoc.szPname);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    line+=1;
    sprintf(s,"device number      = %d", device_id);
    sprintf(&s[strlen(s)],", native MME");
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    dev_valid=TRUE;
    }
  else
    {
    sprintf(s,"SOUNDCARD device number %d : INVALID DEVICE ", device_id);
    tmptxt=s;
    ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
    SNDLOG"\n%s\n",s);
    }
  }
show_txda:;
lir_text(24,line,s);
settextcolor(7);
line++;
if(dev_valid)
  {
  sprintf(s,"sample rate        = %i",ui.tx_da_speed );
  if(latency)sprintf(&s[strlen(s)],", Latency %d",latency);
  SNDLOG"%s\n",s);
  lir_text(24,line,s);
  line++;
  i=ui.tx_da_bytes;
  if ( (ui.use_alsa&PORTAUDIO_TX_OUT) == 0 && ui.tx_da_bytes == 4)i=3;
  sprintf(s,"No of output bytes = %d (%d bits)", i, 8*i);
  SNDLOG"%s\n",s);
  lir_text(24,line,s);
  line++;
  sprintf(s,"No of output chans = %d", ui.tx_da_channels);
  SNDLOG"%s\n",s);
  lir_text(24,line,s);
  line++;
  sprintf(s,"Associated radio   = %s",tx_soundcard_radio_names[ui.tx_soundcard_radio]);
  if(ui.transceiver_mode)sprintf(&s[strlen(s)],"  TRANSCEIVER MODE.  Si570 freq");
  if(ui.transceiver_mode==1)sprintf(&s[strlen(s)]," fixed.");
  if(ui.transceiver_mode==2)sprintf(&s[strlen(s)]," variable.");
  SNDLOG"%s\n",s);
  lir_text(24,line,s);
  line++;
  if(ui.transceiver_mode==1)
    {
    sprintf(s,"PTT control        =");
    if(ui.ptt_control == 0)sprintf(&s[strlen(s)]," USB."); 
    if(ui.ptt_control == 1)sprintf(&s[strlen(s)]," Pilot tone."); 
    SNDLOG"%s\n",s);
    lir_text(24,line,s);
    line++;
    }
  }
line++;
// **************************************************************
// Show the current TX input settings on the screen.
// **************************************************************
lir_text(0,line,"Linrad TX input to:");
settextcolor(10);
sprintf(s,"UNKNOWN (no=%d)",ui.tx_addev_no);
tmptxt=s;
dev_valid=FALSE;
if(ui.tx_addev_no == UNDEFINED_DEVICE_CODE)
  {
  tmptxt="NOT YET SELECTED";
  goto show_txad1;
  }
if ( (ui.use_alsa&PORTAUDIO_TX_IN) != 0)
  {
  err=pa_get_device_info (ui.tx_addev_no,
                          TXAD,
                          &pa_device_name,
                          &pa_device_hostapi,
			  &pa_device_max_speed,
			  &pa_device_min_speed,
			  &pa_device_max_bytes,
			  &pa_device_min_bytes,
			  &pa_device_max_channels,
			  &pa_device_min_channels );
  if (err >= 0)
    {
    sprintf(s,"SOUNDCARD device   = %s ", pa_device_name);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    line+=1;
    sprintf(s,"device number      = %d", ui.tx_addev_no);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    line+=1;
    sprintf(s,"hostapi            = Portaudio (%s)", pa_device_hostapi);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    latency=ui.tx_ad_latency;
    dev_valid=TRUE;
    tmptxt=NULL;
    }
  else
    {
    sprintf (s,"PORTAUDIO device number %d : INVALID DEVICE ",ui.tx_addev_no );
    tmptxt=s;
    ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
    }
  goto show_txad;
  }
mmrs=waveInGetDevCaps(ui.tx_addev_no, &pwic, sizeof(WAVEINCAPS));
if(mmrs==MMSYSERR_NOERROR)
  {
  settextcolor(10);
  sprintf(s,"SOUNDCARD device   = %s ", pwic.szPname);
  lir_text(24,line,s);
  SNDLOG"\n%s",s);
  line++;
  sprintf(s,"device number      = %d", ui.tx_addev_no);
  sprintf(&s[strlen(s)],", native MME");
  lir_text(24,line,s);
  SNDLOG"\n%s",s);
  tmptxt=NULL;
  dev_valid=TRUE;
  latency=0;
  }
else
  {
  sprintf(s,"SOUNDCARD device number %d : INVALID DEVICE ", ui.tx_addev_no);
  tmptxt=s;
  ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
  }
show_txad:;
line++;
show_txad1:;
if(tmptxt)
  {
  lir_text(24,line,tmptxt);
  line++;
  }
settextcolor(7);
if(dev_valid)
  {
  sprintf(s,"sample rate        = %i",ui.tx_ad_speed);
  if(latency)sprintf(&s[strlen(s)],", Latency %d",latency);
  SNDLOG"%s\n",s);
  lir_text(2,line,s);
  line++;
  sprintf(s,"No of input bytes  = %d (%d bits)",
                ui.tx_ad_bytes, 8*ui.tx_ad_bytes);
  SNDLOG"%s\n",s);
  lir_text(2,line,s);
  line++;
  sprintf(s,"No of input chans  = %d", ui.tx_ad_channels);
  SNDLOG"%s\n",s);
  lir_text(2,line,s);
  }
line+=2;
set_tx_io_ask:;
// **************************************************************
// Ask whether the user wants to change something and act accordingly.
// **************************************************************
settextcolor(7);
lir_text(0,line,"A = Change the output settings.");
line++;
lir_text(0,line,"B = Change the input settings.");
line++;
if(ui.tx_soundcard_radio > TX_SOUNDCARD_RADIO_UNDEFINED_REVERSED)
  {
  lir_text(0,line,"C = Change parameters for soundcard radio.");
  line++;
  }
lir_text(0,line,"X = To main menu.");
//setup_menu:;
await_processed_keyboard();
if(kill_all_flag)goto set_tx_io_errexit;
switch(lir_inkey)
  {
  case 'A':
  ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
  ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
  clear_screen();
  line=2;
  lir_text(10,line,"SELECT HARDWARE FOR TX");
  line+=2;
  lir_text(4,line,"A = Soundcard");
  line++;
  lir_text(4,line,"B = bladeRF");
  line++;
  lir_text(4,line,"C = OpenHPSDR");
  await_processed_keyboard();
  if(kill_all_flag)goto set_tx_io_errexit;
  clear_screen();
  ui.tx_soundcard_radio=0;
  line=2;
  switch (lir_inkey)
    {
    case 'A':
    if(ui.tx_enable != 0)
      {
      if((ui.rx_addev_no >=0 && ui.rx_addev_no < SPECIFIC_DEVICE_CODES) &&
                                        (ui.network_flag&NET_RX_INPUT) == 0)
        {
        if(start_iotest(&line,RXAD) == FALSE)
          {
          if(kill_all_flag)goto set_tx_io_errexit;
          goto set_txout_exit2;
          }
        }
      if(ui.rx_dadev_no >=0 && ui.rx_dadev_no < SPECIFIC_DEVICE_CODES)
        {
        if(start_iotest(&line,RXDA) == FALSE)
          {
          if(kill_all_flag)goto set_tx_io_errexit;
          goto set_txout_exit;
          }
        }
      }  
sel_tx_output:;
    ui.use_alsa&=~(PORTAUDIO_TX_IN+PORTAUDIO_TX_OUT);
    ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
    ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
    lir_text(10,5,"Use Portaudio for tx output? (Y/N) =>");
    lir_text(15,7,press_f1_for_info);
    await_processed_keyboard();
    if(kill_all_flag)goto set_tx_io_errexit;;
    if(lir_inkey == F1_KEY || lir_inkey == '!')
      {
      help_message(343);
go_sel_tx_output:;
      clear_screen();
      goto sel_tx_output;
      }
    if(lir_inkey == 'Y')
      {
      if( ((ui.use_alsa&PORTAUDIO_DLL_VERSION)>>5) == 0)
        {
        i=select_portaudio_dll();
        if(kill_all_flag)goto set_tx_io_errexit;
        if(i<0)
          {
          ui.use_alsa&=~PORTAUDIO_DLL_VERSION;
          goto set_txout_exit;
          }
        ui.use_alsa|=(i+1)<<5;   
        }
      ui.use_alsa|=PORTAUDIO_TX_OUT;
      }
    else
      {
      if(lir_inkey != 'N')
        {
        goto go_sel_tx_output;
        }
      }
    i=1;
    if(portaudio_active_flag==FALSE)i=portaudio_start(301);
    if(kill_all_flag)goto set_tx_io_errexit;   
    if(!i)goto go_sel_tx_output;
    clear_screen();
    line=1;
    if( (ui.use_alsa&PORTAUDIO_TX_OUT) != 0)
      {
      set_portaudio(TXDA, &line);
      goto set_txout_exit;
      }
//get a 'native MME' selection list
    no_of_dadev=waveOutGetNumDevs();
    if(no_of_dadev == 0)
      {
      lir_text(5,2,"No output device detected while RX is running.");
      lir_text(5,3,"PRESS ANY KEY");
      await_keyboard();
      if (kill_all_flag) goto set_txout_exit;
      goto set_txout_exit;
      }
    settextcolor(14);
    lir_text(10,0,"Select SOUNDCARD device for Tx output");
    settextcolor(7);
    line=2;
    if(no_of_dadev > MAX_WAV_DEVICES)no_of_dadev=MAX_WAV_DEVICES;
    for(device_id=0; device_id<no_of_dadev; device_id++)
      {
      mmrs=waveOutGetDevCaps(device_id, &pwoc, sizeof(WAVEOUTCAPS));
      if(mmrs==MMSYSERR_NOERROR)
        {
        sprintf(s,"%d  %s", device_id, pwoc.szPname);
        lir_text(10,line,s);
        line++;
        }
      }
    line+=2;
    if(no_of_dadev == 1)
      {
      ui.tx_dadev_no=0;
      }
    else
      {
      lir_text(10,line,"Select device for Tx output by line number>");
      ui.tx_dadev_no=lir_get_integer(54,line, 2, 0, no_of_dadev-1);
      if(kill_all_flag)goto set_txout_exit;
      line+=2;
      }
// This will set the number of channels in ui.tx_da_channels
    select_txout_format(&line);
    line ++;
get_wavfmt:;
    lir_text(10,line,"Use extended format (WAVEFORMATEXTENSIBLE) ? (Y/N)");
    lir_text(10,line+1,"F1 for info/help");
getfmt:;
    to_upper_await_keyboard();
    if(lir_inkey == F1_KEY || lir_inkey == '!')
      {
      help_message(108);
      clear_screen();
      goto get_wavfmt;
      }
    if(lir_inkey == 'N')
      {
      ui.use_alsa&=~TXDA_USE_EXTENSIBLE;
      }
    else
      {
      if(lir_inkey != 'Y')goto getfmt;
      ui.use_alsa|=TXDA_USE_EXTENSIBLE;
      }
    line++;
    clear_lines(line,line);
    line++;
    lir_text(10,line,"Sampling speed (Hz)>");
    ui.tx_da_speed=lir_get_integer(31,line, 6, 5000, 999999);
    if(kill_all_flag) goto set_tx_io_errexit;
    bits=16;
    if((ui.use_alsa&TXDA_USE_EXTENSIBLE) == TXDA_USE_EXTENSIBLE)
      {
get_txbits:;
      lir_text(40,line,"No of bits (16/24):");
      i=lir_get_integer(60,line, 2, 16, 24);
      if(kill_all_flag) goto set_tx_io_errexit;
      clear_screen();
      if(i==16)
        {
        ui.tx_da_bytes=2;
        }
      else
        {
        if(i==24)
          {
          ui.tx_da_bytes=4;
          bits=24;
          }
        else
          {
          goto get_txbits;
          }
        }
      }
    else
      {
      ui.tx_da_bytes=2;
      }
    clear_screen();
// Check whether the device(s) can do whatever the user asked for.
    j=8;
test_channels:;
    j/=2;
    fill_wavefmt(j,bits,(ui.use_alsa&TXDA_USE_EXTENSIBLE),ui.tx_da_speed);
// Make the buffer 40 milliseconds for this test.
    snd[TXDA].framesize=j*bits/8;
    snd[TXDA].block_bytes=0.04*ui.tx_ad_speed*snd[TXDA].framesize;
    mmrs=waveOutOpen(&hwav_txdaout, ui.tx_dadev_no, (WAVEFORMATEX*)&fmt, 
                                 0, 0, WAVE_FORMAT_QUERY|WAVE_FORMAT_DIRECT);
    if(mmrs != MMSYSERR_NOERROR)
      {
      if(j==4)goto test_channels;
      if(mmrs == MMSYSERR_ALLOCATED)
        {
        lirerr(1218);
        goto set_tx_io_errexit;
        }
      if(mmrs == MMSYSERR_BADDEVICEID)
        {
        lirerr(1219);
        goto set_tx_io_errexit;
        }
      if(mmrs == MMSYSERR_NODRIVER)
        {
        lirerr(1220);
        goto set_tx_io_errexit;
        }
      if(mmrs == MMSYSERR_NOMEM)
        {
        lirerr(1221);
        goto set_tx_io_errexit;
        }
      if(mmrs == WAVERR_BADFORMAT)
        {
        lirerr(1222);
        goto set_tx_io_errexit;
        }
      lirerr(1238);
      goto set_tx_io_errexit;
      }
    if(ui.tx_da_bytes == 4)
      {
      if( (ui.use_alsa&TXDA_USE_EXTENSIBLE) == 0)lirerr(5823198);
      fill_wavefmt(j,24,(ui.use_alsa&TXDA_USE_EXTENSIBLE),ui.tx_da_speed);
      mmrs=waveOutOpen(&hwav_txdaout, ui.tx_dadev_no, (WAVEFORMATEX*)&fmt, 
                      0, 0, WAVE_FORMAT_QUERY|WAVE_FORMAT_DIRECT);
      if(mmrs != MMSYSERR_NOERROR)
        {
        clear_screen();
        lir_text(5,5,"Can not use 24 bit D/A. Will use 16 bits instead.");
        lir_text(5,7,press_any_key);
        await_keyboard();
        ui.tx_da_bytes=2;
        }
      }
set_txout_exit:;
    if(ui.tx_enable != 0)
      {
      if(ui.rx_dadev_no >=0 && ui.rx_dadev_no < SPECIFIC_DEVICE_CODES)
        {
        stop_iotest(RXDA);
        }
set_txout_exit2:;
      if((ui.rx_addev_no >=0 && ui.rx_addev_no < SPECIFIC_DEVICE_CODES) &&
                                        (ui.network_flag&NET_RX_INPUT) == 0)
        {
        stop_iotest(RXAD);
        }
      }
    clear_screen();
    line=4;
    lir_text(9,line,"Select transmitter hardware to use with soundcard.");
    line+=3;
    for(i=0; i<MAX_TX_SOUNDCARD_RADIO; i++)
      {
      sprintf(s,"%d   %s",i,tx_soundcard_radio_names[i]);
      lir_text(5,line,s);
      line++;
      }
    line++;
    lir_text(0,line,"Select by line number=>");
    ui.tx_soundcard_radio=lir_get_integer(25,line,1,0,MAX_TX_SOUNDCARD_RADIO);
    break;
    
    case 'B':
    lir_text(0,line,"NOT YET IMPLEMENTED");
    lir_text(0,line+2,press_any_key);
    await_processed_keyboard();
    break;

    case 'C':
    lir_text(0,line,"NOT YET IMPLEMENTED");
    lir_text(0,line+2,press_any_key);
    await_processed_keyboard();
    break;
    }    
  break;

  case 'B':
  clear_screen();
  soundcard_error_flag=TRUE;
  line=5;
  if(ui.tx_dadev_no == UNDEFINED_DEVICE_CODE)
    {
    lir_text(0,line,"Set tx output device first.");
    line+=2;
    lir_text(10,line,press_any_key);
    await_processed_keyboard();
    break;
    }
  if(ui.tx_enable != 0)
    {
    if((ui.rx_addev_no >=0 && ui.rx_addev_no < SPECIFIC_DEVICE_CODES) &&
                                        (ui.network_flag&NET_RX_INPUT) == 0)
      {
      if(start_iotest(&line,RXAD) == FALSE)
        {
        if(kill_all_flag)goto set_tx_io_errexit;
        goto set_txin_exit2;
        }
      }
    if(ui.rx_dadev_no >=0 && ui.rx_dadev_no < SPECIFIC_DEVICE_CODES)
      {
      if(start_iotest(&line,RXDA) == FALSE)
        {
        if(kill_all_flag)goto set_tx_io_errexit;
        goto set_txin_exit1;
        }
      }
    }  
  if(ui.tx_dadev_no >=0 && ui.tx_dadev_no < SPECIFIC_DEVICE_CODES)
    {
    if(start_iotest(&line,TXDA) == FALSE)
      {
      if(kill_all_flag)goto set_tx_io_errexit;
      goto set_txin_exit;
      }
    }
  soundcard_error_flag=FALSE;    
sel_tx_input:;
  ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
  ui.use_alsa&=~PORTAUDIO_TX_IN;
  lir_text(10,5,"Use Portaudio for tx input? (Y/N) =>");
  lir_text(15,7,press_f1_for_info);
  await_processed_keyboard();
  if(kill_all_flag)goto set_tx_io_errexit;;
  if(lir_inkey == F1_KEY || lir_inkey == '!')
    {
    help_message(343);
go_sel_tx_input:;
    clear_screen();
    goto sel_tx_input;
    }
  if(lir_inkey == 'Y')
    {
    if( ((ui.use_alsa&PORTAUDIO_DLL_VERSION)>>5) == 0)
      {
      i=select_portaudio_dll();
      if(kill_all_flag)goto set_tx_io_errexit;
      if(i<0)
        {
        ui.use_alsa&=~PORTAUDIO_DLL_VERSION;
        ui.use_alsa|=(i+1)<<5;   
        }
      goto set_txin_exit;
      ui.use_alsa|=(i+1)<<5;   
      }
    ui.use_alsa|=PORTAUDIO_TX_IN;
    }
  else
    {
    if(lir_inkey != 'N')
      {
      goto go_sel_tx_input;
      }
    }
  i=1;
  if(portaudio_active_flag==FALSE)i=portaudio_start(302);
  if(kill_all_flag)goto set_tx_io_errexit;     
  if(!i)goto go_sel_tx_input;
  clear_screen();
  line=1;
  if( (ui.use_alsa&PORTAUDIO_TX_IN) != 0)
    {
    set_portaudio(TXAD, &line);
    goto set_txin_exit;
    }
  lir_text(10,0,"Select SOUNDCARD device for TX microphone input from list ");
  settextcolor(7);
  no_of_addev=waveInGetNumDevs();
  if(no_of_addev == 0)
    {
    lir_text(10,2,"No soundcard detected.");
    lir_text(10,3,press_any_key);
    await_keyboard();
    ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
    break;
    }
  line=2;
  k=0;
  if(no_of_addev > MAX_WAV_DEVICES)no_of_addev=MAX_WAV_DEVICES;
  for(device_id=0; device_id<no_of_addev; device_id++)
    {
    mmrs=waveInGetDevCaps(device_id, &pwic, sizeof(WAVEINCAPS));
    if(mmrs==MMSYSERR_NOERROR)
      {
      sprintf(s,"%d  %s", device_id, pwic.szPname);
      settextcolor(7);
      k++;
      }
    else
      {
      sprintf(s,"%d ERROR (mmrs=%d) %s", device_id, mmrs, pwic.szPname);
      settextcolor(3);
      }
    lir_text(10,line,s);
    line++;
    }
  line+=2;
  settextcolor(7);
  if(k == 0)
    {
    lir_text(10,line,"No usable input device found.");
    line+=2;
    lir_text(14,line,press_any_key);
    await_keyboard();
    ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
    break;
    }
  if(no_of_addev == 1)
    {
    ui.tx_addev_no=0;
    lir_text(10,line,"Device autoselected.");
    line++;
    }
  else
    {
    sprintf(s,"Select device for microphone input by line number>");
    lir_text(10,line,s);
    ui.tx_addev_no=lir_get_integer(61,line, 2, 0, no_of_addev-1);
    if(kill_all_flag) goto set_tx_io_errexit;
    }
// Windows does not allow us to query the device for its capabilities.
// Ask the user to specify what he wants.
  line+=2;
  lir_text(10,line,"Linrad can not query hardware because Windows will report that");
  line++;
  lir_text(10,line,"everything is possible. Windows will silently resample and provide");
  line++;
  lir_text(10,line,"data that would be meaningless in an SDR context.");
  line++;
  lir_text(10,line,"Therefore, make sure you enter data that is compatible with the");
  line++;
  lir_text(10,line,"native capabilities of your soundcardhardware. (And make sure that");
  line++;
  lir_text(10,line,"the soundcard really is set to the speed you have selected.)");
  line+=2;
get_txad_bits:;
  lir_text(40,line,"Select no of bits (16/24):");
  i=lir_get_integer(66,line, 2, 16, 24);
  if(kill_all_flag) goto set_tx_io_errexit;
  if(i==24)
    {
    ui.use_alsa|=TXAD_USE_EXTENSIBLE;
    ui.tx_ad_bytes=4;
    line++;
    lir_text(30,line,"Device set to use WAVEFORMATEXTENSIBLE");
    }
  else
    {
    if(i != 16)goto get_txad_bits;
    ui.tx_ad_bytes=2;
    }
  line++;  
  ui.tx_ad_channels=0;
try_channels:;
  ui.tx_ad_channels++;
// Check whether the device can do what we want.
  if(ui.tx_ad_bytes == 4)ui.use_alsa|=TXDA_USE_EXTENSIBLE;
  fill_wavefmt(2,16,(ui.use_alsa&TXDA_USE_EXTENSIBLE),ui.tx_da_speed);
// Make the buffer 40 milliseconds for this test.
  snd[TXAD].framesize=ui.tx_ad_bytes*ui.tx_ad_channels;
  snd[TXAD].block_bytes=0.04*ui.tx_ad_speed*snd[TXAD].framesize;
  mmrs=waveInOpen(&hwav_txadin, ui.tx_addev_no, (WAVEFORMATEX*)&fmt, 
                               0, 0, WAVE_FORMAT_QUERY|WAVE_FORMAT_DIRECT);
  if(mmrs != MMSYSERR_NOERROR)
    {
    if(mmrs == MMSYSERR_ALLOCATED)
      {
      lirerr(1218);
      goto set_tx_io_errexit;
      }
    if(mmrs == MMSYSERR_BADDEVICEID)
      {
      lirerr(1219);
      goto set_tx_io_errexit;
      }
    if(mmrs == MMSYSERR_NODRIVER)
      {
      lirerr(1220);
      goto set_tx_io_errexit;
      }
    if(mmrs == MMSYSERR_NOMEM)
      {
      lirerr(1221);
      goto set_tx_io_errexit;
      }
    if(mmrs == WAVERR_BADFORMAT)
      {
      if(ui.tx_ad_channels == 1)goto try_channels;
      if(ui.tx_ad_bytes == 4)
        {
        ui.use_alsa&=~TXDA_USE_EXTENSIBLE;
        ui.tx_ad_bytes=2;
        ui.tx_ad_channels=0;
        goto try_channels;
        }      
      lirerr(1222);
      goto set_tx_io_errexit;
      }
    lirerr(1238);
    goto set_tx_io_errexit;
    }
  ui.tx_ad_speed=get_mic_speed(&line, 8000, 192000);
set_txin_exit:;
  if(ui.tx_dadev_no >=0 && ui.tx_dadev_no < SPECIFIC_DEVICE_CODES)
    {
    stop_iotest(TXDA);
    }
set_txin_exit1:;
  if(ui.tx_enable != 0)
    {
    if(ui.rx_dadev_no >=0 && ui.rx_dadev_no < SPECIFIC_DEVICE_CODES)
      {
      stop_iotest(RXDA);
      }
set_txin_exit2:;
    if((ui.rx_addev_no >=0 && ui.rx_addev_no < SPECIFIC_DEVICE_CODES) &&
                                        (ui.network_flag&NET_RX_INPUT) == 0)
      {
      stop_iotest(RXAD);
      }
    }
  if(kill_all_flag)goto set_tx_io_errexit;
  if(!soundcard_error_flag)
    {
    line=5;
    clear_screen();
    lir_text(0,line,"The tx pilot tone may control fast Rx/Tx switching.");
    line++;
    lir_text(0,line,"Set pilot tone level in dB below signal (50-130)");
    ui.tx_pilot_tone_db=lir_get_integer(49, line, 3, 50, 130);
    line+=2;
    lir_text(0,line,"Set prestart in microseconds (0-100000)");
    ui.tx_pilot_tone_prestart=lir_get_integer(39, line, 6, 0, 100000);
    line+=2;
    settextcolor(15);
    lir_text(5,line,remind_parsave);
    settextcolor(7);
    line+=2;
    lir_text(5,line,press_any_key);
    await_processed_keyboard();
    }
  break;

  case 'C':
  switch (ui.tx_soundcard_radio)
    {
    case TX_SOUNDCARD_RADIO_SI570:
    si570_tx_setup();
    break;
    }
  break;

  case 'X':
  goto set_tx_io_exit;
  }
SNDLOG"goto begin_tx_set_io\n\n");
goto begin_tx_set_io;
set_tx_io_exit:;
SNDLOG"\nNormal end\n");
set_tx_io_errexit:;
if(portaudio_active_flag==TRUE)portaudio_stop(210);
fclose(sndlog);
write_log=FALSE;
}
