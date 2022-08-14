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
#include "screendef.h"
#include "fft1def.h"
#include "options.h"
#include "llsqdef.h"
#include "fft3def.h"

#define MIN_DASHNO 8
#define CW_CORRELATE_GOODLEN 25
#define MIN_CORRELATE_SEP 70
#define MAX_CORRSUM 10
#define OVFL_PROTECT 0.00000000000000001
#define MORSE_CLOCK_COH_TIME 16


#define ZZ 0.000001
//   name     data    length
// CW_DASH    |---_|     4
// CW_DOT     |-_|       2 
// CW_SPACE   |__|       2
// CW_WORDSEP |____|     4 

void check_cw(int num,int type);
void first_morse_decode(void);
void continued_morse_decode(void);
void show_cw(char *caller);

// *********************************************************
// *********************************************************
// These defines allow a lot of information to be written to dmp
// define DUMPFILE 1 in main.c to use this.
#if DUMPFILE == TRUE
#define PR00 0
#define PR01 0
#define PR02 1
#define PR03 0
#define PR04 0
#define PR05 0
#define PR06 0
#define PR07 0
#define PR08 0
#define PR09 0
#define PR10 0
#else
#define PR00 0
#define PR01 0
#define PR02 0
#define PR03 0
#define PR04 0
#define PR05 0
#define PR06 0
#define PR07 0
#define PR08 0
#define PR09 0
#define PR10 0
#endif


#define PRT00 if(PR00 != 0)DEB
#define PRT01 if(PR01 != 0)DEB
#define PRT02 if(PR02 != 0)DEB
#define PRT03 if(PR03 != 0)DEB
#define PRT04 if(PR04 != 0)DEB
#define PRT05 if(PR05 != 0)DEB
#define PRT06 if(PR06 != 0)DEB
#define PRT07 if(PR07 != 0)DEB
#define PRT08 if(PR08 != 0)DEB
#define PRT09 if(PR09 != 0)DEB
#define PRT10 if(PR10 != 0)DEB
// *********************************************************
// *********************************************************

// float[2] baseb_out=data to send to loudspeaker
// float[2] baseb=complex amplitude of first level coherent data.
// float[2] baseb_raw=complex amplitude of baseband signal. Raw data, pol. adapted.
// float[2] baseb_raw_orthog=complex amplitude of pol. orthog signal. Raw data.
// float[2] baseb_carrier=phase of carrier. Complex data, cosines and sines.
// float[1] baseb_carrier_ampl=amplitude of carrier
// float[1] baseb_totpwr=total power of baseb_raw
// float[2] baseb_envelope=complex amplitude from fitted dots and dashes.
// float[1] baseb_upthreshold=forward peak detector for power
// float[1] baseb_threshold=combined forward and backward peak detector at -6dB
// float[2] baseb_fit=fitted dots and dashes.
// float[2] baseb_tmp=array for intermediate data in complex format
// float[2] baseb_sho1=array for showing intermediate data in complex format
// float[2] baseb_sho2=array for showing intermediate data in complex format
// float[1] baseb_agc_level=used only when AGC is enabled.
// short int[1] baseb_ramp=indicator for time of power above/below threshold.
// short_int[1] baseb_clock=CW code clock
// float[2] baseb_tmp=for debug during development

// baseb_pa  The starting point of the latest mix2.size/2 points.
// baseb_pb  The point up to which thresholds exist.
// baseb_pc  The point up to which cw speed statistics and ramp is collected.
// baseb_pd  A key up region close before baseb_pc
// baseb_pe  The point up to which we have run first detect. 
// baseb_pf
// baseb_px  The oldest point that contains valid data.

int best_corr_ia;
int best_corr_ib;
float best_corr_ampl;
int corr_len;
int no_of_corr;
int msig_ptr;
float msig_signal;
float msig_noise;

#define MAX_GUESS_DATA 100
typedef struct {
float pos;
float re;
float im;
float noise;
int wei;
} GUESS_DATA;

GUESS_DATA msig[MAX_GUESS_DATA];

typedef struct {
int pos;
int sep;
float fpos;
float val;
} CORR_INFO;

CORR_INFO corr[MAX_CORRSUM];
CORR_INFO tmpcorr;

void show_msig(int ic)
{
int k;
for(k=0; k<ic; k++)
  {
  fprintf( dmp,"\nmsig[%d] [%.1f]=%f, (%f,%f) W=%d",k,msig[k].pos,
    msig[k].pos/cwbit_pts,ZZ*msig[k].re,ZZ*msig[k].im,msig[k].wei);
   }
}


void fit_msig(void)
{
int i, k, m;
float t1, r1, re, im, midpoint;
// The structure msig[0] to msig[msig_ptr-1] contains complex amplitudes 
// that are hopefully reasonably well synchronized to the Morse symbol clock.
// msig[].wei is > 0 for those points that the current guess assumes
// the key is pressed. Fit a slowly varying complex amplitude to
// all points with wei > 0.

// The time interval contained in msig is typically 30 ticks of the
// morse symbol clock.
// Over a short time like this, the complex amplitude of the
// carrier can not change very much so we should only fit a
// small number of parameters to describe it.
t1=msig[msig_ptr-1].pos-msig[0].pos;
if(t1<0)t1+=baseband_size;
t1/=2;
midpoint=msig[0].pos+t1;
if(midpoint >= baseband_size)midpoint+=baseband_size;
t1=sqrt(t1);
llsq_neq=0;
for(i=0; i<msig_ptr; i++)
  {
  if(msig[i].wei > 0)llsq_neq++;
  }
llsq_errors=fftn_tmp;
llsq_derivatives=&llsq_errors[2*llsq_neq];
k=0;
for(i=0; i<msig_ptr; i++)
  {
  if(msig[i].wei > 0)
    {
    llsq_errors[2*k  ]=msig[i].re*msig[i].wei;
    llsq_errors[2*k+1]=msig[i].im*msig[i].wei;
    llsq_derivatives[k]=t1*msig[i].wei;
    r1=msig[i].pos-midpoint;
    if(r1<-(baseband_size>>1))r1+=baseband_size;
    llsq_derivatives[llsq_neq+k]=r1*msig[i].wei;
    llsq_derivatives[2*llsq_neq+k]=r1*r1*msig[i].wei;   
    llsq_derivatives[3*llsq_neq+k]=r1*r1*r1*msig[i].wei/t1;
    k++;
    }
  }
if(3*llsq_neq+k > 2*fftn_tmp_size)
   {
   lirerr(93657);
   return;
   }
llsq_npar=4;  
if(llsq2() != 0)
  {
  lirerr(93610);
  return;
  }
// compute signal and noise  

msig_noise=0;
msig_signal=0;
k=0;
m=0;
for(i=0; i<msig_ptr; i++)
  {
  if(msig[i].wei > 0)
    {
    llsq_errors[2*k  ]=msig[i].re*msig[i].wei;
    llsq_errors[2*k+1]=msig[i].im*msig[i].wei;
    llsq_derivatives[k]=t1*msig[i].wei;
    r1=msig[i].pos-midpoint;
    if(r1<-(baseband_size>>1))r1+=baseband_size;
    re=t1*llsq_steps[0]+r1*llsq_steps[2]+r1*r1*llsq_steps[4]+r1*r1*r1*llsq_steps[6]/t1;
    im=t1*llsq_steps[1]+r1*llsq_steps[3]+r1*r1*llsq_steps[5]+r1*r1*r1*llsq_steps[7]/t1;
    msig_signal+=msig[i].wei*(re*re+im*im);
    re-=msig[i].re;
    im-=msig[i].im;
// Dashes integrate 3 times longer so their noise is sqrt(3) smaller.
// Take that into account.
// A region with noise only would give Ndots=n1*n1+n2*n2+n3*n3 when 
// evaluated as a dot (assume n1=n2=n3 on the average) so the
// noise contribution from a false dash would be Ndots=3*n*n.
// When evaluated as a dash, Ndash=3*n4*n4, because we have three parts
// with the same complex amplitude n4. n4 is n/sqrt(3) on the average
// because of the longer integration time so when evaluating as a
// dash we would get 3 times less noise when there is no signal.
// Therefore, multiply the contribution to msig_noise by 9 for a dash.
    re*=msig[i].wei;
    im*=msig[i].wei;
    msig_noise+=msig[i].wei*(re*re+im*im);
    m+=msig[i].wei;
    k++;
    }
  else
    {
    if(msig[i].wei == 0)msig_noise+=msig[i].noise;
    }
  }  
msig_signal=sqrt(msig_signal/m);
msig_noise=sqrt(msig_noise/msig_ptr);
}


void store_msig(int k)
{
int j;
switch (cw[k].type)
  {
  case CW_DOT:
// CW_DOT     |-_|       2 
//             ^
  msig[msig_ptr].pos=cw[k].midpoint;
  msig[msig_ptr].wei=1;
  msig[msig_ptr].re=cw[k].raw_re;
  msig[msig_ptr].im=cw[k].raw_im;
  msig_ptr++;
  cg_wave_midpoint=cw[k].midpoint+cwbit_pts;
  if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
  msig[msig_ptr].pos=cg_wave_midpoint;
  wb_investigate_region(cg_wave_midpoint, 1);
  msig[msig_ptr].re=reg_dot_re[0];
  msig[msig_ptr].im=reg_dot_im[0];
  msig[msig_ptr].noise=reg_dot_re[0]*reg_dot_re[0]+reg_dot_im[0]*reg_dot_im[0];
  msig[msig_ptr].wei=0;
  msig_ptr++;
  break;

  case CW_SPACE:
// CW_SPACE   |__|       2
//             ^  
  cg_wave_midpoint=cw[k].midpoint;
  msig[msig_ptr].pos=cg_wave_midpoint;
  wb_investigate_region(cg_wave_midpoint, 1);
  msig[msig_ptr].re=reg_dot_re[0];
  msig[msig_ptr].im=reg_dot_im[0];
  msig[msig_ptr].noise=reg_dot_re[0]*reg_dot_re[0]+reg_dot_im[0]*reg_dot_im[0];
  msig[msig_ptr].wei=0;
  msig_ptr++;
  cg_wave_midpoint+=cwbit_pts;
  if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
  msig[msig_ptr].pos=cg_wave_midpoint;
  wb_investigate_region(cg_wave_midpoint, 1);
  msig[msig_ptr].re=reg_dot_re[0];
  msig[msig_ptr].im=reg_dot_im[0];
  msig[msig_ptr].noise=reg_dot_re[0]*reg_dot_re[0]+reg_dot_im[0]*reg_dot_im[0];
  msig[msig_ptr].wei=0;
  msig_ptr++;
  break;

  case CW_DASH:
// CW_DASH    |---_|     4
//              ^
  msig[msig_ptr].pos=cw[k].midpoint-cwbit_pts;
  msig[msig_ptr].wei=0;
  msig[msig_ptr].re=0;
  msig[msig_ptr].im=0;
  msig[msig_ptr].noise=0;
  msig_ptr++;
  msig[msig_ptr].pos=cw[k].midpoint;
  msig[msig_ptr].wei=3;
  msig[msig_ptr].re=cw[k].raw_re;
  msig[msig_ptr].im=cw[k].raw_im;
  msig_ptr++;
  msig[msig_ptr].pos=cw[k].midpoint+cwbit_pts;
  cg_wave_midpoint=msig[msig_ptr].pos;
  msig[msig_ptr].wei=0;
  msig[msig_ptr].re=0;
  msig[msig_ptr].im=0;
  msig[msig_ptr].noise=0;
  msig_ptr++;
  cg_wave_midpoint+=cwbit_pts;
  if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
  msig[msig_ptr].pos=cg_wave_midpoint;
  wb_investigate_region(cg_wave_midpoint, 1);
  msig[msig_ptr].re=reg_dot_re[0];
  msig[msig_ptr].im=reg_dot_im[0];
  msig[msig_ptr].noise=reg_dot_re[0]*reg_dot_re[0]+reg_dot_im[0]*reg_dot_im[0];
  msig[msig_ptr].wei=0;
  msig_ptr++;
  break;

  case CW_WORDSEP:
// CW_WORDSEP |____|     4 
//              ^
  cg_wave_midpoint=cw[k].midpoint;
  wb_investigate_region(cg_wave_midpoint, 3);
  cg_wave_midpoint-=cwbit_pts;
  if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
  for(j=0; j<3; j++)
    {
    msig[msig_ptr].pos=cg_wave_midpoint;
    msig[msig_ptr].re=reg_dot_re[j];
    msig[msig_ptr].im=reg_dot_im[j];
    msig[msig_ptr].noise=reg_dot_re[j]*reg_dot_re[j]+reg_dot_im[j]*reg_dot_im[j];
    msig[msig_ptr].wei=0;
    msig_ptr++;
    cg_wave_midpoint+=cwbit_pts;
    if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
    }
  msig[msig_ptr].pos=cg_wave_midpoint;
  wb_investigate_region(cg_wave_midpoint, 1);
  msig[msig_ptr].re=reg_dot_re[0];
  msig[msig_ptr].im=reg_dot_im[0];
  msig[msig_ptr].noise=reg_dot_re[0]*reg_dot_re[0]+reg_dot_im[0]*reg_dot_im[0];
  msig[msig_ptr].wei=0;
  msig_ptr++;
  break;

  default:
  lirerr(874529);
  break;
  }
}

void fill_known_high_msig(void)
{
int k, dist;
// ptr points to a decoded Morse part in the CW array.
// Step downwards through decoded parts until the first
// undecoded is encountered or until the total distance is 
// 10 parts.
show_cw("----- fill high guesses  -----");
dist=0;
k=cw_ptr+1;
if(cw[k].unkn == 0)lirerr(8835263);
store_msig(k);
k++;
while(k<no_of_cwdat && cw[k].unkn == 0 && dist < 10)
  {
  dist+=cw[k].len;
  store_msig(k);
  k++;
  }
}

void fill_known_low_msig(void)
{
int k, dist;
// ptr points to a decoded Morse part in the CW array.
// Step downwards through decoded parts until the first
// undecoded is encountered or until the total distance is 
// 10 parts.
dist=0;
k=cw_ptr;
while(k>0 && cw[k].unkn == 0 && dist < 10)
  {
  dist+=cw[k].len;
  k--;
  }
// fill known info into msig.
while(k<cw_ptr || cw[k].unkn == 0)
  {
  store_msig(k);
  k++;
  }
}

char check_hguess3(int val, int bits, int charval_dwn_rev, int charbits_dwn)
{
int i, m, charbits, charval;
charbits=bits+charbits_dwn+2;
if(charbits > 6)return FALSE;
charval=val<<2;
m=charval_dwn_rev;
for(i=0; i<charbits_dwn; i++)
  {
  charval<<=1;
  charval+=m&1;
  m>>=1;
  }    
if(morsascii[charbits-1][charval] == 242 ||
   morsascii[charbits-1][charval] == 243)return FALSE;
return TRUE;
}

char check_hguess4(int val, int bits, int charval_dwn_rev, int charbits_dwn)
{
int i, m, charbits, charval;
charbits=bits+charbits_dwn+1;
if(charbits > 6)return FALSE;
charval=val<<1;
charval++;
m=charval_dwn_rev;
for(i=0; i<charbits_dwn; i++)
  {
  charval<<=1;
  charval+=m&1;
  m>>=1;
  }    
if(morsascii[charbits-1][charval] == 242 ||
   morsascii[charbits-1][charval] == 243)return FALSE;
return TRUE;
}

char check_hguess3a(int val, int bits, int charval_dwn_rev, int charbits_dwn)
{
int i, m, charbits, charval;
charbits=bits+charbits_dwn+1;
if(charbits > 6)return FALSE;
charval=val<<1;
m=charval_dwn_rev;
for(i=0; i<charbits_dwn; i++)
  {
  charval<<=1;
  charval+=m&1;
  m>>=1;
  }    
if(morsascii[charbits-1][charval] == 242 ||
   morsascii[charbits-1][charval] == 243)return FALSE;
return TRUE;
}

char check_hguess4a(int val, int bits, int charval_dwn_rev, int charbits_dwn)
{
int i, m, charbits, charval;
charbits=bits+charbits_dwn;
if(charbits > 6)return FALSE;
charval=val;
m=charval_dwn_rev;
for(i=0; i<charbits_dwn; i++)
  {
  charval<<=1;
  charval+=m&1;
  m>>=1;
  }    
if(morsascii[charbits-1][charval] == 242 ||
   morsascii[charbits-1][charval] == 243)return FALSE;
return TRUE;
}

int part_guesses(void)
{
// Look for long undecoded regions where a decision might be certain
// enough based on an extrapolation from the surrounding decoded
// regions.

return 0;
}
       
