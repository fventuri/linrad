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


void ttest(char c);

void fft1_reherm_dit_one(int timf1p_ref, float *out, float *tmp)
{
int m, k, n, nn;
int p0, pa, pb, pc;
int ia,ib;
n=fft1_size;
nn=2*fft1_size;
if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
  {
  m=timf1_bytemask/sizeof(short int);
  k=m+1;
  p0=timf1p_ref/sizeof(short int);
  p0=(p0-fft1_interleave_points*2+timf1_bytes)&m;
  pa=p0;
  if(genparm[FIRST_FFT_SINPOW] != 0)
    {
    ib=2*fft1_size-1;
    pb=(pa+ib)&m;
    for( ia=0; ia<n; ia++)
      {
      tmp[fft1_permute[ia  ]]=timf1_short_int[pa]*fft1_window[ia];
      tmp[fft1_permute[ib  ]]=timf1_short_int[pb]*fft1_window[ia];
      ib--; 
      pa=(pa+1)&m;
      pb=(pb-1+k)&m;
      }
    }
  else
    {
    for( ia=0; ia<nn; ia++)
      {
      tmp[fft1_permute[ia]]=timf1_short_int[pa];
      pa=(pa+1)&m;
      }
    }
  }
else
  {
  m=timf1_bytemask/sizeof(int);
  k=m+1;
  p0=timf1p_ref/sizeof(int);
  p0=(p0-fft1_interleave_points*2+timf1_bytes)&m;
  pa=p0;
  if(genparm[FIRST_FFT_SINPOW] != 0)
    {
    ib=2*fft1_size-1;
    pb=(pa+ib)&m;
    for( ia=0; ia<n; ia++)
      {
      tmp[fft1_permute[ia  ]]=timf1_int[pa]*fft1_window[ia];
      tmp[fft1_permute[ib  ]]=timf1_int[pb]*fft1_window[ia];
      ib--; 
      pa=(pa+1)&m;
      pb=(pb-1+k)&m;
      }
    }
  else
    {
    for( ia=0; ia<nn; ia++)
      {
      tmp[fft1_permute[ia]]=timf1_int[pa];
      pa=(pa+1)&m;
      }
    }
  }
fft_real_to_hermitian( tmp, 2*fft1_size, fft1_n, fft1tab);

// Output is {Re(z^[0]),...,Re(z^[n/2),Im(z^[n/2-1]),...,Im(z^[1]).
if(fft1_direction > 0)
  {
  out[1]=tmp[0];
  out[0]=tmp[fft1_size];
  k=fft1_first_point;
  m=fft1_last_point;
  if(k==0)k=1;
  pc=2*k;
  for(ia=k; ia <= m; ia++)
    {
    out[pc+1]=tmp[ia  ];
    out[pc  ]=tmp[2*fft1_size-ia];
    pc+=2;
    }
  }
else
  {
  pc=2*(fft1_size-1);
  out[pc  ]=tmp[0];
  out[pc+1]=tmp[fft1_size];
  k=fft1_size-1-fft1_last_point;
  m=1+k+fft1_last_point-fft1_first_point;
  if(k==0)k=1;
  pc-=2*(k-1);
  for(ia=k; ia <= m; ia++)
    {
    out[pc  ]=tmp[ia  ];
    out[pc+1]=tmp[2*fft1_size-ia];
    pc-=2;
    }
  }
}

void fft1_reherm_dit_two(int timf1p_ref, float *out, float *tmp)
{
int chan, m, k;
int p0, pa, pb, pc;
int ia,ib;
for(chan=0; chan<2; chan++)
  {
  if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    m=timf1_bytemask/sizeof(short int);
    k=m+1;
    p0=timf1p_ref/sizeof(short int);
    p0=(p0-fft1_interleave_points*4+timf1_bytes)&m;
    pa=p0+chan;
    if(genparm[FIRST_FFT_SINPOW] != 0)
      {
      ib=2*fft1_size-1;
      pb=(pa+2*ib)&m;
      for( ia=0; ia<fft1_size; ia++)
        {
        tmp[fft1_permute[ia  ]]=timf1_short_int[pa]*fft1_window[ia];
        tmp[fft1_permute[ib  ]]=timf1_short_int[pb]*fft1_window[ia];
        ib--; 
        pa=(pa+2)&m;
        pb=(pb-2+k)&m;
        }
      }
    else
      {
      for( ia=0; ia<2*fft1_size; ia++)
        {
        tmp[fft1_permute[ia]]=timf1_short_int[pa];
        pa=(pa+2)&m;
        }
      }
    }
  else
    {
    m=timf1_bytemask/sizeof(int);
    k=m+1;
    p0=timf1p_ref/sizeof(int);
    p0=(p0-fft1_interleave_points*4+timf1_bytes)&m;
    pa=p0+chan;  
    if(genparm[FIRST_FFT_SINPOW] != 0)
      {
      ib=2*fft1_size-1;
      pb=(pa+2*ib)&m;
      for( ia=0; ia<fft1_size; ia++)
        {
        tmp[fft1_permute[ia  ]]=timf1_int[pa]*fft1_window[ia];
        tmp[fft1_permute[ib  ]]=timf1_int[pb]*fft1_window[ia];
        ib--; 
        pa=(pa+2)&m;
        pb=(pb-2+k)&m;
        }
      }
    else
      {
      for( ia=0; ia<2*fft1_size; ia++)
        {
        tmp[fft1_permute[ia]]=timf1_int[pa];
        pa=(pa+2)&m;
        }
      }
    }
  fft_real_to_hermitian( tmp, 2*fft1_size, fft1_n, fft1tab);
  if(fft1_direction > 0)
    {
    pc=2*chan;
    out[pc+1]=tmp[0];
    out[pc  ]=tmp[fft1_size];
    k=fft1_first_point;
    m=fft1_last_point;
    if(k==0)k=1;
    pc+=4*k;
    for(ia=k; ia <= m; ia++)
      {
      out[pc+1]=tmp[ia  ];
      out[pc  ]=tmp[2*fft1_size-ia];
      pc+=4;
      }
    }
  else
    {
    pc=2*chan+4*(fft1_size-1);
    out[pc  ]=tmp[0];
    out[pc+1]=tmp[fft1_size];
    k=fft1_size-1-fft1_last_point;
    m=1+k+fft1_last_point-fft1_first_point;
    if(k==0)k=1;
    pc-=4*(k-1);
    for(ia=k; ia <= m; ia++)
      {
      out[pc  ]=tmp[ia  ];
      out[pc+1]=tmp[2*fft1_size-ia];
      pc-=4;
      }
    }
  }
}
