
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
#include <string.h>

#if(OSNUM == OSNUM_WINDOWS)
#include <winsock.h>
#include "wscreen.h"
#define INVSOCK INVALID_SOCKET
#endif
#if(OSNUM == OSNUM_LINUX)
#include "lscreen.h"
#define INVSOCK -1
#endif

#include "uidef.h"
#include "screendef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "fft3def.h"
#include "blnkdef.h"
#include "sigdef.h"
#include "seldef.h"
#include "thrdef.h"
#include "hwaredef.h"
#include "txdef.h"
#include "options.h"

// In case there is a users_extra.c present we insert it here.
#if (USERS_EXTRA_PRESENT == 1)
float users_extra_update_interval;
double users_extra_time;
#include "users_extra.c"
#endif

int local_ag_ps;
void update_meter_graph(void);
void hg_stonbars_redraw(void);
void show_map65_buttons(void);
void show_sellim_buttons(void);

double get_center_frequency(void)
{
double dt1;
if( (ui.converter_mode & CONVERTER_USE) != 0)
  {
  if( (ui.converter_mode & CONVERTER_LO_BELOW) != 0)
    {
    if( (ui.converter_mode & CONVERTER_UP) == 0)
      {
      dt1=converter_offset_mhz+fg.passband_center;  
      }
    else
      {
      dt1=fg.passband_center-converter_offset_mhz;
      }
    }
  else
    {
    dt1=converter_offset_mhz-fg.passband_center;  
    }
  }
else
  {
  dt1=fg.passband_center;
  }
return dt1;
}  

void redraw_radar_graph(void)
{
char s[80];
int i, j, k, kk, m, n, nn;
int xp, yp;
int color;
float t1, t2;
hide_mouse(rg_old_x1,rg_old_x2,rg_old_y1,rg_old_y2);
if(rg_old_time != rg.time)
  {
  sprintf(s,"%f",rg.time);
  s[4]=0;
  show_button(&rgbutt[RG_TIME],s);
  rg_old_time=rg.time;
  }
if(rg_old_gain != rg.gain)
  {
  sprintf(s,"%f",rg.gain);
  s[4]=0;
  show_button(&rgbutt[RG_GAIN],s);
  rg_old_gain=rg.gain;
  }
if(rg_old_zero != rg.zero)
  {
  sprintf(s,"%f",rg.zero);
  s[4]=0;
  show_button(&rgbutt[RG_ZERO],s);
  rg_old_zero=rg.zero;
  }
m=0;
yp=rg_old_y2-1;
if(radar_show_bins<=0)return;
kk=radar_bins/radar_show_bins;
nn=radar_maxlines/radar_lines;
for(i=0; i<radar_lines; i++)
  {
  xp=rg_old_x1+1;
  for(j=0; j<radar_show_bins-1; j++)
    {
    t1=radar_average[m];
    t2=(radar_average[m+1]-t1)/kk;
    for(k=0; k<kk;k++)
      {
      color=rg_cfac*(log10(t1/radar_redraw_count)-rg_czer);
      t1+=t2;
      if(color<0)color=0;
      if(color >= MAX_COLOR_SCALE)color=MAX_COLOR_SCALE-1;
      for(n=0; n<nn; n++)
        {
        if(yp-n < rgbutt[RG_GAIN].y1)
          {
          lir_setpixel(xp,yp-n,color_scale[color]);
          }
        else
          {
          if(xp < rgbutt[RG_GAIN].x1)
            {
            if(yp-n < rgbutt[RG_ZERO].y1)
              {
              lir_setpixel(xp,yp-n,color_scale[color]);
              }
            else
            if(xp > rgbutt[RG_TIME].x2)
              {
              lir_setpixel(xp,yp-n,color_scale[color]);
              }
            
            }                
          }
        }
      xp++;
      }
    m++;
    }
  m++;  
  yp-=nn;
  }
}

void show_wheel_stepmult(void)
{
int k;
char s[80];
k=bg.wheel_stepn+1;
if(genparm[AFC_ENABLE]!=0 && genparm[AFC_LOCK_RANGE] != 0)k+=30;
if(k<0)
  {
  s[0]='-';
  }
else
  {
  s[0]=' ';
  }  
k=abs(k);
sprintf(&s[1],"%d",k/10);
lir_pixwrite(bg.xright-2*text_width-2,bg.ytop+2+1.5*text_height,s);
sprintf(&s[1],"%d",k%10);
lir_pixwrite(bg.xright-2*text_width-2,bg.ytop+2+2.5*text_height,s);
}

void update_spur_info(void)
{
int i, k, pnt, yb;
char s[80];
yb=wg.ytop+4+text_height;
// Indicate the spur elimination on the wide graph frequency scale
lir_hline(wg_first_xpixel,yb,wg_last_xpixel-2*text_width,7);
lir_hline(wg_first_xpixel,yb-1,wg_last_xpixel-2*text_width,0);
if(kill_all_flag) return;
lir_hline(wg.xleft+1,yb,wg_first_xpixel,7);
if(kill_all_flag) return;
lir_hline(wg_last_xpixel-2*text_width,yb,wg.xright-1,7);
lir_hline(wg_last_xpixel-2*text_width,yb-1,wg.xright-1,0);
if(no_of_spurs == 0)
  {
  lir_fillbox(wg.xright-4*text_width-2,yb+2,4*text_width,text_height,0);
  return;
  }
for(i=0; i<no_of_spurs; i++)
  {
  pnt=spur_location[i]+SPUR_WIDTH/2;
  if(genparm[SECOND_FFT_ENABLE] != 0)
    {
    pnt/=fft2_to_fft1_ratio;
    }
  pnt-=wg.first_xpoint;
  if(wg.xpoints_per_pixel != 1 && wg.pixels_per_xpoint != 1)
    {
    if(wg.xpoints_per_pixel == 0)
      {
      pnt*=wg.pixels_per_xpoint;
      }
    else
      {
      pnt=(pnt+(wg.xpoints_per_pixel>>1))/wg.xpoints_per_pixel;
      }  
    }
  k=pnt+wg_first_xpixel;
  if(k>=wg_first_xpixel && k<=wg_last_xpixel)
    {
    lir_setpixel(k,yb,15);
    lir_setpixel(k,yb-1,15);
    }
  }
sprintf(s,"%d",no_of_spurs);
i=0;
while(s[i] != 0)i++;
lir_pixwrite(wg.xright-i*text_width-2,yb+3,s);
}

void txtest_wrerr(char *s)
{
int ix, iy;
ix=(screen_width>>1)-9*text_width;
iy=wg.ytop+(text_height>>1);
settextcolor(15);
while(iy < wg.ybottom-text_height)
  {
  lir_pixwrite(ix,iy,s);
  iy+=1.5*text_height;
  }
settextcolor(7);
}  

void show_adsat(void)
{
int i;
float t1;
char s[80];
t1=0;
for(i=0; i<ui.rx_ad_channels; i++)
  {
  if(t1<rx_ad_maxamp[i])t1=rx_ad_maxamp[i];
  }
if(t1<1)t1=1;
t1/=0x8000;
t1=-20*log10(t1);
if(t1 < 0.0000001) t1=0.0000001;
sprintf(s,"A/D margin %5.2f dB",t1);
lir_text(10,screen_last_line,s);
if(t1 < 0.1 && txtest_saturated > t1)
  {
  txtest_wrerr(s);
  txtest_saturated=t1;
  }
}

void redraw_meter_graph(void)
{
char s[1000];
char trackinfo[3]={'2','P','M'};
int i, k, ypix, sunit_flag;
double t1, t2, t3;
hide_mouse(mg.xleft,mg.xright,mg.ytop,mg.ybottom);
lir_fillbox(mg.xleft,mg.ytop,mg.xright-mg.xleft+1,mg.ybottom-mg.ytop+1,0);
graph_borders((void*)&mg,7);
settextcolor(7);
lir_mutex_lock(MUTEX_METER_GRAPH);
lir_sleep(20000);
ypix=mg.yzero+0.1*mg.ygain*(mg_dbscale_start+mg_scale_offset);
t1=mg_dbscale_start;
t2=mg_dbstep;
sunit_flag=0;
while(mg_ymin-ypix > mg_ymax)
  {
  if(ypix >=0)
    {
    if(mg.scale_type != MG_SCALE_SUNITS)
      {
      if(t1 > 0)t3=0.00001; else t3=-0.00001;
      sprintf(s,"%f",t1+t3);
      s[6]=0;
      }
    else
      {
      if(t1 > 54.111 && sunit_flag == 0)
        {
        sunit_flag=1;
        i=1+mg_dbstep/5;
        t2=5*i;
        t1=54+t2;
        ypix=mg.yzero+0.1*mg.ygain*(t1+mg_scale_offset);
        while(ypix < 0)
          {
          t1+=t2;
          ypix=mg.yzero+0.1*mg.ygain*(t1+mg_scale_offset);
          }
        }        
      if(t1 > -3)
        {
        if( t1 < 57)
          {
          sprintf(s,"   S%d",(int)((t1+0.00001)/6));
          }
        else            
          {
          sprintf(s,"S9+%d",(int)t1-54);
          }
        }
      else
        {
        s[0]=0;
        }
      }
    k=mg.xright-mg.xleft-3;
    k/=text_width;
    if(k>999)k=999;
    s[k]=0;  
    if(mg_ymin-ypix > mg_ymax)
      {
      if(!mg_bar)lir_hline(mg_first_xpixel,mg_ymin-ypix,mg_last_xpixel, MG_DBSCALE_COLOR);
      if(mg_bar || (ypix > text_height/2 && 
                                   mg_ymin-ypix > mg_ymax+text_height/2) )
        {
        lir_pixwrite(mg.xleft+2,mg_ymin-ypix-text_height/2,s);
        }
      }
    }
  t1+=t2;
  ypix=mg.yzero+0.1*mg.ygain*(t1+mg_scale_offset);
  }
mgw_p=mg_first_xpixel;
if(mg_clear_flag == TRUE)
  {
  k=mg_size;
  if(!sw_onechan)k<<=1;
  for(i=0; i<k; i++)
    {
    mg_rms_meter[i]=0;
    mg_peak_meter[i]=0;
    }
  mg_px=mg_pa;
  mg_valid=0;
  mg_bar_y=mg_y0;
  mg_clear_flag=FALSE;
  }
else
  {
  i=(4*(mg_last_xpixel-mg_first_xpixel))/5;
  if(i>mg_valid) i=mg_valid;
  mg_px=(mg_pa-i+mg_size)&mg_mask;
  }
if(mg_bar)
  {
  update_bar(mg_bar_x1,mg_bar_x2,mg_y0,mg_bar_y,-1,
                                                 MG_BAR_COLOR,mg_barbuf);
  }
else  
  {
  sprintf(s,"%d ",mg.avgnum);
  lir_pixwrite(mgbutt[MG_CHANGE_TYPE].x2+text_width,
                                       mgbutt[MG_CHANGE_TYPE].y1+2,s);
  sprintf(s,"%c",trackinfo[mg.tracks]);
  show_button(&mgbutt[MG_CHANGE_TRACKS],s);
  s[0]=27;
  show_button(&mgbutt[MG_INCREASE_AVGN],s);
  s[0]=26;
  show_button(&mgbutt[MG_DECREASE_AVGN],s);
  s[0]=25;
  show_button(&mgbutt[MG_DECREASE_GAIN],s);
  show_button(&mgbutt[MG_INCREASE_YREF],s);
  s[0]=24;
  show_button(&mgbutt[MG_DECREASE_YREF],s);
  show_button(&mgbutt[MG_INCREASE_GAIN],s);
  if(mg.scale_type != MG_SCALE_DB)
    {
    if(mg.scale_type == MG_SCALE_DBM)
      {
      sprintf(s,"%f",mg.cal_dbm);
      }
    else
      {
      if(mg.scale_type == MG_SCALE_DBHZ)
        {
        sprintf(s,"%f",mg.cal_dbhz);
        }
    else
        {
        if(mg.scale_type == MG_SCALE_STON)
          {
          sprintf(s,"%f",mg.cal_ston_sigshift);
          s[6]=0;
          show_button(&mgbutt[MG_SCALE_STON_SIGSHIFT],s);
          sprintf(s,"%f",mg.cal_ston);
          }
        else
          {
          sprintf(s,"%f",mg.cal_s_units);
          }
        }
      }  
    s[8]=0;
    show_button(&mgbutt[MG_CHANGE_CAL],s);
    }
  switch (mg.scale_type)
    {
    case MG_SCALE_DB:
    strcpy(s,"dB");
    break;
  
    case MG_SCALE_DBM:
    strcpy(s,"dBm");
    break;

    case MG_SCALE_STON:
    strcpy(s,"S/N");
    break;

    case MG_SCALE_DBHZ:
    strcpy(s,"dB/Hz");
    break;

    case MG_SCALE_SUNITS:
    strcpy(s,"S");
    break;
    }
  show_button(&mgbutt[MG_CHANGE_TYPE],s);
  }
lir_mutex_unlock(MUTEX_METER_GRAPH);
}

void show_tx_frequency(void)
{
char s[80];
int i, k;
if(diskread_flag < 2 && netfd.rec_rx==INVSOCK)
  {
  sprintf(s,"         %.12f",tg.freq+0.0000005);
  i=1;
  while(s[i] != '.' && i< 20)i++;
  k=i-FREQ_MHZ_DIGITS;
  s[6+i+1]=0;
  hide_mouse(tg.xleft,tg.xright,tg.ytop,tg.ybottom);
  button_color=7;
  show_button(&tgbutt[TG_NEW_TX_FREQUENCY],&s[k]);
  sprintf(s,"Proc %02d",tg.spproc_no);
  show_button(&tgbutt[TG_CHANGE_TXPAR_FILE_NO],s);
  sprintf(s,"dwn %d   ",mute_on);
  s[7]=0;
  show_button(&tgbutt[TG_MUTE_ON],s);
  sprintf(s,"up %d    ",mute_off);
  s[7]=0;
  show_button(&tgbutt[TG_MUTE_OFF],s);
  sprintf(s,"Att %.1f dB   ",tg.level_db);
  s[12]=0;
  show_button(&tgbutt[TG_SET_SIGNAL_LEVEL],s);
  if(tx_onoff_flag ==0)
    {
    settextcolor(3);
    show_button(&tgbutt[TG_ONOFF],"Off");
    }
  else
    {
    settextcolor(14);
    show_button(&tgbutt[TG_ONOFF],"On");
    }
  settextcolor(7);
  if(rx_mode == MODE_RADAR)
    {
    sprintf(s,"%d",txcw.radar_interval);
    show_button(&tgbutt[TG_RADAR_INTERVAL],s);
    }
  }
}

void show_center_frequency(void)
{
double dt1;
int i,k;
dt1=0;
char s[128];
// Get the center frequency in ascii with too many decimals and
// too many leading blanks.
if(!freq_from_file)
  {
  if( (ui.converter_mode & CONVERTER_USE) != 0)
    {
    if( (ui.converter_mode & CONVERTER_LO_BELOW) != 0)
      {
      if( (ui.converter_mode & CONVERTER_UP) == 0)
        {
        dt1=converter_offset_mhz+fg.passband_center;  
        }
      else
        {
        dt1=fg.passband_center-converter_offset_mhz;
        }
      }
    else
      {
      dt1=converter_offset_mhz-fg.passband_center;  
      }
    }
  else
    {
    dt1=fg.passband_center;
    }
  sprintf(s,"         %.12f",dt1+FREQ_MHZ_ROUNDCORR);
  i=1;
  while(s[i] != '.' && i< 20)i++;
  k=i-FREQ_MHZ_DIGITS;
  s[FREQ_MHZ_DECIMALS+i+1]=0;
  hide_mouse(fg.xleft+2.5*text_width,fg.xright-2.5*text_width,
                                       fg.yborder-text_height-1,fg.ybottom);
  lir_pixwrite(fg.xleft+2.5*text_width,fg.yborder-text_height-1,&s[k]);
  }
wide_fq_scale();
if(hg_flag == 1)
  {
  hires_fq_scale();
  if(kill_all_flag) return;
  }
if(bg_flag == 1)
  {
  baseband_fq_scale();
  if(kill_all_flag) return;
  }
frequency_readout();
}


void frequency_readout(void)
{
char s[80];
int ia,ib,j,k,n;
float t1;
if(rx_mode == MODE_TXTEST || rx_mode == MODE_RADAR)return;
hide_mouse(freq_readout_x1-1,freq_readout_x2+1,
                          freq_readout_y1,freq_readout_y1+text_height+1);
t1=mix1_selfreq[0];
if(t1<0 )
  {
  lir_fillbox(freq_readout_x1-1,freq_readout_y1,
             freq_readout_x2-freq_readout_x1+1,text_height+1,0);
  return;
  }
if((genparm[AFC_ENABLE] != 0 && genparm[AFC_LOCK_RANGE] != 0 ) && 
               ag.mode_control != 0 && mix1_status[0] != 0 && 
                                          mix1_fq_mid[fftx_nx] >= 0)
   {
   t1=mix1_fq_mid[fftx_nx];
   }
if(rx_mode == MODE_SSB)t1+=bg.bfo_freq;  
hwfreq=0.001*(t1)+100*frequency_scale_offset;
update_users_rx_frequency();
if(ui.rx_soundcard_radio == RX_SOUNDCARD_RADIO_ELEKTOR && diskread_flag < 2)
  {
  update_elektor_rx_frequency();
  }
hwfreq+=0.001F*fg_truncation_error;

if(bg_hz_per_pixel < 0.1)
  {
  sprintf(s,"%.6f     ",hwfreq);
  }
else
  {
  sprintf(s,"%.4f     ",hwfreq);
  }
k=0;
while(s[k]!='.')k++;
j=k;
while(s[j]!=0)j++;
ia=freq_readout_x1;
ib=freq_readout_x2;
n=(ib-ia)/text_width;
if(n >= k)
  {
  lir_fillbox(ia-1,freq_readout_y1,ib-ia+2,text_height+1,0);
  if(j > n)j=n;
  s[j]=0;
  ia+=text_width*(n-j)/2;
  lir_pixwrite(ia,freq_readout_y1+2, s);
  }
}

void show_keying_spectrum(void)
{
int i,j,k,n,kk;
float pwrmax,pwrmin,pwr_range;
pwrmax=0;
pwrmin=BIGFLOAT;
n=1+keying_spectrum_size/10;
for(i=n; i<keying_spectrum_size; i++)
  {
  if(keying_spectrum[i] > pwrmax)pwrmax=keying_spectrum[i];
  if(keying_spectrum[i] < pwrmin)pwrmin=keying_spectrum[i];
  }
if(pwrmin > 0)
  {
  if(pwrmax/pwrmin > 100)pwrmin=0.01*pwrmax;
  pwr_range=log(pwrmax/pwrmin);
  kk=1+(bg_first_xpixel+bg_last_xpixel)/2;
  k=bg.pixels_per_point;    
  kk+=k/2;
  j=kk;
  n=(MAX_COLOR_SCALE-1)*log(keying_spectrum[0]/pwrmin)/pwr_range;
  if(n<0)n=0;
  lir_fillbox(kk,bg_ymax,k,3,color_scale[n]);
  for(i=1; i<keying_spectrum_size; i++)
    {
    kk-=k;
    j+=k;
    if(j<bg_last_xpixel)
      {
      n=(MAX_COLOR_SCALE-1)*log(keying_spectrum[i]/pwrmin)/pwr_range;
      if(n<0)n=0;
      lir_fillbox(kk,bg_ymax,k,3,color_scale[n]);
      lir_fillbox(j,bg_ymax,k,3,color_scale[n]);
      }
    }
  }
}