int character_guesses(void)
{
char lguess_flag[7],hguess_flag[5],guess_flag[7][5];
int charval_up, charbits_up;
int charval_dwn, charval_dwn_rev, charbits_dwn;
int added_parts;
int i, j, k, m, ia, ib, ic, gap, iptr;
int nlg, nhg, mlg, mhg;
int val, bits;
float r1, t1;
float dash_re[3], dash_im[3];
float dot_re[20], dot_im[20],dot_pwr[20];
// We have dashes and hopefully some dots and separators.
// Look for a space or a word separator and try to decode upwards.
show_cw("  char guesses");
cw_ptr=0;
added_parts=0;
region_guesses:;
show_cw("  char guesses");
while(cw_ptr < no_of_cwdat && 
      cw[cw_ptr].type != CW_WORDSEP && 
      cw[cw_ptr].type != CW_SPACE)cw_ptr++;
fprintf( dmp,"\nCHARGUESS cw_ptr=%d",cw_ptr);
if(cw_ptr < no_of_cwdat)
  {
// We have a separator here!
// Check for an undecoded region above it.
  iptr=cw_ptr+1;
  while(iptr < no_of_cwdat && cw[iptr].unkn == 0)
    {
    if( cw[iptr].type == CW_WORDSEP || cw[iptr].type == CW_SPACE)cw_ptr=iptr;
    iptr++;
    }
fprintf( dmp,"\ncw_ptr %d   iptr %d",cw_ptr,iptr);    
  if(iptr < no_of_cwdat-1)
    {
    charval_up=0;
    charbits_up=0;
    k=cw_ptr;
    k++;
    while(k < iptr)
      {
      charbits_up++;
      charval_up<<=1;
      if(cw[k].type==CW_DASH)charval_up++;
      k++;
      }
    gap=cw[iptr].unkn;
fprintf( dmp,"\ngap=%d iptr %d",gap,iptr);
fprintf( dmp,"\nbits %d  val %d  %c", charbits_up,charval_up,
                morsascii[charbits_up-1][charval_up]);
    if(charbits_up > 6 ||
       morsascii[charbits_up-1][charval_up] == 243 ||
       gap >= 16 ||
       gap <= 4 ||
       cw[iptr].type == CW_WORDSEP || 
       cw[iptr].type == CW_SPACE)
      {
      cw_ptr=iptr;
fprintf( dmp,"\nERROR below");      
// This is an error. Do nothing for now.
      goto region_guesses;
      }
// We here have a sequence like this:
//  known-_??????????????_-known
// gap is the length of the undecoded region and it has to be
// an even number. 
    msig_ptr=0;
// This is a small gap. Use the information from decoded dots and 
// dashes from both sides of the region.
    cw_ptr=iptr-1;
    fill_known_low_msig();
    ia=msig_ptr;
    msig_ptr+=gap;
    ib=msig_ptr;
    fill_known_high_msig();
    ic=msig_ptr;
    cg_wave_midpoint=msig[ib].pos-cwbit_pts;
    ib--;
    if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size; 
    wb_investigate_region(cg_wave_midpoint, 1);
    msig[ib].pos=cg_wave_midpoint;
    msig[ib].re=reg_dot_re[0];
    msig[ib].im=reg_dot_im[0];
    msig[ib].noise=reg_dot_re[0]*reg_dot_re[0]+reg_dot_im[0]*reg_dot_im[0];
    msig[ib].wei=0;
// We will fill in different guesses in the region ia to ib.
// First we fill in what is known above the undecoded region.
// note that we do not trust the symbol clock speed very much - 
// we only use it 4 positions into the unknown region from
// either side. The operator could use hand keying and
// the length of dots, dashes and separators could be
// deviating from the nominal values.
    t1=msig[ia-1].pos;
    for(k=ia; k<ib; k++)
      {
      t1+=cwbit_pts;
      if(t1>=baseband_size)t1-=baseband_size;
      msig[k].pos=t1;
      msig[k].re=0;
      msig[k].im=k;
      msig[k].wei=-1;
      }
    ib--;
    t1=cg_wave_midpoint;
    k=1+(ia+ib)/2;
    t1-=(ib-k+2)*cwbit_pts;
    if(t1<0)t1+=baseband_size;
    while(k<=ib)
      {
      t1+=cwbit_pts;
      if(t1>=baseband_size)t1-=baseband_size;
      msig[k].pos=t1;
      k++;
      }
// The Morse code goes in units of 2 or 4.
// The region has key-down at both ends so we know that the first
// pand the last points in the gap region must be key-up.
// Here are all possible guesses for the beginning:
//  ****-_____?????_-****   lguess 0
//  ****-___-_?????_-****   lguess 1
//  ****-_-___?????_-****   lguess 2
//  ****-_-_-_?????_-****   lguess 3
//  ****-_---_?????_-****   lguess 4
//  ****-___--?????_-****   lguess 5
//  ****-_-_--?????_-****   lguess 6
//      ia^
//        0123
//
// Here are all possible guesses for the end:
//  ****-_?????____-****   hguess 0
//  ****-_?????-___-****   hguess 1
//  ****-_?????__-_-****   hguess 2
//  ****-_?????-_-_-****   hguess 3
//  ****-_?????---_-****   hguess 4
//             ib^
//             210
//
// In all we have 35 guesses, but many of them are likely
// to be impossible since the decoded data would not fit
// the Morse alphabet.
    for(nlg=0; nlg<7; nlg++)lguess_flag[nlg]=TRUE;
    for(nhg=0; nhg<5; nhg++)hguess_flag[nhg]=TRUE;
// Fill in acceptable guesses in lguess_flag.
    if(charbits_up == 6)
      {
// This is the maximum length for a Morse character.
// Skip if not legal, force key-up otherwise.
      if(morsascii[charbits_up-1][charval_up] == 243)
        {
fprintf( dmp,"\nERROR above");      
        goto skip_guesses;
        }
      lguess_flag[2]=FALSE;
      lguess_flag[3]=FALSE;
      lguess_flag[4]=FALSE;
      lguess_flag[6]=FALSE;
      }
    else
      {
      if(charbits_up == 5)
        {
// Skip guesses that would add more than one part.        
        lguess_flag[3]=FALSE;
        lguess_flag[6]=FALSE;
// In case the character is not legal, but adding one part
// could make it legal, skip guesses that do not add one part.
        if(morsascii[charbits_up-1][charval_up] == 242)
          {
          lguess_flag[0]=FALSE;
          lguess_flag[1]=FALSE;
          lguess_flag[5]=FALSE;
          }
// Check whether adding a dot would give a legal character.
        if(morsascii[charbits_up][charval_up<<1] == 243)
          {
          lguess_flag[2]=FALSE;
          }
// Check whether adding a dash would give a legal character.
        if(morsascii[charbits_up][(charval_up<<1)+1] == 243)
          {
          lguess_flag[4]=FALSE;
          }
        }
      else
        {
        if(charbits_up == 4)
          {
          if(morsascii[charbits_up][charval_up<<1] == 243)
            {
            lguess_flag[2]=FALSE;
            }
          if(morsascii[charbits_up+1][charval_up<<2] == 243)
            {
            lguess_flag[3]=FALSE;
            }
          if(morsascii[charbits_up][(charval_up<<1)+1] == 243)
            {
            lguess_flag[4]=FALSE;
            }
          if(morsascii[charbits_up+1][(charval_up<<2)+1] == 243)
            {
            lguess_flag[6]=FALSE;
            }
          }
        }
      }
// Look for a separator upwards.
    k=iptr+1;
    while( k < no_of_cwdat &&
           cw[k].unkn==0 &&
           cw[k].type != CW_WORDSEP && 
           cw[k].type != CW_SPACE)k++;
    if( k == no_of_cwdat || cw[k].unkn != 0)goto skip_guesses;
// There is a known separator above the current region.
// Decode it downwards.
    charbits_dwn=0;
    charval_dwn_rev=0;
    while(k > iptr)
      {
      charval_dwn_rev<<=1;
      k--;
      if(cw[k].type==CW_DASH)charval_dwn_rev++;
      charbits_dwn++;
      }
    if(charbits_dwn > 6)goto skip_guesses;
    k=0;
    m=charval_dwn_rev;
    for(i=0; i<charbits_dwn; i++)
      {
      k<<=1;
      k+=m&1;
      m>>=1;
      }
    charval_dwn=k;
    if(charbits_dwn==6)
      {
      if(morsascii[charbits_dwn-1][charval_dwn] == 243)
        {
        goto skip_guesses;
        }
      hguess_flag[2]=FALSE;
      hguess_flag[3]=FALSE;
      hguess_flag[4]=FALSE;
      }
    else
      {
      if(charbits_dwn==5)    
        {
        hguess_flag[3]=FALSE;
        if(morsascii[charbits_dwn-1][charval_dwn] == 243)
          {
          hguess_flag[0]=FALSE;
          hguess_flag[1]=FALSE;
          }
        if(morsascii[charbits_dwn][charval_dwn] == 243)
          {
          hguess_flag[2]=FALSE;
          }
        if(morsascii[charbits_dwn][charval_dwn+32] == 243)
          {
          hguess_flag[4]=FALSE;
          }
        }
      else
        {
        if(charbits_dwn==4)
          {
          if(morsascii[charbits_dwn+1][charval_dwn] == 243)
            {
            hguess_flag[3]=FALSE;
            }
          }
        }
      }
    for(nlg=0;nlg<7;nlg++)
      {
      for(nhg=0;nhg<5;nhg++)
        {
        guess_flag[nlg][nhg]=lguess_flag[nlg]&hguess_flag[nhg];
        }
      }
    if(gap == 8)
      {
// The regions touch each other.
// We can not combine lguesses 5 and 6 with hguesses 0,2 or 4
      for(i=5; i<=6; i++)
        {
        for(j=0; j<=4; j+=2)
          {
          guess_flag[i][j]=FALSE;
          }
        }
// When we combine lguesses and hguesses that both contain no
// space or word separator, the entire region has to be a valid
// Morse code.
// We have to check l=3,4,6 combined with h=3,4
// **************************************************
// lguess=3
      val=charval_up<<2;
      bits=charbits_up+2;       
      guess_flag[3][3]=check_hguess3(val, bits, charval_dwn_rev, charbits_dwn);
      guess_flag[3][4]=check_hguess4(val, bits, charval_dwn_rev, charbits_dwn);
// lguess=4
      val=(charval_up<<1)+1;
      bits=charbits_up+1;       
      guess_flag[4][3]=check_hguess3(val, bits, charval_dwn_rev, charbits_dwn);
      guess_flag[4][4]=check_hguess4(val, bits, charval_dwn_rev, charbits_dwn);
// lguess=6
      val=(charval_up<<2)+1;
      bits=charbits_up+2;       
      guess_flag[6][3]=check_hguess3a(val, bits, charval_dwn_rev, charbits_dwn);
      }
    else
      {
      if(gap == 6)
        {
// Two positions overlap. Make all combinations that have
// different patterns in the overlapping region illegal.
        guess_flag[0][1]=FALSE;
        guess_flag[0][3]=FALSE;
        guess_flag[0][4]=FALSE;
        guess_flag[1][0]=FALSE;
        guess_flag[1][2]=FALSE;
        guess_flag[1][4]=FALSE;
        guess_flag[2][1]=FALSE;
        guess_flag[2][3]=FALSE;
        guess_flag[2][4]=FALSE;
        guess_flag[3][0]=FALSE;
        guess_flag[3][2]=FALSE;
        guess_flag[3][4]=FALSE;
        guess_flag[4][0]=FALSE;
        guess_flag[4][2]=FALSE;
        guess_flag[4][4]=FALSE;
        guess_flag[5][0]=FALSE;
        guess_flag[5][1]=FALSE;
        guess_flag[5][2]=FALSE;
        guess_flag[5][3]=FALSE;
        guess_flag[6][0]=FALSE;
        guess_flag[6][1]=FALSE;
        guess_flag[6][2]=FALSE;
        guess_flag[6][3]=FALSE;
// When we combine lguesses and hguesses without any space 
// or word separator, the entire region has to be a valid
// Morse code.
// We have to check lguess=3,4,6 when combined with hguess=2,3,4 
// **************************************************
// lguess=3: only hguess=3 is possible.
        val=charval_up<<1;
        bits=charbits_up+1;
        guess_flag[3][3]=check_hguess3(val, bits, charval_dwn_rev, charbits_dwn);
// lguess=4: only hguess=3 is possible.
        val=(charval_up<<1)+1;
        bits=charbits_up+1;
        guess_flag[4][3]=check_hguess3a(val, bits, charval_dwn_rev, charbits_dwn);
// lguess=6: only hguess=4 is possible.
        val=(charval_up<<2)+1;
        bits=charbits_up+2;
        guess_flag[6][4]=check_hguess4a(val, bits, charval_dwn_rev, charbits_dwn);
        }
      }  

fprintf( dmp,"\nib-ia %d, gap %d",ib-ia,gap);
fprintf( dmp,"\nia %d  ib %d",ia,ib);
for(k=1; k<msig_ptr; k++)
fprintf( dmp,"\n%d  %f  (%f,%f)  %d  %f",k,msig[k].pos,ZZ*msig[k].re,ZZ*msig[k].im,msig[k].wei,msig[k].pos-msig[k-1].pos);
for(k=0; k<7; k++)fprintf( dmp,"\nlguess_flag[%d]  %d",k,lguess_flag[k]);
for(k=0; k<5; k++)fprintf( dmp,"\nhguess_flag[%d]  %d",k,hguess_flag[k]);
fprintf( dmp,"\n");
for(i=0; i<7; i++)
  {
  fprintf( dmp,"\n");
  for(j=0; j<5; j++)fprintf( dmp,"[%d][%d] %d   ",i,j,guess_flag[i][j]);
  }
fprintf( dmp,"\n");
show_msig(ic);



// *****************************************************************
// Prepare to set up msig for all allowed guesses.
// Dashes may be placed at ia+1 and at ib-2.
    cg_wave_midpoint=msig[ia+1].pos;
    wb_investigate_region(cg_wave_midpoint, -5);
    dash_re[0]=reg_dash_re;
    dash_im[0]=reg_dash_im;
    cg_wave_midpoint=msig[ia+3].pos;
    wb_investigate_region(cg_wave_midpoint, -5);
    dash_re[1]=reg_dash_re;
    dash_im[1]=reg_dash_im;


    cg_wave_midpoint=msig[ib-2].pos;
    wb_investigate_region(cg_wave_midpoint, -5);
    dash_re[2]=reg_dash_re;
    dash_im[2]=reg_dash_im;
    for(k=ia; k<ib; k++)
      {
      cg_wave_midpoint=msig[k].pos;
      wb_investigate_region(cg_wave_midpoint, 1);
      dot_re[k-ia]=reg_dot_re[0];
      dot_im[k-ia]=reg_dot_im[0];
      dot_pwr[k-ia]=reg_dot_re[0]*reg_dot_re[0]+reg_dot_im[0]*reg_dot_im[0];
      }
// Fill in all allowed guesses in msig one by one.
// Fit a smooth function to the carrier at those points where there 
// is a key down according to the current guess.
// Compute the associated RMS error (noise) and average carrier
// amplitude (signal.) Remember S and N for each valid guess.
    r1=0;
    mlg=-1;
    mhg=-1;
    for(nlg=0; nlg<7; nlg++)
      {
      if(lguess_flag[nlg] == TRUE)
        {
        for(k=0; k<4; k++)
          {
          msig[ia+k].wei=0;
          msig[ia+k].re=dot_re[k];
          msig[ia+k].im=dot_im[k];
          msig[ia+k].noise=dot_pwr[k];
          }
        switch (nlg)
          {
          case 0:
//  ****-_____?????_-****   lguess 0
//      ia^
//        0123
          break;

          case 1:
//  ****-___-_?????_-****   lguess 1
//      ia^
//        0123
          msig[ia+2].wei=1;
          break;

          case 2:
//  ****-_-___?????_-****   lguess 2
//      ia^
//        0123
          msig[ia].wei=1;
          break;

          case 3:
//  ****-_-_-_?????_-****   lguess 3
//      ia^
//        0123
          msig[ia].wei=1;
          msig[ia+2].wei=1;
          break;

          case 4:
//  ****-_---_?????_-****   lguess 4
//      ia^
//        0123
          msig[ia  ].wei=0;
          msig[ia  ].re=0;
          msig[ia  ].im=0;
          msig[ia  ].noise=0;
          msig[ia+1].wei=3;
          msig[ia+1].re=dash_re[0];
          msig[ia+1].im=dash_im[0];
          msig[ia+1].noise=dash_re[0]*dash_re[0]+dash_im[0]*dash_im[0];
          msig[ia+2].wei=0;
          msig[ia+2].re=0;
          msig[ia+2].im=0;
          msig[ia+2].noise=0;
          break;

          case 5:
//  ****-___--?????_-****   lguess 5
//      ia^
//        0123
          msig[ia+2].wei=1;
          msig[ia+3].wei=1;
          break;

          case 6:
//  ****-_-_--?????_-****   lguess 6
//      ia^
//        0123
          msig[ia  ].wei=1;
          msig[ia+2].wei=1;
          msig[ia+3].wei=1;
          break;
          }
        for(nhg=0; nhg<5; nhg++)
          {
          if(guess_flag[nlg][nhg]==TRUE)
            {
            for(k=ib-ia-3; k<ib-ia; k++)
              {
              msig[ia+k].wei=0;
              msig[ia+k].re=dot_re[k];
              msig[ia+k].im=dot_im[k];
              msig[ia+k].noise=dot_pwr[k];
              }
            switch (nhg)
              {
              case 0:
//  ****-_?????____-****   hguess 0
//             ib^
//             210
              break;

              case 1:
//  ****-_?????-___-****   hguess 1
//             ib^
//             210
              msig[ib-2].wei=1;
              break;

              case 2:
//  ****-_?????__-_-****   hguess 2
//             ib^
//             210
              msig[ib].wei=1;
              break;

              case 3:
//  ****-_?????-_-_-****   hguess 3
//             ib^
//             210
              msig[ib].wei=1;
              msig[ib-2].wei=1;
              break;

              case 4:
//  ****-_?????---_-****   hguess 4
//             ib^
//             210
              msig[ib-2].wei=0;
              msig[ib-2].re=0;
              msig[ib-2].im=0;
              msig[ib-2].noise=0;
              msig[ib-1].wei=3;
              msig[ib-1].re=dash_re[2];
              msig[ib-1].im=dash_im[2];
              msig[ib-1].noise=dash_re[2]*dash_re[2]+dash_im[2]*dash_im[2];
              msig[ib].wei=0;
              msig[ib].re=0;
              msig[ib].im=0;
              msig[ib].noise=0;
              break;
              }

            fit_msig();
            t1=msig_signal/msig_noise;
            if(t1 > r1)
              {
              r1=t1;
              mhg=nhg;
              mlg=nlg;
              }
            }
          }
        }
      }  
// Implement the best guess.
    if(mlg < 0)goto skip_guesses;
    cw_ptr=iptr;
    
fprintf( dmp,"\n\n!!!!!!!!!!!! mlg %d  mhg %d !!!!!!!!!!!!!",mlg,mhg);
fprintf( dmp,"\nia %d  ib %d",ia,ib);
//  ****-_____?????_-****   lguess 0
//  ****-___-_?????_-****   lguess 1
//  ****-_-___?????_-****   lguess 2
//  ****-_-_-_?????_-****   lguess 3
//  ****-_---_?????_-****   lguess 4
//  ****-___--?????_-****   lguess 5
//  ****-_-_--?????_-****   lguess 6
//      ia^
//        0123
//
// Here are all possible guesses for the end:
//  ****-_?????____-****   hguess 0
//  ****-_?????-___-****   hguess 1
//  ****-_?????__-_-****   hguess 2
//  ****-_?????-_-_-****   hguess 3
//  ****-_?????---_-****   hguess 4
//             ib^
//             210

    switch (mlg)
      {
      case 0:
//  ****-_____?????_-****   lguess 0
//      ia^
//        0123
      cg_wave_raw_re=0;
      cg_wave_raw_im=0;
      cg_wave_midpoint=msig[ia+1].pos;
      insert_item(CW_WORDSEP);
      cw_ptr++;
      break;

      case 1:
//  ****-___-_?????_-****   lguess 1
//      ia^
//        0123
      cg_wave_raw_re=0;
      cg_wave_raw_im=0;
      cg_wave_midpoint=msig[ia].pos;
      insert_item(CW_SPACE);
      cw_ptr++;
      cg_wave_raw_re=dot_re[2];
      cg_wave_raw_im=dot_im[2];
      cg_wave_midpoint=msig[ia+2].pos;
      insert_item(CW_DOT);
      cw_ptr++;
      break;

      case 2:
//  ****-_-___?????_-****   lguess 2
//      ia^
//        0123
      cg_wave_raw_re=dot_re[0];
      cg_wave_raw_im=dot_im[0];
      cg_wave_midpoint=msig[ia].pos;
      insert_item(CW_DOT);
      cw_ptr++;
      cg_wave_raw_re=0;
      cg_wave_raw_im=0;
      cg_wave_midpoint=msig[ia+2].pos;
      insert_item(CW_SPACE);
      cw_ptr++;
      break;

          case 3:
//  ****-_-_-_?????_-****   lguess 3
//      ia^
//        0123
      cg_wave_raw_re=dot_re[0];
      cg_wave_raw_im=dot_im[0];
      cg_wave_midpoint=msig[ia].pos;
      insert_item(CW_DOT);
      cw_ptr++;
      cg_wave_raw_re=dot_re[2];
      cg_wave_raw_im=dot_im[2];
      cg_wave_midpoint=msig[ia+2].pos;
      insert_item(CW_DOT);
      cw_ptr++;
      break;

      case 4:
//  ****-_---_?????_-****   lguess 4
//      ia^
//        0123
      cg_wave_raw_re=dash_re[0];
      cg_wave_raw_im=dash_im[0];
      cg_wave_midpoint=msig[ia+1].pos;
      insert_item(CW_DASH);
      cw_ptr++;
      break;

      case 5:
//  ****-___--?????_-****   lguess 5
//      ia^
//        0123
      cg_wave_raw_re=0;
      cg_wave_raw_im=0;
      cg_wave_midpoint=msig[ia].pos;
      insert_item(CW_SPACE);
      cw_ptr++;
      if(gap==8)goto gap8_add_dash;
      break;

      case 6:
//  ****-_-_--?????_-****   lguess 6
//      ia^
//        0123
      cg_wave_raw_re=dot_re[0];
      cg_wave_raw_im=dot_im[0];
      cg_wave_midpoint=msig[ia].pos;
      insert_item(CW_DOT);
      cw_ptr++;
      if(gap==8)
        {
gap8_add_dash:;
        cg_wave_raw_re=dash_re[1];
        cg_wave_raw_im=dash_im[1];
        cg_wave_midpoint=msig[ia+3].pos;
        insert_item(CW_DOT);
        cw_ptr++;
        }    
      break;

      default:
      lirerr(634293);
      return 0;
      }
    switch (mhg)
      {
      case 0:
//  ****-_?????____-****   hguess 0
//             ib^
//             210
      cg_wave_raw_re=0;
      cg_wave_raw_im=0;
      if(gap==8)
        {
        cg_wave_midpoint=msig[ib-1].pos;
        insert_item(CW_WORDSEP);
        }
      else
        {
        cg_wave_midpoint=msig[ib].pos;
        insert_item(CW_SPACE);
        }
      cw_ptr++;
      break;

      case 1:
//  ****-_?????-___-****   hguess 1
//             ib^
//             210
      if(gap==8)
        {
        cg_wave_raw_re=dot_re[ib-ia-2];
        cg_wave_raw_im=dot_im[ib-ia-2];
        cg_wave_midpoint=msig[ib-2].pos;
        insert_item(CW_DOT);
        cw_ptr++;
        }
      cg_wave_raw_re=0;
      cg_wave_raw_im=0;
      cg_wave_midpoint=msig[ib].pos;
      insert_item(CW_SPACE);
      cw_ptr++;
      break;


      case 2:
//  ****-_?????__-_-****   hguess 2
//             ib^
//             210
      if(gap==8)
        {
        cg_wave_raw_re=0;
        cg_wave_raw_im=0;
        cg_wave_midpoint=msig[ib-2].pos;
        insert_item(CW_SPACE);
        cw_ptr++;
        }
      cg_wave_raw_re=dot_re[ib-ia];
      cg_wave_raw_im=dot_im[ib-ia];
      cg_wave_midpoint=msig[ib].pos;
      insert_item(CW_DOT);
      cw_ptr++;
      break;

      case 3:
//  ****-_?????-_-_-****   hguess 3
//             ib^
//             210
      if(gap==8)
        {
        cg_wave_raw_re=dot_re[ib-ia-2];
        cg_wave_raw_im=dot_im[ib-ia-2];
        cg_wave_midpoint=msig[ib-2].pos;
        insert_item(CW_DOT);
        cw_ptr++;
        }
      cg_wave_raw_re=dot_re[ib-ia];
      cg_wave_raw_im=dot_im[ib-ia];
      cg_wave_midpoint=msig[ib].pos;
      insert_item(CW_DOT);
      cw_ptr++;
      break;
      
      case 4:
//  ****-_?????---_-****   hguess 4
//             ib^
//             210
      cg_wave_raw_re=dash_re[2];
      cg_wave_raw_im=dash_im[2];
      cg_wave_midpoint=msig[ib-1].pos;
      insert_item(CW_DASH);
      cw_ptr++;
      break;

      default:
      lirerr(634294);
      return 0;
      }
    added_parts+=cw_ptr-iptr;     
    iptr=cw_ptr; 
 skip_guesses:;      
    cw_ptr=iptr;
    goto region_guesses;
    }
  }

cw_detect_flag=CWDETECT_DEBUG_STOP;
show_cw("  char guesses finished ");
return added_parts;
}





