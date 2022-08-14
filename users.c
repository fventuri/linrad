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


// In case you want to use the WSE converters or the SDR-14
// it is a good idea to fetch the routines for these
// hardwares from wse_sdrxx.c and to place them in here.
// you may subsequently change the routines to allow
// more hardwares or add tx functions, antenna control, whatever.


// Prototype for user defined hardware drive routines.
// These routines will replace the routines in hwaredriver.c
// in case a routine users_hwaredriver.c is present when configure is run.
// The Linrad package does not contain a users_hwaredriver.c file
// so it is safe to copy a nev version on top of an old one.
// The users_hwaredriver.c file will not be overwritten.


void update_users_rx_frequency(void){};

void hware_command(void)
{
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_HWARE_COMMAND);
#endif
thread_status_flag[THREAD_HWARE_COMMAND]=THRFLAG_ACTIVE;
while(!kill_all_flag && 
         thread_command_flag[THREAD_HWARE_COMMAND]==THRFLAG_ACTIVE)
  {
  lir_sleep(30000);
  while(hware_flag != 0)
    {
    control_hware();
    lir_sleep(3000);
    }
  }
thread_status_flag[THREAD_HWARE_COMMAND]=THRFLAG_RETURNED;
while(!kill_all_flag &&
            thread_command_flag[THREAD_HWARE_COMMAND] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

void hware_set_ptt(int state)
{
// Use state to set your Rx/Tx switching.
(void)state;
}

void set_hardware_tx_frequency(void)
{
}



void users_eme(void)
{
// Called each time moon position is updated.
// data in degrees is available in:
// float moon_az;
// float moon_el;
}


void hware_hand_key(void)
{
// This routine is called from the rx_input_thread.
// It is responsible for reading the hardware and setting
// hand_key to TRUE or FALSE depending on whether the hand key
// is depressed or not.
}

void userdefined_u(void)
{
// This routine is called when the 'U' key is pressed
}

// valid ranges: SERPORT_NUMBER = 1 to 4 (1=COM1,4=COM4 )
#define SERPORT_NUMBER 1

void userdefined_q(void)
{
// This routine is called when the 'Q' key is pressed
// The serial port can be opened and closed for each message
// as is done here.
// The serial port can also be opened by the function
// users_open_devices() which is executed by the main
// program before threads are started.
// There is no need to close the serial port because the main
// program checks its flag and closes the port if it is left open.
//
// open and close around each write could cost overhead if
// one writes single characters very often.
//
// Leaving the port open the whole time could leave it in an
// undefined state if Linrad crashes.
//
int n;
int serport_baudrate;
int serport_stopbits;
serport_stopbits=1;
serport_baudrate=9600;
char serport_message []="TTTTTTTTTTTTTTTTTTTTTTTTT";
lir_open_serport(SERPORT_NUMBER,        // 1=COM1 or /dev/ttyS0 , 2=COM2....
                 serport_baudrate,      // baudrate 110 to 57600
                 serport_stopbits,      // 1=two stopbits, 0=one stopbit
                 0);                    // RTS mode = 0
n=lir_write_serport(serport_message,10);
if(n != 10)fprintf( stderr,"\nwrite_serport failed n=%d",n);
lir_close_serport();
}

void users_close_devices(void)
{
// Write whatever you want to the hardware before exit from Linrad.
// the lmain, xmain or wmain will close the serial and/or parallel
// ports in case they have been opened so do not worry about them.
}


void users_open_devices(void)
{
// Uncomment to open the Linux serial port. 
// The routine is in lxsys.c for Linux and wsys.c for Windows.
// lir_open_serport();
// ********************************************************
// Place this elsewhere in case you want to make commands to hardware
// via the serial port. 
// Use for a mouse control box or a user key. 
//
// n=write(serport,hhunhz,1);
// n=write(serport,htenkhz,1); 
// n=write(serport,hmhz,1);
// ********************************************************
}

void set_hardware_rx_gain(void)
{
// Input data:
// int fg.gain
// int fg_new_band
// The argument fg_new_band will allow different hardware on different
// bands. 
// Set hardware gain
// This is the data entered in the rx gain control box.
// This routine is responsible for checking that it is compatible to
// hardware capabilities and adjusting it accordingly.
// this routine is also responsible for sending the control information
// to the hardware. Either by setting hware_flag=1 which will
// cause calls to control_hware() until hware_flag is cleared.
// (look at hwaredriver.c to see how it works to make serial data
// on the parallel port).
// In case the hardware can be controlled by the serial port, 
// just write the control informatio to the serial port here.
//************************************************************
// This routine is responsible for setting
// int fg.gain_increment = The increment in dB for clicking
// the arrows in the gain control box.
}

void set_hardware_rx_frequency(void)
{
// Input data:
// double fg.passband_center = The midpoint of the waterfall in MHz.
// This is the data entered in the rx frequency control box.
// This routine is responsible for checking that it is compatible to
// hardware capabilities and adjusting it accordingly.
// this routine is also responsible for sending the control information
// to the hardware. Either by setting hware_flag=1 which will
// cause calls to control_hware() until hware_flag is cleared.
// (look at hwaredriver.c to see how it works to make serial data
// on the parallel port).
// In case the hardware can be controlled by the serial port, 
// just write the control informatio to the serial port here.
//************************************************************
// This routine is responsible for setting
// double fg.passband_increment = The increment in MHz for clicking
// the arrows in the rx control box.
//************************************************************
// This routine is responsible for setting
// int fg.passband_direction = -1 or +1 depending on whether the
// hardware local oscillator is above or below the desired signal. 
//************************************************************
// This routine is responsible for setting
// int fft1_direction = fg.passband_direction
// Here is an example:
fg.passband_increment=0;
fg.passband_direction = 1;
fft1_direction = fg.passband_direction;
// You may override the frequency that has been entered to Linrad
// by setting whatever value that your hardware accepts. 
// Send fg.passband center to your hardware and read whatever
// value that the hardware returns (serial or parallel port,
// network or whatever)
// Or just set the value of your center frequency if it is a fixed
// frequency. Like this:
// fg.passband_center=28.125;
}


//    ************ PROTOTYPE ROUTINES FOR USER WINDOWS  ***************
// User defined windows to allow mouse control.
// In case several windows are desired, give them different
// numbers (64 and above) and use scro[m].no to decide what to do
// in your mouse_on_users_graph routine.
#define USERS_GRAPH_TYPE1 64
// Define your parameters in this structure.
// Save it to a file and recover it from init_users_control_window(void)

typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int par1;
int par3;
float par2;
} UG_PARMS;

