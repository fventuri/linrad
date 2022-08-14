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
#include "fft1def.h"
#include "fft2def.h"
#include "fft3def.h"
#include "screendef.h"
#include "seldef.h"
#include "sigdef.h"
#include "hwaredef.h"
#include "thrdef.h"
#include "options.h"
#include "txdef.h"

extern char netsend_rx_multi_group[];
fpos_t file_startpos;
fpos_t file_playpos;

void internal_generator(void)
{
#if INTERNAL_GEN_ADD_AGCTEST == TRUE
char s[80];
int i, j;
int *iib;
double dt2, dt3, dt4;
#define IG_CF1 0.03
#define IG_CF2 0.04
#define IG_WIDTH  0.0001
#define KEY_COUNT (ui.rx_ad_speed*0.0008)
if(internal_generator_key > KEY_COUNT)
  {
  internal_generator_key=0;
  }
internal_generator_key++;
if( (ui.rx_input_mode&DWORD_INPUT) == 0)
  {
  for(i=0; i<(int)snd[RXAD].block_bytes/2; i+=2*ui.rx_rf_channels)
    {
    dt3=0x7e00*sin(internal_generator_phase1);
    dt2=0x7e00*cos(internal_generator_phase1);
    internal_generator_phase1+=fft1_direction*IG_CF1;
    internal_generator_shift+=0.02/ui.rx_ad_speed;
    if(internal_generator_shift > IG_WIDTH)internal_generator_shift=0;
    if(internal_generator_phase1 > 2*(double)(PI_L))internal_generator_phase1-=2*(double)(PI_L);
    if(internal_generator_phase1 < -2*(double)(PI_L))internal_generator_phase1+=2*(double)(PI_L);
    if(internal_generator_key < KEY_COUNT/4)
      {
      dt3+=0x7e00*0.001*sin(internal_generator_phase2);
      dt2+=0x7e00*0.001*cos(internal_generator_phase2);
      internal_generator_phase2+=fft1_direction*IG_CF2;
      if(internal_generator_phase2 > 2*(double)(PI_L))internal_generator_phase2-=2*(double)(PI_L);
      if(internal_generator_phase2 < -2*(double)(PI_L))internal_generator_phase2+=2*(double)(PI_L);
      }
    if(internal_generator_noise != 0)
      {
      dt3+=lir_noisegen(internal_generator_noise-1);
      dt2+=lir_noisegen(internal_generator_noise-1);
      }
    if(truncate_flag != 0) 
      {
      rxin_isho[i  ]+=floor(dt3);
      rxin_isho[i+1]+=floor(dt2);
      }
    else
      {
      rxin_isho[i  ]+=rint(dt3);
      rxin_isho[i+1]+=rint(dt2);
      }  
    if(ui.rx_rf_channels == 2)
      {
      if(truncate_flag != 0) 
        {
        rxin_isho[i+2]+=floor(dt3);
        rxin_isho[i+3]+=floor(dt2);
        }
      else
        {
        rxin_isho[i+2]+=rint(dt3);
        rxin_isho[i+3]+=rint(dt2);
        }  
      }
    }
  }
else
  {
  iib=(int*)(&timf1_char[timf1p_pa]);
  for(i=0; i<(int)snd[RXAD].block_bytes/4; i+=2*ui.rx_rf_channels)
    {
    dt3=0.01*0x7e000000*sin(8*internal_generator_phase1);
    dt2=0.01*0x7e000000*cos(8*internal_generator_phase1);
    dt3+=0.01*0x7e000000*sin(16*internal_generator_phase1);
    dt2+=0.01*0x7e000000*cos(16*internal_generator_phase1);
    internal_generator_phase1+=fft1_direction*IG_CF1;
    internal_generator_shift+=0.0001/ui.rx_ad_speed;
    if(internal_generator_shift > IG_WIDTH)internal_generator_shift=0;
    if(internal_generator_shift > IG_WIDTH-0.005/ui.rx_ad_speed )
      {
      dt3+=lir_noisegen(50);
      dt2+=lir_noisegen(50);
      }      
    if(internal_generator_phase1 > 2*(double)(PI_L))internal_generator_phase1-=2*(double)(PI_L);
    if(internal_generator_phase1 < -2*(double)(PI_L))internal_generator_phase1+=2*(double)(PI_L);
    internal_generator_phase2+=fft1_direction*IG_CF2;
    if(internal_generator_phase2 > 2*(double)(PI_L))internal_generator_phase2-=2*(double)(PI_L);
    if(internal_generator_phase2 < -2*(double)(PI_L))internal_generator_phase2+=2*(double)(PI_L);
    if(internal_generator_key < 0.2*KEY_COUNT)
      {
      dt3+=0x7e000000*0.005*sin(4*internal_generator_phase2);
      dt2+=0x7e000000*0.005*cos(4*internal_generator_phase2);
      dt3+=0x7e000000*0.005*sin(18*internal_generator_phase2);
      dt2+=0x7e000000*0.005*cos(18*internal_generator_phase2);
      }
    else
      {
      dt4=60*internal_generator_key/KEY_COUNT;
      j=dt4;
      dt4-=j;
      if(dt4 > 0.5 && 
           ( internal_generator_key < 0.75*KEY_COUNT ||
             internal_generator_key > 0.9*KEY_COUNT) )
        {
        dt3+=0x7e000000*0.00001*sin(204*internal_generator_phase2);
        dt2+=0x7e000000*0.00001*cos(204*internal_generator_phase2);
        }
      }  
    if(internal_generator_noise != 0)
      {
      dt3+=lir_noisegen(internal_generator_noise-1);
      dt2+=lir_noisegen(internal_generator_noise-1);
      }
    if(truncate_flag != 0) 
      {
      iib[i  ]+=floor(dt3);
      iib[i+1]+=floor(dt2);
      }
    else
      {
      iib[i  ]+=rint(dt3);
      iib[i+1]+=rint(dt2);
      }  
    if(ui.rx_rf_channels == 2)
      {
      if(truncate_flag != 0) 
        {
        iib[i+2]+=floor(dt3);
        iib[i+3]+=floor(dt2);
        }
      else
        {
        iib[i+2]+=rint(dt3);
        iib[i+3]+=rint(dt2);
        }
      }
    }
  }
if(internal_generator_noise != 0)
  {
  sprintf(s,"NOISE LEVEL %d bits",internal_generator_noise);
  lir_text(30,0,s); 
  }
#endif
// *****************************************************************
// *****************************************************************
// *****************************************************************

#if INTERNAL_GEN_AGC_TEST == TRUE
char s[80];
int i, j;
int *iib;
double dt2, dt3, dt4;
#define IG_CF1 0.1
#define IG_CF2 0.003
#define IG_WIDTH  (50.0/96000)
#define KEY_COUNT (150000000.0/ui.rx_ad_speed)
if(internal_generator_key > KEY_COUNT)
  {
  internal_generator_key=0;
  }
internal_generator_key++;
if( (ui.rx_input_mode&DWORD_INPUT) == 0)
  {
  for(i=0; i<(int)snd[RXAD].block_bytes/2; i+=2*ui.rx_rf_channels)
    {
    dt3=0x7e00*sin(internal_generator_phase1);
    dt2=0x7e00*cos(internal_generator_phase1);
    internal_generator_phase1+=fft1_direction*IG_CF1;
    internal_generator_shift+=0.02/ui.rx_ad_speed;
    if(internal_generator_shift > IG_WIDTH)internal_generator_shift=0;
    if(internal_generator_phase1 > 2*(double)(PI_L))internal_generator_phase1-=2*(double)(PI_L);
    if(internal_generator_phase1 < -2*(double)(PI_L))internal_generator_phase1+=2*(double)(PI_L);
    if(internal_generator_key < KEY_COUNT/4)
      {
      dt3+=0x7e00*0.001*sin(internal_generator_phase2);
      dt2+=0x7e00*0.001*cos(internal_generator_phase2);
      internal_generator_phase2+=fft1_direction*IG_CF2;
      if(internal_generator_phase2 > 2*(double)(PI_L))internal_generator_phase2-=2*(double)(PI_L);
      if(internal_generator_phase2 < -2*(double)(PI_L))internal_generator_phase2+=2*(double)(PI_L);
      }
    if(internal_generator_noise != 0)
      {
      dt3+=lir_noisegen(internal_generator_noise-1);
      dt2+=lir_noisegen(internal_generator_noise-1);
      }
    if(truncate_flag != 0) 
      {
      rxin_isho[i  ]=floor(dt3);
      rxin_isho[i+1]=floor(dt2);
      }
    else
      {
      rxin_isho[i  ]=rint(dt3);
      rxin_isho[i+1]=rint(dt2);
      }  
    if(ui.rx_rf_channels == 2)
      {
      rxin_isho[i+2]=rxin_isho[i  ];
      rxin_isho[i+3]=rxin_isho[i+1];
      }
    }
  }
else
  {
  iib=(int*)(&timf1_char[timf1p_pa]);
  for(i=0; i<(int)snd[RXAD].block_bytes/4; i+=2*ui.rx_rf_channels)
    {
    dt3=0.01*0x7e000000*sin(8*internal_generator_phase1);
    dt2=0.01*0x7e000000*cos(8*internal_generator_phase1);
    dt3+=0.01*0x7e000000*sin(10*internal_generator_phase1);
    dt2+=0.01*0x7e000000*cos(10*internal_generator_phase1);
    internal_generator_phase1+=fft1_direction*IG_CF1;
    internal_generator_shift+=0.0001/ui.rx_ad_speed;
    if(internal_generator_shift > IG_WIDTH)internal_generator_shift=0;
    if(internal_generator_shift > IG_WIDTH-0.02/ui.rx_ad_speed )
      {
      dt3+=lir_noisegen(30);
      dt2+=lir_noisegen(30);
      }      
    if(internal_generator_phase1 > 2*(double)(PI_L))internal_generator_phase1-=2*(double)(PI_L);
    if(internal_generator_phase1 < -2*(double)(PI_L))internal_generator_phase1+=2*(double)(PI_L);
    internal_generator_phase2+=fft1_direction*IG_CF2;
    if(internal_generator_phase2 > 2*(double)(PI_L))internal_generator_phase2-=2*(double)(PI_L);
    if(internal_generator_phase2 < -2*(double)(PI_L))internal_generator_phase2+=2*(double)(PI_L);
    if(internal_generator_key < 0.2*KEY_COUNT)
      {
      dt3+=0x7e000000*0.005*sin(200*internal_generator_phase2);
      dt2+=0x7e000000*0.005*cos(200*internal_generator_phase2);
      dt3+=0x7e000000*0.005*sin(210*internal_generator_phase2);
      dt2+=0x7e000000*0.005*cos(210*internal_generator_phase2);
      }
    else
      {
      dt4=60*internal_generator_key/KEY_COUNT;
      j=dt4;
      dt4-=j;
      if(dt4 > 0.5 && 
           ( internal_generator_key < 0.75*KEY_COUNT ||
             internal_generator_key > 0.9*KEY_COUNT) )
        {
        dt3+=0x7e000000*0.00001*sin(204*internal_generator_phase2);
        dt2+=0x7e000000*0.00001*cos(204*internal_generator_phase2);
        }
      }  
    if(internal_generator_noise != 0)
      {
      dt3+=lir_noisegen(internal_generator_noise-1);
      dt2+=lir_noisegen(internal_generator_noise-1);
      }
    if(truncate_flag != 0) 
      {
      iib[i  ]=floor(dt3);
      iib[i+1]=floor(dt2);
      }
    else
      {
      iib[i  ]=rint(dt3);
      iib[i+1]=rint(dt2);
      }  
    if(ui.rx_rf_channels == 2)
      {
      iib[i+2]=iib[i  ];
      iib[i+3]=iib[i+1];
      }
    }
  }
if(internal_generator_noise != 0)
  {
  sprintf(s,"NOISE LEVEL %d bits",internal_generator_noise);
  lir_text(30,0,s); 
  }
#endif
// *****************************************************************
// *****************************************************************
// *****************************************************************
#if INTERNAL_GEN_FILTER_TEST == TRUE
int i;
int *iib;
double dt2, dt3, dt5;
#define IG_WIDTH 2.5
#define IG_CF 0.5
#define KEY_COUNT (5000000.0/ui.rx_ad_speed)

//#define IG_WIDTH .02
//#define IG_CF -.1
//#define KEY_COUNT 16
dt5=pow(0.1,0.5*internal_generator_att);
if(internal_generator_key > KEY_COUNT)
  {
  internal_generator_key=0;
  }
internal_generator_key++;
if( (ui.rx_input_mode&DWORD_INPUT) == 0)
  {
  for(i=0; i<(int)snd[RXAD].block_bytes/2; i+=2*ui.rx_rf_channels)
    {
    dt3=dt5*0x7e00*sin(internal_generator_phase1);
    dt2=dt5*0x7e00*cos(internal_generator_phase1);
    internal_generator_phase1+=fft1_direction*(-IG_CF-IG_WIDTH+
                                                     internal_generator_shift);
    internal_generator_shift+=0.02/ui.rx_ad_speed;
    if(internal_generator_shift > 2*IG_WIDTH)internal_generator_shift=0;
    if(internal_generator_phase1 > 2*(double)(PI_L))internal_generator_phase1-=2*(double)(PI_L);
    if(internal_generator_phase1 < -2*(double)(PI_L))internal_generator_phase1+=2*(double)(PI_L);
    if(internal_generator_key < KEY_COUNT/4)
      {
      dt3+=0x7e00*0.00001*sin(internal_generator_phase2);
      dt2+=0x7e00*0.00001*cos(internal_generator_phase2);
      internal_generator_phase2+=fft1_direction*IG_CF;
      if(internal_generator_phase2 > 2*(double)(PI_L))internal_generator_phase2-=2*(double)(PI_L);
      if(internal_generator_phase2 < -2*(double)(PI_L))internal_generator_phase2+=2*(double)(PI_L);
      }
    if(internal_generator_noise != 0)
      {
      dt3+=lir_noisegen(internal_generator_noise-1);
      dt2+=lir_noisegen(internal_generator_noise-1);
      }
    if(truncate_flag != 0) 
      {
      rxin_isho[i  ]=floor(dt3);
      rxin_isho[i+1]=floor(dt2);
      }
    else
      {
      rxin_isho[i  ]=rint(dt3);
      rxin_isho[i+1]=rint(dt2);
      }  
    if(ui.rx_rf_channels == 2)
      {
      rxin_isho[i+2]=rxin_isho[i  ];
      rxin_isho[i+3]=rxin_isho[i+1];
      }
    }
  }
else
  {
  iib=(int*)(&timf1_char[timf1p_pa]);
  for(i=0; i<(int)snd[RXAD].block_bytes/4; i+=2*ui.rx_rf_channels)
    {
    dt3=dt5*0x7e000000*sin(internal_generator_phase1);
    dt2=dt5*0x7e000000*cos(internal_generator_phase1);
    internal_generator_phase1+=fft1_direction*(-IG_WIDTH+
                                                     internal_generator_shift);
    internal_generator_shift+=0.05/ui.rx_ad_speed;
    if(internal_generator_shift > 2*IG_WIDTH)internal_generator_shift=0;
    if(internal_generator_phase1 > 2*(double)(PI_L))internal_generator_phase1-=2*(double)(PI_L);
    if(internal_generator_phase1 < -2*(double)(PI_L))internal_generator_phase1+=2*(double)(PI_L);
    internal_generator_phase2+=fft1_direction*IG_CF;
    if(internal_generator_phase2 > 2*(double)(PI_L))internal_generator_phase2-=2*(double)(PI_L);
    if(internal_generator_phase2 < -2*(double)(PI_L))internal_generator_phase2+=2*(double)(PI_L);
    if(internal_generator_key < KEY_COUNT/4)
      {
      dt3+=0x7e000000*0.000001*sin(internal_generator_phase2);
      dt2+=0x7e000000*0.000001*cos(internal_generator_phase2);
      }
    if(internal_generator_noise != 0)
      {
      dt3+=lir_noisegen(internal_generator_noise-1);
      dt2+=lir_noisegen(internal_generator_noise-1);
      }
    if(truncate_flag != 0) 
      {
      iib[i  ]=floor(dt3);
      iib[i+1]=floor(dt2);
      }
    else
      {
      iib[i  ]=rint(dt3);
      iib[i+1]=rint(dt2);
      }  
    if(ui.rx_rf_channels == 2)
      {
      iib[i+2]=iib[i  ];
      iib[i+3]=iib[i+1];
      }
    }
  }
#endif
// *****************************************************************
// *****************************************************************
// *****************************************************************
#if INTERNAL_GEN_CARRIER == TRUE
int i;
int *iib;
double dt2, dt3, dt5, dt6, dt7;
#define IG_WIDTH 0.04
#define IG_CF 0.5
dt5=pow(0.1,0.5*internal_generator_att);
if( (ui.rx_input_mode&IQ_DATA) == 0)
  {
  if( (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    for(i=0; i<(int)snd[RXAD].block_bytes/2; i+=ui.rx_rf_channels)
      {
      dt2=dt5*0x7e00*cos(internal_generator_phase1);
      internal_generator_phase1+=fft1_direction*(-IG_CF-IG_WIDTH+
                                                     internal_generator_shift);
      if(internal_generator_phase1 > 2*(double)(PI_L))internal_generator_phase1-=2*(double)(PI_L);
      if(internal_generator_phase1 < -2*(double)(PI_L))internal_generator_phase1+=2*(double)(PI_L);
      if(internal_generator_noise != 0)
        {
        dt2+=lir_noisegen(internal_generator_noise-1);
        }
      dt6=dt2;
      if(internal_generator_noise != 0)
        {
         if(ui.rx_rf_channels == 2)
          {
          dt6+=lir_noisegen(internal_generator_noise-1);
          }
        dt2+=lir_noisegen(internal_generator_noise-1);
        }
      if(truncate_flag != 0) 
        {
        rxin_isho[i]=floor(dt2);
        }
      else
        {
        rxin_isho[i]=rint(dt2);
        }  
      if(ui.rx_rf_channels == 2)
        {
        if(truncate_flag != 0) 
          {
          rxin_isho[i+1]=floor(dt6);
          }
        else
          {
          rxin_isho[i+1]=rint(dt6);
          }   
        }
      }
    }
  else
    {
    iib=(int*)(&timf1_char[timf1p_pa]);
    for(i=0; i<(int)snd[RXAD].block_bytes/4; i+=ui.rx_rf_channels)
      {
      dt2=dt5*0x7e000000*cos(internal_generator_phase1);
      internal_generator_phase1+=fft1_direction*(-IG_WIDTH+
                                                     internal_generator_shift);
      if(internal_generator_phase1 > 2*(double)(PI_L))internal_generator_phase1-=2*(double)(PI_L);
      if(internal_generator_phase1 < -2*(double)(PI_L))internal_generator_phase1+=2*(double)(PI_L);
      dt6=dt2;
      if(internal_generator_noise != 0)
        {
         if(ui.rx_rf_channels == 2)
          {
          dt6+=lir_noisegen(internal_generator_noise-1);
          }
        dt2+=lir_noisegen(internal_generator_noise-1);
        }
      if(truncate_flag != 0) 
        {
        iib[i]=floor(dt2);
        }
      else
        {
        iib[i]=rint(dt2);
        }  
      if(ui.rx_rf_channels == 2)
        {
        if(truncate_flag != 0) 
          {
          iib[i+1]=floor(dt6);
          }
        else
          {
          iib[i+1]=rint(dt6);
          }   
        }
      }
    }
  }
else
  {  
  if( (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    for(i=0; i<(int)snd[RXAD].block_bytes/2; i+=2*ui.rx_rf_channels)
      {
      dt3=dt5*0x7e00*sin(internal_generator_phase1);
      dt2=dt5*0x7e00*cos(internal_generator_phase1);
      internal_generator_phase1+=fft1_direction*(-IG_CF-IG_WIDTH+
                                                     internal_generator_shift);
      if(internal_generator_phase1 > 2*(double)(PI_L))internal_generator_phase1-=2*(double)(PI_L);
      if(internal_generator_phase1 < -2*(double)(PI_L))internal_generator_phase1+=2*(double)(PI_L);
      dt6=dt2;
      dt7=dt3;
      if(internal_generator_noise != 0)
        {
         if(ui.rx_rf_channels == 2)
          {
          dt6+=lir_noisegen(internal_generator_noise-1);
          dt7+=lir_noisegen(internal_generator_noise-1);
          }
        dt2+=lir_noisegen(internal_generator_noise-1);
        dt3+=lir_noisegen(internal_generator_noise-1);
        }
      if(truncate_flag != 0) 
        {
        rxin_isho[i  ]=floor(dt3);
        rxin_isho[i+1]=floor(dt2);
        }
      else
        {
        rxin_isho[i  ]=rint(dt3);
        rxin_isho[i+1]=rint(dt2);
        }  
      if(ui.rx_rf_channels == 2)
        {
        if(truncate_flag != 0) 
          {
          rxin_isho[i+2]=floor(dt7);
          rxin_isho[i+3]=floor(dt6);
          }
        else
          {
          rxin_isho[i+2]=rint(dt7);
          rxin_isho[i+3]=rint(dt6);
          }
        }
      }
    }
  else
    {
    iib=(int*)(&timf1_char[timf1p_pa]);
    for(i=0; i<(int)snd[RXAD].block_bytes/4; i+=2*ui.rx_rf_channels)
      {
      dt3=dt5*0x7e000000*sin(internal_generator_phase1);
      dt2=dt5*0x7e000000*cos(internal_generator_phase1);
      internal_generator_phase1+=fft1_direction*(-IG_WIDTH+
                                                     internal_generator_shift);
      if(internal_generator_phase1 > 2*(double)(PI_L))internal_generator_phase1-=2*(double)(PI_L);
      if(internal_generator_phase1 < -2*(double)(PI_L))internal_generator_phase1+=2*(double)(PI_L);
      dt6=dt2;
      dt7=dt3;
      if(internal_generator_noise != 0)
        {
        if(ui.rx_rf_channels == 2)
          {
          dt6+=lir_noisegen(internal_generator_noise-1);
          dt7+=lir_noisegen(internal_generator_noise-1);
          }
        dt2+=lir_noisegen(internal_generator_noise-1);
        dt3+=lir_noisegen(internal_generator_noise-1);
        }
      if(truncate_flag != 0) 
        {
        iib[i  ]=floor(dt3);
        iib[i+1]=floor(dt2);
        }
      else
        {
        iib[i  ]=rint(dt3);
        iib[i+1]=rint(dt2);
        }  
      if(ui.rx_rf_channels == 2)
        {
        if(truncate_flag != 0) 
          {
          iib[i+2]=floor(dt7);
          iib[i+3]=floor(dt6);
          }
        else
          {
          iib[i+2]=rint(dt7);
          iib[i+3]=rint(dt6);
          }
        }  
      }
    }
  }
#endif
}  

void set_raw_userfreq(void)
{
net_rxdata_16.userx_freq=-1;
net_rxdata_16.userx_no=-1;
net_rxdata_18.userx_freq=-1;
net_rxdata_18.userx_no=-1;
net_rxdata_24.userx_freq=-1;
net_rxdata_24.userx_no=-1;
}

void write_raw_file(void)
{
int i, timf1p_pnw;
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_WRITE_RAW_FILE);
#endif
thread_status_flag[THREAD_WRITE_RAW_FILE]=THRFLAG_ACTIVE;
timf1p_pnw=timf1p_pa;
while(!kill_all_flag &&
         thread_command_flag[THREAD_WRITE_RAW_FILE]==THRFLAG_ACTIVE)
  {
  lir_await_event(EVENT_WRITE_RAW_FILE);
  while(timf1p_pnw!=timf1p_pa &&
           thread_command_flag[THREAD_WRITE_RAW_FILE]==THRFLAG_ACTIVE)
    {
    if( (ui.rx_input_mode&DWORD_INPUT) == 0)
      {
      i=fwrite(&timf1_char[timf1p_pnw],1,snd[RXAD].block_bytes,save_wr_file);
      if(i != (int)snd[RXAD].block_bytes)goto file_end;
      }
    else
      {
      timf1p_pc_disk=timf1p_pnw;  
      compress_rawdat_disk();
      i=fwrite(rawsave_tmp_disk,1,save_rw_bytes,save_wr_file);
      if(i != save_rw_bytes)goto file_end;
      }
    timf1p_pnw=(timf1p_pnw+snd[RXAD].block_bytes)&timf1_bytemask;
    }
  }
file_end:
fclose(save_wr_file);
save_wr_file=NULL;
thread_status_flag[THREAD_WRITE_RAW_FILE]=THRFLAG_RETURNED;
while(!kill_all_flag &&
            thread_command_flag[THREAD_WRITE_RAW_FILE] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}  

void do_network_send(int timf1p_pn)
{
char *charbuf;
int j, k, m, nn, mm;
float *za, *zb;
double dt1;
short int *ya, *yb;
netstart_time=current_time();
charbuf=(char*)&timf1_char[timf1p_pn];
//    ******************* SEND RAW16 *****************************
if( (ui.network_flag & NET_RXOUT_RAW16) != 0)
  {
  net_rxdata_16.time=rx_read_time;
  net_rxdata_16.passband_center=rx_passband_center_mhz;
  net_rxdata_16.passband_direction=fg.passband_direction;
  j=0;
  if( (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    while(j < (int)snd[RXAD].block_bytes)
      {
      net_rxdata_16.buf[netsend_ptr_16  ]=charbuf[j  ];
      net_rxdata_16.buf[netsend_ptr_16+1]=charbuf[j+1];
      netsend_ptr_16+=2;
      j+=2;
      if(netsend_ptr_16 >= NET_MULTICAST_PAYLOAD)
        {
        netraw16_blknum++;
        net_rxdata_16.block_no=netraw16_blknum;
        net_rxdata_16.ptr=next_blkptr_16;
        lir_send_raw16();
        next_blkptr_16=j;
        if(next_blkptr_16 >= snd[RXAD].block_bytes)next_blkptr_16=0;
        netsend_ptr_16=0;
        }
      }  
    }
  else
    {
    while(j < (int)snd[RXAD].block_bytes)
      {
      net_rxdata_16.buf[netsend_ptr_16  ]=charbuf[j+2];
      net_rxdata_16.buf[netsend_ptr_16+1]=charbuf[j+3];
      netsend_ptr_16+=2;
      j+=4;
      if(netsend_ptr_16 >= NET_MULTICAST_PAYLOAD)
        {
        netraw16_blknum++;
        net_rxdata_16.block_no=netraw16_blknum;
        net_rxdata_16.ptr=next_blkptr_16;
        lir_send_raw16();
        next_blkptr_16=j/2;
        if(next_blkptr_16 >= snd[RXAD].block_bytes/2)next_blkptr_16=0;
        netsend_ptr_16=0;
        }
      }  
    }  
  }
//    ******************* SEND RAW18 *****************************
if( (ui.network_flag & NET_RXOUT_RAW18) != 0)
  {
  timf1p_pc_net=timf1p_pn;  
  compress_rawdat_net();
  net_rxdata_18.time=rx_read_time;
  net_rxdata_18.passband_center=rx_passband_center_mhz;
  net_rxdata_18.passband_direction=fg.passband_direction;
  j=0;
  while(j < save_rw_bytes)
    {
    net_rxdata_18.buf[netsend_ptr_18]=rawsave_tmp_net[j];
    netsend_ptr_18++;
    j++;
    if(netsend_ptr_18 >= NET_MULTICAST_PAYLOAD)
      {
      netraw18_blknum++;
      net_rxdata_18.block_no=netraw18_blknum;
      net_rxdata_18.ptr=next_blkptr_18;
      lir_send_raw18();
      next_blkptr_18=j;
      if( next_blkptr_18 >= save_rw_bytes)next_blkptr_18=0;
      netsend_ptr_18=0;
      }
    }
  }
//    ******************* SEND RAW24 *****************************
if( (ui.network_flag & NET_RXOUT_RAW24) != 0)
  {
  net_rxdata_24.time=rx_read_time;
  net_rxdata_24.passband_center=rx_passband_center_mhz;
  net_rxdata_24.passband_direction=fg.passband_direction;
  j=0;
  while(j < (int)snd[RXAD].block_bytes)
    {
    net_rxdata_24.buf[netsend_ptr_24  ]=charbuf[j+1];
    net_rxdata_24.buf[netsend_ptr_24+1]=charbuf[j+2];
    net_rxdata_24.buf[netsend_ptr_24+2]=charbuf[j+3];
    netsend_ptr_24+=3;
    j+=4;
    if(netsend_ptr_24 >= NET_MULTICAST_PAYLOAD)
      {
      netraw24_blknum++;
      net_rxdata_24.block_no=netraw24_blknum;      
      net_rxdata_24.ptr=next_blkptr_24;
      lir_send_raw24();
      next_blkptr_24=j;
      if( next_blkptr_24 >= snd[RXAD].block_bytes)next_blkptr_24=0;
      netsend_ptr_24=0;
      }
    }
  }
if( (ui.network_flag&NET_RXOUT_FFT1) != 0)
  {
// It is important for slow computers that may be connected via the
// network that we distribute packets evenly in time.  
// The fft1 transform may be very large and it could arrive at a rate 
// of 25 Hz or even lower.
// The A/D interrupt rate is high when we send fft1 transforms.
//  (see buf.c) 
// It may be different if input is from the network or from the
// hard disk.
  mm=snd[RXAD].block_bytes/(1-fft1_interleave_ratio);
  if( (ui.rx_input_mode&DWORD_INPUT) == 0)mm*=2;

  k=(fft1net_pa-fft1net_px+fft1net_size)&fft1net_mask;
  if( k > mm+fft1_blockbytes)
    {
    mm+=snd[RXAD].block_bytes/2;
    }
  if( k < 1.5*mm)
    {
    mm-=snd[RXAD].block_bytes/2;
    }
  mm&=-4;
  if( k > mm)
    {
    net_rxdata_fft1.time=rx_read_time;
    net_rxdata_fft1.passband_center=rx_passband_center_mhz;
    net_rxdata_fft1.passband_direction=fg.passband_direction;
    j=0;
    while(j < mm)
      {
      net_rxdata_fft1.buf[netsend_ptr_fft1  ]=fft1_netsend_buffer[fft1net_px  ];
      net_rxdata_fft1.buf[netsend_ptr_fft1+1]=fft1_netsend_buffer[fft1net_px+1];
      net_rxdata_fft1.buf[netsend_ptr_fft1+2]=fft1_netsend_buffer[fft1net_px+2];
      net_rxdata_fft1.buf[netsend_ptr_fft1+3]=fft1_netsend_buffer[fft1net_px+3];
      netsend_ptr_fft1+=4;
      j+=4;
      fft1net_px=(fft1net_px+4)&fft1net_mask;
      if(netsend_ptr_fft1 >= NET_MULTICAST_PAYLOAD)
        {
        netfft1_blknum++;
        net_rxdata_fft1.block_no=netfft1_blknum;      
        net_rxdata_fft1.ptr=next_blkptr_fft1;
        lir_send_fft1();
        next_blkptr_fft1=fft1net_px&(fft1_blockbytes-1);
        netsend_ptr_fft1=0;
        }
      }
    }  
  }
if( (ui.network_flag&NET_RXOUT_BASEB) != 0)
  {
  net_rxdata_baseb.time=rx_read_time;
  net_rxdata_baseb.passband_center=0;
  net_rxdata_baseb.passband_direction=1;
  while( ((basebnet_pa-basebnet_px+basebnet_size)&basebnet_mask) > 
                                                    basebnet_block_bytes+32)
// The baseband signal is timed to fit a loudspeaker output so
// it should already be evenly distributed in time. 
    {
    j=0;
    while(j < (int)basebnet_block_bytes)
      {
      net_rxdata_baseb.buf[netsend_ptr_baseb  ]=baseb_netsend_buffer[basebnet_px  ];
      net_rxdata_baseb.buf[netsend_ptr_baseb+1]=baseb_netsend_buffer[basebnet_px+1];
      net_rxdata_baseb.buf[netsend_ptr_baseb+2]=baseb_netsend_buffer[basebnet_px+2];
      net_rxdata_baseb.buf[netsend_ptr_baseb+3]=baseb_netsend_buffer[basebnet_px+3];
      netsend_ptr_baseb+=4;
      basebnet_px=(basebnet_px+4)&basebnet_mask;
      j+=4;
      if(netsend_ptr_baseb >= NET_MULTICAST_PAYLOAD)
        {
        netbaseb_blknum++;
        net_rxdata_baseb.block_no=netbaseb_blknum;
        net_rxdata_baseb.ptr=next_blkptr_baseb;
        lir_send_baseb();
        next_blkptr_baseb=j;
        if((int)next_blkptr_baseb >= basebnet_block_bytes)next_blkptr_baseb=0;
        netsend_ptr_baseb=0;
        }
      }  
    }
  }
if( (ui.network_flag&NET_RXOUT_BASEBRAW) != 0)
  {
#if NET_BASEBRAW_MODE == BASEBAND_IQ_TEST
  DEB"\nOUT:%d\n",basebrawnet_px/16);
#endif
  net_rxdata_basebraw.time=rx_read_time;
  net_rxdata_basebraw.passband_center=0.001*hwfreq;
  net_rxdata_basebraw.passband_direction=basebraw_passband_direction;
  while( ((basebrawnet_pa-basebrawnet_px+basebrawnet_size)&basebrawnet_mask) > 
                                                basebrawnet_block_bytes+32)
    {
    j=0;
    while(j < (int)basebrawnet_block_bytes)
      {
      net_rxdata_basebraw.buf[netsend_ptr_basebraw  ]=basebraw_netsend_buffer[basebrawnet_px+1];
      net_rxdata_basebraw.buf[netsend_ptr_basebraw+1]=basebraw_netsend_buffer[basebrawnet_px+2];
      net_rxdata_basebraw.buf[netsend_ptr_basebraw+2]=basebraw_netsend_buffer[basebrawnet_px+3];
      netsend_ptr_basebraw+=3;
#if NET_BASEBRAW_MODE == BASEBAND_IQ_MASTER_TEST
int* ntbuf;
ntbuf=(int*)&basebraw_netsend_buffer[basebrawnet_px];
if(ntbuf[0] != (int)floor(0.05*basebraw_test_cnt3))
  {
  fprintf( stderr,"?");
  DEB"\nerror");
  }
DEB"\nnet_px %d  %d  %f",basebrawnet_px,ntbuf[0],floor(0.05*basebraw_test_cnt3));
basebraw_test_cnt3+=256;
#endif
#if NET_BASEBRAW_MODE == BASEBAND_IQ_TEST
if( (ui.network_flag & NET_RX_INPUT) == 0)
  {
  if( (basebrawnet_px&15) ==0)
    {
    int *iit;
    iit=(int*)(void*)&basebraw_netsend_buffer[basebrawnet_px];
    DEB"(%d,%d)",iit[0]/256, basebrawnet_px/16);
    } 
  }
#endif
      basebrawnet_px=(basebrawnet_px+4)&basebrawnet_mask;
      j+=4;
      if(netsend_ptr_basebraw >= NET_MULTICAST_PAYLOAD)
        {
        netbasebraw_blknum++;
        net_rxdata_basebraw.block_no=netbasebraw_blknum;
        net_rxdata_basebraw.ptr=next_blkptr_basebraw;
        lir_send_basebraw();
        next_blkptr_basebraw=j;
        if((int)next_blkptr_basebraw >= basebrawnet_block_bytes)next_blkptr_basebraw=0;
        netsend_ptr_basebraw=0;
        }
      }  
    }
  }
