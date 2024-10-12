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
#include <fcntl.h>
#include "uidef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "fft3def.h"
#include "screendef.h"
#include "sigdef.h"
#include "seldef.h"
#include "thrdef.h"
#include "txdef.h"
#include "options.h"

extern double q_time;
double iir3_gain[9]={6.0,
                    31.55634919,
                    188.6646578,
                    1274.138623,
                    9304.588559,
                    70996.86308,
                    554452.4788,
                    4382011.796, 
                    34842632.96};

double iir3_c0[9]={0.0,
                  0.1978251873,
                  0.4535459334,
                  0.6748018873,
                  0.8216600080,
                  0.9064815219,
                  0.9520967536,
                  0.9757549044,
                  0.9878031267};

double iir3_c1[9]={ -0.3333333333,
                   -0.9103690003,
                   -1.7151178300,
                   -2.2889939150,
                   -2.6262484669,
                   -2.8084292678,
                   -2.9030250284,
                   -2.9512131915,
                   -2.9755315293};

double iir3_c2[9]={ 0.0,
                   1.4590290622,
                   2.2191686183,
                   2.6079132760,
                   2.8037286680,
                   2.9018350649,
                   2.9509138462,
                   2.9754564614,
                   2.9877281729};

void blocking_rxout(void)
{
int max_wrbytes;
int i, wrnum, sleep_cnt;
int local_workload_reset;
float t1;
float sleep_time, short_sleep_time;
max_wrbytes=0;
mailbox[12]=max_wrbytes;
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_BLOCKING_RXOUT);
#endif
local_workload_reset=workload_reset_flag;
sleep_time=1000000./snd[RXDA].interrupt_rate;
if(sleep_time < 1010)sleep_time=1010;
if(sleep_time > 100000)sleep_time=100000;
short_sleep_time=0.2*sleep_time;
if(short_sleep_time > 10000)short_sleep_time=10000;
if(short_sleep_time < 1010)short_sleep_time=1010;
if(thread_status_flag[THREAD_BLOCKING_RXOUT]==THRFLAG_INIT)
  {
  thread_command_flag[THREAD_BLOCKING_RXOUT] = THRFLAG_IDLE;
  lir_sched_yield();
  goto idle;
  }
restart_blo:;
thread_status_flag[THREAD_BLOCKING_RXOUT]=THRFLAG_BLOCKING;
lir_await_event(EVENT_BLOCKING_RXOUT);
if(kill_all_flag) goto exit2;
if(thread_command_flag[THREAD_BLOCKING_RXOUT] != THRFLAG_ACTIVE)goto exit;
lir_sched_yield();
sys_func(THRFLAG_OPEN_RX_SNDOUT);
lir_empty_da_device_buffer();
thread_status_flag[THREAD_BLOCKING_RXOUT]=THRFLAG_ACTIVE;
snd[RXDA].open_flag=CALLBACK_CMD_ACTIVE;
if(rxda_status != LIR_OK)
  {
  lirerr(1477);
  goto exit2;
  } 
make_timing_info();
t1=total_wttim-da_wait_time-min_wttim;
if(t1 < 0)
  {
  lir_sleep(-1000000.*t1);
  make_timing_info();
  t1=total_wttim-da_wait_time-min_wttim;
  }
if(t1 > 0.02  && allow_audio==FALSE)
  {
// We have too much data in the buffers. Move pointers.
  if(baseb_pa != baseb_py)
    {
    i=(baseb_pa-baseb_py+baseband_size)&baseband_mask;      
    baseb_wttim=(float)i/baseband_sampling_speed;
    if(baseb_wttim < t1)
      {
      baseb_py=baseb_pa;
      }
    else
      {
      i=t1*baseband_sampling_speed;
      baseb_py=(baseb_py+i+baseband_size)&baseband_mask;
      }
    }
  } 
allow_audio=TRUE;
lir_sched_yield();
t1=total_wttim-da_wait_time-min_wttim;
lir_sched_yield();
min_daout_samps=daout_bufmask;
sleep_cnt=0;
wrnum=0;
while(!kill_all_flag && 
         thread_command_flag[THREAD_BLOCKING_RXOUT]==THRFLAG_ACTIVE)
  {
// *******************************************************
  update_snd(RXDA);
  if(wrnum == 100)
    {
    snd[RXDA].min_valid_frames=snd[RXDA].tot_frames;
    wrnum++;
    }
  if(local_workload_reset!=workload_reset_flag)
    {
    snd[RXDA].min_valid_frames=snd[RXDA].tot_frames;
    local_workload_reset=workload_reset_flag;
    }
  if( ((daout_pa-daout_px+daout_size)&daout_bufmask) > snd[RXDA].block_bytes)
    {
    sleep_cnt=0;
    if(audio_dump_flag==0 && rx_audio_out != -1)
      {
      while( ((daout_pa-daout_px+daout_size)&daout_bufmask) > 
                                            snd[RXDA].block_bytes)
        {
        lir_sched_yield();
        if(kill_all_flag)goto exit; 
        if(thread_command_flag[THREAD_BLOCKING_RXOUT] != THRFLAG_ACTIVE)goto exit;
        lir_rx_dawrite();
        if(wrnum < 1000)wrnum++;;
        }
      }
    else
      {
      while( ((daout_pa-daout_px+daout_bufmask)&daout_bufmask) > 
                                                   snd[RXDA].block_bytes)
        {
        daout_px=(daout_px+snd[RXDA].block_bytes)&daout_bufmask;
        }
      }
    }
  else
    {
    if(audio_dump_flag == 1 || rx_audio_out == -1)
      {
      lir_sleep(sleep_time);
      }
    else
      {
      if(snd[RXDA].valid_bytes > 8*snd[RXDA].block_bytes)
        {
        lir_sleep(sleep_time);
        goto loop_end;;   
        }
      else
        {
        sleep_cnt++;
        if(sleep_cnt > 4)
          {
          lir_sleep(short_sleep_time);
          }
        }
      }
    }
loop_end:;
  lir_sched_yield();
  }
exit:;
if(thread_command_flag[THREAD_BLOCKING_RXOUT] != THRFLAG_IDLE)
  {
  goto exit2;
  }
/*
    if( (ui.use_alsa&(PORTAUDIO_TX_IN+PORTAUDIO_RX_OUT)) == 0)
      {
      while( rx_audio_out == tx_audio_in)lir_sleep(10000);
      }
// Perhaps when using OSS in RDWR mode???
*/
idle:;
if(rx_audio_out != -1)
  {
  sys_func(THRFLAG_CLOSE_RX_SNDOUT);
  while(rx_audio_out != -1)
    {
    lir_sleep(100);
    }
  }
thread_status_flag[THREAD_BLOCKING_RXOUT]=THRFLAG_IDLE;
while(thread_command_flag[THREAD_BLOCKING_RXOUT] == THRFLAG_IDLE)
  {
  lir_await_event(EVENT_BLOCKING_RXOUT);
  if(kill_all_flag)goto exit2;
  }
if(thread_command_flag[THREAD_BLOCKING_RXOUT] == 
                                             THRFLAG_ACTIVE)goto restart_blo;

exit2:;  
thread_status_flag[THREAD_BLOCKING_RXOUT]=THRFLAG_RETURNED;
if(rx_audio_out != -1)
  {
  sys_func(THRFLAG_CLOSE_RX_SNDOUT);
  }
while(!kill_all_flag &&
            thread_command_flag[THREAD_BLOCKING_RXOUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

void rx_output(void)
{
char delay_margin_flag;
float dasync_time_interval;
int da_start_bytes;
int dasync_errors;
int i;
int speed_cnt_maxval;
char s[80];
double dt1;
int local_workload_reset;
int speed_cnt, blocking;
float t1, t2;
float sleep_time;
double total_time2;
double daspeed_time;
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_RX_OUTPUT);
#endif
blocking=TRUE;
#if(OSNUM == OSNUM_WINDOWS)
blocking=FALSE;
#endif
if( (ui.use_alsa&PORTAUDIO_RX_OUT) != 0)blocking=FALSE;
if(thread_status_flag[THREAD_RX_OUTPUT]==THRFLAG_INIT)
  {
  thread_command_flag[THREAD_RX_OUTPUT] = THRFLAG_IDLE;
  goto idle;
  }
else
  {
  lirerr(765897);
  }  
resume:;
if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_IDLE) goto idle;
if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_KILL)goto exit2;
if(thread_command_flag[THREAD_RX_OUTPUT] != THRFLAG_ACTIVE)
  {
  lirerr(639762);
  }
