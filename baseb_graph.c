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

	
#define YWF 4
#define YBO 4
#define DA_GAIN_RANGE 10.
#define DA_GAIN_REF 1.
#define BG_MINYCHAR 8
#define BG_MIN_WIDTH (36*text_width)


#define BG_BFO    1
#define BG_BFO10  2
#define BG_BFO100 3
#define BG_FLAT   4
#define BG_CURV   5
#define BG_VOLUME 6

#include "osnum.h"
#include "globdef.h"
#include <string.h>
#include "uidef.h"
#include "fft3def.h"
#include "fft2def.h"
#include "fft1def.h"
#include "sigdef.h"
#include "seldef.h"
#include "vernr.h"
#include "screendef.h"
#include "graphcal.h"
#include "thrdef.h"
#include "options.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

int binshape_points;
int binshape_total;
int reinit_baseb;
int baseband_graph_scro;
int bg_old_x1;
int bg_old_x2;
int bg_old_y1;
int bg_old_y2;
int new_bg_agc_flag;
int new_bg_fm_mode;
int new_bg_fm_subtract;
int new_bg_ch2_phase;

int new_daout_channels;
int new_daout_bytes;

int new_bg_delay;
int new_bg_coherent;
int new_bg_twopol;

float make_interleave_ratio(int nn);
void prepare_mixer(MIXER_VARIABLES *m, int nn);

static int ch2_signs[3]={1,-1,0};


double iir5_c4(float fq)
{
double f, dt1;
f=fq;
f+=(double)1.09997;
dt1=(double)-8180.447687211407;
dt1+=(double)7606.001654061646/f;
dt1+=(double)4911.238063416248*f;
dt1+=(double)-3722.590923768796/(f*f);
dt1+=(double)-1563.849845278449*f*f;
dt1+=(double)752.161694907701/(f*f*f);
dt1+=(double)204.513728752513*f*f*f;
return dt1;
}

double iir5_c3(float fq)
{
double f, dt1;
f=fq;
f+=(double).486;
dt1=(double)1216.739279441632;
dt1+=(double)-498.037837421718/f;
dt1+=(double)-1605.884219112067*f;
dt1+=(double)98.522399169193/(f*f);
dt1+=(double)1163.334654719262*f*f;
dt1+=(double)-8.037391363636/(f*f*f);
dt1+=(double)-378.207481959566*f*f*f;
dt1+=(double)0.460060792302*f*f*f*f;
return dt1;
}

double iir5_c2(float fq)
{
double f, dt1;
f=fq;
f+=(double).373;
dt1=(double)-1367.817993435964;
dt1+=(double)321.426710716263/f;
dt1+=(double)3143.873042678646*f;
dt1+=(double)-33.921164217429/(f*f);
dt1+=(double)-4207.054014954479*f*f;
dt1+=(double)1.474943544810/(f*f*f);
dt1+=(double)3173.210883130213*f*f*f;
dt1+=(double)-1061.231815380881*f*f*f*f;
return dt1;
}

double iir5_c1(float fq)
{
double f, dt1;
f=fq;
f+=(double).13607;
dt1=(double)44.536544420484;
dt1+=(double)-6.546900527778/f;
dt1+=(double)-135.019183000212*f;
dt1+=(double)0.298668662095/(f*f);
dt1+=(double)185.556866049127*f*f;
dt1+=(double)-0.006124743273/(f*f*f);
dt1+=(double)-66.083966540342*f*f*f;
dt1+=(double)-56.694842234911*f*f*f*f;
return dt1;
}

double iir5_c0(float fq)
{
double f, dt1;
f=fq;
f=f+(double)0.25;
f=pow(f,(double)1.02);
dt1= (double)36.927768658067;
dt1+=(double)-70.589472628117*f;
dt1+=(double)72.191321694731*f*f;
dt1+=(double)-31.339811219801*f*f*f;
dt1+=(double)-10.505632906345/f;
dt1+=(double)1.399038105205/(f*f);
dt1+=(double)-0.043679124720/(f*f*f);
return dt1;
}

double iir5_gain(float fq)
{
double f, dt1;
// fq is the corner frequency divided by the sampling frequency
f=fq;
dt1=(double)0.147800765172*pow(f,-3);
dt1+=(double)0.03169658430080*pow(f,-4);
dt1+=(double)0.0032849456565132*pow(f,-5);
dt1+=(double)-0.1237864623*pow(f,-2);
return dt1;
}

void make_iir5(float fq, IIR5_PARMS *iir5)
{
iir5[0].gain=iir5_gain(fq);
iir5[0].c0=iir5_c0(fq);
iir5[0].c1=iir5_c1(fq);
iir5[0].c2=iir5_c2(fq);
iir5[0].c3=iir5_c3(fq);
iir5[0].c4=iir5_c4(fq);
}

void halt_rx_output(void)
{
int i;
i=0;
await_semclear:;
if(all_threads_started &&
         thread_status_flag[THREAD_RX_OUTPUT]!=THRFLAG_SEMCLEAR)
  {
  if(new_baseb_flag != 0)
    {
    i++;
    lir_sleep(20000);
    if(i<200)goto await_semclear;
    lirerr(564292);
    return;
    }
// The output is running. Stop it now!
  pause_thread(THREAD_RX_OUTPUT);
  thread_command_flag[THREAD_RX_OUTPUT]=THRFLAG_SEMCLEAR;
  lir_sched_yield();
  while(thread_status_flag[THREAD_RX_OUTPUT] != THRFLAG_SEMCLEAR)
    {
    lir_sleep(1000);
    }
  baseb_reset_counter++;
  }
}

void clear_baseb_arrays(int nn,int k)
{
memset(&baseb_raw[2*nn],0,2*k*sizeof(float));
memset(&baseb_raw_orthog[2*nn],0,2*k*sizeof(float));
if(fft1_correlation_flag != 0)
  {
  memset(&d_baseb[2*nn],0,2*k*sizeof(double));
  memset(&d_baseb_carrier[2*nn],0,2*k*sizeof(double));
  }
memset(&baseb[2*nn],0,2*k*sizeof(float));
memset(&baseb_carrier[2*nn],0,2*k*sizeof(float));
memset(&baseb_totpwr[nn],0,k*sizeof(float));
memset(&baseb_carrier_ampl[nn],0,k*sizeof(float));
if(bg.agc_flag != 0)
  {
  memset(&baseb_agc_level[nn],0,k*sizeof(float));
  memset(&baseb_agc_det[nn],0,k*sizeof(float));
  memset(&baseb_threshold[bg.agc_flag*nn],0,bg.agc_flag*k*sizeof(float));
  memset(&baseb_upthreshold[bg.agc_flag*nn],0,bg.agc_flag*k*sizeof(float));
  }
lir_sched_yield();
if(genparm[CW_DECODE_ENABLE] != 0)
  {
  if(bg.agc_flag == 0)
    {
    memset(&baseb_threshold[nn],0,k*sizeof(float));
    memset(&baseb_upthreshold[nn],0,k*sizeof(float));
    }
  memset(&baseb_ramp[nn],-cg_code_unit,k*sizeof(short int));
  memset(&baseb_wb_raw[2*nn],0,2*k*sizeof(float));
  memset(&baseb_fit[2*nn],0,2*k*sizeof(float));
  memset(&baseb_tmp[2*nn],0,2*k*sizeof(float));
  memset(&baseb_sho1[2*nn],0,2*k*sizeof(float));
  memset(&baseb_sho2[2*nn],0,2*k*sizeof(float));
  memset(&baseb_envelope[2*nn],0,2*k*sizeof(float));
  }
sg_inhibit_count=5;
baseb_pa=0;
baseb_pb=0;
baseb_pc=0;
baseb_pd=0;
baseb_pe=0;
baseb_pf=0;
baseb_ps=0;
baseb_pm=0;
baseb_pn=0;
baseb_py=0;
baseb_px=0;
timf3_pa=0;
timf3_px=0;
timf3_py=0;
timf3_ps=0;
timf3_pn=0;
timf3_pc=0;
timf3_pd=0;
lir_sched_yield();
}

int filcur_pixel(int points)
{
float t1;
t1=(float)(2*points)/fft3_size;
t1=sqrt(t1);
return bg_first_xpixel+t1*bg_xpixels;
//return bg_first_xpixel+(bg_xpixels*2*points)/fft3_size;
}

int filcur_points(void)
{
float t1;
t1=(float)(mouse_x-bg_first_xpixel+1)/bg_xpixels;
t1=t1*t1;
return 0.5*fft3_size*t1;
}

void make_bg_waterf_cfac(void)
{
bg_waterf_cfac=bg.waterfall_gain*0.1;
bg_waterf_czer=10*bg.waterfall_zero;
}

void make_bg_yfac(void)
{
if(genparm[SECOND_FFT_ENABLE]==0)
  {
  bg.yfac_power=FFT1_BASEBAND_FACTOR;
  bg.yfac_power*=ui.rx_rf_channels*ui.rx_rf_channels;
  }
else
  {  
  bg.yfac_power=FFT2_BASEBAND_FACTOR*
                (float)(1<<genparm[FIRST_BCKFFT_ATT_N])/fft2_size;
  if(fft_cntrl[FFT2_CURMODE].mmx != 0)
    {
    bg.yfac_power*=1.4*(1<<(genparm[SECOND_FFT_ATT_N]));
    }
  bg.yfac_power*=bg.yfac_power;
  bg.yfac_power*=(float)(1<<genparm[MIX1_BANDWIDTH_REDUCTION_N]);
  }
bg.yfac_power*=(1 + 1/(0.5+genparm[FIRST_FFT_SINPOW]))/(float)(fft1_size);
bg.yfac_power/=fft3_size;
baseband_pwrfac=10*bg.yfac_power/fft3_size;
bg_waterf_yfac=20*baseband_pwrfac/bg.waterfall_avgnum;
bg.yfac_power/=bg.yzero*bg.yzero;
bg.yfac_log=10/bg.db_per_pixel;
}

void make_daout_gain(void)
{
float t1;
if(daout_gain_y>bg_y0)
  {
  daout_gain_y=bg_y0;
  daout_gain=0;
  return;
  }
if(daout_gain_y<bg_ymax)daout_gain_y=bg_ymax;
t1=(float)((bg_y0+bg_ymax)/2-daout_gain_y)/(bg_y0-bg_ymax);
bg.output_gain=pow(10.,t1*DA_GAIN_RANGE)/DA_GAIN_REF;    
if(rx_mode == MODE_FM && bg_coherent < 2)
  {
  daout_gain=5*bg.output_gain;
  }
else
  {
  daout_gain=0.5*bg.output_gain/fft3_size;
  if(genparm[SECOND_FFT_ENABLE]!=0)
    {
    daout_gain/=fft2_size;
    daout_gain*=1<<genparm[FIRST_BCKFFT_ATT_N];
    if(fft_cntrl[FFT2_CURMODE].mmx != 0)
      {
      daout_gain*=1<<(genparm[SECOND_FFT_ATT_N]);
      }
    }
  daout_gain*=sqrt((1 + 1/(0.5+genparm[FIRST_FFT_SINPOW]))/
                                                   (float)(fft1_size));
  }
if(rx_daout_bytes == 2)
  {
  daout_gain*=256;
  }
}

void make_daout_gainy(void)
{
int old;
float t1;
old=daout_gain_y;
t1=log10(bg.output_gain*DA_GAIN_REF)/DA_GAIN_RANGE;
daout_gain_y=(bg_y0+bg_ymax)/2-t1*(bg_y0-bg_ymax);
make_daout_gain();
update_bar(bg_vol_x1,bg_vol_x2,bg_y0,daout_gain_y,old,
                                                 BG_GAIN_COLOR,bg_volbuf);
}

void make_new_daout_upsamp(void)
{
int k, new;
float t1;
t1=da_resample_ratio;
new=1;
daout_upsamp_n=0;
if( (ui.network_flag & NET_RXOUT_BASEB) == 0)
  {
  k=genparm[DA_OUTPUT_SPEED];
  while(t1 > 5 && k > 16000 && new < 32)
    {
    daout_upsamp_n++;
    k/=2;
    t1/=2;
    new*=2;
    }
  }  
if(new != daout_upsamp)
  {
  halt_rx_output();
  daout_upsamp=new;
  }
}

void make_bfo(void)
{
float bforef, bfooff, t1;
if(rx_mode == MODE_SSB)sc[SC_FREQ_READOUT]++;
if(use_bfo == 0)
  {
  rx_daout_cos=1;
  rx_daout_sin=0;
  rx_daout_phstep_sin=0;
  rx_daout_phstep_cos=1;
  bfo_xpixel=-1;
  bfo10_xpixel=-1;
  bfo100_xpixel=-1;
  return;
  }
if(bfo_xpixel > 0)
  {
  if(fft1_correlation_flag != 2)  
    {
    if(bg_filterfunc_y[bfo_xpixel] > 0)
                     lir_setpixel(bfo_xpixel,bg_filterfunc_y[bfo_xpixel],14);
    if(bg_filterfunc_y[bfo10_xpixel] > 0)
                 lir_setpixel(bfo10_xpixel,bg_filterfunc_y[bfo10_xpixel],14);
    if(bg_filterfunc_y[bfo100_xpixel] > 0)
               lir_setpixel(bfo100_xpixel,bg_filterfunc_y[bfo100_xpixel],14);
    }
  if(bg_carrfilter_y[bfo_xpixel] > 0)
                     lir_setpixel(bfo_xpixel,bg_carrfilter_y[bfo_xpixel],58);
  if(bg_carrfilter_y[bfo10_xpixel] > 0)
                 lir_setpixel(bfo10_xpixel,bg_carrfilter_y[bfo10_xpixel],58);
  if(bg_carrfilter_y[bfo100_xpixel] > 0)
               lir_setpixel(bfo100_xpixel,bg_carrfilter_y[bfo100_xpixel],58);
  }
// When we arrive here only bg.bfo_freq is defined.
// Set up the other variables we need that depend on it. 
make_new_daout_upsamp();
t1=(2*PI_L*bg.bfo_freq)/genparm[DA_OUTPUT_SPEED];
rx_daout_phstep_cos=cos(t1);
rx_daout_phstep_sin=sin(t1);
bforef=bg_first_xpixel+0.5+bg.pixels_per_point*(fft3_size/2-bg_first_xpoint);
bfooff=bg.pixels_per_point*bg.bfo_freq/bg_hz_per_pixel;
bfo_xpixel=bforef+bfooff;
if(bfo_xpixel < bg_first_xpixel)bfo_xpixel=bg_first_xpixel;
if(bfo_xpixel > bg_last_xpixel)bfo_xpixel=bg_last_xpixel;
bfo10_xpixel=bforef+0.1*bfooff;
if(bfo10_xpixel < bg_first_xpixel)bfo10_xpixel=bg_first_xpixel;
if(bfo10_xpixel > bg_last_xpixel)bfo10_xpixel=bg_last_xpixel;
bfo100_xpixel=bforef+0.01*bfooff;
if(bfo100_xpixel < bg_first_xpixel)bfo100_xpixel=bg_first_xpixel;
if(bfo100_xpixel > bg_last_xpixel)bfo100_xpixel=bg_last_xpixel;
lir_line(bfo100_xpixel, bg_y0,bfo100_xpixel,bg_y1,12);
if(kill_all_flag) return;
lir_line(bfo10_xpixel, bg_y1-1,bfo10_xpixel,bg_y2,12);
if(kill_all_flag) return;
lir_line(bfo_xpixel, bg_y2-1,bfo_xpixel,bg_y3,12);
}

void chk_bg_avgnum(void)
{
if(fft3_blocktime*bg.fft_avgnum > genparm[BASEBAND_STORAGE_TIME])
           bg.fft_avgnum=genparm[BASEBAND_STORAGE_TIME]/fft3_blocktime;
if(bg.fft_avgnum >9999)bg.fft_avgnum=9999;
if(bg.fft_avgnum <1)bg.fft_avgnum=1;
}

void clear_agc(void)
{
// The AGC attack is operated from two series connected low pass filters.
// followed by a peak detector.
// The AGC is in mix2.c
rx_agc_factor1=1;
rx_agc_factor2=1;
rx_agc_sumpow1=0;
rx_agc_sumpow2=0;
rx_agc_sumpow3=0;
rx_agc_sumpow4=0;
rx_agc_fastsum1=0;
rx_agc_fastsum2=0;
rx_agc_fast_factor1=pow(0.5,2000./baseband_sampling_speed);
rx_agc_fast_factor2=1-rx_agc_fast_factor1;
agc_attack_factor1=pow(0.5,1000./(baseband_sampling_speed*(1<<bg.agc_attack)));
agc_release_factor=pow(0.5,500./(baseband_sampling_speed*(1<<bg.agc_release)));
agc_attack_factor2=1-agc_attack_factor1;
bg_agc_hang_pts=0.01*baseband_sampling_speed*pow(1.7,(float)(bg.agc_hang));
if(bg_agc_hang_pts > baseband_size/2)bg_agc_hang_pts=baseband_size/2;
}

/*void construct_fir(int win, int siz, int n, int *points, int ct, 
                                              float *fir, float small)
*/
void construct_fir(int win, int siz, int n, int *points, int ct, 
                                              float *fir, float zsmall)

{
int i,j,k,ib,ic,ja,jb,cutoff;
float t1,t2;
float *firbuf;
float *fir_window;
unsigned short int *fir_permute;
COSIN_TABLE *fir_tab;
cutoff=ct;
if(cutoff < 1)cutoff=1;
if(cutoff>=siz/2)cutoff=siz/2-1;
// Construct a FIR filter of size siz with a cut-off frequency cutoff.
// The array of FIR filter coefficients is the pulse response of a FIR 
// filter. Get it by taking the FFT.
// We use only the real half of a complex FFT.
firbuf=malloc(siz*(3*sizeof(float)+sizeof(short int)+sizeof(COSIN_TABLE)/2));
if(firbuf == NULL)
  {
  lirerr(493367);
  return;
  }
fir_window=&firbuf[siz*2];
fir_permute=(void*)(&fir_window[siz]);
fir_tab=(void*)(&fir_permute[siz]);
init_fft(1,n, siz, fir_tab, fir_permute);
make_window(1,siz, win, fir_window);
firbuf[0]=1;
firbuf[1]=0;
jb=siz-1;
for(ja=1; ja<cutoff; ja++)
  {
  firbuf[2*ja  ]=1;
  firbuf[2*jb  ]=1;
  firbuf[2*ja+1]=0;
  firbuf[2*jb+1]=0;
  jb--;
  }
for(ja=cutoff; ja<siz/2; ja++)
  {
  firbuf[2*ja  ]=0;
  firbuf[2*jb  ]=0;
  firbuf[2*ja+1]=0;
  firbuf[2*jb+1]=0;
  jb--;
  }
firbuf[2*jb  ]=0;
firbuf[2*jb+1]=0;
for( i=0; i<siz/2; i++)
  {
  t1=firbuf[2*i  ];
  t2=firbuf[siz+2*i  ];
  firbuf[2*i  ]=t1+t2;
  firbuf[siz+2*i  ]=fir_tab[i].cos*(t1-t2);
  firbuf[siz+2*i+1]=fir_tab[i].sin*(t1-t2);
  }
#if IA64 == 0 && CPU == CPU_INTEL
asmbulk_of_dif(siz, n, firbuf, fir_tab, yieldflag_ndsp_fft3);
#else
bulk_of_dif(siz, n, firbuf, fir_tab, yieldflag_ndsp_fft3);
#endif
for(i=0; i < siz; i+=2)
  {
  ib=fir_permute[i];
  ic=fir_permute[i+1];
  fir[ib]=firbuf[2*i  ]+firbuf[2*i+2];
  fir[ic]=firbuf[2*i  ]-firbuf[2*i+2];
  }
// Now take the effects of our window into account.
// Note the order in which fir_window is stored.
for(i=0; i<siz/2; i++)
  {
  fir[i]*=fir_window[2*i];  
  fir[siz/2+i]*=fir_window[2*i+1];  
  }
free(firbuf);  
// The FIR filter must be symmetric. Actually forcing symmetry
// might reduce rounding errors slightly.
for(i=1; i<siz/2; i++)
  {
  fir[i]=0.5*(fir[i]+fir[siz-i]);
  fir[siz-i]=fir[i];
  }
// There is no reason to make the FIR filter extend outside
// the range where the coefficients are below zsmall (supplied parameter).
t1=zsmall*fir[siz/2];
k=0;
while(fabs(fir[k]) < t1)k++;
j=k;
points[0]=0;
while(k<=siz-j-1)
  {
  fir[points[0]]=fir[k];
  k++;
  points[0]++;
  }
k=points[0];
if(k<siz)memset(&fir[k],0,siz-k);
// Normalize the FIR filter.
t1=0;
for(i=0; i<points[0]; i++)
  {
  t1+=fir[i];
  }
t1=1/t1;  
for(i=0; i<points[0]; i++)
  {
  fir[i]*=t1;
  }
}



