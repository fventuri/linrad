/////////////////////////////////////////////////////////////////////////////
//
//  Linrad driver code for the Funcube Dongle Pro Plus
//
//  Copyright (c) <2013> <Mike. J. Keehan>
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
//
//
//
//
//
//  This code uses HIDAPI from http://www.signal11.us/oss/hidapi/
//  and the Linux kernel hidraw interface.
//
//  M. J. Keehan, 25th Feb 2013  (mike@keehan.net)
//
//
// NOTES:-
//
// Frequency setting
//   The user graphic window shows the FCD's current frequency setting.  It
//   is not an input field.
//
//   Linrad's frequency control box is configured for input in MHz, and 
//   allows for 7 digits of input plus a decimal point, and uses a
//   floating point value to hold this input (floats are limited to approx
//   7 digits of precision).
//   The FCD supports setting input by individual Hz upto the 240MHz point,
//   and 7 digit precision above that.
//   Hence, it is possible to enter 1.234567MHz (or 12.34567, or 123.4567)
//   in Linrad and all digits will be used to set the FCD.  But above 240MHz
//   only 6 digits of frequency input are used (the code actually checks at
//   300MHz as the FCD has a break in coverage between approx 240MHz and 420MHz)
//
// Frequency increment 
//   The value to be used in Linrad's frequency control box when up/down
//   buttons are used.  Defaults to 100kHz, range 1kHz-100MHz.
//
// Rx Gain control
//   The lower control in Linrad's Frequency control box has been configured
//   to adjust the FCD Pro Plus's IF Gain settings.
//
// PPB (Parts per billion) Correction
//   A ppb correction value can be set in the lowest line of the graphic box.
//   Either enter a value by clicking on the current value and using the
//   keyboard, or use the up/down arrow keys to increment/decrement the
//   current value.
//
//
//
//   Note: The usergraphic position, the ppb setting, frequency increment
//         and other values are saved in a parameterfile which is read
//         when Linrad is started.
//         The file name is "par_xxx_hwd_fcdgr" (with xxx = rx_mode) and its
//         content can be displayed (and changed - carefully) with an editor.
//
//  
//////////////////////////////////////////////////////////////////////////////

#include "osnum.h"
#include "globdef.h"
#include <string.h>
#include "fcdpp.h"
#include "hidapi.h"
#include "uidef.h"
#include "screendef.h"
#include "fft1def.h"
#include "thrdef.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

// UserGraphic Button defs.
#define UG_TOP 0
#define UG_BOTTOM 1
#define UG_LEFT 2
#define UG_RIGHT 3
#define UG_INCREASE_PPB_CORR 4
#define UG_DECREASE_PPB_CORR 5
#define UG_LNA_ON_OFF 6
#define UG_MIXER_GAIN_ON_OFF 7
#define UG_BIAS_T_ON_OFF 8
#define MAX_FCDGRBUTT 9

//Size of messages in user graphic box
#define MAX_MSGSIZE 30


// FCDPro+ Global variables.
// =========================
int FCDstate;
#define FCD_NOTFOUND 0
#define FCD_RUNNING  2
#define FCD_BOOTLOAD 1

#define MAX_HWAREDRIVER_PARM 12
char *fcdprodriver_parm_text[MAX_HWAREDRIVER_PARM]={
        "fcdgr_xleft          ",
        "fcdgr_ytop           ",
        "center_freq          ",
        "freq_increment       ",
        "lna_on_off           ",
        "mixer_gain_on_off    ",
	"rf_filter_val        ",
        "if_filter_val        ",
        "ppb_correction       ",
        "if_gain              ",
        "bias_t_on_off        ",
        "dummy_entry_to_end   "
        }; 

typedef struct {
        int fcdgr_xleft;
        int fcdgr_ytop;
	unsigned int center_freq;
        int freq_incr;
	int lna_on_off;
	int mixer_gain_on_off;
	int rf_filter_val;
	int if_filter_val;
	int ppb_correction;
        int if_gain;
	int biasT_on_off;
	int dummy;
} HWAREDRIVER_PARMS;
HWAREDRIVER_PARMS fcdprodriver;

typedef struct {
        int ytop;
        int ybottom;
        int xleft;
        int xright;
        int yborder;
} UG_PARMS;
UG_PARMS fcdgr;

BUTTONS fcdgrbutt[MAX_FCDGRBUTT];

int fcdgr_old_y1;
int fcdgr_old_y2;
int fcdgr_old_x1;
int fcdgr_old_x2;
int fcdproplus_graph_scro;

