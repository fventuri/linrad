
// program users_tr.c for CAT control with Linrad
// supported transeivers: ELCRAFT K3
//                        ICOM    IC-275/IC-706
//                        KENWOOD TS-850/TS-2000
//                        YAESU   FT-450/FT-736/FT-817/FT-897/FT-920/FT-950/FT-1000/FT-2000
//                        TENTEC  ORION I or II
// Runs under Windows and Linux 
// Based on input from DL3IAE/IK7EZN/K1JT/ON4IY/ON7EH/W3SZ/YO4AUL/NH7O
// with the support of Leif SM5BSZ
// All errors are mine
// Use at your own risk and enjoy
// Pierre/ON5GN
//
// UPDATE HISTORY
//
//   4 may   2007  first version for linrad 02.31
//                 FT-736 tested by DL3IAE
//                 TS-850 tested by ON7EH
//                 ICOM tested by IK7EZN
//  22 july  2008  update for linrad-02.48
//                 added FT-897, tested by YO4AUL and ON4KHG
//  23 may   2010  bug correction and documentation updates
//   8 july  2010  added FT-920 (NH7O)
//  20 july  2010  added FT-817, tested by DL3IAE
//                 changed parameter file (uses now the Linrad standards)
//  28 aug   2011  added Elecraft K3 (NH7O)
//   4 nov   2013  Initial RTS status in the transceiver structure and
//                 frequency int k changed to double kxx.
//  10 feb   2015  added TenTec Orion I or II (W5PNY) 
//
// INSTALLATION
//
// To install under Linux for Linux:
//   download lirxx-xx.tbz and extract it to your linrad directory
//   rename users_tr.c to users_hwaredriver.c in your linrad directory 
//   run ./configure and  make xlinrad (or make linrad) from your linrad
//   directory
//
// To generate a windows version under Linux:
//   install mingw32 from Leif's website
//   download lirxx-xx.tbz and extract it to your linrad directory
//   rename  users_tr.c to wusers_hwaredriver.c in your linrad directory 
//   run ./configure and  make linrad.exe from your linrad directory
//
// To generate a windows version under Windows:
//   Install mingw and nasm from Leif's website in the C:\MinGW  directory
//   Download lirxx-xx.zip and unpack the linrad sources in your linrad
//   directory
//   Rename  users_tr.c to wusers_hwaredriver.c in your linrad directory 
//   Start a MS-DOS Command Window on your computer: click the Windows Start
//   menu and enter the "cmd.exe" command into the run box
//   In the MS-DOS Command Window issue a "cd" command to your linrad directory
//   followed by a "configure.exe" and "make.bat" command
//
// USERGUIDE
//
// Moving the usergraph:
//   Move the usergraph on the screen by hitting the left side of the graph
//   with the mousepointer, holding the left mouse button down, and dragging
//   it to the desired position. 
//
// Setting the VFO frequency:
//   Select first the transceiver-type with the arrow buttons in the upper
//   right corner of the usergraph.
//
//   Next select the serial port number with the arrow buttons next to the
//   SERIAL PORT field:
//   Under Windows one can select COM0 to COM999.
//   Under Linux one can select /devttyS0 to /dev/ttyS3
//   or /dev/ttyUSB0 to /dev/ttyUSB3 for USB-serial devices.
//   Under BSD one can select /dev/ttyd0 to /dev/ttyd7.
//
//   Enter the values for Offset-raw (Khz) and Offset-fine (Hz) in the
//   corresponding fields and use the corresponding arrow buttons for fine
//   tuning. This offset is intended for transverter usage or for the
//   correction of small frequency offsets between Linrad and the transceiver. 
//
//   Select a frequency on the Linrad waterfall graph.
//   Hit the Q-key to transfer the Linrad frequency, incremented with the
//   total offset, to the transceiver VFO.
//
//   Note: The graph position, the serial port number, the transceiver-type, 
//         and the offset values for 12 frequency bands are saved in a 
//         parameterfile and retrieved afterwards when Linrad is (re)started.
//         The file name is "par_xxx_hwd_ug" (with xxx = rx_mode) and its
//         content can be displayed with an editor.
//         The frequency bands are defined by the variable bandlim. 
//
// Calibration procedure:
//   This function sets automatically the values for Offset-raw and Offset-fine
//   by reading the VFO frequency from the transceiver, calculating the
//   difference with the Linrad frequency and storing this difference in
//   the offset variables:
//
//   Tune the TRX receiver and Linrad to the same carrier (zero beat).
//   Click on the blue 'calibration' box with the mouse.
//   Hit the Q-key to -read the VFO frequency from the transceiver
//                    -calculate the frequency-offset between Linrad and the
//                     transceiver VFO
//                    -store this offset in Offset-raw and Offset-fine.
// 
// CUSTOMIZATION 
//
// To add support for a new transceiver:
//   1.Add an entry to the list of supported transceivers between line 149 and
//     line 164 with the name of the transceiver, its  serial port parameters
//     and its frequency range. Use the  existing lines as examples. 
//   2.Add the corresponding 'SET VFO' and 'CALIBRATION' code (only a few
//     lines of coding ) for the new transceiver:
//     Get the technical specifications for the CAT commands for the new
//     transceiver and use the existing code for transceivers from the same
//     transceiver-family as a framework/guideline:
//       For a new YAESU transceiver p.e. have a look at the code for the
//       FT-450/FT-950/FT-2000 :
//       look at lines 1577-1595 for the 'SET VFO' code and lines 1299-1353
//       for the 'CALIBRATION' code. 
//     The 'CALIBRATION' code is not mandatory. If not present, an error
//     message will be displayed in the usergraph.
//  
// ****************************************************************************
// Change the define for RX_HARDWARE and modify the dummy routines
// in the prototype section to fit your needs.
// The routines in wse_sdrxx.c can serve as a starting point.
#define RX_HARDWARE 0
// ****************************************************************************

#include <string.h>

int serport_open_status;
int serport_number = 1 ;       
int transceiver_number = 0 ;      
int chnge_serport_switch =0;
int chnge_calibr_switch =0;
double offset_hz;
double offset_khz;
double old_hwfreq;

#define MAX_NO_OF_TRANSCEIVERS 32

struct transceiver
{
 char name[16];
 int serport_baudrate;
 int serport_stopbits;
 double min_transceiver_freq;
 double max_transceiver_freq;
 int serport_rts_mode; 
 };

// List of supported transceivers and corresponding values for  
//   serport_baudrate (valid values between 110 and  57600 )
//   serport_stopbits (0 for one stopbit or 1 for two stopbits )
//   min and max transceiver_frequency(Hz)
// check the transceiver manual to get the correct values
const struct transceiver list[MAX_NO_OF_TRANSCEIVERS] =
{{"K3         ",38400, 1,  1800000.,  54000000., 0},
 {"FT-450     ", 4800, 1,    30000.,  56000000., 0},
 {"FT-736     ", 4800, 1,144000000., 146000000., 0},
 {"FT-817     ",38400, 1,  1500000., 470000000., 0},
 {"FT-897     ",38400, 1,  1500000., 470000000., 0},
 {"FT-920     ", 4800, 1,   100000.,  56000000., 0},
 {"FT-950     ", 4800, 1,    30000.,  56000000., 0},
 {"FT-1000    ", 4800, 1,  1600000.,  30000000., 0},
 {"FT-2000    ", 4800, 1,    30000.,  60000000., 0},
 {"IC-275     ", 9600, 0,144000000., 146000000., 0},
 {"ICOM-all   ", 9600, 0,  1800000.,2450000000., 0},
 {"IC-706     ", 9600, 0,  1800000., 148000000., 0},
 {"IC-706MKII ", 9600, 0,  1800000., 148000000., 0},
 {"IC-706MKIIG", 9600, 0,  1800000., 148000000., 0},
 {"TS-850     ", 4800, 1,  1600000.,  30000000., 0},
 {"TS-2000    ", 9600, 0,  1600000., 148000000., 0},
 {"ORION      ",57600, 0,   500000.,  30000000., 0},
 {"END LIST   ",    0, 0,        0.,         0., 0}};