#define UG_TOP 0
#define UG_BOTTOM 1
#define UG_LEFT 2
#define UG_RIGHT 3
#define UG_INCREASE_PAR1 4
#define UG_DECREASE_PAR1 5
#define UG_DO_WHATEVER 6
#define MAX_UGBUTT 7

UG_PARMS ug;
BUTTONS ugbutt[MAX_UGBUTT];

int ug_old_y1;
int ug_old_y2;
int ug_old_x1;
int ug_old_x2;
int users_graph_scro;
void make_users_control_graph(void);


void show_user_parms(void)
{
char s[80];
// Show the user parameters on screen
// and issue hardware commands if required.
// Use hware_flag to direct calls to control_hware() when cpu time 
// is available in case whatever you want to do takes too much time 
// to be done immediately.
// Note that mouse control is done in the narrowband processing thread
// and that you can not use lir_sleep to wait for hardware response here.
hide_mouse(ug.xleft, ug.xright,ug.ytop,ug.ybottom);
sprintf(s,"Par1 %d ",ug.par1);
lir_pixwrite(ug.xleft+6*text_width,ug.ytop+text_height,s);
sprintf(s,"Par2 %f ",ug.par2);
lir_pixwrite(ug.xleft+6*text_width,ug.ytop+2*text_height,s);
}


