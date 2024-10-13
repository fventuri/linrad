//
//  Code for the Elektor USB SDR
//
//  Driver for the USB SDR as described in the May 2007 Elektor magazine.
//
//  Based on users_tr.c from Linrad, and on SDRelektor.pas from
//  Burkhard Kainka, and ElektorSDR4Linux.c by Markus Goppelt,
//  which are available at web-site :-  http://www.b-kainka.de
//
//  This driver requires the USB FTDI Driver library, libftdi.
//
//
//
//  M. J. Keehan, 12th Feb 2013  (mike@keehan.net)
//
//
//
// USERGUIDE
//
// Moving the usergraphic:
//   Move the usergraphic on the screen by hitting the left side of the box
//   with the mousepointer, holding the left mouse button down, and dragging
//   it to the desired position.  Also works with the upper side. 
//
// Setting the VFO frequency:
//   Enter a value in Mhz into the upper field in the small Frequency/Gain box.
//   Or click on the adjacent up/down arrows to shift the frequency by the
//   increment value (default 20kHz).
//   Elektor's frequency band is calculated from the frequency entered. 
//
// Frequency increment
//   The value used in the frequency up/down arrows, default 20kHz, range 1kHz-1MHz.
//
// Aerial Multiplexor (Mux)
//   The eight mux settings can be chosen manually by either entering the number
//   (0-7) into the Mux box, or by incrementing/decrementing using the associated 
//   buttons.  Choosing the value 8 puts the software into Automatic mode, where
//   the Mux is chosen according to the frequency in use.
//
// Pre-Selector
//   Elektor's PreSelector option board is catered for.  The preselector
//   settings are loaded from a file - with the name stored in the variable
//   'preselector_parfile_name' - when Linrad starts.
//   The values will be sent to the board when Mux is set to 3,4,5 or 6, or if
//   Mux is set to 8 (auto), then the Aerial Mux to use is chosen from the 
//   available preselector channels if one covers the current tuning frequency.
//   The Pre-Selector has a range of 8 bits for its settings, i.e. values 0 - 255.
//   This canbe entered manually, or by use of coarse buttons which increment/decrement
//   by 10, or by fine buttons which increment/decrement by 1.
//   If no file exists at startup, then all preselector values are set to 0, and
//   manual input is required for each specific presel channel to tune it.
//
// Pre-Selector tuning
//   The preselector can be tuned when Mux=3,4,5,6 or 8.
//   When Aerial Mux=8 is selected, preselector tuning points are not saved to file.
//   When specific preselector channels are chosen, i.e. Mux=3,4,5 or 6, then when
//   preselector tuning is performed, the settings are saved to the parameter file.
//   An array of ten values are kept in memory to cover the preselector range.
//   The software maintains a guard value at each end of the array, both to
//   ensure that the limit values are present (0 & 255), and to make 
//   interpolation between settings easier to determine when near band edges.
//   The software interpolates linearly between the
//   array values as the frequency is changed.
//   The software tries to keep entries spaced more than ~5% apart by frequency.
//   The contents of the preselector parfile can be edited by hand if values
//   closer than 5% are required, or change the divisor used in function
//   withinXpercent() below.
//   To clear all settings and setup anew, edit the file and delete all values
//   for the required preselector channel(s), or delete the whole file.  Restart
//   Linrad and tune the preselector at up to ten signals spread across the 
//   preselector's tuning range.  It is not necessary to find signals which make
//   the values 0 and 255 as the software will extrapolate these from whatever
//   values are in the array. Neither is it necessary to go through all ten
//   tunings, just as many as you need accurate settings for.
//   The file format is described in the comments to the function
//   "Load_PreSelectFile" at the end of this source.
//   
// Fine tune the Crystal
//   The Elektor's 10MHz crystal can be fine tuned over the range 0-255 in the
//   lowest line of the usergraphic box.  Either enter a value by clicking on
//   the current value and using the keyboard, or use the up/down arrow keys
//   to increment/decrement the current value.  The actual frequency change
//   represented by this range (0-255) depends upon the crystal itself.  I get
//   approx +-1.1kHz at 10MHz.
//
// Test signal
//   The Elektor provides a test signal at 5MHz on aerial Mux 7, at a strength
//   of S9 +40dB.
//
//   Note: The usergraphic position, the Xtal fine tune, and frequency increment
//         values for each frequency band are saved in a parameterfile and are
//         retrieved when Linrad is started.
//         The file name is "par_xxx_elektor" (with xxx = rx_mode) and its
//         content can be displayed (and changed - carefully) with an editor.
//
//  


#include "osnum.h"
#include "loadusb.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "uidef.h"
#include "fft1def.h"
#include "screendef.h"
#include "seldef.h"
#include "thrdef.h"
#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#include <unistd.h>
#endif



#define IIC_EEPROM 208             // Elektor's hardware interface
#define IIC_RAM 210                // Elektor's hardware interface
#define PCF8951_ADDRESS 0x90       // Elektor's preselector interface
#define DAC_ENABLE 0x40            // Elektor's preselector interface


// UserGraphic Button defs.
#define UG_TOP 0
#define UG_BOTTOM 1
#define UG_LEFT 2
#define UG_RIGHT 3
#define UG_INCREASE_OFFS_XTAL 4
#define UG_DECREASE_OFFS_XTAL 5
#define UG_INCREASE_OFFS_KHZ 6
#define UG_DECREASE_OFFS_KHZ 7
#define UG_INCREASE_MUX_NMBR 8
#define UG_DECREASE_MUX_NMBR 9
#define UG_INCREASE_PRESEL 10
#define UG_DECREASE_PRESEL 11
#define UG_INCREASE_PRESELFINE 12
#define UG_DECREASE_PRESELFINE 13
#define MAX_EGRBUTT 14

//Size of messages in user graphic box
#define MAX_MSGSIZE 30


// Elektor Global variables.
// =========================
int FreqWanted;
int Fdti_Data_Count=0;
int Mux = 8;                // Automatic mode
int ActiveMux = -1;
int VFOhigh;
int Write_Pins=0;
int PreselUpdated = 0;
unsigned char Div1N;
unsigned char FT_port;
unsigned char FtdiData[65536];

char* preselector_parfile_name={"par_elektor_preselector"};

struct bands
{       int Min[6];
        int Max[6];
        int VFOhigh[6];
        unsigned char Div1N[6];
        float High[6];
} bands_struct;

#define NUM_SETTINGS 10     // Allow for 10 PreSelector calibration values
struct preselval {
  int freq[NUM_SETTINGS+8]; // adds a guard entry at each end.
  int val[NUM_SETTINGS+8];  // ditto.
} PreSelData[4];            // Four pre-selector front-ends.
int PreSelValue = 0;

struct ftdi_context ftdic;


#define MAX_HWAREDRIVER_PARM 16
char *elektordriver_parm_text[MAX_HWAREDRIVER_PARM]={
                                       "egr_xleft            ",
                                       "egr_ytop             ",
                                       "mux_number           ",
                                       "xtal_fine_0.1-0.8    ",
                                       "xtal_fine_0.4-1.6    ",
                                       "xtal_fine_0.8-3.2    ",
                                       "xtal_fine_2-8        ",
                                       "xtal_fine_4-16       ",
                                       "xtal_fine_10-30      ",
                                       "freq_incr_0.1-0.8    ",
                                       "freq_incr_0.4-1.6    ",
                                       "freq_incr_0.8-3.2    ",
                                       "freq_incr_2-8        ",
                                       "freq_incr_4-16       ",
                                       "freq_incr_10-30      ",
                                       "offs_khz_band_BIG    ",
                                       }; 

