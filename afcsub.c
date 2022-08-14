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


void collect_initial_spectrum(void)
{
// char s[80];
int i, j, k, m, ia, ib, ja, jb, ka, kb, first_fqp, fqp;
int np, lowres_points;
int nn,na,nb,ma,mb,n1,n2,n3;
int pol_known;
int fq_points;
float drifti, sina;
float t1, t2, t3, t4, r1, r2, drift, maxval2, minval;
float der;
float noi2,x2s, y2s;
float *pwra;
float fq, ampl[3], pos[3];
int kk, res, imax, jmax, lowres_imax, imax2, jmax2 ;
float x2, y2, re_xy, im_xy;
float *x;
short int * zxy;
TWOCHAN_POWER *pxy;
if(pg.adapt == 0 && ui.rx_rf_channels == 2)pol_known = 0; else pol_known=1;
afcf_search_range=genparm[AFC_LOCK_RANGE]*fftx_points_per_hz*ag.search_range;
if(afcf_search_range < 12)afcf_search_range=12;
fq_points=1.1*afcf_search_range;
if(fq_points < afcf_search_range+20)fq_points=afcf_search_range+20;
if(fq_points > max_afcf_points)
  {
  fq_points = max_afcf_points;
  afcf_search_range=fq_points-20;
  }
afc_ia=(fq_points-afcf_search_range)/2;
afc_ib=fq_points-afc_ia;
search_again:;
afc_noise=-1;
// Collect summed power spectra with appropriate frequency shift
first_fqp=mix1_selfreq[ag_ss]*fftx_points_per_hz-fq_points/2;
np=(fftx_na-1+max_fftxn)&fftxn_mask;
drifti=0;
kk=fq_points;
if(sw_onechan)
  {
// With one channel only, just use the power spectrum.  
  for(i=0; i<fq_points*afc_speknum; i++)afc_spectrum[i]=0;
  for(i=0; i<ag.fit_points; i++)
    {    
    drift=-drifti*afc_half_speknum;
    pwra=&fftx_pwr[np*fftx_size];
    for(j=0; j<afc_speknum; j++)
      {
      fqp=first_fqp+drift; 
      for(k=0; k<kk; k++)
        {
        afc_spectrum[fq_points*j+k]+=pwra[k+fqp];
        }
      drift+=drifti;
      }
    np=(np+fftxn_mask)&fftxn_mask;
    drifti+=afc_drift_step;
    }
  }
else  //ui.rx_rf_channels = 2
  {
  if(pol_known == 0)
    {
// We have two channels and polarization is unknown.
// First make an average of channel powers and correlations assuming
// the different frequency drifts we look for.    
    for(i=0; i<fq_points*afc_speknum; i++)
      {
      afc_xysum[i].x2=0;
      afc_xysum[i].y2=0;
      afc_xysum[i].re_xy=0;
      afc_xysum[i].im_xy=0;
      }
    for(i=0; i<ag.fit_points; i++)
      {    
      drift=-drifti*afc_half_speknum;
      pxy=(TWOCHAN_POWER*)(&fftx_xypower[np*fftx_size].x2);
      for(j=0; j<afc_speknum; j++)
        {
        fqp=first_fqp+drift; 
        for(k=0; k<kk; k++)
          {
          afc_xysum[fq_points*j+k].x2+=pxy[k+fqp].x2;
          afc_xysum[fq_points*j+k].y2+=pxy[k+fqp].y2;
          afc_xysum[fq_points*j+k].re_xy+=pxy[k+fqp].re_xy;
          afc_xysum[fq_points*j+k].im_xy+=pxy[k+fqp].im_xy;
          }
        drift+=drifti;
        }
      np=(np+fftxn_mask)&fftxn_mask;
      drifti+=afc_drift_step;
      }
// Now extract the power for a signal using the channel correlation
// (Slightly better than just adding x2 and y2)
    for(i=0; i<fq_points*afc_speknum; i++)
      {
      t1=afc_xysum[i].x2+afc_xysum[i].y2;
      afc_spectrum[i]=t1 + 2*(afc_xysum[i].re_xy*afc_xysum[i].re_xy
                             +afc_xysum[i].im_xy*afc_xysum[i].im_xy
                             -afc_xysum[i].x2*afc_xysum[i].y2 ) / t1;
      }
    }
  else
    {
// When the polarization is known we have just one channel.
// The power spectra have to be calculated directly from the transforms
// since the channel is a linear combination of two channels.
    for(i=0; i<fq_points*afc_speknum; i++)afc_spectrum[i]=0;
    if(swmmx_fft2)
      {
      for(i=0; i<ag.fit_points; i++)
        {    
        drift=-drifti*afc_half_speknum;
        zxy=&fft2_short_int[4*np*fft2_size];
        for(j=0; j<afc_speknum; j++)
          {
          fqp=first_fqp+drift; 
          for(k=0; k<kk; k++)
            {
            m=4*(k+fqp);
            t1=pg.c1*zxy[m  ]+pg.c2*zxy[m+2]+pg.c3*zxy[m+3];
            t2=pg.c1*zxy[m+1]+pg.c2*zxy[m+3]-pg.c3*zxy[m+2];
            afc_spectrum[fq_points*j+k]+=t1*t1+t2*t2;
            }
          drift+=drifti;
          }
        np=(np+fftxn_mask)&fftxn_mask;
        drifti+=afc_drift_step;
        }
      }
    else
      {
      for(i=0; i<ag.fit_points; i++)
        {    
        drift=-drifti*afc_half_speknum;
        x=&fftx[np*fftx_size*4];
        for(j=0; j<afc_speknum; j++)
          {
          fqp=first_fqp+drift; 
          for(k=0; k<kk; k++)
            {
            m=4*(k+fqp);
            t1=pg.c1*x[m  ]+pg.c2*x[m+2]+pg.c3*x[m+3];
            t2=pg.c1*x[m+1]+pg.c2*x[m+3]-pg.c3*x[m+2];
            afc_spectrum[fq_points*j+k]+=t1*t1+t2*t2;
            }
          drift+=drifti;
          }
        np=(np+fftxn_mask)&fftxn_mask;
        drifti+=afc_drift_step;
        }
      }
    }
  }
// Look for the largest value.
afc_maxval=-1;
imax=0;
jmax=0;
ia=afc_ia;
ib=afc_ib;
// In case the user has zoomed in very much the search range
// may reach outside the permitted frequency range.
// Adjust ia and ib in case they are too far out.
j=mix1_lowest_fq*fftx_points_per_hz-first_fqp+5;
if( ia< j)ia=j; 
j=mix1_highest_fq*fftx_points_per_hz-first_fqp-5;
if( ib > j)ib=j; 
for(j=0; j<afc_speknum; j++)
  {
  for(k=ia; k<ib; k++)
    {
    if(afc_maxval<afc_spectrum[fq_points*j+k])
      {
      afc_maxval=afc_spectrum[fq_points*j+k];
      imax=k;
      jmax=j;
      }
    }
  }
// Assuming the operator selected a suitable baseband filter we look
// outside the region of the current signal to see if there is another
// candidate.
// This time we look at the spectrum with a resolution
// corresponding to 25% of the baseband filter.
res=baseband_bw_fftxpts/2;
if(res < 2 || fq_points < 6*baseband_bw_fftxpts)goto desicion_made;
//ööö  Check this when afc_speknum == 1
lowres_points=fq_points/res;
if(pol_known == 1)
  {
// For a single channel just add neighbouring points.
  for(j=0; j<afc_speknum; j++)
    {
    ja=0;
    i=0;
    for(jb=res; jb<fq_points; jb+=res)
      {
      afc_lowres_spectrum[i]=0;
      for(k=ja; k<jb; k++)
        {
        afc_lowres_spectrum[i]+=afc_spectrum[fq_points*j+k];
        }
      ja=jb;
      i++;
      }
    }
  }  
else
  { 
// We have two channels and the polarization is unknown.
// Average individual channels and correlations first, then
// calculate power!
    for(j=0; j<afc_speknum; j++)
      {
      ja=0;
      i=0;
      for(jb=res; jb<fq_points; jb+=res)
        {
        t1=t2=t3=t4=0;
        for(k=ja; k<jb; k++)
          {
          t1+=afc_xysum[fq_points*j+k].x2;
          t2+=afc_xysum[fq_points*j+k].y2;
          t3+=afc_xysum[fq_points*j+k].re_xy;
          t4+=afc_xysum[fq_points*j+k].im_xy;
          }
        afc_lowres_spectrum[i]=t1+t2+2*(t3*t3+t4*t4-t1*t2)/(t1+t2);
        ja=jb;
        i++;
        }
      }
    }
minval=BIGFLOAT;    
for(i=1; i<lowres_points-1; i++)
  {
  if(minval > afc_lowres_spectrum[i])
    {
    minval=afc_lowres_spectrum[i];
    }    
  }
t1=-1;
lowres_imax=0;
ja=(imax-baseband_bw_fftxpts)/res;
jb=(imax+baseband_bw_fftxpts)/res;
ia/=res;
ib/=res;
for(i=ia; i<ja; i++)
  {
  if(t1 < afc_lowres_spectrum[i])
    {
    t1=afc_lowres_spectrum[i];
    lowres_imax=i;
    }    
  }
for(i=jb; i<ib; i++)
  {
  if(t1 < afc_lowres_spectrum[i])
    {
    t1=afc_lowres_spectrum[i];
    lowres_imax=i;
    }
  }      
if(minval < 0.02*t1)minval=0.02*t1;  
lowres_imax*=res;
minval/=afc_speknum*res;
// Another signal seems better at a larger bandwidth.
// This one may be the desired signal while the other one may be
// a frequency stable and unmodulated spur.
// Get max point and frequency drift of second candidate.
ka=lowres_imax-baseband_bw_fftxpts;
kb=lowres_imax+baseband_bw_fftxpts; 
maxval2=-1;
imax2=0;
jmax2=0;
for(j=0; j<afc_speknum; j++)
  {
  for(k=ka; k<kb; k++)
    {
    if(maxval2<afc_spectrum[fq_points*j+k])
      {
      maxval2=afc_spectrum[fq_points*j+k];
      imax2=k;
      jmax2=j;
      }
    }
  }
// Get an estimate of the noise floor as the average of points that
// are smaller than 5*minval. For verification, get also the
// average of points that are in the interval 5*minval to 25*minval
if(imax2 > imax)
  {
  na=ja;
  nb=jb;
  ma=ka;
  mb=kb;
  }
else
  {
  na=ka;
  nb=kb;
  ma=ja;
  mb=jb;
  }
t1=0;
t2=0;
n1=0;
n2=0;
r1=5*minval;
r2=25*minval;
kk=0;
for(j=0; j<afc_speknum; j++)
  {
  for(k=0; k<na; k++)
    {
    if( afc_spectrum[kk+k] < r2)
      {
      if( afc_spectrum[kk+k] < r1)
        {
        t1+=afc_spectrum[kk+k];
        n1++;
        }
      else
        {
        t2+=afc_spectrum[kk+k];
        n2++;
        }
      }
    }
      
  for(k=nb+1; k<ma; k++)
    {
    if( afc_spectrum[kk+k] < r2)
      {
      if( afc_spectrum[kk+k] < r1)
        {
        t1+=afc_spectrum[kk+k];
        n1++;
        }
      else
        {
        t2+=afc_spectrum[kk+k];
        n2++;
        }
      }
    }
  for(k=mb+1; k<fq_points; k++)
    {
    if( afc_spectrum[kk+k] < r2)
      {
      if( afc_spectrum[kk+k] < r1)
        {
        t1+=afc_spectrum[kk+k];
        n1++;
        }
      else
        {
        t2+=afc_spectrum[kk+k];
        n2++;
        }
      }
    }
  kk+=fq_points;
  }
t3=t1+t2;
n3=n1+n2;
// If noise floor could not be established, just use maximum.
if(n3 < 12+(3+fq_points/20)*afc_speknum/2)goto desicion_made; 
if(n2 > n3/3)
  {
  afc_noise=t3/n3;
  }
else
  {
  afc_noise=t1/n1;
  }  
// If the second signal is not as high above the noise floor
// as the operator specified, use the first signal.
if(maxval2 < ag.minston*afc_noise)goto desicion_made;
// Both signals are strong enough.
// Get the modulation sidebands
t1=2*afc_noise;
for(i=0; i<res; i++)
  {
  afc_lowres_spectrum[2*i  ]=afc_spectrum[fq_points*jmax+imax+i]+
              afc_spectrum[fq_points*jmax+imax-i]-t1;
  afc_lowres_spectrum[2*i+1]=afc_spectrum[fq_points*jmax2+imax2+i]+
              afc_spectrum[fq_points*jmax2+imax2-i]-t1;
  }
// This routine is currently for Morse code only.
// Besides a carrier there should be keying sidebands.
// The signal may be phase unstable and smeared out so be careful.
// Get the center of gravity for the signals.
t1=0;
t2=0;
r1=0;
r2=0;
for(i=0; i<res; i++)
  {
  if( afc_lowres_spectrum[2*i  ] <0) afc_lowres_spectrum[2*i  ]=0;
  t1+=i*afc_lowres_spectrum[2*i  ];
  r1+=afc_lowres_spectrum[2*i  ];
  if( afc_lowres_spectrum[2*i+1] <0) afc_lowres_spectrum[2*i+1]=0;
  t2+=i*afc_lowres_spectrum[2*i+1];
  r2+=afc_lowres_spectrum[2*i+1];
  }
t1/=r1*res;
t2/=r2*res;
// An unmodulated spur is typically below 0.05 while a 
// normal keyed signal is in the region 0.1 to 0.3
if(t2 > 0.1 && t1 < 0.05)
  {
use2:;  
  imax=imax2;
  jmax=jmax2;
  afc_maxval=maxval2;
  goto desicion_made;
  }
if(t1 > 0.1 && t2 < 0.1)goto desicion_made;
if(t1 < 0.05 && t2 < 0.05)goto desicion_made;
if(t1 > 0.1 && t2 > 0.1)
  {
  if(maxval2/afc_maxval < 0.25)goto desicion_made;
  if(fabs(t1-0.2)/(fabs(t2-0.2)+0.000001)>2)goto use2;
  }
if(t2/t1 > 2)goto use2;
desicion_made:;
// We now have imax and jmax pointing to a signal.
// We may have a noise floor, but if it is not present, get it now.
if(afc_noise < 0)
  {
  minval=BIGFLOAT;
  kk=fq_points*jmax;
  for(i=0; i<fq_points; i++)
    {
    if(afc_spectrum[kk+i]<minval)minval=afc_spectrum[kk+i];
    }
  t1=minval*5;
  if(t1 < 0.1*afc_maxval)t1=0.1*afc_maxval;
// use 5* lowest or 0.1* maximum to get some idea about the noise level
  afc_noise=0;
  j=0;
  for(i=0; i<fq_points; i++)
    {
    if(afc_spectrum[kk+i]<t1)
      {
      afc_noise+=afc_spectrum[kk+i];
      j++;
      }
    }
  if(j == 0)goto errexit;  
  t1=afc_noise*5./j;  
// Include points within 5 times the average we obtained in the
// final estimate of the noise floor.
  afc_noise=0;
  j=0;
  for(i=0; i<fq_points; i++)
    {
    if(afc_spectrum[kk+i]<t1)
      {
      afc_noise+=afc_spectrum[kk+i];
      j++;
      }
    }
  if(j == 0)goto errexit;  
  afc_noise/=j;
  }
if(afc_maxval < ag.minston*afc_noise)
  {
// xytext(90,4,"S/N too low       ");  
// If the signal is poor, store a constant frequency
// and return failure code.
errexit:;
  der=afc_half_speknum;
  fq=0;
  mix1_status[ag_ss]=1;
  goto fillfq;
  }
mix1_status[ag_ss]=2;
// The signal is good enough.
if(pol_known == 0)
  {
// If we have two channels and have found the signal from
// the unpolarised power spectrum.
// (Actually slightly better using correlations!)  
// We can determine the polarization from channel correlations
// at the peak.
// get the S/N weighted average over the peak.
  kk=fq_points*jmax;
  ia=imax-res;
  ib=imax+res;
  x2=0;
  y2=0;
  re_xy=0;
  im_xy=0;
  for(i=ia; i<=ib; i++)
    {
    t1=(afc_xysum[kk+i].x2+afc_xysum[kk+i].y2)-afc_noise;
    if(t1 > 0)
      {
      x2+=t1*afc_xysum[kk+i].x2;
      y2+=t1*afc_xysum[kk+i].y2;
      re_xy+=t1*afc_xysum[kk+i].re_xy;
      im_xy+=t1*afc_xysum[kk+i].im_xy;
      }
    }
  t1=x2+y2;
  x2/=t1;
  y2/=t1;
  re_xy/=t1;
  im_xy/=t1;
// Now we have x2,y2 (real values) and xy (complex).
// For explanation purposes, assume im_xy == 0, which corresponds to linear
// polarization. The signal vill then be polarised in a plane.
// a = angle between polarization plane and the horisontal antenna.
// Assume that the noise level n is the same in the two antennas, and that
// the noise is uncorrelated.
// We then find:
//             x2 = cos(a)**2 + n**2
//             y2 = sin(a)**2 + n**2
//             xy = sin(a)*cos(a)
// From this we find: x2 * y2 - xy*xy = n**2 + n**4
// Neglect n**4:
// cos(a)=sqr( x2 - ( x2 * y2 - xy*xy) )
// sin(a)=sqr( y2 - ( x2 * y2 - xy*xy) )
// The transformation formula to use for rotating the polarization
// plane to produce new signals A and B, where A has all the signal and B
// only noise, will then be:
//  A = X * cos(a) + Y * sin(a)
//  B = Y * cos(a) - X * sin(a)
// Extending to im_xy != 0 the transformation becomes
// re_A=C1*re_X+C2*re_Y-C3*im_Y
// im_A=C1*im_X+C2*im_Y+C3*re_Y
// re_B=C1*re_Y-C2*re_X-C3*im_X
// im_B=C1*im_Y-C2*im_X+C3*re_X
// C1 = cos(a)
// C2 = sin(a) * re_xy / sqr( re_xy**2 + im_xy**2)
// C3 = sin(a) * im_xy / sqr( re_xy**2 + im_xy**2)
  t2=re_xy*re_xy+im_xy*im_xy;
  noi2=x2*y2-t2;
  x2s=x2-noi2;
  if(x2s > 0)
    {
    y2s=y2-noi2;
    pg.c1=sqrt(x2s);
    if(y2s > 0 && t2 > 0)
      {
      sina=sqrt(y2s);
      pg.c2=sina*re_xy/sqrt(t2);
      pg.c3=-sina*im_xy/sqrt(t2);
      t1=sqrt(pg.c1*pg.c1+pg.c2*pg.c2+pg.c3*pg.c3)+0.000001;
      if(pg.c2 < 0)t1=-t1;
      pg.c1/=t1;
      pg.c2/=t1;
      pg.c3/=t1;
      }
    else 
      {
      if(x2 > y2)
        {      
        pg.c1=1;
        pg.c2=0;
        }
      else
        {  
        pg.c1=0;
        pg.c2=1;
        }
      pg.c3=0;
      }  
    }
  else 
    {
    pg.c1=1;
    pg.c2=0;
    pg.c3=0;
    }
  if(recent_time-show_pol_time > 0.1)
    {  
    sc[SC_SHOW_POL]++;
    show_pol_time=current_time();
    }
// Set the flag and search again. This time using the correct optimum
// polarization only.
  pol_known = 1;
  goto search_again;
  }
// Store different frequencies for each transform if signal drifts.
if(jmax == 0 || jmax == afc_speknum-1)
  {
// Here it would perhaps be appropriate to add interpolation between
// two spectra, but poor fit for fast drifting signals is no problem,
// the user just specified inadequate drift range!   
  der=jmax;
  fq=imax;
  goto fillfq;
  }
// Fit a parabola to the peak in the best spectrum and in the
// surrounding ones by use of 3 points.
nn=0;
for(j=jmax-1; j<=jmax+1; j++)
  {
  ja=imax-2;
  jb=imax+2;
  t1=0; 
  k=imax;
  kk=fq_points*j;
  for(i=ja; i<=jb; i++)
    {
    if(afc_spectrum[kk+i]>t1)
      {
      t1=afc_spectrum[kk+i];
      k=i;
      }
    }  
  parabolic_fit(&ampl[nn],&pos[nn],afc_spectrum[kk+k-1],
                               afc_spectrum[kk+k],afc_spectrum[kk+k+1]);   
  pos[nn]+=k;
  nn++;
  }
parabolic_fit(&t1,&t2,ampl[0],ampl[1],ampl[2]);
der=jmax+t2;
// Interpolate peak position between spectra
if(t2 < 0)
  {
  fq=(t2+1)*pos[1]-t2*pos[0];
  }
else
  {
  fq=(1-t2)*pos[1]+t2*pos[2];
  }  
fillfq:;
fq=(fq+first_fqp)/fftx_points_per_hz;
der=(afc_half_speknum-der)*afc_drift_step/fftx_points_per_hz;
// Now fill the frequencies in  mix1_fq_mid[].
np=fftx_na;
fq+=der;
der/=2;
for(i=0; i<max_fftxn; i++)
  {    
  if(fq < mix1_lowest_fq)fq = mix1_lowest_fq;
  if(fq > mix1_highest_fq)fq = mix1_highest_fq;
  mix1_fq_mid[mix1p0+np]=fq;
  mix1_fq_curv[mix1p0+np]=0;
  mix1_fq_slope[mix1p0+np]=der;
  fq-=der;
  if(fq < mix1_lowest_fq)fq = mix1_lowest_fq;
  if(fq > mix1_highest_fq)fq = mix1_highest_fq;
  mix1_fq_start[mix1p0+np]=fq;
  fq-=der;
  np=(np+fftxn_mask)&fftxn_mask;
  }
}







