// program extra.c for Linrad
// This demo program displays the following information in the two upper lines
// on the Linrad 'receive mode' screens:
// -the selected frequency
// -the selected mode
// -the selected bandwith
// -the selected input device
// -the selected network-modes
// -the UTC date and time
//
//  Pierre/ON5GN
//
// UPDATE HISTORY
//   7 sept  2004  first version for linrad 01.24
//  23 may   2010  BASEB16 and BASEB24 network-modes added
//                 layout changes
//                 documentation updates
//  27 june  2010  input device information added
//  
// INSTALLATION
//
// To install under Linux for Linux:
//   download lirxx-xx.tbz and extract it to your linrad directory
//   rename extra.c to users_extra.c in your linrad directory 
//   run ./configure and  make xlinrad (or make linrad) from your linrad directory
// 
// To generate a windows version under Linux:
//   install mingw32 from Leif's website
//   download lirxx-xx.tbz and extract it to your linrad directory
//   rename extra.c to users_extra.c in your linrad directory 
//   run ./configure and  make linrad.exe from your linrad directory
//
// To generate a windows version under Windows:
//   Install mingw and nasm from Leif's website in the C:\MinGW  directory
//   Download lirxx-xx.zip and unpack the linrad sources in your linrad
//   directory
//   rename extra.c to users_extra.c in your linrad directory 
//   Start a MS-DOS Command Window on your computer: click the Windows Start
//   menu and enter the "cmd.exe" command into the run box
//   In the MS-DOS Command Window issue a "cd" command to your linrad
//   directory, followed by a "configure.exe" and "make.bat" command
//

#include <time.h>
#include <string.h>


double users_extra_time;
double users_extra_fast_time;
extern float users_extra_update_interval;

void init_users_extra(void)
{
// This routine is called just before a receive mode is entered.
// Use it to set your own things from the keyboard.
// Remove the comment statements below for testing purposes 
/*
int line,col;
char s[80];
col=1;
line=2;
sprintf(s,"THIS ROUTINE init_users_extra IS CALLED JUST BEFORE A RECEIVE MODE IS ENTERED");
lir_text(col,line,s);
sprintf(s,"USE IT TO SET YOUR OWN THINGS FROM THE KEYBOARD");
lir_text(col,line+2,s);
sprintf(s,"HIT ANY KEY TO CONTINUE");
lir_text(col,line+4,s);
await_processed_keyboard();
*/
//
// Set users_extra_update_interval to the highest rate at which
// you want your routines called.
// The users_extra routines will be called only when the CPU is idle
// and if the time interval you have specified is exceeded.
users_extra_update_interval=0.25;
}
 
void users_extra(void)
{
// Demo code for users_extra by ON5GN, Pierre
//
// color codes:
//
//0      - black.
//1      - blue.
//2      - green.
//3      - cyan.
//4      - red.
//5      - magenta.
//6      - brown.
//7      - grey.
//8      - dark grey 
//9      - light blue.
//10     - light green.
//11     - light cyan.
//12     - light red.
//13     - light magenta.
//14     - yellow 
//15     - bright white 
//
struct tm *tm_ptr;
time_t the_time;
char s[80];
char s1[80];
double freq_Mhz;
double freq_khz;
double freq_hz;
double absfreq;
double bw;
double bw_khz;
double bw_hz;

// Hide the mouse while we update the screen so it will
// not be destroyed by what we write.
hide_mouse(0,screen_width,0,text_height);

//display mode
sprintf (s,"MODE=%s",rxmodes[rx_mode]);
settextcolor(12); //red
lir_text (1,1,s);
 
//display bandwith
bw =0.001*baseband_bw_hz;
bw_khz= floor (bw);
bw_hz= floor ((bw-(bw_khz))*1000);
if (bw_khz > 99)
  {
  sprintf (s,"B/W=%.0f kHz     ",bw);
  }
else
  {
  if (bw_khz > 9)
    {
    sprintf (s,"B/W=%.1f kHz     ",bw);
    }
  else
    {
    if (bw_khz > 0)
      {
      sprintf (s,"B/W=%.2f kHz     ",bw);
      }
    else
      {
      if(bw_hz > 9)
        {
        sprintf (s,"B/W=%3.0f Hz    ",baseband_bw_hz);
        }
      else
        {
        sprintf (s,"B/W=%2.2f Hz    ",baseband_bw_hz);
        }
      }  
    }      
  }
settextcolor(14); //yellow
lir_text(26,0,s);

//display frequency
if(mix1_selfreq[0] < 0)
  {
  sprintf (s,"FREQ=        off         ");
  }
else
  {
  absfreq=fabs(hwfreq);
  freq_Mhz= floor(absfreq/1000) ;
  freq_khz= floor (absfreq-(freq_Mhz*1000)) ;
  freq_hz=  floor (absfreq*1000-(freq_Mhz*1000000)-(freq_khz*1000)) ;
  if (freq_Mhz != 0)
    {
    if(hwfreq < 0)freq_Mhz=-freq_Mhz;  
    sprintf (s,"FREQ=%3.0f %03.0f.%03.0f kHz  ",freq_Mhz,freq_khz,freq_hz);
    }
  else 
    {
    if(hwfreq < 0)freq_khz=-freq_khz;  
    sprintf (s,"FREQ= %3.0f.%03.0f kHz        ",freq_khz,freq_hz);
    }
  }
settextcolor(10); //  green
lir_text(1,0,s);

//display UTC date and time 
(void) time(&the_time);
tm_ptr=gmtime(&the_time);
sprintf(s,"DATE=%2d/%02d/%04d   TIME=%02d:%02d:%02d ",
               tm_ptr->tm_mday, tm_ptr->tm_mon+1,tm_ptr->tm_year+1900, 
               tm_ptr->tm_hour,tm_ptr->tm_min, tm_ptr->tm_sec );
settextcolor (15);  //15   white bright
lir_text(75,0,s);



  settextcolor(11);   //11  bright cyan
  display_rx_input_source(&s[0]);
  sprintf(s1,"INPUT=%s",s); 
  lir_text(45,0,s1);
  settextcolor(7);



// display NETWORK OUTPUT mode(s)
s1[0]='\0';
if((ui.network_flag & NET_RXOUT_RAW16) != 0)  strcpy ( s1,"RAW16");
if((ui.network_flag & NET_RXOUT_RAW18) != 0)  strcpy ( s1,"RAW18");
if((ui.network_flag & NET_RXOUT_RAW24) != 0)  strcpy ( s1,"RAW24");
if((ui.network_flag & NET_RXOUT_FFT1)  != 0)  strcpy ( s1,"FFT1");
if((ui.network_flag & NET_RXOUT_TIMF2) != 0)  strcpy ( s1,"TIMF2");
if((ui.network_flag & NET_RXOUT_FFT2)  != 0)  strcpy ( s1,"FFT2");
if((ui.network_flag & NET_RXOUT_BASEB)  != 0)  strcpy ( s1,"BASEB16");
if((ui.network_flag & NET_RXOUT_BASEBRAW) != 0)  strcpy ( s1,"BASEB24");
if (strlen(s1) != 0) 
{
sprintf (s,"NETWORK OUTPUT = %s ",s1);
settextcolor(13); //13  bright magenta
lir_text(45,1,s);
settextcolor(7);
}
}

void users_extra_fast(void)
{
}

