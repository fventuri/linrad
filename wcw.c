
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
#include "seldef.h"
#include "sigdef.h"
#include "thrdef.h"
#include "options.h"
#include "blnkdef.h"
#include "sdrdef.h"
#include "bufbars.h"
#if OPENCL_PRESENT == 1
#include "oclprogs.h"
#endif

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif
#if OPENCL_PRESENT == 1
//#include <clFFT.h>
#endif
#if HAVE_CUFFT == 1
#include <cufft.h>
#include <cufftXt.h>
#endif
#define MOUSE_MIN_TIME 0.05
#define NBPROC_FFT1 1
#define CHK_INACTIVE 0
#define CHK_ACTIVE 1

#if (USERS_EXTRA_PRESENT == 1)
extern void users_extra_fast(void);
extern double users_extra_fast_time;
extern double users_extra_time;
#endif

int thrnum_fft1b[MAX_FFT1_THREADS];
int inptr_fft1b[MAX_FFT1_THREADS];
int ptrchk_fft1b[MAX_FFT1_THREADS];
float* out_fft1b[MAX_FFT1_THREADS]; 
float* buf_fft1b[MAX_FFT1_THREADS];
#if HAVE_CUFFT == 1
cufftHandle cufft_handle_fft1b[MAX_FFT1_THREADS];
cuFloatComplex *cuda_in[MAX_FFT1_THREADS];
cuFloatComplex *cuda_out[MAX_FFT1_THREADS];
#endif

extern void fix_prio(int n);

void compute_timf2_powersum(void)
{
int i;
float t1, t2;
while( ((timf2_pn2-timf2_pb+timf2_size)&timf2_mask) > timf2_blockpower_block)
  {
  i=timf2_pb;
  timf2_pb=(timf2_pb+timf2_blockpower_block)&timf2_mask;
  t1=0;
  if(sw_onechan)
    {
    if( fft_cntrl[FFT1_BCKCURMODE].mmx == 0)
      {
      while(i != timf2_pb)
        {
        t1+=timf2_float[i  ]*timf2_float[i  ]+
            timf2_float[i+1]*timf2_float[i+1];
        i=(i+4)&timf2_mask;
        }
      }
    else
      {
      while(i != timf2_pb)
        {
        t1+=(float)timf2_shi[i  ]*(float)timf2_shi[i  ]+
            (float)timf2_shi[i+1]*(float)timf2_shi[i+1];
        }
      }
    timf2_blockpower[timf2_blockpower_pa]=t1;
    }
  else  
    {
    t2=0;
    if( fft_cntrl[FFT1_BCKCURMODE].mmx == 0)
      {
      while(i != timf2_pb)
        {
        t1+=timf2_float[i  ]*timf2_float[i  ]+
            timf2_float[i+1]*timf2_float[i+1];
        t2+=timf2_float[i+2]*timf2_float[i+2]+
            timf2_float[i+3]*timf2_float[i+3];
        i=(i+8)&timf2_mask;
        }
      }
    else
      {
      while(i != timf2_pb)
        {
        t1+=(float)timf2_shi[i  ]*(float)timf2_shi[i  ]+
            (float)timf2_shi[i+1]*(float)timf2_shi[i+1];
        t2+=(float)timf2_shi[i+2]*(float)timf2_shi[i+2]+
            (float)timf2_shi[i+3]*(float)timf2_shi[i+3];
        }
      }
    timf2_blockpower[2*timf2_blockpower_pa  ]=t1;
    timf2_blockpower[2*timf2_blockpower_pa+1]=t2;
    }
  timf2_blockpower_pa=(timf2_blockpower_pa+1)&timf2_blockpower_mask;
  }  
}




#if BUFBARS == TRUE
int bufbar_maxpos[MAX_BUFBAR];
int bufbar_minpos[MAX_BUFBAR];
int bufbar_reset_no;
double bufbar_reset_time;

void clear_bufbars(void)
{
int i;
for(i=0; i<MAX_BUFBAR; i++)bufbar_maxpos[i]=-1;
for(i=0; i<MAX_BUFBAR; i++)bufbar_minpos[i]=-1;
bufbar_reset_time=current_time();;
bufbar_reset_no=-1;
indicator_first_pixel=29*text_width;
indicator_ypix=screen_height-3.5*text_height;
}

void show_bufbar(int n, int k)
{
int ia, ib;
int ka, nn;
if(bufbar_minpos[n] > k || bufbar_maxpos[n] < k)
  {
  ka=indicator_first_pixel;
  nn=n;
  if(n >= RX_BUFBAR_MAX)
    {
    ka+=INDICATOR_SIZE+10;
    nn-=RX_BUFBAR_MAX;
    }
  ia=bufbar_minpos[n];
  if(ia > k)ia=k;
  ib=bufbar_maxpos[n];
  if(ib < k)ib=k;
  lir_hline(ka,indicator_ypix-4*nn, ka+ia-1,15);
  lir_hline(ka+ia, indicator_ypix-4*nn, ka+ib-1,12);
  lir_hline(ka+ib,indicator_ypix-4*nn, ka+INDICATOR_SIZE-1,1);
  bufbar_minpos[n]=ia;
  bufbar_maxpos[n]=ib;
  }
}  

void reset_bufbar_pos(void)
{
bufbar_reset_no++;
if(bufbar_reset_no >= MAX_BUFBAR)
  {
  bufbar_reset_no=-1;
  bufbar_reset_time=recent_time;
  return;
  }
bufbar_maxpos[bufbar_reset_no]=-1;
bufbar_minpos[bufbar_reset_no]=0x8ffffff;
}
#endif