// ***************************************************************************
// ***************************************************************************
// ROUTINES FOR THE PROCESSING OF A 'MOVABLE' FIXED SIZE USERGRAPH
// the screenposition of the usergraph and its variables are saved
// in the parameterfile "par_xxx_hwd_ug" ( with xxx = rx_mode) in the linrad directory
// ****************************************************************************
// ****************************************************************************

#define MAX_HWAREDRIVER_PARM 28

char *users_hwaredriver_parm_text[MAX_HWAREDRIVER_PARM]={
                                       "ug_xleft             ",
                                       "ug_ytop              ",
                                       "serport_number       ",
                                       "transceiver_number   ",
                                       "offs_hz_band_0-2.5   ",
                                       "offs_hz_band_2.5-5   ",
                                       "offs_hz_band_5-9     ",
                                       "offs_hz_band_9-12    ",
                                       "offs_hz_band_12-16   ",
                                       "offs_hz_band_16-19   ",
                                       "offs_hz_band_19-23   ",
                                       "offs_hz_band_23-26   ",
                                       "offs_hz_band_26-40   ",
                                       "offs_hz_band_40-100  ",
                                       "offs_hz_band_100-300 ",
                                       "offs_hz_band_BIG     ",
                                       "offs_khz_band_0-2.5  ",
                                       "offs_khz_band_2.5-5  ",
                                       "offs_khz_band_5-9    ",
                                       "offs_khz_band_9-12   ",
                                       "offs_khz_band_12-16  ",
                                       "offs_khz_band_16-19  ",
                                       "offs_khz_band_19-23  ",
                                       "offs_khz_band_23-26  ",
                                       "offs_khz_band_26-40  ",
                                       "offs_khz_band_40-100 ",
                                       "offs_khz_band_100-300",
                                       "offs_khz_band_BIG    ",
                                       }; 

#define NO_OF_BANDS 12               
float bandlim[]={2.5,  //0 
                 5.0,  //1   
                 9.0,  //2
                12.0,  //3
                16.0,  //4
                19.0,  //5
                23.0,  //6
                26.0,  //7
                40.0,  //8
               100.0,  //9
               300.0,  //10
               BIGFLOAT};   //11
  
typedef struct {
int ug_xleft;
int ug_ytop;
int serport_number;
int transceiver_number;
int offs_hz[NO_OF_BANDS];
int offs_khz[NO_OF_BANDS];
} HWAREDRIVER_PARMS;
HWAREDRIVER_PARMS hwaredriver;

#define USERS_GRAPH_TYPE1 64
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int yborder;    // required for move graph
} UG_PARMS;
UG_PARMS ug;

#define UG_TOP 0
#define UG_BOTTOM 1
#define UG_LEFT 2
#define UG_RIGHT 3
#define UG_INCREASE_OFFS_HZ 4
#define UG_DECREASE_OFFS_HZ 5
#define UG_INCREASE_OFFS_KHZ 6
#define UG_DECREASE_OFFS_KHZ 7
#define UG_INCREASE_SERPORT_NMBR 8
#define UG_DECREASE_SERPORT_NMBR 9
#define UG_INCREASE_TRANSCEIVER_NMBR 10
#define UG_DECREASE_TRANSCEIVER_NMBR 11
#define MAX_UGBUTT 12

BUTTONS ugbutt[MAX_UGBUTT];

int ug_old_y1;
int ug_old_y2;
int ug_old_x1;
int ug_old_x2;
int users_graph_scro;

int ug_oldx;     // required for move graph
int ug_oldy;     // required for move graph
int ug_yborder;  // required for move graph
int ug_hsiz;     // required for move graph
int ug_vsiz;     // required for move graph 

int ug_band_no;

#define MAX_MSGSIZE 30     //Size of messages. min size=30
char ug_msg0[MAX_MSGSIZE]; //messages in usergraph 
char ug_msg1[MAX_MSGSIZE];
char ug_msg2[MAX_MSGSIZE];
int ug_msg_color;   //switch for message-color in usergraph
double min_transceiver_freq=0;
double max_transceiver_freq=0;

void set_press_q_key_new_freq_msg(void)
{
ug_msg_color=14;
strcpy(ug_msg0,"PRESS Q-KEY TO SET TRX FREQ."); 
ug_msg1[0]=0;
ug_msg2[0]=0;
}

void set_press_q_key_chnge_port_msg(void)
{
//change the name of the serial port
//char s[80];
#if OSNUM == OSNUM_WINDOWS
sprintf(&serport_name[7],"%d",serport_number);
#else  
if(strncmp(serport_name,"/dev/ttyd",9)==0) 
//this is BSD
  {
  sprintf(&serport_name[9],"%d",serport_number-1); 
  }
else
  {  
// this is linux 
// use either /dev/ttyS0 to /dev/ttyS3
//         or /dev/ttyUSB0 to /dev/ttyUSB3
  if (serport_number <= 4)                   // 
    {
    sprintf(serport_name,"/dev/ttyS%d",serport_number-1);
    }
  else                                       //
    {
    sprintf(serport_name,"/dev/ttyUSB%d",serport_number-5);
    }
  }
#endif

ug_msg_color=15;
strcpy(ug_msg0,"PRESS Q-KEY TO OPEN SER. PORT");
ug_msg1[0]=0;
ug_msg2[0]=0;
chnge_serport_switch=1;
}

void check_ug_borders(void)  // required for move graph
{
current_graph_minh=ug_vsiz;
current_graph_minw=ug_hsiz;
check_graph_placement((void*)(&ug));
set_graph_minwidth((void*)(&ug));
}

void users_show_offset(void)
{
char s[80];
settextcolor(15);
sprintf(s,"Offset-fine(Hz): %d        ",hwaredriver.offs_hz[ug_band_no]);
s[25]=0;
lir_pixwrite(ug.xleft+1.5*text_width,ug.ytop+1+3.5*text_height,s);
sprintf(s,"Offset-raw(Khz): %d        ",hwaredriver.offs_khz[ug_band_no]);
s[25]=0;
lir_pixwrite(ug.xleft+1.5*text_width,ug.ytop+2.5*text_height,s);
settextcolor(7);
offset_hz= hwaredriver.offs_hz[ug_band_no];
offset_khz=hwaredriver.offs_khz[ug_band_no];
}

void blankfill_ugmsg(char *msg)
{
int i;
i=strlen(msg);
if(i>=MAX_MSGSIZE)lirerr(3413400);
while(i<MAX_MSGSIZE-1)
  {
  msg[i]=' ';
  i++;
  }
msg[MAX_MSGSIZE-1]=0;  
}

void show_user_parms(void)
{
int i;
char s[80];
// Show the user parameters on screen
// and issue hardware commands if required.
// Use hware_flag to direct calls to control_hware() when cpu time 
// is available in case whatever you want to do takes too much time 
// to be done immediately.
// Note that mouse control is done in the narrowband processing thread
// and that you can not use lir_sleep to wait for hardware response here.
hide_mouse(ug.xleft, ug.xright,ug.ytop,ug.ybottom);
sprintf(s,"CALIBRATE");
settextcolor(11);
lir_pixwrite(ug.xleft+.5*text_width,ug.ytop+0.35*text_height,s);
sprintf(s,"   CAT FREQUENCY CONTROL FOR TRX: %s ",list[transceiver_number].name);
settextcolor(14);
lir_pixwrite(ug.xleft+10.5*text_width,ug.ytop+0.35*text_height,s);
settextcolor(15);
#if OSNUM == OSNUM_WINDOWS
i=4;
#else
i=0;
#endif
sprintf(s," SERIAL PORT:%s ",&serport_name[i]);
lir_pixwrite(ug.xleft+.5*text_width,ug.ytop+1.5*text_height,s);

settextcolor(7);
users_show_offset();

//DISPLAY MESSAGES IN  MESSAGE-AREA OF USERGRAPH

// Fill message with blanks up to character MAX_MSGSIZE-2
blankfill_ugmsg(ug_msg0);
blankfill_ugmsg(ug_msg1);
blankfill_ugmsg(ug_msg2);
settextcolor(ug_msg_color);
lir_pixwrite(ug.xleft+31*text_width,ug.ytop+1.5*text_height,ug_msg0);
lir_pixwrite(ug.xleft+31*text_width,ug.ytop+2.5*text_height,ug_msg1);
lir_pixwrite(ug.xleft+31*text_width,ug.ytop+3.5*text_height,ug_msg2);
settextcolor(7);

// save screenposition, serport_number, transceiver_number and offset-values
// to the parameterfile on disk

char hwaredriver_parfil_name[20];
sprintf(hwaredriver_parfil_name,"%s_hwd_ug",rxpar_filenames[rx_mode]);
FILE *hwaredriver_file;
int *sdr_pi;
hwaredriver_file=fopen(hwaredriver_parfil_name,"w");
hwaredriver.ug_xleft=ug.xleft;
hwaredriver.ug_ytop=ug.ytop;
hwaredriver.serport_number=serport_number;
hwaredriver.transceiver_number=transceiver_number;
sdr_pi=(int*)(&hwaredriver);
for(i=0; i<MAX_HWAREDRIVER_PARM; i++)
  {
  fprintf(hwaredriver_file,"%s [%d]\n",users_hwaredriver_parm_text[i],sdr_pi[i]);
  }
parfile_end(hwaredriver_file);
}

