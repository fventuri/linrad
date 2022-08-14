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
#include "screendef.h"
#include "caldef.h"
#include "seldef.h"
#include "thrdef.h"
#include "options.h"
#include "hwaredef.h"
#if OPENCL_PRESENT == 1
#include "oclprogs.h"
#endif
#if HAVE_CUFFT == 1
#include <cufft.h>
#include <cufftXt.h>
extern cufftHandle cufft_handle_fft1b[MAX_FFT1_THREADS];
extern cuFloatComplex *cuda_in[MAX_FFT1_THREADS];
extern cuFloatComplex *cuda_out[MAX_FFT1_THREADS];
#endif


/*!  @brief Specify the expected direction of each FFT, time or the frequency domains */
typedef enum clfftDirection_
{
	CLFFT_FORWARD	= -1,		/*!< FFT transform from time to frequency domain. */
	CLFFT_BACKWARD	= 1,		/*!< FFT transform from frequency to time domain. */
	CLFFT_MINUS		= -1,		/*!< Alias for the forward transform. */
	CLFFT_PLUS		= 1,		/*!< Alias for the backward transform. */
	ENDDIRECTION			/*!< The last value of the enum, and marks the length of clfftDirection. */
} clfftDirection;

void fix2_dspnames(char *s,int i)
{
s[i]='c';  
s[i+1]='o';
s[i+2]='r';
s[i+3]='r';
s[i+4]=0;
s[0]='d';
s[1]='s';
s[2]='p';
}


int fix1_dspnames(char *s)
{
int i;
i=0;
while(rxpar_filenames[rx_mode][i] != 0)
  {
  s[i]=rxpar_filenames[rx_mode][i];
  i++;
  }
s[i]='_';
i++;  
return i;
}



void make_iqcorr_filename(char *s)
{
int i;
i=fix1_dspnames(s);
s[i]='i';
s[i+1]='q';
i+=2;
fix2_dspnames(s,i);
}  


void make_filfunc_filename(char *s)
{
int i;
i=fix1_dspnames(s);
fix2_dspnames(s,i);
}  

void update_wg_waterf(void)
{      
wg_waterf_ptr-=wg_xpixels;
if(wg_waterf_ptr < 0)wg_waterf_ptr+=wg_waterf_size;
if(audio_dump_flag)
  {
  awake_screen();
  }  
wg_waterf_sum_counter=0;
}

void fft1_waterfall(void)
{
int i,j,k,m,ix;
float der,t1,t2,y,yval;
while(fft1_sumsq_pwg != fft1_sumsq_pa )
  {
  j=fft1_sumsq_pwg+wg.first_xpoint; 
  for(i=wg_first_point; i <= wg_last_point; i++)
    {
    wg_waterf_sum[i]+=fft1_sumsq[j];
    j++;
    }
  fft1_sumsq_pwg=(fft1_sumsq_pwg+fft1_size)&fft1_sumsq_mask; 
  wg_waterf_sum_counter+=wg.fft_avg1num;
  if(wg_waterf_sum_counter >= wg.waterfall_avgnum)
    {
// A new summed power spectrum has arrived.
// Convert it from linear power scale to log in units of 0.1dB.
// Expand or contract so we get the number of pixels that
// will fit on the screen.
// Store at the pos of our current line in waterfall
    if(wg.xpoints_per_pixel == 1 || wg.pixels_per_xpoint == 1)
      {
      i=wg.first_xpoint;
      for(ix=0; ix < wg_xpixels; ix++)
        {
        y=1000*(float)log10(wg_waterf_sum[i]*wg_waterf_yfac[i]);
        if(y < -32767)y=-32767;
        if(y>32767)y=32767;
        wg_waterf[wg_waterf_ptr+ix]=(short int)y;
        i++;
        }
      }    
    else
      {
      if(wg.xpoints_per_pixel == 0)
        {
// There are more pixels than data points so we must interpolate.
        y=1000*(float)log10(wg_waterf_sum[wg.first_xpoint]*
                                           wg_waterf_yfac[wg.first_xpoint]);
        yval=y;
        if(y < -32767)y=-32767;
        if(y>32767)y=32767;
        wg_waterf[wg_waterf_ptr]=(short int)y;
        i=wg.first_xpoint+1;
        m=wg_xpixels-wg.pixels_per_xpoint;
        for(ix=0; ix<m; ix+=wg.pixels_per_xpoint)
          {
          t1=1000*(float)log10(wg_waterf_sum[i]*wg_waterf_yfac[i]);
          der=(t1-yval)/(float)wg.pixels_per_xpoint;
          for(k=ix+1; k<=ix+wg.pixels_per_xpoint; k++)
            {
            yval=yval+der;
            y=yval;
            if(y < -32767)y=32767;
            if(y>32767)y=32767;
            wg_waterf[wg_waterf_ptr+k]=(short int)y;
            }  
          yval=t1;
          i++;
          }      
        if(i < fft1_size)
          {          
          t1=1000*(float)log10(wg_waterf_sum[i]*wg_waterf_yfac[i]);
          der=(t1-yval)/(float)wg.pixels_per_xpoint;
          for(k=ix+1; k<=ix+wg.pixels_per_xpoint; k++)
            {
            yval=yval+der;
            y=yval;
            if(y < -32767)y=32767;
            if(y>32767)y=32767;
            wg_waterf[wg_waterf_ptr+k]=(short int)y;
            }
          }  
        }
      else
        {
// There is more than one data point for each pixel.
// Pick the strongest bin. We want to enhance the visibility of 
// frequency stable signal.
// Slide a triangular filter across the spectrum to make it
// smoother before resampling.
        i=wg.first_xpoint;
        for(ix=0; ix<wg_xpixels; ix++)
          {
          t1=0;
          for(k=0; k<wg.xpoints_per_pixel; k++)
            {
            t2=wg_waterf_sum[i+k]*wg_waterf_yfac[i+k];
            if(t2 >t1)t1=t2;
            }
          y=1000*(float)log10(t1);
          if(y < -32767)y=-32767;
          if(y>32767)y=32767;
          wg_waterf[wg_waterf_ptr+ix]=(short int)y;
          i+=wg.xpoints_per_pixel;
          }
        }  
      }
    for(i=wg_first_point; i <= wg_last_point; i++)
      {
// Set a very small value rather than zero to avoid errors
// due to sending zero into the log function.      
      wg_waterf_sum[i]=0.00001F;
      }
    update_wg_waterf();      
    }                                            
  }
}

void fft1win_gpu(int timf1p_ref, float *tmp)
{
int m, nn, no;
int p0, pa, pb, pa1, pb1;
int ia,ib;
float *z;
nn=fft1_size/2;
z=tmp;
if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
  {
  m=timf1_bytemask/sizeof(short int);
  p0=timf1p_ref/sizeof(short int);
  p0=(p0-fft1_interleave_points*2+m+1)&m;
  pa=p0;
  pb=(pa+fft1_size)&m;
  pa1=pa;
  pb1=pb;
  if(ui.sample_shift < 0)
    {
    pa1=(pa+2*ui.sample_shift+m+1)&m;
    pb1=(pa1+fft1_size)&m;
    }
  if(ui.sample_shift > 0)
    {
    pa=(pa-2*ui.sample_shift+m+1)&m;
    pb=(pb-2*ui.sample_shift+m+1)&m;
    }          
  for(no=0; no<gpu_fft1_batch_size; no++)
    {
    ib=fft1_size-2;
    if(ui.sample_shift == 0)
      {
      if(genparm[FIRST_FFT_SINPOW] != 0)
        {
        for( ia=0; ia<nn; ia++)
          {
          z[2*ia  ]=timf1_short_int[pa  ]*fft1_window[2*ia];
          ib+=2;
          z[2*ia+1]=timf1_short_int[pa+1]*fft1_window[2*ia];      
          pa=(pa+2)&m;
          z[ib  ]=timf1_short_int[pb  ]*fft1_window[2*ia+1];
          z[ib+1]=timf1_short_int[pb+1]*fft1_window[2*ia+1];   
          pb=(pb+2)&m;
          }
        }
      else
        {
        for( ia=0; ia<nn; ia++)
          {
          z[2*ia  ]=timf1_short_int[pa  ];
          ib+=2;
          z[2*ia+1]=timf1_short_int[pa+1];      
          pa=(pa+2)&m;
          z[ib  ]=timf1_short_int[pb  ];
          z[ib+1]=timf1_short_int[pb+1];   
          pb=(pb+2)&m;
          }
        }
      }
    else
      {
      if(genparm[FIRST_FFT_SINPOW] != 0)
        {
        for( ia=0; ia<nn; ia++)
          {
          z[2*ia  ]=timf1_short_int[pa  ]*fft1_window[2*ia];
          ib+=2;
          pa=(pa+2)&m;
          z[2*ia+1]=timf1_short_int[pa1+1]*fft1_window[2*ia];      
          pa1=(pa1+2)&m;
          z[ib  ]=timf1_short_int[pb  ]*fft1_window[2*ia+1];
          pb=(pb+2)&m;
          z[ib+1]=timf1_short_int[pb1+1]*fft1_window[2*ia+1];   
          pb1=(pb1+2)&m;
          }
        }
      else
        {
        for( ia=0; ia<nn; ia++)
          {
          z[2*ia  ]=timf1_short_int[pa  ];
          ib+=2;
          z[2*ia+1]=timf1_short_int[pa1+1];      
          pa=(pa+2)&m;
          pa1=(pa1+2)&m;
          z[ib  ]=timf1_short_int[pb  ];
          pb=(pb+2)&m;
          z[ib+1]=timf1_short_int[pb1+1];   
          pb1=(pb1+2)&m;
          }
        }
      }
    z+=2*fft1_size;  
    pa=(pa+fft1_size-2*fft1_interleave_points)&m;
    pb=(pb+fft1_size-2*fft1_interleave_points)&m;
    }
  }  
else
  {
  m=timf1_bytemask/sizeof(int);
  p0=timf1p_ref/sizeof(int);
  p0=(p0-fft1_interleave_points*2+m+1)&m;
  pa=p0;
  pb=(pa+fft1_size)&m;
  pa1=pa;
  pb1=pb;
  if(ui.sample_shift < 0)
    {
    pa1=(pa+2*ui.sample_shift+m+1)&m;
    pb1=(pa1+fft1_size)&m;
    }
  if(ui.sample_shift > 0)
    {
    pa=(pa-2*ui.sample_shift+m+1)&m;
    pb=(pb-2*ui.sample_shift+m+1)&m;
    }          
  for(no=0; no<gpu_fft1_batch_size; no++)
    {
    ib=fft1_size-2;
    if(ui.sample_shift == 0)
      {
      if(genparm[FIRST_FFT_SINPOW] != 0)
        {
        for( ia=0; ia<nn; ia++)
          {
          z[2*ia  ]=timf1_int[pa  ]*fft1_window[2*ia];
          ib+=2;
          z[2*ia+1]=timf1_int[pa+1]*fft1_window[2*ia];      
          pa=(pa+2)&m;
          z[ib  ]=timf1_int[pb  ]*fft1_window[2*ia+1];
          z[ib+1]=timf1_int[pb+1]*fft1_window[2*ia+1];   
          pb=(pb+2)&m;
          }
        }
      else
        {
        for( ia=0; ia<nn; ia++)
          {
          ib+=2;
          pa=(pa+2)&m;
          pb=(pb+2)&m;
          z[2*ia  ]=(float)timf1_int[pa  ];
          z[2*ia+1]=(float)timf1_int[pa+1];
          z[ib  ]=(float)timf1_int[pb  ];;
          z[ib+1]=(float)timf1_int[pb+1];;
          }
        }
      }
    else
      {
      if(genparm[FIRST_FFT_SINPOW] != 0)
        {
        for( ia=0; ia<nn; ia++)
          {
          z[2*ia  ]=timf1_int[pa  ]*fft1_window[2*ia];
          ib+=2;
          pa=(pa+2)&m;
          z[2*ia+1]=timf1_int[pa1+1]*fft1_window[2*ia];      
          pa1=(pa1+2)&m;
          z[ib  ]=timf1_int[pb  ]*fft1_window[2*ia+1];
          pb=(pb+2)&m;
          z[ib+1]=timf1_int[pb1+1]*fft1_window[2*ia+1];   
          pb1=(pb1+2)&m;
          }
        }
      else
        {
        for( ia=0; ia<nn; ia++)
          {
          z[2*ia  ]=timf1_int[pa  ];
          ib+=2;
          z[2*ia+1]=timf1_int[pa1+1];      
          pa=(pa+2)&m;
          pa1=(pa1+2)&m;
          z[ib  ]=timf1_int[pb  ];
          z[ib+1]=timf1_int[pb1+1];   
          pb=(pb+2)&m;
          pb1=(pb1+2)&m;
          }
        }
      }
    z+=2*fft1_size;  
    pa=(pa+fft1_size-2*fft1_interleave_points)&m;
    pb=(pb+fft1_size-2*fft1_interleave_points)&m;
    }
  }
}

void fft1win_dif_one(int timf1p_ref, float *tmp)
{
int m, nn;
int p0, pa, pb, pa1, pb1;
int ia,ib;
float t1,t2,t3,t4;
float x1,x2;
nn=fft1_size/2;
if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
  {
  m=timf1_bytemask/sizeof(short int);
  p0=timf1p_ref/sizeof(short int);
  p0=(p0-fft1_interleave_points*2+m+1)&m;
  pa=p0;
  pb=(pa+fft1_size)&m;
  ib=fft1_size-2;
  if(ui.sample_shift == 0)
    {
    if(genparm[FIRST_FFT_SINPOW] != 0)
      {
      for( ia=0; ia<nn; ia++)
        {
        t1=timf1_short_int[pa  ]*fft1_window[2*ia];
        t2=timf1_short_int[pa+1]*fft1_window[2*ia];      
        ib+=2;
        t3=timf1_short_int[pb  ]*fft1_window[2*ia+1];
        t4=timf1_short_int[pb+1]*fft1_window[2*ia+1];   
        pa=(pa+2)&m;
        x1=t1-t3;
        tmp[2*ia  ]=  t1+t3;
        x2=t4-t2;
        tmp[2*ia+1]=-(t2+t4);
        pb=(pb+2)&m;
        tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        }
      }
    else
      {
      for( ia=0; ia<nn; ia++)
        {
        t1=timf1_short_int[pa  ];
        ib+=2;
        t2=timf1_short_int[pa+1];      
        t3=timf1_short_int[pb  ];
        t4=timf1_short_int[pb+1];   
        x1=t1-t3;
        pa=(pa+2)&m;
        x2=t4-t2;
        tmp[2*ia  ]=  t1+t3;
        tmp[2*ia+1]=-(t2+t4);
        pb=(pb+2)&m;
        tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        }
      }
    }
  else
    {
    if(ui.sample_shift < 0)
      {
      pa1=(pa+2*ui.sample_shift+m+1)&m;
      pb1=(pa1+fft1_size)&m;
      }
    else
      {
      pa1=pa;
      pb1=pb;
      pa=(pa-2*ui.sample_shift+m+1)&m;
      pb=(pb-2*ui.sample_shift+m+1)&m;
      }          
    if(genparm[FIRST_FFT_SINPOW] != 0)
      {
      for( ia=0; ia<nn; ia++)
        {
        t1=timf1_short_int[pa  ]*fft1_window[2*ia];
        t2=timf1_short_int[pa1+1]*fft1_window[2*ia];      
        ib+=2;
        t3=timf1_short_int[pb  ]*fft1_window[2*ia+1];
        t4=timf1_short_int[pb1+1]*fft1_window[2*ia+1];   
        pa=(pa+2)&m;
        x1=t1-t3;
        pb=(pb+2)&m;
        tmp[2*ia  ]=  t1+t3;
        x2=t4-t2;
        pa1=(pa1+2)&m;
        tmp[2*ia+1]=-(t2+t4);
        pb1=(pb1+2)&m;
        tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        }
      }
    else
      {
      for( ia=0; ia<nn; ia++)
        {
        t1=timf1_short_int[pa  ];
        ib+=2;
        t2=timf1_short_int[pa1+1];      
        t3=timf1_short_int[pb  ];
        pa=(pa+2)&m;
        t4=timf1_short_int[pb1+1];   
        x1=t1-t3;
        pb=(pb+2)&m;
        x2=t4-t2;
        tmp[2*ia  ]=  t1+t3;
        pa1=(pa1+2)&m;
        tmp[2*ia+1]=-(t2+t4);
        pb1=(pb1+2)&m;
        tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        }
      }
    }
  }  
else
  {
  m=timf1_bytemask/sizeof(int);
  p0=timf1p_ref/sizeof(int);
  p0=(p0-fft1_interleave_points*2+m+1)&m;
  pa=p0;
  pb=(pa+fft1_size)&m;
  ib=fft1_size-2;
  if(ui.sample_shift == 0)
    {
    if(genparm[FIRST_FFT_SINPOW] != 0)
      {
      for( ia=0; ia<nn; ia++)
        {
        t1=timf1_int[pa  ]*fft1_window[2*ia];
        t2=timf1_int[pa+1]*fft1_window[2*ia];      
        ib+=2;
        t3=timf1_int[pb  ]*fft1_window[2*ia+1];
        t4=timf1_int[pb+1]*fft1_window[2*ia+1];   
        pa=(pa+2)&m;
        x1=t1-t3;
        tmp[2*ia  ]=  t1+t3;
        x2=t4-t2;
        tmp[2*ia+1]=-(t2+t4);
        pb=(pb+2)&m;
        tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        }
      }
    else
      {
      for( ia=0; ia<nn; ia++)
        {
        t1=timf1_int[pa  ];
        t2=timf1_int[pa+1];      
        ib+=2;
        t3=timf1_int[pb  ];
        t4=timf1_int[pb+1];   
        pa=(pa+2)&m;
        x1=t1-t3;
        x2=t4-t2;
        pb=(pb+2)&m;
        tmp[2*ia  ]=  t1+t3;
        tmp[2*ia+1]=-(t2+t4);
        tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        }
      }
    }
  else
    {
    if(ui.sample_shift < 0)
      {
      pa1=(pa+2*ui.sample_shift+m+1)&m;
      pb1=(pa1+fft1_size)&m;
      }
    else
      {
      pa1=pa;
      pb1=pb;
      pa=(pa-2*ui.sample_shift+m+1)&m;
      pb=(pb-2*ui.sample_shift+m+1)&m;
      }          
    if(genparm[FIRST_FFT_SINPOW] != 0)
      {
      for( ia=0; ia<nn; ia++)
        {
        t1=timf1_int[pa  ]*fft1_window[2*ia];
        t2=timf1_int[pa1+1]*fft1_window[2*ia];      
        ib+=2;
        t3=timf1_int[pb  ]*fft1_window[2*ia+1];
        t4=timf1_int[pb1+1]*fft1_window[2*ia+1];   
        pa=(pa+2)&m;
        x1=t1-t3;
        pb=(pb+2)&m;
        tmp[2*ia  ]=  t1+t3;
        x2=t4-t2;
        pa1=(pa1+2)&m;
        tmp[2*ia+1]=-(t2+t4);
        pb1=(pb1+2)&m;
        tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        }
      }
    else
      {
      for( ia=0; ia<nn; ia++)
        {
        t1=timf1_int[pa  ];
        t2=timf1_int[pa1+1];      
        ib+=2;
        t3=timf1_int[pb  ];
        t4=timf1_int[pb1+1];   
        pa=(pa+2)&m;
        x1=t1-t3;
        x2=t4-t2;
        pb=(pb+2)&m;
        tmp[2*ia  ]=  t1+t3;
        tmp[2*ia+1]=-(t2+t4);
        pa1=(pa1+2)&m;
        tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        pb1=(pb1+2)&m;
        tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        }
      }
    }
  }
}

void dif_permute_one(float *out, float *tmp)
{
int nn, ia, ib, ic;
nn=fft1_size;
for(ia=0; ia < nn; ia+=2)
  {
  ib=2*fft1_permute[ia];                               
  ic=2*fft1_permute[ia+1];                             
  out[ib  ]=tmp[2*ia  ]+tmp[2*ia+2];
  out[ic  ]=tmp[2*ia  ]-tmp[2*ia+2];
  out[ib+1]=tmp[2*ia+1]+tmp[2*ia+3];
  out[ic+1]=tmp[2*ia+1]-tmp[2*ia+3];
  }
}

void dif_permute_chan(int chan, float *out, float *tmp)
{
int n, ia, ib, ic, pc;
n=fft1_size;
pc=2*chan;
for(ia=0; ia < n; ia+=2)
  {
  ib=pc+4*fft1_permute[ia];                               
  ic=pc+4*fft1_permute[ia+1];
  out[ib  ]=tmp[2*ia  ]+tmp[2*ia+2];
  out[ic  ]=tmp[2*ia  ]-tmp[2*ia+2];
  out[ib+1]=tmp[2*ia+1]+tmp[2*ia+3];
  out[ic+1]=tmp[2*ia+1]-tmp[2*ia+3];
  }
}

void dif_bigpermute_chan(int chan, float *out, float *tmp)
{
int n, ia, ib, ic, pc;
n=fft1_size;
pc=2*chan;
for(ia=0; ia < n; ia+=2)
  {
  ib=pc+4*fft1_bigpermute[ia];                               
  ic=pc+4*fft1_bigpermute[ia+1];
  out[ib  ]=tmp[2*ia  ]+tmp[2*ia+2];
  out[ic  ]=tmp[2*ia  ]-tmp[2*ia+2];
  out[ib+1]=tmp[2*ia+1]+tmp[2*ia+3];
  out[ic+1]=tmp[2*ia+1]-tmp[2*ia+3];
  }
}

