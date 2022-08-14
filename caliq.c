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
#include "caldef.h"
#include "thrdef.h"
#include "keyboard_def.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif


void expand_foldcorr(float *x, float *tmp)
{
int i, j, k, n, mm, mx;
mm=twice_rxchan;
if(bal_segments < fft1_size)
  {
  mx=bal_segments;
  }
else
  {
  mx=fft1_size;
  }
mx*=2;    
for(j=0; j<mm; j+=2)
  {
  k=fft1_size;
  n=4*bal_segments;
  i=0;
  while(i<mx && i<=k) 
    {
    k--;
    n--;
    tmp[2*i  ]=contracted_iq_foldcorr[mm*i+j  ];
    tmp[2*i+1]=contracted_iq_foldcorr[mm*i+j+1];
    tmp[2*k  ]=contracted_iq_foldcorr[mm*n+j  ];
    tmp[2*k+1]=contracted_iq_foldcorr[mm*n+j+1];
    i++;
    }
  for(i=2*bal_segments; i<fft1_size-2*bal_segments; i++)
    {
    tmp[2*i  ]=0;
    tmp[2*i+1]=0;
    }
  big_fftback(fft1_size, fft1_n, tmp, cal_table, cal_permute, FALSE);
  for(i=0; i<fft1_size; i++)
    {
    x[mm*i+j  ]=tmp[2*i  ];
    x[mm*i+j+1]=tmp[2*i+1];
    }
  }  
}

void contract_foldcorr(float *x)
{
int i, j, k, n, m, mm;
float t1,t2,t3,r1,r2;
mm=twice_rxchan;
m=fft1_size/bal_segments;
for(j=0; j<mm; j+=2)
  {
  for(i=0; i<fft1_size; i++)
    {
    cal_tmp[2*i  ]=x[mm*i+j  ]/fft1_size;
    cal_tmp[2*i+1]=x[mm*i+j+1]/fft1_size;
    }
  for(n=0; n<2; n++)
    {
    for(i=0; i<m; i++)
      {
      k=fft1_size-i-1;
      t1=cal_tmp[2*i+n];
      t2=cal_tmp[2*k+n];
      cal_tmp[2*i+n]=((m+i)*t1+(m-i)*t2)/(2*m);
      cal_tmp[2*k+n]=((m+i)*t2+(m-i)*t1)/(2*m);
      }
    } 
  big_fftforward(fft1_size, fft1_n, cal_tmp, cal_table, cal_permute, FALSE);
// The errors at both ends of the spectrum create a transcient
// which is just an error of the measurement.
// This transcient is a wideband pulse, we remove it
// by making the higher frequencies zero in the transform.  
  r1=0;
  r2=0;
  for(i=3*(bal_segments>>1); i<2*bal_segments; i++)
    {    
    k=fft1_size-i-1;
    r1+=cal_tmp[2*i  ];
    r2+=cal_tmp[2*i+1];
    r1+=cal_tmp[2*k  ];
    r2+=cal_tmp[2*k+1];
    }
  r1/=bal_segments;
  r2/=bal_segments;  
  for(i=0; i<4*bal_segments; i++)
    {
    cal_tmp[2*i  ]-=r1;
    cal_tmp[2*i+1]-=r2;
    }
  t1=0.5*PI_L/bal_segments;
  t2=0;
  for(i=bal_segments; i<2*bal_segments; i++)
    {    
    t3=pow(cos(t2),2);
    k=fft1_size-i-1;
    cal_tmp[2*i  ]*=t3;
    cal_tmp[2*i+1]*=t3;
    cal_tmp[2*k  ]*=t3;
    cal_tmp[2*k+1]*=t3;
    t2+=t1;
    }
  for(i=0; i<2*bal_segments; i++) 
    {
    k=fft1_size-i-1;
    n=4*bal_segments-i-1;
    contracted_iq_foldcorr[mm*i+j  ]=cal_tmp[2*i  ];
    contracted_iq_foldcorr[mm*i+j+1]=cal_tmp[2*i+1];
    contracted_iq_foldcorr[mm*n+j  ]=cal_tmp[2*k  ];
    contracted_iq_foldcorr[mm*n+j+1]=cal_tmp[2*k+1];
    }
  }
}

