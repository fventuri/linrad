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


#define LOG_ZERO -0.8
#define LINFAC 0.000025F

#include "osnum.h"
#include "globdef.h"
#include "uidef.h"
#include "fft1def.h"
#include "screendef.h"
#include "thrdef.h"
#include "powtdef.h"

#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif
#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif

#define OUTTEST_SIZE 16
#define LAST_LINE 9

float *ch1_y;
float *ch2_y; 
int *ch1_yold;
int *ch2_yold; 
int adtest_yzer;
int adtest_ymax;

void show_adtest(void)
{   
float ya, yb;
int x;
if(powtim_displaymode == 0)
  {
  if( (ui.rx_input_mode&IQ_DATA) != 0)
    {
    for(x=0; x<screen_width; x++)
      {
      lir_setpixel(x,ch1_yold[x],ADTEST_BACKGROUND_COLOR);
      lir_setpixel(x,ch2_yold[x],ADTEST_BACKGROUND_COLOR);
      ya=ch1_y[x];
      yb=ch2_y[x];
      ya*=(float)adtest_scale*LINFAC;  
      if(ya < -1)ya=-1;
      if(ya >  1)ya= 1; 
      yb*=(float)adtest_scale*LINFAC;  
      if(yb < -1)yb=-1;
      if(yb >  1)yb= 1; 
      ch1_yold[x]=adtest_yzer-(int)(ya*(float)adtest_ymax);
      ch2_yold[x]=adtest_yzer-(int)(yb*(float)adtest_ymax);
      if(ch1_yold[x] == ch2_yold[x]) 
        {
        lir_setpixel(x,ch1_yold[x],15);
        }
      else
        {
        lir_setpixel(x,ch1_yold[x],14);
        lir_setpixel(x,ch2_yold[x],12);
        }
      }
    }  
  else
    {
    for(x=0; x<screen_width; x++)
      {
      lir_setpixel(x,ch1_yold[x],ADTEST_BACKGROUND_COLOR);
      ya=ch1_y[x];
      ya*=(float)adtest_scale*LINFAC;  
      if(ya < -1)ya=-1;
      if(ya >  1)ya= 1; 
      ch1_yold[x]=adtest_yzer-(int)(ya*(float)adtest_ymax);
      lir_setpixel(x,ch1_yold[x],15);
      }
    }
  }
else
  {
  if( (ui.rx_input_mode&IQ_DATA) != 0)
    {
    for(x=1; x<screen_width; x++)
      {
      lir_line(x-1,ch1_yold[x-1],x,ch1_yold[x],ADTEST_BACKGROUND_COLOR);
      if(kill_all_flag) return;
      lir_line(x-1,ch2_yold[x-1],x,ch2_yold[x],ADTEST_BACKGROUND_COLOR);
      if(kill_all_flag) return;
      } 
    for(x=1; x<screen_width; x++)
      {
      ya=ch1_y[x];
      yb=ch2_y[x];
      ya*=(float)adtest_scale*LINFAC;  
      if(ya < -1)ya=-1;
      if(ya >  1)ya= 1; 
      yb*=(float)(adtest_scale*LINFAC);  
      if(yb < -1)yb=-1;
      if(yb >  1)yb= 1; 
      ch1_yold[x]=adtest_yzer-(int)(ya*(float)adtest_ymax);
      ch2_yold[x]=adtest_yzer-(int)(yb*(float)adtest_ymax);
      lir_line(x-1,ch1_yold[x-1],x,ch1_yold[x],14);
      if(kill_all_flag) return;
      lir_line(x-1,ch2_yold[x-1],x,ch2_yold[x],12);
      if(kill_all_flag) return;
      }
    }  
  else
    {
    for(x=0; x<screen_width; x++)
      {
      lir_setpixel(x,ch1_yold[x],ADTEST_BACKGROUND_COLOR);
      ya=ch1_y[x];
      ya*=(float)(adtest_scale*LINFAC);  
      if(ya < -1)ya=-1;
      if(ya >  1)ya= 1; 
      ch1_yold[x]=adtest_yzer-(int)(ya*(float)adtest_ymax);
      lir_setpixel(x,ch1_yold[x],15);
      }
    }
  }
lir_refresh_screen();
}



