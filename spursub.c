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
#include "seldef.h"

void initial_remove_spur(void);
int store_new_spur(int pnt);
void swap_spurs(int ia, int ib);

#define SPUR_PERCENTAGE_LIMIT 0.08

float spur_search_threshold;

void spursearch_spectrum_cleanup(void)
{
float amp, pos, maxpow, maxamp, refamp, noise;
float *spk;
int i, j, k, nn;
int ia,ib;
float t1, t2, t3, t4;
float r1;
// Collect the smallest out of each group of 32 points in sp_tmp.
k=0;
for(i=spur_search_first_point; i<spur_search_last_point; i+=32)
  {
  sp_tmp[k]=BIGFLOAT;
  for(j=0; j<32; j++)
    {
    if(spursearch_spectrum[i+j]<sp_tmp[k])sp_tmp[k]=spursearch_spectrum[i+j];
    }
  k++;
  } 
if(k == 0)return;
// Get the average of the smallest points.
t1=0;
for(i=0; i<k; i++)
  {
  t1+=sp_tmp[i];
  }
t1/=k;
// Get the average of those smallest points that are below the average of all
// the smallest points.
noise=0;
j=0;  
for(i=0; i<k; i++)
  {
  if(sp_tmp[i] < t1)
    {
    noise+=sp_tmp[i];
    j++;
    }
  }
noise/=j;
// We now have a reasonable estimate of the minimum value of the
// noise floor in 32 points in noise.
// The RMS value would be somewhere around 7dB/sqrt(3*spur_speknum) higher.
// Subtract the noise floor.
noise*=pow(10.,0.7/sqrt((float)(3*spur_speknum)));
for(i=spur_search_first_point; i<spur_search_last_point; i++)
  {
  spursearch_spectrum[i]-=noise;
  if(spursearch_spectrum[i] < 0)spursearch_spectrum[i]=0;
  }
// anything that is 15dB/sqrt(3*spur_speknum) above the noise floor 
// has to be a signal.
// Locate the max point.
spur_search_threshold=noise*pow(10.,1.5/sqrt((float)(3*spur_speknum)));
ia=spur_search_first_point;
search_signal:;
while(ia < spur_search_last_point && 
                        spursearch_spectrum[ia] <spur_search_threshold)ia++;
if(ia == spur_search_last_point )goto search_signal_x;
ib=ia+1;
while(ib < spur_search_last_point && 
                        spursearch_spectrum[ib] >spur_search_threshold)ib++;
if(ib==spur_search_last_point && ib-ia < SPUR_SIZE)goto search_signal_x;
maxpow=0;
for(i=ia; i<ib; i++)
  {
  if(spursearch_spectrum[i]>maxpow)
    {
    maxpow=spursearch_spectrum[i];
    k=i;
    }
  }
parabolic_fit(&amp, &pos, spursearch_spectrum[k-1],
                 spursearch_spectrum[k], spursearch_spectrum[k+1]);
nn=k-SPUR_SIZE/2+1;
if(pos < 0)
  {
  pos+=1;
  nn--;
  }
i=pos*NO_OF_SPUR_SPECTRA;
if(i>=NO_OF_SPUR_SPECTRA)i=NO_OF_SPUR_SPECTRA-1;
spk=&spur_spectra[i*SPUR_SIZE];
refamp=fabs(spk[SPUR_SIZE/2]);
if(refamp<fabs(spk[SPUR_SIZE/2-1]))refamp=fabs(spk[SPUR_SIZE/2-1]);
maxamp=sqrt(maxpow);
t2=maxamp/refamp;
// Subtract the reference spectrum from the search data.
// Compute the percentage of the power that remains within SPUR_SIZE points.
t1=0;
t3=0;
t4=0;
for(i=0; i<SPUR_SIZE; i++)
  {
  if(spursearch_spectrum[nn+i]<0)
    {
    t3=1;
    t1=1;
    goto bad_peak;
    }
  t1+=spursearch_spectrum[nn+i];
  r1=pow(sqrt(spursearch_spectrum[nn+i])-t2*fabs(spk[i]),2.0);
  t3+=r1;
  if(t4<r1&&(i<2 ||i>=SPUR_SIZE-2))t4=r1;
  }
// t1 is now the total power of the spur/signal.
// t3 is the amount of power that we should expect to remain after subtracting a spur.
// t4 is the power of the biggest peak we should expect to remain.
// A spur might however drift. That would broaden its spectrum over
// the long time (3*spur_speknum) that was used to collect spursearch_spectrum.
// This means that we may have to accept a residue that is significantly
// above the noise floor.
if( (t4/noise > 5 && t1/t4 < 1000) ||
    (t4/noise > 2 && t1/t4 <  300) || t3/t1 > 0.1)
  {
bad_peak:;  
// More than SPUR_PERCENTAGE_LIMIT of the signal remains.   
// This means that the spectrum we see is not only a spur.
// It might be one of our desired signals!!
// Clear the search spectrum from this peak.
  while( (spursearch_spectrum[ia] > spursearch_spectrum[ia-1] ||
          spursearch_spectrum[ia] > spursearch_spectrum[ia-2] ||
          spursearch_spectrum[ia] > spursearch_spectrum[ia-3]) &&
                         ia>spur_search_first_point)ia--;
  while( (spursearch_spectrum[ib] > spursearch_spectrum[ib+1] ||
          spursearch_spectrum[ib] > spursearch_spectrum[ib+2] ||
          spursearch_spectrum[ib] > spursearch_spectrum[ib+3]) && 
                         ib<spur_search_last_point)ib++;
  for(i=ia; i<ib; i++)spursearch_spectrum[i]=-0.00000001;
  }
ia=ib;
goto search_signal;
search_signal_x:;
if(autospur_point >= spur_search_last_point-SPUR_WIDTH/2)
                       autospur_point=spur_search_first_point+SPUR_WIDTH/2+1;
}