#define NO_OF_BANDS 6
typedef struct {
        int egr_xleft;
        int egr_ytop;
        int aerial;
        int offs_hz[NO_OF_BANDS];
        int offs_khz[NO_OF_BANDS];
	int offs_khz_band_BIG;
} HWAREDRIVER_PARMS;
HWAREDRIVER_PARMS elektordriver;

typedef struct {
        int ytop;
        int ybottom;
        int xleft;
        int xright;
        int yborder;        // required for move graph
} UG_PARMS;
UG_PARMS egr;

BUTTONS egrbutt[MAX_EGRBUTT];

int egr_old_y1;
int egr_old_y2;
int egr_old_x1;
int egr_old_x2;
int egr_oldx;               // required for move graph
int egr_oldy;               // required for move graph
int egr_yborder;            // required for move graph
int egr_hsiz;               // required for move graph
int egr_vsiz;               // required for move graph 
int egr_band_no;
int elektor_graph_scro;

char egr_msg0[MAX_MSGSIZE]; //messages in usergraph 
char egr_msg1[MAX_MSGSIZE];
char egr_msg2[MAX_MSGSIZE+3];
int egr_msg_color;          //switch for message-color in usergraph


// Prototypes
void append_freq(char *, double);
void add_guard_values(int);
void calc_Band(double);
int calc_Mux(double);
void calc_PreselVal(int);
void ChangeElektorFreq(int);
double EFreq(int);
void ftdi_I2C_Init(void);
void ftdi_I2C_Start(void);
void ftdi_I2C_Stop(void);
void ftdi_I2C_Write_Byte(int);
void I2C_Init(void);
void i2c_write_reg(int, int, int);
void init_bands(void);
int init_Elektor(void);
void Load_PreSelectFile(void);
void print_array(void);
void save_presel_to_file(void);
void SCL(int);
void SDA(int);
int Set_Elektor_Atten(int);
int Set_Elektor_Freq(double);
int Set_Elektor_Mux(int);
int Set_Elektor_PreSelect(void);
void SetupBytes(unsigned char);
void Setup_Preselector(void);
void show_elektor_parms(void);
void Start(void);
void Stop(void);
void update_presel_settings(int, int);
int valid_Presel(int);
int withinXpercent(int, int, int);
void WriteData(void *, int);


void elektor_setup(void)
{
int retval;
retval=load_ftdi_library();
if(retval != 0)return;
  // Routine to set up elektor from the soundcard setup menu.
  clear_screen();
  lir_text(3,3,"Elektor selected. (Nothing to do)");
  lir_text(3,6,press_any_key);
  await_processed_keyboard();
}


// ***************************************************************************
// ROUTINES FOR THE PROCESSING OF A 'MOVABLE' FIXED SIZE USERGRAPH
// The screenposition of the usergraph and its variables are saved
// in the parameterfile "par_xxx_elektor" ( with xxx = rx_mode)
// in the linrad directory
// ***************************************************************************
void check_egr_borders(void)  // required for move graph
{
  current_graph_minh=egr_vsiz;
  current_graph_minw=egr_hsiz;
  check_graph_placement((void*)(&egr));
  set_graph_minwidth((void*)(&egr));
}

void make_elektor_control_graph(void)
{
  pause_thread(THREAD_SCREEN);

  check_egr_borders();        // required for move graph
  scro[elektor_graph_scro].no=ELEKTOR_GRAPH;

  // These are the coordinates of the border lines.
  scro[elektor_graph_scro].x1=egr.xleft;
  scro[elektor_graph_scro].x2=egr.xright;
  scro[elektor_graph_scro].y1=egr.ytop;
  scro[elektor_graph_scro].y2=egr.ybottom;

  // Each border line is treated as a button.
  // That is for the mouse to get hold of them so the window can be moved.
  egrbutt[UG_LEFT].x1=egr.xleft;
  egrbutt[UG_LEFT].x2=egr.xleft+2;
  egrbutt[UG_LEFT].y1=egr.ytop;
  egrbutt[UG_LEFT].y2=egr.ybottom;
  egrbutt[UG_RIGHT].x1=egr.xright;
  egrbutt[UG_RIGHT].x2=egr.xright-2;
  egrbutt[UG_RIGHT].y1=egr.ytop;
  egrbutt[UG_RIGHT].y2=egr.ybottom;
  egrbutt[UG_TOP].x1=egr.xleft;
  egrbutt[UG_TOP].x2=egr.xright;
  egrbutt[UG_TOP].y1=egr.ytop;
  egrbutt[UG_TOP].y2=egr.ytop+2;
  egrbutt[UG_BOTTOM].x1=egr.xleft;
  egrbutt[UG_BOTTOM].x2=egr.xright;
  egrbutt[UG_BOTTOM].y1=egr.ybottom;
  egrbutt[UG_BOTTOM].y2=egr.ybottom-2;

  // Draw the border lines
  graph_borders((void*)&egr,7);
  egr_oldx=-10000;                           // from freq_control
  settextcolor(7);
  make_button(egr.xleft+27.5*text_width+2,-2+egr.ybottom-1*text_height/2-1,
                                     egrbutt,UG_DECREASE_OFFS_XTAL,25);
  make_button(egr.xleft+29*text_width+2,-2+egr.ybottom-1*text_height/2-1,
                                     egrbutt,UG_INCREASE_OFFS_XTAL,24); 

  make_button(egr.xleft+27.5*text_width+2,-2+egr.ybottom-3*text_height/2-1,
                                     egrbutt,UG_DECREASE_OFFS_KHZ,25);
  make_button(egr.xleft+29*text_width+2,-2+egr.ybottom-3*text_height/2-1,
                                     egrbutt,UG_INCREASE_OFFS_KHZ,24); 

  make_button(egr.xleft+27.5*text_width+2,-2+egr.ybottom-5*text_height/2-1,
                                     egrbutt,UG_DECREASE_MUX_NMBR,25);
  make_button(egr.xleft+29*text_width+2,-2+egr.ybottom-5*text_height/2-1,
                                     egrbutt,UG_INCREASE_MUX_NMBR,24);
  // presel buttons
  make_button(egr.xleft+54*text_width+2,-3+egr.ybottom-1*text_height/2-1,
                                     egrbutt,UG_DECREASE_PRESEL,25);
  make_button(egr.xleft+55.5*text_width+2,-3+egr.ybottom-1*text_height/2-1,
                                     egrbutt,UG_DECREASE_PRESELFINE,25);
  make_button(egr.xleft+57*text_width+2,-3+egr.ybottom-1*text_height/2-1,
                                     egrbutt,UG_INCREASE_PRESELFINE,24); 
  make_button(egr.xleft+58.5*text_width+2,-3+egr.ybottom-1*text_height/2-1,
                                     egrbutt,UG_INCREASE_PRESEL,24); 
  
  // Draw separator lines in usergraph
  // vertical
  lir_line(egr.xleft+30.5*text_width,egr.ytop+1.3*text_height,
                                    egr.xleft+30.5*text_width,egr.ybottom,7);
  // horizontal
  lir_line(egr.xleft,egr.ytop+1.3*text_height,egr.xright,egr.ytop+1.3*text_height,7);

  elektor_rx_freq_control();
  show_elektor_parms();
  resume_thread(THREAD_SCREEN);
}