void fft1win_dit_one(int timf1p_ref, float *out)
{
int j, m, nn;
int p0, pa, pa1;
int ia,ib,ic,id;
float t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12;
int ja, jb, jc, jd;
float a1,a2;
float b1,b2;
float c1,c2;
float d1,d2;
nn=fft1_size;
if( (ui.rx_input_mode&DWORD_INPUT) == 0)
  {
  m=timf1_bytemask/sizeof(short int);
  p0=timf1p_ref/sizeof(short int);
  p0=(p0-fft1_interleave_points*2+m+1)&m;
  if(ui.sample_shift == 0)
    {
    if(genparm[FIRST_FFT_SINPOW] != 0)
      {
      for(j=0; j<nn; j+=4)
        {
        ja=fft1_permute[j  ];
        jb=fft1_permute[j+1];
        jc=fft1_permute[j+2];
        jd=fft1_permute[j+3];
        ia=(p0+2*ja)&m;
        ib=(p0+2*jb)&m;
        ic=(p0+2*jc)&m;
        id=(p0+2*jd)&m;
        a1=fft1_window[ja]*timf1_short_int[ia  ];
        a2=fft1_window[ja]*timf1_short_int[ia+1];
        b1=fft1_window[jb]*timf1_short_int[ib  ];
        b2=fft1_window[jb]*timf1_short_int[ib+1];
        c1=fft1_window[jc]*timf1_short_int[ic  ];
        c2=fft1_window[jc]*timf1_short_int[ic+1];
        d1=fft1_window[jd]*timf1_short_int[id  ];
        d2=fft1_window[jd]*timf1_short_int[id+1];
        t1=a1+b1;
        t2=a2+b2;
        t3=c1+d1;
        t4=c2+d2;
        t5=a1-b1;
        t7=a2-b2;
        t10=c1-d1;
        t6= c2-d2;
        out[2*j  ]=t1+t3;
        out[2*j+1]=t2+t4;
        out[2*j+4]=t1-t3;
        out[2*j+5]=t2-t4;
        t11=t5-t6;
        t8=t7-t10;
        t12=t5+t6;
        t9=t7+t10;
        out[2*j+2]=t12;
        out[2*j+3]=t8;
        out[2*j+6]=t11;
        out[2*j+7]=t9;
        }
      }
    else
      {
      for(j=0; j<nn; j+=4)
        {
        ia=(p0+2*fft1_permute[j  ])&m;
        ib=(p0+2*fft1_permute[j+1])&m;
        ic=(p0+2*fft1_permute[j+2])&m;
        id=(p0+2*fft1_permute[j+3])&m;
        t1=(float)(timf1_short_int[ia  ])+(float)(timf1_short_int[ib  ]);
        t2=(float)(timf1_short_int[ia+1])+(float)(timf1_short_int[ib+1]);
        t3=(float)(timf1_short_int[ic  ])+(float)(timf1_short_int[id  ]);
        t4=(float)(timf1_short_int[ic+1])+(float)(timf1_short_int[id+1]);
        t5=(float)(timf1_short_int[ia  ])-(float)(timf1_short_int[ib  ]);
        t7=(float)(timf1_short_int[ia+1])-(float)(timf1_short_int[ib+1]);
        t10=(float)(timf1_short_int[ic  ])-(float)(timf1_short_int[id  ]);
        t6= (float)(timf1_short_int[ic+1])-(float)(timf1_short_int[id+1]);
        out[2*j  ]=t1+t3;
        out[2*j+1]=t2+t4;
        out[2*j+4]=t1-t3;
        out[2*j+5]=t2-t4;
        t11=t5-t6;
        t8=t7-t10;
        t12=t5+t6;
        t9=t7+t10;
        out[2*j+2]=t12;
        out[2*j+3]=t8;
        out[2*j+6]=t11;
        out[2*j+7]=t9;
        }
      }
    }
  else
    {
    if(ui.sample_shift < 0)
      {
      pa=p0;
      pa1=(p0+2*ui.sample_shift+m+1)&m;
      }
    else
      {
      pa1=p0;
      pa=(p0-2*ui.sample_shift+m+1)&m;
      }          
    if(genparm[FIRST_FFT_SINPOW] != 0)
      {
      for(j=0; j<nn; j+=4)
        {
        ja=fft1_permute[j  ];
        jb=fft1_permute[j+1];
        jc=fft1_permute[j+2];
        jd=fft1_permute[j+3];
        a1=fft1_window[ja]*timf1_short_int[((pa+2*ja)&m)  ];
        a2=fft1_window[ja]*timf1_short_int[((pa1+2*ja)&m)+1];
        b1=fft1_window[jb]*timf1_short_int[((pa+2*jb)&m)  ];
        b2=fft1_window[jb]*timf1_short_int[((pa1+2*jb)&m)+1];
        c1=fft1_window[jc]*timf1_short_int[((pa+2*jc)&m)  ];
        c2=fft1_window[jc]*timf1_short_int[((pa1+2*jc)&m)+1];
        d1=fft1_window[jd]*timf1_short_int[((pa+2*jd)&m)  ];
        d2=fft1_window[jd]*timf1_short_int[((pa1+2*jd)&m)+1];
        t1=a1+b1;
        t2=a2+b2;
        t3=c1+d1;
        t4=c2+d2;
        t5=a1-b1;
        t7=a2-b2;
        t10=c1-d1;
        t6= c2-d2;
        out[2*j  ]=t1+t3;
        out[2*j+1]=t2+t4;
        out[2*j+4]=t1-t3;
        out[2*j+5]=t2-t4;
        t11=t5-t6;
        t8=t7-t10;
        t12=t5+t6;
        t9=t7+t10;
        out[2*j+2]=t12;
        out[2*j+3]=t8;
        out[2*j+6]=t11;
        out[2*j+7]=t9;
        }
      }
    else
      {
      for(j=0; j<nn; j+=4)
        {
        t1=(float)(timf1_short_int[((pa+2*fft1_permute[j  ])&m)  ])+
           (float)(timf1_short_int[((pa+2*fft1_permute[j+1])&m)  ]);
        t2=(float)(timf1_short_int[((pa1+2*fft1_permute[j  ])&m)+1])+
           (float)(timf1_short_int[((pa1+2*fft1_permute[j+1])&m)+1]);
        t3=(float)(timf1_short_int[((pa+2*fft1_permute[j+2])&m)  ])+
           (float)(timf1_short_int[((pa+2*fft1_permute[j+3])&m)  ]);
        t4=(float)(timf1_short_int[((pa1+2*fft1_permute[j+2])&m)+1])+
           (float)(timf1_short_int[((pa1+2*fft1_permute[j+3])&m)+1]);
        t5=(float)(timf1_short_int[((pa+2*fft1_permute[j  ])&m)  ])-
           (float)(timf1_short_int[((pa+2*fft1_permute[j+1])&m)  ]);
        t7=(float)(timf1_short_int[((pa1+2*fft1_permute[j  ])&m)+1])-
           (float)(timf1_short_int[((pa1+2*fft1_permute[j+1])&m)+1]);
        t10=(float)(timf1_short_int[((pa+2*fft1_permute[j+2])&m)  ])-
            (float)(timf1_short_int[((pa+2*fft1_permute[j+3])&m)  ]);
        t6= (float)(timf1_short_int[((pa1+2*fft1_permute[j+2])&m)+1])-
            (float)(timf1_short_int[((pa1+2*fft1_permute[j+3])&m)+1]);
        out[2*j  ]=t1+t3;
        out[2*j+1]=t2+t4;
        out[2*j+4]=t1-t3;
        out[2*j+5]=t2-t4;
        t11=t5-t6;
        t8=t7-t10;
        t12=t5+t6;
        t9=t7+t10;
        out[2*j+2]=t12;
        out[2*j+3]=t8;
        out[2*j+6]=t11;
        out[2*j+7]=t9;
        }
      }
    }
  }  
else
  {
  m=timf1_bytemask/sizeof(int);
  p0=timf1p_ref/sizeof(int);
  p0=(p0-fft1_interleave_points*2+m+1)&m;
  if(ui.sample_shift == 0)
    {
    if(genparm[FIRST_FFT_SINPOW] != 0)
      {
      for(j=0; j<nn; j+=4)
        {
        ja=fft1_permute[j  ];
        jb=fft1_permute[j+1];
        jc=fft1_permute[j+2];
        jd=fft1_permute[j+3];
        ia=(p0+2*ja)&m;
        ib=(p0+2*jb)&m;
        ic=(p0+2*jc)&m;
        id=(p0+2*jd)&m;
        a1=fft1_window[ja]*timf1_int[ia  ];
        a2=fft1_window[ja]*timf1_int[ia+1];
        b1=fft1_window[jb]*timf1_int[ib  ];
        b2=fft1_window[jb]*timf1_int[ib+1];
        c1=fft1_window[jc]*timf1_int[ic  ];
        c2=fft1_window[jc]*timf1_int[ic+1];
        d1=fft1_window[jd]*timf1_int[id  ];
        d2=fft1_window[jd]*timf1_int[id+1];
        t1=a1+b1;
        t2=a2+b2;
        t3=c1+d1;
        t4=c2+d2;
        t5=a1-b1;
        t7=a2-b2;
        t10=c1-d1;
        t6= c2-d2;
        out[2*j  ]=t1+t3;
        out[2*j+1]=t2+t4;
        out[2*j+4]=t1-t3;
        out[2*j+5]=t2-t4;
        t11=t5-t6;
        t8=t7-t10;
        t12=t5+t6;
        t9=t7+t10;
        out[2*j+2]=t12;
        out[2*j+3]=t8;
        out[2*j+6]=t11;
        out[2*j+7]=t9;
        }
      }
    else
      {
      for(j=0; j<nn; j+=4)
        {
        ia=(p0+2*fft1_permute[j  ])&m;
        ib=(p0+2*fft1_permute[j+1])&m;
        ic=(p0+2*fft1_permute[j+2])&m;
        id=(p0+2*fft1_permute[j+3])&m;
        t1=(float)(timf1_int[ia  ])+(float)(timf1_int[ib  ]);
        t2=(float)(timf1_int[ia+1])+(float)(timf1_int[ib+1]);
        t3=(float)(timf1_int[ic  ])+(float)(timf1_int[id  ]);
        t4=(float)(timf1_int[ic+1])+(float)(timf1_int[id+1]);
        t5=(float)(timf1_int[ia  ])-(float)(timf1_int[ib  ]);
        t7=(float)(timf1_int[ia+1])-(float)(timf1_int[ib+1]);
        t10=(float)(timf1_int[ic  ])-(float)(timf1_int[id  ]);
        t6= (float)(timf1_int[ic+1])-(float)(timf1_int[id+1]);
        out[2*j  ]=t1+t3;
        out[2*j+1]=t2+t4;
        out[2*j+4]=t1-t3;
        out[2*j+5]=t2-t4;
        t11=t5-t6;
        t8=t7-t10;
        t12=t5+t6;
        t9=t7+t10;
        out[2*j+2]=t12;
        out[2*j+3]=t8;
        out[2*j+6]=t11;
        out[2*j+7]=t9;
        }
      }
    }
  else
    {
    if(ui.sample_shift < 0)
      {
      pa=p0;
      pa1=(p0+2*ui.sample_shift+m+1)&m;
      }
    else
      {
      pa1=p0;
      pa=(p0-2*ui.sample_shift+m+1)&m;
      }          
    if(genparm[FIRST_FFT_SINPOW] != 0)
      {
      for(j=0; j<nn; j+=4)
        {
        ja=fft1_permute[j  ];
        jb=fft1_permute[j+1];
        jc=fft1_permute[j+2];
        jd=fft1_permute[j+3];
        a1=fft1_window[ja]*timf1_int[(pa+2*ja)&m  ];
        a2=fft1_window[ja]*timf1_int[((pa1+2*ja)&m)+1];
        b1=fft1_window[jb]*timf1_int[(pa+2*jb)&m    ];
        b2=fft1_window[jb]*timf1_int[((pa1+2*jb)&m) +1];
        c1=fft1_window[jc]*timf1_int[(pa+2*jc)&m    ];
        c2=fft1_window[jc]*timf1_int[((pa1+2*jc)&m)+1];
        d1=fft1_window[jd]*timf1_int[(pa+2*jd)&m    ];
        d2=fft1_window[jd]*timf1_int[((pa1+2*jd)&m)+1];
        t1=a1+b1;
        t2=a2+b2;
        t3=c1+d1;
        t4=c2+d2;
        t5=a1-b1;
        t7=a2-b2;
        t10=c1-d1;
        t6= c2-d2;
        out[2*j  ]=t1+t3;
        out[2*j+1]=t2+t4;
        out[2*j+4]=t1-t3;
        out[2*j+5]=t2-t4;
        t11=t5-t6;
        t8=t7-t10;
        t12=t5+t6;
        t9=t7+t10;
        out[2*j+2]=t12;
        out[2*j+3]=t8;
        out[2*j+6]=t11;
        out[2*j+7]=t9;
        }
      }
    else
      {
      for(j=0; j<nn; j+=4)
        {
        t1= (float)(timf1_int[((pa+2*fft1_permute[j  ])&m)  ])+
            (float)(timf1_int[((pa+2*fft1_permute[j+1])&m)  ]);
        t2= (float)(timf1_int[((pa1+2*fft1_permute[j  ])&m)+1])+
            (float)(timf1_int[((pa1+2*fft1_permute[j+1])&m)+1]);
        t3= (float)(timf1_int[((pa+2*fft1_permute[j+2])&m)  ])+
            (float)(timf1_int[((pa+2*fft1_permute[j+3])&m)  ]);
        t4= (float)(timf1_int[((pa1+2*fft1_permute[j+2])&m)+1])+
            (float)(timf1_int[((pa1+2*fft1_permute[j+3])&m)+1]);
        t5= (float)(timf1_int[((pa+2*fft1_permute[j  ])&m)  ])-
            (float)(timf1_int[((pa+2*fft1_permute[j+1])&m)  ]);
        t6= (float)(timf1_int[((pa1+2*fft1_permute[j+2])&m)+1])-
            (float)(timf1_int[((pa1+2*fft1_permute[j+3])&m)+1]);
        t7= (float)(timf1_int[((pa1+2*fft1_permute[j  ])&m)+1])-
            (float)(timf1_int[((pa1+2*fft1_permute[j+1])&m)+1]);
        t10=(float)(timf1_int[((pa+2*fft1_permute[j+2])&m)  ])-
            (float)(timf1_int[((pa+2*fft1_permute[j+3])&m)  ]);
        out[2*j  ]=t1+t3;
        out[2*j+1]=t2+t4;
        out[2*j+4]=t1-t3;
        out[2*j+5]=t2-t4;
        t11=t5-t6;
        t8=t7-t10;
        t12=t5+t6;
        t9=t7+t10;
        out[2*j+2]=t12;
        out[2*j+3]=t8;
        out[2*j+6]=t11;
        out[2*j+7]=t9;
        }
      }
    }
  }
}

void fft1win_dit_one_real(int timf1p_ref, float *tmp)
{
int j, m, mm, nn;
int p0;
int ia,ib,ic,id;
float t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12;
int ja, jb, jc, jd;
int ka, kb, kc, kd;
float a1,a2;
float b1,b2;
float c1,c2;
float d1,d2;
nn=2*fft1_size;
if( (ui.rx_input_mode&DWORD_INPUT) == 0)
  {
  m=timf1_bytemask/sizeof(short int);
  p0=timf1p_ref/sizeof(short int);
  p0=(p0-4*fft1_interleave_points+m+1)&m;
  mm=timf1_blockbytes/(2*sizeof(short int));
  if(genparm[FIRST_FFT_SINPOW] != 0)
    {
    for(j=0; j<nn; j+=4)
      {
      ja=fft1_permute[j  ];
      jb=fft1_permute[j+1];
      jc=fft1_permute[j+2];
      jd=fft1_permute[j+3];
      ia=(p0+ja)&m;
      ib=(p0+jb)&m;
      ic=(p0+jc)&m;
      id=(p0+jd)&m;
      ka=(mm+p0+ja)&m;
      kb=(mm+p0+jb)&m;
      kc=(mm+p0+jc)&m;
      kd=(mm+p0+jd)&m;
      a1=fft1_window[ja]*timf1_short_int[ia];
      a2=fft1_window[ja]*timf1_short_int[ka];
      b1=fft1_window[jb]*timf1_short_int[ib];
      b2=fft1_window[jb]*timf1_short_int[kb];
      c1=fft1_window[jc]*timf1_short_int[ic];
      c2=fft1_window[jc]*timf1_short_int[kc];
      d1=fft1_window[jd]*timf1_short_int[id];
      d2=fft1_window[jd]*timf1_short_int[kd];
      t1=a1+b1;
      t2=a2+b2;
      t3=c1+d1;
      t4=c2+d2;
      t5=a1-b1;
      t7=a2-b2;
      t10=c1-d1;
      t6= c2-d2;
      tmp[2*j  ]=t1+t3;
      tmp[2*j+1]=t2+t4;
      tmp[2*j+4]=t1-t3;
      tmp[2*j+5]=t2-t4;
      t11=t5-t6;
      t8=t7-t10;
      t12=t5+t6;
      t9=t7+t10;
      tmp[2*j+2]=t12;
      tmp[2*j+3]=t8;
      tmp[2*j+6]=t11;
      tmp[2*j+7]=t9;
      }
    }
  else
    {
    for(j=0; j<nn; j+=4)
      {
      ia=(p0+fft1_permute[j  ])&m;
      ib=(p0+fft1_permute[j+1])&m;
      ic=(p0+fft1_permute[j+2])&m;
      id=(p0+fft1_permute[j+3])&m;
      ka=(mm+ia)&m;
      kb=(mm+ib)&m;
      kc=(mm+ic)&m;
      kd=(mm+id)&m;
      t1=(float)(timf1_short_int[ia])+(float)(timf1_short_int[ib]);
      t2=(float)(timf1_short_int[ka])+(float)(timf1_short_int[kb]);
      t3=(float)(timf1_short_int[ic])+(float)(timf1_short_int[id]);
      t4=(float)(timf1_short_int[kc])+(float)(timf1_short_int[kd]);
      t5=(float)(timf1_short_int[ia])-(float)(timf1_short_int[ib]);
      t7=(float)(timf1_short_int[ka])-(float)(timf1_short_int[kb]);
      t10=(float)(timf1_short_int[ic])-(float)(timf1_short_int[id]);
      t6= (float)(timf1_short_int[kc])-(float)(timf1_short_int[kd]);
      tmp[2*j  ]=t1+t3;
      tmp[2*j+1]=t2+t4;
      tmp[2*j+4]=t1-t3;
      tmp[2*j+5]=t2-t4;
      t11=t5-t6;
      t8=t7-t10;
      t12=t5+t6;
      t9=t7+t10;
      tmp[2*j+2]=t12;
      tmp[2*j+3]=t8;
      tmp[2*j+6]=t11;
      tmp[2*j+7]=t9;
      }
    }
  }  
else
  {
  m=timf1_bytemask/sizeof(int);
  p0=timf1p_ref/sizeof(int);
  p0=(p0-4*fft1_interleave_points+m+1)&m;
  mm=timf1_blockbytes/(2*sizeof(int));
  if(genparm[FIRST_FFT_SINPOW] != 0)
    {
    for(j=0; j<nn; j+=4)
      {
      ja=fft1_permute[j  ];
      jb=fft1_permute[j+1];
      jc=fft1_permute[j+2];
      jd=fft1_permute[j+3];
      ia=(p0+ja)&m;
      ib=(p0+jb)&m;
      ic=(p0+jc)&m;
      id=(p0+jd)&m;
      ka=(mm+p0+ja)&m;
      kb=(mm+p0+jb)&m;
      kc=(mm+p0+jc)&m;
      kd=(mm+p0+jd)&m;
      a1=fft1_window[ja]*timf1_int[ia];
      a2=fft1_window[ja]*timf1_int[ka];
      b1=fft1_window[jb]*timf1_int[ib];
      b2=fft1_window[jb]*timf1_int[kb];
      c1=fft1_window[jc]*timf1_int[ic];
      c2=fft1_window[jc]*timf1_int[kc];
      d1=fft1_window[jd]*timf1_int[id];
      d2=fft1_window[jd]*timf1_int[kd];
      t1=a1+b1;
      t2=a2+b2;
      t3=c1+d1;
      t4=c2+d2;
      t5=a1-b1;
      t7=a2-b2;
      t10=c1-d1;
      t6= c2-d2;
      tmp[2*j  ]=t1+t3;
      tmp[2*j+1]=t2+t4;
      tmp[2*j+4]=t1-t3;
      tmp[2*j+5]=t2-t4;
      t11=t5-t6;
      t8=t7-t10;
      t12=t5+t6;
      t9=t7+t10;
      tmp[2*j+2]=t12;
      tmp[2*j+3]=t8;
      tmp[2*j+6]=t11;
      tmp[2*j+7]=t9;
      }
    }
  else
    {
    for(j=0; j<nn; j+=4)
      {
      ia=(p0+fft1_permute[j  ])&m;
      ib=(p0+fft1_permute[j+1])&m;
      ic=(p0+fft1_permute[j+2])&m;
      id=(p0+fft1_permute[j+3])&m;
      ka=(mm+p0+fft1_permute[j  ])&m;
      kb=(mm+p0+fft1_permute[j+1])&m;
      kc=(mm+p0+fft1_permute[j+2])&m;
      kd=(mm+p0+fft1_permute[j+3])&m;
      t1=(float)(timf1_int[ia])+(float)(timf1_int[ib]);
      t2=(float)(timf1_int[ka])+(float)(timf1_int[kb]);
      t3=(float)(timf1_int[ic])+(float)(timf1_int[id]);
      t4=(float)(timf1_int[kc])+(float)(timf1_int[kd]);
      t5=(float)(timf1_int[ia])-(float)(timf1_int[ib]);
      t7=(float)(timf1_int[ka])-(float)(timf1_int[kb]);
      t10=(float)(timf1_int[ic])-(float)(timf1_int[id]);
      t6= (float)(timf1_int[kc])-(float)(timf1_int[kd]);
      tmp[2*j  ]=t1+t3;
      tmp[2*j+1]=t2+t4;
      tmp[2*j+4]=t1-t3;
      tmp[2*j+5]=t2-t4;
      t11=t5-t6;
      t8=t7-t10;
      t12=t5+t6;
      t9=t7+t10;
      tmp[2*j+2]=t12;
      tmp[2*j+3]=t8;
      tmp[2*j+6]=t11;
      tmp[2*j+7]=t9;
      }
    }
  }
}

void fft1win_dit_one_real_chan(int timf1p_ref, float *tmp, int chan)
{
int j, m, mm, nn;
int p0;
int ia,ib,ic,id;
float t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12;
int ja, jb, jc, jd;
int ka, kb, kc, kd;
float a1,a2;
float b1,b2;
float c1,c2;
float d1,d2;
nn=2*fft1_size;
if( (ui.rx_input_mode&DWORD_INPUT) == 0)
  {
  m=timf1_bytemask/sizeof(short int);
  p0=timf1p_ref/sizeof(short int);
  p0=(p0-4*fft1_interleave_points+m+1)&m;
  mm=timf1_blockbytes/(2*sizeof(short int));
  if(genparm[FIRST_FFT_SINPOW] != 0)
    {
    for(j=0; j<nn; j+=4)
      {
      ja=fft1_permute[j  ];
      jb=fft1_permute[j+1];
      jc=fft1_permute[j+2];
      jd=fft1_permute[j+3];
      ia=(p0+2*ja+chan)&m;
      ib=(p0+2*jb+chan)&m;
      ic=(p0+2*jc+chan)&m;
      id=(p0+2*jd+chan)&m;
      ka=(mm+ia)&m;
      kb=(mm+ib)&m;
      kc=(mm+ic)&m;
      kd=(mm+id)&m;
      a1=fft1_window[ja]*timf1_short_int[ia];
      a2=fft1_window[ja]*timf1_short_int[ka];
      b1=fft1_window[jb]*timf1_short_int[ib];
      b2=fft1_window[jb]*timf1_short_int[kb];
      c1=fft1_window[jc]*timf1_short_int[ic];
      c2=fft1_window[jc]*timf1_short_int[kc];
      d1=fft1_window[jd]*timf1_short_int[id];
      d2=fft1_window[jd]*timf1_short_int[kd];
      t1=a1+b1;
      t2=a2+b2;
      t3=c1+d1;
      t4=c2+d2;
      t5=a1-b1;
      t7=a2-b2;
      t10=c1-d1;
      t6= c2-d2;
      tmp[2*j  ]=t1+t3;
      tmp[2*j+1]=t2+t4;
      tmp[2*j+4]=t1-t3;
      tmp[2*j+5]=t2-t4;
      t11=t5-t6;
      t8=t7-t10;
      t12=t5+t6;
      t9=t7+t10;
      tmp[2*j+2]=t12;
      tmp[2*j+3]=t8;
      tmp[2*j+6]=t11;
      tmp[2*j+7]=t9;
      }
    }
  else
    {
    for(j=0; j<nn; j+=4)
      {
      ia=(p0+2*fft1_permute[j  ]+chan)&m;
      ib=(p0+2*fft1_permute[j+1]+chan)&m;
      ic=(p0+2*fft1_permute[j+2]+chan)&m;
      id=(p0+2*fft1_permute[j+3]+chan)&m;
      ka=(mm+ia)&m;
      kb=(mm+ib)&m;
      kc=(mm+ic)&m;
      kd=(mm+id)&m;
      t1=(float)(timf1_short_int[ia])+(float)(timf1_short_int[ib]);
      t2=(float)(timf1_short_int[ka])+(float)(timf1_short_int[kb]);
      t3=(float)(timf1_short_int[ic])+(float)(timf1_short_int[id]);
      t4=(float)(timf1_short_int[kc])+(float)(timf1_short_int[kd]);
      t5=(float)(timf1_short_int[ia])-(float)(timf1_short_int[ib]);
      t7=(float)(timf1_short_int[ka])-(float)(timf1_short_int[kb]);
      t10=(float)(timf1_short_int[ic])-(float)(timf1_short_int[id]);
      t6= (float)(timf1_short_int[kc])-(float)(timf1_short_int[kd]);
      tmp[2*j  ]=t1+t3;
      tmp[2*j+1]=t2+t4;
      tmp[2*j+4]=t1-t3;
      tmp[2*j+5]=t2-t4;
      t11=t5-t6;
      t8=t7-t10;
      t12=t5+t6;
      t9=t7+t10;
      tmp[2*j+2]=t12;
      tmp[2*j+3]=t8;
      tmp[2*j+6]=t11;
      tmp[2*j+7]=t9;
      }
    }
  }  
else
  {
  m=timf1_bytemask/sizeof(int);
  p0=timf1p_ref/sizeof(int);
  p0=(p0-4*fft1_interleave_points+m+1)&m;
  mm=timf1_blockbytes/(2*sizeof(int));
  if(genparm[FIRST_FFT_SINPOW] != 0)
    {
    for(j=0; j<nn; j+=4)
      {
      ja=fft1_permute[j  ];
      jb=fft1_permute[j+1];
      jc=fft1_permute[j+2];
      jd=fft1_permute[j+3];
      ia=(p0+2*ja+chan)&m;
      ib=(p0+2*jb+chan)&m;
      ic=(p0+2*jc+chan)&m;
      id=(p0+2*jd+chan)&m;
      ka=(mm+ia)&m;
      kb=(mm+ib)&m;
      kc=(mm+ic)&m;
      kd=(mm+id)&m;
      a1=fft1_window[ja]*timf1_int[ia];
      a2=fft1_window[ja]*timf1_int[ka];
      b1=fft1_window[jb]*timf1_int[ib];
      b2=fft1_window[jb]*timf1_int[kb];
      c1=fft1_window[jc]*timf1_int[ic];
      c2=fft1_window[jc]*timf1_int[kc];
      d1=fft1_window[jd]*timf1_int[id];
      d2=fft1_window[jd]*timf1_int[kd];
      t1=a1+b1;
      t2=a2+b2;
      t3=c1+d1;
      t4=c2+d2;
      t5=a1-b1;
      t7=a2-b2;
      t10=c1-d1;
      t6= c2-d2;
      tmp[2*j  ]=t1+t3;
      tmp[2*j+1]=t2+t4;
      tmp[2*j+4]=t1-t3;
      tmp[2*j+5]=t2-t4;
      t11=t5-t6;
      t8=t7-t10;
      t12=t5+t6;
      t9=t7+t10;
      tmp[2*j+2]=t12;
      tmp[2*j+3]=t8;
      tmp[2*j+6]=t11;
      tmp[2*j+7]=t9;
      }
    }
  else
    {
    for(j=0; j<nn; j+=4)
      {
      ia=(p0+2*fft1_permute[j  ]+chan)&m;
      ib=(p0+2*fft1_permute[j+1]+chan)&m;
      ic=(p0+2*fft1_permute[j+2]+chan)&m;
      id=(p0+2*fft1_permute[j+3]+chan)&m;
      ka=(mm+ia)&m;
      kb=(mm+ib)&m;
      kc=(mm+ic)&m;
      kd=(mm+id)&m;
      t1=(float)(timf1_int[ia])+(float)(timf1_int[ib]);
      t2=(float)(timf1_int[ka])+(float)(timf1_int[kb]);
      t3=(float)(timf1_int[ic])+(float)(timf1_int[id]);
      t4=(float)(timf1_int[kc])+(float)(timf1_int[kd]);
      t5=(float)(timf1_int[ia])-(float)(timf1_int[ib]);
      t7=(float)(timf1_int[ka])-(float)(timf1_int[kb]);
      t10=(float)(timf1_int[ic])-(float)(timf1_int[id]);
      t6= (float)(timf1_int[kc])-(float)(timf1_int[kd]);
      tmp[2*j  ]=t1+t3;
      tmp[2*j+1]=t2+t4;
      tmp[2*j+4]=t1-t3;
      tmp[2*j+5]=t2-t4;
      t11=t5-t6;
      t8=t7-t10;
      t12=t5+t6;
      t9=t7+t10;
      tmp[2*j+2]=t12;
      tmp[2*j+3]=t8;
      tmp[2*j+6]=t11;
      tmp[2*j+7]=t9;
      }
    }
  }
}