int make_afc_signoi(void)
{
int i, k, np,j;
float t1,t2;
// Estimate S/N for the signal we see in mix1_eval data.
// Last time we used ag.fit_points blocks of eval data to evaluate
// the signal frequency. We estimate S/N over a similar time span.
// Each frequency in eval data is obtained from an average over
// afct_avgnum fft1 or fft2 blocks but this value may have changed so there
// may be a few points more or less.
np=(fftx_na-ag.fit_points-afct_avgnum+max_fftxn)&fftxn_mask;
t2=0;
k=0;
i=np;
while(i != fftx_na)
  {
  if(mix1_eval_sigpwr[mix1p0+i]>0)
    {
    k++;
    t2+=mix1_eval_noise[mix1p0+i];
    }
  i=(i+1)&fftxn_mask;
  }
if(k == 0)return 0;
t2/=k;
// Get the AFC noise floor as the average of noise levels that are
// within 3 times the average of the valid data points.
afc_noise=0;
k=0;
t2*=3;
i=np;
while(i != fftx_na)
  {
  if(mix1_eval_avgn[mix1p0+i]>0)
    {
    if(mix1_eval_noise[mix1p0+i]<t2)
      {
      k++;
      afc_noise+=mix1_eval_noise[mix1p0+i];
      }
    }
  i=(i+1)&fftxn_mask;
  }
if(k == 0)return 0;  
afc_noise/=k;
// Get the amplitude of the signal as the average of
// points with (S+N)/N above 3dB
t1=0;
t2=2*afc_noise;
k=0;
i=np;
while(i != fftx_na)
  {
  if(mix1_eval_avgn[mix1p0+i]>0)
    {
    if(mix1_eval_sigpwr[mix1p0+i]>t2)
      {
      k++;
      t1+=mix1_eval_sigpwr[mix1p0+i];
      }
    }
  i=(i+1)&fftxn_mask;
  }
if(k == 0)return 0;  
t1/=k;
// Get afc_ampl from the points that are above 25% of the average
// or have (S+N(avg))/N(avg) larger than 10dB.
// Exclude points with (S+N)/N < 3dB+3dB/sqrt(avgn)
// the same as:  0.5*(S+N) < (1+sqrt(avgn))*N

afc_maxval=0;
t2=10*afc_noise;
if(t2 > 0.25*t1)t2=0.25*t1;
k=0;
i=np;
j=0;
while(i != fftx_na)
  {
  j++;
  if(mix1_eval_avgn[mix1p0+i]>0)
    {
    if(mix1_eval_sigpwr[mix1p0+i]>t2)
      {
      if(0.5*mix1_eval_sigpwr[mix1p0+i] > 
                (sqrt(mix1_eval_avgn[mix1p0+i])+1)*mix1_eval_noise[mix1p0+i])
        {
        k++;
        afc_maxval+=mix1_eval_sigpwr[mix1p0+i];
        } 
      }
    }
  i=(i+1)&fftxn_mask;
  }
if(k == 0)return 0;  
afc_maxval/=k;
return 1;      
}