void write_iq_foldcorr()
{
FILE *file;
int i;
int wrbuf[20];
char s[80];
if(bal_updflag!=0)
  {
  cal_type=CAL_TYPE_IQWRITE;
  cal_initscreen();
  if(bal_updflag==-1)
    {
    lir_text(0,5,"Nothing to write to disk.");
    lir_text(0,6,"Update RAM first");
    lir_text(0,8,press_any_key);
    await_keyboard();
    return;
    }
  if(bal_updflag==1)
    {
    lir_text(0,5,"RAM cleared.");
    lir_text(0,6,"Do you really want to write cleared data to file to");
    lir_text(0,7,"use as I/Q balancing (Y/N)");
kbdinp:;
    await_processed_keyboard();
    if(kill_all_flag) return;
    if(lir_inkey == 'N')return;
    if(lir_inkey != 'Y')goto kbdinp;
    }
  }
contract_foldcorr(fft1_foldcorr);
if(kill_all_flag)return;
for(i=0; i<twice_rxchan*4*bal_segments; i++)
  {
  if(contracted_iq_foldcorr[i] > BIGFLOAT || contracted_iq_foldcorr[i] < -BIGFLOAT)
    {
    lirerr(1225);
    return;
    }
  }  
make_iqcorr_filename(s);
file = fopen(s, "wb");
if (file == NULL)
  {
  lirerr(1087);
  return;
  }
wrbuf[0]=bal_segments;
wrbuf[1]=ui.rx_input_mode&IQ_DATA;
wrbuf[2]=ui.rx_ad_speed;
wrbuf[3]=ui.rx_rf_channels;
wrbuf[4]=FOLDCORR_VERNR;
for(i=5; i<10; i++)wrbuf[i]=0;
i=fwrite(wrbuf, sizeof(int),10,file);
if(i!=10)
  {
errx:;
  fclose(file);
  lirerr(1116);
  return;
  }
i=fwrite(contracted_iq_foldcorr, twice_rxchan*sizeof(float),
                                                      4*bal_segments, file);  
if(i != 4*bal_segments)goto errx;
i=fwrite(wrbuf, sizeof(int),10,file);
if(i!=10)goto errx;
fclose(file);
bal_updflag=-1;
}