void make_baseb_sho1(void)
{
int i,k;
k=2*baseband_size;
for(i=0; i<k; i++)baseb_sho1[i]=baseb_tmp[i]*0.5;
}

void make_baseb_sho2(void)
{
int i,k;
k=2*baseband_size;
for(i=0; i<k; i++)baseb_sho2[i]=baseb_fit[i]*0.5;
}

void correlated_detect(void)
{
int pa;
int ia, ka;
char s[80];
// Check the detected dots and dashes against the average waveform.
ka=corr[0].pos;
pa=-1;
for(cw_ptr=0; cw_ptr<no_of_cwdat; cw_ptr++)
  {
  ia=((int)(cw[cw_ptr].midpoint+0.5)-ka+baseband_size)&baseband_mask;
  if(ia > baseband_sizhalf)goto outside;
  ia=(ia-corr_len+baseband_size)&baseband_mask;
  if(ia < baseband_sizhalf)goto outside;
  if(pa < 0)pa=cw_ptr;
  switch (cw[cw_ptr].type)
    {
    case CW_DASH:
    sprintf(s,"dash   ");
    break;
    
    case CW_DOT:
    sprintf(s,"dot    ");
    break;
    
    case CW_SPACE:
    sprintf(s,"space  ");
    break;
    
    case CW_WORDSEP:
    sprintf(s,"wordsep");
    break;
    
    default:
    sprintf(s,"{%3d} %c",cw[cw_ptr].type,cw[cw_ptr].type);
    break;
    }
  DEB"\n%s %5d [%8.3f]%8.3f %2d %3d   (%f  %f)",
                     s,cw_ptr,cw[cw_ptr].midpoint,cw[cw_ptr].sep,cw[cw_ptr].len,cw[cw_ptr].unkn,
                    ZZ*cw[cw_ptr].coh_re,ZZ*cw[cw_ptr].coh_im);
outside:;
  }
}

int find_best_region(void)
{
int i, j, k, m;
int ia, ib;
float t1,da;
// Locate the best region by use of the amplitudes stored in cw[].tmp
j=0;
t1=0;
for(i=0; i<no_of_cwdat; i++)
  {  
  if(cw[i].tmp > t1)
    {
    j=i;
    t1=cw[i].tmp;
    }
  }
if(best_corr_ampl==0)
  {
  best_corr_ampl=t1;
  }
else
  {
  if(t1 < 0.33*best_corr_ampl)
    {
PRT10"\n-----------  exit -------------");
    return FALSE;
    }
  }    
// Decide what length to use for correlation.
ia=j;
ib=j;
k=no_of_cwdat-1;
t1*=.25;
m=0;
get_corrlen:;
da=cw[ib].midpoint-cw[ia].midpoint;
if(da < 0)da+=baseband_size;
if(da < MIN_CORRELATE_SEP*cwbit_pts)
  {
  if(ia > 0)
    {
    if(ib < k)
      {
      if(cw[ia-1].tmp > cw[ib+1].tmp)
        {
        goto iadec;
        }
      else  
        {
        goto ibinc;
        }
      }
    else
      {  
      goto iadec;
      }
    }
  else
    {
    if(ib < k)
      {
      goto ibinc;
      }
    }
iadec:;  
  if(cw[ia-1].tmp > t1)
    {
    ia--;
    m=-1;
    goto get_corrlen;
    }
  goto corrlen_x;
ibinc:;
  if(cw[ib+1].tmp > t1)
    {
    m=1;
    ib++;
    goto get_corrlen;
    }
  }  
corrlen_x:;
best_corr_ia=((int)(cw[ia].midpoint-3*cwbit_pts)+baseband_size)&baseband_mask;
best_corr_ib=((int)(cw[ib].midpoint+3*cwbit_pts))&baseband_mask;
i=(best_corr_ib-best_corr_ia+baseband_size)&baseband_mask;
i-=MIN_CORRELATE_SEP*cwbit_pts;
if(i > 0)
  {
  if(m>0)
    {
    ib--;
    best_corr_ib=(best_corr_ib-i+baseband_size)&baseband_mask;
    }
  else
    {
    ia++;
    best_corr_ia=(best_corr_ia+i)&baseband_mask;
    }  
  }
corr_len=(best_corr_ib-best_corr_ia+baseband_size)&baseband_mask;
// best_corr_ia and best_corr_ib are pointers in the baseband.
// Clear cw[].tmp for points within the region ia to ib so we
// will not try to correlate the same region again.  
ib++;
while(ia != ib)
  {
  cw[ia].tmp=0;
  ia++;
  }
return TRUE;
}

void compute_region_quality(void)
{
int i, ia, ib;
float amp, da, db, t1, trilen;
//
// Good regions contain several dashes and or dots within a time
// frame corresponding to a morse character.
// Store a triangle of width +/- CW_CORRELATE_GOODLEN norse code units
// around each part. 
// Make the height A or 3*A for dots and dashes respectively
// where A is the signal amplitude.
// Good regions are characterised by sucessful detection of the
// carrier so we use cw[].coh_re for the amplitude A.
trilen=CW_CORRELATE_GOODLEN*cwbit_pts;
for(i=0; i<no_of_cwdat; i++)
  {
  cw[i].tmp=0;
  }
for(i=0; i<no_of_cwdat; i++)
  {
  if(cw[i].type==CW_DOT)
    {
    amp=1;
    }
  else
    {
    if(cw[i].type==CW_DASH)
      {
      amp=3;
      }
    else
      {
      goto keyup;
      }
    }  
  amp*=cw[i].coh_re;
  cw[i].tmp+=amp;
  amp/=trilen;
  ia=i-1;
  ib=i+1;
  da=0.5*cw[i].len;
  db=da;
  while(ia >=0 && da<trilen)
    {
    da+=0.5*(cw[ia+1].len+cw[ia].len)+cw[ia+1].unkn;
    t1=trilen-da;
    if(t1>0)
      {
      cw[ia].tmp+=t1*amp;
      }
    ia--;  
    }
  while(ib < no_of_cwdat && db<trilen)
    {
    db+=0.5*(cw[ib].len+cw[ib-1].len)+cw[ib].unkn;
    t1=trilen-db;
    if(t1>0)
      {
      cw[ib].tmp+=t1*amp;
      }
    ib++;
    }
keyup:;
  }
}

void find_good_regions(void)
{
int i, j, k, m, ia, ib, ja, jb, corr_step, clrlen;
int first_corr, last_corr;
float t1, t2, maxval;
double dt2;
// Our data does not change very rapidly. The shortest signal
// duration of interest is cwbit_pts/2, the approximate time for 
// the separation between the 3 dB points of a Morse code dot.
// Search for correlations using a step size of cwbit_pts/2;
corr_step=0.5*(cwbit_pts+1);
ja=baseb_px/corr_step;
baseb_tmp[ja]=0;
ia=ja*corr_step;
corr[1].val=0;
corr[1].pos=-1;
getcorr_ia:;
ia+=corr_step;
ja++;
if(ia > baseband_mask)
  {
  ia=0;
  ja=0;
  }
k=(best_corr_ia-ia-corr_len+baseband_size)&baseband_mask;
if(k<baseband_neg)
  {
  dt2=0;
  j=best_corr_ia;
  k=ia;
  while(j != best_corr_ib)
    {
    dt2+=baseb[2*j]*baseb[2*k];
    j=(j+1)&baseband_mask;
    k=(k+1)&baseband_mask;
    }
  t2=dt2*OVFL_PROTECT;  
  if(corr[1].val < t2)
    {
    corr[1].pos=ja;
    corr[1].val=t2;    
    }  
  baseb_tmp[ja]=t2;
  goto getcorr_ia;
  }
jb=best_corr_ib/corr_step;  
m=baseband_mask/corr_step;
while(ja != jb)
  {
  baseb_tmp[ja]=0;
  ja++;
  if(ja > m)
    {
    ja=0;
    }
  }
ib=jb*corr_step;
getcorr_ib:;
k=(baseb_pc-ib-corr_len+baseband_size)&baseband_mask;
if(k<baseband_sizhalf)
  {
  dt2=0;
  j=best_corr_ia;
  k=ib;
  while(j != best_corr_ib)
    {
    dt2+=baseb[2*j]*baseb[2*k];
    j=(j+1)&baseband_mask;
    k=(k+1)&baseband_mask;
    }
  t2=dt2*OVFL_PROTECT;  
  if(corr[1].val < t2)
    {
    corr[1].pos=jb;
    corr[1].val=t2;    
    }  
  baseb_tmp[jb]=t2;
  ib+=corr_step;
  jb++;
  if(ib > baseband_mask)
    {
    ib=0;
    jb=0;
    }
  goto getcorr_ib;
  }  
baseb_tmp[jb]=0;
// If all the data is zero, return here.
if(corr[1].pos ==-1)return;
// Make sure that the very first point does not create a maximum
// from either end.
jb=jb-1;
if(jb<0)jb=m;
k=jb-1;
if(k<0)k=m;
while(baseb_tmp[k] !=0 && baseb_tmp[k] <= baseb_tmp[jb])
  {
  baseb_tmp[jb]=0;
  jb=k;
  k--;
  if(k<0)k=m;
  }  
last_corr=jb;
ja=baseb_px/corr_step;
ja++;
if(ja>0)ja=0;
k=ja+1;
if(k>m)k=0;
while(baseb_tmp[k] !=0 && baseb_tmp[k] <= baseb_tmp[ja])
  {
  baseb_tmp[ja]=0;
  ja=k;
  k++;
  if(k>m)k=0;
  }  
first_corr=ja;  
clrlen=1+corr_len/(4*corr_step);
// Clear baseb_tmp in the surroundings of the current maximum.
// Then search for the largest value in baseb_tmp.
// This way corr[].pos and corr_val[] will contain correlation
// maxima in descending order by corr_val.
no_of_corr=1;
while( no_of_corr < MAX_CORRSUM-1 && corr[no_of_corr].pos != -1)
  {
// First just clear 2*clrlen points.
  i=corr[no_of_corr].pos-clrlen;
  if(i<0)i=m;
  k=corr[no_of_corr].pos+clrlen;
  if(k>m)k=0;
  j=i;
  while(i != k)
    {
    baseb_tmp[i]=0;
    i++;
    if(i>m)i=0;
    }
// Clear points upwards to avoid that the first point becomes a maximum.
   i=k+1;
   if(i>m)i=0;
   while(baseb_tmp[i] !=0 && baseb_tmp[i] <= baseb_tmp[k])
    {
    baseb_tmp[k]=0;
    k=i;
    i++;
    if(i>m)i=0;
    }  
  j--;
  if(j<0)j=m;
  k=j-1;
  if(k<0)k=m;
  while(baseb_tmp[k] !=0 && baseb_tmp[k] <= baseb_tmp[j])
    {
    baseb_tmp[j]=0;
    j=k;
    k--;
    if(k<0)k=m;
    }  
  no_of_corr++;
  corr[no_of_corr].val=0;
  corr[no_of_corr].pos=-1;
  ia=first_corr;
  while(ia != last_corr)
    {
    if(baseb_tmp[ia] > corr[no_of_corr].val)
      {
      corr[no_of_corr].val=baseb_tmp[ia];
      corr[no_of_corr].pos=ia;
      }
    ia++;
    if(ia>m)ia=0;  
    }
  if(corr[no_of_corr].val < 0.5*corr[1].val)corr[no_of_corr].pos=-1;
  }
while(no_of_corr<MAX_CORRSUM-1)
  {
  no_of_corr++;
  corr[no_of_corr].pos=-1;
  corr[no_of_corr].val=0;
  }
if(PR10 != 0)
  {
  for(i=1; i<MAX_CORRSUM; i++)
    {
    DEB"\nNo:%d  pos:%d val %f",i,corr[i].pos*corr_step,corr[i].val);
    }
  }
// Now we know points where correlation has maxima,
// but we know the points within corr_step points only.
// Make a finer search so we get the optimum within a single point
// and include the complex part this time.
// Accumulate the summed baseband function in baseb_tmp.
i=best_corr_ia;
while(i != best_corr_ib)
  {
  baseb_tmp[2*i  ]=baseb[2*i  ];
  baseb_tmp[2*i+1]=baseb[2*i+1];
  i=(i+1)&baseband_mask;
  }
// Make the best region itself region 0
corr[0].pos=(best_corr_ia+(corr_step>>1))/corr_step;
no_of_corr=0;
corr[0].sep=0;
maxval=0;
accumulate_corr:;  
ia=((corr[no_of_corr].pos-1)*corr_step+baseband_mask)&baseband_mask;
ib=((corr[no_of_corr].pos+1)*corr_step+2)&baseband_mask;
corr[no_of_corr].val=0;
while(ia!=ib)
  {
  dt2=0;
  i=best_corr_ia;
  k=ia;
// Correlate to the summed function in baseb_tmp
  while(i != best_corr_ib)
    {
    dt2+=baseb_tmp[2*i]*baseb[2*k]+baseb_tmp[2*i+1]*baseb[2*k+1];
    i=(i+1)&baseband_mask;
    k=(k+1)&baseband_mask;
    }
  t2=dt2*OVFL_PROTECT;  
  if(corr[no_of_corr].val < t2)
    {
    corr[no_of_corr].pos=ia;
    corr[no_of_corr].val=t2;    
    }  
  ia=(ia+1)&baseband_mask;
  }
if(no_of_corr==0)
  {
PRT10"\nCorr no:%d  pos:%d val %f  maxval %f",
            no_of_corr,corr[no_of_corr].pos,corr[no_of_corr].val,maxval);
  goto noadd;
  }
if(no_of_corr==1)
  {
PRT10"\nCorr no:%d  pos:%d val %f  maxval %f",
            no_of_corr,corr[no_of_corr].pos,corr[no_of_corr].val,maxval);
  corr[1].sep=(corr[1].pos-best_corr_ia+baseband_size)&baseband_mask;
  if(corr[1].sep>(baseband_size>>1))corr[1].sep=
                               (baseband_size-corr[1].sep)&baseband_mask;
  
PRT10"  sep%d",corr[1].sep);
  }
  
// Check if this point has an integer relation to previously esteblished
// repeat separations.
if(no_of_corr > 1)
  {
  corr[no_of_corr].val*=corr[0].val/maxval;
PRT10"\nCorr no:%d  pos:%d val %f  maxval %f",
            no_of_corr,corr[no_of_corr].pos,corr[no_of_corr].val,maxval);
  corr[no_of_corr].sep=(corr[no_of_corr].pos-best_corr_ia
                                               +baseband_size)&baseband_mask;
  if(corr[no_of_corr].sep>(baseband_size>>1))corr[no_of_corr].sep=
                        (baseband_size-corr[no_of_corr].sep)&baseband_mask;