void blankfill_egrmsg(char *msg)
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
void show_elektor_parms(void)
{ char s[80];
  char *mode = "";

  if ( elektordriver.offs_khz_band_BIG == 0 )	// if control window does not yet exist, exit.
    return;

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

  // Note that mouse control is done in the narrowband processing thread
  // and that you can not use lir_sleep to wait for hardware response here.
  hide_mouse(egr.xleft, egr.xright,egr.ytop,egr.ybottom);
  settextcolor(11);
  sprintf(s," LINRAD SDR (Elektor USB)   %s", mode);
  settextcolor(14);
  lir_pixwrite(egr.xleft+17.5*text_width,egr.ytop+0.35*text_height,s);
  settextcolor(15);
  sprintf(s," Aerial Mux:      %d ", Mux);
  lir_pixwrite(egr.xleft+.5*text_width,egr.ytop+1.5*text_height,s);

  egr_msg_color=10;
  settextcolor(15);
  sprintf(s,"Fine tune Xtal : %d        ", elektordriver.offs_hz[egr_band_no]);
  s[25]=0;
  lir_pixwrite(egr.xleft+1.5*text_width,egr.ytop+1+3.5*text_height,s);
  sprintf(s,"Freq incr (Khz): %d        ", elektordriver.offs_khz[egr_band_no]);
  s[25]=0;
  lir_pixwrite(egr.xleft+1.5*text_width,egr.ytop+2.5*text_height,s);

  sprintf(egr_msg0,"Elektor Freq: ");
  append_freq(egr_msg0,EFreq(FreqWanted)*1000);
  //sprintf(egr_msg1,"Linrad Freq:  ");
  //append_freq(egr_msg1,floor(hwfreq+0.5)*1000);
  if ( ActiveMux >= 3 && ActiveMux <= 6 )
    sprintf(egr_msg2,"Preselect: %d     %5d", ActiveMux-2, PreSelValue);
  else
    sprintf(egr_msg2,"Preselect: none    ");
  blankfill_egrmsg(egr_msg0);
  //blankfill_egrmsg(egr_msg1);
  blankfill_egrmsg(egr_msg2);
  egr_msg2[MAX_MSGSIZE-8]=0;
  settextcolor(egr_msg_color);
  lir_pixwrite(egr.xleft+31*text_width,egr.ytop+1.5*text_height,egr_msg0);
  //lir_pixwrite(egr.xleft+31*text_width,egr.ytop+2.5*text_height,egr_msg1);
  lir_pixwrite(egr.xleft+31*text_width,egr.ytop+3.5*text_height,egr_msg2);
  settextcolor(7);
}

void save_elektor_parms(void)
{ int i;
  int *sdr_pi;
  char elektordriver_parfil_name[20];
  FILE *elektordriver_file;

  // Save screenposition, mux-number, and other values
  // to the parameterfile on disk
  sprintf(elektordriver_parfil_name,"%s_hwd_egr",rxpar_filenames[rx_mode]);
  elektordriver_file = fopen(elektordriver_parfil_name,"w");
  elektordriver.egr_xleft = egr.xleft;
  elektordriver.egr_ytop = egr.ytop;
  elektordriver.aerial = Mux;
  sdr_pi = (int*)(&elektordriver);

  for(i=0; i<MAX_HWAREDRIVER_PARM; i++)
    fprintf(elektordriver_file,"%s [%d]\n",elektordriver_parm_text[i],sdr_pi[i]);

  parfile_end(elektordriver_file);
  save_presel_to_file();
}

void mouse_continue_elektor_graph(void)
{ int i, j;      // required for move graph

  // Move border lines immediately.
  // Other functions must wait until button is released.
  // Move fixed size window  based on coding in freq_control.c
  switch (mouse_active_flag-1)
  {
    case UG_TOP:
    if (egr.ytop!=mouse_y)break;
    break;
     
    case UG_BOTTOM:
    if (egr.ybottom!=mouse_y)break;
    break;
  
    case UG_LEFT:
      if (egr.xleft!=mouse_x)break;
    break;

    case UG_RIGHT:
    if (egr.xright==mouse_x)break;
    break;
  
    default:
    goto await_release;
  }

  pause_screen_and_hide_mouse();
  graph_borders((void*)&egr,0);
  if (egr_oldx==-10000)
  {
    egr_oldx=mouse_x;
    egr_oldy=mouse_y;
  }
  else
  {
    i=mouse_x-egr_oldx;
    j=mouse_y-egr_oldy;  
    egr_oldx=mouse_x;
    egr_oldy=mouse_y;
    egr.ytop+=j;
    egr.ybottom+=j;
    egr.xleft+=i;
    egr.xright+=i;
    check_egr_borders();    
    egr.yborder=(egr.ytop+egr.ybottom)>>1;
  }
  graph_borders((void*)&egr,15); 
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
      case UG_INCREASE_OFFS_XTAL: 
        if ( elektordriver.offs_hz[egr_band_no] < 255 )
        { elektordriver.offs_hz[egr_band_no]++;
          elektor_rx_freq_control();
        }
        break;

      case UG_DECREASE_OFFS_XTAL:
        if ( elektordriver.offs_hz[egr_band_no] > 0 )
        { elektordriver.offs_hz[egr_band_no]--;
          elektor_rx_freq_control();
        }
        break;
 
      case UG_INCREASE_OFFS_KHZ: 
        if ( elektordriver.offs_khz[egr_band_no] < 1000 )
        { elektordriver.offs_khz[egr_band_no]++;
          fg.passband_increment = (double) elektordriver.offs_khz[egr_band_no]/1000;
        }
        break;
 
      case UG_DECREASE_OFFS_KHZ:
        if ( elektordriver.offs_khz[egr_band_no] > 0 )
        { elektordriver.offs_khz[egr_band_no]--;
          fg.passband_increment = (double) elektordriver.offs_khz[egr_band_no]/1000;
        }
        break;
 
      case UG_INCREASE_MUX_NMBR: 
        if (Mux <= 7)
          ++Mux;
        if ( Mux == 8 )         // have to call set-elektor-freq to do the auto mux
          Set_Elektor_Freq(fg.passband_center);
        else
          Set_Elektor_Mux(Mux);
        Setup_Preselector();
        break;
 
      case UG_DECREASE_MUX_NMBR:
        if (Mux > 0)
          --Mux;
        Set_Elektor_Mux(Mux);
        Setup_Preselector();
        break;
 
      case UG_INCREASE_PRESELFINE: 
        if ( ActiveMux >=3  && ActiveMux <= 6 )
          if ( PreSelValue < 255 )
          { PreSelValue++;
            update_presel_settings(ActiveMux-3, PreSelValue);
            Set_Elektor_PreSelect();
          }
        break;

      case UG_DECREASE_PRESELFINE:
        if ( ActiveMux >=3  && ActiveMux <= 6 )
          if ( PreSelValue > 0 )
          { PreSelValue--;
            update_presel_settings(ActiveMux-3, PreSelValue);
            Set_Elektor_PreSelect();
          }
        break;
 
      case UG_INCREASE_PRESEL: 
        if (ActiveMux >=3  && ActiveMux <= 6 )
        { if (  PreSelValue <= 245 )
            PreSelValue+=10;
          else
            PreSelValue = 255;
          update_presel_settings(ActiveMux-3, PreSelValue);
          Set_Elektor_PreSelect();
        }
        break;

      case UG_DECREASE_PRESEL:
        if ( ActiveMux >=3  && ActiveMux <= 6 )
        { if ( PreSelValue >= 10 )
            PreSelValue-=10;
          else
            PreSelValue = 0;
          update_presel_settings(ActiveMux-3, PreSelValue);
          Set_Elektor_PreSelect();
        }
        break;
 
      default:        // This should never happen.    
        lirerr(211053);
        break;
    }
  }  

