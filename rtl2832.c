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
#include <signal.h>
#include <string.h>
#include "uidef.h"
#include "sdrdef.h"
#include "fft1def.h"
#include "screendef.h"
#include "thrdef.h"
#include "options.h"
#include "vernr.h"
#include "hwaredef.h"



#define MAX_RTL2832_SAMP_RATE_H  2400000
#define MIN_RTL2832_SAMP_RATE_H  900001
#define MAX_RTL2832_SAMP_RATE_L  300000
#define MIN_RTL2832_SAMP_RATE_L  230000


typedef struct rtlsdr_dev rtlsdr_dev_t;
typedef void(*rtlsdr_read_async_cb_t)(unsigned char *buf, 
                                        uint32_t len, void *ctx);
                                                    
static rtlsdr_dev_t *dev_rtlsdr;

char *rtl2832_parm_text[MAX_RTL2832_PARM]={"Sampling speed",
                                     "Serno1",
                                     "Serno2",
                                     "Serno3",
                                     "Gain mode",
                                     "Freq adjust",
                                     "Direct sampling",
                                     "Bandwidth",
                                     "Check"};

char *rtl2832_parfil_name="par_rtl2832";
int rtlsdr_library_flag;
P_RTL2832 rtl2832;

typedef int (*p_rtlsdr_close)(rtlsdr_dev_t *dev);
typedef int (*p_rtlsdr_open)(rtlsdr_dev_t **dev, uint32_t iindex);
typedef int (*p_rtlsdr_get_device_usb_strings)(uint32_t iindex,
					       char *manufact,
					       char *product,
					       char *serial);
typedef int (*p_rtlsdr_get_tuner_gains)(rtlsdr_dev_t *dev, int *gains);
typedef int (*p_rtlsdr_set_sample_rate)(rtlsdr_dev_t *dev, uint32_t rate);
typedef int (*p_rtlsdr_set_tuner_gain_mode)(rtlsdr_dev_t *dev, int manual);
typedef int (*p_rtlsdr_get_tuner_bandwidths)(rtlsdr_dev_t *dev, int *bandwidths);
typedef int (*p_rtlsdr_set_tuner_bandwidth)(rtlsdr_dev_t *dev, int bandwidth);
typedef int (*p_rtlsdr_set_tuner_gain)(rtlsdr_dev_t *dev, int gain);
typedef int (*p_rtlsdr_set_center_freq)(rtlsdr_dev_t *dev, uint32_t freq);
typedef int (*p_rtlsdr_read_async)(rtlsdr_dev_t *dev,
				   rtlsdr_read_async_cb_t cb,
				   void *ctx,
				   uint32_t buf_num,
				   uint32_t buf_len);
typedef int (*p_rtlsdr_cancel_async)(rtlsdr_dev_t *dev);
typedef int (*p_rtlsdr_set_direct_sampling)(rtlsdr_dev_t *dev, int on);
typedef int (*p_rtlsdr_set_agc_mode)(rtlsdr_dev_t *dev, int on);
typedef uint32_t (*p_rtlsdr_get_sample_rate)(rtlsdr_dev_t *dev);
typedef uint32_t (*p_rtlsdr_get_device_count)(void);
typedef int (*p_rtlsdr_reset_buffer)(rtlsdr_dev_t *dev);
typedef const char* (*p_rtlsdr_get_device_name)(uint32_t iindex);


p_rtlsdr_reset_buffer rtlsdr_reset_buffer;
p_rtlsdr_get_device_name rtlsdr_get_device_name;
p_rtlsdr_get_device_count rtlsdr_get_device_count;
p_rtlsdr_get_sample_rate rtlsdr_get_sample_rate;
p_rtlsdr_set_agc_mode rtlsdr_set_agc_mode;
p_rtlsdr_set_direct_sampling rtlsdr_set_direct_sampling;
p_rtlsdr_cancel_async rtlsdr_cancel_async;
p_rtlsdr_set_center_freq rtlsdr_set_center_freq;
p_rtlsdr_close rtlsdr_close;
p_rtlsdr_open rtlsdr_open;
p_rtlsdr_get_device_usb_strings rtlsdr_get_device_usb_strings;
p_rtlsdr_get_tuner_gains rtlsdr_get_tuner_gains;
p_rtlsdr_set_sample_rate rtlsdr_set_sample_rate;
p_rtlsdr_set_tuner_gain_mode rtlsdr_set_tuner_gain_mode;
p_rtlsdr_set_tuner_gain rtlsdr_set_tuner_gain;
p_rtlsdr_read_async rtlsdr_read_async;
p_rtlsdr_get_tuner_bandwidths rtlsdr_get_tuner_bandwidths;
p_rtlsdr_set_tuner_bandwidth rtlsdr_set_tuner_bandwidth;