void make_ag_point(int np, int nno)
{
int i, j, k, m, na, pn1, pn2, nn_offset,psref;
int no_of_points;
float noise1[AG_MAKEP], noise2[AG_MAKEP],signal1[AG_MAKEP];
float t1, t2, t3;
float pos;
float *pwra;
float *x;
short int * zxy;
// Get the frequency of a signal that is very close to
// afc_fq
// Store old values extrapolated from old data (we might like to compare)
afc_fq=mix1_fq_mid[mix1p0+np];
if(afc_fq<0)
  {
  lirerr(889955);
  return;
  }
na=(np-afct_half_avgnum+max_fftxn)&fftxn_mask;
if(na > fftx_nm)return;
// Get spectrum and noise floor on both sides in no_of_points points.
if(ag.mode_control == 1)
  {
  no_of_points=nno;
  }
else
  {
  no_of_points=ag.lock_range*fftx_points_per_hz;
  }
no_of_points=(no_of_points&0xfffffffe)+1;
if(no_of_points < 9)no_of_points=9;
if(no_of_points > AG_MAKEP)no_of_points=AG_MAKEP;        
if(baseband_bw_fftxpts > afcf_search_range)
  {
  t1=afcf_search_range;
  }
else
  {
  t1=baseband_bw_fftxpts;
  }
nn_offset=2.5*t1+no_of_points;
for(i=0; i<no_of_points; i++)
  {
  noise1[i]=0;
  noise2[i]=0;
  signal1[i]=0;
  }
psref=mix1_fq_mid[mix1p0+np]*fftx_points_per_hz-no_of_points/2;
pn1=psref-nn_offset;
if(pn1 < 0)pn1=psref+2*nn_offset;
pn2=psref+nn_offset;
if(pn2 >= fftx_size)pn2=psref-2*nn_offset;
if(sw_onechan)
  {
  for(i=0; i<afct_avgnum; i++)
    {
    pwra=&fftx_pwr[na*fftx_size];
    t1=afct_window[i];
    for(j=0; j<no_of_points; j++)
      {
      signal1[j]+=pwra[psref+j]*t1;
      noise1[j]+=pwra[pn1+j]*t1;
      noise2[j]+=pwra[pn2+j]*t1;
      }
    na=(na+1)&fftxn_mask;
    }
  }
else
  {
  if(swmmx_fft2)
    {
    for(i=0; i<afct_avgnum; i++)
      {
      zxy=&fft2_short_int[4*na*fft2_size];
      t3=afct_window[i];
      for(j=0; j<no_of_points; j++)
        {
        m=4*(psref+j);
        t1=pg.c1*zxy[m  ]+pg.c2*zxy[m+2]+pg.c3*zxy[m+3];
        t2=pg.c1*zxy[m+1]+pg.c2*zxy[m+3]-pg.c3*zxy[m+2];
        signal1[j]+=t3*(t1*t1+t2*t2);
        m=4*(pn1+j);
        t1=pg.c1*zxy[m  ]+pg.c2*zxy[m+2]+pg.c3*zxy[m+3];
        t2=pg.c1*zxy[m+1]+pg.c2*zxy[m+3]-pg.c3*zxy[m+2];
        noise1[j]+=t3*(t1*t1+t2*t2);
        m=4*(pn2+j);
        t1=pg.c1*zxy[m  ]+pg.c2*zxy[m+2]+pg.c3*zxy[m+3];
        t2=pg.c1*zxy[m+1]+pg.c2*zxy[m+3]-pg.c3*zxy[m+2];
        noise2[j]+=t3*(t1*t1+t2*t2);
        }
      na=(na+1)&fftxn_mask;
      }
    }
  else
    {
    for(i=0; i<afct_avgnum; i++)
      {
      x=&fftx[na*fftx_size*4];
      t3=afct_window[i];
      for(j=0; j<no_of_points; j++)
        {
        m=4*(psref+j);
        t1=pg.c1*x[m  ]+pg.c2*x[m+2]+pg.c3*x[m+3];
        t2=pg.c1*x[m+1]+pg.c2*x[m+3]-pg.c3*x[m+2];
        signal1[j]+=t3*(t1*t1+t2*t2);
        m=4*(pn1+j);
        t1=pg.c1*x[m  ]+pg.c2*x[m+2]+pg.c3*x[m+3];
        t2=pg.c1*x[m+1]+pg.c2*x[m+3]-pg.c3*x[m+2];
        noise1[j]+=t3*(t1*t1+t2*t2);
        m=4*(pn2+j);
        t1=pg.c1*x[m  ]+pg.c2*x[m+2]+pg.c3*x[m+3];
        t2=pg.c1*x[m+1]+pg.c2*x[m+3]-pg.c3*x[m+2];
        noise2[j]+=t3*(t1*t1+t2*t2);
        }
      na=(na+1)&fftxn_mask;
      }
    }
  }
// Locate the maximum point.
t1=0;
k=-1;
for(j=0; j<no_of_points; j++)
  {
  if(t1 < signal1[j])
    {
    t1=signal1[j];
    k=j;
    }
  } 
// If maximum is one of the end points we failed!
// Set negative amplitude for error
if(k <= 0 || k==no_of_points-1)
  {
  mix1_eval_sigpwr[mix1p0+np]=-1;
  }
else
  {
  parabolic_fit(&t1,&pos,sqrt(signal1[k-1]),sqrt(signal1[k]),sqrt(signal1[k+1]));
  mix1_eval_sigpwr[mix1p0+np]=t1*t1;
  mix1_eval_fq[mix1p0+np]=(psref+pos+k)/fftx_points_per_hz;
  }
// Get the noise floor.
// There may be a signal1 outside the passband.
// Therefore we try to avoid exceptionally large values.
// First get the average on the two sides.
t1=0;
t2=0;
for(j=0; j<no_of_points; j++)
  {
  t1+=noise1[j];
  t2+=noise2[j];
  } 
if(t1 > t2)
  {
  t1=t2;
  }
t1/=no_of_points;
// Select 8 times the lowest average as a threshold and
// get the noise floor as the average of points below this threshold.
t2=0;
t1*=8;
k=0;
for(j=0; j<no_of_points; j++)
  {
  if(noise1[j] < t1)
    {
    t2+=noise1[j];
    k++;
    }
  if(noise2[j] < t1)
    {
    t2+=noise2[j];
    k++;
    }
  }
if(k > no_of_points/4)
  {
  mix1_eval_noise[mix1p0+np]=t2/k;
  }
else
  {
  mix1_eval_noise[mix1p0+np]=t1/8;
  }  
// Signal and noise are averaged over afct_avgnum.
// In case the signal is zero we still get (S+N)/N larger than 1
// because of the statistical variation of the noise.
// Subtract the probable extra noise from mix1_eval_sigpwr
mix1_eval_sigpwr[mix1p0+np]-=mix1_eval_noise[mix1p0+np]*afc_noisefac;
if(mix1_eval_sigpwr[mix1p0+np] <=0 )
  {
  mix1_eval_sigpwr[mix1p0+np]=0;
  mix1_eval_noise[mix1p0+np]=0.000000000001;
  } 

// Add a little signal to the noise to make sure S/N will not overflow.
mix1_eval_noise[mix1p0+np]+=0.000001*mix1_eval_sigpwr[mix1p0+np];
mix1_eval_avgn[mix1p0+np]=afct_avgnum;
}