void mouse_continue_users_graph(void)
{
switch (mouse_active_flag-1)
  {
// Move border lines immediately.
// for other functions, wait until button is released.
// Look in freq_control.c how to move a fixed size window.
  case UG_TOP:
  graph_borders((void*)&ug,0);
  ug.ytop=mouse_y;
  graph_borders((void*)&ug,15);
  break;

  case UG_BOTTOM:
  graph_borders((void*)&ug,0);
  ug.ybottom=mouse_y;
  graph_borders((void*)&ug,15);
  break;
  
  case UG_LEFT:
  graph_borders((void*)&ug,0);
  ug.xleft=mouse_x;
  graph_borders((void*)&ug,15);
  break;
  
  case UG_RIGHT:
  graph_borders((void*)&ug,0);
  ug.xright=mouse_x;
  graph_borders((void*)&ug,15);
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
if( (ui.network_flag&NET_RX_INPUT) == 0)

  {
  switch (mouse_active_flag-1)
    {
    case UG_INCREASE_PAR1: 
    ug.par1++;
    break;
  
    case UG_DECREASE_PAR1:
    ug.par1--;
    break;

    case UG_DO_WHATEVER: 
// Issue hardware commands or fill the screen
// with whatever you like.
// Use hware_flag to direct calls to control_hware() when cpu time 
// is available in case whatever you want to do takes too much time 
// to be done immediately.
    ug.par3++;
    if(ug.par3 < 16)
      {
      settextcolor(ug.par3);
      lir_text(0,screen_last_line-8,"USERS 'DO WHATEVER' PRESSED");
      }
    else
      {
      ug.par3=0;
      lir_text(0,screen_last_line-8,"                           ");
      }  
    settextcolor(7);  
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
}

void new_user_par1(void)
{
ug.par1=numinput_int_data;
pause_thread(THREAD_SCREEN);
show_user_parms();
resume_thread(THREAD_SCREEN);
}

void new_user_par2(void)
{
ug.par2=numinput_float_data;
pause_thread(THREAD_SCREEN);
show_user_parms();
resume_thread(THREAD_SCREEN);
}




void mouse_on_users_graph(void)
{
int event_no;
// First find out is we are on a button or border line.
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
// Prompt the user for par1 or par2 depending on whether the mouse is
// in the upper or the lower part of the window.
mouse_active_flag=1;
numinput_xpix=ug.xleft+11*text_width;
if(mouse_x > ug.xleft+2*text_width && mouse_x<ug.xright-2*text_width)
  {
  if(mouse_y > (ug.ytop+ug.ybottom)/2)
    {
    numinput_ypix=ug.ytop+2*text_height;
    numinput_chars=12;
    erase_numinput_txt();
    numinput_flag=FIXED_FLOAT_PARM;
    par_from_keyboard_routine=new_user_par2;
    }
  else
    {
    numinput_ypix=ug.ytop+text_height;
    numinput_chars=4;
    erase_numinput_txt();
    numinput_flag=FIXED_INT_PARM;
    par_from_keyboard_routine=new_user_par1;
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


void make_users_control_graph(void)
{
pause_thread(THREAD_SCREEN);
// Set a negative number here.
// In case several windows are desired, give them different
// negative numbers and use scro[m].no to decide what to do
// in your mouse_on_users_graph routine.
scro[users_graph_scro].no=USERS_GRAPH_TYPE1;
// These are the coordinates of the border lines.
scro[users_graph_scro].x1=ug.xleft;
scro[users_graph_scro].x2=ug.xright;
scro[users_graph_scro].y1=ug.ytop;
scro[users_graph_scro].y2=ug.ybottom;
// Each border line is treated as a button.
// That is for the mouse to get hold of them so the window can be moved.
ugbutt[UG_LEFT].x1=ug.xleft;
ugbutt[UG_LEFT].x2=ug.xleft;
ugbutt[UG_LEFT].y1=ug.ytop;
ugbutt[UG_LEFT].y2=ug.ybottom;
ugbutt[UG_RIGHT].x1=ug.xright;
ugbutt[UG_RIGHT].x2=ug.xright;
ugbutt[UG_RIGHT].y1=ug.ytop;
ugbutt[UG_RIGHT].y2=ug.ybottom;
ugbutt[UG_TOP].x1=ug.xleft;
ugbutt[UG_TOP].x2=ug.xright;
ugbutt[UG_TOP].y1=ug.ytop;
ugbutt[UG_TOP].y2=ug.ytop;
ugbutt[UG_BOTTOM].x1=ug.xleft;
ugbutt[UG_BOTTOM].x2=ug.xright;
ugbutt[UG_BOTTOM].y1=ug.ybottom;
ugbutt[UG_BOTTOM].y2=ug.ybottom;
// Draw the border lines
graph_borders((void*)&ug,7);
settextcolor(7);
make_button(ug.xleft+text_width,ug.ybottom-3*text_height/2-2,
                                         ugbutt,UG_DECREASE_PAR1,25);
make_button(ug.xright-text_width,ug.ybottom-3*text_height/2-2,
                                     ugbutt,UG_INCREASE_PAR1,24); 
make_button(ug.xleft+text_width,ug.ybottom-text_height/2-2,
                                         ugbutt,UG_DO_WHATEVER,'A');
show_user_parms();
resume_thread(THREAD_SCREEN);
}

void init_users_control_window(void)
{
// Set initial values in code or by reading
// a file of your own.
ug.xleft=0;
ug.xright=25*text_width;
ug.ytop=0;
ug.ybottom=3.5*text_height;
ug.par1=5;
ug.par2=PI_L;
ug.par3=14;
users_graph_scro=no_of_scro;
make_users_control_graph();
no_of_scro++;
if(no_of_scro >= MAX_SCRO)lirerr(89);
}

void users_init_mode(void)
{
// A switch statement can be used to do different things depending on
// the processing mode.
switch (rx_mode)
  {
  case MODE_WCW:
  case MODE_HSMS:
  case MODE_QRSS:
  break;
  
  case MODE_NCW:
  case MODE_SSB:
// Open a window to allow mouse control of your own things
  init_users_control_window();  
  break;
  
  case MODE_FM:
  case MODE_AM:
  case MODE_TXTEST:
  case MODE_RX_ADTEST:
  case MODE_TUNE:
  break;
  }
}