void show_baseband_spectrum(void)
{
int i,j,k,kk,color,pp,bg_xpix1,bg_xpix2;
int color1, color2;
int new_y1, new_y2,old_y1, old_y2, ymax;
float p1,p2,p3,p4,der1,der2;
float pwrfac;
ymax=bg_ymax;
if(genparm[CW_DECODE_ENABLE] != 0)ymax-=3;
pwrfac=bg.yfac_power/fft3_slowsum_cnt;
// Show baseband graph on screen for selected frequency no 0
bg_xpix1=bg_first_xpixel;
pp=bg.pixels_per_point;
bg_xpix2=bg_last_xpixel-pp/2;
i=(bg_xpix1-bg_first_xpixel)/pp;
hide_mouse(bg_xpix1,bg_xpix2,ymax,bg_y0);
if(sw_onechan)
  {  
  p2=bg.yfac_log*log10(fft3_slowsum[i]*pwrfac);
  for(k=bg_xpix1; k<bg_xpix2; k+=pp)
    {
    p1=p2;
    p2=bg.yfac_log*log10(fft3_slowsum[i+1]*pwrfac);
    der1=(p2-p1)/pp;
    for(j=0; j<pp; j++)
      {
      kk=k+j;
      new_y1=p1;
      p1+=der1;
      old_y1=fft3_spectrum[kk];
      new_y1=p1;
      if(new_y1 < 0)new_y1=0;
      new_y1=bg_y0-new_y1;
      if(new_y1 < ymax)new_y1=ymax;
      if( new_y1 != old_y1)
        {
        color=bg_background[old_y1];
        if( (kk == curv_xpixel && old_y1 >= bg_y4 && old_y1 < bg_y3) ||
            (kk == flat_xpixel && old_y1 >= bg_ymax && old_y1 < bg_y4) )
          {
          color=14;
          }
        else 
          {
          if( (kk == bfo_xpixel && old_y1 >= bg_y3 && old_y1 < bg_y2) ||
              (kk == bfo10_xpixel && old_y1 >= bg_y2 && old_y1 < bg_y1) ||
              (kk == bfo100_xpixel && old_y1 >= bg_y1) )
            {
            color=12;
            }
          }
        if( bg_carrfilter_y[kk] == old_y1)color=58;
        if( bg_filterfunc_y[kk] == old_y1)color=14;
        if(kk < bgbutt[BG_SQUELCH_LEVEL].x1)
          {
          lir_setpixel(kk,old_y1,color);
          lir_setpixel(kk,new_y1,15);
          }
        else
          {
          if(old_y1 > bgbutt[BG_SQUELCH_LEVEL].y2)
            {
            lir_setpixel(kk,old_y1,color);
            }
          if(new_y1 > bgbutt[BG_SQUELCH_LEVEL].y2)
            {
            lir_setpixel(kk,new_y1,15);
            }
          }          
        fft3_spectrum[kk]=new_y1;
        }
      }
    i++;
    }
  }
else
  {
  p3=bg.yfac_log*log10(fft3_slowsum[2*i]*pwrfac);
  p4=bg.yfac_log*log10(fft3_slowsum[2*i+1]*pwrfac);
  for(k=bg_xpix1; k<bg_xpix2-1; k+=pp)
    {
    p1=p3;
    p2=p4;
    p3=bg.yfac_log*log10(fft3_slowsum[2*i+2]*pwrfac);
    p4=bg.yfac_log*log10(fft3_slowsum[2*i+3]*pwrfac);
    der1=(p3-p1)/pp;
    der2=(p4-p2)/pp;
    for(j=0; j<pp; j++)
      {
      kk=k+j;
      new_y1=p1;
      new_y2=p2;
      p1+=der1;
      p2+=der2;
      if(new_y1 < 0)new_y1=0;
      new_y1=bg_y0-1-new_y1;
      if(new_y1 < ymax)new_y1=ymax;
      old_y1=fft3_spectrum[2*kk];
      old_y2=fft3_spectrum[2*kk+1];
      if(new_y2 < 0)new_y2=0;
      new_y2=bg_y0-new_y2;
      if(new_y2 < ymax)new_y2=ymax;
      if(new_y1 == new_y2)
        {
        color=bg_background[old_y2];
        if( (kk == curv_xpixel && old_y2 >= bg_y4 && old_y2 < bg_y3) ||
            (kk == flat_xpixel && old_y2 >= bg_ymax && old_y2 < bg_y4) )
          {
          color=14;
          }
        else 
          {
          if( (kk == bfo_xpixel && old_y2 >= bg_y3 && old_y2 < bg_y2) ||
              (kk == bfo10_xpixel && old_y2 >= bg_y2 && old_y2 < bg_y1) ||
              (kk == bfo100_xpixel && old_y2 >= bg_y1) )
            {
            color=12;
            }
          }
        if(bg_carrfilter_y[kk] == old_y2)color=58;
        if(fft1_correlation_flag <= 1 && 
                               bg_filterfunc_y[kk] == old_y2)color=14;
        color1=bg_background[old_y1];
        if( (kk == curv_xpixel && old_y1 >= bg_y4 && old_y1 < bg_y3) ||
            (kk == flat_xpixel && old_y1 >= bg_ymax && old_y1 < bg_y4) )
          {
          if(fft1_correlation_flag <= 1)color1=14;
          }
        else 
          {
          if( (kk == bfo_xpixel && old_y1 >= bg_y3 && old_y1 < bg_y2) ||
              (kk == bfo10_xpixel && old_y1 >= bg_y2 && old_y1 < bg_y1) ||
              (kk == bfo100_xpixel && old_y1 >= bg_y1) )
            {
            color1=12;
            }
          }
        if(bg_carrfilter_y[kk] == old_y1)color1=58;
        if(fft1_correlation_flag <= 1 &&
                                bg_filterfunc_y[kk] == old_y1)color1=14;
        if(kk < bgbutt[BG_SQUELCH_LEVEL].x1)
          {
          lir_setpixel(kk,old_y2,color);
          lir_setpixel(kk,old_y1,color1);
          lir_setpixel(kk,new_y2,15);
          }
        else
          {
          if(old_y2 > bgbutt[BG_SQUELCH_LEVEL].y2)
            {
            lir_setpixel(kk,old_y2,color);
            }
          if(old_y1 > bgbutt[BG_SQUELCH_LEVEL].y2)
            {
            lir_setpixel(kk,old_y1,color1);
            }
          if(new_y2 > bgbutt[BG_SQUELCH_LEVEL].y2)
            {
            lir_setpixel(kk,new_y2,15);
            }
          }          
        fft3_spectrum[2*kk+1]=new_y2;
        fft3_spectrum[2*kk]=new_y2;
        }
      else
        {  
        if( new_y2 != old_y2)
          {
          color2=bg_background[old_y2];
          if( (kk == curv_xpixel && old_y2 >= bg_y4 && old_y2 < bg_y3) ||
              (kk == flat_xpixel && old_y2 >= bg_ymax && old_y2 < bg_y4) )
            {
            if(fft1_correlation_flag <= 1)color2=14;
            }
          else 
            {  
            if( (kk == bfo_xpixel && old_y2 >= bg_y3 && old_y2 < bg_y2) ||
                (kk == bfo10_xpixel && old_y2 >= bg_y2 && old_y2 < bg_y1) ||
                (kk == bfo100_xpixel && old_y2 >= bg_y1) )
              {
              color2=12;
              }
            }
          if(bg_carrfilter_y[kk] == old_y2)color2=58;
          if(bg_filterfunc_y[kk] == old_y2 && 
                                 fft1_correlation_flag <= 1)color2=14;
          if(kk < bgbutt[BG_SQUELCH_LEVEL].x1)
            {
            lir_setpixel(kk,old_y2,color2);
            lir_setpixel(kk,new_y2,13);
            }
          else
            {
            if(old_y2 > bgbutt[BG_SQUELCH_LEVEL].y2)
              {
              lir_setpixel(kk,old_y2,color2);
              }
            if(new_y2 > bgbutt[BG_SQUELCH_LEVEL].y2)
              {
              lir_setpixel(kk,new_y2,13);
              }
            }
          fft3_spectrum[2*kk+1]=new_y2;
          }
        if( new_y1 != old_y1)
          {
          color1=bg_background[old_y1];
          if( (kk == curv_xpixel && old_y1 >= bg_y4 && old_y1 < bg_y3) ||
              (kk == flat_xpixel && old_y1 >= bg_ymax && old_y1 < bg_y4) )
            {
            if(fft1_correlation_flag <= 1)color1=14;
            }
          else 
            {  
            if( (kk == bfo_xpixel && old_y1 >= bg_y3 && old_y1 < bg_y2) ||
                (kk == bfo10_xpixel && old_y1 >= bg_y2 && old_y1 < bg_y1) ||
                (kk == bfo100_xpixel && old_y1 >= bg_y1) )
              {
              color1=12;
              }
            }
          if(bg_carrfilter_y[kk] == old_y1)color1=58;
          if(bg_filterfunc_y[kk] == old_y1 &&
                               fft1_correlation_flag <= 1)color1=14;
          if(kk < bgbutt[BG_SQUELCH_LEVEL].x1)
            {
            lir_setpixel(kk,old_y1,color1);
            lir_setpixel(kk,new_y1,10);
            }
          else
            {
            if(old_y1 > bgbutt[BG_SQUELCH_LEVEL].y2)
              {
              lir_setpixel(kk,old_y1,color1);
              }
            if(new_y1 > bgbutt[BG_SQUELCH_LEVEL].y2)
              {
              lir_setpixel(kk,new_y1,10);
              }
            }
          fft3_spectrum[2*kk]=new_y1;
          }
        }
      }
    i++;
    }
  }
}


void show_blanker_info(void)
{
char s[80];
char color;
int i,j,xnew,xold,chan,qeflag,cleflag[2],stuflag[2];
stuflag[1]=0;
cleflag[1]=0;
hide_mouse(timf2_hg_xmin, hg.xright,hg.ybottom-text_height,hg.ybottom);
qeflag=0;
for(chan=0; chan<ui.rx_rf_channels; chan++)
  {
  cleflag[chan]=0;
  stuflag[chan]=0;
  xnew=log(timf2_hg_xlog*timf2_despiked_pwr[chan])*timf2_hg_xfac;
  if(xnew<0)xnew=0;
  if(xnew+timf2_hg_xmin >= timf2_hg_xmax)xnew=timf2_hg_xmax-timf2_hg_xmin-1;
  xold=timf2_hg_x[chan];
  timf2_hg_x[chan]=xnew;
  if(xold != xnew)
    {
    if(xnew > xold)
      {
      lir_fillbox(timf2_hg_xmin+xold,timf2_hg_y[chan],
                               xnew-xold+1,timf2_hg_yh,chan_color[chan]);
      if(timf2_hg_qex >=xold && timf2_hg_qex <=xnew)qeflag=1; 
      if(timf2_hg_clex>=xold && timf2_hg_clex<=xnew)cleflag[chan]=1; 
      if(timf2_hg_stux>=xold && timf2_hg_stux<=xnew)stuflag[chan]=1; 
      }
    else
      {
      lir_fillbox(timf2_hg_xmin+xnew,timf2_hg_y[chan],
                                        xold-xnew+1,timf2_hg_yh,8);
      if(timf2_hg_qex >=xnew && timf2_hg_qex <=xold)qeflag=1; 
      if(timf2_hg_clex>=xnew && timf2_hg_clex<=xold)cleflag[chan]=1; 
      if(timf2_hg_stux>=xnew && timf2_hg_stux<=xold)stuflag[chan]=1; 
      }
    }
  }
if(qeflag != 0)
  {
  lir_line(timf2_hg_xmin+timf2_hg_qex,timf2_hg_y[0],
               timf2_hg_xmin+timf2_hg_qex,timf2_hg_y[0]+text_height-1,12);
  if(kill_all_flag) return;
  }
if(hg.clever_bln_mode!=0)
  {
  j=rint(clever_blanker_rate);
  sprintf(s,"%02d",(int)j);
  if(s[0]=='0')s[0]=' ';
  settextcolor(11);
  lir_pixwrite(hg.xright-2-2*text_width,hg_ymax,s);
  xold=timf2_hg_clex;
  xnew=log(timf2_hg_xlog*(hg.clever_bln_limit>>1))*timf2_hg_xfac;
  if(xnew<0)xnew=0;
  timf2_hg_clex=xnew;
  if(xold == xnew)
    {
    if(cleflag[0]+cleflag[1] == 0)goto clever_ok;
    }
  else  
    {
    for(chan=0; chan<ui.rx_rf_channels; chan++)
      {
      if(cleflag[chan] == 0)
        {
        if(xold <= timf2_hg_x[chan])color=chan_color[chan]; else color=8;
        lir_line(timf2_hg_xmin+xold,timf2_hg_y[chan],
                timf2_hg_xmin+xold,timf2_hg_y[chan]+timf2_hg_yh-1,color);
        if(kill_all_flag) return;
        }
      }  
    }
  lir_line(timf2_hg_xmin+xnew,timf2_hg_y[0],
                timf2_hg_xmin+xnew,timf2_hg_y[0]+text_height-1,11);
  if(kill_all_flag) return;
clever_ok:;
  }
if(hg.stupid_bln_mode!=0)
  {  
  j=rint(stupid_blanker_rate);
  sprintf(s,"%02d",(int)j);
  if(s[0]=='0')s[0]=' ';
  settextcolor(14);
  lir_pixwrite(hg.xleft+2,hg_ymax,s);
  xold=timf2_hg_stux;
  xnew=log(timf2_hg_xlog*(hg.stupid_bln_limit>>1))*timf2_hg_xfac;
  if(xnew<0)xnew=0;
  if(xnew==timf2_hg_clex)xnew++;
  timf2_hg_stux=xnew;
  if(xold == xnew)
    {
    if(stuflag[0]+stuflag[1] == 0)goto stupid_ok;
    }
  else  
    {
    for(chan=0; chan<ui.rx_rf_channels; chan++)
      {
      if(stuflag[chan] == 0)
        {
        if(xold < timf2_hg_x[chan])color=chan_color[chan]; else color=8;
        lir_line(timf2_hg_xmin+xold,timf2_hg_y[chan],
                timf2_hg_xmin+xold,timf2_hg_y[chan]+timf2_hg_yh-1,color);
        if(kill_all_flag) return;
        }
      }  
    }
  lir_line(timf2_hg_xmin+xnew,timf2_hg_y[0],
                  timf2_hg_xmin+xnew,timf2_hg_y[0]+text_height-1,14);
  if(kill_all_flag) return;
stupid_ok:;
  }
settextcolor(7);  
if(timf2_ovfl != 0)
  {
  sprintf(s,"OVFL %d",timf2_ovfl);
  i=6;
  while(s[i] != 0)i++;
  lir_pixwrite(timf2_hg_xmax-text_width*i,hg.ybottom-text_height,s);
  }
}



void show_ag_buttons(void)
{
char s[80];
if(agbutt[AG_MANAUTO].x2 > screen_last_xpixel)return;
s[0]=modes_man_auto[ag.mode_control];
s[1]=0;
show_button(&agbutt[AG_MANAUTO],s);
if(kill_all_flag) return;
if(agbutt[AG_WINTOGGLE].x2 > screen_last_xpixel)return;
if(ag.window == 1) s[0]='W'; else s[0]='-';
show_button(&agbutt[AG_WINTOGGLE],s);
if(kill_all_flag) return;
if(ag.mode_control == 2)
  {
  make_afct_window(ag.avgnum);
  }
if(agbutt[AG_SEL_AVGNUM].x2 > screen_last_xpixel)return;
sprintf(s,"Avg%3d",afct_avgnum);
show_button(&agbutt[AG_SEL_AVGNUM],s);
if(kill_all_flag) return;
if(agbutt[AG_SEL_FIT].x2 >  screen_last_xpixel)return;
sprintf(s,"Fit%3d",ag.fit_points);
show_button(&agbutt[AG_SEL_FIT],s);
if(kill_all_flag) return;
if(agbutt[AG_SEL_DELAY].x2 >=  screen_last_xpixel)return;
sprintf(s,"Del%3d",ag.delay);
show_button(&agbutt[AG_SEL_DELAY],s);
if(kill_all_flag) return;
update_bar(ag_ston_x1,ag_ston_x2,ag_fpar_y0,ag_ston_y,-1,
                                             AG_STON_RANGE_COLOR,ag_stonbuf);
update_bar(ag_lock_x1,ag_lock_x2,ag_fpar_y0,ag_lock_y,-1,
                                             AG_LOCK_RANGE_COLOR,ag_lockbuf);
update_bar(ag_srch_x1,ag_srch_x2,ag_fpar_y0,ag_srch_y,-1,
                                              AG_SRC_RANGE_COLOR,ag_srchbuf);
}


