


// ****************************************************************
// Change the define for RX_HARDWARE and modify the dummy routines
// in the prototype section to fit your needs.
// The routines in wse_sdrxx.c can serve as a starting point.
#define RX_HARDWARE 0
// ****************************************************************

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
//added August 2007
#include <time.h>

#include "sigdef.h" // Remove this define in linrad-03.01 and later.
// The definition of baseb_reset_counter is moved from sigdef.h to uidef.h
#include "screendef.h" //w3sz

extern int w3sz_offset_hz;  //w3sz offset equal to ug.par2 to be used elsewhere in program
extern int w3sz_offset_hz_old;  //w3sz 
extern int w3sz_users_flag;//w3sz

#ifndef W3SZ_VARIABLES
#define W3SZ_VARIABLES
int w3sz_offset_hz;  //w3sz offset equal to ug.par2 to be used elsewhere in program
int w3sz_offset_hz_old;  //w3sz 
int w3sz_users_flag;//w3sz
#endif

typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
float par1;
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


void userdefined_u(void)
{
// This routine is called when the 'U' key is pressed
// It then sends frequency information to Radio 0 via
// to a file that another program uses to send the info
// across the ethernet to Writelog.  It then mutes Linrad.
// This routine written by w3sz

char ft2[80]="";
char bb[80]="";

float freqpre;
char ft[80]="";
char zro[80]="";
char zr1[80]="";
char ffin[11]=""; 
 
float freq;
float lofreq;

time_t now;
int l_time;
char *ft1000file[19]={"\\lin2ft\\Radio0file"}; 

l_time = time(&now);

FILE *Fp1;

strcat(bb,"bb");
strcat(zro,"0");
strcat(zr1,"0");
 
freqpre=fg.passband_center;
freqpre=freqpre*10E6;
freq=(mix1_fq_mid[fftx_nx]);
freq=freq*10;
freq=freq+freqpre-960000;
freq=freq*100;

/*
The info in mix1_fq_mid[fftx_nx] is only valid when the AFC has
locked to a signal. In case AFC is deselected or the AFC failed
the data may be incorrect. The number 480000 is ui.sampling_speed/2
and it should be applied only in I/Q mode. 
*/
 
freq= hwfreq*1000000.0 ;
freqpre=1000.0*ug.par2;
/* 

This should be correct always. Linrad looks at the flags and selects
the frequency actually in use for conversion to the baseband and adds
the bfo offset 'bg.bfo_freq' in ssb mode.
 
Look at the routine 'frequency_readout' in baseb_graph.c. for
details.
 
*/

// If transverter is used, set par1 to LO frequency and
//  subtract LO value based on user_parameter 'par1' 
// par1 is multiplied by 10**8 to give IF freq in desired units
//if there is no transverter, set par1 to zero
lofreq= ug.par1 * 10E8;
freq=freq-lofreq;


//Add or subtract to put Linrad and FT1000 on same frequency
// 1,000,000 equals 1 kHz; freqpre = 1000*ug.par2, so 
// ug.par2 is entered directly in Hz for correction
freq=freq+freqpre;

//test by sending corrected output to file for review

/*
FILE *Fp2;
Fp2=fopen("aatestfile","a+");
fprintf(Fp2,"%f\n",freq);
fclose(Fp2);
*/

// Round up to compensate for truncation
freq=freq+5000;
freq=freq*10; 
// Add leading zeroes as necessary for proper format
 
sprintf(ft,"%11f",freq);
 
if ((freq < 10E10) && (freq >=10E9))
{
strcat(zro,zro);
strcat(zro,ft);
strncpy(ft2,zro,12);
}
else if (freq < 10E9)
{
strcat(zro,zro);
strcat(zro,zr1);
strcat(zro,ft);
strncpy(ft2,zro,12);
}
 
else
{
strcat(zro,ft);
strncpy(ft2,zro,12);
}
 
strncpy(ffin,ft2,10);
 
Fp1=fopen(*ft1000file,"w");
//fprintf(Fp1,ffin);
fprintf(Fp1,"%s\n%i\n",ffin,l_time);
fclose(Fp1); 


FILE *Fp2;
char *aatestfile[47]={"aatestfile"}; 

Fp2=fopen(*aatestfile,"w");
fprintf(Fp2,"%s\n%i\n",ffin,l_time);
fclose(Fp2);

mix1_selfreq[0]=-1;
lir_empty_da_device_buffer();
baseb_reset_counter++;
}

void users_eme(void)
{
// Called each time moon position is updated.
// data in degrees is available in:
// float moon_az;
// float moon_el;
}