int fcdgr_oldx;
int fcdgr_oldy;
int fcdgr_yborder;
int fcdgr_hsiz;
int fcdgr_vsiz;

int fcdgr_msg_color;          // switch for message-color in usergraph
int LinFreqColor = 7;
char fcdgr_msg0[MAX_MSGSIZE]; // messages in usergraph 
char fcdgr_msg1[MAX_MSGSIZE];
char fcdgr_msg2[MAX_MSGSIZE];

// Prototypes
int fcdppGetBiasT(unsigned char *);
int fcdppGetIFGain(unsigned char *);
int fcdppGetLNA(unsigned char *);
int fcdppGetMixerGain(unsigned char *);
int fcdppSetBiasT(unsigned char);
int fcdppSetFreq(unsigned int, unsigned int *);
int fcdppSetIFGain(unsigned char);
int fcdppSetLNA(unsigned char);
int fcdppSetMixerGain(unsigned char);
void fcd_append_freq(char *, double);
void init_fcdproplus_control_window(void);
void readDevice(int);
int read_value_from_fcd(unsigned char, unsigned char *);
void show_fcdpp_parms(void);
int write_value_to_fcd(unsigned char, unsigned int, unsigned int *);

//////////////
// Code
//////////////

void fcdproplus_setup(void)
{
// Routine to set up FCDProPlus from the soundcard setup menu.
clear_screen();
#if OSNUM == OSNUM_LINUX && defined(__linux__)
load_udev_library();
if(!udev_library_flag)return;
#endif
lir_text(3,3,"FCDProPlus selected. (Nothing to do)");
lir_text(3,6,press_any_key);
await_processed_keyboard();
}

// This is the data entered in the rx gain control box.
// int fg.gain_increment = The increment in dB for clicking
// the arrows in the gain control box.
// The global fg_new_band can be used to allow for different
// settings on different bands. 
//
// Uses global data:
//   int fg.gain
//************************************************************
void fcdproplus_rx_amp_control(void)
{
#if OSNUM == OSNUM_LINUX && defined(__linux__)
load_udev_library();
if(!udev_library_flag)
  {
  lirerr(999999);
  return;
  }
#endif
fg.gain_increment = 1;	// FCD's IF GAIN, step by 1dB.
// Limit gain to 0-59dB.  Be aware that values
// above about 30dB are likely to overload.
if ( fg.gain < 0 )fg.gain = 0;
if ( fg.gain > 59 )fg.gain = 59;
FCDstate = fcdppSetIFGain(fg.gain);
FCDstate = fcdppGetIFGain((unsigned char *)&fcdprodriver.if_gain);
fg.gain = fcdprodriver.if_gain;
}

void fcdproplus_rx_freq_control(void)
{ 
unsigned int fcdFreq, askFreq, centerfreq;
 int correction;
float t1;
#if OSNUM == OSNUM_LINUX && defined(__linux__)
load_udev_library();
if(!udev_library_flag)
  {
  lirerr(999999);
  return;
  }
#endif
if ( fcdprodriver.dummy == 0 )	// not yet loaded user parameters
    return;

fg.passband_increment = ((double)fcdprodriver.freq_incr) / 1000.0;

// Limit the requested frequency to what is possible on FCDproplus.
LinFreqColor = 10;			// normal GREEN
if ( fg.passband_center < .15 )     // set min freq to 150kHz.
  { 
  fg.passband_center = .15;
  LinFreqColor = 12;		// show frequency in RED
  }
if ( fg.passband_center > 2050 )    // set max freq to 2.05GHz.
  { fg.passband_center = 2050;
    LinFreqColor = 12;		// show frequency in RED
  }

  // Following horrible expression uses floating point to maintain precision till result.
correction = (int)(((fg.passband_center*1000000.0*
                     fcdprodriver.ppb_correction))/1000000000);

  // Round() request frequency to 7 digits of precision
t1=rint(fg.passband_center*1000000);
centerfreq = (unsigned int)t1;
if ( centerfreq > 300000000 )centerfreq = ((centerfreq)/1000)*1000;
if ( centerfreq > 99999999 )centerfreq = ((centerfreq+4)/100)*100;
if ( centerfreq > 9999999 )centerfreq = ((centerfreq+4)/10)*10;
fcdprodriver.center_freq = centerfreq;
askFreq = centerfreq + correction;
if ( askFreq > 300000000 )askFreq = ((askFreq)/1000)*1000;
FCDstate = fcdppSetFreq(askFreq, &fcdFreq);
if ( fcdFreq != askFreq )
  { 
  if ( FCDstate != FCD_RUNNING )
    {
    fprintf( stderr,"fcdpp: FAILED TO SET FCD FREQ! mode: %s, ask:%d, got:%d\n",
                (FCDstate==FCD_RUNNING) ? "Running" :
                (FCDstate==FCD_BOOTLOAD) ? "BootLoad" : "Not Found", askFreq, fcdFreq);
    }
  LinFreqColor = 12;		// show frequency in RED text!
  fg.passband_center = (double) (fcdFreq - correction)/1000000.0;
  fcdprodriver.center_freq = fcdFreq;
  }

show_fcdpp_parms();
//fprintf( stderr,"fcdpp_rx_freq: fg.pass %f, center %d, corr %d, askFreq %d, fcdFreq %d\n",
//        fg.passband_center, centerfreq, correction, askFreq, fcdFreq);
}