void init_spur_elimination(void)
{
int i, j, k, pnt, ia, ib, fft1_pnt;
int spur_search_pnts;
float t1;
//mailbox[1]=0;
if(no_of_spurs >=genparm[MAX_NO_OF_SPURS])goto skip;
pnt=-1;
fft1_pnt=-1;
if(genparm[AFC_ENABLE] == 1)
  {
//  if(autospur_point!=spur_search_first_point+SPUR_WIDTH/2+1)return;
  if(autospur_point!=spur_search_first_point+SPUR_WIDTH/2+1)lirerr(883461);
// Check if we are in the main/waterfall graph area.
  for(i=0; i<no_of_scro; i++)
    {
    if( scro[i].x1 <= mouse_x && 
        scro[i].x2 >= mouse_x &&      
        scro[i].y1 <= mouse_y && 
        scro[i].y2 >= mouse_y) 
      {
      if(scro[i].no == WIDE_GRAPH)
        {
        pnt=mouse_x-wg_first_xpixel;
        if(wg.xpoints_per_pixel != 1 && wg.pixels_per_xpoint != 1)
          {
          if(wg.xpoints_per_pixel == 0)
            {
            pnt=(pnt+(wg.pixels_per_xpoint>>1))/wg.pixels_per_xpoint;
            }
          else
            {
            pnt*=wg.xpoints_per_pixel;
            }  
          }
        pnt+=wg.first_xpoint;
        fft1_pnt=1;
        if(genparm[SECOND_FFT_ENABLE] != 0)
          {
          pnt*=fft2_to_fft1_ratio;
          }
        }  
      if(scro[i].no == HIRES_GRAPH)
        {
        pnt=mouse_x-hg_first_xpixel+hg_first_point;
        fft1_pnt=-1;
        }  
      }
    }
  if(pnt<0 || pnt >= fftx_size)return;  
// Check that we can locate a spur and move pnt to the peak. 
  k=SPUR_WIDTH/2;
  if(genparm[SECOND_FFT_ENABLE] != 0) k*=fft2_size/fft1_size;
  if(fft1_pnt > 0)
    {
    if(wg.xpoints_per_pixel > 1)
      {
      k=SPUR_WIDTH*wg.xpoints_per_pixel/2;
      }
    else
      {
      k=0;
      }
    }
  if(k < 1+SPUR_WIDTH/2)k=1+SPUR_WIDTH/2;  
  spur_search_pnts=2*k;  
  if(spur_search_pnts>=fft1_size/2)
    {
    spur_search_pnts=fft1_size/2-1;
    k=spur_search_pnts/2;
    }
  if(ffts_nm < spur_speknum-1)return;
  if(k<9)k=9;
  ia=pnt-k;
  ib=pnt+k;
  if(ia < spur_search_first_point)ia=spur_search_first_point;
  if(ib > spur_search_last_point)ib=spur_search_last_point;
  pnt=-1;
  t1=0;
  for(i=ia; i<=ib; i++)
    {
    if(t1<spursearch_spectrum[i])
      {
      t1=spursearch_spectrum[i];
      pnt=i;
      }
    }  
  if(pnt<0)return;
// Always point to the first of SPUR_WIDTH points surrounding the peak.
  pnt-=SPUR_WIDTH/2;
  if(pnt < spur_search_first_point)return;
  }
else
  {
  while(autospur_point < spur_search_last_point && 
          spursearch_spectrum[autospur_point] <spur_search_threshold)
                                                            autospur_point++;
  while(autospur_point < spur_search_last_point && 
          (spursearch_spectrum[autospur_point] < spursearch_spectrum[autospur_point+1] ||
           spursearch_spectrum[autospur_point] < spursearch_spectrum[autospur_point+1]) )autospur_point++;
  if(autospur_point >= spur_search_last_point-SPUR_WIDTH/2)
    {
skip:;    
    autospur_point=spur_search_last_point;
    return;
    }
  pnt=autospur_point-SPUR_WIDTH/2;
  for(i=0;i<no_of_spurs;i++)
    {
    if(abs(pnt-spur_location[i]) <=SPUR_WIDTH/2)goto skip;
    }
  autospur_point += SPUR_WIDTH;
  }
//if(pnt > 9390-SPUR_WIDTH/2 && pnt < 9390+SPUR_WIDTH/2)mailbox[1]=1; else mailbox[1]=0;
//mailbox[1]=1;
spurno=no_of_spurs;
spur_ampl[spurno]=1;
spur_noise[spurno]=0.001;
spur_avgd2[spurno]=0;
if(store_new_spur(pnt) != 0)
  {
//  if(mailbox[1]==1)fprintf( dmp,"\nSTORE %d failed",pnt);
//  mailbox[1]=0;
  return;
  }
//if(mailbox[1]==1)fprintf( dmp,"\nSTORE %d OK",pnt);
// Set up phase locked loop parameters.
if(spur_phase_lock(ffts_na) != 0)
  {
//if(mailbox[1]==1)fprintf( dmp,"\nPLL failed");
//mailbox[1]=0;
  return;
  }
//if(mailbox[1] == 1)fprintf( dmp,"\nspurno %d  loc %d   pnt %d",spurno, spur_location[spurno],pnt);
no_of_spurs++;
// Remove the spur for the latest spur_speknum fftx transforms.
initial_remove_spur();
// Rearrange spurs so we keep them in order by increasing frequency.
for(i=0; i<spurno; i++)
  {
retry_j:;
  for(j=i+1; j<no_of_spurs; j++)
    {
    if(abs(spur_location[i]-spur_location[j]) < 4)
      {
// These two spurs are too close. remove the weakest one.
      if(spur_ampl[i] > spur_ampl[j])
        {
        k=j;
        }
      else
        {
        k=i;
        }
      no_of_spurs--;
      if(k != no_of_spurs)
        {
        remove_spur(k);
        }
      goto retry_j;
      }
    if(spur_freq[i] > spur_freq[j])
      {
      swap_spurs(i,j);
      }
    }  
  }
//mailbox[1]=0;
}


void initial_remove_spur(void)
{
int i, j, ind;
int izz, pnt;
int ni;
int *uind;
float t1,t2,t3,t4,r1,r2;
float a1, a2, b1, b2, d1, d2;
float average_rot,rot;
float freq;
float phase_slope,phase_curv;
float *z;
short int *zmmx;
// PLL was sucessful. We now have a "local oscillator"
// that is locked to the spur. The LO is characterised
// by phase and amplitude parameters. 
// Subtract the LO from the original data in fftx.
uind=&spur_ind[spurno*max_fftxn];
average_rot=spur_freq[spurno]*spur_freq_factor;
a1=spur_ampl[spurno]*cos(spur_d0pha[spurno]);
a2=spur_ampl[spurno]*sin(spur_d0pha[spurno]);
b1=cos(spur_d1pha[spurno]);
b2=sin(spur_d1pha[spurno]);
d1=cos(spur_d2pha[spurno]);
d2=sin(spur_d2pha[spurno]);
phase_slope=spur_d1pha[spurno];
phase_curv=spur_d2pha[spurno];
ni=(ffts_na+fftxn_mask)&fftxn_mask;
pnt=spur_location[spurno];
izz=spur_speknum-1;
while(izz >= 0)
  {
  rot=-0.5*phase_slope/PI_L;
  i=average_rot-rot+0.5;
  rot+=i;
  freq=rot/spur_freq_factor;
  j=(int)(freq)+2-pnt-SPUR_SIZE/2;
  if(j<0)j=0;
  j=1-j;
  if(j<0)j=0;
  ind=uind[ni];
  if(sw_onechan)
    {
    if((j^(spur_location[spurno]&1))==1)
      {
      t1=-a1;
      t2=-a2;
      }
    else
      {
      t1=a1;
      t2=a2;
      }
    if(swmmx_fft2)
      {
      zmmx=&fft2_short_int[twice_rxchan*(ni*fft2_size+pnt)];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        zmmx[2*i  ]-=spur_spectra[ind+i]*t1;
        zmmx[2*i+1]-=spur_spectra[ind+i]*t2;
        }
      }
    else
      {  
      z=&fftx[twice_rxchan*(ni*fftx_size+pnt)];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        z[2*i  ]-=spur_spectra[ind+i]*t1;
        z[2*i+1]-=spur_spectra[ind+i]*t2;
        }
      }
    }
  else
    {
    if((j^(spur_location[spurno]&1))==1)
      {
      t1=-sp_c1*a1;
      t2=-sp_c1*a2;
      t3=-sp_c2*a1+sp_c3*a2;
      t4=-sp_c2*a2-sp_c3*a1;
      }
    else
      {
      t1=sp_c1*a1;
      t2=sp_c1*a2;
      t3=sp_c2*a1-sp_c3*a2;
      t4=sp_c2*a2+sp_c3*a1;
      }
    if(swmmx_fft2)
      {
      zmmx=&fft2_short_int[twice_rxchan*(ni*fft2_size+pnt)];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        zmmx[4*i  ]-=spur_spectra[ind+i]*t1;
        zmmx[4*i+1]-=spur_spectra[ind+i]*t2;
        zmmx[4*i+2]-=spur_spectra[ind+i]*t3;
        zmmx[4*i+3]-=spur_spectra[ind+i]*t4;
        }
      }
    else
      {    
      z=&fftx[twice_rxchan*(ni*fftx_size+pnt)];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        z[4*i  ]-=spur_spectra[ind+i]*t1;
        z[4*i+1]-=spur_spectra[ind+i]*t2;
        z[4*i+2]-=spur_spectra[ind+i]*t3;
        z[4*i+3]-=spur_spectra[ind+i]*t4;
        }
      }
    }  
  ni=(ni+fftxn_mask)&fftxn_mask;
  izz--;
  r1=a1*b1+a2*b2;
  a2=a2*b1-a1*b2;
  a1=r1;
  r2=b1*d1+b2*d2;
  b2=b2*d1-b1*d2;
  b1=r2;
  phase_slope-=phase_curv;
  }  
}


void spurspek_norm(void)
{
int i;
float t1,t2;
// We have a power spectrum in spur_power[]  
// Assume that the level at the spectrum end points correspond
// to the white noise floor.
// Remove the average of the end points and normalise for the sum
// of all points = 1
t1=0.5*(spur_power[0]+spur_power[SPUR_WIDTH-1]);
t2=0;
for(i=0; i<SPUR_WIDTH; i++)
  {
  spur_power[i]-=t1;
  if(spur_power[i] <0)spur_power[i]=0;
  t2+=spur_power[i];
  }
for(i=0; i<SPUR_WIDTH; i++)
  {
  spur_power[i]/=t2;
  }
}