void fill_afc_graph(void)
{
char s[80];
int i, j, k, m;
float t1, cfq, fq_value, fq_y;
double fq_scalestep;
double afc_hwfreq,fq_offset;
// Clear the graph.
i=agbutt[AG_FQSCALE_EXPAND].y2+1;
hide_mouse(ag.xleft,ag.xright,i,ag.ybottom);
lir_fillbox(ag.xleft+1 ,i,ag.xright-ag.xleft-2,ag.ybottom-i-1,0);
j=ag.xleft+9*text_width/2;
k=ag.xright-j-1;
if(k > 0)lir_fillbox(j,ag.ytop+1,k,3*text_height/2,0);
// Get center frequency and put on screen
cfq=mix1_fq_mid[fftx_nx];
if(cfq < 0)goto skipsig;
afc_hwfreq=0.001*mix1_fq_mid[fftx_nx]+100*frequency_scale_offset;
fq_offset=100000*frequency_scale_offset;
sprintf(s,"%.4f kHz",afc_hwfreq);
k=0;
while(s[k]!=0)k++;
while(k*text_width+j>=ag.xright)k--;
if(k > 0)
  {
  s[k]=0;
  lir_pixwrite(j,ag.ytop+2, s);
  }
// Make a frequency scale, showing decimals of the selected frequency.
ag_first_fq=cfq-ag.frange/2;
ag_hz_per_pixel=ag.frange/(ag_y0-ag_y1);
fq_scalestep=ag_hz_per_pixel*(text_height+2);
j=adjust_scale(&fq_scalestep);
i=0.5+ag_first_fq/fq_scalestep;
fq_value=i*fq_scalestep;
fq_y=ag_y0+(ag_first_fq-fq_value)/ag_hz_per_pixel;
// Find out how many digits to show. 
t1=ag.frange;
k=0;
while(t1>0.003)
  {
  t1*=0.1;
  k++;
  }
for(i=0; i<screen_height; i++)ag_background[i]=0;
while( fq_y > ag_y1+text_height/2)
  {
  if(fq_y< ag_y0-text_height/2)
    {
    sprintf(s,"%f",fq_value+fq_offset);
    j=0;
    while(s[j] != '.' && s[j] != 0)j++;
    j-=k;
    if(j<0)j=0;
    s[j+AG_DIGITS]=0;
    m=fq_y;
    lir_pixwrite(ag.xleft+2,m-text_height/2+2, &s[j]);
    lir_hline(ag_first_xpixel,m,ag.xright-1,8);
    if(kill_all_flag) return;
    ag_background[m]=8;
    }
  fq_y-=fq_scalestep/ag_hz_per_pixel;
  fq_value+=fq_scalestep;
  }
k=(ag_pa-ag_px+ag_size)&ag_mask;
if(k >= ag_xpixels)
  {
  ag_px=ag_pa-0.9*ag_xpixels;
  ag_px=(ag_px+ag_size)&ag_mask;
  }
// Clear the points in oldpix we will not show now
k=ag_pa;
if(ag.xright-ag_first_xpixel <= k)k=ag.xright-ag_first_xpixel-1;
for(i=k*AG_CURVNUM; i<AG_CURVNUM*(ag_xpixels+1); i++)ag_oldpix[i]=-1;
ag_px=(ag_ps-afct_half_avgnum-1+ag_size)&ag_mask;
local_ag_ps=ag_ps;
skipsig:;
// Show dB scale for S/N
t1=1;
j=0;
make_dbscale:;
m=ag_y0-log10(t1)*STON_YFAC;
settextcolor(AG_DBSCALE_COLOR);
if(m > ag.ytop+text_height && j<= 30)
  {
  sprintf(s,"%2d",j);
  lir_pixwrite(ag.xright-2*text_width-2,m-text_height/2+2, s);
  lir_hline(ag_first_xpixel,m,ag.xright-2*text_width-4,AG_DBSCALE_COLOR);
  if(kill_all_flag) return;
  ag_background[m]=AG_DBSCALE_COLOR;
  t1*=sqrt(10.);
  j+=5;
  goto make_dbscale;
  }
settextcolor(7);
show_ag_buttons();  
}


void show_afc(void)
{
int yshift;
int ia, ib, ic, k;
int j, kk, m, pix;
int ka, kb, kc, kd;
frequency_readout();
if(kill_all_flag) return;
hide_mouse(ag.xleft,ag.xright,ag.ytop,ag.ybottom);
ia=local_ag_ps;
k=(ia-ag_px+ag_size)&ag_mask;
ib=(ag_pa-afct_half_avgnum+ag_size)&ag_mask;
ic=ag_pa;
if( ((ib-ag_px+ag_size)&ag_mask) > ag_xpixels-2*text_width ||
    ((ib-ia+ag_size)&ag_mask)+k  > ag_xpixels-2*text_width) 
  {
// There is not room for more data in the graph.  
// Shift the old data so 10% of the graph will be left unused for
// future calls.
  ag_px=(ib-ag_shift_xpixels+ag_size)&ag_mask;
  ia=ag_px;
  k=0;
  ic=(ag_px+ag_xpixels)&ag_mask;
  }
local_ag_ps=ag_ps;
yshift=1;
while(ia != ib)
  {
  kk=k*AG_CURVNUM;
  pix=k+ag_first_xpixel;
// First clear all the curves
  for(j=0; j<AG_CURVNUM; j++)
    {
    if(ag_oldpix[kk+j]>0)
      {
      m=ag_oldpix[kk+j];
      if(m<ag_y0 && m>ag_y1)
        {
        lir_setpixel(pix,m,ag_background[m]);
        }
      }
    }
// Set a red pixel for the fitted line.
// It may become overwritten by some other colour.
  kc=ag_y0-(ag_fitted_fq[ia]-ag_first_fq)/ag_hz_per_pixel;
  if(kc<ag_y0 && kc>ag_y1)
    {
    lir_setpixel(pix,kc,12);
    ag_oldpix[kk+2]=kc;
    }
  else
    {
    ag_oldpix[kk+2]=-1;
    }  
// Set a yellow pixel for the S/N ratio.
  kd=ag_y0-(ag_ston[ia])*STON_YFAC;
  if(kd<ag_y0 && kd>ag_y1)
    {
    lir_setpixel(pix,kd,14);
    ag_oldpix[kk+3]=kd;
    }
  else
    {
    ag_oldpix[kk+3]=-1;
    }
// Set a white pixel for the fitted frequency
  ka=ag_y0-(ag_mix1freq[ia]-ag_first_fq)/ag_hz_per_pixel;
  if(ka<ag_y0 && ka>ag_y1)
    {
    lir_setpixel(pix,ka,15);
    ag_oldpix[kk]=ka;
    }
  else
    {
    ag_oldpix[kk]=-1;
    }  
// Put a green dot for the average frequency to which we do
// the line-fitting.
// Shift by 1 pixel in case we would put the pixel on top
// of a white one.    
  kb=ag_y0-(ag_freq[ia]-ag_first_fq)/ag_hz_per_pixel;
  if(kb == ka)
    {
    kb+=yshift;
    yshift*=-1;
    }
  if(kb<ag_y0 && kb>ag_y1)
    {
    lir_setpixel(pix,kb,10);
    ag_oldpix[kk+1]=kb;
    }
  else
    {
    ag_oldpix[kk+1]=-1;
    }
  ia=(ia+1)&ag_mask;
  k++;
  }
while(ia != ic)
  {
  kk=k*AG_CURVNUM;
  pix=k+ag_first_xpixel;
  for(j=0; j<AG_CURVNUM; j++)
    {
    if(ag_oldpix[kk+j]>0)
      {
      m=ag_oldpix[kk+j];
      if(m<ag_y0 && m>ag_y1)
        {
        lir_setpixel(pix,m,ag_background[m]);
        }
      }
    }
  ia=(ia+1)&ag_mask;
  k++;
  }
}

void show_timf2(void)
{
#define TIMF2_YSCALE 0.3      
float stg, wkg;
int ia, ib, ka, kb;
int mm, m, p0;
hide_mouse(0,screen_width>>1,0,screen_height);
wkg=hg.timf2_oscilloscope_wk_gain;
stg=hg.timf2_oscilloscope_st_gain;
// Show timf2 on the left half of the screen (using screen_width/2 pixels)
// Place the strongest peak at 25% of the x-axis.
mm=4*ui.rx_rf_channels;
ia=0;
ib=1;
m=mm+1;
ka=0;
kb=m;
if(  (hg.timf2_oscilloscope_hold&1) == 0)
  {
  if(  (hg.timf2_oscilloscope_lines&1) != 0)
    {
    while( ib<screen_width/2)
      {
      lir_line(ia,timf2pix[ka   ],ib,timf2pix[kb   ],0);
      if(kill_all_flag) return;
      lir_line(ia,timf2pix[ka+1 ],ib,timf2pix[kb+1  ],0);
      if(kill_all_flag) return;
      lir_line(ia,timf2pix[ka+2 ],ib,timf2pix[kb+2  ],0);
      if(kill_all_flag) return;
      lir_line(ia,timf2pix[ka+3 ],ib,timf2pix[kb+3  ],0);
      if(kill_all_flag) return;
      lir_line(ia,timf2pix[ka+mm],ib,timf2pix[kb+mm ],0);
      if(kill_all_flag) return;
      if(ui.rx_rf_channels==2)
        {
        lir_line(ia,timf2pix[ka+4],ib,timf2pix[kb+4],0);
        if(kill_all_flag) return;
        lir_line(ia,timf2pix[ka+5],ib,timf2pix[kb+5],0);
        if(kill_all_flag) return;
        lir_line(ia,timf2pix[ka+6],ib,timf2pix[kb+6],0);
        if(kill_all_flag) return;
        lir_line(ia,timf2pix[ka+7],ib,timf2pix[kb+7],0);
        if(kill_all_flag) return;
        }
      ia=ib;
      ib++;
      ka=kb;
      kb+=m;
      }
    }
  else
    {
    for(ia=0; ia<screen_width/2; ia++)
      {
      lir_setpixel(ia,timf2pix[ka   ],0);
      lir_setpixel(ia,timf2pix[ka+1 ],0);
      lir_setpixel(ia,timf2pix[ka+2 ],0);
      lir_setpixel(ia,timf2pix[ka+3 ],0);
      lir_setpixel(ia,timf2pix[ka+mm],0);
      if(ui.rx_rf_channels==2)
        {
        lir_setpixel(ia,timf2pix[ka+4],0);
        lir_setpixel(ia,timf2pix[ka+5],0);
        lir_setpixel(ia,timf2pix[ka+6],0);
        lir_setpixel(ia,timf2pix[ka+7],0);
        }
      ka+=m;
      }
    }      
  }      
ia=0;
ka=0;
kb=m*screen_width/2;
p0=(mm*(timf2_show_pointer-screen_width/8)+timf2_mask+1)&timf2_mask;
while( ka<kb)
  {
  if(ui.rx_rf_channels==1)
    {
    if(swfloat)
      {
      timf2pix[ka  ]=timf2_y0[3]-timf2_float[p0  ]*wkg;
      timf2pix[ka+1]=timf2_y0[2]-timf2_float[p0+1]*wkg;
      timf2pix[ka+2]=timf2_y0[1]-timf2_float[p0+2]*stg;
      timf2pix[ka+3]=timf2_y0[0]-timf2_float[p0+3]*stg;
      timf2pix[ka+4]=timf2_y0[4]-sqrt(timf2_pwr_float[p0>>2])*wkg;
      }
    else
      {
      timf2pix[ka  ]=timf2_y0[3]-timf2_shi[p0  ]*wkg;
      timf2pix[ka+1]=timf2_y0[2]-timf2_shi[p0+1]*wkg;
      timf2pix[ka+2]=timf2_y0[1]-timf2_shi[p0+2]*stg;
      timf2pix[ka+3]=timf2_y0[0]-timf2_shi[p0+3]*stg;
      timf2pix[ka+4]=timf2_y0[4]-sqrt((float)(timf2_pwr_int[p0>>2]))*wkg;
      }
    if(timf2pix[ka]<timf2_ymin[3])timf2pix[ka]=timf2_ymin[3];
    if(timf2pix[ka]>timf2_ymax[3])timf2pix[ka]=timf2_ymax[3];
    if(timf2pix[ka+1]<timf2_ymin[2])timf2pix[ka+1]=timf2_ymin[2];
    if(timf2pix[ka+1]>timf2_ymax[2])timf2pix[ka+1]=timf2_ymax[2];
    if(timf2pix[ka+2]<timf2_ymin[1])timf2pix[ka+2]=timf2_ymin[1];
    if(timf2pix[ka+2]>timf2_ymax[1])timf2pix[ka+2]=timf2_ymax[1];
    if(timf2pix[ka+3]<timf2_ymin[0])timf2pix[ka+3]=timf2_ymin[0];
    if(timf2pix[ka+3]>timf2_ymax[0])timf2pix[ka+3]=timf2_ymax[0];
    }
  else
    {  
    if(swfloat)
      {
      timf2pix[ka  ]=timf2_y0[7]-timf2_float[p0  ]*wkg;
      timf2pix[ka+1]=timf2_y0[6]-timf2_float[p0+1]*wkg;
      timf2pix[ka+2]=timf2_y0[5]-timf2_float[p0+2]*wkg;
      timf2pix[ka+3]=timf2_y0[4]-timf2_float[p0+3]*wkg;
      timf2pix[ka+4]=timf2_y0[3]-timf2_float[p0+4]*stg;
      timf2pix[ka+5]=timf2_y0[2]-timf2_float[p0+5]*stg;
      timf2pix[ka+6]=timf2_y0[1]-timf2_float[p0+6]*stg;
      timf2pix[ka+7]=timf2_y0[0]-timf2_float[p0+7]*stg;
      timf2pix[ka+8]=timf2_y0[8]-sqrt(timf2_pwr_float[p0>>3])*wkg;
      }
    else
      {
      timf2pix[ka  ]=timf2_y0[7]-timf2_shi[p0  ]*wkg;
      timf2pix[ka+1]=timf2_y0[6]-timf2_shi[p0+1]*wkg;
      timf2pix[ka+2]=timf2_y0[5]-timf2_shi[p0+2]*wkg;
      timf2pix[ka+3]=timf2_y0[4]-timf2_shi[p0+3]*wkg;
      timf2pix[ka+4]=timf2_y0[3]-timf2_shi[p0+4]*stg;
      timf2pix[ka+5]=timf2_y0[2]-timf2_shi[p0+5]*stg;
      timf2pix[ka+6]=timf2_y0[1]-timf2_shi[p0+6]*stg;
      timf2pix[ka+7]=timf2_y0[0]-timf2_shi[p0+7]*stg;
      timf2pix[ka+8]=timf2_y0[8]-sqrt((float)(timf2_pwr_int[p0>>3]))*wkg;
      }
    if(timf2pix[ka]<timf2_ymin[7])timf2pix[ka]=timf2_ymin[7];
    if(timf2pix[ka]>timf2_ymax[7])timf2pix[ka]=timf2_ymax[7];
    if(timf2pix[ka+1]<timf2_ymin[6])timf2pix[ka+1]=timf2_ymin[6];
    if(timf2pix[ka+1]>timf2_ymax[6])timf2pix[ka+1]=timf2_ymax[6];
    if(timf2pix[ka+2]<timf2_ymin[5])timf2pix[ka+2]=timf2_ymin[5];
    if(timf2pix[ka+2]>timf2_ymax[5])timf2pix[ka+2]=timf2_ymax[5];
    if(timf2pix[ka+3]<timf2_ymin[4])timf2pix[ka+3]=timf2_ymin[4];
    if(timf2pix[ka+3]>timf2_ymax[4])timf2pix[ka+3]=timf2_ymax[4];
    if(timf2pix[ka+4]<timf2_ymin[3])timf2pix[ka+4]=timf2_ymin[3];
    if(timf2pix[ka+4]>timf2_ymax[3])timf2pix[ka+4]=timf2_ymax[3];
    if(timf2pix[ka+5]<timf2_ymin[2])timf2pix[ka+5]=timf2_ymin[2];
    if(timf2pix[ka+5]>timf2_ymax[2])timf2pix[ka+5]=timf2_ymax[2];
    if(timf2pix[ka+6]<timf2_ymin[1])timf2pix[ka+6]=timf2_ymin[1];
    if(timf2pix[ka+6]>timf2_ymax[1])timf2pix[ka+6]=timf2_ymax[1];
    if(timf2pix[ka+7]<timf2_ymin[0])timf2pix[ka+7]=timf2_ymin[0];
    if(timf2pix[ka+7]>timf2_ymax[0])timf2pix[ka+7]=timf2_ymax[0];
    }
  if(timf2pix[ka+mm]<timf2_ymin[mm])timf2pix[ka+mm]=timf2_ymin[mm];
  if(timf2pix[ka+mm]>timf2_ymax[mm])timf2pix[ka+mm]=timf2_ymax[mm];
  p0=(p0+mm)&timf2_mask;  
  ka+=m;
  }      
ia=0;
ib=1;
m=mm+1;
ka=0;
kb=m;
if(  (hg.timf2_oscilloscope_lines&1) != 0)
  {
  while( ib<screen_width/2)
    {
    lir_line(ia,timf2pix[ka  ],ib,timf2pix[kb  ],14);
    if(kill_all_flag) return;
    lir_line(ia,timf2pix[ka+1],ib,timf2pix[kb+1],13);
    if(kill_all_flag) return;
    lir_line(ia,timf2pix[ka+2],ib,timf2pix[kb+2],12);
    if(kill_all_flag) return;
    lir_line(ia,timf2pix[ka+3],ib,timf2pix[kb+3],11);
    if(kill_all_flag) return;
    lir_line(ia,timf2pix[ka+mm],ib,timf2pix[kb+mm],15);
    if(kill_all_flag) return;
    if(ui.rx_rf_channels==2)
      {
      lir_line(ia,timf2pix[ka+4],ib,timf2pix[kb+4],14);
      if(kill_all_flag) return;
      lir_line(ia,timf2pix[ka+5],ib,timf2pix[kb+5],13);
      if(kill_all_flag) return;
      lir_line(ia,timf2pix[ka+6],ib,timf2pix[kb+6],12);
      if(kill_all_flag) return;
      lir_line(ia,timf2pix[ka+7],ib,timf2pix[kb+7],11);
      if(kill_all_flag) return;
      }
    ia=ib;
    ib++;
    ka=kb;
    kb+=m;
    }      
  }
else
  {
  for(ia=0; ia<screen_width/2; ia++)
    {
    lir_setpixel(ia,timf2pix[ka  ],14);
    lir_setpixel(ia,timf2pix[ka+1],13);
    lir_setpixel(ia,timf2pix[ka+2],12);
    lir_setpixel(ia,timf2pix[ka+3],11);
    lir_setpixel(ia,timf2pix[ka+mm],15);
    if(ui.rx_rf_channels==2)
      {
      lir_setpixel(ia,timf2pix[ka+4],14);
      lir_setpixel(ia,timf2pix[ka+5],13);
      lir_setpixel(ia,timf2pix[ka+6],12);
      lir_setpixel(ia,timf2pix[ka+7],11);
      }
    ka+=m;
    }      
  }
if(swfloat)return;
if(timf2_oscilloscope_maxval_uint < 65530)return;
timf2_ovfl++;
}

void show_sg_buttons(void)
{
char s[80], ss[8];
int i,k;
settextcolor(7);
sprintf(s,"%c",sg_modes[sg.mode]);
show_button(&sgbutt[SG_NEW_MODE],s);
sprintf(s,"%s",sg_xscales[sg.xscale]);
show_button(&sgbutt[SG_NEW_XSCALE],s);
sprintf(s,"N %2d",sg.fft_n);
show_button(&sgbutt[SG_NEW_FFT_N],s);
sprintf(ss,"%2.1f",sg.ygain);
i=0;
while(ss[i] != '.' && ss[i] != 0)i++;
if(ss[i] == '.' && ss[i+1] == '0')ss[i]=0;
i=strlen(ss);
sprintf(s,"dB      ");
k=6;
k+=i-2;
while(i>=0)
  {
  s[k]=ss[i];
  i--;
  k--;
  }
show_button(&sgbutt[SG_NEW_YGAIN],s);
sprintf(s,"C %2d",sg.ymax);
show_button(&sgbutt[SG_NEW_YMAX],s);
sprintf(s,"Avg %4d",sg.avg);
show_button(&sgbutt[SG_NEW_AVGNUM],s);
if(sg.xgain >= 1)
  {
  sprintf(s,"Xgain %5.0f",sg.xgain);
  }
else
  {  
  sprintf(s,"Xgain %5.3f",sg.xgain);
  }
show_button(&sgbutt[SG_NEW_XGAIN],s);
}