void fft1win_dit_one_dual_real(int timf1p_ref, float *tmp)
{
int j, m, n, mm;
int p0;
int ia,ib,ic,id;
int ja,jb,jc,jd;
int ka,kb,kc,kd;
int la,lb,lc,ld;
int ma,mb,mc,md;
float t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12;
float r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12;
float a1,a2,a3,a4;
float b1,b2,b3,b4;
float c1,c2,c3,c4;
float d1,d2,d3,d4;
n=2*fft1_size;
if( (ui.rx_input_mode&DWORD_INPUT) == 0)
  {
  m=timf1_bytemask/sizeof(short int);
  p0=timf1p_ref/sizeof(short int);
  mm=timf1_blockbytes/(4*sizeof(short int));
  p0=(p0-fft1_interleave_points*4+m+1)&m;
  if(genparm[FIRST_FFT_SINPOW] == 0)
    {
    for(j=0; j<n; j+=4)
      {
      ia=(p0+fft1_permute[j  ])&m;
      ib=(p0+fft1_permute[j+1])&m;
      ic=(p0+fft1_permute[j+2])&m;
      id=(p0+fft1_permute[j+3])&m;

      ka=(mm+p0+fft1_permute[j  ])&m;
      kb=(mm+p0+fft1_permute[j+1])&m;
      kc=(mm+p0+fft1_permute[j+2])&m;
      kd=(mm+p0+fft1_permute[j+3])&m;

      la=(2*mm+p0+fft1_permute[j  ])&m;
      lb=(2*mm+p0+fft1_permute[j+1])&m;
      lc=(2*mm+p0+fft1_permute[j+2])&m;
      ld=(2*mm+p0+fft1_permute[j+3])&m;

      ma=(3*mm+p0+fft1_permute[j  ])&m;
      mb=(3*mm+p0+fft1_permute[j+1])&m;
      mc=(3*mm+p0+fft1_permute[j+2])&m;
      md=(3*mm+p0+fft1_permute[j+3])&m;

      t1=(float)(timf1_short_int[ia])+(float)(timf1_short_int[ib]);
      t2=(float)(timf1_short_int[ka])+(float)(timf1_short_int[kb]);
      r1=(float)(timf1_short_int[la])+(float)(timf1_short_int[lb]);
      r2=(float)(timf1_short_int[ma])+(float)(timf1_short_int[mb]);

      t3=(float)(timf1_short_int[ic])+(float)(timf1_short_int[id]);
      t4=(float)(timf1_short_int[kc])+(float)(timf1_short_int[kd]);
      r3=(float)(timf1_short_int[lc])+(float)(timf1_short_int[ld]);
      r4=(float)(timf1_short_int[mc])+(float)(timf1_short_int[md]);

      t5=(float)(timf1_short_int[ia])-(float)(timf1_short_int[ib]);
      t7=(float)(timf1_short_int[ka])-(float)(timf1_short_int[kb]);
      r5=(float)(timf1_short_int[la])-(float)(timf1_short_int[lb]);
      r7=(float)(timf1_short_int[ma])-(float)(timf1_short_int[mb]);

      t10=(float)(timf1_short_int[ic])-(float)(timf1_short_int[id]);
      t6= (float)(timf1_short_int[kc])-(float)(timf1_short_int[kd]);
      r10=(float)(timf1_short_int[lc])-(float)(timf1_short_int[ld]);  
      r6= (float)(timf1_short_int[mc])-(float)(timf1_short_int[md]);
      tmp[4*j  ]=t1+t3;
      tmp[4*j+1]=t2+t4;
      tmp[4*j+2]=r1+r3;
      tmp[4*j+3]=r2+r4;
      tmp[4*j+8]=t1-t3;
      tmp[4*j+9]=t2-t4;
      tmp[4*j+10]=r1-r3;
      tmp[4*j+11]=r2-r4;
      t11=t5-t6;
      t8=t7-t10;
      r11=r5-r6;
      r8=r7-r10;
      t12=t5+t6;
      t9=t7+t10;
      r12=r5+r6;
      r9=r7+r10;
      tmp[4*j+4]=t12;
      tmp[4*j+5]=t8;
      tmp[4*j+6]=r12;
      tmp[4*j+7]=r8;
      tmp[4*j+12]=t11;
      tmp[4*j+13]=t9;
      tmp[4*j+14]=r11;
      tmp[4*j+15]=r9;
      }
    }
  else
    {
    for(j=0; j<n; j+=4)
      {
      ja=fft1_permute[j  ];
      jb=fft1_permute[j+1];
      jc=fft1_permute[j+2];
      jd=fft1_permute[j+3];
      ia=(p0+ja)&m;
      ib=(p0+jb)&m;
      ic=(p0+jc)&m;
      id=(p0+jd)&m;
      ka=(mm+p0+fft1_permute[j  ])&m;
      kb=(mm+p0+fft1_permute[j+1])&m;
      kc=(mm+p0+fft1_permute[j+2])&m;
      kd=(mm+p0+fft1_permute[j+3])&m;
      la=(2*mm+p0+fft1_permute[j  ])&m;
      lb=(2*mm+p0+fft1_permute[j+1])&m;
      lc=(2*mm+p0+fft1_permute[j+2])&m;
      ld=(2*mm+p0+fft1_permute[j+3])&m;
      ma=(3*mm+p0+fft1_permute[j  ])&m;
      mb=(3*mm+p0+fft1_permute[j+1])&m;
      mc=(3*mm+p0+fft1_permute[j+2])&m;
      md=(3*mm+p0+fft1_permute[j+3])&m;
      a1=fft1_window[ja]*timf1_short_int[ia];
      a2=fft1_window[ja]*timf1_short_int[ka];
      a3=fft1_window[ja]*timf1_short_int[la];
      a4=fft1_window[ja]*timf1_short_int[ma];
      b1=fft1_window[jb]*timf1_short_int[ib];
      b2=fft1_window[jb]*timf1_short_int[kb];
      b3=fft1_window[jb]*timf1_short_int[lb];
      b4=fft1_window[jb]*timf1_short_int[mb];
      c1=fft1_window[jc]*timf1_short_int[ic];
      c2=fft1_window[jc]*timf1_short_int[kc];
      c3=fft1_window[jc]*timf1_short_int[lc];
      c4=fft1_window[jc]*timf1_short_int[mc];
      d1=fft1_window[jd]*timf1_short_int[id];
      d2=fft1_window[jd]*timf1_short_int[kd];
      d3=fft1_window[jd]*timf1_short_int[ld];
      d4=fft1_window[jd]*timf1_short_int[md];
      t1=a1+b1;
      t2=a2+b2;
      r1=a3+b3;
      r2=a4+b4;
      t5=a1-b1;
      t7=a2-b2;
      r5=a3-b3;
      r7=a4-b4;
      t3=c1+d1;
      t4=c2+d2;
      r3=c3+d3;
      r4=c4+d4;
      t10=c1-d1;
      t6= c2-d2;
      r10=c3-d3;
      r6= c4-d4;
      tmp[4*j  ]=t1+t3;
      tmp[4*j+1]=t2+t4;
      tmp[4*j+2]=r1+r3;
      tmp[4*j+3]=r2+r4;
      tmp[4*j+8]=t1-t3;
      tmp[4*j+9]=t2-t4;
      tmp[4*j+10]=r1-r3;
      tmp[4*j+11]=r2-r4;
      tmp[4*j+4]=t5+t6;
      tmp[4*j+5]=t7-t10;
      tmp[4*j+6]=r5+r6;
      tmp[4*j+7]=r7-r10;
      tmp[4*j+12]=t5-t6;
      tmp[4*j+13]=t7+t10;
      tmp[4*j+14]=r5-r6;
      tmp[4*j+15]=r7+r10;
      }
    }
  }
else
  {
  m=timf1_bytemask/sizeof(int);
  p0=timf1p_ref/sizeof(int);
  p0=(p0-fft1_interleave_points*4+m+1)&m;
  mm=timf1_blockbytes/(4*sizeof(int));
  if(genparm[FIRST_FFT_SINPOW] == 0)
    {
    for(j=0; j<n; j+=4)
      {
      ia=(p0+fft1_permute[j  ])&m;
      ib=(p0+fft1_permute[j+1])&m;
      ic=(p0+fft1_permute[j+2])&m;
      id=(p0+fft1_permute[j+3])&m;
      ka=(mm+p0+fft1_permute[j  ])&m;
      kb=(mm+p0+fft1_permute[j+1])&m;
      kc=(mm+p0+fft1_permute[j+2])&m;
      kd=(mm+p0+fft1_permute[j+3])&m;
      la=(2*mm+p0+fft1_permute[j  ])&m;
      lb=(2*mm+p0+fft1_permute[j+1])&m;
      lc=(2*mm+p0+fft1_permute[j+2])&m;
      ld=(2*mm+p0+fft1_permute[j+3])&m;
      ma=(3*mm+p0+fft1_permute[j  ])&m;
      mb=(3*mm+p0+fft1_permute[j+1])&m;
      mc=(3*mm+p0+fft1_permute[j+2])&m;
      md=(3*mm+p0+fft1_permute[j+3])&m;
      t1=(float)(timf1_int[ia])+(float)(timf1_int[ib]);
      t2=(float)(timf1_int[ka])+(float)(timf1_int[kb]);
      r1=(float)(timf1_int[la])+(float)(timf1_int[lb]);
      r2=(float)(timf1_int[ma])+(float)(timf1_int[mb]);
      t3=(float)(timf1_int[ic])+(float)(timf1_int[id]);
      t4=(float)(timf1_int[kc])+(float)(timf1_int[kd]);
      r3=(float)(timf1_int[lc])+(float)(timf1_int[ld]);
      r4=(float)(timf1_int[mc])+(float)(timf1_int[md]);
      t5=(float)(timf1_int[ia])-(float)(timf1_int[ib]);
      t7=(float)(timf1_int[ka])-(float)(timf1_int[kb]);
      r5=(float)(timf1_int[la])-(float)(timf1_int[lb]);
      r7=(float)(timf1_int[ma])-(float)(timf1_int[mb]);
      t10=(float)(timf1_int[ic])-(float)(timf1_int[id]);
      t6= (float)(timf1_int[kc])-(float)(timf1_int[kd]);
      r10=(float)(timf1_int[lc])-(float)(timf1_int[ld]);  
      r6= (float)(timf1_int[mc])-(float)(timf1_int[md]);
      tmp[4*j  ]=t1+t3;
      tmp[4*j+1]=t2+t4;
      tmp[4*j+2]=r1+r3;
      tmp[4*j+3]=r2+r4;
      tmp[4*j+8]=t1-t3;
      tmp[4*j+9]=t2-t4;
      tmp[4*j+10]=r1-r3;
      tmp[4*j+11]=r2-r4;
      t11=t5-t6;
      t8=t7-t10;
      r11=r5-r6;
      r8=r7-r10;
      t12=t5+t6;
      t9=t7+t10;
      r12=r5+r6;
      r9=r7+r10;
      tmp[4*j+4]=t12;
      tmp[4*j+5]=t8;
      tmp[4*j+6]=r12;
      tmp[4*j+7]=r8;
      tmp[4*j+12]=t11;
      tmp[4*j+13]=t9;
      tmp[4*j+14]=r11;
      tmp[4*j+15]=r9;
      }
    }
  else
    {
    for(j=0; j<n; j+=4)
      {
      ja=fft1_permute[j  ];
      jb=fft1_permute[j+1];
      jc=fft1_permute[j+2];
      jd=fft1_permute[j+3];
      ia=(p0+ja)&m;
      ib=(p0+jb)&m;
      ic=(p0+jc)&m;
      id=(p0+jd)&m;
      ka=(mm+p0+fft1_permute[j  ])&m;
      kb=(mm+p0+fft1_permute[j+1])&m;
      kc=(mm+p0+fft1_permute[j+2])&m;
      kd=(mm+p0+fft1_permute[j+3])&m;
      la=(2*mm+p0+fft1_permute[j  ])&m;
      lb=(2*mm+p0+fft1_permute[j+1])&m;
      lc=(2*mm+p0+fft1_permute[j+2])&m;
      ld=(2*mm+p0+fft1_permute[j+3])&m;
      ma=(3*mm+p0+fft1_permute[j  ])&m;
      mb=(3*mm+p0+fft1_permute[j+1])&m;
      mc=(3*mm+p0+fft1_permute[j+2])&m;
      md=(3*mm+p0+fft1_permute[j+3])&m;
      a1=fft1_window[ja]*timf1_int[ia];
      a2=fft1_window[ja]*timf1_int[ka];
      a3=fft1_window[ja]*timf1_int[la];
      a4=fft1_window[ja]*timf1_int[ma];
      b1=fft1_window[jb]*timf1_int[ib];
      b2=fft1_window[jb]*timf1_int[kb];
      b3=fft1_window[jb]*timf1_int[lb];
      b4=fft1_window[jb]*timf1_int[mb];
      c1=fft1_window[jc]*timf1_int[ic];
      c2=fft1_window[jc]*timf1_int[kc];
      c3=fft1_window[jc]*timf1_int[lc];
      c4=fft1_window[jc]*timf1_int[mc];
      d1=fft1_window[jd]*timf1_int[id];
      d2=fft1_window[jd]*timf1_int[kd];
      d3=fft1_window[jd]*timf1_int[ld];
      d4=fft1_window[jd]*timf1_int[md];
      t1=a1+b1;
      t2=a2+b2;
      r1=a3+b3;
      r2=a4+b4;
      t3=c1+d1;
      t4=c2+d2;
      r3=c3+d3;
      r4=c4+d4;
      t5=a1-b1;
      t7=a2-b2;
      r5=a3-b3;
      r7=a4-b4;
      t10=c1-d1;
      t6= c2-d2;
      r10=c3-d3;
      r6= c4-d4;
      tmp[4*j  ]=t1+t3;
      tmp[4*j+1]=t2+t4;
      tmp[4*j+2]=r1+r3;
      tmp[4*j+3]=r2+r4;
      tmp[4*j+8]=t1-t3;
      tmp[4*j+9]=t2-t4;
      tmp[4*j+10]=r1-r3;
      tmp[4*j+11]=r2-r4;
      t11=t5-t6;
      t8=t7-t10;
      r11=r5-r6;
      r8=r7-r10;
      t12=t5+t6;
      t9=t7+t10;
      r12=r5+r6;
      r9=r7+r10;
      tmp[4*j+4]=t12;
      tmp[4*j+5]=t8;
      tmp[4*j+6]=r12;
      tmp[4*j+7]=r8;
      tmp[4*j+12]=t11;
      tmp[4*j+13]=t9;
      tmp[4*j+14]=r11;
      tmp[4*j+15]=r9;
      }
    }
  }
}

void fft1win_dit_one_dual_real_chan(int timf1p_ref, float *tmp, int chan)
{
int j, m, n, mm;
int p0;
int ia,ib,ic,id;
int ja,jb,jc,jd;
int ka,kb,kc,kd;
int la,lb,lc,ld;
int ma,mb,mc,md;
float t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12;
float r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12;
float a1,a2,a3,a4;
float b1,b2,b3,b4;
float c1,c2,c3,c4;
float d1,d2,d3,d4;
n=2*fft1_size;
if( (ui.rx_input_mode&DWORD_INPUT) == 0)
  {
  m=timf1_bytemask/sizeof(short int);
  p0=timf1p_ref/sizeof(short int);
  mm=timf1_blockbytes/(4*sizeof(short int));
  p0=(p0-fft1_interleave_points*4+m+1)&m;
  if(genparm[FIRST_FFT_SINPOW] == 0)
    {
    for(j=0; j<n; j+=4)
      {
      ia=(p0+2*fft1_permute[j  ]+chan)&m;
      ib=(p0+2*fft1_permute[j+1]+chan)&m;
      ic=(p0+2*fft1_permute[j+2]+chan)&m;
      id=(p0+2*fft1_permute[j+3]+chan)&m;
      ka=(mm+ia)&m;
      kb=(mm+ib)&m;
      kc=(mm+ic)&m;
      kd=(mm+id)&m;
      la=(2*mm+ia)&m;
      lb=(2*mm+ib)&m;
      lc=(2*mm+ic)&m;
      ld=(2*mm+id)&m;
      ma=(3*mm+ia)&m;
      mb=(3*mm+ib)&m;
      mc=(3*mm+ic)&m;
      md=(3*mm+id)&m;
      t1=(float)(timf1_short_int[ia])+(float)(timf1_short_int[ib]);
      t2=(float)(timf1_short_int[ka])+(float)(timf1_short_int[kb]);
      r1=(float)(timf1_short_int[la])+(float)(timf1_short_int[lb]);
      r2=(float)(timf1_short_int[ma])+(float)(timf1_short_int[mb]);
      t3=(float)(timf1_short_int[ic])+(float)(timf1_short_int[id]);
      t4=(float)(timf1_short_int[kc])+(float)(timf1_short_int[kd]);
      r3=(float)(timf1_short_int[lc])+(float)(timf1_short_int[ld]);
      r4=(float)(timf1_short_int[mc])+(float)(timf1_short_int[md]);
      t5=(float)(timf1_short_int[ia])-(float)(timf1_short_int[ib]);
      t7=(float)(timf1_short_int[ka])-(float)(timf1_short_int[kb]);
      r5=(float)(timf1_short_int[la])-(float)(timf1_short_int[lb]);
      r7=(float)(timf1_short_int[ma])-(float)(timf1_short_int[mb]);
      t10=(float)(timf1_short_int[ic])-(float)(timf1_short_int[id]);
      t6= (float)(timf1_short_int[kc])-(float)(timf1_short_int[kd]);
      r10=(float)(timf1_short_int[lc])-(float)(timf1_short_int[ld]);  
      r6= (float)(timf1_short_int[mc])-(float)(timf1_short_int[md]);
      tmp[4*j  ]=t1+t3;
      tmp[4*j+1]=t2+t4;
      tmp[4*j+2]=r1+r3;
      tmp[4*j+3]=r2+r4;
      tmp[4*j+8]=t1-t3;
      tmp[4*j+9]=t2-t4;
      tmp[4*j+10]=r1-r3;
      tmp[4*j+11]=r2-r4;
      t11=t5-t6;
      t8=t7-t10;
      r11=r5-r6;
      r8=r7-r10;
      t12=t5+t6;
      t9=t7+t10;
      r12=r5+r6;
      r9=r7+r10;
      tmp[4*j+4]=t12;
      tmp[4*j+5]=t8;
      tmp[4*j+6]=r12;
      tmp[4*j+7]=r8;
      tmp[4*j+12]=t11;
      tmp[4*j+13]=t9;
      tmp[4*j+14]=r11;
      tmp[4*j+15]=r9;
      }
    }
  else
    {
    for(j=0; j<n; j+=4)
      {
      ja=fft1_permute[j  ];
      jb=fft1_permute[j+1];
      jc=fft1_permute[j+2];
      jd=fft1_permute[j+3];
      ia=(p0+2*ja+chan)&m;
      ib=(p0+2*jb+chan)&m;
      ic=(p0+2*jc+chan)&m;
      id=(p0+2*jd+chan)&m;
      ka=(mm+ia)&m;
      kb=(mm+ib)&m;
      kc=(mm+ic)&m;
      kd=(mm+id)&m;
      la=(2*mm+ia)&m;
      lb=(2*mm+ib)&m;
      lc=(2*mm+ic)&m;
      ld=(2*mm+id)&m;
      ma=(3*mm+ia)&m;
      mb=(3*mm+ib)&m;
      mc=(3*mm+ic)&m;
      md=(3*mm+id)&m;
      a1=fft1_window[ja]*timf1_short_int[ia];
      a2=fft1_window[ja]*timf1_short_int[ka];
      a3=fft1_window[ja]*timf1_short_int[la];
      a4=fft1_window[ja]*timf1_short_int[ma];
      b1=fft1_window[jb]*timf1_short_int[ib];
      b2=fft1_window[jb]*timf1_short_int[kb];
      b3=fft1_window[jb]*timf1_short_int[lb];
      b4=fft1_window[jb]*timf1_short_int[mb];
      c1=fft1_window[jc]*timf1_short_int[ic];
      c2=fft1_window[jc]*timf1_short_int[kc];
      c3=fft1_window[jc]*timf1_short_int[lc];
      c4=fft1_window[jc]*timf1_short_int[mc];
      d1=fft1_window[jd]*timf1_short_int[id];
      d2=fft1_window[jd]*timf1_short_int[kd];
      d3=fft1_window[jd]*timf1_short_int[ld];
      d4=fft1_window[jd]*timf1_short_int[md];
      t1=a1+b1;
      t2=a2+b2;
      r1=a3+b3;
      r2=a4+b4;
      t5=a1-b1;
      t7=a2-b2;
      r5=a3-b3;
      r7=a4-b4;
      t3=c1+d1;
      t4=c2+d2;
      r3=c3+d3;
      r4=c4+d4;
      t10=c1-d1;
      t6= c2-d2;
      r10=c3-d3;
      r6= c4-d4;
      tmp[4*j  ]=t1+t3;
      tmp[4*j+1]=t2+t4;
      tmp[4*j+2]=r1+r3;
      tmp[4*j+3]=r2+r4;
      tmp[4*j+8]=t1-t3;
      tmp[4*j+9]=t2-t4;
      tmp[4*j+10]=r1-r3;
      tmp[4*j+11]=r2-r4;
      tmp[4*j+4]=t5+t6;
      tmp[4*j+5]=t7-t10;
      tmp[4*j+6]=r5+r6;
      tmp[4*j+7]=r7-r10;
      tmp[4*j+12]=t5-t6;
      tmp[4*j+13]=t7+t10;
      tmp[4*j+14]=r5-r6;
      tmp[4*j+15]=r7+r10;
      }
    }
  }
else
  {
  m=timf1_bytemask/sizeof(int);
  p0=timf1p_ref/sizeof(int);
  p0=(p0-fft1_interleave_points*4+m+1)&m;
  mm=timf1_blockbytes/(4*sizeof(int));
  if(genparm[FIRST_FFT_SINPOW] == 0)
    {
    for(j=0; j<n; j+=4)
      {
      ia=(p0+2*fft1_permute[j  ]+chan)&m;
      ib=(p0+2*fft1_permute[j+1]+chan)&m;
      ic=(p0+2*fft1_permute[j+2]+chan)&m;
      id=(p0+2*fft1_permute[j+3]+chan)&m;
      ka=(mm+ia)&m;
      kb=(mm+ib)&m;
      kc=(mm+ic)&m;
      kd=(mm+id)&m;
      la=(2*mm+ia)&m;
      lb=(2*mm+ib)&m;
      lc=(2*mm+ic)&m;
      ld=(2*mm+id)&m;
      ma=(3*mm+ia)&m;
      mb=(3*mm+ib)&m;
      mc=(3*mm+ic)&m;
      md=(3*mm+id)&m;
      t1=(float)(timf1_int[ia])+(float)(timf1_int[ib]);
      t2=(float)(timf1_int[ka])+(float)(timf1_int[kb]);
      r1=(float)(timf1_int[la])+(float)(timf1_int[lb]);
      r2=(float)(timf1_int[ma])+(float)(timf1_int[mb]);
      t3=(float)(timf1_int[ic])+(float)(timf1_int[id]);
      t4=(float)(timf1_int[kc])+(float)(timf1_int[kd]);
      r3=(float)(timf1_int[lc])+(float)(timf1_int[ld]);
      r4=(float)(timf1_int[mc])+(float)(timf1_int[md]);
      t5=(float)(timf1_int[ia])-(float)(timf1_int[ib]);
      t7=(float)(timf1_int[ka])-(float)(timf1_int[kb]);
      r5=(float)(timf1_int[la])-(float)(timf1_int[lb]);
      r7=(float)(timf1_int[ma])-(float)(timf1_int[mb]);
      t10=(float)(timf1_int[ic])-(float)(timf1_int[id]);
      t6= (float)(timf1_int[kc])-(float)(timf1_int[kd]);
      r10=(float)(timf1_int[lc])-(float)(timf1_int[ld]);  
      r6= (float)(timf1_int[mc])-(float)(timf1_int[md]);
      tmp[4*j  ]=t1+t3;
      tmp[4*j+1]=t2+t4;
      tmp[4*j+2]=r1+r3;
      tmp[4*j+3]=r2+r4;
      tmp[4*j+8]=t1-t3;
      tmp[4*j+9]=t2-t4;
      tmp[4*j+10]=r1-r3;
      tmp[4*j+11]=r2-r4;
      t11=t5-t6;
      t8=t7-t10;
      r11=r5-r6;
      r8=r7-r10;
      t12=t5+t6;
      t9=t7+t10;
      r12=r5+r6;
      r9=r7+r10;
      tmp[4*j+4]=t12;
      tmp[4*j+5]=t8;
      tmp[4*j+6]=r12;
      tmp[4*j+7]=r8;
      tmp[4*j+12]=t11;
      tmp[4*j+13]=t9;
      tmp[4*j+14]=r11;
      tmp[4*j+15]=r9;
      }
    }
  else
    {
    for(j=0; j<n; j+=4)
      {
      ja=fft1_permute[j  ];
      jb=fft1_permute[j+1];
      jc=fft1_permute[j+2];
      jd=fft1_permute[j+3];
      ia=(p0+2*ja+chan)&m;
      ib=(p0+2*jb+chan)&m;
      ic=(p0+2*jc+chan)&m;
      id=(p0+2*jd+chan)&m;
      ka=(mm+ia)&m;
      kb=(mm+ib)&m;
      kc=(mm+ic)&m;
      kd=(mm+id)&m;
      la=(2*mm+ia)&m;
      lb=(2*mm+ib)&m;
      lc=(2*mm+ic)&m;
      ld=(2*mm+id)&m;
      ma=(3*mm+ia)&m;
      mb=(3*mm+ib)&m;
      mc=(3*mm+ic)&m;
      md=(3*mm+id)&m;
      a1=fft1_window[ja]*timf1_int[ia];
      a2=fft1_window[ja]*timf1_int[ka];
      a3=fft1_window[ja]*timf1_int[la];
      a4=fft1_window[ja]*timf1_int[ma];
      b1=fft1_window[jb]*timf1_int[ib];
      b2=fft1_window[jb]*timf1_int[kb];
      b3=fft1_window[jb]*timf1_int[lb];
      b4=fft1_window[jb]*timf1_int[mb];
      c1=fft1_window[jc]*timf1_int[ic];
      c2=fft1_window[jc]*timf1_int[kc];
      c3=fft1_window[jc]*timf1_int[lc];
      c4=fft1_window[jc]*timf1_int[mc];
      d1=fft1_window[jd]*timf1_int[id];
      d2=fft1_window[jd]*timf1_int[kd];
      d3=fft1_window[jd]*timf1_int[ld];
      d4=fft1_window[jd]*timf1_int[md];
      t1=a1+b1;
      t2=a2+b2;
      r1=a3+b3;
      r2=a4+b4;
      t3=c1+d1;
      t4=c2+d2;
      r3=c3+d3;
      r4=c4+d4;
      t5=a1-b1;
      t7=a2-b2;
      r5=a3-b3;
      r7=a4-b4;
      t10=c1-d1;
      t6= c2-d2;
      r10=c3-d3;
      r6= c4-d4;
      tmp[4*j  ]=t1+t3;
      tmp[4*j+1]=t2+t4;
      tmp[4*j+2]=r1+r3;
      tmp[4*j+3]=r2+r4;
      tmp[4*j+8]=t1-t3;
      tmp[4*j+9]=t2-t4;
      tmp[4*j+10]=r1-r3;
      tmp[4*j+11]=r2-r4;
      t11=t5-t6;
      t8=t7-t10;
      r11=r5-r6;
      r8=r7-r10;
      t12=t5+t6;
      t9=t7+t10;
      r12=r5+r6;
      r9=r7+r10;
      tmp[4*j+4]=t12;
      tmp[4*j+5]=t8;
      tmp[4*j+6]=r12;
      tmp[4*j+7]=r8;
      tmp[4*j+12]=t11;
      tmp[4*j+13]=t9;
      tmp[4*j+14]=r11;
      tmp[4*j+15]=r9;
      }
    }
  }
}



