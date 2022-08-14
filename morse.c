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
#include "sigdef.h"
#include "screendef.h"

#define ZZ 0.0000001

// Variables and definitions for morse decoding in sigdef.h.
// typedef struct {
// unsigned char type;    // Part type (CW_DOT, CW_SPACE,....A,B,C,...)
// unsigned char unkn;    // Number of unknown states before this part
// unsigned char len;     // Length of character in states 
// float midpoint;
// float sep;
// float re;
// float im;
// float tmp;
// }MORSE_DECODE_DATA;
//
// Defines for MORSE_DECODE_DATA.type (cw.type)
// Note that CW_DASH must have the lowest value.
// The data in cw_item_len[255-CW_DASH]; must agree with these definitions!
//
// #define CW_WORDSEP 255
// #define CW_SPACE 254
// #define CW_DOT 253
// #define CW_DASH 252

//   name     data    length
// CW_DASH    |---_|     4
//              ^
// CW_DOT     |-_|       2 
//             ^
// CW_SPACE   |__|       2
//             ^  
// CW_WORDSEP |____|     4 
//              ^
// The way the midpoint is defined does not include the trailing
// key-up of each cw part.
// For characters, midpoint points to the first dash or dot
// of the decoded character.



void check_cw(int num,int type);
void show_cw(char *s);
void find_preceeding_part(void);
void find_prev_char_part(void);  
void extrapolate_region_downwards(void);
int mr_dotpos_nom, mr_dashpos_nom;

void insert_char(int charbits, int charlen, int charval)
{
char chr;
int i, j;
float r1;
if(charbits > 6)
  {
  chr=(char)243;
  }
else
  {
  cw_decoded_chars++;
  chr=morsascii[charbits-1][charval];
  }
// The current character extends from cw_ptr-charbits to cw_ptr-1 
// Include the trailing space at cw_ptr in the character, 
// but leave trailing word separators unchanged.
// If a word separator preceeded by a character is present 
// at cw_ptr-charbits-1, replace the word separator by a blank.
i=cw_ptr-charbits;
if(i >= 2)
  {
  if(cw[i-1].type == CW_WORDSEP && cw[i-2].type < CW_DASH)
    {
    cw[i-1].type=' ';
    }
  }  
cw[i].type=chr;
cw[i].len=charlen;
j=cw_ptr;
if(cw[cw_ptr].type == CW_SPACE)j++;
cw_ptr=i;
i++;
if(j < no_of_cwdat)
  {
  r1=cw[j].midpoint-cw[cw_ptr].midpoint;
  if(r1 < 0)r1+=baseband_size;
  cw[j].sep=r1;
  }
while(j < no_of_cwdat)
  {
  cw[i]=cw[j];
  i++;
  j++;
  }
no_of_cwdat=i-1;  
}

void detect_previous_character(int char_pos)
{
int i, j, charlen, charbits, charval;
if(char_pos < 1)
  {
  lirerr(674421);
  return;
  }
if(cw[char_pos].type != CW_SPACE && cw[char_pos].type != CW_WORDSEP)
  {
  show_cw("ERROR in detect_previous_character");
  lirerr(674422);
  return;
  }
// We have been called properly with the pointer on a space.
// Step backwards and see if it is preceeded by something
// we are likely to interpret correctly.
cw_ptr=char_pos-1;
restart:;
charlen=0;
charbits=0;
collect_parts:;
while(cw_ptr >= 0 && (cw[cw_ptr].type==CW_DASH || cw[cw_ptr].type==CW_DOT))
  {
  charbits++;
  charlen+=cw[cw_ptr].len;
  cw_ptr--;
  }
if(charbits==0)return;
cw_ptr++;  
XZ("xx1");
if( cw[cw_ptr].unkn > 0 && 
    cw[cw_ptr].type != CW_SPACE && 
    cw[cw_ptr].type != CW_WORDSEP)
  {
  XZ("xx2");
// We do not know what preceeds this sequence of dots and dashes
// and whatever it was, it was not good enough to give a morse decode.
// Try to fit a dot or a dash in the appropriate position.
  i=no_of_cwdat;
  find_preceeding_part();  
if(kill_all_flag)return;
  if(i != no_of_cwdat)
    {
    XZ("xx3");
    goto collect_parts;
    }
  }
//show_cw("  XXXX  ");
if( cw[cw_ptr-1].type == CW_SPACE || cw[cw_ptr-1].type == CW_WORDSEP)
  {
  charlen=0;
  charval=0;
  j=charbits;
  while(j > 0)
    {
    j--;
    charlen+=cw[cw_ptr].len;  
    charval<<=1;
    if(cw[cw_ptr].type==CW_DASH)charval++;
    cw_ptr++;
    }
  insert_char(charbits, charlen, charval);
// Our search for code parts begins by a search for dashes.
// A sequence without dashes or with very poor ones may
// preceed what we now have detected.
  cw_ptr--;
  if(cw[cw_ptr].unkn > 0)
    {
    i=no_of_cwdat;
    find_prev_char_part();
if(kill_all_flag)return;
    if(i != no_of_cwdat)
      {
      XZ("goto restart");
      goto restart;
      }
    }
  }
else
  {
  DEB"\nWARNING: ROUTINE NOT COMPLETED  ");
  lirerr(564397);
  }
}