void users_set_band_no(void)
{
ug_band_no=0;
while(bandlim[ug_band_no] < fg.passband_center)ug_band_no++;
if(all_threads_started == TRUE)
  {
  pause_thread(THREAD_SCREEN);
  set_press_q_key_new_freq_msg();
  show_user_parms();
  resume_thread(THREAD_SCREEN);
  }
}

void make_users_control_graph(void)
{
pause_thread(THREAD_SCREEN);
// Set a negative number here.
// In case several windows are desired, give them different
// negative numbers and use scro[m].no to decide what to do
// in your mouse_on_users_graph routine.
check_ug_borders();  // required for move graph
scro[users_graph_scro].no=USERS_GRAPH_TYPE1;
// These are the coordinates of the border lines.
scro[users_graph_scro].x1=ug.xleft;
scro[users_graph_scro].x2=ug.xright;
scro[users_graph_scro].y1=ug.ytop;
scro[users_graph_scro].y2=ug.ybottom;
// Each border line is treated as a button.
// That is for the mouse to get hold of them so the window can be moved.
ugbutt[UG_LEFT].x1=ug.xleft;
ugbutt[UG_LEFT].x2=ug.xleft+2;
ugbutt[UG_LEFT].y1=ug.ytop;
ugbutt[UG_LEFT].y2=ug.ybottom;
ugbutt[UG_RIGHT].x1=ug.xright;
ugbutt[UG_RIGHT].x2=ug.xright-2;
ugbutt[UG_RIGHT].y1=ug.ytop;
ugbutt[UG_RIGHT].y2=ug.ybottom;
ugbutt[UG_TOP].x1=ug.xleft;
ugbutt[UG_TOP].x2=ug.xright;
ugbutt[UG_TOP].y1=ug.ytop;
ugbutt[UG_TOP].y2=ug.ytop+2;
ugbutt[UG_BOTTOM].x1=ug.xleft;
ugbutt[UG_BOTTOM].x2=ug.xright;
ugbutt[UG_BOTTOM].y1=ug.ybottom;
ugbutt[UG_BOTTOM].y2=ug.ybottom-2;
// Draw the border lines
graph_borders((void*)&ug,7);
ug_oldx=-10000;                   //from freq_control
settextcolor(7);
make_button(ug.xleft+27.5*text_width+2,ug.ybottom-1*text_height/2-1,
                                         ugbutt,UG_DECREASE_OFFS_HZ,25);
make_button(ug.xleft+29*text_width+2,ug.ybottom-1*text_height/2-1,
                                     ugbutt,UG_INCREASE_OFFS_HZ,24); 

make_button(ug.xleft+27.5*text_width+2,ug.ybottom-3*text_height/2-1,
                                         ugbutt,UG_DECREASE_OFFS_KHZ,25);
make_button(ug.xleft+29*text_width+2,ug.ybottom-3*text_height/2-1,
                                     ugbutt,UG_INCREASE_OFFS_KHZ,24); 

make_button(ug.xleft+27.5*text_width+2,ug.ybottom-5*text_height/2-1,
                                         ugbutt,UG_DECREASE_SERPORT_NMBR,25);
make_button(ug.xleft+29*text_width+2,ug.ybottom-5*text_height/2-1,
                                     ugbutt,UG_INCREASE_SERPORT_NMBR,24);

make_button(ug.xleft+57.5*text_width+2,ug.ytop+1.5*text_height/2-1,
                                         ugbutt,UG_DECREASE_TRANSCEIVER_NMBR,25);
make_button(ug.xleft+59*text_width+2,ug.ytop+1.5*text_height/2-1,
                                     ugbutt,UG_INCREASE_TRANSCEIVER_NMBR,24);
show_user_parms();
//draw separatorlines in usergraph
//vertical
lir_line(ug.xleft+30.5*text_width,ug.ytop+1.3*text_height,ug.xleft+30.5*text_width,ug.ybottom,7);
lir_line(ug.xleft+10*text_width,ug.ytop+0*text_height,ug.xleft+10*text_width,ug.ytop+1.3*text_height,7);
//horizontal
lir_line(ug.xleft,ug.ytop+1.3*text_height,ug.xright,ug.ytop+1.3*text_height,7);
//
resume_thread(THREAD_SCREEN);
}

void mouse_continue_users_graph(void)
{
int i, j;      // required for move graph
switch (mouse_active_flag-1)
  {
// Move border lines immediately.
// for other functions, wait until button is released.
// Move fixed size window  based on coding in freq_control.c
case UG_TOP:
  if(ug.ytop!=mouse_y)goto ugm;
  break;

  case UG_BOTTOM:
  if(ug.ybottom!=mouse_y)goto ugm;
  break;

  case UG_LEFT:
  if(ug.xleft!=mouse_x)goto ugm;
  break;

  case UG_RIGHT:
  if(ug.xright==mouse_x)break;
ugm:;
  pause_screen_and_hide_mouse();
  graph_borders((void*)&ug,0);
  if(ug_oldx==-10000)
    {
    ug_oldx=mouse_x;
    ug_oldy=mouse_y;
    }
  else
    {
    i=mouse_x-ug_oldx;
    j=mouse_y-ug_oldy;  
    ug_oldx=mouse_x;
    ug_oldy=mouse_y;
    ug.ytop+=j;
    ug.ybottom+=j;
    ug.xleft+=i;
    ug.xright+=i;
    check_ug_borders();    
    ug.yborder=(ug.ytop+ug.ybottom)>>1;
    }
  graph_borders((void*)&ug,15); 
  resume_thread(THREAD_SCREEN);
  break;

  default:
  goto await_release;
  }
if(leftpressed == BUTTON_RELEASED)goto finish;
return;
await_release:;
if(leftpressed != BUTTON_RELEASED) return;
// Assuming the user wants to control hardware we
// allow commands only when data is not over the network.
if((ui.network_flag&NET_RX_INPUT) == 0) //for linrad02.23
  {
  switch (mouse_active_flag-1)
    {
    case UG_INCREASE_OFFS_HZ: 
    hwaredriver.offs_hz[ug_band_no]++;
    set_press_q_key_new_freq_msg();
    show_user_parms();
    break;

    case UG_DECREASE_OFFS_HZ:
    hwaredriver.offs_hz[ug_band_no]--;
    set_press_q_key_new_freq_msg();
    show_user_parms();
    break;

    case UG_INCREASE_OFFS_KHZ: 
    hwaredriver.offs_khz[ug_band_no]++;
    set_press_q_key_new_freq_msg();
    show_user_parms();
    break;

    case UG_DECREASE_OFFS_KHZ:
    hwaredriver.offs_khz[ug_band_no]--;
    set_press_q_key_new_freq_msg();
    show_user_parms();
    break;

    case UG_INCREASE_SERPORT_NMBR: 
    serport_number++;
#if OSNUM == OSNUM_WINDOWS
    if (serport_number > 999) serport_number = 999;
#endif
#if OSNUM == OSNUM_LINUX
    if (serport_number > 8) serport_number = 8;
#endif
    set_press_q_key_chnge_port_msg();
    show_user_parms();
    break;

    case UG_DECREASE_SERPORT_NMBR:
    serport_number--;  
#if OSNUM == OSNUM_WINDOWS
    if (serport_number < 0) serport_number =0;
#endif
#if OSNUM == OSNUM_LINUX
    if (serport_number < 1) serport_number =1;
#endif
    set_press_q_key_chnge_port_msg();
    show_user_parms();
    break;

    case UG_INCREASE_TRANSCEIVER_NMBR:
    transceiver_number++;
    if  ((strcmp(list[transceiver_number].name,"END LIST   ")==0))
         transceiver_number--;
    set_press_q_key_new_freq_msg();
    show_user_parms();
    break;

    case UG_DECREASE_TRANSCEIVER_NMBR:
    transceiver_number--;
    if (transceiver_number <= 0) transceiver_number =0;
    set_press_q_key_new_freq_msg();
    show_user_parms();
    break;

    default:
// This should never happen.    
    lirerr(211053);
    break;
    }
  }  
finish:;
hide_mouse(ug_old_x1,ug_old_x2,ug_old_y1,ug_old_y2);
leftpressed=BUTTON_IDLE;  
mouse_active_flag=0;
graph_borders((void*)&ug,0);
lir_fillbox(ug_old_x1,ug_old_y1,ug_old_x2-ug_old_x1,ug_old_y2-ug_old_y1,0);
make_users_control_graph();
ug_oldx=-10000; 
}