finish:;
  hide_mouse(egr_old_x1,egr_old_x2,egr_old_y1,egr_old_y2);
  leftpressed=BUTTON_IDLE;  
  mouse_active_flag=0;
  graph_borders((void*)&egr,0);
  lir_fillbox(egr_old_x1,egr_old_y1,egr_old_x2-egr_old_x1,egr_old_y2-egr_old_y1,0);
  make_elektor_control_graph();
  egr_oldx=-10000; 
}

void new_elektor_presel(void)
{
  if ( numinput_int_data >= 0 && numinput_int_data <= 255 )
  {
    PreSelValue = numinput_int_data;
    Set_Elektor_PreSelect();
    pause_thread(THREAD_SCREEN);
    show_elektor_parms();
    resume_thread(THREAD_SCREEN);
  }
}

void new_elektor_mux(void)
{
  if ( numinput_int_data >= 0 && numinput_int_data <= 7 )
  {
    Mux = numinput_int_data;
    Set_Elektor_Mux(Mux);
    pause_thread(THREAD_SCREEN);
    show_elektor_parms();
    resume_thread(THREAD_SCREEN);
  }
  else if ( numinput_int_data == 8 )
  {
    Mux = numinput_int_data;
    Set_Elektor_Freq(fg.passband_center);
  }
  Setup_Preselector();
}

void new_elektor_offs_xtal(void)
{
  if ( numinput_int_data >= 0 && numinput_int_data <= 255 )
  {
    elektordriver.offs_hz[egr_band_no]=numinput_int_data;
    pause_thread(THREAD_SCREEN);
    show_elektor_parms();
    resume_thread(THREAD_SCREEN);
    elektor_rx_freq_control();
  }
}

void new_elektor_offs_khz(void)
{
  if ( numinput_int_data < 1 )
    numinput_int_data = 1;
  if ( numinput_int_data > 1000 )
    numinput_int_data = 1000;

  elektordriver.offs_khz[egr_band_no]=numinput_int_data;
  fg.passband_increment = (double) elektordriver.offs_khz[egr_band_no]/1000;
  pause_thread(THREAD_SCREEN);
  show_elektor_parms();
  resume_thread(THREAD_SCREEN);
}

void mouse_on_elektor_graph(void)
{ int event_no;

  // First find out if we are on a button or border line.
  for(event_no=0; event_no<MAX_EGRBUTT; event_no++)
  {
    if( egrbutt[event_no].x1 <= mouse_x && 
        egrbutt[event_no].x2 >= mouse_x &&      
        egrbutt[event_no].y1 <= mouse_y && 
        egrbutt[event_no].y2 >= mouse_y) 
    {
      egr_old_y1=egr.ytop;
      egr_old_y2=egr.ybottom;
      egr_old_x1=egr.xleft;
      egr_old_x2=egr.xright;
      mouse_active_flag=1+event_no;
      current_mouse_activity=mouse_continue_elektor_graph;
      return;
    }
  }

  mouse_active_flag=1;

  if(mouse_x > (egr.xleft+50*text_width) && mouse_x < (egr.xleft+54*text_width))
  {
    numinput_xpix=egr.xleft+50*text_width;
    if( (mouse_y > (egr.ytop+3.8*text_height))  &&  Mux >=3  && Mux <= 6 )
    {
      numinput_ypix=egr.ytop+3.6*text_height;
      numinput_chars=3;
      erase_numinput_txt();
      numinput_flag=FIXED_INT_PARM;
      par_from_keyboard_routine=new_elektor_presel;     // PreSelector tune
    }
  }
  if(mouse_x > egr.xleft+18*text_width && mouse_x < egr.xleft+25*text_width)
  {
    numinput_xpix=egr.xleft+18.5*text_width;
    if(mouse_y > (egr.ytop+3.5*text_height))
    {
      numinput_ypix=egr.ytop+3.6*text_height;
      numinput_chars=4;
      erase_numinput_txt();
      numinput_flag=FIXED_INT_PARM;
      par_from_keyboard_routine=new_elektor_offs_xtal;  // Xtal fine tune
    }
    else if(mouse_y > (egr.ytop+2.5*text_height))
    {
      numinput_ypix=egr.ytop+(2.4*text_height)+2;
      numinput_chars=5;
      erase_numinput_txt();
      numinput_flag=FIXED_INT_PARM;
      par_from_keyboard_routine=new_elektor_offs_khz;   // vfo increment
    }  
    else if(mouse_y > (egr.ytop+1.5*text_height))
    {
      numinput_ypix=egr.ytop+(1.4*text_height)+2;
      numinput_chars=1;
      erase_numinput_txt();
      numinput_flag=FIXED_INT_PARM;
      par_from_keyboard_routine=new_elektor_mux;        // aerial multiplexor
    }  
  }
  else
  {
    // If we did not select a numeric input by setting numinput_flag
    // we have to set mouse_active flag.
    // Set the routine to mouse_nothing, we just want to
    // set flags when the mouse button is released.    
    current_mouse_activity=mouse_nothing;
    mouse_active_flag=1;
  }
}

int init_elektor_control_window(void)
{ int i;
  int errcod;
  int *sdr_pi;
  char elektordriver_parfil_name[20];
  FILE *elektordriver_file;

  // Get values from parameterfile
  sprintf(elektordriver_parfil_name,"%s_hwd_egr",rxpar_filenames[rx_mode]);
  errcod = read_sdrpar(elektordriver_parfil_name, MAX_HWAREDRIVER_PARM, 
                       elektordriver_parm_text, (int*)((void*)&elektordriver));
  if (errcod != 0)
  {
    elektordriver_file = fopen(elektordriver_parfil_name,"w");
    //set_default_parameter values
    elektordriver.egr_xleft = 60*text_width;                    
    elektordriver.egr_ytop = (screen_last_line-4)*text_height; 
    elektordriver.aerial = 1;
    for(i=0; i<NO_OF_BANDS; i++)
    {
      elektordriver.offs_hz[i] = 127;       // middle ~ of range 0-255
      elektordriver.offs_khz[i] = 20;       // default of 20kHz increments          
    }
    sdr_pi = (int*)(&elektordriver);
    for( i=0; i<MAX_HWAREDRIVER_PARM; i++ )
    {
      fprintf(elektordriver_file,"%s [%d]\n",elektordriver_parm_text[i],sdr_pi[i]);
    }
    parfile_end(elektordriver_file);
  }

  Load_PreSelectFile();
  Mux = elektordriver.aerial;

  // Need to setup some stuff for Elektor before usergraphic is drawn
  if(Set_Elektor_Freq(fg.passband_center) != EXIT_SUCCESS)return EXIT_FAILURE;
  if(Set_Elektor_Atten(fg.gain) != EXIT_SUCCESS)return EXIT_FAILURE;
  egr.xleft = elektordriver.egr_xleft;
  egr.ytop = elektordriver.egr_ytop;
  egr_msg_color = 10;
  egr_msg1[0] = 0;
  egr_hsiz = (30+MAX_MSGSIZE)*text_width;  //  WIDTH OF USERGRAPH
  egr_vsiz = (4.8*text_height);            //  HEIGHT OF USERGRAPH
  egr.xright = egr.xleft+egr_hsiz;
  egr.ybottom = egr.ytop+egr_vsiz;
  if(rx_mode < MODE_TXTEST)
  {
    elektor_graph_scro = no_of_scro;
    make_elektor_control_graph();
    no_of_scro++;
    if(no_of_scro >= MAX_SCRO)
      lirerr(89);
  }
  elektordriver.offs_khz_band_BIG = 1;  // params setup OK now.
return EXIT_SUCCESS;
}

