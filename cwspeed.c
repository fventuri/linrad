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


void show_cw(char *s);


// Skip processing in case the number of dashes is below 
// the MIN_NO_OF_CWDAT limit.
#define MIN_NO_OF_CWDAT 5

#define ZZ 0.000000001

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
// float[1] baseb_agc_level=used only when AGC is enabled.
// float[2] baseb_wb_raw=complex amplitude like baseb_raw at a larger bandwidth.
// float[2] baseb_wb=complex amplitude of coherent detect like baseb, at larger bw.
// short int[1] baseb_ramp=indicator for time of power above/below threshold.
// short_int[1] baseb_clock=CW code clock
// float[2] baseb_tmp=for debug during development

// baseb_pa  The starting point of the latest mix2_size/2 points.
// baseb_pb  The point up to which thresholds exist.
// baseb_pc  The point up to which cw speed statistics and ramp is collected.
// baseb_pd  A key up region close before baseb_pc
// baseb_pe  The point up to which we have run first detect. 
// baseb_pf
// baseb_px  The oldest point that contains valid data.


void check_cw(int num,int type)
{
int i;
float t1;
for(i=0; i<no_of_cwdat;i++)
  {
  if(cw[i].len == 0)
    {
    DEB"\nLen is zero");
    goto err;
    }  
  if(type > 0)
    {
    if(i>0)
      {
      t1=cw[i].midpoint-cw[i-1].midpoint;
      if(t1 > baseband_neg)
        {
        DEB"\nt1>neg  %f",t1);
        goto err;
        }
      if(t1 < 0)t1+=baseband_size;
      if(fabs (t1-cw[i].sep)/baseband_size > 0.0000001)
        {
        DEB"\nt1!=sep  %f  %f",t1,cw[i].sep);
        goto err;
        }
      }
    }
  }
return;
err:;
DEB"\nERROR[%d] i=%d",num,i);
for(i=0; i<no_of_cwdat; i++)
  {
  DEB"\n%3d %10.3f  %8.3f   (re %f im %f)",i,
            cw[i].midpoint,cw[i].sep, ZZ*cw[i].coh_re, ZZ*cw[i].coh_im );
  }
DEB"\n");  
DEB"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
lirerr(3001);
}