void sg_line(int x1, int iy1, int x2, int y2, char c)
{
int y;
if(sg.xscale == 0)
  {
  y=sg.ytop;
  }
else
  {
  y=sg_ytop2;
  }
//if(sg.mode != 3)
  {    
  if(x2 > sg.xright-21*text_width)
    {
    if(iy1 < y+11*text_height || y2 < y+11*text_height)return;
    if(iy1 < y+11*text_height)iy1=y+11*text_height;
    if(y2 < y+11*text_height)y2=y+11*text_height;
    }
  }  
lir_line(x1,iy1,x2,y2,c);
}  

void  sg_setpixel(int ix,int iy, char c)
{
int y;
if(sg.xscale == 0)
  {
  y=sg.ytop;
  }
else
  {
  y=sg_ytop2;
  }  
if(ix > sg.xright-22*text_width)
  {
  if(iy < y+12*text_height)return;
  }
lir_setcross(ix, iy, c);
}  

int sg_logpix_from_point(int point)
  {
  return sg_first_xpixel+
    (int)(0.5+(log10(point*baseband_sampling_speed/(sg_siz*sg_log_zerfreq))-
                log10(sg_first_logfreq/sg_log_zerfreq))/sg_log_scale);
  }

int sg_logpix_from_freq(float f)
  {
  return sg_first_xpixel+
      (int)(0.5+(log10(f/sg_log_zerfreq)-
             log10(sg_first_logfreq/sg_log_zerfreq))/sg_log_scale);
  }

void fill_sg_graph(void)
{
char s[80];
int i, j, k, xa;
float x1, x2, y, t1, t2;
short int ypix;
double ydb;
settextcolor(7);
// place sg.ymax dB at the top of the dB scale 
ypix=sg_ytop2+text_height;
ydb=-sg.ymax;
// sg.ygain is vertical scale in dB/pixel.
while(ypix < sg_y0)
  {
  if(sg.ygain < 1)
    {
    sprintf(s,"%.1f",ydb-0.00001F);
    }
  else
    {
    sprintf(s,"%d",(int)(ydb-0.00001F));
    }
  i=strlen(s);
  xa=sg_last_xpixel;
  if(ypix < sg_ytop2+11*text_height)xa-=21*text_width;
  lir_hline(sg_first_xpixel,ypix,xa, SG_DBSCALE_COLOR);
  lir_pixwrite(sg.xleft+2+(6-i)*text_width,ypix-text_height/2,s);
  ypix+=2*text_height;
  ydb-=sg.ygain;
  }
y=sg_y0-1;
x1=sg.xleft+text_width;
x2=sg_last_xpixel;
if(sg.xscale == 0)
  {
  fq_scale(x1, x2, y, sg_first_xpixel, -1., 
           sg_points_per_pixel*sg_hz_per_pixel/sg_pixels_per_point);
  }
else
  {
// Logarithmic scale
// The lowest frequency we want to display is bin #8 in the sg transform
// or 3/4 of the total width of the carrier filter whichever largest.
  sg_first_logfreq=(float)bg_carrfilter_points/(2*fft3_size);
  t1=8.0/sg_siz;
  if(t1 > sg_first_logfreq)sg_first_logfreq=t1;
  sg_first_logpoint=0.5+sg_first_logfreq*sg_siz;;
  sg_first_logfreq*=baseband_sampling_speed;
  sg_last_logfreq=0.375*baseband_sampling_speed;;
  sg_last_logpoint=3*sg_siz/8;
  t1=.001*sg_first_logfreq;
  i=3;
  j=0;
  while(j == 0)
    {
    j=t1;
    t1*=10;
    i--;
    }
  i++;
  sg_log_zerfreq=0.1*t1;
  sg_log_scale=log10(sg_last_logfreq/sg_first_logfreq)/
                                       (sg_last_xpixel-sg_first_xpixel);
  t2=pow(10,i);
  t1=j*t2;  
// t1 is the first integer frequency, t2 the increment and i the 
// decimal position.
  xa=0;
  sg_log_intfreq=t1;
  sg_logoffset=log10(sg_log_intfreq/sg_log_zerfreq);
  sg_log_xfac=(sg_log_zerfreq/sg_log_intfreq)*(sg_log_zerfreq/sg_log_intfreq);
  while(xa < sg_last_xpixel)
    {
    j++;
    t1+=t2;
    if(j == 10)
      {
      j=1;
      t2*=10;
      }
    xa=sg_logpix_from_freq(t1);
    if(xa > sg_last_xpixel-3*text_width)break;
    k=sg.ytop;
    if(xa > sg_last_xpixel-20*text_width)
      {
      k+=11*text_height;
      if(sg.mode == 3)
        {
        lir_line(xa,sg_ytop2,xa,sg.ytop,SG_DBSCALE_COLOR);  
        k=sg_ytop2+11*text_width;
        }
      }  
    lir_line(xa,sg_y0,xa,k,SG_DBSCALE_COLOR);  
    if(j==1 || j==5)
      {
      lir_line(xa-1,sg_y0,xa-1,sg_y0-3,15);
      lir_line(xa,sg_y0,xa,sg_y0-3,15);
      lir_line(xa+1,sg_y0,xa+1,sg_y0-3,15);
      if(t1 < 1.0)
        {
        sprintf(s,"%.1f",t1);
        }
      else
        {
        if(t1 < 10)
          {
          sprintf(s," %d",(int)t1);
          }
        else
          {
          sprintf(s,"%d",(int)t1);
          }
        }
      lir_pixwrite(xa-3*text_width/2,sg_y0+text_height/2,s);
      }
    }
  }
if(sg.mode == 3)
  {
  lir_hline(sg_first_xpixel,sg_mode3_ymid, sg_last_xpixel, 12);
  lir_hline(sg_first_xpixel,sg_mode3_ymid+sg_mode3_ypix, sg_last_xpixel, 12);
  lir_hline(sg_first_xpixel,sg_mode3_ymid-sg_mode3_ypix, sg_last_xpixel, 12);
  settextcolor(15);
  lir_pixwrite(sg.xleft+2*text_width,sg.ytop+2*text_height,"Re");
  settextcolor(14);
  lir_pixwrite(sg.xleft+2*text_width,sg.ytop+4*text_height,"Im");
  settextcolor(7);
  }
if(sg.xscale == 0)
  {
  if(sg.mode == 1 || sg.mode == 2)
    {
    i=1+sg_first_xpixel+(sg_last_xpixel-sg_first_xpixel)/16;
    i-=sg_pixels_per_point;
    sg_line(i,sg.ybottom,sg_last_xpixel,sg.ybottom,2);
    sg_line(i,sg.ybottom-1,sg_last_xpixel,sg.ybottom-1,2);
    sg_line(i,sg.ybottom-2,sg_last_xpixel,sg.ybottom-2,2);
    }
  }
lir_pixwrite(sg_last_xpixel-12*text_width,sg_ytop2+15*text_height/2,fft3_level);
lir_pixwrite(sg_last_xpixel-17*text_width,sg_ytop2+17*text_height/2,fft3_skip);
sprintf(s, "%6d",sg_corrnum);
lir_pixwrite(sg_last_xpixel-10*text_width,sg_ytop2+9*text_height/2,s);
i=recent_time-sg_reset_time;
j=i/3600;
i%=3600;
sprintf(s, "%02d:%02d:%02d",j,i/60,i%60);
lir_pixwrite(sg_last_xpixel-10*text_width,sg_ytop2+11*text_height/2,s);
}

void redraw_siganal_graph(void)
{
lir_sched_yield();  
hide_mouse(sg.xleft,sg.xright,sg.ytop,sg.ybottom);
lir_fillbox(sg.xleft,sg.ytop,sg.xright-sg.xleft+1,sg.ybottom-sg.ytop+1,0);
graph_borders((void*)&sg,7);
fill_sg_graph();
show_sg_buttons();
}

void update_siganal_graph(void)
{
float logsum_pn, logsum_an;
int logsum_n;
int i, n, n2, ia, ib, ix, iy, ixold;
int iya1, iya2, iyac, iyaq;
int iyp1, iyp2, iypc, iypq;
int sg_last_point;
float anq, an2, an1, anc;
float pnq, pn2, pn1, pnc;
float anpn_i, anpn_q;
float pwrfac,yfac;
float rat;
char anc_color, pnc_color;
char s[80];
// sg_spectrum is the y-value currently on screen.
hide_mouse(sg_first_xpixel, sg_last_xpixel, sg.ytop, sg.ybottom);
fill_sg_graph();
// Average the spectrum over n points.
if(sg.xscale == 0)
  {
  ix=sg_first_xpixel;
  n=sg_points_per_pixel;
  ia=0;
  ib=n;
  iy=ix+n;
  }
else
  {  
  ia=sg_first_logpoint;
  ix=sg_logpix_from_point(ia);
  ib=ia;
  iy=ix;
  while(ix == iy)
    {
    ib+=1;
    iy=sg_logpix_from_point(ib);
    }
  n=ib-ia;
  }
n2=n/2;
pwrfac=sg_siz/(3.956*baseband_sampling_speed);
yfac=-20*text_height/sg.ygain;
sg_last_point=3*sg_siz/8;
rat=(float)fft3_size/(float)sg_siz;
logsum_pn=0;
logsum_an=0;
logsum_n=0;
ixold=ix;
switch (sg.mode)
  {
  case 0:
zz:;
  while(ib < sg_last_point && iy < sg_last_xpixel)
    {
    anc=0;
    pnc=0;
    anq=0;
    pnq=0;
    for(i=ia; i<ib; i++)
      {
      anc+=sg_corrsum[2*i  ];
      anq+=sg_corrsum[2*i+1];
      pnc+=sg_corrsum[sg_siz+2*i  ];
      pnq+=sg_corrsum[sg_siz+2*i+1];
      }
    anc=sqrt(anc*anc+anq*anq);
    pnc=sqrt(pnc*pnc+pnq*pnq);  
    pnc*=fft3_fqwin_inv[fft3_size/2+(int)((float)(ia+n2)*rat)]/n;
    anc*=fft3_fqwin_inv[fft3_size/2+(int)((float)(ia+n2)*rat)]/n;
    if(anc < 1.e-25)anc=1.e-25;
    if(pnc < 1.e-25)pnc=1.e-25;
    iyac=yfac*(log10(pwrfac*anc/sg_corrnum)+sg.ymax/10.);
    iypc=yfac*(log10(pwrfac*pnc/sg_corrnum)+sg.ymax/10.);
    if(iyac < 1)iyac=1;
    if(iypc < 1)iypc=1;
    iyac+=sg_ytop2+text_height;
    iypc+=sg_ytop2+text_height;
    if(iyac > sg_y0-1)iyac=sg_y0-1;
    if(iypc > sg_y0-1)iypc=sg_y0-1;
    if(ix != sg_first_xpixel)
      {
      sg_line(ixold,sg_oldancspectrum[ixold],ix,sg_ancspectrum[ix],0);
      sg_oldancspectrum[ix]=sg_ancspectrum[ix];
      sg_ancspectrum[ix]=iyac;
      sg_line(ixold, sg_ancspectrum[ixold], ix, sg_ancspectrum[ix],15);
      sg_line(ixold,sg_oldpncspectrum[ixold],ix,sg_pncspectrum[ix],0);
      sg_oldpncspectrum[ix]=sg_pncspectrum[ix];
      sg_pncspectrum[ix]=iypc;
      sg_line(ixold, sg_pncspectrum[ixold], ix, sg_pncspectrum[ix],14);
      }
    else
      {
      sg_oldpncspectrum[ix]=sg_pncspectrum[ix];
      sg_oldancspectrum[ix]=sg_ancspectrum[ix];
      sg_ancspectrum[ix]=iyac;
      sg_pncspectrum[ix]=iypc;
      }
    ixold=ix;
    if(sg.xscale == 0)
      {
      ib+=n;
      ia+=n;
      ix+=sg_pixels_per_point;
      iy=ix;
      }
    else
      {   
      ia=ib+1;
      ib=ia;
      ix=sg_logpix_from_point(ia);
      iy=ix;
      while(ix == iy)
        {
        ib+=1;
        iy=sg_logpix_from_point(ib);
        }
      n=ib-ia;
      n2=n/2;
      }     
    }
  break;
    
  case 1:    
  while(ib < sg_last_point && ix < sg_last_xpixel)
    {
    pnq=0;
    pn1=0;
    pn2=0;
    pnc=0;
    for(i=ia; i<ib; i++)
      {
      pnc+=sg_corrsum[sg_siz+2*i];
      pnq+=sg_corrsum[sg_siz+2*i+1];
      pn1+=sg_pwrsum[4*i+1];
      pn2+=sg_pwrsum[4*i+3];
      }   
    pnq=fabs(pnq);
    if(pnc > 0)
      {
      pnc_color=14;
      }
    else
      {
      pnc_color=3;
      }
    pnc=fabs(pnc);  
    pnc*=fft3_fqwin_inv[fft3_size/2+(int)((float)(ia+n2)*rat)]/n;
    pnq*=fft3_fqwin_inv[fft3_size/2+(int)((float)(ia+n2)*rat)]/n;
    pn1*=fft3_fqwin_inv[fft3_size/2+(int)((float)(ia+n2)*rat)]/n;
    pn2*=fft3_fqwin_inv[fft3_size/2+(int)((float)(ia+n2)*rat)]/n;
    if(pnc < 1.e-25)pnc=1.e-25;
    if(pnq < 1.e-25)pnq=1.e-25;
    if(pn1 < 1.e-25)pn1=1.e-25;
    if(pn2 < 1.e-25)pn2=1.e-25;
    iypq=yfac*(log10(pwrfac*pnq/sg_corrnum)+sg.ymax/10.);
    iyp1=yfac*(log10(pwrfac*pn1/sg_numpow)+sg.ymax/10.);
    iyp2=yfac*(log10(pwrfac*pn2/sg_numpow)+sg.ymax/10.);
    iypc=yfac*(log10(pwrfac*pnc/sg_corrnum)+sg.ymax/10.);
    if(iypq < 1)iypq=1;
    if(iyp1 < 1)iyp1=1;
    if(iyp2 < 1)iyp2=1;
    if(iypc < 1)iypc=1;
    iypq+=sg_ytop2+text_height;
    iyp1+=sg_ytop2+text_height;
    iyp2+=sg_ytop2+text_height;
    iypc+=sg_ytop2+text_height;
    if(iypq > sg_y0-1)iypq=sg_y0-1;
    if(iyp1 > sg_y0-1)iyp1=sg_y0-1;
    if(iyp2 > sg_y0-1)iyp2=sg_y0-1;
    if(iypc > sg_y0-1)iypc=sg_y0-1;
    if(ix != sg_first_xpixel)
      {
      sg_line(ixold,sg_oldpncspectrum[ixold],ix,sg_pncspectrum[ix],0);
      sg_oldpncspectrum[ix]=sg_pncspectrum[ix];
      sg_pncspectrum[ix]=iypc;
      sg_line(ixold,sg_pncspectrum[ixold],ix,iypc,pnc_color);
      if(ix > sg_first_xpixel+(sg_last_xpixel-sg_first_xpixel)/16)
        {
        logsum_n++;
        logsum_pn+=log10(pwrfac*pnc/sg_corrnum);
        }
      }
    else
      {
      sg_oldpncspectrum[ix]=sg_pncspectrum[ix];
      sg_pncspectrum[ix]=iypc;
      }
    sg_setpixel(ix,sg_an1spectrum[ix],0);
    sg_setpixel(ix,sg_pn1spectrum[ix],0);
    sg_setpixel(ix,sg_pn2spectrum[ix],0);
    sg_setpixel(ix,iypq,12);
    sg_setpixel(ix,iyp1,10);
    sg_setpixel(ix,iyp2,13);
    sg_an1spectrum[ix]=iypq;
    sg_pn1spectrum[ix]=iyp1;
    sg_pn2spectrum[ix]=iyp2;
    ixold=ix;
    if(sg.xscale == 0)
      {
      ib+=n;
      ia+=n;
      ix+=sg_pixels_per_point;
      iy=ix;
      }
    else
      {   
      ia=ib+1;
      ib=ia;
      ix=sg_logpix_from_point(ia);
      iy=ix;
      while(ix == iy)
        {
        ib+=1;
        iy=sg_logpix_from_point(ib);
        }
      n=ib-ia;
      n2=n/2;
      }     
    }
  sprintf(s,"Avg %6.2f",-10*logsum_pn/logsum_n);
  lir_pixwrite(sg_last_xpixel-11*text_width,sg_ytop2+19*text_height/2,s);
  fprintf(stderr,"\n%d PN= %.2f",sg_corrnum,-10*logsum_pn/logsum_n);
  break;

  case 2:
  while(ib < sg_last_point && ix < sg_last_xpixel)
    {
    an1=0;
    an2=0;
    anq=0;
    anc=0;
    for(i=ia; i<ib; i++)
      {
      an1+=sg_pwrsum[4*i  ];
      an2+=sg_pwrsum[4*i+2];
      anc+=sg_corrsum[2*i];
      anq+=sg_corrsum[2*i+1];
      }   
    anq=fabs(anq);
    if(anc > 0)
      {
      anc_color=15;
      }
    else
      {
      anc_color=41;
      }
    anc=fabs(anc);  
    anc*=fft3_fqwin_inv[fft3_size/2+(int)((float)(ia+n2)*rat)]/n;
    anq*=fft3_fqwin_inv[fft3_size/2+(int)((float)(ia+n2)*rat)]/n;
    an1*=fft3_fqwin_inv[fft3_size/2+(int)((float)(ia+n2)*rat)]/n;
    an2*=fft3_fqwin_inv[fft3_size/2+(int)((float)(ia+n2)*rat)]/n;
    if(anq < 1.e-25)anq=1.e-25;
    if(an1 < 1.e-25)an1=1.e-25;
    if(an2 < 1.e-25)an2=1.e-25;
    if(anc < 1.e-25)anc=1.e-25;
    iya1=yfac*(log10(pwrfac*an1/sg_numpow)+sg.ymax/10.);
    iya2=yfac*(log10(pwrfac*an2/sg_numpow)+sg.ymax/10.);
    iyac=yfac*(log10(pwrfac*anc/sg_corrnum)+sg.ymax/10.);
    iyaq=yfac*(log10(pwrfac*anq/sg_corrnum)+sg.ymax/10.);
    if(iyaq < 1)iyaq=1;
    if(iya1 < 1)iya1=1;
    if(iya2 < 1)iya2=1;
    if(iyac < 1)iyac=1;
    iyaq+=sg_ytop2+text_height;
    iya1+=sg_ytop2+text_height;
    iya2+=sg_ytop2+text_height;
    iyac+=sg_ytop2+text_height;
    if(iyaq > sg_y0-1)iyaq=sg_y0-1;
    if(iya1 > sg_y0-1)iya1=sg_y0-1;
    if(iya2 > sg_y0-1)iya2=sg_y0-1;
    if(iyac > sg_y0-1)iyac=sg_y0-1;
    if(ix != sg_first_xpixel)
      {
      sg_line(ixold,sg_oldancspectrum[ixold],ix,sg_ancspectrum[ix],0);
      sg_oldancspectrum[ix]=sg_ancspectrum[ix];
      sg_ancspectrum[ix]=iyac;
      sg_line(ixold,sg_ancspectrum[ixold],ix,sg_ancspectrum[ix],anc_color);
      if(ix > sg_first_xpixel+(sg_last_xpixel-sg_first_xpixel)/16)
        {
        logsum_n++;
        logsum_an+=log10(pwrfac*anc/sg_corrnum);
        }
      }
    else
      {
      sg_oldancspectrum[ix]=sg_ancspectrum[ix];
      sg_ancspectrum[ix]=iyac;
      }  
    sg_setpixel(ix,sg_an1spectrum[ix],0);
    sg_setpixel(ix,sg_pn1spectrum[ix],0);
    sg_setpixel(ix,sg_pn2spectrum[ix],0);
    sg_setpixel(ix,iya1,10);
    sg_setpixel(ix,iya2,13);
    sg_setpixel(ix,iyaq,12);
    sg_an1spectrum[ix]=iyaq;
    sg_pn1spectrum[ix]=iya1;
    sg_pn2spectrum[ix]=iya2;
    ixold=ix;
    if(sg.xscale == 0)
      {
      ib+=n;
      ia+=n;
      ix+=sg_pixels_per_point;
      }
    else
      {   
      ia=ib+1;
      ib=ia;
      ix=sg_logpix_from_point(ia);
      iy=ix;
      while(ix == iy)
        {
        ib+=1;
        iy=sg_logpix_from_point(ib);
        }
      n=ib-ia;
      n2=n/2;
      }     
    }
  sprintf(s,"Avg %6.2f",-10*logsum_an/logsum_n);
  lir_pixwrite(sg_last_xpixel-11*text_width,sg_ytop2+19*text_height/2,s);
  fprintf(stderr,"\n%d AN= %.2f",sg_corrnum,-10*logsum_an/logsum_n);
  break;

  case 3:
  while(ib < sg_last_point && ix < sg_last_xpixel)
    {
    anpn_i=0;
    anpn_q=0;
    anc=0;
    anq=0;
    pnc=0;
    pnq=0;
    for(i=ia; i<ib; i++)
      {
      anc+=sg_corrsum[2*i];
      anq+=sg_corrsum[2*i+1];
      pnc+=sg_corrsum[sg_siz+2*i];
      pnq+=sg_corrsum[sg_siz+2*i+1];
      anpn_i+=sg_anpn_corr[2*i  ];
      anpn_q+=sg_anpn_corr[2*i+1];
      }
    anpn_i/=sqrt(2*(anc*anc+anq*anq+pnc*pnc+pnq*pnq));  
    anpn_q/=sqrt(2*(anc*anc+anq*anq+pnc*pnc+pnq*pnq));  
    if(anpn_i > 1)anpn_i=1;
    if(anpn_q > 1)anpn_q=1;
    if(anpn_i < -1)anpn_i=-1;
    if(anpn_q < -1)anpn_q=-1;
    iyac=sg_mode3_ypix*anpn_i+sg_mode3_ymid;
    iypc=sg_mode3_ypix*anpn_q+sg_mode3_ymid;
    if(ix != sg_first_xpixel)
      {
      sg_line(ixold,sg_oldanpncorr_ispectrum[ixold],ix,
                    sg_anpncorr_ispectrum[ix],0);
      sg_oldanpncorr_ispectrum[ix]=sg_anpncorr_ispectrum[ix];
      sg_anpncorr_ispectrum[ix]=iyac;
      sg_line(ixold,sg_anpncorr_ispectrum[ixold],ix,
                    sg_anpncorr_ispectrum[ix],15);
      sg_line(ixold,sg_oldanpncorr_qspectrum[ixold],ix,
                    sg_anpncorr_qspectrum[ix],0);
      sg_oldanpncorr_qspectrum[ix]=sg_anpncorr_qspectrum[ix];
      sg_anpncorr_qspectrum[ix]=iypc;
      sg_line(ixold,sg_anpncorr_qspectrum[ixold],ix,
                    sg_anpncorr_qspectrum[ix],14);
      }
    else
      {
      sg_oldanpncorr_ispectrum[ix]=sg_anpncorr_ispectrum[ix];  
      sg_oldanpncorr_qspectrum[ix]=sg_anpncorr_qspectrum[ix];  
      sg_anpncorr_ispectrum[ix]=iyac;
      sg_anpncorr_qspectrum[ix]=iypc;
      }  
    ixold=ix;
    if(sg.xscale == 0)
      {
      ib+=n;
      ia+=n;
      ix+=sg_pixels_per_point;
      iy=ix;
      }
    else
      {   
      ia=ib+1;
      ib=ia;
      ix=sg_logpix_from_point(ia);
      iy=ix;
      while(ix == iy)
        {
        ib+=1;
        iy=sg_logpix_from_point(ib);
        }
      n=ib-ia;
      n2=n/2;
      }     
    }
  if(sg.xscale == 0)
    {
    ix=sg_first_xpixel;
    n=sg_points_per_pixel;
    ia=0;
    ib=n;  
    iy=ix+sg_pixels_per_point;
    }
  else
    {
    ia=sg_first_logpoint;
    ix=sg_logpix_from_point(ia);
    ib=ia;
    iy=ix;
    while(ix == iy)
      {
      ib+=1;
      iy=sg_logpix_from_point(ib);
      }
    n=ib-ia;
    }
  n2=n/2;
  ixold=ix;
  goto zz;
  }
}