void fft1win_dif_chan(int chan, int timf1p_ref, float *tmp)
{
int m, nn;
int p0, pa, pb;
int ia,ib;
float x1,x2;
float t1,t2,t3,t4;
nn=fft1_size/2;
if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
  {
  m=timf1_bytemask/sizeof(short int);
  p0=timf1p_ref/sizeof(short int);
  p0=(p0-fft1_interleave_points*4+m+1)&m;
  pa=(p0+2*chan)&m;
  pb=(pa+2*fft1_size)&m;
  ib=fft1_size-2;
  if(genparm[FIRST_FFT_SINPOW] != 0)
    {
    for( ia=0; ia<nn; ia++)
      {
      ib+=2;
      t1=timf1_short_int[pa  ]*fft1_window[2*ia];
      t2=timf1_short_int[pa+1]*fft1_window[2*ia];      
      t3=timf1_short_int[pb  ]*fft1_window[2*ia+1];
      t4=timf1_short_int[pb+1]*fft1_window[2*ia+1];   
      x1=t1-t3;
      tmp[2*ia  ]=  t1+t3;
      x2=t4-t2;
      tmp[2*ia+1]=-(t2+t4);
      pa=(pa+4)&m;
      pb=(pb+4)&m;
      tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
      tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
      }
    }
  else
    {
    for( ia=0; ia<nn; ia++)
      {
      t1=timf1_short_int[pa  ];
      ib+=2;
      t2=timf1_short_int[pa+1];      
      t3=timf1_short_int[pb  ];
      t4=timf1_short_int[pb+1];   
      x1=t1-t3;
      x2=t4-t2;
      tmp[2*ia  ]=  t1+t3;
      tmp[2*ia+1]=-(t2+t4);
      tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
      tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
      pa=(pa+4)&m;
      pb=(pb+4)&m;
      }
    }
  }  
else
  {
  m=timf1_bytemask/sizeof(int);
  p0=timf1p_ref/sizeof(int);
  p0=(p0-fft1_interleave_points*4+m+1)&m;
  pa=(p0+2*chan)&m;
  pb=(pa+2*fft1_size)&m;
  ib=fft1_size-2;
  if(genparm[FIRST_FFT_SINPOW] != 0) 
    {
    for( ia=0; ia<nn; ia++)
      {
      t1=timf1_int[pa  ]*fft1_window[2*ia];
      ib+=2;
      t2=timf1_int[pa+1]*fft1_window[2*ia];      
      t3=timf1_int[pb  ]*fft1_window[2*ia+1];
      t4=timf1_int[pb+1]*fft1_window[2*ia+1];   
      x1=t1-t3;
      tmp[2*ia  ]=  t1+t3;
      x2=t4-t2;
      tmp[2*ia+1]=-(t2+t4);
      pa=(pa+4)&m;
      pb=(pb+4)&m;
      tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
      tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
      }
    }
  else
    {
    for( ia=0; ia<nn; ia++)
      {
      t1=timf1_int[pa  ];
      ib+=2;
      t2=timf1_int[pa+1];      
      t3=timf1_int[pb  ];
      t4=timf1_int[pb+1];   
      tmp[2*ia  ]=  t1+t3;
      x1=t1-t3;
      tmp[2*ia+1]=-(t2+t4);
      x2=t4-t2;
      pa=(pa+4)&m;
      pb=(pb+4)&m;
      tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
      tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
      }
    }
  }
}

void d_fft1win_dif_chan(int chan, int timf1p_ref, double *tmp)
{
int m, nn;
int p0, pa, pb;
int ia,ib;
double x1,x2;
double t1,t2,t3,t4;
nn=fft1_size/2;
if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
  {
  m=timf1_bytemask/sizeof(short int);
  p0=timf1p_ref/sizeof(short int);
  p0=(p0-fft1_interleave_points*4+m+1)&m;
  pa=(p0+2*chan)&m;
  pb=(pa+2*fft1_size)&m;
  ib=fft1_size-2;
  if(genparm[FIRST_FFT_SINPOW] != 0)
    {
    for( ia=0; ia<nn; ia++)
      {
      ib+=2;
      t1=timf1_short_int[pa  ]*d_fft1_window[2*ia];
      t2=timf1_short_int[pa+1]*d_fft1_window[2*ia];      
      t3=timf1_short_int[pb  ]*d_fft1_window[2*ia+1];
      t4=timf1_short_int[pb+1]*d_fft1_window[2*ia+1];   
      x1=t1-t3;
      tmp[2*ia  ]=  t1+t3;
      x2=t4-t2;
      tmp[2*ia+1]=-(t2+t4);
      pa=(pa+4)&m;
      pb=(pb+4)&m;
      tmp[ib  ]=d_fft1tab[ia].cos*x1-d_fft1tab[ia].sin*x2;
      tmp[ib+1]=d_fft1tab[ia].sin*x1+d_fft1tab[ia].cos*x2;
      }
    }
  else
    {
    for( ia=0; ia<nn; ia++)
      {
      t1=timf1_short_int[pa  ];
      ib+=2;
      t2=timf1_short_int[pa+1];      
      t3=timf1_short_int[pb  ];
      t4=timf1_short_int[pb+1];   
      x1=t1-t3;
      x2=t4-t2;
      tmp[2*ia  ]=  t1+t3;
      tmp[2*ia+1]=-(t2+t4);
      tmp[ib  ]=d_fft1tab[ia].cos*x1-d_fft1tab[ia].sin*x2;
      tmp[ib+1]=d_fft1tab[ia].sin*x1+d_fft1tab[ia].cos*x2;
      pa=(pa+4)&m;
      pb=(pb+4)&m;
      }
    }
  }  
else
  {
  m=timf1_bytemask/sizeof(int);
  p0=timf1p_ref/sizeof(int);
  p0=(p0-fft1_interleave_points*4+m+1)&m;
  pa=(p0+2*chan)&m;
  pb=(pa+2*fft1_size)&m;
  ib=fft1_size-2;
  if(genparm[FIRST_FFT_SINPOW] != 0) 
    {
    for( ia=0; ia<nn; ia++)
      {
      t1=timf1_int[pa  ]*d_fft1_window[2*ia];
      ib+=2;
      t2=timf1_int[pa+1]*d_fft1_window[2*ia];      
      t3=timf1_int[pb  ]*d_fft1_window[2*ia+1];
      t4=timf1_int[pb+1]*d_fft1_window[2*ia+1];   
      x1=t1-t3;
      tmp[2*ia  ]=  t1+t3;
      x2=t4-t2;
      tmp[2*ia+1]=-(t2+t4);
      pa=(pa+4)&m;
      pb=(pb+4)&m;
      tmp[ib  ]=d_fft1tab[ia].cos*x1-d_fft1tab[ia].sin*x2;
      tmp[ib+1]=d_fft1tab[ia].sin*x1+d_fft1tab[ia].cos*x2;
      }
    }
  else
    {
    for( ia=0; ia<nn; ia++)
      {
      t1=timf1_int[pa  ];
      ib+=2;
      t2=timf1_int[pa+1];      
      t3=timf1_int[pb  ];
      t4=timf1_int[pb+1];   
      tmp[2*ia  ]=  t1+t3;
      x1=t1-t3;
      tmp[2*ia+1]=-(t2+t4);
      x2=t4-t2;
      pa=(pa+4)&m;
      pb=(pb+4)&m;
      tmp[ib  ]=d_fft1tab[ia].cos*x1-d_fft1tab[ia].sin*x2;
      tmp[ib+1]=d_fft1tab[ia].sin*x1+d_fft1tab[ia].cos*x2;
      }
    }
  }
}

void fft1win_dif_two(int timf1p_ref, float *tmp)
{
int m, p0, pa, pb, ia, ib, nn;
float t1, t2, t3, t4, x1, x2;
nn=fft1_size/2; 

  if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    m=timf1_bytemask/sizeof(short int);
    p0=timf1p_ref/sizeof(short int);
    p0=(p0-fft1_interleave_points*4+m+1)&m;
    pa=p0;
    pb=(pa+2*fft1_size)&m;
    ib=2*fft1_size-4;
    if(genparm[FIRST_FFT_SINPOW] != 0) 
      {
      for( ia=0; ia<nn; ia++)
        {
        t1=timf1_short_int[pa  ]*fft1_window[2*ia];
        t2=timf1_short_int[pa+1]*fft1_window[2*ia];      
        t3=timf1_short_int[pb  ]*fft1_window[2*ia+1];
        t4=timf1_short_int[pb+1]*fft1_window[2*ia+1];   
        ib+=4;
        x1=t1-t3;
        tmp[4*ia  ]=  t1+t3;
        x2=t4-t2;
        tmp[4*ia+1]=-(t2+t4);
        tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        t1=timf1_short_int[pa+2]*fft1_window[2*ia];
        t2=timf1_short_int[pa+3]*fft1_window[2*ia];      
        t3=timf1_short_int[pb+2]*fft1_window[2*ia+1];
        t4=timf1_short_int[pb+3]*fft1_window[2*ia+1];   
        x1=t1-t3;
        tmp[4*ia+2]=  t1+t3;
        x2=t4-t2;
        tmp[4*ia+3]=-(t2+t4);
        pa=(pa+4)&m;
        pb=(pb+4)&m;
        tmp[ib+2]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        tmp[ib+3]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        }
      }
    else
      {
      for( ia=0; ia<nn; ia++)
        {
        t1=timf1_short_int[pa  ];
        t2=timf1_short_int[pa+1];      
        t3=timf1_short_int[pb  ];
        t4=timf1_short_int[pb+1];   
        ib+=4;
        x1=t1-t3;
        tmp[4*ia  ]=  t1+t3;
        x2=t4-t2;
        tmp[4*ia+1]=-(t2+t4);
        tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        t1=timf1_short_int[pa+2];
        t2=timf1_short_int[pa+3];      
        t3=timf1_short_int[pb+2];
        t4=timf1_short_int[pb+3];   
        x1=t1-t3;
        tmp[4*ia+2]=  t1+t3;
        x2=t4-t2;
        tmp[4*ia+3]=-(t2+t4);
        pa=(pa+4)&m;
        pb=(pb+4)&m;
        tmp[ib+2]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        tmp[ib+3]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        }
      }
    }  
  else
    {
    m=timf1_bytemask/sizeof(int);
    p0=timf1p_ref/sizeof(int);
    p0=(p0-fft1_interleave_points*4+m+1)&m;
    pa=p0;
    pb=(pa+2*fft1_size)&m;
    ib=2*fft1_size-4;
    if(genparm[FIRST_FFT_SINPOW] != 0) 
      {
      for( ia=0; ia<nn; ia++)
        {
        t1=timf1_int[pa  ]*fft1_window[2*ia];
        t2=timf1_int[pa+1]*fft1_window[2*ia];      
        t3=timf1_int[pb  ]*fft1_window[2*ia+1];
        t4=timf1_int[pb+1]*fft1_window[2*ia+1];   
        ib+=4;
        x1=t1-t3;
        tmp[4*ia  ]=  t1+t3;
        x2=t4-t2;
        tmp[4*ia+1]=-(t2+t4);
        tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        t1=timf1_int[pa+2]*fft1_window[2*ia];
        t2=timf1_int[pa+3]*fft1_window[2*ia];      
        t3=timf1_int[pb+2]*fft1_window[2*ia+1];
        t4=timf1_int[pb+3]*fft1_window[2*ia+1];   
        x1=t1-t3;
        tmp[4*ia+2]=  t1+t3;
        x2=t4-t2;
        tmp[4*ia+3]=-(t2+t4);
        pa=(pa+4)&m;
        pb=(pb+4)&m;
        tmp[ib+2]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        tmp[ib+3]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        }
      }
    else
      {
      for( ia=0; ia<nn; ia++)
        {
        t1=timf1_int[pa  ];
        t2=timf1_int[pa+1];      
        t3=timf1_int[pb  ];
        t4=timf1_int[pb+1];   
        ib+=4;
        x1=t1-t3;
        tmp[4*ia  ]=  t1+t3;
        x2=t4-t2;
        tmp[4*ia+1]=-(t2+t4);
        tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        t1=timf1_int[pa+2];
        t2=timf1_int[pa+3];      
        t3=timf1_int[pb+2];
        t4=timf1_int[pb+3];   
        x1=t1-t3;
        tmp[4*ia+2]=  t1+t3;
        x2=t4-t2;
        tmp[4*ia+3]=-(t2+t4);
        pa=(pa+4)&m;
        pb=(pb+4)&m;
        tmp[ib+2]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        tmp[ib+3]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        }
      }
    }
}

void dif_permute_two(float *out, float * tmp)
{
int n, ia, ib, ic;
n=fft1_size;
for(ia=0; ia < n; ia+=2)
  {
  ib=4*fft1_permute[ia];                               
  ic=4*fft1_permute[ia+1];                             
  out[ib  ]=tmp[4*ia  ]+tmp[4*ia+4];
  out[ic  ]=tmp[4*ia  ]-tmp[4*ia+4];
  out[ib+1]=tmp[4*ia+1]+tmp[4*ia+5];
  out[ic+1]=tmp[4*ia+1]-tmp[4*ia+5];
  out[ib+2]=tmp[4*ia+2]+tmp[4*ia+6];
  out[ic+2]=tmp[4*ia+2]-tmp[4*ia+6];
  out[ib+3]=tmp[4*ia+3]+tmp[4*ia+7];
  out[ic+3]=tmp[4*ia+3]-tmp[4*ia+7];
  }
}

void fft1win_dit_chan(int chan, int timf1p_ref, float *tmp)
{
int j, m, n;
int p0;
int ia,ib,ic,id;
int ja,jb,jc,jd;
float t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12;
float a1,a2;
float b1,b2;
float c1,c2;
float d1,d2;
n=fft1_size;
    if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
      {
      m=timf1_bytemask/sizeof(short int);
      p0=timf1p_ref/sizeof(short int);
      p0=(p0-fft1_interleave_points*4+m+1+2*chan)&m;
      if(genparm[FIRST_FFT_SINPOW] == 0)
        {
        for(j=0; j<n; j+=4)
          {
          ia=(p0+4*fft1_permute[j  ])&m;
          ib=(p0+4*fft1_permute[j+1])&m;
          ic=(p0+4*fft1_permute[j+2])&m;
          id=(p0+4*fft1_permute[j+3])&m;
      
          t1=(float)(timf1_short_int[ia  ])+(float)(timf1_short_int[ib  ]);
          t2=(float)(timf1_short_int[ia+1])+(float)(timf1_short_int[ib+1]);
      
          t3=(float)(timf1_short_int[ic  ])+(float)(timf1_short_int[id  ]);
          t4=(float)(timf1_short_int[ic+1])+(float)(timf1_short_int[id+1]);
      
          t5=(float)(timf1_short_int[ia  ])-(float)(timf1_short_int[ib  ]);
          t7=(float)(timf1_short_int[ia+1])-(float)(timf1_short_int[ib+1]);
      
          t10=(float)(timf1_short_int[ic  ])-(float)(timf1_short_int[id  ]);
          t6= (float)(timf1_short_int[ic+1])-(float)(timf1_short_int[id+1]);
      
          tmp[2*j  ]=t1+t3;
          tmp[2*j+1]=t2+t4;
      
          tmp[2*j+4]=t1-t3;
          tmp[2*j+5]=t2-t4;
      
          t11=t5-t6;
          t8=t7-t10;
        
          t12=t5+t6;
          t9=t7+t10;
      
          tmp[2*j+2]=t12;
          tmp[2*j+3]=t8;
     
          tmp[2*j+6]=t11;
          tmp[2*j+7]=t9;
          }
        }
      else 
        {
        for(j=0; j<n; j+=4)
          {
          ja=fft1_permute[j  ];
          jb=fft1_permute[j+1];
          jc=fft1_permute[j+2];
          jd=fft1_permute[j+3];
  
          ia=(p0+4*ja)&m;
          ib=(p0+4*jb)&m;
          ic=(p0+4*jc)&m;
          id=(p0+4*jd)&m;
  
          a1=fft1_window[ja]*timf1_short_int[ia  ];
          a2=fft1_window[ja]*timf1_short_int[ia+1];
   
          b1=fft1_window[jb]*timf1_short_int[ib  ];
          b2=fft1_window[jb]*timf1_short_int[ib+1];
    
          c1=fft1_window[jc]*timf1_short_int[ic  ];
          c2=fft1_window[jc]*timf1_short_int[ic+1];
    
          d1=fft1_window[jd]*timf1_short_int[id  ];
          d2=fft1_window[jd]*timf1_short_int[id+1];
  
          t1=a1+b1;
          t2=a2+b2;
    
          t3=c1+d1;
          t4=c2+d2;
    
          t5=a1-b1;
          t7=a2-b2;
      
          t10=c1-d1;
          t6= c2-d2;
   
          tmp[2*j  ]=t1+t3;
          tmp[2*j+1]=t2+t4;
     
          tmp[2*j+4]=t1-t3;
          tmp[2*j+5]=t2-t4;
    
          t11=t5-t6;
          t8=t7-t10;
       
          t12=t5+t6;
          t9=t7+t10;
     
          tmp[2*j+2]=t12;
          tmp[2*j+3]=t8;
      
          tmp[2*j+6]=t11;
          tmp[2*j+7]=t9;
          }
        }
      }
    else
      {  
      m=timf1_bytemask/sizeof(int);
      p0=timf1p_ref/sizeof(int);
      p0=(p0-fft1_interleave_points*4+m+1+2*chan)&m;
      if(genparm[FIRST_FFT_SINPOW] == 0)
        {
        for(j=0; j<n; j+=4)
          {
          ia=(p0+4*fft1_permute[j  ])&m;
          ib=(p0+4*fft1_permute[j+1])&m;
          ic=(p0+4*fft1_permute[j+2])&m;
          id=(p0+4*fft1_permute[j+3])&m;
      
          t1=(float)(timf1_int[ia  ])+(float)(timf1_int[ib  ]);
          t2=(float)(timf1_int[ia+1])+(float)(timf1_int[ib+1]);
      
          t3=(float)(timf1_int[ic  ])+(float)(timf1_int[id  ]);
          t4=(float)(timf1_int[ic+1])+(float)(timf1_int[id+1]);
      
          t5=(float)(timf1_int[ia  ])-(float)(timf1_int[ib  ]);
          t7=(float)(timf1_int[ia+1])-(float)(timf1_int[ib+1]);
      
          t10=(float)(timf1_int[ic  ])-(float)(timf1_int[id  ]);
          t6= (float)(timf1_int[ic+1])-(float)(timf1_int[id+1]);
      
          tmp[2*j  ]=t1+t3;
          tmp[2*j+1]=t2+t4;
      
          tmp[2*j+4]=t1-t3;
          tmp[2*j+5]=t2-t4;
      
          t11=t5-t6;
          t8=t7-t10;
        
          t12=t5+t6;
          t9=t7+t10;
      
          tmp[2*j+2]=t12;
          tmp[2*j+3]=t8;
     
          tmp[2*j+6]=t11;
          tmp[2*j+7]=t9;
          }
        }
      else 
        {
        for(j=0; j<n; j+=4)
          {
          ja=fft1_permute[j  ];
          jb=fft1_permute[j+1];
          jc=fft1_permute[j+2];
          jd=fft1_permute[j+3];
  
          ia=(p0+4*ja)&m;
          ib=(p0+4*jb)&m;
          ic=(p0+4*jc)&m;
          id=(p0+4*jd)&m;
  
          a1=fft1_window[ja]*timf1_int[ia  ];
          a2=fft1_window[ja]*timf1_int[ia+1];
   
          b1=fft1_window[jb]*timf1_int[ib  ];
          b2=fft1_window[jb]*timf1_int[ib+1];
    
          c1=fft1_window[jc]*timf1_int[ic  ];
          c2=fft1_window[jc]*timf1_int[ic+1];
    
          d1=fft1_window[jd]*timf1_int[id  ];
          d2=fft1_window[jd]*timf1_int[id+1];

          t1=a1+b1;
          t2=a2+b2;
    
          t3=c1+d1;
          t4=c2+d2;
    
          t5=a1-b1;
          t7=a2-b2;
      
          t10=c1-d1;
          t6= c2-d2;
   
          tmp[2*j  ]=t1+t3;
          tmp[2*j+1]=t2+t4;
     
          tmp[2*j+4]=t1-t3;
          tmp[2*j+5]=t2-t4;
    
          t11=t5-t6;
          t8=t7-t10;
       
          t12=t5+t6;
          t9=t7+t10;
     
          tmp[2*j+2]=t12;
          tmp[2*j+3]=t8;
      
          tmp[2*j+6]=t11;
          tmp[2*j+7]=t9;
          }
        }
      }
}

void dit_finish_chan (int chan, float *out, float *tmp)
{
int i, k, m;
k=2*chan;
m=2*fft1_size;
for(i=0; i<m; i+=2)
  {
  out[k  ]=tmp[i];
  out[k+1]=tmp[i+1];  
  k+=4;
  }
}