// Append frequency formatted in Hz or kHz to the end of
// the already present text in the MAX_MSGSIZE buffer.
void fcd_append_freq(char *buffer, double fq)
{ int j, freq;
  char *src, *dst, *tail = " Hz", stmp[MAX_MSGSIZE];
  unsigned u,spaceleft;

  freq = (int)fq;                // need Hertz, and not fractions of
  sprintf(stmp, "%d", freq);

  // Find out how much space is left in given buffer.
  spaceleft = MAX_MSGSIZE - strlen(buffer) - 1;
  
  // Change freq to kHz if too many digits present as Hz
  if ( (strlen(stmp) + strlen(" Hz")) > spaceleft )
  {
    freq = freq / 1000;          // kHz
    sprintf(stmp, "%d", freq);
    tail = " kHz";
  }

  // space pad the rest of the buffer, and terminate with the tail.
  for ( u=strlen(buffer) ; u<(MAX_MSGSIZE - strlen(tail) -1) ; ++u )
    buffer[u] = ' ';
  strcpy(&buffer[MAX_MSGSIZE - strlen(tail) -1], tail);

  // copy frequency tail first, adding separators every 3rd digit
  src = &stmp[strlen(stmp)];
  dst = &buffer[MAX_MSGSIZE - strlen(tail) -1];
  for ( u=strlen(stmp), j=0 ; u ; --u )
  {
    *--dst = *--src;
    if ( (++j % 3) == 0  &&  u > 1 ) // but no sep. at front of digits
      *--dst = ',';
  }         
}

void check_fcdgr_borders(void)  // required for move graph
{
  current_graph_minh=fcdgr_vsiz;
  current_graph_minw=fcdgr_hsiz;
  check_graph_placement((void*)(&fcdgr));
  set_graph_minwidth((void*)(&fcdgr));
}