if(genparm[SECOND_FFT_ENABLE] != 0)
  {
//    ******************* SEND TIMF2 *****************************
  if( (ui.network_flag & NET_RXOUT_TIMF2) )
    {
    net_rxdata_timf2.time=rx_read_time;
    net_rxdata_timf2.passband_center=rx_passband_center_mhz;
    net_rxdata_timf2.passband_direction=-1;
// Set userx_freq to sampling speed.
    net_rxdata_timf2.userx_freq=timf1_sampling_speed;
    mm=((timf2_pn2-timf2_pt+timf2_size)&timf2_mask);
    if(mm>(timf2_neg))mm=0;
    j=snd[RXAD].block_bytes;
    if( (ui.rx_input_mode&DWORD_INPUT) != 0)j/=2;
    
    if( mm <= j/2)
      {
      mm=0;
      }
    else
      {
      if( mm > 1.25*j)
        {
        mm=1.25*j;
        }
      }    
    j=0;
    if(swfloat)
      {
// Set userx_no to the number of receiver RF channels with
// the sign negative to indicate float format.
      net_rxdata_timf2.userx_no=-ui.rx_rf_channels;
      mm*=2;
      while(j < mm)
        {
        za=(float*)&net_rxdata_timf2.buf[netsend_ptr_timf2];
        zb=&timf2_float[timf2_pt];
        for(nn=0; nn<twice_rxchan; nn++)
          {
          za[nn]=hg_map65_gain*(zb[nn]+hg.map65_strong*zb[twice_rxchan+nn]);
          }
        netsend_ptr_timf2+=twice_rxchan*sizeof(float);
        j+=twice_rxchan*sizeof(float);
        timf2_pt=(timf2_pt+2*twice_rxchan)&timf2_mask;
        if(netsend_ptr_timf2 >= NET_MULTICAST_PAYLOAD)
          {
          nettimf2_blknum++;
          net_rxdata_timf2.block_no=nettimf2_blknum;
          net_rxdata_timf2.ptr=(2*timf2_pt)&(snd[RXAD].block_bytes-1);
          lir_send_timf2();
          netsend_ptr_timf2=0;
          }
        }  
      }  
    else
      {
// Set userx_no to the number of receiver RF channels.
      net_rxdata_timf2.userx_no=ui.rx_rf_channels;
      while(j < mm)
        {
        ya=(short int*)(&net_rxdata_timf2.buf[netsend_ptr_timf2]);
        yb=(short int*)(&timf2_shi[timf2_pt]);
        for(nn=0; nn<twice_rxchan; nn++)
          {
          ya[nn]=hg_map65_gain*(yb[nn]+hg.map65_strong*yb[twice_rxchan+nn]);
          }
        netsend_ptr_timf2+=twice_rxchan*sizeof(short int);
        j+=twice_rxchan*sizeof(short int);
        timf2_pt=(timf2_pt+2*twice_rxchan)&timf2_mask;
        if(netsend_ptr_timf2 >= NET_MULTICAST_PAYLOAD)
          {
          nettimf2_blknum++;
          net_rxdata_timf2.block_no=nettimf2_blknum;
          net_rxdata_timf2.ptr=timf2_pt&(snd[RXAD].block_bytes-1);
          lir_send_timf2();
          netsend_ptr_timf2=0;
          }
        }  
      }
    }
//    ******************* SEND FFT2 *****************************
  if( (ui.network_flag & NET_RXOUT_FFT2) )
    {
// It is important for slow computers that may be connected via the
// network that we distribute packets evenly in time.  
// The fft2 transform may be very large and it could arrive at a rate 
// of 1 Hz or even lower.
// It may be different if input is from the network or from the
// hard disk.
    mm=1.1*snd[RXAD].block_bytes/(1-fft2_interleave_ratio);
    m=mm;
    if( (ui.rx_input_mode&DWORD_INPUT) != 0)mm/=2;
    k=(fft2_pa-fft2_pt+fft2_totbytes)&fft2_mask;
    if( k > mm+fft2_blockbytes)
      {
      mm+=m/2;
      }
    if( k < 1.5*mm)
      {
      mm-=m/2;
      }
    mm&=-4;
    if( k > mm)
      {
      net_rxdata_fft2.time=rx_read_time;
      net_rxdata_fft2.passband_center=rx_passband_center_mhz;
      net_rxdata_fft2.passband_direction=fg.passband_direction;
      j=0;
      if(fft_cntrl[FFT2_CURMODE].mmx == 0)
        { 
        mm*=2;
        charbuf=(char*)(fft2_float);
        while(j < mm)
          {
          net_rxdata_fft2.buf[netsend_ptr_fft2  ]=charbuf[4*fft2_pt  ];
          net_rxdata_fft2.buf[netsend_ptr_fft2+1]=charbuf[4*fft2_pt+1];
          net_rxdata_fft2.buf[netsend_ptr_fft2+2]=charbuf[4*fft2_pt+2];
          net_rxdata_fft2.buf[netsend_ptr_fft2+3]=charbuf[4*fft2_pt+3];
          netsend_ptr_fft2+=4;
          j+=4;
          fft2_pt=(fft2_pt+1)&fft2_mask;
          if(netsend_ptr_fft2 >= NET_MULTICAST_PAYLOAD)
            {
            netfft2_blknum++;
            if( (netfft2_blknum&1) == 0)
              {
// If netfft2_blknum is even, send these things:              
// Set userx_no to the number of receiver RF channels with
// the sign negative to indicate float format.
              net_rxdata_fft2.userx_no=-ui.rx_rf_channels;
// Set userx_freq to sampling speed.
              net_rxdata_fft2.userx_freq=timf1_sampling_speed;
              }
            else
              {
// If fft2_blknum is odd, send these things:              
              net_rxdata_fft2.userx_no=fft2_n;
              net_rxdata_fft2.userx_freq=genparm[SECOND_FFT_SINPOW];
              }
            net_rxdata_fft2.block_no=netfft2_blknum;      
            net_rxdata_fft2.ptr=next_blkptr_fft2;
            lir_send_fft2();
            next_blkptr_fft2=fft2_pt&(fft2_blockbytes-1);
            netsend_ptr_fft2=0;
            }
          }
        }
      else
        {
        charbuf=(char*)(fft2_short_int);
        while(j < mm)
          {
          net_rxdata_fft2.buf[netsend_ptr_fft2  ]=charbuf[2*fft2_pt  ];
          net_rxdata_fft2.buf[netsend_ptr_fft2+1]=charbuf[2*fft2_pt+1];
          netsend_ptr_fft2+=2;
          j+=2;
          fft2_pt=(fft2_pt+1)&fft2_mask;
          if(netsend_ptr_fft2 >= NET_MULTICAST_PAYLOAD)
            {
            netfft2_blknum++;
            if( (netfft2_blknum&1) == 0)
              {
// If fft2_blknum is even, send these things:              
// Set userx_no to the number of receiver RF channels.
              net_rxdata_fft2.userx_no=ui.rx_rf_channels;
// Set userx_freq to sampling speed.
              net_rxdata_fft2.userx_freq=timf1_sampling_speed;
              }
            else
              {
// If fft2_blknum is odd, send these things:              
              net_rxdata_fft2.userx_no=fft2_n;
              net_rxdata_fft2.userx_freq=genparm[SECOND_FFT_SINPOW];
              }
            net_rxdata_fft2.block_no=netfft2_blknum;      
            net_rxdata_fft2.ptr=next_blkptr_fft2;
            lir_send_fft2();
            next_blkptr_fft2=fft2_pt&(fft2_blockbytes-1);
            netsend_ptr_fft2=0;
            }
          }
        }
      }
    }
  }