void fft1win_dit_two(int timf1p_ref, float *out)
{
int j, m, n;
int p0;
int ia,ib,ic,id;
int ja,jb,jc,jd;
float t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12;
float r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12;
float a1,a2,a3,a4;
float b1,b2,b3,b4;
float c1,c2,c3,c4;
float d1,d2,d3,d4;
n=fft1_size;
if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
  {
  m=timf1_bytemask/sizeof(short int);
  p0=timf1p_ref/sizeof(short int);
  p0=(p0-fft1_interleave_points*4+m+1)&m;
  ib=2*fft1_size-4;
  if(genparm[FIRST_FFT_SINPOW] == 0)
    {
    for(j=0; j<n; j+=4)
      {
      ia=(p0+4*fft1_permute[j  ])&m;
      ib=(p0+4*fft1_permute[j+1])&m;
      ic=(p0+4*fft1_permute[j+2])&m;
      id=(p0+4*fft1_permute[j+3])&m;
  
      t1=(float)(timf1_short_int[ia  ])+(float)(timf1_short_int[ib  ]);
      t2=(float)(timf1_short_int[ia+1])+(float)(timf1_short_int[ib+1]);
      r1=(float)(timf1_short_int[ia+2])+(float)(timf1_short_int[ib+2]);
      r2=(float)(timf1_short_int[ia+3])+(float)(timf1_short_int[ib+3]);
  
      t3=(float)(timf1_short_int[ic  ])+(float)(timf1_short_int[id  ]);
      t4=(float)(timf1_short_int[ic+1])+(float)(timf1_short_int[id+1]);
      r3=(float)(timf1_short_int[ic+2])+(float)(timf1_short_int[id+2]);
      r4=(float)(timf1_short_int[ic+3])+(float)(timf1_short_int[id+3]);
  
      t5=(float)(timf1_short_int[ia  ])-(float)(timf1_short_int[ib  ]);
      t7=(float)(timf1_short_int[ia+1])-(float)(timf1_short_int[ib+1]);
      r5=(float)(timf1_short_int[ia+2])-(float)(timf1_short_int[ib+2]);
      r7=(float)(timf1_short_int[ia+3])-(float)(timf1_short_int[ib+3]);
  
      t10=(float)(timf1_short_int[ic  ])-(float)(timf1_short_int[id  ]);
      t6= (float)(timf1_short_int[ic+1])-(float)(timf1_short_int[id+1]);
      r10=(float)(timf1_short_int[ic+2])-(float)(timf1_short_int[id+2]);  
      r6= (float)(timf1_short_int[ic+3])-(float)(timf1_short_int[id+3]);
  
      out[4*j  ]=t1+t3;
      out[4*j+1]=t2+t4;
      out[4*j+2]=r1+r3;
      out[4*j+3]=r2+r4;
  
      out[4*j+8]=t1-t3;
      out[4*j+9]=t2-t4;
      out[4*j+10]=r1-r3;
      out[4*j+11]=r2-r4;
  
      t11=t5-t6;
      t8=t7-t10;
      r11=r5-r6;
      r8=r7-r10;
    
      t12=t5+t6;
      t9=t7+t10;
      r12=r5+r6;
      r9=r7+r10;
  
      out[4*j+4]=t12;
      out[4*j+5]=t8;
      out[4*j+6]=r12;
      out[4*j+7]=r8;
  
      out[4*j+12]=t11;
      out[4*j+13]=t9;
      out[4*j+14]=r11;
      out[4*j+15]=r9;
      }
    }
  else
    {
    for(j=0; j<n; j+=4)
      {
      ja=fft1_permute[j  ];
      jb=fft1_permute[j+1];
      jc=fft1_permute[j+2];
      jd=fft1_permute[j+3];

      ia=(p0+4*ja)&m;
      ib=(p0+4*jb)&m;
      ic=(p0+4*jc)&m;
      id=(p0+4*jd)&m;

      a1=fft1_window[ja]*timf1_short_int[ia  ];
      a2=fft1_window[ja]*timf1_short_int[ia+1];
      a3=fft1_window[ja]*timf1_short_int[ia+2];
      a4=fft1_window[ja]*timf1_short_int[ia+3];

      b1=fft1_window[jb]*timf1_short_int[ib  ];
      b2=fft1_window[jb]*timf1_short_int[ib+1];
      b3=fft1_window[jb]*timf1_short_int[ib+2];
      b4=fft1_window[jb]*timf1_short_int[ib+3];

      c1=fft1_window[jc]*timf1_short_int[ic  ];
      c2=fft1_window[jc]*timf1_short_int[ic+1];
      c3=fft1_window[jc]*timf1_short_int[ic+2];
      c4=fft1_window[jc]*timf1_short_int[ic+3];

      d1=fft1_window[jd]*timf1_short_int[id  ];
      d2=fft1_window[jd]*timf1_short_int[id+1];
      d3=fft1_window[jd]*timf1_short_int[id+2];
      d4=fft1_window[jd]*timf1_short_int[id+3];

      t1=a1+b1;
      t2=a2+b2;
      r1=a3+b3;
      r2=a4+b4;
  
      t3=c1+d1;
      t4=c2+d2;
      r3=c3+d3;
      r4=c4+d4;
  
      t5=a1-b1;
      t7=a2-b2;
      r5=a3-b3;
      r7=a4-b4;

      t10=c1-d1;
      t6= c2-d2;
      r10=c3-d3;
      r6= c4-d4;

      out[4*j  ]=t1+t3;
      out[4*j+1]=t2+t4;
      out[4*j+2]=r1+r3;
      out[4*j+3]=r2+r4;

      out[4*j+8]=t1-t3;
      out[4*j+9]=t2-t4;
      out[4*j+10]=r1-r3;
      out[4*j+11]=r2-r4;

      t11=t5-t6;
      t8=t7-t10;
      r11=r5-r6;
      r8=r7-r10;

      t12=t5+t6;
      t9=t7+t10;
      r12=r5+r6;
      r9=r7+r10;

      out[4*j+4]=t12;
      out[4*j+5]=t8;
      out[4*j+6]=r12;
      out[4*j+7]=r8;

      out[4*j+12]=t11;
      out[4*j+13]=t9;
      out[4*j+14]=r11;
      out[4*j+15]=r9;
      }
    }
  }
else
  {
  m=timf1_bytemask/sizeof(int);
  p0=timf1p_ref/sizeof(int);
  p0=(p0-fft1_interleave_points*4+m+1)&m;
  if(genparm[FIRST_FFT_SINPOW] == 0)
    {
    for(j=0; j<n; j+=4)
      {
      ia=(p0+4*fft1_permute[j  ])&m;
      ib=(p0+4*fft1_permute[j+1])&m;
      ic=(p0+4*fft1_permute[j+2])&m;
      id=(p0+4*fft1_permute[j+3])&m;

      t1=(float)(timf1_int[ia  ])+(float)(timf1_int[ib  ]);
      t2=(float)(timf1_int[ia+1])+(float)(timf1_int[ib+1]);
      r1=(float)(timf1_int[ia+2])+(float)(timf1_int[ib+2]);
      r2=(float)(timf1_int[ia+3])+(float)(timf1_int[ib+3]);

      t3=(float)(timf1_int[ic  ])+(float)(timf1_int[id  ]);
      t4=(float)(timf1_int[ic+1])+(float)(timf1_int[id+1]);
      r3=(float)(timf1_int[ic+2])+(float)(timf1_int[id+2]);
      r4=(float)(timf1_int[ic+3])+(float)(timf1_int[id+3]);

      t5=(float)(timf1_int[ia  ])-(float)(timf1_int[ib  ]);
      t7=(float)(timf1_int[ia+1])-(float)(timf1_int[ib+1]);
      r5=(float)(timf1_int[ia+2])-(float)(timf1_int[ib+2]);
      r7=(float)(timf1_int[ia+3])-(float)(timf1_int[ib+3]);

      t10=(float)(timf1_int[ic  ])-(float)(timf1_int[id  ]);
      t6= (float)(timf1_int[ic+1])-(float)(timf1_int[id+1]);
      r10=(float)(timf1_int[ic+2])-(float)(timf1_int[id+2]);  
      r6= (float)(timf1_int[ic+3])-(float)(timf1_int[id+3]);

      out[4*j  ]=t1+t3;
      out[4*j+1]=t2+t4;
      out[4*j+2]=r1+r3;
      out[4*j+3]=r2+r4;

      out[4*j+8]=t1-t3;
      out[4*j+9]=t2-t4;
      out[4*j+10]=r1-r3;
      out[4*j+11]=r2-r4;

      t11=t5-t6;
      t8=t7-t10;
      r11=r5-r6;
      r8=r7-r10;

      t12=t5+t6;
      t9=t7+t10;
      r12=r5+r6;
      r9=r7+r10;

      out[4*j+4]=t12;
      out[4*j+5]=t8;
      out[4*j+6]=r12;
      out[4*j+7]=r8;

      out[4*j+12]=t11;
      out[4*j+13]=t9;
      out[4*j+14]=r11;
      out[4*j+15]=r9;
      }
    }
  else
    {
    for(j=0; j<n; j+=4)
      {
      ja=fft1_permute[j  ];
      jb=fft1_permute[j+1];
      jc=fft1_permute[j+2];
      jd=fft1_permute[j+3];

      ia=(p0+4*ja)&m;
      ib=(p0+4*jb)&m;
      ic=(p0+4*jc)&m;
      id=(p0+4*jd)&m;

      a1=fft1_window[ja]*timf1_int[ia  ];
      a2=fft1_window[ja]*timf1_int[ia+1];
      a3=fft1_window[ja]*timf1_int[ia+2];
      a4=fft1_window[ja]*timf1_int[ia+3];

      b1=fft1_window[jb]*timf1_int[ib  ];
      b2=fft1_window[jb]*timf1_int[ib+1];
      b3=fft1_window[jb]*timf1_int[ib+2];
      b4=fft1_window[jb]*timf1_int[ib+3];

      c1=fft1_window[jc]*timf1_int[ic  ];
      c2=fft1_window[jc]*timf1_int[ic+1];
      c3=fft1_window[jc]*timf1_int[ic+2];
      c4=fft1_window[jc]*timf1_int[ic+3];

      d1=fft1_window[jd]*timf1_int[id  ];
      d2=fft1_window[jd]*timf1_int[id+1];
      d3=fft1_window[jd]*timf1_int[id+2];
      d4=fft1_window[jd]*timf1_int[id+3];

      t1=a1+b1;
      t2=a2+b2;
      r1=a3+b3;
      r2=a4+b4;

      t3=c1+d1;
      t4=c2+d2;
      r3=c3+d3;
      r4=c4+d4;

      t5=a1-b1;
      t7=a2-b2;
      r5=a3-b3;
      r7=a4-b4;

      t10=c1-d1;
      t6= c2-d2;
      r10=c3-d3;
      r6= c4-d4;

      out[4*j  ]=t1+t3;
      out[4*j+1]=t2+t4;
      out[4*j+2]=r1+r3;
      out[4*j+3]=r2+r4;

      out[4*j+8]=t1-t3;
      out[4*j+9]=t2-t4;
      out[4*j+10]=r1-r3;
      out[4*j+11]=r2-r4;

      t11=t5-t6;
      t8=t7-t10;
      r11=r5-r6;
      r8=r7-r10;

      t12=t5+t6;
      t9=t7+t10;
      r12=r5+r6;
      r9=r7+r10;

      out[4*j+4]=t12;
      out[4*j+5]=t8;
      out[4*j+6]=r12;
      out[4*j+7]=r8;

      out[4*j+12]=t11;
      out[4*j+13]=t9;
      out[4*j+14]=r11;
      out[4*j+15]=r9;
      }
    }
  }
}

void fft1win_dit_one_dual(int timf1p_ref, float *out)
{
int j, m, n, nn;
int p0;
int ia,ib,ic,id;
int ka,kb,kc,kd;
int ja,jb,jc,jd;
float t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12;
float r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12;
float a1,a2,a3,a4;
float b1,b2,b3,b4;
float c1,c2,c3,c4;
float d1,d2,d3,d4;
n=fft1_size;
if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
  {
  m=timf1_bytemask/sizeof(short int);
  p0=timf1p_ref/sizeof(short int);
  nn=timf1_blockbytes/(2*sizeof(short int));
  p0=(p0-fft1_interleave_points*4+m+1)&m;
  ib=2*fft1_size-4;
  if(genparm[FIRST_FFT_SINPOW] == 0)
    {
    for(j=0; j<n; j+=4)
      {
      ia=(p0+2*fft1_permute[j  ])&m;
      ib=(p0+2*fft1_permute[j+1])&m;
      ic=(p0+2*fft1_permute[j+2])&m;
      id=(p0+2*fft1_permute[j+3])&m;
  
      ja=(nn+p0+2*fft1_permute[j  ])&m;
      jb=(nn+p0+2*fft1_permute[j+1])&m;
      jc=(nn+p0+2*fft1_permute[j+2])&m;
      jd=(nn+p0+2*fft1_permute[j+3])&m;
  
      t1=(float)(timf1_short_int[ia  ])+(float)(timf1_short_int[ib  ]);
      t2=(float)(timf1_short_int[ia+1])+(float)(timf1_short_int[ib+1]);
      r1=(float)(timf1_short_int[ja  ])+(float)(timf1_short_int[jb  ]);
      r2=(float)(timf1_short_int[ja+1])+(float)(timf1_short_int[jb+1]);
  
      t3=(float)(timf1_short_int[ic  ])+(float)(timf1_short_int[id  ]);
      t4=(float)(timf1_short_int[ic+1])+(float)(timf1_short_int[id+1]);
      r3=(float)(timf1_short_int[jc  ])+(float)(timf1_short_int[jd  ]);
      r4=(float)(timf1_short_int[jc+1])+(float)(timf1_short_int[jd+1]);
  
      t5=(float)(timf1_short_int[ia  ])-(float)(timf1_short_int[ib  ]);
      t7=(float)(timf1_short_int[ia+1])-(float)(timf1_short_int[ib+1]);
      r5=(float)(timf1_short_int[ja  ])-(float)(timf1_short_int[jb  ]);
      r7=(float)(timf1_short_int[ja+1])-(float)(timf1_short_int[jb+1]);
  
      t10=(float)(timf1_short_int[ic  ])-(float)(timf1_short_int[id  ]);
      t6= (float)(timf1_short_int[ic+1])-(float)(timf1_short_int[id+1]);
      r10=(float)(timf1_short_int[jc  ])-(float)(timf1_short_int[jd  ]);  
      r6= (float)(timf1_short_int[jc+1])-(float)(timf1_short_int[jd+1]);
  
      out[4*j  ]=t1+t3;
      out[4*j+1]=t2+t4;
      out[4*j+2]=r1+r3;
      out[4*j+3]=r2+r4;
  
      out[4*j+8]=t1-t3;
      out[4*j+9]=t2-t4;
      out[4*j+10]=r1-r3;
      out[4*j+11]=r2-r4;
  
      t11=t5-t6;
      t8=t7-t10;
      r11=r5-r6;
      r8=r7-r10;
    
      t12=t5+t6;
      t9=t7+t10;
      r12=r5+r6;
      r9=r7+r10;
  
      out[4*j+4]=t12;
      out[4*j+5]=t8;
      out[4*j+6]=r12;
      out[4*j+7]=r8;
  
      out[4*j+12]=t11;
      out[4*j+13]=t9;
      out[4*j+14]=r11;
      out[4*j+15]=r9;
      }
    }
  else
    {
    for(j=0; j<n; j+=4)
      {
      ja=fft1_permute[j  ];
      jb=fft1_permute[j+1];
      jc=fft1_permute[j+2];
      jd=fft1_permute[j+3];

      ia=(p0+2*ja)&m;
      ib=(p0+2*jb)&m;
      ic=(p0+2*jc)&m;
      id=(p0+2*jd)&m;

      ka=(nn+p0+2*ja)&m;
      kb=(nn+p0+2*jb)&m;
      kc=(nn+p0+2*jc)&m;
      kd=(nn+p0+2*jd)&m;

      a1=fft1_window[ja]*timf1_short_int[ia  ];
      a2=fft1_window[ja]*timf1_short_int[ia+1];
      a3=fft1_window[ja]*timf1_short_int[ka  ];
      a4=fft1_window[ja]*timf1_short_int[ka+1];

      b1=fft1_window[jb]*timf1_short_int[ib  ];
      b2=fft1_window[jb]*timf1_short_int[ib+1];
      b3=fft1_window[jb]*timf1_short_int[kb  ];
      b4=fft1_window[jb]*timf1_short_int[kb+1];

      c1=fft1_window[jc]*timf1_short_int[ic  ];
      c2=fft1_window[jc]*timf1_short_int[ic+1];
      c3=fft1_window[jc]*timf1_short_int[kc  ];
      c4=fft1_window[jc]*timf1_short_int[kc+1];

      d1=fft1_window[jd]*timf1_short_int[id  ];
      d2=fft1_window[jd]*timf1_short_int[id+1];
      d3=fft1_window[jd]*timf1_short_int[kd  ];
      d4=fft1_window[jd]*timf1_short_int[kd+1];

      t1=a1+b1;
      t2=a2+b2;
      r1=a3+b3;
      r2=a4+b4;
  
      t3=c1+d1;
      t4=c2+d2;
      r3=c3+d3;
      r4=c4+d4;
  
      t5=a1-b1;
      t7=a2-b2;
      r5=a3-b3;
      r7=a4-b4;

      t10=c1-d1;
      t6= c2-d2;
      r10=c3-d3;
      r6= c4-d4;

      out[4*j  ]=t1+t3;
      out[4*j+1]=t2+t4;
      out[4*j+2]=r1+r3;
      out[4*j+3]=r2+r4;

      out[4*j+8]=t1-t3;
      out[4*j+9]=t2-t4;
      out[4*j+10]=r1-r3;
      out[4*j+11]=r2-r4;

      t11=t5-t6;
      t8=t7-t10;
      r11=r5-r6;
      r8=r7-r10;

      t12=t5+t6;
      t9=t7+t10;
      r12=r5+r6;
      r9=r7+r10;

      out[4*j+4]=t12;
      out[4*j+5]=t8;
      out[4*j+6]=r12;
      out[4*j+7]=r8;

      out[4*j+12]=t11;
      out[4*j+13]=t9;
      out[4*j+14]=r11;
      out[4*j+15]=r9;
      }
    }
  }
else
  {
  m=timf1_bytemask/sizeof(int);
  p0=timf1p_ref/sizeof(int);
  nn=timf1_blockbytes/(2*sizeof(int));
  p0=(p0-fft1_interleave_points*4+m+1)&m;
  if(genparm[FIRST_FFT_SINPOW] == 0)
    {
    for(j=0; j<n; j+=4)
      {
      ia=(p0+2*fft1_permute[j  ])&m;
      ib=(p0+2*fft1_permute[j+1])&m;
      ic=(p0+2*fft1_permute[j+2])&m;
      id=(p0+2*fft1_permute[j+3])&m;
  
      ja=(nn+p0+2*fft1_permute[j  ])&m;
      jb=(nn+p0+2*fft1_permute[j+1])&m;
      jc=(nn+p0+2*fft1_permute[j+2])&m;
      jd=(nn+p0+2*fft1_permute[j+3])&m;

      t1=(float)(timf1_int[ia  ])+(float)(timf1_int[ib  ]);
      t2=(float)(timf1_int[ia+1])+(float)(timf1_int[ib+1]);
      r1=(float)(timf1_int[ja  ])+(float)(timf1_int[jb  ]);
      r2=(float)(timf1_int[ja+1])+(float)(timf1_int[jb+1]);

      t3=(float)(timf1_int[ic  ])+(float)(timf1_int[id  ]);
      t4=(float)(timf1_int[ic+1])+(float)(timf1_int[id+1]);
      r3=(float)(timf1_int[jc  ])+(float)(timf1_int[jd  ]);
      r4=(float)(timf1_int[jc+1])+(float)(timf1_int[jd+1]);

      t5=(float)(timf1_int[ia  ])-(float)(timf1_int[ib  ]);
      t7=(float)(timf1_int[ia+1])-(float)(timf1_int[ib+1]);
      r5=(float)(timf1_int[ja  ])-(float)(timf1_int[jb  ]);
      r7=(float)(timf1_int[ja+1])-(float)(timf1_int[jb+1]);

      t10=(float)(timf1_int[ic  ])-(float)(timf1_int[id  ]);
      t6= (float)(timf1_int[ic+1])-(float)(timf1_int[id+1]);
      r10=(float)(timf1_int[jc  ])-(float)(timf1_int[jd  ]);  
      r6= (float)(timf1_int[jc+1])-(float)(timf1_int[jd+1]);

      out[4*j  ]=t1+t3;
      out[4*j+1]=t2+t4;
      out[4*j+2]=r1+r3;
      out[4*j+3]=r2+r4;

      out[4*j+8]=t1-t3;
      out[4*j+9]=t2-t4;
      out[4*j+10]=r1-r3;
      out[4*j+11]=r2-r4;

      t11=t5-t6;
      t8=t7-t10;
      r11=r5-r6;
      r8=r7-r10;

      t12=t5+t6;
      t9=t7+t10;
      r12=r5+r6;
      r9=r7+r10;

      out[4*j+4]=t12;
      out[4*j+5]=t8;
      out[4*j+6]=r12;
      out[4*j+7]=r8;

      out[4*j+12]=t11;
      out[4*j+13]=t9;
      out[4*j+14]=r11;
      out[4*j+15]=r9;
      }
    }
  else
    {
    for(j=0; j<n; j+=4)
      {
      ja=fft1_permute[j  ];
      jb=fft1_permute[j+1];
      jc=fft1_permute[j+2];
      jd=fft1_permute[j+3];

      ia=(p0+2*ja)&m;
      ib=(p0+2*jb)&m;
      ic=(p0+2*jc)&m;
      id=(p0+2*jd)&m;

      ka=(nn+p0+2*ja)&m;
      kb=(nn+p0+2*jb)&m;
      kc=(nn+p0+2*jc)&m;
      kd=(nn+p0+2*jd)&m;

      a1=fft1_window[ja]*timf1_int[ia  ];
      a2=fft1_window[ja]*timf1_int[ia+1];
      a3=fft1_window[ja]*timf1_int[ka  ];
      a4=fft1_window[ja]*timf1_int[ka+1];

      b1=fft1_window[jb]*timf1_int[ib  ];
      b2=fft1_window[jb]*timf1_int[ib+1];
      b3=fft1_window[jb]*timf1_int[kb  ];
      b4=fft1_window[jb]*timf1_int[kb+1];

      c1=fft1_window[jc]*timf1_int[ic  ];
      c2=fft1_window[jc]*timf1_int[ic+1];
      c3=fft1_window[jc]*timf1_int[kc  ];
      c4=fft1_window[jc]*timf1_int[kc+1];

      d1=fft1_window[jd]*timf1_int[id  ];
      d2=fft1_window[jd]*timf1_int[id+1];
      d3=fft1_window[jd]*timf1_int[kd  ];
      d4=fft1_window[jd]*timf1_int[kd+1];

      t1=a1+b1;
      t2=a2+b2;
      r1=a3+b3;
      r2=a4+b4;

      t3=c1+d1;
      t4=c2+d2;
      r3=c3+d3;
      r4=c4+d4;

      t5=a1-b1;
      t7=a2-b2;
      r5=a3-b3;
      r7=a4-b4;

      t10=c1-d1;
      t6= c2-d2;
      r10=c3-d3;
      r6= c4-d4;

      out[4*j  ]=t1+t3;
      out[4*j+1]=t2+t4;
      out[4*j+2]=r1+r3;
      out[4*j+3]=r2+r4;

      out[4*j+8]=t1-t3;
      out[4*j+9]=t2-t4;
      out[4*j+10]=r1-r3;
      out[4*j+11]=r2-r4;

      t11=t5-t6;
      t8=t7-t10;
      r11=r5-r6;
      r8=r7-r10;

      t12=t5+t6;
      t9=t7+t10;
      r12=r5+r6;
      r9=r7+r10;

      out[4*j+4]=t12;
      out[4*j+5]=t8;
      out[4*j+6]=r12;
      out[4*j+7]=r8;

      out[4*j+12]=t11;
      out[4*j+13]=t9;
      out[4*j+14]=r11;
      out[4*j+15]=r9;
      }
    }
  }
}