dasync_errors=0;
da_start_bytes=-1;
baseb_output_block=128;
da_start_bytes=-1;
speed_cnt_maxval=0.2*snd[RXDA].interrupt_rate;
count_rx_underrun_flag=FALSE;
if(kill_all_flag) goto exit2;
lir_sched_yield();
if(rxda_status != LIR_OK) 
  {
  wg_error("Output soundcard disabled",WGERR_SOUNDCARD);
  }
thread_status_flag[THREAD_RX_OUTPUT]=THRFLAG_ACTIVE;
count_rx_underrun_flag=FALSE;
lir_sched_yield();
sleep_time=1000000./snd[RXDA].interrupt_rate;
if(sleep_time < 1010)sleep_time=1010;
if(sleep_time > 100000)sleep_time=100000;
dasync_time_interval=(float)(7*fft3_size)/timf3_sampling_speed;
if(dasync_time_interval < (float)(7*fft1_size)/timf1_sampling_speed)
             dasync_time_interval = (float)(7*fft1_size)/timf1_sampling_speed;
if(genparm[SECOND_FFT_ENABLE]!=0)
  {
  if(dasync_time_interval < (float)(7*fft2_size)/timf1_sampling_speed)
             dasync_time_interval = (float)(7*fft2_size)/timf1_sampling_speed;
  }
if(dasync_time_interval < 0.5)dasync_time_interval=0.5;
local_workload_reset=workload_reset_flag;
speed_cnt=snd[RXDA].interrupt_rate;
baseb_output_block=snd[RXDA].block_bytes/(2*da_resample_ratio);
if(baseb_output_block < 16)baseb_output_block=16;
baseb_min_block=1+1/da_resample_ratio;
if(baseb_min_block < baseb_output_block/16)
  {
  baseb_min_block=baseb_output_block/16;
  }
dasync_time=current_time();
daspeed_time=dasync_time;
if(!blocking)
  {
  allow_audio=TRUE;
  daout_px=0;
  daout_pa=0;
  daout_py=0;
await_narrowband_processing:;
  if(kill_all_flag) goto exit2;
  if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_IDLE)goto idle;
  if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_KILL)goto exit2;
  make_timing_info();
  t1=total_wttim-da_wait_time-min_wttim;
  if(t1 < 0)
    {
    lir_sleep(3000);
    goto await_narrowband_processing;
    }
  make_timing_info();
  t1=total_wttim-da_wait_time-min_wttim;
  if(kill_all_flag) goto exit2;
  if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_IDLE)goto idle;
  if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_KILL)goto exit2;
  if(t1*baseband_sampling_speed > 3*baseb_output_block )//|| k<2)
    {
    if(recent_time-dasync_time > 2)
      {
      lirerr(1199);
      goto exit2;
      }
    goto await_narrowband_processing;
    }
  baseb_fx=0;
  daout_pa=0;
  daout_px=0;
  daout_py=0;
  snd[RXDA].min_valid_frames=snd[RXDA].tot_frames;
  min_daout_samps=baseband_size;
  lir_sleep(3000);
  make_timing_info();
  t1=total_wttim-da_wait_time-min_wttim;
  if(t1*baseband_sampling_speed > 3*baseb_output_block )//|| k<2)
    {
    goto await_narrowband_processing;
    }
  }
if(da_start_bytes == -1)
  {
  dasync_counter=0;
  dasync_sum=0;
  dasync_avg1=-1.5;
  dasync_avg2=-1.5;
  dasync_avg3=-1.5;
  dasync_time=current_time();
  dasync_avgtime=dasync_time;
  daspeed_time=dasync_time+1;
  }
delay_margin_flag=TRUE;
lir_sleep(1000);
if(kill_all_flag) goto exit2;
if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_IDLE)goto idle;
if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_KILL)goto exit2;
while(!kill_all_flag &&
                   thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_ACTIVE)
  {
// We use speed_cnt to avoid using a lot of CPU time on this code section.
  if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_IDLE)goto idle;
  if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_KILL)goto exit2;
  if(audio_dump_flag==0)speed_cnt--;
  if(speed_cnt==0)
    {
    if(delay_margin_flag)
      {
      snd[RXDA].min_valid_frames=snd[RXDA].tot_frames;
      min_daout_samps=baseband_size;
      delay_margin_flag=FALSE;
      }
    if(local_workload_reset!=workload_reset_flag)
      {
      snd[RXDA].min_valid_frames=snd[RXDA].tot_frames;
      local_workload_reset=workload_reset_flag;
      }
    speed_cnt=speed_cnt_maxval;
    total_time2=recent_time;
    if(recent_time-daspeed_time > 0)
      {
// Compute the D/A speed and once every second.
      if(audio_dump_flag==0)
        {
        daspeed_time=recent_time+1;
        lir_sched_yield();
        if(da_start_bytes == -1)
          {
          da_start_time=total_time2;
          da_block_counter=0;
          da_start_bytes=snd[RXDA].empty_bytes;
          snd[RXDA].min_valid_frames=snd[RXDA].tot_frames;
          count_rx_underrun_flag=TRUE;
          }
        else
          {
          measured_da_speed=(da_block_counter*snd[RXDA].block_bytes+
                                  snd[RXDA].empty_bytes-da_start_bytes)/
                          (snd[RXDA].framesize*(total_time2-da_start_time));
          }
        }
// Keep track of the error in the frequency ratio between input and output.
// Average the total processing delay and adjust resampling rate to
// keep it constant.
#define DASYNC_MAXCOUNT 15 //Number of seconds used to form average 
      dt1=current_time();
      if(  diskread_flag == 0 &&
           new_baseb_flag == 0 &&
           dt1-dasync_avgtime > dasync_time_interval &&
           rx_audio_out != -1)
        {
        dasync_avgtime=dt1;
        make_timing_info();
        if(kill_all_flag) goto exit2;
        if(timinfo_flag != 0)
          {
          sprintf(s,"sync:%f ",total_wttim-dasync_avg3);
          lir_text(26,screen_last_line-8,s);
          }
        if(dasync_avg3 >= 0 && fabs(total_wttim-dasync_avg3)>0.5)
          {
          dasync_errors++;
          sprintf(s,"DA SYNC ERRORS %d",dasync_errors);
          wg_error(s,WGERR_DASYNC);
//öö??          baseb_reset_counter++;
          da_start_bytes=-1;
          dasync_counter=0; 
          }
        dasync_counter++;
        dasync_sum+=total_wttim;
        if(dasync_counter >= DASYNC_MAXCOUNT)
          {
          dasync_avg1=dasync_sum/dasync_counter;
          dasync_counter=0;
          dasync_sum=0;
          t2=dt1-dasync_time;
          dasync_time=dt1;
          if(dasync_avg2 > 0)
            {
// The drift between input and output after t2 seconds is
// the difference between the two averages.
// Convert to frequency ratio, but use half the error only
// to avoid oscillations.
// Correct for the deviation from the first average, but only 20%
// so we drift slowy towards it if we are off.
// also add 5% of the time deviation from the ideal time.
            t1= (t2+0.5*(dasync_avg2-dasync_avg1)+
                                   0.2*(dasync_avg3-dasync_avg1))/t2;
//            dasync_avg3=0.9*dasync_avg3+0.1*da_wait_time;
            new_da_resample_ratio=da_resample_ratio*t1;
            dasync_avg2=dasync_avg1;
            }
          else
            {
            if(dasync_avg3 <0)
              {
              dasync_avg3=dasync_avg1;
              }
            else
              {
              dasync_avg2=dasync_avg1;
              }
            }
          }
        }
      if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_IDLE)goto idle;
      if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_KILL)goto exit2;
      }
    }
