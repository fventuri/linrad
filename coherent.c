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
#include "fft3def.h"
#include "options.h"
#include "thrdef.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

void show_cw(char *s);
void check_cw(int num,int type);
void make_coherent_graph(int clear_old);
void manage_meter_graph(void);
int coherent_graph_scro;

extern int meter_graph_scro;

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
// short int[1] baseb_ramp=indicator for time of power above/below threshold.
// short_int[1] baseb_clock=CW code clock
// float[2] baseb_tmp=for debug during development

// baseb_pa  baseb exists up to baseb_pa-1.
// baseb_pb  The point up to which thresholds exist.
// baseb_pc  The point up to which ramp is collected.
// baseb_pd  A key up region close before baseb_pc
// baseb_pe  The point up to which we have run first detect.
// baseb_pf
// baseb_px  The oldest point that contains valid data.

#define COH_GRAPH_ACTIVE -1000
#define X_SIZE 5

float evaluate_keying_spectrum(void)
{
int ia, ib, ic, kk, nn;
float t1, t2;
// keying_spectrum contains the modulation of the Morse-coded signal
// computed in mix2.c as the power spectrum of the real part of
// baseb_wb.
// One of the strong components should be the frequency of
// a string of Morse code dots, the "clock frequency" of the
// Morse code.
// The user has to set the baseband filter to allow this frequency
// within the baseband filter but the filter must not be set
// more than 2.5 times wider than optimum because we do not search
// for the "Morse clock" lower than this.
// keying_spectrum is computed over twice the bandwidth compared to
// the bandwidth of the user selected filter.
// The full baseband bandwidth, bw, corresponds to half the
// keying_spectrum width. (keying_spectrum_size/2)
// Start at bw/3 and step upwards until a minimum is reached.
// Look for signal power in pairs of points.
ia=3+keying_spectrum_size/6;
t1=keying_spectrum[ia]+keying_spectrum[ia-1];
t2=keying_spectrum[ia]+keying_spectrum[ia+1];
while(t2<t1 && ia<keying_spectrum_size/2)
  {
  t1=t2;
  ia++;
  t2=keying_spectrum[ia]+keying_spectrum[ia+1];
  }
// Now we should have stepped away from the peak associated
// with the "Morse clock" divided by three.
// Look for the strongest peak above ia.
ib=ia;
t1=keying_spectrum[ib];
kk=ib;
while(ib<keying_spectrum_size/2)
  {
  if(t1<keying_spectrum[ib])
    {
    t1=keying_spectrum[ib];
    kk=ib;
    }
  ib++;
  }
// kk may be the Morse code clock, but it could also be
// half the frequency. Search 1.5 to 2.5 times kk for another
// maximum. If it has at least 50% of the power, attribute the
// higher peak to the Morse clock.
ib=1.5*kk;
ic=2.5*kk+1;
if(ic > keying_spectrum_size/2)ic=keying_spectrum_size/2;
nn=0;
t2=0;
while(ib < ic)
  {
  if(t2<keying_spectrum[ib])
    {
    t2=keying_spectrum[ib];
    nn=ib;
    }
  ib++;  
  }
if(2*t2 > t1)kk=nn;
// Get the peak posuition with some decimals. Not very accurate,
// but the first decimal should be close to correct.
if(kk < keying_spectrum_size-1)
  {
  parabolic_fit(&t2, &t1, sqrt(keying_spectrum[kk-1]),
                           sqrt(keying_spectrum[kk]),sqrt(keying_spectrum[kk+1]));
  t1+=kk;                           
  }
else 
  {
  t1=kk;
  t2=sqrt(keying_spectrum[kk]);
  } 
return t1;
}

void collect_ramp(void)
{
int ia, ib;
// Make a first decision key up/key down based on signal power.
// This is the conventional approach, a matched filter (the operator
// has to select the optimum bandwidth) followed by a squarelaw detector.
// The threshold is conventionally kept at a fixed level but here it is
// very close to the noise when no signal is present. while it is at
// half the peak amplitude for strong signals.
// Store the result in baseb_ramp as ramps.
// Positive values indicate key down while negative values
// indicate key up (or signal too weak)
ib=(baseb_pc+baseband_mask)&baseband_mask;
ia=baseb_pc;
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
baseb_pc=ib;
// Step baseb_pc backwards until a long key up region is found. 
ib=2.5*cwbit_pts;
while(abs(baseb_ramp[baseb_pc]) < ib || baseb_ramp[baseb_pc] > 0) 
  {
  baseb_pc=(baseb_pc-abs(baseb_ramp[baseb_pc])+baseband_size)&baseband_mask;
  if( ((baseb_pc-baseb_pe+baseband_size)&baseband_mask)>baseband_neg)
    {
    baseb_pc=baseb_pe;
    }
  }
// Now we know that baseb_pc points to the start of a long negative ramp.
}