// ***************************************************************************
// END ROUTINES FOR THE PROCESSING OF A 'MOVABLE' FIXED SIZE USERGRAPH
// ***************************************************************************


// Append frequency formatted in Hz or kHz to the end of
// the already present text in the MAX_MSGSIZE buffer.
void append_freq(char *buffer, double fq)
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

// Is the given entry within X% of the current frequency or Preselect value?
// Use simple division - /20 for 5%, /10 for 10%, /32 for 3%...
int withinXpercent(int p, int i, int freq)
{ int ret;

  ret = ( abs(PreSelData[p].freq[i]-freq) < (freq/(NUM_SETTINGS+6)) );
//  fprintf( stderr,"within(%d,%d)=%d\n",pos,freq,ret);
  return ret;
}

// Update preselector tuning value in memory array
void update_presel_settings(int p,int val)
{ int i, j, freq;

  freq = floor(hwfreq);
 //fprintf( stderr,"up-presel(%d,%d) ",p, val);
  // If value is close to zero and array is full, replace first position
  if  ( val <= (256 / (NUM_SETTINGS + 6)) && PreSelData[p].freq[NUM_SETTINGS] != 0 )
  { PreSelData[p].val[1] = val;
    PreSelData[p].freq[1] = freq;
    for ( j=2; j<=NUM_SETTINGS; ++j )
      if ( PreSelData[p].freq[j] <= freq )
      { PreSelData[p].freq[j] = 0;
        PreSelData[p].val[j] = 0;
      }
   //fprintf( stderr,"PreSelect: 1st replaced\n");
    add_guard_values(p);
    PreselUpdated = 1;
    return;
  }
  else    // else if value is close to limit and array is full, replace last
  if ( (val >= 256 - (256 / (NUM_SETTINGS + 6))) && PreSelData[p].freq[NUM_SETTINGS] != 0 )
  { PreSelData[p].val[NUM_SETTINGS] = val;
    PreSelData[p].freq[NUM_SETTINGS] = freq;
    for ( j=NUM_SETTINGS-1; j>=1; --j )
      if ( PreSelData[p].freq[j] >= freq )
      { PreSelData[p].freq[j] = 0;
        PreSelData[p].val[j] = 0;
      }
   //fprintf( stderr,"PreSelect: last replaced\n");
    add_guard_values(p);
    PreselUpdated = 1;
    return;
  }

  for ( i=1; i<=NUM_SETTINGS; ++i )
  {
    if ( withinXpercent(p,i,freq) )
    {
      PreSelData[p].freq[i] = freq;
      PreSelData[p].val[i] = val;
    //fprintf( stderr,"PreSelect: replaced %d\n",i);
      add_guard_values(p);
      PreselUpdated = 1;
      return;
    }

    if ( PreSelData[p].freq[i] == 0 )     // no entry yet?
    {
      PreSelData[p].freq[i] = freq;
      PreSelData[p].val[i] = val;
    //fprintf( stderr,"PreSelect: new entry %d\n",i);
      add_guard_values(p);
      PreselUpdated = 1;
      return;
    }

    if ( PreSelData[p].freq[i] > freq )   // Need to insert?
    {
      for ( j=NUM_SETTINGS; j>i; --j )
      { PreSelData[p].val[j] = PreSelData[p].val[j-1];
        PreSelData[p].freq[j] = PreSelData[p].freq[j-1];
      }
      PreSelData[p].val[i] = val;
      PreSelData[p].freq[i] = freq;
    //fprintf( stderr,"PreSelect: inserted %d\n",i);
      add_guard_values(p);
      PreselUpdated = 1;
      return;
    }
  }
  
  // Not found anywhere to put it, force it
  if ( freq < PreSelData[p].freq[1] )
  {
    PreSelData[p].val[1] = val;
    PreSelData[p].freq[1] = freq;
   //fprintf( stderr,"PreSelect: forced 1\n");
    add_guard_values(p);
    PreselUpdated = 1;
  }
  else if ( freq > PreSelData[p].freq[NUM_SETTINGS] )
  {
    PreSelData[p].val[NUM_SETTINGS] = val;
    PreSelData[p].freq[NUM_SETTINGS] = freq;
   //fprintf( stderr,"PreSelect: force ten\n");
    add_guard_values(p);
    PreselUpdated = 1;
  }
}

// Save current preselector settings to a file.
void save_presel_to_file(void)
{ int j, p;
  FILE *filep;

  if ( PreselUpdated  &&  Mux != 8 )  // Only save changes in manual mode
  {
    if ( (filep=fopen(preselector_parfile_name,"w")) != NULL )
    {
      // could write out a header for the file, e.g. file documentation?
      for ( p=0; p<4; ++p )
      {
        fprintf(filep, "In%d\n",p+4);
        for ( j=1; j<=NUM_SETTINGS; ++j )
          if ( PreSelData[p].freq[j]  &&  (PreSelData[p].freq[j] != PreSelData[p].freq[j-1]) )
            fprintf(filep, "%d %d\n",PreSelData[p].freq[j], PreSelData[p].val[j]);
      }
      // could write out a trailer?
      fclose(filep);
    }  
    else
      fprintf( stderr,"Writing to %s failed: errno=%d\n", preselector_parfile_name, errno);
  }
  PreselUpdated = 0;
}

//************************************************************
// This is the data entered in the rx gain control box.
// int fg.gain_increment = The increment in dB for clicking
// the arrows in the gain control box.
// The global fg_new_band can be used to allow for different
// settings on different bands. 
//
// Uses global data:
//   int fg.gain
//************************************************************
void elektor_rx_amp_control(void)
{
  fg.gain_increment = 10;

  // Elektor has an IF attenuator with 3 settings, 0, -10, -20dB.
  if ( fg.gain > -5 )
  { fg.gain = 0;
    Set_Elektor_Atten(0);
  }
  else if (fg.gain > -15)
  { fg.gain = -10;
    Set_Elektor_Atten(-10);
  }
  else
  { fg.gain = -20;
    Set_Elektor_Atten(-20);
  }
}