void show_vg_buttons(void)
{
char s[80];
settextcolor(7);
if(vg.mintau >= 1)
  {
  sprintf(s,"Tmin %4.0f",vg.mintau);
  }
else
  {  
  sprintf(s,"Tmin %4.2f",vg.mintau);
  }
show_button(&vgbutt[VG_NEW_MINTAU],s);
if(vg.maxtau >= 1)
  {
  sprintf(s,"Tmax %5.0f",vg.maxtau);
  }
else
  {  
  sprintf(s,"Tmax %5.3f",vg.maxtau);
  }
show_button(&vgbutt[VG_NEW_MAXTAU],s);
sprintf(s,"Pnts %d",vg.points_per_decade);
show_button(&vgbutt[VG_NEW_POINTS_PER_DECADE],s);
sprintf(s,"Ymin %d",vg.ymin);
show_button(&vgbutt[VG_NEW_YMIN],s);
sprintf(s,"Ymax %d",vg.ymax);
show_button(&vgbutt[VG_NEW_YMAX],s);
s[0]=vg_modes[vg.mode];
s[1]=0;
show_button(&vgbutt[VG_NEW_MODE],s);
s[0]=vg_clears[vg.clear];
s[1]=0;
show_button(&vgbutt[VG_NEW_CLEAR],s);

}

int vg_tau_to_xpix(double tau)
{
return (int)(log10(tau/vg.mintau)*vg_xpix_per_decade+0.5F)+vg_first_xpixel;
}

void fill_vg_graph(void)
{
char s [20];
int i, k, m, iy, jy, yval, x0, jx  ;
double dt1, dt2;
iy=vg_yt;
yval=vg.ymax;
x0=vg.xleft+text_width/2;
while(iy <= vg_yb)
  {
  sprintf(s,"1E-%d",yval);
  lir_pixwrite(x0,iy+2-text_height/2,s);
  lir_hline(vg_first_xpixel,iy,vg_last_xpixel, VG_DBSCALE_COLOR);
  for(i=1; i<10; i++)
    {
    jy=iy+vg_decimal_ypixel[i];
    if(jy < vg_yb)
      { 
      lir_hline(vg_first_xpixel, jy, vg_last_xpixel,
                                               VG_DBSCALE_COLOR);
      }
    }
  iy+=vg_ypix_per_decade;
  yval++;
  }
// Draw vertical lines for the time scale on the X-axis.
// We want the scale from vg.mintau to vg.maxtau.
dt1=0.00001;
m=0;
while(dt1 <= vg.maxtau-.000001)
  {
  if(dt1 >= vg.mintau-.00001)
    {  
    i=vg_tau_to_xpix(dt1);
    lir_line(i,vg_yb,i, vg_yt, VG_DBSCALE_COLOR);
    if(dt1 < 0.999)
      {
      sprintf(s,"%3.2fs",dt1);
      }
    else
      {
      sprintf(s,"%3.0fs",dt1);
      }
    jx=i-3*text_width+(strlen(s)+1)/2;  
    lir_pixwrite(jx, vg.ybottom-text_height,s);  
    for(k=1; k<10; k++)
      {
      jx=i+vg_xpix_per_decade-vg_decimal_xpixel[k];
      if(jx < vg_last_xpixel)
        { 
        lir_line(jx, vg_yb, jx,vg_yt, VG_DBSCALE_COLOR);
        }
      }
    if(m == 0)
      {
      m=-1;
      k=9;
next:;      
      jx=i-vg_decimal_xpixel[k];
      if(jx >= vg_first_xpixel-1)
        { 
        lir_line(jx, vg_yb, jx,vg_yt, VG_DBSCALE_COLOR);
        k--;
        goto next;
        }
      if(k==0)k=1;  
      if(k != 9)
        {  
        dt2=(k+1)*dt1/10.; 
        if(dt2 < 0.999)
          {
          sprintf(s,"%3.2fs",dt2);
          }
        else
          {
          sprintf(s,"%3.0fs",dt2);
          }
        jx=i-vg_decimal_xpixel[k+1];
        jx-=3*text_width+(strlen(s)+1)/2;  
        lir_pixwrite(jx, vg.ybottom-text_height,s);
        }
      }
    } 
  dt1*= 10;
  }  
}

void redraw_allan_graph(void)
{
lir_sched_yield();  
hide_mouse(vg.xleft,vg.xright,vg.ytop,vg.ybottom);
lir_fillbox(vg.xleft,vg.ytop,vg.xright-vg.xleft+1,vg.ybottom-vg.ytop+1,0);
graph_borders((void*)&vg,7);
fill_vg_graph();
show_vg_buttons();
}

void update_allan_graph(void)
{
int i, j, x, iy1, y2, yc, xold;
char s[32];
double centerinv, sum, dy, dy1, dy2;
unsigned char color;
//hide_mouse(vg_first_xpixel, vg_last_xpixel, vg.ytop, vg.ybottom);
fill_vg_graph();
centerinv=.000001/(fg.passband_center*sqrt(2.0));
x=0;
yc=0;
xold=vg_first_xpixel;
for(i=0;i<vg_no_of_tau; i++)
  {
  if(vg.mode == 0)
    {
    if(vg_sumno[i] > 0)
      {
      x=vg_tau_to_xpix(vg_tau[i]/baseband_sampling_speed);
      dy1=vg_asum[2*i  ];
      dy2=vg_asum[2*i+1];
      dy1=centerinv*sqrt(dy1/(2*vg_sumno[i]));
      dy2=centerinv*sqrt(dy2/(2*vg_sumno[i]));
      if(dy1 < 1.e-18)dy1=1.e-18;
      if(dy2 < 1.e-18)dy2=1.e-18;
      iy1=vg_yb-(int)((vg_ypix_per_decade)*(log10(dy1)+vg.ymin));
      y2=vg_yb-(int)((vg_ypix_per_decade)*(log10(dy2)+vg.ymin));
      if(y2 < vg_yt)y2=vg_yt;
      if(y2 > vg_yb)y2=vg_yb;
      if(vg_acorrsum[i] < 0)
        {
        color=3;
        sum=-vg_acorrsum[i];
        }
      else
        {  
        color=14;
        sum=vg_acorrsum[i];
        }
      if(sum < 1.e-18)sum=1.e-18;
      dy=centerinv*sqrt(sum/(2*vg_sumno[i]));
      yc=vg_yb-(int)((vg_ypix_per_decade)*(log10(dy)+vg.ymin));
      if(yc < vg_yt)yc=vg_yt;
      if(yc > vg_yb)yc=vg_yb;
      if(iy1 < vg_yt)iy1=vg_yt;
      if(iy1 > vg_yb)iy1=vg_yb;
      if(y2 < vg_yt)y2=vg_yt;
      if(y2 > vg_yb)y2=vg_yb;
      if(vg.clear == 0 && i != 0)
        {
        lir_line(xold,vg_ycpix[2*(i-1)+1],x,vg_ycpix[2*i],0);
        }
      if(i != 0)lir_line(xold,vg_ycpix[2*(i-1)],x,yc,color);
      lir_setcross(x,vg_y1pix[i],0);
      lir_setcross(x,vg_y2pix[i],0);
      lir_setcross(x,vg_ycpix[2*i],0);
      vg_y1pix[i]=iy1;
      vg_y2pix[i]=y2;
      vg_ycpix[2*i+1]=vg_ycpix[2*i];
      vg_ycpix[2*i]=yc;
      lir_setcross(x,yc,color);
      lir_setcross(x,iy1,10);
      lir_setcross(x,y2,55);
      xold=x;
      }
    }
  else
    {
    if(vg_sumno[i] > 0)
      {
      x=vg_tau_to_xpix(vg_tau[i]/baseband_sampling_speed);
      dy1=vg_hsum[2*i  ];
      dy2=vg_hsum[2*i+1];
      if(dy1 < 1.e-18)dy1=1.e-18;
      if(dy2 < 1.e-18)dy2=1.e-18;
      dy1=centerinv*sqrt(dy1/(2*vg_sumno[i]));
      dy2=centerinv*sqrt(dy2/(2*vg_sumno[i]));
      iy1=vg_yb-(int)((vg_ypix_per_decade)*(log10(dy1)+vg.ymin));
      y2=vg_yb-(int)((vg_ypix_per_decade)*(log10(dy2)+vg.ymin));
      if(y2 < vg_yt)y2=vg_yt;
      if(y2 > vg_yb)y2=vg_yb;
      if(vg_hcorrsum[i] < 0)
        {
        color=3;
        sum=-vg_hcorrsum[i];
        }
      else
        {  
        color=14;
        sum=vg_hcorrsum[i];
        }
      dy=centerinv*sqrt(sum/(2*vg_sumno[i]));
      yc=vg_yb-(int)((vg_ypix_per_decade)*(log10(dy)+vg.ymin));
      if(yc < vg_yt)yc=vg_yt;
      if(yc > vg_yb)yc=vg_yb;
      if(iy1 < vg_yt)iy1=vg_yt;
      if(iy1 > vg_yb)iy1=vg_yb;
      if(y2 < vg_yt)y2=vg_yt;
      if(y2 > vg_yb)y2=vg_yb;
      if(vg.clear == 0 && i != 0)
        {
        lir_line(xold,vg_ycpix[2*(i-1)+1],x,vg_ycpix[2*i],0);
        }
      if(i != 0)lir_line(xold,vg_ycpix[2*(i-1)],x,yc,color);
      lir_setcross(x,vg_y1pix[i],0);
      lir_setcross(x,vg_y2pix[i],0);
      lir_setcross(x,vg_ycpix[2*i],0);
      vg_y1pix[i]=iy1;
      vg_y2pix[i]=y2;
      vg_ycpix[2*i+1]=vg_ycpix[2*i];
      vg_ycpix[2*i]=yc;
      lir_setcross(x,yc,color);
      lir_setcross(x,iy1,10);
      lir_setcross(x,y2,55);
      xold=x;
      }
    }
  }
i=recent_time-vg_reset_time;
j=i/3600;
i%=3600;
sprintf(s, "%02d:%02d:%02d",j,i/60,i%60);
lir_pixwrite(vgbutt[VG_NEW_CLEAR].x1-17*text_width/2,vg.ytop+text_height/2,s);
}

void fill_vgf_graph(void)
{
int i, j, k, m, h, iy0;
char s[16];
float timestep, t1;
// One y pixel is vgf.freqgain milliHz.
// Place scale divisions at a multiple of 10 pixels
iy0=(vgf_yt+vgf_yb)/2;
lir_hline(vgf_first_xpixel,iy0,vgf_last_xpixel,VGF_FREQSCALE_COLOR);
lir_pixwrite(vgf.xleft+2+5*text_width,iy0-text_height/2,"0");
lir_pixwrite(vgf.xleft+text_width/2,iy0-text_height/2,"mHz");
m=2*text_height;
i=10;
while(i < m)i+=10;
j=i;
h=(vgf_yb-vgf_yt)/2;
while(j<h)
  {
  lir_hline(vgf_first_xpixel,iy0+j,vgf_last_xpixel,VGF_FREQSCALE_COLOR);
  lir_hline(vgf_first_xpixel,iy0-j,vgf_last_xpixel,VGF_FREQSCALE_COLOR);
  sprintf(&s[1],"%.4f",j*vgf.freqgain+.00001);
  s[0]=' ';
  m=strlen(s)-1;
  while(s[m]=='0')m--;
  if(s[m]== '.')m--;
  m++;
  s[m]=0;
  lir_pixwrite(vgf.xleft+2+(7-m)*text_width,iy0-j-text_height/2,s);
  s[0]='-';
  lir_pixwrite(vgf.xleft+2+(7-m)*text_width,iy0+j-text_height/2,s);
  j+=i;
  }  
// Place a time scale at the bottom. One pixel=vgf.time
// Place vertical lines for the time scale at least 7 characters apart.
lir_line(vgf_first_xpixel,vgf_yt,vgf_first_xpixel,vgf_yb,VGF_FREQSCALE_COLOR);
lir_pixwrite(vgf_first_xpixel-text_width/2,vgf.ybottom-text_height,"0");
m=7*text_width; 
i=100;
while(i < m)i+=100;
timestep=i*vgf.time;
k=0;
if(timestep > 10)
  {
  k--;
  timestep/=10;
  }
if(timestep < 1)
  {
  k++;
  timestep*=10;
  }
timestep=nearbyint(timestep);
while(k > 0)
  {  
  timestep/=10;
  k--;
  }
while(k<0)
  {
  timestep*=10;
  k++;
  }  
j=0;
t1=timestep;
while(j<vgf_last_xpixel-4*text_width)
  {
  j=timestep/vgf.time+vgf_first_xpixel;
  lir_line(j,vgf_yt,j,vgf_yb,VGF_FREQSCALE_COLOR);
  sprintf(s,"%.4f",timestep);
  m=strlen(s)-1;
  while(s[m]=='0')m--;
  if(s[m]== '.')m--;
  m++;
  s[m]='s';
  m++;
  s[m]=0;
  lir_pixwrite(j-(m*text_width)/2,vgf.ybottom-text_height,s);
  j+=i;
  timestep+=t1;
  }

}

void show_vgf_buttons(void)
{
char s[32];
if(vgf.time >= 1)
  {
  sprintf(s,"T %.0f",vgf.time);
  }
else
  {  
  sprintf(s,"T %3.2f",vgf.time);
  }
show_button(&vgfbutt[VGF_NEW_TIME],s);
if(vgf.freqgain >= 1)
  {
  sprintf(s,"mHz %6.0f",vgf.freqgain);
  }
else
  {  
  sprintf(s,"mHz %6.5f",vgf.freqgain);
  }
show_button(&vgfbutt[VGF_NEW_FREQGAIN],s);
s[0]=42;
s[1]=0;
show_button(&vgfbutt[VGF_NEW_CENTER_TRACES],s);
}

void redraw_allanfreq_graph(void)
{
hide_mouse(vgf.xleft,vgf.xright,vgf.ytop,vgf.ybottom);
lir_fillbox(vgf.xleft,vgf.ytop,vgf.xright-vgf.xleft+1,vgf.ybottom-vgf.ytop+1,0);
graph_borders((void*)&vgf,7);
show_vgf_buttons();
fill_vgf_graph();
}