void make_fcdproplus_control_graph(void)
{
  pause_thread(THREAD_SCREEN);

  check_fcdgr_borders();        // required for move graph
  scro[fcdproplus_graph_scro].no=FCDPROPLUS_GRAPH;

  // These are the coordinates of the border lines.
  scro[fcdproplus_graph_scro].x1=fcdgr.xleft;
  scro[fcdproplus_graph_scro].x2=fcdgr.xright;
  scro[fcdproplus_graph_scro].y1=fcdgr.ytop;
  scro[fcdproplus_graph_scro].y2=fcdgr.ybottom;

  // Each border line is treated as a button.
  // That is for the mouse to get hold of them so the window can be moved.
  fcdgrbutt[UG_LEFT].x1=fcdgr.xleft;
  fcdgrbutt[UG_LEFT].x2=fcdgr.xleft+2;
  fcdgrbutt[UG_LEFT].y1=fcdgr.ytop;
  fcdgrbutt[UG_LEFT].y2=fcdgr.ybottom;
  fcdgrbutt[UG_RIGHT].x1=fcdgr.xright;
  fcdgrbutt[UG_RIGHT].x2=fcdgr.xright-2;
  fcdgrbutt[UG_RIGHT].y1=fcdgr.ytop;
  fcdgrbutt[UG_RIGHT].y2=fcdgr.ybottom;
  fcdgrbutt[UG_TOP].x1=fcdgr.xleft;
  fcdgrbutt[UG_TOP].x2=fcdgr.xright;
  fcdgrbutt[UG_TOP].y1=fcdgr.ytop;
  fcdgrbutt[UG_TOP].y2=fcdgr.ytop+2;
  fcdgrbutt[UG_BOTTOM].x1=fcdgr.xleft;
  fcdgrbutt[UG_BOTTOM].x2=fcdgr.xright;
  fcdgrbutt[UG_BOTTOM].y1=fcdgr.ybottom;
  fcdgrbutt[UG_BOTTOM].y2=fcdgr.ybottom-2;

  // Draw the border lines
  graph_borders((void*)&fcdgr,7);
  fcdgr_oldx=-10000;                           // from freq_control
  settextcolor(7);
  make_button(fcdgr.xleft+57.2*text_width+2,-2+fcdgr.ybottom-1*text_height/2-1,
                                     fcdgrbutt,UG_DECREASE_PPB_CORR,25);
  make_button(fcdgr.xleft+58.7*text_width+2,-2+fcdgr.ybottom-1*text_height/2-1,
                                     fcdgrbutt,UG_INCREASE_PPB_CORR,24); 
  
  // Draw separator lines in usergraph
  // vertical
  lir_line(fcdgr.xleft+30.5*text_width,fcdgr.ytop+1.3*text_height,
                                    fcdgr.xleft+30.5*text_width,fcdgr.ybottom,7);
  // horizontal
  lir_line(fcdgr.xleft,fcdgr.ytop+1.3*text_height,fcdgr.xright,fcdgr.ytop+1.3*text_height,7);

  // Make button for LNA
  fcdgrbutt[UG_LNA_ON_OFF].x1=fcdgr.xleft+16*text_width;
  fcdgrbutt[UG_LNA_ON_OFF].x2=fcdgr.xleft+20*text_width;
  fcdgrbutt[UG_LNA_ON_OFF].y1=fcdgr.ytop+1.5*text_height;
  fcdgrbutt[UG_LNA_ON_OFF].y2=fcdgr.ytop+2.4*text_height;

  // Make button for Mixer Gain
  fcdgrbutt[UG_MIXER_GAIN_ON_OFF].x1=fcdgr.xleft+16*text_width;
  fcdgrbutt[UG_MIXER_GAIN_ON_OFF].x2=fcdgr.xleft+20*text_width;
  fcdgrbutt[UG_MIXER_GAIN_ON_OFF].y1=fcdgr.ytop+2.5*text_height;
  fcdgrbutt[UG_MIXER_GAIN_ON_OFF].y2=fcdgr.ytop+3.4*text_height;

  // Make button for BiasT
  fcdgrbutt[UG_BIAS_T_ON_OFF].x1=fcdgr.xleft+16*text_width;
  fcdgrbutt[UG_BIAS_T_ON_OFF].x2=fcdgr.xleft+20*text_width;
  fcdgrbutt[UG_BIAS_T_ON_OFF].y1=fcdgr.ytop+3.5*text_height;
  fcdgrbutt[UG_BIAS_T_ON_OFF].y2=fcdgr.ytop+4.4*text_height;

  fcdproplus_rx_freq_control();
  show_fcdpp_parms();
  resume_thread(THREAD_SCREEN);
}

void append_blanks_fcdgrmsg(char *msg)
{ int i;

  i=strlen(msg);
  if(i>=MAX_MSGSIZE)
    lirerr(3413400);
  while(i<MAX_MSGSIZE-1)
  {
    msg[i]=' ';
    i++;
  }
  msg[MAX_MSGSIZE-1]=0;  
}

