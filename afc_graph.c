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
#include "fft2def.h"
#include "screendef.h"
#include "vernr.h"
#include "seldef.h"
#include "llsqdef.h"
#include "sigdef.h"
#include "thrdef.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

#define FT 0.000001
#define AG_STON_RANGE 1.5  //=15dB
#define AG_CHARS 24

// Global pointers for AFC
// ag_ss    Current signal ( 0 is signal to loudspeaker)
// agp0     Pointer to ag_....[0] for current signal  
// mix1p0   Pointer to mix1_...[0] for current signal
// fftx_na  Latest available fftx block
// fftx_nx  Latest unused fftx block
//          Frequency at start of block is mix1_fq_start (set by mix1)     
//          Frequency af block midpoint is mix1_fq_mid (set here and by mix1)
// fftx_nc  Old value for fftx_na (from previous afc call)
// fftx_ny  Latest fftx_block for which data is stored in ag_mix1freq[]


// mix1 arrays:
// mix1_fq_mid    frequency to use by mix1 (at midpoint of block)
// mix1_fq_start  frequency to use by mix1 (at beginning of block)
// mix1_fq_slope  frequency change, set and used by mix1
// mix1_fq_curv   frequency change, 2nd order, set and used by mix1
// mix1_eval_avgn  No of fftx blocks used to calculate eval data
// mix1_eval_fq    A maximum is found at this frequency (if positive)
// mix1_eval_ampl  Signal power at eval_fq
// mix1_eval_noise Noise around fq

int afc_graph_scro;
int ag_old_y1;
int ag_old_y2;
int ag_old_x1;
int ag_old_x2;
int new_ag_mode_control;

void make_afc_graph(int clear_old);

int afc_fit_line(void)
{
int i,j,k, np;
float t1, t2, wfq, ston_avgsumsq, r1, r2;
// Make an S/N weighted least squares fit of a straight line
// to mix1_eval_fq from fftx_nf1 to fftx_nf2.
// First store weights ( S/N ) in afc_spectrum which can 
// be used as scratch area.
afc_slope=0;
t1=0;
ston_avgsumsq=0;
k=0;
j=0;
np=fftx_nf1;
while(np != fftx_nf2)
  {
  afc_spectrum[j]=(mix1_eval_sigpwr[mix1p0+np]-mix1_eval_noise[mix1p0+np])/
                                mix1_eval_noise[mix1p0+np];
  if( afc_spectrum[j] > 0)
    {
    t1+=afc_spectrum[j];
    ston_avgsumsq+=afc_spectrum[j]*afc_spectrum[j];
    k++; 
    }
  else
    {
    afc_spectrum[j]=0;
    }
  j++;     
  np=(np+1)&fftxn_mask;
  }
if(k==0)return 0;
ston_avgsumsq/=(float)k;
// ston_avgsumsq is average of S/N ratio squared.
// If RMS value of S/N is below ag.minston the data is no good so there
// is no reason to try to fit any line to the points.
if(ston_avgsumsq < ag.minston) return 0;
t1/=(float)k;     
// Exclude points that contribute with less than 10% of the average
// contribution.
// Get the S/N weighted average frequency into wfq
t1*=.1F;
t2=0;
wfq=0;
np=fftx_nf1;
for(i=0; i<ag.fit_points; i++)
  {
  if(afc_spectrum[i] > t1)
    {
    t2+=afc_spectrum[i];
    wfq+=afc_spectrum[i]*mix1_eval_fq[mix1p0+np];
    }
  np=(np+1)&fftxn_mask;
  }  
if(t2==0)return 0;
wfq/=t2;
// wfq is the S/N weighted center frequency.
// Count how many points remain when we skip points that are clearly
// not compatible with a reasonable frequency drift.
t2=0.5F*(float)(ag.fit_points*(afc_speknum+1))*afc_drift_step/fftx_points_per_hz;
llsq_neq=0;
np=fftx_nf1;
for(i=0; i<ag.fit_points; i++)
  {
  if(afc_spectrum[i] > t1)
    {
    if(fabs(wfq-mix1_eval_fq[mix1p0+np]) < t2)
      {
      llsq_neq++;
      }
    }
  np=(np+1)&fftxn_mask;
  }  
// If we have less than 4 points, do not fit any line, just use average.
if(ag.fit_points < 4 || llsq_neq < 2)goto use_average;
llsq_derivatives=(float*)((char*)(afc_spectrum)+
                  (((unsigned int)ag.fit_points*sizeof(float)+15)&0xfffffff0));
llsq_errors=llsq_derivatives+
                     ((2*(unsigned int)llsq_neq*sizeof(float)+15)&0xfffffff0);  
llsq_npar=2;
k=0;
np=fftx_nf1;
for(i=0; i<ag.fit_points; i++)
  {
  if(afc_spectrum[i] > t1)
    {
    if(fabs(wfq-mix1_eval_fq[mix1p0+np]) < t2)
      {
      llsq_derivatives[k]=afc_spectrum[i];
      llsq_derivatives[llsq_neq+k]=(float)i*afc_spectrum[i];
      llsq_errors[k]=afc_spectrum[i]*mix1_eval_fq[mix1p0+np];
      k++;
      }
    }
  np=(np+1)&fftxn_mask;
  }
if(llsq1() != 0)
  {
  lirerr(32330);
  return 0;
  }
// Store the straight line we got into mix1_fitted_fq.
// Calculate S/N weighted sum of squares for frequency error  
// Also calculate under assumption we adopted the average frequency wfq.
t1=0;
t2=0;
r2=0;
np=fftx_nf1;
afc_slope=llsq_steps[1];
for(i=0; i<ag.fit_points; i++)
  {
  r1=llsq_steps[0]+llsq_steps[1]*(float)i;
  if(r1 < mix1_lowest_fq)r1=mix1_lowest_fq;
  if(r1 > mix1_highest_fq)r1=mix1_highest_fq;
  mix1_fitted_fq[mix1p0+np]=r1;
  t1+=(float)pow((r1-mix1_eval_fq[mix1p0+np]),2.0)*afc_spectrum[i];
  t2+=(float)pow((wfq-mix1_eval_fq[mix1p0+np]),2.0)*afc_spectrum[i];
  r2+=(float)pow((mix1_good_freq[ag_ss]-mix1_eval_fq[mix1p0+np]),2.0)*afc_spectrum[i];
  np=(np+1)&fftxn_mask;
  }
// t1, t2 and r2 are sums of squared errors.
// for reasonably good data t1<t2<r2.
// If the slope does not really help, just use a constant frequency.  
if(t1/t2 > 0.7)
  {
use_average:;  
  afc_slope=0;
  if(wfq < mix1_lowest_fq)wfq=mix1_lowest_fq;
  if(wfq > mix1_highest_fq)wfq=mix1_highest_fq;
  np=fftx_nf1;
  for(i=0; i<ag.fit_points; i++)
    {
    mix1_fitted_fq[mix1p0+np]=wfq;
    np=(np+1)&fftxn_mask;
    }
  }  
return 1;
}