#if(OSNUM == OSNUM_WINDOWS)
#include <windows.h>
#include "wscreen.h"
HANDLE rtlsdr_libhandle;

void load_rtlsdr_library(void)
{
char rtlsdr_dllname[80];
int info;
if(rtlsdr_library_flag)return;
info=0;
sprintf(rtlsdr_dllname,"%srtlsdr.dll",DLLDIR);
rtlsdr_libhandle=LoadLibraryEx(rtlsdr_dllname, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
if(!rtlsdr_libhandle)goto rtlsdr_load_error;
info=1;
rtlsdr_reset_buffer=(p_rtlsdr_reset_buffer)(void*)GetProcAddress(rtlsdr_libhandle, "rtlsdr_reset_buffer");
if(!rtlsdr_reset_buffer)goto rtlsdr_sym_error;
rtlsdr_get_device_name=(p_rtlsdr_get_device_name)(void*)GetProcAddress(rtlsdr_libhandle, "rtlsdr_get_device_name");
if(!rtlsdr_get_device_name)goto rtlsdr_sym_error;
rtlsdr_get_device_count=(p_rtlsdr_get_device_count)(void*)GetProcAddress(rtlsdr_libhandle, "rtlsdr_get_device_count");
if(!rtlsdr_get_device_count)goto rtlsdr_sym_error;
rtlsdr_get_sample_rate=(p_rtlsdr_get_sample_rate)(void*)GetProcAddress(rtlsdr_libhandle, "rtlsdr_get_sample_rate");
if(!rtlsdr_get_sample_rate)goto rtlsdr_sym_error;
rtlsdr_set_agc_mode=(p_rtlsdr_set_agc_mode)(void*)GetProcAddress(rtlsdr_libhandle, "rtlsdr_set_agc_mode");
if(!rtlsdr_set_agc_mode)goto rtlsdr_sym_error;
rtlsdr_set_direct_sampling=(p_rtlsdr_set_direct_sampling)(void*)GetProcAddress(rtlsdr_libhandle, "rtlsdr_set_direct_sampling");
if(!rtlsdr_set_direct_sampling)goto rtlsdr_sym_error;
rtlsdr_cancel_async=(p_rtlsdr_cancel_async)(void*)GetProcAddress(rtlsdr_libhandle, "rtlsdr_cancel_async");
if(!rtlsdr_cancel_async)goto rtlsdr_sym_error;
rtlsdr_set_center_freq=(p_rtlsdr_set_center_freq)(void*)GetProcAddress(rtlsdr_libhandle, "rtlsdr_set_center_freq");
if(!rtlsdr_set_center_freq)goto rtlsdr_sym_error;
rtlsdr_close=(p_rtlsdr_close)(void*)GetProcAddress(rtlsdr_libhandle, "rtlsdr_close");
if(!rtlsdr_close)goto rtlsdr_sym_error;
rtlsdr_open=(p_rtlsdr_open)(void*)GetProcAddress(rtlsdr_libhandle, "rtlsdr_open");
if(!rtlsdr_open)goto rtlsdr_sym_error;
rtlsdr_get_device_usb_strings=(p_rtlsdr_get_device_usb_strings)(void*)GetProcAddress(rtlsdr_libhandle, "rtlsdr_get_device_usb_strings");
if(!rtlsdr_get_device_usb_strings)goto rtlsdr_sym_error;
rtlsdr_get_tuner_gains=(p_rtlsdr_get_tuner_gains)(void*)GetProcAddress(rtlsdr_libhandle, "rtlsdr_get_tuner_gains");
if(!rtlsdr_get_tuner_gains)goto rtlsdr_sym_error;
rtlsdr_set_sample_rate=(p_rtlsdr_set_sample_rate)(void*)GetProcAddress(rtlsdr_libhandle, "rtlsdr_set_sample_rate");
if(!rtlsdr_set_sample_rate)goto rtlsdr_sym_error;
rtlsdr_set_tuner_gain_mode=(p_rtlsdr_set_tuner_gain_mode)(void*)GetProcAddress(rtlsdr_libhandle, "rtlsdr_set_tuner_gain_mode");
if(!rtlsdr_set_tuner_gain_mode)goto rtlsdr_sym_error;
rtlsdr_set_tuner_gain=(p_rtlsdr_set_tuner_gain)(void*)GetProcAddress(rtlsdr_libhandle, "rtlsdr_set_tuner_gain");
if(!rtlsdr_set_tuner_gain)goto rtlsdr_sym_error;
rtlsdr_read_async=(p_rtlsdr_read_async)(void*)GetProcAddress(rtlsdr_libhandle, "rtlsdr_read_async");
if(!rtlsdr_read_async)goto rtlsdr_sym_error;
rtlsdr_library_flag=TRUE;
rtlsdr_get_tuner_bandwidths=(p_rtlsdr_get_tuner_bandwidths)(void*)GetProcAddress(rtlsdr_libhandle, "rtlsdr_get_tuner_bandwidths");
rtlsdr_set_tuner_bandwidth=(p_rtlsdr_set_tuner_bandwidth)(void*)GetProcAddress(rtlsdr_libhandle, "rtlsdr_set_tuner_bandwidth");
return;
rtlsdr_sym_error:;
FreeLibrary(rtlsdr_libhandle);
rtlsdr_load_error:;
library_error_screen(rtlsdr_dllname,info);
return;
}

void unload_rtlsdr_library(void)
{
if(!rtlsdr_library_flag)return;
FreeLibrary(rtlsdr_libhandle);
rtlsdr_library_flag=FALSE;
}
#endif

#if(OSNUM == OSNUM_LINUX)
#include <dlfcn.h>
#include "lscreen.h"
void *rtlsdr_libhandle;

void load_rtlsdr_library(void)
{
int info;
if(rtlsdr_library_flag)return;
info=0;
rtlsdr_libhandle=dlopen(RTLSDR_LIBNAME, RTLD_LAZY);
if(!rtlsdr_libhandle)goto rtlsdr_load_error;
info=1;
rtlsdr_reset_buffer=(p_rtlsdr_reset_buffer)dlsym(rtlsdr_libhandle, "rtlsdr_reset_buffer");
if(dlerror() != 0)goto rtlsdr_sym_error;
rtlsdr_get_device_name=(p_rtlsdr_get_device_name)dlsym(rtlsdr_libhandle, "rtlsdr_get_device_name");
if(dlerror() != 0)goto rtlsdr_sym_error;
rtlsdr_get_device_count=(p_rtlsdr_get_device_count)dlsym(rtlsdr_libhandle, "rtlsdr_get_device_count");
if(dlerror() != 0)goto rtlsdr_sym_error;
rtlsdr_get_sample_rate=(p_rtlsdr_get_sample_rate)dlsym(rtlsdr_libhandle, "rtlsdr_get_sample_rate");
if(dlerror() != 0)goto rtlsdr_sym_error;
rtlsdr_set_agc_mode=(p_rtlsdr_set_agc_mode)dlsym(rtlsdr_libhandle, "rtlsdr_set_agc_mode");
if(dlerror() != 0)goto rtlsdr_sym_error;
rtlsdr_set_direct_sampling=(p_rtlsdr_set_direct_sampling)dlsym(rtlsdr_libhandle, "rtlsdr_set_direct_sampling");
if(dlerror() != 0)goto rtlsdr_sym_error;
rtlsdr_cancel_async=(p_rtlsdr_cancel_async)dlsym(rtlsdr_libhandle, "rtlsdr_cancel_async");
if(dlerror() != 0)goto rtlsdr_sym_error;
rtlsdr_set_center_freq=(p_rtlsdr_set_center_freq)dlsym(rtlsdr_libhandle, "rtlsdr_set_center_freq");
if(dlerror() != 0)goto rtlsdr_sym_error;
rtlsdr_close=(p_rtlsdr_close)dlsym(rtlsdr_libhandle, "rtlsdr_close");
if(dlerror() != 0)goto rtlsdr_sym_error;
rtlsdr_open=(p_rtlsdr_open)dlsym(rtlsdr_libhandle, "rtlsdr_open");
if(dlerror() != 0)goto rtlsdr_sym_error;
rtlsdr_get_device_usb_strings=(p_rtlsdr_get_device_usb_strings)dlsym(rtlsdr_libhandle, "rtlsdr_get_device_usb_strings");
if(dlerror() != 0)goto rtlsdr_sym_error;
rtlsdr_get_tuner_gains=(p_rtlsdr_get_tuner_gains)dlsym(rtlsdr_libhandle, "rtlsdr_get_tuner_gains");
if(dlerror() != 0)goto rtlsdr_sym_error;
rtlsdr_set_sample_rate=(p_rtlsdr_set_sample_rate)dlsym(rtlsdr_libhandle, "rtlsdr_set_sample_rate");
if(dlerror() != 0)goto rtlsdr_sym_error;
rtlsdr_set_tuner_gain_mode=(p_rtlsdr_set_tuner_gain_mode)dlsym(rtlsdr_libhandle, "rtlsdr_set_tuner_gain_mode");
if(dlerror() != 0)goto rtlsdr_sym_error;
rtlsdr_set_tuner_gain=(p_rtlsdr_set_tuner_gain)dlsym(rtlsdr_libhandle, "rtlsdr_set_tuner_gain");
if(dlerror() != 0)goto rtlsdr_sym_error;
rtlsdr_read_async=(p_rtlsdr_read_async)dlsym(rtlsdr_libhandle, "rtlsdr_read_async");
if(dlerror() != 0)goto rtlsdr_sym_error;
rtlsdr_library_flag=TRUE;
rtlsdr_get_tuner_bandwidths=(p_rtlsdr_get_tuner_bandwidths)dlsym(rtlsdr_libhandle, "rtlsdr_get_tuner_bandwidths");
rtlsdr_set_tuner_bandwidth=(p_rtlsdr_set_tuner_bandwidth)dlsym(rtlsdr_libhandle, "rtlsdr_set_tuner_bandwidth");
return;
rtlsdr_sym_error:;
dlclose(rtlsdr_libhandle);
rtlsdr_load_error:;
library_error_screen(RTLSDR_LIBNAME,info);
}

void unload_rtlsdr_library(void)
{
if(!rtlsdr_library_flag)return;
dlclose(rtlsdr_libhandle);
rtlsdr_library_flag=FALSE;
}
#endif

static  void rtlsdr_callback(unsigned char *buf, uint32_t len, void *ctx)
{
(void) ctx;
unsigned int i;
short int *iz;
iz=(short int*)&timf1_char[timf1p_sdr];
for(i=0; i<len; i++)
  {
// Subtract 127.5 convert range (0,255) to (-127.5,127.5)
// Shift by 8 and add a small DC offset, the typical sum of 
// truncatíon errors in the RTL2832 resampler. 
// It is about 0.14 bit.
// (-127.5+0.14)*256 = 32604
  iz[i]=(buf[i]<<8)-32604;
  }
timf1p_sdr=(timf1p_sdr+(len<<1))&timf1_bytemask;
if( ((timf1p_sdr-timf1p_pa+timf1_bytes)&timf1_bytemask) >= 
                                                     snd[RXAD].block_bytes)
  {
  lir_set_event(EVENT_HWARE1_RXREADY);
  }
}

void set_rtl2832_att(void)
{
if(rtl2832.gain_mode == 0)return;
rtlsdr_set_tuner_gain(dev_rtlsdr, old_rtl2832_gain);
}

void set_rtl2832_frequency(void)
{
uint32_t frequency;
frequency=(fg.passband_center*(100000000-rtl2832.freq_adjust))/100;
rtlsdr_set_center_freq(dev_rtlsdr, frequency);
}

void rtl_starter(void)
{
int k, no_of_buffers;
k=snd[RXAD].block_bytes/2;
if(k < 512)k=512;
// Allocate buffers for 50 ms minimum.
no_of_buffers=(int)((0.1*rtl2832.sampling_speed)/k);
// Never use less than 4 buffers.
if(no_of_buffers < 4)no_of_buffers=4;
while(thread_status_flag[THREAD_RTL2832_INPUT]!=THRFLAG_ACTIVE)
  {
  lir_sleep(10000);
  if(kill_all_flag)return;
  }
rtlsdr_read_async(dev_rtlsdr, rtlsdr_callback, NULL,
				      no_of_buffers, k);
}

void rtl2832_input(void)
{
int i, j;
int rxin_local_workload_reset;
char s[256];
double dt1, read_start_time, total_reads;
int timing_loop_counter,timing_loop_counter_max,initial_skip_flag;
int local_att_counter;
int local_nco_counter;
char vendor[256], product[256], serial[256];
int *ise;
unsigned int no_of_rtl;
float t1;
uint32_t idx;
lir_init_event(EVENT_HWARE1_RXREADY);
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_RTL2832_INPUT);
#endif
local_att_counter=sdr_att_counter;
local_nco_counter=sdr_nco_counter;
dt1=current_time();
i=read_sdrpar(rtl2832_parfil_name, MAX_RTL2832_PARM, 
                                     rtl2832_parm_text, (int*)((void*)&rtl2832));
if(i != 0 || rtl2832.check != RTL2832PAR_VERNR)
  {
  lirerr(1356);
  goto rtl2832_init_error_exit;
  }
t1=((rint)((rtl2832.sampling_speed*
                     (100000000L+(double)rtl2832.freq_adjust))/100000000L));
if(ui.rx_ad_speed != (int)t1)
  {
  lirerr(1356);
  goto rtl2832_init_error_exit;
  }
load_rtlsdr_library();
if(!rtlsdr_library_flag)
  {
  lirerr(1414);
  goto rtl2832_init_error_exit;
  }
timf1p_sdr=timf1p_pa;  
j=0;
while(sdr == -1)
  {
  ise=(int*)(void*)s;
  dev_rtlsdr=NULL;
  no_of_rtl = rtlsdr_get_device_count();
  for(idx=0; idx<no_of_rtl; idx++)
    {
    rtlsdr_get_device_usb_strings(idx, vendor, product, serial);
    sprintf(s,"%s",serial);
    i=0;     
    while(i<16 && s[i]!=0)i++;
    while(i<16 )
      {
      s[i]=' ';
      i++;
      }
    if( rtl2832.sernum1 == ise[0] &&
        rtl2832.sernum2 == ise[1] &&
        rtl2832.sernum3 == ise[2])
      {  
      sdr=rtlsdr_open(&dev_rtlsdr, idx);
      if(kill_all_flag)  goto rtl2832_init_error_exit;
      if(sdr >= 0)
        {
        i=rtlsdr_set_sample_rate(dev_rtlsdr, (uint32_t)rtl2832.sampling_speed);
        if(i < 0)
          {
          lirerr(1339);
opnerr:;        
          rtlsdr_close(dev_rtlsdr);
          goto rtl2832_init_error_exit;
          }
// Direct Sampling mode. tuner = 0 (default), I = 1, Q = 2
        if(rtl2832.direct_sampling>0)
          {
          i=rtlsdr_set_direct_sampling(dev_rtlsdr, rtl2832.direct_sampling);
          if(i!=0)
            {
            lirerr(1358);
            goto opnerr; 
            }
// Enable rtl2831 AGC
          if(rtl2832.gain_mode==0) i=rtlsdr_set_agc_mode(dev_rtlsdr, 1);
          }
// Reset endpoint before we start reading from the RTL2832 (mandatory)
        i=rtlsdr_reset_buffer(dev_rtlsdr);
        if(i < 0)
          {
          lirerr(1340);
          goto opnerr;
          }
        linrad_thread_create(THREAD_RTL_STARTER);
        i=rtlsdr_set_tuner_gain_mode(dev_rtlsdr, rtl2832.gain_mode);
        if(i != 0 && i != rtl2832.gain_mode)goto rtl2832_error_exit;
        if(rtl2832.gain_mode != 0)
          {
          no_of_rtl2832_gains=rtlsdr_get_tuner_gains(dev_rtlsdr, NULL);
          if(no_of_rtl2832_gains < 1 ||
             no_of_rtl2832_gains > 256)goto rtl2832_error_exit;
          rtl2832_gains=malloc(no_of_rtl2832_gains*sizeof(int));
          if(rtl2832_gains == NULL)goto rtl2832_error_exit;
          rtlsdr_get_tuner_gains(dev_rtlsdr, rtl2832_gains);
          }
        break;
        }
      else
        {
        if(j==0)
          {
          clear_screen();
          j=1;
          }
        }
      }
    }
  sprintf(s,"looking for rtlsdr %.2f", current_time()-dt1);
  lir_text(3,5,s);
  lir_refresh_screen();
  if(kill_all_flag)goto rtl2832_init_error_exit;
  lir_sleep(100000);
  }
set_hardware_rx_gain();
set_rtl2832_att();
if(rtlsdr_set_tuner_bandwidth)
  {
  rtlsdr_set_tuner_bandwidth(dev_rtlsdr, rtl2832.bandwidth);
  }
set_rtl2832_frequency();
fft1_block_timing();
if(thread_command_flag[THREAD_SCREEN]!=THRFLAG_NOT_ACTIVE)
  {
  while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
    {
    if(thread_command_flag[THREAD_RTL2832_INPUT] ==
                                           THRFLAG_KILL)goto rtl2832_error_exit1;
    lir_sleep(10000);
    }
  }
#include "timing_setup.c"
thread_status_flag[THREAD_RTL2832_INPUT]=THRFLAG_ACTIVE;
lir_status=LIR_OK;
while(!kill_all_flag && 
            thread_command_flag[THREAD_RTL2832_INPUT] == THRFLAG_ACTIVE)
  {
  if(local_att_counter != sdr_att_counter)
    {
    local_att_counter=sdr_att_counter;
    set_rtl2832_att();
    }
  if(local_nco_counter != sdr_nco_counter)
    {
    local_nco_counter=sdr_nco_counter;
    set_rtl2832_frequency();
    }
  lir_await_event(EVENT_HWARE1_RXREADY);
  if(kill_all_flag)goto rtl2832_error_exit1;
  
  while (!kill_all_flag && timf1p_sdr != timf1p_pa)
    {
#include "input_speed.c"
    finish_rx_read();
    }
  }
rtl2832_error_exit1:;
if(rtl2832.gain_mode != 0)free(rtl2832_gains);
rtl2832_error_exit:;
rtlsdr_cancel_async(dev_rtlsdr);
lir_join(THREAD_RTL_STARTER);
rtlsdr_close(dev_rtlsdr);
unload_rtlsdr_library();
rtl2832_init_error_exit:;
sdr=-1;
thread_status_flag[THREAD_RTL2832_INPUT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_RTL2832_INPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
lir_close_event(EVENT_HWARE1_RXREADY);
} 

#define MAX_BW 32
void init_rtl2832(void)
{
FILE *rtl2832_file;
char s[1024];
int bw[MAX_BW];
char vendor[256], product[256], serial[256];
int *ise, *sdr_pi;
int i, line, no_of_rtl, devno;
int no_of_bw;
float t1;
load_rtlsdr_library();
if(!rtlsdr_library_flag)return;
ise=(int*)(void*)s;
lir_text(3,2,"SEARCHING");
no_of_rtl = rtlsdr_get_device_count();
clear_lines(2,2);
if (no_of_rtl == 0) 
  {
  lir_text(5,5,"RTL2832 not found.");
  lir_text(5,7,press_any_key);
  await_keyboard();
  goto init_x;
  }
line=2;  
for(i=0; i<no_of_rtl; i++)
  {
  rtlsdr_get_device_usb_strings(i, vendor, product, serial);
  sprintf(s," %2d: %s   MFG:%s, PROD:%s, SN: %s", i, 
                           rtlsdr_get_device_name(i), vendor, product, serial);
  lir_text(3,line,s);
  line++;
  }
line++;
if (no_of_rtl == 1) 
  {
  lir_text(3,line,"Device autoselected.");
  devno=0;
  }
else
  {  
  lir_text(3, line, "Select device by line number:");
  devno=lir_get_integer(32,line,2,0,no_of_rtl-1);
  }
if(kill_all_flag)
  {
  goto init_x;
  }
rtlsdr_get_device_usb_strings(devno, vendor, product, serial);
sprintf(s,"%s",serial);
i=0;
while(i<16 && s[i]!=0)i++;
while(i<16)
  {
  s[i]=' ';
  i++;
  }
rtl2832.sernum1=ise[0];
rtl2832.sernum2=ise[1];
rtl2832.sernum3=ise[2];
line+=2;
lir_sleep(100000);
i=rtlsdr_open(&dev_rtlsdr, (uint32_t)devno);
if(i<0)
  {
#if(OSNUM == OSNUM_WINDOWS)
  lirerr(1348);
#else
  lirerr(1357);
#endif
  goto init_x;
  }
getspeed:;
sprintf(s,"Set sampling speed in Hz (%d to %d or %d to %d) =>",
                                    MIN_RTL2832_SAMP_RATE_L,MAX_RTL2832_SAMP_RATE_L,
                                    MIN_RTL2832_SAMP_RATE_H,MAX_RTL2832_SAMP_RATE_H);
lir_text(3,line,s);
rtl2832.sampling_speed=lir_get_integer(4+strlen(s),line,7,
                                MIN_RTL2832_SAMP_RATE_L,MAX_RTL2832_SAMP_RATE_H);
if(kill_all_flag)
  {
  rtlsdr_close(dev_rtlsdr);
  goto init_x;
  }
line+=2;
if(rtl2832.sampling_speed > MAX_RTL2832_SAMP_RATE_L &&
   rtl2832.sampling_speed < MIN_RTL2832_SAMP_RATE_H)
  {
  if( rtl2832.sampling_speed-MAX_RTL2832_SAMP_RATE_L >
      MIN_RTL2832_SAMP_RATE_H-rtl2832.sampling_speed)
    {
    rtl2832.sampling_speed=MIN_RTL2832_SAMP_RATE_H;
    }
  else       
    {
    rtl2832.sampling_speed=MAX_RTL2832_SAMP_RATE_L;
    }
  }  
i=rtlsdr_set_sample_rate(dev_rtlsdr, (uint32_t)rtl2832.sampling_speed);
rtl2832.sampling_speed=rtlsdr_get_sample_rate(dev_rtlsdr);
if(i < 0)
  {
  lir_text(3,line,"Failed to set sampling rate");
  lir_text(3,line+1,press_any_key);
  await_keyboard();
  clear_lines(line,line+1);
  goto getspeed;
  }
lir_text(3, line, "RF input modes:");
line++;
lir_text(3, line, "0 = Tuner");
line++;
lir_text(3, line, "1 = I Direct Sampling");
line++;
lir_text(3, line, "2 = Q Direct Sampling");
line+=2;
lir_text(3, line, "Select :");
rtl2832.direct_sampling=lir_get_integer(12,line,2,0,2);
if(kill_all_flag)
  {
  rtlsdr_close(dev_rtlsdr);
  goto init_x;
  }
if(rtl2832.direct_sampling==0)
  {
  i=rtlsdr_set_tuner_gain_mode(dev_rtlsdr, 32);
  } 
else 
  {
  i=1;
  }
if(i<=0)i=1;
lir_text(3,line,"Select gain mode:");
line++;
lir_text(3,line,"0 = Auto");
line++;
lir_text(3,line,"1 = Original Osmocom");
line++;
if(i>1)
  {
  lir_text(3,line,"2 = Linearity");
  line++;
    {
    if(i > 3)
      {
      lir_text(3,line,"to");
      line++;
      }
    }
  sprintf(s,"%d = Sensitivity",i);
  lir_text(3,line,s);
  }
line++;
sprintf(s,"Enter gain mode (0 to %d) =>",i);
rtlsdr_close(dev_rtlsdr);
lir_text(3,line,s);
rtl2832.gain_mode=lir_get_integer(4+strlen(s),line,1,0,i);
if(kill_all_flag)goto init_x;
line+=2;
if(rtlsdr_get_tuner_bandwidths && rtlsdr_set_tuner_bandwidth)
  {
  no_of_bw=rtlsdr_get_tuner_bandwidths(dev_rtlsdr, NULL);
  if(no_of_bw > 1)
    {
    if(no_of_bw > MAX_BW)no_of_bw=MAX_BW;
    rtlsdr_get_tuner_bandwidths(dev_rtlsdr, bw);
    for(i=0; i<no_of_bw; i++)
      {
      sprintf(s,"%c => %d kHz",i+'A', bw[i]/1000);
      lir_text(3,line,s);
      line++;
      }
    line++;
    check_line(&line);
    lir_text(0,line,"Select bandwidth from table.");
getbw:;
    to_upper_await_keyboard();
    if(kill_all_flag)goto init_x;
    if(lir_inkey < 'A')goto getbw;
    i=lir_inkey-'A';
    if(i >= no_of_bw)goto getbw;
    rtl2832.bandwidth=bw[i];
    clear_lines(line,line);
    sprintf(s,"Selected bandwidth = %d kHz",bw[i]/1000);
    lir_text(10,line,s);  
    line++;
    }
  }
check_line(&line);  
lir_text(3,line,"Enter xtal error in ppb =>");
rtl2832.freq_adjust=0.1*lir_get_float(32,line,9,-300000.,300000.);
if(kill_all_flag)goto init_x;
rtl2832_file=fopen(rtl2832_parfil_name,"w");
if(rtl2832_file == NULL)
  {
  lirerr(381268);
  goto init_x;
  }
rtl2832.check=RTL2832PAR_VERNR;
sdr_pi=(int*)(&rtl2832);
for(i=0; i<MAX_RTL2832_PARM; i++)
  {
  fprintf(rtl2832_file,"%s [%d]\n",rtl2832_parm_text[i],sdr_pi[i]);
  }
parfile_end(rtl2832_file);
ui.rx_addev_no=RTL2832_DEVICE_CODE;
t1=((rint)((rtl2832.sampling_speed*
                   (100000000L+(double)rtl2832.freq_adjust))/100000000L));
ui.rx_ad_speed=(int)t1;
ui.rx_input_mode=IQ_DATA;
ui.rx_rf_channels=1;
ui.rx_ad_channels=2;
ui.rx_admode=0;
init_x:
unload_rtlsdr_library();
}

int display_rtl2832_parm_info(int *line)
{
char s[80];
int errcod;
char* input_modes[3]={"Tuner",
                      "I Direct Sampling",
                      "Q Direct Sampling"}; 
//if(FBDEV == 0)fflush(NULL);
errcod=read_sdrpar(rtl2832_parfil_name, MAX_RTL2832_PARM, 
                               rtl2832_parm_text, (int*)((void*)&rtl2832));
//if(FBDEV == 0)fflush(NULL);
if(errcod == 0)
  {
  settextcolor(7);
  sprintf(s,"Sampling rate = %i, Gain mode = %d, Xtal adjust = %.0f ppb",  
                              rtl2832.sampling_speed, rtl2832.gain_mode,
                              10.*rtl2832.freq_adjust);
  lir_text(24,line[0],s);
  SNDLOG"\n%s",s);
  line[0]++;
  sprintf(s,"Rf input mode = %d (%s)",rtl2832.direct_sampling,
                                 input_modes[rtl2832.direct_sampling]);
  lir_text(24,line[0],s);
  line[0]++;
  sprintf(s,"IF bandwidth  = %d kHz",rtl2832.bandwidth/1000);
  lir_text(24,line[0],s);
  SNDLOG"\n%s",s);
  line[0]++;
  }
return (errcod);
}