void update_elektor_rx_frequency()
{ static double old_freq=0.0;

//fprintf( stderr,"update-freq() cntrfreq=%f, hwfreq=%f\n",fg.passband_center,hwfreq);
  // This routine is called from the screen thread.
  if ( fabs(old_freq-hwfreq) > 0.001 )
  { Setup_Preselector();
    old_freq = hwfreq;
  show_elektor_parms();
  }
}

void elektor_rx_freq_control(void)
{ static double old_passband = 0;
  static int Xtal=-1;
//fprintf( stderr,"freq-control: passbnd=%f, hwfreq=%f\n",fg.passband_center,hwfreq);

  fg.passband_direction = 1;
  fft1_direction = fg.passband_direction;
  if ( elektordriver.offs_khz_band_BIG == 0 )  // not yet read in params.
    return;

  fg.passband_increment = (double) elektordriver.offs_khz[egr_band_no]/1000;
  if ( fg.passband_center < 0.13 )
    fg.passband_center = 0.13;
  if ( fg.passband_center > 30.0 )
    fg.passband_center = 30.0;

  if ( (fabs(fg.passband_center-old_passband) >= 0.001)  ||  (Xtal != elektordriver.offs_hz[egr_band_no]) )
  {
    Set_Elektor_Freq(fg.passband_center);
    old_passband = hwfreq;
    Xtal = elektordriver.offs_hz[egr_band_no];
  }
  save_elektor_parms();
}

// ******************************************************************
// ******************************************************************
// ******************************************************************
//    Following based on ElektorSDR4Linux.c
// ******************************************************************
// ******************************************************************
// ******************************************************************

int init_Elektor(void)
{ 
int f;
Fdti_Data_Count = 0;
Write_Pins = 0;
f = ftdi_init(&ftdic);
if (f<0)
  {
  fprintf( stderr, "ftdi_init failed: %d (%s)\n",
                        f, ftdi_get_error_string(&ftdic));
  return EXIT_FAILURE;
  }
f = ftdi_usb_open(&ftdic, 0x0403, 0x6001);
if(f<0)
  {
  fprintf( stderr, "ftdi_usb_open failed: %d (%s)\n",
                         f, ftdi_get_error_string(&ftdic));
  return EXIT_FAILURE;
  }
  
f = ftdi_set_bitmode(&ftdic, 0xFF, BITMODE_BITBANG);
if (f<0)
  {
  fprintf( stderr, "ftdi_set_bitmode failed: %d (%s)\n", f, 
                                 ftdi_get_error_string(&ftdic));
  return EXIT_FAILURE;
  }
f = ftdi_set_baudrate(&ftdic, 38400/2);        // 38400 is too fast for preselector
  if (f<0)
  {
  fprintf( stderr, "ftdi_set_baudrate failed: %d (%s)\n",
                                        f, ftdi_get_error_string(&ftdic));
  return EXIT_FAILURE;
  }
f = ftdi_read_pins(&ftdic, &FT_port);
if (f<0)
  {
  fprintf( stderr, "ftdi_read_pins failed: %d (%s)\n",
                                         f, ftdi_get_error_string(&ftdic));
  return EXIT_FAILURE;
  }
return EXIT_SUCCESS;
}

int Send_To_FTDI(void)
{ 
int f;
if (!(Fdti_Data_Count>0) && Write_Pins)
  {
  FtdiData[Fdti_Data_Count] = FT_port;
  Fdti_Data_Count++;
  }
  f = ftdi_write_data_set_chunksize(&ftdic, 16384);
if (f<0)
  {
  fprintf( stderr, "ftdi_write_data_set_chunksize: %d (%s)\n",
                                        f, ftdi_get_error_string(&ftdic));
  Write_Pins = 0;
  return EXIT_FAILURE;
  }

  f = ftdi_write_data(&ftdic, FtdiData, Fdti_Data_Count);
  if (f<0)
  {
    fprintf( stderr,"ftdi_write failed: %d (%s)\n",
            f, ftdi_get_error_string(&ftdic));
  }
//  fprintf( stderr,"Number of bytes written: %d\n", f);
  
  ftdi_usb_close(&ftdic);
  ftdi_deinit(&ftdic);

  Fdti_Data_Count = 0;
  Write_Pins = 0;
  
  return EXIT_SUCCESS;
}

void ChangeElektorFreq(int IICadr)
{ int Pump = 1;
  unsigned char R40, R41, R42;
  
  if (FreqWanted>479)
  {
    Pump = 2;
  }
  if (FreqWanted>639)
  {
    Pump = 3;
  }
  if (FreqWanted>799)
  {
    Pump = 4;
  }
  
  R40 = (FreqWanted/2-4)/256+4*Pump+192;
  R41 = (FreqWanted/2-4)&255;
  if (VFOhigh)
  {
    R42 = 23 + 128*(FreqWanted%2);
  }
  else
  {
    R42 = 48 + 128*(FreqWanted%2);
  }
  
  I2C_Init();
  
  Start();
  SetupBytes(IICadr);
  SetupBytes(64);
  SetupBytes(R40);
  SetupBytes(R41);
  SetupBytes(R42);
  Stop();
  
  Start();
  SetupBytes(IICadr);
  SetupBytes(12);
  SetupBytes(Div1N);
  Stop();
  
  Start();
  SetupBytes(IICadr);
  SetupBytes(19);
  SetupBytes(elektordriver.offs_hz[egr_band_no]);
  Stop();
}

void I2C_Init(void)
{
  SCL(1);
  SDA(1);
}
  
void Start(void)
{
  SDA(0);
  SCL(0);
}

void Stop(void)
{
  SCL(0);
  SDA(0);
  SCL(1);
  SDA(1);
}

void SDA(int TXD)
{
  if (TXD)
  {
    FT_port = FT_port|1;
  }
  else
  {
    FT_port = FT_port&(255-1);
  }
  FtdiData[Fdti_Data_Count] = FT_port;
  Fdti_Data_Count++;
}

void SCL(int RXD)
{  
  if (RXD)
  {
    FT_port = FT_port|2;
  }
  else
  {
    FT_port = FT_port&(255-2);
  }
  FtdiData[Fdti_Data_Count] = FT_port;
  Fdti_Data_Count++;
}

void SetupBytes(unsigned char Value)
{ int i = 128;
  
  while (i>0)
  {
    if (Value&i)
    {
      SDA(1);
    }
    else
    {
      SDA(0);
    }
    SCL(1);
    SCL(0);
    i = i/2;
  }
  SDA(1);
  SCL(1);
  SCL(0);
}

void init_bands(void)
{
  bands_struct.VFOhigh[0] = 1;
  bands_struct.VFOhigh[1] = 0;
  bands_struct.VFOhigh[2] = 0;
  bands_struct.VFOhigh[3] = 0;
  bands_struct.VFOhigh[4] = 0;
  bands_struct.VFOhigh[5] = 0;
  
  bands_struct.Min[0] = 400;
  bands_struct.Min[1] = 400;
  bands_struct.Min[2] = 400;
  bands_struct.Min[3] = 400;
  bands_struct.Min[4] = 400;
  bands_struct.Min[5] = 260;
  
  bands_struct.Max[0] = 1200;
  bands_struct.Max[1] = 1600;
  bands_struct.Max[2] = 1600;
  bands_struct.Max[3] = 1600;
  bands_struct.Max[4] = 1600;
  bands_struct.Max[5] = 1600;
  
  bands_struct.Div1N[0] = 4;
  bands_struct.Div1N[1] = 5;
  bands_struct.Div1N[2] = 10;
  bands_struct.Div1N[3] = 25;
  bands_struct.Div1N[4] = 50;
  bands_struct.Div1N[5] = 100;

  bands_struct.High[0] = 30.0;
  bands_struct.High[1] = 16.0;
  bands_struct.High[2] =  8.0;
  bands_struct.High[3] =  3.2;
  bands_struct.High[4] =  1.6;
  bands_struct.High[5] =  0.8;
}