void update_allanfreq_graph(void)
{
char s[40];
int i, k, m, n, p0, iy0;
double dy1, dy2, dt1, dt2;
int x, iy1, y2;
#if SHOW_ALLAN_FREQDIFF == TRUE
double dy3;
int y3;
#endif
// Look for frequency as a function of time
k=(vgf_pa+1-vgf_px+vgf_size)&vgf_mask;
if(k < 5)return;
//Find the average center frequency for the latest 20 points
if(k > 20)
  {
  p0=(vgf_pa-20+vgf_size)&vgf_mask;
  }
else
  {  
  p0=vgf_px;
  }
m=vgf_pa;   
dt1=0;
dt2=0;
n=0;
while(p0 != m)
  {
  dt1+=vgf_freq[2*p0  ];
  dt2+=vgf_freq[2*p0+1];
  p0=(p0+1)&vgf_mask;
  n++;
  }
dt1/=n;
dt2/=n;
if(vgf_mid_freq == BIGDOUBLE || vgf_center_traces == TRUE)
  {
  vgf_mid_freq=(dt1+dt2)/2.0;
  redraw_allanfreq_graph();
  fill_vgf_graph();
  vgf_center_traces=FALSE;
  }
lir_fillbox(vgf_first_xpixel,vgf_yt,vgf_xpixels,vgf_yb-vgf_yt+1,0);
fill_vgf_graph();
// Redraw the curves. k is the number of points we have.
p0=vgf_px;
x=vgf_first_xpixel;
iy0=(vgf_yt+vgf_yb)/2;
while(p0 != vgf_pa)
  {
  dy1=vgf_freq[2*p0  ]-vgf_mid_freq;
  dy2=vgf_freq[2*p0+1]-vgf_mid_freq;
  iy1=iy0-dy1*1000.0/vgf.freqgain;
  y2=iy0-dy2*1000.0/vgf.freqgain;
  if(iy1 < vgf_yt)iy1=vgf_yt;
  if(y2 < vgf_yt)y2=vgf_yt;
  if(iy1 > vgf_yb)iy1=vgf_yb;
  if(y2 > vgf_yb)y2=vgf_yb;
  lir_setpixel(x,iy1,10);
  lir_setpixel(x,y2,55);
#if SHOW_ALLAN_FREQDIFF == TRUE
  dy3=vgf_freq[2*p0  ]-vgf_freq[2*p0+1];
  y3=iy0-dy3*1000.0/vgf.freqgain;
  if(y3 < vgf_yt)y3=vgf_yt;
  if(y3 > vgf_yb)y3=vgf_yb;
  lir_setpixel(x,y3,14);
 #endif 
  x++;
  p0=(p0+1)&vgf_mask;
  }
for(i=0;i<32; i++)s[i]=' ';
s[32]=0;
lir_pixwrite(vgf_freq_xpix, vgf.ytop+text_height/2, s);
sprintf(s,"%.7f Hz %.7f Hz %.0f%c",vg_basebfreq1,vg_basebfreq2,
                                         vg_interchannel_phase,0xf7);
lir_pixwrite(vgf_freq_xpix, vgf.ytop+text_height/2, s);
}


void show_wg_buttons(int all)
{
int iw;
char s[256];
hide_mouse(wg.xleft, wg.xright, wg.yborder, wg.ybottom);
if(all)
  {
  sprintf(s,"%5d",wg.spek_avgnum);
  show_button(&wgbutt[WG_FFT1_AVGNUM],s);
  if(kill_all_flag) return;
  }
if(wg_freq_adjustment_mode > 0)
  {
  if(par_from_keyboard_routine != change_wg_lowest_freq)
    {
    sprintf(s,"%f",wg_lowest_freq);
    show_button(&wgbutt[WG_LOWEST_FREQ],s);
    }
  if(par_from_keyboard_routine != change_wg_highest_freq)
    {
    sprintf(s,"%f",wg_highest_freq);
    show_button(&wgbutt[WG_HIGHEST_FREQ],s);
    }
  iw=(wgbutt[WG_FREQ_ADJUSTMENT_MODE].x2 -
                         wgbutt[WG_FREQ_ADJUSTMENT_MODE].x1)/text_width;
  iw=iw/2-3;
  if(iw<0)iw=0;
  sprintf(&s[iw],"Apply");                  
  iw--;
  while(iw >=0)
    {
    s[iw]=' ';
    iw--;
    }
  settextcolor(14);
  show_button(&wgbutt[WG_FREQ_ADJUSTMENT_MODE],s);
  settextcolor(7);
  }
if(genparm[AFC_ENABLE] == 2 && genparm[MAX_NO_OF_SPURS] != 0)
  {
  if(wg.spur_inhibit == 0)
    {
    button_color=14;
    }
  else
    {
    button_color=3;
    }  
  settextcolor(button_color);
  show_button(&wgbutt[WG_SPUR_TOGGLE],"c");
  button_color=7;
  settextcolor(7);
  }
iw=0;
if(par_from_keyboard_routine != change_waterfall_avgnum)
  {
  sprintf(s,"%4d",wg.waterfall_avgnum);
  if(wgbutt[WG_WATERF_AVGNUM].y1 > wg.ytop+1.5*text_height)
    {
    show_wtrfbutton(&wgbutt[WG_WATERF_AVGNUM],s);
    if(kill_all_flag) return;
    }
  }
else
  {
  iw=WG_WATERF_AVGNUM+1;
  }
if(par_from_keyboard_routine != change_wg_waterfall_zero)
  {
  sprintf(s,"%f",wg.waterfall_db_zero);
  s[5]=0;
  if(wgbutt[WG_WATERF_ZERO].y1 > wg.ytop+1.5*text_height)
    {
    show_wtrfbutton(&wgbutt[WG_WATERF_ZERO],s);
    if(kill_all_flag) return;
    }    
  }
else
  {
  iw=WG_WATERF_ZERO+1;
  }
if(par_from_keyboard_routine != change_wg_waterfall_gain)
  {
  sprintf(s,"%f",wg.waterfall_db_gain);
  s[4]=0;
  if(wgbutt[WG_WATERF_GAIN].y1 > wg.ytop+1.5*text_height)
    {
    show_wtrfbutton(&wgbutt[WG_WATERF_GAIN],s);
    if(kill_all_flag) return;
    }
  }
else
  {
  iw=WG_WATERF_GAIN+1;
  }
if(iw != 0)
  {
  settextcolor(15);
  show_wtrfbutton(&wgbutt[iw-1],numinput_txt);
  }
settextcolor(7);  
}

void show_bg_buttons(int all)
{
int iw;
char s[80];
char fmsubtr[3]={'S','X','M'};
// First make buttons that use the default colour (=7)
hide_mouse(bg.xleft, bg.xright, bg.yborder, bg.ybottom);
if(all)
  {
  sprintf(s,"%4d",bg.fft_avgnum);
  show_button(&bgbutt[BG_SEL_FFT3AVGNUM],s);
  s[0]=arrow_mode_char[bg.horiz_arrow_mode];
  s[1]=0;
  show_button(&bgbutt[BG_HORIZ_ARROW_MODE],s);
  s[0]='0'+bg.mixer_mode;
  show_button(&bgbutt[BG_MIXER_MODE],s);
  if(bg.mixer_mode == 1)
    {
    s[0]='0'+bg_current_notch;
    show_button(&bgbutt[BG_NOTCH_NO],s);
    }
  if(rx_mode == MODE_AM  && bg.mixer_mode == 1)
    {
    sprintf(s,"Sh%4d",bg.filter_shift);
    show_button(&bgbutt[BG_FILTER_SHIFT],s);
    }
  if(bg_no_of_notches > 0 && bg_current_notch > 0 && bg.mixer_mode == 1)
    {
    sprintf(s,"w%3d",bg_notch_width[bg_current_notch-1]);
    show_button(&bgbutt[BG_NOTCH_WIDTH],s);
    sprintf(s,"Pos%4d",bg_notch_pos[bg_current_notch-1]);
    show_button(&bgbutt[BG_NOTCH_POS],s);
    }
  else
    {
    lir_fillbox(bgbutt[BG_NOTCH_WIDTH].x1,bgbutt[BG_NOTCH_WIDTH].y1,
        bgbutt[BG_NOTCH_WIDTH].x2-bgbutt[BG_NOTCH_WIDTH].x1+1, text_height+2,0);
    lir_fillbox(bgbutt[BG_NOTCH_POS].x1,bgbutt[BG_NOTCH_POS].y1,
        bgbutt[BG_NOTCH_POS].x2-bgbutt[BG_NOTCH_POS].x1+1, text_height+2,0);
    }
  if(rx_daout_bytes==1)
    {
    s[0]=' ';
    s[1]='8';
    }
  else
    {
    s[0]='1';
    s[1]='6';
    }
  s[2]=0; 
  show_button(&bgbutt[BG_TOGGLE_BYTES],s);
  if(kill_all_flag) return;
// **************************************  
  sprintf(s,"Rat%3d",bg.coh_factor);
  show_button(&bgbutt[BG_SEL_COHFAC],s);
  if(kill_all_flag) return;
// **************************************  
  if(rx_mode == MODE_FM)
    {
    sprintf(s,"FM%d",bg.fm_mode);
    show_button(&bgbutt[BG_TOGGLE_FM_MODE],s);
    s[0]=fmsubtr[bg.fm_subtract];
    s[1]=0;
    show_button(&bgbutt[BG_TOGGLE_FM_SUBTRACT],s);
    sprintf(s,"Bw%2d",bg.fm_audio_bw);
    show_button(&bgbutt[BG_SEL_FM_AUDIO_BW],s);
    }
  else  
    {
// Buttons that change colour when something is enabled
    if(fft1_correlation_flag <= 1)
      {
      if( bg.agc_flag == 0 )
        {
        button_color=BG_INACTIVE_BUTTON_COLOR;
        }
      else
        {
        button_color=BG_ACTIVE_BUTTON_COLOR;
        }
      settextcolor(button_color);
      if(button_color == BG_INACTIVE_BUTTON_COLOR)
        {
        sprintf(s,"Off");
        }
      else
        {
        sprintf(s,"A%d",bg.agc_attack);
        show_button(&bgbutt[BG_SEL_AGC_ATTACK],s);
        if(kill_all_flag) return;
        sprintf(s,"R%d",bg.agc_release);
        show_button(&bgbutt[BG_SEL_AGC_RELEASE],s);
        sprintf(s,"H%d",bg.agc_hang);
        show_button(&bgbutt[BG_SEL_AGC_HANG],s);
        if(kill_all_flag) return;
        sprintf(s,"AGC");
        if( bg.agc_flag == 2)
          {
          if(bg_twopol == 0)
            {
            sprintf(s,"COH");
            }
          else
            {
            sprintf(s,"X+Y");
            }
          }          
        }
      show_button(&bgbutt[BG_TOGGLE_AGC],s);
      if(kill_all_flag) return;
      if( bg_expand == 0 )
        {
        button_color=BG_INACTIVE_BUTTON_COLOR; 
        }
      else
        {
        button_color=BG_ACTIVE_BUTTON_COLOR;
        }
      settextcolor(button_color);
      if(button_color == BG_INACTIVE_BUTTON_COLOR)
        {
        sprintf(s,"Off");
        }
      else
        {
        if(bg_expand == 1)sprintf(s,"Exp");else sprintf(s,"Lim");
        }
      if(rx_mode != MODE_AM && rx_mode != MODE_FM)
        {
        show_button(&bgbutt[BG_TOGGLE_EXPANDER],s);
        if(kill_all_flag) return;
      }
    }
// ***************************************
    if( bg_coherent == 0 )
      {
      button_color=BG_INACTIVE_BUTTON_COLOR;
      }
    else
      {
      button_color=BG_ACTIVE_BUTTON_COLOR;
      }
    settextcolor(button_color);
    if(button_color == BG_INACTIVE_BUTTON_COLOR)
      {
      sprintf(s,"Off");
      }
    else
      {
      sprintf(s,"Coh%d",bg_coherent);
      }
    show_button(&bgbutt[BG_TOGGLE_COHERENT],s);
    if(kill_all_flag) return;
// **************************************  
    if( bg_delay == 0 )
      {
      button_color=BG_INACTIVE_BUTTON_COLOR;
      }
    else
      {
      button_color=BG_ACTIVE_BUTTON_COLOR;
      }
    settextcolor(button_color);
    if(button_color == BG_INACTIVE_BUTTON_COLOR)
      {
      sprintf(s,"Off");
      }
    else
      {
      sprintf(s,"Del");
      }
    show_button(&bgbutt[BG_TOGGLE_PHASING],s);
    if(kill_all_flag) return;
    sprintf(s,"%3d",bg.delay_points);
    show_button(&bgbutt[BG_SEL_DELPNTS],s);
    if(kill_all_flag) return;
// **************************************  
    if(ui.rx_rf_channels == 2)
      {
      if( bg_twopol == 0 )
        {
        button_color=BG_INACTIVE_BUTTON_COLOR; 
        }
      else
        {
        button_color=BG_ACTIVE_BUTTON_COLOR;
        }
      settextcolor(button_color);
      if(button_color == BG_INACTIVE_BUTTON_COLOR)
        {
        sprintf(s,"Off");
        }
      else
        {
        sprintf(s,"X+Y");
        }
      show_button(&bgbutt[BG_TOGGLE_TWOPOL],s);
      if(kill_all_flag) return;
      }
    button_color = 7;
    settextcolor(button_color);
    }
  }  
iw=0;
if(bg.yborder-bg.ytop > 2.5*text_height+2)
  {
  if(par_from_keyboard_routine!=change_bg_waterf_avgnum)
    {  
    sprintf(s,"%4d",bg.waterfall_avgnum);
    show_wtrfbutton(&bgbutt[BG_WATERF_AVGNUM],s);
    if(kill_all_flag) return;
    }
  else
    {
    iw=BG_WATERF_AVGNUM+1;
    }
  if(par_from_keyboard_routine != new_bg_waterfall_zero)
    {
    sprintf(s,"%f",bg.waterfall_zero);
    s[5]=0;
    show_wtrfbutton(&bgbutt[BG_WATERF_ZERO],s);
    if(kill_all_flag) return;
    }
  else
    {
    iw=BG_WATERF_ZERO+1;
    }
  }    
if(bg.yborder-bg.ytop > 3.5*text_height + 2)
  {
  if(par_from_keyboard_routine != new_bg_waterfall_gain)
    {
    sprintf(s,"%f",bg.waterfall_gain);
    s[4]=0;
    show_wtrfbutton(&bgbutt[BG_WATERF_GAIN],s);
    if(kill_all_flag) return;
    }
  else
    {
    iw=BG_WATERF_GAIN+1;
    }  
  }
if(iw != 0)
  {
  settextcolor(15);
  show_wtrfbutton(&bgbutt[iw-1],numinput_txt);
  }
settextcolor(7);  
}

void redraw_wg_waterfall(void)
{
int k;
int jw, color, i, ix, iy,iymax;
make_wg_waterf_cfac();
iy=wg_waterf_y;
iymax=wg_waterf_y+wg_waterf_lines-wg_waterf_yinc;
hide_mouse(wg.xleft, wg_last_xpixel, wg_waterf_y, iymax);
jw=wg_waterf_ptr2;
k=0;
while(iy < iymax)
  {
  lir_hline(wg.xleft+1,iy,wg_first_xpixel-1,0);
  i=wg_first_xpixel;
  ix=0;
  while(ix<wg_xpixels && i<screen_width)
    {
    color=wg_waterf_cfac*(wg_waterf[jw+ix]-wg_waterf_czer);
    ix++;
    if(color<0)
      {
      color=0;
      }
    else
      {  
      if(color >= MAX_COLOR_SCALE)color=MAX_COLOR_SCALE-1;
      }
    lir_setpixel(i,iy,color_scale[color]);
    i++;
    }
  iy++;
  jw+=wg_xpixels;
  if(jw > wg_waterf_size-wg_xpixels-1)jw=0;
  k++;
  if(audio_dump_flag && no_of_processors >2)k=0;
  if(k>=waterfall_yield_interval)
    {
    k=0;
    if(thread_command_flag[THREAD_SCREEN]==THRFLAG_KILL)return;
    if(thread_command_flag[THREAD_SCREEN]==THRFLAG_IDLE)
      {
      thread_status_flag[THREAD_SCREEN]=THRFLAG_IDLE;
      while(thread_command_flag[THREAD_SCREEN] == THRFLAG_IDLE)
        {
        lir_await_event(EVENT_SCREEN);
        if(thread_command_flag[THREAD_SCREEN]==THRFLAG_KILL)return;
        }
      thread_status_flag[THREAD_SCREEN]=THRFLAG_ACTIVE;
      }  
    else
      {
      lir_sched_yield();
      }
    }
  }  
i=0;
while(wg_waterf_times[i].line < local_wg_yborder-2*text_height)
  {
  lir_pixwrite(wg.xleft+2, wg_waterf_times[i].line,wg_waterf_times[i].text);
  i++;
  }
show_wg_buttons(0);
}


void redraw_bg_waterfall(void)
{
int k;
int jw, color, i, ix, iy, iymax;
make_bg_waterf_cfac();
iy=bg_waterf_y;
iymax=bg_waterf_y+bg_waterf_lines-bg_waterf_yinc;
hide_mouse(bg.xleft, bg_last_xpixel, bg_waterf_y, iymax);
jw=bg_waterf_ptr2;
k=0;
while(iy < iymax)
  {
  lir_hline(bg.xleft+1,iy,bg_first_xpixel-1,0);
  i=bg_first_xpixel;
  ix=0;
  while(ix<bg_xpixels && i<screen_width)
    {
    color=bg_waterf_cfac*(bg_waterf[jw+ix]-bg_waterf_czer);
    ix++;
    if(color<0)
      {
      color=0;
      }
    else
      {
      if(color >= MAX_COLOR_SCALE)color=MAX_COLOR_SCALE-1;
      }
    lir_setpixel(i,iy,color_scale[color]);
    i++;
    }
  iy++;
  jw+=bg_xpixels;
  if(jw > bg_waterf_size-bg_xpixels-1)jw=0;
  k++;
  if(k>=waterfall_yield_interval)
    {
    k=0;
    if(thread_command_flag[THREAD_SCREEN]==THRFLAG_KILL)return;
    if(thread_command_flag[THREAD_SCREEN]==THRFLAG_IDLE)
      {
      thread_status_flag[THREAD_SCREEN]=THRFLAG_IDLE;
      while(thread_command_flag[THREAD_SCREEN] == THRFLAG_IDLE)
        {
        lir_await_event(EVENT_SCREEN);
        if(thread_command_flag[THREAD_SCREEN]==THRFLAG_KILL)return;
        }
      thread_status_flag[THREAD_SCREEN]=THRFLAG_ACTIVE;
      }  
    else
      {
      lir_sched_yield();
      }
    }
  }  
i=0;
while(bg_waterf_times[i].line < local_bg_yborder-2*text_height)
  {
  lir_pixwrite(bg.xleft+2, bg_waterf_times[i].line,bg_waterf_times[i].text);
  i++;
  }
show_bg_buttons(0);
}