void init_basebmem(void)
{
float t1, t2;
int i, k, clr_size;
// Sampling rate for baseband has changed.
// Set flag to flush old data and reinit output.
reinit_baseb=FALSE;
lir_sched_yield();
if( (cg.oscill_on&6) != 0)
  {
  clear_cg_traces();
  cg.oscill_on=1;
  cg_osc_ptr=0;
  }
halt_rx_output();
if(kill_all_flag)return;
if(baseband_handle != NULL)
  {
  baseband_handle=chk_free(baseband_handle);
  }
// Make the daout buffer big enough to hold data for at least 6
// times longer than the rate at which the wideband thread posts
// to the narrowband thread.
t1=(6*fftx_size)/ui.rx_ad_speed;
// Make sure minimum is 0.5 seconds
if(t1<0.5)t1=0.5;
t1*=genparm[DA_OUTPUT_SPEED];
// and make sure we can hold 8192 samples
if(t1 < 8192)t1=8192;
daout_size=t1;
if(daout_size < 8*snd[RXDA].block_frames)daout_size=8*snd[RXDA].block_frames;
// Do not use snd[RXDA].framesize since bytes or channels may have changed.
// Do not multiply by channels here, allocate 8 times this size
make_power_of_two(&daout_size);
t1=3*(float)(daout_size)/genparm[DA_OUTPUT_SPEED];
if(t1<genparm[BASEBAND_STORAGE_TIME])t1=genparm[BASEBAND_STORAGE_TIME];
// Allocate memory for transformation from fft3 to the baseband.
// We are already in the baseband but the filter in use may allow 
// a reduction of the sampling speed.
baseband_size=t1*baseband_sampling_speed;
make_power_of_two(&baseband_size);
daout_bufmask=daout_size-1;
cg_size=2+(COH_SIDE*text_width)/3;
cg_size&=0xfffffffe;
k=2+bg_flatpoints+2*bg_curvpoints;
if(genparm[THIRD_FFT_SINPOW] > 3)k+=2;
if(genparm[THIRD_FFT_SINPOW] == 8)k+=2;
cg_code_unit=0.5*(float)(mix2.size)/k;
cg_decay_factor=pow(0.5,0.2/cg_code_unit);
cw_waveform_max=14*cg_code_unit;
if(cw_waveform_max >= 2*fftn_tmp_size)
  {
  cw_waveform_max=2*fftn_tmp_size-1;
  }
cg_osc_offset=mix2.size+50*cg_code_unit;
while(baseband_size < 4*cg_osc_offset)baseband_size*=2;
if(baseband_size < 8*(int)mix2.size)baseband_size=8*mix2.size;
reduce:;
if(baseband_size < 4*cg_osc_offset)
  {
  cg_osc_offset=baseband_size/4;
  }
cg_osc_offset_inc=cg_osc_offset/2;
max_cwdat=MAX_CW_PARTS*baseband_size/cg_code_unit;
baseband_mask=baseband_size-1;
baseband_neg=(7*baseband_size)>>3;
baseband_sizhalf=baseband_size>>1;
// The time stored in the baseband buffers is 
// baseband_size/baseband_sampling_speed, the same time is contained in
// the basblock buffers in mix2.size/2 fewer points.
// Find out the number of basblock points that correspond to 
// 5 seconds, the time constant for dB meter peak hold. 
basblock_size=2*baseband_size/mix2.size;
basblock_mask=basblock_size-1;
basblock_hold_points=5*basblock_size*baseband_sampling_speed/baseband_size;
if(basblock_hold_points<3)basblock_hold_points=3;
if(basblock_hold_points>basblock_mask)basblock_hold_points=basblock_mask;
// When listening to wideband signals, the time between basblock
// points may become short.
// Make sure we do not update the coherent graph too often, 
// graphics may be rather slow.
cg_update_interval=basblock_hold_points/20;
cg_update_count=0;
clear_agc();
fft3_interleave_ratio=make_interleave_ratio(THIRD_FFT_SINPOW);
// We get signals from back transformation of fft3.
// We must use an interleave ratio that makes the interleave points
// go even up in mix2.size.     
mix2.interleave_points=fft3_interleave_ratio*mix2.size;
mix2.interleave_points&=0xfffffffe;  
mix2.new_points=mix2.size-mix2.interleave_points;
fft3_interleave_points=mix2.interleave_points*(fft3_size/mix2.size);
fft3_interleave_ratio=(float)(fft3_interleave_points)/fft1_size;
fft3_new_points=fft3_size-fft3_interleave_points;
fft3_blocktime=(float)(fft3_new_points)/timf3_sampling_speed;
if( rx_mode == MODE_FM )
  {
// ******************  fm_audiofil  ***************************
// We need a low pass filter with cut-off as specified by the user.
// Make the cross-over region 250 Hz, but degrade the steepness
// in case the baseband sampling speed is very high.
  fm_audiofil_size=baseband_sampling_speed/250;
  if(fm_audiofil_size > 512)fm_audiofil_size=512;
  fm_audiofil_n=make_power_of_two(&fm_audiofil_size);
  }
if( rx_mode == MODE_FM && 
    baseband_sampling_speed > 105000 &&
    fft1_correlation_flag == 0)
  {
// ******************  fmfil70  ************************
// We need a low pass filter with cut-off at about 70 kHz.
// Construct a FIR filter that will allow the cross-over region
// to be about 5 kHz.
// This filter will attenuate by about 100 dB above 80 kHz so we
// can resample the output if the baseband sampling rate is above 160kHz.
// For FM processing we have no reason to keep data in memory for
// a long time. Make the buffer one second.
#if NET_BASEBRAW_MODE == WFM_FM_FIRST_LOWPASS || \
    NET_BASEBRAW_MODE == WFM_AM_FIRST_LOWPASS || \
    NET_BASEBRAW_MODE == WFM_SYNTHETIZED_FROM_FIRST_LOWPASS || \
    NET_BASEBRAW_MODE == WFM_SUBTRACT_FROM_FIRST_LOWPASS
  fmfil70_size=baseband_sampling_speed/2500;
#else
  fmfil70_size=baseband_sampling_speed/5000;
#endif
  fmfil70_n=make_power_of_two(&fmfil70_size);
  fm1_resample_ratio=baseband_sampling_speed/160000;
  fm1_sampling_speed=baseband_sampling_speed/fm1_resample_ratio;
  fm1_size=fm1_sampling_speed;
  make_power_of_two(&fm1_size);
  fm1_mask=fm1_size-1;
// ******************  fmfil55  ************************
// We need a low pass filter with cut-off at 55 kHz.
// Construct a FIR filter that will allow the cross-over region
// to be about 2 kHz.
  fmfil55_size=baseband_sampling_speed/2000;
  fmfil55_n=make_power_of_two(&fmfil55_size);
// ******************  fmfil  ***************************
// We need a low pass filter with cut-off at about 17 kHz
// for wideband FM.
  fmfil_size=baseband_sampling_speed/2000;
  fmfil_n=make_power_of_two(&fmfil_size);
// ******************************************************
// ******************  fmfil_rds  ***************************
// We need a low pass filter with cut-off at 1.8 kHz for RDS.
// Construct a FIR filter that will allow the cross-over region
// to be about 1 kHz.
  fmfil_rds_size=baseband_sampling_speed/1000;
  fmfil_rds_n=make_power_of_two(&fmfil_rds_size);
// ******************************************************
// For wideband FM we look for a pilot tone at 19 kHz.
// Make a filter with a bandwidth of about 100 Hz
  fm_pilot_size=0.01*baseband_sampling_speed;
  }
else
  {
  fm_pilot_size=0;
  }
// ********************************************************
init_memalloc(basebmem, MAX_BASEB_ARRAYS);
mem(1,&baseb_out,baseband_size*2*baseb_channels*sizeof(float),0);
if(fft1_correlation_flag != 0)
  {
  mem(202,&d_baseb_carrier,baseband_size*4*sizeof(double),0);
  mem(205,&d_baseb,baseband_size*4*sizeof(double),0);
  mem(302,&baseb_carrier,baseband_size*4*sizeof(float),0);
  mem(305,&baseb,baseband_size*4*sizeof(float),0);
  mem(309,&d_mix2_table,mix2.size*sizeof(D_COSIN_TABLE)/2,0);
  }
else
  {  
  mem(2,&baseb_carrier,baseband_size*2*sizeof(float),0);
  mem(5,&baseb,baseband_size*2*sizeof(float),0);
  }
mem(6,&baseb_totpwr,baseband_size*sizeof(float),0);
mem(7,&baseb_carrier_ampl,baseband_size*sizeof(float),0);
mem(3,&baseb_raw,baseband_size*2*sizeof(float),0);
mem(4,&baseb_raw_orthog,baseband_size*2*sizeof(float),0);
mem(8,&mix2.permute,mix2.size*sizeof(short int),0);
mem(9,&mix2.table,mix2.size*sizeof(COSIN_TABLE)/2,0);
mem(10,&daout,8*daout_size,0);
mem(11,&cg_map,cg_size*cg_size*sizeof(float),0);
mem(13,&cg_traces,CG_MAXTRACE*MAX_CG_OSCW*sizeof(short int),0);
mem(16,&basblock_maxpower,basblock_size*sizeof(float),0);
mem(17,&basblock_avgpower,basblock_size*sizeof(float),0);
mem(171,&mix2.window,(mix2.size/2+1)*sizeof(float),0);
mem(172,&mix2.cos2win,mix2.new_points*sizeof(float),0);
mem(173,&mix2.sin2win,mix2.new_points*sizeof(float),0);
if(bg.agc_flag != 0)
  {
  mem(18,&baseb_agc_level,bg.agc_flag*baseband_size*sizeof(float),0);
  mem(1892,&baseb_agc_det,bg.agc_flag*baseband_size*sizeof(float),0);
  mem(14,&baseb_upthreshold,bg.agc_flag*baseband_size*sizeof(float),0);
  mem(15,&baseb_threshold,bg.agc_flag*baseband_size*sizeof(float),0);
  }
if(genparm[CW_DECODE_ENABLE] != 0)
  {
  if(bg.agc_flag == 0)
    {
    mem(14,&baseb_upthreshold,baseband_size*sizeof(float),0);
    mem(15,&baseb_threshold,baseband_size*sizeof(float),0);
    }
  mem(27,&baseb_ramp,baseband_size*sizeof(short int),0);
  keying_spectrum_size=mix2.size/cg_code_unit;
  if(keying_spectrum_size > (int)mix2.size/2)keying_spectrum_size=mix2.size/2;
  mem(19,&baseb_envelope,baseband_size*2*sizeof(float),0);
  mem(21,&dash_waveform,cw_waveform_max*2*sizeof(float),0);
  mem(215,&dash_wb_waveform,cw_waveform_max*2*sizeof(float),0);
  mem(225,&dot_wb_waveform,cw_waveform_max*sizeof(float),0);
  mem(23,&baseb_fit,baseband_size*2*sizeof(float),0);
  mem(24,&cw_carrier_window,mix2.size*sizeof(float),0);
  mem(25,&mix2_tmp,2*mix2.size*sizeof(float),0);
  mem(26,&mix2_pwr,mix2.size*sizeof(float),0);
  mem(28,&baseb_tmp,baseband_size*2*sizeof(float),0);
  mem(281,&baseb_sho1,baseband_size*2*sizeof(float),0);
  mem(282,&baseb_sho2,baseband_size*2*sizeof(float),0);
  mem(283,&baseb_wb_raw,baseband_size*2*sizeof(float),0);
  mem(30,&cw,max_cwdat*sizeof(MORSE_DECODE_DATA),0);
  mem(31,&keying_spectrum,keying_spectrum_size*sizeof(float),0);
  mem(285,&baseb_clock,baseband_size*sizeof(float),0);
  }
if(rx_mode == MODE_FM)
  {
  mem(311,&baseb_fm_demod,baseband_size*2*baseb_channels*sizeof(float),0);
  mem(1311,&fm_audiofil_fir,fm_audiofil_size*sizeof(float),0);
    mem(314,&baseb_fm_phase,baseband_size*baseb_channels*sizeof(float),0);
  if(fm_pilot_size > 0)
    {
    if(baseb_channels != 1)
      {
      lirerr(873105);
      return;
      }
    mem(312,&baseb_fm_demod_low,baseband_size*baseb_channels*sizeof(float),0);
    mem(313,&baseb_fm_demod_high,baseband_size*baseb_channels*sizeof(float),0);
    mem(320,&fm_pilot_tone,2*fm_pilot_size*sizeof(float),0);
    mem(321,&baseb_fm_pil2,baseband_size*baseb_channels*sizeof(float),0);
    mem(322,&baseb_fm_pil3,baseband_size*2*baseb_channels*sizeof(float),0);
    mem(324,&fmfil55_fir,fmfil55_size*sizeof(float),0);
    mem(325,&baseb_fm_pil2det,baseband_size*baseb_channels*sizeof(float),0);
    mem(326,&baseb_fm_pil3det,baseband_size*2*baseb_channels*sizeof(float),0);
    mem(327,&baseb_fm_sumchan,baseband_size*baseb_channels*sizeof(float),0);
    mem(328,&baseb_fm_diffchan,baseband_size*baseb_channels*sizeof(float),0);
    mem(329,&baseb_fm_rdsraw,2*baseband_size*baseb_channels*sizeof(float),0);
    mem(330,&fmfil_fir,fmfil_size*sizeof(float),0);
    mem(331,&fmfil_rds_fir,fmfil_rds_size*sizeof(float),0);
    mem(332,&baseb_fm_composite,baseband_size*baseb_channels*sizeof(float),0);
    mem(333,&fmfil70_fir,fmfil70_size*sizeof(float),0);
    mem(334,&fm1_all,fm1_size*sizeof(float),0);
    }
  }  
if( (ui.network_flag&NET_RXOUT_BASEBRAW) != 0)
  {
#if NET_BASEBRAW_MODE == WFM_FM_FIRST_LOWPASS || \
    NET_BASEBRAW_MODE == WFM_AM_FIRST_LOWPASS || \
    NET_BASEBRAW_MODE == WFM_FM_FULL_BANDWIDTH || \
    NET_BASEBRAW_MODE == WFM_FM_FIRST_LOWPASS_UPSAMP || \
    NET_BASEBRAW_MODE == WFM_STEREO_LOW || \
    NET_BASEBRAW_MODE == WFM_STEREO_HIGH

  basebraw_sampling_speed=baseband_sampling_speed;
  basebraw_ad_channels=1;
  basebraw_rf_channels=1;
  basebraw_mode=DWORD_INPUT;
  basebraw_passband_direction=1;
#endif
#if NET_BASEBRAW_MODE == WFM_FM_FIRST_LOWPASS_RESAMP
  basebraw_sampling_speed=fm1_sampling_speed;
  basebraw_ad_channels=1;
  basebraw_rf_channels=1;
  basebraw_mode=DWORD_INPUT;
  basebraw_passband_direction=1;
#endif
#if NET_BASEBRAW_MODE == BASEBAND_IQ || \
    NET_BASEBRAW_MODE == BASEBAND_IQ_TEST || \
    NET_BASEBRAW_MODE == BASEBAND_IQ_MASTER_TEST || \
    NET_BASEBRAW_MODE == WFM_SYNTHETIZED_FROM_FIRST_LOWPASS || \
    NET_BASEBRAW_MODE == WFM_SUBTRACT_FROM_FIRST_LOWPASS
  basebraw_sampling_speed=baseband_sampling_speed;
  basebraw_ad_channels=2*ui.rx_rf_channels;
  basebraw_rf_channels=ui.rx_rf_channels;
  basebraw_mode=DWORD_INPUT+IQ_DATA+DIGITAL_IQ;
  basebraw_passband_direction=-1;
#endif
#if NET_BASEBRAW_MODE == BASEBAND_IQ_FULL_BANDWIDTH
  basebraw_sampling_speed=timf1_sampling_speed;
  k=genparm[MIX1_BANDWIDTH_REDUCTION_N];
  while(k>0)
    {
    k--;
    basebraw_sampling_speed/=2;
    }
  basebraw_ad_channels=2*ui.rx_rf_channels;
  basebraw_rf_channels=ui.rx_rf_channels;
  basebraw_mode=DWORD_INPUT+IQ_DATA+DIGITAL_IQ;
  basebraw_passband_direction=-1;
#endif
#if NET_BASEBRAW_MODE == WFM_AM_FULL_BANDWIDTH
  basebraw_sampling_speed=baseband_sampling_speed;
  basebraw_ad_channels=1;
  basebraw_rf_channels=1;
  basebraw_mode=DWORD_INPUT;
  basebraw_passband_direction=1;
#endif




  basebrawnet_block_bytes=(basebraw_ad_channels*basebraw_rf_channels*
                                    basebraw_sampling_speed)/ui.max_dma_rate;
  make_power_of_two(&basebrawnet_block_bytes);
// Allocate space for 0.3 seconds of data.  
  basebrawnet_size=0.3*basebraw_sampling_speed*2*
                                      ui.rx_rf_channels*sizeof(int);
  make_power_of_two(&basebrawnet_size);
  if(basebrawnet_size/basebrawnet_block_bytes < 8)
                                 basebrawnet_size=8*basebrawnet_block_bytes ;
  basebrawnet_mask=basebrawnet_size-1;
  mem(7578,&basebraw_netsend_buffer,basebrawnet_size,0);
  }
baseband_totmem=memalloc(&baseband_handle,"baseband");
if(baseband_totmem == 0)
  {
  lir_status=LIR_OK;
  baseband_size/=2;
  lir_pixwrite(bg.xleft+text_width,bg.ybottom-4*text_height,"BUFFERS REDUCED");
  goto reduce;
  }
k=fft3_size/mix2.size;
mix2.n=fft3_n;
while(k>1)
  {
  mix2.n--;
  k/=2;
  }
if(mix2.n > 12)
  {
  yieldflag_ndsp_mix2=TRUE;
  }
else
  {
  yieldflag_ndsp_mix2=FALSE;
  }
if(genparm[SECOND_FFT_ENABLE] == 0)
  {
  if(ui.max_blocked_cpus > 1)yieldflag_ndsp_mix2=FALSE;
  }
else  
  {
  if(ui.max_blocked_cpus > 3)yieldflag_ndsp_mix2=FALSE;
  }
prepare_mixer(&mix2, THIRD_FFT_SINPOW);
if(fft1_correlation_flag != 0)
  {
  make_d_sincos(0, mix2.size, d_mix2_table);
  }
if(genparm[CW_DECODE_ENABLE] != 0)
  {
  make_window(4,mix2.size, genparm[THIRD_FFT_SINPOW], cw_carrier_window);
  }
memset(basblock_maxpower,0,basblock_size*sizeof(float));
memset(basblock_avgpower,0,basblock_size*sizeof(float));
clr_size=4*mix2.size;
if(clr_size > baseband_size)clr_size=baseband_size;  
lir_sched_yield();
clear_baseb_arrays(0,clr_size);
if(genparm[CW_DECODE_ENABLE] != 0)
  {
  for(i=0; i<max_cwdat; i++)
    {
    cw[i].tmp=-1;
    cw[i].unkn=-1;
    }
  lir_sched_yield();
  memset(keying_spectrum,0,keying_spectrum_size*sizeof(float));
  }
memset(baseb_out,0,clr_size*2*baseb_channels*sizeof(float));
baseb_pa=0;
baseb_py=0;
baseb_wts=0;
baseb_fx=0;
baseb_pf=0;
fm1_pa=0;
fm1_px=0;
rx_daout_cos=1;
rx_daout_sin=0;
if(kill_all_flag)return;
am_dclevel1=0;
am_dclevel2=0;
am_dclevel_factor1=pow(0.5,2000./(baseband_sampling_speed*(1<<bg.agc_release)));
am_dclevel_factor2=1-am_dclevel_factor1;
baseb_indicator_block=(baseband_mask+1)/INDICATOR_SIZE;
daout_indicator_block=(daout_bufmask+1)/INDICATOR_SIZE;
// *********************** pilot tone filter *****************************
if(fm_pilot_size > 0)
  {
  t1=0;
  t2=2*PI_L*19000./baseband_sampling_speed;
  for(i=0; i<fm_pilot_size; i++)
    {
    fm_pilot_tone[2*i  ]=cos(t1);
    fm_pilot_tone[2*i+1]=sin(t1);
    t1+=t2;
    }
// *********************** fmfil70 *********************************          
#if NET_BASEBRAW_MODE == WFM_FM_FIRST_LOWPASS || \
    NET_BASEBRAW_MODE == WFM_AM_FIRST_LOWPASS || \
    NET_BASEBRAW_MODE == WFM_SYNTHETIZED_FROM_FIRST_LOWPASS
  i=fmfil70_size*65000/baseband_sampling_speed;
#else
  i=fmfil70_size*68000/baseband_sampling_speed;


#endif
  construct_fir(7,fmfil70_size, fmfil70_n, 
                                 &fmfil70_points, i, fmfil70_fir, 0.00001);
// *********************** fmfil55 *********************************          
  i=fmfil55_size*53000/baseband_sampling_speed;
  construct_fir(7,fmfil55_size, fmfil55_n, 
                                 &fmfil55_points, i, fmfil55_fir, 0.00001);
// *********************** fmfil *********************************          
  t1=15000;
  if(genparm[DA_OUTPUT_SPEED] < 40000)
    {
    t1*=genparm[DA_OUTPUT_SPEED]/40000.;
    }
  i=fmfil_size*t1/baseband_sampling_speed;
  construct_fir(7,fmfil_size, fmfil_n, 
                                 &fmfil_points, i, fmfil_fir, 0.00001);
// *********************** fmfil_rds ********************************
  i=fmfil_rds_size*4000/baseband_sampling_speed;
  construct_fir(7,fmfil_rds_size, fmfil_rds_n, 
                               &fmfil_rds_points, i, fmfil_rds_fir, 0.0001);
// *****************************************************************
// collect the RDS phase in a leaky integrator of about 0.01 second.
  rds_f1=pow(0.5,100/baseband_sampling_speed);
  rds_f2=1-rds_f1;
  rds_phase=0;
  rds_power=0;
  }
DEB"\nbaseband_sampling_speed=%f",baseband_sampling_speed);
}