void update_iq_foldcorr(void)
{
char s[80];
int i, j, k, m, mm;
int ia,ib;
int seg;
float scale,t1,t2,r1,r2,ta,tb,ra,rb;
float z[2*MAX_ADCHAN];
for(j=0; j<4*ui.rx_rf_channels; j++)z[j]=0;
mm=twice_rxchan;
scale=5;
// Find average values for amplitude ratio and phase sum.
// for those segments where we collected BAL_AVGNUM data points.
for(seg=0; seg<bal_segments; seg++)
  {
  if(bal_flag[seg] == BAL_AVGNUM)
    {
    for(j=0; j<4*ui.rx_rf_channels; j++)z[j]=0;
    for(m=0; m<BAL_AVGNUM; m++)
      {
      k=(m*bal_segments+seg)*ui.rx_rf_channels;  
      for(j=0; j<ui.rx_rf_channels; j++)
        {
        z[4*j  ]+=bal_pos[k+j];
        t1=cos(bal_phsum[k+j]);
        t2=sin(bal_phsum[k+j]);
        z[4*j+1]+=t1;
        z[4*j+2]+=t2;
        z[4*j+3]+=bal_amprat[k+j];
        }
      }
    for(j=0; j<ui.rx_rf_channels; j++)
      {
      bal_pos[seg*ui.rx_rf_channels+j]=0.5+z[4*j  ]/BAL_AVGNUM;  
      bal_phsum[seg*ui.rx_rf_channels+j]=atan2(z[4*j+2],z[4*j+1]);
      bal_amprat[seg*ui.rx_rf_channels+j]=z[4*j+3]/BAL_AVGNUM;
      }
    }
  }  
// Expand to fft1_size and draw straight lines between the points we have.
for(j=0; j<ui.rx_rf_channels; j++)
  {
// The center point is incorrect since the spur and the signal are not
// enough separated. Replace with average from surroundings.  
  k=bal_segments*ui.rx_rf_channels/2+j;
  if( bal_flag[k+ui.rx_rf_channels] == BAL_AVGNUM &&
                 bal_flag[k-ui.rx_rf_channels] == BAL_AVGNUM)
    {
    bal_phsum[k]=(bal_phsum[k+ui.rx_rf_channels]+bal_phsum[k-ui.rx_rf_channels])/2;
    bal_amprat[k]=(bal_amprat[k+ui.rx_rf_channels]+bal_amprat[k-ui.rx_rf_channels])/2;
    bal_pos[k]=(bal_pos[k+ui.rx_rf_channels]+bal_pos[k-ui.rx_rf_channels])/2;
    bal_flag[k]=BAL_AVGNUM;
    }
  else
    {
    bal_flag[k]=0;  
    }
  seg=0;
  while(bal_flag[seg] != BAL_AVGNUM  && seg < bal_segments) seg++;
  if(seg == bal_segments)return;
  ia=0;
  t1=bal_phsum[seg*ui.rx_rf_channels+j];
  t2=bal_amprat[seg*ui.rx_rf_channels+j];
  ta=t2*cos(t1);
  ra=t2*sin(t1);
fillbuf:;
  ib=bal_pos[seg*ui.rx_rf_channels+j];
  t1=bal_phsum[seg*ui.rx_rf_channels+j];
  t2=bal_amprat[seg*ui.rx_rf_channels+j];
  tb=t2*cos(t1);
  rb=t2*sin(t1);
fillbuf1:;
  if(ia<0)ia=0;
  if(ib>fft1_size)ib=fft1_size;
  for(i=ia; i<ib; i++)
    {
    t1=(i-ia);
    t1/=(ib-ia);
    t2=1-t1;
    r1=t2*ta+t1*tb;
    r2=t2*ra+t1*rb;
    cal_buf[mm*i+2*j  ]=r1;
    cal_buf[mm*i+2*j+1]=r2;
    }    
  ia=ib;
  ta=tb;
  ra=rb;
  seg++;
  if(seg <= bal_segments)
    {
    while(seg < bal_segments)
      {
      if(bal_flag[seg] == BAL_AVGNUM)goto fillbuf;
      seg++;
      }
    seg++;  
    ib=fft1_size;
    goto fillbuf1;
    }  
  }
// Make the new correction curve smoother.
contract_foldcorr(cal_buf);
expand_foldcorr(cal_buf,cal_tmp);
for(j=0; j<mm; j+=2)
  {
  for(i=0; i<fft1_size/2; i++)
    {
    r1=cal_buf[mm*i+j  ];
    r2=cal_buf[mm*i+j+1];
    fft1_foldcorr[mm*i+j  ]+=r1;
    fft1_foldcorr[mm*i+j+1]-=r2;
    }
  for(i=fft1_size/2+1; i<fft1_size; i++)
    {
    r1=cal_buf[mm*i+j  ];
    r2=cal_buf[mm*i+j+1];
    fft1_foldcorr[mm*i+j  ]+=r1;
    fft1_foldcorr[mm*i+j+1]+=r2;
    }
  }  
redraw:;
cal_type=CAL_TYPE_SHOW_IQBAL;
cal_initscreen();
for(j=0; j<mm; j+=2)
  {
  t1=cal_yzer+j*.16F;
  lir_fillbox(0,screen_height*(t1-cal_ymax),bal_screen,
                                           screen_height*2*cal_ymax,37);
  for(i=0; i<bal_screen; i++)
    {
    k=i*(fft1_size/bal_screen);
    if(k>fft1_size-1)k=fft1_size-1;
    lir_setpixel(i, cal_graph[bal_screen*j+i], 0);  
    t2=scale*cal_buf[mm*k+j];   
    if(t2 < -cal_ymax)t2=-cal_ymax;
    if(t2 > cal_ymax)t2=cal_ymax;
    t1=cal_yzer;
    if(j > 1)
      {
      t2-=0.32;
      t1+=0.32;
      }
    cal_graph[bal_screen*j+i]=screen_height*(cal_yzer-t2);
    lir_setpixel(i, screen_height*t1,20);
    lir_setpixel(i, cal_graph[bal_screen*j+i], 13);  
    lir_setpixel(i, cal_graph[bal_screen*(j+1)+i], 0);  
    t2=scale*cal_buf[mm*k+j+1];   
    if(t2 <-cal_ymax)t2=-cal_ymax;
    if(t2 >cal_ymax)t2=cal_ymax;
    if(j > 1)t2-=0.32;
    cal_graph[bal_screen*(j+1)+i]=screen_height*(0.5F-t2);
    lir_setpixel(i, cal_graph[bal_screen*(j+1)+i], 10);  
    }
  }
lir_text(0,3,"Complex amplitude of spur frequency to add to signal.");
lir_text(0,4,"If amplitudes are large, run the routine once more");
lir_text(0,5,"to make sure second order effects are eliminated.");
lir_text(7,7,"Press + or - to change scale, any other key to exit.)");
sprintf(s,"Scale=%f ",scale);
lir_text(7,8,s);
await_processed_keyboard();
if(kill_all_flag) return;
if(lir_inkey == '+')
  {
  scale*=3.13;
  goto redraw;
  }
if(lir_inkey == '-')
  {
  scale/=3.13;
  goto redraw;
  }
if(lir_inkey == F1_KEY || lir_inkey == '!')
  {
  help_message(312);
  goto redraw;
  }
fft1_calibrate_flag|=CALIQ;
bal_updflag=0;
}