int short_region_guesses(int types)
{
int i, j, k;
int dashadd;
float t1, t2, r1, r2;
float a1,a2,p1,p2;
float noise[5], npow[5], powston[5];
float re, im, re1, im1, re3, im3;
// Step through all the dashes and look for small separations that
// fit to the keying speed we just determined.
// Look for the following cases:
//    Code       Sep
//  ___ ___       4
//  ___ _ ___     6
//  ___   ___     6
//  ___ _ _ ___   8
//  ___ ___ ___   8
//  ___     ___   8
// Store the most probable signal in each case.
cw_ptr=1;
cg_wave_coh_re=0;
cg_wave_coh_im=0;
dashadd=0;
show_cw("**************short region guesses");
while(cw_ptr < no_of_cwdat)
  {
  if( (cw[cw_ptr].type==CW_DASH || types*cw[cw_ptr].type==CW_DOT) && 
       (cw[cw_ptr-1].type==CW_DASH || types*cw[cw_ptr-1].type==CW_DOT))
    {
    r1=cw[cw_ptr].sep/cwbit_pts;
    j=(cw[cw_ptr].len+cw[cw_ptr-1].len)/2;
    r1-=j;
    k=(int)(r1)&0xfffffffe;
    if(r1-k > 1)k+=2;
    if(k<0)k=0;
    cw[cw_ptr].unkn=k;
    switch (k)
      {
      case 0:
      break;

      case 2:
//  ___ _ ___   2 guess 0 (dash ? dash) 
//  ___   ___   2 guess 1 (dash ? dash)
//     012
//  ___ _ _     2 guess 0 (dash ? dot)
//  ___   _     2 guess 1 (dash ? dot)
//     012
//  _ _ ___     2 guess 0 (dot ? dash)
//  _   ___     2 guess 1 (dot ? dash)
//     012
fprintf( dmp,"\n\ncheck2 [%f to %f]  (%f,%f),(%f,%f)",cw[cw_ptr-1].midpoint,cw[cw_ptr].midpoint,
                          ZZ*cw[cw_ptr-1].raw_re,ZZ*cw[cw_ptr-1].raw_im, ZZ*cw[cw_ptr].raw_re,ZZ*cw[cw_ptr].raw_im);
      cg_wave_midpoint=cw[cw_ptr].midpoint-0.5*cw[cw_ptr].sep;
fprintf( dmp,"\n midpnt=%f",cg_wave_midpoint);
      cg_wave_midpoint-=0.25*(cw[cw_ptr].len-cw[cw_ptr-1].len)*cwbit_pts;
      if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
fprintf( dmp," new midpnt=%f",cg_wave_midpoint);
      wb_investigate_region(cg_wave_midpoint, 3);
// Compute signal and noise at the three positions assuming that 
// phase and amplitude change linearly between the dashes.      
// Put the total noise in noise[i] for the two guesses.
      r1=reg_dot_re[0]*reg_dot_re[0]+reg_dot_im[0]*reg_dot_im[0]+
         reg_dot_re[2]*reg_dot_re[2]+reg_dot_im[2]*reg_dot_im[2];
      a1=sqrt(cw[cw_ptr-1].raw_re*cw[cw_ptr-1].raw_re+
              cw[cw_ptr-1].raw_im*cw[cw_ptr-1].raw_im);
      a2=sqrt(cw[cw_ptr  ].raw_re*cw[cw_ptr  ].raw_re+
              cw[cw_ptr  ].raw_im*cw[cw_ptr  ].raw_im);
      p1=atan2(cw[cw_ptr-1].raw_im,cw[cw_ptr-1].raw_re);
      p2=atan2(cw[cw_ptr  ].raw_im,cw[cw_ptr  ].raw_re);
      if(p1-p2 > PI_L)p1+=2*PI_L;
      if(p2-p1 > PI_L)p2+=2*PI_L;
      t1=0.5*(a1+a2);
      t2=0.5*(p1+p2);
      re=t1*cos(t2);
      im=t1*sin(t2);
      noise[0]=0.3333333*(r1+(re-reg_dot_re[1])*(re-reg_dot_re[1])+
                       (im-reg_dot_im[1])*(im-reg_dot_im[1]));
      r2=reg_dot_re[1]*reg_dot_re[1]+reg_dot_im[1]*reg_dot_im[1];
      noise[1]=0.3333333*(r1+r2);
// (due to operator preference of speed above accuracy) the phase 
// could be incorrect. Compute S/N based on total power in the 
// wb_raw signal so strong signals with phase errors will decode here.
      powston[0]=2*reg_dot_power[1]/(reg_dot_power[0]+reg_dot_power[2])-1;
fprintf( dmp,"\nPOWSTON %f  COHNOI %f %f", powston[0],ZZ*ZZ*noise[0],ZZ*ZZ*noise[1]);      
// Take a decision (assuming that the surrounding dashes are correct):
      if( (powston[0] > 10) ||
          (powston[0] >  3  && noise[0] < 1.5*noise[1]) ||
          (powston[0] > 0.4 && noise[0] < 2.0*noise[1]) )
        {
        cg_wave_raw_re=reg_dot_re[1];
        cg_wave_raw_im=reg_dot_im[1];
        insert_item(CW_DOT);
        cw[cw_ptr].unkn=0;
        if(kill_all_flag)return 0;
        cw_ptr++;
        cw[cw_ptr].unkn=0;
        check_cw(100931,1);
        if(kill_all_flag)return 0;
        }
      else
        {
        if( (powston[0] < 4 && noise[1] < 2.0*noise[0]) ||
            (powston[0] < 1 && noise[1] < 1.5*noise[0]) )
          {
          cg_wave_raw_re=0;
          cg_wave_raw_im=0;
          insert_item(CW_SPACE);
          cw[cw_ptr].unkn=0;
          if(kill_all_flag)return 0;
          cw_ptr++;
          cw[cw_ptr].unkn=0;
          check_cw(100932,1);
          if(kill_all_flag)return 0;
          }  
        }
      break;
 
      case 4:
//  ___ ___ ___   8  guess 0
//  ___ _ _ ___   8  guess 1
//  ___ _   ___   8  guess 2
//  ___   _ ___   8  guess 3
//  ___     ___   8  guess 4
//     01234
fprintf( dmp,"\n\ncheck4 [%f to %f]  (%f,%f),(%f,%f)",cw[cw_ptr-1].midpoint,cw[cw_ptr].midpoint,
                          ZZ*cw[cw_ptr-1].raw_re,ZZ*cw[cw_ptr-1].raw_im, ZZ*cw[cw_ptr].raw_re,ZZ*cw[cw_ptr].raw_im);
      cg_wave_midpoint=cw[cw_ptr].midpoint-0.5*cw[cw_ptr].sep;
      cg_wave_midpoint-=0.25*(cw[cw_ptr].len-cw[cw_ptr-1].len)*cwbit_pts;
      if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
      wb_investigate_region(cg_wave_midpoint, 5);
// Compute the average noise power for the 5 different guesses and
// store in noise[i].
      r1=reg_dot_re[0]*reg_dot_re[0]+reg_dot_im[0]*reg_dot_im[0]+
         reg_dot_re[4]*reg_dot_re[4]+reg_dot_im[4]*reg_dot_im[4];
      a1=sqrt(cw[cw_ptr-1].raw_re*cw[cw_ptr-1].raw_re+
              cw[cw_ptr-1].raw_im*cw[cw_ptr-1].raw_im);
      a2=sqrt(cw[cw_ptr  ].raw_re*cw[cw_ptr  ].raw_re+
              cw[cw_ptr  ].raw_im*cw[cw_ptr  ].raw_im);
      p1=atan2(cw[cw_ptr-1].raw_im,cw[cw_ptr-1].raw_re);
      p2=atan2(cw[cw_ptr  ].raw_im,cw[cw_ptr  ].raw_re);
      if(p1-p2 > PI_L)p1+=2*PI_L;
      if(p2-p1 > PI_L)p2+=2*PI_L;
      t1=0.5*(a1+a2);
      t2=0.5*(p1+p2);
      re=t1*cos(t2);
      im=t1*sin(t2);
// Guess 0
      noise[0]=0.2*(r1+3*((re-reg_dash_re)*(re-reg_dash_re)+
                          (im-reg_dash_im)*(im-reg_dash_im)) );
// Guess 4
      t1=0;
      for(i=1; i<4; i++)
        {
        npow[i]=reg_dot_re[i]*reg_dot_re[i]+
                reg_dot_im[i]*reg_dot_im[i];
        t1+=npow[i];
        } 
      noise[4]=0.2*(r1+t1);
      t1=0.625*a1+0.375*a2;
      t2=0.625*p1+0.375*p2;
      re1=t1*cos(t2);
      im1=t1*sin(t2);
      t1=0.375*a1+0.625*a2;
      t2=0.375*p1+0.625*p2;
      re3=t1*cos(t2);
      im3=t1*sin(t2);
// Guess 1
      noise[1]=0.2*(r1+npow[2]+
                       (re1-reg_dot_re[1])*(re1-reg_dot_re[1])+
                       (im1-reg_dot_im[1])*(im1-reg_dot_im[1])+
                       (re3-reg_dot_re[3])*(re3-reg_dot_re[3])+
                       (im3-reg_dot_im[3])*(im1-reg_dot_im[3]) );
// Guess 2
      noise[2]=noise[4]+0.2*( 
                   (re1-reg_dot_re[1])*(re1-reg_dot_re[1])+
                   (im1-reg_dot_im[1])*(im1-reg_dot_im[1])-npow[1] );
// Guess 3
      noise[3]=noise[4]+0.2*(
                   (re3-reg_dot_re[3])*(re3-reg_dot_re[3])+
                   (im3-reg_dot_im[3])*(im3-reg_dot_im[3])-npow[3] );
// In case the signal went through a QSB minimum or AFC failed 
// (due to operator preference of speed above accuracy) the phase 
// could be incorrect. Compute (S+N)/N based on total power in the 
// wb_raw signal so strong signals with phase errors will decode here.
      r2=reg_dot_power[0]+reg_dot_power[4];
      powston[0]=2*reg_dash_power/r2;
      powston[4]=2.5*(r2+cw[cw_ptr-1].raw_re*cw[cw_ptr-1].raw_re+
                         cw[cw_ptr-1].raw_im*cw[cw_ptr-1].raw_im+
                         cw[cw_ptr  ].raw_re*cw[cw_ptr  ].raw_re+
                         cw[cw_ptr  ].raw_im*cw[cw_ptr  ].raw_im)/
                                                (r2+3*reg_dash_power);
      r2+=reg_dot_power[2];
      powston[1]=1.5*(reg_dot_power[1]+reg_dot_power[3])/r2;
      powston[2]=4*reg_dot_power[1]/(reg_dot_power[3]+r2);
      powston[3]=4*reg_dot_power[3]/(reg_dot_power[1]+r2);
// Check if any of the guesses seems much more likely than the others.
// based on the integrated signal over a time unit on the assumption
// that the phase rotates slowly.
      t1=BIGFLOAT;
      k=-1;

      for(i=0; i<5; i++)
        {
        if(noise[i] < t1)
          {
          k=i;
          t1=noise[i];
          }
        }
      t2=0;
      j=-1;
      for(i=0; i<5; i++)
        {
        if(powston[i] > t2)
          {
          j=i;
          t2=powston[i];
          }
        }
// In case we get different guesses based on powston and noise the 
// guess for the phase and amplitude must be bad. Check if one or
// the other is good enough (while the other is bad) so we can
// base our decision on one or the other.
      if(j != k)
        {
        if( t2 > 10 || (t2 >  3  && noise[j] < 1.25*t1) )
          {
          k=j;
          }
        else
          {
          if( (t2 < 4.0 && t1 > 2.0*noise[j]) ||
              (t2 < 1.3 && t1 > 1.5*noise[j]) )
            {
            j=k;
            }
          else
            {
            goto skip8;
            }
          }
        }    
// Check the probability we made the correct guess.
// Hope for some help here!!!
      t1=BIGFLOAT;
      t2=0;
      for(i=0; i<5; i++)
        {
        if(i!=k)
          {
          if(noise[i]<t1)t1=noise[i];
          if(powston[i]>t2)t2=powston[i];
          }
        }  
      if( (powston[k] > 10) ||
          (powston[k] >  3  && noise[k] < 1.5*t1) ||
          (powston[k] > 0.4 && noise[k] < 2.0*t1) )
        { 
        switch (k)
          {
          case 0:
//  ___ ___ ___   8  guess 0
          cg_wave_raw_re=reg_dash_re;
          cg_wave_raw_im=reg_dash_im;
          insert_item(CW_DASH);
          dashadd++;
          cw[cw_ptr].unkn=0;
          if(kill_all_flag)return 0;
          cw_ptr++;
          cw[cw_ptr].unkn=0;
          check_cw(110931,1);
          if(kill_all_flag)return 0; 
          break;
          
          case 1:
//  ___ _ _ ___   8  guess 1
          cg_wave_raw_re=reg_dot_re[1];
          cg_wave_raw_im=reg_dot_im[1];
          cg_wave_midpoint-=cwbit_pts;
          if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
          insert_item(CW_DOT);
          cw[cw_ptr].unkn=0;
          if(kill_all_flag)return 0;
          cw_ptr++;
          check_cw(110931,1);
          if(kill_all_flag)return 0; 
          cg_wave_raw_re=reg_dot_re[3];
          cg_wave_raw_im=reg_dot_im[3];
          cg_wave_midpoint+=2*cwbit_pts;
          if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
          insert_item(CW_DOT);
          cw[cw_ptr].unkn=0;
          if(kill_all_flag)return 0;
          cw_ptr++;
          cw[cw_ptr].unkn=0;
          check_cw(110932,1);
          if(kill_all_flag)return 0;  
          break;
          
          case 2:
//  ___ _   ___   8  guess 2
          cg_wave_raw_re=reg_dot_re[1];
          cg_wave_raw_im=reg_dot_im[1];
          cg_wave_midpoint-=cwbit_pts;
          if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
          insert_item(CW_DOT);
          cw[cw_ptr].unkn=0;
          if(kill_all_flag)return 0;
          cw_ptr++;
          check_cw(110931,1);
          if(kill_all_flag)return 0; 
          cg_wave_raw_re=0;
          cg_wave_raw_im=0;
          cg_wave_midpoint+=2*cwbit_pts;
          if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
          insert_item(CW_SPACE);
          cw[cw_ptr].unkn=0;
          if(kill_all_flag)return 0;
          cw_ptr++;
          cw[cw_ptr].unkn=0;
          check_cw(110932,1);
          if(kill_all_flag)return 0; 
          break;
          
          case 3:
//  ___   _ ___   8  guess 3
          cg_wave_raw_re=0;
          cg_wave_raw_im=0;
          cg_wave_midpoint-=cwbit_pts;
          if(cg_wave_midpoint < 0)cg_wave_midpoint+=baseband_size;
          insert_item(CW_SPACE);
          cw[cw_ptr].unkn=0;
          if(kill_all_flag)return 0;
          cw_ptr++;
          check_cw(110932,1);
          if(kill_all_flag)return 0; 
          cg_wave_raw_re=reg_dot_re[3];
          cg_wave_raw_im=reg_dot_im[3];
          cg_wave_midpoint+=2*cwbit_pts;
          if(cg_wave_midpoint >= baseband_size)cg_wave_midpoint-=baseband_size;
          insert_item(CW_DOT);
          cw[cw_ptr].unkn=0;
          if(kill_all_flag)return 0;
          cw_ptr++;
          cw[cw_ptr].unkn=0;
          check_cw(110931,1);
          if(kill_all_flag)return 0; 
          break;

          case 4:
//  ___     ___   8  guess 4
          cg_wave_raw_re=0;
          cg_wave_raw_im=0;
          insert_item(CW_WORDSEP);
          cw[cw_ptr].unkn=0;
          if(kill_all_flag)return 0;
          cw_ptr++;
          cw[cw_ptr].unkn=0;
          check_cw(110931,1);
          if(kill_all_flag)return 0; 
          break;
          }
        }
skip8:;

for(i=0;i<5;i++)      fprintf( dmp,"\nguess %d WSTON %f   COHNOI %f  ",i, powston[i], ZZ*ZZ*noise[i]);
      break;

      default:
fprintf( dmp,"\n\ncheckx %d [%f to %f]  (%f,%f),(%f,%f)",k,cw[cw_ptr-1].midpoint,cw[cw_ptr].midpoint,
                          ZZ*cw[cw_ptr-1].raw_re,ZZ*cw[cw_ptr-1].raw_im, ZZ*cw[cw_ptr].raw_re,ZZ*cw[cw_ptr].raw_im);
      break;
      }
    }
  cw_ptr++;
  }
check_cw(1009,1);
show_cw("short reg exit ");
XZ(" ");
return dashadd;
}