void afc_eval_line(float wid)
{
int i, np, retcod,p_nf1,p_nf2,pb;
int no_of_points;
float t1,t2;
pb=ag_pf1;
while(pb != ag_pf2)
  {
  ag_fitted_fq[pb]=-1;
  pb=(pb+1)&ag_mask;
  }
// The data in the mix1_eval arrays is obtained under the assumption
// that the extrapolated data in mix1_fq is essentially correct.
// Fit a line to the data in mix1_eval and use it to update the
// data in mix1_fq if the fit is reasonably good.
// Make mix1_fq constant in case the fit is poor.
// First update afct_avgnum according to current afc_maxval and afc_noise
// Make no_of_points=sqrt(w*w+5*5) toget something going from minimum 5 to 
// search width in a smooth way.
t1=wid*(float)baseband_bw_fftxpts;
if( t1 > afcf_search_range/2)
  {
  t1=afcf_search_range/2;
  }
t1=sqrt(t1*t1+25.F);
no_of_points=(int)t1;
if(ag.mode_control == 1)make_afct_avgnum();
// Step through the currently available mix1_eval data and
// check if it is already present with the correct afct_avgnum. 
// Call make_ag_point if not.
fftx_nf2=(fftx_na-afct_half_avgnum+max_fftxn)&fftxn_mask;
fftx_nf1=(fftx_nf2-ag.fit_points+max_fftxn)&fftxn_mask;
p_nf1=(ag_pa-((fftx_na-fftx_nf1+max_fftxn)&fftxn_mask)+ag_size)&ag_mask;
ag_ps=p_nf1;
p_nf2=(p_nf1+ag.fit_points)&ag_mask;
ag_pf1=p_nf1;
ag_pf2=p_nf2;
np=fftx_nf1;
while(np != fftx_nf2)
  {
  if(mix1_eval_avgn[mix1p0+np]!=afct_avgnum || mix1_eval_sigpwr[mix1p0+np] <0)
    {
    make_ag_point(np,no_of_points);
    if(kill_all_flag) return;
    }
  np=(np+1)&fftxn_mask;
  }
// Make an S/N weighted least squares fit of a straight line
// to the frequency from fftx_nf1 to fftx_nf2.
if(afc_fit_line() == 0)  
  {
  retcod=0;
  }
else
  {
  retcod=1;  
  }
if(kill_all_flag) return;  
// The first mixer has already used the points fftx_ny to fftx_nx-1 
// Fill in what we already did in ag_mix1freq
// fftx_na corresponds to ag_pa.

if(fftx_nx != fftx_ny)
  {
  i=(ag_pa-((fftx_na-fftx_ny+max_fftxn)&fftxn_mask)+ag_size)&ag_mask;
  while(fftx_ny != fftx_nx)
    {
    ag_mix1freq[i]=mix1_fq_mid[mix1p0+fftx_ny];
    fftx_ny=(fftx_ny+1)&fftxn_mask;
    i=(i+1)&ag_mask;
    }
// We will write on screen up to ag_pa. Clear data!  
  while(i != ag_pa)
    {
    ag_mix1freq[i]=-1;
    i=(i+1)&ag_mask;
    }
  }
// Store the new fitted line in ag_fitted_fq
// Set new_redraw to point to the first value, we want to remember
// in order to clear the point next time.
// fftx_na corresponds to ag_pa.
np=fftx_nf1;
pb=p_nf1;
if(retcod != 0)
  {
  while(np != fftx_nf2)
    {
    ag_fitted_fq[pb]=mix1_fitted_fq[mix1p0+np];
    ag_freq[pb]=mix1_eval_fq[mix1p0+np];
    if(mix1_eval_sigpwr[mix1p0+np] > 0)
      {
      ag_ston[pb]=(float)log10(mix1_eval_sigpwr[mix1p0+np]/mix1_eval_noise[mix1p0+np]);
      }
    else
      {
      ag_ston[pb]=0;
      }  
    pb=(pb+1)&ag_mask;
    np=(np+1)&fftxn_mask;
    }
  }
else
  {
  while(np != fftx_nf2)
    {
    mix1_fitted_fq[mix1p0+np]=mix1_good_freq[ag_ss];
    ag_freq[pb]=mix1_eval_fq[mix1p0+np];
    pb=(pb+1)&ag_mask;
    np=(np+1)&fftxn_mask;
    }
  }
i=(ag_pa+4)&ag_mask;
while(pb != i)
  {  
  ag_freq[pb]=-1;
  ag_ston[pb]=0;
  pb=(pb+1)&ag_mask;
  }
// mix1_fitted_fq contains a frequency that is drifting linearly with 
// time, with the slope afc_slope.
// The mixer already used points up to fftx_nx-1 so it is to late to
// do anything with them.
// Fill mix1_fq_mid from fftx_nx to fftx_na by use of the
// data in mix1_fitted_fq.
np=(fftx_nf2-1+max_fftxn)&fftxn_mask;
t2=mix1_fitted_fq[mix1p0+np];
i=(fftx_nx-np+fftxn_mask)&fftxn_mask;
if(i > (max_fftxn>>2))i-=max_fftxn;
t2+=((float)i-0.5F)*afc_slope;
np=(fftx_nx+fftxn_mask)&fftxn_mask;
while(np != fftx_na)
  {
  t2+=afc_slope;
  np=(np+1)&fftxn_mask;
  if(t2 < mix1_lowest_fq)t2=mix1_lowest_fq;
  if(t2 > mix1_highest_fq)t2=mix1_highest_fq;
  mix1_fq_mid[mix1p0+np]=t2;
  }
// We have now set up a frequency function for the AFC.
// Display it on screen if it is the main signal (for the loudspeaker).
}