void fft1_b(int timf1p_ref, float *out, float *tmp, int gpu_handle_number)
{
int i,j;
int chan, k, m, n, nn, ia, ib, ic;
float t1,t2,t3,t4;
double *dtmp;
int pc;
int multiplicity;
multiplicity=1;
if(ui.rx_rf_channels==1)
  {
  switch (FFT1_CURMODE)
    {
    case 0:
    multiplicity=2;
//  Twin Radix 4 DIT SIMD complex
#if CPU == CPU_INTEL  
    fft1win_dit_one_dual(timf1p_ref, tmp);
/*
    if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
      {
      if(genparm[FIRST_FFT_SINPOW] == 0)
        {
        simd1_16_nowin_dual();
        }
      else
        {
        simd1_16_win_dual();
        }
      }
    else
      {
      if(genparm[FIRST_FFT_SINPOW] == 0)
        {
        simd1_32_nowin_dual();
        }
      else
        {
        simd1_32_win_dual();
        }
      }
*/
    simdbulk_of_dual_dit(fft1_size, fft1_n, tmp, fft1tab);
    for(i=0; i<fft1_size; i++)
      {
      out[2*i  ]=tmp[4*i ]; 
      out[2*i+1]=tmp[4*i+1]; 
      out[2*fft1_size+2*i  ]=tmp[4*i+2]; 
      out[2*fft1_size+2*i+1]=tmp[4*i+3]; 
      }
#else
    lirerr(998546);
#endif  
    break;

    case 1:
    multiplicity=2;    
// Twin Radix 4 DIT C real    
    fft1win_dit_one_real(timf1p_ref, tmp);
    bulk_of_dit(2*fft1_size, fft1_n+1, tmp, fft1tab, yieldflag_wdsp_fft1);
    for(i=1; i<fft1_size; i++)
      {
      j=2*fft1_size-i;
      out[2*fft1_size-2*i  ]=(tmp[2*i  ]+tmp[2*j  ]);
      out[2*fft1_size-2*i+1]=-(tmp[2*i+1]-tmp[2*j+1]);
      out[2*fft1_size+fft1_block-2*i  ]=-(tmp[2*i+1]+tmp[2*j+1]);
      out[2*fft1_size+fft1_block-2*i+1]=-(tmp[2*i  ]-tmp[2*j  ]);
      }
    out[0]=tmp[4*fft1_size-2];
    out[1]=tmp[0];
    out[fft1_block  ]=-tmp[4*fft1_size-1];
    out[fft1_block+1]=tmp[1];
    break;

    case 2:  
// Split radix DIT C real
    fft1_reherm_dit_one(timf1p_ref, out, tmp);
    goto fft_done;

    case 3:
    multiplicity=4;
// Quad Radix 4 DIT C real    
    fft1win_dit_one_dual_real(timf1p_ref,tmp);
    bulk_of_dual_dit(2*fft1_size, fft1_n+1, tmp, fft1tab, yieldflag_wdsp_fft1);
    for(i=1; i<fft1_size; i++)
      {
      j=2*fft1_size-i;
      out[2*fft1_size-2*i  ]=              (tmp[4*i  ]+tmp[4*j  ]);
      out[2*fft1_size-2*i+1]=             -(tmp[4*i+1]-tmp[4*j+1]);
      out[2*fft1_size+fft1_block-2*i  ]=  -(tmp[4*i+1]+tmp[4*j+1]);
      out[2*fft1_size+fft1_block-2*i+1]=  -(tmp[4*i  ]-tmp[4*j  ]);
      out[2*fft1_size+2*fft1_block-2*i  ]= (tmp[4*i+2]+tmp[4*j+2]);
      out[2*fft1_size+2*fft1_block-2*i+1]=-(tmp[4*i+3]-tmp[4*j+3]);
      out[2*fft1_size+3*fft1_block-2*i  ]=-(tmp[4*i+3]+tmp[4*j+3]);
      out[2*fft1_size+3*fft1_block-2*i+1]=-(tmp[4*i+2]-tmp[4*j+2]);
      }
    out[0]=tmp[8*fft1_size-4];
    out[1]=tmp[0];
    out[fft1_block  ]=-tmp[8*fft1_size-3];
    out[fft1_block+1]=tmp[1];
    out[2*fft1_block  ]=tmp[8*fft1_size-2];
    out[2*fft1_block+1]=tmp[2];
    out[3*fft1_block  ]=-tmp[8*fft1_size-1];
    out[3*fft1_block+1]=tmp[3];
    break;

    case 4:
    multiplicity=4;    
// Quad Radix 4 DIT SIMD real  
#if CPU == CPU_INTEL  
// Do real to complex for a single channel by computing four
// transforms in parallel.
    if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
      {
      if(genparm[FIRST_FFT_SINPOW] == 0)
        {
// The SIMD assembly routine is just a little faster than the C routine
// so we use it - but only on 32 bit systems.
#if IA64 == 0
        simd1_16_nowin_real(timf1p_ref, tmp);
#else
        fft1win_dit_one_dual_real(timf1p_ref, tmp);
#endif
        }
      else
        {
// The difference between SIMD assembly and plain C is very small here!!!
// it seems SIMD is just a little faster than C.
#if IA64 == 0
        simd1_16_win_real(timf1p_ref, tmp);
#else
        fft1win_dit_one_dual_real(timf1p_ref, tmp);
#endif
        }
      }
    else
      {
      if(genparm[FIRST_FFT_SINPOW] == 0)
        {
// Assembly not properly implemented. High speed 32 bit hardware will most
// probably not be available in a very long time. 
    fft1win_dit_one_dual_real(timf1p_ref, tmp);
//        simd1_32_nowin_real();
        }
      else
        {
// Assembly not properly implemented. High speed 32 bit hardware will most
// probably not be available in a very long time. 
    fft1win_dit_one_dual_real(timf1p_ref, tmp);
//        simd1_32_win_real();
        }
      }
    simdbulk_of_dual_dit(2*fft1_size, fft1_n+1, tmp, fft1tab);
#else
    lirerr(988546);
#endif  
    j=2*fft1_size-1;  
    for(i=1; i<fft1_size; i++)
      {
      out[2*fft1_size-2*i  ]=              (tmp[4*i  ]+tmp[4*j  ]);
      out[2*fft1_size-2*i+1]=             -(tmp[4*i+1]-tmp[4*j+1]);
      out[2*fft1_size+fft1_block-2*i  ]=  -(tmp[4*i+1]+tmp[4*j+1]);
      out[2*fft1_size+fft1_block-2*i+1]=  -(tmp[4*i  ]-tmp[4*j  ]);
      out[2*fft1_size+2*fft1_block-2*i  ]= (tmp[4*i+2]+tmp[4*j+2]);
      out[2*fft1_size+2*fft1_block-2*i+1]=-(tmp[4*i+3]-tmp[4*j+3]);
      out[2*fft1_size+3*fft1_block-2*i  ]=-(tmp[4*i+3]+tmp[4*j+3]);
      out[2*fft1_size+3*fft1_block-2*i+1]=-(tmp[4*i+2]-tmp[4*j+2]);
      j--;
      }
    out[0]=tmp[8*fft1_size-4];
    out[1]=tmp[0];
    out[fft1_block  ]=-tmp[8*fft1_size-3];
    out[fft1_block+1]=tmp[1];
    out[2*fft1_block  ]=tmp[8*fft1_size-2];
    out[2*fft1_block+1]=tmp[2];
    out[3*fft1_block  ]=-tmp[8*fft1_size-1];
    out[3*fft1_block+1]=tmp[3];
    break;
     
    case 5:
    multiplicity=2;    
//  Twin Radix 4 DIT complex
    fft1win_dit_one_dual(timf1p_ref, tmp);
    bulk_of_dual_dit(fft1_size, fft1_n, tmp, fft1tab, yieldflag_wdsp_fft1);
    for(i=0; i<fft1_size; i++)
      {
      out[2*i  ]=tmp[4*i ]; 
      out[2*i+1]=tmp[4*i+1]; 
      out[2*fft1_size+2*i  ]=tmp[4*i+2]; 
      out[2*fft1_size+2*i+1]=tmp[4*i+3]; 
      }
    break;

    case 6:
// Radix 4 DIT C complex
    fft1win_dit_one(timf1p_ref, out);
    bulk_of_dit(fft1_size, fft1_n, out, fft1tab, yieldflag_wdsp_fft1);
    break;
     
    case 7:
// Radix 2 DIF C complex
    fft1win_dif_one(timf1p_ref, tmp);
    bulk_of_dif(fft1_size, fft1_n, tmp, fft1tab, yieldflag_wdsp_fft1);
    dif_permute_one(out, tmp);    
    break;
     
    case 8:
// Radix 2 DIF ASM complex
    fft1win_dif_one(timf1p_ref, tmp);
#if IA64 == 0 && CPU == CPU_INTEL
    asmbulk_of_dif(fft1_size, fft1_n, tmp, fft1tab, yieldflag_wdsp_fft1);
#else
    bulk_of_dif(fft1_size, fft1_n, tmp, fft1tab, yieldflag_wdsp_fft1);
#endif
    dif_permute_one(out, tmp);    
    break;

    case 18:
// Use OpenCL with clFFT.    
    multiplicity=gpu_fft1_batch_size;
#if OPENCL_PRESENT == 1
    fft1win_gpu(timf1p_ref, tmp);
    execute_clFFT_plan(ocl_handle_fft1b[gpu_handle_number], 
                               CLFFT_FORWARD , (void *) tmp, (void *) out);
#else
    fprintf( stderr,"\nOpenCL needed %d ",gpu_handle_number);
#endif
    break;
    
    case 19:
// Use OpenCL with clFFT.    
    multiplicity=gpu_fft1_batch_size;
#if HAVE_CUFFT == 1
    fft1win_gpu(timf1p_ref, tmp);
    cudaMemcpy(cuda_in[gpu_handle_number], tmp, gpu_fft1_batch_size*fft1_size*
                              sizeof(cuFloatComplex), cudaMemcpyHostToDevice);
      {
      cufftResult res;
      res = cufftExecC2C(cufft_handle_fft1b[gpu_handle_number], 
                                       cuda_in[gpu_handle_number], 
                                       cuda_out[gpu_handle_number], CUFFT_FORWARD);
      if(res != CUFFT_SUCCESS)
        {
        lirerr(1462);
        return;
        }
      }  
    cudaMemcpy(out, cuda_out[gpu_handle_number], gpu_fft1_batch_size*fft1_size*sizeof(cuFloatComplex), cudaMemcpyDeviceToHost);
#else
    fprintf( stderr,"\nCUDA needed  ");
#endif
    break;
    
    default:
    fprintf( stderr,"\nFFT1_CURMODE= %d",FFT1_CURMODE);
    return;
    }
  pc=0;
  m=fft1_first_sym_point;
  if(m==0)m=1;
  k=fft1_size/2;
  n=0;
  for(nn=0; nn<multiplicity; nn++)
    {
    if( (ui.network_flag & NET_RXIN_TIMF2) != 0)
      {
// The timf2 mode is only for testing networks. The spectrum could
// be rearranged by use of a different set of permute tables, but there
// are several fft implementations so doing it here was easier.      
      if(fft1_use_gpu)
        {
        lirerr(1457);
        return;
        }
      for(ia=0; ia < k; ia++)
        {
        t1=out[n+2*ia  ]; 
        t2=out[n+2*ia+1];
        out[n+2*ia  ]=out[n+2*(ia+k)  ];
        out[n+2*ia+1]=out[n+2*(ia+k)+1];
        out[n+2*(ia+k)  ]=t1;
        out[n+2*(ia+k)+1]=t2;
        }
      }
    if(fft1_use_gpu)
      {
      for(ia=0; ia < k; ia++)
        {
        t1=out[n+2*ia  ]; 
        t2=out[n+2*ia+1];
        out[n+2*ia  ]=out[n+2*(ia+k)+1];
        out[n+2*ia+1]=out[n+2*(ia+k)];
        out[n+2*(ia+k)  ]=t2;
        out[n+2*(ia+k)+1]=t1;
        }
      }
// In direct conversion mode it is not possible to get perfect amplitude
// and phase relations between the two channels in the analog circuitry.
// As a result the signal has a mirror image that can be expected in the
// -40dB region for 1% analog filter components.
// The image is removed by the below section that orthogonalises the
// signal and it's mirror using data obtained in a hardware calibration
// process.
    ib=m;
    ic=fft1_size-m;
    if( (fft1_calibrate_flag&CALIQ)==CALIQ)
      {    
      if(fft1_direction > 0)
        {
        for(ia=m; ia < k; ia++)
          {
          t1=   out[n+2*ib  ] * fft1_foldcorr[2*ia  ] 
              - out[n+2*ib+1] * fft1_foldcorr[2*ia+1];
          t2=   out[n+2*ib  ] * fft1_foldcorr[2*ia+1]
              + out[n+2*ib+1] * fft1_foldcorr[2*ia  ];
          out[n+2*ib  ] -=   out[n+2*ic  ] * fft1_foldcorr[2*ic  ] 
                                + out[n+2*ic+1] * fft1_foldcorr[2*ic+1];
          out[n+2*ib+1] -=   out[n+2*ic  ] * fft1_foldcorr[2*ic+1] 
                              - out[n+2*ic+1] * fft1_foldcorr[2*ic  ];
          out[n+2*ic  ]-=t1;
          out[n+2*ic+1]+=t2;
          ib++;
          ic--;
          }
        }
      else        
        {
        for(ia=m; ia < k; ia++)
          {
          t1= out[n+2*ic  ] -
              out[n+2*ib  ] * fft1_foldcorr[2*ia  ] +
              out[n+2*ib+1] * fft1_foldcorr[2*ia+1];
          t2= out[n+2*ic+1] +
              out[n+2*ib  ] * fft1_foldcorr[2*ia+1] +
              out[n+2*ib+1] * fft1_foldcorr[2*ia  ];
          t3= out[n+2*ib  ] -
              out[n+2*ic  ] * fft1_foldcorr[2*ic  ] - 
              out[n+2*ic+1] * fft1_foldcorr[2*ic+1];
          t4= out[n+2*ib+1] -
              out[n+2*ic  ] * fft1_foldcorr[2*ic+1] +
              out[n+2*ic+1] * fft1_foldcorr[2*ic  ];
          out[n+2*ib+1]=t1;
          out[n+2*ib  ]=t2;
          out[n+2*ic+1]=t3;
          out[n+2*ic  ]=t4;
          ib++;
          ic--;
          }
        t1=out[n+2*ib  ];
        t2=out[n+2*pc  ];
        out[n+2*ib  ]=out[n+2*ib+1];
        out[n+2*pc  ]=out[n+2*pc+1];
        out[n+2*ib+1]=t1;
        out[n+2*pc+1]=t2;
        }
      }
    else
      {    
      if(fft1_direction < 0)
        {
        for(ia=m; ia < k; ia++)
          {
          t1= out[n+2*ic  ];
          t2= out[n+2*ic+1];
          out[n+2*ic+1]=out[n+2*ib  ];
          out[n+2*ic  ]=out[n+2*ib+1];
          out[n+2*ib+1]=t1;
          out[n+2*ib  ]=t2;
          ib++;
          ic--;
          }
        t1=out[n+2*ib  ];
        t2=out[n+2*pc  ];
        out[n+2*ib  ]=out[n+2*ib+1];
        out[n+2*pc  ]=out[n+2*pc+1];
        out[n+2*ib+1]=t1;
        out[n+2*pc+1]=t2;
        }
      }
    n+=2*fft1_size;
    }
  }
else
  {
  switch (FFT1_CURMODE)
    {
    case 1:
// Twin Radix 4 DIT C real    
    for(chan=0; chan<2; chan++)
      {
      fft1win_dit_one_real_chan(timf1p_ref, tmp, chan);
      bulk_of_dit(2*fft1_size, fft1_n+1, tmp, fft1tab, yieldflag_wdsp_fft1);
      for(i=1; i<fft1_size; i++)
        {
        j=2*fft1_size-i;
        k=4*(fft1_size-i)+2*chan;
        out[k  ]=(tmp[2*i  ]+tmp[2*j  ]);
        out[k+1]=-(tmp[2*i+1]-tmp[2*j+1]);
        out[k+fft1_block  ]=-(tmp[2*i+1]+tmp[2*j+1]);
        out[k+fft1_block+1]=-(tmp[2*i  ]-tmp[2*j  ]);
        }
      out[2*chan]=tmp[4*fft1_size-2];
      out[2*chan+1]=tmp[0];
      out[fft1_block+2*chan]=-tmp[4*fft1_size-1];
      out[fft1_block+2*chan+1]=tmp[1];
      }
    break;

    case 2:  
// Split radix DIT C real
    fft1_reherm_dit_two(timf1p_ref, out, tmp);
    goto fft_done;

    case 3:
// Quad Radix 4 DIT C real    
    for(chan=0; chan<2; chan++)
      {
      fft1win_dit_one_dual_real_chan(timf1p_ref, tmp, chan);
      bulk_of_dual_dit(2*fft1_size, fft1_n+1, tmp, fft1tab, yieldflag_wdsp_fft1);
      for(i=1; i<fft1_size; i++)
        {
        j=2*fft1_size-i;
        k=4*(fft1_size-i)+2*chan;
        out[k  ]=              (tmp[4*i  ]+tmp[4*j  ]);
        out[k+1]=             -(tmp[4*i+1]-tmp[4*j+1]);
        out[k+fft1_block  ]=  -(tmp[4*i+1]+tmp[4*j+1]);
        out[k+fft1_block+1]=  -(tmp[4*i  ]-tmp[4*j  ]);
        out[k+2*fft1_block  ]= (tmp[4*i+2]+tmp[4*j+2]);
        out[k+2*fft1_block+1]=-(tmp[4*i+3]-tmp[4*j+3]);
        out[k+3*fft1_block  ]=-(tmp[4*i+3]+tmp[4*j+3]);
        out[k+3*fft1_block+1]=-(tmp[4*i+2]-tmp[4*j+2]);
        }
      out[2*chan]=tmp[8*fft1_size-4];
      out[2*chan+1]=tmp[0];
      out[fft1_block+2*chan]=-tmp[8*fft1_size-3];
      out[fft1_block+2*chan+1]=tmp[1];
      out[2*fft1_block+2*chan  ]=tmp[8*fft1_size-2];
      out[2*fft1_block+2*chan+1]=tmp[2];
      out[3*fft1_block+2*chan  ]=-tmp[8*fft1_size-1];
      out[3*fft1_block+2*chan+1]=tmp[3];
      }
    break;

    case 4:
#if(CPU == CPU_INTEL && IA64 == 0)    
    for(chan=0; chan<2; chan++)
      {
      fft1win_dit_one_dual_real_chan(timf1p_ref, tmp, chan);
      simdbulk_of_dual_dit(2*fft1_size, fft1_n+1, tmp, fft1tab);
      for(i=1; i<fft1_size; i++)
        {
        j=2*fft1_size-i;
        k=4*(fft1_size-i)+2*chan;
        out[k  ]=              (tmp[4*i  ]+tmp[4*j  ]);
        out[k+1]=             -(tmp[4*i+1]-tmp[4*j+1]);
        out[k+fft1_block  ]=  -(tmp[4*i+1]+tmp[4*j+1]);
        out[k+fft1_block+1]=  -(tmp[4*i  ]-tmp[4*j  ]);
        out[k+2*fft1_block  ]= (tmp[4*i+2]+tmp[4*j+2]);
        out[k+2*fft1_block+1]=-(tmp[4*i+3]-tmp[4*j+3]);
        out[k+3*fft1_block  ]=-(tmp[4*i+3]+tmp[4*j+3]);
        out[k+3*fft1_block+1]=-(tmp[4*i+2]-tmp[4*j+2]);
        }
      out[2*chan]=tmp[8*fft1_size-4];
      out[2*chan+1]=tmp[0];
      out[fft1_block+2*chan]=-tmp[8*fft1_size-3];
      out[fft1_block+2*chan+1]=tmp[1];
      out[2*fft1_block+2*chan  ]=tmp[8*fft1_size-2];
      out[2*fft1_block+2*chan+1]=tmp[2];
      out[3*fft1_block+2*chan  ]=-tmp[8*fft1_size-1];
      out[3*fft1_block+2*chan+1]=tmp[3];
      }
#else
    lirerr(238553);
#endif    
    break;

    case 6:
// Radix 4 DIT C complex
    for(chan=0; chan<2; chan++)
      {
      fft1win_dit_chan(chan,timf1p_ref, tmp);
      bulk_of_dit(fft1_size, fft1_n, tmp, fft1tab, yieldflag_wdsp_fft1);
      dit_finish_chan (chan, out, tmp);
      }
    break;
     
    case 7:
// Radix 2 DIF C complex
    for(chan=0; chan<2; chan++)
      {
      fft1win_dif_chan(chan,timf1p_ref, tmp);
      bulk_of_dif(fft1_size, fft1_n, tmp, fft1tab, yieldflag_wdsp_fft1);
      dif_permute_chan (chan, out, tmp);
      }
    break;

    case 8:
// Radix 2 DIF ASM complex
#if IA64 == 0 && CPU == CPU_INTEL
    for(chan=0; chan<2; chan++)
      {
      fft1win_dif_chan(chan, timf1p_ref, tmp);
      asmbulk_of_dif(fft1_size, fft1_n, tmp, fft1tab, yieldflag_wdsp_fft1);
      dif_permute_chan (chan, out, tmp);
      }
#else
    for(chan=0; chan<2; chan++)
      {
      fft1win_dif_chan(chan,timf1p_ref, tmp);
      bulk_of_dif(fft1_size, fft1_n, tmp, fft1tab, yieldflag_wdsp_fft1);
      dif_permute_chan (chan, out, tmp);
      }
#endif    
    break;
     
    case 9:
    multiplicity=2;    
// Twin radix 2 DIF ASM complex
    fft1win_dif_two(timf1p_ref, tmp);
#if IA64 == 0 && CPU == CPU_INTEL
  asmbulk_of_dual_dif(fft1_size, fft1_n, 
                                    tmp, fft1tab, yieldflag_wdsp_fft1);
#else
  bulk_of_dual_dif(fft1_size, fft1_n, 
                                    tmp, fft1tab, yieldflag_wdsp_fft1);
#endif
    dif_permute_two(out, tmp);
    break;

    case 10:
    multiplicity=2;    
// Twin Radix 4 DIT C complex
    fft1win_dit_two(timf1p_ref, out);
    bulk_of_dual_dit(fft1_size, fft1_n, out, fft1tab, yieldflag_wdsp_fft1);
    break;
     
    case 11:
    multiplicity=2;    
// Twin Radix 4 DIT SIMD complex
#if CPU == CPU_INTEL
#if IA64 == 0
  if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    if(genparm[FIRST_FFT_SINPOW] == 0)
      {
      simd1_16_nowin(timf1p_ref,out);
      }
    else
      {
      simd1_16_win(timf1p_ref,out);
      }
    }
  else
    {
    if(genparm[FIRST_FFT_SINPOW] == 0)
      {
      simd1_32_nowin(timf1p_ref,out);
      }
    else
      {
      simd1_32_win(timf1p_ref,out);
      }
    }
#else
  fft1win_dit_two(timf1p_ref, out);
#endif
  simdbulk_of_dual_dit(fft1_size, fft1_n, out, fft1tab);
#else
  lirerr(998546);
#endif  
    break;

    case 18:
    fprintf( stderr," \nfft1ver 18 not implemented ");
    break;

    case 19:
    fprintf( stderr," \nfft1ver 19 not implemented ");
    break;
    
    case 20:
// double precision radix 2 DIF C complex
    dtmp=(double*)tmp;
    for(chan=0; chan<2; chan++)
      {
      d_fft1win_dif_chan(chan,timf1p_ref, dtmp);
      d_bulk_of_dif(fft1_size, fft1_n, dtmp, d_fft1tab, yieldflag_wdsp_fft1);
      for(i=0; i<2*fft1_size; i++)
        {
        tmp[i]=dtmp[i];
        }
      dif_bigpermute_chan (chan, out, tmp);
      }
    break;

    default:
    fprintf( stderr,"\nFFT1_CURMODE= %d",FFT1_CURMODE);
    return;
    }
  if( (ui.network_flag & NET_RXIN_TIMF2) != 0)
    {
    if(ui.rx_addev_no != NETWORK_DEVICE_CODE)
      {
      lirerr(1319);
      return;
      }
    k=fft1_size/2;
    for(ia=0; ia < k; ia++)
      {
      t1=out[4*ia  ]; 
      t2=out[4*ia+1];
      t3=out[4*ia+2]; 
      t4=out[4*ia+3];
      out[4*ia  ]=out[4*(ia+k)  ];
      out[4*ia+1]=out[4*(ia+k)+1];
      out[4*ia+2]=out[4*(ia+k)+2];
      out[4*ia+3]=out[4*(ia+k)+3];
      out[4*(ia+k)  ]=t1;
      out[4*(ia+k)+1]=t2;
      out[4*(ia+k)+2]=t3;
      out[4*(ia+k)+3]=t4;
      }
    }
  pc=0;
  m=fft1_first_sym_point;
  if(m==0)m=1;
  k=fft1_size/2;
  n=0;
  for(nn=0; nn<multiplicity; nn++)
    {
// In direct conversion mode it is not possible to get perfect amplitude
// and phase relations between the two channels in the analog circuitry.
// As a result the signal has a mirror image that can be expected in the
// -40dB region for 1% analog filter components.
// The image is removed by the below section that orthogonalises the
// signal and it's mirror using data obtained in a hardware calibration
// process.
    ib=m;
    ic=fft1_size-m;
    if( (fft1_calibrate_flag&CALIQ)==CALIQ &&
         !(internal_generator_flag != 0 &&
         (INTERNAL_GEN_FILTER_TEST == TRUE ||
         INTERNAL_GEN_CARRIER == TRUE)))
      {    
      if(fft1_direction > 0)
        {
        for(ia=m; ia < k; ia++)
          {
          t1=   out[n+4*ib  ] * fft1_foldcorr[4*ia  ] 
              - out[n+4*ib+1] * fft1_foldcorr[4*ia+1];
          t2=   out[n+4*ib  ] * fft1_foldcorr[4*ia+1]
              + out[n+4*ib+1] * fft1_foldcorr[4*ia  ];
          out[n+4*ib  ] -=   out[n+4*ic  ] * fft1_foldcorr[4*ic  ] 
                                + out[n+4*ic+1] * fft1_foldcorr[4*ic+1];
          out[n+4*ib+1] -=   out[n+4*ic  ] * fft1_foldcorr[4*ic+1] 
                              - out[n+4*ic+1] * fft1_foldcorr[4*ic  ];
          out[n+4*ic  ]-=t1;
          out[n+4*ic+1]+=t2;
          t1=   out[n+4*ib+2] * fft1_foldcorr[4*ia+2] 
              - out[n+4*ib+3] * fft1_foldcorr[4*ia+3];
          t2=   out[n+4*ib+2] * fft1_foldcorr[4*ia+3]
              + out[n+4*ib+3] * fft1_foldcorr[4*ia+2];
          out[n+4*ib+2] -=   out[n+4*ic+2] * fft1_foldcorr[4*ic+2] 
                                + out[n+4*ic+3] * fft1_foldcorr[4*ic+3];
          out[n+4*ib+3] -=   out[n+4*ic+2] * fft1_foldcorr[4*ic+3] 
                                - out[n+4*ic+3] * fft1_foldcorr[4*ic+2];
          out[n+4*ic+2]-=t1;
          out[n+4*ic+3]+=t2;
          ib++;
          ic--;
          }
        }
      else
        {
        for(ia=m; ia < k; ia++)
          {
          t1= out[n+4*ic  ] -
              out[n+4*ib  ] * fft1_foldcorr[4*ia  ] +
              out[n+4*ib+1] * fft1_foldcorr[4*ia+1];
          t2= out[n+4*ic+1] +
              out[n+4*ib  ] * fft1_foldcorr[4*ia+1] +
              out[n+4*ib+1] * fft1_foldcorr[4*ia  ];
          t3= out[n+4*ib  ] -
              out[n+4*ic  ] * fft1_foldcorr[4*ic  ] - 
              out[n+4*ic+1] * fft1_foldcorr[4*ic+1];
          t4= out[n+4*ib+1] -
              out[n+4*ic  ] * fft1_foldcorr[4*ic+1] +
              out[n+4*ic+1] * fft1_foldcorr[4*ic  ];
          out[n+4*ib+1]=t1;
          out[n+4*ib  ]=t2;
          out[n+4*ic+1]=t3;
          out[n+4*ic  ]=t4;
          t1= out[n+4*ic+2] -
              out[n+4*ib+2] * fft1_foldcorr[4*ia+2] +
              out[n+4*ib+3] * fft1_foldcorr[4*ia+3];
          t2= out[n+4*ic+3] +
              out[n+4*ib+2] * fft1_foldcorr[4*ia+3] +
              out[n+4*ib+3] * fft1_foldcorr[4*ia+2];
          t3= out[n+4*ib+2] -
              out[n+4*ic+2] * fft1_foldcorr[4*ic+2] - 
              out[n+4*ic+3] * fft1_foldcorr[4*ic+3];
          t4= out[n+4*ib+3] -
              out[n+4*ic+2] * fft1_foldcorr[4*ic+3] +
              out[n+4*ic+3] * fft1_foldcorr[4*ic+2];
          out[n+4*ib+3]=t1;
          out[n+4*ib+2]=t2;
          out[n+4*ic+3]=t3;
          out[n+4*ic+2]=t4;
          ib++;
          ic--;
          }
        t1=out[n+4*ib  ];
        t2=out[n+4*pc  ];
        t3=out[n+4*ib+2];
        t4=out[n+4*pc+2];
        out[n+4*ib  ]=out[n+4*ib+1];
        out[n+4*ib+2]=out[n+4*ib+3];
        out[n+4*pc  ]=out[n+4*pc+1];
        out[n+4*pc+2]=out[n+4*pc+3];
        out[n+4*ib+1]=t1;
        out[n+4*pc+1]=t2;
        out[n+4*ib+3]=t3;
        out[n+4*pc+3]=t4;
        }
      }
    else
      {    
      if(fft1_direction < 0)
        {
        for(ia=m; ia < k; ia++)
          {
          t1= out[n+4*ic  ];
          t2= out[n+4*ic+1];
          t3= out[n+4*ic+2];
          t4= out[n+4*ic+3];
          out[n+4*ic+1]=out[n+4*ib  ];
          out[n+4*ic  ]=out[n+4*ib+1];
          out[n+4*ic+3]=out[n+4*ib+2];
          out[n+4*ic+2]=out[n+4*ib+3];
          out[n+4*ib+1]=t1;
          out[n+4*ib  ]=t2;
          out[n+4*ib+3]=t3;
          out[n+4*ib+2]=t4;
          ib++;
          ic--;
          }
        t1=out[n+4*ib  ];
        t2=out[n+4*pc  ];
        t3=out[n+4*ib+2];
        t4=out[n+4*pc+2];
        out[n+4*ib  ]=out[n+4*ib+1];
        out[n+4*ib+2]=out[n+4*ib+3];
        out[n+4*pc  ]=out[n+4*pc+1];
        out[n+4*pc+2]=out[n+4*pc+3];
        out[n+4*ib+1]=t1;
        out[n+4*pc+1]=t2;
        out[n+4*ib+3]=t3;
        out[n+4*pc+3]=t4;
        }
      }
    n+=4*fft1_size;
    }
  if(pg_ch2_c1 != 1.0F || pg_ch2_c2 != 0.0F)
    {
    m=fft1_first_sym_point;
    k=fft1_size-m;
    n=0;
    for(nn=0; nn<multiplicity; nn++)
      {
      for(i=m; i<k; i++)
        {
        t1=out[n+4*i+2];
        t2=out[n+4*i+3];        
        out[n+4*i+2]= t1*pg_ch2_c1+t2*pg_ch2_c2;
        out[n+4*i+3]= t2*pg_ch2_c1-t1*pg_ch2_c2;
        }
      n+=4*fft1_size;
      }
    }
  }  