void find_good_dashes(int ia, int ic, char color)
{
int lmax, lmin, old_no_of_cwdat;
int i, j, k, ib;
char s[80];
float t1;
MORSE_DECODE_DATA tmpcw;
// Step backwards using baseb_ramp and fill in all good fits to
// dash_waveform. We try to find dashes first because they contain
// much more energy and occupy less bandwidth than the dots.
// Comparing the complex waveform to the desired waveform corresponds
// to a low pass filter so we look for dashes because they have much
// better S/N since they last 3 times longer.
// fit_dash returns 1.000 if the waveform fits exactly.
if(baseb_ramp[ic] >= 0)lirerr(925727);
old_no_of_cwdat=no_of_cwdat;
lmax=1.5*dash_pts;
lmin=0.4*dash_pts;
ib=ic;
check_cw(30232,0);
if(kill_all_flag)return;
while( ib != ia)
  {
// Step downwards by the (negative) ramp length.
// We should always be on a negative ramp here.
  ib=(ib+baseb_ramp[ib]+baseband_size)&baseband_mask;
  if( ((ib-baseb_pe+baseband_size)&baseband_mask) > 
                                            baseband_neg)goto good_dash_x;
// Now we must be on a positive ramp.
  if(baseb_ramp[ib] >= lmin && baseb_ramp[ib] <= lmax)
    {
    cg_wave_start=
           (ib-((baseb_ramp[ib]+dash_pts)>>1)+baseband_size)&baseband_mask;
    fit_dash();
    if(cg_wave_fit >DASH_DETECT_LIMIT)
      {
      cw_ptr=no_of_cwdat;
      cg_wave_raw_re=0;
      cg_wave_raw_im=0;
      insert_item(CW_DASH);
      }
    }  
  ib=(ib-baseb_ramp[ib]+baseband_size)&baseband_mask;
  }
good_dash_x:;
sprintf(s,"%4d    ",no_of_cwdat);
settextcolor(color);
lir_pixwrite(MAX_CG_OSCW+2,0,s);
settextcolor(7);
// We stepped backwards using the ramps when collecting dash midpoints.
// Reorder dash midpoints
if(no_of_cwdat - old_no_of_cwdat > 1)
  {
  i=no_of_cwdat-1;
  j=old_no_of_cwdat;
  while(j < i)
    {
    tmpcw=cw[i];
    cw[i]=cw[j];
    cw[j]=tmpcw;
    j++;
    i--;
    }
  }
// Collect dash separations
for(i=old_no_of_cwdat; i<no_of_cwdat; i++)
  {
  t1=cw[i].midpoint-cw[i-1].midpoint;
  if(t1 < 0)t1+=baseband_size;
  cw[i].sep=t1;
  t1/=cwbit_pts;
  j=(cw[i].len+cw[i-1].len)/2;
  t1-=j;
  k=(int)(t1)&0xfffffffe;
  if(t1-k > 1)k+=2;
  cw[i].unkn=k;
  }
}


  
void detect_cw_speed(void)
{
int first_icw, iter;
int i, j, k;
int dashadd;
float r1, t1;
float average_sep, largest_sep;
int largest_sep_pos;
// Use baseb ramp and our current guess for the dash waveform
// to find those dashes that are reasonably similar.
find_good_dashes(baseb_pe, baseb_pc,12);
if(no_of_cwdat < MIN_NO_OF_CWDAT)
  {
fail:;
  cw_detect_flag=CWDETECT_ERROR;
  no_of_cwdat=0;
  return;
  }
// Collect dash separations
first_icw=1;
largest_sep=0;
largest_sep_pos=-1;
t1=cw[0].midpoint-baseb_px;
if(t1 < 0)t1+=baseband_size;
cw[0].sep=t1;
for(i=1; i<no_of_cwdat; i++)
  {
  if(cw[i].sep > largest_sep)
    {
    largest_sep=cw[i].sep;
    largest_sep_pos=i;
    }
  }
 check_cw(1003,1);
if(kill_all_flag)return;
// *****************************************************
// *****************************************************
// ************** Find cw keying speed *****************
// *****************************************************
// *****************************************************
// In case we are trying to find an extremely weak signal the
// waveform might be poor due to the inclusion of a lot of noise.
// We may also have incorrect speed information because the
// speed has changed - maybe one station stopped transmission and 
// another started.    
// largest_sep_pos points to the dash after the longest 
// separation between dashes.
// If there are many enough dashes after it, skip data before it.


if(no_of_cwdat - largest_sep_pos > 8)
  {
  average_sep=cw[no_of_cwdat-1].midpoint-cw[0].midpoint;
  if(average_sep < 0)average_sep+=baseband_size;
  average_sep/=no_of_cwdat;
  t1=cw[no_of_cwdat-1].midpoint-cw[largest_sep_pos].midpoint;
  if(t1 < 0)t1+=baseband_size;
  t1/=(no_of_cwdat-largest_sep_pos);      
  if(average_sep/t1 > 1.5)
    {
    first_icw=largest_sep_pos;
    }
  }
// Collect the average waveform for the selected dashes
// over a range that will contain surrounding dashes (and dots)
// and evaluate the cw keying speed and a more accurate dash waveform.
seldash_cwspeed(first_icw, no_of_cwdat);
if(cw_detect_flag == CWDETECT_ERROR)
  {
  return;
  }
// *****************************************************
// *****************************************************
// *********** Find good dashes again. This time *******
// ***********     use improved dash waveform    *******
// *****************************************************
// *****************************************************
iter=0;
iterate:;
iter++;
no_of_cwdat=0;
find_good_dashes(baseb_pe, baseb_pc,12);
if(no_of_cwdat < MIN_NO_OF_CWDAT)goto fail;
check_cw(1007,1);
if(kill_all_flag)return;
// *****************************************************
// *****************************************************
// ********     Eliminate obvious errors     ***********
// *****************************************************
// *****************************************************
cw_ptr=1;
while(cw_ptr < no_of_cwdat)
  {
  r1=cw[cw_ptr].sep/cwbit_pts;
  if(r1 < 3)
    {
// When two dashes have been stored too close, on top of each other,
// the waveform is most probably very ugly.
// Skip these dashes for now.
    i=cw_ptr-1;
    j=cw_ptr+1;
    while(j < no_of_cwdat)
      {
      cw[i]=cw[j];
      i++;
      j++;
      }
    if(cw_ptr >= 2)
      {  
      t1=cw[cw_ptr-1].midpoint-cw[cw_ptr-2].midpoint;
      if(t1 < 0)t1+=baseband_size;
      cw[cw_ptr-1].sep=t1;
      }
    t1=cw[cw_ptr].midpoint-cw[cw_ptr-1].midpoint;
    if(t1 < 0)t1+=baseband_size;
    cw[cw_ptr].sep=t1;
    no_of_cwdat-=2;
    cw_ptr--;
    }
  cw_ptr++;
  }  
 check_cw(1008,1);
if(kill_all_flag)return;
get_wb_average_dashes();
guess_wb_average_dots();
qq2("WIDE OK");
// ***************************************************
// ***************************************************
// ****** Investigate short regions between   ********
// ******    dashes we already detected.      ********
// ***************************************************
// ***************************************************
dashadd=short_region_guesses(0);
if(kill_all_flag)return;
show_cw("**************************************");  
// *****************************************************
// *****************************************************
// ************** Find cw keying speed again ***********
// **** by averaging the narrowband shape of dashes ****
// ******  but only if we have more dashes  ************
// *****************************************************
// *****************************************************
cw_ptr=0;
if(dashadd == 0 && iter != 1)goto skip_shape_improvement;
fprintf( dmp,"\n more dashes!!");
first_icw=1;
largest_sep=0;
largest_sep_pos=-1;
for(i=1; i<no_of_cwdat; i++)
  {
  t1=cw[i].midpoint-cw[i-1].midpoint;
  if(t1 < 0)t1+=baseband_size;
  if(t1 > largest_sep)
    {
    largest_sep=t1;
    largest_sep_pos=i;
    }
  }
// In case we are trying to find an extremely weak signal the
// waveform might be poor due to the inclusion of a lot of noise.
// We may also have incorrect speed information because the
// speed has changed - maybe one station stopped transmission and 
// another started.    
// largest_sep_pos points to the dash after the longest 
// separation between dashes.
// If there are many enough dashes after it, skip data before it.
if(no_of_cwdat - largest_sep_pos > 8)
  {
  average_sep=cw[no_of_cwdat-1].midpoint-cw[0].midpoint;
  if(average_sep < 0)average_sep+=baseband_size;
  average_sep/=no_of_cwdat;
  t1=cw[no_of_cwdat-1].midpoint-cw[largest_sep_pos].midpoint;
  if(t1 < 0)t1+=baseband_size;
  t1/=(no_of_cwdat-largest_sep_pos);      
  if(average_sep/t1 > 1.5)
    {
    first_icw=largest_sep_pos;
    }
  }
// Collect the average waveform for the selected dashes
// over a range that will contain surrounding dashes (and dots)
// and evaluate the cw keying speed and a more accurate dash waveform.
check_cw(102210,1);
for(cw_ptr=first_icw; cw_ptr<no_of_cwdat; cw_ptr++)
  {
fprintf( dmp,"\ncw_ptr=%d",cw_ptr);
  if(cw[cw_ptr].type == CW_DASH)
    {
    if(cw[cw_ptr].coh_re==0 && cw[cw_ptr].coh_im==0 )
      {
      cg_wave_start=cw[cw_ptr].midpoint-0.5*dash_pts+baseband_size;
      cg_wave_start&=baseband_mask;
      fit_dash();
      cw[cw_ptr].coh_re=cg_wave_coh_re;
      cw[cw_ptr].coh_im=cg_wave_coh_im;
      cw[cw_ptr].midpoint=cg_wave_midpoint;
      if(cw_ptr > 0)
        {
        t1=cw[cw_ptr].midpoint-cw[cw_ptr-1].midpoint;
        if(t1 < 0)t1+=baseband_size;
        cw[cw_ptr].sep=t1;
        t1/=cwbit_pts;
        j=(cw[cw_ptr].len+cw[cw_ptr-1].len)/2;
        t1-=j;
        k=(int)(t1)&0xfffffffe;
        if(t1-k > 1)k+=2;
        cw[cw_ptr].unkn=k;
        }
      if(cw_ptr+1 < no_of_cwdat)
        {
        if(cw[cw_ptr+1].coh_re!=0 || cw[cw_ptr+1].coh_im!=0 )
          {
          t1=cw[cw_ptr+1].midpoint-cw[cw_ptr].midpoint;
          if(t1 < 0)t1+=baseband_size;
          cw[cw_ptr+1].sep=t1;
          t1/=cwbit_pts;
          j=(cw[cw_ptr+1].len+cw[cw_ptr].len)/2;
          t1-=j;
          k=(int)(t1)&0xfffffffe;
          if(t1-k > 1)k+=2;
          cw[cw_ptr+1].unkn=k;
          }
        }
      }    
    }
  } 
check_cw(102211,1);
seldash_cwspeed(first_icw, no_of_cwdat);
if(cw_detect_flag == CWDETECT_ERROR)
  {
  XZ("speed_error");
  return;
  }
if(kill_all_flag)return;
first_icw--;
cw_ptr=0;
if(first_icw > 0)
  {
  while(cw_ptr<first_icw)
    {
    if(cw[cw_ptr].type == CW_DASH)
      {
      cg_wave_start=cw[cw_ptr].midpoint-0.5*dash_pts+baseband_size;
      cg_wave_start&=baseband_mask;
      fit_dash();
      if(cg_wave_fit < DASH_MISFIT_LIMIT)
        {
        remove_dash();
        first_icw--;
        }
      else
        {  
        cw[cw_ptr].coh_re=cg_wave_coh_re;
        cw[cw_ptr].coh_im=cg_wave_coh_im;
        cw_ptr++;
        }
      }
    else
      {
      cw_ptr++;
      }
    }
  }  
if(iter < 2)goto iterate;
if(no_of_cwdat < MIN_NO_OF_CWDAT)goto fail;
get_wb_average_dashes();
guess_wb_average_dots();
skip_shape_improvement:;
if(collect_wb_ston() < CW_WAVEFORM_MINSTON )goto fail;
// ******************************************************
// ******************************************************
// ******  Morse code waveforms are accurate now.  ******  
// ******    Set flag so we will not return here   ******  
// ******************************************************
// ******************************************************
cw_detect_flag=CWDETECT_WAVEFORM_ESTABLISHED;
show_cw("WAVEFORM OK");
}                                      