void wg_waterf_line(void)
{
int i,j,ix,color;
if(local_wg_waterf_ptr < 0)return;
hide_mouse(wg_first_xpixel, wg_last_xpixel, wg_waterf_y, wg_waterf_y);
// Write the current line of waterfall to the screen 
if(wg_waterfall_blank_points != 0)
  {
  j=0;
  for(ix=0; ix<wg_xpixels; ix++)
    {
    color=wg_waterf_cfac*(wg_waterf[local_wg_waterf_ptr+ix]-wg_waterf_czer);
    if(color >= MAX_COLOR_SCALE)
      { 
      j++;
      }
    }
  if(j >= wg_waterfall_blank_points) 
    {
    for(ix=0; ix<wg_xpixels; ix++)  
      {
      wg_waterf[local_wg_waterf_ptr+ix]=wg_waterf_czer;
      }
    }
  }
i=wg_first_xpixel;
for(ix=0; ix<wg_xpixels; ix++)
  {
  color=wg_waterf_cfac*(wg_waterf[local_wg_waterf_ptr+ix]-wg_waterf_czer);
  if(color<0)
    {
    color=0;
    }
  else
    {  
    if(color >= MAX_COLOR_SCALE)
      {
      color=MAX_COLOR_SCALE-1;
      }
    }
  lir_setpixel(i,wg_waterf_y,color_scale[color]);
  i++;
  }
wg_waterf_y--;
wg_timestamp_counter++;
if(wg_timestamp_counter>2*text_height)
  {
  i=max_wg_waterf_times-1;
  while(i>0)
    {
    wg_waterf_times[i]=wg_waterf_times[i-1];
    i--;
    }
hide_mouse(wg.xleft, wg.xleft+10*text_width, 
                                      wg_waterf_y, wg_waterf_y+text_height ); 
  if(diskread_flag < 2)
    {
    i=lir_get_epoch_seconds();
    }
  else
    {
    i=diskread_time+diskread_block_counter*snd[RXAD].block_frames/ui.rx_ad_speed;
    }
  i%=24*3600;
  j=i/3600;
  i%=3600;
  sprintf(wg_waterf_times[0].text,"%02d.%02d.%02d",j,i/60,i%60);
  wg_waterf_times[0].line=wg_waterf_y+text_height/2;
  lir_pixwrite(wg.xleft+2, wg_waterf_times[0].line,wg_waterf_times[0].text);
  wg_timestamp_counter=0;
  }
local_wg_waterf_ptr-=wg_xpixels;
if(local_wg_waterf_ptr < 0)local_wg_waterf_ptr+=wg_waterf_size;
if(wg_waterf_y < wg_waterf_y1)
  {
  for(i=0; i<max_wg_waterf_times; i++)wg_waterf_times[i].line+=wg_waterf_yinc;
  wg_waterf_ptr2-=wg_waterf_block;
  if(wg_waterf_ptr2 < 0)wg_waterf_ptr2+=wg_waterf_size;
  wg_waterf_y=wg_waterf_y2;
  redraw_wg_waterfall();
  if(kill_all_flag) return;
  lir_fillbox(wg.xleft+1,wg_waterf_y1,wg_last_xpixel-wg.xleft,
                                    wg_waterf_y2-wg_waterf_y1+1,0);
  }
}

void bg_waterf_line(void)
{
int i, j, ix, color;
hide_mouse(bg_first_xpixel, bg_last_xpixel, bg_waterf_y, bg_waterf_y);
// Write the current line of waterfall to the screen 
if(bg_waterfall_blank_points != 0)
  {
  j=0;
  for(ix=0; ix<bg_xpixels; ix++)
    {
    color=bg_waterf_cfac*(bg_waterf[local_bg_waterf_ptr+ix]-bg_waterf_czer);
    if(color >= MAX_COLOR_SCALE)
      { 
      j++;
      }
    }
  if(j >= bg_waterfall_blank_points) 
    {
    for(ix=0; ix<bg_xpixels; ix++)  
      {
      bg_waterf[local_bg_waterf_ptr+ix]=bg_waterf_czer;
      }
    }
  }
i=bg_first_xpixel;
for(ix=0; ix<bg_xpixels; ix++)
  {
  color=bg_waterf_cfac*(bg_waterf[local_bg_waterf_ptr+ix]-bg_waterf_czer);
  if(color<0)
    {
    color=0;
    }
  else
    {  
    if(color >= MAX_COLOR_SCALE)
      { 
      color=MAX_COLOR_SCALE-1;
      }
    }
  lir_setpixel(i,bg_waterf_y,color_scale[color]);
  i++;
  }
bg_waterf_y--;
bg_timestamp_counter++;
if(bg_timestamp_counter>2*text_height)
  {
  i=max_bg_waterf_times-1;
  while(i>0)
    {
    bg_waterf_times[i]=bg_waterf_times[i-1];
    i--;
    }
hide_mouse(bg.xleft, bg.xleft+10*text_width, 
                                        bg_waterf_y, bg_waterf_y+text_height); 
  if(diskread_flag < 2)
    {
    i=lir_get_epoch_seconds();
    }
  else
    {
    i=diskread_time+diskread_block_counter*snd[RXAD].block_frames/ui.rx_ad_speed;
    }
  i%=24*3600;
  j=i/3600;
  i%=3600;
  sprintf(bg_waterf_times[0].text,"%02d.%02d.%02d",j,i/60,i%60);
//  i%=3600;
//  sprintf(bg_waterf_times[0].text,"%02d.%02d",i/60,i%60);
  bg_waterf_times[0].line=bg_waterf_y+text_height/2;
  lir_pixwrite(bg.xleft+2, bg_waterf_times[0].line,bg_waterf_times[0].text);
  bg_timestamp_counter=0;
  }
local_bg_waterf_ptr-=bg_xpixels;
if(local_bg_waterf_ptr < 0)local_bg_waterf_ptr+=bg_waterf_size;
if(bg_waterf_y < bg_waterf_y1)
  {
  for(i=0; i<max_bg_waterf_times; i++)bg_waterf_times[i].line+=bg_waterf_yinc;
  bg_waterf_ptr2-=bg_waterf_block;
  if(bg_waterf_ptr2 < 0)bg_waterf_ptr2+=bg_waterf_size;
  bg_waterf_y=bg_waterf_y2;
  redraw_bg_waterfall();
  if(kill_all_flag) return;
  lir_fillbox(bg.xleft+1,bg_waterf_y1,bg_last_xpixel-bg.xleft,
                                    bg_waterf_y2-bg_waterf_y1+1,0);
  }
}

void update_hg_spectrum(void)
{
int i, j, k, ia, ib, ja, jb, hgac1, hgac2, hgm1, hgm2;
float t1, t2;
if(mix1_selfreq[0]<=0 || new_baseb_flag < 0)return;
make_hires_valid();
if(show_map65)
  {
  k=hgbutt[HG_MAP65_STRONG].x2;
  }      
else
  {
  k=hgbutt[HG_SPECTRUM_ZERO].x2;
  }
i=0;
hide_mouse(hg_first_xpixel, hg_last_xpixel, hg_ymax, hg_y0);
if(sw_onechan)
  {
  hgm1=hg_spectrum[hg_center];
  if(afc_curx > 0)
    {
    hgac1=hg_spectrum[afc_curx];
    }
  else 
    {
    hgac1=0;
    }
  for(j=hg_first_xpixel; j<hg_last_xpixel; j++)
    {
    t1=hg_yfac_log*log10(hg_fft2_pwrsum[i  ]*hg_yfac_power);
    ia=t1;
    if(ia<0)ia=0;
    ia=hg_y0-ia;
    if(ia<hg_ymax)ia=hg_ymax;
    ja=hg_spectrum[j];
    hg_spectrum[j  ]=ia;
    if(ia != ja)
      {
      if(j > k && j < hgbutt[HG_SPECTRUM_GAIN].x1)
        {
        lir_setpixel(j,ja,hg_background[ja]);
        lir_setpixel(j,ia,15);
        }
      else
        {
        if(ja < hgbutt[HG_SPECTRUM_GAIN].y1)
          {
          lir_setpixel(j,ja,hg_background[ja]);
          }
        if(ia < hgbutt[HG_SPECTRUM_GAIN].y1)
          {
          lir_setpixel(j,ia,15);
          }
        }        
      }
    i++;
    }
  if(hg_center > 0 && hgm1 != hg_spectrum[hg_center])     
    {
    lir_setpixel(hg_center,hgm1,8);
    }
  if(afc_curx > 0 && hgac1 >= hg_cury0 && hgac1 != hg_spectrum[afc_curx])
    {
    lir_setpixel(afc_curx,hgac1,afc_cursor_color);
    }
  }
else
  {
  hgm1=hg_spectrum[2*hg_center];
  hgm2=hg_spectrum[2*hg_center+1];
  if(afc_curx > 0)
    {
    hgac1=hg_spectrum[2*afc_curx];
    hgac2=hg_spectrum[2*afc_curx+1];
    }
  else 
    {
    hgac1=0;
    hgac2=0;
    }
  for(j=hg_first_xpixel; j<hg_last_xpixel; j++)
    {
    t1=hg_yfac_log*log10(hg_fft2_pwrsum[2*i  ]*hg_yfac_power);
    t2=hg_yfac_log*log10(hg_fft2_pwrsum[2*i+1]*hg_yfac_power);  
    ia=t1;
    ib=t2;
    if(ia<0)ia=0;
    if(ib<0)ib=0;  
    ia=hg_y0-ia;
    ib=hg_y0-ib;
    if(ia<hg_ymax)ia=hg_ymax;
    if(ib<hg_ymax)ib=hg_ymax;
    ja=hg_spectrum[2*j];
    jb=hg_spectrum[2*j+1];
    hg_spectrum[2*j  ]=ia;
    hg_spectrum[2*j+1]=ib;
    if(j > k && j < hgbutt[HG_SPECTRUM_GAIN].x1)
      {
      if(ia == ja)
        {
        if(ib != jb)
          {
          lir_setpixel(j,jb,hg_background[jb]);
          if(ia == ib)
            {
            lir_setpixel(j,ib,15);
            }
          else
            {
            lir_setpixel(j,ib,13);
            }  
          if(ja==jb)lir_setpixel(j,ja,10);
          } 
        }
      else
        {
        if(ib == jb)
          {
          lir_setpixel(j,ja,hg_background[ja]);
          if(ia == ib)
            {
            lir_setpixel(j,ia,15);
            }
          else
            {
            lir_setpixel(j,ia,10);
            }  
          if(ja==jb)
            {
            lir_setpixel(j,jb,10);
            }
          else
            {
            lir_setpixel(j,ja,hg_background[ja]);
            }              
          }  
       if(ia == ib)
          {
          lir_setpixel(j,ja,hg_background[ja]);
          lir_setpixel(j,jb,hg_background[jb]);
          lir_setpixel(j,ia,15);
          }       
        else 
          {
          lir_setpixel(j,jb,hg_background[jb]);
          lir_setpixel(j,ib,13);
          lir_setpixel(j,ja,hg_background[ja]);
          lir_setpixel(j,ia,10);
          }
        }  
      i++;
      }
    else
      {
      if(ia == ja)
        {
        if(ib != jb)
          {
          if(jb < hgbutt[HG_SPECTRUM_GAIN].y1)
            {
            lir_setpixel(j,jb,hg_background[jb]);
            }
          if(ib < hgbutt[HG_SPECTRUM_GAIN].y1)
            {
            if(ia == ib)
              {
              lir_setpixel(j,ib,15);
              }
            else
              {
              lir_setpixel(j,ib,13);
              }
            }    
          if(ja==jb)
            {
            if(jb < hgbutt[HG_SPECTRUM_GAIN].y1)
              {
              lir_setpixel(j,ja,10);
              }
            }  
          } 
        }
      else
        {
        if(ib == jb)
          {
          if(ja < hgbutt[HG_SPECTRUM_GAIN].y1)
            {
            lir_setpixel(j,ja,hg_background[ja]);
            }
          if(ia < hgbutt[HG_SPECTRUM_GAIN].y1)
            {
            if(ia == ib)
              {
              lir_setpixel(j,ia,15);
              }
            else
              {
              lir_setpixel(j,ia,10);
              }
            }
          if(ja < hgbutt[HG_SPECTRUM_GAIN].y1)
            {
            if(ja==jb)
              {
              lir_setpixel(j,ja,10);
              }
            else
              {
              lir_setpixel(j,ja,hg_background[ja]);
              }
            }                
          }  
       if(ia == ib)
          {
          if(ja < hgbutt[HG_SPECTRUM_GAIN].y1)
            {
            lir_setpixel(j,ja,hg_background[ja]);
            }
          if(jb < hgbutt[HG_SPECTRUM_GAIN].y1)
            {
            lir_setpixel(j,jb,hg_background[jb]);
            }
          if(ia < hgbutt[HG_SPECTRUM_GAIN].y1)
            {
            lir_setpixel(j,ia,15);
            }
          }       
        else 
          {
          if(jb < hgbutt[HG_SPECTRUM_GAIN].y1)
            {
            lir_setpixel(j,jb,hg_background[jb]);
            }
          if(ib < hgbutt[HG_SPECTRUM_GAIN].y1)
            {
            lir_setpixel(j,ib,13);
            }
          if(ja < hgbutt[HG_SPECTRUM_GAIN].y1)
            {
            lir_setpixel(j,ja,hg_background[ja]);
            }
          if(ia < hgbutt[HG_SPECTRUM_GAIN].y1)
            {
            lir_setpixel(j,ia,10);
            }
          }
        }  
      i++;
      }
    }
// Put the middle line back in case we moved a point

  if(hg_center > 0)
    { 
    if(hgm1!=hg_spectrum[2*hg_center] && hgm1!=hg_spectrum[2*hg_center+1])     
      {
      lir_setpixel(hg_center,hgm1,8);
      }
    if(hgm2!=hg_spectrum[2*hg_center] && hgm2!=hg_spectrum[2*hg_center+1])     
      {
      lir_setpixel(hg_center,hgm2,8);
      }
    }  
  if(afc_curx > 0)
    {
    if(hgac1 >= hg_cury0 && 
       hgac1 != hg_spectrum[2*afc_curx] &&
       hgac1 != hg_spectrum[2*afc_curx+1])
      {
      lir_setpixel(afc_curx,hgac1,afc_cursor_color);
      }
    if(hgac2 >= hg_cury0 && 
       hgac2 != hg_spectrum[2*afc_curx] &&
       hgac2 != hg_spectrum[2*afc_curx+1])
      {
      lir_setpixel(afc_curx,hgac2,afc_cursor_color);
      }
    }
  }
}

void screen_routine(void)
{
char s[80];
int local_workload_counter;
int local_squelch_on;
int col, line;
int local_truncate_flag;
double bg_maxamp_time, amp_info_time;
double sleep_time;
double latest_expose_time;
float t1,t2;
int local_reset;
int i, j;
int screen_overload_count;
#if INTERNAL_GEN_FILTER_TEST == TRUE || INTERNAL_GEN_CARRIER == TRUE
int local_internal_generator_att;
int local_internal_generator_noise;
#endif
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_SCREEN);
#endif
screen_overload_count=0;
local_reset=workload_reset_flag;
local_bg_yborder=bg.yborder;
local_wg_yborder=wg.yborder;
if(genparm[AFC_ENABLE]!=0 && genparm[AFC_LOCK_RANGE] != 0)show_ag_buttons();  
bg_maxamp_time=current_time();
cursor_blink_time=bg_maxamp_time+2;
time_info_time=bg_maxamp_time+.5;
amp_info_time=bg_maxamp_time+.3;
if(!timinfo_flag)time_info_time+=10;
sleep_time=bg_maxamp_time;
#if (USERS_EXTRA_PRESENT == 1)
users_extra_time=bg_maxamp_time;
#endif
for(i=0;i<wg_waterf_size;i++)wg_waterf[i]=0x8000;
if(screen_type & (SCREEN_TYPE_SVGALIB+SCREEN_TYPE_FBDEV))
  {
  waterfall_yield_interval=3;
  if(min_delay_time > 0.002)
    {
    waterfall_yield_interval+=min_delay_time/0.002;
    if(waterfall_yield_interval > 200)waterfall_yield_interval=200;
    }
  }
else
  {
  waterfall_yield_interval=200;
  }
local_internal_generator_noise=internal_generator_noise;
local_internal_generator_att=internal_generator_att;
local_truncate_flag=truncate_flag;
#if INTERNAL_GEN_FILTER_TEST == TRUE || INTERNAL_GEN_CARRIER == TRUE
if(internal_generator_noise != 0)
  {
  local_internal_generator_noise=-1;
  }
if(internal_generator_att != 0)
  {
  local_internal_generator_att=-1;
  }
if(truncate_flag != 0)
  {
  local_truncate_flag=-1;
  }