void make_afc(void)
{
int i, np, kk;
float t1, current_selfreq;
if(old_mix1_selfreq != mix1_selfreq[0])
  {
  baseb_reset_counter++;
  return;
  }
// Find a signal.
// Or follow a signal on which we already are locked.
// Depending on mix1_status:
// status=0    =>  first call. Everything unknown.
// status=1    =>  a frequency is set but no signal was really detected.
// status=2    =>  Signal detected. Frequency with linear drift stored.
// status=3    =>  Signal tracking seems ok.
// status=4    =>  Signal lost, using constant frequency
if(genparm[SECOND_FFT_ENABLE] != 0)
  {
  fftx_na=fft2_na;
  fftx_nc=fft2_nc;
  fftx_nm=fft2_nm;
  fftx_nx=fft2_nx;
  }
else
  {
  fftx_na=fft1_nb;
  fftx_nc=fft1_nc;
  fftx_nm=fft1_nm;
  fftx_nx=fft1_nx;
  }

for(ag_ss=0; ag_ss<genparm[MIX1_NO_OF_CHANNELS]; ag_ss++)
  {
  current_selfreq=mix1_selfreq[ag_ss];
  if(current_selfreq >= 0)
    {
    mix1p0=ag_ss*max_fftxn;
// ****************************************************
// If AFC is disabled, stay at a constant frequency.
    if(ag.mode_control == 0)
      {
      if(mix1_status[ag_ss] == 1000)goto skip;
      if(mix1_status[ag_ss] == 0)
        {
        t1=current_selfreq;
        mix1_good_freq[ag_ss]=current_selfreq;
        }
      else
        {
        t1=mix1_good_freq[ag_ss];
        }
      for(np=0; np<max_fftxn; np++)
        {      
        mix1_fq_mid[mix1p0+np]=t1;
        }
      afc_cursor_color=14;
      mix1_status[ag_ss]=1000;
      goto skip;
      }  
// *******************************************************
// Set mix1_eval_avgn to -1 so we know evaluation data is 
// not valid for new data points.
// Set mix1_eval_sigpwr to -1 so we do not have to test flag when inconvenient.
    np=fftx_nc;
    while( np != fftx_na )
      {
      mix1_eval_avgn[mix1p0+np]=-1;
      mix1_eval_sigpwr[mix1p0+np]=-1;
      np=(np+1)&fftxn_mask;  
      }
    switch (mix1_status[ag_ss])
      {
      case 0:
      afc_graph_filled=0;
      afc_cursor_color=14;
// The user has selected a signal.
// Try to find a signal and store frequencies in mix1_fq_mid[]
      collect_initial_spectrum();
// The mixer wants to know some data from the previous transform
// in order to produce a continous phase
      np=(fftx_nx+fftxn_mask)&fftxn_mask;
      t1=mix1_fq_mid[mix1p0+np];
      mix1_fq_start[mix1p0+fftx_nx]=0.5F*(t1+mix1_fq_mid[mix1p0+fftx_nx]);
      t1*=fftx_points_per_hz;
      kk=(int)(t1+0.5F);
      mix1_good_freq[ag_ss]=mix1_fq_start[mix1p0+fftx_nx];
      mix1_point[ag_ss]=kk;   
      mix1_phase[ag_ss]=0;
      mix1_phase_step[ag_ss]=0;
      ag_pa=afc_tpts;
      ag_px=0;  
      fftx_ny=fftx_nx;
// Set mix1_eval_avgn to -1 so we know evaluation data is not valid
// Set mix1_eval_sigpwr to -1 so we do not have to test flag when inconvenient.
      for(i=0; i<max_fftxn; i++)
        {
        mix1_eval_avgn[mix1p0+i]=-1;
        mix1_eval_sigpwr[mix1p0+i]=-1;
        }
      for(i=0; i<ag_size; i++)
        {
        ag_fitted_fq[i] = -1;
        ag_freq[i]=-1;
        ag_ston[i]=0;
        }
      break;
      
      case 1:
// A frequency is set but the signal may be elsewhere.
// We can not afford a complete search each time arriving here,
// and it would not be very helpful either. 
// For weak signals averaging is done over many transforms so
// just one more will not help.
// If the signal is readable we will easily find it without
// searching over all possible frequency drifts unless it is
// terribly unstable in which case the operator will have to
// press the button again to initiate a new search.      
// öö  simple search routine missing!!!!!!!!!!
      np=fftx_nc;
      t1=mix1_fq_mid[mix1p0+np];
      while(np != fftx_na)
        {      
        np=(np+1)&fftxn_mask;
        mix1_fq_mid[mix1p0+np]=t1;
        }
      mix1_good_freq[ag_ss]=t1;
      break;
      
      
      case 2:
// If we arrive here, the first search was sucessful.
// In case we are in manual mode we want a fixed number of points
// to average over.
      if(ag.mode_control != 2)
        {
// In auto mode, force evaluation of S/N and a new eval size
// by setting an impossible afct_avgnum.
        afct_avgnum=-1;
        }
      afc_eval_line(0.5);
      mix1_status[ag_ss]=3;
      afc_cursor_color=10;
      afc_graph_filled=1;
      sc[SC_FILL_AFC]++;
      goto cs3;

      case 3:
// Step through recent points and get the average noise levels from
// valid points only.
      if(make_afc_signoi()==0 && ag.mode_control != 2)
        {
        afc_cursor_color=9;
        mix1_status[ag_ss]=4;
        }
      else
        {  
        afc_eval_line(0.5);
cs3:;
        if(ag.mode_control != 2)
          {
          mix1_status[ag_ss]=4;
          afc_cursor_color=9;
          }
        goto skip;
        }
      break;

      case 4:
// We have lost the signal when following at narrow search range.
// Set the maximum value for afct_avgnum (by forcing S/N=0).
      afc_maxval=1;
      afc_noise=1;
// Now search a wider range. 
// 2.5 times the selected bandwidth or half the search range
      afc_eval_line(2.5);
      mix1_status[ag_ss]=4;
      afc_cursor_color=9;
      goto skip;

      case 5:
// We have lost the signal.
      break;

      case 1000:
// AFC was deselected and is now enabled again.
      if(mix1_selfreq[ag_ss] > 0)
        {
        mix1_status[ag_ss]=0;
        mix1_selfreq[ag_ss]=mix1_good_freq[ag_ss];
        }
      break;      

      default:
      lirerr(889962);
      return;
      }
    }
  }
skip:;  
if(mix1_selfreq[0] > 0 && genparm[SECOND_FFT_ENABLE] != 0)
  {
  sc[SC_AFC_CURSOR]++;
  }
fft2_nc=(fft2_nc+1+max_fft2n)&fft2n_mask;
fft1_nc=(fft1_nc+1+max_fft1n)&fft1n_mask;
}