void conditional_insert_dashdot(void)
{
lirerr(8888002);
return;
/*
int sizhalf;
float dash_re, dash_im, dashpos, dashpos_err, dash_fit;
float dot_re, dot_im, dotpos, dotpos_err, dot_fit;
sizhalf=baseband_size>>1;
cg_wave_start=mr_dashpos_nom-(dash_pts>>1)+baseband_size;
cg_wave_start&=baseband_mask;
fit_dash();
dash_re=cg_wave_coh_re;
dash_im=cg_wave_coh_im;
dashpos=cg_wave_midpoint;
dashpos_err=mr_dashpos_nom-cg_wave_midpoint;
if(dashpos_err < -sizhalf)dashpos_err+=baseband_size;
dash_fit=cg_wave_fit; 
cg_wave_start=mr_dotpos_nom-(dot_pts>>1)+baseband_size;
cg_wave_start&=baseband_mask;
fit_dot();
dot_re=cg_wave_coh_re;
dot_im=cg_wave_coh_im;
dotpos=cg_wave_midpoint;
dotpos_err=mr_dotpos_nom-cg_wave_midpoint;
if(dotpos_err < -sizhalf)dotpos_err+=baseband_size;
dot_fit=cg_wave_fit; 
//fprintf( dmp,"\n(fit) dot: %f  dash: %f",dot_fit,dash_fit);
//fprintf( dmp,"\n(err) dot: %f  dash: %f",dotpos_err,dashpos_err);
if( dash_fit >DASH_DETECT_LIMIT && 
    dash_fit - dot_fit > 0.05 &&
    fabs(dashpos_err) < 0.9 * cwbit_pts &&
    cw[cw_ptr].unkn > 4)
  {
  insert_item(CW_DASH);
if(kill_all_flag)return;
  cw[cw_ptr].unkn-=4;
  cw[cw_ptr+1].unkn=0;
XZ("insert dash");
  return;
  }
if( dot_fit > DOT_DETECT_LIMIT &&
    dot_fit-dash_fit > 0.05 &&
    fabs(dotpos_err) < 0.9 * cwbit_pts)
  {
  insert_item(CW_DOT);
if(kill_all_flag)return;
  cw[cw_ptr].unkn-=2;
  cw[cw_ptr+1].unkn=0;
XZ("insert dot");
  return;
  }
*/  
}



void find_prev_char_part(void)
{
int i;
float t1;
if(cw[cw_ptr].type == CW_SPACE)
  {
  i=3;
  }
else  // CW_WORDSEP
  {
  i=4;
  }
t1=baseband_size+cw[cw_ptr].midpoint-i*cwbit_pts+0.5;
mr_dotpos_nom=((int)t1)&baseband_mask;    
t1-=cwbit_pts;
mr_dashpos_nom=((int)t1)&baseband_mask;    
conditional_insert_dashdot();
}