// Show the user parameters on screen
void show_fcdpp_parms(void)
{ int i;
  int *sdr_pi;
  char s[80];
  char *mode = "";
  char fcdprodriver_parfil_name[20];
  FILE *fcdprodriver_file;

  switch(rx_mode)
  {
    case MODE_WCW:       mode = "WCW";       break;
    case MODE_HSMS:      mode = "HSMS";      break;
    case MODE_QRSS:      mode = "QRSS";      break;
    case MODE_NCW:       mode = "NCW";       break;
    case MODE_SSB:       mode = "SSB";       break;
    case MODE_FM:        mode = "FM ";       break;
    case MODE_AM:        mode = "AM ";       break;
  }

  if ( fcdprodriver.dummy == 0 )	// if control window does not yet exist, exit.
    return;

  hide_mouse(fcdgr.xleft, fcdgr.xright,fcdgr.ytop,fcdgr.ybottom);
  sprintf(s,"FunCubeDongle Pro+  %4s         LINRAD  %s mode",
                  (FCDstate==FCD_RUNNING) ? "Running" : \
                  (FCDstate==FCD_BOOTLOAD) ? "BootLoad" : "Not Found", mode);
  settextcolor(14); // yellow
  lir_pixwrite(fcdgr.xleft+1.5*text_width,fcdgr.ytop+0.35*text_height,s);
  settextcolor(15); // white
  settextcolor(7);  // grey

  sprintf(s,"PPB correction : %d ", fcdprodriver.ppb_correction);
  s[26]=0;
  lir_pixwrite(fcdgr.xleft+31*text_width,fcdgr.ytop+1+3.5*text_height,s);
  sprintf(s,"VFO incr (Khz) : %d        ", fcdprodriver.freq_incr);
  s[25]=0;
  lir_pixwrite(fcdgr.xleft+31*text_width,fcdgr.ytop+2.5*text_height,s);

  //Display messages in message-area of usergraph
  settextcolor(11); // light blue
  sprintf(s,"Low Noise Amp: %3s", fcdprodriver.lna_on_off ? "On" : "Off");
  append_blanks_fcdgrmsg(s);
  lir_pixwrite(fcdgr.xleft+1.5*text_width,fcdgr.ytop+1.5*text_height,s);
  sprintf(s,"Mixer Gain   : %3s", fcdprodriver.mixer_gain_on_off? "On":"Off");
  append_blanks_fcdgrmsg(s);
  lir_pixwrite(fcdgr.xleft+1.5*text_width,fcdgr.ytop+2.5*text_height,s);
  sprintf(s,"Bias T       : %3s", fcdprodriver.biasT_on_off ? "On" : "Off");
  append_blanks_fcdgrmsg(s);
  lir_pixwrite(fcdgr.xleft+1.5*text_width,fcdgr.ytop+3.5*text_height,s);

  sprintf(fcdgr_msg0,"FCDPP Freq: ");
  fcd_append_freq(fcdgr_msg0,fcdprodriver.center_freq);
  append_blanks_fcdgrmsg(fcdgr_msg0);
  settextcolor(LinFreqColor);
  lir_pixwrite(fcdgr.xleft+31*text_width,fcdgr.ytop+1.5*text_height,fcdgr_msg0);

  // Save screenposition, and FCD settings to the parameterfile on disk
  sprintf(fcdprodriver_parfil_name,"%s_hwd_fcdgr",rxpar_filenames[rx_mode]);
  fcdprodriver_file=fopen(fcdprodriver_parfil_name,"w");
  fcdprodriver.fcdgr_xleft=fcdgr.xleft;
  fcdprodriver.fcdgr_ytop=fcdgr.ytop;
  sdr_pi=(int*)(&fcdprodriver);
  for(i=0; i<MAX_HWAREDRIVER_PARM; i++)
    fprintf(fcdprodriver_file,"%s [%d]\n",fcdprodriver_parm_text[i],sdr_pi[i]);
  parfile_end(fcdprodriver_file);

  settextcolor(7);
}

void new_fcd_lna_on_off(void)
{ int fme;

  fcdprodriver.lna_on_off ^= 1;
  fme = fcdppSetLNA(fcdprodriver.lna_on_off);
  if ( fme == 0 )
    fprintf( stderr,"new_fcd_lna_on_off: Failed to change LNA setting\n");
}

void new_fcd_biasT(void)
{ int fme;

  fcdprodriver.biasT_on_off ^= 1;
  fme = fcdppSetBiasT(fcdprodriver.biasT_on_off);
  if ( fme == 0 )
    fprintf( stderr,"new_fcd_biasT: Failed to change BIAS T setting\n");
}

void new_fcd_mixer_gain(void)
{ int fme;

  fcdprodriver.mixer_gain_on_off ^= 1;
  fme = fcdppSetMixerGain(fcdprodriver.mixer_gain_on_off);
  if ( fme == 0 )
    fprintf( stderr,"new_fcd_mixer_gain: Failed to change MixerGain setting\n");
}