void wb_investigate_region(float pos, int len)
{
float rpos, pwr;
float re, im, r1, r2, t1, t2;
int i, j, ia, ib;
fprintf( dmp,"\nwb_investigate_region(%.1f, %d)",pos,len);
if(len == -5)goto dasheval;
// pos is the midpoint of a len code units region region that
// could be any of these waveforms:
// **************
//    len = 3
//  ??_ _ _??   
//  ??_   _??   
//     123
// These two waveforms differ only in position 2.
//
// **************
//    len = 5
//  ??_ _ _ _??   
//  ??_   _ _??   
//  ??_ _   _??   
//  ??_ ___ _??   
//  ??_     _??   
//     12345
// These five waveforms differ only in positions 2,3 and 4.
//
// We now want to find out which waveform is most likely. 
// The final decision will be left to the calling routine.
// Information about what is before and after these 3 or 5 
// code units will affect the final decision.
// Here we just compute powers and complex amplitudes for the
// different cases.
//
// 
// First compute the phase and power we would attribute
// to a dot in each of the len positions.
i=len/2;
fprintf( dmp,"\npos %f dot_sizhalf %d i*cwbit_pts %f",pos,dot_sizhalf,i*cwbit_pts);
rpos=pos-dot_sizhalf+baseband_size-i*cwbit_pts;
for(i=0; i<len; i++)
  {
  re=0;
  im=0;
  pwr=0;
  t1=rpos;
  ia=rpos;
  t1-=ia;
  t2=1-t1;
  ia+=dot_wb_ia;
  ia &= baseband_mask;
  ib=ia;
  ib=(ia+1)&baseband_mask;
  for(j=dot_wb_ia; j<dot_wb_ib; j++)
    {
    r1=baseb_wb_raw[2*ib  ]*t1+baseb_wb_raw[2*ia  ]*t2;
    r2=baseb_wb_raw[2*ib+1]*t1+baseb_wb_raw[2*ia+1]*t2;
    pwr+=r1*r1+r2*r2;
    re+= dot_wb_waveform[2*j  ]*r1+dot_wb_waveform[2*j+1]*r2;
    im+=-dot_wb_waveform[2*j  ]*r2+dot_wb_waveform[2*j+1]*r1;

if(mailbox[0]==9)
{
baseb_sho1[2*ia]=100*dot_wb_waveform[2*j  ]/ZZ;
baseb_sho1[2*ia+1]=100*dot_wb_waveform[2*j+1]/ZZ;

baseb_sho2[2*ia]=r1;
baseb_sho2[2*ia+1]=r2;
}
    ia=ib;
    ib=(ib+1)&baseband_mask;
    }      
  reg_dot_re[i]=re;
  reg_dot_im[i]=im;
  reg_dot_power[i]=pwr/(dot_wb_ib-dot_wb_ia);
  rpos+=cwbit_pts;

{
t1=rpos+(dot_wb_ib-dot_wb_ia)/2;
if(t1>baseband_size)t1-=baseband_size;
t2=ZZ*reg_dot_re[i]*ZZ*reg_dot_re[i]+ZZ*reg_dot_im[i]*ZZ*reg_dot_im[i];
fprintf( dmp,"\nreg %d pos %f  re %f im %f   pwr %f %f",i, t1, ZZ*reg_dot_re[i], ZZ*reg_dot_im[i],ZZ*ZZ*reg_dot_power[i],t2);
}


  }
// If we have len=5, check how well a dash would fit.
if(len==5)
  {
dasheval:;  
  re=0;
  im=0;
  pwr=0;
  rpos=pos-dash_sizhalf+baseband_size;
  t1=rpos;
  ia=rpos;
  t1-=ia;
  t2=1-t1;
  ia+=dash_wb_ia;
  ia &= baseband_mask;
  ib=ia;
  ib=(ia+1)&baseband_mask;
  for(j=dash_wb_ia; j<dash_wb_ib; j++)
    {
    r1=baseb_wb_raw[2*ib  ]*t1+baseb_wb_raw[2*ia  ]*t2;
    r2=baseb_wb_raw[2*ib+1]*t1+baseb_wb_raw[2*ia+1]*t2;
    pwr+=r1*r1+r2*r2;
    re+= dash_wb_waveform[2*j  ]*r1+dash_wb_waveform[2*j+1]*r2;
    im+=-dash_wb_waveform[2*j  ]*r2+dash_wb_waveform[2*j+1]*r1;


/*
baseb_sho1[2*ia]=50*dash_wb_waveform[2*j  ]/ZZ;
baseb_sho1[2*ia+1]=50*dash_wb_waveform[2*j+1]/ZZ;
baseb_sho2[2*ia]=r1;
baseb_sho2[2*ia+1]=r2;
*/


    ia=ib;
    ib=(ib+1)&baseband_mask;
    }      
  reg_dash_re=re;
  reg_dash_im=im;
  reg_dash_power=pwr/(dash_wb_ib-dash_wb_ia);
  rpos+=cwbit_pts;

{
t1=rpos+(dash_wb_ib-dash_wb_ia)/2;
if(t1>baseband_size)t1-=baseband_size;
t2=ZZ*reg_dash_re*ZZ*reg_dash_re+ZZ*reg_dash_im*ZZ*reg_dash_im;
fprintf( dmp,"\nreg dashpos %f  re %f im %f   pwr %f %f", t1, ZZ*reg_dash_re, ZZ*reg_dash_im,ZZ*ZZ*reg_dash_power,t2);
}

  }

}