void make_afct_window(int n)
{
char s[80];
int i, new_avgn;
float t1,t2,t3;
new_avgn=n;
if(new_avgn < 1)new_avgn=1;
if(new_avgn > 0.75*ag_xpixels)new_avgn=0.75*ag_xpixels;
if(new_avgn>max_afc_fit) new_avgn=max_afc_fit;
// We need an odd number of points to have weight 1.000 on the
// midpoint of the window.
if(new_avgn < 1)
  {
  lirerr(72591);
  return;
  }
new_avgn=(new_avgn&0xfffffffe)+1;  
// If the same number (or nearly) is already adopted, return here.
if(fabs((float)(new_avgn-afct_avgnum))/(new_avgn+afct_avgnum) <0.15)return;
afct_avgnum=new_avgn;   
afct_half_avgnum=afct_avgnum>>1;
afc_tpts=ag.fit_points+afct_half_avgnum;
// Set up a sine window over afct_avgnum.
afc_noisefac=1/sqrt((float)(afct_avgnum));
if(ag.window == 1)
  {
  afc_noisefac*=1.414;
  t1=PI_L/(afct_avgnum+1);
  t2=0;
  t3=0;
  for(i=0; i<afct_avgnum; i++)
    {
    t2+=t1;
    afct_window[i]=sin(t2);
    t3+=afct_window[i];
    }        
  for(i=0; i<afct_avgnum; i++)
    {
    afct_window[i]/=t3;
    }
  }
else
  {
  t3=1.0/afct_avgnum;  
  for(i=0; i<afct_avgnum; i++)
    {
    afct_window[i]=t3;
    }
  }  
sprintf(s,"%3d",afct_avgnum);
lir_pixwrite(agbutt[AG_SEL_AVGNUM].x1+7*text_width/2-1,
                         agbutt[AG_SEL_AVGNUM].y1+2,s);
}



void make_afct_avgnum(void)
{
float t1, t2, t3;
int new_avgn;
// For an individual spectrum, the signal must be 10db above the noise
// to allow a good frequency determination.
// Set afc_avgnum for 10dB S/N (10 times) if possible.
// If the averaging process uses a sin() window the actual S/N 
// improvement is smaller than sqrt(N), therefore
// another factor 1.5 is used if a window is selected.
// Find out how many spectra we should average over in order to
// be able to see the signal. 
// The relative noise floor drops as sqrt(N) when we average.
t3=afc_maxval-afc_noise;
if(t3<0.01*afc_maxval)t3=0.01*afc_maxval;
if(ag.window == 0)t2 = 10; else t2= 15;
t1=(t2*afc_noise/t3);
t1=t1*t1;
new_avgn=t1+1;
// In case we need less than 2*afct_delay_points ask for 6dB better S/N
if(new_avgn < 2*afct_delay_points)
  {
  new_avgn=2*t1+1;
  }
make_afct_window(new_avgn);
}