void update_squelch_buttons(void)
{
int colour;
char s[40];
if(bg_filter_points > 5 && 2*bg_filter_points < bg_xpoints)
  {
  colour=BG_ACTIVE_BUTTON_COLOR;
  }
else
  {
  colour=BG_INACTIVE_BUTTON_COLOR;
  }
settextcolor(colour);  
sprintf(s,"%2d",bg.squelch_level);
show_button(&bgbutt[BG_SQUELCH_LEVEL],s);
if(bg.squelch_level == 0)colour=3;
settextcolor(colour);  
sprintf(s,"%d",bg.squelch_time);
show_button(&bgbutt[BG_SQUELCH_TIME],s);
sprintf(s,"%2d",bg.squelch_point);
show_button(&bgbutt[BG_SQUELCH_POINT],s);
settextcolor(7);
} 


void make_bg_filter(void)
{
int i,k,max,mm;
int j, iy;
int ja, jb, m;
float t1,t2,t3,t4;
double dt1, dt2, dt3, dt4;
int ib,ic;
// Set up the filter function in the baseband for
// the main signal and show it on the screen.
// bg.filter_flat is the size of the flat region
// of the filter in Hz (divided by 2)
// bg.filter_curv is the curvature expressed as ten times the distance 
// in Hz to the 6dB point from the end of the flat region.
// The filter response is a flat center region with parabolic fall off.
//
// The filter is applied to the fft3 transforms with a symmetric
// function which is flat over 2*bg_flatpoints and falls off over
// bg_curvpoints at each end.
if(flat_xpixel > 0)
  {
  for(i=bg_ymax; i<=bg_y4; i++)lir_setpixel(flat_xpixel, i,bg_background[i]);
  }
if(curv_xpixel > 0)
  {
  for(i=bg_y4; i<bg_y3; i++)lir_setpixel(curv_xpixel, i,bg_background[i]);
  }
if(rx_mode != MODE_FM)
  {
// We should not generate frequencies above the Nyquist frequency 
// of our output.
  max=(float)(fft3_size)*genparm[DA_OUTPUT_SPEED]/(2*timf3_sampling_speed);
  if( (ui.network_flag & NET_RXOUT_BASEB) != 0)max*=2;
// We must make bg_filter_points smaller than fft3_size/2.
  if(max > MAX_BASEBFILTER_WIDTH*fft3_size/2-2)max=MAX_BASEBFILTER_WIDTH*fft3_size/2-2;
  }
else
  {
  max=MAX_BASEBFILTER_WIDTH*fft3_size/2-2;
  }
bg_flatpoints=bg.filter_flat/bg_hz_per_pixel;
if(bg_flatpoints < 1)bg_flatpoints=1;
if(mg.scale_type == MG_SCALE_DBHZ && cg.meter_graph_on != 0)
  {
  if(bg_flatpoints < 8)bg_flatpoints=8;
  }
bg_curvpoints=0.1*bg.filter_curv/bg_hz_per_pixel;
if( genparm[CW_DECODE_ENABLE] != 0)
  {
  k=bg_xpixels/(2*bg.pixels_per_point)-NOISE_SEP-
                                NOISE_FILTERS*(NOISE_POINTS+NOISE_SEP)-1;
  if(max>k)max=k;
  k=bg_flatpoints+2*bg_curvpoints-max;  
  if(k>0)
    {
    k/=2;
    bg_flatpoints-=k;
    if(bg_flatpoints<1)bg_flatpoints=1;
    bg_curvpoints=(max-bg_flatpoints-1)/2;
    if(bg_curvpoints < 0)
      {
      k=1+2*bg_curvpoints;
      bg_flatpoints+=k;
      bg_curvpoints=0;
      }
    }
  }
else
  {
  k=bg_flatpoints+bg_curvpoints-max;  
  if(k>0)
    {
    k=(k+1)/2;
    bg_flatpoints-=k;
    if(bg_flatpoints<1)bg_flatpoints=1;
    bg_curvpoints=max-bg_flatpoints-1;
    if(bg_curvpoints < 0)
      {
      bg_flatpoints+=bg_curvpoints;
      bg_curvpoints=0;
      }
    }
  }
bg.filter_flat=bg_hz_per_pixel*bg_flatpoints;
bg.filter_curv=10*bg_hz_per_pixel*bg_curvpoints;
bg_curvpoints=0.1*bg.filter_curv/bg_hz_per_pixel;
bg_flatpoints=bg.filter_flat/bg_hz_per_pixel;
if(bg_flatpoints < 1)bg_flatpoints=1;
if(fft1_correlation_flag != 0)
  {
  d_bgfil_weight=1;
  d_timf3_float[fft3_size/2]=1;
  for(i=1; i<bg_flatpoints; i++)
    {
    d_timf3_float[fft3_size/2+i]=1;
    d_timf3_float[fft3_size/2-i]=1;
    d_bgfil_weight+=2;  
    }
  bg_filter_points=bg_flatpoints;  
  if(bg_curvpoints > 0)
    {
    dt1=.5/bg_curvpoints;
    dt2=1;
    dt3=dt1;
    dt2=1-dt3*dt3;
    dt3+=dt1;
    while(dt2 > 0 && bg_filter_points<fft3_size/2)
      {
      d_timf3_float[fft3_size/2+bg_filter_points]=dt2;
      d_timf3_float[fft3_size/2-bg_filter_points]=dt2;
      d_bgfil_weight+=2*dt2*dt2;  
      dt2=1-dt3*dt3;
      dt3+=dt1;
      bg_filter_points++;
      }
    }  
  for(i=bg_filter_points; i<fft3_size/2; i++)
    {
    d_timf3_float[fft3_size/2+i]=0;
    d_timf3_float[fft3_size/2-i]=0;
    }
  d_timf3_float[0]=0;
  }
bgfil_weight=1;
bg_filterfunc[fft3_size/2]=1;
for(i=1; i<bg_flatpoints; i++)
  {
  bg_filterfunc[fft3_size/2+i]=1;
  bg_filterfunc[fft3_size/2-i]=1;
  bgfil_weight+=2;  
  }
bg_filter_points=bg_flatpoints;  
if(bg_curvpoints > 0)
  {
  t1=.5/bg_curvpoints;
  t2=1;
  t3=t1;
  t2=1-t3*t3;
  t3+=t1;
  while(t2 > 0 && bg_filter_points<fft3_size/2)
    {
    bg_filterfunc[fft3_size/2+bg_filter_points]=t2;
    bg_filterfunc[fft3_size/2-bg_filter_points]=t2;
    bgfil_weight+=2*t2*t2;  
    t2=1-t3*t3;
    t3+=t1;
    bg_filter_points++;
    }
  }  
for(i=bg_filter_points; i<fft3_size/2; i++)
  {
  bg_filterfunc[fft3_size/2+i]=0;
  bg_filterfunc[fft3_size/2-i]=0;
  }
bg_filterfunc[0]=0;
// The filter we just specified determines the bandwidth of
// the signal we recover when backtransforming from fft3.
// Find out by what factor we should reduce the sampling speed
// for further processing
if(fft1_correlation_flag != 0)
  {
  k=1;
  }
else
  {  
  k=bg_filter_points+binshape_points;
  k=(0.4*(fft3_size+0.5*k))/k;
  make_power_of_two(&k);
  if(genparm[CW_DECODE_ENABLE] != 0)k/=2;
  }
if(k > 2)
  { 
  baseband_sampling_speed=2*timf3_sampling_speed/k;
  if((int)mix2.size != 2*fft3_size/k || 
     (reinit_baseb && mouse_active_flag==0))
    {
    mix2.size=2*fft3_size/k;
    init_basebmem();
    baseb_reset_counter++;
    }
  }
else
  {
  baseband_sampling_speed=timf3_sampling_speed;   
  if((int)mix2.size != fft3_size ||
     (reinit_baseb && mouse_active_flag==0))
    {
    mix2.size=fft3_size;
    init_basebmem();
    baseb_reset_counter++;
    }  
  }
timf1_to_baseband_speed_ratio=rint(timf1_sampling_speed/baseband_sampling_speed);
timf2_blockpower_block=4*ui.rx_rf_channels*timf1_to_baseband_speed_ratio;
if((int)mix2.size>fft3_size)
  {
  lirerr(88888);  
  return;
  }
if(fft1_correlation_flag != 0)
  {
  d_carrfil_weight=1;
  d_fftn_tmp[fft3_size/2]=1;
  mm=1;
  k=bg.coh_factor;
  while(mm < fft3_size/2)
    {
    if(k<fft3_size/2)
      {
      dt2=d_timf3_float[fft3_size/2+k];
      }
    else
      {
      dt2=0;
      }  
    d_fftn_tmp[fft3_size/2+mm]=dt2;
    d_fftn_tmp[fft3_size/2-mm]=dt2;
    d_carrfil_weight+=2*dt2*dt2;
    mm++;
    k+=bg.coh_factor;
    }
  d_fftn_tmp[0]=0;
  }
carrfil_weight=1;
bg_carrfilter[fft3_size/2]=1;
mm=1;
k=bg.coh_factor;
while(mm < fft3_size/2)
  {
  if(k<fft3_size/2)
    {
    t2=bg_filterfunc[fft3_size/2+k];
    }
  else
    {
    t2=0;
    }  
  bg_carrfilter[fft3_size/2+mm]=t2;
  bg_carrfilter[fft3_size/2-mm]=t2;
  carrfil_weight+=2*t2*t2;
  mm++;
  k+=bg.coh_factor;
  }
bg_carrfilter[0]=0;
bg_filtershift_points=bg.filter_shift*(fft3_size/4+8)/999;
// Shift bg_filterfunc as specified by bg_filtershift_points
// But do it only if the mixer mode is back transformation
// and Rx mode is AM
if(bg.mixer_mode == 1 && rx_mode == MODE_AM)
  {
  if(bg_filtershift_points > 0)
    {
    k=0;
    for(i=bg_filtershift_points; i< fft3_size; i++)
      {
      bg_filterfunc[k]=bg_filterfunc[i];
      k++;
      }
    while(k < fft3_size)
      {
      bg_filterfunc[k]=0;
      k++;
      }
    }
  if(bg_filtershift_points < 0)
    {
    k=fft3_size-1;
    for(i=fft3_size+bg_filtershift_points; i>=0; i--)
      {
      bg_filterfunc[k]=bg_filterfunc[i];
      k--;
      }
    while(k >= 0)
      {
      bg_filterfunc[k]=0;
      k--;
      }
    }
  }  
// **************************************************************
// Place the current filter function on the screen.
// First remove any curve that may exist on the screen.
for(i=bg_first_xpixel; i<=bg_last_xpixel; i+=bg.pixels_per_point)
  {
  if(bg_filterfunc_y[i] < bg.ybottom && bg_filterfunc_y[i] >bg_ymax-2)
    {
    lir_setpixel(i,bg_filterfunc_y[i],bg_background[bg_filterfunc_y[i]]);
    }
  if(bg_carrfilter_y[i] < bg.ybottom && bg_carrfilter_y[i] >bg_ymax-2)
    {
    lir_setpixel(i,bg_carrfilter_y[i],bg_background[bg_carrfilter_y[i]]);
    }
  }
// Slide the bin filter shape over our carrier filter and accumulate
// to take the widening due to the fft window into account.
for(i=0; i<fft3_size; i++)
  {
  ja=i-fft3_size/2;
  jb=i+fft3_size/2;
  m=0;
  if(ja<0)
    {
    m=-ja;
    ja=0;
    }
  j=(fft3_size/2-binshape_total)-m;
  if(j>0)
    {
    ja+=j;
    m+=j;
    }
  if(jb > fft3_size)jb=fft3_size;    
  if(jb-ja > 2*binshape_total+1)jb=ja+2*binshape_total+1;
  t1=0;
  for(j=ja; j<jb; j++)
    {
    t1+=bg_carrfilter[j]*bg_binshape[m];
    m++;
    }
  bg_ytmp[i]=t1;
  }
if(fft3_size > 16384)lir_sched_yield();
t1=0;
for(i=0; i<fft3_size; i++)
  {
  if(bg_ytmp[i]>t1)t1=bg_ytmp[i];
  }
for(i=0; i<fft3_size; i++)
  {
  bg_ytmp[i]/=t1;
  }
j=bg_first_xpoint;
for(i=bg_first_xpixel; i<=bg_last_xpixel; i+=bg.pixels_per_point)
  {
  if(bg_ytmp[j] > 0.000000001)
    {
    iy=2*bg.yfac_log*log10(1./bg_ytmp[j]);  
    iy=bg_ymax-1+iy;
    if(iy>bg_y0)
      {
      iy=-1;
      }
    else
      {
      lir_setpixel(i,iy,58);
      }
    }
  else
    {
    iy=-1;
    }
  bg_carrfilter_y[i]=iy;
  j++;
  }
if(bg.mixer_mode == 1)
  {
  for(i=0; i<bg_no_of_notches; i++)
    {
    j=fft3_size/2-bg_notch_pos[i]*bg_filter_points/1000;
    k=bg_notch_width[i]*bg_filter_points/1000;
    ja=j-k;
    jb=j+k;
    if(ja < 0)ja=0;
    if(jb >= fft3_size)jb=fft3_size-1;
    for(j=ja; j<=jb; j++)
      {
      bg_filterfunc[j]=0;
      }
    }
  }  
// Slide the bin filter shape over our filter function and accumulate
// to take the widening due to the fft window into account.
for(i=0; i<fft3_size; i++)
  {
  ja=i-fft3_size/2;
  jb=i+fft3_size/2;
  m=0;
  if(ja<0)
    {
    m=-ja;
    ja=0;
    }
  j=(fft3_size/2-binshape_total)-m;
  if(j>0)
    {
    ja+=j;
    m+=j;
    }
  if(jb > fft3_size)jb=fft3_size;    
  if(jb-ja > 2*binshape_total+1)jb=ja+2*binshape_total+1;
  t1=0;
  for(j=ja; j<jb; j++)
    {
    t1+=bg_filterfunc[j]*bg_binshape[m];
    m++;
    }
  bg_ytmp[i]=t1;
  }
t1=0;
for(i=0; i<fft3_size; i++)
  {
  if(bg_ytmp[i]>t1)t1=bg_ytmp[i];
  }
t2=0;
for(i=0; i<fft3_size; i++)
  {
  bg_ytmp[i]/=t1;
  t2+=bg_ytmp[i]*bg_ytmp[i];
  }
// The integrated power spectrum is t2. With an ideal
// rectangular filter  we would need t2 points with an
// amplitude of 1.0 to get an equivalent amount of power
// from white noise. That should be the equivalent noise bandwidth.
bg_noise_bw=t2*bg_hz_per_pixel;
bg_120db_points=fft3_size;
while(bg_ytmp[bg_120db_points-1] < 0.000001 && 
                                      bg_120db_points >0)bg_120db_points--;
bg_60db_points=bg_120db_points;
while(bg_ytmp[bg_60db_points-1] < 0.001 && 
                                        bg_60db_points >0)bg_60db_points--;
bg_6db_points=bg_60db_points;
while(bg_ytmp[bg_6db_points-1] < 0.5 && bg_6db_points >0)bg_6db_points--;
if(bg_6db_points == 0)
  {
  lirerr(287553);
  return;
  }
i=0;
while(bg_ytmp[i+1] < 0.000001)i++;
bg_120db_points-=i;
while(bg_ytmp[i+1] < 0.001)i++;
bg_60db_points-=i;
while(bg_ytmp[i+1] < 0.5)i++;
bg_6db_points-=i;
baseband_bw_hz=bg_6db_points*bg_hz_per_pixel;
baseband_bw_fftxpts=baseband_bw_hz*fftx_points_per_hz;
j=bg_first_xpoint;
for(i=bg_first_xpixel; i<=bg_last_xpixel; i+=bg.pixels_per_point)
  {
  if(j>0 && j<fft3_size && bg_ytmp[j] > 0.000000001)
    {
    iy=2*bg.yfac_log*log10(1./bg_ytmp[j]);  
    iy=bg_ymax-1+iy;
    if(iy>bg_y0)
      {
      iy=-1;
      }
    else
      {    
      if(fft1_correlation_flag != 2)lir_setpixel(i,iy,14);
      }
    }
  else
    {
    iy=-1;
    }
  bg_filterfunc_y[i]=iy;
  j++;
  }
// We apply a filter in the frequency domain when picking a subset
// of fft1 or fft2 for back-transformation.
// That filter, mix1_fqwin, affects our frequency response when wide
// filters are used. Compensate for mix1_fqwin by multiplying with
// its inverse in fft3 points.
for(i=0; i<fft3_size;i++)bg_filterfunc[i]*=sqrt(fft3_fqwin_inv[i]);  
// Construct a FIR filter of size fft3 with a frequency response like the
// baseband filter we have set up.
// The array of FIR filter coefficients is the pulse response of a FIR 
// filter. Get it by taking the FFT.
// Since we use a complex FFT we can use the real side for the
// baseband filter and the imaginary part for the carrier filter.
if(fft1_correlation_flag != 0)
  {
  for(i=0; i<fft3_size;i++)d_fftn_tmp[i]*=sqrt(d_fft3_fqwin_inv[i]);  
  d_timf3_float[0]=0;
  d_timf3_float[1]=1;
  i=fft3_size/2+1;
  jb=fft3_size-1;
  for(ja=1; ja<fft3_size/2; ja++)
    {
    d_timf3_float[2*ja+1]=d_fftn_tmp[i];
    d_timf3_float[2*jb+1]=d_fftn_tmp[i];
    d_timf3_float[2*ja  ]=0;
    d_timf3_float[2*jb  ]=0;
    i++;
    jb--;
    }
  d_timf3_float[2*jb  ]=0;
  d_timf3_float[2*jb+1]=0;
  for( i=0; i<fft3_size/2; i++)
    {
    dt1=d_timf3_float[2*i  ];
    dt2=d_timf3_float[fft3_size+2*i  ];
    dt3=d_timf3_float[2*i+1];
    dt4=d_timf3_float[fft3_size+2*i+1];
    d_timf3_float[2*i  ]=dt1+dt2;
    d_timf3_float[2*i+1]=dt3+dt4;
    d_timf3_float[fft3_size+2*i  ]=
                d_fft3_tab[i].cos*(dt1-dt2)+d_fft3_tab[i].sin*(dt4-dt3);
    d_timf3_float[fft3_size+2*i+1]=
                d_fft3_tab[i].sin*(dt1-dt2)-d_fft3_tab[i].cos*(dt4-dt3);
    } 
  d_bulk_of_dif(fft3_size,fft3_n,d_timf3_float,
                                          d_fft3_tab, yieldflag_ndsp_fft3);
  for(i=0; i < fft3_size; i+=2)
    {
    ib=fft3_permute[i];                               
    ic=fft3_permute[i+1];
    d_basebcarr_fir[ib]=d_timf3_float[2*i+1]+d_timf3_float[2*i+3];
    d_basebcarr_fir[ic]=d_timf3_float[2*i+1]-d_timf3_float[2*i+3];
    }
  }
fft3_tmp[0]=1;
fft3_tmp[1]=1;
i=fft3_size/2+1;
jb=fft3_size-1;
for(ja=1; ja<fft3_size/2; ja++)
  {
  fft3_tmp[2*ja  ]=bg_filterfunc[i];
  fft3_tmp[2*jb  ]=bg_filterfunc[i];
  fft3_tmp[2*ja+1]=bg_carrfilter[i];
  fft3_tmp[2*jb+1]=bg_carrfilter[i];
  i++;
  jb--;
  }
fft3_tmp[2*jb  ]=0;
fft3_tmp[2*jb+1]=0;
for( i=0; i<fft3_size/2; i++)
  {
  t1=fft3_tmp[2*i  ];
  t2=fft3_tmp[fft3_size+2*i  ];
  t3=fft3_tmp[2*i+1];
  t4=fft3_tmp[fft3_size+2*i+1];
  fft3_tmp[2*i  ]=t1+t2;
  fft3_tmp[2*i+1]=t3+t4;
  fft3_tmp[fft3_size+2*i  ]=fft3_tab[i].cos*(t1-t2)+fft3_tab[i].sin*(t4-t3);
  fft3_tmp[fft3_size+2*i+1]=fft3_tab[i].sin*(t1-t2)-fft3_tab[i].cos*(t4-t3);
  } 
if(fft3_size > 2*8192)lir_sched_yield();
#if IA64 == 0 && CPU == CPU_INTEL
asmbulk_of_dif(fft3_size, fft3_n, fft3_tmp, fft3_tab, yieldflag_ndsp_fft3);
#else
bulk_of_dif(fft3_size, fft3_n, fft3_tmp, fft3_tab, yieldflag_ndsp_fft3);
#endif
if(fft3_size > 8192)lir_sched_yield();
for(i=0; i < fft3_size; i+=2)
  {
  ib=fft3_permute[i];                               
  ic=fft3_permute[i+1];
  basebraw_fir[ib]=fft3_tmp[2*i  ]+fft3_tmp[2*i+2];
  basebraw_fir[ic]=fft3_tmp[2*i  ]-fft3_tmp[2*i+2];
  basebcarr_fir[ib]=fft3_tmp[2*i+1]+fft3_tmp[2*i+3];
  basebcarr_fir[ic]=fft3_tmp[2*i+1]-fft3_tmp[2*i+3];
  }
// Now take the effects of our window into account.
// Note the order in which fft3_window is stored.
for(i=0; i<fft3_size/2; i++)
  {
  basebraw_fir[i]*=fft3_window[2*i];  
  basebcarr_fir[i]*=fft3_window[2*i];  
  basebraw_fir[fft3_size/2+i]*=fft3_window[2*i+1];  
  basebcarr_fir[fft3_size/2+i]*=fft3_window[2*i+1];  
  }
// The FIR filter must be symmetric. Actually forcing symmetry
// might reduce rounding errors slightly.
for(i=1; i<fft3_size/2; i++)
  {
  basebraw_fir[i]=0.5*(basebraw_fir[i]+basebraw_fir[fft3_size-i]);
  basebcarr_fir[i]=0.5*(basebcarr_fir[i]+basebcarr_fir[fft3_size-i]);
  basebraw_fir[fft3_size-i]=basebraw_fir[i];
  basebcarr_fir[fft3_size-i]=basebcarr_fir[i];
  }
// The fft1 algorithms use float variables with 24 bit accuracy.
// the associated spur level is -140 dB.
// There is no reason to make the FIR filter extend outside
// the range where the coefficients are below -140 dB.
t1=1.e-7*basebraw_fir[fft3_size/2];
k=0;
while(fabs(basebraw_fir[k]) < t1 && k < fft3_size)
  {
  k++;
  }
j=k;
basebraw_fir_pts=0;
while(k <= fft3_size-j+1)
  {
  basebraw_fir[basebraw_fir_pts]=basebraw_fir[k];
  k++;
  basebraw_fir_pts++;
  }
basebraw_fir_pts-=2;  
k=basebraw_fir_pts;
memset(&basebraw_fir[k],0,(fft3_size-k)*sizeof(float));
t1=1.e-7*basebcarr_fir[fft3_size/2];
k=0;
while(fabs(basebcarr_fir[k]) < t1 && k < fft3_size)k++;
j=k;
basebcarr_fir_pts=0;
while(k<fft3_size-j+1)
  {
  basebcarr_fir[basebcarr_fir_pts]=basebcarr_fir[k];
  k++;
  basebcarr_fir_pts++;
  }
basebcarr_fir_pts-=2;  
k=basebcarr_fir_pts;
memset(&basebcarr_fir[k],0,(fft3_size-k)*sizeof(float));
// Normalize the FIR filter so it gives the same amplitude as we have
// with the back transformation of fft3 in mix2.
t1=0;
for(i=0; i<basebraw_fir_pts; i++)
  {
  t1+=basebraw_fir[i];
  }
t1=2.78*fft3_size/t1;  
for(i=0; i<basebraw_fir_pts; i++)
  {
  basebraw_fir[i]*=t1;
  }
t1=0;
for(i=0; i<basebcarr_fir_pts; i++)
  {
  t1+=basebcarr_fir[i];
  }
t1=2.78*fft3_size/t1;  
for(i=0; i<basebcarr_fir_pts; i++)
  {
  basebcarr_fir[i]*=t1;
  }
if(fft1_correlation_flag != 0)
  {
  for(i=0; i<fft3_size/2; i++)
    {
    d_basebcarr_fir[i]*=d_fft3_window[2*i];  
    d_basebcarr_fir[fft3_size/2+i]*=d_fft3_window[2*i+1];  
    }
// The FIR filter must be symmetric. Actually forcing symmetry
// might reduce rounding errors slightly. (but not needed with double floaf...
  for(i=1; i<fft3_size/2; i++)
    {
    d_basebcarr_fir[i]=0.5*(d_basebcarr_fir[i]+d_basebcarr_fir[fft3_size-i]);
    d_basebcarr_fir[fft3_size-i]=d_basebcarr_fir[i];
    }
// The fft1 algorithms use float variables with 24 bit accuracy.
// the associated spur level is -140 dB.
// with correlation we can go deep into the noise....
// There is no reason to make the FIR filter extend outside
// the range where the coefficients are below -200 dB.
  dt1=1.e-10*d_basebcarr_fir[fft3_size/2];
  k=0;
  while(fabs(d_basebcarr_fir[k]) < dt1 && k < fft3_size)k++;
  j=k;
  d_basebcarr_fir_pts=0;
  while(k<fft3_size-j+1)
    {
    d_basebcarr_fir[d_basebcarr_fir_pts]=d_basebcarr_fir[k];
    k++;
    d_basebcarr_fir_pts++;
    }
  d_basebcarr_fir_pts-=2;  
  k=d_basebcarr_fir_pts;
  memset(&d_basebcarr_fir[k],0,(fft3_size-k)*sizeof(double));
// Normalize the FIR filter so it gives the same amplitude as we have
// with the back transformation of fft3 in mix2.
  dt1=0;
  for(i=0; i<basebcarr_fir_pts; i++)
    {
    dt1+=d_basebcarr_fir[i];
    }
  dt1=2.78*fft3_size/dt1;  
  for(i=0; i<d_basebcarr_fir_pts; i++)
    {
    d_basebcarr_fir[i]*=dt1;
    }
  }
if(fft3_size > 4096)lir_sched_yield();
flat_xpixel=filcur_pixel(bg_flatpoints);
lir_line(flat_xpixel, bg_ymax,flat_xpixel,bg_y4-1,14);
curv_xpixel=filcur_pixel(bg_curvpoints);
lir_line(curv_xpixel, bg_y4,curv_xpixel,bg_y3-1,14);
da_resample_ratio=genparm[DA_OUTPUT_SPEED]/baseband_sampling_speed;
make_new_daout_upsamp();
// Place the BFO line on the screen and make bfo related stuff.
make_bfo();
// Set up a 5th order IIR low pass filter to apply after the
// fractional resampler in make_audio_signal (rxout.c)
// The resampler produces aliases that this filter will remove.
t1=0.25*(bg_60db_points+bg_6db_points)*daout_upsamp*
                               bg_hz_per_pixel/genparm[DA_OUTPUT_SPEED];
if(t1 > 0.25)
  {
  enable_resamp_iir5=FALSE;
  }
else
  {
  if(t1<0.011)t1=0.011;  
  make_iir5(t1, &iir_a);
  for(i=0; i<6; i++)
    {
    xva1[i]=0;
    yva1[i]=0;
    xvb1[i]=0;
    yvb1[i]=0;
    xva2[i]=0;
    yva2[i]=0;
    xvb2[i]=0;
    yvb2[i]=0;
    }
  enable_resamp_iir5=TRUE;
  }  
out1=0;
out2=0;
out3=0;
out4=0;
for(i=0; i<4; i++)
  {
  xv1[i]=0;
  yv1[i]=0;
  xv2[i]=0;
  yv2[i]=0;
  xv3[i]=0;
  yv3[i]=0;
  xv4[i]=0;
  yv4[i]=0;
  }
update_squelch_buttons();
}