// ***********************************************************
  lir_sched_yield();
  if( ((baseb_pa-baseb_py+baseband_size)&baseband_mask) > baseb_output_block )
    {
    if( ((daout_px-snd[RXDA].block_bytes-daout_pa+daout_bufmask)&daout_bufmask) > (daout_size>>1) )
      {
      make_audio_signal();
      }
    else
      {
      if(audio_dump_flag==0)lir_sleep(sleep_time);
      }
    }
  lir_sched_yield();  
  if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_IDLE)goto idle;
  if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_KILL)goto exit2;
  if(kill_all_flag)goto exit2;
  if( ((baseb_pa-baseb_py+baseband_size)&baseband_mask) <= baseb_output_block )
    {
    thread_status_flag[THREAD_RX_OUTPUT]=THRFLAG_SEM_WAIT;
    lir_await_event(EVENT_BASEB);
    thread_status_flag[THREAD_RX_OUTPUT]=THRFLAG_ACTIVE;
    if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_IDLE)goto idle;
    if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_KILL)goto exit2;
    }
  if( (ui.network_flag&NET_RXOUT_BASEBRAW) != 0)
    {
#if NET_BASEBRAW_MODE == WFM_FM_FIRST_LOWPASS || \
    NET_BASEBRAW_MODE == WFM_FM_FIRST_LOWPASS_UPSAMP || \
    NET_BASEBRAW_MODE == WFM_STEREO_LOW || \
    NET_BASEBRAW_MODE == WFM_STEREO_HIGH    
    int *ntbuf;
    while(baseb_pn != baseb_pa && 
     ((basebrawnet_px-basebrawnet_pa+basebrawnet_mask)&basebrawnet_mask) >16)
      {
      ntbuf=(int*)&basebraw_netsend_buffer[basebrawnet_pa];
      ntbuf[0]=floor(1000000000.*baseb_fm_demod_low[baseb_pn]);
      baseb_pn=(baseb_pn+1)&baseband_mask;
      basebrawnet_pa=(basebrawnet_pa+4)&basebrawnet_mask;
      }
#endif
#if NET_BASEBRAW_MODE == WFM_FM_FULL_BANDWIDTH
    int *ntbuf;
    while(baseb_pn != baseb_pa && 
     ((basebrawnet_px-basebrawnet_pa+basebrawnet_mask)&basebrawnet_mask) >16)
      {
      ntbuf=(int*)&basebraw_netsend_buffer[basebrawnet_pa];
      ntbuf[0]=floor(300000000.*baseb_fm_demod[baseb_pn]);
      baseb_pn=(baseb_pn+1)&baseband_mask;
      basebrawnet_pa=(basebrawnet_pa+4)&basebrawnet_mask;
      }
#endif
#if NET_BASEBRAW_MODE == WFM_AM_FIRST_LOWPASS
    int *ntbuf;
    while(baseb_pn != baseb_pa && 
     ((basebrawnet_px-basebrawnet_pa+basebrawnet_mask)&basebrawnet_mask) >16)
      {
      ntbuf=(int*)&basebraw_netsend_buffer[basebrawnet_pa];
      ntbuf[0]=floor(0.2*sqrt(baseb_carrier_ampl[baseb_pn]));
      baseb_pn=(baseb_pn+1)&baseband_mask;
      basebrawnet_pa=(basebrawnet_pa+4)&basebrawnet_mask;
      }
#endif
#if NET_BASEBRAW_MODE == WFM_FM_FIRST_LOWPASS_RESAMP
    int *ntbuf;
    while(fm1_px != fm1_pa && 
     ((basebrawnet_px-basebrawnet_pa+basebrawnet_mask)&basebrawnet_mask) >16)
      {
      ntbuf=(int*)&basebraw_netsend_buffer[basebrawnet_pa];
      ntbuf[0]=floor(fm1_all[fm1_px]);
//        baseb_pn=(baseb_pn+1)&baseband_mask;
      fm1_px=(fm1_px+1)&fm1_mask;
      basebrawnet_pa=(basebrawnet_pa+4)&basebrawnet_mask;
      }
#endif
#if NET_BASEBRAW_MODE == BASEBAND_IQ_TEST
    int *ntbuf;
    if(ui.rx_rf_channels == 1)
      {
      while(baseb_pn != baseb_pa && 
        ((basebrawnet_px-basebrawnet_pa+basebrawnet_mask)&basebrawnet_mask) >16)
        {
        ntbuf=(int*)&basebraw_netsend_buffer[basebrawnet_pa];
        ntbuf[0]=basebraw_test_cnt1;
        ntbuf[1]=basebraw_test_cnt1+256;
        baseb_pn=(baseb_pn+1)&baseband_mask;
        basebrawnet_pa=(basebrawnet_pa+8)&basebrawnet_mask;
        basebraw_test_cnt1+=2*256;
        }
      }  
    else
      {    
      DEB"\nSET:%d\n",basebrawnet_pa/16);
      while(baseb_pn != baseb_pa && 
        ((basebrawnet_px-basebrawnet_pa+basebrawnet_mask)&basebrawnet_mask) >16)
        {
        ntbuf=(int*)&basebraw_netsend_buffer[basebrawnet_pa];
        ntbuf[0]=basebraw_test_cnt1;
        ntbuf[1]=basebraw_test_cnt1+256;
        ntbuf[2]=basebraw_test_cnt1+512;
        ntbuf[3]=basebraw_test_cnt1+768;
        DEB"[%d %d]",ntbuf[0]/256,basebrawnet_pa/16);
        baseb_pn=(baseb_pn+1)&baseband_mask;
        basebrawnet_pa=(basebrawnet_pa+16)&basebrawnet_mask;
        basebraw_test_cnt1+=4*256;

        }
      }  
#endif
#if NET_BASEBRAW_MODE == BASEBAND_IQ_MASTER_TEST
    int *ntbuf;
    if(ui.rx_rf_channels == 1)
      {
      while(baseb_pn != baseb_pa && 
        ((basebrawnet_px-basebrawnet_pa+basebrawnet_mask)&basebrawnet_mask) >16)
        {
        ntbuf=(int*)&basebraw_netsend_buffer[basebrawnet_pa];
        ntbuf[0]=floor(0.05*baseb_raw[2*baseb_pn  ]);
        ntbuf[1]=floor(0.05*baseb_raw[2*baseb_pn+1]);
        if(baseb_raw[2*baseb_pn  ] !=basebraw_test_cnt2)
          {
          fprintf( stderr,"*");
          DEB"\nERROR");
          }
        if(baseb_raw[2*baseb_pn+1] !=basebraw_test_cnt2+256)
          {
          fprintf( stderr,"!");
          DEB"\nERROR");
          }
        basebraw_test_cnt2=baseb_raw[2*baseb_pn  ]+1024;
        DEB"\npn=%d  %f %f",baseb_pn,baseb_raw[2*baseb_pn  ]/256,
                                           baseb_raw[2*baseb_pn+1]/256);
        baseb_pn=(baseb_pn+1)&baseband_mask;
        basebrawnet_pa=(basebrawnet_pa+8)&basebrawnet_mask;
        }
      }  
    else
      {
      while(baseb_pn != baseb_pa && 
                 ((basebrawnet_px-basebrawnet_pa+basebrawnet_mask)&
                                                   basebrawnet_mask) >16)
        {
        ntbuf=(int*)&basebraw_netsend_buffer[basebrawnet_pa];
        ntbuf[0]=floor(0.05*baseb_raw[2*baseb_pn  ]);
        ntbuf[1]=floor(0.05*baseb_raw[2*baseb_pn+1]);
        ntbuf[2]=floor(0.05*baseb_raw_orthog[2*baseb_pn  ]);
        ntbuf[3]=floor(0.05*baseb_raw_orthog[2*baseb_pn+1]);
        if(baseb_raw[2*baseb_pn  ] !=basebraw_test_cnt2)
          {
          fprintf( stderr,"*");
          DEB"\nERROR");
          }
        if(baseb_raw[2*baseb_pn+1] !=basebraw_test_cnt2+256)
          {
          fprintf( stderr,"!");
          DEB"\nERROR");
          }
        if(baseb_raw_orthog[2*baseb_pn] !=basebraw_test_cnt2+512)
          {
          fprintf( stderr,"@");
          DEB"\nERROR");
          }
        if(baseb_raw_orthog[2*baseb_pn+1] !=basebraw_test_cnt2+768)
          {
          fprintf( stderr,"#");
          DEB"\nERROR");
          }
        basebraw_test_cnt2=baseb_raw[2*baseb_pn  ]+1024;
        DEB"\npn=%d  net_pa%d %f %f %f %f",baseb_pn,basebrawnet_pa,
                                    baseb_raw[2*baseb_pn  ]/256,
                                    baseb_raw[2*baseb_pn+1]/256,
                                    baseb_raw_orthog[2*baseb_pn  ]/256,
                                    baseb_raw_orthog[2*baseb_pn+1]/256);
        baseb_pn=(baseb_pn+1)&baseband_mask;
        basebrawnet_pa=(basebrawnet_pa+16)&basebrawnet_mask;
        }
      }  