void find_preceeding_part(void)
{
lirerr(888100);
/*
int i;
float dash_chk, dot_chk;
if( cw_ptr > 0 && cw[cw_ptr].unkn < 8 &&
    (cw[cw_ptr-1].type == CW_DOT || cw[cw_ptr-1].type == CW_DASH))
  {
  set_region_envelope();
  }
else
  {
  extrapolate_region_downwards();
  }  
if(cw[cw_ptr].type == CW_DASH)
  {
  i=4;
  }
else  // CW_DOT
  {
  i=3;
  }
mr_dashpos_nom=cw[cw_ptr].midpoint+baseband_size-i*cwbit_pts+0.5;
mr_dashpos_nom&=baseband_mask;
mr_dotpos_nom=(mr_dashpos_nom+(int)(cwbit_pts+0.5))&baseband_mask;
dot_chk=check_dot(mr_dotpos_nom);
if(cw[cw_ptr].unkn > 2)
  {
// See if a dash will fit well.
  dash_chk=check_dash(mr_dashpos_nom);
  if(dash_chk < -0.25 && dot_chk < -0.25)
    {
    cg_wave_midpoint=mr_dashpos_nom;
    cg_wave_coh_re=baseb_envelope[2*mr_dashpos_nom];
    cg_wave_coh_im=baseb_envelope[2*mr_dashpos_nom+1];
    insert_item(CW_WORDSEP);
if(kill_all_flag)return;
    cw[cw_ptr].unkn-=4;
    cw[cw_ptr+1].unkn=0;
    return;
    }
  }
else
  {
  dash_chk=-1;
  }
if(dash_chk < 0.25 && dot_chk < 0.25)return;
conditional_insert_dashdot();
*/
}

void decoded_cwspeed(void)
{
float t1, t2, r1, lsum_real, lsum_ideal;
int i, k;
cw_ptr=1;
lsum_real=0;
lsum_ideal=0;
t1=0;
t2=0;
while(cw_ptr < no_of_cwdat)
  {
// Get the length of the gap between decoded dots and dashes.
  r1=cw[cw_ptr].sep/cwbit_pts;
  r1-=0.5*(cw[cw_ptr].len+cw[cw_ptr-1].len);
  if(r1 < 0) r1=0;
  k=(int)(r1)&0xfffffffe;
  if(r1-k > 1)k+=2;
// Accumulate regions where we see the keying reasonably well.
  if(k<=8 && fabs(r1-k) < 0.5)
    {
    lsum_real+=cw[cw_ptr].sep;
    lsum_ideal+=k+0.5*(cw[cw_ptr].len+cw[cw_ptr-1].len);
    }
  else
    {
    t1+=lsum_ideal*lsum_real;  //ideal*ideal*(real/ideal)  
    t2+=lsum_ideal*lsum_ideal; 
    lsum_real=0;
    lsum_ideal=0;
    }  
  cw_ptr++;
  }
t1+=lsum_ideal*lsum_real;  //ideal*ideal*(real/ideal)  
t2+=lsum_ideal*lsum_ideal; 
// Get the average ratio of real to ideal, weighted by ideal squared.
// Store as an improved value for cw speed
fprintf( dmp,"\ndecoded_cwspeed t1 %f t2 %f",t1,t2);
cwbit_pts=t1/t2;
// Compute the length of un-decoded regions and store in cw[].unkn
i=baseband_size+cw[0].midpoint-baseb_px;
i&=baseband_mask;
cw[0].unkn=(float)(i)/cwbit_pts-cw[0].len/2;
cw_ptr=1;
while(cw_ptr < no_of_cwdat)
  {
  r1=cw[cw_ptr].sep/cwbit_pts;
  r1-=0.5*(cw[cw_ptr].len+cw[cw_ptr-1].len);
  if(r1 < 0) r1=0;
  k=(int)(r1)&0xfffffffe;
  if(r1-k > 1)k+=2;
  cw[cw_ptr].unkn=k;
  cw_ptr++;
  }
}