void make_spur_pol(void)
{
int i;
float t1,t2;
float x2,y2,re_xy,im_xy;
float noi2, x2s, y2s, sina;
x2=0;
y2=0;
re_xy=0;
im_xy=0;
for(i=0; i<SPUR_WIDTH; i++)
  {
  x2+=spur_power[i]*spur_pxy[i].x2;
  y2+=spur_power[i]*spur_pxy[i].y2;
  re_xy+=spur_power[i]*spur_pxy[i].re_xy;
  im_xy+=spur_power[i]*spur_pxy[i].im_xy;
  }
t1=x2+y2;
x2/=t1;
y2/=t1;
re_xy/=t1;
im_xy/=t1;  
// Extract polarization from the power weighted average of
// powers and cross channel correlation.
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
//if(mailbox[1]==1)fprintf( dmp,"\nXY x2 %f  y2 %f ",x2,y2);
if(x2s > 0)
  {
  y2s=y2-noi2;
  sp_c1=sqrt(x2s);
  if(y2s > 0 && t2 > 0)
    {
    sina=sqrt(y2s);
    sp_c2=sina*re_xy/sqrt(t2);
    sp_c3=-sina*im_xy/sqrt(t2);
    t1=sqrt(sp_c1*sp_c1+sp_c2*sp_c2+sp_c3*sp_c3);
    if(sp_c2 < 0)t1=-t1;
    sp_c1/=t1;
    sp_c2/=t1;
    sp_c3/=t1;
    }
  else 
    {
    if(x2 > y2)
      {
      sp_c1=1;
      sp_c2=0;
      }
    else
      {  
      sp_c1=0;
      sp_c2=1;
      }
    sp_c3=0;  
    }
  }
else 
  {
  sp_c1=1;
  sp_c2=0;
  sp_c3=0;
  }
i=3*spurno;
spur_pol[i  ]=sp_c1; 
spur_pol[i+1]=sp_c2;
spur_pol[i+2]=sp_c3;
//if(mailbox[1]==1)fprintf( dmp,"\nPOL:  %f %f %f",sp_c1,sp_c2,sp_c3);
}

int make_spur_freq(void)
{
int i,k;
float amp,pos;
float t1,t2,t3;
// Get the spur frequency with decimals
t2=0;
k=0;
for(i=0; i<SPUR_WIDTH; i++)
  {
  if(t2<spur_power[i])
    {
    t2=spur_power[i];
    k=i;
    }
  }
if(k == 0 || k == SPUR_WIDTH-1)return -1;
t1=sqrt(spur_power[k-1]);
t3=sqrt(spur_power[k+1]);
t2=sqrt(t2);
parabolic_fit( &amp, &pos, t1, t2, t3);
spur_freq[spurno]=spur_location[spurno]+k+pos;
return 0;
}


int store_new_spur(int pnt)
{
int i, k, np;
float t1;
short int *zmmx;
float *z, *pwra;
TWOCHAN_POWER *pxy;
spur_flag[spurno]=0;
// Store old transforms in spur table for the new spur and 
// accumulate power spectrum.
spurp0=spurno*spur_block;
np=(ffts_na-spur_speknum+max_fftxn)&fftxn_mask;
if(swmmx_fft2)
  {
  if(sw_onechan)
    {
// With one channel only, just use the power spectrum.  
    for(i=0; i<SPUR_WIDTH; i++)spur_power[i]=0;
    while(np != ffts_na)
      {
      k=spurp0+np*SPUR_WIDTH*twice_rxchan;
      zmmx=&fft2_short_int[twice_rxchan*(np*fft2_size+pnt)];
      pwra=&fftx_pwr[np*fftx_size+pnt];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        spur_table_mmx[2*i+k  ]=zmmx[2*i  ];
        spur_table_mmx[2*i+k+1]=zmmx[2*i+1];
        spur_power[i]+=pwra[i];
        }
      np=(np+1)&fftxn_mask;
      }  
    }
  else  //ui.rx_channels = 2
    {
// We have two channels and polarization is unknown.
// First make an average of channel powers and correlations.
    for(i=0; i<SPUR_WIDTH; i++)
      {
      spur_pxy[i].x2=0;
      spur_pxy[i].y2=0;
      spur_pxy[i].re_xy=0;
      spur_pxy[i].im_xy=0;
      }
    while(np != ffts_na)
      {
      pxy=(TWOCHAN_POWER*)(&fftx_xypower[np*fftx_size+pnt].x2);
      k=spurp0+np*SPUR_WIDTH*twice_rxchan;
      zmmx=&fft2_short_int[twice_rxchan*(np*fft2_size+pnt)];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        spur_table_mmx[4*i+k  ]=zmmx[4*i  ];
        spur_table_mmx[4*i+k+1]=zmmx[4*i+1];
        spur_table_mmx[4*i+k+2]=zmmx[4*i+2];
        spur_table_mmx[4*i+k+3]=zmmx[4*i+3];
        spur_pxy[i].x2+=pxy[i].x2;
        spur_pxy[i].y2+=pxy[i].y2;
        spur_pxy[i].re_xy+=pxy[i].re_xy;
        spur_pxy[i].im_xy+=pxy[i].im_xy;
        }
      np=(np+1)&fftxn_mask;
      }
    }
  }
else
  {  
  if(sw_onechan)
    {
// With one channel only, just use the power spectrum.  
    for(i=0; i<SPUR_WIDTH; i++)spur_power[i]=0;
    while(np != ffts_na)
      {
      k=spurp0+np*SPUR_WIDTH*twice_rxchan;
      z=&fftx[twice_rxchan*(np*fftx_size+pnt)];
      pwra=&fftx_pwr[np*fftx_size+pnt];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        spur_table[2*i+k  ]=z[2*i  ];
        spur_table[2*i+k+1]=z[2*i+1];
        spur_power[i]+=pwra[i];
        }
      np=(np+1)&fftxn_mask;
      }  
    }
  else  //ui.rx_channels = 2
    {
// We have two channels and polarization is unknown.
// First make an average of channel powers and correlations.
    for(i=0; i<SPUR_WIDTH; i++)
      {
      spur_pxy[i].x2=0;
      spur_pxy[i].y2=0;
      spur_pxy[i].re_xy=0;
      spur_pxy[i].im_xy=0;
      }
    while(np != ffts_na)
      {
      pxy=(TWOCHAN_POWER*)(&fftx_xypower[np*fftx_size+pnt].x2);
      k=spurp0+np*SPUR_WIDTH*twice_rxchan;
      z=&fftx[twice_rxchan*(np*fftx_size+pnt)];
      for(i=0; i<SPUR_WIDTH; i++)
        {
        spur_table[4*i+k  ]=z[4*i  ];
        spur_table[4*i+k+1]=z[4*i+1];
        spur_table[4*i+k+2]=z[4*i+2];
        spur_table[4*i+k+3]=z[4*i+3];
        spur_pxy[i].x2+=pxy[i].x2;
        spur_pxy[i].y2+=pxy[i].y2;
        spur_pxy[i].re_xy+=pxy[i].re_xy;
        spur_pxy[i].im_xy+=pxy[i].im_xy;
        }
      np=(np+1)&fftxn_mask;
      }
    }
  }
if(sw_onechan)
  {
  spurspek_norm();  
  }
else
  {
// Two channels. Extract the power for a signal using the channel 
// correlation (Slightly better than just adding x2 and y2)
  for(i=0; i<SPUR_WIDTH; i++)
    {
    t1=spur_pxy[i].x2+spur_pxy[i].y2;
    spur_power[i]=t1+2*(spur_pxy[i].re_xy*spur_pxy[i].re_xy+
                        spur_pxy[i].im_xy*spur_pxy[i].im_xy
                       -spur_pxy[i].x2*spur_pxy[i].y2 ) / t1;
    }
  spurspek_norm();  
  make_spur_pol();
  }
spur_location[spurno]=pnt;
return make_spur_freq();
}