int Set_Elektor_Atten(int atten)                // 0, 10, 20.
{ 
static int old_Atten=-1;
if ( atten == old_Atten )
return EXIT_SUCCESS;
if(init_Elektor() != EXIT_SUCCESS)return EXIT_FAILURE;
FT_port = FT_port&0x1F;
switch(atten)
  { 
  case 0:   FT_port = FT_port+32*0; break;
  case -10: FT_port = FT_port+32*1; break;
  case -20: FT_port = FT_port+32*2; break;
  default:  return EXIT_SUCCESS;
  }
Write_Pins = 1;
if(Send_To_FTDI() != EXIT_SUCCESS)return EXIT_FAILURE;
old_Atten = atten;
return EXIT_SUCCESS;
}

int Set_Elektor_Mux(int new)                    // 0-8, where 8=automatic.
{ static int old_Mux = -1;
 //fprintf( stderr,"SetMux %d, Active=%d\n",new,ActiveMux);

  if ( (new != old_Mux)  &&  (new != 8) )
  {
  if(init_Elektor() != EXIT_SUCCESS)return EXIT_FAILURE;

    FT_port = FT_port&0x63;
    FT_port = FT_port+4*new;
    Write_Pins=1;
    if(Send_To_FTDI() != EXIT_SUCCESS)return EXIT_FAILURE;
    old_Mux = ActiveMux = new;
  }
return EXIT_SUCCESS;
}

int Set_Elektor_Freq(double CenterFreq)         // CenterFreq in MHz
{
//fprintf( stderr,"CenterFreq=%f, hwfreq=%f\n",CenterFreq,hwfreq);

  // Limit the requested frequency to what is possible on Elektor.
  if ( CenterFreq < 0.13 )                       // min freq is 130kHz.
    CenterFreq = 0.13;
  if ( CenterFreq > 30.0 )                       // max freq is 30MHz.
    CenterFreq = 30.0;

  calc_Band(CenterFreq);
  Set_Elektor_Mux(calc_Mux(CenterFreq));
  Setup_Preselector();
  if ( VFOhigh )
    FreqWanted = (int)((CenterFreq)*4000/(400/Div1N)+0.5);
  else
    FreqWanted = (int)((CenterFreq)*4000/(200/Div1N)+0.5);

//fprintf( stderr,"FreqWanted:%fkHz, Band:%d, VFOhigh:%d, Div1N:%d\n",EFreq(FreqWanted),egr_band_no,VFOhigh,Div1N);
  if(init_Elektor() != EXIT_SUCCESS)return EXIT_FAILURE;
  ChangeElektorFreq(IIC_RAM);
    if(Send_To_FTDI() != EXIT_SUCCESS)return EXIT_FAILURE;
return EXIT_SUCCESS;
}

int Set_Elektor_PreSelect(void)
{ 
static int old_value = 0;

  //fprintf( stderr,"SetPreSel %d,old_sel %d\n",PreSelValue, old_value);
  if ( old_value != PreSelValue )
  {
  if(init_Elektor() != EXIT_SUCCESS)return EXIT_FAILURE;
    I2C_Init();
    Start();
    SetupBytes(144);
    SetupBytes(64);
    SetupBytes(PreSelValue);        // value from 0 to 255
    Stop();
    if(Send_To_FTDI() != EXIT_SUCCESS)return EXIT_FAILURE;
    old_value = PreSelValue;
  }
return EXIT_SUCCESS;
}

double EFreq(int F)
{
  if (VFOhigh)
    return (double)(F*400/Div1N)/4;

  return (double)(F*200/Div1N)/4;
}

void calc_Band(double freq)
{ static int init_done=0;
  int old_band_no;

  if ( init_done == 0 )
  {
    init_bands();
    init_done = 1;
  }

  old_band_no = egr_band_no;

  if      ( freq <= bands_struct.High[5] )  egr_band_no = 5;        // 800kHz
  else if ( freq <= bands_struct.High[4] )  egr_band_no = 4;        // 1.6MHz
  else if ( freq <= bands_struct.High[3] )  egr_band_no = 3;
  else if ( freq <= bands_struct.High[2] )  egr_band_no = 2;
  else if ( freq <= bands_struct.High[1] )  egr_band_no = 1;
  else                                      egr_band_no = 0;

  if ( old_band_no != egr_band_no )
    fg.passband_increment = (double) elektordriver.offs_khz[egr_band_no]/1000;

  Div1N = bands_struct.Div1N[egr_band_no];
  VFOhigh = bands_struct.VFOhigh[egr_band_no];
}

// Mux settings: 0 - wideband with a simple audio shunt choke
//               1 - Medium Wave, low pass filter at 1.6Mhz
//               2 - Short Wave, via R-C high pass at 1.6MHz
//               3 - PC1 on the Elektor pcb - or PreSel 1 if connected
//               4 - unconnected - or PreSel 2
//               5 - unconnected - or PreSel 3
//               6 - unconnected - or PreSel 4
//               7 - 5MHz calibration signal at S9 +40dB.
//               8 - Automatic.  Mux settings are chosen according to
//                               a suitable pre-selector front-end, or
//                               to mux1 for < 1.6MHz, or mux2 for >1.6MHz.
int calc_Mux(double freq)
{
  int i;

  //fprintf( stderr,"calc_Mux(%f): oldMux=%d\n",freq,Mux);
  if ( Mux != 8 )       // if manual mode selected,
    return Mux;         // just return the current setting.

  // Look through the PreSelector settings for a close match
  for ( i=0; i<4; ++i )
    if ( valid_Presel(i) )
      return i + 3;     // found mux positon 3,4,5 or 6

  if ( freq < 1.6 )     // Use the MW setting for < 1.6MHz 
    return 1;

  return 2;             // Use the SW setting for all others.
}

void Setup_Preselector()
{
//fprintf( stderr,"Setup_Preselector: ActMux=%d\n",ActiveMux);
  if ( ActiveMux >= 3  &&  ActiveMux <= 6 )
  {
    calc_PreselVal(ActiveMux-3);
    Set_Elektor_PreSelect();
  }
}

// Does this preselector channel include the current hardware frequency
int valid_Presel(int presel)
{ int ifreq;
float t1;
t1= floor(hwfreq);
  ifreq = (int)t1;
  if ( ifreq == 0 )	// initialy, there is no hardware freq value.
    ifreq = floor(fg.passband_center)*1000;

  if ( PreSelData[presel].freq[0] != 0            // if there is data in this array
  &&   PreSelData[presel].freq[0] <= ifreq        // and it brackets the wanted
  &&   PreSelData[presel].freq[NUM_SETTINGS+1] >= ifreq )     // frequency, say yes.
    return 1;

  return 0;
}