void make_ideal_waveform(void)
{
unsigned int i, j, k, m, ia;
float t1;
// Set the size we use for averaging the waveform around dashes.
// Make it an even number.
cw_avg_points=15*cwbit_pts+1;
cw_avg_points&=0xfffffffe;
if(cw_avg_points >= (unsigned)2*fftn_tmp_size)
  {
  lirerr(853021);
  return;
  }
// Assuming the modulation spectrum has been correctly evaluated
// the length of a Morse code bit should be about cwbit_pts points.
// Construct a dash of the appropriate lengths, make it symmetric
// around mix2.size/2 and run it through the baseband filter.
// Store the result as the first guess for our CW waveforms.
for(i=0; i<2*mix2.size; i++) mix2_tmp[i]=0;
j=mix2.size;
t1=cwbit_pts*3.0;
k=t1;
m=k/2;
k=2*m;
mix2_tmp[j]=1; 
for(i=0; i<m; i++)
  {
  mix2_tmp[j+2*i]=1;
  mix2_tmp[j-2-2*i]=1;
  }
t1=0.5*(t1-k);
// t1 is the amount by which the dash is longer than the
// amount we set to 1 (on both sides).
// With full level for a fractional (=t1) sample period,
// the power within the interval is t1. 
// The amplitude we should set is therefore sqrt(t1)
mix2_tmp[j+2*m]=sqrt(t1);
mix2_tmp[j-2-2*m]=sqrt(t1);
fftforward(mix2.size, mix2.n, mix2_tmp, 
                                mix2.table, mix2.permute, yieldflag_ndsp_mix2);
i=1;
j=fft3_size/2;
while(bg_filterfunc[j] != 0)
  {
  mix2_tmp[2*i  ]*=bg_filterfunc[j];    
  mix2_tmp[2*i+1]*=bg_filterfunc[j];    
  mix2_tmp[2*(mix2.size-i)  ]*=bg_filterfunc[j];    
  mix2_tmp[2*(mix2.size-i)+1]*=bg_filterfunc[j];    
  i++;
  j++;
  }
while(i <= mix2.size/2)
  {
  mix2_tmp[2*i  ]=0;    
  mix2_tmp[2*i+1]=0;    
  mix2_tmp[2*(mix2.size-i)  ]=0;    
  mix2_tmp[2*(mix2.size-i)+1]=0;    
  i++;
  }
fftback(mix2.size, mix2.n, mix2_tmp, 
                               mix2.table, mix2.permute, yieldflag_ndsp_mix2);
ia=mix2.size-cw_avg_points;
for(j=0; j<cw_avg_points; j++)
  {
  fftn_tmp[2*j  ]=mix2_tmp[ia];
  fftn_tmp[2*j+1]=0;
  ia+=2;
  }      
store_symmetry_adapted_dash(TRUE);
}