PRT10"  sep%d", corr[no_of_corr].sep);

  t1=(float)(corr[no_of_corr].sep)/corr[1].sep;
PRT10"\nt1=%f",t1);
  if(corr[no_of_corr].val < 0.4*corr[0].val)
    {
    PRT10"  skip for small corr_value.");    
    goto skip_corr;
    }
  if(!(fabs(t1-1.0) < 0.015 ||
       fabs(t1-2.0) < 0.02 ||
       fabs(t1-3.0) < 0.03 ||
       fabs(t1-0.5) < 0.01 ||     
       fabs(t1-1.5) < 0.015 ||
       fabs(t1-0.33333333) < 0.01 ||     
       fabs(t1-0.66666666) < 0.015 ||
       fabs(t1-1.33333333) < 0.015 ||     
       fabs(t1-1.66666666) < 0.015))
    {
PRT10"  skip for non-integer ratio.");    
skip_corr:;
    i=no_of_corr+1;
    while(i<MAX_CORRSUM)
      {
      corr[i-1].pos=corr[i].pos;
      corr[i-1].val=corr[i].val;
      i++;
      }
    corr[MAX_CORRSUM-1].pos=-1;
    corr[MAX_CORRSUM-1].val=0;
    goto next_corr;
    }
  }          
i=best_corr_ia;
k=corr[no_of_corr].pos;
while(i!=best_corr_ib)
  {
  baseb_tmp[2*i  ]+=baseb[2*k  ];
  baseb_tmp[2*i+1]+=baseb[2*k+1];
  i=(i+1)&baseband_mask;
  k=(k+1)&baseband_mask;
  }
noadd:;
maxval+=corr[no_of_corr].val;
no_of_corr++;
next_corr:;
if( corr[no_of_corr].pos != -1 && no_of_corr<MAX_CORRSUM) goto accumulate_corr;
PRT10"\nNO_OF_CORR %d",no_of_corr);
}



// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!
// !!                          WORK AREA
// !!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!



void correlate_undecoded_baseband(void)
{
int region, region_flag;
int i,j,k,ia,ib,ja,jb;
int ka,kb,k1,k2,k3;
int mix2_mask, sizhalf, carrfilter_pts;
float amp,t1,t2,t3,r1,r2;
double dt1, dt2, dt3;
sizhalf=mix2.size>>1;
mix2_mask=mix2.size-1;
// The standard procedure for detect has failed but there is 
// probably a signal present because we have decoded many dots 
// and dashes although not well enough to identify a single character.
// First find a few particularly good regions, then compute
// the auto-correlation function to see if the pattern repeats. 
compute_region_quality();
best_corr_ampl=0;
DEB"\n-----------  start correlate undecoded -------------");
goto zzb;
retry_correlation:;
DEB"\n----------   retry correlate undecoded -------------");
zzb:;
if(find_best_region() == FALSE)return;
find_good_regions();
if(no_of_corr==0)goto retry_correlation;
// The average waveform we now have in baseb_tmp would be correct
// and it would have S/N better by a factor of no_of_corr compared to an
// individual waveform if AND ONLY IF the phase of the carrier
// were correct and the signal level were constant.
//
// Real signals typically suffer from multipath propagation and
// as a result there is fading, QSB. When the amplitude goes
// through a minimum, the phase may twist rapidly and the
// carrier phase is very uncertain at such times. Whether a
// 180 degree phase shift goes over 90 or 270 degrees may depend
// on the noise only and the very weak signal close to the
// minimum might come out from our coherent detection procedure
// with the wrong phase. Actually with a random phase.
//
// Coherent averaging improves S/N for reasonably good signals,
// good enough to give good S/N for the carrier, but the price 
// to pay is that poor signals average out to nothing.
//
// Here we have some good coherently averaged signals in baseb_tmp.
// We will use them to improve the carrier phase.
//
// Compute the optimum position with decimals for each sequence
// from the correlation with the summed curve. Store in corr_val[]
j=0;
while(j<no_of_corr)
  {
  ia=corr[j].pos;
  dt1=0;
  dt2=0;
  dt3=0;
  i=best_corr_ia;
  k1=(ia+baseband_mask)&baseband_mask;
  k2=ia;
  k3=(ia+1)&baseband_mask;
  while(i != best_corr_ib)
    {
    dt1+=baseb_tmp[2*i]*baseb[2*k1]+baseb_tmp[2*i+1]*baseb[2*k1+1];
    dt2+=baseb_tmp[2*i]*baseb[2*k2]+baseb_tmp[2*i+1]*baseb[2*k2+1];
    dt3+=baseb_tmp[2*i]*baseb[2*k3]+baseb_tmp[2*i+1]*baseb[2*k3+1];
    k1=k2;
    k2=k3;
    i=(i+1)&baseband_mask;
    k3=(k3+1)&baseband_mask;
    }
  t1=dt1*OVFL_PROTECT;  
  t2=dt2*OVFL_PROTECT;  
  t3=dt3*OVFL_PROTECT;  
  if(t1 > t2 && t3 > t1)
    {
// If the middle point is a local minimum, something is very wrong.
// Then skip this waveform.
skipit:;
    i=j;
    while(i < no_of_corr-1) 
      {
      corr[i].pos=corr[i+1].pos;
      i++;
      }
    no_of_corr--;       
    goto shiftfind_x;
    }  
  if(t1 > t2)
    {      
    t3=t2;
    t2=t1;
    ia=corr[j].pos;
    ia=(ia+baseband_mask)&baseband_mask;
    corr[j].pos=ia;
    dt1=0;
    i=best_corr_ia;
    k1=(ia+baseband_mask)&baseband_mask;
    while(i != best_corr_ib)
      {
      dt1+=baseb_tmp[2*i]*baseb[2*k1]+baseb_tmp[2*i+1]*baseb[2*k1+1];
      i=(i+1)&baseband_mask;
      k1=(k1+1)&baseband_mask;
      }
    t1=dt1*OVFL_PROTECT;  
    if(t1>t2)goto skipit;
    }
  if(t3 > t2)
    {      
    t1=t2;
    t2=t3;
    ia=corr[j].pos;
    ia=(ia+1)&baseband_mask;
    corr[j].pos=ia;
    dt3=0;
    i=best_corr_ia;
    k3=(ia+1)&baseband_mask;
    while(i != best_corr_ib)
      {
      dt3+=baseb_tmp[2*i]*baseb[2*k3]+baseb_tmp[2*i+1]*baseb[2*k3+1];
      i=(i+1)&baseband_mask;
      k3=(k3+1)&baseband_mask;
      }
    t3=dt3*OVFL_PROTECT;  
    if(t3>t2)goto skipit;
    }
  parabolic_fit(&amp,&corr[j].fpos,t1*t1,t2*t2,t3*t3);
  corr[j].fpos+=ia;
  j++;
shiftfind_x:;
  }
if(no_of_corr < 2)goto retry_correlation;
// Shift our pos references for corr[0].pos to become equal to best_corr_ia.
// Then increase the separation between corr[].fpos and corr[].pos
// by a factor no_of_corr/(no_of_corr-1) to compensate to some extent
// for the fact that correlation contains a component of the
// signal to itself.
t1=corr[0].fpos-best_corr_ia;
t3=(float)(no_of_corr)/(no_of_corr-1);
for(i=0; i<no_of_corr; i++)
  {
  t2=corr[i].fpos-t1;
  if(t2 < 0)t2+=baseband_size;
  if(t2 > baseband_size)t2-=baseband_size;
  t2=corr[i].pos-t2;
  t2*=t3;
  corr[i].fpos=corr[i].pos-t2;
  if(corr[i].fpos < 0)corr[i].fpos+=baseband_size;
  if(corr[i].fpos > baseband_size)corr[i].fpos-=baseband_size;
  corr[i].pos=((int)(corr[i].fpos+0.5))&baseband_mask;
  corr[i].sep=(corr[i].pos-baseb_px+baseband_size)&baseband_mask;
  }
// Reorder the regions from ascending order of .val (quality)
// to .sep (now time = distance to baseb_px)
for(i=0; i<no_of_corr-1; i++)
  {
  for(j=i; j<no_of_corr; j++)
    {
    if(corr[i].sep > corr[j].sep)
      {
      tmpcorr=corr[i];
      corr[i]=corr[j];
      corr[j]=tmpcorr;
      }
    }
  }
// Make corr[].sep the distance between consecutive regions.
corr[0].sep=0;  
for(i=1; i<no_of_corr; i++)
  {
  corr[i].sep=corr[i].fpos-corr[i-1].fpos+0.5;
  }
// Find out what the real repeat separation might be.
k=baseband_size;
for(i=1; i<no_of_corr; i++)
  {
  if(k > corr[i].sep)
    {
    k=corr[i].sep;
    }
  }
// k is now the smallest repeat separation.
// Divide k by 2 or 3 (or 6) in case it is required to make 
// the longer separations multiples of k.
i=0;
while(i<no_of_corr)
  {
  t1=(float)(corr[i].sep)/k;
  if(fabs(t1-1.5) < 0.05||
     fabs(t1-2.5) < 0.05)
    {
    k/=2;
    i=no_of_corr;
    }
  else
    {
    i++;
    }  
  }
i=0;
while(i<no_of_corr)
  {
  t1=(float)(corr[i].sep)/k;
  if(fabs(t1-1.3333333333) < 0.05 ||
     fabs(t1-1.6666666666) < 0.05 ||
     fabs(t1-2.3333333333) < 0.05 )
    {
    k=(k+1)/3;
    i=no_of_corr;
    }
  else
    {
    i++;
    }  
  }
// Change k from being the repeat interval to become the max
// number of points by which we may increase corr_len at both sides, but
// do not allow corr_len to increase by more than a factor of three.
k=(k-corr_len)/2;
if(k>corr_len)k=corr_len;
// Shift the start pointers by k downwards.
// make sure we do not go below baseb_px.
ia=0; 
for(i=0; i<no_of_corr; i++)
  {
  corr[i].fpos-=k;
  if(corr[i].fpos < 0)corr[i].fpos+=baseband_size;
  j=corr[i].fpos-baseb_px+baseband_mask;
  j&=baseband_mask;
  if(j > baseband_sizhalf)
    {
    j=baseband_size-j;
    if(j>ia)ia=j;
    }
  }
for(i=0; i<no_of_corr; i++)
  {
  corr[i].fpos+=ia;
  if(corr[i].fpos >= baseband_size)corr[i].fpos-=baseband_size;
  corr[i].pos=corr[i].fpos+0.5;
  corr[i].pos&=baseband_mask;
  }
k=corr_len+2*k-ia;  
// Make sure the last point is not above baseb_pc
ib=0; 
for(i=0; i<no_of_corr; i++)
  {
  j=corr[i].fpos+k+1;
  j=(baseb_pc-j+baseband_size)&baseband_mask;
  if(j > baseband_sizhalf)
    {
    j=baseband_size-j;
    if(j>ib)ib=j;
    }
  }
corr_len=k-ib;
for(i=0; i<no_of_corr; i++)
  {
  corr[i].pos=((int)(corr[i].fpos+0.5))&baseband_mask;
  }

//  !!!!!!!!!!!!!!!!!!!!!  for debug purposes !!!!!!!!!!!!!!!
//  ö  
for(i=0;i<baseband_size/2;i+=2)
  {
  baseb_tmp[4*i  ]=-1/(ZZ*ZZ);
  baseb_tmp[4*i+1]=1/(ZZ*ZZ);
  baseb_tmp[4*i+2]=1;
  baseb_tmp[4*i+3]=1;
  baseb_tmp[4*i+4]=0;
  baseb_tmp[4*i+5]=0;
  baseb_tmp[4*i+6]=0;
  baseb_tmp[4*i+7]=0;
  baseb_fit[4*i  ]=-1/(ZZ*ZZ);
  baseb_fit[4*i+1]=1/(ZZ*ZZ);
  baseb_fit[4*i+2]=1;
  baseb_fit[4*i+3]=1;
  baseb_fit[4*i+4]=0;
  baseb_fit[4*i+5]=0;
  baseb_fit[4*i+6]=0;
  baseb_fit[4*i+7]=0;
  }

// Use the detect information from all regions to select the time
// intervals when the key is likely to have been pressed. Use the
// same pattern for all regions.
// store the gated raw data in baseb_fit.
for(region=0; region<no_of_corr; region++)
  {
  ia=corr[region].pos;
  ib=(ia+corr_len)&baseband_mask;
  while(ia!=ib)
    {
    baseb_fit[2*ia  ]=0;
    baseb_fit[2*ia+1]=0;
    ia=(ia+1)&baseband_mask;
    }
  }
region=0;
ia=corr[region].pos;
for(i=0; i<no_of_cwdat; i++)
  {
  if( cw[i].type == CW_DASH || cw[i].type == CW_DOT)
    {
gate_part:;  
    k=((int)(cw[i].midpoint)-ia+baseband_size)&baseband_mask;
    if(k<baseband_neg)
      {
      if(k>corr_len)
        {
        region++;
        if(region > no_of_corr)goto gate_x;
        ia=corr[region].pos;
        goto gate_part;
        }
      for(j=0; j<no_of_corr; j++)
        {
        if( cw[i].type == CW_DASH)
          {
          t1=1.5;
          }
        else
          {
          t1=0.5;
          }  
        ja=cw[i].midpoint-t1*cwbit_pts;
        jb=cw[i].midpoint+t1*cwbit_pts;
        ja=(ja+corr[j].pos-corr[region].pos+baseband_size)&baseband_mask;
        jb=(jb+corr[j].pos-corr[region].pos+baseband_size)&baseband_mask;
        while(ja != jb)
          {
          baseb_fit[2*ja  ]=baseb_wb_raw[2*ja  ];
          baseb_fit[2*ja+1]=baseb_wb_raw[2*ja+1];
          ja=(ja+1)&baseband_mask;
          }
        }
      }
    }
  }
gate_x:;
// We use a mix2.size fft to filter out the carrier in the 
// frequency domain with a filter of the same width as used
// before when extracting the carrier.
// In case the carrier clearly is narrower we use a narrower filter.
// We will use a sine squared window with 50 % overlap for the transforms.
carrfilter_pts=1+(bg_flatpoints+2*bg_curvpoints)/bg.coh_factor;
k3=carrfilter_pts/2+1;
for(region=0; region<no_of_corr; region++)
  {
  ka=corr[region].pos;
  kb=(ka+corr_len)&baseband_mask;
  for(i=0; i<2*sizhalf; i++)mix2_tmp[i]=0;
  ia=ka;
  ka=(ka-sizhalf+baseband_size)&baseband_mask;
  for(i=sizhalf; i<(int)mix2.size; i++)
    {
    mix2_tmp[2*i  ]=baseb_fit[2*ia  ]*cw_carrier_window[i];
    mix2_tmp[2*i+1]=baseb_fit[2*ia+1]*cw_carrier_window[i];
    ia=(ia+1)&baseband_mask;
    }
  region_flag=1;  
new_carrier:;    
  fftforward(mix2.size, mix2.n, mix2_tmp, 
                               mix2.table, mix2.permute, yieldflag_ndsp_mix2);
// Compute the power spectrum and locate the carrier.
// The carrier has to be close to frequency=0 so we search a limited
// range only (4 times the carrier filter).
  t1=0;
  t2=4;
  if(t2>0.5*bg.coh_factor)t2=0.5*bg.coh_factor;
  i=1+t2*carrfilter_pts;
  k=mix2.size-i-1;
  k2=i+1;
  k1=k;
  ia=2*k3-1;
  ib=(k2-k1-ia+mix2.size)&mix2_mask;
  while(i>=0)
    {
    t2=mix2_tmp[2*i  ]*mix2_tmp[2*i  ]+
       mix2_tmp[2*i+1]*mix2_tmp[2*i+1];
    mix2_pwr[i]=t2;
    if(t2 > t1)
      {
      j=i;
      t1=t2;
      }      
    t2=mix2_tmp[2*k  ]*mix2_tmp[2*k  ]+
       mix2_tmp[2*k+1]*mix2_tmp[2*k+1];
    mix2_pwr[k]=t2;
    if(t2 > t1)
      {
      j=k;
      t1=t2;
      }      
    i--;  
    k++;
    }
  i=j;
  if(i>sizhalf)i-=mix2.size;
  if(abs(i) > k3)goto fail;
  i=k1;
  r1=0;
  r2=0;
  while(i!=k2)
    {
    k=(i-j+mix2.size)&mix2_mask;
    if(k>sizhalf)k=mix2.size-k;
    if(k<k3)
      {
      r1+=mix2_pwr[i];
      }
    else
      {
      r2+=mix2_pwr[i];
      }
    i=(i+1)&mix2_mask;
    }
  r1/=ia;  
  r2/=ib;
  ia=(j-k3+mix2.size)&mix2_mask;
  ib=(j+k3)&mix2_mask;
  if( (r2 != 0 && t1/r2 < 4) || t1 == 0)
    {
fail:;
    t1=0;    
    ia=0;
    ib=0;
    mix2_tmp[0]=0.1;
    mix2_tmp[1]=0;
    }
  else
    {
// Our passband is from ia to ib. It is centered on the maximum power bin.
// Try to shift the passband up or down to see if we can get more signal.
    for(i=k3; i>0; i--)
      {
      t2=0;
      k1=(ia+mix2_mask)&mix2_mask;
      k2=ib;
      j=i;
      while(j!=0)
        {
        t2+=mix2_pwr[k1]-mix2_pwr[k2];
        k1=(k1+mix2_mask)&mix2_mask;
        k2=(k2+mix2_mask)&mix2_mask;
        j--;
        }
      t3=0;
      k1=ia;
      k2=(ib+1)&mix2_mask;
      j=i;
      while(j!=0)
        {
        t3+=mix2_pwr[k2]-mix2_pwr[k1];
        k1=(k1+1)&mix2_mask;
        k2=(k2+1)&mix2_mask;
        j--;
        }
      if(t2 > t3)
        {
        if(t2 > 0)
          {
          ia=(ia-i+mix2.size)&mix2_mask;
          ib=(ib-i+mix2.size)&mix2_mask;
          }
        }
      else  
        {
        if(t3 > 0)
          {
          ia=(ia+i)&mix2_mask;
          ib=(ib+i)&mix2_mask;
          }
        }
      }
// Now that we have the optimum position for the passband,
// see if there is a good reason to increase the bandwidth slightly.
// The noise floor is r2 and the max signal is t1.
// Look at both sides for a signal that is 9 dB or more above the noise.
// The signal must also be less than 15 dB below the main signal.
    t3=r2*4;
    if(t3 < t1/32)t3=t1/32;
    for(i=1; i<k3; i++)
      {
      k1=(ia-i+mix2.size)&mix2_mask;
      k2=(ib+i)&mix2_mask;
      if(mix2_pwr[k1]>t3)
        {
        ia=k1;
        }
      if(mix2_pwr[k2]>t3)
        {
        ib=k2;
        }
      }    
// Now, use a 3 dB lower threshold and see if we can make the
// filter narrower.
    t3/=2;
    while(mix2_pwr[ia] < t3)ia=(ia+1)&mix2_mask;
    while(mix2_pwr[ib] < t3)ib=(ib+mix2_mask)&mix2_mask;
    }
// Now that we decided to use the frequencies from ia to ib, clear all 
// other frequencies in our buffer.
  ib=(ib+1)&mix2_mask;
  while(ib != ia)
    {
    mix2_tmp[2*ib  ]=0;
    mix2_tmp[2*ib+1]=0;
    ib=(ib+1)&mix2_mask;
    }
// Go back to the time domain to get the carrier.
  fftback(mix2.size, mix2.n, mix2_tmp, mix2.table, mix2.permute, yieldflag_ndsp_mix2);
  if(region_flag < 3)
    {
    ia=(ka+sizhalf)&baseband_mask;
    for(i=sizhalf; i<(int)mix2.size; i++)
      {
      baseb_tmp[2*ia  ]=mix2_tmp[2*i  ];
      baseb_tmp[2*ia+1]=mix2_tmp[2*i+1];
      ia=(ia+1)&baseband_mask;
      }
    }    
  else
    {
    if(region_flag ==  3)
      {
      ia=(ka+sizhalf)&baseband_mask;
      i=sizhalf;
      while(i<(int)mix2.size && ia != kb)
        {
        baseb_tmp[2*ia  ]=mix2_tmp[2*i  ];
        baseb_tmp[2*ia+1]=mix2_tmp[2*i+1];
        ia=(ia+1)&baseband_mask;
        i++;
        }
      }    
    }
    
  if(region_flag == 2 || region_flag == 3)
    {
    ia=ka;
    for(i=0; i<sizhalf; i++)
      {
      baseb_tmp[2*ia  ]+=mix2_tmp[2*i  ];
      baseb_tmp[2*ia+1]+=mix2_tmp[2*i+1];
      ia=(ia+1)&baseband_mask;
      }
    }
  else
    {
    if(region_flag == 4)
      {
      ia=ka;
      i=0;
      while(i<sizhalf && ia != kb)
        {
        baseb_tmp[2*ia  ]+=mix2_tmp[2*i  ];
        baseb_tmp[2*ia+1]+=mix2_tmp[2*i+1];
        ia=(ia+1)&baseband_mask;
        i++;
        }
      }
    }
// Prepare for a new section of the current region.
  ka=(ka+sizhalf)&baseband_mask;
  if( ((kb-ka-(int)mix2.size+baseband_size)&baseband_mask)< baseband_sizhalf)
    {
    region_flag=2;
    }
  else
    {
    region_flag++;
    }
  if( ((kb-ka+baseband_size)&baseband_mask)< baseband_sizhalf)
    {
    ia=ka;
    i=0;
    while(i<(int)mix2.size && ia != kb)
      {
      mix2_tmp[2*i  ]=baseb_fit[2*ia  ]*cw_carrier_window[i];
      mix2_tmp[2*i+1]=baseb_fit[2*ia+1]*cw_carrier_window[i];
      ia=(ia+1)&baseband_mask;
      i++;
      }
    while(i<(int)mix2.size)
      {
      mix2_tmp[2*i  ]=0;
      mix2_tmp[2*i+1]=0;
      ia=(ia+1)&baseband_mask;
      i++;
      }
    goto new_carrier;
    }
  }