#endif
#if NET_BASEBRAW_MODE == BASEBAND_IQ
    int *ntbuf;
    int pa;
    pa=basebrawnet_pa;
    if(ui.rx_rf_channels == 1)
      {
      while(baseb_pn != baseb_pa && 
        ((basebrawnet_px-pa+basebrawnet_mask)&basebrawnet_mask) >16)
        {
        ntbuf=(int*)&basebraw_netsend_buffer[pa];
        ntbuf[0]=floor(0.05*baseb_raw[2*baseb_pn  ]);
        ntbuf[1]=floor(0.05*baseb_raw[2*baseb_pn+1]);
        baseb_pn=(baseb_pn+1)&baseband_mask;
        pa=(pa+8)&basebrawnet_mask;
        }
      }  
    else
      {    
      while(baseb_pn != baseb_pa && 
        ((basebrawnet_px-pa+basebrawnet_mask)&basebrawnet_mask) >16)
        {
        ntbuf=(int*)&basebraw_netsend_buffer[pa];
        ntbuf[0]=floor(0.05*baseb_raw[2*baseb_pn  ]);
        ntbuf[1]=floor(0.05*baseb_raw[2*baseb_pn+1]);
        ntbuf[2]=floor(0.05*baseb_raw_orthog[2*baseb_pn  ]);
        ntbuf[3]=floor(0.05*baseb_raw_orthog[2*baseb_pn+1]);
        baseb_pn=(baseb_pn+1)&baseband_mask;
        pa=(pa+16)&basebrawnet_mask;
        }
      }  
    lir_sched_yield();
    basebrawnet_pa=pa;  
#endif
#if NET_BASEBRAW_MODE == BASEBAND_IQ_FULL_BANDWIDTH 
    int *ntbuf;
    float *z;
    if(ui.rx_rf_channels == 1)
      {
      while( ((timf3_pa-timf3_pn+timf3_size)&timf3_mask) > timf3_block && 
        ((basebrawnet_px-basebrawnet_pa+basebrawnet_mask)&basebrawnet_mask) >
                         2*(int)timf3_block)
        {
        k=timf3_block;
        z=&timf3_float[timf3_pn];
        for(i=0; i<k; i+=2)
          {
          ntbuf=(int*)&basebraw_netsend_buffer[basebrawnet_pa];
          ntbuf[0]=floor(z[i  ]);
          ntbuf[1]=floor(z[i+1]);
          basebrawnet_pa=(basebrawnet_pa+8)&basebrawnet_mask;
          }
        timf3_pn=(timf3_pn+timf3_block)&timf3_mask;
        }
      }  
    else
      {
      while( ((timf3_pa-timf3_pn+timf3_size)&timf3_mask) > timf3_block && 
        ((basebrawnet_px-basebrawnet_pa+basebrawnet_mask)&basebrawnet_mask) >
                         4*(int)timf3_block)
        {
        k=timf3_block;
        z=&timf3_float[timf3_pn];
        for(i=0; i<k; i+=4)
          {
          ntbuf=(int*)&basebraw_netsend_buffer[basebrawnet_pa];
          ntbuf[0]=floor(pg.c1*z[i  ]+pg.c2*z[i+2]+pg.c3*z[i+3]);
          ntbuf[1]=floor(pg.c1*z[i+1]+pg.c2*z[i+3]-pg.c3*z[i+2]);
          ntbuf[2]=floor(pg.c1*z[i+2]-pg.c2*z[i  ]+pg.c3*z[i+1]);
          ntbuf[3]=floor(pg.c1*z[i+3]-pg.c2*z[i+1]-pg.c3*z[i  ]);
          basebrawnet_pa=(basebrawnet_pa+16)&basebrawnet_mask;
          }
        timf3_pn=(timf3_pn+timf3_block)&timf3_mask;
        }
      }  
#endif
#if NET_BASEBRAW_MODE == WFM_SYNTHETIZED_FROM_FIRST_LOWPASS || \
    NET_BASEBRAW_MODE == WFM_SUBTRACT_FROM_FIRST_LOWPASS
    int *ntbuf;
    while(baseb_pn != baseb_pa && 
     ((basebrawnet_px-basebrawnet_pa+basebrawnet_mask)&basebrawnet_mask) >16)
      {
      ntbuf=(int*)&basebraw_netsend_buffer[basebrawnet_pa];
      for(i=0; i<basebraw_ad_channels; i++)
        {
        ntbuf[i]=floor(0.05*baseb_carrier[basebraw_ad_channels*baseb_pn+i]);
        }
      baseb_pn=(baseb_pn+1)&baseband_mask;
      basebrawnet_pa=(basebrawnet_pa+4*basebraw_ad_channels)&basebrawnet_mask;
      }
#endif
#if NET_BASEBRAW_MODE == WFM_AM_FULL_BANDWIDTH
    int *ntbuf;
    while(baseb_pn != baseb_pa && 
     ((basebrawnet_px-basebrawnet_pa+basebrawnet_mask)&basebrawnet_mask) >16)
      {
      ntbuf=(int*)&basebraw_netsend_buffer[basebrawnet_pa];
      ntbuf[0]=0.1*floor(sqrt(baseb_totpwr[baseb_pn]));
      baseb_pn=(baseb_pn+1)&baseband_mask;
      basebrawnet_pa=(basebrawnet_pa+4)&basebrawnet_mask;
      }
#endif
    }  
  if(audio_dump_flag == 1 || rx_audio_out == -1)
    {
    while( ((daout_pa-daout_px+daout_size)&daout_bufmask) > 
                                                 snd[RXDA].block_bytes)
      {
      daout_px=(daout_px+snd[RXDA].block_bytes)&daout_bufmask;
      }
    }
  lir_sched_yield();
  if(kill_all_flag) goto exit2;
  if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_IDLE)goto idle;
  if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_KILL)goto exit2;
  while(daout_px != daout_py)
    {
    if(wav_write_flag != 0)
      {
      if(wavfile_squelch_flag == 0 || squelch_on)
        {
        if(fwrite(&daout[daout_py],snd[RXDA].block_bytes,1,wav_file)!=1)
                                                        wavsave_start_stop(0);
        }
      }
    da_block_counter+=1;
    daout_py=(daout_py+snd[RXDA].block_bytes)&daout_bufmask;
    }
#if BUFBARS == TRUE
  if(baseband_handle != NULL && timinfo_flag!=0)
    {
    i=(daout_pa+2-daout_px+daout_bufmask+1)&daout_bufmask;
    i/=daout_indicator_block;
    show_bufbar(RX_BUFBAR_DAOUT,i);
    }
#endif
  }