void userdefined_q(void)
{
// This routine is called when the 'Q' key is pressed
// It then sends frequency information to Radio1 via
// to a file that another program uses to send the info
// across the ethernet to Writelog.  It then mutes Linrad.
// This routine written by w3sz
char ft2[80]="";
char bb[80]="";

float freqpre;
char ft[80]="";
char zro[80]="";
char zr1[80]="";
char ffin[11]=""; 
 
float freq;
float lofreq;

time_t now;
int l_time;
char *ft1000file[19]={"\\lin2ft\\Radio1file"}; 

l_time = time(&now);

FILE *Fp1;

strcat(bb,"bb");
strcat(zro,"0");
strcat(zr1,"0");
 
freqpre=fg.passband_center;
freqpre=freqpre*10E6;
freq=(mix1_fq_mid[fftx_nx]);
freq=freq*10;
freq=freq+freqpre-960000;
freq=freq*100;

/*
The info in mix1_fq_mid[fftx_nx] is only valid when the AFC has
locked to a signal. In case AFC is deselected or the AFC failed
the data may be incorrect. The number 480000 is ui.sampling_speed/2
and it should be applied only in I/Q mode. 
*/
 
freq= hwfreq*1000000.0 ;
freqpre=1000.0*ug.par2;
/* 

This should be correct always. Linrad looks at the flags and selects
the frequency actually in use for conversion to the baseband and adds
the bfo offset 'bg.bfo_freq' in ssb mode.
 
Look at the routine 'frequency_readout' in baseb_graph.c. for
details.
 
*/

// If transverter is used, set par1 to LO frequency and
//  subtract LO value based on user_parameter 'par1' 
// par1 is multiplied by 10**8 to give IF freq in desired units
//if there is no transverter, set par1 to zero
lofreq= ug.par1 * 10E8;
freq=freq-lofreq;


//Add or subtract to put Linrad and FT1000 on same frequency
// 1,000,000 equals 1 kHz; freqpre = 1000*ug.par2, so 
// ug.par2 is entered directly in Hz for correction
freq=freq+freqpre;

//test by sending corrected output to file for review

/*
FILE *Fp2;
Fp2=fopen("aatestfile","a+");
fprintf(Fp2,"%f\n",freq);
fclose(Fp2);
*/

// Round up to compensate for truncation
freq=freq+5000;
freq=freq*10; 
// Add leading zeroes as necessary for proper format
 
sprintf(ft,"%11f",freq);
 
if ((freq < 10E10) && (freq >=10E9))
{
strcat(zro,zro);
strcat(zro,ft);
strncpy(ft2,zro,12);
}
else if (freq < 10E9)
{
strcat(zro,zro);
strcat(zro,zr1);
strcat(zro,ft);
strncpy(ft2,zro,12);
}
 
else
{
strcat(zro,ft);
strncpy(ft2,zro,12);
}
 
strncpy(ffin,ft2,10);
 
Fp1=fopen(*ft1000file,"w");
//fprintf(Fp1,ffin);
fprintf(Fp1,"%s\n%i\n",ffin,l_time);
fclose(Fp1); 


FILE *Fp2;
char *aatestfile[47]={"aatestfile"}; 

Fp2=fopen(*aatestfile,"w");
fprintf(Fp2,"%s\n%i\n",ffin,l_time);
fclose(Fp2);

mix1_selfreq[0]=-1;
lir_empty_da_device_buffer();
baseb_reset_counter++;
return;
}
 
//end of w3sz routine


//    ************ PROTOTYPE ROUTINES FOR USER WINDOWS  ***************
// User defined windows to allow mouse control.
// In case several windows are desired, give them different
// numbers (64 and above) and use scro[m].no to decide what to do
// in your mouse_on_users_graph routine.



#define USERS_GRAPH_TYPE1 64



// Define your parameters in this structure.
// Save it to a file and recover it from init_users_control_window(void)

