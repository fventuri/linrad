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
#include <ctype.h>
#include "uidef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "caldef.h"
#include "thrdef.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif



#define INFOLINE 3

void cal_initscreen(void)
{
int i;
char s[256];
clear_screen();
sprintf(s,"%s for %s. Press F1 or '!' for info.",cal_type_text[cal_type],
                                                     rxmodes[rx_mode]);
for(i=0; i<screen_width*(2*MAX_ADCHAN); i++)cal_graph[i]=screen_height-1;
settextcolor(14);
lir_text(10,0,s);
settextcolor(7);
lir_refresh_screen();
}

void normalise_fft1_filtercorr(float *out)
{
int i, j, k;
int mm;
float t1, t2, sum[2];
mm=2*ui.rx_rf_channels;
for(i=0; i<mm*fft1_size; i++)out[i]=1000000000*fft1_filtercorr[i];
//Make the average gain equal to one (weighted by desired spectrum)
t1=0;
sum[0]=0;
sum[1]=0;
for(i=0; i<fft1_size; i++)
  {
  t2=pow(fft1_desired[i],2.0);
  t1+=t2;
  k=0;
  for(j=0; j<mm; j+=2)
    {
    sum[k]+=t2*(pow(out[mm*i+j  ],2.0)+
                pow(out[mm*i+j+1],2.0));
    k++;
    }
  }  
sum[0]=sqrt(sum[0]/t1);
sum[1]=sqrt(sum[1]/t1);
for(i=0; i<fft1_size; i++)
  {
  k=0;
  for(j=0; j<mm; j+=2)
    {
    out[mm*i+j  ]/=sum[k];
    out[mm*i+j+1]/=sum[k];
    k++;
    }
  }
}

void write_filcorr(int siz)
{
FILE *file;
int i;
int wrbuf[10];
char s[80];
make_filfunc_filename(s);
file = fopen(s, "wb");
if (file == NULL)
  {
  DEB"\nCould not open file %s\n",s);  
  lirerr(1144);
  return;
  }
wrbuf[2]=rx_mode;
wrbuf[3]=ui.rx_input_mode;
wrbuf[4]=genparm[FIRST_FFT_VERNR];
wrbuf[5]=ui.rx_ad_speed;
wrbuf[6]=ui.rx_rf_channels;
if(siz >= 0)
  {
  wrbuf[0]=fft1_n;
  wrbuf[1]=fft1_size;
  wrbuf[7]=siz;
  }
else
  {
  wrbuf[0]=fft2_n;
  wrbuf[1]=fft2_size;
  wrbuf[7]=0;
  }  
for(i=8; i<10; i++)wrbuf[i]=0;
i=fwrite(wrbuf, sizeof(int),10,file);
if(i != 10)
  {
errx:  
  fclose(file);
  lirerr(1145);
  return;
  }
if(siz < 0)
  {
  i=fwrite(cal_fft1_filtercorr, 2*ui.rx_rf_channels*sizeof(float),fft2_size, file);  
  if(i != fft2_size)goto errx;
  i=fwrite(cal_fft1_desired,sizeof(float),fft2_size,file);
  if(i != fft2_size)goto errx;
  }
else
  {  
  if(siz == 0)
    {
    normalise_fft1_filtercorr(cal_tmp);
    i=fwrite(cal_tmp, twice_rxchan*sizeof(float),fft1_size, file);  
    if(i != fft1_size)goto errx;
    i=fwrite(fft1_desired,sizeof(float),fft1_size,file);
    if(i != fft1_size)goto errx;
    }
  else
    {
    i=fwrite(cal_buf, 2*ui.rx_rf_channels*sizeof(float),siz, file);  
    if(i != siz)goto errx;
// Store fft1_desired in siz points in cal_buf2
    resize_fft1_desired(fft1_size, fft1_desired, siz, cal_buf2);
    i=fwrite(cal_buf2,sizeof(float),siz,file);
    if(i != siz)goto errx;
    }  
  }