if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_KILL)goto exit2;
idle:;
if(thread_command_flag[THREAD_RX_OUTPUT] != THRFLAG_IDLE)
  {
  lirerr(333412);
  goto exit2;
  }
thread_status_flag[THREAD_RX_OUTPUT] = THRFLAG_IDLE;
if(!blocking)
  {
  if(rx_audio_out != -1)
    {
    sys_func(THRFLAG_CLOSE_RX_SNDOUT);
    while(rx_audio_out != -1)
      {
      lir_sleep(100);
      }
    }
  }    
else
  {
  while(thread_status_flag[THREAD_BLOCKING_RXOUT] == THRFLAG_INIT)
    {
    lir_sched_yield();
    lir_sleep(100);
    }
  if(thread_status_flag[THREAD_BLOCKING_RXOUT] != THRFLAG_IDLE)
    {
    thread_command_flag[THREAD_BLOCKING_RXOUT] = THRFLAG_IDLE;
    i=0;
    if(thread_status_flag[THREAD_BLOCKING_RXOUT] == THRFLAG_BLOCKING)
      {
      lir_set_event(EVENT_BLOCKING_RXOUT);
      }
    while(thread_status_flag[THREAD_BLOCKING_RXOUT] != THRFLAG_IDLE)
      {
      i++;
      lir_sched_yield();
      lir_sleep(3000);
      if(kill_all_flag)goto exit2;
      if(i>500)
        {
        lirerr(776123);
        goto exit2;
        }
      }  
    }
  }
while(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_IDLE)
  {
  lir_sleep(10000);
  if(kill_all_flag ||
            thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_KILL)goto exit2;
  }
if(thread_command_flag[THREAD_RX_OUTPUT] != THRFLAG_ACTIVE)goto exit2;
if(!blocking)
  {
  sys_func(THRFLAG_OPEN_RX_SNDOUT);
  i=0;
  while(rx_audio_out == -1)
    {
    if(kill_all_flag || 
         thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_KILL)goto exit2;
    if(thread_command_flag[THREAD_RX_OUTPUT] != THRFLAG_ACTIVE)
      {
      lirerr(679375);
      goto exit2;
      }
    lir_sleep(20);
    i++;
    if(i>100000)
      {
      lirerr(1017);
      goto exit2;
      }
    }
  snd[RXDA].open_flag=CALLBACK_CMD_ACTIVE;
  if(kill_all_flag)goto exit2;
  daout_pa=0;
  daout_px=0;
  daout_py=0;
  if(rx_audio_out >= 0)lir_empty_da_device_buffer();  
  thread_status_flag[THREAD_RX_OUTPUT] = THRFLAG_ACTIVE;
  goto resume;
  }  
else 
  {
  if(thread_status_flag[THREAD_BLOCKING_RXOUT] == THRFLAG_BLOCKING)goto run;
  if(thread_status_flag[THREAD_BLOCKING_RXOUT] == THRFLAG_IDLE)
    {
    thread_command_flag[THREAD_BLOCKING_RXOUT] = THRFLAG_ACTIVE;
    lir_set_event(EVENT_BLOCKING_RXOUT);
    i=0;
    while(thread_status_flag[THREAD_BLOCKING_RXOUT] != THRFLAG_BLOCKING)
      {
      i++;
      lir_sleep(10);
      if(i>1000)
        {
        lirerr(388421);
        goto exit2;
        }
      }
    lir_set_event(EVENT_BLOCKING_RXOUT);
    i=0;
    while(thread_status_flag[THREAD_BLOCKING_RXOUT] != THRFLAG_ACTIVE)
      {
      i++;
      lir_sleep(3000);
      if(i>300)
        {
        lirerr(388421);
        goto exit2;
        }
      }
run:;
    daout_pa=0;
    daout_px=0;
    daout_py=0;
    lir_set_event(EVENT_BLOCKING_RXOUT);
    thread_status_flag[THREAD_RX_OUTPUT] = THRFLAG_ACTIVE;
    goto resume;
    }
  else
    {
    lirerr(683327);
    goto exit2;
    }
  }
exit2:;
if(!blocking)
  {
  if(rx_audio_out != -1)
    {
    sys_func(THRFLAG_CLOSE_RX_SNDOUT);
    while(rx_audio_out != -1)
      {
      lir_sleep(100);
      }
    }
  }    

thread_status_flag[THREAD_RX_OUTPUT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_RX_OUTPUT] != THRFLAG_NOT_ACTIVE &&
      thread_command_flag[THREAD_RX_OUTPUT] != THRFLAG_KILL)
  {
  lir_sleep(10000);
  }
}
// MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
//
//                     make audio_signal
//
// MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM

