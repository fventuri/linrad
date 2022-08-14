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
#include "sigdef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "screendef.h"
#include "thrdef.h"
#include "seldef.h"
#include "powtdef.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

void make_rf_envelopes(void)
{
int i, pa, pc;
float t1,t2,t3;
// Make a backwards transform in floating point format.
for(i=0; i<fft1_first_point; i++)
  {
  fftw_tmp[2*i  ]=0;
  fftw_tmp[2*i+1]=0;
  }
for(i=fft1_first_point; i<=fft1_last_point; i++)
  {
  fftw_tmp[2*i  ]=fft1_float[fft1_px+2*i  ];
  fftw_tmp[2*i+1]=fft1_float[fft1_px+2*i+1];
  }
for(i=fft1_last_point+1; i<fft1_size; i++)
  {
  fftw_tmp[2*i  ]=0;
  fftw_tmp[2*i+1]=0;
  }    
if( (ui.rx_input_mode&IQ_DATA) != 0)fft_iqshift(fft1_size, fftw_tmp);
fftback(fft1_size, fft1_n, fftw_tmp, fft2_tab, fft2_permute, FALSE);
pa=timf2_pa;
pc=(timf2_pa+timf2pow_mask)&timf2pow_mask;
for(i=0; i<fft1_size/2; i++)
  {  
  t1=timf2_float[2*pa  ]+fftw_tmp[2*i  ];
  t2=timf2_float[2*pa+1]+fftw_tmp[2*i+1];
  timf2_float[2*pc  ]=powtim_powfac*sqrt(t1*t1+t2*t2);
  if(timf2_float[2*pc  ] !=0)
    {
    t3=atan2(t2,t1);
    timf2_float[2*pa+1]=t3;
    t3-=timf2_float[2*pc+1];
    if(t3 > PI_L)t3-=2*PI_L;
    if(t3 <-PI_L)t3+=2*PI_L;
    timf2_float[2*pc+1]=t3;
    }
  else
    {
    timf2_float[2*pc+1]=0;
    timf2_float[2*pa+1]=0;
    }  
  pc=pa;
  pa=(pa+1)&timf2pow_mask;
  }
pa=(timf2_pa+fft1_size/2)&timf2pow_mask;
for(i=0; i<fft1_size/2; i++)
  {
  timf2_float[2*pa  ]=fftw_tmp[fft1_size+2*i  ];
  timf2_float[2*pa+1]=fftw_tmp[fft1_size+2*i+1];
  pa=(pa+1)&timf2pow_mask;
  }
fft1_px=(fft1_px+fft1_block)&fft1_mask;  
timf2_pa=(timf2_pa+fft1_size/2)&timf2pow_mask;
}

int powtim_trigger(void)
{
int ia, p0, pa, pb;
float t1,t2;
pa=(timf2_pn1-screen_width*powtim_xstep+timf2pow_mask)&timf2pow_mask;
pb=(timf2_pa-screen_width*powtim_xstep+timf2pow_mask)&timf2pow_mask;
p0=pa;
pa=(pa-screen_width*powtim_xstep+timf2pow_mask)&timf2pow_mask;
t1=0;
ia=p0;
while(p0 != pb)
  {
  if(t1 < timf2_float[2*p0])
    {
    t1=timf2_float[2*p0];
    ia=p0;
    }
  p0=(p0+1)&timf2pow_mask;
  }
switch (powtim_trigmode)
  {
  case 0:
// Low rise. Step backwards for 1% power (10% amplitude)
  t2=t1;
  t1*=0.1;
  while(ia != pa && t2 >= t1)
    {
    t2=timf2_float[2*ia];
    ia=(ia+timf2pow_mask)&timf2pow_mask;
    }
  break;    

  case 1:
// Low fall. Step forwards for 1% power (10% amplitude)
  t2=t1;
  t1*=0.1;
  while(ia != pb && t2 >= t1)
    {
    t2=timf2_float[2*ia];
    ia=(ia+1)&timf2pow_mask;
    }
  break;    

  case 2:
// High rise. Step backwards for 90% power (95% amplitude)
  t2=t1;
  t1*=0.95;
  while(ia != pa && t2 >= t1)
    {
    t2=timf2_float[2*ia];
    ia=(ia+timf2pow_mask)&timf2pow_mask;
    }
  break;    

  case 3:
// High fall. Step forwards for 90% power (95% amplitude)
  t2=t1;
  t1*=0.95;
  while(ia != pb && t2 >= t1)
    {
    t2=timf2_float[2*ia];
    ia=(ia+1)&timf2pow_mask;
    }
  break;    
  }
timf2_pn2=ia;
timf2_pn1=timf2_pa;
return 1;
}