void cal_iqbalance(void)
{
char s[80];
int seg,color;
int ia,ib;
int i,j,k,m,n,mm,errskip;
float z[4*MAX_ADCHAN];
float t1,t2,t3;
caliq_clear_flag=TRUE;
clear_fft1_filtercorr();
bal_updflag=-1;
bal_segments=32;
bal_screen=screen_width;
make_power_of_two(&bal_screen);
if(bal_screen > screen_width)bal_screen>>=1;
// Have to set twice_rxchan again. 
// Bug in gcc version 5.1.1 20150612 (Red Hat 5.1.1-3) (GCC)
twice_rxchan=2*ui.rx_rf_channels;
mm=twice_rxchan;
restart:;
thread_status_flag[THREAD_CAL_IQBALANCE]=THRFLAG_ACTIVE;
if(bal_segments > BAL_MAX_SEG)bal_segments = BAL_MAX_SEG;
if(bal_segments > fft1_size/4)bal_segments = fft1_size/4;
if(bal_segments < 8)bal_segments = 8;
cal_type=CAL_TYPE_COLLECT_IQBAL;
cal_initscreen();
lir_text(0,1,"Connect signal generator to antenna input(s).");
sprintf(s,"The frequency range is split in %d segments.",bal_segments);
lir_text(0,2,s);
lir_text(0,3,"Tune and wait for each segment to become green.");
settextcolor(14);
lir_text(5,4,"+/- => Change no of segments");
lir_text(7,5,  "S => Save current RAM contents to disk");
lir_text(7,6,  "C => Clear RAM");
lir_text(7,7,  "U => Compute new calibration and store in RAM");
settextcolor(7);
if(caliq_clear_flag)
  {
  for(i=0; i<=bal_segments; i++)bal_flag[i]=0;
  }
lir_refresh_screen();
while(thread_command_flag[THREAD_CAL_IQBALANCE] == THRFLAG_ACTIVE)
  {
  clear_lines(8,9);
  errskip=0;
  for(j=0; j<ui.rx_ad_channels; j++)
    {
    t1=(100.*rx_ad_maxamp[j])/0x8000;
    if(t1>90)
      {
      settextcolor(12);
      lir_text(10,8,"O V E R L O A D");
      errskip=1;
      }
    if(t1<5)
      {
      settextcolor(12);
      lir_text(10,8,"Signal too weak");
      errskip=1; 
      }
    sprintf(s,"A/D(%d) %.5f%%",j,t1);
    lir_text(24*j,9,s);
    settextcolor(7);
    }
  lir_refresh_screen();
  workload_reset_flag++;
  fft1_px=fft1_pb;
  fft1_nx=fft1_px/fft1_block;
  while(fft1_pb==fft1_px && !kill_all_flag &&
                thread_command_flag[THREAD_CAL_IQBALANCE] == THRFLAG_ACTIVE)
    {     
    lir_await_event(EVENT_FFT1_READY);
    }
  i=0;
  while(i < snd[RXAD].interrupt_rate/5)
    {
    k=0;
    for(j=0; j<ui.rx_ad_channels; j++)
      {
      t1=(100*rx_ad_maxamp[j])/0x8000;
      if(t1<5)k++;
      }
    if(k == 0)goto no_wait;
    lir_await_event(EVENT_FFT1_READY);
    i++;
    }
no_wait:;
// Convert complex amplitudes to power and phase, store in cal_buf.
// Get the point of maximum. 
  t2=0;
  k=0;
  for(i=0; i<fft1_size; i++)
    {
    t1=0;
    for(j=0; j<mm; j+=2)
      {
      cal_buf[mm*i+j]=pow(fft1_float[fft1_px+mm*i+j  ],2.0)+
                       pow(fft1_float[fft1_px+mm*i+j+1],2.0);
      t1+=cal_buf[mm*i+j];
      cal_buf[mm*i+j+1]=atan2(fft1_float[fft1_px+mm*i+j+1],
                               fft1_float[fft1_px+mm*i+j  ]);
      }
    if(t2<t1)
      {
      t2=t1;
      k=i;
      }
    }
  fft1_nx=(fft1_nx+1)&fft1n_mask;
  fft1_px=(fft1_px+fft1_block)&fft1_mask;
  if(errskip==1) goto skipdat; 
  if(k<2)k=2;
  if(k>fft1_size-3)k=fft1_size-3;
// Since we use a sin power 4 window a peak will be a few points wide.
// collect the average amplitude ratio and average phase sum 
// Use power as weight factor in averaging.
  clear_lines(10,10);
  for(j=0; j<mm; j+=2)
    {
    z[2*j  ]=0;
    z[2*j+1]=0;
    z[2*j+2]=0;
    z[2*j+3]=0;
    for(i=k-2; i<=k+2; i++)
      {
      m=fft1_size-i;
      t1=cal_buf[mm*m+j+1]+cal_buf[mm*i+j+1];
      if(t1<-PI_L)t1+=2*PI_L;
      if(t1>PI_L)t1-=2*PI_L;
      t3=cal_buf[mm*i+j];
      if(t3 > 0)
        {
        t2=sqrt(cal_buf[mm*m+j]/cal_buf[mm*i+j]);
        z[2*j]+=t3;
        z[2*j+1]+=t3*t2;
        z[2*j+2]+=t3*t1;
        z[2*j+3]+=t3*i;
        }
      }
    if(z[2*j] > 0)
      {     
      z[2*j+1]/=z[2*j];
      z[2*j+2]/=z[2*j];
      z[2*j+3]/=z[2*j];
      }
    else
      {
      goto skipdat;
      }  
    ia=z[2*j+3]+0.5;
    ib=ia+4;
    ia=ia-4;
    if(ia<0)ia=0;
    if(ib>fft1_size)ib=fft1_size;
    if(cal_buf[mm*ia+j]+cal_buf[mm*ib+j]>0.02*cal_buf[mm*k+j])
      {
      lir_text(0,10,"Signal too unstable");
      goto skipdat;
      }
    }
  clear_lines(10,10);
  seg=(bal_segments*z[3])/fft1_size+0.5;
  if(seg > bal_segments)seg=bal_segments;
  for(j=2; j<mm; j+=2)
    {
    k=(bal_segments*z[2*j+3])/fft1_size+0.5;
    if(seg != k )
      {
      lir_text(0,10,"Channels differ");
      goto skipdat;
      }
    }    
  if(bal_flag[seg]==BAL_AVGNUM)
    {
    sprintf(s,"Segment %d ok",seg);
    lir_text(0,10,s);
    goto skipdat_a;
    }
  k=(bal_flag[seg]*bal_segments+seg)*ui.rx_rf_channels;  
  for(j=0; j<ui.rx_rf_channels; j++)
    {
    bal_pos[k+j]=z[4*j+3]+0.5;
    bal_phsum[k+j]=z[4*j+2];
    bal_amprat[k+j]=z[4*j+1];
    }
  bal_flag[seg]++;
  skipdat_a:;
  for(j=0; j<ui.rx_rf_channels; j++)
    {
    sprintf(s,"Ch(%d) A=%f ph=%f",j,z[4*j+1],z[4*j+2]);
    lir_text(30*j,11,s);
    }
skipdat:;
  lir_refresh_screen();
  m=fft1_size/bal_screen;
  for(j=0; j<mm; j+=2)
    {
    for(i=0; i<bal_screen; i++)
      {
      lir_setpixel(i, cal_graph[bal_screen*j+i], 0);  
      k=i*m;
      if(k>fft1_size-m)k=fft1_size-m;
      t2=0;
      for(n=0;n<m;n++)t2+=cal_buf[mm*(k+n-m/2)+j];
      t2/=m;
      t2=0.03*log10(t2)-0.05;   
      if(t2 <-0.28)t2=-0.28;
      if(t2 > 0.28)t2= 0.28;
      if(j>0)t2-=.2;
      cal_graph[bal_screen*j+i]=screen_height*(0.5-t2);
      seg=(float)(bal_segments*k)/fft1_size+0.5;    
      color=13;
      if(bal_flag[seg]==0) color=15;
      if(bal_flag[seg]==BAL_AVGNUM)color=10;
      lir_setpixel(i, cal_graph[bal_screen*j+i], color);  
      }
    }
  lir_refresh_screen();
  }
if(thread_command_flag[THREAD_CAL_IQBALANCE]==THRFLAG_IDLE)
  {
  thread_status_flag[THREAD_CAL_IQBALANCE]=THRFLAG_IDLE;
  while(thread_command_flag[THREAD_CAL_IQBALANCE]==THRFLAG_IDLE)
    {
    lir_await_event(EVENT_FFT1_READY);
    fft1_nx=(fft1_nx+1)&fft1n_mask;
    fft1_px=fft1_nx*fft1_block;
    }
  if(thread_command_flag[THREAD_CAL_IQBALANCE]==THRFLAG_ACTIVE)goto restart;
  }  
thread_status_flag[THREAD_CAL_IQBALANCE]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_CAL_IQBALANCE] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}