void init_users_control_window(void)
{
char *w3szfile[10]={"aajunkfile"}; 
FILE *Fp;
switch (rx_mode)
  {

case MODE_WCW:
	*w3szfile="aawcw_file";  
	break;
  case MODE_HSMS:
   	*w3szfile="aahsmsfile"; 
  case MODE_QRSS:
 	*w3szfile="aaqrssfile"; 
  break;
  
  case MODE_NCW:
 	*w3szfile="aancw_file"; 
  case MODE_SSB:
	*w3szfile="aassb_file";
 break;
  
  case MODE_FM:
 	*w3szfile="aafm__file"; 
  case MODE_AM:
 	*w3szfile="aaam__file"; 
  case MODE_TXTEST:
 	*w3szfile="aatxtefile"; 
  case MODE_RX_ADTEST:
 	*w3szfile="aarxadfile"; 
  case MODE_TUNE:
 	*w3szfile="aatunefile";

  break;
  }
Fp=fopen(*w3szfile,"r");
if(Fp == NULL)
  {
// Set initial values if there is no file.
  ug.xleft=0;
  ug.ytop=0;
  ug.par1=0.0;
  ug.par2=0.0;
  }
else
  {
  fscanf(Fp,"%d%d%f%f",&ug.xleft,&ug.ytop,&ug.par1,&ug.par2);
  fclose(Fp);
  }
ug.xright=ug.xleft+25*text_width;
ug.ybottom=ug.ytop+3.5*text_height;
if (ug.par2 > -10000 && ug.par2<=10000)//w3sz
{
w3sz_offset_hz=(int)ug.par2; //w3sz
}
else//w3sz
{
w3sz_offset_hz = 0;//w3sz
}
w3sz_users_flag=1;//w3sz

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
// Open a window to allow mouse control of your own things
	init_users_control_window();  
	break;
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


void show_user_parms(void)
{
char s[80];

char *w3szfile[10]={"aajunkfile"}; 

// Show the user parameters on screen
// and issue hardware commands if required.
// Use hware_flag to direct calls to control_hware() when cpu time 
// is available in case whatever you want to do takes too much time 
// to be done immediately.
// Note that mouse control is done in the narrowband processing thread
// and that you can not use lir_sleep to wait for hardware response here.
hide_mouse(ug.xleft, ug.xright,ug.ytop,ug.ybottom);
sprintf(s,"LO(MHz) %10.6f ",ug.par1);
lir_pixwrite(ug.xleft+3*text_width,ug.ytop+text_height,s);
sprintf(s,"SH (Hz) %10.1f ",ug.par2);
lir_pixwrite(ug.xleft+3*text_width,ug.ytop+2*text_height,s);
if (ug.par2 >= -10000 && ug.par2<=10000)//w3sz
{
w3sz_offset_hz=(int)ug.par2; //w3sz
}
else//w3sz
{
w3sz_offset_hz = 0;//w3sz
}
FILE *Fp;

switch (rx_mode)
  {

case MODE_WCW:
	*w3szfile="aawcw_file";  
	break;
  case MODE_HSMS:
   	*w3szfile="aahsmsfile"; 
  case MODE_QRSS:
 	*w3szfile="aaqrssfile"; 
  break;
  
  case MODE_NCW:
 	*w3szfile="aancw_file"; 
  case MODE_SSB:
	*w3szfile="aassb_file";
 break;
  
  case MODE_FM:
 	*w3szfile="aafm__file"; 
  case MODE_AM:
 	*w3szfile="aaam__file"; 
  case MODE_TXTEST:
 	*w3szfile="aatxtefile"; 
  case MODE_RX_ADTEST:
 	*w3szfile="aarxadfile"; 
  case MODE_TUNE:
 	*w3szfile="aatunefile";

  break;
  }

Fp=fopen(*w3szfile,"w");

fprintf(Fp,"%d\n%d\n%f\n%f\n",ug.xleft,ug.ytop,ug.par1,ug.par2);
fclose(Fp);

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
ug.par1=numinput_float_data;
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
    numinput_chars=12;
    erase_numinput_txt();
    numinput_flag=FIXED_FLOAT_PARM;
    par_from_keyboard_routine=new_user_par1;
    }  
   } //added }
  else
    {
// If we did not select a numeric input by setting numinput_flag
// we have to set mouse_active flag.
// Set the routine to mouse_nothing, we just want to
// set flags when the mouse button is released.    
    current_mouse_activity=mouse_nothing;
    mouse_active_flag=1;
    }
// removed }
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


void users_close_devices(void)
{
// WSE units do not use the parallel port.
}

void users_open_devices(void)
{
}



void users_set_band_no(void)
{
// See users_tr.c
}

void update_users_rx_frequency(void)
{
// This routine is called from the screen thread.
// See users_tr.c
}  



// **********************************************************
//         WSE Rx and Tx and SDR-14/SDR-IQ Rx routines
// **********************************************************
#if (RX_HARDWARE == 0)
#include "wse_sdrxx.c"
#endif
// **********************************************************
// Dummy routines to support converters and other hardwares
// **********************************************************
#if (RX_HARDWARE == 1)
void control_hware(void){};
void set_hardware_tx_frequency(void){};

void set_hardware_rx_frequency(void)
{
fg.passband_increment=0.01;
fg.passband_direction = -1;
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