void mouse_continue_fcdproplus_graph(void)
{ int i, j;      // required for move graph

  // Move border lines immediately.
  // Other functions must wait until button is released.
  // Move fixed size window  based on coding in freq_control.c
  switch (mouse_active_flag-1)
  {
    case UG_TOP:
    if (fcdgr.ytop!=mouse_y)break;
    break;

    case UG_BOTTOM:
    if (fcdgr.ybottom!=mouse_y)break;
    break;
  
    case UG_LEFT:
    if (fcdgr.xleft!=mouse_x)break;
    break;

    case UG_RIGHT:
    if (fcdgr.xright==mouse_x)break;
    break;
  
    default:
    goto await_release;
  }

  pause_screen_and_hide_mouse();
  graph_borders((void*)&fcdgr,0);
  if (fcdgr_oldx==-10000)
  {
    fcdgr_oldx=mouse_x;
    fcdgr_oldy=mouse_y;
  }
  else
  {
    i=mouse_x-fcdgr_oldx;
    j=mouse_y-fcdgr_oldy;  
    fcdgr_oldx=mouse_x;
    fcdgr_oldy=mouse_y;
    fcdgr.ytop+=j;
    fcdgr.ybottom+=j;
    fcdgr.xleft+=i;
    fcdgr.xright+=i;
    check_fcdgr_borders();    
    fcdgr.yborder=(fcdgr.ytop+fcdgr.ybottom)>>1;
  }
  graph_borders((void*)&fcdgr,15); 
  resume_thread(THREAD_SCREEN);

  if (leftpressed == BUTTON_RELEASED)
    goto finish;
  return;

await_release:;
  if (leftpressed != BUTTON_RELEASED)
    return;

  // Assuming the user wants to control hardware, allow
  // commands only when data is not over the network.
  if((ui.network_flag&NET_RX_INPUT) == 0) //for linrad02.23
  {
    switch (mouse_active_flag-1)
    {
      case UG_INCREASE_PPB_CORR: 
        if ( fcdprodriver.ppb_correction < 255000 )
        {
          fcdprodriver.ppb_correction+=100;
          fcdproplus_rx_freq_control();
        }
        break;

      case UG_DECREASE_PPB_CORR:
        if ( fcdprodriver.ppb_correction > -255000 )
        {
          fcdprodriver.ppb_correction-=100;
          fcdproplus_rx_freq_control();
        }
        break;
 
      case UG_LNA_ON_OFF: 
        new_fcd_lna_on_off();
        break;
 
      case UG_MIXER_GAIN_ON_OFF:
        new_fcd_mixer_gain();
        break;

      case UG_BIAS_T_ON_OFF:
        new_fcd_biasT();
        break;
 
      default:        // This should never happen.    
        lirerr(211053);
        break;
    }
  }  

finish:;
  hide_mouse(fcdgr_old_x1,fcdgr_old_x2,fcdgr_old_y1,fcdgr_old_y2);
  leftpressed=BUTTON_IDLE;  
  mouse_active_flag=0;
  graph_borders((void*)&fcdgr,0);
  lir_fillbox(fcdgr_old_x1,fcdgr_old_y1,fcdgr_old_x2-fcdgr_old_x1,fcdgr_old_y2-fcdgr_old_y1,0);
  make_fcdproplus_control_graph();
  fcdgr_oldx=-10000; 
}

void new_fcd_ppb_correction(void)
{
  if ( numinput_int_data >= -255000 && numinput_int_data <= 255000 )
  {
    fcdprodriver.ppb_correction=numinput_int_data;
    pause_thread(THREAD_SCREEN);
    show_fcdpp_parms();
    resume_thread(THREAD_SCREEN);
    fcdproplus_rx_freq_control();
  }
}

void new_fcd_vfo_incr(void)
{
  if ( numinput_int_data < 1 )
    numinput_int_data = 1;
  if ( numinput_int_data > 100000 )
    numinput_int_data = 100000;

  fcdprodriver.freq_incr=numinput_int_data;
  fg.passband_increment = ((double)fcdprodriver.freq_incr) / 1000.0;
  pause_thread(THREAD_SCREEN);
  show_fcdpp_parms();
  resume_thread(THREAD_SCREEN);
}

void mouse_on_fcdproplus_graph(void)
{ int event_no;
  // First find out if we are on a button or border line.
  for(event_no=0; event_no<MAX_FCDGRBUTT; event_no++)
  {
    if( fcdgrbutt[event_no].x1 <= mouse_x &&
        fcdgrbutt[event_no].x2 >= mouse_x &&
        fcdgrbutt[event_no].y1 <= mouse_y &&
        fcdgrbutt[event_no].y2 >= mouse_y)
    {
      fcdgr_old_y1=fcdgr.ytop;
      fcdgr_old_y2=fcdgr.ybottom;
      fcdgr_old_x1=fcdgr.xleft;
      fcdgr_old_x2=fcdgr.xright;
      mouse_active_flag=1+event_no;
      current_mouse_activity=mouse_continue_fcdproplus_graph;
      return;
    }
  }

  mouse_active_flag=1;

  if(mouse_x > fcdgr.xleft+48*text_width && mouse_x < fcdgr.xleft+53*text_width)
  {
    numinput_xpix=fcdgr.xleft+48*text_width;
    if(mouse_y > (fcdgr.ytop+3.5*text_height))
    {
      numinput_ypix=fcdgr.ytop+3.6*text_height;
      numinput_chars=6;
      erase_numinput_txt();
      numinput_flag=FIXED_INT_PARM;
      par_from_keyboard_routine=new_fcd_ppb_correction;	// PPB value
    }
    else if(mouse_y > (fcdgr.ytop+2.5*text_height))
    {
      numinput_ypix=fcdgr.ytop+(2.4*text_height)+2;
      numinput_chars=6;
      erase_numinput_txt();
      numinput_flag=FIXED_INT_PARM;
      par_from_keyboard_routine=new_fcd_vfo_incr;	// VFO incr value
    }
  }
  else
  { // If we did not select a numeric input by setting numinput_flag
    // we have to set mouse_active flag.
    // Set the routine to mouse_nothing, we just want to
    // set flags when the mouse button is released.    
    current_mouse_activity=mouse_nothing;
    mouse_active_flag=1;
  }
}