goto debug_x;

// Use the positons with decimals to form the summed baseband
// function over a larger time interval, the new corr_len.


// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
ia=corr[0].pos;
ib=(ia+corr_len)&baseband_mask;
while(ia!=ib)
  {
  baseb_tmp[2*ia  ]=0;
  baseb_tmp[2*ia+1]=0;
  ia=(ia+1)&baseband_mask;
  }
for(i=0; i<no_of_corr; i++)
  {
  ia=corr[0].pos;
  ka=corr[i].pos;
  kb=(ka+1)&baseband_mask;
  t1=corr[i].fpos-ka;
  t2=1-t1;
  while(ia!=ib)
    {
    baseb_tmp[2*ia  ]+=t2*baseb[2*ka  ]+t1*baseb[2*kb  ];
    baseb_tmp[2*ia+1]+=t2*baseb[2*ka+1]+t1*baseb[2*kb+1];
    ia=(ia+1)&baseband_mask;
    ka=kb;
    kb=(kb+1)&baseband_mask;
    }
  }  



goto debug_x; //ö
  
  



debug_x:;
make_baseb_sho1();  
make_baseb_sho2();  
if(no_of_corr > 2)
  {
  cw_detect_flag=CWDETECT_DEBUG_STOP;//öö
  XZ("STOP!!");
  }
return;//öööööööööööööööööööööööööööööööööööööööööööööööööööö




// Use the carrier now present in baseb_tmp[] to compute a new
// and hopefully more accurate coherently detected signal in baseb_fit[]
// Accumulate the average waveform in baseb_envelope[].

//  !!!!!!!!!!!!!!!!!!!!!  for debug purposes !!!!!!!!!!!!!!!
//  ö  ö  ö  
for(i=0;i<baseband_size/2;i+=2)
  {
  baseb_envelope[4*i  ]=-1/(ZZ*ZZ);
  baseb_envelope[4*i+1]=1/(ZZ*ZZ);
  baseb_envelope[4*i+2]=1;
  baseb_envelope[4*i+3]=1;
  baseb_envelope[4*i+4]=0;
  baseb_envelope[4*i+5]=0;
  baseb_envelope[4*i+6]=0;
  baseb_envelope[4*i+7]=0;
  }
// """"""""""""""""""""""""""""""""""""""""""""""""""""""""""""

ia=corr[0].pos;
ib=(ia+corr_len)&baseband_mask;
while(ia != ib)
  {
  baseb_envelope[2*ia  ]=0;     
  baseb_envelope[2*ia+1]=0;     
  ia=(ia+1)&baseband_mask;
  }
for(region=0; region<no_of_corr; region++)
  {
  ia=corr[0].pos;
  ka=corr[region].pos;

DEB"\nfinal:[%d] %d",region,ka); 

  kb=(ka+corr_len)&baseband_mask;
  while(ka != kb)
    {
    t1=sqrt(baseb_tmp[2*ka  ]*baseb_tmp[2*ka  ]+
            baseb_tmp[2*ka+1]*baseb_tmp[2*ka+1]);
    if(t1!=0)
      {   
      t2=baseb_tmp[2*ka  ]/t1;
      t3=baseb_tmp[2*ka+1]/t1;
      baseb_fit[2*ka  ]=t2*baseb_raw[2*ka  ]+t3*baseb_raw[2*ka+1];     
      baseb_fit[2*ka+1]=t2*baseb_raw[2*ka+1]-t3*baseb_raw[2*ka  ];     
      }
    else
      {
      baseb_fit[2*ka  ]=0;     
      baseb_fit[2*ka+1]=0;     
      }
    baseb_envelope[2*ia  ]+=0.3*baseb_fit[2*ka  ];
    baseb_envelope[2*ia+1]+=0.3*baseb_fit[2*ka+1];
    ka=(ka+1)&baseband_mask;
    ia=(ia+1)&baseband_mask;
    }
  }
correlated_detect();


show_cw("correlate end");






return;
goto retry_correlation;
// set cw_detect_flag >40 if sucessful
}

void show_cw(char *caller)
{
int i;
char s[80];
DEB"\n**** show_cw: (%s) *****",caller);
DEB"\n           N    [mid]      sep   l  unk  tmp           coh             raw");
for(i=0; i<no_of_cwdat; i++)
  {
  switch (cw[i].type)
    {
    case CW_DASH:
    sprintf(s,"dash   ");
    break;
    
    case CW_DOT:
    sprintf(s,"dot    ");
    break;
    
    case CW_SPACE:
    sprintf(s,"space  ");
    break;
    
    case CW_WORDSEP:
    sprintf(s,"wordsep");
    break;
    
    default:
    sprintf(s,"{%3d} %c",cw[i].type,cw[i].type);
    break;
    }
  DEB"\n%s %5d [%8.2f]%8.2f %2d %3d %.4f  (%7.1f,%7.1f)  (%7.1f,%7.1f)  ",
                     s,i,cw[i].midpoint,cw[i].sep,cw[i].len,cw[i].unkn,
                0.00001*ZZ*cw[i].tmp, ZZ*cw[i].coh_re,ZZ*cw[i].coh_im,
                          ZZ*cw[i].raw_re,ZZ*cw[i].raw_im);
  }
DEB"\n");
}  

void insert_item(int type)
{
int i, ja, jb, k, m;
float t1,t2,r1,r2;
check_cw(4001,1);
if(kill_all_flag)return;
r2=0;
if(cw_ptr==0)
  {
  ja=0;
  r1=cg_wave_midpoint-baseb_px;
  }
else
  {
  ja=cw[cw_ptr-1].len;
  r1=cg_wave_midpoint-cw[cw_ptr-1].midpoint;
  }
if(r1 < 0)r1+=baseband_size;
i=no_of_cwdat;
while(i > cw_ptr)
  {
  cw[i]=cw[i-1];
  i--;
  }
if(cw_ptr != no_of_cwdat)
  {
  r2=cw[cw_ptr].midpoint-cg_wave_midpoint;
  if(r2 < 0)r2+=baseband_size;
  cw[cw_ptr+1].sep=r2;
  jb=(cw[cw_ptr+1].len+cw[cw_ptr].len)/2;
  r2/=cwbit_pts;
  r2-=jb;
  if(r2 < 0)r2=0;
  k=(int)(r2)&0xfffffffe;
  if(r2-k > 1)k+=2;
  cw[cw_ptr+1].unkn=k;
  }
cw[cw_ptr].midpoint=cg_wave_midpoint;
cw[cw_ptr].type=type;
cw[cw_ptr].coh_re=cg_wave_coh_re;
cw[cw_ptr].coh_im=cg_wave_coh_im;
if(cg_wave_raw_re == 0 && cg_wave_raw_im == 0)
  {
  m=cg_wave_midpoint+0.5;
  t1=baseb_carrier[2*m  ];
  t2=baseb_carrier[2*m+1];
  cg_wave_raw_re=cg_wave_coh_re*t1+cg_wave_coh_im*t2;
  cg_wave_raw_im=cg_wave_coh_im*t1-cg_wave_coh_re*t2;
  }
cw[cw_ptr].raw_re=cg_wave_raw_re;
cw[cw_ptr].raw_im=cg_wave_raw_im;
cw[cw_ptr].len=cw_item_len[255-type];
ja=(ja+cw[cw_ptr].len)/2;
cw[cw_ptr].sep=r1;
r1/=cwbit_pts;
r1-=ja;
k=(int)(r1)&0xfffffffe;
if(r1-k > 1)k+=2;
cw[cw_ptr].unkn=k;
DEB"\ninsert[%f] type %s  len %d   sep[i] %.0f sep[i+1] %.0f",
         cg_wave_midpoint,cw_item_text[255-type],cw_item_len[255-type],r1,r2);
no_of_cwdat++;
if(no_of_cwdat > max_cwdat)lirerr(1189);
check_cw(40011,1);
}


void second_detect(void)
{
int k;
float r1;
// *****************************************************
// *****************************************************
// *****  Fit key up or key down patterns to short *****
// *****  regions that are surrounded by dashes    *****
// *****************************************************
// *****************************************************
// Look for the following cases:
//  -_-_          2
//  ---_-         3
//  -_---         3
//  -___-         4
//  ---___-       5
//  -___---       5
//  -_____-       7
//      
//    Code       Sep
//  ---_---       4
//  ---_-_---     6
//  ---___---     6
//  ---_-_-_---   8
//  ---_---_---   8
//  ---___-_---   8
//  ---_-___---   8
//  ---_____---   8
//  ---_---_?_---   10
//  ---_?_---_---   10
//  ---_---_????....  default
//  ....????_---_---  default

// Look for the following cases:
//    Code       Unkn
//  |__|          2
//  |-_|          2
//  |____|        4
//  |-___|        4
//  |__-_|        4
//  |-_-_|        4
//  |---_|        4
//  |______|      6
//  |-_____|      6
//  |__-___|      6
//show_cw("second_detect enter");


PRT02"\nsecond_detect: CWBIT PTS %f x",cwbit_pts);
// Store the most probable signal in each case.
//   name     data    length
// CW_DASH    |---_|     4
// CW_DOT     |-_|       2 
// CW_SPACE   |__|       2
// CW_WORDSEP |____|     4 
// The item starts (length/2 + 0.5) below cw[].midpoint because
// midpoint is the center of a key down period while the item
// midpoint is 0.5 above that because one space is always
// included in an item.   
  PRT02"\n0 PRT02(2)>     [%7.1f] ", cw[0].midpoint);
cw_ptr=1;