void swap_spurs(int ia, int ib)
{
int i, k;
short int m;
float t1;
k=spur_flag[ia];
spur_flag[ia]=spur_flag[ib];
spur_flag[ib]=k;
k=spur_location[ia];
spur_location[ia]=spur_location[ib];
spur_location[ib]=k;
t1=spur_d0pha[ia];
spur_d0pha[ia]=spur_d0pha[ib];
spur_d0pha[ib]=t1;
t1=spur_d1pha[ia];
spur_d1pha[ia]=spur_d1pha[ib];
spur_d1pha[ib]=t1;
t1=spur_d2pha[ia];
spur_d2pha[ia]=spur_d2pha[ib];
spur_d2pha[ib]=t1;
t1=spur_ampl[ia];
spur_ampl[ia]=spur_ampl[ib];
spur_ampl[ib]=t1;
t1=spur_noise[ia];
spur_noise[ia]=spur_noise[ib];
spur_noise[ib]=t1;
t1=spur_avgd2[ia];
spur_avgd2[ia]=spur_avgd2[ib];
spur_avgd2[ib]=t1;
t1=spur_pol[ia];
spur_pol[ia]=spur_pol[ib];
spur_pol[ib]=t1;
t1=spur_freq[ia];
spur_freq[ia]=spur_freq[ib];
spur_freq[ib]=t1;
if(swmmx_fft2)
  {
  for(i=0; i<spur_block; i++)
    {
    m=spur_table_mmx[ia*spur_block+i];
    spur_table_mmx[ia*spur_block+i]=
                               spur_table_mmx[ib*spur_block+i];
    spur_table_mmx[ib*spur_block+i]=m;
    }
  }
else
  {
  for(i=0; i<spur_block; i++)
    {
    t1=spur_table[ia*spur_block+i];
    spur_table[ia*spur_block+i]=spur_table[ib*spur_block+i];
    spur_table[ib*spur_block+i]=t1;
    }
  }  
for(i=0; i<twice_rxchan*max_fftxn; i++)
  {
  t1=spur_signal[twice_rxchan*max_fftxn*ia+i];
  spur_signal[twice_rxchan*max_fftxn*ia+i]=
                    spur_signal[twice_rxchan*max_fftxn*ib+i];
  spur_signal[twice_rxchan*max_fftxn*ib+i]=t1;
  }        
for(i=0; i<max_fftxn; i++)
  {
  k=spur_ind[max_fftxn*ia+i];
  spur_ind[max_fftxn*ia+i]=spur_ind[max_fftxn*ib+i];
  spur_ind[max_fftxn*ib+i]=k;
  }
}

void init_spur_spectra(void)
{
int i, j, ifreq;
int iwin;
float *tmp_window,*spk, *tmp_buf;
float t1, t2, t3;
#define SPI_N 8
#define SPI_SIZE (1<<SPI_N)
unsigned short int *tmp_permute;
COSIN_TABLE *tmp_tab;
tmp_permute=malloc(SPI_SIZE*sizeof(short int));
if(tmp_permute == NULL)
  {
alloc_err:;
  lirerr(1103);
  return;
  }
tmp_tab=malloc(SPI_SIZE*sizeof(COSIN_TABLE));
if(tmp_tab == NULL)
  {
alloc_err1:;
  free(tmp_permute);
  goto alloc_err; 
  }
tmp_window=malloc(SPI_SIZE*sizeof(float));
if(tmp_window == NULL)
  {
alloc_err2:;
  free(tmp_tab);
  goto alloc_err1; 
  }
tmp_buf=malloc(2*SPI_SIZE*sizeof(float));
if(tmp_buf == NULL)
  {
  goto alloc_err2;
  }
if(genparm[SECOND_FFT_ENABLE] != 0)
  {
  iwin=genparm[SECOND_FFT_SINPOW];
  }
else
  {
  iwin=genparm[FIRST_FFT_SINPOW];
  }
init_fft(0,SPI_N, SPI_SIZE, tmp_tab, tmp_permute);
if(iwin !=0)
  {
  make_window(4,SPI_SIZE,iwin, tmp_window);
  }
else
  {
  for(i=0; i<SPI_SIZE; i++)
    {
    tmp_window[i]=1;
    }  
  }
// Generate a sine wave with frequency SPUR_SIZE/4 to SPUR_SIZE/4+1
// in NO_OF_SPUR_SPECTRA steps.
for(ifreq=0; ifreq<NO_OF_SPUR_SPECTRA; ifreq++)
  {
  t1=1+SPUR_SIZE/4+(float)(ifreq)/NO_OF_SPUR_SPECTRA;
  t1=2*PI_L*t1/SPI_SIZE;
  t2=0;
  for(i=0; i<SPI_SIZE; i++)
    {
    tmp_buf[2*i  ]=tmp_window[i]*sin(t2);
    tmp_buf[2*i+1]=tmp_window[i]*cos(t2);
    t2+=t1;
    }
  fftforward(SPI_SIZE, SPI_N, tmp_buf, tmp_tab, tmp_permute,FALSE);
// For simplicity we determine phases from the weighted sum of
// the individual spectral lines.
// Calculate the phase of the spectra we have got here with the
// same procedure and twist the phase angle so we store these
// spectra in phase zero with the simplified phase method.
  t1=0;
  t2=0;
  j=1;
  for(i=0; i<SPUR_SIZE; i++)
    {
    t3=j*(tmp_buf[2*i]*tmp_buf[2*i]+tmp_buf[2*i+1]*tmp_buf[2*i+1]);
    j*=-1;
    t1+=t3*tmp_buf[2*i  ];
    t2+=t3*tmp_buf[2*i+1];
    }
  t3=sqrt(t1*t1+t2*t2);
  t1/=t3;
  t2/=t3;
  for(i=0; i<SPUR_SIZE; i++)
    {
    t3=t1*tmp_buf[2*i]+t2*tmp_buf[2*i+1];
    tmp_buf[2*i+1]=t1*tmp_buf[2*i+1]-t2*tmp_buf[2*i];
    tmp_buf[2*i]=t3;
    }
// Store the real part only
// The imaginary part we have is just because a too small size
// was used for SPI_SIZE to make program start fast 
// (The real size, fftx_n may go very slow).
  spk=&spur_spectra[ifreq*SPUR_SIZE];
  for(i=0; i<SPUR_SIZE; i++)spk[i]=tmp_buf[2*i];
// Normalise the spectrum for sum of squares equal to 1
  t1=0;
  for(i=0; i<SPUR_SIZE; i++)
    {
    t1+=spk[i]*spk[i];
    }
  t1=sqrt(t1);
  for(i=0; i<SPUR_SIZE; i++)
    {
    spk[i]/=t1;
    }
  }
free(tmp_permute);
free(tmp_tab);
free(tmp_window);
for(i=0; i<fftx_size; i++)spursearch_spectrum[i]=0;
}

float average_curvature(float *z, int siz)
{
int i, k, m0, m1, m2;
float t1,t2,t3;
// Calculate the average at 3 well separated points.
// Three values are needed for the second derivative.
  k=(siz-1)/3;
  if(k<1)k=1;
  m1=(siz-k-1)/2;
  m0=m1-k;
  if(m0<0)
    {
    m0=0;
    m1=m0+k;
    }
  m2=m1+k;
if(m2+k < siz-1)
  {
  m0++;
  m1++;
  m2++;
  }
t1=0;
t2=0;
t3=0;
for(i=0; i<k; i++)
  {
  t1+=z[m0+i];
  t2+=z[m1+i];
  t3+=z[m2+i];
  }
return (t3-2*t2+t1)/(k*k*k);
}

float average_slope(float *z, int siz)
{
int i, k, m1, m2;
float t2,t3;
// Calculate the average at 2 well separated points.
// Two values are needed for the first derivative.
k=siz/2;
m1=0;
m2=k;
t2=0;
t3=0;
for(i=0; i<k; i++)
  {
  t2+=z[m1+i];
  t3+=z[m2+i];
  }
return (t3-t2)/(k*k);
}



void remove_phasejumps(float *z, int siz)
{
float t1;
int i;
// Remove any 2*PI discontinuities in a phase function.   
t1=0;
for(i=1; i<siz; i++)
  {
  z[i]+=t1;
  if(z[i]-z[i-1] > PI_L)
    {
    z[i]-=2*PI_L;
    t1-=2*PI_L;
    }
  if(z[i]-z[i-1] < -PI_L)
    {
    z[i]+=2*PI_L;
    t1+=2*PI_L;
    }
  }
}


void complex_lowpass(float *zin, float *zout, int nn, int siz)
{
int i,j,k,avgnum;
float r1,r2,t1,t2,t3;
// Average a complex function over avgnum points
avgnum=nn|1;
if(avgnum > nn && avgnum > siz/4)avgnum-=2;
if(avgnum < 1 )
  {
  lirerr(1105);
  return;
  }
t1=0;
t2=0;
t3=1.0/avgnum;
for(i=0; i<avgnum; i++)
  {
  t1+=zin[2*i  ];
  t2+=zin[2*i+1];
  }
j=1+avgnum/2;
r1=t1*t3;
r2=t2*t3;
for(i=0; i<j; i++)
  {
  zout[2*i  ]=r1;
  zout[2*i+1]=r2;
  }
i=0;
k=avgnum;
while(k < siz)
  {
  t1+=zin[2*k  ]-zin[2*i  ];
  t2+=zin[2*k+1]-zin[2*i+1];
  zout[2*j  ]=t3*t1;
  zout[2*j+1]=t3*t2;
  i++;
  j++;
  k++;
  }
t1*=t3;
t2*=t3;
while(j < siz)
  {
  zout[2*j  ]=t1;
  zout[2*j+1]=t2;
  j++;
  }
}