void check_bg_cohfac(void)
{
int j;
j=0.8*bg_6db_points;  
if(j>9999)j=9999;
if(bg.coh_factor > j)bg.coh_factor=j;
if(use_bfo != 0)
  {
  if(bg.coh_factor < 3)bg.coh_factor=3;
  }
else
  {
  if(bg.coh_factor < 1)bg.coh_factor=1;
  }
}

void check_bg_fm_audio_bw(void)
{
if(bg.fm_audio_bw < 1)bg.fm_audio_bw=1;
if(bg.fm_audio_bw > genparm[DA_OUTPUT_SPEED]/2000.)
  {
  bg.fm_audio_bw=genparm[DA_OUTPUT_SPEED]/2000.;
  }
}

void new_bg_cohfac(void)
{
bg.coh_factor=numinput_int_data;
check_bg_cohfac();
init_baseband_sizes();
make_baseband_graph(TRUE);
}

void new_bg_fm_audio_bw(void)
{
bg.fm_audio_bw=numinput_int_data;
check_bg_fm_audio_bw();
init_baseband_sizes();
make_baseband_graph(TRUE);
}

void new_bg_filter_shift(void)
{
int i;
i=numinput_int_data;
// i has to be between -999 and 9999 because we allocate 4 characters
// for this parameter button.
if(i > 999)i=999;
bg.filter_shift=i;
make_baseband_graph(TRUE);
}

void new_bg_delpnts(void)
{
bg.delay_points=numinput_int_data;
if(bg.delay_points < 1)bg.delay_points=1;
if(bg.delay_points > 999)bg.delay_points=999;
sc[SC_BG_BUTTONS]++;
}


void new_bg_agc_attack(void)
{
bg.agc_attack=numinput_int_data;
sc[SC_BG_BUTTONS]++;
make_modepar_file(GRAPHTYPE_BG);
clear_agc();
}

void new_bg_agc_release(void)
{
bg.agc_release=numinput_int_data;
am_dclevel_factor1=pow(0.5,2000./(baseband_sampling_speed*(1<<bg.agc_release)));
am_dclevel_factor2=1-am_dclevel_factor1;
sc[SC_BG_BUTTONS]++;
make_modepar_file(GRAPHTYPE_BG);
clear_agc();
}

void new_bg_agc_hang(void)
{
bg.agc_hang=numinput_int_data;
sc[SC_BG_BUTTONS]++;
make_modepar_file(GRAPHTYPE_BG);
clear_agc();
}

void new_bg_waterfall_gain(void)
{
bg.waterfall_gain=numinput_float_data;
sc[SC_BG_BUTTONS]++;
make_bg_waterf_cfac();
make_modepar_file(GRAPHTYPE_BG);
sc[SC_BG_WATERF_REDRAW]++;
}

void new_bg_waterfall_zero(void)
{
bg.waterfall_zero=numinput_float_data;
sc[SC_BG_BUTTONS]++;
make_bg_waterf_cfac();
make_modepar_file(GRAPHTYPE_BG);
sc[SC_BG_WATERF_REDRAW]++;
}

void help_on_baseband_graph(void)
{
int msg_no;
int event_no;
// Set msg invalid in case we are not in any select area.
msg_no=-1;
if(mouse_y < bg_y0)
  {
  if(mouse_x < bg_first_xpixel)
    {
    if(mouse_x >= bg_vol_x1 && 
       mouse_x <= bg_vol_x2 &&
       mouse_y >= bg_ymax)
      {   
      msg_no=33;
      }
    }
  else
    { 
    if(mouse_x < bg.xright-text_width-6)
      { 
      msg_no=34;
      }
    }
  }
for(event_no=0; event_no<MAX_BGBUTT; event_no++)
  {
  if( bgbutt[event_no].x1 <= mouse_x && 
      bgbutt[event_no].x2 >= mouse_x &&      
      bgbutt[event_no].y1 <= mouse_y && 
      bgbutt[event_no].y2 >= mouse_y) 
    {
    switch (event_no)
      {
      case BG_TOP:
      case BG_BOTTOM:
      case BG_LEFT:
      case BG_RIGHT:
      case BG_YBORDER:
      msg_no=100;
      break;

      case BG_YSCALE_EXPAND:
      msg_no=35;
      break;

      case BG_YSCALE_CONTRACT:
      msg_no=36;
      break;

      case BG_YZERO_DECREASE:
      msg_no=37;
      break;

      case BG_YZERO_INCREASE:
      msg_no=38;
      break;

      case BG_RESOLUTION_DECREASE:
      msg_no=39;
      break;

      case BG_RESOLUTION_INCREASE:
      msg_no=40;
      break;

      case BG_OSCILLOSCOPE: 
      msg_no=41;
      break;

      case BG_OSC_INCREASE: 
      msg_no=42;
      break;

      case BG_OSC_DECREASE:
      msg_no=43;
      break;

      case BG_PIX_PER_PNT_INC:
      msg_no=44;
      break;    

      case BG_PIX_PER_PNT_DEC:
      msg_no=45;
      break;    

      case BG_TOGGLE_EXPANDER:
      msg_no=46;
      break;

      case  BG_TOGGLE_COHERENT:
      msg_no=47;
      break;

      case  BG_TOGGLE_PHASING:
      msg_no=48;
      break;

      case BG_TOGGLE_TWOPOL:
      msg_no=49;
      break;

      case  BG_TOGGLE_CHANNELS:
      msg_no=50;
      break;

      case BG_TOGGLE_CH2_PHASE:
      msg_no=328;
      break;
      
      case  BG_TOGGLE_BYTES:
      msg_no=51;
      break;

      case BG_SEL_COHFAC:
      msg_no=52;
      break;

      case BG_SEL_DELPNTS:
      msg_no=53;
      break;

      case BG_SEL_FFT3AVGNUM:
      msg_no=63;
      break;

      case BG_WATERF_AVGNUM:
      msg_no=61;
      break;

      case BG_SEL_AGC_ATTACK:
      msg_no=80;
      break;

      case BG_TOGGLE_AGC:
      msg_no=79;
      break;

      case BG_SEL_AGC_RELEASE:
      msg_no=81;
      break;

      case BG_SEL_AGC_HANG:
      msg_no=331;
      break;

      case BG_WATERF_ZERO:
      msg_no=64;
      break;

      case BG_WATERF_GAIN:
      msg_no=66;
      break;

      case BG_HORIZ_ARROW_MODE:
      msg_no=319;
      break;

      case BG_MIXER_MODE:
      msg_no=88;
      break;

      case BG_FILTER_SHIFT:
      msg_no=329;
      break;
      
      case BG_NOTCH_NO:
      msg_no=320;
      break;

      case BG_NOTCH_WIDTH:
      msg_no=321;
      break;  

      case BG_NOTCH_POS:
      msg_no=322;
      break;
      
      case BG_TOGGLE_FM_MODE:
      msg_no=326;
      break;

      case BG_TOGGLE_FM_SUBTRACT:
      msg_no=330;
      break;
        
      case BG_SEL_FM_AUDIO_BW:
      msg_no=327;
      break;

      case BG_SQUELCH_LEVEL:
      msg_no=336;
      break;
  
      case BG_SQUELCH_TIME:
      msg_no=338;
      break;

      case BG_SQUELCH_POINT:
      msg_no=337;
      break;
      
      }
    }  
  }
help_message(msg_no);
}  