void coherent_cw_detect(void)
{
int i, j;
float t1, t2, t3;
switch (cw_detect_flag)
  {
  case CWDETECT_CLEARED:
  if(keying_spectrum_cnt < keying_spectrum_max)return;
  keying_spectrum_cnt=0;
// Initiate a new search for Morse code.
// We will glook for the "Morse code clock" in keying_spectrum.
  keying_spectrum_pos[0]=evaluate_keying_spectrum();
  keying_spectrum_ampl[0]=keying_spectrum[0];
  cwbit_pts=0.5*mix2.size/keying_spectrum_pos[0];
  collect_ramp();
  return;//öööööööööö
  make_ideal_waveform();
  keying_spectrum_ptr=1;
  detect_cw_speed();
  if(cw_detect_flag == CWDETECT_ERROR)
    {
    cw_detect_flag=CWDETECT_SEARCH_SPEED;
    }
  break;

  case CWDETECT_SEARCH_SPEED:
  if(keying_spectrum_cnt < keying_spectrum_max)return;
  keying_spectrum_cnt=0;
// Collect more data from keying_spectrum.
  keying_spectrum_pos[keying_spectrum_ptr]=evaluate_keying_spectrum();
  keying_spectrum_ampl[keying_spectrum_ptr]=keying_spectrum[0];
  cwbit_pts=0.5*mix2.size/keying_spectrum_pos[keying_spectrum_ptr];
  collect_ramp();
  make_ideal_waveform();
  detect_cw_speed();
  if(cw_detect_flag == CWDETECT_ERROR)
    {
    cw_detect_flag=CWDETECT_SEARCH_SPEED;
    keying_spectrum_ptr++;
// When KEYING_SPECTRUM_MAX spectra are evaluated, skip initial
// ones until the carrier power of the first one is above
// one third of the average carrier power.
    if(keying_spectrum_ptr >= KEYING_SPECTRUM_MAX)
      {
      t1=keying_spectrum_ampl[0];
      for(i=1; i<KEYING_SPECTRUM_MAX; i++)
        {
        t1+=keying_spectrum_ampl[i];
        }
      t1/=3*KEYING_SPECTRUM_MAX;
      i=0;
      while(keying_spectrum_ampl[i] < t1)i++;
      if(i != 0)
        {
        j=0;
        while(i<KEYING_SPECTRUM_MAX)
          {
          keying_spectrum_ampl[j]=keying_spectrum_ampl[i];
          keying_spectrum_pos[j]=keying_spectrum_pos[i];
          i++;
          j++;
          }
        keying_spectrum_ptr=j;
        }
      else
        {
// We have KEYING_SPECTRUM_MAX peaks in keying_spectrum.
// Check if they are close together.
        t1=0;
        t2=0;
        for(i=0; i<KEYING_SPECTRUM_MAX; i++)
          {
          t1+=keying_spectrum_ampl[i];
          t2+=keying_spectrum_ampl[i]*keying_spectrum_pos[i];
          }
        t3=t2/t1;
        t1=0;
        t2=0;
        for(i=0; i<KEYING_SPECTRUM_MAX; i++)
          {
          if(fabs(t3-keying_spectrum_pos[i]) < 0.1*keying_spectrum_size)
            {
            t1+=keying_spectrum_ampl[i];
            t2+=keying_spectrum_ampl[i]*keying_spectrum_pos[i];
            }
          }
        if(t1 == 0)goto fail;
        t2/=t1;
        if(fabs(t3-t2) > 0.1*keying_spectrum_size)goto fail;
// The most probable frequency for the fundamental of the
// "Morse code clock" seems to be t2.
// Convert from frequency to time (samples)
        cwbit_pts=0.5*mix2.size/t2;
        make_ideal_waveform();
        detect_cw_speed();
        if(cw_detect_flag == CWDETECT_ERROR)goto fail;
        }
      }
    }
  break;

  case CWDETECT_WAVEFORM_ESTABLISHED:
  collect_ramp();
  first_find_parts();
  break;

  case CWDETECT_SOME_PARTS_FITTED:
  collect_ramp();
  second_find_parts();
  break;

  case CWDETECT_LIMITS_FOUND:
  collect_ramp();
  init_cw_decode_region();
  break;

  case CWDETECT_REGION_INITIATED:
  collect_ramp();
  cw_decode_region();
  break;

  case CWDETECT_REGION_WAVEFORM_OK:
  collect_ramp();
  init_cw_decode();
  break;

  case CWDETECT_SOME_ASCII_FITTED:
  collect_ramp();
  cw_decode();
  break;

  case CWDETECT_DEBUG_STOP:
  lir_fillbox(screen_width/2,0,50,screen_height-1,3);
  DEB"\nno more!!");
  cw_detect_flag=CWDETECT_DEBUG_IDLE;
  break;

  case CWDETECT_DEBUG_IDLE:
  break;
  
  case CWDETECT_ERROR:
// Detect failed. Remove old data so buffers will not overflow.
fail:;
  cw_detect_flag=CWDETECT_CLEARED;
// Do not store more than 2 minutes of bad data
// and make sure we leave some space in the buffer.  
  i=(baseb_pa-baseb_px+baseband_size)&baseband_mask;
  j=i;
  if(i>baseband_neg)i=baseband_neg;
  if(i>120*baseband_sampling_speed)i=120*baseband_sampling_speed;
  if(i != j)
    {
    baseb_px=(baseb_pa-i+baseband_size)&baseband_mask;
    }
  no_of_cwdat=0;
  baseb_pf=baseb_px;
  baseb_pe=baseb_px;
  baseb_pd=baseb_px;
  baseb_pc=baseb_px;
XZ("\n\nERROR  error  fail!! ");
  break;
  }
}