void get_wb_average_dashes(void)
{
int i, j, k, m, ia, ib;
int icw;
float t1, t2, r1, r2;
float re, im, avgpwr;
float *pwr;
// Make an avergage waveform of all dashes in the larger bandwidth.
dash_siz=4*cwbit_pts+4;
dash_siz&=-2;
j=dash_siz<<1;
dash_sizhalf=dash_siz>>1;
for(i=0; i<j; i++)fftn_tmp[i]=0;
pwr=&fftn_tmp[j];
for(icw=0; icw<no_of_cwdat; icw++)
  {
  if(cw[icw].type == CW_DASH)
    {
    re=cw[icw].raw_re;
    im=cw[icw].raw_im;
    t1=cw[icw].midpoint-dash_sizhalf+baseband_size;
    ia=t1;
    t1-=ia;
    t2=1-t1;
    ia &= baseband_mask;
    r2=sqrt(re*re+im*im);
    re/=r2;
    im/=r2;
    ib=ia;
    ib=(ia+1)&baseband_mask;
    for(j=0; j<dash_siz; j++)
      {
      r1=baseb_wb_raw[2*ib  ]*t1+baseb_wb_raw[2*ia  ]*t2;
      r2=baseb_wb_raw[2*ib+1]*t1+baseb_wb_raw[2*ia+1]*t2;
//fprintf( dmp,"\n%d  %d  %f  %f  %f %f ",icw,j,re,im, ZZ*r1,ZZ*r2);
      fftn_tmp[2*j  ]+=re*r1-im*r2;
      fftn_tmp[2*j+1]+=re*r2+im*r1;
      ia=ib;
      ib=(ib+1)&baseband_mask;
      }      
    }
  }
// Compute the power of the waveform, but divide by the midpoint
// amplitude first to avoid overflows.
t1=3/(fftn_tmp[dash_siz-2]+fftn_tmp[dash_siz]+fftn_tmp[dash_siz+2]);
for(i=0;i<dash_siz; i++)
  {
  fftn_tmp[2*i  ]*=t1;
  fftn_tmp[2*i+1]*=t1;
  pwr[i]=fftn_tmp[2*i]*fftn_tmp[2*i]+fftn_tmp[2*i+1]*fftn_tmp[2*i+1];
//fprintf( dmp,"\n%d %f ",i,pwr[i]);
  }
// The coherently averaged waveform should have a much better S/N than
// the original waveform. It is however computed in a large bandwidth
// so the points may scatter quite a bit due to noise.
// The power should ideally be constant over the duration of the
// dash. Some transmitters may give a somewhat higher power at the
// beginning before the voltage of the power amplifier dropped, but
// there is no reason to include such effects, the probability that
// an incorrect detect decision will be made due to this neglect 
// is extremely small.
// Ideally the full power should last 3*cwbit_pts symmetrically
// around the midpoint dash_sizhalf.
// Compute the power level as the average over 85% of this range.
ia=dash_sizhalf-1.5*0.85*cwbit_pts;
ib=dash_sizhalf+1.5*0.85*cwbit_pts+1;
avgpwr=0;
for(i=ia; i<ib; i++)avgpwr+=pwr[i];
avgpwr/=ib-ia;
// Step ia and ib outwards until the next point would have passed 
// the -3dB (half power) points 
t2=0.5*avgpwr;
while(ia > 0 && pwr[ia-1] > t2)ia--;
while(ib < dash_siz-1 && pwr[ib+1] > t2)ib++;
// Fit a second order polynomial to the imaginary part between ia and ib.
llsq_neq=ib-ia+1;
llsq_errors=&pwr[dash_siz];
llsq_derivatives=&llsq_errors[llsq_neq];
llsq_npar=3;
k=0;
m=(ib+ia)/2;
for(i=ia; i<=ib; i++)
  {
  llsq_derivatives[k]=1;
  llsq_derivatives[llsq_neq+k]=k;
  llsq_derivatives[2*llsq_neq+k]=(k-m)*(k-m);
  llsq_errors[k]=fftn_tmp[2*i+1];
  k++;
  }
if(llsq1() != 0)
  {
  lirerr(532330);
  return;
  }
dash_wb_ia=ia;
dash_wb_ib=ib+1;  
// Store the fitted second order function, compute
// its average and make the average zero.
t1=0;
k=0;
for(i=dash_wb_ia; i<dash_wb_ib; i++)
  {
  r1=llsq_steps[0]+llsq_steps[1]*k+llsq_steps[2]*(k-m)*(k-m);
  fftn_tmp[2*i+1]=r1;
  t1+=r1;
  k++;
  }
t1/=dash_wb_ib-dash_wb_ia;  
for(i=dash_wb_ia; i<dash_wb_ib; i++)
  {
  fftn_tmp[2*i+1]-=t1;
  }
// The imaginary part is our fitted curve, the power is avgpwr.
// Normalize the curve for the sum of amplitudes to 
// become 1.0 so we get the average amplitude of a dash by
// a multiplication with this function.
t2=1/sqrt(avgpwr);
t2/=(dash_wb_ib-dash_wb_ia);
for(i=dash_wb_ia; i<dash_wb_ib; i++)
  {
  r1=fftn_tmp[2*i+1];
  dash_wb_waveform[2*i+1]=r1*t2;
  dash_wb_waveform[2*i]=sqrt(avgpwr-r1*r1)*t2;
  }
for(i=0; i<2*dash_wb_ia; i++) dash_wb_waveform[i]=0;
for(i=2*dash_wb_ib; i<2*dash_siz; i++) dash_wb_waveform[i]=0;
//for(i=0; i<dash_siz; i++)fprintf( dmp,"\nDASH %d  %f  %f",i,dash_wb_waveform[2*i],dash_wb_waveform[2*i+1]);
// Use the new wb waveform to compute the real and imaginary parts
// of all known dashes using the original wide baseband signal 
// baseb_wb_raw (without use of coherent detection)
for(icw=0; icw<no_of_cwdat; icw++)
  {
  if(cw[icw].type == CW_DASH)
    {
    re=0;
    im=0;
    t1=cw[icw].midpoint-dash_sizhalf+baseband_size;
    ia=t1;
    t1-=ia;
    t2=1-t1;
    ia+=dash_wb_ia;
    ia &= baseband_mask;
    ib=ia;
    ib=(ia+1)&baseband_mask;
    for(j=dash_wb_ia; j<dash_wb_ib; j++)
      {
      r1=baseb_wb_raw[2*ib  ]*t1+baseb_wb_raw[2*ia  ]*t2;
      r2=baseb_wb_raw[2*ib+1]*t1+baseb_wb_raw[2*ia+1]*t2;
      re+= dash_wb_waveform[2*j  ]*r1+dash_wb_waveform[2*j+1]*r2;
      im+=-dash_wb_waveform[2*j  ]*r2+dash_wb_waveform[2*j+1]*r1;
      ia=ib;
      ib=(ib+1)&baseband_mask;
      }      
    cw[icw].raw_re=re;
    cw[icw].raw_im=im;
    }
  }
}