while(cw_ptr < no_of_cwdat && 
      cw[cw_ptr].type==CW_DASH && 
      cw[cw_ptr-1].type==CW_DASH)
  {
  r1=cw[cw_ptr].sep/cwbit_pts;
  if( (cw[cw_ptr].type==CW_DASH && cw[cw_ptr-1].type==CW_DASH)||
      (cw[cw_ptr].type==CW_DOT && cw[cw_ptr-1].type==CW_DOT) )
    {      
    k=(int)(r1)&0xfffffffe;
    if(r1-k > 1)k+=2;
    }
  else
    {
    if( (cw[cw_ptr].type==CW_DASH && cw[cw_ptr-1].type==CW_DOT)||
      (cw[cw_ptr].type==CW_DOT && cw[cw_ptr-1].type==CW_DASH) )
      {
      k=(int)(r1+1)&0xfffffffe;
      if(r1+1-k > 1)k+=2;
      k--;
      


goto skip; // !!!!!  REMOVE !!!!!
      
      
      }
    else
      {
      goto skip; 
      }
    }
  PRT02"\n%d PRT02(2)>k=%3d [%7.1f][%7.1f]",cw_ptr,k,cw[cw_ptr-1].midpoint,
                                                  cw[cw_ptr].midpoint);
  switch (k)
    {
    case 2:
    case 4:
// We have two close spaced dashes.
// There can be nothing in between.
    break;
    
    case 6:
//  ---_-_---     6
//  ---___---     6
if(kill_all_flag)return;
    break;

    case 8:
//  ---_-_-_---   8  ity=4
//  ---_---_---   8  ity=1
//  ---___-_---   8  ity=3
//  ---_-___---   8  ity=2
//  ---_____---   8  ity=5
/*
      cg_wave_midpoint=dashpos1+t2*dashpos1_err;
      if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
      if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
      ia=cg_wave_midpoint;
      cg_wave_coh_re=t3*dash1_re+t2*baseb_envelope[2*ia  ]; 
      cg_wave_coh_im=t3*dash1_im+t2*baseb_envelope[2*ia+1]; 
      PRT04"\nstore_dash %f",cg_wave_midpoint);
      insert_item(CW_DASH);
if(kill_all_flag)return;
      cw_ptr++;

      break;

      case 2:
// Store dot1 and space
//  ---_-___---   8  ity=2
//  ---___-_---   8  ity=3
//  ---_____---   8  ity=5
//  ---_-_-_---   8  ity=4
      t3=0;
      if(dot_fit1 > DOT_DETECT_LIMIT && fabs(dotpos1_err) < 0.25*cwbit_pts)
        {
        if(dot1_chk < 0.25)
          {
          t3=0.75;
          }
        else
          {
          t3=0.5;
          }
        }  
      t2=1-t3; 
      cg_wave_midpoint=dotpos1+t2*dotpos1_err;
      if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
      if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
      ia=cg_wave_midpoint;
      cg_wave_coh_re=t3*dot1_re+t2*baseb_envelope[2*ia  ]; 
      cg_wave_coh_im=t3*dot1_im+t2*baseb_envelope[2*ia+1]; 
      PRT04"\nstore dot %f",cg_wave_midpoint);
      insert_item(CW_DOT);
if(kill_all_flag)return;
      cw_ptr++;
      cg_wave_midpoint=dotpos2_nom;
      PRT04"\nstore_space %f",cg_wave_midpoint);
      insert_item(CW_SPACE);
if(kill_all_flag)return;
      cw_ptr++;
      break;

      case 3:
// Store space and dot2 
//  ---___-_---   8  ity=3
      cg_wave_midpoint=dotpos1_nom;
      PRT04"\nstore_space %f",cg_wave_midpoint);
      insert_item(CW_SPACE);
if(kill_all_flag)return;
      cw_ptr++;
      t3=0;
      if(dot_fit2 > DOT_DETECT_LIMIT && fabs(dotpos2_err) < 0.25*cwbit_pts)
        {
        if(dot2_chk < 0.25)
          {
          t3=0.75;
          }
        else
          {
          t3=0.5;
          }
        }  
      t2=1-t3; 
      cg_wave_midpoint=dotpos2+t2*dotpos2_err;
      if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
      if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
      ia=cg_wave_midpoint;
      cg_wave_coh_re=t3*dot2_re+t2*baseb_envelope[2*ia  ]; 
      cg_wave_coh_im=t3*dot2_im+t2*baseb_envelope[2*ia+1]; 
      PRT04"\nstore dot %f",cg_wave_midpoint);
      insert_item(CW_DOT);
if(kill_all_flag)return;
      cw_ptr++;
      break;

      case 4:
// Store dot1 and dot2 
//  ---_-_-_---   8  ity=4
      t3=0;
      if(dot_fit1 > DOT_DETECT_LIMIT && fabs(dotpos1_err) < 0.25*cwbit_pts)
        {
        if(dot1_chk < 0.25)
          {
          t3=0.75;
          }
        else
          {
          t3=0.5;
          }
        }  
      t2=1-t3; 
      cg_wave_midpoint=dotpos1+t2*dotpos1_err;
      if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
      if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
      ia=cg_wave_midpoint;
      cg_wave_coh_re=t3*dot1_re+t2*baseb_envelope[2*ia  ]; 
      cg_wave_coh_im=t3*dot1_im+t2*baseb_envelope[2*ia+1]; 
      PRT04"\nstore dot %f",cg_wave_midpoint);
      insert_item(CW_DOT);
if(kill_all_flag)return;
      cw_ptr++;
      t3=0;
      if(dot_fit2 > DOT_DETECT_LIMIT && fabs(dotpos2_err) < 0.25*cwbit_pts)
        {
        if(dot2_chk < 0.25)
          {
          t3=0.75;
          }
        else
          {
          t3=0.5;
          }
        }  
      t2=1-t3; 
      cg_wave_midpoint=dotpos2+t2*dotpos2_err;
      if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
      if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
      ia=cg_wave_midpoint;
      cg_wave_coh_re=t3*dot2_re+t2*baseb_envelope[2*ia  ]; 
      cg_wave_coh_im=t3*dot2_im+t2*baseb_envelope[2*ia+1]; 
      PRT04"\nstore dot %f",cg_wave_midpoint);
      insert_item(CW_DOT);
if(kill_all_flag)return;
      cw_ptr++;
      break;

      case 5:
// Store word separator
//  ---_____---   8  ity=5
      cg_wave_midpoint=dashpos1_nom;
      PRT04"\nstore word sep %f",cg_wave_midpoint);
      insert_item(CW_WORDSEP);
if(kill_all_flag)return;
      cw_ptr++;
      break;
      }
*/
    break;

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//  ööAug 2004
//
    default:
  /*
    if(k==10)
      {    
      set_region_envelope();
      }
    else
      {    
      set_long_region_envelope();
      }
// See if a dash will fit in the first position.
    dashpos1_nom=cw[cw_ptr-1].midpoint+4*cwbit_pts;
    if(dashpos1_nom >= baseband_size)dashpos1_nom-=baseband_size;
    dash1_chk=check_dash(dashpos1_nom);
    cg_wave_start=dashpos1_nom-(dash_pts>>1)+baseband_size;
    cg_wave_start&=baseband_mask;
    fit_dash();
    dash1_re=cg_wave_coh_re;
    dash1_im=cg_wave_coh_im;
    dashpos1=cg_wave_midpoint;
    dashpos1_err=dashpos1_nom-cg_wave_midpoint;
    if(dashpos1_err < -sizhalf)dashpos1_err+=baseband_size;
    dash_fit1=cg_wave_fit; 
    PRT05"\n{def.l(2)}CHK DASH %f fit= %f  (%f) [%f]",
                            dash1_chk,dash_fit1,dashpos1_nom,cg_wave_midpoint);
// See if a dot will fit in the first position
    dotpos1_nom=dashpos1_nom-cwbit_pts;
    if(dotpos1_nom < 0)dotpos1_nom+=baseband_size;
    dot1_chk=check_dot(dotpos1_nom);
    cg_wave_start=dotpos1_nom-(dot_pts>>1)+baseband_size;
    cg_wave_start&=baseband_mask;
    fit_dot();
    dot1_re=cg_wave_coh_re;
    dot1_im=cg_wave_coh_im;
    dotpos1=cg_wave_midpoint;
    dotpos1_err=dotpos1_nom-cg_wave_midpoint;
    if(dotpos1_err < -sizhalf)dotpos1_err+=baseband_size;
    dot_fit1=cg_wave_fit; 
    PRT05"\n{def.l(2)}CHK DOT1 %f fit=%f  (%f)  [%f]",
                          dot1_chk,dot_fit1,dotpos1_nom,cg_wave_midpoint);
    ity=0;
    if(  (dash_fit1 > DASH_DETECT_LIMIT && 
          fabs(dashpos1_err) < 0.7*cwbit_pts &&
          dash1_chk > 0)    ||
               (dash_fit1 > DASH_MISFIT_LIMIT && 
                fabs(dashpos1_err) < 0.5*cwbit_pts &&
                dash1_chk > 0.5)  ) 
      {
// A dash fits reasonably well in the lower position.
      ity=1;
// Check how well dots would fit at the beginning or end of the dash position.        
      if(fabs(dotpos1_err) < 0.7 * cwbit_pts && 
         dash_fit1 < dot_fit1+0.05 &&
         dot1_chk > 0)
        {
        PRT05"\ndot1.l good(2). Skip dash for now.");     
        ity=0;
        if( dot_fit1 > DOT_DETECT_LIMIT ||
               (dot_fit1 > DOT_MISFIT_LIMIT && 
                fabs(dotpos1_err) < 0.5*cwbit_pts &&
                dash_fit1 < dot_fit1 &&
                dot1_chk > 0.5)  ) 
          {
          goto store_dot1;
          }
        }
      else
        {
        dotpos2_nom=dashpos1_nom+cwbit_pts;
        if(dotpos2_nom >= baseband_size)dotpos2_nom-=baseband_size;
        dot2_chk=check_dot(dotpos2_nom);
        cg_wave_start=dotpos2_nom-(dot_pts>>1)+baseband_size;
        cg_wave_start&=baseband_mask;
        fit_dot();
        dotpos2=cg_wave_midpoint;
        dotpos2_err=dotpos2_nom-cg_wave_midpoint;
        if(dotpos2_err < -sizhalf)dotpos2_err+=baseband_size;
        dot_fit2=cg_wave_fit; 
        PRT05"\n{def.l}CHK DOT2 %f fit=%f  (%f)  [%f]",
                          dot2_chk,cg_wave_fit,dotpos2_nom,cg_wave_midpoint);
        if(fabs(dotpos2_err) < 0.5 * cwbit_pts && 
           dash_fit1 < dot_fit2+0.05 &&
           dot2_chk > 0.25)
          {
          PRT05"\ndot2.h good, skip dash for now.");     
          ity=0;
          }
        }
      }
    else
      {
// A dash did not fit in the first position. 
// See if a dot does.
    if(  (dot_fit1 > DOT_DETECT_LIMIT && 
          fabs(dotpos1_err) < 0.7*cwbit_pts &&
          dot1_chk > 0)    ||
               (dot_fit1 > DOT_MISFIT_LIMIT && 
                fabs(dotpos1_err) < 0.5*cwbit_pts &&
                dot1_chk > 0.5)  ) 
        {
        PRT05"\ndash1 bad, dot1 good.");     
store_dot1:;
        cg_wave_midpoint=dotpos1+0.5*dotpos1_err;
        if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
        if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
        ia=cg_wave_midpoint;
        cg_wave_coh_re=0.5*dot1_re+0.5*baseb_envelope[2*ia  ]; 
        cg_wave_coh_im=0.5*dot1_im+0.5*baseb_envelope[2*ia+1];     
        PRT05"\nstore dot1.l  %f",cg_wave_midpoint);
        insert_item(CW_DOT);
if(kill_all_flag)return;
        cw_ptr++;
        }
      }        
// See if a dash will fit in the upper position.
    dashpos2_nom=cw[cw_ptr].midpoint-4*cwbit_pts;
    if(dashpos2_nom < 0)dashpos2_nom+=baseband_size;
    dash2_chk=check_dash(dashpos2_nom);
    cg_wave_start=dashpos2_nom-(dash_pts>>1)+baseband_size;
    cg_wave_start&=baseband_mask;
    fit_dash();
    dash2_re=cg_wave_coh_re;
    dash2_im=cg_wave_coh_im;
    dashpos2=cg_wave_midpoint;
    dashpos2_err=dashpos2_nom-cg_wave_midpoint;
    if(dashpos2_err < -sizhalf)dashpos2_err+=baseband_size;
    dash_fit2=cg_wave_fit; 
    PRT05"\n{def.h(2)}CHK DASH %f fit= %f  (%f) [%f]",
                         dash2_chk,dash_fit2,dashpos2_nom,cg_wave_midpoint);
    dotpos4_nom=dashpos2_nom+cwbit_pts;
    if(dotpos4_nom >= baseband_size)dotpos4_nom-=baseband_size;
    dot4_chk=check_dot(dotpos4_nom);
    cg_wave_start=dotpos4_nom-(dot_pts>>1)+baseband_size;
    cg_wave_start&=baseband_mask;
    fit_dot();
    dot4_re=cg_wave_coh_re;
    dot4_im=cg_wave_coh_im;
    dotpos4=cg_wave_midpoint;
    dotpos4_err=dotpos4_nom-cg_wave_midpoint;
    if(dotpos4_err < -sizhalf)dotpos4_err+=baseband_size;
    dot_fit4=cg_wave_fit; 
    PRT05"\n{def.h(2)}CHK DOT4 %f fit=%f  (%f)  [%f]",
                          dot4_chk,dot_fit4,dotpos4_nom,cg_wave_midpoint);

    if(  (dash_fit2 > DASH_DETECT_LIMIT && 
          fabs(dashpos2_err) < 0.7*cwbit_pts &&
          dash2_chk > 0)  ||
                (dash_fit2 > DASH_MISFIT_LIMIT && 
                 fabs(dashpos2_err) < 0.5*cwbit_pts &&
                 dash2_chk > 0.5)  )
      {
// A dash fits reasonably well in the upper position.
      ity|=2;
// Check how well dots would fit at the beginning or end of the dash position.        
      if( fabs(dotpos4_err) < 0.7 * cwbit_pts && 
          dash_fit2 < dot_fit4+0.05 &&
          dot4_chk > 0)
        {
        PRT05"\ndot4.h good(2), skip dash for now.");     
        ity&=1;
        if( dot_fit4 > DOT_DETECT_LIMIT ||
               (dot_fit4 > DOT_MISFIT_LIMIT && 
                fabs(dotpos4_err) < 0.5*cwbit_pts &&
                dash_fit2 < dot_fit4 &&
                dot4_chk > 0.5)  ) 
          {
          goto store_dot4;
          }
        }
      else
        {
        dotpos3_nom=dashpos2_nom-cwbit_pts;
        if(dotpos3_nom < 0)dotpos3_nom+=baseband_size;
        dot3_chk=check_dot(dotpos3_nom);
        cg_wave_start=dotpos3_nom-(dot_pts>>1)+baseband_size;
        cg_wave_start&=baseband_mask;
        fit_dot();
        dotpos3=cg_wave_midpoint;
        dotpos3_err=dotpos3_nom-cg_wave_midpoint;
        if(dotpos3_err < -sizhalf)dotpos3_err+=baseband_size;
        dot_fit3=cg_wave_fit; 
        PRT05"\n{def.h}CHK DOT3 %f fit=%f  (%f)  [%f]",
                          dot3_chk,cg_wave_fit,dotpos3_nom,cg_wave_midpoint);
        if(fabs(dotpos3_err) < 0.5 * cwbit_pts && 
           dash_fit2 < dot_fit3+0.05 &&
           dot3_chk > 0.25)
          {
          PRT05"\ndot3.h good, skip dash for now.");     
          ity&=1;
          }
        }  
      if( (ity & 3) == 3 && k < 12)
        {         
// We can not have one dash at each side in a short region. 
// choose the best one.
        if( fabs(dash_fit1-dash_fit2) > 0.05)
          {
          if(dash_fit1 > dash_fit2)
            {
            ity=1;
            }
          else
            {
            ity=2;
            }
          }
        else
          {  
          t1=2*dash_fit1-dot_fit1-dot_fit2-2*dash_fit2+dot_fit3+dot_fit4;
          if(fabs(t1) < 0.05)
            {
            ity=0;
            }
          else
            {
            if(t1>0)ity=1; else ity=2;
            }
          }
        } 
      }
    else
      {
// A dash did not fit in the upper position. 
// See if a dot does.
    if(  (dot_fit4 > DOT_DETECT_LIMIT && 
          fabs(dotpos4_err) < 0.7*cwbit_pts &&
          dot4_chk > 0)    ||
               (dot_fit4 > DOT_MISFIT_LIMIT && 
                fabs(dotpos4_err) < 0.5*cwbit_pts &&
                dot4_chk > 0.5)  ) 
        {
        PRT05"\ndash2 bad, dot4 good.");     
store_dot4:;
        ity|=4;
        }
      }      
    if( (ity & 3) == 3 && k < 12)
      {         
// We can not have one dash at each side in a short region. 
// choose the best one.
      if( fabs(dash_fit1-dash_fit2) > 0.05)
        {
        if(dash_fit1 > dash_fit2)
          {
          ity=1;
          }
        else
          {
          ity=2;
          }
        }
      else
        {  
        t1=2*dash_fit1-dot_fit1-dot_fit2-2*dash_fit2+dot_fit3+dot_fit4;
        if(fabs(t1) < 0.05)
          {
          ity=0;
          }
        else
          {
          if(t1>0)ity=1; else ity=2;
          }
        }
      } 
    if( (ity&1) != 0)
      {
      cg_wave_midpoint=dashpos1+0.5*dashpos1_err;
      if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
      if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
      ia=cg_wave_midpoint;
      cg_wave_coh_re=0.5*dash1_re+0.5*baseb_envelope[2*ia  ]; 
      cg_wave_coh_im=0.5*dash1_im+0.5*baseb_envelope[2*ia+1];     
      PRT05"\nstore_dash.l  %f",cg_wave_midpoint);
      insert_item(CW_DASH);
if(kill_all_flag)return;
      cw_ptr++;
      }
    if( (ity&2) != 0)
      {
      cg_wave_midpoint=dashpos2+0.5*dashpos2_err;
      if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
      if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
      ia=cg_wave_midpoint;
      cg_wave_coh_re=0.5*dash2_re+0.5*baseb_envelope[2*ia  ]; 
      cg_wave_coh_im=0.5*dash2_im+0.5*baseb_envelope[2*ia+1];     
      PRT05"\nstore_dash.h  %f",cg_wave_midpoint);
      insert_item(CW_DASH);
if(kill_all_flag)return;
      cw_ptr++;
      }
    if( (ity&4) != 0)
      {
      cg_wave_midpoint=dotpos4+0.5*dotpos4_err;
      if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
      if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
      ia=cg_wave_midpoint;
      cg_wave_coh_re=0.5*dot4_re+0.5*baseb_envelope[2*ia  ]; 
      cg_wave_coh_im=0.5*dot4_im+0.5*baseb_envelope[2*ia+1];     
      PRT05"\nstore dot4.h(2)  %f",cg_wave_midpoint);
      insert_item(CW_DOT);
if(kill_all_flag)return;
      cw_ptr++;
      }
*/
    break;
    }
skip:;
  cw_ptr++;
  }
//show_cw("second_detect exit");
}


void get_old_dashes(void)
{
int i;
int ia, ib;
// Speed and wave forms are known.
// Look for new dashes, in the range cw[0].midpoint
// to baseb_px.
ia=baseb_px;
while(baseb_ramp[ia] < 0)ia=(ia+1)&baseband_mask;
while(baseb_ramp[ia] > 0)ia=(ia+1)&baseband_mask;
while(baseb_ramp[ia] < 0)ia=(ia+1)&baseband_mask;
ia=(ia+baseband_mask)&baseband_mask;
ib=cw[0].midpoint;
ib=(ib-abs(baseb_ramp[ib])+baseband_size)&baseband_mask;
if(baseb_ramp[ib] > 0)ib=(ib-baseb_ramp[ib]+baseband_size)&baseband_mask;
if(no_of_cwdat < 1)lirerr(876231);
i=((int)(cw[no_of_cwdat-1].midpoint)-ib+baseband_size)&baseband_mask;
if( i+((ib-ia+baseband_size)&baseband_mask) > baseband_neg)return;
find_good_dashes(ia, ib, 14);
check_cw(30035,1);
}

void get_recent_dashes(void)
{
int ia;
// Speed and wave forms are known.
// Look for new dashes, in the range cw[no_of_cwdat-1].midpoint
// to baseb_pc.
ia=cw[no_of_cwdat-1].midpoint;
while(baseb_ramp[ia] > 0)ia=(ia+1)&baseband_mask;
while(baseb_ramp[ia] < 0)ia=(ia+1)&baseband_mask;
ia=(ia+baseband_mask)&baseband_mask;
// ia now points to the last point of the first negative ramp
// after the most recent cw[].midpoint.
if( ((baseb_pc-ia+baseband_size)&baseband_mask) > baseband_neg)return;
find_good_dashes(ia, baseb_pc, 14);
check_cw(30030,1);
}