void init_fcdproplus_control_window(void)
{ 
int i;
int errcod;
int *sdr_pi;
char fcdprodriver_parfil_name[20];
FILE *fcdprodriver_file;
#if OSNUM == OSNUM_LINUX && defined(__linux__)
load_udev_library();
if(!udev_library_flag)
  {
  lirerr(999999);
  return;
  }
#endif
// Get values from parameterfile
sprintf(fcdprodriver_parfil_name,"%s_hwd_fcdgr",rxpar_filenames[rx_mode]);
errcod = read_sdrpar(fcdprodriver_parfil_name, MAX_HWAREDRIVER_PARM, 
                       fcdprodriver_parm_text, (int*)((void*)&fcdprodriver));
if (errcod != 0)
  {
  fcdprodriver_file = fopen(fcdprodriver_parfil_name,"w");
//set_default_parameter values
  fcdprodriver.fcdgr_xleft = 60*text_width;                    
  fcdprodriver.fcdgr_ytop = (screen_last_line-4)*text_height; 
  fcdprodriver.center_freq = 1000000;
  fcdprodriver.freq_incr = 100;	// khz
  fcdprodriver.lna_on_off = 1;	// on
  fcdprodriver.mixer_gain_on_off = 1; // on
  fcdprodriver.if_filter_val = 0;
  fcdprodriver.if_gain = 0;		// dB
  fcdprodriver.ppb_correction = 0;
  fcdprodriver.biasT_on_off = 0;	// off
  sdr_pi = (int*)(&fcdprodriver);
  for( i=0; i<MAX_HWAREDRIVER_PARM; i++ )
    {
    fprintf(fcdprodriver_file,"%s [%d]\n",fcdprodriver_parm_text[i],sdr_pi[i]);
    }
  parfile_end(fcdprodriver_file);
  }
fcdgr.xleft = fcdprodriver.fcdgr_xleft;
fcdgr.ytop = fcdprodriver.fcdgr_ytop;
// Read state of FCD itself
FCDstate = fcdppGetLNA((unsigned char *)&fcdprodriver.lna_on_off);
FCDstate = fcdppGetMixerGain((unsigned char *)&fcdprodriver.mixer_gain_on_off);
FCDstate = fcdppGetBiasT((unsigned char *)&fcdprodriver.biasT_on_off);
FCDstate = fcdppGetIFGain((unsigned char *)&fcdprodriver.if_gain);
fg.gain = fcdprodriver.if_gain;
fg.passband_center = ((double)fcdprodriver.center_freq) / 1000000.0;
fg.passband_increment = ((double)fcdprodriver.freq_incr) / 1000.0;
 fcdgr_msg_color = 10;
fcdgr_msg1[0] = 0;
fcdgr_hsiz = (30+MAX_MSGSIZE)*text_width;  //  WIDTH OF USERGRAPH
fcdgr_vsiz = (4.8*text_height);            //  HEIGHT OF USERGRAPH
fcdgr.xright = fcdgr.xleft+fcdgr_hsiz;
fcdgr.ybottom = fcdgr.ytop+fcdgr_vsiz;
if(rx_mode < MODE_TXTEST)
  {
  fcdproplus_graph_scro = no_of_scro;
  make_fcdproplus_control_graph();
  no_of_scro++;
  if(no_of_scro >= MAX_SCRO)
  lirerr(89);
  }
fcdprodriver.dummy = 1;		// indicates control window is setup OK.
}

/////////////////////
// FCDPP I/O routines
/////////////////////

int fcdppGetBiasT(unsigned char *state)
{ return read_value_from_fcd(FCD_HID_CMD_GET_BIAS_TEE, state);
}

int fcdppGetIFGain(unsigned char *value)
{ return read_value_from_fcd(FCD_HID_CMD_GET_IF_GAIN, value);
}

int fcdppGetLNA(unsigned char *state)
{ return read_value_from_fcd(FCD_HID_CMD_GET_LNA_GAIN, state);
}