float collect_wb_ston(void)
{
int j, k, m, ia, ib, icw;
float s1, s2, t1, t2, r1, r2, r3, r4;
float re, im;
s1=0;
s2=0;
k=dash_wb_ib-dash_wb_ia;
m=0;
for(icw=0; icw<no_of_cwdat; icw++)
  {
  if(cw[icw].type == CW_DASH)
    {
    m++;
    t1=cw[icw].midpoint-dash_sizhalf+baseband_size;
    ia=t1;
    t1-=ia;
    t2=1-t1;
    ia+=dash_wb_ia;
    ia &= baseband_mask;
    ib=ia;
    ib=(ia+1)&baseband_mask;
    re=k*cw[icw].raw_re;
    im=k*cw[icw].raw_im;
    for(j=dash_wb_ia; j<dash_wb_ib; j++)
      {
      r1=baseb_wb_raw[2*ib  ]*t1+baseb_wb_raw[2*ia  ]*t2;
      r2=baseb_wb_raw[2*ib+1]*t1+baseb_wb_raw[2*ia+1]*t2;
      r3= dash_wb_waveform[2*j  ]*re+dash_wb_waveform[2*j+1]*im;
      r4=-dash_wb_waveform[2*j  ]*im+dash_wb_waveform[2*j+1]*re;
      s1+=r1*r1+r2*r2;
      s2+=(r1-r3)*(r1-r3)+(r2-r4)*(r2-r4);
      ia=ib;
      ib=(ib+1)&baseband_mask;
      }      
fprintf( dmp,"\ncollect_wb_ston: %d  %f  %f",m,ZZ*ZZ*s1, ZZ*ZZ*s2);
    }
  }
cw_stoninv=s2/s1;
seldash_cwspeed(0, no_of_cwdat);
return sqrt((float)(m))*s1/s2; 
}