void first_detect(int first_item)
{
mailbox[0]=first_item;
cw_detect_flag=CWDETECT_DEBUG_STOP;
return;
lirerr(888922);
/*

int sizhalf;
int k, ia;
int ity, item;
float dotpos1_nom, dotpos2_nom, dotpos3_nom, dotpos4_nom;
float dashpos1_nom, dashpos2_nom;
float dot1_chk, dot2_chk, dot3_chk, dot4_chk, dash1_chk, dash2_chk;
float dashpos1, dashpos2, dotpos1, dotpos2, dotpos3, dotpos4;
float dashpos1_err, dashpos2_err;
float dotpos1_err, dotpos2_err, dotpos3_err, dotpos4_err;
float dash_fit1, dash_fit2, dot_fit1, dot_fit2, dot_fit3, dot_fit4;
float r1, t1, t2, t3;
float dash1_re, dash1_im, dash2_re, dash2_im;
float dot1_re, dot1_im, dot2_re, dot2_im;
// **************************************************************
// **************************************************************
// ****    This routine is called to initiate a recently     ****
// ***     collected data area for which dashes have been    ****
// **** fitted but for which nothing else has yet been done. ****
// **************************************************************
// **************************************************************
sizhalf=baseband_size>>1;
// Initiate with stupid values. This avoids compiler complaints.
dot1_chk=-BIGFLOAT;
dotpos1=BIGFLOAT;
dotpos1_err=BIGFLOAT;
dot1_re=BIGFLOAT;
dot1_im=BIGFLOAT;
dot_fit1=-BIGFLOAT;
dot2_chk=-BIGFLOAT;
dotpos2=BIGFLOAT;
dotpos2_err=BIGFLOAT;
dot2_re=BIGFLOAT;
dot2_im=BIGFLOAT; 
dot_fit2=-BIGFLOAT;
dotpos1_nom=BIGFLOAT;
dotpos2_nom=BIGFLOAT;
dash_fit2=-BIGFLOAT;
dot_fit4=-BIGFLOAT;
// *****************************************************
// *****************************************************
// *****  Fit key up or key down patterns to short *****
// *****  regions that are surrounded by dashes    *****
// *****************************************************
// *****************************************************
// Look for the following cases:
//    Code       Sep
//  ---_---       4
//  ---_-_---     6
//  ---___---     6
//  ---_-_-_---   8
//  ---_---_---   8
//  ---___-_---   8
//  ---_-___---   8
//  ---_____---   8
//  ---_---_?_---   10
//  ---_?_---_---   10
//  ---_---_????....  default
//  ....????_---_---  default
#if PR02 == 1
show_cw("first_detect enter");
PRT02"\nfirst_detect: CWBIT PTS %f",cwbit_pts);
#endif
// Store the most probable signal in each case.
//   name     data    length
// CW_DASH    |---_|     4
// CW_DOT     |-_|       2 
// CW_SPACE   |__|       2
// CW_WORDSEP |____|     4 
// The item starts (length/2 + 0.5) below cw[].midpoint because
// midpoint is the center of a key down period while the item
// midpoint is 0.5 above that because one space is always
// included in an item.   
cw_ptr=first_item;
while(cw_ptr < no_of_cwdat)
  {
  r1=cw[cw_ptr].sep/cwbit_pts;
  k=(int)(r1)&0xfffffffe;
  if(r1-k > 1)k+=2;
  PRT02"\n%d PRT02(1)> k=%d  %f  [%f][%f]",cw_ptr,k,
                          r1,cw[cw_ptr-1].midpoint,cw[cw_ptr].midpoint);
check_cw(467501,1);
if(kill_all_flag)return;

  switch (k)
    {
    case 2:
    case 4:
// We have two close spaced dashes.
// There can be nothing in between.
    break;
    
    case 6:
//  ---_-_---     6
//  ---___---     6
// Base the decision on the probability that there is a dot
// right between the two dashes.
    set_region_envelope();
    dotpos1_nom=cw[cw_ptr].midpoint-0.5*cw[cw_ptr].sep;
    if(dotpos1_nom < 0)dotpos1_nom+=baseband_size;
    dot1_chk=check_dot(dotpos1_nom);
    PRT03"\n{6}CHK DOT %f (%f)",dot1_chk,dotpos1_nom);
// If dot1_chk in negative, the fit is better for a space than for a dot.
// Be careful and check if a dot would fit even for dot1_chk a little below zero.
    if(dot1_chk > -0.25)
      {
      cg_wave_start=dotpos1_nom-(dot_pts>>1)+baseband_size;
      cg_wave_start&=baseband_mask;
      fit_dot();
      PRT03" [%f]", cg_wave_midpoint);
      dotpos1_err=dotpos1_nom-cg_wave_midpoint;
      if(dotpos1_err < -sizhalf)dotpos1_err+=baseband_size;
      PRT03"  poserr %f  fit %f",dotpos1_err/cwbit_pts,cg_wave_fit);
      t3=-1;
// Use t3 as an indicator. 
// t3 < 0 => store space.
// t3 > 0 => store dot using t3 as the weight on fit_dot data.      
      if(cg_wave_fit > DOT_DETECT_LIMIT && fabs(dotpos1_err) < 0.25*cwbit_pts)
        {
        if(dot1_chk < 0.25)
          {
          t3=0.75;
          }
        else
          {
          t3=0.5;
          }
        }  
      if(cg_wave_fit > DOT_MISFIT_LIMIT && 
                       fabs(dotpos1_err) < 0.5*cwbit_pts && dot1_chk > 0.5)t3=0.5;
      if(t3 >= 0)
        {
        t2=1-t3; 
        cg_wave_midpoint+=t2*dotpos1_err;
        if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
        if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
        ia=cg_wave_midpoint;
        cg_wave_coh_re=t3*cg_wave_coh_re+t2*baseb_envelope[2*ia  ]; 
        cg_wave_coh_im=t3*cg_wave_coh_im+t2*baseb_envelope[2*ia+1]; 
        clear_region();
        PRT03"\nstore dot %f",cg_wave_midpoint);
        item=CW_DOT;
        }
      else
        {
        if( dot1_chk > 0)
          {
          item=0;
          }
        else  
          {
          cg_wave_midpoint=dotpos1_nom;
          clear_region();
          PRT03"\nstore space %f",cg_wave_midpoint);
          item=CW_SPACE;
          }
        }
      }
    else
      {
      cg_wave_midpoint=dotpos1_nom;
      clear_region();
      PRT03"\nstore space %f",cg_wave_midpoint);
      item=CW_SPACE;
      }
    if(item!=0)insert_item(item);
if(kill_all_flag)return;
    break;

    case 8:
//  ---_-_-_---   8  ity=4
//  ---_---_---   8  ity=1
//  ---___-_---   8  ity=3
//  ---_-___---   8  ity=2
//  ---_____---   8  ity=5
    set_region_envelope();
    ity=0;
// See if a dash will fit well.
    dashpos1_nom=cw[cw_ptr].midpoint-0.5*cw[cw_ptr].sep;
    if(dashpos1_nom < 0)dashpos1_nom+=baseband_size;
    dash1_chk=check_dash(dashpos1_nom);
    cg_wave_start=dashpos1_nom-(dash_pts>>1)+baseband_size;
    cg_wave_start&=baseband_mask;
    fit_dash();
    dash1_re=cg_wave_coh_re;
    dash1_im=cg_wave_coh_im;
    dashpos1=cg_wave_midpoint;
    dashpos1_err=dashpos1_nom-cg_wave_midpoint;
    if(dashpos1_err < -sizhalf)dashpos1_err+=baseband_size;
    dash_fit1=cg_wave_fit; 
    if( dash_fit1 > DASH_DETECT_LIMIT && 
                   dash1_chk > 0.5 &&  fabs(dashpos1_err) < 0.5*cwbit_pts)
      {
      ity=1;
      PRT04"\n{8}GOOD DASH %f fit= %f  (%f) [%f]",
                        dash1_chk,cg_wave_fit,dashpos1_nom,cg_wave_midpoint);      
      goto store8;
      }
    PRT04"\n{8}CHK DASH %f fit= %f  (%f) [%f]",
                         dash1_chk,cg_wave_fit,dashpos1_nom,cg_wave_midpoint);
    dotpos1_nom=cw[cw_ptr].midpoint-0.625*cw[cw_ptr].sep;
    if(dotpos1_nom < 0)dotpos1_nom+=baseband_size;
    dot1_chk=check_dot(dotpos1_nom);
    dotpos2_nom=cw[cw_ptr].midpoint-0.375*cw[cw_ptr].sep;
    if(dotpos2_nom < 0)dotpos2_nom+=baseband_size;
    dot2_chk=check_dot(dotpos2_nom);
    if(dash1_chk < -0.25 && dot1_chk < -0.25 && dot2_chk < -0.25)
      {
      PRT04"\nALL chk NEG (< -0.25)");
      ity=5;
      goto store8;
      }
    cg_wave_start=dotpos1_nom-(dot_pts>>1)+baseband_size;
    cg_wave_start&=baseband_mask;
    fit_dot();
    dot1_re=cg_wave_coh_re;
    dot1_im=cg_wave_coh_im;
    dotpos1=cg_wave_midpoint;
    dotpos1_err=dotpos1_nom-cg_wave_midpoint;
    if(dotpos1_err < -sizhalf)dotpos1_err+=baseband_size;
    dot_fit1=cg_wave_fit; 
    PRT04"\n{8}CHK DOT1 %f fit=%f  (%f)  [%f]",
                          dot1_chk,cg_wave_fit,dotpos1_nom,cg_wave_midpoint);
    cg_wave_start=dotpos2_nom-(dot_pts>>1)+baseband_size;
    cg_wave_start&=baseband_mask;
    fit_dot();
    dot2_re=cg_wave_coh_re;
    dot2_im=cg_wave_coh_im;
    dotpos2=cg_wave_midpoint;
    dotpos2_err=dotpos2_nom-cg_wave_midpoint;
    if(dotpos2_err < -sizhalf)dotpos2_err+=baseband_size;
    dot_fit2=cg_wave_fit; 
    PRT04"\n{8}CHK DOT2 %f fit=%f  (%f)  [%f]",
                         dot2_chk,cg_wave_fit,dotpos2_nom,cg_wave_midpoint);
    if( dash_fit1 < DASH_MISFIT_LIMIT || 
             ( (dash1_chk < -0.25 && fabs(dashpos1_err) < 0.5*cwbit_pts) ||
                ( dash1_chk < 0.25 && fabs(dashpos1_err) < cwbit_pts ) ) ) 
      {
// The fit to a dash is very poor. 
// Set ity to 5 (word separator), then check whether dot or space will fit.     
      ity=5;
      PRT04"\nset ity=5");
      PRT04"\ndot1 fit %f    chk %f    poserr %f ",dot_fit1,dot1_chk,dotpos1_err);
      if( dot_fit1 > DOT_MISFIT_LIMIT &&
                      dot1_chk > 0.25 && fabs(dotpos1_err) < 0.5*cwbit_pts ) 
        {
        ity=2;
        PRT04"\nset ity=2");
        } 
      PRT04"\ndot2 fit %f    chk %f    poserr %f ",dot_fit2,dot2_chk,dotpos2_err);
      if( dot_fit2 > DOT_MISFIT_LIMIT &&
                      dot2_chk > 0.25 && fabs(dotpos2_err) < 0.5*cwbit_pts ) 
        {
        if(ity == 2)
          {
          ity=4;
          PRT04"\nset ity=4");
          }
        else
          {
          ity=3;
          PRT04"\nset ity=3");
          }
        }    
      goto store8;
      }
    PRT04"\nUnclear");
// The situation is unclear. Nothing fits well...........
    if(dash1_chk < 0 && dot1_chk < 0 && dot2_chk < 0)
      {
      ity=5;
      PRT04"\nset ity=5  (all neg)");
      goto store8;
      }
    t1=dot1_chk*dot2_chk;
    if( fabs(t1) > dash1_chk*fabs(dash1_chk) )
      {
// The fit to a dash is worse than dots and/or spaces.
      PRT04"\nset ity=5  (dash worse than dots)");
      ity=5;
      if( dot_fit1 > DOT_MISFIT_LIMIT &&
                         dot1_chk > 0 && fabs(dotpos1_err) < 0.7*cwbit_pts ) 
        {
        ity=2;
        PRT04"\nset ity=2");
        } 
      if( dot_fit2 > DOT_MISFIT_LIMIT &&
                            dot2_chk > 0 && fabs(dotpos2_err) < 0.7*cwbit_pts ) 
        {
        if(ity == 2)
          {
          ity=4;
          PRT04"\nset ity=4");
          }
        else
          {
          ity=3;
          PRT04"\nset ity=3");
          }
        }    
      goto store8;
      }
    if( dash_fit1 > DASH_MISFIT_LIMIT && 
                (dash1_chk > 0 || fabs(dashpos1_err) < cwbit_pts ) )
      {
      ity=1;
      PRT04"\nset ity=1");
      }
store8:;
    clear_region();
    PRT04"\nity=%d",ity);
    switch (ity)
      {
      case 1:
// Store a dash
//  ---_---_---   8  ity=1
      t3=0;
      if(dash_fit1 > DASH_DETECT_LIMIT && fabs(dashpos1_err) < 0.25*cwbit_pts)
        {
        if(dash1_chk < 0.25)
          {
          t3=0.75;
          }
        else
          {
          t3=0.5;
          }
        }  
      t2=1-t3; 
      cg_wave_midpoint=dashpos1+t2*dashpos1_err;
      if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
      if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
      ia=cg_wave_midpoint;
      cg_wave_coh_re=t3*dash1_re+t2*baseb_envelope[2*ia  ]; 
      cg_wave_coh_im=t3*dash1_im+t2*baseb_envelope[2*ia+1]; 
      PRT04"\nstore_dash %f",cg_wave_midpoint);
      insert_item(CW_DASH);
if(kill_all_flag)return;
      cw_ptr++;
      break;

      case 2:
// Store dot1 and space
//  ---_-___---   8  ity=2
//  ---___-_---   8  ity=3
//  ---_____---   8  ity=5
//  ---_-_-_---   8  ity=4
      t3=0;
      if(dot_fit1 > DOT_DETECT_LIMIT && fabs(dotpos1_err) < 0.25*cwbit_pts)
        {
        if(dot1_chk < 0.25)
          {
          t3=0.75;
          }
        else
          {
          t3=0.5;
          }
        }  
      t2=1-t3; 
      cg_wave_midpoint=dotpos1+t2*dotpos1_err;
      if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
      if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
      ia=cg_wave_midpoint;
      cg_wave_coh_re=t3*dot1_re+t2*baseb_envelope[2*ia  ]; 
      cg_wave_coh_im=t3*dot1_im+t2*baseb_envelope[2*ia+1]; 
      PRT04"\nstore dot %f",cg_wave_midpoint);
      insert_item(CW_DOT);
if(kill_all_flag)return;
      cw_ptr++;
      cg_wave_midpoint=dotpos2_nom;
      PRT04"\nstore_space %f",cg_wave_midpoint);
      insert_item(CW_SPACE);
if(kill_all_flag)return;
      cw_ptr++;
      break;

      case 3:
// Store space and dot2 
//  ---___-_---   8  ity=3
      cg_wave_midpoint=dotpos1_nom;
      PRT04"\nstore_space %f",cg_wave_midpoint);
      insert_item(CW_SPACE);
if(kill_all_flag)return;
      cw_ptr++;
      t3=0;
      if(dot_fit2 > DOT_DETECT_LIMIT && fabs(dotpos2_err) < 0.25*cwbit_pts)
        {
        if(dot2_chk < 0.25)
          {
          t3=0.75;
          }
        else
          {
          t3=0.5;
          }
        }  
      t2=1-t3; 
      cg_wave_midpoint=dotpos2+t2*dotpos2_err;
      if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
      if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
      ia=cg_wave_midpoint;
      cg_wave_coh_re=t3*dot2_re+t2*baseb_envelope[2*ia  ]; 
      cg_wave_coh_im=t3*dot2_im+t2*baseb_envelope[2*ia+1]; 
      PRT04"\nstore dot %f",cg_wave_midpoint);
      insert_item(CW_DOT);
if(kill_all_flag)return;
      cw_ptr++;
      break;

      case 4:
// Store dot1 and dot2 
//  ---_-_-_---   8  ity=4
      t3=0;
      if(dot_fit1 > DOT_DETECT_LIMIT && fabs(dotpos1_err) < 0.25*cwbit_pts)
        {
        if(dot1_chk < 0.25)
          {
          t3=0.75;
          }
        else
          {
          t3=0.5;
          }
        }  
      t2=1-t3; 
      cg_wave_midpoint=dotpos1+t2*dotpos1_err;
      if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
      if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
      ia=cg_wave_midpoint;
      cg_wave_coh_re=t3*dot1_re+t2*baseb_envelope[2*ia  ]; 
      cg_wave_coh_im=t3*dot1_im+t2*baseb_envelope[2*ia+1]; 
      PRT04"\nstore dot %f",cg_wave_midpoint);
      insert_item(CW_DOT);
if(kill_all_flag)return;
      cw_ptr++;
      t3=0;
      if(dot_fit2 > DOT_DETECT_LIMIT && fabs(dotpos2_err) < 0.25*cwbit_pts)
        {
        if(dot2_chk < 0.25)
          {
          t3=0.75;
          }
        else
          {
          t3=0.5;
          }
        }  
      t2=1-t3; 
      cg_wave_midpoint=dotpos2+t2*dotpos2_err;
      if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
      if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
      ia=cg_wave_midpoint;
      cg_wave_coh_re=t3*dot2_re+t2*baseb_envelope[2*ia  ]; 
      cg_wave_coh_im=t3*dot2_im+t2*baseb_envelope[2*ia+1]; 
      PRT04"\nstore dot %f",cg_wave_midpoint);
      insert_item(CW_DOT);
if(kill_all_flag)return;
      cw_ptr++;
      break;


      case 5:
// Store word separator
//  ---_____---   8  ity=5
      cg_wave_midpoint=dashpos1_nom;
      PRT04"\nstore word sep %f",cg_wave_midpoint);
      insert_item(CW_WORDSEP);
if(kill_all_flag)return;
      cw_ptr++;
      break;
      }
    break;

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    default:
    if(k==10)
      {    
      set_region_envelope();
      }
    else
      {    
      set_long_region_envelope();
      }
// See if a dash will fit in the first position.
    dashpos1_nom=cw[cw_ptr-1].midpoint+4*cwbit_pts;
    if(dashpos1_nom >= baseband_size)dashpos1_nom-=baseband_size;
    dash1_chk=check_dash(dashpos1_nom);
    cg_wave_start=dashpos1_nom-(dash_pts>>1)+baseband_size;
    cg_wave_start&=baseband_mask;
    fit_dash();
    dash1_re=cg_wave_coh_re;
    dash1_im=cg_wave_coh_im;
    dashpos1=cg_wave_midpoint;
    dashpos1_err=dashpos1_nom-cg_wave_midpoint;
    if(dashpos1_err < -sizhalf)dashpos1_err+=baseband_size;
    dash_fit1=cg_wave_fit; 
    PRT05"\n{def.l}CHK DASH %f fit= %f  (%f) [%f]",
                            dash1_chk,cg_wave_fit,dashpos1_nom,cg_wave_midpoint);
    ity=0;
    if(  (dash_fit1 > DASH_DETECT_LIMIT && 
          fabs(dashpos1_err) < 0.7*cwbit_pts &&
          dash1_chk > 0)    ||
               (dash_fit1 > DASH_MISFIT_LIMIT && 
                fabs(dashpos1_err) < 0.5*cwbit_pts &&
                dash1_chk > 0.5)  ) 
      {
// A dash fits reasonably well in the lower position.
      ity=1;
// Check how well dots would fit at the beginning or end of the dash position.        
      dotpos1_nom=dashpos1_nom-cwbit_pts;
      if(dotpos1_nom < 0)dotpos1_nom+=baseband_size;
      dot1_chk=check_dot(dotpos1_nom);
      cg_wave_start=dotpos1_nom-(dot_pts>>1)+baseband_size;
      cg_wave_start&=baseband_mask;
      fit_dot();
      dotpos1=cg_wave_midpoint;
      dotpos1_err=dotpos1_nom-cg_wave_midpoint;
      if(dotpos1_err < -sizhalf)dotpos1_err+=baseband_size;
      dot_fit1=cg_wave_fit; 
      PRT05"\n{def.l}CHK DOT1 %f fit=%f  (%f)  [%f]",
                          dot1_chk,cg_wave_fit,dotpos1_nom,cg_wave_midpoint);
      if(fabs(dotpos1_err) < 0.5 * cwbit_pts && 
         dash_fit1 < dot_fit1+0.05 &&
         dot1_chk > 0.25)
        {
        PRT05"\ndot1.l good, skip dash for now.");     
        ity=0;
        }
      else
        {
        dotpos2_nom=dashpos1_nom+cwbit_pts;
        if(dotpos2_nom >= baseband_size)dotpos2_nom-=baseband_size;
        dot2_chk=check_dot(dotpos2_nom);
        cg_wave_start=dotpos2_nom-(dot_pts>>1)+baseband_size;
        cg_wave_start&=baseband_mask;
        fit_dot();
        dotpos2=cg_wave_midpoint;
        dotpos2_err=dotpos2_nom-cg_wave_midpoint;
        if(dotpos2_err < -sizhalf)dotpos2_err+=baseband_size;
        dot_fit2=cg_wave_fit; 
        PRT05"\n{def.l}CHK DOT2 %f fit=%f  (%f)  [%f]",
                          dot2_chk,cg_wave_fit,dotpos2_nom,cg_wave_midpoint);
        if(fabs(dotpos2_err) < 0.5 * cwbit_pts && 
           dash_fit1 < dot_fit2+0.05 &&
           dot2_chk > 0.25)
          {
          PRT05"\ndot2.h good, skip dash for now.");     
          ity=0;
          }
        }
      }
// See if a dash will fit in the upper position.
    dashpos2_nom=cw[cw_ptr].midpoint-4*cwbit_pts;
    if(dashpos2_nom < 0)dashpos2_nom+=baseband_size;
    dash2_chk=check_dash(dashpos2_nom);
    cg_wave_start=dashpos2_nom-(dash_pts>>1)+baseband_size;
    cg_wave_start&=baseband_mask;
    fit_dash();
    dash2_re=cg_wave_coh_re;
    dash2_im=cg_wave_coh_im;
    dashpos2=cg_wave_midpoint;
    dashpos2_err=dashpos2_nom-cg_wave_midpoint;
    if(dashpos2_err < -sizhalf)dashpos2_err+=baseband_size;
    dash_fit2=cg_wave_fit; 
    PRT05"\n{def.u}CHK DASH %f fit= %f  (%f) [%f]",
                         dash2_chk,cg_wave_fit,dashpos2_nom,cg_wave_midpoint);
    if(  (dash_fit2 > DASH_DETECT_LIMIT && 
          fabs(dashpos2_err) < 0.7*cwbit_pts &&
          dash2_chk > 0)  ||
                (dash_fit2 > DASH_MISFIT_LIMIT && 
                 fabs(dashpos2_err) < 0.5*cwbit_pts &&
                 dash2_chk > 0.5)  )
      {
// A dash fits reasonably well in the upper position.
      ity|=2;
// Check how well dots would fit at the beginning or end of the dash position.        
      dotpos3_nom=dashpos2_nom-cwbit_pts;
      if(dotpos3_nom < 0)dotpos3_nom+=baseband_size;
      dot3_chk=check_dot(dotpos3_nom);
      cg_wave_start=dotpos3_nom-(dot_pts>>1)+baseband_size;
      cg_wave_start&=baseband_mask;
      fit_dot();
      dotpos3=cg_wave_midpoint;
      dotpos3_err=dotpos3_nom-cg_wave_midpoint;
      if(dotpos3_err < -sizhalf)dotpos3_err+=baseband_size;
      dot_fit3=cg_wave_fit; 
      PRT05"\n{def.h}CHK DOT3 %f fit=%f  (%f)  [%f]",
                          dot3_chk,cg_wave_fit,dotpos3_nom,cg_wave_midpoint);

      if(fabs(dotpos3_err) < 0.5 * cwbit_pts && 
         dash_fit2 < dot_fit3+0.05 &&
         dot3_chk > 0.25)
        {
        PRT05"\ndot3.h good, skip dash for now.");     
        ity&=1;
        }
      else
        {
        dotpos4_nom=dashpos2_nom+cwbit_pts;
        if(dotpos4_nom >= baseband_size)dotpos4_nom-=baseband_size;
        dot4_chk=check_dot(dotpos4_nom);
        cg_wave_start=dotpos4_nom-(dot_pts>>1)+baseband_size;
        cg_wave_start&=baseband_mask;
        fit_dot();
        dotpos4=cg_wave_midpoint;
        dotpos4_err=dotpos4_nom-cg_wave_midpoint;
        if(dotpos4_err < -sizhalf)dotpos4_err+=baseband_size;
        dot_fit4=cg_wave_fit; 
        PRT05"\n{def.h}CHK DOT4 %f fit=%f  (%f)  [%f]",
                          dot4_chk,cg_wave_fit,dotpos4_nom,cg_wave_midpoint);
        if(fabs(dotpos4_err) < 0.5 * cwbit_pts && 
           dash_fit2 < dot_fit4+0.05 &&
           dot4_chk > 0.25)
          {
          PRT05"\ndot2.h good, skip dash for now.");     
          ity&=1;
          }

        }  
      if( (ity & 3) == 3 && k < 12)
        {         
// We can not have one dash at each side in a short region. 
// choose the best one.
        if( fabs(dash_fit1-dash_fit2) > 0.05)
          {
          if(dash_fit1 > dash_fit2)
            {
            ity=1;
            }
          else
            {
            ity=2;
            }
          }
        else
          {  
          t1=2*dash_fit1-dot_fit1-dot_fit2-2*dash_fit2+dot_fit3+dot_fit4;
          if(fabs(t1) < 0.05)
            {
            ity=0;
            }
          else
            {
            if(t1>0)ity=1; else ity=2;
            }
          }
        } 
      }
    if( (ity&1) != 0)
      {
      cg_wave_midpoint=dashpos1+0.5*dashpos1_err;
      if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
      if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
      ia=cg_wave_midpoint;
      cg_wave_coh_re=0.5*dash1_re+0.5*baseb_envelope[2*ia  ]; 
      cg_wave_coh_im=0.5*dash1_im+0.5*baseb_envelope[2*ia+1];     
      PRT05"\nstore_dash.l  %f",cg_wave_midpoint);
      insert_item(CW_DASH);
if(kill_all_flag)return;
      cw_ptr++;
      }
    if( (ity&2) != 0)
      {
      cg_wave_midpoint=dashpos2+0.5*dashpos2_err;
      if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
      if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
      ia=cg_wave_midpoint;
      cg_wave_coh_re=0.5*dash2_re+0.5*baseb_envelope[2*ia  ]; 
      cg_wave_coh_im=0.5*dash2_im+0.5*baseb_envelope[2*ia+1];     
      PRT05"\nstore_dash.h  %f",cg_wave_midpoint);
      insert_item(CW_DASH);
if(kill_all_flag)return;
      cw_ptr++;
      }
    break;
    }
  cw_ptr++;
  }
if(PR05 != 0)show_cw("first_detect exit");
*/
}