i=fwrite(wrbuf, sizeof(int),10,file);
if(i != 10)goto errx;
fclose(file);
}


void do_cal_interval(void)
{
int i, j, k, l, m, n, wid,first_pulse;
unsigned char color;
int p0, ia, ib, ic, ja, jb;
int imode,  lim;
int rawbytes;
int maxval[MAX_ADCHAN],minval[MAX_ADCHAN];
int old_end;
int points, mask; 
int textend,ysiz,ytop;
char s[80];
float *filpwr;
float pwr_l, pwr_h;
float old_trig_power,trig_power,dc[MAX_ADCHAN],ampmax;
float noise_floor;
float t1,t2,t3;
// Collect 0.5 seconds of raw data. 
rawbytes=ui.rx_ad_speed*snd[RXAD].framesize;
if(rawbytes > timf1_bytes-3*timf1_blockbytes)
                                      rawbytes=timf1_bytes-3*timf1_blockbytes;
if(rawbytes < 0.25*ui.rx_ad_speed*snd[RXAD].framesize && fft1_size < 32768)
  {
  lirerr(1241);  
  goto calint_error_exit;
  }
points=rawbytes/snd[RXAD].framesize;
make_power_of_two(&points);
points >>= 1;
rawbytes=points*snd[RXAD].framesize;
if(rawbytes > fft1_bytes/2)
  {
  rawbytes=fft1_bytes/2;
  points=rawbytes/snd[RXAD].framesize;
  }
filpwr=&fft1_float[points];
old_trig_power=1;
old_end=-1;
cal_ygain=1;
cal_xgain=1;
cal_interval=0;
imode=ui.rx_input_mode&(DWORD_INPUT+TWO_CHANNELS+IQ_DATA);
restart:;
cal_signal_level=0;  
cal_type=CAL_TYPE_PULSE_INTERVAL;
cal_initscreen();
lir_text(3, 1,"Connect pulse generator to all antenna inputs");
lir_text(3, 2,"Press ENTER when OK.    (+),(-) for y-scale     E,C for x-scale");
lir_text(0,INFOLINE,"DC offset");
lir_text(5,INFOLINE+1,"Min");
lir_text(5,INFOLINE+2,"Max"); 
lir_text(3,INFOLINE+3,"Level"); 
lir_refresh_screen();
timf1p_px=timf1p_pa;
thread_status_flag[THREAD_CAL_INTERVAL]=THRFLAG_ACTIVE;
while(thread_command_flag[THREAD_CAL_INTERVAL] == THRFLAG_ACTIVE)
  {
// Get power as a function of time.
// The A/D converters are AC coupled so we remove DC components.
  for(i=0; i<MAX_ADCHAN; i++)
    {
    maxval[i]=0x80000000; 
    minval[i]=0x7fffffff;
    dc[i]=0;
    }
  while( ((timf1p_pa-timf1p_px+timf1_bytes)&timf1_bytemask) < rawbytes &&
                  thread_command_flag[THREAD_CAL_INTERVAL] == THRFLAG_ACTIVE)
    {
    lir_await_event(EVENT_TIMF1);
    }
  timf1p_px=(timf1p_pa-rawbytes+timf1_bytes)&timf1_bytemask;
  if( (ui.rx_input_mode & DWORD_INPUT) == 0)
    {
    p0=timf1p_px/sizeof(short int);
    mask=timf1_bytemask/sizeof(short int);
    }
  else
    {
    p0=timf1p_px/sizeof(int);
    mask=timf1_bytemask/sizeof(int);
    }
  switch ( imode )
    {
    case 0:
// One channel 16 bit real data    
    for(i=0; i<points; i++)
      {
      k=(p0+i)&mask;
      dc[0]+=timf1_short_int[k];
      if(minval[0]>timf1_short_int[k])minval[0]=timf1_short_int[k];
      if(maxval[0]<timf1_short_int[k])maxval[0]=timf1_short_int[k];
      }
    dc[0]/=points;
    for(i=0; i<points; i++)
      {
      k=(p0+i)&mask;
      fft1_float[i]=pow( 65536.0*(timf1_short_int[k]-dc[0]), 2.);
      }
    break;

    case 1:
// One channel 32 bit real data.  
    for(i=0; i<points; i++)
      {
      k=(p0+i)&mask;
      dc[0]+=timf1_int[k];
      if(minval[0]>timf1_int[k])minval[0]=timf1_int[k];
      if(maxval[0]<timf1_int[k])maxval[0]=timf1_int[k];
      }
    dc[0]/=points;
    for(i=0; i<points; i++)
      {
      k=(p0+i)&mask;
      fft1_float[i]=pow( timf1_int[k]-dc[0], 2.);
      }
    break;

    case 2:
// Two channels 16 bit real data  
    for(i=0; i<points; i++)
      {
      k=(p0+2*i)&mask;
      for(j=0; j<2; j++)
        {
        dc[j]+=timf1_short_int[k+j];
        if(minval[j]>timf1_short_int[k+j])minval[j]=timf1_short_int[k+j];
        if(maxval[j]<timf1_short_int[k+j])maxval[j]=timf1_short_int[k+j];
        }
      }
    for(j=0; j<2; j++)dc[j]/=points;
    for(i=0; i<points; i++)
      {
      t1=0;
      k=(p0+2*i)&mask;
      for(j=0; j<2; j++)
        {
        t2=timf1_short_int[k+j]-dc[j];
        t1+=t2*t2;
        }
      fft1_float[i]=t1*65536.0*65536.0;
      }
    break;
  
    case 3:
// Two channels 32 bit real data  
    for(i=0; i<points; i++)
      {
      k=(p0+2*i)&mask;
      for(j=0; j<2; j++)
        {
        dc[j]+=timf1_int[k+j];
        if(minval[j]>timf1_int[k+j])minval[j]=timf1_int[k+j];
        if(maxval[j]<timf1_int[k+j])maxval[j]=timf1_int[k+j];
        }
      }
    for(j=0; j<2; j++)dc[j]/=points;
    for(i=0; i<points; i++)
      {
      k=(p0+2*i)&mask;
      t1=0;
      for(j=0; j<2; j++)
        {
        t2=timf1_int[k+j]-dc[j];
        t1+=t2*t2;
        }
      fft1_float[i]=t1;
      }
    break;
  

    case 4:
// One channel 16 bit IQ data  
    for(i=0; i<points; i++)
      {
      k=(p0+2*i)&mask;
      for(j=0; j<2; j++)
        {
        dc[j]+=timf1_short_int[k+j];
        if(minval[j]>timf1_short_int[k+j])minval[j]=timf1_short_int[k+j];
        if(maxval[j]<timf1_short_int[k+j])maxval[j]=timf1_short_int[k+j];
        }
      }
    for(j=0; j<2; j++)dc[j]/=points;
    for(i=0; i<points; i++)
      {
      k=(p0+2*i)&mask;
      t1=0;
      for(j=0; j<2; j++)
        {
        t2=timf1_short_int[k+j]-dc[j];
        t1+=t2*t2;
        }
      fft1_float[i]=t1*65536.0*65536.0;
      }
    break;

    case 5:
// One channel 32 bit IQ data  
    for(i=0; i<points; i++)
      {
      k=(p0+2*i)&mask;
      for(j=0; j<2; j++)
        {
        dc[j]+=timf1_int[k+j];
        if(minval[j]>timf1_int[k+j])minval[j]=timf1_int[k+j];
        if(maxval[j]<timf1_int[k+j])maxval[j]=timf1_int[k+j];
        }
      }
    for(j=0; j<2; j++)dc[j]/=points;
    for(i=0; i<points; i++)
      {
      k=(p0+2*i)&mask;
      t1=0;
      for(j=0; j<2; j++)
        {
        t2=timf1_int[k+j]-dc[j];
        t1+=t2*t2;
        }
      fft1_float[i]=t1;
      }
    break;

    case 6:
// Two channels 16 bit IQ data  
    for(i=0; i<points; i++)
      {
      k=(p0+4*i)&mask;
      for(j=0; j<4; j++)
        {
        dc[j]+=timf1_short_int[k+j];
        if(minval[j]>timf1_short_int[k+j])minval[j]=timf1_short_int[k+j];
        if(maxval[j]<timf1_short_int[k+j])maxval[j]=timf1_short_int[k+j];
        }
      }
    for(j=0; j<4; j++)dc[j]/=points;
    for(i=0; i<points; i++)
      {
      k=(p0+4*i)&mask;
      t1=0;
      for(j=0; j<4; j++)
        {
        t2=timf1_short_int[k+j]-dc[j];
        t1+=t2*t2;
        }
      fft1_float[i]=t1*65536.0*65536.0;
      }
    break;
  
    case 7:
// Two channels 32 bit IQ data  
    for(i=0; i<points; i++)
      {
      k=(p0+4*i)&mask;
      for(j=0; j<4; j++)
        {
        dc[j]+=timf1_int[k+j];
        if(minval[j]>timf1_int[k+j])minval[j]=timf1_int[k+j];
        if(maxval[j]<timf1_int[k+j])maxval[j]=timf1_int[k+j];
        }
      }
    for(j=0; j<4; j++)dc[j]/=points;
    for(i=0; i<points; i++)
      {
      k=(p0+4*i)&mask;
      t1=0;
      for(j=0; j<4; j++)
        {
        t2=timf1_int[k+j]-dc[j];
        t1+=t2*t2;
        }
      fft1_float[i]=t1;
      }
    break;
    }
  lir_sched_yield();  
  cal_signal_level=BIGFLOAT;  
  if( (ui.rx_input_mode&(DWORD_INPUT)) == 0)lim=0x7ff0; else lim=0x7ff00000; 
  for(j=0; j<ui.rx_ad_channels; j++)
    {
    sprintf(s," %4.2f ",dc[j]);
    lir_text(10+15*j,INFOLINE,s);
    sprintf(s," %d ",minval[j]);
    lir_text(10+15*j,INFOLINE+1,s);
    sprintf(s," %d ",maxval[j]);
    lir_text(10+15*j,INFOLINE+2,s);
    t1=-minval[j];
    if(t1<maxval[j])t1=maxval[j];
    if(t1 < cal_signal_level)cal_signal_level=t1; 
    if(minval[j] <= -lim || maxval[j] >= lim)
      {
      lir_text(10+15*j,INFOLINE+3,"OVERFLOW");
      t1=1;
      }
    else
      {
      t1/=lim;
      sprintf(s," %1.6f%% ",100*t1);
      lir_text(10+15*j,INFOLINE+3,s);
      }
    }
  cal_signal_level/=lim;  
// Find the largest power value and point wid to it.
  trig_power=0;
  wid=0;
  for(i=0; i<points; i++)
    {
    if(trig_power < fft1_float[i])
      {
      trig_power=fft1_float[i];
      wid=i;
      }
    }
  lir_sched_yield();  
// Get an approximate width of the pulse (remember it may have a
// very ugly shape!)  
  i=wid;  
  while( i<points-3 && (fft1_float[i] >= trig_power/20 ||
                        fft1_float[i+1] >= trig_power/20 ||
                        fft1_float[i+2] >= trig_power/20 ||
                        fft1_float[i+3] >= trig_power/20 ))
    {
    i++;
    if(i-wid >100)goto skip;
    }
  while( wid>2 && (fft1_float[wid] >= trig_power/20 ||
                 fft1_float[wid-1] >= trig_power/20 ||
                 fft1_float[wid-2] >= trig_power/20 ||
                 fft1_float[wid-3] >= trig_power/20 ))
    {
    wid--;
    if(i-wid > 100)goto skip;
    }
  wid=(i-wid);
  wid=wid/2+1;
// Low pass filter the power function and store in filpwr
  for(i=wid; i<points; i++)
    {
    t1=0;
    for(j=i-wid; j<=i; j++)
      {
      t1+=fft1_float[j];
      }
    filpwr[i-wid/2]=t1/wid;  
    }
// Save the maximum amplitude and make trig power 31.6 times smaller
// than the maximum value (-15dB)
  ampmax=sqrt(trig_power);
  if(trig_power > old_trig_power)old_trig_power=trig_power; 
  trig_power=old_trig_power/31.6;
// Remember trig_power for the next set of data.
  old_trig_power/=2;
  ia=0;
  i=wid;
  if(wid < points/16)
    {
// find the position of the first pulse
interval_init:;
    while( i<points-3*wid && filpwr[i] <= trig_power)i++;
    ja=i;
    while( i<points-wid && filpwr[i] >= trig_power/20)i++;
    if(ja==wid)goto interval_init;
    jb=i;
    t1=0;
    for(k=ja; k<jb; k++)
      {
      if(fft1_float[k] > t1)
        {
        t1=fft1_float[k];
        ia=k;
        }
      }
    first_pulse=ia;  
// Find sucessive pulses and store the intervals in cal_tmp 
    k=0;
    cal_interval=0;
    j=0;
find_interval:;
    while( i<points-wid && filpwr[i] <= trig_power)i++;
    ja=i;
    while( i<points-wid && filpwr[i] >= trig_power/20)i++;
    jb=i;
    t1=0;
    ib=i;
    for(k=ja; k<jb; k++)
      {
      if(fft1_float[k] > t1)
        {
        t1=fft1_float[k];
        ib=k;
        }
      }
    if(i<points-wid)
      {
      if(ib-ia<4*wid)goto find_interval;
      cal_tmp[j]=ib-ia;
      j++;
      ia=ib;
      if(j<fft1_size)goto find_interval;
      }
// Since the pulses may vary in amplitude by more than we allow for in
// the setting of the trigger level we sort the intervals in ascending order
// and take desicions if the interval is 1,2 or 3 pulses.  
    if(j>0)
      {
      for(i=0; i<j-1; i++)
        {
        for(k=i+1; k<j; k++)
          {
          if(cal_tmp[k]<cal_tmp[i])
            {     
            t1=cal_tmp[k];
            cal_tmp[k]=cal_tmp[i];
            cal_tmp[i]=t1;
            }
          }
        }
      i=j/4+1;  
      t1=0;
      while(i<j && cal_tmp[i-1]>0.7*cal_tmp[i])
        {
        t1+=cal_tmp[i];
        i++;
        }
      i-=(j/4+1);
      if(i != 0)
        {
        t1/=i;
        k=0;
        for(i=0; i<j; i++)
          {
          if(cal_tmp[i]>0.8*t1 && cal_tmp[i]<1.2*t1)
            {
            cal_interval+=cal_tmp[i];
            k++;
            }
          else
            {
            if(cal_tmp[i]>1.8*t1 && cal_tmp[i]<2.2*t1)
              {
              cal_interval+=cal_tmp[i];
              k+=2;
              }
            else
              {
              if(cal_tmp[i]>2.7*t1 && cal_tmp[i]<3.3*t1)
                {
                cal_interval+=cal_tmp[i];
                k+=3;
                }
              }      
            }
          }
        cal_interval/=k;
// Get the noise floor.
// First locate the smallest power value in the filtered power vs time curve.
        pwr_l=BIGFLOAT;
        pwr_h=0;
        for(i=wid; i<points-wid; i++)
          {
          if(pwr_l>filpwr[i])pwr_l=filpwr[i];
          if(pwr_h<filpwr[i])pwr_h=filpwr[i];
          }
// Assume the noise floor is 10dB above this level
        pwr_l*=10;
// Limit our guess to the range -10dB to -60 dB
        if(pwr_l < 0.000001*pwr_h)pwr_l=0.000001*pwr_h;
        if(pwr_l > 0.1*pwr_h)pwr_l=0.1*pwr_h;
// Count the number of points above the noise floor and
// get the average value of the points below the noise floor.
        j=0;
get_floor:;
        t2=0;
        k=0;
        j++;
        if(j < 10)
          {
          for(i=wid; i<points-wid; i++)
            {
            if(filpwr[i]<pwr_l)
              {
              k++;
              t2+=fft1_float[i];
              }
            }
          t2/=k;
          if(k < 0.4*(points-2*wid))
            {
            pwr_l*=3.333;
            goto get_floor;
            }
          if(k > 0.6*(points-2*wid))
            {
            if(pwr_l > 4*t2) 
              {
              pwr_l=3*t2;
              goto get_floor;
              }
            else
              {
              if(pwr_l < 2*t2) 
                {
                pwr_l/=2;
                goto get_floor;
                }
              }
            }
          }
        else
          {
          t2=pwr_h;
          }          
        noise_floor=pwr_h/t2;
// Show one of the pulses.
        while(first_pulse+7*(screen_width>>3) > points) 
                                                first_pulse -= cal_interval; 
        while(first_pulse < (screen_width>>3))
                                                first_pulse += cal_interval;
        if(first_pulse+7*(screen_width>>3) <= points) 
          {
          ic=first_pulse+7*(screen_width>>3);
          ia=first_pulse-(screen_width>>3);
          for(j=0; j<=ui.rx_ad_channels; j++)
            {
            m=screen_width*j;
            n=screen_width*(ui.rx_ad_channels+1);
            for(i=1; i<=old_end; i++)
              {
              lir_line(cal_graph[n+i-1], cal_graph[m+i-1],
                      cal_graph[n+i  ], cal_graph[m+i  ],0);
              if(kill_all_flag) goto calint_error_exit;
              }
            }
          textend=(INFOLINE+4)*text_height;
          ytop=textend+screen_height*0.3;
          ysiz=screen_height-ytop;
          n=-1;  
          for(j=ia; j<ic; j++)
            {
            i=(screen_width>>3)+cal_xgain*(j-first_pulse);
            if(i>=0 && i<screen_width)
              {
              n++;
              if( (n&0x7f)==0) lir_sched_yield();  
              old_end=n;
              cal_graph[screen_width*(ui.rx_ad_channels+1)+n]=i;
              cal_graph[n]=textend+screen_height*(0.4-0.008*log(fft1_float[j]));
              switch ( imode )
                {
// One channel 16 bit  
                case 0:
                k=(p0+j)&mask;
                t2=4*6553.6*cal_ygain*timf1_short_int[k]/ampmax;
                if(t2 <-0.45)t2=-0.45;
                if(t2 >0.45)t2=0.45;
                cal_graph[screen_width+n]=ytop+ysiz*(0.5-t2);
                m=1;
                break;

// One channel 32 bit  
                case 1:
                k=(p0+j)&mask;
                t2=0.4*cal_ygain*timf1_int[k]/ampmax;
                if(t2 <-0.45)t2=-0.45;
                if(t2 >0.45)t2=0.45;
                cal_graph[screen_width+n]=ytop+ysiz*(0.5-t2);
                m=1;
                break;

// Two channels 16 bit real data  
                case 2:
                k=(p0+2*j)&mask;
                for(m=0; m<2; m++)
                  {
                  t2=2*6553.6*cal_ygain*timf1_short_int[k+m]/ampmax;
                  if(t2 <-0.22)t2=-0.22;
                  if(t2 >0.22)t2=0.22;
                  if(m == 1)t2-=0.5;
                  cal_graph[screen_width*(m+1)+n]=ytop+ysiz*(0.25-t2);
                  }
                m=2;  
                break;

// Two channels 32 bit real data  
                case 3:
                k=(p0+2*j)&mask;
                for(m=0; m<2; m++)
                  {
                  t2=0.4*cal_ygain*timf1_int[k+m]/ampmax;
                  if(t2 <-0.22)t2=-0.22;
                  if(t2 >0.22)t2=0.22;
                  if(m == 1)t2-=0.5;
                  cal_graph[screen_width*(m+1)+n]=ytop+ysiz*(0.25-t2);
                  }
                m=2;  
                break;
  
// One channel 16 bit IQ data  
                case 4:
                k=(p0+2*j)&mask;
                for(m=0; m<2; m++)
                  {
                  t2=4*6553.6*cal_ygain*timf1_short_int[k+m]/ampmax;
                  if(t2 <-0.45)t2=-0.45;
                  if(t2 >0.45)t2=0.45;
                  cal_graph[screen_width*(m+1)+n]=ytop+ysiz*(0.5-t2);
                  }
                m=2;  
                break;

// One channel 32 bit IQ data  
                case 5:
                k=(p0+2*j)&mask;
                for(m=0; m<2; m++)
                  {
                  t2=0.4*cal_ygain*timf1_int[k+m]/ampmax;
                  if(t2 <-0.45)t2=-0.45;
                  if(t2 >0.45)t2=0.45;
                  cal_graph[screen_width*(m+1)+n]=ytop+ysiz*(0.5-t2);
                  }
                m=2;  
                break;

// Two channels 16 bit IQ data  
                case 6:
                k=(p0+4*j)&mask;
                t3=4*6553.6*cal_ygain/ampmax;
                for(m=0; m<4; m++)
                  {
                  t2=t3*timf1_short_int[k+m];
                  if(t2 <-0.22)t2=-0.22;
                  if(t2 >0.22)t2=0.22;
                  if(m > 1)t2-=0.5;
                  cal_graph[screen_width*(m+1)+n]=ytop+ysiz*(0.25-t2);
                  }
                m=4;
                break;

// Two channels 32 bit IQ data  
                case 7:
                k=(p0+4*j)&mask;
                for(m=0; m<4; m++)
                  {
                  t2=0.4F*cal_ygain*timf1_int[k+m]/ampmax;
                  if(t2 <-0.22)t2=-0.22F;
                  if(t2 >0.22)t2=0.22F;
                  if(m > 1)t2-=0.5F;
                  cal_graph[screen_width*(m+1)+n]=ytop+ysiz*(0.25F-t2);
                  }
                m=4;
                break;

                default:
                m=0;
                }
              if(n>0)
                {
                lir_line(cal_graph[screen_width*(ui.rx_ad_channels+1)+n-1],
                                        cal_graph[n-1],i,cal_graph[n],15);
                if(kill_all_flag) goto calint_error_exit;
                for(l=0; l<m; l++)
                  {
                  color=(unsigned char)(10+3*(l&1));
                  if( (ui.rx_input_mode&IQ_DATA) == 0)color=14;
                  lir_line(cal_graph[screen_width*(ui.rx_ad_channels                  +1)+n-1],
                                cal_graph[(l+1)*screen_width+n-1],
                                   i,cal_graph[(l+1)*screen_width+n],color);
                  if(kill_all_flag) goto calint_error_exit;
                  }
                }
              }
            sprintf(s,"Scale x=%f  y=%f      S/N=%f dB      PRF %.1f Hz    ",
                          cal_xgain,cal_ygain,10*log10(noise_floor),
                                        (float)ui.rx_ad_speed/cal_interval);
            lir_text(1,INFOLINE+4,s);
            }
          }
        }
      }
    }
skip:;    
  timf1p_px=(timf1p_px+rawbytes)&timf1_bytemask;
  lir_refresh_screen();
  }
if(thread_command_flag[THREAD_CAL_INTERVAL]==THRFLAG_IDLE)
  {
  thread_status_flag[THREAD_CAL_INTERVAL]=THRFLAG_IDLE;
  while(thread_command_flag[THREAD_CAL_INTERVAL]==THRFLAG_IDLE)
    {
    lir_await_event(EVENT_TIMF1);
    timf1p_px=(timf1p_px+rawbytes)&timf1_bytemask;
    }
  if(thread_command_flag[THREAD_CAL_INTERVAL]==THRFLAG_ACTIVE)goto restart;
  }  
calint_error_exit:;
thread_status_flag[THREAD_CAL_INTERVAL]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_CAL_INTERVAL] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