#endif
s_meter_avg_filled_flag=FALSE;
local_workload_counter=workload_counter;
latest_expose_time = expose_time;
local_squelch_on=-1;
restart:;
thread_status_flag[THREAD_SCREEN]=THRFLAG_ACTIVE;
while(thread_command_flag[THREAD_SCREEN]==THRFLAG_ACTIVE)
  {
// We arrive here at least 10 times per second because that is the rate
// at which we are posted by the A/D input routine.
// Time critical things that set the semaphore to get immediate
// response have their own sem_wait() statements by a jump to
// fast_screen_end to assure that the semaphore is decremented.
// This routine may have very low priority and may fail to
// execute now and then.
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  if(sd[SC_WG_WATERF_INIT] != sc[SC_WG_WATERF_INIT])
    {
    sd[SC_WG_WATERF_INIT]=sc[SC_WG_WATERF_INIT];
    local_wg_waterf_ptr=0;
    wg_waterf_y=wg_waterf_y2;
    for(i=0;i<wg_waterf_size;i++)wg_waterf[i]=0x8000;
    wg_waterf_ptr2=0;
    local_wg_yborder=wg.yborder;
    }
  if(local_wg_waterf_ptr != wg_waterf_ptr)
    {
    while(local_wg_waterf_ptr != wg_waterf_ptr)
      {
      wg_waterf_line();
      }
    if(kill_all_flag) goto screen_exit;
    if(genparm[ MAX_NO_OF_SPURS] != 0)update_spur_info();
    }
  if(sd[SC_BG_WATERF_INIT] != sc[SC_BG_WATERF_INIT])
    {
    sd[SC_BG_WATERF_INIT]=sc[SC_BG_WATERF_INIT];
    local_bg_waterf_ptr=0;
    bg_waterf_y=bg_waterf_y2;
    for(i=0;i<bg_waterf_size;i++)bg_waterf[i]=0x8000;
    bg_waterf_ptr2=0;
    local_bg_yborder=bg.yborder;
    }
  if(local_bg_waterf_ptr != bg_waterf_ptr)
    {
    while(local_bg_waterf_ptr != bg_waterf_ptr)
      {
      bg_waterf_line();
      }
    if(kill_all_flag) goto screen_exit;
    }
  if(sd[SC_SHOW_FFT1]!=sc[SC_SHOW_FFT1])
    {
    sd[SC_SHOW_FFT1]=sc[SC_SHOW_FFT1];
    lir_mutex_lock(MUTEX_FFT1SLOWSUM);
    update_wg_spectrum();
    lir_mutex_unlock(MUTEX_FFT1SLOWSUM);
    if(kill_all_flag) goto screen_exit;
    }  
  if(sd[SC_SHOW_FFT2]!=sc[SC_SHOW_FFT2]&& 
     sd[SC_HG_Y_SCALE]==sc[SC_HG_Y_SCALE])
    {
    sd[SC_SHOW_FFT2]=sc[SC_SHOW_FFT2]; 
    update_hg_spectrum();
    if(sd[SC_SHOW_MAP65]!=sc[SC_SHOW_MAP65]) 
      {
      sd[SC_SHOW_MAP65]=sc[SC_SHOW_MAP65]; 
      show_map65_buttons();
      }
    if(sd[SC_SHOW_SELLIM]!=sc[SC_SHOW_SELLIM]) 
      {
      sd[SC_SHOW_SELLIM]=sc[SC_SHOW_SELLIM]; 
      show_sellim_buttons();
      }
    if(kill_all_flag) goto screen_exit;
    }
  if(sd[SC_SHOW_FFT3]!=sc[SC_SHOW_FFT3])
    {
    sd[SC_SHOW_FFT3]=sc[SC_SHOW_FFT3];
    show_baseband_spectrum();
    if(kill_all_flag) goto screen_exit;
    }
   if(no_of_processors < 2 && 
      fabs(sleep_time - recent_time) < 0.1) goto fast_screen_end;
// Less critical things. There may be a 0.1 second delay before they
// are actually written to the screen.
  if(local_workload_counter != workload_counter)
    {
    local_workload_counter=workload_counter;
    if(local_reset != workload_reset_flag)
      {
      local_reset--;
      settextcolor(15);
      }
    if(workload > 0)
      {
      sprintf(s,"%2.2f%% ",workload);
      lir_text(0,screen_last_line,s);
      }
    else
      {
      lir_text(0,screen_last_line,"Unkn%");
      }
    line=screen_last_line;
    col=30;
    t1=0;
    for(i=0; i<THREAD_MAX; i++)
      {
      if(thread_command_flag[i]!=THRFLAG_NOT_ACTIVE)
        {
        t2=thread_workload[i];
        if(t2<0)t2=0;
        if(t2>99.99)t2=99.99;
        if(t1 < t2)t1=t2;
        if(timinfo_flag)
          { 
          sprintf(s,"%s:%2.2f  ",thread_names[i],t2);
          if(col+11 > screen_last_col)
            {
            col=30;
            line --;
            }
          lir_text(col,line,s);
          col+=11;
          }
        }
      }  
    if(no_of_processors > 1)
      {
      sprintf(s,"(%2.0f%%)",t1);
      lir_text(7,screen_last_line,s);
      }
    if(local_reset != workload_reset_flag)
      {
      local_reset=workload_reset_flag;
      settextcolor(7);
      }
    }  
  if( new_mouse_x!=mouse_x || new_mouse_y!=mouse_y)
    {
    lir_move_mouse_cursor();
    if(kill_all_flag) goto screen_exit;
    }
  if(sd[SC_SHOW_KEYING_SPECTRUM]!=sc[SC_SHOW_KEYING_SPECTRUM])
    {
    sd[SC_SHOW_KEYING_SPECTRUM]=sc[SC_SHOW_KEYING_SPECTRUM];
    show_keying_spectrum();
    if(kill_all_flag) goto screen_exit;
    }
  if(sd[SC_WG_WATERF_REDRAW]!=sc[SC_WG_WATERF_REDRAW])
    {
    sd[SC_WG_WATERF_REDRAW]=sc[SC_WG_WATERF_REDRAW];
    redraw_wg_waterfall();
    if(kill_all_flag) goto screen_exit;
    if(genparm[SECOND_FFT_ENABLE] != 0)
      {
      hg_ston1_yold=-1;
      hg_ston2_yold=-1;
      make_hg_yscale();
      if(kill_all_flag) goto screen_exit;
      hires_fq_scale();
      if(kill_all_flag) goto screen_exit;
      }
    }
  if(sd[SC_HG_STONBARS_REDRAW]!=sc[SC_HG_STONBARS_REDRAW])
    {
    sd[SC_HG_STONBARS_REDRAW]=sc[SC_HG_STONBARS_REDRAW];
    hg_stonbars_redraw();
    if(kill_all_flag) goto screen_exit;
    }    
  if(sd[SC_HG_FQ_SCALE]!=sc[SC_HG_FQ_SCALE])
    {
    sd[SC_HG_FQ_SCALE]=sc[SC_HG_FQ_SCALE];
    if(genparm[SECOND_FFT_ENABLE] != 0)
      {
      hires_fq_scale();
      if(kill_all_flag) goto screen_exit;
      }
    }   
  if(sd[SC_BG_FQ_SCALE]!=sc[SC_BG_FQ_SCALE])
    {
    sd[SC_BG_FQ_SCALE]=sc[SC_BG_FQ_SCALE];
    baseband_fq_scale();
    if(kill_all_flag) goto screen_exit;
    }   
  if(sd[SC_WG_FQ_SCALE]!=sc[SC_WG_FQ_SCALE])
    {
    sd[SC_WG_FQ_SCALE]=sc[SC_WG_FQ_SCALE];
    wide_fq_scale();
    if(kill_all_flag) goto screen_exit;
    } 
  if(sd[SC_HG_Y_SCALE]!=sc[SC_HG_Y_SCALE])
    {
    sd[SC_HG_Y_SCALE]=sc[SC_HG_Y_SCALE];
    if(genparm[SECOND_FFT_ENABLE] != 0)
      {
      make_hg_yscale();
      if(kill_all_flag) goto screen_exit;
      }
    }   
  if(sd[SC_BG_WATERF_REDRAW]!=sc[SC_BG_WATERF_REDRAW])
    {
    sd[SC_BG_WATERF_REDRAW]=sc[SC_BG_WATERF_REDRAW];
    redraw_bg_waterfall();
    if(kill_all_flag) goto screen_exit;
    } 
  if(sd[SC_CG_REDRAW]!=sc[SC_CG_REDRAW])
    {
    sd[SC_CG_REDRAW]++;
    update_cg_traces();
    if(kill_all_flag) goto screen_exit;
    }
  if(sd[SC_WG_BUTTONS]!=sc[SC_WG_BUTTONS])
    {
    sd[SC_WG_BUTTONS]=sc[SC_WG_BUTTONS];
    show_wg_buttons(1);
    if(kill_all_flag) goto screen_exit;
    }
  if(sd[SC_BG_BUTTONS]!=sc[SC_BG_BUTTONS])
    {
    sd[SC_BG_BUTTONS]=sc[SC_BG_BUTTONS];
    show_bg_buttons(1);
    if(kill_all_flag) goto screen_exit;
    }
  if(sd[SC_SG_BUTTONS]!=sc[SC_SG_BUTTONS])
    {
    sd[SC_SG_BUTTONS]=sc[SC_SG_BUTTONS];
    show_sg_buttons();
    if(kill_all_flag) goto screen_exit;
    }
  if(sd[SC_FILL_AFC]!=sc[SC_FILL_AFC])
    {
    sd[SC_FILL_AFC]=sc[SC_FILL_AFC];
    fill_afc_graph();
    if(kill_all_flag) goto screen_exit;
    }
  if(sd[SC_SHOW_AFC]!=sc[SC_SHOW_AFC])
    {
    sd[SC_SHOW_AFC]=sc[SC_SHOW_AFC];
    if(sd[SC_FILL_AFC]==sc[SC_FILL_AFC])
      {
      show_afc();
      if(kill_all_flag) goto screen_exit;
      }
    }
  if(sd[SC_AFC_CURSOR]!=sc[SC_AFC_CURSOR])
    {
    sd[SC_AFC_CURSOR]=sc[SC_AFC_CURSOR];
    afc_cursor();
    if(kill_all_flag) goto screen_exit;
    }
  if(sd[SC_SHOW_POL]!=sc[SC_SHOW_POL])
    {
    sd[SC_SHOW_POL]=sc[SC_SHOW_POL];
    show_pol();
    if(kill_all_flag) goto screen_exit;
    }
  if(sd[SC_FREQ_READOUT]!=sc[SC_FREQ_READOUT])
    {
    sd[SC_FREQ_READOUT]=sc[SC_FREQ_READOUT];
    frequency_readout();
    if(kill_all_flag) goto screen_exit;
    }
  if(sd[SC_SHOW_CENTER_FQ]!=sc[SC_SHOW_CENTER_FQ])
    {
    sd[SC_SHOW_CENTER_FQ]=sc[SC_SHOW_CENTER_FQ];
    show_center_frequency();
    if(kill_all_flag) goto screen_exit;
    }
  if(fg.passband_center != old_passband_center)
    {
    show_center_frequency();
    sd[SC_SHOW_CENTER_FQ]=sc[SC_SHOW_CENTER_FQ];
    old_passband_center=fg.passband_center;
    }
// Show baseband max amplitude in the baseband graph
  current_time();
  if(recent_time-bg_maxamp_time > 0.05)
    {
    bg_maxamp_time=recent_time;
    if( bg_maxamp != 0)show_bg_maxamp();
    }
#if (USERS_EXTRA_PRESENT == 1)
  if(fabs(users_extra_time-recent_time) > users_extra_update_interval)
    {
    users_extra();
    users_extra_time=current_time();
    }
#endif
  if(rx_mode != MODE_TXTEST && rx_mode != MODE_RADAR)
    {
    if(ampinfo_flag)
      {
      if(recent_time > amp_info_time)
        {
        show_amp_info();
        amp_info_time=recent_time+.3;
        }
      }  
    if(recent_time > time_info_time)
      {
      lir_sched_yield();
      show_timing_info();
      time_info_time=recent_time+.2;
      }
    }  
  else
    {
    if(recent_time > amp_info_time)
      {
      show_adsat();
      amp_info_time=recent_time+.2;
      }
    }  
  if(new_baseb_flag >= 0)
    {
    if(bg.oscill_on != 0)
      {
      if( ((timf3_pa-timf3_ps+timf3_mask) & timf3_mask) > 
                                             timf3_oscilloscope_limit)
        {
        timf3_oscilloscope();
        if(kill_all_flag) goto screen_exit;
        }
      }
    }
  if(sd[SC_TIMF2_OSCILLOSCOPE]!=sc[SC_TIMF2_OSCILLOSCOPE])
    {
    sd[SC_TIMF2_OSCILLOSCOPE]=sc[SC_TIMF2_OSCILLOSCOPE];
    show_timf2();
    if(kill_all_flag) goto screen_exit;
    }
  if(sd[SC_BLANKER_INFO]!=sc[SC_BLANKER_INFO])
    {
    sd[SC_BLANKER_INFO]=sc[SC_BLANKER_INFO];
    show_blanker_info();
    if(kill_all_flag) goto screen_exit;
    }
  if(sd[SC_SHOW_COHERENT]!=sc[SC_SHOW_COHERENT])
    {
    sd[SC_SHOW_COHERENT]=sc[SC_SHOW_COHERENT];
    show_coherent();
    if(kill_all_flag) goto screen_exit;
    }
  if(fft1_correlation_flag <= 1)
    {
    if(sd[SC_UPDATE_METER_GRAPH]!=sc[SC_UPDATE_METER_GRAPH])
      {
      sd[SC_UPDATE_METER_GRAPH]=sc[SC_UPDATE_METER_GRAPH];
      update_meter_graph();
      if(kill_all_flag) goto screen_exit;
      }
    if(sd[SC_MG_REDRAW]!=sc[SC_MG_REDRAW])
      {
      sd[SC_MG_REDRAW]=sc[SC_MG_REDRAW];
      redraw_meter_graph();
      }
    }
  if(sd[SC_COMPUTE_EME_DATA]!=sc[SC_COMPUTE_EME_DATA])
    {
    sd[SC_COMPUTE_EME_DATA]=sc[SC_COMPUTE_EME_DATA];
    calculate_moon_data();
    }
  if(sd[SC_SHOW_TX_FQ]!=sc[SC_SHOW_TX_FQ])
    {
    sd[SC_SHOW_TX_FQ]=sc[SC_SHOW_TX_FQ];
    show_tx_frequency();
    }
  if(sd[SC_SHOW_WHEEL]!=sc[SC_SHOW_WHEEL])
    {
    sd[SC_SHOW_WHEEL]=sc[SC_SHOW_WHEEL];
    if(bg.yborder-bg.ytop > 5.5*text_height + 4)
      {
      show_wheel_stepmult();
      }
    }
  if(sd[SC_RG_REDRAW]!=sc[SC_RG_REDRAW])
    {
    sd[SC_RG_REDRAW]=sc[SC_RG_REDRAW];
    redraw_radar_graph();
    }
  if(sd[SC_SG_REDRAW]!=sc[SC_SG_REDRAW])
    {
    sd[SC_SG_REDRAW]=sc[SC_SG_REDRAW];
    redraw_siganal_graph();
    }
  if(sd[SC_VG_REDRAW]!=sc[SC_VG_REDRAW])
    {
    sd[SC_VG_REDRAW]=sc[SC_VG_REDRAW];
    redraw_allan_graph();
    }
  if(sd[SC_VGF_REDRAW]!=sc[SC_VGF_REDRAW])
    {
    sd[SC_VGF_REDRAW]=sc[SC_VGF_REDRAW];
    redraw_allanfreq_graph();
    }
  if(sd[SC_METER_MOUSE]!=sc[SC_METER_MOUSE])
    {
    sd[SC_METER_MOUSE]=sc[SC_METER_MOUSE];
    mg_compute_stonavg();
    }
  if(sd[SC_SG_UPDATE]!=sc[SC_SG_UPDATE])
    {
    sd[SC_SG_UPDATE]=sc[SC_SG_UPDATE];
    update_siganal_graph();
    }
  if(sd[SC_VG_UPDATE]!=sc[SC_VG_UPDATE])
    {
    sd[SC_VG_UPDATE]=sc[SC_VG_UPDATE];
    update_allan_graph();
    }
  if(sd[SC_VGF_UPDATE]!=sc[SC_VGF_UPDATE])
    {
    sd[SC_VGF_UPDATE]=sc[SC_VGF_UPDATE];
    update_allanfreq_graph();
    }
  if(fft1_correlation_flag == 0)
    {
    if(local_squelch_on != squelch_on)
      {
      local_squelch_on=squelch_on;
      if(usercontrol_mode == USR_NORMAL_RX)
        {
        if(local_squelch_on==0)
          {
          lir_pixwrite(bg.xleft+text_width,bg.ybottom-4*text_height,"OFF");
          }
        else
          {
          lir_pixwrite(bg.xleft+text_width,bg.ybottom-4*text_height,"ON ");
          }
        }  
      }
    }
#if INTERNAL_GEN_FILTER_TEST == TRUE || INTERNAL_GEN_CARRIER == TRUE
  if(internal_generator_flag != 0)
    {
    if(local_internal_generator_att !=  internal_generator_att)
      {
      if(internal_generator_att == 0)
        {
        for(i=0; i<20; i++)s[i]=' ';
        s[20]=0;
        }
      else
        {  
        local_internal_generator_att=internal_generator_att;
        }
      sprintf(s,"ATTENUATION %d dB",-10*internal_generator_att);
      lir_text(50,0,s);
      }
    } 
#endif
  if(local_internal_generator_noise !=  internal_generator_noise)
    {
    local_internal_generator_noise=internal_generator_noise;
    if(internal_generator_noise == 0)
      {
      for(i=0; i<21; i++)s[i]=' ';
      s[21]=0;
      }
    else
      {  
      sprintf(s,"NOISE LEVEL %.1f bits",0.5*(float)(internal_generator_noise+1));
      }
    lir_text(30,0,s);
    } 
  if(local_truncate_flag != truncate_flag)
    {
    local_truncate_flag=truncate_flag;
    if(truncate_flag == 0)
      {
      for(i=0; i<30; i++)s[i]=' ';
      s[30]=0;
      }
    else
      {
      lir_sleep(2000);
      sprintf(s,"INPUT TRUNCATED TO %d bits",rxin_nbits);
      }
    lir_text(0,0,s);
    }
  if( recent_time-cursor_blink_time > 0.3)
    {
    if( recent_time-cursor_blink_time > 0.6)
      {
      cursor_blink_time=current_time();
      update_indicator(12);
      }
    else
      {
      update_indicator(22);
      }
    }
  fast_screen_end:;
  if(numinput_flag == 0 && baseb_control_flag ==0)show_mouse();
  if(screen_type&(SCREEN_TYPE_X11+SCREEN_TYPE_X11_SHM+SCREEN_TYPE_WINDOWS))
    {
    if(expose_time != latest_expose_time)
      {
      if(recent_time-expose_time > 0.1 ||
         recent_time-latest_expose_time > 0.3 )
        {
        latest_expose_time = expose_time;
        lir_refresh_entire_screen();
        }
      }
    }  
  if(fabs(sleep_time - recent_time) > 0.5)
    {
    j=0;
    for(i=0; i<MAX_SC; i++)
      {
      j+=sc[i]-sd[i];
      }
    if(j > 200)
      {
      screen_overload_count++;
      sprintf(s,"SCREEN OVERLOAD %d",screen_overload_count);
      wg_error(s,WGERR_SCREEN);
      }
    if(j == 0)screen_overload_count=0;
// Make sure that we leave at least 5% of the CPU time for
// other threads even if the semaphore is posted
// continously.
// This takes care of various problems on exit from this
// thread because it might otherwise never get the
// correct status of kill_all_flag etc in case the semaphore
// has been heavily overloaded. 
// The time we sleep here will normally just cause less
// waiting time on the semaphore next time.
    if(no_of_processors < 2)lir_sleep(25000);  
    if(kill_all_flag) goto screen_exit;
    sleep_time=recent_time;
// Just in case. Make sure the mouse moves.
    if( new_mouse_x!=mouse_x || new_mouse_y!=mouse_y)
      {
      lir_move_mouse_cursor();
      if(kill_all_flag) goto screen_exit;
      }
    }
  thread_status_flag[THREAD_SCREEN]=THRFLAG_SEM_WAIT;
  lir_await_event(EVENT_SCREEN);
  if(kill_all_flag) goto screen_exit;
  thread_status_flag[THREAD_SCREEN]=THRFLAG_ACTIVE;
  lir_refresh_screen();
  }
if(thread_command_flag[THREAD_SCREEN]==THRFLAG_IDLE)
  {
  thread_status_flag[THREAD_SCREEN]=THRFLAG_IDLE;
  while(thread_command_flag[THREAD_SCREEN] == THRFLAG_IDLE)
    {
    lir_sleep(3000);
    }
  if(kill_all_flag) goto screen_exit;
  if(thread_command_flag[THREAD_SCREEN] == THRFLAG_ACTIVE)goto restart;
  }  
screen_exit:;
thread_status_flag[THREAD_SCREEN]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_SCREEN] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
thread_status_flag[THREAD_SCREEN]=THRFLAG_NOT_ACTIVE;
}