void continued_morse_decode(void)
{
int charflag,charlen,charval,charbits;
cw_ptr=1;
charflag=0;
charlen=0;
charval=0;
charbits=0;
while(cw_ptr < no_of_cwdat)
  {
  if(cw[cw_ptr].unkn != 0)charflag=0;
  if(charbits != 0 && charflag == 1 && 
               (cw[cw_ptr].type == CW_WORDSEP || cw[cw_ptr].type == CW_SPACE))
    {
// This is a character surrounded by spaces.   
// Get an ascii character from the table.
    insert_char(charbits, charlen, charval);
    }
  if(cw[cw_ptr].type != CW_DASH && cw[cw_ptr].type != CW_DOT)
    {
    charflag=1;
    charlen=0;
    charval=0;
    charbits=0;
    }
  else
    {
    charbits++;
    charlen+=cw[cw_ptr].len;  
    charval<<=1;
    if(cw[cw_ptr].type==CW_DASH)charval++;
    }  
  cw_ptr++;
  }
}


void first_morse_decode(void)
{
int charflag,charlen,charval,charbits;
int first_char_pos;
// We have stored dashes and dots that are next to a dash.
// Look through the data and see if we can find characters.
//show_cw("  AAAA  ");
cw_decoded_chars=0;
first_char_pos=-1;
cw_ptr=1;
charflag=0;
charlen=0;
charval=0;
charbits=0;
while(cw_ptr < no_of_cwdat)
  {
  if(cw[cw_ptr].unkn != 0)charflag=0;
  if(charbits != 0 && charflag == 1 && 
               (cw[cw_ptr].type == CW_WORDSEP || cw[cw_ptr].type == CW_SPACE))
    {
// This is a character surrounded by spaces.   
// Get an ascii character from the table.
    insert_char(charbits, charlen, charval);
    if(first_char_pos==-1)first_char_pos=cw_ptr;
    }
  if(cw[cw_ptr].type != CW_DASH && cw[cw_ptr].type != CW_DOT)
    {
    charflag=1;
    charlen=0;
    charval=0;
    charbits=0;
    }
  else
    {
    charbits++;
    charlen+=cw[cw_ptr].len;  
    charval<<=1;
    if(cw[cw_ptr].type==CW_DASH)charval++;
    }  
  cw_ptr++;
  }
if(cw_decoded_chars > 0)
  {
  if(first_char_pos > 1)
  DEB"\nFirst char pos: %d",first_char_pos);
  detect_previous_character(first_char_pos-1);
  }
}





void remove_dash(void)
{
float t1;
int ia, ib;
ia=cw[cw_ptr].midpoint-0.5*dash_pts+baseband_size;
ia&=baseband_mask;
ib=(ia+dash_pts+1)&baseband_mask;
while(ia != ib)
  {
  baseb_fit[2*ia]=0;
  baseb_fit[2*ia+1]=0;
  ia=(ia+1)&baseband_mask;
  }
ia=cw_ptr+1;
while(ia < no_of_cwdat)
  {
  cw[ia-1]=cw[ia];
  ia++;
  }
if(cw_ptr !=0)
  {  
  t1=cw[cw_ptr].midpoint-cw[cw_ptr-1].midpoint;
  if(t1 < 0)t1+=baseband_size;
  cw[cw_ptr].sep=t1;
  }
no_of_cwdat--;
}  


void set_long_region_envelope(void)
{
int i, ia, ib, len;
float t1,t2,r1,r2;
// We have two dashes pointed to by cw_ptr and cw_ptr-1.
// We know the envelope at these points but the separation is large.
// Set envelope at both ends by extrapolation.
ia=cw[cw_ptr-1].midpoint;
ia+=(dash_pts>>2);
ia&=baseband_mask;
ib=cw[cw_ptr].midpoint;
t1=cw[cw_ptr-1].coh_re;
r1=cw[cw_ptr-1].coh_im;
t2=cw[cw_ptr].coh_re;
r2=cw[cw_ptr].coh_im;
ib+=baseband_size-(dash_pts>>2);
ib&=baseband_mask;
len=10*cwbit_pts;
i=0;
while( i < len )
  {
  baseb_envelope[2*ia]=t1;
  baseb_envelope[2*ia+1]=r1;
  baseb_envelope[2*ib]=t2;
  baseb_envelope[2*ib+1]=r2;
  ia=(ia+1)&baseband_mask;
  if(ia == ib)goto avgenv;
  ib=(ib+baseband_mask)&baseband_mask;
  if(ia == ib)goto avgenv;
  i++;
  }
return;
avgenv:;
t1=0.5*(t1+t2);
r1=0.5*(r1+r2);
while( i < len )
  {
  baseb_envelope[2*ia]=t1;
  baseb_envelope[2*ia+1]=r1;
  baseb_envelope[2*ib]=t1;
  baseb_envelope[2*ib+1]=r1;
  ia=(ia+1)&baseband_mask;
  ib=(ib+baseband_mask)&baseband_mask;
  i++;
  }
}  

