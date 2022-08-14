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
#include "uidef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "screendef.h"
#include "caldef.h"
#include "thrdef.h"
#include "keyboard_def.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

#define TOPLINE 0
#define WAITMENU_LINE TOPLINE+4
#define ERRLINE (WAITMENU_LINE+5)
#define MAX_PULSE_ERRORS 5

#define SKIP_BEFORE 0.15
#define SKIP_AFTER 0.30


void final_filtercorr_init(void)
{
int i, j, k, mm, line;
int ja, jb;
int ka, kb;
int siz_n, siz;
float t1, t2, t3, renorm;
double dt1, dt2;
double *dbuf;
double pwrinteg[17];
char s[80];
double dc[4];
if(cal_fft1_size == 0)
  {
  show_missing_cal_info();
  return;
  }

mm=twice_rxchan;
// In case the native size of the calibration data is smaller
// than fft1_size, change fft1_size and remember old values in fft2_size.
fft2_size=fft1_size;
fft2_n=fft1_n;
if(cal_fft1_size < fft1_size)
  {
  fft1_size=cal_fft1_size;
  fft1_n=cal_fft1_n;
  init_fft1_filtercorr();
  }
init_fft(0,fft1_n, fft1_size, fft2_tab, fft2_permute);
// The correction function fft1_filtercorr is present in the same 
// number of points as we currently have for fft1.
// Since the Q of the hardware is limited there is no need to use
// too many points in the filter function.
// This routine will reduce the number of points used for the filtercorr
// function while removing erronous points and reducing noise.
// ******************************************************************
// First get the magnitude of the second derivative into cal_buf.
// Large values over narrow frequency regions indicate the presence 
// of discontinuities in the calibration function due to narrowband
// signals (spurs)
// Also store the amplitude and the second derivative of the phase
// in cal_buf2.
// We can expect large second derivatives at the passband ends.
// Set ka and kb to point to the interesting region.
ka=5;
kb=fft1_size-6;
while(fft1_desired[ka]<0.5)ka++;
while(fft1_desired[kb]<0.5)kb--;
for(j=0; j<mm; j+=2)
  {
// The calibration function consists of small numbers because
// it serves the purpose of giving a suitable output level
// from the first fft to interface to the integer format of
// MMX routines in case they might be used.
// Renormalize the calibration function to become near unity so we
// do not get underflows when computing powers.
  t1=0;
  for(i=ka; i<kb; i++)
    {
    t2=(float)(fabs(fft1_filtercorr[mm*i+j  ])+
               fabs(fft1_filtercorr[mm*i+j+1]));
    if(t2 > t1)t1=t2;
    }
  for(i=0; i<fft1_size; i++)
    {
    fft1_filtercorr[mm*i+j  ]/=t1;
    fft1_filtercorr[mm*i+j+1]/=t1;
    }
// First derivative into cal_tmp[0,1].
  for(i=1; i<fft1_size; i++)
    {
    cal_tmp[mm*(i-1)+j  ]=fft1_filtercorr[mm*i+j  ]-
                          fft1_filtercorr[mm*(i-1)+j  ];
    cal_tmp[mm*(i-1)+j+1]=fft1_filtercorr[mm*i+j+1]-
                          fft1_filtercorr[mm*(i-1)+j+1];
    }
  cal_tmp[mm*(fft1_size-1)+j  ]=0;
  cal_tmp[mm*(fft1_size-1)+j+1]=0;
// Amplitude into cal_buf2[0] and phase into cal_buf2[1]
  for(i=0; i<fft1_size; i++)
    {
    cal_buf2[mm*i+j  ]=(float)sqrt(
                  fft1_filtercorr[mm*i+j  ]*fft1_filtercorr[mm*i+j  ]+
                  fft1_filtercorr[mm*i+j+1]*fft1_filtercorr[mm*i+j+1]);
    if(cal_buf2[mm*i+j  ] == 0)
      {
      cal_buf2[mm*i+j+1]=0;
      }
    else
      {  
      cal_buf2[mm*i+j+1]=(float)atan2(fft1_filtercorr[mm*i+j+1],
                                      fft1_filtercorr[mm*i+j  ]);
      }
    }
  }
for(j=0; j<mm; j+=2)
  {
// Power of second derivative into cal_buf[0]
  for(i=1; i<fft1_size; i++)
    {
    t1=cal_tmp[mm*i+j  ]-cal_tmp[mm*(i-1)+j  ];
    t2=cal_tmp[mm*i+j+1]-cal_tmp[mm*(i-1)+j+1];
    cal_buf[mm*(i-1)+j]=t1*t1+t2*t2;
    }
  cal_buf[mm*(fft1_size-1)+j  ]=0;
// A carrier is about 2 bins wide, but after differentiation twice
// we might expect it to be 4 bins wide. Remove HF noise in the
// second derivative power to make signals stand out better by
// low pass filtering over 5 points.
  t1=0;
  for(i=0; i<5; i++)t1+=cal_buf[mm*i+j];
  for(i=5; i<fft1_size; i++)
    {
    cal_buf[mm*(i-2)+j+1]=t1;
    t1+=cal_buf[mm*i+j]-cal_buf[mm*(i-5)+j]; 
    }
  t3=0;
  for(i=ka; i<kb; i++)
    {
    t3+=cal_buf[mm*i+j+1];
    }
  dc[j]=t3/(float)(kb-ka+1);
  }
// make a single channel for the summed relative power of both channels.  
for(i=1; i<fft1_size; i++)
  {
  t1=0;
  for(j=0; j<mm; j+=2)
    {
    t1+=cal_buf[mm*i+j+1]/(float)dc[j];
    }
  cal_buf[i]=t1;  
  }  
ja=ka;
repl:;
while(ja<kb && cal_buf[ja] < 100)ja++;
if(ja < kb)
  {
  jb=ja;
  while(jb<kb && cal_buf[jb] >20)jb++;
  if(jb<kb)
    {
    while(ja>ka && cal_buf[ja] > 20)ja--;
    k=4+(jb-ja)/4;
    while(ja>ka+k && cal_buf[ja]/cal_buf[ja-k]>4)ja--;
    while(jb<kb-k && cal_buf[jb]/cal_buf[jb+k]>4)jb++;
// Make the amplitude constant across the bad region.
// The amplitude is set to the average amplitude from both sides.
    for(j=0; j<mm; j+=2)
      {
      t1=0;
      for(i=0; i<k; i++)
        {
        t1+=cal_buf2[mm*(ja-i)+j];
        t1+=cal_buf2[mm*(jb+i)+j];
        }
      t1/=(float)(2*k);  
      for(i=ja; i<jb; i++)
        {
        cal_buf2[mm*i+j]=t1;
        t2+=cal_buf2[mm*i+j];
        }   
      }
    ja=jb;
    goto repl;
    }
  }
// Restore the calibration function.

for(j=0; j<mm; j+=2)
  {
  for(i=1; i<fft1_size; i++)
    {
    t1=cal_buf2[mm*i+j]*(float)cos(cal_buf2[mm*i+j+1]);
    t2=cal_buf2[mm*i+j]*(float)sin(cal_buf2[mm*i+j+1]);
    fft1_filtercorr[mm*i+j  ]=t1;
    fft1_filtercorr[mm*i+j+1]=t2;
    }
  }
// Get the transform of the current filtercorr function.
begin_final:;
cal_type=CAL_TYPE_REFINE_FILTERCORR;
cal_initscreen();
line=1;
dbuf=(double*)(cal_buf2);
k=mm*fft1_size;
t1=0;
for(i=0; i<k; i++)t1+=(float)fabs(fft1_filtercorr[i]);
renorm=(float)fft1_size/t1;
for(j=0; j<mm; j+=2)
  {
  for(i=0; i<fft1_size; i++)
    {
    cal_tmp[2*i  ]=fft1_filtercorr[mm*i+j  ]*renorm;
    cal_tmp[2*i+1]=fft1_filtercorr[mm*i+j+1]*renorm;
    }
  fftforward(fft1_size, fft1_n, cal_tmp, fft2_tab, fft2_permute, FALSE);
  for(i=0; i<fft1_size; i++)
    {
    cal_buf[mm*i+j  ]=cal_tmp[2*i  ]/(float)fft1_size;
    cal_buf[mm*i+j+1]=cal_tmp[2*i+1]/(float)fft1_size;
    }
  }  
// We now have the pulse response of the hardware filter chain.
// The pulse is centered at point 0.
// Calculate the integrated power of the pulse summed over the channels.
dt1=0;
for(i=0; i<fft1_size; i++)
  {
  for(j=0; j<mm; j++)
    {
    dt1+=pow(cal_buf[mm*i+j],2.);
    }
  dbuf[i]=dt1;
  }  
dt2=0.5*(dbuf[fft1_size/2-8]+dbuf[fft1_size/2+8]);
// Add an integration constant for the integral to become zero
// in the center region, as far away as possible from the pulse.
for(i=0; i<fft1_size; i++)
  {
  dbuf[i]-=dt2;
  }
// Find out how fast the integral falls when the size is doubled.
i=2;
k=0;
lir_text(10,line,"Part of pulse energy lost outside range.");
line++;
dt1=dbuf[fft1_size-1]-dbuf[0];
while(i<=fft1_size/2)
  {
  if(i == fft1_size/2)
    {
    pwrinteg[k]=0;
    }
  else
    {  
    pwrinteg[k]=fft1_size*(dbuf[fft1_size-i]-dbuf[i-1])/(dt1*(fft1_size-2*i));
    }
  sprintf(s,"Range=%d",2*i);
  lir_text(12,line,s);
  sprintf(s,"Lost=%.25f%%",pwrinteg[k]);
  lir_text(25,line,s);
  i*=2;
  k++;
  line++;
  }
// Step siz_n until the remaining energy content of the integral
// is below 0.0001 (0.01% in power or 1% in voltage).    
siz_n=1;
while(siz_n < k && pwrinteg[siz_n] > 0.0001)siz_n++;    
if(siz_n == k)siz_n--;
if(k-siz_n < 3 && siz_n > 4)
  {
  settextcolor(14);
  lir_text(5,line,"The calibration function seems noisy.");
  line++;
  lir_text(5,line,"It could also need all the points.");
  line++;
  lir_text(5,line,"Make your own judgement!");
  line++;
  settextcolor(7);
  } 
line+=2;
siz_n+=2;
siz=1<<siz_n;
sprintf(s,"Suggested new size for calibration function %d  (old = %d)",
                                                            siz,fft1_size);
lir_text(5,line,s);
line+=2;
save_msg:;
sprintf(s,"Save modified calibration function in %d points? (Y/N,F1)",siz);
lir_text(5,line,s);
kbdinp:;
await_processed_keyboard();
if(kill_all_flag) return;
if(lir_inkey == 'N') 
  {
  lir_text(5,line+3,"Enter size to save calibration. 0 to skip.");
  siz=lir_get_integer(49,line+3,5,0,fft1_size);
  if(siz == 0) return;
  siz_n=make_power_of_two(&siz);
  clear_lines(line,line+3);
  goto save_msg;
  }
if(lir_inkey == F1_KEY || lir_inkey == '!')
  {
  help_message(310);
  goto begin_final;
  }
if(lir_inkey != 'Y') goto kbdinp;
if(siz_n < 4)
  {
  lir_text(5,line+2,"Calibration data seems incorrect");
  lir_text(5,line+3,"Nothing changed");
skip:;
  lir_text(5,line+5,press_any_key);
  await_keyboard();
  return;
  }
if( siz_n >= fft1_n)
  {
  lir_text(5,line+2,"Filter response needs current number of data points");
  lir_text(5,line+3,"Data will be used without change");
  goto skip;
  }
resize_filtercorr_td_to_fd( FALSE, fft1_size, cal_buf, siz_n, siz, cal_buf);
write_filcorr(siz);
if(kill_all_flag) return;
sprintf(s,"Filter correction function saved in %d points",siz);
lir_text(5,line+2,s);
sprintf(s,"File size reduced from %d to %d bytes",
       (1+twice_rxchan)*fft1_size*(int)sizeof(float)+20*(int)sizeof(int), 
       (1+twice_rxchan)*siz*(int)sizeof(float)+20*(int)sizeof(int));
lir_text(5,line+3,s);
lir_text(5,line+5,press_any_key);
await_keyboard();
// Restore filtercorr and fft1 desired.
if(fft2_size != fft1_size)
  {
  fft1_size=fft2_size;
  fft1_n=fft2_n;
  init_fft1_filtercorr();
  }
}