void clear_baseb(void)
{
int i, n, m;
float raw_wttim,t1;
// We have to recalculate timf3 from old transforms.
// Find out how much time the data waiting before fft1 sums up to.
make_ad_wttim();
while(ad_wts > timf1_usebytes)
  {
  lir_sleep(3000);
  make_ad_wttim();
  }
raw_wttim=ad_wttim;
// We need a total delay from input sample to output sample
// that can accomodate the most unfavourable situation while collecting
// enough data to make a new transform.
// Make da_wait_time the desired number of seconds from input sample
// to output sample.
// First a safety margin for processing delay and adjusting speed
// errors in case different soundboards are used for input and output.
// All delays are not maximum simultaneously. Subtract 10 ms
// just in case...
da_wait_time=0.001*genparm[OUTPUT_DELAY_MARGIN];
// Maximum data waiting in timf3 that is not enough for a new fft3 transform
// plus the number of 50% interleaved transforms we want to wait.
da_wait_time+=fft3_size/timf3_sampling_speed;
t1=da_wait_time-raw_wttim+fft1_size/ui.rx_ad_speed;
// We use a window for fft3 so we need to recalculate
// fft3_interleave-points extra to get the first transform output right.
t1+=fft3_interleave_points/timf3_sampling_speed;
// make data for 0.1 seconds extra
t1+=.1;
if(genparm[SECOND_FFT_ENABLE]!=0)
  {
// Add time for data waiting in timf2.
  make_fft2_wttim();
  raw_wttim+=timf2_wttim;
// Maximum data waiting in timf2 that is not enough for a new fft2 transform
  da_wait_time+=fft2_size/timf1_sampling_speed;
  sc[SC_HG_FQ_SCALE]++;
  n=t1/fft2_blocktime;
  if(fft2_blocktime > 0.25)n++;
  if(genparm[AFC_ENABLE] != 0 && genparm[AFC_LOCK_RANGE] != 0)
    {
    n+=afct_delay_points;
    da_wait_time+=afct_delay_points*fft2_blocktime;
    }
  if(n > fft2_nm)n=fft2_nm;
  if(n > max_fft2n-3)n=max_fft2n-3;
  fft2_nx=(fft2_na-n+max_fft2n)&fft2n_mask;
  fft2_nc=fft2_nx;
  new_fft2_averages();
  }
else
  {
// Maximum raw data waiting that is not enough for a new fft1 transform
  if(fft_cntrl[FFT1_CURMODE].gpu == 0)
    {
    m=fft1_size*(fft_cntrl[FFT1_CURMODE].real2complex+1)*
                                 fft_cntrl[FFT1_CURMODE].parall_fft;
    }
  else  
    {
    m=fft1_size*gpu_fft1_batch_size;
    }
  m=fft1_size*(fft_cntrl[FFT1_CURMODE].real2complex+1)*
                                 fft_cntrl[FFT1_CURMODE].parall_fft;
  da_wait_time+=m/timf1_sampling_speed;
  n=t1/fft1_blocktime;
  if(fft1_blocktime > 0.25)n++;
  if(genparm[AFC_ENABLE] != 0 && genparm[AFC_LOCK_RANGE] != 0)
    {
    n+=afct_delay_points;
    da_wait_time+=afct_delay_points*fft1_blocktime;
    }
  i=fft1_nx;
  if(n > max_fft1n-3)n=max_fft1n-3;
  if(n>fft1_nm)n=fft1_nm;
  fft1_nx=(fft1_na-n+max_fft1n+1)&fft1n_mask;
  if(fft1_skip_flag !=0 && fft1_nx > max_fft1n/2)
    {
    fft1_nx=fft1_na;
    }
  if(fft1_nx > 25)fft1_skip_flag=0;  
  fft1_nc=fft1_nx;
  fft1_px=fft1_nx*fft1_block;
  }
timf3_pa=0;
timf3_px=0;
timf3_py=0;
timf3_ps=0;
timf3_pn=0;
timf3_pc=0;
timf3_pd=0;
if(fft1_correlation_flag >= 2)
  {
//  timf3_pc=(timf3_pc-mix2.new_points*ui.rx_rf_channels+timf3_size)&timf3_mask;
//  timf3_pd=timf3_pc;
  baseb_pa=0;
  baseb_pb=0;
  baseb_pc=0;
  baseb_pd=0;
  baseb_pe=0;
  baseb_pf=0;
  baseb_ps=0;
  baseb_pm=0;
  baseb_pn=0;
  baseb_py=0;
  baseb_px=0;
// We want to discard all old data in buffers.
// Une unit of inhibit_count discards one block of size mix2.new_points
// in the fft3 buffer. The associated time is 
// 2*mix2.new_points*ui.rx_rf_channels/timf3_sampling_speed
  basebcorr_inhibit_count=1+rint(0.5+da_wait_time/(
          (float)(2*mix2.new_points*ui.rx_rf_channels)/timf3_sampling_speed));
  }
for(i=0;i<timf3_block;i++)timf3_float[i]=0;
basebnet_pa=0;
basebnet_px=0;
basebrawnet_pa=0;
basebrawnet_px=0;
basebraw_test_cnt1=0;
basebraw_test_cnt2=0;
basebraw_test_cnt3=0;
da_resample_ratio=genparm[DA_OUTPUT_SPEED]/baseband_sampling_speed;
new_da_resample_ratio=da_resample_ratio;
for(i=0; i<genparm[MIX1_NO_OF_CHANNELS]; i++)
  {
  mix1_status[i]=0;
  }
old_afct_delay=afct_delay_points;
poleval_pointer=0;
clear_coherent();
if(audio_dump_flag)
  {
  fft1_nb=fft1_na;
  fft1_nx=fft1_na;
  fft1_pb=fft1_pa;
  fft1_px=fft1_pa;
  }
}

int local_spurcancel_flag;
int local_spurinit_flag;

void spur_removal(void)
{
int i, k, m, mm;
double dt1;
float t2, t3;
k=0;
if(genparm[AFC_ENABLE] == 2 && wg.spur_inhibit == 0)
  {
  if(autospur_point < spur_search_last_point)
    {
    t2=1/snd[RXAD].interrupt_rate;
    t3=0.1*fftx_size/ui.rx_ad_speed;
    if(t2<t3)t2=t3;
    dt1=current_time();
    m=512;
    mm=16;
    if(m < fftx_size/1024)m=fftx_size/1024;
    if(mm < genparm[MAX_NO_OF_SPURS]/100)mm=genparm[MAX_NO_OF_SPURS]/100;
more_spurs:;
    i=no_of_spurs;
    k=0;
    lir_sched_yield();
    while( k<m && autospur_point < spur_search_last_point &&
                                                         no_of_spurs-i < mm)
      {
      init_spur_elimination();
      k++;
      }
    if(autospur_point < spur_search_last_point)
      {
      if(current_time()-dt1 < t2)goto more_spurs;
      }
    }
  }
if(local_spurinit_flag != spurinit_flag)
  {
  local_spurinit_flag=spurinit_flag;
  init_spur_elimination();
  }
if(local_spurcancel_flag != spurcancel_flag)
  {
  local_spurcancel_flag=spurcancel_flag;
  no_of_spurs=0;
  }
}

void second_fft(void)
{
int m;
#if BUFBARS == TRUE
int k;
#endif
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_SECOND_FFT);
#endif
thread_status_flag[THREAD_SECOND_FFT]=THRFLAG_ACTIVE;
while(!kill_all_flag &&
       thread_command_flag[THREAD_SECOND_FFT] == THRFLAG_ACTIVE)
  {
restart:;
  m=0;
  while( ((timf2_pn2-timf2_px+timf2_size)&timf2_mask) <
                                              4*ui.rx_rf_channels*fft2_size )
    {
    m=1;
    thread_status_flag[THREAD_SECOND_FFT]=THRFLAG_SEM_WAIT;
    lir_await_event(EVENT_FFT2);
    thread_status_flag[THREAD_SECOND_FFT]=THRFLAG_ACTIVE;
    if(kill_all_flag || thread_command_flag[THREAD_SECOND_FFT] !=
                                               THRFLAG_ACTIVE)goto fft2_x;
    }
  if(yieldflag_fft2_fft2 && m==0)lir_sched_yield();
// Add the two parts of timf2 so we get a single time function
// and produce the second fft.
// Note that the fft2 routine still is split into a number of small
// chunks so we have to call until the flag is set.
  make_fft2_status=FFT2_NOT_ACTIVE;
  while(make_fft2_status != FFT2_COMPLETE)
    {
    make_fft2();
    if(yieldflag_fft2_fft2)lir_sched_yield();
    }
  if(genparm[AFC_ENABLE] >= 1 && genparm[MAX_NO_OF_SPURS] != 0)
    {
    ffts_na=fft2_na;
    ffts_nm=fft2_nm;
    spur_removal();
    }
  if(fft1_waterfall_flag)
    {
    if(latest_wg_spectrum_shown!=latest_wg_spectrum)
      {
      if(recent_time-fft1_show_time > 0.05)
        {
        fft1_show_time=recent_time;
        sc[SC_SHOW_FFT1]++;
        awake_screen();
        }
      }  
    }
  lir_set_event(EVENT_FFT1_READY);
#if BUFBARS == TRUE
  if(timinfo_flag!=0)
    {
    k=(fft2_na-fft2_nx+max_fft2n)&fft2n_mask;
    k*=fft2_size;
    k/=fft2n_indicator_block;
    show_bufbar(RX_BUFBAR_FFT2,k);
    }
#endif
  }
if(thread_command_flag[THREAD_SECOND_FFT]==THRFLAG_IDLE)
  {
  thread_status_flag[THREAD_SECOND_FFT]=THRFLAG_IDLE;
  while(thread_command_flag[THREAD_SECOND_FFT] == THRFLAG_IDLE)
    {
    lir_sleep(3000);
    }
  if(kill_all_flag) goto fft2_x;
  if(thread_command_flag[THREAD_SECOND_FFT] == THRFLAG_ACTIVE)goto restart;
  }
fft2_x:;
thread_status_flag[THREAD_SECOND_FFT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_SECOND_FFT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

void do_fft1_c(void)
{  
#if BUFBARS == TRUE
int i;
#endif
fft1_c();
#if BUFBARS == TRUE
if(timinfo_flag!=0)
  {
  i=(fft1_pa+fft1_block-fft1_px+fft1_mask+1)&fft1_mask;
  i/=fft1_indicator_block;
  show_bufbar(RX_BUFBAR_FFT1,i);
  }
#endif
}

void do_fft1c(void)
{
int k;
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_DO_FFT1C);
#endif
thread_status_flag[THREAD_DO_FFT1C]=THRFLAG_ACTIVE;
while(!kill_all_flag && 
        thread_command_flag[THREAD_DO_FFT1C] == THRFLAG_ACTIVE)
  {
  lir_await_event(EVENT_DO_FFT1C);
  if(fft1_na != fft1_nb)
    {
repeat:;    
    k=0;
    while(fft1_na != fft1_nb)
      {
      do_fft1_c();
      k++;
      if(k == fft1_muln)
        {
        lir_set_event(EVENT_FFT1_READY);
        k=0;
        lir_sched_yield();
        }
      }
    if(k != 0)
      {
      lir_set_event(EVENT_FFT1_READY);
      }
    if(fft1_waterfall_flag)
      {
      fft1_waterfall();
      if(latest_wg_spectrum_shown!=latest_wg_spectrum)
        {
        if(recent_time-fft1_show_time > 0.05)
          {
          fft1_show_time=recent_time;
          sc[SC_SHOW_FFT1]++;
          awake_screen();
          }
        }
      }
    }
  if(fft1_na != fft1_nb)
    {
    goto repeat;
    }
  }
thread_status_flag[THREAD_DO_FFT1C]=THRFLAG_RETURNED;
}