void extrapolate_region_downwards(void)
{
lirerr(8888003);
/*
int ia, ib;
float t1, r1;
// We have a dash or dot pointed to by cw_ptr.
// We know the envelope at this point.
// Set envelope at points preceeding this code part by extrapolation.
ib=cw[cw_ptr].midpoint+baseband_size;
ia=ib-6*cwbit_pts;
ia&=baseband_mask;
t1=cw[cw_ptr].coh_re;
r1=cw[cw_ptr].coh_im;
if(cw[cw_ptr].type == CW_DASH)
  {
  ib-=dash_pts>>2;
  }
else
  {
  ib-=dot_pts>>2;
  }
ib&=baseband_mask;
while( ia != ib )
  {
  baseb_envelope[2*ia]=t1;
  baseb_envelope[2*ia+1]=r1;
  ia=(ia+1)&baseband_mask;
  }
*/  
}

void set_region_envelope(void)
{
lirerr(8888005);
/*
int ia, ib, len;
float t1,t2,r1,r2;
// We have two dashes/dots pointed to by cw_ptr and cw_ptr-1.
// We know the envelope at these points.
// Set a reasonable envelope across the interval by linear interpolation.
ia=cw[cw_ptr-1].midpoint;
if(cw[cw_ptr-1].type == CW_DASH)
  {
  ia+=(dash_pts>>2);
  }
else
  {
  ia+=(dot_pts>>2);
  }
ia&=baseband_mask;
ib=baseband_size+cw[cw_ptr].midpoint;
if(cw[cw_ptr].type == CW_DASH)
  {
  ib-=(dash_pts>>2);
  }
else
  {
  ib-=(dot_pts>>2);
  }
ib&=baseband_mask;
t1=cw[cw_ptr-1].coh_re;
r1=cw[cw_ptr-1].coh_im;
len=(ib-ia+baseband_size)&baseband_mask;
t2=(cw[cw_ptr].coh_re-t1)/len;
r2=(cw[cw_ptr].coh_im-r1)/len;
while(ia != ib)
  {
  t1+=t2;
  r1+=r2;
  baseb_envelope[2*ia]=t1;
  baseb_envelope[2*ia+1]=r1;
  ia=(ia+1)&baseband_mask;
  }
*/
}  

void clear_region(void)
{
int ia, ib;
// We have two dashes pointed to by cw_ptr and cw_ptr-1.
// We know the envelope at these points.
// Clear baseb_fit across the interval.
ia=cw[cw_ptr-1].midpoint+0.5*dash_pts+baseband_size;
ib=cw[cw_ptr].midpoint-0.5*dash_pts+1+baseband_size;
ia&=baseband_mask;
ib&=baseband_mask;
while(ia != ib)
  {
  baseb_fit[2*ia]=0;
  baseb_fit[2*ia+1]=0;
  ia=(ia+1)&baseband_mask;
  }
}  