void powtim_screen(void)
{
char s[80];
unsigned int uj;
int i,j,k, p0;
float t1,f0;
if(timf2_pn2 < 0)return;
// Clear the old traces
if( (powtim_displaymode&4) == 0)
  {
  for(i=0; i<screen_width; i++)
    {
    lir_setpixel(i,timf2_pwr_int[2*i  ],0);
    lir_setpixel(i,timf2_pwr_int[2*i+1],0);
    }
  }  
else
  {
  for(i=1; i<screen_width; i++)
    {
    for(j=0; j<2; j++)
      {
      lir_line(i-1,timf2_pwr_int[2*(i-1)+j],i,timf2_pwr_int[2*i+j],0);
      if(kill_all_flag) return;
      }
    }
  powtim_displaymode&=-1-4;  
  }
for(uj=powtim_y0-powtim_scalepix; uj>powtim_y1; uj-=powtim_scalepix)
  {
  lir_hline(0,uj,screen_width-1,3);
  if(kill_all_flag) return;
  }
lir_hline(0,powtim_y0,screen_width-1,7);
if(kill_all_flag) return;
for(i=0; i<screen_width; i+=powtim_scalepix)
  {
  for(uj=powtim_y0; uj<powtim_y0+text_height/2; uj++)
    {
    lir_setpixel(i,uj,15);
    }
  }  
// Display data using timf2_pn2 as pointer.
f0=timf2_float[2*timf2_pn2+1];
sprintf(s,"%f kHz   ",-f0*(0.0005*timf1_sampling_speed/(PI_L)));
settextcolor(12);
lir_pixwrite(5*text_width, powtim_y0+4*text_height, s);
settextcolor(7);
p0=(timf2_pn2-powtim_xstep*screen_width/4+timf2pow_mask)&timf2pow_mask;
sprintf(s,"%d          ",p0);
lir_pixwrite(5*text_width, powtim_y0+5*text_height, s);
if( (powtim_displaymode & 2) ==0)
  {
  for(i=0; i<screen_width; i++)
    {
    t1=powtim_gain*timf2_float[2*p0];
    if( (powtim_displaymode&1) !=0)
      {
      if(t1<0.0000000001)t1=0.0000000001;
      t1=2*powtim_scalepix*(1+log10(t1));
      }
    t1=powtim_y0-t1;
    if(t1<powtim_y1)t1=powtim_y1;
    if(t1 > powtim_y0)t1=powtim_y0;
    timf2_pwr_int[2*i]=t1;
    lir_setpixel(i,timf2_pwr_int[2*i],15);
    t1=powtim_fgain*(timf2_float[2*p0+1]-f0);
    t1=powtim_y2-t1;
    if(t1 < powtim_y1)t1=powtim_y1;
    if(t1 > powtim_y0)t1=powtim_y0;
    timf2_pwr_int[2*i+1]=t1;
    lir_setpixel(i,timf2_pwr_int[2*i+1],12);
    p0=(p0+powtim_xstep)&timf2pow_mask;
    k=(timf2_pa-p0+timf2pow_mask+1)&timf2pow_mask;
    if(k >=timf2pow_mask-powtim_xstep)goto showx; 
    }
  }
else
  {
  powtim_displaymode|=4;
  for(i=0; i<screen_width; i++)
    {
    t1=powtim_gain*timf2_float[2*p0];
    if( (powtim_displaymode&1) !=0)
      {
      if(t1<0.0000000001)t1=0.0000000001;
      t1=2*powtim_scalepix*(1+log10(t1));
      }
    t1=powtim_y0-t1;
    if(t1<powtim_y1)t1=powtim_y1;
    if(t1 > powtim_y0)t1=powtim_y0;
    timf2_pwr_int[2*i]=t1;
    if(i!=0)
      {
      lir_line(i-1,timf2_pwr_int[2*(i-1)],i,timf2_pwr_int[2*i],15);
      if(kill_all_flag) return;
      }
    t1=powtim_fgain*(timf2_float[2*p0+1]-f0);
    t1=powtim_y2-t1;
    if(t1 < powtim_y1)t1=powtim_y1;
    if(t1 > powtim_y0)t1=powtim_y0;
    timf2_pwr_int[2*i+1]=t1;
    if(i!=0)
      {
      lir_line(i-1,timf2_pwr_int[2*(i-1)+1],i,timf2_pwr_int[2*i+1],12);
      if(kill_all_flag) return;
      }
    p0=(p0+powtim_xstep)&timf2pow_mask;
    k=(timf2_pa-p0+timf2pow_mask+1)&timf2pow_mask;
    if(k >=timf2pow_mask-powtim_xstep)goto showx; 
    }
  }
showx:;  
lir_refresh_screen();
}