void cal_filtercorr(void)
{
int j,jj,totbytes;    
char s[160];
int i,k,m,n,width,old_pb;
int ia,ib,ic,ja,jb,max_pulpos;
int ka, kb;
float collect_noiselevel[MAX_ADCHAN/2];
float collect_powerlevel[MAX_ADCHAN/2];
float collect_powermax[MAX_ADCHAN/2];
float old_trig_power,trig_power,dc[2*MAX_ADCHAN],ampmax;
float avg_power,summed_power;
float t1,t2,t3,r1,r2;
int pulse_error[MAX_PULSE_ERRORS]; 
int siz128, mm, collected_pulses;
int pulpos;
float summed_timediff;
int no_of_timediff;
int mask;
int show_flag;
double redraw_time;
double sum[4],isum[4];
double dsum1, dsum2;
double dt1,dt2,dt3;
fft2_size=(int)(2*cal_interval*(1-SKIP_BEFORE-SKIP_AFTER));
fft2_n=make_power_of_two(&fft2_size);
init_fft(0,fft2_n, fft2_size, fft2_tab, fft2_permute);
cal_ygain=1;
cal_xgain=1;
cal_xshift=0;
cal_domain=0;
mm=twice_rxchan;
for(j=0;j<mm; j++)
  {
  sum[j]=0;
  isum[j]=0;
  }
siz128=fft2_size/128;
// **********************************************************************
// To get the pulses on one format independently of the hardware and
// fft implementation (some could contain filters) we use the standard 
// routine for the first fft to get fourier transforms of the input data.
// By back transformation the input data is converted to floating point
// complex format regardless of the input format.
// **********************************************************************
i=0;
lir_sleep(10000);
fft1_px=fft1_pb;
while(i < 2 && thread_command_flag[THREAD_CAL_FILTERCORR]==THRFLAG_ACTIVE)
  {
  while(fft1_pb==fft1_px)
    {
    lir_await_event(EVENT_FFT1_READY);
    }
  fft1_px=fft1_pb;
  i++;
  }
clr_restart:;
summed_power=0;
summed_timediff=0;
no_of_timediff=0;
for(i=0; i<mm*fft1_size-1; i++)cal_buf5[i]=0;
for(j=0; j<ui.rx_rf_channels; j++)
  {
  collect_noiselevel[j]=0;
  collect_powerlevel[j]=0;
  }
totbytes=0;
collected_pulses=0;
old_trig_power=0;
for(i=0; i<MAX_PULSE_ERRORS; i++)pulse_error[i]=0;
redraw_time=current_time();
restart:;
cal_type=CAL_TYPE_COLLECT_PULSE_AVERAGE;
cal_initscreen();
lir_text(0,WAITMENU_LINE,
             "Wait until curves are stable.  +,-,E,C => Change scale");
lir_text(0,WAITMENU_LINE+1,
                    "U => Compute new corrections in RAM. Do not save on disk.");
lir_text(0,WAITMENU_LINE+2,"S => Save to disk from RAM");
lir_text(0,WAITMENU_LINE+3,"T => Toggle time/frequency domain");
lir_text(0,WAITMENU_LINE+4,"A => Clear RAM");      
lir_refresh_screen();
thread_status_flag[THREAD_CAL_FILTERCORR]=THRFLAG_ACTIVE;
show_flag=TRUE;
// Wait a full second. Some hardware may not settle fast...
lir_sleep(1000000);
old_pb=fft1_pb;
while(thread_command_flag[THREAD_CAL_FILTERCORR] == THRFLAG_ACTIVE)
  {
  while(old_pb==fft1_pb)
    {
    lir_await_event(EVENT_FFT1_READY);
    if(thread_command_flag[THREAD_CAL_FILTERCORR] != THRFLAG_ACTIVE)
                                                     goto check_command_flag;
    fft1_px=(fft1_pb-fft1_block+fft1_mask+1)&fft1_mask;
    }
// Skip old data in case the cpu is a bit slow.
  while( ((fft1_pb-fft1_px+fft1_mask+1)&fft1_mask) > fft1_block)
    {
    fft1_px=(fft1_px+fft1_block)&fft1_mask; 
    }
  old_pb=fft1_pb;
  for(j=0; j<mm; j+=2)
    {
    for(i=0; i<fft1_size; i++)
      {
      if(fft1_px+mm*i+j > fft1_mask)
        {
        lirerr(879456);
        goto filcorr_error_exit;
        }
      cal_tmp[2*i  ]=fft1_float[fft1_px+mm*i+j  ];
      cal_tmp[2*i+1]=fft1_float[fft1_px+mm*i+j+1];
      }
    if( (ui.rx_input_mode&IQ_DATA) != 0)fft_iqshift(fft1_size, cal_tmp);
    big_fftback(fft1_size, fft1_n, cal_tmp, cal_table, cal_permute,FALSE);
    for(i=0; i<fft1_size; i++)
      {
      cal_buf[mm*i+j  ]=cal_tmp[2*i  ];
      cal_buf[mm*i+j+1]=cal_tmp[2*i+1];
      }
    }  
// The time function we got in cal_buf is the time function after 
// multiplication with the sin pow4 window.
// Correct the time function by a division by cal_win - but use only the
// center half so we avoid division by small numbers.
  for(j=0; j<mm; j+=2)
    {
    t1=0;
    t2=0;
    for(i=fft1_size/4; i<=fft1_size/2; i++)
      {
      cal_buf[i*mm+j  ]/=cal_win[i];
      t1+=cal_buf[i*mm+j  ];
      cal_buf[i*mm+j+1]/=cal_win[i];
      t2+=cal_buf[i*mm+j+1];
      }                               
    for(i=fft1_size/2+1; i<=3*fft1_size/4; i++)
      {
      cal_buf[i*mm+j  ]/=cal_win[fft1_size-1-i];
      t1+=cal_buf[i*mm+j  ];
      cal_buf[i*mm+j+1]/=cal_win[fft1_size-1-i];
      t2+=cal_buf[i*mm+j+1];
      }
// Remove any DC component to make pulses more visible.
    t1/=(float)fft1_size/2+1;  
    t2/=(float)fft1_size/2+1;  
    for(i=fft1_size/4; i<=3*fft1_size/4; i++)
      {
      cal_buf[i*mm+j  ]-=t1;
      cal_buf[i*mm+j+1]-=t2;
      }
    }


// Get the total power summed over all channels and store in cal_buf7    
  ampmax=0;
  for(i=fft1_size/4; i<=3*fft1_size/4; i++)
    {
    cal_buf7[i]=(float)pow(cal_buf[i*mm  ],2.0);
    for(j=1; j<mm; j++)
      {
      cal_buf7[i]+=(float)pow(cal_buf[i*mm+j],2.0);
      }
    if(ampmax < cal_buf7[i])ampmax=cal_buf7[i];
    }            
  if(old_trig_power == 0)
    {
    old_trig_power=ampmax;
    }
  else
    {
    old_trig_power=(old_trig_power*(INIT_PULSENUM-1)+ampmax)/INIT_PULSENUM;         
    }
// Make sure we have a reasonable value for trig power by not
// going further until we looked at a few pulses.
  totbytes+=fft1_size/2;
  if(totbytes > INIT_PULSENUM*cal_interval)
    {
    totbytes=(int)(INIT_PULSENUM*cal_interval);
// Set trig power at -15dB (30 times)
// Pulses must be separated well enough for the power level to
// have dropped well below -15dB after 70% of the interval time.
    trig_power=old_trig_power/30;
// Look for a pulse, at least interval/2 points into our data.
    pulpos=(int)(cal_interval+(float)fft1_size/4);
    max_pulpos=(int)((float)(3*fft1_size)/4-cal_interval);
    while( cal_buf7[pulpos] > trig_power/4 && pulpos<max_pulpos)pulpos++;
    if(pulpos >= max_pulpos)
      {
      lir_text(1,TOPLINE+1,"Noise level too high");
      lir_refresh_screen();
      }
    else
      {
      sprintf(s,"Search range %d to %d",pulpos,max_pulpos);
      lir_text(1,TOPLINE+1,s);
      lir_refresh_screen();
      while( cal_buf7[pulpos] < trig_power && pulpos<max_pulpos)pulpos++;
find_pulse:;
      if(ui.rx_rf_channels == 2)
        {
        sprintf(s,"Time difference between channels %7.4f samples.",
                                           summed_timediff/no_of_timediff);
        lir_text(38,TOPLINE+1,s);
        }
      lir_sched_yield();
      if(thread_command_flag[THREAD_CAL_FILTERCORR]!=THRFLAG_ACTIVE)
        {
        goto check_command_flag;
        }
      if(pulpos >= max_pulpos)goto go_get_data;
      dt1=current_time();
      if(dt1 - redraw_time > 0.1)
        {
        redraw_time=dt1;
        show_flag=TRUE;
        sprintf(s,
        "Accepted %4d Wide %4d Weak %4d S/N %4d Spur %4d  Gain y=%.2f x=%.2f xpos %d",
                          pulse_error[0],pulse_error[1],pulse_error[2],
                        pulse_error[3],pulse_error[4],cal_ygain,1/cal_xgain,cal_xshift);
        lir_text(0,TOPLINE+2,s);
        lir_refresh_screen();
        }
      ia=pulpos-(int)(cal_interval/2);
      ib=pulpos+(int)(cal_interval/2);
      if(ib < max_pulpos)
        {
        ampmax=0;
        for(i=ia; i<=ib; i++)
          {
          if(cal_buf7[i] > 1.1*ampmax)
            {
            ampmax=cal_buf7[i];
            pulpos=i;
            }
          }
// Copy data into cal_buf2 so the pulse becomes placed at location 0
// Also copy the power function to cal_fft1_sumsq with the pulse at location 0
        k=pulpos;
        for(i=0; i<fft2_size/2; i++)
          {
          for(j=0; j<mm; j++)
            {
            cal_buf2[i*mm+j]=cal_buf[k*mm+j];
            }
          cal_fft1_sumsq[i]=cal_buf7[k];
          k++;
          }
        k=pulpos-1;  
        for(i=fft1_size-1; i >= fft1_size-fft2_size/2; i--)
          {
          for(j=0; j<mm; j++)cal_buf2[i*mm+j]=cal_buf[k*mm+j];
          cal_fft1_sumsq[i]=cal_buf7[k];
          k--;
          }
// Find the width of the current pulse.
// First find the maximum power.
        k=pulpos;
        pulpos+=(int)cal_interval;
        if( ampmax < old_trig_power/20)
          {
          pulse_error[2]++;
          goto find_pulse;
          }
// Now step from the peak position until we reach power/8  
        ampmax/=8;
        ja=0;
        jb=fft1_size-1;    
        while( cal_fft1_sumsq[ja] > ampmax && ja<fft1_size)ja++;
        while( cal_fft1_sumsq[jb] > ampmax && jb>ja)jb--;
        width=ja+fft1_size-jb;
        if(width > cal_interval/4)
          {
          pulse_error[1]++;
          goto find_pulse;
          }
// Some soundcards like Maya44 have a time shift between the channels.
// compute the center of gravity of the pulse power in the two channels
// and accumulate the difference. We will use it to shift the pulse
// positions by changing the first derivative of the phase. 
        if(ui.rx_rf_channels == 2)
          {
          t1=0;
          t2=0;
          r1=0;
          r2=0; 
          for(i=-width; i<width; i++)
            {
            t1+=cal_buf[(k+i)*mm  ]*cal_buf[(k+i)*mm  ]+
                cal_buf[(k+i)*mm+1]*cal_buf[(k+i)*mm+1];
            t2+=i*(cal_buf[(k+i)*mm  ]*cal_buf[(k+i)*mm  ]+
                   cal_buf[(k+i)*mm+1]*cal_buf[(k+i)*mm+1]);
            r1+=cal_buf[(k+i)*mm+2]*cal_buf[(k+i)*mm+2]+
                cal_buf[(k+i)*mm+3]*cal_buf[(k+i)*mm+3];
            r2+=i*(cal_buf[(k+i)*mm+2]*cal_buf[(k+i)*mm+2]+
                   cal_buf[(k+i)*mm+3]*cal_buf[(k+i)*mm+3]);
            }
          summed_timediff+=r2/r1-t2/t1;
          no_of_timediff++;
          if(no_of_timediff < 100)goto find_pulse;
          }
// Filter the power function to reduce noise. Place in cal_fft1_slowsum.
        width/=2;
        if(width < (int)(cal_interval/64))width=(int)(cal_interval/64);
        if(width < 4)width=4;
        ja=fft1_size-width;
        jb=width;
        width=2*width+1;
        dsum1=0;
        for(i=0;    i<=jb;     i++)dsum1+=cal_fft1_sumsq[i];
        for(i=ja; i<fft1_size; i++)dsum1+=cal_fft1_sumsq[i];
        for(i=0; i<fft1_size; i++)
          {
          jb=(jb+1)&(fft1_size-1);
          if(jb >= fft1_size)jb-=fft1_size;
          cal_fft1_slowsum[i]=(float)dsum1/(float)width;
          dsum1+=cal_fft1_sumsq[jb]-cal_fft1_sumsq[ja];
          ja=(ja+1)&(fft1_size-1);
          } 
// Locate the surrounding pulses.
        ja=(int)(0.5F*cal_interval);
        jb=(int)(1.5F*cal_interval);
        t1=0;
        for(i=ja; i<jb; i++)
          {
          if(cal_fft1_slowsum[i] > t1)
            {
            t1=cal_fft1_slowsum[i];
            ib=i;
            }
          }  
        ja=fft1_size-(int)(1.5F*cal_interval);
        jb=fft1_size-(int)(0.5F*cal_interval);
        t1=0;
        ic=ja;
        for(i=ja; i<jb; i++)
          {
          if(cal_fft1_slowsum[i] > t1)
            {
            t1=cal_fft1_slowsum[i];
            ic=i;
            }
          }
// ib and ic point to the peaks of surrounding pulses.
// skip a range after the previous pulse and before the next one.
        ib=(int)((1-SKIP_BEFORE)*(float)ib);
        ic=fft1_size-(int)((float)(fft1_size-ic)*(1.0F-SKIP_AFTER));
// When we pick the range ic to ib we may create a discontinuity.
// because there may be a DC level.
// Compute the DC level around ib and ic and add a straight line
// that will make the average DC level zero at both ends. 
        mask=fft1_size-1;
        jj=2+(int)0.05*cal_interval;
        for(j=0;j<mm; j++)
          {
          sum[j]=0;
          isum[j]=0;
          }
        for(i=0; i<jj; i++)
          {
          for(j=0;j<mm; j++)
            {
            sum[j]+=cal_buf2[(ib+i+1)*mm+j]+cal_buf2[(ib-i)*mm+j];
            isum[j]+=cal_buf2[(ic+i)*mm+j]+cal_buf2[(ic-i-1)*mm+j];
            }
          }  
        for(j=0;j<mm; j++)
          {
          sum[j]/=2*jj;
          isum[j]/=2*jj;
          sum[j]=(sum[j]-isum[j])/( (ib-ic-1)&mask );
          }
        ja=ic;
        while(ja != ib)
          {
          for(j=0; j<mm; j++)
            {
            cal_buf2[ja*mm+j]-=(float)isum[j];
            isum[j]+=sum[j];
            }
          ja=(ja+1)&mask;
          }
// Attenuate before and after the pulse with a cos squared window.
        k=fft1_size-ic;
        if(ib < k)k=ib;
        k/=2;
        ja=ic+k;
        jb=ib-k;
        t1=0;
        for(i=0; i<k; i++)
          {
          t2=(float)pow(cos(t1),2.0);
          for(j=0; j<mm; j++)
            {
            cal_buf2[ja*mm+j]*=t2;
            cal_buf2[jb*mm+j]*=t2;
            }
          t1+=(float)PI_L/(2*(float)k);
          ja--;
          jb++;
          }
        ia=ja;
        ib=jb;
        for(i=jb; i<=ja; i++)
          {
          for(j=0; j<mm; j++)
            {
            cal_buf2[i*mm+j]=0;
            }
          }
        if(show_flag)
          {  
          if(cal_domain == 1)
            {
            for(j=0; j<mm; j+=2)
              {
              for(i=0; i<screen_width; i++)
                {
                k=i-screen_width/3;
                k=(int)((float)k*cal_xgain);
                k+=cal_xshift;
                k&=(fft1_size-1);
                lir_setpixel(i, cal_graph[screen_width*j+i], 0);  
                t2=0.00001F*cal_ygain*cal_buf2[mm*k+j];   
                if(t2 <-cal_ymax)t2=-cal_ymax;
                if(t2 >cal_ymax)t2=cal_ymax;
                if(j > 1)t2-=0.32F;
                cal_graph[screen_width*j+i]=
                              (short int)((float)screen_height*(cal_yzer-t2));
                lir_setpixel(i, cal_graph[screen_width*j+i], 13);  
                lir_setpixel(i, cal_graph[screen_width*(j+1)+i], 0);  
                t2=0.00001F*cal_ygain*cal_buf2[mm*k+j+1];   
                if(t2 <-cal_ymax)t2=-cal_ymax;
                if(t2 >cal_ymax)t2=cal_ymax;
                if(j > 1)t2-=0.32F;
                cal_graph[screen_width*(j+1)+i]=
                               (short int)((float)screen_height*(cal_yzer-t2));
                lir_setpixel(i, cal_graph[screen_width*(j+1)+i], 10);  
                }
              }
            lir_refresh_screen();
            show_flag=FALSE;
            }
          }
        for(j=0; j<ui.rx_rf_channels; j++)collect_powermax[j]=0;  
// Now that we have a single pulse (hopefully), get the
// fourier transform of it.
        for(j=0; j<mm; j+=2)
          {
          jj=j/2;
          for(i=0; i<ib; i++)
            {
            cal_tmp[2*i  ]=cal_buf2[mm*i+j  ];
            cal_tmp[2*i+1]=cal_buf2[mm*i+j+1];
            }
          k=fft2_size-1;  
          for(i=fft1_size-1; i>ia; i--)  
            {
            cal_tmp[2*k  ]=cal_buf2[mm*i+j  ];
            cal_tmp[2*k+1]=cal_buf2[mm*i+j+1];
            k--;
            }
         while(k >= ib)
            {
            cal_tmp[2*k  ]=0;
            cal_tmp[2*k+1]=0;
            k--;
            }
          fftforward(fft2_size, fft2_n, cal_tmp, fft2_tab, fft2_permute,fft2_n > 12);
          if( (ui.rx_input_mode&IQ_DATA) != 0)fft_iqshift(fft2_size, cal_tmp);
          for(i=0; i<fft2_size; i++)
            {
            cal_buf2[mm*i+j  ]=cal_tmp[2*i  ];
            cal_buf2[mm*i+j+1]=cal_tmp[2*i+1];
            }
// The complex transform must not contain high frequencies because the
// filter function must vary slowly with frequency.
// Store a low pass filtered version in cal_buf3 as complex numbers
// with the corresponding filtered powers in cal_buf6
          dsum1=dsum2=0;
          n=2*siz128;
          for(i=2; i<n+2; i++)
            {
            dsum1+=cal_tmp[2*i];
            dsum2+=cal_tmp[2*i+1];
            }
          for(i=0; i<siz128+2; i++)
            {
            cal_buf3[mm*i+j  ]=(float)(dsum1/n);
            cal_buf3[mm*i+j+1]=(float)(dsum2/n);
            cal_buf6[mm*i+j  ]=(float)(pow(dsum1/n,2.0)+pow(dsum2/n,2.0));
            }
          ja=fft2_size-siz128-1;  
          if( (ui.rx_input_mode&IQ_DATA) == 0)
            {
            for(i=siz128+2; i<ja; i++)
              {
              cal_buf3[mm*i+j  ]=(float)(dsum1/n);
              cal_buf3[mm*i+j+1]=(float)(dsum2/n);
              cal_buf6[mm*i+j  ]=(float)(pow(dsum1/n,2.0)+pow(dsum2/n,2.0));
              if(cal_buf6[mm*i+j  ]>collect_powermax[jj])
                {
                collect_powermax[jj]=cal_buf6[mm*i+j  ];
                }
              dsum1+=cal_tmp[2*(i+siz128)  ]-cal_tmp[2*(i-siz128)  ];
              dsum2+=cal_tmp[2*(i+siz128)+1]-cal_tmp[2*(i-siz128)+1];
              }
            }
// ******************************************************
// In direct conversion mode there may be a discontinuity at fft2_size/2
// because the A/D converters are AC coupled.      
          else
            {
            jb=fft2_size/2-siz128-1;
            for(i=siz128+2; i<jb; i++)
              {
              cal_buf3[mm*i+j  ]=(float)(dsum1/n);
              cal_buf3[mm*i+j+1]=(float)(dsum2/n);
              cal_buf6[mm*i+j  ]=(float)(pow(dsum1/n,2.0)+pow(dsum2/n,2.0));
              if(cal_buf6[mm*i+j  ]>collect_powermax[jj])
                { 
                collect_powermax[jj]=cal_buf6[mm*i+j  ];
                }
              dsum1+=cal_tmp[2*(i+siz128)]-cal_tmp[2*(i-siz128)];
              dsum2+=cal_tmp[2*(i+siz128)+1]-cal_tmp[2*(i-siz128)+1];
              }
            for(i=jb; i<fft2_size/2; i++)
              {
              cal_buf3[mm*i+j  ]=(float)(dsum1/n);
              cal_buf3[mm*i+j+1]=(float)(dsum2/n);
              cal_buf6[mm*i+j  ]=(float)(pow(dsum1/n,2.0)+pow(dsum2/n,2.0));
              }
            dsum1=dsum2=0;
            jb=fft2_size/2+2*siz128+3;
            for(i=fft2_size/2+3; i<jb; i++)
              {
              dsum1+=cal_tmp[2*i];
              dsum2+=cal_tmp[2*i+1];
              }
            jb=fft2_size/2+siz128+1;
            for(i=fft2_size/2+1; i<jb; i++)
              {
              cal_buf3[mm*i+j  ]=(float)(dsum1/n);
              cal_buf3[mm*i+j+1]=(float)(dsum2/n);
              cal_buf6[mm*i+j  ]=(float)(pow(dsum1/n,2.0)+pow(dsum2/n,2.0));
              }
            for(i=jb; i<ja; i++)
              {
              cal_buf3[mm*i+j  ]=(float)(dsum1/n);
              cal_buf3[mm*i+j+1]=(float)(dsum2/n);
              cal_buf6[mm*i+j  ]=(float)(pow(dsum1/n,2.0)+pow(dsum2/n,2.0));
              if(cal_buf6[mm*i+j  ]>collect_powermax[jj])
                    collect_powermax[jj]=cal_buf6[mm*i+j  ];
              dsum1+=cal_tmp[2*(i+siz128)]-cal_tmp[2*(i-siz128)];
              dsum2+=cal_tmp[2*(i+siz128)+1]-cal_tmp[2*(i-siz128)+1];
              }
            cal_buf3[ui.rx_rf_channels*fft2_size+j  ]=
                              (cal_buf3[ui.rx_rf_channels*(fft2_size+2)+j  ]+
                               cal_buf3[ui.rx_rf_channels*(fft2_size-2)+j  ])/2;
            cal_buf3[ui.rx_rf_channels*fft2_size+j+1]=
                              (cal_buf3[ui.rx_rf_channels*(fft2_size+2)+j+1]+
                               cal_buf3[ui.rx_rf_channels*(fft2_size-2)+j+1])/2;
            cal_buf6[ui.rx_rf_channels*fft2_size+j]=
                              (cal_buf6[ui.rx_rf_channels*(fft2_size+2)+j]+
                               cal_buf6[ui.rx_rf_channels*(fft2_size-2)+j])/2;
            }
          for(i=ja; i<fft2_size; i++)
            {
            cal_buf3[mm*i+j  ]=(float)(dsum1/n);
            cal_buf3[mm*i+j+1]=(float)(dsum2/n);
            cal_buf6[mm*i+j  ]=(float)(pow(dsum1/n,2.0)+pow(dsum2/n,2.0));
            }
          }
// ******************************************************
//Valid spectra:
//cal_buf5 = accumulated power and d2phase/df2 (not normalized)
//cal_buf4 = accumulated power and phase (normalised)
//cal_buf2 = current pulse: complex amplitude
//cal_buf3 = current pulse: smoothed complex amplitude
//cal_buf6 = current pulse: smoothed power
// ******************************************************
// Collect noise weighted by power spectrum 
// Also store the noise in cal_buf6
        for(j=0; j<mm; j+=2)
          {
          dc[j]=0;
          dc[j+1]=0;
          }
        if( (ui.rx_input_mode&IQ_DATA) == 0)
          {
          ib=fft2_size-siz128;
          }
        else
          {
          ib=fft2_size/2-siz128;
          }
        for(i=siz128; i<ib; i++)
          {
          for(j=0; j<mm; j+=2)
            {
            t1=cal_buf6[mm*i+j  ];
            t2=(float)(pow(cal_buf2[mm*i+j  ]-cal_buf3[mm*i+j  ],2.0)+
                       pow(cal_buf2[mm*i+j+1]-cal_buf3[mm*i+j+1],2.0));
            cal_buf6[mm*i+j+1]=t2;
            dc[j]+=t1*t2;
            dc[j+1]+=t1;
            }
          }  
        if( (ui.rx_input_mode&IQ_DATA) != 0)
          {
          ib=fft2_size-siz128;
          for(i=fft2_size/2+siz128; i<ib; i++)
            {
            for(j=0; j<mm; j+=2)
              {
              t1=cal_buf6[mm*i+j  ];
              t2=(float)(pow(cal_buf2[mm*i+j  ]-cal_buf3[mm*i+j  ],2.0)+
                         pow(cal_buf2[mm*i+j+1]-cal_buf3[mm*i+j+1],2.0));
              cal_buf6[mm*i+j+1]=t2;
              dc[j]+=t1*t2;
              dc[j+1]+=t1;
              }
            }  
          }
        for(i=0; i<siz128; i++)
          {
          for(j=0; j<mm; j+=2)
            {
            cal_buf6[mm*i+j+1]=(float)
                             (pow(cal_buf2[mm*i+j  ]-cal_buf3[mm*i+j  ],2.0)+
                              pow(cal_buf2[mm*i+j+1]-cal_buf3[mm*i+j+1],2.0));
            }
          }
        for(i=ib; i<fft2_size; i++)
          {
          for(j=0; j<mm; j+=2)
            {
            cal_buf6[mm*i+j+1]=(float)(
                               pow(cal_buf2[mm*i+j  ]-cal_buf3[mm*i+j  ],2.0)+
                               pow(cal_buf2[mm*i+j+1]-cal_buf3[mm*i+j+1],2.0));
            }
          }
        if( (ui.rx_input_mode&IQ_DATA) != 0)
          {
          ib=fft2_size/2+siz128;
          for(j=0; j<mm; j+=2)
            {
            for(i=fft2_size/2-siz128; i<ib; i++)
              {
              cal_buf6[mm*i+j+1]=(float)
                         (pow(cal_buf2[mm*i+j  ]-cal_buf3[mm*i+j  ],2.0)+
                          pow(cal_buf2[mm*i+j+1]-cal_buf3[mm*i+j+1],2.0));
              }
            }
          }  
        if(show_flag == TRUE)
          {   
          if(cal_domain == 2)
            {
            for(j=0; j<mm; j+=2)
              {
              for(i=0; i<screen_width; i++)
                {
                if( (i&0x7f) == 0)lir_sched_yield();
                k=i*fft2_size/screen_width;
                k=(int)((float)k*cal_xgain);
                k+=cal_xshift;
                k&=(fft2_size-1);
                lir_setpixel(i, cal_graph[screen_width*j+i], 0);  
                t2=0.0001F*cal_ygain*(cal_buf2[mm*k+j+1]-cal_buf3[mm*k+j+1]);   
                if(t2 <-cal_ymax)t2=-cal_ymax;
                if(t2 >cal_ymax)t2=cal_ymax;
                if(j > 1)t2-=0.32F;
                cal_graph[screen_width*j+i]=(short int)
                       ((float)screen_height*(cal_yzer-t2));
                lir_setpixel(i, cal_graph[screen_width*j+i], 13);  
                lir_setpixel(i, cal_graph[screen_width*(j+1)+i], 0);  
                t2=0.0001F*cal_ygain*cal_buf3[mm*k+j+1];   
                if(t2 <-cal_ymax)t2=-cal_ymax;
                if(t2 >cal_ymax)t2=cal_ymax;
                if(j > 1)t2-=0.32F;
                cal_graph[screen_width*(j+1)+i]=
                               (short int)((float)screen_height*(0.5F-t2));
                lir_setpixel(i, cal_graph[screen_width*(j+1)+i], 10);  
                }
              }
            lir_refresh_screen();
            show_flag=FALSE;
            }
          }
        for(j=0; j<mm; j+=2)
          {
          dc[j]/=dc[j+1];
          }
        for(j=0; j<ui.rx_rf_channels; j++)
          {
          t1=-10*(float)(log10(dc[2*j]/collect_powermax[j]));
          sprintf(s,"(%.1f)",t1);
          lir_text(8+20*(j+ui.rx_rf_channels),TOPLINE+3,s);   
          }
        if(collected_pulses < START_PULSES)
          {
          collected_pulses++;
          for(j=0; j<ui.rx_rf_channels; j++)
            {
            collect_noiselevel[j]+=dc[2*j];
            collect_powerlevel[j]+=collect_powermax[j];
            t1=-10*(float)(log10(collect_noiselevel[j]/collect_powerlevel[j]));
            sprintf(s,"S/N  %.1fdB",t1);
            lir_text(20*j,TOPLINE+3,s);   
            }
          if(collected_pulses < START_PULSES)goto go_get_data;
          for(j=0; j<ui.rx_rf_channels; j++)
            {
            collect_noiselevel[j]/=START_PULSES;
            collect_powerlevel[j]/=START_PULSES;
            }
          if(t1 < STON_LIMIT)
            {
            clear_lines(3,13);
            sprintf(s,"Pulse S/N not good enough (%.1f) Limit %ddB",
                                                               t1,STON_LIMIT);
            lir_text(0,ERRLINE,s);
            lir_text(0,ERRLINE+1,"Amplify pulses, reduce noise or increase PRF");
            lir_text(0,ERRLINE+2,
                            "Reason may also be old bad correction function");
            lir_text(5,ERRLINE+4,"I => Ignore (=try anyway)");
            lir_text(5,ERRLINE+5,"S => Save (bad?) correction on hard disk.");
            lir_text(5,ERRLINE+6,"A => Clear old correction function.");
            lir_refresh_screen();
            goto errkbd;
            }
          }
        for(j=0; j<ui.rx_rf_channels; j++)
          {
          t1=collect_noiselevel[j]/collect_powerlevel[j];
          t2=dc[2*j]/collect_powermax[j];
          if(t2<0.4*t1)
            {
            pulse_error[3]++;
            goto find_pulse;
            }
          if(1.5*collect_powermax[j]<collect_powerlevel[j])
            {
            pulse_error[2]++;
            goto find_pulse;
            }
          }
// ******************************************************
//Valid spectra:
//cal_buf5 = accumulated power and d2phase/df2 (not normalised)
//cal_buf4 = accumulated power and phase (normalised)
//cal_buf2 = current pulse: complex amplitude
//cal_buf3 = current pulse: smoothed complex amplitude
//cal_buf6 = current pulse: smoothed power and noise power
// ******************************************************
// This pulse is probably ok.
// Convert the complex data in cal_buf2 to power and phase.
// Store noise as square of amplitude differences between 
// raw and filtered data.
        for(i=0; i<fft2_size; i++)
          {
          for(j=0; j<mm; j+=2)
            {
            t1=(float)(pow(cal_buf2[mm*i+j  ],2.0)+
                       pow(cal_buf2[mm*i+j+1],2.0));
            if(t1 > 0.001F)
              {
              cal_buf2[mm*i+j+1]=(float)atan2(cal_buf2[mm*i+j+1],cal_buf2[mm*i+j  ]);
              }
            else  
              {
              cal_buf2[mm*i+j+1]=0;
              }
            cal_buf2[mm*i+j  ]=t1;
            }
          } 
// Get the first derivative of the phase function
        for(j=0; j<mm; j+=2)
          {
          for(i=0; i<fft2_size-1; i++)
            {
            cal_buf2[mm*i+j+1]=cal_buf2[mm*(i+1)+j+1]-cal_buf2[mm*i+j+1];
            if(cal_buf2[mm*i+j+1] >  PI_L)cal_buf2[mm*i+j+1] -= 2*(float)PI_L;
            if(cal_buf2[mm*i+j+1] < -PI_L)cal_buf2[mm*i+j+1] += 2*(float)PI_L;
            }
          }  
// Get the second derivative of the phase function
        for(j=0; j<mm; j+=2)
          {
          for(i=0; i<fft2_size-2; i++)
            {
            cal_buf2[mm*i+j+1]=cal_buf2[mm*(i+1)+j+1]-cal_buf2[mm*i+j+1];
            if(cal_buf2[mm*i+j+1] >  PI_L)cal_buf2[mm*i+j+1] -= 2*(float)PI_L;
            if(cal_buf2[mm*i+j+1] < -PI_L)cal_buf2[mm*i+j+1] += 2*(float)PI_L;
            }
          }
// ******************************************************
//Valid spectra:
//cal_buf5 = accumulated power and d2phase/df2 (not normalised)
//cal_buf4 = accumulated power and phase (normalised)
//cal_buf2 = current pulse: power and d2phase/df2
//cal_buf3 = current pulse: smoothed complex amplitude
//cal_buf6 = current pulse: smoothed power and noise power
// ******************************************************
// Step through the data and look for points with excessive noise.
// Make sure the integral is smaller than PI over the bad interval.
// Collect 10 times the average noise power weighted by the 
// smoothed power in dc[j] 
// Also save 0.1 times the largest smoothed power in dc[j+1] so we
// easily can see if a point is outside the passband.
        k=siz128/2;
        for(j=0; j<mm; j+=2)
          {
          dc[j]=0;
          t1=0;
          t2=0;
          for(i=siz128; i<fft2_size-siz128; i++)
            {
// When computing the noise as the deviation from a smoothed curve
// we get errors in regions where the curvature is large.
// Subtract the square of the second derivative so we do not
// make mistakes believing a spur is present just because the 
// hardware has sharp filters.   
            t3=cal_buf6[mm*i+j+1];
            t3-=(float)((pow((2*cal_buf3[mm*(i)+j  ]-
                                cal_buf3[mm*(i+k)+j  ]-
                                cal_buf3[mm*(i-k)+j  ]),2.)+
                         pow((2*cal_buf3[mm*(i)+j+1]-
                                cal_buf3[mm*(i+k)+j+1]-
                                cal_buf3[mm*(i-k)+j+1]),2.)));
            if(t3 < 0)
              {
              cal_buf6[mm*i+j+1]=0;
              }
            else
              {
              cal_buf6[mm*i+j+1]=(float)sqrt(t3);
              }  
            t3=cal_buf6[mm*i+j  ];
            dc[j]+=t3*cal_buf6[mm*i+j+1];
            t2+=t3;
            if(t1 < t3 ) t1=t3;
            }
// Set dc[j] to 10 times the average noise power
// and dc[j+1] to 0.1 times the largest power. 
          dc[j]*=10/t2;
          dc[j+1]=0.1F*t1;
          }
        n=0;
        ja=siz128;
spur_remove:;
        k=0;
        while(ja < fft2_size-siz128  && k==0)
          {
          ja++;
          for(j=0; j<mm; j+=2)
            {
            if(dc[j]<cal_buf6[mm*ja+j+1])k=1;
            if(fabs(cal_buf2[mm*ja+j+1]-cal_buf2[mm*(ja+1)+j+1])>0.5*PI_L)k=1;
            if(fabs(cal_buf2[mm*ja+j+1]+
                    cal_buf2[mm*(ja+1)+j+1]+
                    cal_buf2[mm*(ja-11)+j+1])>PI_L)k=1;
            }
          }
        if(ja>=fft2_size-siz128)goto spurrem_x;
// ja is first bad point  
        jb=ja;
        while(jb < fft2_size-siz128  && k==1)
          {
          k=0;
          jb++;
          for(j=0; j<mm; j+=2)
            {
            if(dc[j]<cal_buf6[mm*jb+j+1])k=1;
            if(fabs(cal_buf2[mm*jb+j+1]-cal_buf2[mm*(jb+1)+j+1])>0.5*PI_L)k=1;
            if(fabs(cal_buf2[mm*jb+j+1]+
                    cal_buf2[mm*(jb+1)+j+1]+
                    cal_buf2[mm*(jb-11)+j+1])>PI_L)k=1;
            }
          }
// jb-1 is last bad point.
// Check that we are within the passband.
        k=0;
        for(j=0; j<mm; j+=2)
          {
          if(cal_buf6[mm*ja+j] > dc[j+1] || cal_buf6[mm*jb+j] > dc[j+1])k=1;
          }
        if(k==1)
          {
          n+=jb-ja;
          if(jb-ja-1>fft2_size/128 || n > fft2_size/32)
            {
spurerr:;
            pulse_error[4]++;
            goto find_pulse;
            }
          }  
        ja-=fft2_size/256;
        if(ja<0)ja=0;
        jb+=fft2_size/256;
        if(jb>=fft2_size)jb=fft2_size-1;
        for(j=0; j<mm; j+=2)
          {
          m=0;  
          ka=0;
          kb=0;
rem_tupi:;
          t1=0;
          t2=BIGFLOAT;
          t3=0;
// Calculate the integral across the region ja to jb
          for(i=ja; i<jb; i++)
            {
            t1+=cal_buf2[mm*i+j+1];
            if(cal_buf2[mm*i+j+1]<t2)
              {
              t2=cal_buf2[mm*i+j+1];
              ka=i;
              }
            if(t3>cal_buf2[mm*i+j+1])
              {
              t3=cal_buf2[mm*i+j+1];
              kb=i;
              }
            }
          m++;
// If abs of the integral is larger than PI, change the largest point. 
          if(m < 5)
            {
            if(t1 < -PI_L)
              {
              cal_buf2[mm*kb+j+1]+=2*(float)PI_L;
              goto rem_tupi;
              }
            if(t1 > PI_L)
              {
              cal_buf2[mm*ka+j+1]-=2*(float)PI_L;
              goto rem_tupi;
              }
            }
          if(fabs(t1) > PI_L/4 && cal_buf6[mm*ja+j] > 
                                     dc[j+1] && cal_buf6[mm*jb+j] > dc[j+1])
            {
            goto spurerr;
            }
          }
          ja=jb-fft2_size/256+1;
          if(ja < fft2_size-1-fft2_size/256)goto spur_remove;
spurrem_x:
// *******************************************************************
//The pulse seems ok.
//Add the new power spectrum to the powers already collected in cal_buf5.
        avg_power=0;
        for(i=0; i<fft2_size; i++)
          {
          for(j=0; j<mm; j+=2)
            {
            cal_buf5[mm*i+j]+=cal_buf2[mm*i+j];
            avg_power+=cal_buf2[mm*i+j];
            }
          }
        avg_power/=(float)fft2_size;
        summed_power+=avg_power;
// Add the new d2phase weighted by power into cal_buf5
        for(j=0; j<mm; j+=2)
          {
          for(i=0; i<fft2_size-2; i++)
            {
            cal_buf5[mm*i+j+1]+=cal_buf2[mm*i+j+1]*avg_power;
            }
          }
        pulse_error[0]++;
// ******************************************************
//Valid spectra:
//cal_buf5 = accumulated power and d2phase/df2 (not normalised)
// ******************************************************
// Place the normalised average power and d2phase in cal_buf4 
        for(j=0; j<mm; j+=2)
          {
          for(i=0; i<fft2_size; i++)
            {
            cal_buf4[mm*i+j  ]=cal_buf5[mm*i+j  ]/summed_power;
            cal_buf4[mm*i+j+1]=cal_buf5[mm*i+j+1]/summed_power;
            }
          }
// Integrate d2phase so we get d1phase (backwards so the sign becomes wrong).
        for(j=1; j<mm; j+=2)
          {
          cal_buf4[mm*(fft2_size-2)+j]=0;
          for(i=fft2_size-2; i>0; i--)cal_buf4[mm*(i-1)+j]+=cal_buf4[mm*i+j];
          }
// Remove the integration constant.
// Choose it for (d1phase weighted by power) = 0.
// Exclude points near the spectrum ends. 
        k=fft2_size/16;
        for(j=0; j<mm; j+=2)
          {
          dt1=0;
          dt2=0;
          for(i=k; i<fft2_size-k; i++)
            {
            dt3=cal_buf4[mm*i+j]+cal_buf4[mm*(i-1)+j]+cal_buf4[mm*(i+1)+j];
            dt1+=dt3*cal_buf4[mm*i+j+1];
            dt2+=dt3;
            }
          dt1/=dt2;  
          for(i=0; i<fft2_size-1; i++) cal_buf4[mm*i+j+1]-=(float)dt1;
          }
// When we have two RF channels, move the pulses in the time domain
// to make them coincide.
        if(ui.rx_rf_channels == 2)
          {
          t1=PI_L/fft2_size;
          t1*=summed_timediff/no_of_timediff;
          for(i=0; i<fft2_size; i++)
            {
            cal_buf4[mm*i+1]+=t1;
            cal_buf4[mm*i+3]-=t1;
            }
          }
// ******************************************************
//Valid spectra:
//cal_buf5 = accumulated power and dphase/df (not normalised)
//cal_buf4 = accumulated power and dphase/df (normalised)
// ******************************************************
// Integrate d1phase so we get the phase. 
// Backwards so the sign becomes right again.
        for(j=1; j<mm; j+=2)
          {
          cal_buf4[mm*(fft2_size-1)+j]=0;
          for(i=fft2_size-1; i>0; i--)
            {
            cal_buf4[mm*(i-1)+j]+=cal_buf4[mm*i+j];
            }
          }
// Remove the integration constant.
// Choose it for (phase weighted by power) = 0.
        for(j=0; j<mm; j+=2)
          {
          dt1=0;
          dt2=0;
          for(i=1; i<fft2_size-1; i++)
            {
            dt3=cal_buf4[mm*i+j];
            dt1+=dt3*cal_buf4[mm*i+j+1];
            dt2+=dt3;
            }
          dt1/=dt2; 
          for(i=0; i<fft2_size; i++) cal_buf4[mm*i+j+1]-=(float)dt1;
          }
// Limit phase angle to +/- pi
// Filters with large phase shifts will be difficult to see 
// on screen otherwise.
        for(j=0; j<mm; j+=2)
          {
          for(i=0; i<fft2_size; i++)
            {
            t1=cal_buf4[mm*i+j+1]/(2*(float)PI_L);
            t1=t1-(float)((int)(t1));
            if(t1 < -0.5)t1+=1;
            if(t1 >  0.5)t1-=1;
            cal_buf4[mm*i+j+1]=t1*2*(float)PI_L;
            }
          }
        if(show_flag == TRUE)
          {   
          if(cal_domain == 0)
            {
            for(j=0; j<mm; j+=2)
              {
              for(i=0; i<screen_width; i++)
                {
                if( (i&0x7f) == 0)lir_sched_yield();
                k=i*fft2_size/screen_width;
                k=(int)((float)k*cal_xgain);
                k+=cal_xshift;
                k&=(fft2_size-1);
                lir_setpixel(i, cal_graph[screen_width*j+i], 0);  
                t2=0.1F*cal_ygain*cal_buf4[mm*k+j];   
                if(t2 <-cal_ymax)t2=-cal_ymax;
                if(t2 >cal_ymax)t2=cal_ymax;
                if(j > 1)t2-=0.32F;
                cal_graph[screen_width*j+i]=(short int)
                       ((float)screen_height*(cal_yzer-t2));
                lir_setpixel(i, cal_graph[screen_width*j+i], 13);  
                lir_setpixel(i, cal_graph[screen_width*(j+1)+i], 0);  
                t2=0.03F*cal_ygain*cal_buf4[mm*k+j+1];   
                if(t2 <-cal_ymax)t2=-cal_ymax;
                if(t2 >cal_ymax)t2=cal_ymax;
                if(j > 1)t2-=0.32F;
                cal_graph[screen_width*(j+1)+i]=
                               (short int)((float)screen_height*(0.5F-t2));
                lir_setpixel(i, cal_graph[screen_width*(j+1)+i], 10);  
                }
              }
            lir_refresh_screen();
            show_flag=FALSE;
            }
          }
        goto find_pulse;
        }
      }
    }
go_get_data:;
  }
check_command_flag:;
if(thread_command_flag[THREAD_CAL_FILTERCORR]==THRFLAG_IDLE)
  {
  goto idle;
errkbd:;
idle:;
  while(thread_command_flag[THREAD_CAL_FILTERCORR]==THRFLAG_ACTIVE)
    {
    lir_sleep(2000);
    if(kill_all_flag)goto filcorr_error_exit;
    }
  thread_status_flag[THREAD_CAL_FILTERCORR]=THRFLAG_IDLE;
  while(thread_command_flag[THREAD_CAL_FILTERCORR]==THRFLAG_IDLE)
    {
    lir_await_event(EVENT_FFT1_READY);
    old_pb=fft1_pb;
    fft1_px=fft1_pb;
    }
  clear_lines(ERRLINE,ERRLINE+5);
  if(thread_command_flag[THREAD_CAL_FILTERCORR]==THRFLAG_ACTIVE)goto restart;
  if(thread_command_flag[THREAD_CAL_FILTERCORR]==THRFLAG_RESET)
    {
    thread_command_flag[THREAD_CAL_FILTERCORR]=THRFLAG_ACTIVE;
    goto clr_restart;
    }
  }  
filcorr_error_exit:;
thread_status_flag[THREAD_CAL_FILTERCORR]=THRFLAG_RETURNED;
while(!kill_all_flag && 
         thread_command_flag[THREAD_CAL_FILTERCORR] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}