void rx_adtest(void)
{
int local_workload_reset;
int local_adtest_new;
int i, j, x, p0, p_increment, points, px, p_max;
int maxmin_counter;
int ad_maxval1,ad_minval1,ad_maxval2,ad_minval2;
float ya,yb;
int pixmode;
char s[80];
clear_screen();
ch1_y=fftw_tmp;
ch2_y=&fftw_tmp[screen_width];
ch1_yold=(int*)&fftw_tmp[2*screen_width];
ch2_yold=(int*)&fftw_tmp[3*screen_width];;
local_workload_reset=workload_reset_flag;
local_adtest_new=adtest_new;
ad_maxval1=0;
ad_minval1=0;
ad_maxval2=0;
ad_minval2=0;
maxmin_counter=0;
settextcolor(14);
lir_text(10,0,"TEST OF A/D INPUT for RX");
lir_text(38,0,"Make sure frequency and gain are set.");
settextcolor(7);
lir_text(38,0,"Make sure frequency and gain are set.");
lir_text(0,1,
"Press 'X' twice, on a normal receive mode before using this screen.");
adtest_scale=1;
powtim_pause_flag=0;
adtest_new=0;
adtest_channel=0;
powtim_displaymode=0;
// Use lines 1 to LAST_LINE+2 for text messages.
i=(LAST_LINE+2)*text_height;
i+=text_height/2;
j=screen_height-i;
if(j < 4*text_height)
  {
  lirerr(1046);
  goto adtest_exit;
  }
j/=2;
adtest_yzer=screen_height-j-1;
adtest_ymax=j;
for(i=0; i<screen_width; i++)
  {
  ch1_yold[i]=adtest_yzer;
  ch2_yold[i]=adtest_yzer;
  }
restart:;
thread_status_flag[THREAD_RX_ADTEST]=THRFLAG_ACTIVE;
yb=0;
lir_fillbox(0,adtest_yzer-adtest_ymax, screen_width-1,screen_height
                          -adtest_yzer+adtest_ymax-1, ADTEST_BACKGROUND_COLOR);
if(ui.rx_rf_channels == 2)
  {
  sprintf(s,"Channel = %d   'C' to toggle channel.",adtest_channel);
  lir_text(3,3,s);
  }
if(powtim_pause_flag == 0)
  {
  lir_text(3,4,"'P' to pause.");
  }
else
  {
  lir_text(3,4,"'R' to resume.");
  show_adtest();
  if(kill_all_flag) goto adtest_exit;
  }
lir_text(3,5,"'W' to toggle line/pixels.");
lir_text(3,6,"+/- to change scale.");
x=0;
timf1p_px=timf1p_pa;
pixmode=adtest_channel*2;
if( ui.rx_rf_channels == 2)pixmode++;
pixmode*=2;
if( (ui.rx_input_mode&IQ_DATA) != 0)pixmode++;
pixmode*=2;
if( (ui.rx_input_mode&DWORD_INPUT) == 0)pixmode++;
//pixmode bit0  0=>32bit        1=>16bit.
//pixmode bit1  0=>normal       1=>IQ data
//pixmode bit2  0=>one          1=>two rx channels
//pixmode bit3  channel 0 or 1 (if two channels)
settextcolor(15);
sprintf(s,"pixmode=%d      scale=%3.3fdB (%f) ",
                             pixmode,20*log10(adtest_scale),adtest_scale);
lir_text(1,LAST_LINE-1,s);
while(thread_command_flag[THREAD_RX_ADTEST] == THRFLAG_ACTIVE)
  {
  if(local_workload_reset!=workload_reset_flag)
    {
    ad_maxval1=0;
    ad_minval1=0;
    ad_maxval2=0;
    ad_minval2=0;
    maxmin_counter=0;
    local_workload_reset=workload_reset_flag;
    }
  lir_await_event(EVENT_TIMF1);
  if(powtim_pause_flag != 0)timf1p_px = timf1p_pa;
  if(timf1p_px != timf1p_pa)
    {
    points=((timf1p_pa-timf1p_px+timf1_bytes)&timf1_bytemask)/
                                                       snd[RXAD].framesize; 
    if(points > screen_width)
      {
      points=screen_width;
      timf1p_px=(timf1p_pa-points*snd[RXAD].framesize+timf1_bytes)&
                                                          timf1_bytemask;
      }                                                          
    p0=(timf1p_px-snd[RXAD].block_bytes+timf1_bytes)&timf1_bytemask;
    timf1p_px=timf1p_pa;
    p_increment=snd[RXAD].framesize;
    p_max=timf1_bytes;
    if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
      {
      p_increment/=(int)sizeof(short int);
      p_max/=(int)sizeof(short int);
      p0/=(int)sizeof(short int);
      }
    else
      {
      p_increment/=(int)sizeof(int);
      p_max/=(int)sizeof(int);
      p0/=(int)sizeof(int);
      }
    p_max--;
    maxmin_counter+=points;  
    if(maxmin_counter > ui.rx_ad_speed/4)
      {
      if(  (ui.rx_input_mode&DWORD_INPUT) != 0)
        {
        ad_maxval1/=2048;
        ad_minval1/=2048;
        ad_maxval2/=2048;
        ad_minval2/=2048;
        }
      sprintf(s,"AD max = %06d   ",ad_maxval1);
      lir_text(1,LAST_LINE,s); 
      sprintf(s,"AD min = %06d   ",ad_minval1);
      lir_text(41,LAST_LINE,s);
      if(  (ui.rx_input_mode&IQ_DATA) != 0)
        {
        sprintf(s,"%06d   ",ad_maxval2);
        lir_text(22,LAST_LINE,s); 
        sprintf(s,"%06d   ",ad_minval2);
        lir_text(62,LAST_LINE,s);
        }
      ad_maxval1=ad_minval1=ad_maxval2=ad_minval2=0;
      maxmin_counter=0;
      }
    sprintf(s,"N=%d inc=%d ",points,p_increment);
    lir_text(60,LAST_LINE-1,s);
    i=0;
    for( px=0; px<points; px++)
      {
      switch (pixmode)
        {
        case 0:   // One channel 32 bit. Oscilloscope raw data 
        case 4:   // Two channels 32 bit. Oscilloscope raw data ch0
        if(ad_maxval1<timf1_int[p0])ad_maxval1=timf1_int[p0];
        if(ad_minval1>timf1_int[p0])ad_minval1=timf1_int[p0];
        ya=(float)(timf1_int[p0])/65536;

        break;

        case 1:   // One channel 16 bit. Oscilloscope raw data 
        case 5:   // Two channels 16 bit. Oscilloscope raw data ch0
        if(ad_maxval1<timf1_short_int[p0])ad_maxval1=timf1_short_int[p0];
        if(ad_minval1>timf1_short_int[p0])ad_minval1=timf1_short_int[p0];
        ya=timf1_short_int[p0];
        break;
  
        case 2:   // One channel IQ 32 bit. Oscilloscope raw data.   
        case 6:   // Two channels IQ 32 bit. Oscilloscope raw data ch0.
        if(ad_maxval1<timf1_int[p0  ])ad_maxval1=timf1_int[p0  ];
        if(ad_minval1>timf1_int[p0  ])ad_minval1=timf1_int[p0  ];
        if(ad_maxval2<timf1_int[p0+1])ad_maxval2=timf1_int[p0+1];
        if(ad_minval2>timf1_int[p0+1])ad_minval2=timf1_int[p0+1];
        ya=(float)(timf1_int[p0  ])/65536;
        yb=(float)(timf1_int[p0+1])/65536;
        break;

        case 3:   // One channel IQ 16 bit. Oscilloscope raw data.   
        case 7:   // Two channels IQ 16 bit. Oscilloscope raw data ch0.
        if(ad_maxval1<timf1_short_int[p0  ])ad_maxval1=timf1_short_int[p0  ];
        if(ad_minval1>timf1_short_int[p0  ])ad_minval1=timf1_short_int[p0  ];
        if(ad_maxval2<timf1_short_int[p0+1])ad_maxval2=timf1_short_int[p0+1];
        if(ad_minval2>timf1_short_int[p0+1])ad_minval2=timf1_short_int[p0+1];
        ya=timf1_short_int[p0  ];
        yb=timf1_short_int[p0+1];
        break;

        case 12:  // Two channels 32 bit. Oscilloscope raw data ch1
        if(ad_maxval1<timf1_int[p0+1])ad_maxval1=timf1_int[p0+1];
        if(ad_minval1>timf1_int[p0+1])ad_minval1=timf1_int[p0+1];
        ya=(float)(timf1_int[p0+1])/65536;
        break;

        case 13:  // Two channels 16 bit. Oscilloscope raw data ch1
        if(ad_maxval1<timf1_short_int[p0+1])ad_maxval1=timf1_short_int[p0+1];
        if(ad_minval1>timf1_short_int[p0+1])ad_minval1=timf1_short_int[p0+1];
        ya=timf1_short_int[p0+1];
        break;

        case 14:   // Two channels IQ 32 bit. Oscilloscope raw data ch1
        if(ad_maxval1<timf1_int[p0+2])ad_maxval1=timf1_int[p0+2];
        if(ad_minval1>timf1_int[p0+2])ad_minval1=timf1_int[p0+2];
        if(ad_maxval2<timf1_int[p0+3])ad_maxval2=timf1_int[p0+3];
        if(ad_minval2>timf1_int[p0+3])ad_minval2=timf1_int[p0+3];
        ya=(float)(timf1_int[p0+2])/65536;
        yb=(float)(timf1_int[p0+3])/65536;
        break;

        case 15:   // Two channels IQ 16 bit. Oscilloscope raw data ch0.
        if(ad_maxval1<timf1_short_int[p0+2])ad_maxval1=timf1_short_int[p0+2];
        if(ad_minval1>timf1_short_int[p0+2])ad_minval1=timf1_short_int[p0+2];
        if(ad_maxval2<timf1_short_int[p0+3])ad_maxval2=timf1_short_int[p0+3];
        if(ad_minval2>timf1_short_int[p0+3])ad_minval2=timf1_short_int[p0+3];
        ya=timf1_short_int[p0+2];
        yb=timf1_short_int[p0+3];
        break;

        default:
        ya=0;
        yb=0;
        break;
        }
      ch1_y[x]=ya;    
      ch2_y[x]=yb;    
      p0=(p0+p_increment)&p_max;
      x++;

/*
Trigger. Something is wrong in that sweeps sometimes
do not extend over the entire screen.

if(x == 2)
  {
  if((float)ch1_y[0]*(float)ch1_y[1] > 0)x=0;
  if(ch1_y[0] > 0)x=0;
  }
*/  
  
      if(x>=screen_width)
        {
        x=0;
        if(i==0)
          {
          show_adtest();   
          if(kill_all_flag) goto adtest_exit;
          }
        i++;
        }
      }
    }  
lir_sleep(100000);
  if(local_adtest_new != adtest_new)
    {
    local_adtest_new=adtest_new;
    goto restart;
    }
  }
if(thread_command_flag[THREAD_RX_ADTEST]==THRFLAG_IDLE)
  {
  thread_status_flag[THREAD_RX_ADTEST]=THRFLAG_IDLE;
  while(thread_command_flag[THREAD_RX_ADTEST] == THRFLAG_IDLE)
    {
    lir_await_event(EVENT_TIMF1);
    timf1p_px=(timf1p_px+timf1_blockbytes)&timf1_bytemask;
    }
  if(thread_command_flag[THREAD_RX_ADTEST] == THRFLAG_ACTIVE)goto restart;
  }  
adtest_exit:;
thread_status_flag[THREAD_RX_ADTEST]=THRFLAG_RETURNED;
}