typedef struct{
float pos;
float s;
float n;
float ston;
float s_avg;
float n_avg;
int cwptr;
}STONDATA;

void improve_cwspeed(void)
{
int i, k, n, dashno;
int repeat_flag;
float r1, r3, t1, t2, t3;
double s_sum, n_sum;
STONDATA *stondt;
stondt=(STONDATA*)(fftn_tmp);
// Collect the average waveform again.
// We probably have some more dashes now and their positions are better
// known as compared to the previous speed determination.
check_cw(362003,1);
if(kill_all_flag)return;
repeat_flag=0;
repeat:;
check_cw(342003,1);
if(kill_all_flag)return;
seldash_cwspeed(0, no_of_cwdat);
cw_ptr=0;
check_cw(2003,1);
if(kill_all_flag)return;
dashno=0;
t3=0;
s_sum=0;
n_sum=0;
check_cw(22001,1);
if(kill_all_flag)return;
while(cw_ptr<no_of_cwdat)
  {
  t3+=cw[cw_ptr].sep;
  if(cw[cw_ptr].type == CW_DASH)
    {
    cg_wave_start=cw[cw_ptr].midpoint-0.5*dash_pts+baseband_size;
    cg_wave_start&=baseband_mask;
    fit_dash();
    PRT01"\n%3d %8.3f improve cwspeed [%8.3f] %8.3f fit=%8.5f",cw_ptr,t3,
                       cw[cw_ptr].midpoint, cg_wave_midpoint, cg_wave_fit);
    if(cg_wave_fit < DASH_MISFIT_LIMIT)
      {
      PRT01" removed");
      remove_dash();
      }
    else
      {  
check_cw(22002,1);
if(kill_all_flag)return;
      cw[cw_ptr].coh_re=cg_wave_coh_re;
      cw[cw_ptr].coh_im=cg_wave_coh_im;
      cw[cw_ptr].midpoint=cg_wave_midpoint;    
      if(cw_ptr > 0)
        {
        cw[cw_ptr].sep=cg_wave_midpoint-cw[cw_ptr-1].midpoint;
        if(cw[cw_ptr].sep < 0)cw[cw_ptr].sep+=baseband_size;
        }
      if(cw_ptr+1 < no_of_cwdat)
        {
        cw[cw_ptr+1].sep=cw[cw_ptr+1].midpoint-cg_wave_midpoint;
        if(cw[cw_ptr+1].sep < 0)cw[cw_ptr+1].sep+=baseband_size;
        }
      s_sum+=cg_wave_dat;
      n_sum+=cg_wave_err;
      stondt[dashno].s=cg_wave_dat;
      stondt[dashno].n=cg_wave_err;
      stondt[dashno].cwptr=cw_ptr;
      stondt[dashno].pos=t3;
      dashno++;
check_cw(22003,1);
if(kill_all_flag)return;
      }
    }
  cw_ptr++;
  }
check_cw(22009,1);
if(kill_all_flag)return;

if(dashno <= MIN_DASHNO)return;
stondt[dashno].pos=t3;
check_cw(200591,1);
if(kill_all_flag)return;
// Use the data in stondt and to decide if the new data
// has worsened the S/N ratio significantly.
// If it has, the cw speed has changed or the 
// signal has degraded or disappeared. 
// Average S and N over 3 points.
t1=stondt[0].s;
r1=stondt[0].n;
for(i=1; i<3; i++)
  {
  t1+=stondt[i].s;
  r1+=stondt[i].n;
  }
stondt[0].s_avg=t1/3;
stondt[0].n_avg=r1/3;
stondt[0].ston=t1/r1;
k=dashno-2;   
for(i=1; i<k; i++)
  {
  stondt[i].s_avg=t1/3;
  stondt[i].n_avg=r1/3;
  stondt[i].ston=t1/r1;
  t1+=stondt[i+2].s;
  r1+=stondt[i+2].n;
  t1-=stondt[i-1].s;
  r1-=stondt[i-1].n;
  } 
while(k<dashno)
  {
  stondt[k].s_avg=t1/3;
  stondt[k].n_avg=r1/3;
  stondt[k].ston=t1/r1;
  k++;
  }
// Get the center of gravity of S/N assuming S/N is constant
// in each interval (not very clever, but good enough for 
// our purpose. We use smoothed data points due to the averaging just done.)
t1=stondt[0].pos;
t3=0;
k=dashno-1;
r3=0;
n=k/10;
if(n<MIN_DASHNO/2)n=MIN_DASHNO/2;
if(k-n < MIN_DASHNO)goto use_everything; 
for(i=n; i<k; i++)
  {
  t2=t1;
  t1=stondt[i].pos;
  r1=stondt[i-1].ston;
  if(r1>stondt[i+1].ston)r1=stondt[i+1].ston;
  t3+=(t1-t2)*r1*(t1+t2);
  r3+=(t1-t2)*r1;
  }
t3/=r3;
// t3 is now twice the center of gravity for the S/N function.
// If it is much smaller than the total interval, the signal
// has degraded significantly.
if(t3 < 0.8*stondt[k].pos)
  {
// The signal seems to have degraded. 
// Get the average S/N over the interval from 0 to t3 (weighted by S) 
  PRT01"\nSignal seems to have degraded. %f  %f",t3,0.8*stondt[k].pos);
  i=0;
  t1=0;
  t2=0;
  while(stondt[i].pos<t3)
    {
    t1+=stondt[i].s*stondt[i].s/stondt[i].n;
    t2+=stondt[i].s;
    i++;
    }
  t1/=t2;
  PRT01"\nAvg S/N for good pts= %f  ",t1);
// Step backwards until we find the first dash with S/N above the average. 
  i=dashno-1;
  while( stondt[i].ston < t1)i--;
  t1=stondt[i].pos/stondt[dashno-1].pos;
  PRT01"  Good time is %6.2f%%",100*t1);
  if(t1 < 0.8)
    {
// The signal has degraded for long enough so we better evaluate
// the region up to the point in time defined by t1 before we
// try to proceed with newer data.
check_cw(322003,1);
if(kill_all_flag)return;
    no_of_cwdat=stondt[i].cwptr+1;
    repeat_flag++;
check_cw(332003,1);
if(kill_all_flag)return;
    if( repeat_flag < 3)goto repeat;
    }
  }
use_everything:;
if(repeat_flag != 0)
  {
// Treat old data first. Trying to improve the cw speed by including newer
// data may make us try to fit the morse code to two different stations
// that have different dash waveforms.   
// Change baseb_pc to reflect this decision.
  baseb_pc=cw[no_of_cwdat-1].midpoint;
  while(baseb_ramp[baseb_pc] > 0)baseb_pc =(baseb_pc+1)&baseband_mask;
  while(baseb_ramp[baseb_pc] < 0)baseb_pc =(baseb_pc+1)&baseband_mask;
  baseb_pc =(baseb_pc+baseband_mask)&baseband_mask;
  baseb_pe=baseb_pc;
  baseb_pd=baseb_pc;
  cw_detect_flag=CWDETECT_LIMITS_FOUND;
  PRT01"\n***   SET cw_detect_flag=CWDETECT_LIMITS_FOUND   ***");
  return;
  }
t1=cw_stoninv;  
get_wb_average_dashes();
guess_wb_average_dots();
t2=collect_wb_ston();
PRT01"\ncw_stoninv %f   old=%f",cw_stoninv,t1);
for(i=0; i<dashno; i++)
  {
  PRT01"\n%2d [%7.2f]  S=%f   (S/N) %f   %d",i,stondt[i].pos,
               0.001*ZZ*stondt[i].s,
                stondt[i].s/stondt[i].n,stondt[i].cwptr);
  }
check_cw(2007,1);
if(kill_all_flag)return;
if(collect_wb_ston() < CW_WAVEFORM_MINSTON)
  {
  lirerr(8877266);
  }
cw_detect_flag=CWDETECT_REGION_WAVEFORM_OK;
PRT01"\n***   SET cw_detect_flag=CWDETECT_REGION_WAVEFORM_OK   ***");
}


void first_find_parts(void)
{
int addno;
// When we arrive here the keying speed and the waveforms are well known.
// We have detected no_of_cwdat waveforms, many of them fit to a dash.
// First look for recent dashes.
check_cw(547203,1);
if(kill_all_flag)return;
if(PR02 == 1)show_cw("first_find_parts 1");
get_recent_dashes();
if(kill_all_flag)return;
if(PR02 == 1)show_cw("first_find_parts 2");
// Then collect old ones (if there are any)
check_cw(552002,1);
if(kill_all_flag)return;
get_old_dashes();
if(kill_all_flag)return;
if(PR02 == 1)show_cw("first_find_parts 3");
// Check S/N and place a pointer at the end of the current
// transmission in case S/N is severely degraded.
check_cw(552001,1);
if(kill_all_flag)return;
improve_cwspeed();
if(kill_all_flag)return;
// **********************************************************
// **********************************************************
// ******  Use the (hopefully) improved waveforms and *******
// ******  search the entire region for good matches  *******
// **********************************************************
// **********************************************************
more:;
addno=short_region_guesses(0);
check_cw(2010,1);
if(kill_all_flag)return;
if(PR02 == 1)show_cw("first_find_parts 4");
if(addno > 0)
  {
  get_wb_average_dashes();
  guess_wb_average_dots();
  goto more;
  }
addno=character_guesses();
addno+=short_region_guesses(0);
if(addno > 0)goto more;
fprintf( dmp,"\nA cwbit_pts %f",cwbit_pts);
decoded_cwspeed();
fprintf( dmp,"\nB cwbit_pts %f",cwbit_pts);
if(PR02 == 1)show_cw("first_find_parts 5");
get_wb_average_dashes();
get_wb_average_dots();
short_region_guesses(0);
mailbox[0]=9;
short_region_guesses(1);

addno=part_guesses();


if(cw_detect_flag >= CWDETECT_LIMITS_FOUND)
  {
  return;
  }
cw_detect_flag=CWDETECT_SOME_PARTS_FITTED;
PRT08"\n***   SET cw_detect_flag=CWDETECT_SOME_PARTS_FITTED   ***");
baseb_pe=baseb_pc;
baseb_pd=baseb_pc;
}

void set_baseb_pc(void)
{
int ia,ib;
// Make a first decision key up/key down based on signal power.
// This is the conventional approach, a matched filter (the operator
// has to select the optimum bandwidth) followed by a squarelaw detector.
// The threshold is conventionally kept at a fixed level but here it is
// very close to the noise when no signal is present. while it is at
// half the peak amplitude for strong signals.
// Store the result in baseb_ramp as ramps.
// Positive values indicate key down while negative values
// indicate key up (or signal too weak)
ia=baseb_pc;
ib=(ia+baseband_mask)&baseband_mask;
while( ia != baseb_pb )
  {
  if(baseb_totpwr[ia] > baseb_threshold[ia])
    {
    if(baseb_ramp[ib]>0)
      {
      baseb_ramp[ia]=baseb_ramp[ib]+1;
      }
    else
      {
      baseb_ramp[ia]=1;
      }
    }
  else
    {
    if(baseb_ramp[ib]<0)
      {
      baseb_ramp[ia]=baseb_ramp[ib]-1;
      }
    else
      {
      baseb_ramp[ia]=-1;
      }
    }
  ib=ia;
  ia=(ia+1)&baseband_mask;
  }
if(baseb_ramp[ib]>0)ib=(ib-baseb_ramp[ib]+baseband_size)&baseband_mask;
baseb_pc=ib;
}

void second_find_parts(void)
{
int  old_no_of_dashes;
// When we arrive here the keying speed and the waveforms are well known.
// We have detected no_of_cwdat waveforms, most of them dashes, but
// maybe also some dots and spaces.
// First look for recent dashes.
old_no_of_dashes=no_of_cwdat;
set_baseb_pc();
check_cw(252501,1);
if(kill_all_flag)return;
get_recent_dashes();
if(kill_all_flag)return;
// Fit dashes and dots in short gaps and next to
// dashes in the new region.
check_cw(251502,1);
if(kill_all_flag)return;
first_detect(old_no_of_dashes-1);
if(kill_all_flag)return;
if(old_no_of_dashes != no_of_cwdat)
  {
  improve_cwspeed();
if(kill_all_flag)return;
  if(cw_detect_flag >= CWDETECT_LIMITS_FOUND)
    {
    second_detect();
    if(PR08 != 0)
      {
      char s[80];
      sprintf(s,"second_find_parts (flag=%d)",cw_detect_flag);
      show_cw(s);
      }
    return;
    }
  old_no_of_dashes=no_of_cwdat;
  }
// Try to fit more dots and dashes in the whole region from 0 to no_of_cwdat.
second_detect();
if(PR08 != 0)show_cw("second_find_parts (flag unchanged)");
baseb_pe=baseb_pc;
baseb_pd=baseb_pc;
}




void cw_decode(void)
{
/*
XZ("------      cw_decode      ---------");
if(PR09 != 0)show_cw("cw_decode A");
second_find_parts();
if(PR09 != 0)show_cw("cw_decode B");
decoded_cwspeed();
if(PR09 != 0)show_cw("cw_decode C");
continued_morse_decode();
*/

}

void cw_decode_region(void)
{
XZ("cw_decode_region(do nothing)");
}



void init_cw_decode(void)
{
// We decode because S/N is already good enough to perhaps allow it.
// Some more information may have arrived. 
// Process it first and return if flag says the signal disappeared.
cw_detect_flag=CWDETECT_DEBUG_STOP;
return;

second_find_parts();
if(cw_detect_flag != CWDETECT_REGION_WAVEFORM_OK)
  {
  XZ("\nEND OF REGION!!!");
  return;
  }
decoded_cwspeed();
check_cw(256702,1);
if(kill_all_flag)return;
// Try to find complete characters
if(PR09 != 0)show_cw("init_cw_decode 1");
first_morse_decode();
if(kill_all_flag)return;
if(PR09 != 0)show_cw("init_cw_decode 2");
check_cw(256703,1);
if(kill_all_flag)return;
if(cw_decoded_chars != 0)
  {
  cw_detect_flag=CWDETECT_SOME_ASCII_FITTED;
  XZ("\nOKOKOKOKOKOKOK");
//show_cw("Set detect_flag to CWDETECT_SOME_ASCII_FITTED");
  }
else
  {
fprintf( dmp,"\nno_of_cwdat %d  no_of_cwdat-correlate_no_of_cwdat %d",
     no_of_cwdat, no_of_cwdat-correlate_no_of_cwdat);
fprintf( dmp,"\n pa %d  pb %d  pc %d  pd %d  pe %d",
           baseb_pa, baseb_pb, baseb_pc, baseb_pd, baseb_pe);




  if(no_of_cwdat > 25 && no_of_cwdat-correlate_no_of_cwdat > 2)
    {
    correlate_undecoded_baseband();
    correlate_no_of_cwdat=no_of_cwdat;

// ö update baseb_px and other pointers as well as cw[] and no_of_cwdat
// if correlate failed and data is becoming too old.

    }
  }
}

void init_cw_decode_region(void)
{
decoded_cwspeed();
first_morse_decode();
cw_detect_flag=CWDETECT_REGION_INITIATED;
}