void change_fft3_avgnum(void)
{
int j,k;
bg.fft_avgnum=numinput_int_data;
chk_bg_avgnum();
make_modepar_file(GRAPHTYPE_BG);
if(sw_onechan)
  {  
  for(k=bg_first_xpixel; k<=bg_last_xpixel; k++)
    {
    lir_setpixel(k,fft3_spectrum[k],bg_background[fft3_spectrum[k]]);
    fft3_spectrum[k]=bg_y0;
    }
  }
else
  {
  for(k=bg_first_xpixel; k<=bg_last_xpixel; k++)
    {
    for(j=2*k; j<2*k+2; j++)
      {
      lir_setpixel(k,fft3_spectrum[j],bg_background[fft3_spectrum[j]]);
      fft3_spectrum[j]=bg_y0;
      }
    }
  }       
init_baseband_sizes();
make_baseband_graph(TRUE);
}



void change_bg_waterf_avgnum(void)
{
bg.waterfall_avgnum=numinput_int_data;
make_modepar_file(GRAPHTYPE_BG);
make_bg_yfac();
sc[SC_BG_BUTTONS]++;
}

void make_agc_amplimit(void)
{
bg_agc_amplimit=bg_amplimit*0.18;
if(rx_mode==MODE_AM)
  {
  bg_agc_amplimit*=1.2;
  if(bg_coherent != 0) bg_agc_amplimit*=.75;
  }
}

void new_bg_no_of_notches(void)
{
if(numinput_int_data > bg_no_of_notches)
  {
  if(bg_no_of_notches < MAX_BG_NOTCHES)
    {
    bg_notch_pos[bg_no_of_notches]=0;
    bg_notch_width[bg_no_of_notches]=0;
    bg_no_of_notches++;
    }
  }
bg_current_notch=numinput_int_data;
if(bg_current_notch > bg_no_of_notches)bg_current_notch=bg_no_of_notches;
if(bg_current_notch <= 0)
  {
  bg_current_notch=0;
  bg_no_of_notches=0;
  }
//sc[SC_BG_BUTTONS]++;
make_baseband_graph(TRUE);
}

void new_bg_notch_width(void)
{
int i;
if(bg_current_notch > 0)
  {
  if(numinput_int_data < 0)
    {
    if(bg_no_of_notches > 0)
      {
      i=bg_current_notch-1;
      while(i < bg_no_of_notches-1)
        {
        bg_notch_pos[i]=bg_notch_pos[i+1];
        bg_notch_width[i]=bg_notch_width[i+1];
        i++;
        }
      bg_no_of_notches--;
      }
    }    
  else
    {
    bg_notch_width[bg_current_notch-1]=numinput_int_data;
    }
  }
//sc[SC_BG_BUTTONS]++;
make_baseband_graph(TRUE);
}


void new_bg_notch_pos(void)
{
if(bg_current_notch > 0)
  {
  bg_notch_pos[bg_current_notch-1]=numinput_int_data;
  if(bg_notch_pos[bg_current_notch-1] > 999)
                                  bg_notch_pos[bg_current_notch-1]=999;
  }
//sc[SC_BG_BUTTONS]++;
make_baseband_graph(TRUE);
}

void new_bg_squelch_point(void)
{
bg.squelch_point=numinput_int_data;
if(bg.squelch_point < 0)bg.squelch_point=0;
make_modepar_file(GRAPHTYPE_BG);
update_squelch_buttons();
}

void new_bg_squelch_time(void)
{
bg.squelch_time=numinput_int_data;
make_modepar_file(GRAPHTYPE_BG);
update_squelch_buttons();
}

void new_bg_squelch_level(void)
{
bg.squelch_level=numinput_int_data;
if(bg.squelch_level < 0)bg.squelch_level=0;
make_modepar_file(GRAPHTYPE_BG);
update_squelch_buttons();
}

void mouse_new_frequency(void)
{
if(leftpressed == BUTTON_RELEASED)
  {
  leftpressed=BUTTON_IDLE;
  mouse_active_flag=0;
  baseb_control_flag=0;
  move_rx_frequency(((float)(mouse_x-bg_first_xpixel)/bg.pixels_per_point
                           -fft3_size/2+bg_first_xpoint)*bg_hz_per_pixel,0);
  }
}