void check_cg_borders(void)
{
int xsiz, ysiz;
xsiz=3*cg_size+2;
ysiz=xsiz+5*text_height+4;
current_graph_minh=ysiz;
current_graph_minw=xsiz;
check_graph_placement((void*)(&cg));
set_graph_minwidth((void*)(&cg));
}


void help_on_coherent_graph(void)
{
int msg_no;
int event_no;
msg_no=68;
for(event_no=0; event_no<MAX_CGBUTT; event_no++)
  {
  if( cgbutt[event_no].x1 <= mouse_x &&
      cgbutt[event_no].x2 >= mouse_x &&
      cgbutt[event_no].y1 <= mouse_y &&
      cgbutt[event_no].y2 >= mouse_y)
    {
    switch (event_no)
      {
      case CG_TOP:
      case CG_BOTTOM:
      case CG_LEFT:
      case CG_RIGHT:
      msg_no=101;
      break;

      case CG_OSCILLOSCOPE: 
      msg_no=69;
      break;

      case CG_METER_GRAPH:
      msg_no=99;
      break;      
      }
    }
  }
help_message(msg_no);
}




void mouse_continue_coh_graph(void)
{
int i, j;
switch (mouse_active_flag-1)
  {
  case CG_TOP:
  if(cg.ytop!=mouse_y)goto cgm;
  break;

  case CG_BOTTOM:
  if(cg.ybottom!=mouse_y)goto cgm;
  break;

  case CG_LEFT:
  if(cg.xleft!=mouse_x)goto cgm;
  break;

  case CG_RIGHT:
  if(cg.xright==mouse_x)break;
cgm:;
  pause_screen_and_hide_mouse();
  graph_borders((void*)&cg,0);
  if(cg_oldx==COH_GRAPH_ACTIVE)
    {
    cg_oldx=mouse_x;
    cg_oldy=mouse_y;
    }
  else
    {
    i=mouse_x-cg_oldx;
    j=mouse_y-cg_oldy;
    cg_oldx=mouse_x;
    cg_oldy=mouse_y;
    cg.ytop+=j;
    cg.ybottom+=j;
    cg.xleft+=i;
    cg.xright+=i;
    check_cg_borders();
    }
  graph_borders((void*)&cg,15);
  resume_thread(THREAD_SCREEN);
  break;
    
  default:
  goto await_release;
  }
if(leftpressed == BUTTON_RELEASED)goto finish;
return;
await_release:;
if(leftpressed != BUTTON_RELEASED)return;
switch (mouse_active_flag-1)
  {
  case CG_OSCILLOSCOPE:
  if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
    { 
    pause_thread(THREAD_SCREEN);
    hide_mouse(0,screen_width>>1,0,screen_height);
    clear_cg_traces();
    resume_thread(THREAD_SCREEN);
    cg.oscill_on^=1;
    cg.oscill_on&=1;
    cg_max_trlevel=0;
    }
  break;

  case CG_METER_GRAPH:
  pause_thread(THREAD_SCREEN);
  hide_mouse(0,screen_width>>1,0,screen_height);
  cg.meter_graph_on^=1;
  manage_meter_graph();
  resume_thread(THREAD_SCREEN);
  break;
  
  default:
  lirerr(872);
  break;
  }
finish:;
leftpressed=BUTTON_IDLE;
mouse_active_flag=0;
make_coherent_graph(TRUE);
cg_oldx=COH_GRAPH_ACTIVE;
}


void mouse_on_coh_graph(void)
{
int event_no;
// First find out is we are on a button or border line.
for(event_no=0; event_no<MAX_CGBUTT; event_no++)
  {
  if( cgbutt[event_no].x1 <= mouse_x &&
      cgbutt[event_no].x2 >= mouse_x &&
      cgbutt[event_no].y1 <= mouse_y &&
      cgbutt[event_no].y2 >= mouse_y)
    {
    mouse_active_flag=1+event_no;
    current_mouse_activity=mouse_continue_coh_graph;
    return;
    }
  }
// Not button or border.
// Do nothing.
current_mouse_activity=mouse_nothing;
mouse_active_flag=1;
}