void new_afc_graph(void)
{
make_afc_graph(TRUE);
sc[SC_FILL_AFC]++;
}

void check_afct_points(void)
{
if(ag.fit_points > max_afc_fit)ag.fit_points = max_afc_fit;
if(ag.fit_points > max_fftxn-afct_avgnum+1)
                         ag.fit_points = max_fftxn-afct_avgnum+1;
if(ag.fit_points < 1)ag.fit_points=1;
afct_delay_points=afct_half_avgnum+ag.fit_points/2;
if(ag.delay > afct_delay_points)ag.delay=afct_delay_points;
if(ag.delay < 0)ag.delay=0;
if(afct_delay_points > ag.delay)afct_delay_points=ag.delay;
if(new_baseb_flag >= 0 && afct_delay_points != old_afct_delay)baseb_reset_counter++;
afc_tpts=ag.fit_points+afct_half_avgnum;
}

void new_afc_avgnum(void)
{
ag.avgnum=numinput_int_data;
// Make sure to write to screen so user knows it is ok.
afct_avgnum=0;
make_afct_window(ag.avgnum);
if(kill_all_flag) return;
ag.avgnum=afct_avgnum;
check_afct_points();
make_modepar_file(GRAPHTYPE_AG);
}


void new_afc_fit_points(void)
{
ag.fit_points=numinput_int_data;
check_afct_points();
new_afc_graph();
}

void new_afc_delay(void)
{
ag.delay=numinput_int_data;
check_afct_points();
new_afc_graph();
}

void help_on_afc_graph(void)
{
int msg_no;
int event_no;
// Nothing is selected in the data area.
msg_no=-1;
// In case we are on one of the control bars, select the
// appropriate message.
if(mouse_y <= ag_fpar_y0 && mouse_y >= ag_fpar_ytop)
  {
  if( mouse_x<ag_first_xpixel)
    {
    if(mouse_x >= ag_ston_x1 && mouse_x <= ag_ston_x2)
      {
      if(genparm[SECOND_FFT_ENABLE] == 0)
        {
        msg_no=21;
        }
      else
        {
        msg_no=22;
        }
      }
    else
      {
      if(mouse_x >= ag_lock_x1 && mouse_x <= ag_lock_x2)
        {
        msg_no=23;
        }
      else
        {
        if(mouse_x >= ag_srch_x1 && mouse_x <= ag_srch_x2)
          {
          msg_no=24;
          }
        }
      }  
    }
  }
for(event_no=0; event_no<MAX_AGBUTT; event_no++)
  {
  if( agbutt[event_no].x1 <= mouse_x && 
      agbutt[event_no].x2 >= mouse_x &&      
      agbutt[event_no].y1 <= mouse_y && 
      agbutt[event_no].y2 >= mouse_y) 
    {
    switch (event_no)
      {
      case AG_TOP:
      case AG_BOTTOM:
      case AG_LEFT:
      case AG_RIGHT:
      msg_no=100;
      break;

      case AG_FQSCALE_EXPAND:
      msg_no=25;
      break;

      case AG_FQSCALE_CONTRACT:
      msg_no=26;
      break;

      case AG_MANAUTO:
      msg_no=27;
      break;

      case AG_WINTOGGLE:
      msg_no=28;
      break;

      case AG_SEL_AVGNUM:
      if(genparm[SECOND_FFT_ENABLE] == 0)
        {
        msg_no=29;
        }
      else
        {
        msg_no=30;
        }
      break;

      case AG_SEL_DELAY:
      msg_no=31;
      break;

      case AG_SEL_FIT:
      msg_no=32;
      break;
      }
    }  
  }
help_message(msg_no);
}  