if(ui.network_flag & NET_RX_OUTPUT)
  {
  dt1=current_time()-netstart_time;
  if(dt1 > max_netwait_time)max_netwait_time=dt1;
  if(netwait_reset != workload_reset_flag)
    {
    net_error_time=0;
    max_netwait_time=0;
    netwait_reset=workload_reset_flag;
    }
  }
}

void network_send(void)
{
int timf1p_pnw;
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_NETWORK_SEND);
#endif
thread_status_flag[THREAD_NETWORK_SEND]=THRFLAG_ACTIVE;   
timf1p_pnw=timf1p_pa;
while(!kill_all_flag &&
         thread_command_flag[THREAD_NETWORK_SEND]==THRFLAG_ACTIVE)
  {
  lir_await_event(EVENT_NETWORK_SEND);
//    ******************* SEND RAW16 *****************************
  while(timf1p_pnw!=timf1p_pa &&
           thread_command_flag[THREAD_NETWORK_SEND]==THRFLAG_ACTIVE)
    { 
    do_network_send(timf1p_pnw);
    timf1p_pnw=(timf1p_pnw+snd[RXAD].block_bytes)&timf1_bytemask;
    }
  }
thread_status_flag[THREAD_NETWORK_SEND]=THRFLAG_RETURNED;
while(!kill_all_flag &&
       thread_command_flag[THREAD_NETWORK_SEND] != THRFLAG_NOT_ACTIVE)
 {
 lir_sleep(1000);
 }
}