// Look through PreSelector data array for a pair of frequencies that
// bracket the current hardware freq, and interpolate between them.
void calc_PreselVal(int presel)
{ int ifreq,j;
  int fspan, vspan, fdiff;
float t1;
 t1=floor(hwfreq);
  ifreq = (int) t1;
  if ( ifreq == 0 )
    ifreq = floor(fg.passband_center)*1000;

 //fprintf( stderr,"calcPSel(%d), ifreq=%d ",presel,ifreq);
  if ( ifreq == 0 )
    return;

  // only do calculations if necessary.
  if ( ifreq <=  PreSelData[presel].freq[0] )
    PreSelValue = 0;
  else if ( ifreq >=  PreSelData[presel].freq[NUM_SETTINGS+1] )
    PreSelValue = 255;
  else
  {
    for ( j=1; j<=NUM_SETTINGS+1; ++j )
      if ( PreSelData[presel].freq[j] >= ifreq  ||  PreSelData[presel].freq[j] == 0 )
        break;
    
    fspan = PreSelData[presel].freq[j] - PreSelData[presel].freq[j-1];
    vspan = PreSelData[presel].val[j] - PreSelData[presel].val[j-1];
    if ( PreSelData[presel].freq[j] == 0 )
    { fspan = PreSelData[presel].freq[NUM_SETTINGS+1] - PreSelData[presel].freq[j-1];
      vspan = PreSelData[presel].val[NUM_SETTINGS+1] - PreSelData[presel].val[j-1];
    }
    fdiff = ifreq - PreSelData[presel].freq[j-1];

    // Simple linear interpolation between low and high freqs
    if ( fspan > 0 )
      PreSelValue = PreSelData[presel].val[j-1] + ((vspan * fdiff)/fspan);
    else
      PreSelValue = PreSelData[presel].val[j-1];
    //fprintf( stderr," vspan=%d fdiff=%d fspan=%d ",vspan,fdiff,fspan);
  }
  //fprintf( stderr," val=%d\n",PreSelValue);
}


// ******************************************************************
// ******************************************************************
//     Load the Pre-Selector calibrations
// ******************************************************************
// ******************************************************************
//
// A file of Pre-Selector settings is maintained, one section for
// each of the four possible selector channels.
// Each section can contain upto ten pairs of integers -
// frequency in kiloHertz, and a number between 0 and 255 as the
// pre-selector setting.
// The file format is fairly tolerant of whitespace, and can
// look like this:-
//
// In4
// 1234
// 0
// 1456
// 13
// 1689
// 59
// 2345
// 250
//
// In5
// 2200
// 11
// 3000
// 56
// ... etc.
//
// or this:-
// In4
// 1234 0
// 1456 13
// 1689 59
// ...
// 2345 250
//
// or this:-
// In5 2200 11 3000 56 4000 88 ...
//

// Add a limit entry at each end of the given array (simplifies later processing)
void add_guard_values(int p)
{ int i, j=NUM_SETTINGS+1, hival=0, valdiff, hifreq=0, freqdiff;

  // Find highest freq in array
  for ( i=1; i<=NUM_SETTINGS; ++i )
    if ( PreSelData[p].freq[i] > hifreq )
    { hifreq = PreSelData[p].freq[i];
      hival = PreSelData[p].val[i];
    }

  // Calculate the frequencies for the presel values 0 and 255.
  valdiff = hival - PreSelData[p].val[1];
  freqdiff = hifreq - PreSelData[p].freq[1];

  if ( valdiff > 0  &&  freqdiff > 0 )
  {           // The order of execution is important for accuracy in following lines
    PreSelData[p].freq[0] = PreSelData[p].freq[1] - ((PreSelData[p].val[1]*freqdiff)/valdiff);
    if ( PreSelData[p].freq[0] < 0 )
      PreSelData[p].freq[0] = 0;
    PreSelData[p].val[0] = 0;
    PreSelData[p].freq[j] = PreSelData[p].freq[1] + ((255-PreSelData[p].val[1])*freqdiff/valdiff);
    PreSelData[p].val[j] = 255;
  }
  else        // No range of values present - so make some arbitrary ones.
  {
    PreSelData[p].freq[0] = PreSelData[p].freq[1]/2;
    PreSelData[p].val[0] = 0;
    PreSelData[p].freq[j] = PreSelData[p].freq[1]*2;
    PreSelData[p].val[j] = 255;
  }
}

void Load_PreSelectFile(void)
{ struct stat statbuf;
  int fd, i=-1, size;
  char *data, *ptr;
  
  if ( stat(preselector_parfile_name, &statbuf) != 0 )
  { fprintf( stderr,"Cannot find %s\n",preselector_parfile_name);
    return;
  }

  size = statbuf.st_size;
  data = malloc(size+1);
  if ( data == NULL )
  { fprintf( stderr,"Not enough memory for %s\n",preselector_parfile_name);
    return;
  }

  fd = open(preselector_parfile_name, O_RDONLY);
  if ( fd == -1 )
  { fprintf( stderr,"Cannot open %s\n",preselector_parfile_name);
    free(data);
    return;
  }

  if ( read(fd, data, size) != size )
  { fprintf( stderr,"Cannot read %s\n",preselector_parfile_name);
    close(fd);
    free(data);
    return;
  }
  close(fd);
  data[size]=0;

  // Scan the data into the PreSel values arrays.
  ptr = data;
  while (  ptr!=NULL && ptr<(data + size) )
  {
    if ( strncmp(ptr, "In", 2) == 0 )        // start of a section
    {
      if (ptr[2] == '4' )             i = 0; // 1st front end
      else if (ptr[2] == '5' )        i = 1; // 2nd
      else if (ptr[2] == '6' )        i = 2;
      else if (ptr[2] == '7' )        i = 3;
      else
      { ptr += 3;
        continue;                            // not a valid entry, loop again
      }

      ptr += 3;                              // move past the section identifier
      if ( i != -1 )                         // found a valid front-end entry
      { 
        sscanf(ptr, " %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
               &PreSelData[i].freq[1], &PreSelData[i].val[1],
               &PreSelData[i].freq[2], &PreSelData[i].val[2],
               &PreSelData[i].freq[3], &PreSelData[i].val[3],
               &PreSelData[i].freq[4], &PreSelData[i].val[4],
               &PreSelData[i].freq[5], &PreSelData[i].val[5],
               &PreSelData[i].freq[6], &PreSelData[i].val[6],
               &PreSelData[i].freq[7], &PreSelData[i].val[7],
               &PreSelData[i].freq[8], &PreSelData[i].val[8],
               &PreSelData[i].freq[9], &PreSelData[i].val[9],
               &PreSelData[i].freq[10], &PreSelData[i].val[10]);

	add_guard_values(i);
      }
    }
    ptr = strstr(ptr, "In");
  }
  free(data);
 // print_array();
}

/*
void print_array(void)
{ int i;
// debug printout
  fprintf( stderr,"\n");
  for ( i=0; i<NUM_SETTINGS+2; ++i )
    fprintf( stderr,"%2d In4: %5d %5d  In5: %5d %5d  In6: %5d %5d  In7: %5d %5d\n",
          i,
          PreSelData[0].freq[i], PreSelData[0].val[i],
          PreSelData[1].freq[i], PreSelData[1].val[i],
          PreSelData[2].freq[i], PreSelData[2].val[i],
          PreSelData[3].freq[i], PreSelData[3].val[i]);

}
*/