fft_done:;
}

void fft1_c(void)
{
int i, ia, ib, ic, pa, ix;
float *pwr;
TWOCHAN_POWER *pxy;
float t1,t2;
float *z, *sum;
// The total filter chain of the hardware, is normally not
// very flat, nor has it a good pulse response.
// Add one extra filter here so the total filter response becomes what
// the user has asked for.
// At the same time the data becomes normalised to an amplitude that
// is independent of the processes up to this point.
// Normalisation is for the noise floor to be around 0dB in the
// full dynamic range graph. 
// To save memory accesses, calculate fft1_sumsq at the same time.
// In case AFC is run from fft1 we are probably using a slow 
// computer and large fft1 sizes. Then summed powers are
// calculated elsewhere.


pa=fft1_nb*fft1_block;
z=&fft1_float[pa];
sum=&fft1_sumsq[fft1_sumsq_pa];
ix=fft1_last_point;
ic=fft1_sumsq_pa+fft1_first_point;
if(fft1afc_flag <= 0)
  {
// We want fft1_sumsq to contain summed power spectra calculated from fft1
// over wg.fft_avg1num individual spectra.
  if(fft1_sumsq_counter == 0)
    {
    if(sw_onechan)
      {
      for(ia=fft1_first_point; ia <= ix; ia++)
        {
        t1=z[2*ia  ]*fft1_filtercorr[2*ia  ]-
           z[2*ia+1]*fft1_filtercorr[2*ia+1];
        z[2*ia+1]=z[2*ia+1]*fft1_filtercorr[2*ia  ]+
                           z[2*ia  ]*fft1_filtercorr[2*ia+1];
        z[2*ia]=t1;
        sum[ia]=t1*t1+z[2*ia+1]*z[2*ia+1];
        }
      }  
    else
      {
      ib=pa/4+fft1_first_point;
      for(ia=fft1_first_point; ia <= ix; ia++)
        {
        t1=fft1_float[4*ib  ]*fft1_filtercorr[4*ia  ]-
           fft1_float[4*ib+1]*fft1_filtercorr[4*ia+1];
        fft1_float[4*ib+1]=fft1_float[4*ib+1]*fft1_filtercorr[4*ia  ]+
                           fft1_float[4*ib  ]*fft1_filtercorr[4*ia+1];
        fft1_float[4*ib  ]=t1;
        t1=t1*t1+fft1_float[4*ib+1]*fft1_float[4*ib+1];
        t2=fft1_float[4*ib+2]*fft1_filtercorr[4*ia+2]-
           fft1_float[4*ib+3]*fft1_filtercorr[4*ia+3];
        fft1_float[4*ib+3]=fft1_float[4*ib+3]*fft1_filtercorr[4*ia+2]+
                           fft1_float[4*ib+2]*fft1_filtercorr[4*ia+3];
        fft1_float[4*ib+2]=t2;
        fft1_sumsq[ic]=t1+t2*t2+fft1_float[4*ib+3]*fft1_float[4*ib+3];
        if(fft1_correlation_flag == 1)
          {
          fft1_corrsum[2*ic  ]=2*(fft1_float[4*ib  ]*fft1_float[4*ib+2]+
                                  fft1_float[4*ib+1]*fft1_float[4*ib+3]);
          fft1_corrsum[2*ic+1]=2*(fft1_float[4*ib+1]*fft1_float[4*ib+2]-
                                  fft1_float[4*ib  ]*fft1_float[4*ib+3]);
          }
        ib++;
        ic++;
        }
      }    
    }
  else
    {
    if(sw_onechan)
      {
      for(ia=fft1_first_point; ia <= ix; ia++)
        {
        t1=z[2*ia  ]*fft1_filtercorr[2*ia  ]-
           z[2*ia+1]*fft1_filtercorr[2*ia+1];
        z[2*ia+1]=z[2*ia+1]*fft1_filtercorr[2*ia  ]+
                           z[2*ia  ]*fft1_filtercorr[2*ia+1];
        z[2*ia]=t1;
        sum[ia]+=t1*t1+z[2*ia+1]*z[2*ia+1];
        }
      }  
    else
      {
      ib=pa/4+fft1_first_point;
      for(ia=fft1_first_point; ia <= ix; ia++)
        {
        t1=fft1_float[4*ib  ]*fft1_filtercorr[4*ia  ]-
           fft1_float[4*ib+1]*fft1_filtercorr[4*ia+1];
        fft1_float[4*ib+1]=fft1_float[4*ib+1]*fft1_filtercorr[4*ia  ]+
                           fft1_float[4*ib  ]*fft1_filtercorr[4*ia+1];
        fft1_float[4*ib  ]=t1;
        t1=t1*t1+fft1_float[4*ib+1]*fft1_float[4*ib+1];               
        t2=fft1_float[4*ib+2]*fft1_filtercorr[4*ia+2]-
           fft1_float[4*ib+3]*fft1_filtercorr[4*ia+3];
        fft1_float[4*ib+3]=fft1_float[4*ib+3]*fft1_filtercorr[4*ia+2]+
                           fft1_float[4*ib+2]*fft1_filtercorr[4*ia+3];
        fft1_float[4*ib+2]=t2;
        fft1_sumsq[ic]+=t1+t2*t2+fft1_float[4*ib+3]*fft1_float[4*ib+3];
        if(fft1_correlation_flag == 1)
          {
          fft1_corrsum[2*ic  ]+=2*(fft1_float[4*ib  ]*fft1_float[4*ib+2]+
                                   fft1_float[4*ib+1]*fft1_float[4*ib+3]);
          fft1_corrsum[2*ic+1]+=2*(fft1_float[4*ib+1]*fft1_float[4*ib+2]-
                                   fft1_float[4*ib  ]*fft1_float[4*ib+3]);
          }
        ib++;
        ic++;
        }
      }    
    }
  }    
else
  {
// This code is executed to make the last step of fft1 in case
// fft2 is not selected and AFC is in use.
  ffts_na=fft1_nb;
  ffts_nm=fft1_nm;
  if(no_of_spurs > 0)
    {
    if(sw_onechan)
      {
      ib=pa/2+fft1_first_point;
      for(ia=fft1_first_point; ia <= ix; ia++)
        {
        t1=fft1_float[2*ib  ]*fft1_filtercorr[2*ia  ]-
           fft1_float[2*ib+1]*fft1_filtercorr[2*ia+1];
        fft1_float[2*ib+1]=fft1_float[2*ib+1]*fft1_filtercorr[2*ia  ]+
                           fft1_float[2*ib  ]*fft1_filtercorr[2*ia+1];
        fft1_float[2*ib]=t1;
        ib++;
        }
      }  
    else
      {
      ib=pa/4+fft1_first_point;
      for(ia=fft1_first_point; ia <= ix; ia++)
        {
        t1=fft1_float[4*ib  ]*fft1_filtercorr[4*ia  ]-
           fft1_float[4*ib+1]*fft1_filtercorr[4*ia+1];
        fft1_float[4*ib+1]=fft1_float[4*ib+1]*fft1_filtercorr[4*ia  ]+
                           fft1_float[4*ib  ]*fft1_filtercorr[4*ia+1];
        fft1_float[4*ib  ]=t1;
        t2=fft1_float[4*ib+2]*fft1_filtercorr[4*ia+2]-
           fft1_float[4*ib+3]*fft1_filtercorr[4*ia+3];
        fft1_float[4*ib+3]=fft1_float[4*ib+3]*fft1_filtercorr[4*ia+2]+
                           fft1_float[4*ib+2]*fft1_filtercorr[4*ia+3];
        fft1_float[4*ib+2]=t2;
        ib++;
        }
      }
    fftx_na=fft1_nb;
    eliminate_spurs();
    if(fft1_sumsq_counter == 0)
      {
      if(sw_onechan)
        {
        ib=pa/2+fft1_first_point;
        pwr=&fft1_power[fft1_nb*fft1_size];
        for(ia=fft1_first_point; ia <= ix; ia++)
          {
          pwr[ia]=fft1_float[2*ib]*fft1_float[2*ib]+
                  fft1_float[2*ib+1]*fft1_float[2*ib+1];
          fft1_sumsq[ic]=pwr[ia];
          ib++;
          ic++;
          }
        }  
      else
        {
        ib=pa/4+fft1_first_point;
        pxy=&fft1_xypower[fft1_nb*fft1_size];
        for(ia=fft1_first_point; ia <= ix; ia++)
          {
          pxy[ia].x2=fft1_float[4*ib  ]*fft1_float[4*ib  ]+
                     fft1_float[4*ib+1]*fft1_float[4*ib+1];
          pxy[ia].y2=fft1_float[4*ib+2]*fft1_float[4*ib+2]+
                     fft1_float[4*ib+3]*fft1_float[4*ib+3];
          pxy[ia].im_xy=-fft1_float[4*ib  ]*fft1_float[4*ib+3]+
                         fft1_float[4*ib+2]*fft1_float[4*ib+1];
          pxy[ia].re_xy= fft1_float[4*ib  ]*fft1_float[4*ib+2]+
                         fft1_float[4*ib+1]*fft1_float[4*ib+3];
          fft1_sumsq[ic]=pxy[ia].x2+pxy[ia].y2;
          if(fft1_correlation_flag == 1)
            {
            fft1_corrsum[2*ic  ]=2*pxy[ia].re_xy;
            fft1_corrsum[2*ic+1]=2*pxy[ia].im_xy;
            }
          ic++;
          ib++;
          }
        }    
      }
    else
      {
      if(sw_onechan)
        {
        ib=pa/2+fft1_first_point;
        pwr=&fft1_power[fft1_nb*fft1_size];
        for(ia=fft1_first_point; ia <= ix; ia++)
          {
          pwr[ia]=fft1_float[2*ib]*fft1_float[2*ib]+
                  fft1_float[2*ib+1]*fft1_float[2*ib+1];
          fft1_sumsq[ic]+=pwr[ia];
          ic++;
          ib++;
          }
        }  
      else
        {
        ib=pa/4+fft1_first_point;
        pxy=&fft1_xypower[fft1_nb*fft1_size];
        for(ia=fft1_first_point; ia <= ix; ia++)
          {
          pxy[ia].x2=fft1_float[4*ib  ]*fft1_float[4*ib  ]+
                     fft1_float[4*ib+1]*fft1_float[4*ib+1];
          pxy[ia].y2=fft1_float[4*ib+2]*fft1_float[4*ib+2]+
                     fft1_float[4*ib+3]*fft1_float[4*ib+3];
          pxy[ia].im_xy=-fft1_float[4*ib  ]*fft1_float[4*ib+3]+
                         fft1_float[4*ib+2]*fft1_float[4*ib+1];
          pxy[ia].re_xy= fft1_float[4*ib  ]*fft1_float[4*ib+2]+
                         fft1_float[4*ib+1]*fft1_float[4*ib+3];
          fft1_sumsq[ic]+=pxy[ia].x2+pxy[ia].y2;
          if(fft1_correlation_flag == 1)
            {
            fft1_corrsum[2*ic  ]+=2*pxy[ia].re_xy;
            fft1_corrsum[2*ic+1]+=2*pxy[ia].im_xy;
            }
          ic++;
          ib++;
          }
        }    
      }
    }  
  else
    {
    if(fft1_sumsq_counter == 0)
      {
      if(sw_onechan)
        {
        ib=pa/2+fft1_first_point;
        pwr=&fft1_power[fft1_nb*fft1_size];
        for(ia=fft1_first_point; ia <= ix; ia++)
          {
          t1=fft1_float[2*ib  ]*fft1_filtercorr[2*ia  ]-
             fft1_float[2*ib+1]*fft1_filtercorr[2*ia+1];
          fft1_float[2*ib+1]=fft1_float[2*ib+1]*fft1_filtercorr[2*ia  ]+
                             fft1_float[2*ib  ]*fft1_filtercorr[2*ia+1];
          fft1_float[2*ib]=t1;
          pwr[ia]=t1*t1+fft1_float[2*ib+1]*fft1_float[2*ib+1];
          fft1_sumsq[ic]=pwr[ia];
          ib++;
          ic++;
          }
        }  
      else
        {
        ib=pa/4+fft1_first_point;
        pxy=&fft1_xypower[fft1_nb*fft1_size];
        for(ia=fft1_first_point; ia <= ix; ia++)
          {
          t1=fft1_float[4*ib  ]*fft1_filtercorr[4*ia  ]-
             fft1_float[4*ib+1]*fft1_filtercorr[4*ia+1];
          fft1_float[4*ib+1]=fft1_float[4*ib+1]*fft1_filtercorr[4*ia  ]+
                             fft1_float[4*ib  ]*fft1_filtercorr[4*ia+1];
          fft1_float[4*ib  ]=t1;
          t2=fft1_float[4*ib+2]*fft1_filtercorr[4*ia+2]-
             fft1_float[4*ib+3]*fft1_filtercorr[4*ia+3];
          fft1_float[4*ib+3]=fft1_float[4*ib+3]*fft1_filtercorr[4*ia+2]+
                             fft1_float[4*ib+2]*fft1_filtercorr[4*ia+3];
          fft1_float[4*ib+2]=t2;
          pxy[ia].x2=t1*t1+fft1_float[4*ib+1]*fft1_float[4*ib+1];
          pxy[ia].y2=t2*t2+fft1_float[4*ib+3]*fft1_float[4*ib+3];
          pxy[ia].im_xy=-t1*fft1_float[4*ib+3]+t2*fft1_float[4*ib+1];
          pxy[ia].re_xy=t1*t2+fft1_float[4*ib+1]*fft1_float[4*ib+3];
          fft1_sumsq[ic]=pxy[ia].x2+pxy[ia].y2;
          if(fft1_correlation_flag == 1)
            {
            fft1_corrsum[2*ic  ]=2*pxy[ia].re_xy;
            fft1_corrsum[2*ic+1]=2*pxy[ia].im_xy;
            }
          ic++;
          ib++;
          }
        }    
      }
    else
      {
      if(sw_onechan)
        {
        ib=pa/2+fft1_first_point;
        pwr=&fft1_power[fft1_nb*fft1_size];
        for(ia=fft1_first_point; ia <= ix; ia++)
          {
          t1=fft1_float[2*ib  ]*fft1_filtercorr[2*ia  ]-
             fft1_float[2*ib+1]*fft1_filtercorr[2*ia+1];
          fft1_float[2*ib+1]=fft1_float[2*ib+1]*fft1_filtercorr[2*ia  ]+
                             fft1_float[2*ib  ]*fft1_filtercorr[2*ia+1];
          fft1_float[2*ib]=t1;
          pwr[ia]=t1*t1+fft1_float[2*ib+1]*fft1_float[2*ib+1];
          fft1_sumsq[ic]+=pwr[ia];
          ic++;
          ib++;
          }
        }  
      else
        {
        ib=pa/4+fft1_first_point;
        pxy=&fft1_xypower[fft1_nb*fft1_size];
        for(ia=fft1_first_point; ia <= ix; ia++)
          {
          t1=fft1_float[4*ib  ]*fft1_filtercorr[4*ia  ]-
             fft1_float[4*ib+1]*fft1_filtercorr[4*ia+1];
          fft1_float[4*ib+1]=fft1_float[4*ib+1]*fft1_filtercorr[4*ia  ]+
                             fft1_float[4*ib  ]*fft1_filtercorr[4*ia+1];
          fft1_float[4*ib  ]=t1;
          t2=fft1_float[4*ib+2]*fft1_filtercorr[4*ia+2]-
             fft1_float[4*ib+3]*fft1_filtercorr[4*ia+3];
          fft1_float[4*ib+3]=fft1_float[4*ib+3]*fft1_filtercorr[4*ia+2]+
                             fft1_float[4*ib+2]*fft1_filtercorr[4*ia+3];
          fft1_float[4*ib+2]=t2;
          pxy[ia].x2=t1*t1+fft1_float[4*ib+1]*fft1_float[4*ib+1];
          pxy[ia].y2=t2*t2+fft1_float[4*ib+3]*fft1_float[4*ib+3];
          pxy[ia].im_xy=-t1*fft1_float[4*ib+3]+t2*fft1_float[4*ib+1];
          pxy[ia].re_xy=t1*t2+fft1_float[4*ib+1]*fft1_float[4*ib+3];
          fft1_sumsq[ic]+=pxy[ia].x2+pxy[ia].y2;
          if(fft1_correlation_flag == 1)
            {
            fft1_corrsum[2*ic  ]+=2*pxy[ia].re_xy;
            fft1_corrsum[2*ic+1]+=2*pxy[ia].im_xy;
            }
          ic++;
          ib++;
          }
        }    
      }
    }
  if(genparm[MAX_NO_OF_SPURS] > 0)
    {  
    if(sw_onechan)
      {
      pwr=&fft1_power[fft1_nb*fft1_size];
      if(spursearch_sum_counter > 3*spur_speknum)
        {
        spursearch_sum_counter=0;
        for(i=spur_search_first_point; i <= spur_search_last_point; i++)
          {
          spursearch_spectrum[i]=spursearch_powersum[i]+pwr[i];
          }
        spursearch_spectrum_cleanup();
        }
      else
        {
        if(spursearch_sum_counter == 0)
          {
          for(i=spur_search_first_point; i <= spur_search_last_point; i++)
            {
            spursearch_powersum[i]=pwr[i];
            }
          }
        else
          {
          for(i=spur_search_first_point; i <= spur_search_last_point; i++)
            {
            spursearch_powersum[i]+=pwr[i];
            }
          }
        spursearch_sum_counter++;
        }
      }
    else
      {
      pxy=&fft1_xypower[fft1_nb*fft1_size];
      if(spursearch_sum_counter > 3*spur_speknum)
        {
        spursearch_sum_counter=0;
        for(i=wg_first_point; i <= wg_last_point; i++)
          {
          spursearch_xysum[i].x2+=pxy[i].x2;
          spursearch_xysum[i].y2+=pxy[i].y2;
          spursearch_xysum[i].re_xy+=pxy[i].re_xy;
          spursearch_xysum[i].im_xy+=pxy[i].im_xy;
          t1=spursearch_xysum[i].x2+spursearch_xysum[i].y2;
          spursearch_spectrum[i]=t1+
                      2*(spursearch_xysum[i].re_xy*spursearch_xysum[i].re_xy+
                         spursearch_xysum[i].im_xy*spursearch_xysum[i].im_xy-
                         spursearch_xysum[i].x2*spursearch_xysum[i].y2)/t1;
          }
        spursearch_spectrum_cleanup();
        }
      else
        {
        if(spursearch_sum_counter == 0)
          {
          for(i=wg_first_point; i <= wg_last_point; i++)
            {
            spursearch_xysum[i].x2=pxy[i].x2;
            spursearch_xysum[i].y2=pxy[i].y2;
            spursearch_xysum[i].re_xy=pxy[i].re_xy;
            spursearch_xysum[i].im_xy=pxy[i].im_xy;
            }
          }
        else
          {
          for(i=wg_first_point; i <= wg_last_point; i++)
            {
            spursearch_xysum[i].x2+=pxy[i].x2;
            spursearch_xysum[i].y2+=pxy[i].y2;
            spursearch_xysum[i].re_xy+=pxy[i].re_xy;
            spursearch_xysum[i].im_xy+=pxy[i].im_xy;
            }
          }
        spursearch_sum_counter++;
        }
      }
    }
  }
fft1_sumsq_counter++;
if(fft1_sumsq_counter >= wg.fft_avg1num)
  {
  fft1_sumsq_counter=0;
  update_fft1_slowsum();
// We have a new average power spectrum.
// In case the second fft is enabled we have to set up information
// for the selective limiter.
  if(genparm[SECOND_FFT_ENABLE] != 0)
    {
    fft1_liminfo_cnt++;
    }
  fft1_sumsq_pa=(fft1_sumsq_pa+fft1_size)&fft1_sumsq_mask;   
  }
fft1_nb=(fft1_nb+1)&fft1n_mask;
fft1_pb=fft1_nb*fft1_block;
if(genparm[SECOND_FFT_ENABLE] == 0)ag_pa=(ag_pa+1)&ag_mask;  
}

void update_fft1_slowsum(void)
{
int i,ia,pa,pb;
int fresh_recalc;
// Now that we have a new power spectrum in fft1_sumsq, update the
// average of the spectra in fft1_slowsum by adding it's power and 
// subtracting the power of the old one that should no longer 
// contribute to the sum.  
// This way of calculating an average may cause errors after a long
// time since bits may be lost occasionally due to rounding errors.
// Calculate a few points from scratch each time to eliminate this
// problem.
lir_mutex_lock(MUTEX_FFT1SLOWSUM);
pa=fft1_sumsq_pa;
if(change_fft1_flag)
  {
  new_fft1_averages(pa, wg_first_point, wg_last_point);
  lir_mutex_unlock(MUTEX_FFT1SLOWSUM);
  return;
  }
if(genparm[FIRST_FFT_BANDWIDTH] > 200)
  {
  if(wg_fft_avg2num < 100)
    {
    fresh_recalc=4;
    }
  else
    {
    fresh_recalc=8;
    }
  }
else
  {
  if(wg_fft_avg2num < 20)
    {
    fresh_recalc=2;
    }
  else
    {
    fresh_recalc=2;
    }
  }        
pb=(pa-wg_fft_avg2num*fft1_size+fft1_sumsq_bufsize)&fft1_sumsq_mask;
if(fft1_sumsq_recalc == fft1_last_point) fft1_sumsq_recalc=fft1_first_point;
ia=fft1_sumsq_recalc;
fft1_sumsq_recalc+=wg.xpoints/fresh_recalc;
if(fft1_sumsq_recalc>fft1_last_point)fft1_sumsq_recalc=fft1_last_point;
new_fft1_averages(pa, ia, fft1_sumsq_recalc);
for(i=fft1_first_point; i < ia; i++)
  {
  fft1_slowsum[i]+=fft1_sumsq[pa+i]-fft1_sumsq[pb+i];
  if(fft1_slowsum[i]<FFT1_SMALL)fft1_slowsum[i]=FFT1_SMALL;
  }
for(i=fft1_sumsq_recalc+1; i <= fft1_last_point; i++)
  {
  fft1_slowsum[i]+=fft1_sumsq[pa+i]-fft1_sumsq[pb+i];
  if(fft1_slowsum[i]<FFT1_SMALL)fft1_slowsum[i]=FFT1_SMALL;
  }
if(fft1_correlation_flag == 1)
  {
  if(correlation_reset_flag != fft1corr_reset_flag)clear_fft1_correlation();
  for(i=fft1_first_point; i < ia; i++)
    {
    fft1_slowcorr[2*i  ]+=fft1_corrsum[2*(pa+i)  ]-fft1_corrsum[2*(pb+i)  ];
    fft1_slowcorr[2*i+1]+=fft1_corrsum[2*(pa+i)+1]-fft1_corrsum[2*(pb+i)+1];
    }
  for(i=fft1_sumsq_recalc+1; i <= fft1_last_point; i++)
    {
    fft1_slowcorr[2*i  ]+=fft1_corrsum[2*(pa+i)  ]-fft1_corrsum[2*(pb+i)  ];
    fft1_slowcorr[2*i+1]+=fft1_corrsum[2*(pa+i)+1]-fft1_corrsum[2*(pb+i)+1];
    }
  for(i=fft1_first_point; i<=fft1_last_point; i++)
    {
    fft1_slowcorr_tot[2*i  ]+=fft1_corrsum[2*(pa+i)  ];
    fft1_slowcorr_tot[2*i+1]+=fft1_corrsum[2*(pa+i)+1];
    }
  slowcorr_tot_avgnum+=wg.fft_avg1num;
  }
lir_mutex_unlock(MUTEX_FFT1SLOWSUM);
}