int fcdppGetMixerGain(unsigned char *state)
{ return read_value_from_fcd(FCD_HID_CMD_GET_MIXER_GAIN, state);
}

int fcdppSetBiasT(unsigned char state)
{ return write_value_to_fcd(FCD_HID_CMD_SET_BIAS_TEE, state, NULL);
}

int fcdppSetFreq(unsigned int nufreq, unsigned int *fcdfreq)
{ return write_value_to_fcd(FCD_HID_CMD_SET_FREQUENCY_HZ, nufreq, fcdfreq);
}

int fcdppSetIFGain(unsigned char value)
{ return write_value_to_fcd(FCD_HID_CMD_SET_IF_GAIN, value, NULL);
}

int fcdppSetLNA(unsigned char state)
{ return write_value_to_fcd(FCD_HID_CMD_SET_LNA_GAIN, state, NULL);
}

int fcdppSetMixerGain(unsigned char state)
{ return write_value_to_fcd(FCD_HID_CMD_SET_MIXER_GAIN, state, NULL);
}

//////////////////////////////////////////////////////////////////
//
//  I/O routines using HID for the FCDProPlus
//
//////////////////////////////////////////////////////////////////
//  identifiers for the FCDPP USB interface
#define USB_VEND 0x04d8		// USB vendor ID (Microchip Technology Inc.)
#define USB_PROD 0xfb31		// USB product ID
#define FCD_BUFF 65

int read_value_from_fcd(unsigned char cmd, unsigned char *fcdbyte)
{ hid_device *fcd_hid;
  unsigned char buff[FCD_BUFF];
  int result;
  
  fcd_hid = hid_open(USB_VEND, USB_PROD, NULL);
  if ( fcd_hid == NULL )
    return FCD_NOTFOUND;

  memset(buff, 0, FCD_BUFF);
  buff[0] = 0;		// Report ID.  Ignored by FCDPP
  buff[1] = cmd;
  result = hid_write(fcd_hid, buff, FCD_BUFF);
  if ( result < 0 )
    fprintf( stderr,"read_value_from_fcd: hid_write=%d\n", result);

  memset(buff, 0, FCD_BUFF);
  result = hid_read(fcd_hid, buff, FCD_BUFF);
  if ( result < 0 )
    fprintf( stderr,"read_value_from_fcd: hid_read=%d\n", result);

  hid_close(fcd_hid);

  if ( (buff[0] == cmd)  &&  (buff[1] == 1) )
  { *fcdbyte = buff[2];
    return FCD_RUNNING;
  }
  return FCD_BOOTLOAD;
}

int write_value_to_fcd(unsigned char cmd, unsigned int fcdvalue, unsigned int *fcdfreq)
{ hid_device *fcd_hid;
  unsigned char buff[FCD_BUFF];		// buffer to be written to and read from FCDPP
  int result;

  fcd_hid = hid_open(USB_VEND, USB_PROD, NULL);
  if ( fcd_hid == NULL )
    return FCD_NOTFOUND;

  memset(buff, 0, FCD_BUFF);
  buff[0] = 0;		// Report ID.  Ignored by FCDPP
  buff[1] = cmd;
  buff[2] = (unsigned char) fcdvalue & 0xFF;          // most values are single char only
  buff[3] = (unsigned char) (fcdvalue >> 8) & 0xFF;   // Freq needs these extra 3 bytes.
  buff[4] = (unsigned char) (fcdvalue >> 16) & 0xFF;
  buff[5] = (unsigned char) (fcdvalue >> 24) & 0xFF;
  result = hid_write(fcd_hid, buff, FCD_BUFF);
  if ( result < 0 )
    fprintf( stderr,"write_value_to_fcd: hid_write=%d\n", result);

  memset(buff, 0, FCD_BUFF);
  result = hid_read(fcd_hid, buff, FCD_BUFF);
  if ( result < 0 )
    fprintf( stderr,"write_value_to_fcd: hid_read=%d\n", result);

  hid_close(fcd_hid);

  if ( buff[0]==FCD_HID_CMD_SET_FREQUENCY_HZ && buff[1]==1 )
  { if (fcdfreq != NULL)
    { *fcdfreq = (unsigned int) buff[2];
      *fcdfreq += (unsigned int) (buff[3] << 8);
      *fcdfreq += (unsigned int) (buff[4] << 16);
      *fcdfreq += (unsigned int) (buff[5] << 24);
    }
  }

  if ( buff[0] == cmd )
    return FCD_RUNNING;

  return FCD_BOOTLOAD;
}
