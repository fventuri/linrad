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


#define PORTWAIT 1000

#include "osnum.h"
#include "globdef.h"
#include "uidef.h"
#include "fft1def.h"
#include "fft3def.h"
#include "sigdef.h"
#include "screendef.h"
#include "seldef.h"
#include "hwaredef.h"
#include "caldef.h"
#include "thrdef.h"
#include "sdrdef.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

int tune_bytes;

void parport_sendwait(char obyte)
{    
lir_outb(obyte,wse_parport_control);
lir_sleep(PORTWAIT);
}

void store_xtal_data(int unit, int bitdat)
{
char chk;
lir_outb(unit,wse.parport);
lir_sleep(PORTWAIT);
chk=lir_inb(wse_parport_status)&wse_parport_ack;
chk^=wse_parport_ack_sign;
if(bitdat ==0 )
  {
  if( chk == 0) hware_error_flag++;
  }
else  
  {
  if( chk != 0) hware_error_flag++;
  }
}


void send_bit(int bitdat)
{
char odat;
// Set clock low and make data the first bit of current output.
if( bitdat == 0)
  {
  odat=WSE_PARPORT_DATA+WSE_PARPORT_CLOCK;
  }
else
  {
  odat=WSE_PARPORT_CLOCK;
  }
parport_sendwait(odat);    
// Set clock high to clock in new data bit.
odat^=WSE_PARPORT_CLOCK;
lir_outb(odat,wse_parport_control);
lir_sleep(PORTWAIT);
}