void timf2_routine(void)
{
#if BUFBARS == TRUE
int k;
#endif
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_TIMF2);
#endif
thread_status_flag[THREAD_TIMF2]=THRFLAG_ACTIVE;
while(!kill_all_flag &&
       thread_command_flag[THREAD_TIMF2] == THRFLAG_ACTIVE)
  {
restart:;
  thread_status_flag[THREAD_TIMF2]=THRFLAG_SEM_WAIT;
  lir_await_event(EVENT_TIMF2);
  thread_status_flag[THREAD_TIMF2]=THRFLAG_ACTIVE;
  if(kill_all_flag || thread_command_flag[THREAD_TIMF2] !=
                                                 THRFLAG_ACTIVE)goto timf2_x;
  while(fft1_px != fft1_pa && (timf2_px-timf2_pa+timf2_mask+1)>fft1_block)
    {
    while(fft1_na != fft1_nb)
      {
      do_fft1_c();
      make_timf2();
      }
#if BUFBARS == TRUE
    if(timinfo_flag!=0)
      {
      k=(timf2_pa+fft1_block-timf2_px+timf2_mask+1)&timf2_mask;
      k/=timf2_indicator_block;
      show_bufbar(RX_BUFBAR_TIMF2,k);
      }
#endif
    if(yieldflag_timf2_fft1)lir_sched_yield();
    first_noise_blanker();
    lir_sched_yield();
    if( ((timf2_pn2-timf2_px+timf2_size)&timf2_mask) >=
                                            4*ui.rx_rf_channels*fft2_size )
      {
      lir_set_event(EVENT_FFT2);
      }
    if(mg.scale_type == MG_SCALE_STON)compute_timf2_powersum();
    } 
  }
if(thread_command_flag[THREAD_TIMF2]==THRFLAG_IDLE)
  {
  thread_status_flag[THREAD_TIMF2]=THRFLAG_IDLE;
  while(thread_command_flag[THREAD_TIMF2] == THRFLAG_IDLE)
    {
    lir_sleep(3000);
    }
  if(kill_all_flag) goto timf2_x;
  if(thread_command_flag[THREAD_TIMF2] == THRFLAG_ACTIVE)goto restart;
  }
timf2_x:;
thread_status_flag[THREAD_TIMF2]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_TIMF2] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

void mouse_rightpressed_dummy(void)
{
// See wide_graph_add_signal()

if(rightpressed==BUTTON_RELEASED)
  {
  sc[SC_WG_WATERF_REDRAW]++;
  rightpressed=BUTTON_IDLE;
  mouse_task=-1;
  }
}