void shift_spur_table(int j)
{
int i, knd, ni, nj, shift;
// The spur is no longer centered in the SPUR_WIDTH points
// that we are saving.
nj=(ffts_na+1)&fftxn_mask;
if(j < 0)shift=-1; else shift=1;
spur_location[spurno]+=shift;
if(spur_location[spurno] < SPUR_WIDTH)
  {
  spur_flag[spurno]=1;
  spur_location[spurno]=2*SPUR_WIDTH;
  return;
  }
if(spur_location[spurno] > fftx_size-SPUR_WIDTH)
  {
  spur_flag[spurno]=1;
  spur_location[spurno]=fftx_size-2*SPUR_WIDTH;
  return;
  }
ni=(ffts_na-spur_speknum+max_fftxn)&fftxn_mask;
if(sw_onechan)
  {
  if(swmmx_fft2)
    {
    if(shift == 1)
      {
      while(ni != nj)
        {
        knd=spurp0+ni*SPUR_WIDTH*twice_rxchan;
        for(i=1; i<SPUR_WIDTH; i++)
          {
          spur_table_mmx[2*i+knd-2]=spur_table_mmx[2*i+knd  ];
          spur_table_mmx[2*i+knd-1]=spur_table_mmx[2*i+knd+1];
          }
        spur_table_mmx[knd+2*SPUR_WIDTH-2]=0;
        spur_table_mmx[knd+2*SPUR_WIDTH-1]=0;
        ni=(ni+1)&fftxn_mask;
        }
      }
    else
      {
      while(ni != nj)
        {
        knd=spurp0+ni*SPUR_WIDTH*twice_rxchan;
        for(i=SPUR_WIDTH-1; i>0; i--)
          {
          spur_table_mmx[2*i+knd  ]=spur_table_mmx[2*i+knd-2];
          spur_table_mmx[2*i+knd+1]=spur_table_mmx[2*i+knd-1];
          }
        spur_table_mmx[knd  ]=0;
        spur_table_mmx[knd+1]=0;
        ni=(ni+1)&fftxn_mask;
        }
      }
    }
  else
    {
    if(shift == 1)
      {
      while(ni != nj)
        {
        knd=spurp0+ni*SPUR_WIDTH*twice_rxchan;
        for(i=1; i<SPUR_WIDTH; i++)
          {
          spur_table[2*i+knd-2]=spur_table[2*i+knd  ];
          spur_table[2*i+knd-1]=spur_table[2*i+knd+1];
          }
        spur_table[knd+2*SPUR_WIDTH-2]=0;
        spur_table[knd+2*SPUR_WIDTH-1]=0;
        ni=(ni+1)&fftxn_mask;
        }
      }
    else
      {
      while(ni != nj)
        {
        knd=spurp0+ni*SPUR_WIDTH*twice_rxchan;
        for(i=SPUR_WIDTH-1; i>0; i--)
          {
          spur_table[2*i+knd  ]=spur_table[2*i+knd-2];
          spur_table[2*i+knd+1]=spur_table[2*i+knd-1];
          }
        spur_table[knd  ]=0;
        spur_table[knd+1]=0;
        ni=(ni+1)&fftxn_mask;
        }
      }
    }
  }
else
  {
  if(swmmx_fft2)
    {
    if(shift == 1)
      {
      while(ni != nj)
        {
        knd=spurp0+ni*SPUR_WIDTH*twice_rxchan;
        for(i=1; i<SPUR_WIDTH; i++)
          {
          spur_table_mmx[4*i+knd-4]=spur_table_mmx[4*i+knd  ];
          spur_table_mmx[4*i+knd-3]=spur_table_mmx[4*i+knd+1];
          spur_table_mmx[4*i+knd-2]=spur_table_mmx[4*i+knd+2];
          spur_table_mmx[4*i+knd-1]=spur_table_mmx[4*i+knd+3];
          }
        spur_table_mmx[knd+4*SPUR_WIDTH-4]=0;
        spur_table_mmx[knd+4*SPUR_WIDTH-3]=0;
        spur_table_mmx[knd+4*SPUR_WIDTH-2]=0;
        spur_table_mmx[knd+4*SPUR_WIDTH-1]=0;
        ni=(ni+1)&fftxn_mask;
        }
      }
    else
      {
      while(ni != nj)
        {
        knd=spurp0+ni*SPUR_WIDTH*twice_rxchan;
        for(i=SPUR_WIDTH-1; i>0; i--)
          {
          spur_table_mmx[4*i+knd  ]=spur_table_mmx[4*i+knd-4];
          spur_table_mmx[4*i+knd+1]=spur_table_mmx[4*i+knd-3];
          spur_table_mmx[4*i+knd+2]=spur_table_mmx[4*i+knd-2];
          spur_table_mmx[4*i+knd+3]=spur_table_mmx[4*i+knd-1];
          }
        spur_table_mmx[knd  ]=0;
        spur_table_mmx[knd+1]=0;
        spur_table_mmx[knd+2]=0;
        spur_table_mmx[knd+3]=0;
        ni=(ni+1)&fftxn_mask;
        }
      }
    }
  else
    {
    if(shift == 1)
      {
      while(ni != nj)
        {
        knd=spurp0+ni*SPUR_WIDTH*twice_rxchan;
        for(i=1; i<SPUR_WIDTH; i++)
          {
          spur_table[4*i+knd-4]=spur_table[4*i+knd  ];
          spur_table[4*i+knd-3]=spur_table[4*i+knd+1];
          spur_table[4*i+knd-2]=spur_table[4*i+knd+2];
          spur_table[4*i+knd-1]=spur_table[4*i+knd+3];
          }
        spur_table[knd+4*SPUR_WIDTH-4]=0;
        spur_table[knd+4*SPUR_WIDTH-3]=0;
        spur_table[knd+4*SPUR_WIDTH-2]=0;
        spur_table[knd+4*SPUR_WIDTH-1]=0;
        ni=(ni+1)&fftxn_mask;
        }
      }
    else
      {
      while(ni != nj)
        {
        knd=spurp0+ni*SPUR_WIDTH*twice_rxchan;
        for(i=SPUR_WIDTH-1; i>0; i--)
          {
          spur_table[4*i+knd  ]=spur_table[4*i+knd-4];
          spur_table[4*i+knd+1]=spur_table[4*i+knd-3];
          spur_table[4*i+knd+2]=spur_table[4*i+knd-2];
          spur_table[4*i+knd+3]=spur_table[4*i+knd-1];
          }
        spur_table[knd  ]=0;
        spur_table[knd+1]=0;
        spur_table[knd+2]=0;
        spur_table[knd+3]=0;
        ni=(ni+1)&fftxn_mask;
        }
      }
    }
  }
}