void seldash_cwspeed(int da, int db)
{
int i2, ir3;
int i, j, k, strhalf;
int ia, ib, icw;
float a, r1, r2, rdiff, amin;
float re, im, t1, t2, t3, t4, t5, t6, t7;
float *z, *zm;
int aptr;
j=cw_avg_points<<1;
z=&fftn_tmp[j];
for(i=0; i<j; i++)fftn_tmp[i]=0;
strhalf=cw_avg_points>>1;
zm=&z[strhalf];
// Step over the selected dashes and accumulate their
// sum in fftn_tmp. Interpolate the position, we may 
// have a low sampling rate with few points on the rising and falling
// edges.
for(icw=da; icw<db; icw++)
  {
  if(cw[icw].type == CW_DASH)
    {
    re=cw[icw].coh_re;
    im=cw[icw].coh_im;
    t1=cw[icw].midpoint-strhalf+baseband_size;
    ia=t1;
    t1-=ia;
    t2=1-t1;
    ia &= baseband_mask;
    r2=sqrt(re*re+im*im);
    re/=r2;
    im/=r2;
    ib=ia;
    ib=(ia+1)&baseband_mask;
    for(j=0; j<(int)cw_avg_points; j++)
      {
      r1=baseb[2*ib  ]*t1+baseb[2*ia  ]*t2;
      r2=baseb[2*ib+1]*t1+baseb[2*ia+1]*t2;
      fftn_tmp[2*j  ]+=re*r1-im*r2;
      fftn_tmp[2*j+1]+=re*r2+im*r1;
      ia=ib;
      ib=(ib+1)&baseband_mask;
      }      
    }
  }
// The average waveform should have minima at both
// sides of the central dash. There should be other minima
// on both sides that correspond to positions of key up
// in case the dash is surrounded by dots, dual dots or dashes.  
// Form the symmetric component of the real part around
// the midpoint. We have no interest in the individual minima
// at both sides, we just want the average - and we assume
// the carrier phase is reasonably good so the imaginary part
// is just noise.
j=cw_avg_points-2;
k=cw_avg_points;
for(i=0; i<strhalf; i++)
  {
  z[i]=fftn_tmp[j]+fftn_tmp[k];
  k+=2;
  j-=2;
  }
// The zero point of the symmetric curve is at -0.5
// Set up pointers t1,t2,it3 that point to the minima and
// that relate as the expected minimum positions should do.
// r1+0.5 = 2*cwbit_pts
// r2+0.5 = 4*cwbit_pts
// ir3+0.5 = 6*cwbit_pts
// We expect a minimum for ir3=6*cwbit_pts-0.5; 
// Step a wide range around this position
ia=3*cwbit_pts-0.25;
ib=12*cwbit_pts;
if(ib > strhalf)ib=strhalf;
amin=BIGFLOAT;
aptr=0;
for(ir3=ia; ir3<ib; ir3++)
  {
  r1=(ir3+0.5)/3;
  r2=2*r1-0.5;
  r1-=0.5;
// The first point at r1
  i2=r1;
  rdiff=r1-i2;
// Use Lagrange's interpolation formula to fit a third degree
// polynomial to 4 points:
//  z[r1]=-rdiff  *  (rdiff-1)*(rdiff-2)*z[i1]/6
//        +(rdiff+1)*(rdiff-1)*(rdiff-2)*z[i2]/2
//        -(rdiff+1)* rdiff   *(rdiff-2)*z[i3]/2
//        +(rdiff+1)* rdiff   *(rdiff-1)*z[i4]/6; 
// Rewrite slightly to save a few multiplications - do not
// think the compiler is smart enough to do it for us.
  t1=rdiff-1;
  t2=rdiff-2;
  t3=rdiff+1;
  t4=t1*t2;
  t5=t3*rdiff;
  t6=rdiff*t4;
  t4=t3*t4;
  t7=t5*t2;
  t5=t5*t1;
  a=((t5*z[i2+2]-t6*z[i2-1])/3+t4*z[i2]-t7*z[i2+1]);
// The second point at r2
  i2=r2;
  rdiff=r2-i2;
// Use Lagrange's interpolation formula to fit a third degree
// polynomial to 4 points:
//  z[r1]=-rdiff  *  (rdiff-1)*(rdiff-2)*z[i1]/6
//        +(rdiff+1)*(rdiff-1)*(rdiff-2)*z[i2]/2
//        -(rdiff+1)* rdiff   *(rdiff-2)*z[i3]/2
//        +(rdiff+1)* rdiff   *(rdiff-1)*z[i4]/6; 
// Rewrite slightly to save a few multiplications - do not
// think the compiler is smart enough to do it for us.
  t1=rdiff-1;
  t2=rdiff-2;
  t3=rdiff+1;
  t4=t1*t2;
  t5=t3*rdiff;
  t6=rdiff*t4;
  t4=t3*t4;
  t7=t5*t2;
  t5=t5*t1;
  a+=((t5*z[i2+2]-t6*z[i2-1])/3+t4*z[i2]-t7*z[i2+1]);
// The third point at ir3 (note we did not divide by 2 above, do it now)
  a+=0.5*a+z[ir3];
  zm[ir3]=a;
  if(a > 1.5*amin)goto amin_x;
  if(a < amin)
    {
    amin=a;
    aptr=ir3;
    }
  }
amin_x:;
t1=-zm[aptr-1];
t2=-amin;
t3=-zm[aptr+1];
parabolic_fit(&a,&t7,t1,t2,t3);   
if(store_symmetry_adapted_dash(FALSE) == TRUE)
  {
  cwbit_pts = (t7+aptr+0.5)/6;
  fprintf( dmp,"\ncwbit_pts %f",cwbit_pts);
  }
else
  {
  cw_detect_flag=CWDETECT_ERROR;
  }
}


void make_dot_siz(void)
{
dot_siz=2*cwbit_pts+4;
dot_siz&=-2;
dot_sizhalf=dot_siz>>1;
}

