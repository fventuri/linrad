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
#include "seldef.h"
#include "fft2def.h"
#include "fft1def.h"
#include "screendef.h"
#include "sigdef.h"
#include "thrdef.h"

#if CPU == CPU_INTEL
void fft2mmx_a2_nowin(void);
void fft2mmx_a2_win(void);
void fft2mmx_a1_nowin(void);
void fft2mmx_a1_win(void);
void fft2_mmx_b1hi(void);
void fft2_mmx_b2hi(void);
void fft2_mmx_b1med(void);
void fft2_mmx_b2med(void);
void fft2_mmx_b1low(void);
void fft2_mmx_b2low(void);
void fft2_mmx_c1(void);
void fft2_mmx_c2(void);
#endif
void update_wg_waterf(void);


void make_fft2(void)
{
int y;
float yval;
int ja, jb, jc, jd;
int i, ix, mm, m;
float t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12;
float r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12;
float a1,a2,a3,a4;
float b1,b2,b3,b4;
float c1,c2,c3,c4;
float d1,d2,d3,d4;
float der;
int ia, ib, ic, id, itab;
int inc,j,k,n,n1,n2;
int m2, siz,siz_d4,siz_m4,siz_m2,siz_d2;
float u0,u1,u2,u3,u4,u5,u6,u7;
float w0,w1,w2,w3,w4,w5,w6,w7;
float x0,x1,x2,x3,x4,x5,x6,x7;
float y8,y9,y2,y3,y4,y5,y6,y7;
float ss1,ss2,cs1,cs2;
float *z,*fftxy, *fftab, *pwra;
TWOCHAN_POWER *ya;
int p0, mask;
#if CPU == CPU_INTEL
short int *zxy;
#endif
// The second fft may be very large. To produce optimum sensitivity
// bandwidths below 1Hz may be desireable. 
// Every time this routine is called it will do just a fraction of
// the complete transform as indicated by make_fft2_status.
siz=fft2_size;
fftxy=&fft2_float[fft2_pa];
z=(float*)(fftxy);
if(sw_onechan)
  {
  switch (make_fft2_status)
    {
    case FFT2_NOT_ACTIVE:
    mask=timf2_mask;
    if(FFT2_CURMODE == 15)
      {
      mask=timf2_mask;
      p0=timf2_px;
      if(genparm[SECOND_FFT_SINPOW] != 0)
        {
        if(swfloat)
          {
          for(i=0; i<siz; i++)
            {
            z[2*i  ]=fft2_window[i]*(timf2_float[p0  ]+timf2_float[p0+2]);
            z[2*i+1]=fft2_window[i]*(timf2_float[p0+1]+timf2_float[p0+3]);
            p0=(p0+4)&mask;            
            }
          }
        else
          {  
          for(i=0; i<siz; i++)
            {
            z[2*i  ]=fft2_window[i]*(timf2_shi[p0  ]+timf2_shi[p0+2]);
            z[2*i+1]=fft2_window[i]*(timf2_shi[p0+1]+timf2_shi[p0+3]);
            p0=(p0+4)&mask;            
            }
          }
        }  
      else
        {
        if(swfloat)
          {
          for(i=0; i<siz; i++)
            {
            z[2*i  ]=(timf2_float[p0  ]+timf2_float[p0+2]);
            z[2*i+1]=(timf2_float[p0+1]+timf2_float[p0+3]);
            p0=(p0+4)&mask;            
            }
          }
        else
          {
          for(i=0; i<siz; i++)
            {
            z[2*i  ]=(timf2_shi[p0  ]+timf2_shi[p0+2]);
            z[2*i+1]=(timf2_shi[p0+1]+timf2_shi[p0+3]);
            p0=(p0+4)&mask;            
            }
          }
        }
      big_fftforward(siz, fft2_n, z,
                            fft2_tab, fft2_bigpermute, yieldflag_fft2_fft2);
      make_fft2_status=FFT2_ELIMINATE_SPURS;
      return;
      }
    fft2_chunk_counter=2;
    fft2_inc=siz/16;
    fft2_m1=32;
    fft2_m2=8;
    if(FFT2_CURMODE == 6)
      {
      make_fft2_status=FFT2_B;
      }
    else
      {  
// Use MMX routines
#if CPU == CPU_INTEL
      if(genparm[SECOND_FFT_SINPOW] == 0)
        {
        fft2mmx_a1_nowin(); 
        }
      else
        {
        fft2mmx_a1_win(); 
        }
#else
      lirerr(995346);
#endif
      make_fft2_status=FFT2_MMXB;
      return;
      }
    p0=timf2_px;
    if(genparm[SECOND_FFT_SINPOW] == 0)
      {
      if(swfloat)
        {
        for(j=0; j<siz; j+=4)
          {
          ia=(p0+4*fft2_bigpermute[j  ])&mask;
          ib=(p0+4*fft2_bigpermute[j+1])&mask;
          ic=(p0+4*fft2_bigpermute[j+2])&mask;
          id=(p0+4*fft2_bigpermute[j+3])&mask;

          t1=  timf2_float[ia  ]+timf2_float[ib  ]+
               timf2_float[ia+2]+timf2_float[ib+2];
          t2=-(timf2_float[ia+1]+timf2_float[ib+1]+
               timf2_float[ia+3]+timf2_float[ib+3]);
  
          t3=  timf2_float[ic  ]+timf2_float[id  ]+
               timf2_float[ic+2]+timf2_float[id+2];
          t4=-(timf2_float[ic+1]+timf2_float[id+1]+
               timf2_float[ic+3]+timf2_float[id+3]);
  
          t5=  timf2_float[ia  ]-timf2_float[ib  ]+
               timf2_float[ia+2]-timf2_float[ib+2];
          t7=-(timf2_float[ia+1]-timf2_float[ib+1]+
               timf2_float[ia+3]-timf2_float[ib+3]);
  
          t10=  timf2_float[ic  ]-timf2_float[id  ]+
                timf2_float[ic+2]-timf2_float[id+2];
          t6= -(timf2_float[ic+1]-timf2_float[id+1]+
                timf2_float[ic+3]-timf2_float[id+3]);
  
          z[2*j  ]=t1+t3;
          z[2*j+1]=t2+t4;
  
          z[2*j+4]=t1-t3;
          z[2*j+5]=t2-t4;
  
          t11=t5-t6;
          t8=t7-t10;
    
          t12=t5+t6;
          t9=t7+t10;
  
          z[2*j+2]=t12;
          z[2*j+3]=t8;
  
          z[2*j+6]=t11;
          z[2*j+7]=t9;
          }
        }
      else
        {  
        for(j=0; j<siz; j+=4)
          {
          ia=(p0+4*fft2_bigpermute[j  ])&mask;
          ib=(p0+4*fft2_bigpermute[j+1])&mask;
          ic=(p0+4*fft2_bigpermute[j+2])&mask;
          id=(p0+4*fft2_bigpermute[j+3])&mask;

          t1=(float)(timf2_shi[ia  ])+(float)(timf2_shi[ib  ])+
             (float)(timf2_shi[ia+2])+(float)(timf2_shi[ib+2]);
          t2=-((float)(timf2_shi[ia+1])+(float)(timf2_shi[ib+1])+
              (float)(timf2_shi[ia+3])+(float)(timf2_shi[ib+3]));
  
          t3=(float)(timf2_shi[ic  ])+(float)(timf2_shi[id  ])+
             (float)(timf2_shi[ic+2])+(float)(timf2_shi[id+2]);
          t4=-((float)(timf2_shi[ic+1])+(float)(timf2_shi[id+1])+
              (float)(timf2_shi[ic+3])+(float)(timf2_shi[id+3]));
  
          t5=(float)(timf2_shi[ia  ])-(float)(timf2_shi[ib  ])+
             (float)(timf2_shi[ia+2])-(float)(timf2_shi[ib+2]);
          t7=-((float)(timf2_shi[ia+1])-(float)(timf2_shi[ib+1])+
              (float)(timf2_shi[ia+3])-(float)(timf2_shi[ib+3]));
  
          t10=(float)(timf2_shi[ic  ])-(float)(timf2_shi[id  ])+
              (float)(timf2_shi[ic+2])-(float)(timf2_shi[id+2]);
          t6= -((float)(timf2_shi[ic+1])-(float)(timf2_shi[id+1])+
               (float)(timf2_shi[ic+3])-(float)(timf2_shi[id+3]));
  
          z[2*j  ]=t1+t3;
          z[2*j+1]=t2+t4;
  
          z[2*j+4]=t1-t3;
          z[2*j+5]=t2-t4;
  
          t11=t5-t6;
          t8=t7-t10;
    
          t12=t5+t6;
          t9=t7+t10;
  
          z[2*j+2]=t12;
          z[2*j+3]=t8;
  
          z[2*j+6]=t11;
          z[2*j+7]=t9;
          }
        }
      }
    else
      {
      if(swfloat)
        {
        for(j=0; j<siz; j+=4)
          {
          ja=fft2_bigpermute[j  ];
          jb=fft2_bigpermute[j+1];
          jc=fft2_bigpermute[j+2];
          jd=fft2_bigpermute[j+3];
          ia=(p0+4*ja)&mask;
          ib=(p0+4*jb)&mask;
          ic=(p0+4*jc)&mask;
          id=(p0+4*jd)&mask;
          a1=fft2_window[ja]*(timf2_float[ia  ]+timf2_float[ia+2]);
          a2=fft2_window[ja]*(timf2_float[ia+1]+timf2_float[ia+3]);

          b1=fft2_window[jb]*(timf2_float[ib  ]+timf2_float[ib+2]);
          b2=fft2_window[jb]*(timf2_float[ib+1]+timf2_float[ib+3]);
 
          c1=fft2_window[jc]*(timf2_float[ic  ]+timf2_float[ic+2]);
          c2=fft2_window[jc]*(timf2_float[ic+1]+timf2_float[ic+3]);

          d1=fft2_window[jd]*(timf2_float[id  ]+timf2_float[id+2]);
          d2=fft2_window[jd]*(timf2_float[id+1]+timf2_float[id+3]);

          t1=a1+b1;
          t2=-(a2+b2);
  
          t3=c1+d1;
          t4=-(c2+d2);
  
          t5=a1-b1;
          t7=-(a2-b2);
  
          t10=c1-d1;
          t6=-( c2-d2);
          z[2*j  ]=t1+t3;
          z[2*j+1]=t2+t4;
  
          z[2*j+4]=t1-t3;
          z[2*j+5]=t2-t4;
  
          t11=t5-t6;
          t8=t7-t10;
    
          t12=t5+t6;
          t9=t7+t10;

          z[2*j+2]=t12;
          z[2*j+3]=t8;
  
          z[2*j+6]=t11;
          z[2*j+7]=t9;
          }
        }
      else
        {
        for(j=0; j<siz; j+=4)
          {
          ja=fft2_bigpermute[j  ];
          jb=fft2_bigpermute[j+1];
          jc=fft2_bigpermute[j+2];
          jd=fft2_bigpermute[j+3];
          ia=(p0+4*ja)&mask;
          ib=(p0+4*jb)&mask;
          ic=(p0+4*jc)&mask;
          id=(p0+4*jd)&mask;
          a1=fft2_window[ja]*((float)(timf2_shi[ia  ])+(float)(timf2_shi[ia+2]));
          a2=fft2_window[ja]*((float)(timf2_shi[ia+1])+(float)(timf2_shi[ia+3]));

          b1=fft2_window[jb]*((float)(timf2_shi[ib  ])+(float)(timf2_shi[ib+2]));
          b2=fft2_window[jb]*((float)(timf2_shi[ib+1])+(float)(timf2_shi[ib+3]));
 
          c1=fft2_window[jc]*((float)(timf2_shi[ic  ])+(float)(timf2_shi[ic+2]));
          c2=fft2_window[jc]*((float)(timf2_shi[ic+1])+(float)(timf2_shi[ic+3]));

          d1=fft2_window[jd]*((float)(timf2_shi[id  ])+(float)(timf2_shi[id+2]));
          d2=fft2_window[jd]*((float)(timf2_shi[id+1])+(float)(timf2_shi[id+3]));

          t1=a1+b1;
          t2=-(a2+b2);
  
          t3=c1+d1;
          t4=-(c2+d2);
  
          t5=a1-b1;
          t7=-(a2-b2);
  
          t10=c1-d1;
          t6=-( c2-d2);
          z[2*j  ]=t1+t3;
          z[2*j+1]=t2+t4;
  
          z[2*j+4]=t1-t3;
          z[2*j+5]=t2-t4;
  
          t11=t5-t6;
          t8=t7-t10;
    
          t12=t5+t6;
          t9=t7+t10;

          z[2*j+2]=t12;
          z[2*j+3]=t8;
  
          z[2*j+6]=t11;
          z[2*j+7]=t9;
          }
        }
      }  
    return;

    case FFT2_MMXB:
#if CPU == CPU_INTEL
    n1=fft2_chunk_counter;
    n2=n1+fft2_chunk_n;
    if(n2 >= fft2_n-2)
      {
      n2=fft2_n-2;
      make_fft2_status=FFT2_MMXC;
      }
    fft2_chunk_counter=n2;  
    for(n=n1; n<n2; n+=2)
      {
      if(n < fft2_att_limit)
        {
        fft2_mmx_b1hi();
        }
      else
        {  
        if(n == fft2_att_limit)
          {
          fft2_mmx_b1med();
          }
        else
          {  
          fft2_mmx_b1low();
          }
        }         
      }
#else
    lirerr(995346);
      
#endif
    return;

    case FFT2_MMXC:
#if CPU == CPU_INTEL
    fft2_mmx_c1();
    if(no_of_spurs > 0)
      {
      fftx_na=fft2_na;
      eliminate_spurs();
      }
// Make summed power spectrum.
    zxy=&fft2_short_int[fft2_pa];
    pwra=&fft2_power_float[fft2_na*siz];
    if(ampinfo_flag != 0)
      {
      for(i=0; i<siz; i++)
        {
        k=zxy[2*i  ];
        k=abs(k);
        if(k > fft2_maxamp[0])fft2_maxamp[0]=k;
        k=zxy[2*i+1];
        k=abs(k);
        if(k > fft2_maxamp[0])fft2_maxamp[0]=k;
        }
      }
    if(wg_waterf_sum_counter == 0)
      {
      for(i=0; i<siz; i++)
        {
        pwra[i]=(float)(zxy[2*i  ])*(float)(zxy[2*i  ])+
                (float)(zxy[2*i+1])*(float)(zxy[2*i+1]);
        fft2_powersum_float[i]=pwra[i];
        }
      }
    else
      {
      for(i=0; i<siz; i++)
        {
        pwra[i]=(float)(zxy[2*i  ])*(float)(zxy[2*i  ])+
                (float)(zxy[2*i+1])*(float)(zxy[2*i+1]);
        fft2_powersum_float[i]+=pwra[i];
        }
      }
#else
    pwra=NULL;
    lirerr(995347);
#endif
    goto one_finish;
    
    case FFT2_B:
    siz_d4=siz/4;
    siz_m2=siz*2;
    n1=fft2_chunk_counter;
    n2=n1+fft2_chunk_n;
    if(n2 >= fft2_n-2)
      {
      n2=fft2_n-2;
      make_fft2_status=FFT2_C;
      }
    fft2_chunk_counter=n2;  
    for(n=n1; n<n2; n+=2)
      {
      inc=fft2_inc;
      m2=fft2_m2;
      for(j=0; j<siz_m2; j+=fft2_m1)
        {
        itab=0;
        ia=j;
lp1:;    
        ib=ia+m2;
        ss1= fft2_tab[2*itab].sin; 
        cs1= fft2_tab[2*itab].cos;

        x0=z[ia  ];
        y8=z[ia+1];

        x1=z[ib  ];
        y9=z[ib+1];

        t1=cs1*x1+ss1*y9;      
        t2=cs1*y9-ss1*x1;      

        ic=ib+m2;
        id=ic+m2;

        x4=x0+t1;
        y4=y8+t2;

        x6=x0-t1;
        y6=y8-t2;

        x2=z[ic  ];
        y2=z[ic+1];

        x3=z[id  ];
        y3=z[id+1];

        t3=cs1*x3+ss1*y3;      
        t4=cs1*y3-ss1*x3;      

        x5=x2+t3;
        y5=y2+t4;

        x7=x2-t3;
        y7=y2-t4;

        ss2= fft2_tab[itab].sin; 
        cs2= fft2_tab[itab].cos;

        t5=cs2*x5+ss2*y5;
        t6=cs2*y5-ss2*x5;

        z[ia  ]=x4+t5;
        z[ia+1]=y4+t6;

        z[ic  ]=x4-t5;
        z[ic+1]=y4-t6;

        t8=cs2*x7+ss2*y7;
        t7=cs2*y7-ss2*x7;

        z[ib  ]=x6+t7;
        z[ib+1]=y6-t8;

        z[id  ]=x6-t7;
        z[id+1]=y6+t8;

        itab+=inc;
        ia+=2; 
        if(itab<siz_d4)goto lp1;
        }
      fft2_inc/=4;
      fft2_m1*=4;
      fft2_m2*=4;
      }
    return;

    case FFT2_C:
    if( (fft2_n&1)==1)
      {
      siz_d2=siz/2;
      ib=siz;
      ia=0;
      for(itab=0; itab<siz_d2; itab++)
        {
        ss1= fft2_tab[itab].sin; 
        cs1= fft2_tab[itab].cos;
        x0=z[ia  ];
        y8=z[ia+1];
        x1=z[ib  ];
        y9=z[ib+1];
        t1=cs1*x1+ss1*y9;
        t2=cs1*y9-ss1*x1;
        z[ib  ]=x0-t1;
        z[ib+1]=-(y8-t2);
        z[ia  ]=x0+t1;
        z[ia+1]=-(y8+t2);
        ia+=2;
        ib+=2;
        }
      }
    else
      {
      siz_d4=siz/4;
      m2=siz/2;
      itab=0;
      ia=0;
lp3:;    
      ib=ia+m2;
      ss1= fft2_tab[2*itab].sin; 
      cs1= fft2_tab[2*itab].cos;

      x0=z[ia  ];
      y8=z[ia+1];

      x1=z[ib  ];
      y9=z[ib+1];

      t1=cs1*x1+ss1*y9;      
      t2=cs1*y9-ss1*x1;      

      ic=ib+m2;
      id=ic+m2;

      x4=x0+t1;
      y4=y8+t2;

      x6=x0-t1;
      y6=y8-t2;

      x2=z[ic  ];
      y2=z[ic+1];

      x3=z[id  ];
      y3=z[id+1];

      t3=cs1*x3+ss1*y3;      
      t4=cs1*y3-ss1*x3;      

      x5=x2+t3;
      y5=y2+t4;

      x7=x2-t3;
      y7=y2-t4;

      ss2= fft2_tab[itab].sin; 
      cs2= fft2_tab[itab].cos;

      t5=cs2*x5+ss2*y5;
      t6=cs2*y5-ss2*x5;

      z[ia  ]=x4+t5;
      z[ia+1]=-(y4+t6);

      z[ic  ]=x4-t5;
      z[ic+1]=-(y4-t6);

      t8=cs2*x7+ss2*y7;
      t7=cs2*y7-ss2*x7;

      z[ib  ]=x6+t7;
      z[ib+1]=-(y6-t8);

      z[id  ]=x6-t7;
      z[id+1]=-(y6+t8);

      itab++;
      ia+=2; 
      if(itab<siz_d4)goto lp3;
      }
    make_fft2_status=FFT2_ELIMINATE_SPURS;
    return;

    case FFT2_ELIMINATE_SPURS:
    if(no_of_spurs > 0)
      {
      fftx_na=fft2_na;
      eliminate_spurs();
      }
// Make summed power spectrum.
    pwra=&fft2_power_float[fft2_na*siz];
    if(wg_waterf_sum_counter == 0)
      {
      for(i=0; i<siz; i++)
        {
        pwra[i]=z[2*i  ]*z[2*i  ]+z[2*i+1]*z[2*i+1];
        fft2_powersum_float[i]=pwra[i];
        }
      }
    else
      {
      for(i=0; i<siz; i++)
        {
        pwra[i]=z[2*i  ]*z[2*i  ]+z[2*i+1]*z[2*i+1];
        fft2_powersum_float[i]+=pwra[i];
        }
      } 
one_finish:;      
    wg_waterf_sum_counter++;  
    if(genparm[MAX_NO_OF_SPURS] > 0)
      {
      if(spursearch_sum_counter > 3*spur_speknum)
        {
        spursearch_sum_counter=0;
        for(i=spur_search_first_point; i <= spur_search_last_point; i++)
          {
          spursearch_spectrum[i]=spursearch_powersum[i]+pwra[i];
          }
        spursearch_spectrum_cleanup();
        }
      else
        {
        if(spursearch_sum_counter == 0)
          {
          for(i=spur_search_first_point; i <= spur_search_last_point; i++)
            {
            spursearch_powersum[i]=pwra[i];
            }
          }
        else
          {
          for(i=spur_search_first_point; i <= spur_search_last_point; i++)
            {
            spursearch_powersum[i]+=pwra[i];
            }
          }
        spursearch_sum_counter++;
        }
      }
    if(wg_waterf_sum_counter < wg.waterfall_avgnum)break;
    make_fft2_status=FFT2_WATERFALL_LINE;
    return;
   
    case FFT2_WATERFALL_LINE:    
// A new summed power spectrum has arrived.
// Convert it from linear power scale to log in units of 0.1dB.
// Expand or contract so we get the number of pixels that
// will fit on the screen.
// Store at the pos of our current line in waterfall
    a2=1;
    if(wg.xpoints_per_pixel >0)
      {
      a2=wg.xpoints_per_pixel;
      }
    else
      {
      a2=1./wg.pixels_per_xpoint;
      }
    a3=wg.first_xpoint+0.5*a2;
    if(hgwat_xpoints_per_pixel == 1 || hgwat_pixels_per_xpoint == 1)
      {
      i=hgwat_first_xpoint;
      for(ix=0; ix < wg_xpixels; ix++)
        {
        itab=a3;
        a3+=a2;
        y=1000.*log10(fft2_powersum_float[i]*wg_waterf_yfac[itab]);
        if(y < -32767)y=-32767;
        if(y>32767)y=32767;
        wg_waterf[wg_waterf_ptr+ix]=y;
        i++;
        }
      }    
    else
      {
      if(hgwat_xpoints_per_pixel == 0)
        {
// There are more pixels than data points so we must interpolate.
        i=hgwat_first_xpoint;
        itab=a3;
        a3+=a2;
        y=1000.*log10(fft2_powersum_float[i]*wg_waterf_yfac[itab]);
        yval=y;
        if(y < -32767)y=-32767;
        if(y>32767)y=32767;
        wg_waterf[wg_waterf_ptr]=y;
        i++;
        m=wg_xpixels-hgwat_pixels_per_xpoint;
        for(ix=0; ix<m; ix+=hgwat_pixels_per_xpoint)
          {
          itab=a3;
          a3+=a2;
          r1=1000.*log10(fft2_powersum_float[i]*wg_waterf_yfac[itab]);
          der=(r1-yval)/hgwat_pixels_per_xpoint;
          for(k=ix+1; k<=ix+hgwat_pixels_per_xpoint; k++)
            {
            yval=yval+der;
            y=yval;
            if(y < -32767)y=-32767;
            if(y>32767)y=32767;
            wg_waterf[wg_waterf_ptr+k]=y;
            }  
          yval=r1;
          i++;
          }      
        if(i<fft2_size)
          {
          itab=a3;
          r1=1000.*log10(fft2_powersum_float[i]*wg_waterf_yfac[itab]);
          der=(r1-yval)/hgwat_pixels_per_xpoint;
          for(k=ix+1; k<=ix+hgwat_pixels_per_xpoint; k++)
            {
            yval=yval+der;
            y=yval;
            if(y < -32767)y=-32767;
            if(y>32767)y=32767;
            wg_waterf[wg_waterf_ptr+k]=y;
            }  
          }
        }
      else
        {
// There is more than one data point for each pixel.
// Make a region ia to ib for the points behind each pixel.
// find the largest value for each region.
        ia=hgwat_first_xpoint;
        ib=ia+hgwat_xpoints_per_pixel;
        for(ix=0; ix<wg_xpixels; ix++)
          {
          r2=0;
          k=0;
          for(i=ia; i<ib; i++)
            {    
            r1=fft2_powersum_float[i];
            if(r1 > r2)r2=r1;
            }
          itab=a3;
          a3+=a2;
          a1=wg_waterf_yfac[itab];
          y=1000.*log10(a1*r2);
          if(y < -32767)y=-32767;
          if(y>32767)y=32767;
          wg_waterf[wg_waterf_ptr+ix]=y;
          ia=ib;
          ib+=hgwat_xpoints_per_pixel;
          if(ib >= siz)ib=siz;
          }
        }  
      }
    update_wg_waterf();
    fft2_liminfo_cnt++;
    break;
    default:
    break;
    }
  }
else
  {
  
  switch (make_fft2_status)
    {
    case FFT2_NOT_ACTIVE:
    mask=timf2_mask;
    if(FFT2_CURMODE == 15)
      {
      ya=&fft2_xypower[fft2_na*siz];
      mask=timf2_mask;
      mm=twice_rxchan;
      for(j=0; j<mm; j+=2)
        {
        p0=timf2_px;
        if(genparm[SECOND_FFT_SINPOW] != 0)
          {
          if(swfloat)
            {
            for(i=0; i<siz; i++)
              {
              fftxy[2*i  ]=fft2_window[i]*(timf2_float[p0+j  ]+timf2_float[p0+j+4]);
              fftxy[2*i+1]=fft2_window[i]*(timf2_float[p0+j+1]+timf2_float[p0+j+5]);
              p0=(p0+8)&mask;            
              }
            }
          else
            {
            for(i=0; i<siz; i++)
              {
              fftxy[2*i  ]=fft2_window[i]*(timf2_shi[p0+j  ]+timf2_shi[p0+j+4]);
              fftxy[2*i+1]=fft2_window[i]*(timf2_shi[p0+j+1]+timf2_shi[p0+j+5]);
              p0=(p0+8)&mask;            
              }
            }
          }  
        else
          {
          if(swfloat)
            {
            for(i=0; i<siz; i++)
              {
              fftxy[2*i  ]=timf2_float[p0+j  ]+timf2_float[p0+j+4];
              fftxy[2*i+1]=timf2_float[p0+j+1]+timf2_float[p0+j+5];
              p0=(p0+8)&mask;            
              }
            }  
          else
            {
            for(i=0; i<siz; i++)
              {
              fftxy[2*i  ]=timf2_shi[p0+j  ]+timf2_shi[p0+j+4];
              fftxy[2*i+1]=timf2_shi[p0+j+1]+timf2_shi[p0+j+5];
              p0=(p0+8)&mask;            
              }
            }  
          }
        big_fftforward(siz, fft2_n, fftxy, 
                           fft2_tab, fft2_bigpermute, yieldflag_fft2_fft2);
        fftxy=(float*)(ya);
        }
      fftxy=&fft2_float[fft2_pa];
      fftab=(float*)(ya);
      for(i=siz-1; i>=0; i--)
        {
        fftxy[4*i+3]=fftab[2*i+1];    
        fftxy[4*i+2]=fftab[2*i  ];    
        fftxy[4*i+1]=fftxy[2*i+1];    
        fftxy[4*i  ]=fftxy[2*i  ];    
        }
      make_fft2_status=FFT2_ELIMINATE_SPURS;
      return;
      }
    fft2_chunk_counter=2;
    fft2_inc=siz/16;
    fft2_m1=64;
    fft2_m2=16;
    if(FFT2_CURMODE == 16)
      {
      make_fft2_status=FFT2_B;
      }
    else  
      {
// Use MMX routines
#if CPU == CPU_INTEL
      if(genparm[SECOND_FFT_SINPOW] == 0)
        {
        fft2mmx_a2_nowin(); 
        }
      else
        {
        fft2mmx_a2_win(); 
        }
#else
    lirerr(995348);
#endif
      make_fft2_status=FFT2_MMXB;
      return;
      }

    p0=timf2_px;

    if(genparm[SECOND_FFT_SINPOW] == 0)
      {
      if(swfloat)
        {
        for(j=0; j<siz; j+=4)
          {
          ia=(p0+8*fft2_bigpermute[j  ])&mask;
          ib=(p0+8*fft2_bigpermute[j+1])&mask;
          ic=(p0+8*fft2_bigpermute[j+2])&mask;
          id=(p0+8*fft2_bigpermute[j+3])&mask;
  
          t1=  timf2_float[ia  ]+timf2_float[ib  ]+
               timf2_float[ia+4]+timf2_float[ib+4];
          t2=-(timf2_float[ia+1]+timf2_float[ib+1]+
               timf2_float[ia+5]+timf2_float[ib+5]);
          r1=  timf2_float[ia+2]+timf2_float[ib+2]+
               timf2_float[ia+6]+timf2_float[ib+6];
          r2=-(timf2_float[ia+3]+timf2_float[ib+3]+
               timf2_float[ia+7]+timf2_float[ib+7]);
  
          t3=  timf2_float[ic  ]+timf2_float[id  ]+
               timf2_float[ic+4]+timf2_float[id+4];
          t4=-(timf2_float[ic+1]+timf2_float[id+1]+
               timf2_float[ic+5]+timf2_float[id+5]);
          r3=  timf2_float[ic+2]+timf2_float[id+2]+
               timf2_float[ic+6]+timf2_float[id+6];
          r4=-(timf2_float[ic+3]+timf2_float[id+3]+
               timf2_float[ic+7]+timf2_float[id+7]);
  
          t5=(float)(timf2_float[ia  ])-(float)(timf2_float[ib  ])+
             (float)(timf2_float[ia+4])-(float)(timf2_float[ib+4]);
          t7=-((float)(timf2_float[ia+1])-(float)(timf2_float[ib+1])+
              (float)(timf2_float[ia+5])-(float)(timf2_float[ib+5]));
          r5=(float)(timf2_float[ia+2])-(float)(timf2_float[ib+2])+
             (float)(timf2_float[ia+6])-(float)(timf2_float[ib+6]);
          r7=-((float)(timf2_float[ia+3])-(float)(timf2_float[ib+3])+
              (float)(timf2_float[ia+7])-(float)(timf2_float[ib+7]));
  
          t10=(float)(timf2_float[ic  ])-(float)(timf2_float[id  ])+
              (float)(timf2_float[ic+4])-(float)(timf2_float[id+4]);
          t6= -((float)(timf2_float[ic+1])-(float)(timf2_float[id+1])+
               (float)(timf2_float[ic+5])-(float)(timf2_float[id+5]));
          r10=(float)(timf2_float[ic+2])-(float)(timf2_float[id+2])+
              (float)(timf2_float[ic+6])-(float)(timf2_float[id+6]);  
          r6= -((float)(timf2_float[ic+3])-(float)(timf2_float[id+3])+
              (float)(timf2_float[ic+7])-(float)(timf2_float[id+7]));
  
          fftxy[4*j  ]=t1+t3;
          fftxy[4*j+1]=(t2+t4);
          fftxy[4*j+2]=r1+r3;
          fftxy[4*j+3]=(r2+r4);
  
          fftxy[4*j+8]=t1-t3;
          fftxy[4*j+9]=(t2-t4);
          fftxy[4*j+10]=r1-r3;
          fftxy[4*j+11]=(r2-r4);
  
          t11=t5-t6;
          t8=t7-t10;
          r11=r5-r6;
          r8=r7-r10;
    
          t12=t5+t6;
          t9=t7+t10;
          r12=r5+r6;
          r9=r7+r10;
  
          fftxy[4*j+4]=t12;
          fftxy[4*j+5]=t8;
          fftxy[4*j+6]=r12;
          fftxy[4*j+7]=r8;
  
          fftxy[4*j+12]=t11;
          fftxy[4*j+13]=t9;
          fftxy[4*j+14]=r11;
          fftxy[4*j+15]=r9;
          }
        }
      else
        {
        for(j=0; j<siz; j+=4)
          {
          ia=(p0+8*fft2_bigpermute[j  ])&mask;
          ib=(p0+8*fft2_bigpermute[j+1])&mask;
          ic=(p0+8*fft2_bigpermute[j+2])&mask;
          id=(p0+8*fft2_bigpermute[j+3])&mask;
  
          t1=(float)(timf2_shi[ia  ])+(float)(timf2_shi[ib  ])+
              (float)(timf2_shi[ia+4])+(float)(timf2_shi[ib+4]);
          t2=-((float)(timf2_shi[ia+1])+(float)(timf2_shi[ib+1])+
              (float)(timf2_shi[ia+5])+(float)(timf2_shi[ib+5]));
          r1=(float)(timf2_shi[ia+2])+(float)(timf2_shi[ib+2])+
              (float)(timf2_shi[ia+6])+(float)(timf2_shi[ib+6]);
          r2=-((float)(timf2_shi[ia+3])+(float)(timf2_shi[ib+3])+
              (float)(timf2_shi[ia+7])+(float)(timf2_shi[ib+7]));
  
          t3=(float)(timf2_shi[ic  ])+(float)(timf2_shi[id  ])+
             (float)(timf2_shi[ic+4])+(float)(timf2_shi[id+4]);
          t4=-((float)(timf2_shi[ic+1])+(float)(timf2_shi[id+1])+
             (float)(timf2_shi[ic+5])+(float)(timf2_shi[id+5]));
          r3=(float)(timf2_shi[ic+2])+(float)(timf2_shi[id+2])+
             (float)(timf2_shi[ic+6])+(float)(timf2_shi[id+6]);
          r4=-((float)(timf2_shi[ic+3])+(float)(timf2_shi[id+3])+
             (float)(timf2_shi[ic+7])+(float)(timf2_shi[id+7]));
  
          t5=(float)(timf2_shi[ia  ])-(float)(timf2_shi[ib  ])+
             (float)(timf2_shi[ia+4])-(float)(timf2_shi[ib+4]);
          t7=-((float)(timf2_shi[ia+1])-(float)(timf2_shi[ib+1])+
              (float)(timf2_shi[ia+5])-(float)(timf2_shi[ib+5]));
          r5=(float)(timf2_shi[ia+2])-(float)(timf2_shi[ib+2])+
             (float)(timf2_shi[ia+6])-(float)(timf2_shi[ib+6]);
          r7=-((float)(timf2_shi[ia+3])-(float)(timf2_shi[ib+3])+
              (float)(timf2_shi[ia+7])-(float)(timf2_shi[ib+7]));
  
          t10=(float)(timf2_shi[ic  ])-(float)(timf2_shi[id  ])+
              (float)(timf2_shi[ic+4])-(float)(timf2_shi[id+4]);
          t6= -((float)(timf2_shi[ic+1])-(float)(timf2_shi[id+1])+
               (float)(timf2_shi[ic+5])-(float)(timf2_shi[id+5]));
          r10=(float)(timf2_shi[ic+2])-(float)(timf2_shi[id+2])+
              (float)(timf2_shi[ic+6])-(float)(timf2_shi[id+6]);  
          r6= -((float)(timf2_shi[ic+3])-(float)(timf2_shi[id+3])+
              (float)(timf2_shi[ic+7])-(float)(timf2_shi[id+7]));
  
          fftxy[4*j  ]=t1+t3;
          fftxy[4*j+1]=(t2+t4);
          fftxy[4*j+2]=r1+r3;
          fftxy[4*j+3]=(r2+r4);
  
          fftxy[4*j+8]=t1-t3;
          fftxy[4*j+9]=(t2-t4);
          fftxy[4*j+10]=r1-r3;
          fftxy[4*j+11]=(r2-r4);
  
          t11=t5-t6;
          t8=t7-t10;
          r11=r5-r6;
          r8=r7-r10;
    
          t12=t5+t6;
          t9=t7+t10;
          r12=r5+r6;
          r9=r7+r10;
  
          fftxy[4*j+4]=t12;
          fftxy[4*j+5]=t8;
          fftxy[4*j+6]=r12;
          fftxy[4*j+7]=r8;
  
          fftxy[4*j+12]=t11;
          fftxy[4*j+13]=t9;
          fftxy[4*j+14]=r11;
          fftxy[4*j+15]=r9;
          }
        }
      }
    else
      {
      if(swfloat)
        {
        for(j=0; j<siz; j+=4)
          {
          ja=fft2_bigpermute[j  ];
          jb=fft2_bigpermute[j+1];
          jc=fft2_bigpermute[j+2];
          jd=fft2_bigpermute[j+3];
          ia=(p0+8*ja)&mask;
          ib=(p0+8*jb)&mask;
          ic=(p0+8*jc)&mask;
          id=(p0+8*jd)&mask;
          a1=fft2_window[ja]*(timf2_float[ia  ]+timf2_float[ia+4]);
          a2=fft2_window[ja]*(timf2_float[ia+1]+timf2_float[ia+5]);
          a3=fft2_window[ja]*(timf2_float[ia+2]+timf2_float[ia+6]);
          a4=fft2_window[ja]*(timf2_float[ia+3]+timf2_float[ia+7]);

          b1=fft2_window[jb]*(timf2_float[ib  ]+timf2_float[ib+4]);
          b2=fft2_window[jb]*(timf2_float[ib+1]+timf2_float[ib+5]);
          b3=fft2_window[jb]*(timf2_float[ib+2]+timf2_float[ib+6]);
          b4=fft2_window[jb]*(timf2_float[ib+3]+timf2_float[ib+7]);

          c1=fft2_window[jc]*(timf2_float[ic  ]+timf2_float[ic+4]);
          c2=fft2_window[jc]*(timf2_float[ic+1]+timf2_float[ic+5]);
          c3=fft2_window[jc]*(timf2_float[ic+2]+timf2_float[ic+6]);
          c4=fft2_window[jc]*(timf2_float[ic+3]+timf2_float[ic+7]);

          d1=fft2_window[jd]*(timf2_float[id  ]+timf2_float[id+4]);
          d2=fft2_window[jd]*(timf2_float[id+1]+timf2_float[id+5]);
          d3=fft2_window[jd]*(timf2_float[id+2]+timf2_float[id+6]);
          d4=fft2_window[jd]*(timf2_float[id+3]+timf2_float[id+7]);

          t1=a1+b1;
          t2=-(a2+b2);
          r1=a3+b3;
          r2=-(a4+b4);
  
          t3=c1+d1;
          t4=-(c2+d2);
          r3=c3+d3;
          r4=-(c4+d4);
  
          t5=a1-b1;
          t7=-(a2-b2);
          r5=a3-b3;
          r7=-(a4-b4);
  
          t10=c1-d1;
          t6=-( c2-d2);
          r10=c3-d3;
          r6=-( c4-d4);

          fftxy[4*j  ]=t1+t3;
          fftxy[4*j+1]=t2+t4;
          fftxy[4*j+2]=r1+r3;
          fftxy[4*j+3]=r2+r4;
  
          fftxy[4*j+8]=t1-t3;
          fftxy[4*j+9]=t2-t4;
          fftxy[4*j+10]=r1-r3;
          fftxy[4*j+11]=r2-r4;
  
          t11=t5-t6;
          t8=t7-t10;
          r11=r5-r6;
          r8=r7-r10;
    
          t12=t5+t6;
          t9=t7+t10;
          r12=r5+r6;
          r9=r7+r10;

          fftxy[4*j+4]=t12;
          fftxy[4*j+5]=t8;
          fftxy[4*j+6]=r12;
          fftxy[4*j+7]=r8;
  
          fftxy[4*j+12]=t11;
          fftxy[4*j+13]=t9;
          fftxy[4*j+14]=r11;
          fftxy[4*j+15]=r9;
          }
        }  
      else
        {
        for(j=0; j<siz; j+=4)
          {
          ja=fft2_bigpermute[j  ];
          jb=fft2_bigpermute[j+1];
          jc=fft2_bigpermute[j+2];
          jd=fft2_bigpermute[j+3];
          ia=(p0+8*ja)&mask;
          ib=(p0+8*jb)&mask;
          ic=(p0+8*jc)&mask;
          id=(p0+8*jd)&mask;
          a1=fft2_window[ja]*((float)(timf2_shi[ia  ])+(float)(timf2_shi[ia+4]));
          a2=fft2_window[ja]*((float)(timf2_shi[ia+1])+(float)(timf2_shi[ia+5]));
          a3=fft2_window[ja]*((float)(timf2_shi[ia+2])+(float)(timf2_shi[ia+6]));
          a4=fft2_window[ja]*((float)(timf2_shi[ia+3])+(float)(timf2_shi[ia+7]));

          b1=fft2_window[jb]*((float)(timf2_shi[ib  ])+(float)(timf2_shi[ib+4]));
          b2=fft2_window[jb]*((float)(timf2_shi[ib+1])+(float)(timf2_shi[ib+5]));
          b3=fft2_window[jb]*((float)(timf2_shi[ib+2])+(float)(timf2_shi[ib+6]));
          b4=fft2_window[jb]*((float)(timf2_shi[ib+3])+(float)(timf2_shi[ib+7]));

          c1=fft2_window[jc]*((float)(timf2_shi[ic  ])+(float)(timf2_shi[ic+4]));
          c2=fft2_window[jc]*((float)(timf2_shi[ic+1])+(float)(timf2_shi[ic+5]));
          c3=fft2_window[jc]*((float)(timf2_shi[ic+2])+(float)(timf2_shi[ic+6]));
          c4=fft2_window[jc]*((float)(timf2_shi[ic+3])+(float)(timf2_shi[ic+7]));

          d1=fft2_window[jd]*((float)(timf2_shi[id  ])+(float)(timf2_shi[id+4]));
          d2=fft2_window[jd]*((float)(timf2_shi[id+1])+(float)(timf2_shi[id+5]));
          d3=fft2_window[jd]*((float)(timf2_shi[id+2])+(float)(timf2_shi[id+6]));
          d4=fft2_window[jd]*((float)(timf2_shi[id+3])+(float)(timf2_shi[id+7]));

          t1=a1+b1;
          t2=-(a2+b2);
          r1=a3+b3;
          r2=-(a4+b4);
  
          t3=c1+d1;
          t4=-(c2+d2);
          r3=c3+d3;
          r4=-(c4+d4);
  
          t5=a1-b1;
          t7=-(a2-b2);
          r5=a3-b3;
          r7=-(a4-b4);
  
          t10=c1-d1;
          t6=-( c2-d2);
          r10=c3-d3;
          r6=-( c4-d4);

          fftxy[4*j  ]=t1+t3;
          fftxy[4*j+1]=t2+t4;
          fftxy[4*j+2]=r1+r3;
          fftxy[4*j+3]=r2+r4;
  
          fftxy[4*j+8]=t1-t3;
          fftxy[4*j+9]=t2-t4;
          fftxy[4*j+10]=r1-r3;
          fftxy[4*j+11]=r2-r4;
  
          t11=t5-t6;
          t8=t7-t10;
          r11=r5-r6;
          r8=r7-r10;
    
          t12=t5+t6;
          t9=t7+t10;
          r12=r5+r6;
          r9=r7+r10;

          fftxy[4*j+4]=t12;
          fftxy[4*j+5]=t8;
          fftxy[4*j+6]=r12;
          fftxy[4*j+7]=r8;
  
          fftxy[4*j+12]=t11;
          fftxy[4*j+13]=t9;
          fftxy[4*j+14]=r11;
          fftxy[4*j+15]=r9;
          }
        }  
      }
    return;

    case FFT2_MMXB:
#if CPU == CPU_INTEL
    n1=fft2_chunk_counter;
    n2=n1+fft2_chunk_n;
    if(n2 >= fft2_n-2)
      {
      n2=fft2_n-2;
      make_fft2_status=FFT2_MMXC;
      }
    fft2_chunk_counter=n2;  
    for(n=n1; n<n2; n+=2)
      {
      if(n < fft2_att_limit)
        {
        fft2_mmx_b2hi();
        }
      else
        {
        if(n == fft2_att_limit)
          {
          fft2_mmx_b2med();
          }
        else
          {
          fft2_mmx_b2low();
          }
        }
      }
#else
    lirerr(995349);      
#endif
    return;

    case FFT2_MMXC:
#if CPU == CPU_INTEL
    fft2_mmx_c2();
    if(no_of_spurs > 0)
      {
      fftx_na=fft2_na;
      eliminate_spurs();
      }
// Make summed power spectrum.
    zxy=&fft2_short_int[fft2_pa];
    ya=&fft2_xypower[fft2_na*siz];
    if(ampinfo_flag != 0)
      {
      for(i=0; i<siz; i++)
        {
        k=zxy[4*i  ];
        k=abs(k);
        if(k > fft2_maxamp[0])fft2_maxamp[0]=k;
        k=zxy[4*i+1];
        k=abs(k);
        if(k > fft2_maxamp[0])fft2_maxamp[0]=k;
        k=zxy[4*i+2];
        k=abs(k);
        if(k > fft2_maxamp[1])fft2_maxamp[1]=k;
        k=zxy[4*i+3];
        k=abs(k);
        if(k > fft2_maxamp[1])fft2_maxamp[1]=k;
        }
      }
// Make summed power spectrum and channel correlations.
    if(wg_waterf_sum_counter == 0)
      {
      for(i=0; i<siz; i++)
        {
        ya[i].x2=(float)(zxy[4*i  ])*(float)(zxy[4*i  ])+
                 (float)(zxy[4*i+1])*(float)(zxy[4*i+1]);
        fft2_xysum[i].x2   =ya[i].x2;
        ya[i].y2=(float)(zxy[4*i+2])*(float)(zxy[4*i+2])+
                 (float)(zxy[4*i+3])*(float)(zxy[4*i+3]);
        fft2_xysum[i].y2   =ya[i].y2;
        ya[i].im_xy=-(float)(zxy[4*i  ])*(float)(zxy[4*i+3])+
                    (float)(zxy[4*i+1])*(float)(zxy[4*i+2]);
        fft2_xysum[i].im_xy=ya[i].im_xy;
        ya[i].re_xy=(float)(zxy[4*i  ])*(float)(zxy[4*i+2])+
                    (float)(zxy[4*i+1])*(float)(zxy[4*i+3]);
        fft2_xysum[i].re_xy=ya[i].re_xy;
        }
      }
    else
      {
      for(i=0; i<siz; i++)
        {
        ya[i].x2=(float)(zxy[4*i  ])*(float)(zxy[4*i  ])+
                 (float)(zxy[4*i+1])*(float)(zxy[4*i+1]);
        fft2_xysum[i].x2   +=ya[i].x2;
        ya[i].y2=(float)(zxy[4*i+2])*(float)(zxy[4*i+2])+
                 (float)(zxy[4*i+3])*(float)(zxy[4*i+3]);
        fft2_xysum[i].y2   +=ya[i].y2;
        ya[i].im_xy=-(float)(zxy[4*i  ])*(float)(zxy[4*i+3])+
                    (float)(zxy[4*i+1])*(float)(zxy[4*i+2]);
        fft2_xysum[i].im_xy+=ya[i].im_xy;
        ya[i].re_xy=(float)(zxy[4*i  ])*(float)(zxy[4*i+2])+
                    (float)(zxy[4*i+1])*(float)(zxy[4*i+3]);
        fft2_xysum[i].re_xy+=ya[i].re_xy;
        }
      } 
#else
    ya=NULL;
    lirerr(995350);
#endif
    goto two_finish;  

    case FFT2_B:
    siz_d4=siz/4;
    siz_m4=siz*4;
    n1=fft2_chunk_counter;
    n2=n1+fft2_chunk_n;
    if(n2 >= fft2_n-2)
      {
      n2=fft2_n-2;
      make_fft2_status=FFT2_C;
      }
    fft2_chunk_counter=n2;  
    for(n=n1; n<n2; n+=2)
      {
      inc=fft2_inc;
      m2=fft2_m2;
      for(j=0; j<siz_m4; j+=fft2_m1)
        {
        itab=0;
        ia=j;
lp2:;    
        ib=ia+m2;
        ss1= fft2_tab[2*itab].sin; 
        cs1= fft2_tab[2*itab].cos;
        ss2= fft2_tab[itab].sin; 
        cs2= fft2_tab[itab].cos;

        x0=fftxy[ia  ];
        y8=fftxy[ia+1];
        u0=fftxy[ia+2];
        w0=fftxy[ia+3];

        x1=fftxy[ib  ];
        y9=fftxy[ib+1];
        u1=fftxy[ib+2];
        w1=fftxy[ib+3];

        t1=cs1*x1+ss1*y9;      
        t2=cs1*y9-ss1*x1;      
        r1=cs1*u1+ss1*w1;      
        r2=cs1*w1-ss1*u1;      
        ic=ib+m2;
        id=ic+m2;

        x4=x0+t1;
        y4=y8+t2;
        u4=u0+r1;
        w4=w0+r2;

        x6=x0-t1;
        y6=y8-t2;
        u6=u0-r1;
        w6=w0-r2;

        x2=fftxy[ic  ];
        y2=fftxy[ic+1];
        u2=fftxy[ic+2];
        w2=fftxy[ic+3];

        x3=fftxy[id  ];
        y3=fftxy[id+1];
        u3=fftxy[id+2];
        w3=fftxy[id+3];

        t3=cs1*x3+ss1*y3;      
        t4=cs1*y3-ss1*x3;      
        r3=cs1*u3+ss1*w3;      
        r4=cs1*w3-ss1*u3;      

        x5=x2+t3;
        y5=y2+t4;
        u5=u2+r3;
        w5=w2+r4;

        x7=x2-t3;
        y7=y2-t4;
        u7=u2-r3;
        w7=w2-r4;

        t5=cs2*x5+ss2*y5;
        t6=cs2*y5-ss2*x5;
        r5=cs2*u5+ss2*w5;
        r6=cs2*w5-ss2*u5;

        fftxy[ia  ]=x4+t5;
        fftxy[ia+1]=y4+t6;
        fftxy[ia+2]=u4+r5;
        fftxy[ia+3]=w4+r6;

        fftxy[ic  ]=x4-t5;
        fftxy[ic+1]=y4-t6;
        fftxy[ic+2]=u4-r5;
        fftxy[ic+3]=w4-r6;

        t8=cs2*x7+ss2*y7;
        t7=cs2*y7-ss2*x7;
        r8=cs2*u7+ss2*w7;
        r7=cs2*w7-ss2*u7;

        fftxy[ib  ]=x6+t7;
        fftxy[ib+1]=y6-t8;
        fftxy[ib+2]=u6+r7;
        fftxy[ib+3]=w6-r8;

        fftxy[id  ]=x6-t7;
        fftxy[id+1]=y6+t8;
        fftxy[id+2]=u6-r7;
        fftxy[id+3]=w6+r8;

        itab+=inc;
        ia+=4; 
        if(itab<siz_d4)goto lp2;
        }
      fft2_inc/=4;
      fft2_m1*=4;
      fft2_m2*=4;
      }
    return;

    case FFT2_C:
    if( (fft2_n&1)==1)
      {
      siz_d2=siz/2;
      ib=siz*2;
      ia=0;
      for(itab=0; itab<siz_d2; itab++)
        {
        ss1= fft2_tab[itab].sin; 
        cs1= fft2_tab[itab].cos;
        x0=fftxy[ia  ];
        y8=fftxy[ia+1];
        u0=fftxy[ia+2];
        w0=fftxy[ia+3];
        x1=fftxy[ib  ];
        y9=fftxy[ib+1];
        u1=fftxy[ib+2];
        w1=fftxy[ib+3];
        t1=cs1*x1+ss1*y9;
        t2=cs1*y9-ss1*x1;
        r1=cs1*u1+ss1*w1;
        r2=cs1*w1-ss1*u1;
        fftxy[ib  ]=x0-t1;
        fftxy[ib+1]=-(y8-t2);
        fftxy[ib+2]=u0-r1;
        fftxy[ib+3]=-(w0-r2);
        fftxy[ia  ]=x0+t1;
        fftxy[ia+1]=-(y8+t2);
        fftxy[ia+2]=u0+r1;
        fftxy[ia+3]=-(w0+r2);
        ia+=4;
        ib+=4;
        }
      }
    else
      {
      siz_d4=siz/4;
      siz_m4=siz*4;
      for(j=0; j<siz_m4; j+=fft2_m1)
        {
        itab=0;
        ia=j;
lp4:;    
        ib=ia+siz;
        ss1= fft2_tab[2*itab].sin; 
        cs1= fft2_tab[2*itab].cos;
        ss2= fft2_tab[itab].sin; 
        cs2= fft2_tab[itab].cos;

        x0=fftxy[ia  ];
        y8=fftxy[ia+1];
        u0=fftxy[ia+2];
        w0=fftxy[ia+3];

        x1=fftxy[ib  ];
        y9=fftxy[ib+1];
        u1=fftxy[ib+2];
        w1=fftxy[ib+3];

        t1=cs1*x1+ss1*y9;      
        t2=cs1*y9-ss1*x1;      
        r1=cs1*u1+ss1*w1;      
        r2=cs1*w1-ss1*u1;      
        ic=ib+siz;
        id=ic+siz;

        x4=x0+t1;
        y4=y8+t2;
        u4=u0+r1;
        w4=w0+r2;

        x6=x0-t1;
        y6=y8-t2;
        u6=u0-r1;
        w6=w0-r2;

        x2=fftxy[ic  ];
        y2=fftxy[ic+1];
        u2=fftxy[ic+2];
        w2=fftxy[ic+3];

        x3=fftxy[id  ];
        y3=fftxy[id+1];
        u3=fftxy[id+2];
        w3=fftxy[id+3];

        t3=cs1*x3+ss1*y3;      
        t4=cs1*y3-ss1*x3;      
        r3=cs1*u3+ss1*w3;      
        r4=cs1*w3-ss1*u3;      

        x5=x2+t3;
        y5=y2+t4;
        u5=u2+r3;
        w5=w2+r4;

        x7=x2-t3;
        y7=y2-t4;
        u7=u2-r3;
        w7=w2-r4;

        t5=cs2*x5+ss2*y5;
        t6=cs2*y5-ss2*x5;
        r5=cs2*u5+ss2*w5;
        r6=cs2*w5-ss2*u5;

        fftxy[ia  ]=x4+t5;
        fftxy[ia+1]=-(y4+t6);
        fftxy[ia+2]=u4+r5;
        fftxy[ia+3]=-(w4+r6);

        fftxy[ic  ]=x4-t5;
        fftxy[ic+1]=-(y4-t6);
        fftxy[ic+2]=u4-r5;
        fftxy[ic+3]=-(w4-r6);

        t8=cs2*x7+ss2*y7;
        t7=cs2*y7-ss2*x7;
        r8=cs2*u7+ss2*w7;
        r7=cs2*w7-ss2*u7;

        fftxy[ib  ]=x6+t7;
        fftxy[ib+1]=-(y6-t8);
        fftxy[ib+2]=u6+r7;
        fftxy[ib+3]=-(w6-r8);

        fftxy[id  ]=x6-t7;
        fftxy[id+1]=-(y6+t8);
        fftxy[id+2]=u6-r7;
        fftxy[id+3]=-(w6+r8);

        itab++;
        ia+=4; 
        if(itab<siz_d4)goto lp4;
        }
      }
    make_fft2_status=FFT2_ELIMINATE_SPURS;
    return;

    case FFT2_ELIMINATE_SPURS:
    if(no_of_spurs > 0)
      {
      fftx_na=fft2_na;
      eliminate_spurs();
      }
    ya=&fft2_xypower[fft2_na*siz];
// Make summed power spectrum and channel correlations.
    if(wg_waterf_sum_counter == 0)
      {
      for(i=0; i<siz; i++)
        {
        ya[i].x2=   fftxy[4*i  ]*fftxy[4*i  ]+fftxy[4*i+1]*fftxy[4*i+1];
        fft2_xysum[i].x2   =ya[i].x2;
        ya[i].y2=   fftxy[4*i+2]*fftxy[4*i+2]+fftxy[4*i+3]*fftxy[4*i+3];
        fft2_xysum[i].y2   =ya[i].y2;
        ya[i].im_xy=-fftxy[4*i  ]*fftxy[4*i+3]+fftxy[4*i+1]*fftxy[4*i+2];
        fft2_xysum[i].im_xy=ya[i].im_xy;
        ya[i].re_xy=fftxy[4*i  ]*fftxy[4*i+2]+fftxy[4*i+1]*fftxy[4*i+3];
        fft2_xysum[i].re_xy=ya[i].re_xy;
        }
      }
    else
      {
      for(i=0; i<siz; i++)
        {
        ya[i].x2=   fftxy[4*i  ]*fftxy[4*i  ]+fftxy[4*i+1]*fftxy[4*i+1];
        fft2_xysum[i].x2   +=ya[i].x2;
        ya[i].y2=   fftxy[4*i+2]*fftxy[4*i+2]+fftxy[4*i+3]*fftxy[4*i+3];
        fft2_xysum[i].y2   +=ya[i].y2;
        ya[i].im_xy=-fftxy[4*i  ]*fftxy[4*i+3]+fftxy[4*i+1]*fftxy[4*i+2];
        fft2_xysum[i].im_xy+=ya[i].im_xy;
        ya[i].re_xy=fftxy[4*i  ]*fftxy[4*i+2]+fftxy[4*i+1]*fftxy[4*i+3];
        fft2_xysum[i].re_xy+=ya[i].re_xy;
        }
      } 
two_finish:;      
    wg_waterf_sum_counter++;  
    if(genparm[MAX_NO_OF_SPURS] > 0)
      {  
      if(spursearch_sum_counter > 3*spur_speknum)
        {
        spursearch_sum_counter=0;
          for(i=spur_search_first_point; i <= spur_search_last_point; i++)
          {
          spursearch_xysum[i].x2+=ya[i].x2;
          spursearch_xysum[i].y2+=ya[i].y2;
          spursearch_xysum[i].re_xy+=ya[i].re_xy;
          spursearch_xysum[i].im_xy+=ya[i].im_xy;
          t1=spursearch_xysum[i].x2+spursearch_xysum[i].y2;
          spursearch_spectrum[i]=t1+2*(spursearch_xysum[i].re_xy*spursearch_xysum[i].re_xy+
                                       spursearch_xysum[i].im_xy*spursearch_xysum[i].im_xy-
                                       spursearch_xysum[i].x2*spursearch_xysum[i].y2)/t1;
          }
        spursearch_spectrum_cleanup();
        }
      else
        {
        if(spursearch_sum_counter == 0)
          {
          for(i=spur_search_first_point; i <= spur_search_last_point; i++)
            {
            spursearch_xysum[i].x2=ya[i].x2;
            spursearch_xysum[i].y2=ya[i].y2;
            spursearch_xysum[i].re_xy=ya[i].re_xy;
            spursearch_xysum[i].im_xy=ya[i].im_xy;
            }
          }
        else
          {
          for(i=spur_search_first_point; i <= spur_search_last_point; i++)
            {
            spursearch_xysum[i].x2+=ya[i].x2;
            spursearch_xysum[i].y2+=ya[i].y2;
            spursearch_xysum[i].re_xy+=ya[i].re_xy;
            spursearch_xysum[i].im_xy+=ya[i].im_xy;
            }
          }
        spursearch_sum_counter++;
        }
      }
    if(wg_waterf_sum_counter < wg.waterfall_avgnum)break;
    make_fft2_status=FFT2_WATERFALL_LINE;
    return;
   
    case FFT2_WATERFALL_LINE:
// A new summed power spectrum has arrived.
// Convert it from linear power scale to log in units of 0.1dB.
// Expand or contract so we get the number of pixels that
// will fit on the screen.
// Store at the pos of our current line in waterfall
    ya=fft2_xysum;
    a2=1;
    if(wg.xpoints_per_pixel >0)
      {
      a2=wg.xpoints_per_pixel;
      }
    else
      {
      a2=1./wg.pixels_per_xpoint;
      }
    a3=wg.first_xpoint+0.5*a2;
    if(hgwat_xpoints_per_pixel == 1 || hgwat_pixels_per_xpoint == 1)
      {
      i=hgwat_first_xpoint;
      for(ix=0; ix < wg_xpixels; ix++)
        {
        t1=ya[i].x2+ya[i].y2;
        itab=a3;
        a3+=a2;
        y=1000.*log10((t1+2*(ya[i].re_xy*ya[i].re_xy+ya[i].im_xy*ya[i].im_xy-
                                ya[i].x2*ya[i].y2)/t1)*wg_waterf_yfac[itab]);
        if(y < -32767)y=-32767;
        if(y>32767)y=32767;
        wg_waterf[wg_waterf_ptr+ix]=y;
        i++;
        }
      }    
    else
      {
      if(hgwat_xpoints_per_pixel == 0)
        {
// There are more pixels than data points so we must interpolate.
        i=hgwat_first_xpoint;
        t1=ya[i].x2+ya[i].y2;
        itab=a3;
        a3+=a2;
        y=1000.*log10((t1+2*(ya[i].re_xy*ya[i].re_xy+ya[i].im_xy*ya[i].im_xy-
                               ya[i].x2*ya[i].y2)/t1)*wg_waterf_yfac[itab]);
        yval=y;
        if(y < -32767)y=-32767;
        if(y>32767)y=32767;
        wg_waterf[wg_waterf_ptr]=y;
        i=hgwat_first_xpoint+1;
        m=wg_xpixels-hgwat_pixels_per_xpoint;
        for(ix=0; ix<m; ix+=hgwat_pixels_per_xpoint)
          {
          itab=a3;
          a3+=a2;
          t1=ya[i].x2+ya[i].y2;
          r1=1000.*log10((t1+2*(ya[i].re_xy*ya[i].re_xy+ya[i].im_xy*ya[i].im_xy-
                                 ya[i].x2*ya[i].y2)/t1)*wg_waterf_yfac[itab]);
          der=(r1-yval)/hgwat_pixels_per_xpoint;
          for(k=ix+1; k<=ix+hgwat_pixels_per_xpoint; k++)
            {
            yval=yval+der;
            y=yval;
            if(y < -32767)y=-32767;
            if(y>32767)y=32767;
            wg_waterf[wg_waterf_ptr+k]=y;
            }  
          yval=r1;
          i++;
          }      
        if(i<fft2_size)
          {
          itab=a3;
          t1=ya[i].x2+ya[i].y2;
          r1=1000.*log10((t1+2*(ya[i].re_xy*ya[i].re_xy+ya[i].im_xy*ya[i].im_xy-
                                 ya[i].x2*ya[i].y2)/t1)*wg_waterf_yfac[itab]);
          der=(r1-yval)/hgwat_pixels_per_xpoint;
          for(k=ix+1; k<=ix+hgwat_pixels_per_xpoint; k++)
            {
            yval=yval+der;
            y=yval;
            if(y < -32767)y=-32767;
            if(y>32767)y=32767;
            wg_waterf[wg_waterf_ptr+k]=y;
            }  
          }
        }
      else
        {
// There is more than one data point for each pixel.
// Make a region ia to ib for the points behind each pixel.
// find the largest value for each region.
        ia=hgwat_first_xpoint;
        ib=ia+hgwat_xpoints_per_pixel;
        for(ix=0; ix<wg_xpixels; ix++)
          {
          r2=0;
          k=0;
          for(i=ia; i<ib; i++)
            {    
            t1=ya[i  ].x2+ya[i  ].y2;
            r1=t1+2*(ya[i  ].re_xy*ya[i  ].re_xy+
                     ya[i  ].im_xy*ya[i  ].im_xy-
                     ya[i  ].x2*ya[i  ].y2)/t1;
            if(r1 > r2)r2=r1;
            }
          itab=a3;
          a3+=a2;
          a1=wg_waterf_yfac[itab];
          y=1000.*log10(a1*r2);
          if(y < -32767)y=-32767;
          if(y>32767)y=32767;
          wg_waterf[wg_waterf_ptr+ix]=y;
          ia=ib;
          ib+=hgwat_xpoints_per_pixel;
          if(ib >= siz)ib=siz;
          }
        }  
      }
    update_wg_waterf();
    fft2_liminfo_cnt++;
    break;

    default:
    break;
    }
  }
// Place high resolution spectra on the screen in case a frequency is
// selected.
hg_redraw_counter++;
if(hg_redraw_counter >= hg.spek_avgnum/4 ||
   hg_redraw_counter*fft2_blocktime > 0.1)
  {
  hg_redraw_counter=0;
  sc[SC_SHOW_FFT2]++;
  awake_screen();
  }
make_fft2_status=FFT2_COMPLETE;
timf2_px=(timf2_px+timf2_output_block)&timf2_mask;
if(genparm[AFC_ENABLE] != 0 && genparm[AFC_LOCK_RANGE] != 0)
  {
  for(i=0; i<genparm[MIX1_NO_OF_CHANNELS]; i++)
    {
    mix1_eval_avgn[i*max_fft2n+fft2_na]=-1;
    mix1_eval_sigpwr[i*max_fft2n+fft2_na]=-1;
    }
  }
fft2_na=(fft2_na+1)&fft2n_mask;
fft2_pa=twice_rxchan*fft2_na*fft2_size;
fft2_nb=(fft2_nb+1)&fft2n_mask;
ag_pa=(ag_pa+1)&ag_mask;  
if(fft2_nm != fft2n_mask)fft2_nm++;
}