int spur_phase_lock(int nx)
{
int i,k,izz;
int np, nn;
float t1,t2,t3,t4,r1,r2,r3,r4;
float *zsig;
sp_d0=0;
sp_d1=0;
sp_d2=0;
spur_d0pha[spurno]=0;
spur_d1pha[spurno]=0;
spur_d2pha[spurno]=0;
// There is a spur present and we have it's spectrum.
// In case we have two channels we also have polarization parameters
// Calculate amplitude and phase data from spectrum weighted amplitudes.
zsig=&spur_signal[twice_rxchan*max_fftxn*spurno];
izz=0;
nn=(nx-spur_speknum+max_fftxn)&fftxn_mask;
np=nn;
if(swmmx_fft2)
  {
  if(sw_onechan)
    {
    while(np != nx)
      {
      k=spurp0+np*SPUR_WIDTH*twice_rxchan;
      t1=0;
      t2=0;
      for(i=0; i<SPUR_WIDTH-1; i+=2)
        {
        t1+=spur_power[i]*spur_table_mmx[2*i+k  ]-
            spur_power[i+1]*spur_table_mmx[2*i+k+2];
        t2+=spur_power[i]*spur_table_mmx[2*i+k+1]-
            spur_power[i+1]*spur_table_mmx[2*i+k+3];
        }
      if((spur_location[spurno]&1)==1)
        {
        t1=-t1;
        t2=-t2;
        }      
      zsig[2*np  ]=t1;  
      zsig[2*np+1]=t2;  
      sp_sig[2*izz  ]=t1;
      sp_sig[2*izz+1]=t2;
      izz++;
      np=(np+1)&fftxn_mask;
      }
    }  
  else  
    {
    while(np != nx)
      {
      k=spurp0+np*SPUR_WIDTH*twice_rxchan;
      t1=0;
      t2=0;
      t3=0;
      t4=0;
      for(i=0; i<SPUR_WIDTH-1; i+=2)
        {
        t1+=spur_power[i  ]*spur_table_mmx[4*i+k  ]-
            spur_power[i+1]*spur_table_mmx[4*i+k+4];
        t2+=spur_power[i  ]*spur_table_mmx[4*i+k+1]-
            spur_power[i+1]*spur_table_mmx[4*i+k+5];
        t3+=spur_power[i  ]*spur_table_mmx[4*i+k+2]-
            spur_power[i+1]*spur_table_mmx[4*i+k+6];
        t4+=spur_power[i  ]*spur_table_mmx[4*i+k+3]-
            spur_power[i+1]*spur_table_mmx[4*i+k+7];
        }
      if((spur_location[spurno]&1)==1)
        {
        r1=-sp_c1*t1-sp_c2*t3-sp_c3*t4;
        r2=-sp_c1*t2-sp_c2*t4+sp_c3*t3;
        r3=-sp_c1*t3+sp_c2*t1-sp_c3*t2;
        r4=-sp_c1*t4+sp_c2*t2+sp_c3*t1;
        }
      else
        {
        r1=sp_c1*t1+sp_c2*t3+sp_c3*t4;
        r2=sp_c1*t2+sp_c2*t4-sp_c3*t3;
        r3=sp_c1*t3-sp_c2*t1+sp_c3*t2;
        r4=sp_c1*t4-sp_c2*t2-sp_c3*t1;
        }
      zsig[4*np  ]=r1;  
      zsig[4*np+1]=r2;  
      zsig[4*np+2]=r3;  
      zsig[4*np+3]=r4;  
      sp_sig[2*izz  ]=r1;
      sp_sig[2*izz+1]=r2;
//if(mailbox[1]==1)fprintf( dmp,"\nA sp_sig  %f %f   %f %f",r1,r2,r3,r4);
      izz++;
      np=(np+1)&fftxn_mask;
      }
    }
  }
else
  {
  if(sw_onechan)
    {
    while(np != nx)
      {
      k=spurp0+np*SPUR_WIDTH*twice_rxchan;
      t1=0;
      t2=0;
      for(i=0; i<SPUR_WIDTH-1; i+=2)
        {
        t1+=spur_power[i]*spur_table[2*i+k  ]-
            spur_power[i+1]*spur_table[2*i+k+2];
        t2+=spur_power[i]*spur_table[2*i+k+1]-
            spur_power[i+1]*spur_table[2*i+k+3];
        }
      if((spur_location[spurno]&1)==1)
        {
        t1=-t1;
        t2=-t2;
        }      
      zsig[2*np  ]=t1;  
      zsig[2*np+1]=t2;  
      sp_sig[2*izz  ]=t1;
      sp_sig[2*izz+1]=t2;
      izz++;
      np=(np+1)&fftxn_mask;
      }
    }  
  else  
    {
    while(np != nx)
      {
      k=spurp0+np*SPUR_WIDTH*twice_rxchan;
      t1=0;
      t2=0;
      t3=0;
      t4=0;
      for(i=0; i<SPUR_WIDTH-1; i+=2)
        {
        t1+=spur_power[i]*spur_table[4*i+k  ]-
            spur_power[i+1]*spur_table[4*i+k+4];
        t2+=spur_power[i]*spur_table[4*i+k+1]-
            spur_power[i+1]*spur_table[4*i+k+5];
        t3+=spur_power[i]*spur_table[4*i+k+2]-
            spur_power[i+1]*spur_table[4*i+k+6];
        t4+=spur_power[i]*spur_table[4*i+k+3]-
            spur_power[i+1]*spur_table[4*i+k+7];
        }
      if((spur_location[spurno]&1)==1)
        {
        r1=-sp_c1*t1-sp_c2*t3-sp_c3*t4;
        r2=-sp_c1*t2-sp_c2*t4+sp_c3*t3;
        r3=-sp_c1*t3+sp_c2*t1-sp_c3*t2;
        r4=-sp_c1*t4+sp_c2*t2+sp_c3*t1;
        }
      else
        {
        r1=sp_c1*t1+sp_c2*t3+sp_c3*t4;
        r2=sp_c1*t2+sp_c2*t4-sp_c3*t3;
        r3=sp_c1*t3-sp_c2*t1+sp_c3*t2;
        r4=sp_c1*t4-sp_c2*t2-sp_c3*t1;
        }
      zsig[4*np  ]=r1;  
      zsig[4*np+1]=r2;  
      zsig[4*np+2]=r3;  
      zsig[4*np+3]=r4;  
      sp_sig[2*izz  ]=r1;
      sp_sig[2*izz+1]=r2;
//if(mailbox[1]==1)fprintf( dmp,"\nB sp_sig  %f %f   %f %f",r1,r2,r3,r4);
      izz++;
      np=(np+1)&fftxn_mask;
      }
    }
  }
  
spur_phase_parameters();    
if(spur_ampl[spurno] < 3*spur_noise[spurno]/sqrt((float)(spur_speknum)))
  {
//  if(mailbox[1]==1)fprintf( dmp,"\nnoise test fail: %f < %f ",spur_ampl[spurno],3*spur_noise[spurno]/sqrt((float)(spur_speknum)));
  return -1;
  }
i=verify_spur_pll();
spur_avgd2[spurno]=spur_d2pha[spurno];
return i;
}