void set_fft1_endpoints(void)
{
int i;
float t1;
// Set all pointers for a non-inverted spectrum.
wg_first_point=wg.first_xpoint;
wg_last_point=wg.first_xpoint+wg.xpoints;
if(wg_last_point >= fft1_size)wg_last_point=fft1_size-1;
fft1_first_point=0;
fft1_first_inband=0;
fft1_last_point=fft1_size-1;
fft1_last_inband=fft1_size-1;
liminfo_amplitude_factor=1;
if( (ui.network_flag&NET_RXOUT_FFT1) == 0 &&
     genparm[SECOND_FFT_ENABLE] == 0 )
  {
// In case second fft is not enabled, there is no reason to calculate
// the spectrum outside the range that we use on the display when
// we do not need fft1 transforms for the network.
  fft1_first_point=wg_first_point;
  fft1_last_point=wg_last_point;
  }
else
  {
  if( (fft1_calibrate_flag&CALAMP)==CALAMP)
    {
    while(fft1_desired[fft1_first_inband]<0.5 && 
                          fft1_first_inband<fft1_size-1) fft1_first_inband++;
    while(fft1_desired[fft1_last_inband]<0.5 && 
                           fft1_last_inband > 0) fft1_last_inband--;
    if(fft1_last_inband-fft1_first_inband<fft1_size/32)lirerr(1057);
    t1=0;
    for(i=0;i<fft1_size; i++)
      {
      t1+=fft1_desired[i]*fft1_desired[i];
      }
    fft1_desired_totsum=t1;  
    }
  }
fft1_sumsq_recalc=fft1_first_point;
fft1_first_sym_point=fft1_size-1-fft1_last_point;
if(fft1_first_sym_point > fft1_first_point) 
                                     fft1_first_sym_point=fft1_first_point;
fft1_last_sym_point=fft1_size-1-fft1_first_sym_point;
}

void make_filcorrstart(void)
{
fft1_filtercorr_start=150*(float)fft1_size*(float)pow((double)(fft1_size),-0.4);
if( (ui.rx_input_mode&DWORD_INPUT) != 0)
  {
  fft1_filtercorr_start*=4096;
  if( fft_cntrl[FFT1_CURMODE].permute == 2)fft1_filtercorr_start*=16;
  if( (ui.rx_input_mode&IQ_DATA) != 0)
    {
    fft1_filtercorr_start*=12;
    }
  if( fft_cntrl[FFT1_CURMODE].real2complex != 0)fft1_filtercorr_start*=32 ;
  }
else
  {  
  if( fft_cntrl[FFT1_CURMODE].real2complex != 0)fft1_filtercorr_start*=2;
  }
fft1_filtercorr_start=(float)genparm[FIRST_FFT_GAIN]/fft1_filtercorr_start;
}

void clear_fft1_filtercorr(void)
{
float t1, t2, t3;
int i, j, k, mm;
fft1_calibrate_flag&=CALIQ;
settextcolor(8);
if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
  {
  for(i=3; i<20; i++)lir_text(40,i,"FILTERS NOT CALIBRATED!!");
  }
make_filcorrstart();
settextcolor(7);
mm=twice_rxchan;
for(i=0; i<fft1_size; i++)
  {
  fft1_desired[i]=1;
  for(j=0; j<mm; j+=2)  
    {
    fft1_filtercorr[mm*i+j  ]=fft1_filtercorr_start;
    fft1_filtercorr[mm*i+j+1]=0;
    }
  }  
// A DC offset on the A/D converters cause signals
// at frequency=0 and at frequency=fft1_size/2. 
// These frequencies are not properly separated
// by the backwards fft in case a window N other
// than 0 or 2 is used.
// Signals very close to the Nyquist frequency
// are artifacts anyway so we set the desired response
// to filter them out.
  {     
  t1=0.125F*(float)PI_L;
  t2=0;
  i=0;
  k=fft1_size-1;
  while(t2 < 0.5*PI_L)
    {
    t3=(float)(sin(t2)*sin(t2));
    if(t2<0)t3=0;
    if( (ui.rx_input_mode&IQ_DATA) != 0)
      {
      fft1_desired[i]=t3;
      for(j=0; j<mm; j+=2)fft1_filtercorr[mm*i+j  ]=t3*fft1_filtercorr_start;
      }
    fft1_desired[k]=t3;
    for(j=0; j<mm; j+=2)fft1_filtercorr[mm*k+j  ]=t3*fft1_filtercorr_start;
    t2+=t1;
    i++;
    k--;
    }
  }  
}

void clear_iq_foldcorr(void)
{
int i, mm;
settextcolor(8);
if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
  {
  for(i=3; i<20; i++)lir_text(10,i,"I/Q PHASE NOT CALIBRATED!!");
  }
mm=fft1_size*twice_rxchan;
for(i=0; i<mm; i++)fft1_foldcorr[i]=0;
fft1_calibrate_flag&=CALAMP;
settextcolor(7);
bal_updflag=1;
}

void use_iqcorr(void)
{
cal_table=malloc((unsigned int)fft1_size*sizeof(COSIN_TABLE )/2);
cal_permute=malloc((unsigned int)fft1_size*sizeof(int));
if( cal_table == NULL || cal_permute == NULL)
  {
  lirerr(1110);
  return;
  }
make_bigpermute(0, fft1_n, fft1_size, cal_permute);
make_sincos(0, fft1_size, cal_table); 
expand_foldcorr(fft1_foldcorr,fftw_tmp);
free(cal_table);
free(cal_permute);
if(DISREGARD_RAWFILE_IQCORR)
  {
  fft1_calibrate_flag&=CALAMP;
  }
else
  {  
  fft1_calibrate_flag|=CALIQ;
  }
}

void init_foldcorr(void)
{
int rdbuf[10],chkbuf[10];
int i, kk;
FILE *file;
char s[80];
#if SAVE_RAWFILE_IQCORR == TRUE
FILE *new_corr_file;
new_corr_file=NULL;
#endif
if(  (ui.rx_input_mode&DIGITAL_IQ) != 0 || (ui.rx_input_mode&IQ_DATA) == 0)
  {
  return;
  }
make_iqcorr_filename(s);
if(diskread_flag == 2 && (save_init_flag&2) == 2)
  {
  file=save_rd_file;
#if SAVE_RAWFILE_IQCORR == TRUE
  new_corr_file=fopen(s, "wb");
  if(new_corr_file == NULL)
    {
    lirerr(594222);
    return;
    }
#endif
  }
else
  {
  file = fopen(s, "rb");
  }
if (file == NULL)
  {
  clear_iq_foldcorr();
  return;
  }
else
  {
  i=(int)fread(rdbuf, sizeof(int),10,file);
  if(i != 10)goto corrupt;
#if SAVE_RAWFILE_IQCORR == TRUE
  if(new_corr_file != NULL)
    {
    i=fwrite(rdbuf, sizeof(int),10,new_corr_file);
    if(i != 10)
      {
new_corrfile_errx:;
      fclose(new_corr_file);
      lirerr(497326);
      return;
      }    
    }
#endif
  if( rdbuf[1] != (ui.rx_input_mode&IQ_DATA) ||
      rdbuf[2] != ui.rx_ad_speed ||
      rdbuf[3] != ui.rx_rf_channels ||
      rdbuf[4] != FOLDCORR_VERNR )
    {
    if(diskread_flag == 2 && (save_init_flag&2) == 2)
      {
corrupt:;
      lirerr(1060);
      return;
      }
    clear_iq_foldcorr();
    }
  else  
    {
    bal_segments=rdbuf[0];
    kk=twice_rxchan*4*bal_segments;
    contracted_iq_foldcorr=malloc((unsigned int)kk*sizeof(float));
    if(contracted_iq_foldcorr == NULL)
      {
      lirerr(1171);
      return;
      }
    if(kk != (int)fread(contracted_iq_foldcorr, sizeof(float), 
                                            (unsigned int)kk, file))goto err;
    for(i=0; i<kk; i++)
      {
      if(contracted_iq_foldcorr[i] > BIGFLOAT || 
         contracted_iq_foldcorr[i] < -BIGFLOAT)
        {
        lirerr(1202);
        return;
        }
      }
#if SAVE_RAWFILE_IQCORR == TRUE
    if(new_corr_file != NULL)
      {
      if(kk != (int)fwrite(contracted_iq_foldcorr, 1, kk,  new_corr_file))
                                                  goto new_corrfile_errx;
      }
#endif
    if(10 != fread(chkbuf, sizeof(int),10,file))goto err;
#if SAVE_RAWFILE_IQCORR == TRUE
    if(new_corr_file != NULL)
      {
      if(10 != fwrite(chkbuf, sizeof(int),10, new_corr_file))
                                                  goto new_corrfile_errx;
      }
#endif
    for(i=0; i<10; i++)
      {
      if(rdbuf[i]!=chkbuf[i])
        {
err:;      
        lirerr(1053);
        goto exx;
        }
      }
#if SAVE_RAWFILE_IQCORR == TRUE
    if(new_corr_file != NULL)fclose(new_corr_file);
#endif
    if(diskread_flag != 2 || (save_init_flag&2) == 0)fclose(file);
    use_iqcorr();
exx:;
    free(contracted_iq_foldcorr);
    }
  }
}


void resize_filtercorr_td_to_fd(int to_fd, int size_in, float *buf_in, 
                       int n_out, int size_out, float *buf_out)
{
int ka, kb;
int i, j, k, mm;
int mask, ia, ib;
double sum[4],isum[4];
D_COSIN_TABLE *out_ffttab; 
unsigned int *out_permute;
double *ww;
double dt3, dt4;
double dt1, dt2, dr1, dr2;
if(size_in < 8)lirerr(1161);
mm=twice_rxchan;
out_ffttab=malloc((unsigned int)size_out*sizeof(D_COSIN_TABLE )/2);
if( out_ffttab == NULL)
  {
mem_err1:;
  lirerr(1141);
  return;
  }
out_permute=malloc((unsigned int)size_out*sizeof(int));
if(out_permute == NULL)
  {
mem_err2:;  
  free(out_ffttab);
  goto mem_err1;
  }
ww=malloc((unsigned int)(mm*size_out)*sizeof(double));
if(ww == NULL)
  {
  free(out_permute);
  goto mem_err2;
  }
init_d_fft(0, n_out, size_out, out_ffttab, out_permute);
for(j=0; j<mm; j+=2)
  {
  if(size_out > size_in)
    {
// Increase the number of points from size_in to size_out.  
// Store the impulse response at buf_out[0]
// Fill zeroes in the undefined points.
    for(i=0; i<size_in/2; i++)
      {
      ww[2*i  ]=buf_in[mm*i+j  ];
      ww[2*i+1]=buf_in[mm*i+j+1];
      }
    for(i=size_in/2; i<size_in; i++)
      {
      k=i+size_out-size_in;
      ww[2*k  ]=buf_in[mm*i+j  ];
      ww[2*k+1]=buf_in[mm*i+j+1];
      }
    for(i=size_in/2; i<size_out-size_in/2; i++)
      {
      ww[2*i  ]=0;
      ww[2*i+1]=0;
      }
// The fourier transform has to be periodic. We have now introduced
// two discontinuities in case the pulse response is long enough
// to not have fallen off completely at the midpoint of the input waveform.
// To remove the discontinuity, make a smooth transition to
// the zero region using the periodicity of the transform.
    ia=size_in/2-1;
    ib=ia+1;
    ka=size_out-size_in/2;
    kb=ka-1;
    dt3=0.25*PI_L;
    dt4=10*dt3/size_in;
    while(dt3 >0 && ia >= 0)
      {
      dt1=ww[2*ia];
      dt2=ww[2*ka];
      dr1=pow(sin(dt3),2.0);
      dr2=pow(cos(dt3),2.0);
      ww[2*ib]=dr1*dt2;
      ww[2*kb]=dr1*dt1;
      ww[2*ia]=dr2*dt1+dr1*dt2;
      ww[2*ka]=dr1*dt1+dr2*dt2;
      dt1=ww[2*ia+1];
      dt2=ww[2*ka+1];
      ww[2*ib+1]=dr1*dt2;
      ww[2*kb+1]=dr1*dt1;
      ww[2*ia+1]=dr2*dt1+dr1*dt2;
      ww[2*ka+1]=dr1*dt1+dr2*dt2;
      ia--;
      ka++;
      ib++;
      kb--;
      dt3-=dt4;
      }
    }
  else
    {
    for(i=0; i<size_out/2; i++)
      {    
      ww[2*i  ]=buf_in[mm*i+j  ];
      ww[2*i+1]=buf_in[mm*i+j+1];
      }
    for(i=size_out/2; i<size_out; i++)
      {    
      k=size_in-size_out+i;
      ww[2*i  ]=buf_in[mm*k+j  ];
      ww[2*i+1]=buf_in[mm*k+j+1];
      }     
// When limiting the pulse response in the time domain, we may
// introduce a discontinuity. It will occur between size_out/2 and
// fft1/size/2-1.
// Differentiate the pulse response starting at the discontinuity:
    mask=size_out-1;
    ia=size_out/2;
    ib=ia+1;
    for(i=1; i<size_out; i++)
      {
      ww[ia*2  ]=ww[2*ia  ]-ww[2*ib  ];
      ww[ia*2+1]=ww[2*ia+1]-ww[2*ib+1];
      ia=(ia+1)&mask;
      ib=(ib+1)&mask;
      }  
    ww[ia*2  ]=0;
    ww[ia*2+1]=0;
    sum[j  ]=0;
    sum[j+1]=0;
// Get the DC component so we can remove it from the derivative
    for(i=0; i<size_out; i++)
      {
      sum[j  ]+=ww[2*i  ];
      sum[j+1]+=ww[2*i+1];
      }
    sum[j]/=size_out;
    isum[j]=0;
    sum[j+1]/=size_out;
    isum[j+1]=0;
// Integrate the pulse response:
    for(i=0; i<size_out; i++)
      {
      isum[j]+=ww[2*ia  ]-sum[j];
      ww[2*ia  ]=isum[j];
      isum[j+1]+=ww[2*ia+1]-sum[j+1];
      ww[2*ia+1]=isum[j+1];
      ia=(ia+mask)&mask;
      }
    }
// Take the back transform to get the calibration function in 
// size_out points in the frequency domain.    
  if(to_fd)d_fftback(size_out, n_out, ww, out_ffttab, out_permute);
  for(i=0; i<size_out; i++)
    {
    buf_out[mm*i+j  ]=(float)ww[2*i  ];
    buf_out[mm*i+j+1]=(float)ww[2*i+1];
    }
  }
free(out_ffttab);
free(out_permute);
free(ww);
}

void resize_fft1_desired(int siz_in, float *buf_in, 
                         int siz_out, float *buf_out)
{
int i, j, k, m;
float t1, t2;
if(siz_out > siz_in)
  {
  m=siz_out/siz_in;
  t2=0;
  for(j=siz_in; j>0; j--)
    {
    t1=t2;
    t2=buf_in[j-1];
    for(i=0; i<m; i++)
      {
      buf_out[m*j-i-1]=t1+(float)i*(t2-t1)/(float)m;
      }
    }
  buf_out[0]=0;
  }
else
  {
// Reduce buf_out.      
  m=siz_in/siz_out;
  i=m/2;
  buf_out[0]=0;
  for(j=1; j<siz_out; j++)
    {
    buf_out[j]=0;
    for(k=0; k<m; k++)
      {
      buf_out[j]+=buf_in[i];
      i++;
      }      
    buf_out[j]/=(float)m;
    }
  }  
}


void use_filtercorr_td(int cal_size, float *corr, float *desired)
{
int i, j, k, mm;
mm=twice_rxchan;
resize_filtercorr_td_to_fd(TRUE, 
                      cal_size, corr, fft1_n, fft1_size, fft1_filtercorr); 
resize_fft1_desired(cal_size, desired, fft1_size, fft1_desired);
i=0;
while(fft1_desired[i] == 0 && i<fft1_size)
  {
  for(j=0; j<mm; j++) fft1_filtercorr[mm*i+j]=0;
  i++;
  }
k=fft1_size-1;
while(fft1_desired[k] == 0 && k>=0)
  {
  for(j=0; j<mm; j++) fft1_filtercorr[mm*k+j]=0;
  k--;
  }
if(k-i < fft1_size/16+16)
  {
  lirerr(1259);
  return;
  }   
normalise_fft1_filtercorr(fft1_filtercorr);
make_filcorrstart();    
for(i=0; i<mm*fft1_size; i++)
  {
  fft1_filtercorr[i]*=fft1_filtercorr_start;
  } 
for(i=0; i<fft1_size; i++)wg_waterf_sum[i]=0.00001F;
fft1_calibrate_flag|=CALAMP;
}

void convert_filtercorr_fd_to_td(int n, int siz, float *buf)
{
int i, j, mm;
D_COSIN_TABLE *out_ffttab; 
unsigned int *out_permute;
double *ww;
mm=twice_rxchan;
out_ffttab=malloc((unsigned int)siz*sizeof(D_COSIN_TABLE )/2);
if( out_ffttab == NULL)
  {
mem_err1:;
  lirerr(1142);
  return;
  }
out_permute=malloc((unsigned int)siz*sizeof(int));
if(out_permute == NULL)
  {
mem_err2:;  
  free(out_ffttab);
  goto mem_err1;
  }
ww=malloc((unsigned int)(mm*siz)*sizeof(double));
if(ww == NULL)
  {
  free(out_permute);
  goto mem_err2;
  }
init_d_fft(0, n, siz, out_ffttab, out_permute);
for(j=0; j<mm; j+=2)
  {
  for(i=0; i<siz; i++)
    {
    ww[2*i  ]=buf[mm*i+j  ];
    ww[2*i+1]=buf[mm*i+j+1];
    }
  d_fftforward(siz, n, ww, out_ffttab, out_permute);
  for(i=0; i<siz; i++)
    {
    buf[mm*i+j  ]=(float)ww[2*i  ];
    buf[mm*i+j+1]=(float)ww[2*i+1];
    }
  }
}



void use_filtercorr_fd(int cal_n, int cal_size, 
                                          float *cal_corr, float *cal_desired)
{
convert_filtercorr_fd_to_td( cal_n, cal_size, cal_corr);
use_filtercorr_td(cal_size, cal_corr, cal_desired);
}

void init_fft1_filtercorr(void)
{
float *tmpbuf;
int rdbuf[10], chkbuf[10];
int i, mm;
FILE *file;
char s[80];
float t1;
#if DISREGARD_RAWFILE_CORR == TRUE
int disregard_flag;
#endif
#if SAVE_RAWFILE_CORR == TRUE
FILE *new_corr_file;
new_corr_file=NULL;
#endif
#if DISREGARD_RAWFILE_CORR == TRUE
#if SAVE_RAWFILE_CORR == TRUE
lirerr(713126);
#endif
disregard_flag=0;
#endif
file=NULL;
fft1_filtercorr_direction=1;
if( ui.rx_addev_no == NETWORK_DEVICE_CODE &&
               (ui.network_flag & NET_RXIN_TIMF2) != 0)
  {
  clear_fft1_filtercorr();
  return;
  }
mm=twice_rxchan;
make_filfunc_filename(s);
if(diskread_flag == 2 && (save_init_flag&1) == 1)
  {
  file=save_rd_file;
#if SAVE_RAWFILE_CORR == TRUE
  new_corr_file=fopen(s, "wb");
  if(new_corr_file == NULL)
    {
    lirerr(594221);
    return;
    }
#endif
  }
else
  {
  file = fopen(s, "rb");
  }
if (file == NULL)
  {
#if DISREGARD_RAWFILE_CORR == TRUE
disregard:;
#endif
  clear_fft1_filtercorr();
  return;
  }
i=(int)fread(rdbuf, sizeof(int),10,file);
if(i != 10)
  {
  goto calerr;
  }
#if SAVE_RAWFILE_CORR == TRUE
if(new_corr_file != NULL)
  {
  i=fwrite(rdbuf, sizeof(int),10,new_corr_file);
  if(i != 10)
    {
new_corrfile_errx:;
    fclose(new_corr_file);
    lirerr(497325);
    return;
    }    
  }
#endif
t1=(float)(rdbuf[5]-ui.rx_ad_speed);
t1=(float)fabs(200.0F*t1/(float)(rdbuf[5]+ui.rx_ad_speed));
// t1 is now the speed error in %.
if( rdbuf[2] <0 ||
    rdbuf[2] >= MAX_RX_MODE ||
    (rdbuf[3]&IQ_DATA) != (ui.rx_input_mode&IQ_DATA) ||
    t1 > 0.05 ||
    rdbuf[6] != ui.rx_rf_channels ||
    rdbuf[8] != 0 ||
    rdbuf[9] != 0 ) 
  {
calerr:;
  if(diskread_flag == 2 && (save_init_flag&1) == 1)
    {
    lirerr(1059);
    return;
    }
  fclose(file);  
  clear_fft1_filtercorr();
#if SAVE_RAWFILE_CORR == TRUE
if(new_corr_file != NULL)fclose(new_corr_file);
#endif
  return;
  }
if(rdbuf[7] == 0)
  {
  cal_fft1_n=rdbuf[0];
  cal_fft1_size=rdbuf[1];
// The filtercorr function is saved in the frequency domain in 
// rdbuf[1] points.
// This format is used during the initialisation procedures and
// it may be kept for normal operation in case the number of points
// will not be reduced by saving in the time domain.  
  tmpbuf=malloc((unsigned int)(rdbuf[1]*(mm+1))*sizeof(float));
  if( tmpbuf == NULL )
    {
    lirerr(1143);
    return;
    }
  i=(int)fread(tmpbuf, (unsigned int)mm*sizeof(float), 
                                               (unsigned int)rdbuf[1], file);
  if(i != rdbuf[1])
    {
    goto calerr;
    }
#if SAVE_RAWFILE_CORR == TRUE
  if(new_corr_file != NULL)
    {
    i=fwrite(tmpbuf, mm*sizeof(float), rdbuf[1], new_corr_file);
    if(i != rdbuf[1])goto new_corrfile_errx;
    }
#endif
  i=(int)fread(&tmpbuf[mm*rdbuf[1]], sizeof(float), (unsigned int)rdbuf[1], file);
  if(i != rdbuf[1])
    {
    goto calerr;
    }
#if SAVE_RAWFILE_CORR == TRUE
  if(new_corr_file != NULL)
    {
    i=fwrite(&tmpbuf[mm*rdbuf[1]], sizeof(float), rdbuf[1], new_corr_file);
    if(i != rdbuf[1])goto new_corrfile_errx;
    }
#endif
  use_filtercorr_fd(rdbuf[0], rdbuf[1], tmpbuf, &tmpbuf[mm*rdbuf[1]]);
  if(kill_all_flag)return;
  }
else
  {
// The correction function was stored in the time domain in rdbuf[7] points
  tmpbuf=malloc((unsigned int)(rdbuf[7]*(mm+1))*sizeof(float));
  if( tmpbuf == NULL )
    {
    lirerr(1292);
    return;
    }
  cal_fft1_size=rdbuf[7];
  cal_fft1_n=make_power_of_two(&cal_fft1_size);
  i=(int)fread(tmpbuf, (unsigned int)mm*sizeof(float), (unsigned int)rdbuf[7], file);
  if(i != rdbuf[7])
    {
    goto calerr;
    }
#if SAVE_RAWFILE_CORR == TRUE
  if(new_corr_file != NULL)
    {
    i=fwrite(tmpbuf, mm*sizeof(float), rdbuf[7], new_corr_file);
    if(i != rdbuf[7])goto new_corrfile_errx;
    }
#endif
  i=(int)fread(&tmpbuf[mm*rdbuf[7]], sizeof(float), (unsigned int)rdbuf[7], file);
  if(i != rdbuf[7])
    {
    goto calerr;
    }
#if SAVE_RAWFILE_CORR == TRUE
  if(new_corr_file != NULL)
    {
    i=fwrite(&tmpbuf[mm*rdbuf[7]], sizeof(float), rdbuf[7], new_corr_file);
    if(i != rdbuf[7])goto new_corrfile_errx;
    }
#endif
  use_filtercorr_td(rdbuf[7], tmpbuf, &tmpbuf[mm*rdbuf[7]]);
  if(kill_all_flag)return;
  }
if(10 != fread(chkbuf, sizeof(int),10,file))goto exx;
#if SAVE_RAWFILE_CORR == TRUE
  if(new_corr_file != NULL)
    {
    i=fwrite(chkbuf, sizeof(int),10, new_corr_file);
    if(i != 10)goto new_corrfile_errx;
    }
#endif
for(i=0; i<10; i++)
  {
  if(rdbuf[i]!=chkbuf[i])
    {
exx:;    
    if(diskread_flag==2)lirerr(1198); else lirerr(1052);
    return;
    }
  }
#if SAVE_RAWFILE_CORR == TRUE
if(new_corr_file != NULL)fclose(new_corr_file);
#endif
if(diskread_flag == 2 && (save_init_flag&1) == 1)
  {
#if DISREGARD_RAWFILE_CORR == TRUE
  disregard_flag++;
  if(disregard_flag < 2)
    {
    file = fopen(s, "rb");
    goto disregard;
    }
  fclose(file);
#endif
  return;
  }
fclose(file);
}

void clear_fft1_correlation(void)
  {
  char s[80];
  int i;
  memset(fft1_slowcorr,0,2*fft1_size*sizeof(double));
  memset(fft1_corrsum,0,2*fft1_sumsq_bufsize*sizeof(float));
  memset(fft1_slowcorr_tot,0,2*fft1_size*sizeof(double));
  if(slowcorr_tot_avgnum > 0)
    {
    sprintf(s,"%d",slowcorr_tot_avgnum);
    i=0;
    while(s[i] != 0)
      {
      s[i]=' ';
      i++;
      }
    lir_pixwrite(wg.xright-text_width*strlen(s)-2,wg.yborder+text_height,s);
    slowcorr_tot_avgnum=0;
    }
  correlation_reset_flag=fft1corr_reset_flag; 
  }