void guess_wb_average_dots(void)
{
int i, k, ia, ib;
float t1, t2, t3, r1, r2;
// Make a guess for the dot waveform. We simply assume that the
// waveform length is one third of the dash waveform and that
// the chirp is the same as at the beginning of a dash.
make_dot_siz();
k=(2+dash_wb_ib-dash_wb_ia)/3;
dot_wb_ia=dot_sizhalf-(k+1)/2;
dot_wb_ib=dot_wb_ia+k;
for(i=0; i<2*dot_wb_ia; i++) dot_wb_waveform[i]=0;
for(i=2*dot_wb_ib; i<2*dot_siz; i++) dot_wb_waveform[i]=0;
ia=dot_wb_ia;
ib=dash_wb_ia;
t1=0;
t2=0;
t3=0;
while(ia<dot_wb_ib)
  {
  t1+=fftn_tmp[2*ib];
  dot_wb_waveform[2*ia]=fftn_tmp[2*ib];
  t2+=fftn_tmp[2*ib+1];
  dot_wb_waveform[2*ia+1]=fftn_tmp[2*ib+1];
  t3+=fftn_tmp[2*ib]*fftn_tmp[2*ib]+fftn_tmp[2*ib+1]*fftn_tmp[2*ib+1];
  ia++;
  ib++;
  }
r1=t1*t1+t2*t2;
r1=sqrt(r1);
t1/=r1;
t2/=r1;
t3=sqrt(t3*(dot_wb_ib-dot_wb_ia));
for(i=dot_wb_ia; i<dot_wb_ib; i++)
  {
  r1=dot_wb_waveform[2*i  ]/t3;
  r2=dot_wb_waveform[2*i+1]/t3;
  dot_wb_waveform[2*i  ]= r1*t1+r2*t2;
  dot_wb_waveform[2*i+1]=-r2*t1+r1*t2;
  k++;
  }
}

void get_wb_average_dots(void)
{
int i, j, k, ia, ib, dotno;
int icw;
float t1, t2, r1, r2;
float re, im, avgpwr;
float *pwr;
// Make an avergage waveform of all dots in the larger bandwidth.
make_dot_siz();
j=dot_siz<<1;
for(i=0; i<j; i++)fftn_tmp[i]=0;
pwr=&fftn_tmp[j];
dotno=0;
for(icw=0; icw<no_of_cwdat; icw++)
  {
  if(cw[icw].type == CW_DOT)
    {
    dotno++;
    re=cw[icw].raw_re;
    im=cw[icw].raw_im;
    t1=cw[icw].midpoint-dot_sizhalf+baseband_size;
    ia=t1;
    t1-=ia;
    t2=1-t1;
    ia &= baseband_mask;
    r2=sqrt(re*re+im*im);
    re/=r2;
    im/=r2;
    ib=ia;
    ib=(ia+1)&baseband_mask;
    for(j=0; j<dot_siz; j++)
      {
      r1=baseb_wb_raw[2*ib  ]*t1+baseb_wb_raw[2*ia  ]*t2;
      r2=baseb_wb_raw[2*ib+1]*t1+baseb_wb_raw[2*ia+1]*t2;
      fftn_tmp[2*j  ]+=re*r1-im*r2;
      fftn_tmp[2*j+1]+=re*r2+im*r1;
      ia=ib;
      ib=(ib+1)&baseband_mask;
      }      
    }
  }
if(dotno < 4)return;  
// Compute the power of the waveform, but divide by the midpoint
// amplitude first to avoid overflows.
t1=3/(fftn_tmp[dot_siz-2]+fftn_tmp[dot_siz]+fftn_tmp[dot_siz+2]);
for(i=0;i<dot_siz; i++)
  {
  fftn_tmp[2*i  ]*=t1;
  fftn_tmp[2*i+1]*=t1;
  pwr[i]=fftn_tmp[2*i]*fftn_tmp[2*i]+fftn_tmp[2*i+1]*fftn_tmp[2*i+1];
  }
// The coherently averaged waveform should have a much better S/N than
// the original waveform. It is however computed in a large bandwidth
// so the points may scatter quite a bit due to noise.
// The power should ideally be constant over the duration of the
// dot. Some transmitters may give a somewhat higher power at the
// beginning before the voltage of the power amplifier dropped, but
// there is no reason to include such effects, the probability that
// an incorrect detect decision will be made due to this neglect 
// is extremely small.
// Ideally the full power should last 3*cwbit_pts symmetrically
// around the midpoint dot_sizhalf.
// Compute the power level as the average over 85% of this range.
ia=dot_sizhalf-.5*0.85*cwbit_pts;
ib=dot_sizhalf+.5*0.85*cwbit_pts+1;
avgpwr=0;
for(i=ia; i<ib; i++)avgpwr+=pwr[i];
avgpwr/=ib-ia;
// Step ia and ib outwards until the next point would have passed 
// the -3dB (half power) points 
t2=0.5*sqrt(avgpwr);
while(ia > 0 && pwr[ia-1] > t2)ia--;
while(ib < dot_siz-1 && pwr[ib+1] > t2)ib++;
// Fit a straight line to the imaginary part between ia and ib.
llsq_neq=ib-ia+1;
llsq_errors=&pwr[dot_siz];
llsq_derivatives=&llsq_errors[llsq_neq];
llsq_npar=2;
k=0;
for(i=ia; i<=ib; i++)
  {
  llsq_derivatives[k]=1;
  llsq_derivatives[llsq_neq+k]=k;
  llsq_errors[k]=fftn_tmp[2*i+1];
  k++;
  }
if(llsq1() != 0)
  {
  lirerr(532330);
  return;
  }
dot_wb_ia=ia;
dot_wb_ib=ib+1;  
// Store the fitted line, compute
// its average and make the average zero.
t1=0;
k=0;
for(i=dot_wb_ia; i<dot_wb_ib; i++)
  {
  r1=llsq_steps[0]+llsq_steps[1]*k;
  fftn_tmp[2*i+1]=r1;
  t1+=r1;
  k++;
  }
t1/=dot_wb_ib-dot_wb_ia;  
for(i=dot_wb_ia; i<dot_wb_ib; i++)
  {
  fftn_tmp[2*i+1]-=t1;
  }
// The imaginary part is our fitted curve, the power is avgpwr.
// Normalize the curve for the sum of amplitudes to 
// become 1.0 so we get the average amplitude of a dash by
// a multiplication with this function.
t2=1/sqrt(avgpwr);
t2/=(dot_wb_ib-dot_wb_ia);
for(i=dot_wb_ia; i<dot_wb_ib; i++)
  {
  r1=fftn_tmp[2*i+1];
  dot_wb_waveform[2*i+1]=r1*t2;
  dot_wb_waveform[2*i]=sqrt(avgpwr-r1*r1)*t2;
  }
for(i=0; i<2*dot_wb_ia; i++) dot_wb_waveform[i]=0;
for(i=2*dot_wb_ib; i<2*dot_siz; i++) dot_wb_waveform[i]=0;
// Use the new wb waveform to compute the real and imaginary parts
// of all known dots using the original wide baseband signal 
// baseb_wb_raw (without use of coherent detection)
for(icw=0; icw<no_of_cwdat; icw++)
  {
  if(cw[icw].type == CW_DOT)
    {
    re=0;
    im=0;
    t1=cw[icw].midpoint-dot_sizhalf+baseband_size;
    ia=t1;
    t1-=ia;
    t2=1-t1;
    ia+=dot_wb_ia;
    ia &= baseband_mask;
    ib=ia;
    ib=(ia+1)&baseband_mask;
    for(j=dot_wb_ia; j<dot_wb_ib; j++)
      {
      r1=baseb_wb_raw[2*ib  ]*t1+baseb_wb_raw[2*ia  ]*t2;
      r2=baseb_wb_raw[2*ib+1]*t1+baseb_wb_raw[2*ia+1]*t2;
      re+= dot_wb_waveform[2*j  ]*r1+dot_wb_waveform[2*j+1]*r2;
      im+=-dot_wb_waveform[2*j  ]*r2+dot_wb_waveform[2*j+1]*r1;
      ia=ib;
      ib=(ib+1)&baseband_mask;
      }      
    cw[icw].raw_re=re;
    cw[icw].raw_im=im;
    }
  }
}