int verify_spur_pll(void)
{
int iter, i, j, ind, knd;
int izz;
int ni, pnt;
float sigspek[SPUR_WIDTH+2],errspek[SPUR_WIDTH+2];
float t1,t2,t3,t4,r1,r2,r3,r4;
float a1, a2, b1, b2, d1, d2;
float average_rot,rot;
float freq, d2err, d1err, d0err;
float phase_slope,phase_curv;
float *zsig, *z;
short int *zmmx;
int *uind;
// Use the phases to transform the spur signal.
// Ideally the signal should then be entirely in I.
// Make average_rot the phase shift from transform to transform
// based on the average power spectrum.
// Note that this phase rotation is many turns.
// We use average_rot to determine the number of whole turns
// while the decimals are determined from the transform to transform
// phase shifts we get from the phase first derivative. 
// When we know rot with accurate decimals, we can go back to
// frequency (in fft bins) carrying the improved accuracy.
// The decimals of the frequency are used to index spur_spectra
// which contains normalised reference spectra with the same
// frequency.
// Use reference spectra and 90 degree phase shifted reference
// spectra to get I and Q, normalised amplitudes for the
// in phase and out of phase components of the spur signal.
// If our phases were correct I is amplitude while Q is small
// and contains the phase error.
// The procedure is a PLL loop.
// We will use Q to improve the phase function, our local oscillator.
// The phase function is low pass filtered (PLL loop filter) which
// gives the spur removal a narrow bandwidth.
zsig=&spur_signal[twice_rxchan*max_fftxn*spurno];
uind=&spur_ind[spurno*max_fftxn];
freq=spur_freq[spurno];
average_rot=freq*spur_freq_factor;
d0err=0;
d1err=0;
d2err=0;
iter=0;
/*
if(mailbox[1]==1)fprintf( dmp,"\n\n  VERIFY ");
if(mailbox[1]==1)
{
fprintf( dmp,"\n************  %d ********************",spur_location[spurno]);
for(i=0; i<spur_speknum; i++)
  {
  fprintf( dmp,"\n%d    %f   %f   angle %f",i,sp_sig[2*i],sp_sig[2*i+1],atan2(sp_sig[2*i+1],sp_sig[2*i])/PI_L);
  }
}
*/
restart:;
iter++;
a1=cos(spur_d0pha[spurno]);
a2=sin(spur_d0pha[spurno]);
b1=cos(spur_d1pha[spurno]);
b2=sin(spur_d1pha[spurno]);
d1=cos(spur_d2pha[spurno]);
d2=sin(spur_d2pha[spurno]);
phase_slope=spur_d1pha[spurno];
phase_curv=spur_d2pha[spurno];
ni=(ffts_na+fftxn_mask)&fftxn_mask;
izz=spur_speknum-1;
//if(mailbox[1]==1)fprintf( dmp,"\n-------------------------");
while(izz >= 0)
  {
  rot=-0.5*phase_slope/PI_L;
  i=average_rot-rot+0.5;
  rot+=i;
  freq=rot/spur_freq_factor;
  j=(int)(freq)+2-spur_location[spurno]-SPUR_SIZE/2;
  if(j<0)j=0;
  j=1-j;
  if(j<0)j=0;
  ind=NO_OF_SPUR_SPECTRA*(freq-(int)(freq));
  if(ind==NO_OF_SPUR_SPECTRA)ind=NO_OF_SPUR_SPECTRA-1;
  ind=ind*SPUR_SIZE+j;
  uind[ni]=ind;  
  knd=spurp0+ni*SPUR_WIDTH*twice_rxchan;
  if(sw_onechan)
    {
    r1=0;
    r2=0;
    if(swmmx_fft2)
      {
      for(i=0; i<SPUR_WIDTH; i++)
        {
        r1+=spur_spectra[ind+i]*spur_table_mmx[2*i+knd  ];
        r2+=spur_spectra[ind+i]*spur_table_mmx[2*i+knd+1];
        }
      }
    else
      {    
      for(i=0; i<SPUR_WIDTH; i++)
        {
        r1+=spur_spectra[ind+i]*spur_table[2*i+knd  ];
        r2+=spur_spectra[ind+i]*spur_table[2*i+knd+1];
        }
      }
    }
  else
    {
    t1=0;
    t2=0;
    t3=0;
    t4=0;
    if(swmmx_fft2)
      {
      for(i=0; i<SPUR_WIDTH; i++)
        {
        t1+=spur_spectra[ind+i]*spur_table_mmx[4*i+knd  ];
        t2+=spur_spectra[ind+i]*spur_table_mmx[4*i+knd+1];
        t3+=spur_spectra[ind+i]*spur_table_mmx[4*i+knd+2];
        t4+=spur_spectra[ind+i]*spur_table_mmx[4*i+knd+3];
        }
      }  
    else
      {
      for(i=0; i<SPUR_WIDTH; i++)
        {
        t1+=spur_spectra[ind+i]*spur_table[4*i+knd  ];
        t2+=spur_spectra[ind+i]*spur_table[4*i+knd+1];
        t3+=spur_spectra[ind+i]*spur_table[4*i+knd+2];
        t4+=spur_spectra[ind+i]*spur_table[4*i+knd+3];
        } 
      }       
    r1=sp_c1*t1+sp_c2*t3+sp_c3*t4;
    r2=sp_c1*t2+sp_c2*t4-sp_c3*t3;
    r3=sp_c1*t3-sp_c2*t1+sp_c3*t2;
    r4=sp_c1*t4-sp_c2*t2-sp_c3*t1;
    if((j^(spur_location[spurno]&1))==1)
      {
      r3=-r3;
      r4=-r4;
      }
    zsig[4*ni+2]=r3;
    zsig[4*ni+3]=r4;
    }  
  if((j^(spur_location[spurno]&1))==1)
    {
    r1=-r1;
    r2=-r2;
    }
  zsig[twice_rxchan*ni  ]=r1;
  zsig[twice_rxchan*ni+1]=r2;
  sp_sig[2*izz  ]=r1*a1+r2*a2;
  sp_sig[2*izz+1]=r2*a1-r1*a2;
  ni=(ni+fftxn_mask)&fftxn_mask;
  izz--;
  r1=a1*b1+a2*b2;
  a2=a2*b1-a1*b2;
  a1=r1;
  r2=b1*d1+b2*d2;
  b2=b2*d1-b1*d2;
  b1=r2;
  phase_slope-=phase_curv;
  } 
spur_phase_parameters();    
//if(mailbox[1]==1)fprintf( dmp,"\nnoise test: %f < %f ",spur_ampl[spurno],3*spur_noise[spurno]/sqrt((float)(spur_speknum)));

if(spur_ampl[spurno] < 3*spur_noise[spurno]/sqrt((float)(spur_speknum)))
  {
//  if(mailbox[1]==1)fprintf( dmp,"\nnoise test: %f < %f ",spur_ampl[spurno],3*spur_noise[spurno]/sqrt((float)(spur_speknum)));
  return -1;  
  }
spur_d0pha[spurno]+=sp_d0;
spur_d1pha[spurno]+=sp_d1;
spur_d2pha[spurno]+=sp_d2;
while(spur_d0pha[spurno] > PI_L)spur_d0pha[spurno]-=2*PI_L;
while(spur_d0pha[spurno] <-PI_L)spur_d0pha[spurno]+=2*PI_L;
while(spur_d1pha[spurno] > PI_L)spur_d1pha[spurno]-=2*PI_L;
while(spur_d1pha[spurno] <-PI_L) spur_d1pha[spurno]+=2*PI_L;
while(spur_d2pha[spurno] > PI_L)spur_d2pha[spurno]-=2*PI_L;
while(spur_d2pha[spurno] <-PI_L)spur_d2pha[spurno]+=2*PI_L;
phase_slope=spur_d1pha[spurno];
phase_curv=spur_d2pha[spurno];
rot=-0.5*phase_slope/PI_L;
i=average_rot-rot+0.5;
rot+=i;
spur_freq[spurno]=rot/spur_freq_factor;
/*
if(mailbox[1]==1)
{
fprintf( dmp,"\n************  %d ********************",spur_location[spurno]);
for(i=0; i<spur_speknum; i++)
  {
  fprintf( dmp,"\n%d    %f   %f   angle %f",i,sp_sig[2*i],sp_sig[2*i+1],atan2(sp_sig[2*i+1],sp_sig[2*i])/PI_L);
  }
fprintf( dmp,"\nd0 %f,%f  d1 %f,%f   d2 %f,%f",sp_d0,d0err,sp_d1,d1err,sp_d2,d2err);
}
*/
if(iter > 1 &&
         fabs(sp_d0) < 0.1 &&
          fabs(sp_d1) < 0.01 &&
             fabs(sp_d2) < 0.001 &&
                  fabs(d0err) < 0.3 &&
                      fabs(d1err) < 0.03 &&
                          fabs(d2err) < 0.003 )
  {
// Check the spectrum of what would be left when the spur is subtracted.
/*
if(mailbox[1]==1)
{
fprintf( dmp,"\n------------  %d - %f -----------",spur_location[spurno],spur_ampl[spurno]);
for(i=0; i<spur_speknum; i++)
  {
  fprintf( dmp,"\n%d    %f   %f",i,sp_sig[2*i],sp_sig[2*i+1]);
  }
}
*/
  for(i=0; i<SPUR_WIDTH+2; i++)
    {
    sigspek[i]=0;
    errspek[i]=0;
    }
#define ZX 0.01
  average_rot=spur_freq[spurno]*spur_freq_factor;
  a1=spur_ampl[spurno]*cos(spur_d0pha[spurno]);
  a2=spur_ampl[spurno]*sin(spur_d0pha[spurno]);
  b1=cos(spur_d1pha[spurno]);
  b2=sin(spur_d1pha[spurno]);
  d1=cos(spur_d2pha[spurno]);
  d2=sin(spur_d2pha[spurno]);
//  if(mailbox[1]==1)fprintf( dmp,"\nphase: d0 %f  d1 %f  d2 %f",spur_d0pha[spurno],spur_d1pha[spurno],spur_d2pha[spurno]);
  phase_slope=spur_d1pha[spurno];
  phase_curv=spur_d2pha[spurno];
  ni=(ffts_na+fftxn_mask)&fftxn_mask;
  pnt=spur_location[spurno];
  izz=spur_speknum-1;
  while(izz >= 0)
    {
    rot=-0.5*phase_slope/PI_L;
    i=average_rot-rot+0.5;
    rot+=i;
    freq=rot/spur_freq_factor;
    j=(int)(freq)+2-pnt-SPUR_SIZE/2;
    if(j<0)j=0;
    j=1-j;
    if(j<0)j=0;
    ind=uind[ni];
    if(sw_onechan)
      {
      if((j^(spur_location[spurno]&1))==1)
        {
        t1=-a1;
        t2=-a2;
        }
      else
        {
        t1=a1;
        t2=a2;
        }
      if(swmmx_fft2)
        {
        zmmx=&fft2_short_int[twice_rxchan*(ni*fft2_size+pnt)];
        for(i=0; i<SPUR_WIDTH; i++)
          {
          sigspek[i+1]+=spur_spectra[ind+i]*spur_spectra[ind+i]*(t1*t1+t2*t2);
          errspek[i+1]+=pow(zmmx[2*i  ]-spur_spectra[ind+i]*t1,2.0)+
                        pow(zmmx[2*i+1]-spur_spectra[ind+i]*t2,2.0);
          }
        errspek[0]+=pow((float)(zmmx[-2]),2.0)+
                    pow((float)(zmmx[-1]),2.0);
        errspek[SPUR_WIDTH+1]+=pow((float)(zmmx[2*(SPUR_WIDTH+1)  ]),2.0)+
                               pow((float)(zmmx[2*(SPUR_WIDTH+1)+1]),2.0);
        }
      else
        {  
        z=&fftx[twice_rxchan*(ni*fftx_size+pnt)];
        for(i=0; i<SPUR_WIDTH; i++)
          {
          sigspek[i+1]+=spur_spectra[ind+i]*spur_spectra[ind+i]*(t1*t1+t2*t2);
/*
if(mailbox[1]==1)fprintf( dmp,"\nSUB %d %d:  (%f,%.3f)   (%f,%.3f)    (%f,%.3f)",ni,i,
       ZX*sqrt(pow(z[2*i  ],2.0)+pow(z[2*i+1],2.0)),atan2(z[2*i+1],z[2*i])/PI_L,
       ZX*sqrt(pow(spur_spectra[ind+i]*t1,2.0)+pow(spur_spectra[ind+i]*t2,2.0)),
                atan2(spur_spectra[ind+i]*t2,spur_spectra[ind+i]*t1)/PI_L,
       ZX*sqrt(pow(z[2*i  ]-spur_spectra[ind+i]*t1,2.0)+
                      pow(z[2*i+1]-spur_spectra[ind+i]*t2,2.0)),
       atan2(z[2*i+1]-spur_spectra[ind+i]*t2,z[2*i  ]-spur_spectra[ind+i]*t1)/PI_L);
*/       
          errspek[i+1]+=pow(z[2*i  ]-spur_spectra[ind+i]*t1,2.0)+
                        pow(z[2*i+1]-spur_spectra[ind+i]*t2,2.0);
          }

        errspek[0]+=pow(z[-2],2.0)+pow(z[-1],2.0);
        errspek[SPUR_WIDTH+1]+=pow(z[2*(SPUR_WIDTH+1)  ],2.0)+
                               pow(z[2*(SPUR_WIDTH+1)+1],2.0);
        }
      }
    else
      {
      if((j^(spur_location[spurno]&1))==1)
        {
        t1=-sp_c1*a1;
        t2=-sp_c1*a2;
        t3=-sp_c2*a1+sp_c3*a2;
        t4=-sp_c2*a2-sp_c3*a1;
        }
      else
        {
        t1=sp_c1*a1;
        t2=sp_c1*a2;
        t3=sp_c2*a1-sp_c3*a2;
        t4=sp_c2*a2+sp_c3*a1;
        }
      if(swmmx_fft2)
        {
        zmmx=&fft2_short_int[twice_rxchan*(ni*fft2_size+pnt)];
        for(i=0; i<SPUR_WIDTH; i++)
          {
          sigspek[i+1]+=spur_spectra[ind+i]*spur_spectra[ind+i]*
                                             (t1*t1+t2*t2+t3*t3+t4*t4);


          errspek[i+1]+=pow(zmmx[4*i  ]-spur_spectra[ind+i]*t1,2.0)+
                      pow(zmmx[4*i+1]-spur_spectra[ind+i]*t2,2.0)+
                      pow(zmmx[4*i+2]-spur_spectra[ind+i]*t3,2.0)+
                      pow(zmmx[4*i+3]-spur_spectra[ind+i]*t4,2.0);
          }
        errspek[0]+=pow((float)(zmmx[-4]),2.0)+
                    pow((float)(zmmx[-3]),2.0)+
                    pow((float)(zmmx[-2]),2.0)+
                    pow((float)(zmmx[-1]),2.0);
        errspek[SPUR_WIDTH+1]+=pow((float)(zmmx[4*(SPUR_WIDTH+1)  ]),2.0)+
                               pow((float)(zmmx[4*(SPUR_WIDTH+1)+1]),2.0)+
                               pow((float)(zmmx[4*(SPUR_WIDTH+1)+2]),2.0)+
                               pow((float)(zmmx[4*(SPUR_WIDTH+1)+3]),2.0);
        }
      else
        {    
        z=&fftx[twice_rxchan*(ni*fftx_size+pnt)];
        for(i=0; i<SPUR_WIDTH; i++)
          {
          sigspek[i+1]+=spur_spectra[ind+i]*spur_spectra[ind+i]*
                                            (t1*t1+t2*t2+t3*t3+t4*t4);
/*
if(mailbox[1]==1)fprintf( dmp,"\nSUB %d %d:[(%f,%.3f)(%f,%.3f)]   %f  {%f,%f,%f,%f)",ni,i,
       ZX*z[4*i  ],ZX*z[4*i+1], ZX*z[4*i+2],ZX*z[4*i+3],

       ZX*spur_spectra[ind+i],
       ZX*spur_spectra[ind+i]*t1,
       ZX*spur_spectra[ind+i]*t2,
       ZX*spur_spectra[ind+i]*t3,
       ZX*spur_spectra[ind+i]*t4);
*/
          errspek[i+1]+=pow(z[4*i  ]-spur_spectra[ind+i]*t1,2.0)+
                        pow(z[4*i+1]-spur_spectra[ind+i]*t2,2.0)+
                        pow(z[4*i+2]-spur_spectra[ind+i]*t3,2.0)+
                        pow(z[4*i+3]-spur_spectra[ind+i]*t4,2.0);
          }
        errspek[0]+=pow(z[-4],2.0)+pow(z[-3],2.0)+pow(z[-2],2.0)+pow(z[-1],2.0);
        errspek[SPUR_WIDTH+1]+=pow(z[4*(SPUR_WIDTH+1)  ],2.0)+
                               pow(z[4*(SPUR_WIDTH+1)+1],2.0)+
                               pow(z[4*(SPUR_WIDTH+1)+2],2.0)+
                               pow(z[4*(SPUR_WIDTH+1)+3],2.0);
        }
      }  
    ni=(ni+fftxn_mask)&fftxn_mask;
    izz--;
    r1=a1*b1+a2*b2;
    a2=a2*b1-a1*b2;
    a1=r1;
    r2=b1*d1+b2*d2;
    b2=b2*d1-b1*d2;
    b1=r2;
    phase_slope-=phase_curv;
    }  
// Compute the RMS noise amplitude within the fftx bandwidth
// while converting errspek[] to amplitude.
  r1=0;
  for(i=0; i<SPUR_WIDTH+2; i++)
    {
    errspek[i]/=spur_speknum;
    r1+=errspek[i];
    errspek[i]=sqrt(errspek[i]);
//if(mailbox[1]==1)  fprintf( dmp,"\n%d N %f",i,errspek[i]);
    }
  r1=sqrt(r1/(SPUR_WIDTH+2));
// Check whether the noise floor is flat.
  r2=0;
  for(i=0; i<SPUR_WIDTH+2; i++)
    {
    r2+=pow(errspek[i]-r1,2.0);
    }
  r2=sqrt(r2/(SPUR_WIDTH+2));
// r1 is now the amplitude of the noise floor.
// r2 is the standard deviation of the noise floor.
// Spurs may be modulated to some extent, that would lead
// to a non-flat noise floor so we have to accept some
// contribution due to modulation.
// The RMS
//if(mailbox[1]==1)fprintf( dmp,"\nampl %f   noi  %f",spur_ampl[spurno],spur_noise[spurno]);
//if(mailbox[1]==1)fprintf( dmp,"\nerr: %f   spread %f  S/N %f",spur_noise[spurno]/r1,r2/r1,spur_ampl[spurno]/r1);
//if(mailbox[1]==1)fprintf( dmp,"\ntest:%f > %f+%f",r2/r1,0.5/sqrt((int)(spur_speknum)),0.02*spur_ampl[spurno]/r1);
  if( r2 > 0.5*r1/sqrt((int)(spur_speknum))+0.02*spur_ampl[spurno])
    {
//    if(mailbox[1]==1)fprintf( dmp,"\nSPREAD failed");
    return -1;
    }



  return 0;
  }
d0err=sp_d0;
d1err=sp_d1;
d2err=sp_d2;

if(iter < 5  )  goto restart;
return -1;
}