void tune(void)
{
char s[80],ss[128];
int j, k, m, n, ib;
int i, bits, current_bit, unit, ptr;
int tune_size, ptr_inc, ptr_ovl;
int mm, nn;
int yshow[64]; 
float pwr[256][2];
float t1,t2;
// Make sure the compiler is happy...
tune_yzer=0;
tune_size=0;
ptr_inc=0;
ptr_ovl=0;
bits=0;
current_bit=0;
unit=0;
ptr=0;
nn=0;
// Set some memory to zero.
for(j=0; j<2; j++)
  {
  for(i=0; i<256; i++)
    {
    pwr[i][j]=0;
    }
  }  
for(i=0; i<64; i++)yshow[i]=screen_height/2;
clear_screen();
lir_text(5,5,"Select unit");
lir_text(8,7,"A=RX10700");
lir_text(8,8,"B=RX70");
lir_text(8,9,"C=RX144");
lir_text(8,10,"D=RXHFA");
lir_text(8,11,"X=Exit (calibrate)");
lir_refresh_screen();
tune_select:;
lir_sleep(20000);
if(kill_all_flag)goto tune_exit;
if(thread_command_flag[THREAD_TUNE]==THRFLAG_KILL)goto tune_return;
switch (lir_inkey)
  {
  case 'A':
  sprintf(s,"10700");
  unit=RX10700;
  bits=4;
  tune_yzer=3700;
  tune_size=8;
  ptr_ovl=((25000*tune_size+48000)/96000);
  ptr_inc=tune_size+3*ptr_ovl;
  break;

  case 'B':
  sprintf(s,"70");
  unit=RX70;
  bits=5;
  tune_yzer=4200;
  tune_size=4;
  ptr_ovl=((25000*tune_size+48000)/96000);
  ptr_inc=tune_size;
// Set the RX10700 to 10.700 
  hware_error_flag=0;
  lir_outb(0,wse.parport);
  send_bit(0);
  store_xtal_data(RX10700,0);
  lir_outb(0,wse.parport);
  send_bit(0);
  store_xtal_data(RX10700,0);
  lir_outb(0,wse.parport);
  send_bit(1);  
  store_xtal_data(RX10700,1);
  lir_outb(0,wse.parport);
  send_bit(0);
  store_xtal_data(RX10700,0);
  lir_outb(0,wse.parport);
  if(hware_error_flag != 0)
    {
errx:;
    settextcolor(12);
    sprintf(ss,"No response from RX%s",s);
    lir_text(10,12,ss);
    settextcolor(7);
    lir_text(15,15,"Press X");
    goto tune_exit;
    }
  break;

  case 'C':
  sprintf(s,"144");
  unit=RX144;
  bits=4;
  tune_yzer=5400;
  tune_size=4;
  ptr_ovl=((25000*tune_size+48000)/96000);
  ptr_inc=tune_size;
// Set the RX10700 to 10.700 
  hware_error_flag=0;
  lir_outb(0,wse.parport);
  send_bit(0);
  store_xtal_data(RX10700,0);
  lir_outb(0,wse.parport);
  send_bit(0);
  store_xtal_data(RX10700,0);
  lir_outb(0,wse.parport);
  send_bit(1);  
  store_xtal_data(RX10700,1);
  lir_outb(0,wse.parport);
  send_bit(0);
  store_xtal_data(RX10700,0);
  lir_outb(0,wse.parport);
  if(hware_error_flag != 0)
    {
    sprintf(s,"10700");
    goto errx;  
    }
// Set the RX70 to 70.2 
  lir_outb(0,wse.parport);
  send_bit(0);
  store_xtal_data(RX70,0);
  lir_outb(0,wse.parport);
  send_bit(0);
  store_xtal_data(RX70,0);
  lir_outb(0,wse.parport);
  send_bit(1);  
  store_xtal_data(RX70,1);
  lir_outb(0,wse.parport);
  send_bit(0);
  store_xtal_data(RX70,0);
  lir_outb(0,wse.parport);
  send_bit(0);
  store_xtal_data(RX70,0);
  lir_outb(0,wse.parport);
  if(hware_error_flag != 0)
    {
    sprintf(s,"70");
    goto errx;  
    }
  break;
    
  case 'D':
  sprintf(s,"HFA ");
  unit=RXHFA;
  bits=4;
  tune_yzer=3700;
  tune_size=8;
  ptr_ovl=((25000*tune_size+48000)/96000);
  ptr_inc=tune_size+3*ptr_ovl;
  break;
    
  default:
  goto tune_select; 
  }
// Write the number of the current unit to the data port
bal_updflag=0;
genparm[AFC_ENABLE]=0;
genparm[SECOND_FFT_ENABLE]=0;
if(lir_status != LIR_OK)return;
clear_screen();
// Make sure the tables fit the fft routine we use.
for(i=0; i<fft1_size; i++)
  {
  t1=1/fft1_desired[i];
  fft1_desired[i]=t1*t1/1000;
  }
fft1_direction=-1;
lir_text(1,0,"Connect pulse generator to RX");
lir_text(30,0,s);
nn=fft1_size/tune_size;
// Move zeroes to the hardware latch for all crystals but
// the first one.
for(i=1; i<bits; i++)send_bit(0);
current_bit=bits-2;
ptr=0;
// Move 1 into the first latch to enable the first crystal.
// *********************************************************

lir_refresh_screen();
restart:;
thread_status_flag[THREAD_TUNE]=THRFLAG_ACTIVE;
while(thread_command_flag[THREAD_TUNE] == THRFLAG_ACTIVE)
  {
  lir_outb(0,wse.parport);
  current_bit++;
  ptr+=ptr_inc;
  if(current_bit == bits) current_bit=0;
  if(current_bit == bits-1)
    {
    ptr=0;
    switch (unit)
      {
      case RX10700:
      for(i=0; i<ptr_inc; i++)
        {
        for(j=0;j<2; j++)
          {
          for(m=1; m<4; m++)
            {
            pwr[i+m*ptr_ovl][j]+=pwr[i+m*ptr_inc][j];
            }
          }
        }    
      i=0;
      j=ptr_inc-1;
      m=1;
nrm:;
      for(k=0; k<ptr_ovl; k++)
        {
        pwr[i][0]/=m;
        pwr[j][0]/=m;
        pwr[i][1]/=m;
        pwr[j][1]/=m;
        i++;
        j--;
        if(i>j)goto nrm_x;
        }
      m++;
      goto nrm;
nrm_x:;    
      for(i=0; i<ptr_inc; i++)
        {
        lir_fillbox(12*i+30,yshow[i],8,8,0);
        k=1000*log10(pwr[i][0])-tune_yzer;
        k=screen_height-k;
        if(k<30)k=30;
        if(k>screen_height-9)k=screen_height-9;
        yshow[i]=k; 
        lir_fillbox(12*i+30,yshow[i],8,8,10);
        lir_fillbox(12*i+60+12*ptr_inc,yshow[i+ptr_inc],8,8,0);
        k=1000*log10(pwr[i][1])-tune_yzer;
        k=screen_height-k;
        if(k<30)k=30;
        if(k>screen_height-9)k=screen_height-9;
        yshow[i+ptr_inc]=k; 
        lir_fillbox(12*i+60+12*ptr_inc,yshow[i+ptr_inc],8,8,13);
        }
      lir_hline(0,screen_height/2,24*ptr_inc+120,7); 
      if(kill_all_flag) goto tune_exit;
      lir_hline(0,screen_height/2-100,24*ptr_inc+120,7); 
      if(kill_all_flag) goto tune_exit;
      lir_hline(0,screen_height/2+100,24*ptr_inc+120,7); 
      if(kill_all_flag) goto tune_exit;
      for(j=0; j<2; j++)
        {
        for(i=0; i<4*ptr_inc; i++)
          {
          pwr[i][j]=0;
          }
        }  
      i=1;
      break;
    
      case RX70:
      for(i=0; i<5*ptr_inc; i++)
        {
        lir_fillbox(12*i+30,yshow[i],8,8,0);
        k=1000*log10(pwr[i][0])-tune_yzer;
        k=screen_height-k;
        if(k<30)k=30;
        if(k>screen_height-9)k=screen_height-9;
        yshow[i]=k; 
        lir_fillbox(12*i+30,yshow[i],8,8,10);
        lir_fillbox(12*i+60+5*12*ptr_inc,yshow[i+5*ptr_inc],8,8,0);
        k=1000*log10(pwr[i][1])-tune_yzer;
        k=screen_height-k;
        if(k<30)k=30;
        if(k>screen_height-9)k=screen_height-9;
        yshow[i+5*ptr_inc]=k; 
        lir_fillbox(12*i+60+5*12*ptr_inc,yshow[i+5*ptr_inc],8,8,13);
        }
      lir_hline(0,screen_height/2,5*24*ptr_inc+120,7); 
      if(kill_all_flag) goto tune_exit;
      lir_hline(0,screen_height/2-100,5*24*ptr_inc+120,7); 
      if(kill_all_flag) goto tune_exit;
      lir_hline(0,screen_height/2+100,5*24*ptr_inc+120,7); 
      if(kill_all_flag) goto tune_exit;
      for(j=0; j<2; j++)
        {
        for(i=0; i<5*ptr_inc; i++)
          {
          pwr[i][j]=0;
          }
        }  
      i=1;
      break;

      case RX144:
      for(i=0; i<4*ptr_inc; i++)
        {
        lir_fillbox(12*i+30,yshow[i],8,8,0);
        k=1000*log10(pwr[i][0])-tune_yzer;
        k=screen_height-k;
        if(k<30)k=30;
        if(k>screen_height-9)k=screen_height-9;
        yshow[i]=k; 
        lir_fillbox(12*i+30,yshow[i],8,8,10);
        lir_fillbox(12*i+60+5*12*ptr_inc,yshow[i+5*ptr_inc],8,8,0);
        k=1000*log10(pwr[i][1])-tune_yzer;
        k=screen_height-k;
        if(k<30)k=30;
        if(k>screen_height-9)k=screen_height-9;
        yshow[i+5*ptr_inc]=k; 
        lir_fillbox(12*i+60+5*12*ptr_inc,yshow[i+5*ptr_inc],8,8,13);
        }
      lir_hline(0,screen_height/2,5*24*ptr_inc+120,7); 
      if(kill_all_flag) goto tune_exit;
      lir_hline(0,screen_height/2-100,5*24*ptr_inc+120,7); 
      if(kill_all_flag) goto tune_exit;
      lir_hline(0,screen_height/2+100,5*24*ptr_inc+120,7); 
      if(kill_all_flag) goto tune_exit;
      for(j=0; j<2; j++)
        {
        for(i=0; i<4*ptr_inc; i++)
          {
          pwr[i][j]=0;
          }
        }  
      i=1;
      break;
      }
    lir_refresh_screen();
    }
  else
    {
    i=0;
    }  
  send_bit(i);
  store_xtal_data(unit,i);
// All the data we have in memory now is invalid.
// Read and skip data for this time plus 10 milliseconds.
  update_snd(RXDA);
  make_timing_info();
  t1=0.01+ad_wttim+fft1_wttim;
  i=1+t1*96000*sizeof(int)/snd[RXAD].framesize;
  i*=snd[RXAD].framesize;
  i+=snd[RXAD].block_bytes;
  while(i>0)
    {
    lir_await_event(EVENT_TIMF1);
    if(thread_command_flag[THREAD_TUNE]==THRFLAG_KILL)goto tune_return;
    i-=timf1_blockbytes;
    }
  for(j=0; j<2; j++)
    {
    i=timf1p_pa;
    while(i==timf1p_pa)
      {
      lir_await_event(EVENT_TIMF1);
      if(thread_command_flag[THREAD_TUNE]==THRFLAG_KILL)goto tune_return;
      }
    }  
// Now read the data that we will process!
  mm=0;
  k=0;
  timf1p_px=(timf1p_pa-timf1_usebytes-2*snd[RXAD].block_bytes+
                                                 timf1_bytes)&timf1_bytemask;
  while(mm < tune_bytes)
    {
    lir_await_event(EVENT_TIMF1);
    if(thread_command_flag[THREAD_TUNE]==THRFLAG_KILL)goto tune_return;
    if(kill_all_flag)goto tune_exit;
    mm+=timf1_blockbytes;
    fft1_pa=0;
    fft1_b(timf1p_px, fft1_float, fftw_tmp,0);
    for(k=0; k<tune_size; k++)
      {
      for(m=0; m<nn; m++)
        {
        n=k*nn+m; 
        ib=fft1_size-n-1;
        if(fft1_desired[ib] < 0.1)
          { 
          t1=fft1_float[4*n  ]*fft1_filtercorr[4*ib  ]-
             fft1_float[4*n+1]*fft1_filtercorr[4*ib+1];
          t2=fft1_float[4*n+1]*fft1_filtercorr[4*ib  ]+
             fft1_float[4*n  ]*fft1_filtercorr[4*ib+1];
          pwr[ptr+k][0]+=(t1*t1+t2*t2)*fft1_desired[ib];
          t1=fft1_float[4*n+2]*fft1_filtercorr[4*ib+2]-
             fft1_float[4*n+3]*fft1_filtercorr[4*ib+3];
          t2=fft1_float[4*n+3]*fft1_filtercorr[4*ib+2]+
             fft1_float[4*n+2]*fft1_filtercorr[4*ib+3];
          pwr[ptr+k][1]+=(t1*t1+t2*t2)*fft1_desired[ib];
          }
        }
      }
    }
  sprintf(s,"%.1f dB  ",tune_yzer/100);
  lir_text(1,1,s);
  if(hware_error_flag != 0)
    {
    settextcolor(12);
    sprintf(s,"No response from parallel port");
    }
  else
    {
    for(i=0; i<32; i++)s[i]=' ';
    s[i]=0;  
    }
  hware_error_flag=0;
  lir_text(20,1,s);
  settextcolor(7);
  lir_refresh_screen();
  }
if(thread_command_flag[THREAD_TUNE]==THRFLAG_IDLE)
  {
  thread_status_flag[THREAD_TUNE]=THRFLAG_IDLE;
  while(thread_command_flag[THREAD_TUNE]==THRFLAG_IDLE)
    {
    lir_await_event(EVENT_TIMF1);
    timf1p_px=(timf1p_px+timf1_blockbytes)&timf1_bytemask;
    if(kill_all_flag)goto tune_exit;
    }
  if(thread_command_flag[THREAD_TUNE] == THRFLAG_ACTIVE)goto restart;
  }
tune_exit:;    
tune_return:;
thread_status_flag[THREAD_TUNE]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_TUNE] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}
