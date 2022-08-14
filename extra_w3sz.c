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
#include "screendef.h"
extern int w3sz_offset_hz;  //w3sz offset equal to ug.par2 to be used elsewhere in program
extern int w3sz_offset_hz_old;  //w3sz 
extern int w3sz_users_flag;//w3sz

double users_extra_time;
double users_extra_fast_time;
extern float users_extra_update_interval;
int set_freq_flag;

#ifndef W3SZ_VARIABLES
#define W3SZ_VARIABLES
int w3sz_offset_hz;  //w3sz offset equal to ug.par2 to be used elsewhere in program
int w3sz_offset_hz_old;  //w3sz 
int w3sz_users_flag;//w3sz
#endif

int w3szfreq; //w3sz
int w3szfreqold=0; //w3sz
int w3sz_map65_flag=0; //w3sz
char azellocfile[80]="azel_location.txt";//w3sz
char *w3szfreqfile[80]; //w3sz
FILE *Fp; //w3sz
FILE *Fp1;//w3sz
char w3sz_buffer[80];//w3sz
char w3sz_buffer2[80];//w3sz
char w3sz_trash[80];//w3sz
char azel_loc[80];//w3sz

void init_users_extra(void)
{
char s[80];
// This routine is called just before a receive mode is entered.
// Use it to set your own things from the keyboard.
// Remove the comment statements below for testing purposes 
/*
int line,col;
char s[120];
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
Fp=fopen(azellocfile,"r");//w3sz
if(Fp == NULL)//w3sz
  {
  printf("Cannot locate the Azel.dat locator file\n");//w3sz
  *w3szfreqfile="c:\\MAP65\\azel.dat";//w3sz
  Fp=fopen(azellocfile,"w"); //w3sz
  if(Fp == NULL)//w3sz
    {
    clear_screen();
    sprintf(s,"Cannot open to write: %s", azellocfile);//w3sz
    lir_text(0,5,s);
    lir_text(0,7,
       "Hit escape and create file azel_location.txt in the Linrad directory");
    await_processed_keyboard();
    return;//w3sz
    }
  fprintf(Fp,"c:\\\\MAP65\\\\azel.dat");//w3sz
  fclose(Fp);//w3sz
  }
else//w3sz
{
	fgets(azel_loc,80,Fp);//w3sz
	fclose(Fp);//w3sz
	*w3szfreqfile=azel_loc;//w3sz
	printf("Found %s \n",*w3szfreqfile);//w3sz
	} 

users_extra_time=current_time();
users_extra_fast_time=recent_time;
set_freq_flag=FALSE;
}

void users_extra(void)
{
//w3sz the addition below checks the file w3szfreqfile and if it has changed, it changes main display cursor frequency
char s[120];

if (w3sz_users_flag==0) //w3sz
	{
		w3sz_offset_hz =0; //w3sz
		w3sz_offset_hz_old=0; //w3sz
	}


Fp1=fopen(*w3szfreqfile,"r"); //w3sz
if(Fp1 == NULL)//w3sz
  {
  printf("Cannot locate %s\n",*w3szfreqfile);//w3sz
  Fp1=fopen(*w3szfreqfile,"w"); //w3sz
  if(Fp1 == NULL)//w3sz
    {
    sprintf(s,"Cannot open to write: %s", *w3szfreqfile);//w3sz
    clear_screen();
    lir_text(0,5,s);
    sprintf(s,"Hit escape and make sure %s is correctly written",
                                                       *w3szfreqfile);//w3sz
    lir_text(0,7,s);
    sprintf(s,"in the file %s \n",azellocfile);//w3sz
    lir_text(0,8,s);
    await_processed_keyboard();
    return;
    }
  fprintf(Fp1,"Azel.dat was missing\n");//w3sz
  fprintf(Fp1,"Azel.dat was missing\n");//w3sz
  fprintf(Fp1,"Azel.dat was missing\n");//w3sz
  fprintf(Fp1,"Azel.dat was missing\n");//w3sz
  fprintf(Fp1,"%.0f fQSO\n",remainder(fg.passband_center,1)*1000);//w3sz
  fprintf(Fp1,"fQSO2\n");//w3sz
  fclose(Fp1);//w3sz
  }
else //w3sz
  {
	fgets(w3sz_trash,80,Fp1);//w3sz
	fgets(w3sz_trash,80,Fp1);//w3sz
	fgets(w3sz_trash,80,Fp1);//w3sz
	fgets(w3sz_trash,80,Fp1);//w3sz
	fgets(w3sz_buffer,80,Fp1);//w3sz
	fgets(w3sz_buffer2,80,Fp1);//w3sz
	
	if  (strstr(w3sz_buffer,"fQSO")!=NULL && strstr(w3sz_buffer,"fQSO2")==NULL)//w3sz
	{
	strncpy(w3sz_buffer,w3sz_buffer,3);//w3sz
	w3szfreq=atoi(w3sz_buffer);//w3sz
	}
	fclose(Fp1);//w3sz
  set_freq_flag=TRUE;

  }

}

void users_extra_fast(void)
{
int post;
float t1;
int w3sz_diff=-96000;//w3sz
if((w3szfreq != w3szfreqold) || (w3sz_offset_hz != w3sz_offset_hz_old)) //w3sz
{
	new_mix1_curx[0]=-1;//w3sz

	w3sz_diff = w3szfreq*1000 - remainder(fg.passband_center,1)*1000000; //w3sz
		if (w3sz_diff >= -48000 && w3sz_diff <= 48000) //w3sz
			{
			t1=w3sz_diff + 48000 + w3sz_offset_hz; //w3sz  w3sz_offset_hz is offset in Hz from users_w3sz.c file parameter box
			printf(" %d %.0f %.3f  %.0f %d\n",w3szfreq, t1, fg.passband_center, t1-48000+fg.passband_center*1000000,w3sz_offset_hz); //w3sz added
			}
		else
			{
			printf("Selected Frequency is out of Range\n");//w3sz
			printf("%d\n",w3szfreq);//w3sz
			t1=48000;//w3sz
			}

w3szfreqold = w3szfreq; //w3sz
w3sz_offset_hz_old = w3sz_offset_hz;//w3sz
set_freq_flag=TRUE;

post=make_new_signal(0, t1);//w3sz

if(post)
  {
  sc[SC_FREQ_READOUT]++;
  if(genparm[SECOND_FFT_ENABLE] != 0)sc[SC_HG_FQ_SCALE]++;
  if(genparm[AFC_ENABLE]==0 || genparm[AFC_LOCK_RANGE] == 0)
                                                        sc[SC_BG_FQ_SCALE]++;
  }
if(leftpressed != BUTTON_RELEASED)
  {
  if(post)awake_screen();
  return;
  }
if(post)
  {  
  if(genparm[AFC_ENABLE]!=0 && genparm[AFC_LOCK_RANGE] != 0)
                                                        sc[SC_BG_FQ_SCALE]++;
  awake_screen();
  }
  
leftpressed=BUTTON_IDLE;  
baseb_reset_counter++;
mouse_active_flag=0;
set_freq_flag=FALSE;

}
}