void mouse_continue_baseband_graph(void)
{
int j;
switch (mouse_active_flag-1)
  {
  case BG_TOP:
  if(bg.ytop!=mouse_y)
    {
    pause_screen_and_hide_mouse();
    dual_graph_borders((void*)&bg,0);
    bg.ytop=mouse_y;
    j=bg.ybottom-BG_MINYCHAR*text_height;
    if(bg.ytop > j)bg.ytop=j;      
    if(bg_old_y1 > bg.ytop)bg_old_y1=bg.ytop;
    dual_graph_borders((void*)&bg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case BG_BOTTOM:
  if(bg.ybottom!=mouse_y)
    {
    pause_screen_and_hide_mouse();
    dual_graph_borders((void*)&bg,0);
    bg.ybottom=mouse_y;
    j=bg.yborder+BG_MINYCHAR*text_height;
    if(bg.ybottom < j)bg.ybottom=j;      
    if(bg.ybottom >= screen_height)bg.ybottom=screen_height-1; 
    if(bg_old_y2 < bg.ybottom)bg_old_y2=bg.ybottom;
    dual_graph_borders((void*)&bg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case BG_LEFT:
  if(bg.xleft!=mouse_x)
    {
    pause_screen_and_hide_mouse();
    dual_graph_borders((void*)&bg,0);
    bg.xleft=mouse_x;
    j=bg.xright-BG_MIN_WIDTH;
    if(j<0)j=0;
    if(bg.xleft > j)bg.xleft=j;      
    if(bg_old_x1 > bg.xleft)bg_old_x1=bg.xleft;
    dual_graph_borders((void*)&bg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case BG_RIGHT:
  if(bg.xright!=mouse_x)
    {
    pause_screen_and_hide_mouse();
    dual_graph_borders((void*)&bg,0);
    bg.xright=mouse_x;
    j=bg.xleft+BG_MIN_WIDTH;
    if(j>=screen_width)j=screen_width-1;
    if(bg.xright < j)bg.xright=j;      
    if(bg_old_x2 < bg.xright)bg_old_x2=bg.xright;
    dual_graph_borders((void*)&bg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case BG_YBORDER:
  if(bg.yborder!=mouse_y)
    {
    pause_screen_and_hide_mouse();
    dual_graph_borders((void*)&bg,0);
    bg.yborder=mouse_y;
    if(bg.yborder < bg_yborder_min)bg.yborder = bg_yborder_min;
    if(bg.yborder > bg_yborder_max)bg.yborder = bg_yborder_max;
    dual_graph_borders((void*)&bg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  default:
  goto await_release;
  }
if(leftpressed == BUTTON_RELEASED)goto finish;
return;
await_release:;
if(leftpressed != BUTTON_RELEASED) return;
switch (mouse_active_flag-1)
  {
  case BG_YSCALE_EXPAND:
  bg.yrange/=1.5;
  break;
      
  case BG_YSCALE_CONTRACT:
  bg.yrange*=1.5;
  break;
      
  case BG_YZERO_DECREASE:
  bg.yzero/=1.5;
  break;
            
  case BG_YZERO_INCREASE:
  bg.yzero*=1.5;
  break;

  case BG_RESOLUTION_DECREASE:
  bg.bandwidth/=2;
  reinit_baseb=TRUE;
  break;

  case BG_RESOLUTION_INCREASE:
  bg.bandwidth*=2;
  reinit_baseb=TRUE;
  break;

  case BG_OSCILLOSCOPE: 
  if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
    {
    bg.oscill_on^=1;
    }
  break;

  case BG_OSC_INCREASE: 
  if(bg.oscill_on != 0)
    {
    bg.oscill_gain*=5;
    }
  break;
    
  case BG_OSC_DECREASE:
  if(bg.oscill_on != 0)
    {
    bg.oscill_gain*=0.25;
    }
  break;

  case BG_PIX_PER_PNT_INC:
  bg.pixels_per_point++;
  if(bg.pixels_per_point > 16)bg.pixels_per_point=16;
  break;    

  case BG_PIX_PER_PNT_DEC:
  bg.pixels_per_point--;
  if(bg.pixels_per_point < 1)bg.pixels_per_point=1;
  break;    

  case BG_TOGGLE_EXPANDER:
  bg_expand++;
  if(bg_expand > 2)bg_expand=0;
  break;

  case  BG_TOGGLE_COHERENT:
  if(fft1_correlation_flag != 2)
    {
    if(bg_twopol == 0 && bg_delay == 0)
      {
      new_bg_coherent=bg_coherent+1;
      }
    }  
  break;

  case  BG_TOGGLE_PHASING:
  if(fft1_correlation_flag != 2)
    {
    if(bg_coherent == 0)
      {
      new_bg_twopol=0;
      new_bg_delay=(bg_delay^1)&1;
      if(new_bg_delay == 0)new_daout_channels=1+((genparm[OUTPUT_MODE]>>1)&1);
      }
    }
  break;

  case BG_TOGGLE_TWOPOL:
  if(fft1_correlation_flag != 2)
    {
    if(bg_coherent == 0)
      {
      new_bg_delay=0;
      new_bg_twopol=(bg_twopol^1)&1;
      if(new_bg_twopol == 0)new_daout_channels=1+((genparm[OUTPUT_MODE]>>1)&1);
      }
    }  
  break;

  case  BG_TOGGLE_CHANNELS:
  new_daout_channels=rx_daout_channels+1; 
  if(new_daout_channels>ui.rx_max_da_channels)
                                     new_daout_channels=ui.rx_min_da_channels;
  break;

  case BG_TOGGLE_CH2_PHASE:
  if(  baseb_channels == 1  && 
       rx_daout_channels == 2  &&
       bg_delay == 0 &&
       bg_twopol == 0)
    {
    new_bg_ch2_phase=bg.ch2_phase+1;
    }
  break;

  case  BG_TOGGLE_BYTES:
  new_daout_bytes=rx_daout_bytes+1;
  if(new_daout_bytes>ui.rx_max_da_bytes)new_daout_bytes=ui.rx_min_da_bytes;
  break;

  case BG_TOGGLE_AGC:
  if(genparm[CW_DECODE_ENABLE] == 0)
    {
    new_bg_agc_flag=bg.agc_flag+1;
    }
  break;
  
  case BG_SEL_COHFAC:
  mouse_active_flag=1;
  numinput_xpix=bgbutt[BG_SEL_COHFAC].x1+7*text_width/2-1;
  numinput_ypix=bgbutt[BG_SEL_COHFAC].y1+2;
  numinput_chars=4;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=new_bg_cohfac;
  return;

  case BG_SEL_DELPNTS:
  mouse_active_flag=1;
  numinput_xpix=bgbutt[BG_SEL_DELPNTS].x1+3*text_width/2-1;
  numinput_ypix=bgbutt[BG_SEL_DELPNTS].y1+2;
  numinput_chars=2;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=new_bg_delpnts;
  return;

  case BG_SEL_FFT3AVGNUM:
  mouse_active_flag=1;
  numinput_xpix=bgbutt[BG_SEL_FFT3AVGNUM].x1+text_width/2-1;
  numinput_ypix=bgbutt[BG_SEL_FFT3AVGNUM].y1+2;
  numinput_chars=4;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=change_fft3_avgnum;
  return;

  case BG_WATERF_AVGNUM:
  mouse_active_flag=1;
  numinput_xpix=bgbutt[BG_WATERF_AVGNUM].x1+text_width/2-1;
  numinput_ypix=bgbutt[BG_WATERF_AVGNUM].y1+2;
  numinput_chars=4;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=change_bg_waterf_avgnum;
  return;

  case BG_SEL_AGC_ATTACK:
  mouse_active_flag=1;
  numinput_xpix=bgbutt[BG_SEL_AGC_ATTACK].x1+3*text_width/2-1;
  numinput_ypix=bgbutt[BG_SEL_AGC_ATTACK].y1+2;
  numinput_chars=1;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=new_bg_agc_attack;
  return;

  case BG_SEL_AGC_RELEASE:
  mouse_active_flag=1;
  numinput_xpix=bgbutt[BG_SEL_AGC_RELEASE].x1+3*text_width/2-1;
  numinput_ypix=bgbutt[BG_SEL_AGC_RELEASE].y1+2;
  numinput_chars=1;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=new_bg_agc_release;
  return;

  case BG_SEL_AGC_HANG:
  mouse_active_flag=1;
  numinput_xpix=bgbutt[BG_SEL_AGC_HANG].x1+3*text_width/2-1;
  numinput_ypix=bgbutt[BG_SEL_AGC_HANG].y1+2;
  numinput_chars=1;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=new_bg_agc_hang;
  return;



  case BG_WATERF_ZERO:
  mouse_active_flag=1;
  numinput_xpix=bgbutt[BG_WATERF_ZERO].x1+text_width/2-1;
  numinput_ypix=bgbutt[BG_WATERF_ZERO].y1+2;
  numinput_chars=5;    
  erase_numinput_txt();
  numinput_flag=FIXED_FLOAT_PARM;
  par_from_keyboard_routine=new_bg_waterfall_zero;
  return;
 
  case BG_WATERF_GAIN:
  mouse_active_flag=1;
  numinput_xpix=bgbutt[BG_WATERF_GAIN].x1+text_width/2-1;
  numinput_ypix=bgbutt[BG_WATERF_GAIN].y1+2;
  numinput_chars=4;    
  erase_numinput_txt();
  numinput_flag=FIXED_FLOAT_PARM;
  par_from_keyboard_routine=new_bg_waterfall_gain;
  return;

  case BG_HORIZ_ARROW_MODE:
  bg.horiz_arrow_mode++;
  if(bg.horiz_arrow_mode > 2)bg.horiz_arrow_mode=0;
  break;

  case BG_MIXER_MODE:
  if(fft1_correlation_flag == 0)
    { 
    bg.mixer_mode++;
    if(bg.mixer_mode > 2)bg.mixer_mode=1;
    }
  break;

  case BG_FILTER_SHIFT:
  if(rx_mode == MODE_AM && bg.mixer_mode == 1)
    {
    mouse_active_flag=1;
    numinput_xpix=bgbutt[BG_FILTER_SHIFT].x1+5*text_width/2;
    numinput_ypix=bgbutt[BG_FILTER_SHIFT].y1+2;
    numinput_chars=4;    
    erase_numinput_txt();
    numinput_flag=FIXED_INT_PARM;
    par_from_keyboard_routine=new_bg_filter_shift;
    }
  return;
  
  case BG_NOTCH_NO:
  if(bg.mixer_mode == 1)
    {
    mouse_active_flag=1;
    numinput_xpix=bgbutt[BG_NOTCH_NO].x1+2;
    numinput_ypix=bgbutt[BG_NOTCH_NO].y1+2;
    numinput_chars=1;    
    erase_numinput_txt();
    numinput_flag=FIXED_INT_PARM;
    par_from_keyboard_routine=new_bg_no_of_notches;
    return;
    }
  break;
        
  case BG_NOTCH_WIDTH:
  if(bg.mixer_mode == 1 && bg_no_of_notches > 0)
    {
    mouse_active_flag=1;
    numinput_xpix=bgbutt[BG_NOTCH_WIDTH].x1+3*text_width/2;
    numinput_ypix=bgbutt[BG_NOTCH_WIDTH].y1+2;
    numinput_chars=3;    
    erase_numinput_txt();
    numinput_flag=FIXED_INT_PARM;
    par_from_keyboard_routine=new_bg_notch_width;
    return;
    }
  break;  
    
  case BG_NOTCH_POS:
  if(bg.mixer_mode == 1 && bg_no_of_notches > 0)
    {
    mouse_active_flag=1;
    numinput_xpix=bgbutt[BG_NOTCH_POS].x1+7*text_width/2;
    numinput_ypix=bgbutt[BG_NOTCH_POS].y1+2;
    numinput_chars=4;    
    erase_numinput_txt();
    numinput_flag=FIXED_INT_PARM;
    par_from_keyboard_routine=new_bg_notch_pos;
    return;
    }
  break;
  
  case BG_TOGGLE_FM_MODE:
  new_bg_fm_mode=bg.fm_mode+1;
  break;

  case BG_TOGGLE_FM_SUBTRACT:
  new_bg_fm_subtract=bg.fm_subtract+1;
  break;

  case BG_SEL_FM_AUDIO_BW:
  mouse_active_flag=1;
  numinput_xpix=bgbutt[BG_SEL_FM_AUDIO_BW].x1+5*text_width/2-1;
  numinput_ypix=bgbutt[BG_SEL_FM_AUDIO_BW].y1+2;
  numinput_chars=3;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=new_bg_fm_audio_bw;
  return;
  
  case BG_SQUELCH_LEVEL:
  mouse_active_flag=1;
  numinput_xpix=bgbutt[BG_SQUELCH_LEVEL].x1+text_width/2-1;
  numinput_ypix=bgbutt[BG_SQUELCH_LEVEL].y1+2;
  numinput_chars=2;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=new_bg_squelch_level;
  return;
  
  case BG_SQUELCH_TIME:
  mouse_active_flag=1;
  numinput_xpix=bgbutt[BG_SQUELCH_TIME].x1+text_width/2-1;
  numinput_ypix=bgbutt[BG_SQUELCH_TIME].y1+2;
  numinput_chars=1;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=new_bg_squelch_time;
  return;
  
  case BG_SQUELCH_POINT:
  mouse_active_flag=1;
  numinput_xpix=bgbutt[BG_SQUELCH_POINT].x1+text_width/2-1;
  numinput_ypix=bgbutt[BG_SQUELCH_POINT].y1+2;
  numinput_chars=2; 
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=new_bg_squelch_point;
  return;
  
  }
finish:;
leftpressed=BUTTON_IDLE;  
mouse_active_flag=0;
if(new_baseb_flag == -1)
  {
  init_baseband_sizes();
  make_baseband_graph(TRUE);
  }
baseb_reset_counter++;
}

void clear_bfo(void)
{
int i;
if(bfo_xpixel > 0)
  {
  for(i=bg_y1; i<=bg_y0; i++)lir_setpixel(bfo100_xpixel, i,bg_background[i]);
  for(i=bg_y2; i<bg_y1; i++)lir_setpixel(bfo10_xpixel, i,bg_background[i]);
  for(i=bg_y3; i<bg_y2; i++)lir_setpixel(bfo_xpixel, i,bg_background[i]);
  }
}



void baseb_par_control(void)
{
int old;
float t1;
unconditional_hide_mouse();
switch (bfo_flag)
  {
  case BG_BFO:  
  t1=((float)(mouse_x-bg_first_xpixel)/bg.pixels_per_point
                               -fft3_size/2+bg_first_xpoint)*bg_hz_per_pixel;
  if(bg.bfo_freq != t1)
    {
    clear_bfo();
    bg.bfo_freq=t1;
    make_bfo();
    }
  break;

  case BG_BFO10:  
  t1=10*((float)(mouse_x-bg_first_xpixel)/bg.pixels_per_point
                               -fft3_size/2+bg_first_xpoint)*bg_hz_per_pixel;
  if(bg.bfo_freq != t1)
    {
    clear_bfo();
    bg.bfo_freq=t1;
    make_bfo();
    }
  break;

  case BG_BFO100:  
  t1=100*((float)(mouse_x-bg_first_xpixel)/bg.pixels_per_point
                               -fft3_size/2+bg_first_xpoint)*bg_hz_per_pixel;
  if(bg.bfo_freq != t1)
    {
    clear_bfo();
    bg.bfo_freq=t1;
    make_bfo();
    }
  break;

  case BG_FLAT:
  if(mouse_x != flat_xpixel)
    {
    bg_flatpoints=filcur_points();
    if(bg_flatpoints < 1)bg_flatpoints=1;
    bg.filter_flat=bg_hz_per_pixel*bg_flatpoints;
    pause_thread(THREAD_SCREEN);
    make_bg_filter(); 
    if(kill_all_flag)return;  
    resume_thread(THREAD_SCREEN);
    mg_clear_flag=TRUE;
    }
  break;
  
  case BG_CURV:
  if(mouse_x != curv_xpixel)
    {
    bg_curvpoints=filcur_points();
    if(bg_curvpoints < 0)bg_curvpoints=0;
    bg.filter_curv=bg_hz_per_pixel*10*bg_curvpoints;
    pause_thread(THREAD_SCREEN);
    make_bg_filter(); 
    if(kill_all_flag)return;  
    resume_thread(THREAD_SCREEN);
    mg_clear_flag=TRUE;
    }
  break;
    
  case BG_VOLUME:
  if(daout_gain_y != mouse_y)
    {
    old=daout_gain_y;
    daout_gain_y=mouse_y;
    make_daout_gain();
    update_bar(bg_vol_x1,bg_vol_x2,bg_y0,daout_gain_y,old,
                                                 BG_GAIN_COLOR,bg_volbuf);
    }
  break;
  }
if(kill_all_flag)return;  
lir_sched_yield();
if(leftpressed == BUTTON_RELEASED)
  {
  leftpressed=BUTTON_IDLE;
  make_modepar_file(GRAPHTYPE_BG);
  mouse_active_flag=0;
  baseb_control_flag=0;
  if(rx_mode == MODE_FM && (bfo_flag == BG_FLAT || bfo_flag == BG_CURV))
    {
    init_baseband_sizes();
    make_baseband_graph(TRUE);
    }
  }
}

void mouse_on_baseband_graph(void)
{
int event_no;
// First find out if we are on a button or border line.
for(event_no=0; event_no<MAX_BGBUTT; event_no++)
  {
  if( bgbutt[event_no].x1 <= mouse_x && 
      bgbutt[event_no].x2 >= mouse_x &&      
      bgbutt[event_no].y1 <= mouse_y && 
      bgbutt[event_no].y2 >= mouse_y) 
    {
    mouse_active_flag=1+event_no;
    current_mouse_activity=mouse_continue_baseband_graph;
    return;
    }
  }
// Not button or border.
// Check if we are in the waterfall and if so, set a new frequency
if(mouse_y < bg.yborder)
  {
  baseb_control_flag=1;
  current_mouse_activity=mouse_new_frequency;  
  mouse_active_flag=1;
  return;
  }
// User wants to change filter, bfo freq or gain.
// We use the upper part to change the flat region and
// the lower part to change steepness.
bfo_flag=0;
if(mouse_y < bg_y0 && mouse_y > bg_ymax)
  {
  if(mouse_x >= bg_first_xpixel)
    {
    if(mouse_y > bg_ymax && mouse_y <= bg_y4 && abs(mouse_x-flat_xpixel) < 5)
      {
      bfo_flag=BG_FLAT;
      }
    if(mouse_y > bg_y4 && mouse_y <= bg_y3 && abs(mouse_x-curv_xpixel) < 5)
      {
      bfo_flag=BG_CURV;
      }
    if(use_bfo != 0)
      {
      if(mouse_y > bg_y3 && mouse_y <= bg_y2 && abs(mouse_x-bfo_xpixel) < 5)
        {
        bfo_flag=BG_BFO;
        }
      if(mouse_y > bg_y2 && mouse_y <= bg_y1 && abs(mouse_x-bfo10_xpixel) < 5)
        {
        bfo_flag=BG_BFO10;
        }
      if(mouse_y > bg_y1 && abs(mouse_x-bfo100_xpixel) < 5)
        {
        bfo_flag=BG_BFO100;
        }
      } 
    }
  else
    {
    bfo_flag=BG_VOLUME;
    }
  }
if(bfo_flag != 0)
  {
  baseb_control_flag=1;
  current_mouse_activity=baseb_par_control;
  }  
else
  {
  current_mouse_activity=mouse_nothing;  
  }
mouse_active_flag=1;
}

void fft3_size_error(char *txt)
{  
int i;
settextcolor(15);
i=bg.yborder;
while(i<bg.ybottom-text_height)
  {
  lir_pixwrite(bg.xleft+5*text_width,i,"LIMIT");
  lir_pixwrite(bg.xleft+12*text_width,i,txt);
  i+=text_height;
  }
settextcolor(7);
}

void init_baseband_sizes(void)
{
int bcha, coh, twop, dela;
halt_rx_output();
if(use_bfo == 0)
  {
// Make sure delay is zero in AM and FM.
// Routines probably useless and removed in Linrad-03.09
  new_bg_delay=0;
  }
// Set mix2.size=-1 to force a call to init_basebmem() from
// make_bg_filter whenever a parameter that affects memory
// allocations has been changed.
// ----------------------------------------------------
if(new_bg_fm_mode > MAX_FM_FFTN-2)new_bg_fm_mode=0;
if(bg.fm_mode != new_bg_fm_mode)mix2.size=-1;
bg.fm_mode=new_bg_fm_mode;
// ----------------------------------------------------
if(new_bg_fm_subtract == 1 && (ui.network_flag & NET_RXOUT_BASEBRAW) == 0)
  {
  new_bg_fm_subtract=2;
  }
if(new_bg_fm_subtract>2)new_bg_fm_subtract=0;
if(bg.fm_subtract != new_bg_fm_subtract)mix2.size=-1;
bg.fm_subtract=new_bg_fm_subtract;
// ----------------------------------------------------
if(genparm[CW_DECODE_ENABLE] != 0)new_bg_agc_flag=0;
if(new_bg_agc_flag > 2)new_bg_agc_flag=0;
if(new_bg_coherent > 3)new_bg_coherent=0;
if(new_bg_twopol != 0)
  {
  if(new_bg_agc_flag == 1 )new_bg_agc_flag=2;
  }
else
  {  
  if( (rx_mode != MODE_AM || new_bg_coherent == 0) && new_bg_agc_flag == 2)
    {
    if(bg.agc_flag==2)
      {
      new_bg_agc_flag=1;
      }
    else
      {
      new_bg_agc_flag=0;
      }
    }
  }       
if(bg.agc_flag != new_bg_agc_flag)mix2.size=-1;
bg.agc_flag=new_bg_agc_flag;
// ----------------------------------------------------
if(new_daout_bytes>ui.rx_max_da_bytes ||
   new_daout_bytes<ui.rx_min_da_bytes)
  {
  new_daout_bytes=ui.rx_min_da_bytes;
  }
if(rx_daout_bytes!=new_daout_bytes)mix2.size=-1;
rx_daout_bytes=new_daout_bytes;
// ----------------------------------------------------
bcha=baseb_channels;
coh=bg_coherent;
twop=bg_twopol;
dela=bg_delay;
baseb_channels=1;
bg_coherent=new_bg_coherent;
bg_twopol=new_bg_twopol;
if(rx_mode == MODE_FM)
  {
  bg_delay=0;
  }
else
  {  
  bg_delay=new_bg_delay;
  }
if(bg_coherent > 0 && bg_coherent != 3)
  {
  if(ui.rx_max_da_channels == 1)
    {
    bg_coherent=3;
    } 
  else
    {
    baseb_channels=2;
    new_daout_channels=2;
    }    
  }
if(bg_coherent==3)baseb_channels=1;
if(bg_delay != 0)
  {
  if(ui.rx_max_da_channels == 1)
    {
    bg_delay=0;
    } 
  else
    {
    baseb_channels=2;
    new_daout_channels=2;
    }    
  }
if(bg_twopol != 0)
  {
  if(ui.rx_max_da_channels == 1)
    {
    bg_twopol=0;
    } 
  else
    {
    baseb_channels=2;
    new_daout_channels=2;
    }    
  }
// ----------------------------------------------------  
if(new_daout_channels>ui.rx_max_da_channels ||
   new_daout_channels<ui.rx_min_da_channels)
  {
  new_daout_channels=ui.rx_min_da_channels;
  }
if(new_daout_channels < baseb_channels)new_daout_channels=baseb_channels;
if(new_daout_channels > ui.rx_max_da_channels)
                           new_daout_channels=ui.rx_max_da_channels;
if(new_daout_channels < ui.rx_min_da_channels)
                           new_daout_channels=ui.rx_min_da_channels;
if(new_daout_channels!=rx_daout_channels)mix2.size=-1;
rx_daout_channels=new_daout_channels;
// ----------------------------------------------------
if(baseb_channels > rx_daout_channels)baseb_channels=rx_daout_channels;
if( bcha != baseb_channels ||
    coh != bg_coherent ||
    twop != bg_twopol ||
    dela != bg_delay)mix2.size=-1;
bg.ch2_phase=new_bg_ch2_phase;
if(bg.ch2_phase >= MAX_CH2_PHASES)bg.ch2_phase=0;
if(rx_mode != MODE_SSB || bg_coherent==3 )
  {
  if(bg.ch2_phase == MAX_CH2_PHASES-1)bg.ch2_phase=0;
  }
if(  baseb_channels != 1 || 
     rx_daout_channels != 2 ||
     bg_delay != 0 ||
     bg_twopol != 0)bg.ch2_phase=0;
da_ch2_sign=ch2_signs[bg.ch2_phase];
}

void show_bg_maxamp(void)
{
unsigned char color;
float t1;
t1=bg_maxamp/bg_amplimit;
t1*=bg_y0-bg_ymax-2;
hide_mouse(bg_amp_indicator_x,bg_amp_indicator_x+text_width/2,
                                   bg_amp_indicator_y,bg_amp_indicator_y+3);
lir_fillbox(bg_amp_indicator_x,bg_amp_indicator_y,text_width/2,3,0);
bg_amp_indicator_y=bg_y0-t1-1;
if(t1<1)
  {
  color=10;
  }
else
  {
  if(bg_amp_indicator_y<=bg_ymax+3)
    {
    color=12;
    }
  else
    {
    color=7;
    }
  }
hide_mouse(bg_amp_indicator_x,bg_amp_indicator_x+text_width/2,
                                   bg_amp_indicator_y,bg_amp_indicator_y+3);
lir_fillbox(bg_amp_indicator_x,bg_amp_indicator_y,text_width/2,3,color);
bg_maxamp=0;
}

void make_baseband_graph(int clear_old)
{
char *stmp;
char s[80];
int x, y;
int volbuf_bytes;
int i,j,k,sizold,ypixels;
int ix1,ix2,iy1,iy2,ib,ic;
double db_scalestep;
float t1, t2, t3, t4, x1, x2;
double dt1, dt2;
float scale_value, scale_y;
pause_thread(THREAD_SCREEN);
if(clear_old)
  {
  hide_mouse(bg_old_x1,bg_old_x2,bg_old_y1,bg_old_y2);
  lir_fillbox(bg_old_x1,bg_old_y1,bg_old_x2-bg_old_x1+1,
                                                    bg_old_y2-bg_old_y1+1,0);
  }
clear_agc();
current_graph_minh=BG_MINYCHAR*text_height;
current_graph_minw=BG_MIN_WIDTH;
check_graph_placement((void*)(&bg));
clear_button(bgbutt, MAX_BGBUTT);
hide_mouse(bg.xleft,bg.xright,bg.ytop,bg.ybottom);  
bg_yborder_min=bg.ytop+3*text_height/2+3;
bg_yborder_max=bg.ybottom-6*text_height;
if(bg.yborder < bg_yborder_min || bg.yborder > bg_yborder_max)
  {
  bg.yborder=(bg_yborder_max+bg_yborder_min) >> 1;
  }
bg_ymax=bg.yborder+text_height+4; 
bg_y0=bg.ybottom-3*text_height;
bg_y1=(4*bg_y0+bg_ymax)/5;
bg_y2=(3*bg_y0+2*bg_ymax)/5;
bg_y3=(2*bg_y0+3*bg_ymax)/5;
bg_y4=(bg_y0+4*bg_ymax)/5;
bg_avg_counter=0;
scro[baseband_graph_scro].no=BASEBAND_GRAPH;
scro[baseband_graph_scro].x1=bg.xleft;
scro[baseband_graph_scro].x2=bg.xright;
scro[baseband_graph_scro].y1=bg.ytop;
scro[baseband_graph_scro].y2=bg.ybottom;
bgbutt[BG_LEFT].x1=bg.xleft;
bgbutt[BG_LEFT].x2=bg.xleft+2;
bgbutt[BG_LEFT].y1=bg.ytop;
bgbutt[BG_LEFT].y2=bg.ybottom;
bgbutt[BG_RIGHT].x1=bg.xright-2;
bgbutt[BG_RIGHT].x2=bg.xright;
bgbutt[BG_RIGHT].y1=bg.ytop;
bgbutt[BG_RIGHT].y2=bg.ybottom;
bgbutt[BG_TOP].x1=bg.xleft;
bgbutt[BG_TOP].x2=bg.xright;
bgbutt[BG_TOP].y1=bg.ytop;
bgbutt[BG_TOP].y2=bg.ytop+2;
bgbutt[BG_BOTTOM].x1=bg.xleft;
bgbutt[BG_BOTTOM].x2=bg.xright;
bgbutt[BG_BOTTOM].y1=bg.ybottom-2;
bgbutt[BG_BOTTOM].y2=bg.ybottom;
bgbutt[BG_YBORDER].x1=bg.xleft;
bgbutt[BG_YBORDER].x2=bg.xright;
bgbutt[BG_YBORDER].y1=bg.yborder-1;
bgbutt[BG_YBORDER].y2=bg.yborder+1;
// Draw the border lines
dual_graph_borders((void*)&bg,7);
// Set variables that depend on output format.
if(rx_daout_bytes == 1)bg_amplimit=126; else bg_amplimit=32000;
// The expander (if enabled) expands the amplitude with an
// exponential function y = A* (exp(B*x)-1) 
// 0 <= x <= bg_amplimit
// y is always 0 for x=0.
// Make y=bg_amplimit/2 for x=bg_amplimit
bg_expand_b=genparm[AMPLITUDE_EXPAND_EXPONENT]/bg_amplimit;
bg_expand_a=bg_amplimit/(exp(bg_expand_b*bg_amplimit)-1);
bg_maxamp=0;
bg_amp_indicator_y=bg_y0-3;
// Set up all the variables that depend on the window parameters.
bg_first_xpixel=bg.xleft+4*text_width;
bg_last_xpixel=bg.xright-2*text_width;
bg_xpoints=2+(bg_last_xpixel-bg_first_xpixel)/bg.pixels_per_point;
bg_xpixels=bg_last_xpixel-bg_first_xpixel+1;
bg_vol_x1=bg.xleft+text_width;
bg_vol_x2=bg_vol_x1+5*text_width/2;
volbuf_bytes=(bg_y0-bg_ymax+2)*(bg_vol_x2-bg_vol_x1+2)*sizeof(char);
fft3_size=2*timf3_sampling_speed/bg.bandwidth;
if(fft3_size < 1)fft3_size=1;
i=fft3_size;
make_power_of_two(&fft3_size);
if(fft3_size > 1.5*i)fft3_size>>=1;
while(fft3_size < bg_xpoints)fft3_size<<=1;
if(fft3_size > 0x10000)
  {
  fft3_size=0x10000;
  fft3_size_error("Max N = 16");
  }
sizold=fft3_size;  
while(2*fft3_size*ui.rx_rf_channels+timf3_block > 0.8*timf3_size)fft3_size/=2;
if(sizold != fft3_size)
  {
  sizold=fft3_size;  
  if(genparm[SECOND_FFT_ENABLE] == 0)
    {
    fft3_size_error("fft1 storage time");
    }
  else
    {  
    fft3_size_error("fft2 storage time");
    }
  }
while(2*fft3_size/timf3_sampling_speed > 
                               genparm[BASEBAND_STORAGE_TIME])fft3_size/=2;
if(sizold != fft3_size)
  {
  fft3_size_error("baseband storage time");
  }
// fft3 uses sin squared window with interleave factor=2
new_fft3:;
while(fft3_size < bg_xpoints)
  {
  bg_xpoints/=2;
  bg.pixels_per_point*=2;
  }
fft3_n=0;
i=fft3_size;
while(i != 1)
  {
  i>>=1;
  fft3_n++;
  }
if(fft3_size > 8192)
  {
  yieldflag_ndsp_fft3=TRUE;
  if(ui.max_blocked_cpus > 5)yieldflag_ndsp_fft3=FALSE;
  }
else
  {
  yieldflag_ndsp_fft3=FALSE;
  }
bg.bandwidth=1.9*timf3_sampling_speed/fft3_size;
bg_first_xpoint=(fft3_size-bg_xpoints)/2;
bg_hz_per_pixel=timf3_sampling_speed/fft3_size;  
bg_flatpoints=bg.filter_flat/bg_hz_per_pixel;
if(bg_flatpoints < 1)bg_flatpoints=1;
bg_curvpoints=0.1*bg.filter_curv/bg_hz_per_pixel;
fft3_block=fft3_size*2*ui.rx_rf_channels*MAX_MIX1;
fft3_blocktime=0.5*fft3_size/timf3_sampling_speed;
chk_bg_avgnum();
// find out how much memory we need for the fft3 buffer.
// First of all we need to hold the transforms that we use for averaging.
i=bg.fft_avgnum+2;
// And make sure it can hold two blocks of data from
// the source time function.
if(genparm[SECOND_FFT_ENABLE]!=0)
  {
  k=fft2_new_points;
  }
else
  {
  k=fft1_new_points;
  }
t1=2*k/timf1_sampling_speed;
if(t1<2)t1=2;
// make buffer big enough for t1 seconds of data
if(i < t1*bg_hz_per_pixel*2) i=t1*bg_hz_per_pixel*2; 
if(i<4)i=4;
make_power_of_two(&i);
t1=fft3_block*i;
if(fft1_correlation_flag == 0)
  {
  k=sizeof(float);
  }
else
  {
  k=sizeof(double);
  }
if(t1*k > (float)(0x40000000))
  {
  fft3_size/=2;
  fft3_size_error("RAM memory");
  if(fft3_size >= bg_xpoints)
    {
    goto new_fft3;
    }
  bg.fft_avgnum/=2;
  goto new_fft3;
  }  
fft3_totsiz=fft3_block*i;
fft3_mask=fft3_totsiz-1;
fft3_show_size=bg_xpoints*ui.rx_rf_channels*bg.fft_avgnum;
bg_waterf_lines=bg.yborder-bg.ytop-text_height-YWF;
bg_waterf_y1=bg.ytop+text_height+YWF+1;
bg_waterf_y2=bg_waterf_y1+2+bg_waterf_lines/20;
if(bg_waterf_y2 > bg_waterf_y1+bg_waterf_lines-1)
                           bg_waterf_y2=bg_waterf_y1+bg_waterf_lines-1;
bg_waterf_y=bg_waterf_y2;
if(bg_waterf_y2 > bg.yborder-1)bg_waterf_y2=bg.yborder-1;
bg_waterf_yinc=bg_waterf_y2-bg_waterf_y1+1;
bg_waterf_size=bg_xpixels*bg_waterf_lines;
max_bg_waterf_times=2+bg_waterf_lines/(2*text_height);
bg_waterf_ptr=0;
local_bg_waterf_ptr=0;
sc[SC_BG_WATERF_INIT]++;
if((unsigned)fft3_size > mix1.size)
  {
  fftn_tmp_size=fft3_size;
  }
else  
  {
  fftn_tmp_size=mix1.size;
  }
// ***********************************************
if(fft3_handle != NULL)
  {
  fft3_handle=chk_free(fft3_handle);
  }
init_memalloc(fft3mem,MAX_FFT3_ARRAYS);
mem( 1,&fft3,fft3_totsiz*sizeof(float),0);
mem( 2,&fft3_window,fft3_size*sizeof(float),0);
if(fft1_correlation_flag != 0)
  {
  mem( 101,&d_fft3,fft3_totsiz*sizeof(double),0);
  mem( 102,&d_fft3_window,fft3_size*sizeof(double),0);
  mem( 103,&d_fft3_tmp,2*fft3_size*(ui.rx_rf_channels+2)*sizeof(double),0);
  mem( 104,&d_fft3_tab,fft3_size*sizeof(D_COSIN_TABLE)/2,0);
  mem( 106,&d_fft3_fqwin_inv,fft3_size*sizeof(double),0);
  mem( 107,&d_basebcarr_fir,fft3_size*sizeof(double),0);
  mem( 108,&d_fftn_tmp,4*fftn_tmp_size*sizeof(double),0);
  if(fft1_correlation_flag == 2)
    {
    mem( 109,&fft3_cleansum,4*bg_xpoints*sizeof(float),0);
    }
  }
mem( 3,&fft3_tmp,4*fft3_size*(ui.rx_rf_channels+2)*sizeof(float),0);
mem( 4,&fft3_tab,fft3_size*sizeof(COSIN_TABLE)/2,0);
mem( 5,&fft3_permute,fft3_size*sizeof(short int),0);
mem( 6,&fft3_fqwin_inv,fft3_size*sizeof(float),0);
mem( 7,&fft3_power,fft3_show_size*ui.rx_rf_channels*sizeof(float),0);
mem( 8,&fft3_slowsum,bg_xpoints*ui.rx_rf_channels*sizeof(float),0);
mem( 9,&fft3_spectrum,screen_width*ui.rx_rf_channels*sizeof(short int),0);
mem(10,&bg_background,screen_height*sizeof(char),0);
mem(11,&bg_filterfunc,fft3_size*sizeof(float),0);
mem(12,&bg_filterfunc_y,screen_width*sizeof(short int),0);
mem(13,&bg_volbuf,volbuf_bytes,0);
mem(15,&bg_carrfilter,fft3_size*sizeof(float),0);
mem(16,&bg_carrfilter_y,screen_width*sizeof(short int),0);
mem(17,&bg_waterf_sum,bg_xpoints*sizeof(float),0);
mem(18,&bg_waterf,5000+bg_waterf_size*sizeof(short int),0);
mem(19,&bg_waterf_times,max_bg_waterf_times*sizeof(WATERF_TIMES),0);
mem(20,&bg_binshape,fft3_size*sizeof(float),0);
mem(21,&bg_ytmp,fft3_size*sizeof(float),0);
mem(22,&basebraw_fir,fft3_size*sizeof(float),0);
mem(23,&basebcarr_fir,fft3_size*sizeof(float),0);
mem(24,&squelch_info,bg_xpoints*sizeof(float),0);
mem(25,&fftn_tmp,4*fftn_tmp_size*sizeof(float),0);
// **********************************************
fft3_totmem=memalloc(&fft3_handle,"fft3");
if(fft3_totmem == 0)
  {
  fft3_size/=2;
  fft3_size_error("RAM memory");
  if(fft3_size >= bg_xpoints)
    {
    goto new_fft3;
    }
  lirerr(1056);
  return;
  }
lir_sched_yield();
memset(fft3,0,MAX_MIX1*fft3_size*4*ui.rx_rf_channels*sizeof(float));
memset(fft3_slowsum,0,bg_xpoints*ui.rx_rf_channels*sizeof(float));
memset(fft3_power,0,fft3_show_size*ui.rx_rf_channels*sizeof(float));
memset(bg_waterf_sum,0,bg_xpoints*sizeof(float));
if(fft3_size > 16384)lir_sched_yield();
fft3_slowsum_cnt=0;
for(i=0; i<max_bg_waterf_times; i++)
  {
  bg_waterf_times[i].line=5;
  bg_waterf_times[i].text[0]=0;
  }  
// Show the fft size in the upper right corner.
sprintf(s,"%2d",fft3_n);
// Clear the waterfall memory area
for(i=0;i<bg_waterf_size;i++)bg_waterf[i]=0x8000;
// Make sure we know these pixels are not on screen.
for(i=0;i<screen_width; i++)
  {
  bg_filterfunc_y[i]=-1;
  bg_carrfilter_y[i]=-1;
  }
lir_pixwrite(bg.xright-2*text_width,bg.yborder+3*text_height/2,s);
fft3_pa=0;
fft3_px=0;
fft3_indicator_block=(fft3_mask+1)/INDICATOR_SIZE;
stmp=(void*)(fft3);
sprintf(stmp,"Reserved for blanker");
i=(bg.xright-bg.xleft)/text_width-12;
if(i<0)i=0;
stmp[i]=0;
lir_pixwrite(bg.xleft+6*text_width,bg.ybottom-text_height-2,stmp);
init_fft(1,fft3_n, fft3_size, fft3_tab, fft3_permute);
if(fft1_correlation_flag != 0)
  {
  make_d_sincos(1, fft3_size, d_fft3_tab);
  make_d_window(6,fft3_size, 4, d_fft3_fqwin_inv);
// Near the ends the inverse window has very large values.
// We do not want to multiply with something VERY large, so
// we apply a function that goes gently to zero outside the
// maximum permitted baseband bandwidth.
  k=(1-MAX_BASEBFILTER_WIDTH)*fft3_size/2;
  dt1=-4;
  dt2=6.0/k;
  k=fft3_size-1;
  for(i=0; i<fft3_size/2; i++)
    {
    d_fft3_fqwin_inv[i]*=0.5*((double)2-erfc(dt1));
    d_fft3_fqwin_inv[k]=d_fft3_fqwin_inv[i];
    dt1+=dt2;
    k--;
    }
  make_d_window(1,fft3_size, genparm[THIRD_FFT_SINPOW], d_fft3_window);
  if(fft1_correlation_flag == 2)
    {
    memset(fft3_cleansum,0,bg_xpoints*ui.rx_rf_channels*sizeof(float));
    }
  }
make_window(1,fft3_size, genparm[THIRD_FFT_SINPOW], fft3_window);
// Find the average filter shape of an FFT bin.
// It depends on what window we have selected.
// Store a sine-wave with frequency from -0.5 bin to +0.5 bin
// in several steps. Produce the power spectrum in each case
// and collect the average.
lir_sched_yield();
for(i=0; i<fft3_size; i++)
  {
  bg_binshape[i]=0;
  }
for(j=-3; j<=3; j++)
  {
  t1=0;
  t2=j*PI_L/(fft3_size*3);
  for(i=0; i<fft3_size; i++)
    {
    fft3_tmp[2*i  ]=cos(t1);
    fft3_tmp[2*i+1]=sin(t1);
    t1+=t2;
    }
  for( i=0; i<fft3_size/2; i++)
    {
    t1=fft3_tmp[2*i  ]*fft3_window[2*i];
    t2=fft3_tmp[2*i+1]*fft3_window[2*i];      
    t3=fft3_tmp[fft3_size+2*i  ]*fft3_window[2*i+1];
    t4=fft3_tmp[fft3_size+2*i+1]*fft3_window[2*i+1];   
    x1=t1-t3;
    fft3_tmp[2*i  ]=t1+t3;
    x2=t4-t2;
    fft3_tmp[2*i+1]=t2+t4;
    fft3_tmp[fft3_size+2*i   ]=fft3_tab[i].cos*x1+fft3_tab[i].sin*x2;
    fft3_tmp[fft3_size+2*i +1]=fft3_tab[i].sin*x1-fft3_tab[i].cos*x2;
    } 
#if IA64 == 0 && CPU == CPU_INTEL
  asmbulk_of_dif(fft3_size, fft3_n, fft3_tmp, fft3_tab, yieldflag_ndsp_fft3);
#else
  bulk_of_dif(fft3_size, fft3_n, fft3_tmp, fft3_tab, yieldflag_ndsp_fft3);
#endif
  for(i=0; i < fft3_size; i+=2)
    {
    ib=fft3_permute[i];                               
    ic=fft3_permute[i+1];                             
    bg_binshape[ib  ]+=pow(fft3_tmp[2*i  ]+fft3_tmp[2*i+2],2.0)+
                       pow(fft3_tmp[2*i+1]+fft3_tmp[2*i+3],2.0);                          
    bg_binshape[ic  ]+=pow(fft3_tmp[2*i  ]-fft3_tmp[2*i+2],2.0)+
                       pow(fft3_tmp[2*i+1]-fft3_tmp[2*i+3],2.0);
    }
  if(fft3_size > 8192)lir_sched_yield();
  }
// The bin shape has to be symmetric around the midpoint so we
// use the average from both sides. Store in the lower half
// while accumulating errors in the upper half.
i=fft3_size/2-1;
k=fft3_size/2+1;
while(i>0)
  {
  t1=bg_binshape[i]-bg_binshape[k];
  bg_binshape[i]=0.5*(bg_binshape[i]+bg_binshape[k]);
  bg_binshape[k]=fabs(t1);
  i--;
  k++;
  }
t1=0;
for(i=3*fft3_size/4; i<fft3_size; i++)
  {
  t1+=bg_binshape[i];
  }
t1/=fft3_size/8;  
t2=0.5*t1;
// t1 is now twice the average error.
// Subtract it from our bg_binshape values.
for(i=0; i<=fft3_size/2; i++)
  {
  bg_binshape[i]-=t1;
  if(bg_binshape[i]<t2)bg_binshape[i]=t2;
  }
i=fft3_size/2-1;
k=fft3_size/2+1;
while(i > 0)
  {
  bg_binshape[i]=sqrt(bg_binshape[i]/bg_binshape[fft3_size/2]);
  bg_binshape[k]=bg_binshape[i];
  i--;
  k++;
  }
t2=sqrt(t2/bg_binshape[fft3_size/2]);
bg_binshape[fft3_size/2]=1;  
binshape_points=0;
i=fft3_size/2-1;
// Get the -110 dB point
while(i>1 && bg_binshape[i-1] > 0.0000031)
  {
  i--;
  binshape_points++;
  }
while(i>0)
  {
  bg_binshape[i]=0;
  i--;
  }  
binshape_total=binshape_points;  
while(i>0 && bg_binshape[i] > 1.5*t2)
  {
  i--;
  binshape_total++;
  }
// Now we have the spectral shape of a single FFT bin in
// bg_binshape with the maximum at position fft3_size/2.
// **************************************************************
//  *************************************************************
// When the spectrum is picked from fft1 in the mix1 process,
// mix1_fqwin is applied to suppress spurs.
// as a consequence the noise floor will not be flat in the
// baseband graph.
// Get the inverse of mix1_fqwin in fft3_size points so we can compensate.
make_window(6,fft3_size, 4, fft3_fqwin_inv);
// Near the ends the inverse window has very large values.
// We do not want to multiply with something VERY large, so
// we apply a function that goes gently to zero outside the
// maximum permitted baseband bandwidth.
k=(1-MAX_BASEBFILTER_WIDTH)*fft3_size/2;
t1=-4;
t2=6.0/k;
k=fft3_size-1;
for(i=0; i<fft3_size/2; i++)
  {
  fft3_fqwin_inv[i]*=0.5*((double)2-erfc(t1));
  fft3_fqwin_inv[k]=fft3_fqwin_inv[i];
  t1+=t2;
  k--;
  }
// Init fft3_spectrum at the zero level. 
for(i=0; i<screen_width*ui.rx_rf_channels; i++)fft3_spectrum[i]=bg_y0;
bg_show_pa=0;
fft3_slowsum_recalc=0;
x=bg.xleft+text_width;
y=bg.ybottom-text_height/2-2;
make_button(x, y, bgbutt,BG_YSCALE_EXPAND,CHAR_ARROW_UP);
x+=2*text_width;
make_button(x, y, bgbutt,BG_YSCALE_CONTRACT,CHAR_ARROW_DOWN);
x+=2*text_width;
make_button(x, y, bgbutt, BG_HORIZ_ARROW_MODE, 
                                       arrow_mode_char[bg.horiz_arrow_mode]);
i=(bg.ybottom+bg.yborder)/2;
make_button(bg.xright-text_width,i-text_height/2-2,
                                         bgbutt,BG_YZERO_DECREASE,CHAR_ARROW_UP);
make_button(bg.xright-text_width,i+text_height/2+2,
                                         bgbutt,BG_YZERO_INCREASE,CHAR_ARROW_DOWN);
make_button(bg.xright-3*text_width,bg.ytop+text_height/2+3,
                                         bgbutt,BG_RESOLUTION_DECREASE,26);
make_button(bg.xright-text_width,bg.ytop+text_height/2+3,
                                         bgbutt,BG_RESOLUTION_INCREASE,27); 
if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
  {
  make_button(bg.xright-text_width,bg.ybottom-text_height/2-2,
                                         bgbutt,BG_OSCILLOSCOPE,'o'); 
  }
ix1=bg.xright-3*text_width+2;
if(bg.oscill_on != 0)
  {
  make_button(ix1, bg.ybottom-text_height/2-2, bgbutt,BG_OSC_INCREASE,'+'); 
  ix1-=2*text_width-1;
  make_button(ix1, bg.ybottom-text_height/2-2, bgbutt,BG_OSC_DECREASE,'-'); 
  ix1-=2*text_width-2;
  }
lir_sched_yield();
iy2=bg.ybottom-2;
iy1=iy2-text_height-1;
ix2=ix1-3+text_width;
ix1-=text_width;
bgbutt[BG_NOTCH_NO].x1=ix1;
bgbutt[BG_NOTCH_NO].x2=ix2;
bgbutt[BG_NOTCH_NO].y1=iy1;
bgbutt[BG_NOTCH_NO].y2=iy2;
ix2=ix1-3;
ix1-=7*text_width;
bgbutt[BG_NOTCH_WIDTH].x1=ix1;
bgbutt[BG_NOTCH_WIDTH].x2=ix2;
bgbutt[BG_NOTCH_WIDTH].y1=iy1;
bgbutt[BG_NOTCH_WIDTH].y2=iy2;
ix2=ix1-2;
ix1-=8*text_width;
bgbutt[BG_NOTCH_POS].x1=ix1;
bgbutt[BG_NOTCH_POS].x2=ix2;
bgbutt[BG_NOTCH_POS].y1=iy1;
bgbutt[BG_NOTCH_POS].y2=iy2;
make_button(bg.xleft+text_width,bg.ytop+text_height/2+3,
                                         bgbutt,BG_PIX_PER_PNT_DEC,26);
make_button(bg.xleft+3*text_width,bg.ytop+text_height/2+3,
                                         bgbutt,BG_PIX_PER_PNT_INC,27);
bgbutt[BG_WATERF_AVGNUM].x1=2+bg.xleft;
bgbutt[BG_WATERF_AVGNUM].x2=2+bg.xleft+9*text_width/2;
bgbutt[BG_WATERF_AVGNUM].y1=bg.yborder-text_height-2;
bgbutt[BG_WATERF_AVGNUM].y2=bg.yborder-2;
bgbutt[BG_WATERF_GAIN].x1=bg.xright-9*text_width/2-2;
bgbutt[BG_WATERF_GAIN].x2=bg.xright-2;
bgbutt[BG_WATERF_GAIN].y1=bg.yborder-2*text_height-4;
bgbutt[BG_WATERF_GAIN].y2=bg.yborder-text_height-4;
bgbutt[BG_WATERF_ZERO].x1=bg.xright-11*text_width/2-2;
bgbutt[BG_WATERF_ZERO].x2=bg.xright-2;
bgbutt[BG_WATERF_ZERO].y1=bg.yborder-text_height-2;
bgbutt[BG_WATERF_ZERO].y2=bg.yborder-2;
iy1=bg.yborder+2;
iy2=bg.yborder+text_height+2;
bgbutt[BG_SEL_FFT3AVGNUM].x1=bg.xleft+2;
bgbutt[BG_SEL_FFT3AVGNUM].x2=bg.xleft+2+9*text_width/2;
bgbutt[BG_SEL_FFT3AVGNUM].y1=iy1;
bgbutt[BG_SEL_FFT3AVGNUM].y2=iy2;
ix1=bg.xleft+4+9*text_width/2;
ix2=ix1+7*text_width/2;
if(rx_mode == MODE_FM)
  {
  bgbutt[BG_TOGGLE_FM_MODE].x1=ix1;
  bgbutt[BG_TOGGLE_FM_MODE].x2=ix2;
  bgbutt[BG_TOGGLE_FM_MODE].y1=iy1;
  bgbutt[BG_TOGGLE_FM_MODE].y2=iy2;
  ix1=ix2+2;
  ix2=ix1+3*text_width/2;
  bgbutt[BG_TOGGLE_FM_SUBTRACT].x1=ix1;
  bgbutt[BG_TOGGLE_FM_SUBTRACT].x2=ix2;
  bgbutt[BG_TOGGLE_FM_SUBTRACT].y1=iy1;
  bgbutt[BG_TOGGLE_FM_SUBTRACT].y2=iy2;
  ix1=ix2+2;
  ix2=ix1+11*text_width/2;
  bgbutt[BG_SEL_FM_AUDIO_BW].x1=ix1;
  bgbutt[BG_SEL_FM_AUDIO_BW].x2=ix2;
  bgbutt[BG_SEL_FM_AUDIO_BW].y1=iy1;
  bgbutt[BG_SEL_FM_AUDIO_BW].y2=iy2;
  freq_readout_x1=ix2;
  bg.agc_flag=0;
  }
else   
  {
  freq_readout_x1=ix2;
  if(fft1_correlation_flag != 2)
    {
    bgbutt[BG_TOGGLE_AGC].x1=ix1;
    bgbutt[BG_TOGGLE_AGC].x2=ix2;
    bgbutt[BG_TOGGLE_AGC].y1=iy1;
    bgbutt[BG_TOGGLE_AGC].y2=iy2;
    if(bg.agc_flag != 0)
      {  
      ix1=ix2+2;
      ix2=ix1+5*text_width/2;
      bgbutt[BG_SEL_AGC_ATTACK].x1=ix1;
      bgbutt[BG_SEL_AGC_ATTACK].x2=ix2;
      bgbutt[BG_SEL_AGC_ATTACK].y1=iy1;
      bgbutt[BG_SEL_AGC_ATTACK].y2=iy2;
      ix1=ix2+2;
      ix2=ix1+5*text_width/2;
      bgbutt[BG_SEL_AGC_RELEASE].x1=ix1;
      bgbutt[BG_SEL_AGC_RELEASE].x2=ix2;
      bgbutt[BG_SEL_AGC_RELEASE].y1=iy1;
      bgbutt[BG_SEL_AGC_RELEASE].y2=iy2;
      ix1=ix2+2;
      ix2=ix1+5*text_width/2;
      bgbutt[BG_SEL_AGC_HANG].x1=ix1;
      bgbutt[BG_SEL_AGC_HANG].x2=ix2;
      bgbutt[BG_SEL_AGC_HANG].y1=iy1;
      bgbutt[BG_SEL_AGC_HANG].y2=iy2;
      freq_readout_x1=ix2;
      }
    }
  }  
freq_readout_x1+=text_width;  
freq_readout_y1=iy1;
ix2=bg.xright-2;
ix1=ix2-3*text_width/2;
bgbutt[BG_MIXER_MODE].x1=ix1;
bgbutt[BG_MIXER_MODE].x2=ix2;
bgbutt[BG_MIXER_MODE].y1=iy1;
bgbutt[BG_MIXER_MODE].y2=iy2;
if(rx_mode == MODE_AM && bg.mixer_mode == 1)
  {
// ***********  make filter shift button ******************
  ix2=ix1-2;
  ix1-=8*text_width;
  bgbutt[BG_FILTER_SHIFT].x1=ix1;
  bgbutt[BG_FILTER_SHIFT].x2=ix2;
  bgbutt[BG_FILTER_SHIFT].y1=iy1;
  bgbutt[BG_FILTER_SHIFT].y2=iy2;
  }
// Make the squelch buttons. They will be placed on screen by make_bg_filter.
if(fft1_correlation_flag != 2)
  {
  ix2=ix1-2;
  ix1=ix2-3*text_width/2;
  bgbutt[BG_SQUELCH_TIME].x1=ix1;
  bgbutt[BG_SQUELCH_TIME].x2=ix2;
  bgbutt[BG_SQUELCH_TIME].y1=iy1;
  bgbutt[BG_SQUELCH_TIME].y2=iy2;
  ix2=ix1-2;
  ix1=ix2-5*text_width/2;
  bgbutt[BG_SQUELCH_POINT].x1=ix1;
  bgbutt[BG_SQUELCH_POINT].x2=ix2;
  bgbutt[BG_SQUELCH_POINT].y1=iy1;
  bgbutt[BG_SQUELCH_POINT].y2=iy2;
  ix2=ix1-2;
  ix1=ix2-5*text_width/2;
  bgbutt[BG_SQUELCH_LEVEL].x1=ix1;
  bgbutt[BG_SQUELCH_LEVEL].x2=ix2;
  bgbutt[BG_SQUELCH_LEVEL].y1=iy1;
  bgbutt[BG_SQUELCH_LEVEL].y2=iy2;
  squelch_turnon_time=current_time();
  }
squelch_on=-2;
// Write out the y scale for logarithmic spectrum graph.
// We want a point with amplitude 1<<(fft1_n/2) to be placed at the
// zero point of the dB scale. 
for(i=0; i<screen_height; i++)bg_background[i]=0;
ypixels=bg_y0-bg_ymax+1;
bg.db_per_pixel=20*log10(bg.yrange)/ypixels;
db_scalestep=1.3*bg.db_per_pixel*text_height;
adjust_scale(&db_scalestep);
if(db_scalestep < 1)
  {
  db_scalestep=1;
  bg.db_per_pixel=0.5/text_height;
  bg.yrange=pow(10.,ypixels*bg.db_per_pixel/20);
  }
t1=20*log10( bg.yzero);
i=(t1+0.5*db_scalestep)/db_scalestep;
scale_value=i*db_scalestep;
scale_y=bg_y0+(t1-scale_value)/bg.db_per_pixel;
while( scale_y > bg_ymax)
  {
  if(scale_y+text_height/2+1 < bg_y0)
    {
    i=(int)(scale_value);
    sprintf(s,"%3d",i);
    lir_pixwrite(bg.xleft+text_width/2,(int)scale_y-text_height/2+2,s);
    }
  if(scale_y+1 < bg_y0)
    {
    i=scale_y;
    bg_background[i]=BG_DBSCALE_COLOR;    
    if(i > bgbutt[BG_SQUELCH_LEVEL].y2)
      {
      j=bg_last_xpixel;
      }
    else
      {
      j= bgbutt[BG_SQUELCH_LEVEL].x1-1;
      }
    lir_hline(bg_first_xpixel,i,j,BG_DBSCALE_COLOR);
    if(kill_all_flag) return;
    }
  scale_y-=db_scalestep/bg.db_per_pixel;
  scale_value+=db_scalestep;
  }
freq_readout_x2=ix1-text_width;  
make_bg_yfac();
make_bg_filter();
if(kill_all_flag) return;
make_modepar_file(GRAPHTYPE_BG);
bg_flag=1;
daout_gain_y=-1;
make_daout_gainy();
// *************  Make button to select no of channels for mono modes
ix1=bg.xleft+text_width;
if(  baseb_channels == 1  && 
     rx_daout_channels == 2  &&
     bg_delay == 0 &&
     bg_twopol == 0)
  {
  make_button(ix1,bg.ybottom-2*text_height,
                 bgbutt,BG_TOGGLE_CH2_PHASE,ch2_phase_symbol[bg.ch2_phase]);
  }
ix1+=3*text_width/2+2;
make_button(ix1,bg.ybottom-2*text_height,
                       bgbutt,BG_TOGGLE_CHANNELS,'0'+rx_daout_channels);
// ************** Make button to select number of bit's
ix1+=4+text_width/2;
ix2=ix1+5*text_width/2;
iy1=bg.ybottom-5*text_height/2-1;
iy2=iy1+text_height+1;
bgbutt[BG_TOGGLE_BYTES].x1=ix1;
bgbutt[BG_TOGGLE_BYTES].x2=ix2;
bgbutt[BG_TOGGLE_BYTES].y1=iy1;
bgbutt[BG_TOGGLE_BYTES].y2=iy2;
// **************   Make the button for the amplitude expander
if(fft1_correlation_flag != 2)
  {
  if(use_bfo != 0)
    {
    ix1=ix2+2;
    ix2=ix1+7*text_width/2;
    bgbutt[BG_TOGGLE_EXPANDER].x1=ix1;
    bgbutt[BG_TOGGLE_EXPANDER].x2=ix2;
    bgbutt[BG_TOGGLE_EXPANDER].y1=iy1;
    bgbutt[BG_TOGGLE_EXPANDER].y2=iy2;
    }
// *********************     Make the button for coherent CW
  ix1=ix2+2;
  ix2=ix1+9*text_width/2;
  bgbutt[BG_TOGGLE_COHERENT].x1=ix1;
  bgbutt[BG_TOGGLE_COHERENT].x2=ix2;
  bgbutt[BG_TOGGLE_COHERENT].y1=iy1;
  bgbutt[BG_TOGGLE_COHERENT].y2=iy2;
// ***********  make coherent bandwidth factor button ******************
  }
ix1=ix2+2;
ix2=ix1+15*text_width/2;
bgbutt[BG_SEL_COHFAC].x1=ix1;
bgbutt[BG_SEL_COHFAC].x2=ix2;
bgbutt[BG_SEL_COHFAC].y1=iy1;
bgbutt[BG_SEL_COHFAC].y2=iy2;
// ****************************************************************
if(fft1_correlation_flag != 2)
  {
  if(use_bfo != 0)
    {
// Make the button for frequency depending phase shift between ears
// This is the same as a time delay for one channel!
    ix1=ix2+2;
    ix2=ix1+7*text_width/2;
    bgbutt[BG_TOGGLE_PHASING].x1=ix1;
    bgbutt[BG_TOGGLE_PHASING].x2=ix2;
    bgbutt[BG_TOGGLE_PHASING].y1=iy1;
    bgbutt[BG_TOGGLE_PHASING].y2=iy2;
// ******  delay no of points button *************
    ix1=ix2+2;
    ix2=ix1+7*text_width/2;
    bgbutt[BG_SEL_DELPNTS].x1=ix1;
    bgbutt[BG_SEL_DELPNTS].x2=ix2;
    bgbutt[BG_SEL_DELPNTS].y1=iy1;
    bgbutt[BG_SEL_DELPNTS].y2=iy2;
    }
// ****************************************************************
// Make the button for stereo reception of two channels
// if there are two channels.
  if(ui.rx_rf_channels == 2)
    {
    ix1=ix2+2;
    ix2=ix1+7*text_width/2;
    bgbutt[BG_TOGGLE_TWOPOL].x1=ix1;
    bgbutt[BG_TOGGLE_TWOPOL].x2=ix2;
    bgbutt[BG_TOGGLE_TWOPOL].y1=iy1;
    bgbutt[BG_TOGGLE_TWOPOL].y2=iy2;
    }
  }  
i=bg_twopol;                 //bit 7
i=(i<<1)+bg_delay;           //bit 6
i=(i<<2)+bg_coherent;        //bits 4 and 5
i=(i<<2)+bg_expand;          //bits 2 and 3
i=(i<<1)+rx_daout_channels-1;   //bit 1
i=(i<<1)+rx_daout_bytes-1;      //bit 0
sprintf(s,"%3d",i);
lir_pixwrite(bg.xright-7*text_width/2,iy1+2,s);
bg_amp_indicator_x=bg.xleft+2;
show_bg_maxamp();
check_bg_cohfac();
check_bg_fm_audio_bw();
make_agc_amplimit();
bg_waterf_block=(bg_waterf_y2-bg_waterf_y1+1)*bg_xpixels;
make_bg_waterf_cfac();
if(genparm[AFC_ENABLE]==0 || genparm[AFC_LOCK_RANGE] == 0)
  {
  sc[SC_SHOW_WHEEL]++;
  }
bg_old_y1=bg.ytop;
bg_old_y2=bg.ybottom;
bg_old_x1=bg.xleft;
bg_old_x2=bg.xright;
if(rx_mode == MODE_FM)
  {
// *********************** fm_audiofil *********************************          
  i=1000*fm_audiofil_size*bg.fm_audio_bw/baseband_sampling_speed;
  construct_fir(7,fm_audiofil_size, fm_audiofil_n, 
                   &fm_audiofil_points, i, fm_audiofil_fir, 0.00001);
  fm0_ph1=0;
  fm0_ph2=0;
  fm1_ph1=0;
  fm1_ph2=0;
  fm2_ph1=0;
  fm2_ph2=0;
  fm_n=bg.fm_mode+2;
  if(fm_n < 3)fm_n=0;
  if(fm_n > MAX_FM_FFTSIZE)fm_n=MAX_FM_FFTSIZE;
  i=fm_n;
  fm_size=1;
  while(i>0)
    {
    i--;
    fm_size<<=1;
    }
  init_fft(0,fm_n, fm_size, fm_tab, fm_permute);
  make_window(4,fm_size, 2, fm_win);
  }
if(genparm[BG_WATERF_BLANKED_PERCENT] == 0)
  {  
  bg_waterfall_blank_points=0;
  }
else
  {
  bg_waterfall_blank_points=1+(genparm[BG_WATERF_BLANKED_PERCENT]*bg_xpixels)/100;
  }  
resume_thread(THREAD_SCREEN);
if(mix1_selfreq[0]>0)sc[SC_FREQ_READOUT]++;
sc[SC_BG_FQ_SCALE]++;
sc[SC_BG_BUTTONS]++;
}

void init_baseband_graph(void)
{
int i,errcnt;
errcnt=0;
if (read_modepar_file(GRAPHTYPE_BG) == 0)
  {
bg_default:;  
// Make the default window for the baseband graph.
  bg.xleft=hg.xright+2;
  bg.xright=bg.xleft+screen_width/2;
  if(bg.xright > screen_width)
    {
    bg.xright=screen_width-1;
    bg.xleft=bg.xright-BG_MIN_WIDTH;
    }
  bg.ytop=wg.ybottom+2;
  bg.yborder=bg.ytop+screen_height/10;
  bg.ybottom=bg.yborder+screen_height/5;
  if(bg.ybottom>4*screen_height/5)
    {
    bg.ybottom=4*screen_height/5;
    bg.yborder=(2*bg.ybottom+bg.ytop)/3;
    }
  bg.bandwidth=timf3_sampling_speed/256;
  bg.yrange=4096;
  bg.yzero=1;
  switch (rx_mode)
    {
    case MODE_WCW:
    bg.filter_flat=14;
    bg.filter_curv=0;
    bg.output_gain=1;
    bg.agc_flag=0;
    bg.waterfall_avgnum=5;
    bg.bandwidth/=4;
    break;
    
    case MODE_NCW:
    bg.filter_flat=250;
    bg.filter_curv=50;
    bg.output_gain=100;
    bg.agc_flag=1;
    bg.waterfall_avgnum=50;
    break;
    
    case MODE_SSB:
    bg.filter_flat=1200;
    bg.filter_curv=0;
    bg.output_gain=5000;
    bg.agc_flag=1;
    bg.waterfall_avgnum=50;
    break;
    
    case MODE_AM:
    bg.filter_flat=3000;
    bg.filter_curv=250;
    bg.output_gain=5000;
    bg.agc_flag=1;
    bg.waterfall_avgnum=200;
    break;
    
    case MODE_FM:
    bg.filter_flat=5000;
    bg.filter_curv=250;
    bg.output_gain=5;
    bg.agc_flag=0;
    bg.waterfall_avgnum=400;
    break;
    
    default:
    bg.filter_flat=150;
    bg.filter_curv=250;
    bg.output_gain=0.05F;
    bg.agc_flag = 0;
    bg.waterfall_avgnum=100;
    break;
    }
  bg.bfo_freq=bg.filter_flat+0.1F*bg.filter_curv+300;
  bg.check=BG_VERNR;
  bg.fft_avgnum=50;
  bg.pixels_per_point=1;
  bg.coh_factor=8;
  bg.delay_points=4;
  bg.agc_attack = 2;
  bg.agc_release = 4;
  bg.agc_hang=2;
  bg.waterfall_gain=1;
  bg.waterfall_zero=20;
  bg.wheel_stepn=0;
  bg.oscill_on=0;
  bg.oscill_gain=0.1F;
  bg.horiz_arrow_mode=0;
  bg.mixer_mode=1;
  bg.filter_shift=0;
  bg.fm_mode=0;
  bg.fm_subtract=0;
  bg.fm_audio_bw=10;
  }
if(bg.check != BG_VERNR)goto bg_default;
errcnt++; 
if(errcnt < 2)
  {
  if(bg.xleft < 0 || 
     bg.xleft > screen_last_xpixel ||
     bg.xright < 0 || 
     bg.xright > screen_last_xpixel ||
     bg.xright-bg.xleft < BG_MIN_WIDTH ||
     bg.ytop < 0 || 
     bg.ytop > screen_height-1 ||
     bg.ybottom < 0 || 
     bg.ybottom > screen_height-1 ||
     bg.ybottom-bg.ytop < 6*text_height ||
     bg.pixels_per_point > 16 ||
     bg.pixels_per_point < 1 ||
     bg.coh_factor < 1 || 
     bg.coh_factor > 9999 ||
     bg.delay_points < 0 || 
     bg.delay_points > 999 ||
     bg.waterfall_avgnum < 1 ||
     bg.agc_flag < 0 || 
     bg.agc_flag > 2 ||
     bg.horiz_arrow_mode<0 || 
     bg.horiz_arrow_mode > 2 ||
     bg.mixer_mode < 1 ||
     bg.mixer_mode > 2  ||
     bg.filter_shift < -999 ||
     bg.filter_shift > 999 ||
     bg.fm_mode < 0 ||
     bg.fm_mode > MAX_FM_FFTN-2 ||
     bg.fm_subtract < 0 ||
     bg.fm_subtract > 2 ||
     bg.fm_audio_bw > genparm[DA_OUTPUT_SPEED]/2000. ||
     bg.fm_audio_bw < 1 ||
     bg.ch2_phase <0 ||
     bg.ch2_phase >= MAX_CH2_PHASES ||
     bg.agc_attack < 0 ||
     bg.agc_attack > 9 ||
     bg.agc_release < 0 ||
     bg.agc_release > 9 ||
     bg.agc_hang < 0 ||
     bg.agc_hang > 9
     )goto bg_default;
  }
if(fft1_correlation_flag > 0)bg.mixer_mode=2;     
bg_no_of_notches=0;
bg_current_notch=0;
new_bg_agc_flag=bg.agc_flag;
if(bg.wheel_stepn<-30 || bg.wheel_stepn >30)bg.wheel_stepn=0;
bfo_flag=0;  
mix2.size=-1;
if(fft1_correlation_flag == 2)genparm[OUTPUT_MODE]=51;
new_daout_bytes=1+(genparm[OUTPUT_MODE]&1);           //bit 0
new_daout_channels=1+((genparm[OUTPUT_MODE]>>1)&1);   //bit 1
bg_expand=(genparm[OUTPUT_MODE]>>2)&3;                //bit 2 and 3
new_bg_coherent=(genparm[OUTPUT_MODE]>>4)&3;          //bits 4 and 5
new_bg_delay=(genparm[OUTPUT_MODE]>>6)&1;             //bit 6
new_bg_twopol=(genparm[OUTPUT_MODE]>>7)&1;            //bit 7 
new_bg_fm_mode=bg.fm_mode;
new_bg_fm_subtract=bg.fm_subtract;
new_bg_agc_flag=bg.agc_flag;
new_bg_ch2_phase=0;
baseb_channels=0;
baseband_sampling_speed=0;
timf1_to_baseband_speed_ratio=timf1_sampling_speed;
timf2_blockpower_block=4*ui.rx_rf_channels*timf1_to_baseband_speed_ratio;
baseband_graph_scro=no_of_scro;
bfo_xpixel=-1;
if(ui.operator_skil != OPERATOR_SKIL_EXPERT)
  {
  bg.oscill_on=0;
  }  
init_baseband_sizes();
make_baseband_graph(FALSE);
if(kill_all_flag)return;
no_of_scro++;
if(no_of_scro >= MAX_SCRO)lirerr(89);
for(i=0; i<screen_width*ui.rx_rf_channels; i++)fft3_spectrum[i]=bg_y0;
// Clear bg_timestamp_counter which is used to show time in the waterfall graph
// every time 2*text_height new waterfall lines have arrived.
bg_timestamp_counter=0;
}