void do_fft1b(void)
{
int i, thread_no, event_no;
i=0;
thread_no=THREAD_FFT1B1;
while(thrnum_fft1b[i] != -1)
  {
  i++;
  thread_no++;
  }
thrnum_fft1b[i]=thread_no;
#if OSNUM == OSNUM_LINUX
clear_thread_times(thread_no);
//fix_prio(thread_no);
#endif
event_no=EVENT_DO_FFT1B1+i;
while( thread_command_flag[thread_no] == THRFLAG_ACTIVE)
  {
  thread_status_flag[thread_no]=THRFLAG_SEM_WAIT;
  if(kill_all_flag)goto xx;
  lir_await_event(event_no);
  if(kill_all_flag)goto xx;
  thread_status_flag[thread_no]=THRFLAG_ACTIVE;  
  lir_sched_yield();
  fft1_b(inptr_fft1b[i],out_fft1b[i],buf_fft1b[i],i);
  thread_status_flag[thread_no]=THRFLAG_IDLE;
  lir_sched_yield();
  if(kill_all_flag)goto xx;
  lir_set_event(EVENT_TIMF1);
  lir_await_event(event_no);
  }
xx:;
thread_status_flag[thread_no]=THRFLAG_RETURNED;
while(thread_command_flag[thread_no] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

void wideband_dsp(void)
{
int i, k, m;
int fft1_pt, fft1_pz;
int timf1_overload_ticks;
int timf1_overload_count;
int local_ampinfo_reset;
int waitflag;
char s[80];
float t1;
int local_fft1_liminfo_cnt;
int local_fft2_liminfo_cnt;
fft1_liminfo_cnt=0;
fft2_liminfo_cnt=0;
local_fft1_liminfo_cnt=0;
local_fft2_liminfo_cnt=0;
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_WIDEBAND_DSP);
#endif
thread_status_flag[THREAD_WIDEBAND_DSP]=THRFLAG_ACTIVE;
#if OPENCL_PRESENT == 1
if(fft1_use_gpu == GPU_OCL)
  {
  i=0;
create:;  
  ocl_handle_fft1b[i]=create_clFFT_plan((size_t) fft1_size, 
                     (unsigned)gpu_fft1_batch_size, 1.0f, 
                               gpu.fft1_platform, gpu.fft1_device);
  if(!ocl_handle_fft1b[i])
    {
    lirerr(1455);
    goto errexit;
    }
  i++;  
  if(i<no_of_fft1b)goto create;
  }      
#endif
#if HAVE_CUFFT == 1
if(fft1_use_gpu == GPU_CUDA)
  {
  int ncu[1];
  cufftResult res;
  ncu[0]=fft1_size;
  i=0;
create_cu:;
  res=cufftPlanMany(&cufft_handle_fft1b[i], 1, ncu,
                     NULL, 1, fft1_size,  //advanced data layout, NULL shuts it o
                     NULL, 1, fft1_size,  //advanced data layout, NULL shuts it o
                     CUFFT_C2C, gpu_fft1_batch_size);
  if(res != CUFFT_SUCCESS)
    {
    lirerr(1461);
    goto errexit;
    }
  cudaMalloc((void**)&cuda_in[i], 
                gpu_fft1_batch_size*fft1_size*sizeof(cuFloatComplex));
  cudaMalloc((void**)&cuda_out [i], 
                gpu_fft1_batch_size*fft1_size*sizeof(cuFloatComplex));
  i++;  
  if(i<no_of_fft1b)goto create_cu;
  }      
#endif
if( (ui.network_flag & NET_RXIN_TIMF2) != 0 && 
                                ui.rx_addev_no != NETWORK_DEVICE_CODE)
  {
  lirerr(1319);
  goto errexit;
  }
// Allow this thread to use 100% of one CPU if the computer has
// more than one CPU and FFT2 is not selected.
// If FFT2 is selected and we have more than 3 CPUs we can
// also run this thread without yeilds.
local_ampinfo_reset=workload_reset_flag;
local_spurcancel_flag=spurcancel_flag;
local_spurinit_flag=spurinit_flag;
if(fft1_n > 12)
  {
  yieldflag_timf2_fft1=TRUE;
  }
else
  {
  yieldflag_timf2_fft1=TRUE;
  }
while(!kill_all_flag && timf1p_px==timf1p_pb && 
                thread_command_flag[THREAD_WIDEBAND_DSP] == THRFLAG_ACTIVE)
  {
  lir_await_event(EVENT_TIMF1);
  }
if(kill_all_flag)goto wide_error_exit;
if( genparm[SECOND_FFT_ENABLE] != 0 )
  {
  linrad_thread_create(THREAD_SECOND_FFT);
  if(ui.max_blocked_cpus > 4)yieldflag_timf2_fft1=FALSE;
  if(no_of_processors > 1)
    {
    linrad_thread_create(THREAD_TIMF2);
    }
  }
else
  {
  if(ui.max_blocked_cpus > 2)yieldflag_timf2_fft1=FALSE;
  if(no_of_processors > 1)
    {
    linrad_thread_create(THREAD_DO_FFT1C);
    while(thread_status_flag[THREAD_DO_FFT1C] != THRFLAG_ACTIVE &&
                thread_command_flag[THREAD_WIDEBAND_DSP] == THRFLAG_ACTIVE)
      {
      lir_sleep(1000);
      }
    }
  }
if(kill_all_flag || 
            thread_command_flag[THREAD_WIDEBAND_DSP] != THRFLAG_ACTIVE)
  {
  goto wide_error_exit;
  }
if(no_of_fft1b > 0)
  {
  for(i=0; i<MAX_FFT1_THREADS; i++)
    {
    thrnum_fft1b[i]=-1;
    ptrchk_fft1b[i]=CHK_INACTIVE;
    }
  lir_sched_yield();
  for(i=0; i<no_of_fft1b; i++)
    {
    linrad_thread_create(THREAD_FFT1B1+i);
    while(thread_status_flag[THREAD_FFT1B1+i] != THRFLAG_SEM_WAIT &&
          thread_status_flag[THREAD_FFT1B1+i] != THRFLAG_RETURNED )
      {
      lir_sleep(3000);
      }
    }
  }
if(kill_all_flag || 
            thread_command_flag[THREAD_WIDEBAND_DSP] != THRFLAG_ACTIVE)
  {
  goto wide_error_exit;
  }
if(thread_command_flag[THREAD_SCREEN]!=THRFLAG_NOT_ACTIVE)
  {
  while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
    {
    if(thread_command_flag[THREAD_WIDEBAND_DSP] ==
                                           THRFLAG_KILL)goto wide_error_exit;
    lir_sleep(10000);
    if(kill_all_flag || 
               thread_command_flag[THREAD_WIDEBAND_DSP] != THRFLAG_ACTIVE)
      {
      goto wide_error_exit;
      }
    }
  }
restart:;
k=0;
restart_2:;
lir_sched_yield();
timf1p_px=timf1p_pb;
fft1_pt=fft1_pa;
if( (ui.network_flag & NET_RXIN_FFT1) == 0)
  {
  while(!kill_all_flag && timf1p_px==timf1p_pb &&
             thread_command_flag[THREAD_WIDEBAND_DSP] == THRFLAG_ACTIVE)
    {
    thread_status_flag[THREAD_WIDEBAND_DSP]=THRFLAG_SEM_WAIT;
    lir_await_event(EVENT_TIMF1);
    if(kill_all_flag)goto wide_error_exit;;
    }
  k++;
  if(k<3)goto restart_2;
  }
lir_sched_yield();
timf1p_px=timf1p_pb;
mouse_time_wide=current_time();
timf1_overload_ticks=0;
timf1_overload_count=0;
waitflag=0;
fft1_pz=-1;
while(!kill_all_flag &&
        thread_command_flag[THREAD_WIDEBAND_DSP] == THRFLAG_ACTIVE)
  {
  if(ampinfo_flag != 0)
    {
    if(local_ampinfo_reset!=workload_reset_flag)
      {
      local_ampinfo_reset=workload_reset_flag;
      clear_wide_maxamps();
      }
    }
  if( (ui.network_flag & NET_RXIN_FFT1) == 0)
    {
    if( ((timf1p_pb-timf1p_px+timf1_bytes)&timf1_bytemask)
               < timf1_blockbytes && genparm[MAX_NO_OF_SPURS] != 0 &&
             genparm[AFC_ENABLE] >= 1 && genparm[SECOND_FFT_ENABLE] == 0)
      {
      spur_removal();
      }
await_event:;
    if(kill_all_flag || thread_command_flag[THREAD_WIDEBAND_DSP] !=
                                                 THRFLAG_ACTIVE)goto wideend;
    if(((timf1p_pb-timf1p_px+timf1_bytes)&timf1_bytemask) < timf1_blockbytes)
      {
      if(mouse_task==-1 || (mouse_task&GRAPH_MASK) >= MAX_WIDEBAND_GRAPHS)
        {
        thread_status_flag[THREAD_WIDEBAND_DSP]=THRFLAG_SEM_WAIT;
        lir_await_event(EVENT_TIMF1);
        waitflag=0;
        thread_status_flag[THREAD_WIDEBAND_DSP]=THRFLAG_ACTIVE;
        goto await_event;
        }
      else
        {  
        t1=current_time()-mouse_time_wide;
        if(t1 <= MOUSE_MIN_TIME)
          {
          t1=MOUSE_MIN_TIME-t1;
          if(t1 > 0.25/snd[RXAD].interrupt_rate)t1=0.25/snd[RXAD].interrupt_rate;
          lir_sleep(1000000*t1);
          goto await_event;
          }
        }  
      }
    else
      {
      if(no_of_fft1b == 0)
        {
        lir_sched_yield();
        }
      else  
        {
        k=0;
        for(i=0; i<no_of_fft1b; i++)
          {
          if(thread_status_flag[THREAD_FFT1B1+i] != THRFLAG_ACTIVE)k=1;
          }
        if(k==0)
          {
          thread_status_flag[THREAD_WIDEBAND_DSP]=THRFLAG_SEM_WAIT;
          lir_await_event(EVENT_TIMF1);
          waitflag=0;
          thread_status_flag[THREAD_WIDEBAND_DSP]=THRFLAG_ACTIVE;
          goto await_event;
          }
        if(waitflag != 0)
          {
          lir_sleep(10);   
          }
        }
      }
    i=snd[RXAD].block_bytes;
    if(ui.rx_addev_no >= 256 && ui.rx_addev_no < SPECIFIC_DEVICE_CODES)i<<=1;
    k=(timf1p_pa-timf1p_px+timf1_bytes)&timf1_bytemask;
    k/=(i+timf1_blockbytes);
    if(audio_dump_flag == 0 && k > 5+2*no_of_fft1b)
      {
      timf1_overload_ticks++;
      if(timf1_overload_ticks > snd[RXAD].interrupt_rate )
        {
        timf1_overload_count++;
        sprintf(s,"BUFFER ERROR (timf1) %d",timf1_overload_count);
        wg_error(s,WGERR_TIMF1_OVERLOAD);
        timf1_overload_ticks=0;
        }
      }
    else
      {
      timf1_overload_ticks=0;
      }        
    }
  else
    {
    if(genparm[AFC_ENABLE] >= 1 && genparm[MAX_NO_OF_SPURS] != 0 &&
                                         genparm[SECOND_FFT_ENABLE] == 0)
      {
      spur_removal();
      }
await_netevent:;
    if(fft1_na == fft1_nb)
      {
      thread_status_flag[THREAD_WIDEBAND_DSP]=THRFLAG_SEM_WAIT;
      lir_await_event(EVENT_TIMF1);
      thread_status_flag[THREAD_WIDEBAND_DSP]=THRFLAG_ACTIVE;
      }
    }
  if(kill_all_flag || thread_command_flag[THREAD_WIDEBAND_DSP] !=
                                                   THRFLAG_ACTIVE)goto wideend;
// Here we do the mouse actions that affect the wide graph.
// Currently only the waterfall memory area may become re-allocated
  if(recent_time-mouse_time_wide > MOUSE_MIN_TIME)
    {
    mouse_time_wide=current_time();
    if(mouse_task!=-1)
      {
      k=mouse_task&GRAPH_MASK;
      if( k < MAX_WIDEBAND_GRAPHS)
        {
        set_button_states();
        if(mouse_active_flag == 0)
          {
          switch (k)
            {
            case WIDE_GRAPH:
            if( (mouse_task&GRAPH_RIGHTPRESSED) != 0)
              {
              wide_graph_add_signal();
              if(rightpressed==BUTTON_IDLE)mouse_task=-1;
              goto mouse_x;
              }
            else
              {
              mouse_on_wide_graph();
              }
            break;

            case HIRES_GRAPH:
            if( (mouse_task&GRAPH_RIGHTPRESSED) != 0)
              {
              mouse_rightpressed_dummy();
              if(rightpressed==BUTTON_IDLE)mouse_task=-1;
              goto mouse_x;
              }
            else
              {
              mouse_on_hires_graph();
              }
            break;

            case TRANSMIT_GRAPH:
            if( (mouse_task&GRAPH_RIGHTPRESSED) != 0)
              {
              mouse_rightpressed_dummy();
              if(rightpressed==BUTTON_IDLE)mouse_task=-1;
              goto mouse_x;
              }
            else
              {
              mouse_on_tx_graph();
              }
            break;

            case FREQ_GRAPH:
            mouse_on_freq_graph();
            break;

            case RADAR_GRAPH:
            if( (mouse_task&GRAPH_RIGHTPRESSED) != 0)
              {
              mouse_rightpressed_dummy();
              if(rightpressed==BUTTON_IDLE)mouse_task=-1;
              goto mouse_x;
              }
            else
              {
              mouse_on_radar_graph();
              }  
            break;
            }
          if(mouse_active_flag == 0)
            {
            lirerr(28877);
            goto wide_error_exit;
            }
          }
        if(numinput_flag==0)
          {
          current_mouse_activity();
          if(kill_all_flag)goto wide_error_exit;
          }
        if( (numinput_flag&DATA_READY_PARM) != 0)
          {
          par_from_keyboard_routine();
          if(kill_all_flag)goto wide_error_exit;
          par_from_keyboard_routine=NULL;
          mouse_active_flag=0;
          numinput_flag=0;
          leftpressed=BUTTON_IDLE;
          rightpressed=BUTTON_IDLE;
          switch(clear_graph_flag)
            {
            case 2*ALLAN_GRAPH:
            make_allan_graph(TRUE,FALSE);
            break;

            case 2*ALLAN_GRAPH+1:
            make_allan_graph(TRUE,TRUE);
            break;

            case 2*ALLANFREQ_GRAPH:
            make_allanfreq_graph(TRUE,FALSE);
            break;

            case 2*ALLANFREQ_GRAPH+1:
            make_allanfreq_graph(TRUE,TRUE);
            break;

            case 2*SIGANAL_GRAPH:
            make_siganal_graph(TRUE,FALSE);
            break;

            case 2*SIGANAL_GRAPH+1:
            make_siganal_graph(TRUE,TRUE);
            break;

            }
          }
        if(mouse_active_flag == 0)
          {
          mouse_task=-1;
          }
mouse_x:;
        }
      }
    }
  if(kill_all_flag) goto wide_error_exit;
#if (USERS_EXTRA_PRESENT == 1)
  if(users_extra_time != users_extra_fast_time)
    {
    users_extra_fast();
    users_extra_fast_time=users_extra_time;
    }
#endif
  if( (ui.network_flag & NET_RXIN_FFT1) == 0)
    {
    if( ((timf1p_pb-timf1p_px+timf1_bytes)&timf1_bytemask) < 
                                            timf1_blockbytes)
      {
      if(no_of_fft1b == 0)
        {
        goto wideend;
        }
      else
        {
        goto fft1_b_results;
        }
      }    
#if BUFBARS == TRUE
    if(timinfo_flag!=0)
      {
      k=snd[RXAD].block_bytes;
      if(ui.rx_addev_no >= 256 && ui.rx_addev_no < SPECIFIC_DEVICE_CODES)k<<=1;
      k=(timf1p_pa+k-timf1p_px+timf1_bytes)&timf1_bytemask;
      k/=timf1_indicator_block;
      show_bufbar(RX_BUFBAR_TIMF1,k);
      }
#endif
    if(computation_pause_flag != 0)
      {
      timf1p_px=(timf1p_px+timf1_blockbytes)&timf1_bytemask;
      goto await_event;
      }
    else
      {
      if(audio_dump_flag)
        {
        while( ((fft1_na-fft1_nx+max_fft1n)&fft1n_mask) > 
                                  (no_of_fft1b+2)*fft1_muln)lir_sleep(5000);
        }
      if(no_of_fft1b > 0)
        {
// We are doing the bulk of the FFT work, fft1_b, in one or more
// separate threads. We now have a pointer to a new set of
// input data at timf1p_px. Find a free thread to give the task.
        if(kill_all_flag || thread_command_flag[THREAD_WIDEBAND_DSP] !=
                                               THRFLAG_ACTIVE)goto wideend;
        i=0;
        lir_sched_yield();
        while(i < no_of_fft1b && ((timf1p_pb-timf1p_px+timf1_bytes)&
                                          timf1_bytemask) >=timf1_blockbytes)
          {
          if(ptrchk_fft1b[i] == CHK_INACTIVE &&
                  thread_status_flag[THREAD_FFT1B1+i] == THRFLAG_SEM_WAIT)
            {
// There is a thread waiting for the task.            
            ptrchk_fft1b[i]=CHK_ACTIVE;
            inptr_fft1b[i]=timf1p_px;
            timf1p_px=(timf1p_px+timf1_blockbytes)&timf1_bytemask;
            out_fft1b[i]=&fft1_float[fft1_pt];
            fft1_pt=(fft1_pt+fft1_mulblock)&fft1_mask;  
            buf_fft1b[i]=&fftw_tmp[i*(fft1_tmp_bytes/sizeof(float)+4)]; 
            lir_set_event(EVENT_DO_FFT1B1+i);
            waitflag=1;
            }  
          i++;
          }
// ********************************************************************
fft1_b_results:
        if(kill_all_flag || thread_command_flag[THREAD_WIDEBAND_DSP] !=
                                               THRFLAG_ACTIVE)goto wideend;
// Find the oldest fft1_b computation.
        m=-1;
        k=-1;
        lir_sched_yield();
        for(i=0; i<no_of_fft1b; i++)
          {
          if( ((timf1p_px-inptr_fft1b[i] + 
                                  timf1_bytes)&timf1_bytemask) > m)
            {
            m=(timf1p_px-inptr_fft1b[i]+timf1_bytes)&timf1_bytemask;
            k=i;
            }
          }
        if(k >= 0 && ptrchk_fft1b[k] == CHK_ACTIVE &&
                      thread_status_flag[THREAD_FFT1B1+k] == THRFLAG_IDLE)
          {
          ptrchk_fft1b[k]=CHK_INACTIVE;
          lir_set_event(EVENT_DO_FFT1B1+k);
          waitflag=1;
          if( (ui.network_flag&NET_RXOUT_FFT1) != 0)
            {
            memcpy(&fft1_netsend_buffer[fft1net_pa],
                       &fft1_float[fft1_pa],(unsigned int)fft1_mulblock);
            fft1net_pa=(fft1net_pa+fft1_blockbytes)&fft1net_mask;
            }
          fft1_pa=(fft1_pa+fft1_mulblock)&fft1_mask;  
          goto fft1_b_results;
          }
        }
      else
        {
        fft1_b(timf1p_px, &fft1_float[fft1_pa], fftw_tmp, 0);
        timf1p_px=(timf1p_px+timf1_blockbytes)&timf1_bytemask;
        if( (ui.network_flag&NET_RXOUT_FFT1) != 0)
          {
          memcpy(&fft1_netsend_buffer[fft1net_pa],
                           &fft1_float[fft1_pa],(unsigned int)fft1_mulblock);
          fft1net_pa=(fft1net_pa+fft1_blockbytes)&fft1net_mask;
          }
        fft1_pa=(fft1_pa+fft1_mulblock)&fft1_mask;  
        }
      fft1_na=fft1_pa/fft1_block;
      if(fft1_nm != fft1n_mask)fft1_nm++;
      }
    }
  else
    {
    if(fft1_na == fft1_nb)goto wideend;
    }
  if(genparm[SECOND_FFT_ENABLE] == 0 )
    {
    if(no_of_processors > 1)
      {
      if(fft1_pz != fft1_pa)
        {
        lir_set_event(EVENT_DO_FFT1C);
        waitflag=1;
        fft1_pz=fft1_pa;
        }
      }
    else
      {
      while(fft1_na != fft1_nb)
        {
        do_fft1_c();
        }
      if(fft1_waterfall_flag)
        {
        fft1_waterfall();
        if(latest_wg_spectrum_shown!=latest_wg_spectrum)
          {
          if(recent_time-fft1_show_time > 0.05)
            {
            fft1_show_time=recent_time;
            sc[SC_SHOW_FFT1]++;
            awake_screen();
            }
          }  
        }
      }  
    lir_set_event(EVENT_FFT1_READY);
    }
  else
    {
    if(no_of_processors > 1)
      {
      lir_set_event(EVENT_TIMF2);
      lir_sched_yield();
      }
    else
      {
      while(fft1_na != fft1_nb)
        {
        do_fft1_c();
        if(yieldflag_timf2_fft1)lir_sched_yield();
        make_timf2();
        }
#if BUFBARS == TRUE
      if(timinfo_flag!=0)
        {
        k=(timf2_pa+fft1_block-timf2_px+timf2_size)&timf2_mask;
        k/=timf2_indicator_block;
        show_bufbar(RX_BUFBAR_TIMF2,k);
        }
#endif
      first_noise_blanker();
      lir_sched_yield();
      if( ((timf2_pn2-timf2_px+timf2_size)&timf2_mask) >=
                                              4*ui.rx_rf_channels*fft2_size )
        {
        lir_set_event(EVENT_FFT2);
        }
      if(mg.scale_type == MG_SCALE_STON)compute_timf2_powersum();
      }
    }
  if( (ui.network_flag & NET_RXIN_FFT1) != 0)
    {  
    if(fft1_na != fft1_nb)goto await_netevent;
    }
  if(fft1_liminfo_cnt != local_fft1_liminfo_cnt)
    {
    fft1_update_liminfo();
    local_fft1_liminfo_cnt=fft1_liminfo_cnt;
    }
  if(fft2_liminfo_cnt != local_fft2_liminfo_cnt)
    {
    fft2_update_liminfo();
    local_fft2_liminfo_cnt=fft2_liminfo_cnt;
    }
wideend:;
  }
// ********************************************************************
if(thread_command_flag[THREAD_WIDEBAND_DSP]==THRFLAG_IDLE)
  {
// The wideband dsp thread is running and it puts out
// the waterfall graph.
// We stop it now on order from the tx_test thread.
  thread_status_flag[THREAD_WIDEBAND_DSP]=THRFLAG_IDLE;
  while(thread_command_flag[THREAD_WIDEBAND_DSP]==THRFLAG_IDLE)
    {
    lir_await_event(EVENT_TIMF1);
    timf1p_px=(timf1p_px+timf1_blockbytes)&timf1_bytemask;
    }
  if(thread_command_flag[THREAD_WIDEBAND_DSP] == THRFLAG_ACTIVE)goto restart;
  }
wide_error_exit:;
if( genparm[SECOND_FFT_ENABLE] != 0 )
  {
  linrad_thread_stop_and_join(THREAD_SECOND_FFT);
  if(no_of_processors > 1)
    {
    linrad_thread_stop_and_join(THREAD_TIMF2);
    }
  }
else
  {
  if(no_of_processors > 1)
    {
    linrad_thread_stop_and_join(THREAD_DO_FFT1C);
    }
  }
if(no_of_fft1b > 0)
  {
  for(i=0; i<no_of_fft1b; i++)
    {
    linrad_thread_stop_and_join(THREAD_FFT1B1+i);
    }
  }
errexit:;
#if OPENCL_PRESENT == 1
if(ocl_active && fft_cntrl[FFT1_CURMODE].gpu == GPU_OCL)
  {
  i=0;
destroy:;  
  destroy_clFFT_plan(&ocl_handle_fft1b[i]);
  i++;
  if( i<no_of_fft1b)goto destroy;
  do_clfftTeardown();
  }      
#endif
lir_await_event(EVENT_TIMF1);
thread_status_flag[THREAD_WIDEBAND_DSP]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_WIDEBAND_DSP] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

void narrowband_dsp(void)
{
int fftx_flag;
int local_baseb_reset_counter;
int i, k;
int idle_flag;
int local_bg_update_filter;
double dt1;
double total_time1, idle_time, baseb_clear_time;
float t1;
int fft3_overload_ticks;
int fft3_overload_count;
char s[80];
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_NARROWBAND_DSP);
#endif
#if BUFBARS == TRUE
clear_bufbars();
#endif
ampinfo_reset=workload_reset_flag;
fft3_overload_ticks=0;
fft3_overload_count=0;
new_baseb_flag=-1;
total_time1=current_time();
dt1=total_time1;
mouse_time_narrow=total_time1;
idle_time=total_time1;
idle_flag=0;
local_bg_update_filter=bg_update_filter;
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// *******************************************************
//                   MAIN RECEIVE LOOP
// *******************************************************
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
local_baseb_reset_counter=0;
baseb_reset_counter=0;
// � � � � � � � �??init_baseband_sizes();
baseb_clear_time=0;
fftx_flag=0;
while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
      thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
      thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
 {
 if(thread_command_flag[THREAD_NARROWBAND_DSP] ==
                                           THRFLAG_KILL)goto wcw_error_exit;
 lir_sleep(10000);
 }
thread_status_flag[THREAD_NARROWBAND_DSP]=THRFLAG_ACTIVE;
if(first_keypress == 1 && ui.autostart >= 'A' && ui.autostart <= 'G')
  {
  t1=fg.tune_frequency;
  if(t1 < mix1_lowest_fq)t1=mix1_lowest_fq;
  if(t1 > mix1_highest_fq)t1=mix1_highest_fq;
  make_new_signal(0,t1);
  baseb_reset_counter++;
  first_keypress=2;
  }
while(thread_command_flag[THREAD_NARROWBAND_DSP] == THRFLAG_ACTIVE)
  {
  dt1=current_time();
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  idle_flag++;
  switch (idle_flag)
    {
    case 1:
    if(eme_active_flag != 0)
      {
      if(dt1 - eme_time > 3)
        {
        sc[SC_COMPUTE_EME_DATA]++;
        eme_time=dt1;
        }
      }
    break;

    case 2:
    if(fft1_handle != NULL)
      {
      memcheck(198,fft1mem,&fft1_handle);
      if(kill_all_flag) goto wcw_error_exit;
      }
    if(baseband_handle != NULL)
      {
      memcheck(298,basebmem,&baseband_handle);
      if(kill_all_flag) goto wcw_error_exit;
      }
    if(allan_handle != NULL)
      {
      memcheck(698,allanmem,&allan_handle);
      if(kill_all_flag) goto wcw_error_exit;
      }
    
    break;

    case 3:
    if(afc_handle != NULL)
      {
      memcheck(398,afcmem,&afc_handle);
      if(kill_all_flag) goto wcw_error_exit;
      }
    if(fft3_handle != NULL)
      {
      memcheck(498,fft3mem,&fft3_handle);
      if(kill_all_flag) goto wcw_error_exit;
      }
    if(blanker_handle != NULL)
      {
      memcheck(598,blankermem,&blanker_handle);
      if(kill_all_flag) goto wcw_error_exit;
      }
    break;

    case 4:
    if(hires_handle != NULL)
      {
      memcheck(598,hiresmem,&hires_handle);
      if(kill_all_flag) goto wcw_error_exit;
      }
    if( allow_wse_parport != 0)
      {
// Read now and then to make apm aware Linrad is running.
// See z_APM.txt
      if (wse.parport != USB2LPT16_PORT_NUMBER)
        {
        lir_inb(wse.parport);
        }
      }  
    break;

    case 5:
#if BUFBARS == TRUE
    if(recent_time-bufbar_reset_time > 2)reset_bufbar_pos();
#endif
    break;

    default:
    if(recent_time-idle_time >0.2)
      {
      idle_flag=0;
      idle_time=recent_time;
      }
    break;
    }
  if( mouse_task==-1 || (mouse_task&GRAPH_MASK) <= MAX_WIDEBAND_GRAPHS)
    {
    thread_status_flag[THREAD_NARROWBAND_DSP]=THRFLAG_SEM_WAIT;
    lir_await_event(EVENT_FFT1_READY);
    thread_status_flag[THREAD_NARROWBAND_DSP]=THRFLAG_ACTIVE;
    if(kill_all_flag) goto wcw_error_exit;
    if(thread_command_flag[THREAD_NARROWBAND_DSP]
                                             !=THRFLAG_ACTIVE)goto narend;
    }
  else
    {
    t1=current_time()-mouse_time_narrow;
    if(t1 <= MOUSE_MIN_TIME)
      {
      if( ((fft3_pa-fft3_px+fft3_totsiz)&fft3_mask) >= fft3_block &&
          ((baseb_py-baseb_pa+baseband_mask)&baseband_mask) > 4*(int)mix2.size)
        {
        goto more_mix2;
        }
      if( ((timf3_pa-timf3_px+timf3_size)&timf3_mask) >=
                                             twice_rxchan*fft3_size &&
                      ((fft3_px-fft3_pa+fft3_mask)&fft3_mask) > fft3_block)
        {
        goto more_fft3;
        }
      if(fftx_flag != 0)
        {
        goto more_fftx;
        }
      if(t1 > 0.5/snd[RXAD].interrupt_rate)t1=0.5/snd[RXAD].interrupt_rate;
      lir_sleep(1000000*t1);
      }
    }
// Here we do the mouse actions that affect the narrowband graphs.
// Note that memory areas may become re-allocated and that no other
// threads may try to acces the narrowband arrays in the meantime.
// The screen thread is stopped by the memory allocation routines
  if(new_baseb_flag <=0 && recent_time-mouse_time_narrow > MOUSE_MIN_TIME)
    {
    mouse_time_narrow=current_time();
    if(new_rbutton_state == BUTTON_PRESSED)
      {
      if(mouse_x > bg.xleft && mouse_x < bg.xright &&
         mouse_y > bg.ytop && mouse_y < bg.yborder)
        {
        t1=((float)(mouse_x-bg_first_xpixel)/bg.pixels_per_point
                           -fft3_size/2+bg_first_xpoint)*bg_hz_per_pixel;
        if(fabs(t1) > text_width)
          {
          if(t1 > 0)
            {
            move_rx_frequency(0.0001*pow(t1-(float)text_width,2.0),0);
            }
          else
            {  
            move_rx_frequency(-0.0001*pow(t1+(float)text_width,2.0),0);
            }
          }
        }
      }
    if(bg_update_filter != local_bg_update_filter)
      {
      local_bg_update_filter=bg_update_filter;
      make_bg_filter();
      if(kill_all_flag) goto wcw_error_exit;
      make_modepar_file(GRAPHTYPE_BG);
      mg_clear_flag=TRUE;
      }        
    lir_sched_yield();
    k=mouse_task&GRAPH_MASK;
    if( k > MAX_WIDEBAND_GRAPHS)
      {
      set_button_states();
      if( (mouse_task&GRAPH_RIGHTPRESSED) != 0)
        {
        rightpressed=BUTTON_IDLE;
        mouse_task=-1;
        goto mouse_x;
        }
      if(mouse_active_flag == 0)
        {
        switch (k)
          {
          case AFC_GRAPH:
          mouse_on_afc_graph();
          break;

          case BASEBAND_GRAPH:
          mouse_on_baseband_graph();
          break;

          case POL_GRAPH:
          mouse_on_pol_graph();
          break;

          case COH_GRAPH:
          mouse_on_coh_graph();
          break;

          case EME_GRAPH:
          mouse_on_eme_graph();
          break;

          case METER_GRAPH:
          mouse_on_meter_graph();
          break;

          case ELEKTOR_GRAPH:
          mouse_on_elektor_graph();
          break;

          case FCDPROPLUS_GRAPH:
          mouse_on_fcdproplus_graph();
          break;

          case PHASING_GRAPH:
          if(pg.enable_phasing == 1)mouse_on_phasing_graph();
          break;

          case SIGANAL_GRAPH:
          if(fft1_correlation_flag == 2)mouse_on_siganal_graph();
          break;
          
          case ALLAN_GRAPH:
          if(fft1_correlation_flag == 3)mouse_on_allan_graph();
          break;
          
          case ALLANFREQ_GRAPH:
          if(fft1_correlation_flag == 3)mouse_on_allanfreq_graph();
          break;
          
          default:
          mouse_on_users_graph();
          break;
          }
        if(mouse_active_flag == 0)lirerr(8877);
        }
      if(numinput_flag==0)
        {
        current_mouse_activity();
        if(kill_all_flag) goto wcw_error_exit;
        }
      if( (numinput_flag&DATA_READY_PARM) != 0)
        {
        par_from_keyboard_routine();
        if(kill_all_flag) goto wcw_error_exit;
        par_from_keyboard_routine=NULL;
        mouse_active_flag=0;
        numinput_flag=0;
        leftpressed=BUTTON_IDLE;
        switch(clear_graph_flag)
          {
          case 2*ALLAN_GRAPH:
          make_allan_graph(TRUE,FALSE);
          break;

          case 2*ALLAN_GRAPH+1:
          make_allan_graph(TRUE,TRUE);
          break;

          case 2*SIGANAL_GRAPH:
          make_siganal_graph(TRUE,FALSE);
          break;

          case 2*SIGANAL_GRAPH+1:
          make_siganal_graph(TRUE,TRUE);
          break;
          }
        }
        
      if(mouse_active_flag == 0)
        {
        mouse_task=-1;
        }
mouse_x:;
      }
    }
  if(local_baseb_reset_counter!=baseb_reset_counter)
    {
do_baseb_reset:;
    i=0;
    local_baseb_reset_counter=baseb_reset_counter;
// We control the rx output thread from here. It should be either
// active or waiting on the semaphore.
// Send it to the semaphore if it is not already there.
    if(thread_status_flag[THREAD_RX_OUTPUT] == THRFLAG_SEMCLEAR)goto dasem;
    if(thread_status_flag[THREAD_RX_OUTPUT] == THRFLAG_IDLE)goto daidle;
    if(thread_status_flag[THREAD_RX_OUTPUT] == THRFLAG_RETURNED)goto narend;
    pause_thread(THREAD_RX_OUTPUT); 
daidle:;    
    thread_command_flag[THREAD_RX_OUTPUT]=THRFLAG_SEMCLEAR;
    lir_sched_yield();
    while(thread_status_flag[THREAD_RX_OUTPUT] != THRFLAG_SEMCLEAR)
      {
      lir_sleep(1000);
      if(kill_all_flag) goto wcw_error_exit;
      }
dasem:;
    if(mix1_selfreq[0] >= 0)
      {
      new_baseb_flag=3;
      init_baseband_sizes();
      make_baseband_graph(TRUE);
      clear_baseb();
      baseb_clear_time=current_time();
      local_baseb_reset_counter=baseb_reset_counter;
      goto narend_chk;
      }
    else
      {
      if( (ui.network_flag & NET_RX_INPUT) != 0)
        {
        net_send_slaves_freq();
        }
      new_baseb_flag=-1;
      }
    }
more_fftx:;
  if(local_baseb_reset_counter!=baseb_reset_counter)goto do_baseb_reset;
  fftx_flag=0;
  if( genparm[SECOND_FFT_ENABLE] == 0 )
    {
    if(new_baseb_flag >= 0)
      {
      if(genparm[AFC_ENABLE] != 0 && genparm[AFC_LOCK_RANGE] != 0)
        {
        if( fft1_nb != fft1_nc)
          {
          make_afc();
          if( ag.mode_control != 0 && afc_graph_filled != 0)sc[SC_SHOW_AFC]++;
          }
        if( fft1_nm >= afc_tpts)
          {
          if( ((fft1_nc-fft1_nx+max_fft1n)&fft1n_mask) > afct_delay_points
               &&((timf3_px-timf3_pa+timf3_mask)&timf3_mask) >= timf3_block)
             {
             fft1_mix1_afc();
             }
          }
        if( ((fft1_nb-fft1_nx+max_fft1n)&fft1n_mask) >
                                              afct_delay_points )fftx_flag|=1;
        if( ((timf3_px-timf3_pa+timf3_mask)&timf3_mask) <
                                                     timf3_block)fftx_flag|=2;
        }
      else
        {
        if( fft1_nb != fft1_nx &&
                ((timf3_px-timf3_pa+timf3_mask)&timf3_mask) >= timf3_block)
          {
          fft1_mix1_fixed();
          if(fft1_nb != fft1_nx)fftx_flag|=1;
          if( ((timf3_px-timf3_pa+timf3_mask)&timf3_mask) <
                                                     timf3_block)fftx_flag|=2;
          }
        }
      }
    else
      {
      fft1_nx=fft1_nb;
      fft1_px=fft1_pb;
      fft1_nc=fft1_nb;
      goto clear_select;
      }
    }
  else
    {
    if(new_baseb_flag >= 0)
      {
      if(genparm[AFC_ENABLE] != 0 && genparm[AFC_LOCK_RANGE] != 0)
        {
        if(fft2_na != fft2_nc)
          {
          make_afc();
          if( ag.mode_control != 0 && afc_graph_filled != 0)sc[SC_SHOW_AFC]++;
          }
        if( ((fft2_nc-fft2_nx+max_fft2n)&fft2n_mask) > afct_delay_points &&
                 ((timf3_px-timf3_pa+timf3_mask)&timf3_mask) >= timf3_block)
          {
          fft2_mix1_afc();
          }
        if(((fft2_na-fft2_nx+max_fft2n)&fft2n_mask) >
                                               afct_delay_points)fftx_flag|=1;
        if( ((timf3_px-timf3_pa+timf3_mask)&timf3_mask) <
                                                     timf3_block)fftx_flag|=2;
        }
      else
        {
        if(fft2_na != fft2_nx &&
                ((timf3_px-timf3_pa+timf3_mask)&timf3_mask) >= timf3_block)
          {
          fft2_mix1_fixed();
          }
        if(fft2_na != fft2_nx)fftx_flag|=1;
        if( ((timf3_px-timf3_pa+timf3_mask)&timf3_mask) < timf3_block)
          {
          fftx_flag|=2;
          }
        }
      }
    else
      {
clear_select:;
      timf3_px=timf3_pa;
      timf3_py=timf3_pa;
      timf3_pn=timf3_pa;
      fft3_px=fft3_pa;
      baseb_wts=2;
      baseb_pb=baseb_pa;
      baseb_py=baseb_pa;
      baseb_pn=baseb_pa;
      goto narend_chk;
      }
    }
#if BUFBARS == TRUE
  if(timinfo_flag!=0)
    {
    i=(timf3_pa+timf3_block-timf3_px+timf3_size)&timf3_mask;
    i/=timf3_indicator_block;
    show_bufbar(RX_BUFBAR_TIMF3,i);
    }
#endif
  if(local_baseb_reset_counter!=baseb_reset_counter)goto do_baseb_reset;
  if( ((timf3_pa-timf3_px+timf3_size)&timf3_mask) >=
                                             twice_rxchan*fft3_size &&
                    ((fft3_px-fft3_pa+fft3_mask)&fft3_mask) > fft3_block)
    {
more_fft3:;
    make_fft3_all();
    }
#if BUFBARS == TRUE
  if(fft3_handle != NULL && timinfo_flag!=0)
    {
    i=(fft3_pa+fft3_block-fft3_px+fft3_totsiz)&fft3_mask;
    i/=fft3_indicator_block;
    show_bufbar(RX_BUFBAR_FFT3,i);
    }
#endif
  k=(fft3_pa-fft3_px+fft3_totsiz)&fft3_mask;
  k/=fft3_block;
  if(audio_dump_flag == 0 && k >= 3)
    {
    fft3_overload_ticks++;
    if(fft3_overload_ticks > 5)
      {
      fft3_overload_count++;
      sprintf(s,"BUFFER ERROR (fft3) %d",fft3_overload_count);
      wg_error(s,WGERR_FFT3_OVERLOAD);
      fft3_overload_ticks=0;
      }
    }
  else
    {
    fft3_overload_ticks=0;
    }
  if( ((fft3_pa-fft3_px+fft3_totsiz)&fft3_mask) >= fft3_block &&
      ((baseb_py-baseb_pa+baseband_mask)&baseband_mask) > 4*(int)mix2.size)
    {
more_mix2:;
    if(local_baseb_reset_counter!=baseb_reset_counter)goto do_baseb_reset;
    fft3_mix2();
    if( ((baseb_pa-baseb_py+baseband_size)&baseband_mask) >= baseb_output_block)
      {
      lir_set_event(EVENT_BASEB);
      }
    if(kill_all_flag) goto wcw_error_exit;
    if(new_baseb_flag ==0)
      {
      if(genparm[CW_DECODE_ENABLE] != 0)
        {
        coherent_cw_detect();
        if(kill_all_flag) goto wcw_error_exit;
        }
      if(cg.oscill_on != 0)sc[SC_CG_REDRAW]++;
      }
    }
#if BUFBARS == TRUE
  if(baseband_handle != NULL && timinfo_flag!=0)
    {
    i=(baseb_pa+fft3_block+2-baseb_py+baseband_mask+1)&baseband_mask;
    i/=baseb_indicator_block;
    show_bufbar(6,i);
    }
#endif
  if(new_baseb_flag > 0)
    {
    lir_sched_yield();
    make_timing_info();
    update_snd(RXDA);
    if(kill_all_flag) goto wcw_error_exit;
    t1=total_wttim-da_wait_time-da_wttim;
    if(!audio_dump_flag && new_baseb_flag == 3)
      {
      if(t1 > 0.01 )
        {
        new_baseb_flag=2;
        }
      goto narend_chk;
      }
    if(baseb_clear_time+3+2*da_wait_time < current_time())lirerr(1058);
// new_baseb_flag == 1 or 2
// We want data in buffers corresponding to da_wait_time.
// We arrive here when there is more than that, move pointers
// to discard data corresponding to total_wttim-da_wait_time
// If there is too much data in memory, move the baseb pointer
// to discard more data.
    if(new_baseb_flag == 2)
      {
      if(t1*baseband_sampling_speed > 4)
        {
        i=t1*baseband_sampling_speed;
        if(i > baseb_wts-1)
          {
          baseb_py=(baseb_py+(int)(baseb_wts)-1+baseband_size)&baseband_mask;
          goto narend_chk;
          }
        baseb_py=(baseb_py+i-1+baseband_size)&baseband_mask;
        }
      new_baseb_flag=1;
      goto narend_chk;
      }
    lir_sched_yield();
    if( (ui.network_flag & NET_RXIN_FFT1) == 0)
      {
      i=timf1p_pb;
      thread_status_flag[THREAD_NARROWBAND_DSP]=THRFLAG_INPUT_WAIT;
      while(i == timf1p_pb)
        {
        if(thread_command_flag[THREAD_NARROWBAND_DSP] != THRFLAG_ACTIVE)
          {
          thread_status_flag[THREAD_NARROWBAND_DSP]=THRFLAG_ACTIVE;
          goto narend_chk;
          }
        lir_sleep(1000);
        if(kill_all_flag) goto wcw_error_exit;
        }
      thread_status_flag[THREAD_NARROWBAND_DSP]=THRFLAG_ACTIVE;
      }
    if(cg.meter_graph_on != 0)sc[SC_MG_REDRAW]++;
    workload_reset_flag++;
    if( (ui.network_flag & NET_RX_INPUT) != 0)
      {
      net_send_slaves_freq();
      }
    new_baseb_flag=0;
    timf2_blockpower_px=-1;
    timf2_pb=timf2_pn2;
    sc[SC_BG_WATERF_REDRAW]+=3;
    thread_command_flag[THREAD_RX_OUTPUT]=THRFLAG_ACTIVE;
    lir_set_event(EVENT_RX_START_DA);
    goto more_fftx;
    }
narend_chk:;
  if(kill_all_flag)goto wcw_error_exit;
  if( ((fft3_pa-fft3_px+fft3_totsiz)&fft3_mask) >= fft3_block &&
      ((baseb_py-baseb_pa+baseband_mask)&baseband_mask) > 4*(int)mix2.size)
    {
    lir_sched_yield();
    goto more_mix2;
    }
  if( ((timf3_pa-timf3_px+timf3_size)&timf3_mask) >=
                                             twice_rxchan*fft3_size &&
                    ((fft3_px-fft3_pa+fft3_mask)&fft3_mask) > fft3_block)
    {
    lir_sched_yield();
    goto more_fft3;
    }
  if(fftx_flag != 0)
    {
    if(new_baseb_flag == 0)
      {
      if( (fftx_flag&2) == 0)
        {
        lir_sched_yield();
        }
      else
        {
        lir_sleep(5000);
        }
      }
    if(!kill_all_flag && thread_command_flag[THREAD_NARROWBAND_DSP] ==
                                                THRFLAG_ACTIVE)
      {
      goto more_fftx;
      }
    }
narend:;
  }
wcw_error_exit:;
thread_status_flag[THREAD_NARROWBAND_DSP]=THRFLAG_RETURNED;
lir_sched_yield();
while(thread_command_flag[THREAD_NARROWBAND_DSP] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