void mouse_continue_afc_graph(void)
{
char s[80];
int j;
switch (mouse_active_flag-1)
  {
  case AG_TOP:
  if(ag.ytop!=mouse_y)
    {
    pause_screen_and_hide_mouse();
    graph_borders((void*)&ag,0);
    ag.ytop=mouse_y;
    j=ag.ybottom-2*text_height;
    if(ag.ytop > j)ag.ytop=j;      
    if(ag_old_y1 > ag.ytop)ag_old_y1=ag.ytop;
    graph_borders((void*)&ag,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case AG_BOTTOM:
  if(ag.ybottom!=mouse_y)
    {
    pause_screen_and_hide_mouse();
    graph_borders((void*)&ag,0);
    ag.ybottom=mouse_y;
    j=ag.ytop+2*text_height;
    if(ag.ybottom < j)ag.ybottom=j;      
    if(ag_old_y2 < ag.ybottom)ag_old_y2=ag.ybottom;
    graph_borders((void*)&ag,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case AG_LEFT:
  if(ag.xleft!=mouse_x)
    {
    pause_screen_and_hide_mouse();
    graph_borders((void*)&ag,0);
    ag.xleft=mouse_x;
    j=ag.xright-32-6*text_width;
    if(ag.xleft > j)ag.xleft=j;      
    if(ag_old_x1 > ag.xleft)ag_old_x1=ag.xleft;
    graph_borders((void*)&ag,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case AG_RIGHT:
  if(ag.xright!=mouse_x)
    {
    pause_screen_and_hide_mouse();
    graph_borders((void*)&ag,0);
    ag.xright=mouse_x;
    j=ag.xleft+32+6*text_width;
    if(ag.xright < j)ag.xright=j;      
    if(ag_old_x2 < ag.xright)ag_old_x2=ag.xright;
    graph_borders((void*)&ag,15);
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
  case AG_FQSCALE_EXPAND:
  ag.frange/=2;
  if(ag.frange < (float)AG_FRANGE_MIN)ag.frange = (float)AG_FRANGE_MIN;
  break;

  case AG_FQSCALE_CONTRACT:
  ag.frange*=1.6F;
  if(ag.frange > (float)AG_FRANGE_MAX)ag.frange = (float)AG_FRANGE_MAX;
  break;

  case AG_MANAUTO:
  new_ag_mode_control=ag.mode_control+1;
  break;

  case AG_WINTOGGLE:
  ag.window^=1;
  if(ag.window == 1) s[0]='W'; else s[0]='-';
  s[1]=0;
  lir_pixwrite(agbutt[AG_WINTOGGLE].x1+text_width/2-1,
                                         agbutt[AG_WINTOGGLE].y1+2,s);
  make_afct_window(afct_avgnum);
  break;

  case AG_SEL_AVGNUM:
  if(ag.mode_control == 2)
    {
    mouse_active_flag=1;
    numinput_xpix=agbutt[AG_SEL_AVGNUM].x1+7*text_width/2-1;
    numinput_ypix=agbutt[AG_SEL_AVGNUM].y1+2;
    numinput_chars=3;    
    erase_numinput_txt();
    numinput_flag=FIXED_INT_PARM;
    par_from_keyboard_routine=new_afc_avgnum;
    return;
    }
  break;

  case AG_SEL_DELAY:
  if(ag.mode_control == 2)
    {
    mouse_active_flag=1;
    numinput_xpix=agbutt[AG_SEL_DELAY].x1+7*text_width/2-1;
    numinput_ypix=agbutt[AG_SEL_DELAY].y1+2;
    numinput_chars=3;    
    erase_numinput_txt();
    numinput_flag=FIXED_INT_PARM;
    par_from_keyboard_routine=new_afc_delay;
    return;
    }
  break;

  case AG_SEL_FIT:
  if(ag.mode_control == 2)
    {
    mouse_active_flag=1;
    numinput_xpix=agbutt[AG_SEL_FIT].x1+7*text_width/2-1;
    numinput_ypix=agbutt[AG_SEL_FIT].y1+2;
    numinput_chars=3;    
    erase_numinput_txt();
    numinput_flag=FIXED_INT_PARM;
    par_from_keyboard_routine=new_afc_fit_points;
    return;
    }
  break;
  }      
finish:;
leftpressed=BUTTON_IDLE;  
mouse_active_flag=0;
new_afc_graph();
}

void make_afc_stony(void)
{
ag_ston_y=ag_fpar_y0-(int)(ag_floatypix*
                              (float)log10(ag.minston)/(float)AG_STON_RANGE);
if(ag_ston_y < ag_fpar_ytop)
  {
  ag_ston_y = ag_fpar_ytop;
  ag.minston=(float)pow(10.,AG_STON_RANGE*(ag_fpar_y0-ag_ston_y)/ag_floatypix);
  }
}

void make_afc_ston(void)
{
int k;
k=ag_fpar_y0-ag_ston_y;
if(k<2)
  {
  k=2;
  ag_ston_y=ag_fpar_y0+2;
  }
ag.minston=(float)pow(10.,AG_STON_RANGE*k/ag_floatypix);
if(ag.minston < 1.1)
  {
  ag.minston=1.1F;
  make_afc_stony();
  }
}

void make_afc_locky(void)
{
ag_lock_y=ag_fpar_y0-(int)(2*ag_floatypix*ag.lock_range/ag.frange);
if(ag_lock_y < ag_fpar_ytop)
  {
  ag_lock_y = ag_fpar_ytop;
  ag.lock_range=0.5F*(float)(ag_fpar_y0-ag_lock_y)*ag.frange/ag_floatypix;  
  }
}

void make_afc_lock(void)
{
ag.lock_range=0.5F*(float)(ag_fpar_y0-ag_lock_y)*ag.frange/ag_floatypix;  
if(ag.lock_range <0)
  {
  ag.lock_range=0;
  make_afc_locky();
  }  
}

void make_afc_searchy(void)
{
ag_srch_y=ag_fpar_y0-(int)(ag_floatypix*(float)ag.search_range);
if(ag_srch_y < ag_fpar_ytop)
  {
  ag_srch_y = ag_fpar_ytop;
  ag.search_range=(float)(ag_fpar_y0-ag_srch_y)/ag_floatypix;  
  }
}

void make_afc_search(void)
{
ag.search_range=(float)(ag_fpar_y0-ag_srch_y)/ag_floatypix;  
if(ag.search_range <0)
  {
  ag.search_range=0;
  make_afc_searchy();
  }  
}

void ag_ston_control(void)
{
int ya,yb;
yb=mouse_y;
if(yb > ag_fpar_y0-2)yb=ag_fpar_y0-2;
if(yb < ag_fpar_ytop)yb=ag_fpar_ytop;
if(ag_ston_y!=yb)
  {
  pause_screen_and_hide_mouse();
  ya=ag_ston_y;
  ag_ston_y=yb;
  make_afc_ston();
  update_bar(ag_ston_x1,ag_ston_x2,ag_fpar_y0,ag_ston_y,ya,
                                              AG_STON_RANGE_COLOR,ag_stonbuf);
  resume_thread(THREAD_SCREEN);
  }
if(leftpressed == BUTTON_RELEASED)
  {
  leftpressed=BUTTON_IDLE;  
  make_modepar_file(GRAPHTYPE_AG);
  mouse_active_flag=0;
  }
}

void ag_lock_control(void)
{
int ya,yb;
yb=mouse_y;
if(yb > ag_fpar_y0-2)yb=ag_fpar_y0-2;
if(yb < ag_fpar_ytop)yb=ag_fpar_ytop;
if(ag_lock_y!=yb)
  {
  pause_screen_and_hide_mouse();
  ya=ag_lock_y;
  ag_lock_y=yb;
  make_afc_lock();
  update_bar(ag_lock_x1,ag_lock_x2,ag_fpar_y0,ag_lock_y,ya,
                                        AG_LOCK_RANGE_COLOR,ag_lockbuf);
  resume_thread(THREAD_SCREEN);
  }
if(leftpressed == BUTTON_RELEASED)
  {
  leftpressed=BUTTON_IDLE;  
  make_modepar_file(GRAPHTYPE_AG);
  mouse_active_flag=0;

  }
}

void ag_srch_control(void)
{
int ya,yb;
yb=mouse_y;
if(yb > ag_fpar_y0-2)yb=ag_fpar_y0-2;
if(yb < ag_fpar_ytop)yb=ag_fpar_ytop;
if(ag_srch_y!=yb)
  {
  pause_screen_and_hide_mouse();
  ya=ag_srch_y;
  ag_srch_y=yb;
  make_afc_search();
  update_bar(ag_srch_x1,ag_srch_x2,ag_fpar_y0,ag_srch_y,ya,
                                              AG_SRC_RANGE_COLOR,ag_srchbuf);
  resume_thread(THREAD_SCREEN);
  }
if(leftpressed == BUTTON_RELEASED)
  {
  leftpressed=BUTTON_IDLE;  
  make_modepar_file(GRAPHTYPE_AG);
  mouse_active_flag=0;
  }
}

void mouse_on_afc_graph(void)
{
int event_no;
// First find out is we are on a button or border line.
for(event_no=0; event_no<MAX_AGBUTT; event_no++)
  {
  if( agbutt[event_no].x1 <= mouse_x && 
      agbutt[event_no].x2 >= mouse_x &&      
      agbutt[event_no].y1 <= mouse_y && 
      agbutt[event_no].y2 >= mouse_y) 
    {
    ag_old_y1=ag.ytop;
    ag_old_y2=ag.ybottom;
    ag_old_x1=ag.xleft;
    ag_old_x2=ag.xright;
    mouse_active_flag=1+event_no;
    current_mouse_activity=mouse_continue_afc_graph;
    return;
    }
  }
// Not button or border.
if( mouse_y <= ag_fpar_y0 && 
    mouse_y >= ag_fpar_ytop &&
    mouse_x < ag_first_xpixel)
  {
  if(mouse_x >= ag_ston_x1 && mouse_x <= ag_ston_x2)
    {
    current_mouse_activity=ag_ston_control;
    }
  else
    {
    if(mouse_x >= ag_lock_x1 && mouse_x <= ag_lock_x2)
      {
      current_mouse_activity=ag_lock_control;
      }
    else
      {
      if(mouse_x >= ag_srch_x1 && mouse_x <= ag_srch_x2)
        {
        current_mouse_activity=ag_srch_control;
        }
      else
        {
        current_mouse_activity=mouse_nothing;
        }
      }
    }  
  }
else
  {
  current_mouse_activity=mouse_nothing;
  }
mouse_active_flag=1;
}

void make_afc_graph(int clear_old)
{
size_t mem1,mem2,mem3;
int i,k;
int ix1, ix2, iy1, iy2;
int old_pixnum;
pause_thread(THREAD_SCREEN);
if(clear_old)
  {
  hide_mouse(ag_old_x1,ag_old_x2,ag_old_y1,ag_old_y2);
  lir_fillbox(ag_old_x1,ag_old_y1,ag_old_x2-ag_old_x1+1,
                                                    ag_old_y2-ag_old_y1+1,0);
  }
current_graph_minh=4*text_height;
current_graph_minw=8*text_width;
check_graph_placement((void*)(&ag));
clear_button(agbutt, MAX_AGBUTT);
hide_mouse(ag.xleft,ag.xright,ag.ytop,ag.ybottom);  
if(ag.fit_points <= 1)
  {
  afc_speknum=1;
  ag.fit_points=1;
  }
else
  {  
  if(ag.fit_points > max_fftxn-3)ag.fit_points=max_fftxn-3;
  afc_speknum=(int)((float)ag.fit_points*afcf_max_drift/0.75F);
  }
afc_speknum=(int)(((unsigned int)afc_speknum)&0xfffffffe)+1;  
afc_half_speknum=afc_speknum/2;
afc_drift_step=3.F/(float)ag.fit_points;
// The afc graph shows frequency decimals in 5 characters
// at the left side of the graph.
// Make sure the graph is big enough to show ag.fit_points!
old_pixnum=ag_xpixels;
ag_size=(int)(1.1F*(float)(ag.xright-ag.xleft-6*text_width));
if(ag_size < ag.fit_points+20 || ag_size < AG_CHARS*text_width)
  {
  i=(ag.xright+ag.xleft)/2;
  k=ag.fit_points+20;
  if(k < AG_CHARS*text_width)k=AG_CHARS*text_width;
  k+=6*text_width+2;
  k>>=1;
  ag.xright=i+k;
  ag.xleft=i-k;
  k<<=1;
  if(ag.xleft < 0)
    {
    ag.xleft=0;
    ag.xright=k;
    }
  if(ag.xright > screen_last_xpixel)
    {
    ag.xright=screen_last_xpixel;
    ag.xleft=ag.xright-k;  
    }
  }
ag_size=(int)(1.1F*(float)(ag.xright-ag.xleft-6*text_width));
make_power_of_two(&ag_size);
ag_mask=ag_size-1;
ag_y0=ag.ybottom-text_height-4;
ag_fpar_y0=ag_y0;
ag_y1=ag.ytop+text_height+2;
ag_fpar_ytop=ag.ytop+3*text_height/2+2;
scro[afc_graph_scro].no=AFC_GRAPH;
scro[afc_graph_scro].x1=ag.xleft;
scro[afc_graph_scro].x2=ag.xright;
scro[afc_graph_scro].y1=ag.ytop;
scro[afc_graph_scro].y2=ag.ybottom;
agbutt[AG_LEFT].x1=ag.xleft;
agbutt[AG_LEFT].x2=ag.xleft+2;
agbutt[AG_LEFT].y1=ag.ytop;
agbutt[AG_LEFT].y2=ag.ybottom;
agbutt[AG_RIGHT].x1=ag.xright-2;
agbutt[AG_RIGHT].x2=ag.xright;
agbutt[AG_RIGHT].y1=ag.ytop;
agbutt[AG_RIGHT].y2=ag.ybottom;
agbutt[AG_TOP].x1=ag.xleft;
agbutt[AG_TOP].x2=ag.xright;
agbutt[AG_TOP].y1=ag.ytop;
agbutt[AG_TOP].y2=ag.ytop+2;
agbutt[AG_BOTTOM].x1=ag.xleft;
agbutt[AG_BOTTOM].x2=ag.xright;
agbutt[AG_BOTTOM].y1=ag.ybottom-2;
agbutt[AG_BOTTOM].y2=ag.ybottom;
// Draw the border lines
graph_borders((void*)&ag,7);
settextcolor(7);
make_button(ag.xleft+text_width,ag.ytop+text_height/2+3,
                                         agbutt,AG_FQSCALE_EXPAND,24);
make_button(ag.xleft+3*text_width,ag.ytop+text_height/2+3,
                                         agbutt,AG_FQSCALE_CONTRACT,25);
ag_first_xpixel=ag.xleft+(2*AG_DIGITS+1)*text_width/2;
ag_xpixels=ag.xright-ag_first_xpixel-1;
ag_height=ag.ybottom-ag.ytop;
if(old_pixnum == ag_xpixels && 
   ag_height <= ag_old_height && 
   ag_old_fit_points >= ag.fit_points)goto mem_ok;
ag_shift_xpixels=(int)(0.9F*(float)ag_xpixels);
if(ag_shift_xpixels<32)ag_shift_xpixels=32;
if(ag_shift_xpixels >(int)(0.7F*(float)ag_xpixels))
  {
  ag_shift_xpixels=(int)(0.7F*(float)ag_xpixels);
  }
// ***************************************************************
//If the graph size is changed, set main signal status to zero
mix1_status[0]=0;
if(afc_handle != NULL)
  {
  if(kill_all_flag)return;
  afc_handle=chk_free(afc_handle);
  }
init_memalloc(afcmem, MAX_AFC_ARRAYS);
mem1=(size_t)((2+ag.fit_points)*ag.fit_points+16);
if(mem1<(size_t)(afc_speknum*max_afcf_points))
  {
  mem1=(size_t)(afc_speknum*max_afcf_points);
  }
if(mem1<(size_t)max_fftxn)mem1=(size_t)max_fftxn;
mem2=(size_t)(MAX_MIX1*ag_size)*sizeof(float);
ag_old_height=ag_height;
mem3=(size_t)((text_width+1)*ag_height)*sizeof(char);
mem(1,&afc_spectrum,mem1*sizeof(float),0);
if(ui.rx_rf_channels == 2)
  {
  mem(2,&afc_xysum,(size_t)(afc_speknum*max_afcf_points)*
                                              sizeof(TWOCHAN_POWER),0);
  }  
mem(3,&afc_lowres_spectrum,(size_t)(1+max_afcf_points)*sizeof(float)/2,0);
mem(4,&ag_mix1freq,mem2,0);
mem(5,&ag_freq,mem2,0);
mem(6,&ag_ston,mem2,0);
mem(7,&afct_window,(size_t)max_afc_fit*sizeof(float),0);
mem(8,&ag_fitted_fq,mem2,0);
mem(9,&ag_oldpix,AG_CURVNUM*(size_t)(ag_xpixels+1)*sizeof(short int),0);
mem(10,&ag_background,(size_t)screen_height*sizeof(char),0);
mem(11,&ag_stonbuf,mem3,0);
mem(12,&ag_srchbuf,mem3,0);
mem(13,&ag_lockbuf,mem3,0);
afc_totmem=memalloc(&afc_handle,"afc");
if(afc_totmem == 0)
  {
  lirerr(1106);
  return;
  }
for(i=0; i<(int)mem1; i++)
  {
  afc_spectrum[i]=0;
  }
for(i=0; i<MAX_MIX1*ag_size; i++)ag_mix1freq[i]=0;
mem_ok:;
for(i=0; i<AG_CURVNUM*(ag_xpixels+1); i++)ag_oldpix[i]=-1;
// ************** Make button to select AFC mode (Auto, Man, Off)
ix1=ag.xleft+2;
ix2=ix1+3*text_width/2;
iy2=ag.ybottom-2;
iy1=iy2-text_height-1;
agbutt[AG_MANAUTO].x1=ix1;
agbutt[AG_MANAUTO].x2=ix2;
agbutt[AG_MANAUTO].y1=iy1;
agbutt[AG_MANAUTO].y2=iy2;
// ************** Make button window on/off
ix1=ix2+2;
ix2=ix1+3*text_width/2;
agbutt[AG_WINTOGGLE].x1=ix1;
agbutt[AG_WINTOGGLE].x2=ix2;
agbutt[AG_WINTOGGLE].y1=iy1;
agbutt[AG_WINTOGGLE].y2=iy2;
// ***********  make avgnum button ******************
ix1=ix2+2;
ix2=ix1+13*text_width/2;
agbutt[AG_SEL_AVGNUM].x1=ix1;
agbutt[AG_SEL_AVGNUM].x2=ix2;
agbutt[AG_SEL_AVGNUM].y1=iy1;
agbutt[AG_SEL_AVGNUM].y2=iy2;
// ***********  make fit size button ******************
ix1=ix2+2;
ix2=ix1+13*text_width/2;
agbutt[AG_SEL_FIT].x1=ix1;
agbutt[AG_SEL_FIT].x2=ix2;
agbutt[AG_SEL_FIT].y1=iy1;
agbutt[AG_SEL_FIT].y2=iy2;
// ***********  make delay percentage button ******************
ix1=ix2+2;
ix2=ix1+13*text_width/2;
agbutt[AG_SEL_DELAY].x1=ix1;
agbutt[AG_SEL_DELAY].x2=ix2;
agbutt[AG_SEL_DELAY].y1=iy1;
agbutt[AG_SEL_DELAY].y2=iy2;
// Get the number of pixels we have vertically for control bars.
ag_ston_x1=ag.xleft+2;
ag_ston_x2=ag_ston_x1+text_width;
ag_floatypix=(float)(ag_y0-ag.ytop);
make_afc_stony();
make_afc_ston();
ag_lock_x1=ag_ston_x2+2;
ag_lock_x2=ag_lock_x1+text_width;
make_afc_locky();
make_afc_lock();
ag_srch_x1=ag_lock_x2+2;
ag_srch_x2=ag_srch_x1+text_width;
make_afc_searchy();
make_afc_search();
afct_delay_points=ag.delay;
afct_avgnum=0;
make_afct_window(ag.avgnum);
check_afct_points();
ag_old_fit_points = ag.fit_points;
ag_flag=1;
ag_pf1=0;
ag_pf2=0;
afc_curx=-1;
if(kill_all_flag)return;
if(ag.mode_control!=new_ag_mode_control)
  {
  ag.mode_control=new_ag_mode_control;
  if(ag.mode_control==1)ag.mode_control++;
  if(ag.mode_control > 2)
    {
    ag.mode_control=0;
    }
  sc[SC_BG_BUTTONS]++;
  baseb_reset_counter++;
  }
make_modepar_file(GRAPHTYPE_AG);
resume_thread(THREAD_SCREEN);
}

void init_afc_graph(void)
{
if(genparm[AFC_LOCK_RANGE] == 0)
  {
  ag.mode_control=0;
  return;
  }
if (read_modepar_file(GRAPHTYPE_AG) == 0)
  {
ag_default:;  
// Make the default window for the afc graph.
  ag.xright=screen_last_xpixel;
  ag.xleft=ag.xright-(int)(0.3F*(float)screen_width);
  ag.ybottom=(int)(0.9F*(float)screen_height);
  ag.ytop=ag.ybottom-screen_height/5;
  ag.mode_control=2;
  ag.avgnum=11;
  ag.fit_points=25;
  ag.delay=17;
  ag.window=1;
  ag.minston=1.5;
  ag.frange=(float)genparm[AFC_LOCK_RANGE]*0.2F;
  ag.lock_range=ag.frange/2;
  ag.search_range=1;
  ag.check=AG_VERNR;
  }
ag.window &= 1;  
if(ag.avgnum < 0 || ag.avgnum >999)ag.avgnum=5;   
if(ag.check != AG_VERNR)goto ag_default;
if(ag.frange < (float)AG_FRANGE_MIN)ag.frange = (float)AG_FRANGE_MIN;
if(ag.frange > (float)AG_FRANGE_MAX)ag.frange = (float)AG_FRANGE_MAX;
if(ag.mode_control < 0 || ag.mode_control > 2)ag.mode_control=2;
afc_graph_filled=0;
// Get lock range and frequency drift in points.
afcf_max_drift=1+(float)genparm[AFC_MAX_DRIFT]*fftx_points_per_hz/60;
afcf_search_range=(1+(float)genparm[AFC_LOCK_RANGE]*fftx_points_per_hz);
if(genparm[SECOND_FFT_ENABLE] == 0)
  {
  max_afc_fit=max_fft1n-2;
  afcf_max_drift*=fft1_blocktime;
  }
else
  {
  max_afc_fit=max_fft2n-2;
  afcf_max_drift*=fft2_blocktime;
  }
if(ag.mode_control==1)ag.mode_control=2;
new_ag_mode_control=ag.mode_control;
if(max_afc_fit < 1)max_afc_fit = 1;
if(max_afc_fit > 3*screen_width/4)max_afc_fit = 3*screen_width/4;
// We may collect spectrum +/- 5% outside lock range.
// or +/- 10 points, whichever largest.
max_afcf_points=(int)(1.1F*afcf_search_range);
if(max_afcf_points < (int)afcf_search_range+20)
  {
  max_afcf_points=(int)afcf_search_range+20;
  }
// When searching for drifting signals we produce averages
// by adding frequency shifted power spectra.
// In the first search we accept the frequency error in this
// process to be 1.5 points between first and last transform.
// The number of different averages is no of transforms times 
// maximum drift divided by 0.75 (positive or negative sign) 
// We always have data from at least afct_delay_points transforms.
// afcf_max_drift is the maximum number of points a signal may
// drift between two transforms.
ag_xpixels=-1;
ag_old_height=0;
ag_old_fit_points = 0;
afc_graph_scro=no_of_scro;
make_afc_graph(FALSE);
no_of_scro++;
if(no_of_scro >= MAX_SCRO)lirerr(89);
}