void finish_rx_read(void)
{
int *iib;
int mask2, shft;
short int *isho;
int i, j, k, m, ix;
double dt1;
isho=(short int*)(void*)&timf1_char[timf1p_pa];
if(internal_generator_flag != 0)internal_generator();
// Here we post to the screen routine every 0.1 second.      
screen_loop_counter--;
if(screen_loop_counter == 0 && !audio_dump_flag)
  {
  screen_loop_counter=screen_loop_counter_max;
  lir_set_event(EVENT_SCREEN);
  }
if(diskread_flag < 2)
  {
  rx_read_time=ms_since_midnight();
  }
else
  {
  dt1=diskread_time+diskread_block_counter*snd[RXAD].block_frames/ui.rx_ad_speed;
  rx_read_time=dt1/(24*3600);
  dt1-=24*3600*rx_read_time;
  rx_read_time=1000*dt1;
  rx_read_time%=24*3600000;
  }
if( (ui.network_flag & NET_RX_INPUT) != 0)
  { 
  if(abs(latest_listsend_time-rx_read_time) > 1500)
    {
    net_send_slaves_freq();
    }
  }  
if(diskwrite_flag == 1)lir_set_event(EVENT_WRITE_RAW_FILE);
set_raw_userfreq();
if(calibrate_flag)goto skip_network;
if((ui.network_flag&NET_RX_OUTPUT)!=0)
  {
//do_network_send(timf1p_pa);
  lir_set_event(EVENT_NETWORK_SEND);
  }
skip_network:;
if(timf1_mute == TIMF1_MUTED && ptt_state == FALSE)
  {
  if(timf1_mute_counter == 0)
    {
    timf1_mute_counter=mute_off;
    }
  else
    {
    timf1_mute_counter--;
    if(timf1_mute_counter == 0)
      {
      timf1_mute=TIMF1_UNMUTED;
      }
    }
  }    
if(timf1_mute == TIMF1_UNMUTED && ptt_state == TRUE)
  {
  if(timf1_mute_counter == 0)
    {
    timf1_mute_counter=mute_on;
    }
  else
    {
    timf1_mute_counter--;
    if(timf1_mute_counter == 0)
      {
      timf1_mute=TIMF1_MUTED;
      }
    }  
  }
if(timf1_mute == TIMF1_MUTED)
  {
  k=ui.rx_ad_channels;
  m=snd[RXAD].block_frames*k;
  iib=(int*)&timf1_char[timf1p_pa];
  if( (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    for(i=0; i<m; i+=k)
      {
      for(j=0; j<k; j++)
        {
        isho[i+j]=1;
        }
      }  
    }
  else
    {
    for(i=0; i<m; i+=k)
      {
      for(j=0; j<k; j++)
        {
        iib[i+j]=1;
        }
      }
    }
  }

if(truncate_flag != 0) 
  {
  k=ui.rx_ad_channels;
  m=snd[RXAD].block_frames*k;
  iib=(int*)&timf1_char[timf1p_pa];
  mask2=truncate_flag^0xffffffff;
  shft=(truncate_flag+1)/2;
  if( (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    rxin_nbits=16;
    for(i=0; i<m; i+=k)
      {
      for(j=0; j<k; j++)
        {
        ix=isho[i+j];
        ix &= mask2;
        ix+=shft;
        isho[i+j]=ix;
        }
      }  
    }
  else
    {
    rxin_nbits=32;
    for(i=0; i<m; i+=k)
      {
      for(j=0; j<k; j++)
        {
        ix=iib[i+j];
        ix &= mask2;
        ix+=shft;
        iib[i+j]=ix;
        }
      }
    }
  mask2=truncate_flag;
  while(mask2 != 0)
    {
    mask2>>=1;
    rxin_nbits--;
    }
  }
if(internal_generator_noise != 0)
  {
  k=ui.rx_ad_channels;
  m=snd[RXAD].block_frames*k;
  iib=(int*)&timf1_char[timf1p_pa];
  if( (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    for(i=0; i<m; i+=k)
      {
      for(j=0; j<k; j++)
        {
        isho[i+j]+=lir_noisegen(internal_generator_noise-1);
        }
      }  
    }
  else
    {
    for(i=0; i<m; i+=k)
      {
      for(j=0; j<k; j++)
        {
        iib[i+j]+=lir_noisegen(internal_generator_noise-1);
        }
      }
    }
  }
if(ampinfo_flag != 0)
  {
  if(ampinfo_reset != workload_reset_flag)
    {
    ampinfo_reset= workload_reset_flag;
    for(i=0; i<ui.rx_ad_channels; i++)
      {
      rx_ad_maxamp[i]=1;
      }
    }
  k=ui.rx_ad_channels;
  m=snd[RXAD].block_frames*k;
  if(ui.rx_addev_no == EXCALIBUR_DEVICE_CODE)
    {
    if( (ui.rx_input_mode&DWORD_INPUT) == 0)
      {
      for(i=0; i<m; i+=k)
        {
        for(j=0; j<k; j++)
          {
          ix=isho[i+j];
          ix=abs(ix);        
          if(ix > rx_ad_maxamp[j])rx_ad_maxamp[j]=ix;
          }
        }  
      }
    else
      {
      for(i=0; i<m; i+=k)
        {
        for(j=0; j<k; j++)
          {
          ix=isho[1+((i+j)<<1)];
          ix=2*abs(ix);
          if(ix > rx_ad_maxamp[j])rx_ad_maxamp[j]=ix;
          }
        }           
      }
    }
  else
    {
    if( (ui.rx_input_mode&DWORD_INPUT) == 0)
      {
      for(i=0; i<m; i+=k)
        {
        for(j=0; j<k; j++)
          {
          ix=(int)isho[i+j];
          ix=abs(ix);        
          if(ix > rx_ad_maxamp[j])rx_ad_maxamp[j]=ix;
          }
        }  
      }
    else
      {
      for(i=0; i<m; i+=k)
        {
        for(j=0; j<k; j++)
          {
          ix=isho[1+((i+j)<<1)];
          ix=abs(ix);        
          if(ix > rx_ad_maxamp[j])rx_ad_maxamp[j]=ix;
          }
        }           
      }
    }
  }
//!!!!!!!!!!!!!!!!!!!!!!!
#if NET_BASEBRAW_MODE == BASEBAND_IQ_TEST
if( (ui.network_flag & NET_RX_INPUT) != 0)
{
k=ui.rx_ad_channels;
m=snd[RXAD].block_frames*k;
int *nettest;
nettest=(int*)(void*)&timf1_char[timf1p_pa];
  for(i=0; i<m; i+=k)
    {
    DEB"\n%d  ",timf1p_pa);
    for(j=0; j<k; j++)
      {
      ix=nettest[i+j]/256;
      DEB" %d (%d)",ix,i+j);
      }
    if(nettest[i]!=basebraw_test_cnt1)
      {
      fprintf( stderr,"*");
      DEB"\nERROR");
      }
    if(nettest[i]+256 !=nettest[i+1])
      {fprintf( stderr,"!");
      DEB"\nERROR");
      }
    if(k == 4)
      {
      if(nettest[i]+512 != nettest[i+2])
        {
        fprintf( stderr,"@");
        DEB"\nERROR");
        }
      if(nettest[i]+768 != nettest[i+3])
        {
        fprintf( stderr,"#");
        DEB"\nERROR");
        }
      }
    basebraw_test_cnt1=nettest[i]+k*256;
    }           
  }