void new_user_offs_hz(void)
{
hwaredriver.offs_hz[ug_band_no]=numinput_int_data;
pause_thread(THREAD_SCREEN);
set_press_q_key_new_freq_msg();
show_user_parms();
resume_thread(THREAD_SCREEN);
}

void users_new_user_offs_khz(void)
{
hwaredriver.offs_khz[ug_band_no]=numinput_float_data;
pause_thread(THREAD_SCREEN);
set_press_q_key_new_freq_msg();
show_user_parms();
resume_thread(THREAD_SCREEN);
}

void mouse_on_users_graph(void)
{
int event_no;
// First find out if we are on a button or border line.
for(event_no=0; event_no<MAX_UGBUTT; event_no++)
  {
  if( ugbutt[event_no].x1 <= mouse_x && 
      ugbutt[event_no].x2 >= mouse_x &&      
      ugbutt[event_no].y1 <= mouse_y && 
      ugbutt[event_no].y2 >= mouse_y) 
    {
    ug_old_y1=ug.ytop;
    ug_old_y2=ug.ybottom;
    ug_old_x1=ug.xleft;
    ug_old_x2=ug.xright;
    mouse_active_flag=1+event_no;
    current_mouse_activity=mouse_continue_users_graph;
    return;
    }
  }
// Not button or border.
// Prompt the user for offs_hz or offs_khz depending on whether the mouse is
// in the upper or the lower part of the window.
mouse_active_flag=1;
numinput_xpix=ug.xleft+18.5*text_width;
if(mouse_x > ug.xleft && mouse_x < ug.xleft+10*text_width)
{
if(mouse_y > ug.ytop && mouse_y < (ug.ytop+1.3*text_height))
  {
  ug_msg_color=11;
  strcpy(ug_msg0,"PRESS Q-KEY TO CALIBRATE"); 
  ug_msg1[0]=0;
  ug_msg2[0]=0;
  chnge_calibr_switch=1;
  show_user_parms();
  }
}
if(mouse_x > ug.xleft+18.5*text_width && mouse_x < ug.xleft+25*text_width)
  {
if(mouse_y > (ug.ytop+3.8*text_height))
    {
    numinput_ypix=ug.ytop+3.6*text_height;
    numinput_chars=4;
    erase_numinput_txt();
    numinput_flag=FIXED_INT_PARM;
    par_from_keyboard_routine=new_user_offs_hz; //offset-fine

    }
  else
    {
    numinput_ypix=ug.ytop+(2.4*text_height)+1;
    numinput_chars=7;
    erase_numinput_txt();
    numinput_flag=FIXED_FLOAT_PARM;
    par_from_keyboard_routine=users_new_user_offs_khz; //offset-raw
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

void init_users_control_window(void)
{
int i;
// Get values from parameterfile
int errcod;
char hwaredriver_parfil_name[20];
sprintf(hwaredriver_parfil_name,"%s_hwd_ug",rxpar_filenames[rx_mode]);
errcod=read_sdrpar(hwaredriver_parfil_name, MAX_HWAREDRIVER_PARM, 
                                     users_hwaredriver_parm_text, (int*)((void*)&hwaredriver));
if (errcod != 0)
  {
  FILE *hwaredriver_file;
  int *sdr_pi;
  hwaredriver_file=fopen(hwaredriver_parfil_name,"w");
//set_default_parameter values
  hwaredriver.ug_xleft=60*text_width;                    
  hwaredriver.ug_ytop=(screen_last_line-4)*text_height; 
  hwaredriver.serport_number=1;
  hwaredriver.transceiver_number=0; 
  for(i=0; i<NO_OF_BANDS; i++)
   {
   hwaredriver.offs_hz[i]=0;       
   hwaredriver.offs_khz[i]=0;          
   }
  sdr_pi=(int*)(&hwaredriver);
  for(i=0; i<MAX_HWAREDRIVER_PARM; i++)
   {
   fprintf(hwaredriver_file,"%s [%d]\n",users_hwaredriver_parm_text[i],sdr_pi[i]);
   }
  parfile_end(hwaredriver_file);
  }
ug.xleft=hwaredriver.ug_xleft;
ug.ytop=hwaredriver.ug_ytop;
serport_number=hwaredriver.serport_number;
transceiver_number=hwaredriver.transceiver_number;

//trace
//printf("\n serport_number %i\n",serport_number);
lir_close_serport();
// 1=COM1 or /dev/ttyS0 , 2=COM2....
// baudrate 110 to 57600
// 1=two stopbits, 0=one stopbit
serport_open_status=lir_open_serport(serport_number, 
     list[transceiver_number].serport_baudrate,       
     list[transceiver_number].serport_stopbits,
     list[transceiver_number].serport_rts_mode);      
set_press_q_key_new_freq_msg();
ug_hsiz=(30+MAX_MSGSIZE)*text_width;                  //  WIDTH OF USERGRAPH
ug_vsiz=(4.8*text_height);                            //  HEIGHT OF USERGRAPH
ug.xright=ug.xleft+ug_hsiz;
ug.ybottom= ug.ytop+ug_vsiz;
if(rx_mode < MODE_TXTEST)
  {
  users_graph_scro=no_of_scro;
  make_users_control_graph();
  no_of_scro++;
  if(no_of_scro >= MAX_SCRO)lirerr(89);
  }
}

void users_init_mode(void)
{
// A switch statement can be used to do different things depending on
// the processing mode.
//switch (rx_mode)
//  {
//  case MODE_WCW:
//  case MODE_HSMS:
//  case MODE_QRSS:
//  break;
//  case MODE_NCW:
//  case MODE_SSB:
// Open a window to allow mouse control of your own things
  init_users_control_window();  
//  break;
//  case MODE_FM:
//  case MODE_AM:
//  case MODE_TXTEST:
//  case MODE_RX_ADTEST:
//  case MODE_TUNE:
//  break;
//  }
}
// ***************************************************************************
// END ROUTINES FOR THE PROCESSING OF A 'MOVABLE' FIXED SIZE USERGRAPH
// ***************************************************************************

void users_eme(void)
{
}

void userdefined_u(void)
{
}

void edit_ugmsg(char *s, double fq)
{
//edit frequency information
double t1;
int i, j, k, l,m;
// We are called with a frequency in Hz and already something written
// in the msg string. Find out how many characters we have at our disposal.
j=strlen(s);
i=MAX_MSGSIZE-j-3;
// We will insert separators for each group of 3 digits so we loose i/3 
// positions.
i-=i/3;
// Find out the largest fq we can write in Hz. 
// Write in kHz if fq is above the limit.
t1=i;
t1=pow(10.0,t1)-1;
if(fabs(fq) < t1)
  {
  sprintf (&s[j],"%.0f Hz",fq);
  }
else
  {
  sprintf(&s[j],"%.0f kHz",fq/1000);
  }
// Locate the blank in front of Hz or kHz.  
i=strlen(s);
k=i;
while(s[i] != ' ')i--;
// Insert a separator for every 3 digits.
i--;
j=0;
while(s[i-1] != ' ' && s[i-1] != '-')
  {
  j++;
  if(j==3)
    {
    k++;
    m=k;
    while(m > i)
      {
      s[m]=s[m-1];
      m--;
      }
    s[i]=39;
    j=0;
    }
  i--;
  }
// Align frequency information  to the right
j=MAX_MSGSIZE-1;
i=strlen(s); 
for (l=i; l<=j;l++) s[l]='\x0';
k=j;
while(s[k] != 'z')k--;
if (k!=j-1){
m=j;
while(s[m] != '=')m--;
j--;
for (i=k; i > m; i--) {
   s[j] = s[i];
   s[i] = 32;
   j--;
   }
for (i=j; i > m; i--) {
   s[i] = 32;
   j--;
   }
 }
}

void users_close_devices(void)
{
lir_close_serport();
//trace
//printf("\n serport closed\n");
}

void users_open_devices(void)
{
// open selected serial port with the parameters defined in transceiver list
// 1=COM1 or /dev/ttyS0 , 2=COM2..,4=COM4 or /dev/ttyUSB0...
// baudrate 110 to 57600
// 1=two stopbits, 0=one stopbit
serport_open_status=lir_open_serport(serport_number , 
     list[transceiver_number].serport_baudrate,       
     list[transceiver_number].serport_stopbits,
     list[transceiver_number].serport_rts_mode);
//trace to screen
//printf("\n serport open:stat%i,nbr%i,baud%i,stopb%i\n",serport_open_status,serport_number,list
//[transceiver_number].serport_baudrate,            
//     list[transceiver_number].serport_stopbits ); 
}

void userdefined_q(void)
{
// ****************************************************************************
// ****************************************************************************
// THIS ROUTINE IS CALLED EACH TIME THE 'Q' KEY IS PRESSED
// The processing is selected based on the following switch-values:
//   if(chnge_serport_switch==1)
//       then the new name of the serial port is displayed on the usergraph
//       and the port is opened
//   if(chnge_calibr_switch==1)
//       then a "CALIBRATION REQUEST" is processed
//   else a "SET VFO" request is processed 
// ****************************************************************************
// ****************************************************************************
double transceiver_freq;
double offset_hwfreq;
uint64_t freq;
double transverter_offset_raw;  //raw offset in Khz (for tranverter) 
double transverter_offset_fine; //fine offset in Hz (for transverter)
unsigned char a;
int n=-1;
int i, j;
double kxx;
char s[80];

void analyze_serport_error (void)
{
  ug_msg_color=12;
  sprintf (ug_msg0,"SERIAL PORT  ERROR :");
  switch (serport_open_status )
 {
  case 1244: 
  sprintf (ug_msg1,"OPEN FAILED (%i)",serport_open_status);
  break;
  case 1277:
  sprintf (ug_msg1,"GET OPTIONS FAILED (%i)",serport_open_status);
  break;
  case 1278:
  sprintf (ug_msg1,"SET OPTIONS FAILED (%i)",serport_open_status);
  break;
  case 1279:
  sprintf (ug_msg1,"ILLEGAL PORT NUMBER (%i)",serport_open_status);
  break;
  case 1280:
  sprintf (ug_msg1,"ILLEGAL BAUDRATE (%i)",serport_open_status);
  break;
  default:
  sprintf (ug_msg1,"UNKNOWN ERR CODE (%i)",serport_open_status);
  } 
}

if(chnge_serport_switch==1)
// ****************************************************************************
// ****************************************************************************
// ROUTINE FOR THE PROCESSING OF A CHANGE REQUEST FOR THE SERIAL PORT 
// ****************************************************************************
// ****************************************************************************
{
chnge_serport_switch=0;
sprintf(s," SERIAL PORT:%s ",serport_name);
lir_pixwrite(ug.xleft+.5*text_width,ug.ytop+1.5*text_height,s);
users_close_devices();
users_open_devices(); 
//check status serial port
if ( serport_open_status!=0) 
 {
 analyze_serport_error ();
 }
else
 {
 ug_msg_color=15;
 sprintf (ug_msg0,"SERIAL PORT OPEN OK");
 sprintf (ug_msg1,"PRESS Q-KEY TO SET TRX FREQ.");
 }
goto userdefined_q_exit_1;
}

// ****************************************************************************
// ****************************************************************************
// ROUTINES FOR THE PROCESSING OF A "CALIBRATION REQUEST":
//   -validate and initialize
//  - select and execute the transceiver-dependent routine: 
//    - create the CAT command message strings for reading the VFO 
//    - write/read the message strings to/from serial port 
//    - calculate the offset between VFO frequency and Linrad freq
//    - display this offset on the usergraph
// ****************************************************************************
// ****************************************************************************
// generic routines for calibration:
void calculate_offset(void)
{
transceiver_freq=kxx;
offset_hwfreq=transceiver_freq-(hwfreq*1000);
transverter_offset_raw=floor(offset_hwfreq/1000);
transverter_offset_fine=ceil(offset_hwfreq-transverter_offset_raw*1000);
hwaredriver.offs_khz[ug_band_no]=transverter_offset_raw;
hwaredriver.offs_hz[ug_band_no]=transverter_offset_fine;
ug_msg_color=11;
sprintf (ug_msg0,"TRX VFO FREQ:%.0f",kxx);
sprintf (ug_msg1,"NEW OFFSET VALUES LOADED");
sprintf (ug_msg2,"PRESS Q-KEY TO SET TRX FREQ.");
}

void fail_message_1(void)
{ug_msg_color=12;
sprintf (ug_msg0,"CALIBRATION FAILED:");
sprintf (ug_msg1,"WRITE TO SERIAL PORT FAILED");
sprintf (ug_msg2,"PRESS Q-KEY TO SET TRX FREQ.");
}

void fail_message_2(void)
{ug_msg_color=12;
sprintf (ug_msg0,"CALIBRATION FAILED:");
sprintf (ug_msg1,"READ FROM SERIAL PORT FAILED");
sprintf (ug_msg2,"PRESS Q-KEY TO SET TRX FREQ.");
}

void fail_message_3(void)
{ug_msg_color=12;
sprintf (ug_msg0,"CALIBRATION FAILED:");
sprintf (ug_msg1,"TRX VFO FREQ = 0");
sprintf (ug_msg2,"PRESS Q-KEY TO SET TRX FREQ.");
}

//check the calibration-request switch
if(chnge_calibr_switch==1)
  {
  chnge_calibr_switch=0;

//VALIDATE and INITIALIZE:
// exit if no linrad-frequency selected
  if(mix1_selfreq[0] < 0)  
    {
    ug_msg_color=12;
    sprintf (ug_msg0,"NO FREQ. SELECT. ON W/F GRAPH");
    goto userdefined_q_exit_1;
    }
// flush buffers 
   users_close_devices();
   users_open_devices();

// SELECT and execute transceiver-dependent routines for calibration:

// ************************ CAL FT-817 and FT-897 ***********************************
if (((strcmp(list[transceiver_number].name,"FT-817     ")==0))
   |((strcmp(list[transceiver_number].name,"FT-897     ")==0)))
{
// write delay in msec between two bytes
int   write_delay =10;
// CAT command for getting the vfo freq:
unsigned char getfreq_cmnd[5]= { 0x00, 0x00, 0x00, 0x00, 0x03};
// WRITE CAT command to the serial port
unsigned char cat_cmnd[5];
int write_cat_cmnd(void)
{
int r;
for (i=0;i<5;i=i+1)
 {
  r=lir_write_serport((char*)&cat_cmnd[i],1);
  if (r<0) return -1;
// printf("\n %#4.2x ",cat_cmnd[i]);  //trace output to screen
  lir_sleep(write_delay*1000); 
 }
return 0;
}
memcpy(cat_cmnd,getfreq_cmnd, 5);
n=write_cat_cmnd();
if (n<0) 
  {
// write to serial port failed
  fail_message_1();
  goto userdefined_q_exit_1;
  }
// READ the vfo freq
unsigned char ft897_zero[5]= {0x0000000000};
memcpy(s,ft897_zero, 5);
n=lir_read_serport(s,5);
// ANALYSE and process returned values
if (n==5)
  {
// s holds now the vfo frequency encoded as 
// 0x43 0x97 0x00 0x00 0xNN for a frequency of 439.700.000,00 MHz
// last byte, xNN, is the mode (usb,lsb,..) and will be  discarded
// convert now  pos 0 to 3 in s, from packed bcd to double kxx
  kxx=0;
  j=10;
  for (i=3; i >-1; i--)
    {
    a=(s[i]%16 & 0x0f);
    kxx= kxx+a*j;
    j=j*10;
    a= ((s[i]>>4)%16) & 0x0f;
    kxx= kxx+a*j;
    j=j*10;
    }
  if (kxx==0)
   {
// failure: trx vfo freq = 0
   fail_message_3();
   goto userdefined_q_exit_1; 
   }
  else
   {
// success
   calculate_offset();
   goto userdefined_q_exit_1;
   }
  }
else
  {
// failure: read from serial port failed
  fail_message_2();
  goto userdefined_q_exit_1; 
  }
}
// ************************CAL FT-920 ****************************************
if ((strcmp(list[transceiver_number].name,"FT-920     ")==0))
{
// CAT command definition for getting the frequency of VFO A & B
// (it returns 2 x 16 bytes):
unsigned char getfreq_cmnd[5]= { 0x00, 0x00, 0x00, 0x03, 0x10};
// WRITE CAT_cmnd to the serial port
for (i=0;i<5;i=i+1)
 {
  n=lir_write_serport((char*)&getfreq_cmnd[i],1);
  if (n<0) 
    {
// write to serial port failed
    fail_message_1();
    goto userdefined_q_exit_1;
    }
// wait 5msec between each write
  lir_sleep(5000);
 }
// READ the vfo freq 
// wait 60msec to allow the CAT system to settle
lir_sleep(60000); 
unsigned char ft920_zero[14]= {0x0000000000000000000000000000};
memcpy(s,ft920_zero, 14);
n=lir_read_serport(s,28);
// ANALYSE and process returned values
if (n==28)
  {
// s holds now the vfo frequencies of VFO A & B in 28 bytes
// VFO A is in bytes 1-4 (1 MSB -> 4 LSB) in hex format
// convert pos 1 to 4 from the ascii string s, to the double kxx
  kxx=0;
  j=1;
  for (i=4; i >0; i--)
    {
    a=(s[i]%16 & 0x0f);
    kxx= kxx+a*j;
    j=j*16;
    a= ((s[i]>>4)%16) & 0x0f;
    kxx= kxx+a*j;
    j=j*16;
    }
  if (kxx==0)
    {
// failure: trx vfo freq = 0
    fail_message_3();
    goto userdefined_q_exit_1;
    }
  else
    {
// success
    calculate_offset();
    goto userdefined_q_exit_1;
    }
  }
else 
  {
// failure: read from serial port failed
  fail_message_2();
  goto userdefined_q_exit_1;
  }
}
// ************************CAL FT-1000 ****************************************
if ((strcmp(list[transceiver_number].name,"FT-1000    ")==0))
{
// write delay in msec between two bytes
int   write_delay = 5;
// CAT command definition for getting the frequency of VFO A & B
// (it returns 2 x 16 bytes):
unsigned char getfreq_cmnd[5]= { 0x00, 0x00, 0x00, 0x03, 0x10};
// WRITE CAT_cmnd to the serial port
for (i=0;i<5;i=i+1)
 {
  n=lir_write_serport((char*)&getfreq_cmnd[i],1);
  if (n<0) 
    {
// write to serial port failed
    fail_message_1();
    goto userdefined_q_exit_1;
    }
  lir_sleep(write_delay*1000);
 }
// READ the vfo freq 
// wait 60msec to allow the CAT system to settle
lir_sleep(60000); 
unsigned char ft1000_zero[16]= {0x00000000000000000000000000000000};
memcpy(s,ft1000_zero, 16);
n=lir_read_serport(s,32);
// ANALYSE and process returned values
if (n==32)
  {
// s holds now the vfo frequencies of VFO A & B in 32 bytes:
// convert pos 8 to 15 from the ascii string s, to the double kxx
  kxx=0;
  j=1;
  for (i=15; i > 7 ; i--) {
    kxx=kxx+(s[i] & 0x0f)*j;
    j=j*10;
    }
  if (kxx==0)
    {
// failure: trx vfo freq = 0
    fail_message_3();
    goto userdefined_q_exit_1;
    }
  else
    {
// success
    calculate_offset();
    goto userdefined_q_exit_1;
    }
  }
else 
  {
// failure: read from serial port failed
  fail_message_2();
  goto userdefined_q_exit_1;
  }
}

// *********************** CAL TS-850/TS-2000/K3 *********************************
if (((strcmp(list[transceiver_number].name,"K3         ")==0))
   |((strcmp(list[transceiver_number].name,"TS-850     ")==0))
   |((strcmp(list[transceiver_number].name,"TS-2000    ")==0)))
{
// CAT command definition for getting the vfo freq:
sprintf (s,"FA;");
// WRITE CAT command to serial port
n=lir_write_serport(s,3);
if (n<0) 
  {
// write to serial port failed
  fail_message_1();
  goto userdefined_q_exit_1;
  }
// READ the vfo freq
sprintf (s,"0000000000000;");
n=lir_read_serport(s,14);
// ANALYSE and process returned values
if (n==14)
  {
// s holds now the vfo frequency encoded p.e. as 
// FA00014250000; for the frequency 14.250.000 Hz:
// convert pos 2 to 12 of s to the double kxx
  kxx=0; 
  for (i=2; i<=12;i++)
    {
    j=s[i]-'0';
    kxx=kxx*10+j;
    }
  if (kxx==0)
    {
// failure: trx vfo freq = 0
    fail_message_3();
    goto userdefined_q_exit_1;
    }
  else
    {
// success
    calculate_offset();
    goto userdefined_q_exit_1;
    }
  }
else 
  {
// failure: read from serial port failed
  fail_message_2();
  goto userdefined_q_exit_1;
  }
}

// *******************CAL FT-450 and FT-950 and FT-2000********************************
if   (((strcmp(list[transceiver_number].name,"FT-450     ")==0))
     |((strcmp(list[transceiver_number].name,"FT-950     ")==0))
     |((strcmp(list[transceiver_number].name,"FT-2000    ")==0)))
{
// CAT command to request the vfo freq:
sprintf (s,"FA;");
// WRITE to serial port
n=lir_write_serport(s,3);
//traces
//printf("\n %s",s); //trace output to screen
//printf("\n");      //'next line' to display last trace entry
if (n<0) 
  {
// write to serial port failed
  fail_message_1();
  goto userdefined_q_exit_1;
  }
// READ requested vfo freq
sprintf (s,"0000000000;");
n=lir_read_serport(s,11);
//printf("\n s=%s n=%i",s,n); //trace output to screen
//printf("\n");      //'next line' to display last trace entry
if (n==11)
  {
// s holds the vfo frequency encoded as FA14250000; for the frequency
// 14.250.000 Hz   
// convert position 2 to 9 of s to double kxx
  kxx=0; 
  for (i=2; i<=9;i++)
    {
    j=s[i]-'0';
    kxx=kxx*10+j;
    }
  if (kxx==0)
    {
//failure: trx vfo freq = 0
    fail_message_3();
    goto userdefined_q_exit_1;
    }
  else
    {
//success
    calculate_offset();
    goto userdefined_q_exit_1;
    }
  }
else 
  {
//failure: read from serial port failed
  fail_message_2();
  goto userdefined_q_exit_1;
  } 
}

// **************************** CAL ORION I or II *****************************
if (strcmp(list[transceiver_number].name,"ORION      ")==0)
{
// CAT command definition for getting the vfo freq:
sprintf (s,"?AF\r");
// WRITE CAT command to serial port
n=lir_write_serport(s,4);
if (n<0) 
  {
// write to serial port failed
  fail_message_1();
  goto userdefined_q_exit_1;
  }
// READ the vfo freq
sprintf (s,"000000000000");
n=lir_read_serport(s,12);
// ANALYSE and process returned values
if (n==12)
  {
// s holds now the vfo frequency encoded p.e. as 
// @AF14250000\r for the frequency 14.250.000 Hz:
// convert pos 3 to 10 of s to the double kxx
  kxx=0; 
  for (i=3; i<=10;i++)
    {
    j=s[i]-'0';
    kxx=kxx*10+j;
    }
  if (kxx==0)
    {
// failure: trx vfo freq = 0
    fail_message_3();
    goto userdefined_q_exit_1;
    }
  else
    {
// success
    calculate_offset();
    goto userdefined_q_exit_1;
    }
  }
else 
  {
// failure: read from serial port failed
  fail_message_2();
  goto userdefined_q_exit_1;
  }
}

// ***************************** CAL CATCH ALL *******************************

else{
    ug_msg_color=12;
    sprintf (ug_msg0,"NO CALIBRATION AVAILABLE");
    sprintf (ug_msg1,"FOR THIS TRX");
    sprintf (ug_msg2,"PRESS Q-KEY TO SET TRX FREQ.");
    }
goto userdefined_q_exit_1;
}
// ***************************************************************************
// ***************************************************************************
// ROUTINES FOR THE PROCESSING OF A "SET VFO " REQUEST
//  - do some validations and initialize usergraph
//  - select and execute the transceiver-dependent routines for setting the
//    VFO frequency 
//      input:  desired frequency stored in the variables freq (unsigned long)
//              or transceiver_freq (double)
//      proces: create the transceiver-dependent CAT commands for setting the
//              VFO frequency
//              write/read the message strings to/from the serial port
//      output: updated  messages in the usergraph 
// ***************************************************************************
// ***************************************************************************
// VALIDATE and INITIALIZE:
// check status of serial port
if ( serport_open_status!=0)
  {
  analyze_serport_error ();
  goto userdefined_q_exit_1;
  }
// check if linrad frequency is selected
if(mix1_selfreq[0] < 0)  
  {
  ug_msg_color=12;
  sprintf (ug_msg0,"NO FREQ. SELECT. ON W/F GRAPH");
  goto userdefined_q_exit_1;
  }
// check whether the desired frequency is within the valid range for the
// selected transceiver  
//raw offset in Khz for transverter :
transverter_offset_raw=hwaredriver.offs_khz[ug_band_no]; 
//fine offset in Hz for transverter :
transverter_offset_fine=hwaredriver.offs_hz[ug_band_no]; 
//Freq in KHz :   
offset_hwfreq= transverter_offset_raw +(transverter_offset_fine*.001) ; 
//Freq in Hz 
transceiver_freq= (hwfreq+offset_hwfreq)*1000;
transceiver_freq=floor(transceiver_freq);                               
freq= transceiver_freq;
if ((transceiver_freq < list[transceiver_number].min_transceiver_freq )
     ||(transceiver_freq > list[transceiver_number].max_transceiver_freq))
  {
  ug_msg_color=12;
  sprintf (ug_msg0,"INVALID FREQ.FOR %s",list[transceiver_number].name); 
  goto userdefined_q_exit_2;
  }
// initialize message-area in usergraph:
ug_msg_color=12;
strcpy(ug_msg0,"FREQUENCY SETTING IN PROGRESS");
ug_msg1[0]=0;
ug_msg2[0]=0;
show_user_parms();

// SELECT and execute transceiver dependent routines for setting the VFO
// frequency:

// *********************** SET VFO IC-275 and IC-706 ***************************** 
if  (((strcmp(list[transceiver_number].name,"IC-275     ")==0))
    |((strcmp(list[transceiver_number].name,"ICOM-all   ")==0))
    |((strcmp(list[transceiver_number].name,"IC-706     ")==0))
    |((strcmp(list[transceiver_number].name,"IC-706MKII ")==0))
    |((strcmp(list[transceiver_number].name,"IC-706MKIIG")==0)))
{
// CAT command definition.
// 'Set Frequency command' for IC-275/IC-706:
struct {            
   char prefix[2];
   char radio_addr;
   char computer_addr;
   char cmd_nr;
   unsigned char freq[5];
   char suffix;
   } msg0 = {{0xFE, 0xFE},               // preamble code x 2
             0x10,                       // receive address for IC-275
             0xE0,                       // transmit address of controller
             0x05,                       // 'set freq' cmnd code
             {0x00,0x00,0x00,0x00,0x00}, // freq
             // 145.678.900 Hz  is encoded as x00 x89 x67 x45 x01
             0xFD};                    // end of message code

if  ((strcmp(list[transceiver_number].name,"ICOM-all   ")==0))
   msg0.radio_addr='\x00';   // receive address for any icom-radio broadcasting
if  ((strcmp(list[transceiver_number].name,"IC-706     ")==0))
   msg0.radio_addr='\x48';   // receive address for IC-706
if  ((strcmp(list[transceiver_number].name,"IC-706MKII ")==0))
   msg0.radio_addr='\x4e';   // receive address for IC-706MKII
if  ((strcmp(list[transceiver_number].name,"IC-706MKIIG")==0))
   msg0.radio_addr='\x58';   // receive address for IC-706MKIIG
// CONVERT freq to the CAT command format;
//   convert 10 digits of freq to packed bcd and store in msg0.freq,
//   put first digit-pair in FIRST output byte
for (i=0; i < 5; i++) {
   a = freq%10;
   freq /= 10;
   a |= (freq%10)<<4;
   freq /= 10;
   msg0.freq[i]= a;
//printf("\n %#4.2x ",msg0.freq[i]); //trace output to screen
   }
//printf("\n");      //'next line' to display last trace entry
// WRITE CAT command to serial port
n=lir_write_serport((char *) &msg0, sizeof(msg0));
goto display_messages_in_usersgraph;
}

// ******************* SET VFO TS-850, TS-2000 and K3 *********************
if (((strcmp(list[transceiver_number].name,"K3         ")==0))
   |((strcmp(list[transceiver_number].name,"TS-850     ")==0))
   |((strcmp(list[transceiver_number].name,"TS-2000    ")==0)))
{
// CAT command definition for setting the VFO freq:
//   FA is the "set VFO-A frequency" command code
//   Freq in Hz; 11 digits, leading zeros, no decimal point, trailing semicolon.
// CONVERT transceiver_freq to the CAT command format
sprintf (s,"FA%011.0f;",transceiver_freq);
// WRITE CAT command to serial port  
n=lir_write_serport(s,14);
//printf("\n %s",s);    //trace output to screen
//printf("\n");         //add 'next line' to trace output
goto display_messages_in_usersgraph;
}

// ************************ SET VFO FT-736 ************************************

if ((strcmp(list[transceiver_number].name,"FT-736     ")==0))
{
// write delay in msec between two bytes
int   write_delay =50;
// CAT command definitions for setting the VFO freq:
//   Setting the VFO frequency to 145.678.900 HZ is encoded as 
//   0x14 0x56 0x78 0x90 0x01
//   the last byte 0x01 is the "set freq" command code 
unsigned char setfreq_cmnd[5]= { 0x00, 0x00, 0x00, 0x00, 0x01};
unsigned char open_cmnd[5] =  { 0x00, 0x00, 0x00, 0x00, 0x00};
unsigned char close_cmnd[5] = { 0x80, 0x80, 0x80, 0x80, 0x80};
// WRITE CAT commands to serial port
unsigned char cat_cmnd[5];
int write_cat_cmnd(void)
{
int r=0;
for (i=0;i<5;i=i+1)
 {
  r=lir_write_serport((char*)&cat_cmnd[i],1);
  if (r<0) return -1;
// printf("\n %#4.2x ",cat_cmnd[i]);  //trace output to screen
  lir_sleep(write_delay*1000); 
 }
return 0;
}
// 1. open CAT system
memcpy(cat_cmnd,open_cmnd, 5);
n=write_cat_cmnd();
if (n<0) goto ft736_exit;
// 2. CONVERT the content of "freq" variable to the setfreq_cmd format:
//   discard least significant digit of freq
//   convert next 8 digits of freq to packed bcd and store in setfreq_cmd
//   put first digit-pair in LAST  output byte 
freq /= 10;
for (i=0; i < 4; i++) {
   a = freq%10;
   freq /= 10;
   a |= (freq%10)<<4;
   freq /= 10;
   setfreq_cmnd[3-i]= a;
   }
// 3. write setfreq_cmd to the serial port
memcpy(cat_cmnd,setfreq_cmnd, 5);
n=write_cat_cmnd();
if (n<0) goto ft736_exit;
// 4. close CAT system
memcpy(cat_cmnd,close_cmnd, 5);
n=write_cat_cmnd();
ft736_exit:;
//printf("\n");  //'next line' to display last trace entry
goto display_messages_in_usersgraph;
}
// ************* SET VFO FT-1000 and FT-920  ********************************
if (((strcmp(list[transceiver_number].name,"FT-1000    ")==0))
   |((strcmp(list[transceiver_number].name,"FT-920     ")==0)))
{
// write delay in msec between two bytes
int   write_delay =5;
// CAT command definition for setting the VFO freq:
//  the command to set the VFO to p.e. 014.250.000 Hz is encoded as 
//  0x00 0x50 0x42 0x01 0x0a 
//  the last byte 0x0a is the "set freq" command code
unsigned char setfreq_cmnd[5]= { 0x00, 0x00, 0x00, 0x00, 0x0a};
// CONVERT the content of "freq" variable to the CAT command format: 
//  discard the least significant digit of "freq"
//  convert the next 8 digits of "freq" to packed bcd and store them in
//  setfreq_cmd,
//  put the first digit-pair in first output byte 
freq /= 10;
for (i=0; i < 4; i++) {
   a = freq%10;
   freq /= 10;
   a |= (freq%10)<<4;
   freq /= 10;
   setfreq_cmnd[i]= a;
   }
// WRITE CAT command to the serial port
for (i=0;i<5;i=i+1)
 {
  n=lir_write_serport((char*)&setfreq_cmnd[i],1);
  if (n<0)  goto ft1000_exit;             //an error occured
// printf("\n %#4.2x ",setfreq_cmnd[i]);  //trace each character to screen
  lir_sleep(write_delay*1000);                        
 }
ft1000_exit:;
//printf("\n");                          //trace last character
goto display_messages_in_usersgraph;
}

// ********************* SET VFO FT-450 and FT-950 and FT-2000 *************************
if   (((strcmp(list[transceiver_number].name,"FT-450     ")==0))
     |((strcmp(list[transceiver_number].name,"FT-950     ")==0))
     |((strcmp(list[transceiver_number].name,"FT-2000    ")==0)))
{
// CAT command definition for setting the VFO freq:
//   To set the VFO frequency to 14.250.000 Hz the command is encoded as  
//   FA14250000; (ASCII values)
//   "FA" is the "SET VFO A "command code
//   freq in Hz; 8 digits, leading zeros, no decimal point,
//   ";" is a delimiter
// CONVERT the content of "transceiver_freq" variable to the CAT command format: 
sprintf (s,"FA%08.0f;",transceiver_freq);
// WRITE the CAT command to the serial port
n=lir_write_serport(s,11);
//printf("\n %s",s); //trace output to screen
//printf("\n");      //'next line' to display last trace entry
goto display_messages_in_usersgraph;
}

// *************************** SET VFO FT-817 and FT-897 *********************************
if (((strcmp(list[transceiver_number].name,"FT-817     ")==0))
   |((strcmp(list[transceiver_number].name,"FT-897     ")==0)))
 
{
// write delay in msec between two bytes
int   write_delay = 10;
// CAT command definition:
//  setting the  VFO frequency to 439.70 MHz is encoded as  
//  0x43 0x97 0x00 0x00 0x01 (hex values)
//  last byte 0x01 is the "SET FREQUENCY" command code. 
unsigned char setfreq_cmnd[5]= { 0x00, 0x00, 0x00, 0x00, 0x01};
// CONVERT the content of "freq" variable to the setfreq_cmd format: 
//  discard least significant digit of freq
//  convert next 8 digits of freq to packed bcd and store in setfreq_cmd
//  put first digit-pair in last output byte 
freq /= 10;
for (i=0; i < 4; i++) {
   a = freq%10;
   freq /= 10;
   a |= (freq%10)<<4;
   freq /= 10;
   setfreq_cmnd[3-i]= a;
   }
// WRITE setfreq_cmd to the serial port
unsigned char cat_cmnd[5];
int write_cat_cmnd(void)
{
  int r;
  for (i=0;i<5;i=i+1)
   {
    r=lir_write_serport((char*)&cat_cmnd[i],1);
    if (r<0) return -1;
// printf("\n %#4.2x ",cat_cmnd[i]);  //trace output to screen
    lir_sleep(write_delay*1000); 
   }
  return 0;
}
memcpy(cat_cmnd,setfreq_cmnd, 5);
n=write_cat_cmnd();
//printf("\n");  //add 'next line' to display last trace entry
goto display_messages_in_usersgraph;
}

// **************************** SET ORION I or II *****************************
if (strcmp(list[transceiver_number].name,"ORION      ")==0)
{
// CAT command definition for setting the VFO freq:
//   *AF is the "set VFO-A frequency" command code
//   Freq in Hz; 8 digits, leading zeros, no decimal point, trailing carriage return.
// CONVERT transceiver_freq to the CAT command format
sprintf (s,"*AF%08.0f\r",transceiver_freq);
// WRITE CAT command to serial port  
n=lir_write_serport(s,12);
//printf("\n %s",s);    //trace output to screen
//printf("\n");         //add 'next line' to trace output
goto display_messages_in_usersgraph;
}

// ************************ SET VFO CATCH_ALL**********************************
ug_msg_color=12;
sprintf (ug_msg0,"INTERNAL ERROR ");
goto userdefined_q_exit_1;

// ***********************END 'SET VFO' ROUTINES ******************************

// display messages in usergraph
display_messages_in_usersgraph:;
if (n<0)
  {
  ug_msg_color=12;
  sprintf (ug_msg0,"WRITE TO %s FAILED",serport_name);
  goto userdefined_q_exit_1;
  }
ug_msg_color=14;
sprintf (ug_msg0,"LINRAD FREQ = ");
edit_ugmsg(ug_msg0,floor(hwfreq*1000));
userdefined_q_exit_2:;
sprintf (ug_msg1,"TOTAL OFFSET= ");
edit_ugmsg(ug_msg1,offset_hwfreq*1000);
sprintf (ug_msg2,"TRX FREQ= ");
edit_ugmsg(ug_msg2, transceiver_freq);
//main  exit for userdefined_q
userdefined_q_exit_1:;
show_user_parms();
// ****************************************************************************
// END void userdefined_q(void) routine
// ****************************************************************************
}

void update_users_rx_frequency(void)
{
// This routine is called from the screen thread.
if(fabs(hwfreq-old_hwfreq) > 0.001)
  {
  ug_msg_color=10;
  sprintf (ug_msg0,"LINRAD FREQ= ");
  edit_ugmsg(ug_msg0,floor(hwfreq*1000));
  hide_mouse(ug.xleft, ug.xright,ug.ytop,ug.ybottom);
  sprintf (ug_msg1,"PRESS Q-KEY TO SET TRX FREQ");
  sprintf (ug_msg2," ");
  show_user_parms();
  settextcolor(7);
  }
}  
// ****************************************************************************
// WSE Rx and Tx and SDR-14/SDR-IQ Rx routines
// ****************************************************************************
#if (RX_HARDWARE == 0)
#include "wse_sdrxx.c"
#endif
// ****************************************************************************
// Dummy routines to support converters and other hardwares
// ****************************************************************************
#if (RX_HARDWARE == 1)
void control_hware(void){};
void set_hardware_tx_frequency(void){};

void set_hardware_rx_frequency(void)
{
fg.passband_increment=0.01;
fg.passband_direction = 1;
fft1_direction = fg.passband_direction;
}

void set_hardware_rx_gain(void){};
void hware_interface_test(void){};
void hware_set_ptt(int state)
  {
  int i;
  i=state;
  };
void hware_hand_key(void){};
void clear_hware_data(void){};
#endif