void make_audio_signal(void)
{
int i, k, i1, i2, i3, i4, mm, nn;
double t1, t2, t3, t4, t5, t6, t7;
double a1, a2, b1, b2, u1, u2;
double dt1;
short int *intvar;
short int *ntbuf;
double r1,r2,r3,r4;
double rdiff, final_gain1, final_gain2;
double baseb_r1, baseb_r2;
int loop_counter;
if(!allow_audio)return;
audio_status=TRUE;
lir_sched_yield();
if(ui.tx_enable != 0)
  {
  dt1=(2*PI_L*(double)txcw.sidetone_freq)/genparm[DA_OUTPUT_SPEED]; 
  sidetone_phstep_cos=cos(dt1);
  sidetone_phstep_sin=sin(dt1);
  dt1=sqrt(sidetone_cos*sidetone_cos+sidetone_sin*sidetone_sin);
  sidetone_cos*=txcw.sidetone_ampl/dt1;
  sidetone_sin*=txcw.sidetone_ampl/dt1;
  }                      
i=1;
if(bg_filter_points > 5 && 2*bg_filter_points < bg_xpoints)
  {
  if(bg.squelch_level !=0)
    {
    if(recent_time-squelch_turnon_time > bg.squelch_time+0.1)
      {
      i=0;
      }
    }
  }
squelch_on=i;
// Resample the baseband signal so we get the correct sampling speed
// for the D/A converter.
// For each interval in baseb_out there is a non integer number of points
// in da_output.
dt1=1/sqrt(rx_daout_sin*rx_daout_sin+rx_daout_cos*rx_daout_cos);
rx_daout_sin*=dt1;
rx_daout_cos*=dt1;
final_gain1=final_gain2=0;
mm=snd[RXDA].framesize;
nn=2*baseb_channels;
baseb_r1=daout_pa/(mm*da_resample_ratio);
loop_counter=0;
//**************************************************************
resamp:;
baseb_r2=(daout_pa+mm)/(mm*da_resample_ratio);
i2=baseb_fx+baseb_r1+.5;
if(baseb_r2>baseb_r1)
  {
  i3=baseb_fx+baseb_r2+.5;
  }
else
  {
  i3=baseb_fx+baseb_r2+.5+daout_size/da_resample_ratio;
  }  
i2&=baseband_mask;
i3&=baseband_mask;
if(abs(i3-i2) > 1)
  {
  i2=baseb_fx+(baseb_r1+baseb_r2)/2;
  i3=i2+1;
  i2&=baseband_mask;
  i3&=baseband_mask;
  }
else
  {
  if(i3==i2)
    {
    i2=baseb_fx+baseb_r1;
    i2&=baseband_mask;
    if(i3==i2)
      {
      i3=i2+1;
      i3&=baseband_mask;
      }
    }
  }        
i4=(i3+1)&baseband_mask;
baseb_py=i4;
i1=(i2+baseband_mask)&baseband_mask;
loop_counter++;
if(loop_counter > 2*baseb_output_block)lir_sched_yield();
if( ((daout_pa-daout_px+daout_size)&daout_bufmask) > (daout_size>>1) ||
     ((baseb_pa-baseb_py+baseband_size)&baseband_mask) <= baseb_min_block)
  {
  audio_status=FALSE;
  return;  
  }
rdiff=baseb_r1+baseb_fx-i2;
if(rdiff > baseband_size/2)
  {
  rdiff-=baseband_size;
  }
// Use Lagrange's interpolation formula to fit a third degree
// polynomial to 4 points:
//  a1=-rdiff *   (rdiff-1)*(rdiff-2)*baseb_out[nn*i1]/6
//     +(rdiff+1)*(rdiff-1)*(rdiff-2)*baseb_out[nn*i2]/2
//     -(rdiff+1)* rdiff   *(rdiff-2)*baseb_out[nn*i3]/2
//     +(rdiff+1)* rdiff   *(rdiff-1)*baseb_out[nn*i4]/6; 
// Rewrite slightly to save a few multiplications - do not
// think the compiler is smart enough to do it for us.
t1=rdiff-1;
t2=rdiff-2;
t3=rdiff+1;
t4=t1*t2;
t5=t3*rdiff;
t6=rdiff*t4;
t4=t3*t4;
t7=t5*t2;
t5=t5*t1;
final_gain1=daout_gain;
final_gain2=daout_gain;
if(bg.agc_flag == 1)
  {
  if( daout_gain*baseb_agc_level[i2] > bg_agc_amplimit)
    {
    final_gain1=bg_agc_amplimit/baseb_agc_level[i2];
    final_gain2=final_gain1;
    }
  }    
if(bg.agc_flag == 2)
  {
  if( daout_gain*baseb_agc_level[2*i2] > bg_agc_amplimit)
    {
    final_gain1=bg_agc_amplimit/baseb_agc_level[2*i2];
    }
  if( daout_gain*baseb_agc_level[2*i2+1] > bg_agc_amplimit)
    {
    final_gain2=bg_agc_amplimit/baseb_agc_level[2*i2+1];
    }
  }
if(squelch_on == 0)
  {
  final_gain1=0;
  final_gain2=0;
  }      
a1=final_gain1*(((t5*baseb_out[nn*i4  ]-t6*baseb_out[nn*i1  ])/3
                    +t4*baseb_out[nn*i2  ]-t7*baseb_out[nn*i3  ])/2);
a2=final_gain1*(((t5*baseb_out[nn*i4+1]-t6*baseb_out[nn*i1+1])/3
                    +t4*baseb_out[nn*i2+1]-t7*baseb_out[nn*i3+1])/2);
if(enable_resamp_iir5)
  {
  xva1[0]=xva1[1];
  xva1[1]=xva1[2];
  xva1[2]=xva1[3];
  xva1[3]=xva1[4]; 
  xva1[4]=xva1[5];
  xva1[5]=a1/iir_a.gain;
  yva1[0]=yva1[1];
  yva1[1]=yva1[2];
  yva1[2]=yva1[3];
  yva1[3]=yva1[4]; 
  yva1[4]=yva1[5];
  yva1[5]=xva1[0]+xva1[5]+5*(xva1[1]+xva1[4])+10*(xva1[2]+xva1[3])+
          iir_a.c0*yva1[0]+iir_a.c1*yva1[1]+iir_a.c2*yva1[2]+
          iir_a.c3 * yva1[3]+iir_a.c4*yva1[4];
  a1=yva1[5];
  xva2[0]=xva2[1];
  xva2[1]=xva2[2];
  xva2[2]=xva2[3];
  xva2[3]=xva2[4]; 
  xva2[4]=xva2[5];
  xva2[5]=a2/iir_a.gain;
  yva2[0]=yva2[1];
  yva2[1]=yva2[2];
  yva2[2]=yva2[3];
  yva2[3]=yva2[4];
  yva2[4]=yva2[5];
  yva2[5]=xva2[0]+xva2[5]+5*(xva2[1] + xva2[4])+10*(xva2[2]+xva2[3])+
          iir_a.c0*yva2[0]+iir_a.c1*yva2[1]+iir_a.c2*yva2[2]+
          iir_a.c3*yva2[3]+iir_a.c4*yva2[4];
  a2=yva2[5];

  }
old_out1=out1;
old_out2=out2;  
if(baseb_channels == 1)
  {
  if( (ui.network_flag&NET_RXOUT_BASEB) != 0)
    {
    ntbuf=(short int*)&baseb_netsend_buffer[basebnet_pa];
    if(rx_daout_bytes == 1)
      {
      ntbuf[0]=floor(256*a2);
      ntbuf[1]=floor(256*a1);
      }
    else
      {
      ntbuf[0]=floor(a2);
      ntbuf[1]=floor(a1);        
      }
    basebnet_pa=(basebnet_pa+4)&basebnet_mask;
    }
  if(rx_mode == MODE_FM)
    {
    out1=a1;
    out2=a2;
// ************************************
    t1=(out1-old_out1)/daout_upsamp;
    t2=old_out1;
    t3=(out2-old_out2)/daout_upsamp;
    t4=old_out2;
    for(i=0; i<daout_upsamp; i++)
      {
      xv1[0]=xv1[1]; 
      xv1[1]=xv1[2]; 
      xv1[2]=xv1[3]; 
      xv1[3]=t2/iir3_gain[daout_upsamp_n];
      yv1[0]=yv1[1]; 
      yv1[1]=yv1[2]; 
      yv1[2]=yv1[3]; 
      yv1[3]=xv1[0]+xv1[3]+3*(xv1[1]+xv1[2])+
              iir3_c0[daout_upsamp_n]*yv1[0]+
              iir3_c1[daout_upsamp_n]*yv1[1]+
              iir3_c2[daout_upsamp_n]*yv1[2];
      a1 = yv1[3];
      xv2[0]=xv2[1]; 
      xv2[1]=xv2[2]; 
      xv2[2]=xv2[3]; 
      xv2[3]=t4/iir3_gain[daout_upsamp_n];
      yv2[0]=yv2[1]; 
      yv2[1]=yv2[2]; 
      yv2[2]=yv2[3]; 
      yv2[3]=xv2[0]+xv2[3]+3*(xv2[1]+xv2[2])+
                    iir3_c0[daout_upsamp_n]*yv2[0]+
                    iir3_c1[daout_upsamp_n]*yv2[1]+
                    iir3_c2[daout_upsamp_n]*yv2[2];
      a2 = yv2[3];
      if(a1 < -bg_amplimit)a1=-bg_amplimit;
      if(a1 > bg_amplimit)a1=bg_amplimit;
      if(bg_maxamp < fabs(a1))bg_maxamp=fabs(a1); 
      if(a2 < -bg_amplimit)a2=-bg_amplimit;
      if(a2 > bg_amplimit)a2=bg_amplimit;
      if(bg_maxamp < fabs(a2))bg_maxamp=fabs(a2); 
      if(ptt_state == FALSE)
        {
        if(rx_daout_bytes == 1)
          {
          daout[daout_pa]=0x80+a1;
          if(rx_daout_channels == 2)daout[daout_pa+1]=0x80+a2;
          }
        else
          {  
          intvar=(short int*)(&daout[daout_pa]);
          intvar[0]=a1;
          if(rx_daout_channels == 2)intvar[1]=a2;
          }
        }  
      else    
        {
        dt1=sidetone_cos;
        sidetone_cos=sidetone_cos*sidetone_phstep_cos+
                     sidetone_sin*sidetone_phstep_sin;
        sidetone_sin=sidetone_sin*sidetone_phstep_cos-
                              dt1*sidetone_phstep_sin;
        if(rx_daout_bytes == 1)
          {
          k=(int)(sidetone_cos*0x7e/1000.);
          daout[daout_pa]=0x80+k;
          if(rx_daout_channels == 2)daout[daout_pa+1]=0x80+k;
          }
        else
          {
          intvar=(short int*)(&daout[daout_pa]);
          intvar[0]=(int)(sidetone_cos*0x7ffe/1000.);
          if(rx_daout_channels == 2)intvar[1]=intvar[0];
          }
        }  
      daout_pa=(daout_pa+mm)&daout_bufmask;
      t2+=t1;
      t4+=t3;
      }
    }
  else
    {
    if(bg_expand != 2)
      {
      t1=sqrt(a1*a1+a2*a2);
      if(t1 > 0.5)
        {
        if(t1<bg_amplimit)t2=t1; else t2=bg_amplimit;
        if(bg_expand == 1)t2=bg_expand_a*(exp(bg_expand_b*t2)-1);
        t2/=t1;
        out1=t2*a1;
        out2=t2*a2;
        }
      else
        {
        out1=0;
        out2=0;
        }
      }
    else
      {
      out1=a1;
      out2=a2;
      }  
    t1=(out1-old_out1)/daout_upsamp;
    t2=old_out1;      
    t3=(out2-old_out2)/daout_upsamp;
    t4=old_out2;
    for(i=0; i<daout_upsamp; i++)
      {
      dt1=rx_daout_cos;
      rx_daout_cos=rx_daout_cos*rx_daout_phstep_cos+
                   rx_daout_sin*rx_daout_phstep_sin;
      rx_daout_sin=rx_daout_sin*rx_daout_phstep_cos-
                            dt1*rx_daout_phstep_sin;
      xv1[0]=xv1[1]; 
      xv1[1]=xv1[2]; 
      xv1[2]=xv1[3]; 
      xv1[3]=t2/iir3_gain[daout_upsamp_n];
      yv1[0]=yv1[1]; 
      yv1[1]=yv1[2]; 
      yv1[2]=yv1[3]; 
      yv1[3]=xv1[0]+xv1[3]+3*(xv1[1]+xv1[2])+
              iir3_c0[daout_upsamp_n]*yv1[0]+
              iir3_c1[daout_upsamp_n]*yv1[1]+
              iir3_c2[daout_upsamp_n]*yv1[2];
      a1 = yv1[3];
      xv2[0]=xv2[1]; 
      xv2[1]=xv2[2]; 
      xv2[2]=xv2[3]; 
      xv2[3]=t4/iir3_gain[daout_upsamp_n];
      yv2[0]=yv2[1]; 
      yv2[1]=yv2[2]; 
      yv2[2]=yv2[3]; 
      yv2[3]=xv2[0]+xv2[3]+3*(xv2[1]+xv2[2])+
                    iir3_c0[daout_upsamp_n]*yv2[0]+
                    iir3_c1[daout_upsamp_n]*yv2[1]+
                    iir3_c2[daout_upsamp_n]*yv2[2];
      a2 = yv2[3];
      u1= a1*rx_daout_cos+a2*rx_daout_sin;
      if(u1 > bg_amplimit)u1=bg_amplimit;
      if(u1 < -bg_amplimit)u1=-bg_amplimit;
      if(bg_maxamp < fabs(u1))bg_maxamp=fabs(u1); 
      if(da_ch2_sign == 0)
        {          
        u2=-a1*rx_daout_sin+a2*rx_daout_cos;
        if(u2 > bg_amplimit)u2=bg_amplimit;
        if(u2 < -bg_amplimit)u2=-bg_amplimit;
        if(bg_maxamp < fabs(u2))bg_maxamp=fabs(u2); 
        if(ptt_state == FALSE)
          {
          if(rx_daout_bytes == 1)
            {
            daout[daout_pa  ]=0x80+u1;
            daout[daout_pa+1]=0x80+u2;
            }
          else
            {  
            intvar=(short int*)(&daout[daout_pa]);
            intvar[0]=u1;
            intvar[1]=u2;
            }
          }
        else    
          {
          dt1=sidetone_cos;
          sidetone_cos=sidetone_cos*sidetone_phstep_cos+
                       sidetone_sin*sidetone_phstep_sin;
          sidetone_sin=sidetone_sin*sidetone_phstep_cos-
                                dt1*sidetone_phstep_sin;
          if(rx_daout_bytes == 1)
            {
            k=(int)(sidetone_cos*0x7e/1000.);
            daout[daout_pa]=0x80+k;
            if(rx_daout_channels == 2)daout[daout_pa+1]=0x80+k;
            }
          else
            {
            intvar=(short int*)(&daout[daout_pa]);
            intvar[0]=(int)(sidetone_cos*0x7ffe/1000.);
            if(rx_daout_channels == 2)intvar[1]=intvar[0];
            }
          }
        }  
      else
        {
        if(ptt_state == FALSE)
          {
          if(rx_daout_bytes == 1)
            {
            daout[daout_pa]=0x80+u1;
            if(rx_daout_channels == 2)daout[daout_pa+1]=0x80+u1*da_ch2_sign;
            }
          else
            {  
            intvar=(short int*)(&daout[daout_pa]);
            intvar[0]=u1;
            if(rx_daout_channels == 2)intvar[1]=u1*da_ch2_sign;
            }
          }
        else    
          {
          dt1=sidetone_cos;
          sidetone_cos=sidetone_cos*sidetone_phstep_cos+
                       sidetone_sin*sidetone_phstep_sin;
          sidetone_sin=sidetone_sin*sidetone_phstep_cos-
                                dt1*sidetone_phstep_sin;
          if(rx_daout_bytes == 1)
            {
            k=(int)(sidetone_cos*0x7e/1000.);
            daout[daout_pa]=0x80+k;
            if(rx_daout_channels == 2)daout[daout_pa+1]=0x80+k;
            }
          else
            {
            intvar=(short int*)(&daout[daout_pa]);
            intvar[0]=(int)(sidetone_cos*0x7ffe/1000.);
            if(rx_daout_channels == 2)intvar[1]=intvar[0];
            }
          }
        }
      daout_pa=(daout_pa+mm)&daout_bufmask;
      t2+=t1;
      t4+=t3;
      }
    }
  }
else
  {
  old_out3=out3;
  old_out4=out4;
  b1=final_gain2*(((t5*baseb_out[nn*i4+2]-t6*baseb_out[nn*i1+2])/3
                    +t4*baseb_out[nn*i2+2]-t7*baseb_out[nn*i3+2])/2);
  b2=final_gain2*(((t5*baseb_out[nn*i4+3]-t6*baseb_out[nn*i1+3])/3
                    +t4*baseb_out[nn*i2+3]-t7*baseb_out[nn*i3+3])/2);
  if(enable_resamp_iir5)
    {
    xvb1[0]=xvb1[1];
    xvb1[1]=xvb1[2];
    xvb1[2]=xvb1[3];
    xvb1[3]=xvb1[4]; 
    xvb1[4]=xvb1[5];
    xvb1[5]=b1/iir_a.gain;
    yvb1[0]=yvb1[1];
    yvb1[1]=yvb1[2];
    yvb1[2]=yvb1[3];
    yvb1[3]=yvb1[4]; 
    yvb1[4]=yvb1[5];
    yvb1[5]=xvb1[0]+xvb1[5]+5*(xvb1[1]+xvb1[4])+10*(xvb1[2]+xvb1[3])+
            iir_a.c0*yvb1[0]+iir_a.c1*yvb1[1]+iir_a.c2*yvb1[2]+
            iir_a.c3 * yvb1[3]+iir_a.c4*yvb1[4];
    b1=yvb1[5];
    xvb2[0]=xvb2[1];
    xvb2[1]=xvb2[2];
    xvb2[2]=xvb2[3];
    xvb2[3]=xvb2[4];
    xvb2[4]=xvb2[5];
    xvb2[5]=b2/iir_a.gain;
    yvb2[0]=yvb2[1];
    yvb2[1]=yvb2[2];
    yvb2[2]=yvb2[3];
    yvb2[3]=yvb2[4];
    yvb2[4]=yvb2[5];
    yvb2[5]=xvb2[0]+xvb2[5]+5*(xvb2[1] + xvb2[4])+10*(xvb2[2]+xvb2[3])+
            iir_a.c0*yvb2[0]+iir_a.c1*yvb2[1]+iir_a.c2*yvb2[2]+
            iir_a.c3*yvb2[3]+iir_a.c4*yvb2[4];
    b2=yvb2[5];
    }
  if( (ui.network_flag&NET_RXOUT_BASEB) != 0)
    {
    ntbuf=(short int*)&baseb_netsend_buffer[basebnet_pa];
    if(rx_mode == MODE_AM || rx_mode == MODE_FM)
      {
      if(rx_daout_bytes == 1)
        {
        ntbuf[0]=floor(256*a1);
        ntbuf[1]=floor(256*b1);
        }
      else
        {
        ntbuf[0]=floor(a1);        
        ntbuf[1]=floor(b1);        
        }
      basebnet_pa=(basebnet_pa+4)&basebnet_mask;
      }
    else
      {      
      if(rx_daout_bytes == 1)
        {
        ntbuf[0]=floor(256*a2);
        ntbuf[1]=floor(256*a1);
        ntbuf[2]=floor(256*b2);
        ntbuf[3]=floor(256*b1);
        }
      else
        {
        ntbuf[0]=floor(a2);
        ntbuf[1]=floor(a1);        
        ntbuf[2]=floor(b2);
        ntbuf[3]=floor(b1);        
        }
      basebnet_pa=(basebnet_pa+8)&basebnet_mask;
      }
    }
  if( (bg_expand != 2 && bg.agc_flag != 2 ) || bg_twopol != 0)
    {
    t1=a1*a1+a2*a2;
    t2=b1*b1+b2*b2;
    if(t1 < t2)t1=t2;
    t1=sqrt(t1);
    if(t1 > 0.5)
      {
      if(t1<bg_amplimit)t2=t1; else t2=bg_amplimit;
      if(bg_expand == 1)t2=bg_expand_a*(exp(bg_expand_b*t2)-1);
      if(bg_maxamp < t2)bg_maxamp=t2; 
      t2/=t1;
      out1=t2*a1;
      out2=t2*a2;
      out3=t2*b1;
      out4=t2*b2;
      }
    else
      {
      out1=0;
      out2=0;
      out3=0;
      out4=0;
      }  
    }
  else
    {
    out1=a1;
    out2=a2;
    out3=b1;
    out4=b2;
    }
// ************************************
  t1=(out1-old_out1)/daout_upsamp;
  t2=old_out1;
  t3=(out2-old_out2)/daout_upsamp;
  t4=old_out2;
  r1=(out3-old_out3)/daout_upsamp;
  r2=old_out3;
  r3=(out4-old_out4)/daout_upsamp;
  r4=old_out4;
  for(i=0; i<daout_upsamp; i++)
    {
    dt1=rx_daout_cos;
    rx_daout_cos=rx_daout_cos*rx_daout_phstep_cos+
                 rx_daout_sin*rx_daout_phstep_sin;
    rx_daout_sin=rx_daout_sin*rx_daout_phstep_cos-
                        dt1*rx_daout_phstep_sin;
    xv1[0]=xv1[1]; 
    xv1[1]=xv1[2]; 
    xv1[2]=xv1[3]; 
    xv1[3]=t2/iir3_gain[daout_upsamp_n];
    yv1[0]=yv1[1]; 
    yv1[1]=yv1[2]; 
    yv1[2]=yv1[3]; 
    yv1[3]=xv1[0]+xv1[3]+3*(xv1[1]+xv1[2])+
                     iir3_c0[daout_upsamp_n]*yv1[0]+
                     iir3_c1[daout_upsamp_n]*yv1[1]+
                     iir3_c2[daout_upsamp_n]*yv1[2];
    a1 = yv1[3];
    xv2[0]=xv2[1]; 
    xv2[1]=xv2[2]; 
    xv2[2]=xv2[3]; 
    xv2[3]=t4/iir3_gain[daout_upsamp_n];
    yv2[0]=yv2[1]; 
    yv2[1]=yv2[2]; 
    yv2[2]=yv2[3]; 
    yv2[3]=xv2[0]+xv2[3]+3*(xv2[1]+xv2[2])+
                     iir3_c0[daout_upsamp_n]*yv2[0]+
                     iir3_c1[daout_upsamp_n]*yv2[1]+
                     iir3_c2[daout_upsamp_n]*yv2[2];
    a2 = yv2[3];
    xv3[0]=xv3[1]; 
    xv3[1]=xv3[2]; 
    xv3[2]=xv3[3]; 
    xv3[3]=r2/iir3_gain[daout_upsamp_n];
    yv3[0]=yv3[1]; 
    yv3[1]=yv3[2]; 
    yv3[2]=yv3[3]; 
    yv3[3]=xv3[0]+xv3[3]+3*(xv3[1]+xv3[2])+
                     iir3_c0[daout_upsamp_n]*yv3[0]+
                     iir3_c1[daout_upsamp_n]*yv3[1]+
                     iir3_c2[daout_upsamp_n]*yv3[2];
    b1 = yv3[3];
    xv4[0]=xv4[1]; 
    xv4[1]=xv4[2]; 
    xv4[2]=xv4[3]; 
    xv4[3]=r4/iir3_gain[daout_upsamp_n];
    yv4[0]=yv4[1]; 
    yv4[1]=yv4[2]; 
    yv4[2]=yv4[3]; 
    yv4[3]=xv4[0]+xv4[3]+3*(xv4[1]+xv4[2])+
                     iir3_c0[daout_upsamp_n]*yv4[0]+
                     iir3_c1[daout_upsamp_n]*yv4[1]+
                     iir3_c2[daout_upsamp_n]*yv4[2];
    b2 = yv4[3];
    u1= a1*rx_daout_cos+a2*rx_daout_sin;
    if(u1 > bg_amplimit)u1=bg_amplimit;
    if(u1 < -bg_amplimit)u1=-bg_amplimit;
  //  if(bg_maxamp < fabs(u1))bg_maxamp=fabs(u1); 
    u2= b1*rx_daout_cos+b2*rx_daout_sin;
    if(u2 > bg_amplimit)u2=bg_amplimit;
    if(u2 < -bg_amplimit)u2=-bg_amplimit;
 //   if(bg_maxamp < fabs(u2))bg_maxamp=fabs(u2); 
    if(ptt_state == FALSE)
      {
      if(rx_daout_bytes == 1)
        {
        daout[daout_pa]=0x80+u1;
        if(rx_daout_channels == 2)daout[daout_pa+1]=0x80+u2;
        }
      else
        {  
        intvar=(short int*)(&daout[daout_pa]);
        intvar[0]=u1;
        if(rx_daout_channels == 2)intvar[1]=u2;
        }
      }
    else    
      {
      dt1=sidetone_cos;
      sidetone_cos=sidetone_cos*sidetone_phstep_cos+
                   sidetone_sin*sidetone_phstep_sin;
      sidetone_sin=sidetone_sin*sidetone_phstep_cos-
                            dt1*sidetone_phstep_sin;
      if(rx_daout_bytes == 1)
        {
        k=(int)(sidetone_cos*0x7e/1000.);
        daout[daout_pa]=0x80+k;
        if(rx_daout_channels == 2)daout[daout_pa+1]=0x80+k;
        }
      else
        {
        intvar=(short int*)(&daout[daout_pa]);
        intvar[0]=(int)(sidetone_cos*0x7ffe/1000.);
        if(rx_daout_channels == 2)intvar[1]=intvar[0];
        }
      }
    if(rx_daout_channels==1)lirerr(213567);      
    daout_pa=(daout_pa+mm)&daout_bufmask;
    t2+=t1;
    t4+=t3;
    r2+=r1;
    r4+=r3;
    }
  }
baseb_r1=baseb_r2;
if(daout_pa > daout_bufmask)lirerr(994578);
if(daout_pa == 0)
  {
  baseb_fx+=baseb_r2;
  baseb_r1=0; 
  da_resample_ratio=new_da_resample_ratio;
  if(baseb_fx>baseband_size)baseb_fx-=baseband_size;
  if(use_bfo != 0)
    {
    t1=(2*PI_L*bg.bfo_freq)/genparm[DA_OUTPUT_SPEED];
    rx_daout_phstep_cos=cos(t1);
    rx_daout_phstep_sin=sin(t1);
    }
  }
if(thread_command_flag[THREAD_RX_OUTPUT] != THRFLAG_ACTIVE)
  {
  audio_status=FALSE;
  return;
  }
goto resamp;  
}