#endif
//!!!!!!!!!!!!!!!!!!!!!!!!!!!


// Set the EVENT_TIMF1 condition in case there is enough
// data for fft1 to make at least one transform.
if( ((timf1p_pa-timf1p_pb+timf1_bytes)&timf1_bytemask) >= timf1_usebytes)
  {
  timf1p_pb=timf1p_pa;
  lir_set_event(EVENT_TIMF1);
  }
timf1p_pa=(timf1p_pa+snd[RXAD].block_bytes)&timf1_bytemask;
rxin_isho=(short int*)(&timf1_char[timf1p_pa]);
rxin_int=(int*)(&timf1_char[timf1p_pa]);
rxin_char=(char*)(&timf1_char[timf1p_pa]);
rxin_block_counter++;
lir_sched_yield();
}

void rx_file_input(void)
{
int i, j, k;
double speedcalc_counter;
float *z;
int restart_flag;
double dt1, dt2, read_start_time, ideal_block_count;
double total_time1, total_time2;
float t2;
i=0;
screen_loop_counter_max=0.1*snd[RXAD].interrupt_rate;
if(screen_loop_counter_max==0)screen_loop_counter_max=1;
if(file_start_block < 0)
  {
  fgetpos(save_rd_file,&file_startpos);
  diskread_block_counter=0;
  }
else
  {
  fsetpos(save_rd_file,&file_playpos);
  diskread_block_counter=file_start_block;
  }
restart:;
restart_flag=FALSE;
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_RX_FILE_INPUT);
#endif
screen_loop_counter=screen_loop_counter_max;
total_time1=current_time();
read_start_time=total_time1;
total_time2=current_time();
speedcalc_counter=0;
thread_status_flag[THREAD_RX_FILE_INPUT]=THRFLAG_ACTIVE;
while(thread_command_flag[THREAD_RX_FILE_INPUT] == THRFLAG_ACTIVE)
  {
  if(audio_dump_flag != 0)
    {
wait:;    
    lir_set_event(EVENT_TIMF1);
    if(thread_command_flag[THREAD_RX_FILE_INPUT] != THRFLAG_ACTIVE ||
                                     kill_all_flag) goto end_file_rxin;
    if( ((timf1p_pa-timf1p_px+timf1_bytes)&timf1_bytemask) > timf1_bytemask/ 4 ||
        ((fft1_na-fft1_nx+max_fft1n)&fft1n_mask) > max_fft1n/4)
      {
      lir_sleep(5000);
      goto wait;     
      }
    else
      {
      lir_sched_yield();
      }
    dt2=current_time();
    i=0;
    if(dt2 > total_time2+0.25)
      {
      if(no_of_processors < 4) 
// Make sure we do not use 100% of the available CPU time.
// Leave the CPU idle (from Linrad tasks) four times per second.
// First make sure that all important threads have completed,
// Then wait 12.5 milliseconds extra to give at least 5% of the
// total CPU power to other tasks. 
        {
        while(!kill_all_flag &&
             (thread_status_flag[THREAD_WIDEBAND_DSP] != THRFLAG_SEM_WAIT ||
              (thread_status_flag[THREAD_SCREEN] != THRFLAG_SEM_WAIT &&
               thread_status_flag[THREAD_SCREEN] != THRFLAG_IDLE) ) )
          {
          i++; 
          lir_sleep(5000);
          if(i>800)lirerr(88777);
          }
        if(genparm[SECOND_FFT_ENABLE] != 0)
          {
          while(!kill_all_flag && 
                    thread_status_flag[THREAD_SECOND_FFT] != THRFLAG_SEM_WAIT)
            { 
            lir_sleep(5000);
            i++;
            if(i>800)lirerr(88778);
            }
          }
        while(!kill_all_flag && new_baseb_flag <= 0 &&
             (thread_status_flag[THREAD_NARROWBAND_DSP] != THRFLAG_SEM_WAIT &&
              thread_status_flag[THREAD_NARROWBAND_DSP] != THRFLAG_INPUT_WAIT))
          { 
          lir_sleep(5000);
          i++;
          if(i>800)lirerr(88779);
          }
        lir_sleep(5000);  
        }
      dt2=current_time();
      total_time2=dt2;    
      }
    ideal_block_count=speedcalc_counter-0.4;
    dt1=ideal_block_count/snd[RXAD].interrupt_rate;
    read_start_time=dt2-dt1;
    }
  else
    {
    if(speedcalc_counter > 5)
      {
      total_time2=current_time();
      dt1=total_time2-read_start_time;
      ideal_block_count=dt1*snd[RXAD].interrupt_rate+1;
      t2=speedcalc_counter-ideal_block_count-.5;
      if(t2 >0)
        {
        t2/=snd[RXAD].interrupt_rate;
        lir_sleep(1000000*t2);
        }
      total_time2=current_time();
      dt1=total_time2-read_start_time;
      measured_ad_speed=(speedcalc_counter-0.75)*(snd[RXAD].block_frames/dt1);
      }
    }
  if(diskread_pause_flag !=0 )
    {
    lir_sleep(100000);
    restart_flag=TRUE;
    goto skip_read;
    }
  if(restart_flag)goto restart;  
  diskread_block_counter++;
  if(file_stop_block > 0)
    {
    if(diskread_block_counter > file_stop_block)goto end_savfile;
    }
  speedcalc_counter++;
  if(internal_generator_flag == 0 || INTERNAL_GEN_ADD_AGCTEST == TRUE)
    {
    if( diskread_flag == 4)goto end_savfile;
    if( (ui.rx_input_mode&DWORD_INPUT) == 0)
      {
      if( (ui.rx_input_mode&BYTE_INPUT) != 0)
        {
        j=snd[RXAD].block_bytes>>1;
        k=j;
        i=fread(rxin_isho,1,j,save_rd_file);
        while(j > 0)
          {
          j--;
// Subtract 127.5 convert range (0,255) to (-127.5,127.5)
          rxin_isho[j]=(rxin_char[j]<<8)-32640;
          } 
        if(i != k)
          {
          i*=2;
          goto end_savfile;
          }
        }
      else
        { 
        i=fread(rxin_isho,1,snd[RXAD].block_bytes,save_rd_file);
        if(i != (int)snd[RXAD].block_bytes)goto end_savfile;
        }
      }
    else
      {  
      if( (ui.rx_input_mode&(BYTE_INPUT+FLOAT_INPUT+QWORD_INPUT)) != 0)
        {
// Read 24 bit PCM and 32 bit float and PCM wav files here.      
        if( (ui.rx_input_mode&(FLOAT_INPUT+QWORD_INPUT)) == 0)
          {
          j=3*(snd[RXAD].block_bytes>>2);
          k=j;
          i=fread(rxin_isho,1,j,save_rd_file);
          j/=3;
          while(j > 0)
            {
            j--;
            rxin_char[4*j+3]=rxin_char[3*j+2];
            rxin_char[4*j+2]=rxin_char[3*j+1];
            rxin_char[4*j+1]=rxin_char[3*j  ];
            rxin_char[4*j  ]=0;
            } 
          if(i != k)
            {
            i*=4;
            i/=3;
            goto end_savfile;
            }
          }
        else
          {
          z=(float*)rxin_isho;
          i=fread(rxin_isho,1,snd[RXAD].block_bytes,save_rd_file);
          j=snd[RXAD].block_bytes/4;
          if( (ui.rx_input_mode&QWORD_INPUT) == 0)
            {
            while(j > 0)
              {
              j--;
              rxin_int[j]=0x7fffffff*z[j];
              } 
            }
          if(i != (int)snd[RXAD].block_bytes)
            {
            goto end_savfile;
            }
          }   
        }
      else
        {
        i=fread(rawsave_tmp,1,save_rw_bytes,save_rd_file);
        expand_rawdat();
        if(i != save_rw_bytes)
          {
end_savfile:;        
          if(savefile_repeat_flag == 1)
            {
            memset(&timf1_char[timf1p_pa],0,snd[RXAD].block_bytes);
            if(file_start_block < 0)
              {
              fsetpos(save_rd_file,&file_startpos);
              diskread_block_counter=0;
              }
            else
              {
              fsetpos(save_rd_file,&file_playpos);
              diskread_block_counter=file_start_block;
              }
            }
          else
            {
            if(diskread_flag == 2)
              {
              diskread_block_counter=2;
              diskread_flag=4;
              }
            if( (diskread_flag & 4) != 0)
              {
              if(diskread_block_counter/snd[RXAD].interrupt_rate >= total_wttim)
                {
                diskread_flag=8;
                goto end_file_rxin;
                }
              }                                                                
            }
// Clear the 500 last bytes. WAV files contain big numbers
// appended to the signal data in the file that create huge pulses           
          k=(timf1p_pa-500+i+timf1_bytes)&timf1_bytemask;
          while(k != timf1p_pa)
            {
            timf1_char[k]=0;
            k=(k+1)&timf1_bytemask;
            }
          }
        }
      }
    }
  finish_rx_read();
  if(kill_all_flag) goto end_file_rxin;
skip_read:;
  }
end_file_rxin:;
audio_dump_flag=0;
thread_status_flag[THREAD_RX_FILE_INPUT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_RX_FILE_INPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(5000);
  awake_screen();
  }
}