void make_coherent_graph(int clear_old)
{
int len;
pause_thread(THREAD_SCREEN);
hide_mouse(0,screen_width>>1,0,screen_height);
clear_cg_traces();
if(clear_old)
  {
  hide_mouse(cg_old_x1,cg_old_x2,cg_old_y1,cg_old_y2);
  lir_fillbox(cg_old_x1,cg_old_y1,cg_old_x2-cg_old_x1+1,
                                                    cg_old_y2-cg_old_y1+1,0);
  }
check_cg_borders();
clear_button(cgbutt, MAX_CGBUTT);
hide_mouse(cg.xleft,cg.xright,cg.ytop,cg.ybottom);  
scro[coherent_graph_scro].no=COH_GRAPH;
scro[coherent_graph_scro].x1=cg.xleft;
scro[coherent_graph_scro].x2=cg.xright;
scro[coherent_graph_scro].y1=cg.ytop;
scro[coherent_graph_scro].y2=cg.ybottom;
cgbutt[CG_LEFT].x1=cg.xleft;
cgbutt[CG_LEFT].x2=cg.xleft+2;
cgbutt[CG_LEFT].y1=cg.ytop;
cgbutt[CG_LEFT].y2=cg.ybottom;
cgbutt[CG_RIGHT].x1=cg.xright-2;
cgbutt[CG_RIGHT].x2=cg.xright;
cgbutt[CG_RIGHT].y1=cg.ytop;
cgbutt[CG_RIGHT].y2=cg.ybottom;
cgbutt[CG_TOP].x1=cg.xleft;
cgbutt[CG_TOP].x2=cg.xright;
cgbutt[CG_TOP].y1=cg.ytop;
cgbutt[CG_TOP].y2=cg.ytop+2;
cgbutt[CG_BOTTOM].x1=cg.xleft;
cgbutt[CG_BOTTOM].x2=cg.xright;
cgbutt[CG_BOTTOM].y1=cg.ybottom-2;
cgbutt[CG_BOTTOM].y2=cg.ybottom;
// Draw the border lines
graph_borders((void*)&cg,7);
if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
  {
  make_button(cg.xleft+text_width-1,cg.ybottom-text_height/2-1,
                                               cgbutt,CG_OSCILLOSCOPE,'o');
  }
make_button(cg.xleft+text_width-1,cg.ybottom-5*(text_height/2-1),
                                               cgbutt,CG_METER_GRAPH,'s');
cg_oldx=COH_GRAPH_ACTIVE;
settextcolor(7);
cg_x0=(cg.xleft+cg.xright)>>1;
len=((cg.xright-cg.xleft)>>1);
cg_y0=cg.ytop+len+1;
cg_y1=cg_y0+len;
lir_hline(cg.xleft+1,cg_y1,cg.xright-1,7);
if(kill_all_flag) return;
if(clear_old)
  {
  clear_coherent();
  }
cg_old_y1=cg.ytop;
cg_old_y2=cg.ybottom;
cg_old_x1=cg.xleft;
cg_old_x2=cg.xright;
manage_meter_graph();
resume_thread(THREAD_SCREEN);
make_modepar_file(GRAPHTYPE_CG);
update_meter_time=current_time();
}

void init_coherent_graph(void)
{
int xsiz, ysiz;
if(fft1_correlation_flag >= 2)return;
xsiz=3*cg_size+2;
ysiz=xsiz+5*text_height+4;
if (read_modepar_file(GRAPHTYPE_CG) == 0)
  {
reinit:;
  cg.xright=screen_width-1;
  cg.xleft=cg.xright-xsiz;
  cg.ybottom=bg.ybottom;
  cg.ytop=cg.ybottom-ysiz;
  cg.meter_graph_on=1;
  cg.oscill_on=0;
  }
if(cg.ybottom-cg.ytop != ysiz ||
   cg.xright-cg.xleft != xsiz)goto reinit;
cg_flag=1;
mg_flag=0;
cg_osc_shift_flag=0;
cg_max_trlevel=0;
coherent_graph_scro=no_of_scro;
// We have to reserve space for the meter graph here.
// The meter graph can be enabled or disabled from the coherent
// window at run time.
meter_graph_scro=no_of_scro+1;
cg.oscill_on&=1;
if(no_of_scro+2 >= MAX_SCRO)lirerr(89);
if(ui.operator_skil != OPERATOR_SKIL_EXPERT)cg.oscill_on=0;
make_coherent_graph(FALSE);
no_of_scro+=2;
}