void powtim_parmwrite(void)
{
char *tmtxt[POWTIM_TRIGMODES]={"Low rise","Low fall","High rise","High fall"};
char *dmtxt[2]={"Lin","Log"};
char s[80];

if(powtim_pause_flag == 0)
  {
  lir_text(10,0,"Press P for PAUSE to view data in memory.");
  }
else
  {
  clear_lines(0,0);
  settextcolor(14);
  lir_text(10,0,"Press R to restart sampling.");
  settextcolor(7);
  }
sprintf(s,"Trig mode: %s  (M to change)  ",tmtxt[powtim_trigmode]);
lir_text(20,2,s);
sprintf(s,"Display mode: %s  (L to change)  ",dmtxt[powtim_displaymode&1]);
lir_text(20,3,s);
lir_text(20,4,"W to toggle between line or pixels");

sprintf(s,"+/-: Amplitude scale %f     ",powtim_gain);
lir_pixwrite(5*text_width, powtim_y0+text_height, s);
sprintf(s,"I/D: Time scale %f ms ",
                     powtim_scalepix*1000*powtim_xstep/timf1_sampling_speed);
lir_pixwrite(5*text_width, powtim_y0+2*text_height, s);
sprintf(s,"%c/%c: Frequency scale %f kHz ",24,25,
              powtim_scalepix*0.0005*timf1_sampling_speed/(powtim_fgain*PI_L));
lir_pixwrite(5*text_width, powtim_y0+3*text_height, s);
}


void powtim(void)
{
int i;
// This routine stops normal processing and shows RF envelope vs time
// and frequency vs time as an oscilloscope. 
ampinfo_flag=1;
timf2_pa=0;
timf2_pn1=0;
timf2_pn2=-1;
// We use timf2_pwr to store screen data.
powtim_y0=0.8*screen_height;
powtim_y1=0.1*screen_height;
powtim_y2=0.5*screen_height;
powtim_scalepix=(powtim_y0-powtim_y1)/10;
powtim_powfac=0.2*screen_height/(sqrt((float)(fft1_size))*0x10000);
powtim_gain=1;
powtim_fgain=500;
powtim_xstep=1;
powtim_trigmode=0;
powtim_displaymode=0;
// Reconfigure the fft2 tables so they will work for backwards
// fft1.
init_fft(0,fft1_n, fft1_size, fft2_tab, fft2_permute);
for(i=0; i<screen_width*2; i++)timf2_pwr_int[i]=powtim_y0;
for(i=0; i<screen_width; i++)lir_setpixel(i,powtim_y0,15);
powtim_pause_flag=0;
restart:;
thread_status_flag[THREAD_POWTIM] = THRFLAG_ACTIVE;
fft1_px=fft1_pb;
timf2_px=timf2_pa;
powtim_parmwrite();
while(thread_command_flag[THREAD_POWTIM] == THRFLAG_ACTIVE)
  {
  lir_await_event(EVENT_FFT1_READY);
  if(powtim_pause_flag != 0)
    {
    fft1_px=fft1_pb;
    timf2_px=timf2_pa;
    }
  else
    { 
    while( ((fft1_pb-fft1_px+fft1_block+fft1_mask)&fft1_mask) > 2*fft1_block)
      {
      make_rf_envelopes();
      }
// Do not run more often than 25 times per second.
    if( ((timf2_pa-timf2_pn1+timf2pow_mask+1)&timf2pow_mask) >  
                                                        ui.rx_ad_speed/25)
      {
      i=powtim_trigger();
      if(i != 0)
        {
        powtim_screen();
        }
      }
    }  
  if(kill_all_flag) goto powtim_exit;
  }
if(thread_command_flag[THREAD_POWTIM]==THRFLAG_IDLE)
  {
  thread_status_flag[THREAD_POWTIM]=THRFLAG_IDLE;
  while(thread_command_flag[THREAD_POWTIM] == THRFLAG_IDLE)
    {
    lir_await_event(EVENT_FFT1_READY);
    fft1_px=fft1_pb;
    timf2_px=timf2_pa;
    }
  if(thread_command_flag[THREAD_POWTIM] == THRFLAG_ACTIVE)goto restart;
  }  
powtim_exit:;
thread_status_flag[THREAD_POWTIM]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_POWTIM] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }

}