float check_dash(float pos)
{
int ia, ib, ja, sw;
float c0, t1, t2, r1, r2, f1, f2;
float p,err1,err2;
// Place a dash at pos using the phase/amplitude information 
// in baseb_envelope.
// Calculate the transformation required for optimum similarity
// between the observed waveform in baseb and the dash we have stored.
err1=0;
err2=0;
p=0;
ib=pos;
if( pos-ib < 0.001)pos+=0.001; 
ia=(ib+1)&baseband_mask;
if( (dash_pts&1) == 0)
  {
  c0=0.5*(dash_pts-1);
  }
else
  {
  c0=(dash_pts>>1)+1;
  }  
// pos is the center of where we want to place a dash.
sw=0;
while( sw!=2)
  {
  sw=0;
  t1=ia-pos;
  if(t1<0)t1+=baseband_size;
  t1=c0+t1;
  if(t1 >= dash_pts-1)
    {
    baseb_fit[2*ia  ]=0;
    baseb_fit[2*ia+1]=0;
    sw++;
    }
  else
    {
    ja=t1;
    t1-=ja;
    t2=1-t1;
    f1=t2*dash_waveform[2*ja  ]+t1*dash_waveform[2*ja+2];
    f2=t2*dash_waveform[2*ja+1]+t1*dash_waveform[2*ja+3];
    r1=baseb_envelope[2*ia  ];
    r2=baseb_envelope[2*ia+1];
    baseb_fit[2*ia  ]=r1*f1+r2*f2;
    baseb_fit[2*ia+1]=r1*f2-r2*f1;
    }
  err1+=baseb_totpwr[ia];
  p+=baseb_fit[2*ia  ]*baseb_fit[2*ia  ]+baseb_fit[2*ia+1]*baseb_fit[2*ia+1];
  err2+=(baseb_fit[2*ia  ]-baseb[2*ia  ])*(baseb_fit[2*ia  ]-baseb[2*ia  ])+
        (baseb_fit[2*ia+1]-baseb[2*ia+1])*(baseb_fit[2*ia+1]-baseb[2*ia+1]);
  t1=pos-ib;
  if(t1<0)t1+=baseband_size;
  t1=c0-t1;
  if(t1 < 1)
    {
    baseb_fit[2*ib  ]=0;
    baseb_fit[2*ib+1]=0;
    sw++;
    }
  else
    {
    ja=t1;
    t1-=ja;
    t2=1-t1;
    f1=t2*dash_waveform[2*ja  ]+t1*dash_waveform[2*ja+2];
    f2=t2*dash_waveform[2*ja+1]+t1*dash_waveform[2*ja+3];
    r1=baseb_envelope[2*ib  ];
    r2=baseb_envelope[2*ib+1];
    baseb_fit[2*ib  ]=r1*f1+r2*f2;
    baseb_fit[2*ib+1]=r1*f2-r2*f1;
    }
  err1+=baseb_totpwr[ib];
  p+=baseb_fit[2*ib  ]*baseb_fit[2*ib  ]+baseb_fit[2*ib+1]*baseb_fit[2*ib+1];
  err2+=(baseb_fit[2*ib  ]-baseb[2*ib  ])*(baseb_fit[2*ib  ]-baseb[2*ib  ])+
        (baseb_fit[2*ib+1]-baseb[2*ib+1])*(baseb_fit[2*ib+1]-baseb[2*ib+1]);
  ia=(ia+1)&baseband_mask;
  ib=(ib+baseband_mask)&baseband_mask;
  }
// In a perfect fit (noise free) we expect this result:
//
//       Case     err1     err2
//       dash      p        0
//       space     0        p
//
// Due to the presence of noise we expect this instead:
//
//       Case     err1     err2       err1-err2
//       dash     p+n        n           p
//       space     n        p+n         -p 

return (err1-err2)/p;
}    


void store_dash(void)
{
short int ir;
int ia,ib;
float t2,r2;
// We calculated the dot product between the current waveform and 
// the average waveform for a dash when cg_wave_start was computed.
// Normalise and use to set up fitted waveforms.
ia=cg_wave_midpoint-0.5*dash_pts+baseband_size;
ia&=baseband_mask;
ib=(ia+dash_pts)&baseband_mask;
t2=cg_wave_coh_re;
r2=cg_wave_coh_im;
// Store the average waveform at the optimum phase and amplitude giving
// the smallest residue when subtracted from baseb.
ir=0;
while(ia != ib)
  {
  baseb_fit[2*ia  ]=t2*dash_waveform[2*ir  ]+r2*dash_waveform[2*ir+1];
  baseb_fit[2*ia+1]=t2*dash_waveform[2*ir+1]-r2*dash_waveform[2*ir  ];
  ir++;
  ia=(ia+1)&baseband_mask;
  }
}

