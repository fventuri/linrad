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


//w3sz added

#include <string.h>
#include "uidef.h"
#include "screendef.h"
#include "hwaredef.h"
#include "txdef.h"
#include "fft1def.h"
#include "thrdef.h"
#include <stdio.h>  // added by w3sz
#include <stdlib.h> //added by w3sz

extern int fg_new_band;


char rx_hware_init_flag;

char *wse_parfil_name_ser="par_wse";
char *wse_parm_text_ser[MAX_WSE_PARM]={"Parport",         // 0
                                   "Parport pin",     // 1
                                   "Libusb version",  // 2
                                   "RX2500",          // 3
                                   "RX10700[0]",      // 4
                                   "RX10700[1]",      // 5
                                   "RX10700[2]",      // 6
                                   "RX10700[3]",      // 7
                                   "RX70[0]",         // 8
                                   "RX70[1]",         // 9
                                   "RX70[2]",         //10
                                   "RX70[3]",         //11
                                   "RX70[4]",         //12
                                   "RX144[0]",        //13
                                   "RX144[1]",        //14
                                   "RX144[2]",        //15
                                   "RX144[3]",        //16
                                   "RXhfa[0]",        //17
                                   "RXhfa[1]",        //18
                                   "RXhfa[2]",        //19
                                   "RXhfa[3]",        //20
                                   "RXhfa[4]",        //21
                                   "TX2500",          //22
                                   "TX10700[0]",      //23
                                   "TX10700[1]",      //24
                                   "TX10700[2]",      //25
                                   "TX10700[3]",      //26
                                   "TX70[0]",         //27
                                   "TX70[1]",         //28
                                   "TX70[2]",         //29
                                   "TX70[3]",         //30
                                   "TX70[4]",         //31
                                   "TX144[0]",        //32
                                   "TX144[1]",        //33
                                   "TX144[2]",        //34
                                   "TX144[3]",        //35
                                   "TXhfa[0]",        //36
                                   "TXhfa[1]",        //37
                                   "TXhfa[2]",        //38
                                   "TXhfa[3]",        //39
                                   "TXhfa[4]"};       //40


char hware_outdat;
char device_bits[256];  
int device_data[256];
unsigned char hware_updates[256];
char *device_name[256];
int hware_retry;
int hware_device_no;
int hware_data;
int hware_bits;
int hware_no_of_updates;
int fg_old_band;

//below added by w3sz

char strw3sz[80];
int serport_number;
int portno;   
int serport_baudrate;
int serport_stopbits;  // 0 is one stop bit; 1 is two stop bits
int serport_rts_mode;
int serport_open_status;
int portno;
int pass_no = 0;
int data_array[5] = {75,75,75,75,75};
int device_counter = 0;
unsigned char device_number[5] = {' ',' ',' ',' ',' '};
unsigned char oldoutdat = {' '};
char *w3szfile[11]={"comportfile"}; 
FILE *Fp;
 
//end added by w3sz

typedef struct {
double increment;
double lowest;
double highest;
int direction;
int label;
} FQ_INFO;

// **********************************************************
//                  WSE TX drive routines
// **********************************************************
// Associate a number to each frequency band that will
// need a separate case in set_hardware_tx_frequency.
#define TX_AUDIO_MODE 0
#define TX2500_TO_ANT 1
#define TX10700_TO_ANT 2
#define TX70_TO_ANT 3
#define TX144_TO_ANT 4
#define TXHFA_TO_ANT_1800 5
#define TXHFA_TO_ANT_3500 6
#define TXHFA_TO_ANT_7000 7
#define TXHFA_TO_ANT_10000 8
#define TXHFA_TO_ANT_14000 9
#define MAX_TX_BANDS 10
#define TX_BASEBAND_MINFREQ 13000 //13 kHz
#define TX_BASEBAND_MAXFREQ 38000 //38 kHz (13+25kHz)

FQ_INFO tx_fqinfo_ser[MAX_TX_BANDS]={
            {0.0  ,   0.0  ,   0.0  , 1, TX_AUDIO_MODE},
            {0.025,   1.675,   2.150,-1, TXHFA_TO_ANT_1800},
            {0.025,   2.500,   2.500, 1, TX2500_TO_ANT},
            {0.025,   3.475,   3.950,-1, TXHFA_TO_ANT_3500},
            {0.025,   6.975,   7.450,-1, TXHFA_TO_ANT_7000},
            {0.025,   9.975,  10.450,-1, TXHFA_TO_ANT_10000},
            {0.025,  10.675,  10.750,-1, TX10700_TO_ANT},
            {0.025,  13.975,  14.450,-1, TXHFA_TO_ANT_14000},
            {0.025,  69.975,  70.450,-1, TX70_TO_ANT},
            {0.025, 143.975, 145.950,-1, TX144_TO_ANT}}; 
int tx2500_fqcorr_ser=0;
int tx10700_fqcorr_ser[4]={25,-70,-45,-30};
int tx70_fqcorr_ser[5]={250,700,700,500,600};
int tx144_fqcorr_ser[4]={2200,1300,1300,700};

char *device_name_tx10700_ser={"TX10700"};
char *device_name_tx70_ser={"TX70"};
char *device_name_txhfa144_ser={"TX144 or TXHFA"};

// **********************************************************
//                  WSE RX drive routines
// **********************************************************
//
// The radio hardware may be designed in many different ways.
// When the user selects a new frequency there may be several
// different hardware units that have to be changed.
// The routine supplied here is designed to work with the
// RXxxx -> RX70 -> RX10700 -> RX2500 converters.
// If you have some other hardware, you may replace or modify these 
// routines and move them into (w)users_hwaredriver.c   

// Associate a number to each frequency band that will
// need a separate case in set_hardware_rx_frequency.
#define RX_AUDIO_MODE 0
#define RX2500_TO_ANT 1
#define RX10700_TO_ANT 2
#define RX70_TO_ANT 3
#define RX144_TO_ANT 4
#define RXHFA_TO_ANT_1800 5
#define RXHFA_TO_ANT_3500 6
#define RXHFA_TO_ANT_7000 7
#define RXHFA_TO_ANT_10000 8
#define RXHFA_TO_ANT_14000 9
#define MAX_RX_BANDS 10

#define TEST_GROUP_Y1 4
#define TEST_GROUP_Y2 TEST_GROUP_Y1 +10
#define TEST_GROUP_Y3 TEST_GROUP_Y2 +8

char *undefmsg_ser={"Undef"};
char *device_name_rx10700_ser={"RX10700"};
char *device_name_rx70_ser={"RX70"};
char *device_name_rx144_ser={"RX144"};
char *device_name_rxhfa_ser={"RXHFA"};
char *device_name_rxhfa_gain_ser={"Gain(RXHFA)"};


FQ_INFO rx_fqinfo_ser[MAX_RX_BANDS]={
             {0.0  ,   0.0  ,   0.0  , 1, RX_AUDIO_MODE},
             {0.025,   1.675,   2.150,-1, RXHFA_TO_ANT_1800},
             {0.025,   2.500,   2.500, 1, RX2500_TO_ANT},
             {0.025,   3.475,   3.950,-1, RXHFA_TO_ANT_3500},
             {0.025,   6.975,   7.450,-1, RXHFA_TO_ANT_7000},
             {0.025,   9.975,  10.450,-1, RXHFA_TO_ANT_10000},
             {0.025,  10.675,  10.750,-1, RX10700_TO_ANT},
             {0.025,  13.975,  14.450,-1, RXHFA_TO_ANT_14000},
             {0.025,  69.975,  70.450,-1, RX70_TO_ANT},
             {0.025, 143.975, 145.950,-1, RX144_TO_ANT}}; 

extern char hware_outdat;

void control_hware_ser(void) 
//  w3sz this is the modified WSE hardware control routine.
//  it is called by hware_command
//  this routine modified by W3SZ to give serial control of WSE equipment 
//  using an Arduino Duemilanova or Uno or whatever
//  the signals and pin connections are as follows:

//  name		HIGH	LOW		Arduino Pin		LPT DB25 Pin
//  gain_2		1		0			3				2
//  freq_3		2		0			4				3
//  rx144_5		8		0			5				5
//  rx10700_6	16		0			6				6
//  rx70_7		32		0			7				7
//  clock_1		30		50			8				1
//  data_17		4		24			9				17
				
//  Status_10	Status is not used	*				10
				
//  GND												23

{
		
int err;
int n = 0;
n=n;
int ii = 0;
unsigned char outdat;
char s[80];
switch(hware_flag)
  {
  case 1:
  if(hware_no_of_updates <0)
    {
    lirerr(988342);
    printf("Hardware error in control_hware_ser() in wse.c see lirerr 988342");
    return;
    }
  hware_device_no=hware_updates[hware_no_of_updates];
  hware_error_flag=0;
  hware_data=device_data[hware_device_no];
  hware_bits=device_bits[hware_device_no];
// A new unit is selected.
// Write the number of this unit to the data port
  outdat=hware_device_no;
if(outdat != oldoutdat)
{
	oldoutdat = outdat;
    device_number[device_counter] = outdat;
    device_counter = device_counter +1;
}
    //  lir_outb(outdat,wse.parport); //parallel port send data  w3sz note
  hware_flag=2;
// Set clock low and make data the first bit of current output.
  lir_mutex_lock(MUTEX_PARPORT);
  if( (hware_data & 1) == 0)
    {
    hware_outdat=WSE_PARPORT_DATA+WSE_PARPORT_CLOCK;
    //First Set Clock Low Then Set Data High or Low
    data_array[pass_no] = 4;  //Set High w3sz
    pass_no = pass_no + 1; //w3sz increment pass_no pass number
    }
  else
    {
    hware_outdat=WSE_PARPORT_CLOCK;
    data_array[pass_no] = 24; // Set Low  w3sz
    pass_no = pass_no + 1; //w3sz increment pass_no pass number
    }  
 
//  lir_outb(hware_outdat,wse_parport_control); //parallel port send data  w3sz note
  lir_mutex_unlock(MUTEX_PARPORT);
  break;

  case 2:
// Set clock high to clock in new data bit.
  lir_mutex_lock(MUTEX_PARPORT);
  hware_outdat^=WSE_PARPORT_CLOCK; 
//  lir_outb(hware_outdat,wse_parport_control); //parallel port send data  w3sz note
  lir_mutex_unlock(MUTEX_PARPORT);
  hware_flag=3;
  hware_retry=0;
  break;

  case 3:

// Read status to see if we really got the data bit into the unit.
//  outdat=lir_inb(wse_parport_status)&wse_parport_ack;  //parallel port receive data  w3sz note
  outdat = 1;  //added by w3sz
//  outdat^=wse_parport_ack_sign;
  err=0;  //added by w3sz
  if(outdat == 0)
    {
    if( (hware_outdat&WSE_PARPORT_DATA) != 0)
      {
      err=1;
      }
    }
  else  
    {
    if( (hware_outdat&WSE_PARPORT_DATA) == 0)
      {
      err=1;
      }
    }
  hware_retry++;
  if(err==1)
    {
 //   if(hware_retry<4)return;
 //   hware_error_flag=1;
    }
  hware_bits--;
  if(hware_bits == 0)
    {
// No more. Deselect unit.
//    lir_outb(0,wse.parport); //parallel port send data  w3sz note
    
    hware_flag=4;  //was 3 changed by w3sz
    return;
    }
  else
    {
// Set clock low and make data the next bit of current output.
    hware_data=hware_data>>1;
    lir_mutex_lock(MUTEX_PARPORT);
    if( (hware_data & 1) == 0)
      {
      hware_outdat=WSE_PARPORT_DATA+WSE_PARPORT_CLOCK;
    data_array[pass_no] = 4; // Set High //w3sz
    pass_no = pass_no + 1; 
      }
    else
        {
    hware_outdat=WSE_PARPORT_CLOCK;
    data_array[pass_no] = 24; // Set Low  //w3sz
    pass_no = pass_no + 1; 
    	}
	}
//    lir_outb(hware_outdat,wse_parport_control); //parallel port send data  w3sz note
    lir_mutex_unlock(MUTEX_PARPORT);
    hware_flag=2;
    
  break;

  case 4:
// Select unit. This time we actually clock the new data 
// from the shift register into the latch.
  outdat=hware_device_no; 
//  lir_outb(outdat,wse.parport); //parallel port send data  w3sz note
  hware_flag=5;
  return;

  case 5:
//  lir_outb(0,wse.parport); //parallel port send data  w3sz note  
  hware_no_of_updates--;
  if(hware_no_of_updates >= 0)
    {
    hware_flag=1;
    }
  else
    {  
    hware_flag=0;
    rx_hware_init_flag=0;
    }
  lir_mutex_lock(MUTEX_PARPORT);
  hware_outdat=WSE_PARPORT_CLOCK;
 // lir_outb(hware_outdat,wse_parport_control); //parallel port send data  w3sz note

  sprintf(strw3sz, "%d",device_number[device_counter -1]);
  n = lir_write_serport(strw3sz,strlen(strw3sz));
  n = lir_write_serport("\n",1);
  lir_sleep(1000);
  
 for(ii = 0; ii<5;ii++)
 {
	 	if (data_array[ii] !=75)
		{
			 sprintf(strw3sz, "%d",50);
  			 n = lir_write_serport(strw3sz,strlen(strw3sz));
  			 n = lir_write_serport("\n",1);
  			 lir_sleep(1000);
  			 sprintf(strw3sz, "%d",data_array[ii]);
  			 n = lir_write_serport(strw3sz,strlen(strw3sz));
  			 n = lir_write_serport("\n",1);
  			 lir_sleep(1000);
  			 sprintf(strw3sz, "%d",30);
  			 n = lir_write_serport(strw3sz,strlen(strw3sz));
  			 n = lir_write_serport("\n",1);
  			 lir_sleep(1000);
		}
 }	 
  sprintf(strw3sz, "%d",0);
  n = lir_write_serport(strw3sz,strlen(strw3sz));
  n = lir_write_serport("\n",1);
  lir_sleep(1000);
  sprintf(strw3sz, "%d",device_number[device_counter -1]);
  n = lir_write_serport(strw3sz,strlen(strw3sz));
  n = lir_write_serport("\n",1);
  lir_sleep(1000);
  sprintf(strw3sz, "%d",0);
  n = lir_write_serport(strw3sz,strlen(strw3sz));
  n = lir_write_serport("\n",1);
  lir_sleep(1000);
 for (ii=0;ii<5;ii++)
 {
  data_array[ii] = 75; 
 }
  
  lir_mutex_unlock(MUTEX_PARPORT);
  if(hware_error_flag != 0)
    {
    sprintf(s,"  ERROR: %s",device_name[hware_device_no]);
    wg_error(s,WGERR_HWARE);
    }
pass_no = 0;
  break;      
}
device_counter = 0;
}  // end of void control_hardware w3sz note


void hware_command(void)
{
	
users_open_devices();	
	
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
    control_hware_ser();
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


void set_txhfa_data_ser(int band)
{
if(device_data[TX144] != band)
  {
  device_data[TXHFA]=band;
  device_bits[TXHFA]=5;
  hware_no_of_updates++;
  hware_updates[hware_no_of_updates]=TXHFA;
  }
}

int set_tx144_data_ser_ser(int stepno)
{
int i;
i=8;
tx_hware_fqshift+=tx144_fqcorr_ser[stepno/20];
while(stepno >= 20)
  {
  i>>=1;
  stepno-=20;
  }
if(device_data[TX144] != i)
  {
  device_data[TX144]=i;
  device_bits[TX144]=4;
  hware_no_of_updates++;
  hware_updates[hware_no_of_updates]=TX144;
  }
return stepno;  
}

void set_tx10700_data_ser(int stepno)
{
int i;
i=8;
tx_hware_fqshift+=tx10700_fqcorr_ser[stepno];
while(stepno != 0)
  {
  i>>=1;
  stepno--;
  }
if(device_data[TX10700] != i)
  {
  device_data[TX10700]=i;
  device_bits[TX10700]=4;
  hware_no_of_updates++;
  hware_updates[hware_no_of_updates]=TX10700;
  }
}

int set_tx70_data_ser(int stepno)
{
int i;
tx_hware_fqshift+=tx70_fqcorr_ser[stepno/4];
i=16;
while(stepno >= 4)
  {
  i>>=1;
  stepno-=4;
  }
if(device_data[TX70] != i)
  {
  device_data[TX70]=i;
  device_bits[TX70]=5;
  hware_no_of_updates++;
  hware_updates[hware_no_of_updates]=TX70;
  }
return stepno;
}



void set_rxhfa_data_ser(int band)
{
// band   freq
//  16    1.8MHz
//   8    3.5MHz
//   4     7MHz
//   2    10MHz
//   1    14MHz
int i, k;
i=band;
k=0;
while(i!=16)
  {
  i*=2;
  k++;
  }
rx_hware_fqshift+=wse.rxhfa_fqcorr[k];
if(device_data[RXHFA] != band)
  {
  device_data[RXHFA]=band;
  device_bits[RXHFA]=5;
  hware_no_of_updates++;
  hware_updates[hware_no_of_updates]=RXHFA;
  }
}

int set_rx144_data_ser(int stepno)
{
int i;
rx_hware_fqshift+=wse.rx144_fqcorr[stepno/20];
i=8;
while(stepno >= 20)
  {
  i>>=1;
  stepno-=20;
  }
if(device_data[RX144] != i)
  {
  device_data[RX144]=i;
  device_bits[RX144]=4;
  hware_no_of_updates++;
  hware_updates[hware_no_of_updates]=RX144;
  }
return stepno;  
}

void set_rx10700_data_ser(int stepno)
{
int i;
i=8;
rx_hware_fqshift+=wse.rx10700_fqcorr[stepno];
while(stepno != 0)
  {
  i>>=1;
  stepno--;
  }
if(device_data[RX10700] != i)
  {
  device_data[RX10700]=i;
  device_bits[RX10700]=4;
  hware_no_of_updates++;
  hware_updates[hware_no_of_updates]=RX10700;
  }
}

int set_rx70_data_ser(int stepno)
{
int i;
i=16;
rx_hware_fqshift+=wse.rx70_fqcorr[stepno/4];
while(stepno >= 4)
  {
  i>>=1;
  stepno-=4;
  }
if(device_data[RX70] != i)
  {
  device_data[RX70]=i;
  device_bits[RX70]=5;
  hware_no_of_updates++;
  hware_updates[hware_no_of_updates]=RX70;
  }
return stepno;
}



void wse_rx_freq_control_ser(void)
{
float t1;  
int stepno;
int old_updates;
// ***********************************************************************
if( wse.parport == 0 || !allow_wse_parport)return;
if(hware_no_of_updates > 127)lirerr(213768);
fg_new_band=0;
while(fg_new_band < MAX_RX_BANDS-1 && 
     fg.passband_center > rx_fqinfo_ser[fg_new_band].highest)fg_new_band++;
if(fg.passband_center < rx_fqinfo_ser[fg_new_band].lowest && fg_new_band > 0) 
  {
  t1=0.5*(rx_fqinfo_ser[fg_new_band].lowest+rx_fqinfo_ser[fg_new_band-1].highest);
  if(fg.passband_center < t1)fg_new_band--;
  }
if(fg_old_band != fg_new_band)set_hardware_rx_gain();
fg_old_band=fg_new_band;
t1=(fg.passband_center-rx_fqinfo_ser[fg_new_band].lowest)/
                                         rx_fqinfo_ser[fg_new_band].increment;
stepno=t1+0.5;
if(stepno < 0)stepno=0;
set_fq:;
fg.passband_center=rx_fqinfo_ser[fg_new_band].lowest+
                                   stepno*rx_fqinfo_ser[fg_new_band].increment;  
if(fg.passband_center > rx_fqinfo_ser[fg_new_band].highest+0.0000001)
  {
  stepno--;
  goto set_fq;
  }  
fg.passband_increment=rx_fqinfo_ser[fg_new_band].increment;
fg.passband_direction=rx_fqinfo_ser[fg_new_band].direction;
//
// Here the frequency scale can be inverted on some condition.
// Possibly a new user parameter....
//#if(OSNUM == OSNUM_LINUX)
//if( (ui.use_alsa&1) != 0)fg.passband_direction*=-1;
//#endif
// Send a message to the screen thread to redraw the frequency scale
// on the main spectrum.
sc[SC_WG_FQ_SCALE]++;
// Set up data to switch relays to connect units properly.
// and to select the correct LO frequency for all units in use.  
old_updates=hware_no_of_updates;
rx_hware_fqshift=0;
switch (rx_fqinfo_ser[fg_new_band].label)
  {   
  case RX_AUDIO_MODE:
// Send nothing to the hardware, just set frequency
// and direction to zero so frequency scale in waterfall and
// other graphs gives audio frequency.
  break;
     
  case RX2500_TO_ANT:
// Route the signal source to the RX2500 input. 
  rx_hware_fqshift+=wse.rx2500_fqcorr;
  break;
  
  case RX10700_TO_ANT:
// Route RX10700 output to the RX2500 input. 
// Route the signal source to the RX10700 input. 
  goto set_10700;
  
  case RX70_TO_ANT:
// Route RX70 output to the RX10700 input. 
// Route RX10700 output to the RX2500 input. 
// Route the signal source to the RX10700 input. 
  goto set_70;

  case RXHFA_TO_ANT_1800:
// Route RXHFA output to the RX70 input. 
// Route RX70 output to the RX10700 input. 
// Route RX10700 output to the RX2500 input. 
// Route the signal source to the RXHFA input. 
  set_rxhfa_data_ser(16);
  goto set_70;

  case RXHFA_TO_ANT_3500:
// Route RXHFA output to the RX70 input. 
// Route RX70 output to the RX10700 input. 
// Route RX10700 output to the RX2500 input. 
// Route the signal source to the RXHFA input. 
  set_rxhfa_data_ser(8);
  goto set_70;

  case RXHFA_TO_ANT_7000:
// Route RXHFA output to the RX70 input. 
// Route RX70 output to the RX10700 input. 
// Route RX10700 output to the RX2500 input. 
// Route the signal source to the RXHFA input. 
  set_rxhfa_data_ser(4);
  goto set_70;

  case RXHFA_TO_ANT_10000:
// Route RXHFA output to the RX70 input. 
// Route RX70 output to the RX10700 input. 
// Route RX10700 output to the RX2500 input. 
// Route the signal source to the RXHFA input. 
  set_rxhfa_data_ser(2);
  goto set_70;

  case RXHFA_TO_ANT_14000:
// Route RXHFA output to the RX70 input. 
// Route RX70 output to the RX10700 input. 
// Route RX10700 output to the RX2500 input. 
// Route the signal source to the RXHFA input. 
  set_rxhfa_data_ser(1);
  goto set_70;

  case RX144_TO_ANT:
// Route RX144 output to the RX70 input. 
// Route RX70 output to the RX10700 input. 
// Route RX10700 output to the RX2500 input. 
// Route the signal source to the RX10700 input. 
  stepno=set_rx144_data_ser(stepno);
set_70:;
  stepno=set_rx70_data_ser(stepno);
set_10700:;
  set_rx10700_data_ser(stepno);
  rx_hware_fqshift+=wse.rx2500_fqcorr;
  break;
  }
if(hware_no_of_updates != old_updates)
  {
  hware_flag=1;
  hware_time=current_time();
  }
else
  {
  rx_hware_init_flag=0;
  }
}

void wse_rx_amp_control_ser(void)
{
int i;
int gaindat[6]={6,14,3,11,5,13};
if( wse.parport == 0 || !allow_wse_parport)return;
if(hware_no_of_updates > 127)lirerr(213767);
switch (rx_fqinfo_ser[fg_new_band].label)
  {   
  case RXHFA_TO_ANT_1800:
  case RXHFA_TO_ANT_3500:
  case RXHFA_TO_ANT_7000:
  case RXHFA_TO_ANT_10000:
  case RXHFA_TO_ANT_14000:
  fg.gain_increment=5;
  fg.gain/=5;
  fg.gain*=5;
  if(fg.gain < -15)fg.gain=-15;
  if(fg.gain > 10)fg.gain=10;
  i=fg.gain/5+3;
  if(device_data[RXHFA_GAIN] != gaindat[i])
    {
    device_data[RXHFA_GAIN]=gaindat[i];
    device_bits[RXHFA_GAIN]=4;
    hware_no_of_updates++;
    hware_updates[hware_no_of_updates]=RXHFA_GAIN;
    hware_flag=1;
    hware_time=current_time();
    }
  break;
  
  default:
  break;
  }
}


void wse_tx_freq_control_ser(void)
{
int old_updates;
float t1;
int band_no, stepno;
if(ui.tx_da_channels==1)
  {
  tg.band_increment = 0.00001;
  tg.passband_direction = 1;
  tg.band_center=0;
  t1=ui.tx_da_speed/8;
  if(tg.freq > t1)tg.freq=t1;
  if(-tg.freq < -t1)tg.freq=-t1;
  tg_basebfreq_hz=1000000.F*tg.freq;
// Set up data to switch relays to connect units properly.
// and to select the correct LO frequency for all units in use.  
  make_tx_phstep();
  return;
  }
if( wse.parport == 0 || !allow_wse_parport)return;
if(hware_no_of_updates > 127)lirerr(213766);
band_no=0;
while(band_no < MAX_TX_BANDS-1 && tg.freq+TX_BASEBAND_MINFREQ >
                                       tx_fqinfo_ser[band_no].highest)band_no++;
if(tg.freq+TX_BASEBAND_MAXFREQ  < tx_fqinfo_ser[band_no].lowest && band_no > 0) 
  {
  t1=0.5*(tx_fqinfo_ser[band_no].lowest+tx_fqinfo_ser[band_no-1].highest);
  if(tg.freq < t1)band_no--;
  }
t1=tg.freq+0.5*(TX_BASEBAND_MINFREQ-TX_BASEBAND_MAXFREQ);
if(t1<0)t1=0;
stepno=(t1-tx_fqinfo_ser[band_no].lowest)/tx_fqinfo_ser[band_no].increment;
if(stepno < 0)stepno=0;
set_fq:;
tg.band_center=tx_fqinfo_ser[band_no].lowest+stepno*tx_fqinfo_ser[band_no].increment;  
if(tg.band_center > tx_fqinfo_ser[band_no].highest+0.001)
  {
  stepno--;
  goto set_fq;
  } 
tg.band_increment = tx_fqinfo_ser[band_no].increment;
tg.passband_direction = tx_fqinfo_ser[band_no].direction;
if(tg.passband_direction > 0)
  {
  tg_basebfreq_hz=1000000.F*(tg.band_center-tg.freq);
  if(tg_basebfreq_hz < TX_BASEBAND_MINFREQ)tg_basebfreq_hz=TX_BASEBAND_MINFREQ;
  if(tg_basebfreq_hz > TX_BASEBAND_MAXFREQ)tg_basebfreq_hz=TX_BASEBAND_MAXFREQ;
  tg.freq=tg.band_center-0.000001F*tg_basebfreq_hz;
  }
else
  {
  tg_basebfreq_hz=1000000.F*(tg.freq-tg.band_center);
  if(tg_basebfreq_hz < TX_BASEBAND_MINFREQ)tg_basebfreq_hz=TX_BASEBAND_MINFREQ;
  if(tg_basebfreq_hz > TX_BASEBAND_MAXFREQ)tg_basebfreq_hz=TX_BASEBAND_MAXFREQ;
  tg.freq=0.000001F*tg_basebfreq_hz+tg.band_center;
  }
// Set up data to switch relays to connect units properly.
// and to select the correct LO frequency for all units in use.  
old_updates=hware_no_of_updates;
tx_hware_fqshift=0;
switch (tx_fqinfo_ser[band_no].label)
  {   
  case TX_AUDIO_MODE:
// Send nothing to the hardware, just set frequency
// and direction to zero so frequency scale in waterfall and
// other graphs gives audio frequency.
  break;
     
  case TX2500_TO_ANT:
// Route the signal source to the TX2500 input. 
  tx_hware_fqshift+=tx2500_fqcorr_ser;
  break;
  
  case TX10700_TO_ANT:
// Route TX10700 output to the TX2500 input. 
// Route the signal source to the TX10700 input. 
  goto set_10700;
  
  case TX70_TO_ANT:
// Route TX70 output to the TX10700 input. 
// Route TX10700 output to the TX2500 input. 
// Route the signal source to the TX10700 input. 
  goto set_70;

  case TXHFA_TO_ANT_1800:
// Route TXHFA output to the TX70 input. 
// Route TX70 output to the TX10700 input. 
// Route TX10700 output to the TX2500 input. 
// Route the signal source to the TXHFA input. 
  set_txhfa_data_ser(16);
  goto set_70;

  case TXHFA_TO_ANT_3500:
// Route TXHFA output to the TX70 input. 
// Route TX70 output to the TX10700 input. 
// Route TX10700 output to the TX2500 input. 
// Route the signal source to the TXHFA input. 
  set_txhfa_data_ser(8);
  goto set_70;

  case TXHFA_TO_ANT_7000:
// Route TXHFA output to the TX70 input. 
// Route TX70 output to the TX10700 input. 
// Route TX10700 output to the TX2500 input. 
// Route the signal source to the TXHFA input. 
  set_txhfa_data_ser(4);
  goto set_70;

  case TXHFA_TO_ANT_10000:
// Route TXHFA output to the TX70 input. 
// Route TX70 output to the TX10700 input. 
// Route TX10700 output to the TX2500 input. 
// Route the signal source to the TXHFA input. 
  set_txhfa_data_ser(2);
  goto set_70;

  case TXHFA_TO_ANT_14000:
// Route TXHFA output to the TX70 input. 
// Route TX70 output to the TX10700 input. 
// Route TX10700 output to the TX2500 input. 
// Route the signal source to the TXHFA input. 
  set_txhfa_data_ser(1);
  goto set_70;

  case TX144_TO_ANT:
// Route TX144 output to the TX70 input. 
// Route TX70 output to the TX10700 input. 
// Route TX10700 output to the TX2500 input. 
// Route the signal source to the TX10700 input. 
  stepno=set_tx144_data_ser_ser(stepno);
set_70:;
  stepno=set_tx70_data_ser(stepno);
set_10700:;
  set_tx10700_data_ser(stepno);
  tx_hware_fqshift+=tx2500_fqcorr_ser;
  break;
  }
make_tx_phstep();
if(hware_no_of_updates != old_updates)
  {
  hware_flag=1;
  hware_time=current_time();
  }
}



void set_hardware_tx_frequency(void)
{
if(ui.tx_dadev_no < SPECIFIC_DEVICE_CODES)
  {
  switch (ui.tx_soundcard_radio)
    {
    case TX_SOUNDCARD_RADIO_WSE:
    wse_tx_freq_control_ser();
    break;
    
    case TX_SOUNDCARD_RADIO_SI570:
// Set the transmitter carrier frequency to tg.freq. 
    tg.passband_direction=si570.tx_passband_direction;
    tg.band_increment = 0.00001;
    switch (ui.transceiver_mode )
      {
      case 0:
// The transmitter has its own Si570
      break;
      
      case 1:
// The Si570 is common to Rx and Tx. Keep the Si570 frequency constant
// and change tg_basebfreq_hz as needed.
      tg_basebfreq_hz=1000000*(fg.passband_center-tg.freq);
      if(tg_basebfreq_hz > (float)ui.tx_da_speed*0.4F)
        {
        tg_basebfreq_hz=(float)ui.tx_da_speed*0.35F;
        tg.freq=fg.passband_center-tg_basebfreq_hz/1000000.F;
        }
      if(tg_basebfreq_hz < -(float)ui.tx_da_speed*0.4F)
        {
        tg_basebfreq_hz=-(float)ui.tx_da_speed*0.35F;
        tg.freq=fg.passband_center-tg_basebfreq_hz/1000000.F;
        }
      break;
      
      case 2:
// The Si570 is common to Rx and Tx. Keep the audio tone, tg_basebfreq_hz 
// constant and change the Si570 between Rx and tx.
      tx_passband_center=tg.freq+tg_basebfreq_hz/1000000.F;
      break;
      }
    make_tx_phstep();
    break;
    }
  }
else
  {
  switch (ui.tx_dadev_no)
    {
    case BLADERF_DEVICE_CODE:
    break;

    case OPENHPSDR_DEVICE_CODE:
    break;
    }
  }  
}

void hware_hand_key(void)
{
int i;
// This routine is called from the rx_input_thread.
// It is responsible for reading the hardware and setting
// hand_key to TRUE or FALSE depending on whether the hand key
// is depressed or not.
if(ui.tx_dadev_no < SPECIFIC_DEVICE_CODES)
  {
  switch (ui.tx_soundcard_radio)
    {
    case TX_SOUNDCARD_RADIO_WSE:
// disable the polling of the parallelport when USB2LPT is selected 
// (otherwise we get RX OVERRUN ERRORS )
    if(( wse.parport != 0 && allow_wse_parport) && 
                               wse.parport != USB2LPT16_PORT_NUMBER )
      {
      i=lir_inb(wse_parport_status)&WSE_PARPORT_MORSE_KEY;
      if(i==0)
        {
        hand_key=TRUE;
        }
      else
        {
        hand_key=FALSE;
        }
      }
    break;
    
    case TX_SOUNDCARD_RADIO_SI570:
    if(ui.ptt_control == 1)
      {
// We use the pilot tone to control PTT.       
      si570_ptt=0;
      }
    si570_get_ptt();
    break;
    }
  }
else
  {
  switch (ui.tx_dadev_no)  
    {
    case BLADERF_DEVICE_CODE:
    break;

    case OPENHPSDR_DEVICE_CODE:
    break;
    }
  }  
}  

void hware_set_ptt(int state)
{
if(ui.tx_dadev_no < SPECIFIC_DEVICE_CODES)
  {
  switch (ui.tx_soundcard_radio)
    {
    case TX_SOUNDCARD_RADIO_WSE:
    if( wse.parport != 0 && allow_wse_parport)
      {
      lir_mutex_lock(MUTEX_PARPORT);
      if(state == TRUE)
        {
        hware_outdat&=-1-WSE_PARPORT_PTT;
        }
      else
        {
        hware_outdat|=WSE_PARPORT_PTT;
        }
      lir_mutex_unlock(MUTEX_PARPORT);
      lir_outb(hware_outdat,wse_parport_control);
      }
    break;
    
    case TX_SOUNDCARD_RADIO_SI570:
    if(ui.ptt_control == 1)
      {
// We use the pilot tone to control PTT.       
      si570_ptt=0;
      }
    else
      {  
      if(si570_ptt != state)
        {
        si570_ptt=state;
        si570_set_ptt();
        }
      }
    break;
    }
  }
else
  {
  switch (ui.tx_dadev_no)  
    {
    case BLADERF_DEVICE_CODE:
    break;

    case OPENHPSDR_DEVICE_CODE:
    break;
    }
  }  
    
}

void set_hardware_rx_gain(void)
{
int i, j, k, m;
float t1;
if(diskread_flag >= 2)return;
if(ui.use_extio != 0)
  {
  update_extio_rx_gain();
  return;
  }
if(ui.rx_addev_no == SDR14_DEVICE_CODE)
  {
  fg.gain_increment=10;
  fg.gain/=10;
  fg.gain*=10;
  if(fg.gain > 0)fg.gain=0;
  if(fg.gain<-30)fg.gain=-30;
  sdr_att_counter++;
  return;
  }
if(ui.rx_addev_no == SDRIQ_DEVICE_CODE)
  {
  fg.gain_increment=1;
  if(fg.gain > 0)fg.gain=0;
  if(fg.gain<-36)fg.gain=-36;
  sdr_att_counter++;
  return;
  }
if(ui.rx_addev_no == SDRIP_DEVICE_CODE)
  {
  fg.gain_increment=10;
  fg.gain/=10;
  fg.gain*=10;
  if(fg.gain > 0)fg.gain=0;
  if(fg.gain<-30)fg.gain=-30;
  sdr_att_counter++;
  return;
  }
if(ui.rx_addev_no == PERSEUS_DEVICE_CODE)
  {
  fg.gain_increment=10;
  fg.gain/=10;
  fg.gain*=10;
  if(fg.gain > 0)fg.gain=0;
  if(fg.gain<-30)fg.gain=-30;
  sdr_att_counter++;
  return;
  }
if(ui.rx_addev_no == EXCALIBUR_DEVICE_CODE)
  {
  fg.gain_increment=3;
  fg.gain/=3;
  fg.gain*=3;
  if(fg.gain > 0)fg.gain=0;
  if(fg.gain<-21)fg.gain=-21;
  sdr_att_counter++;
  return;
  }
if(ui.rx_addev_no == BLADERF_DEVICE_CODE)
  {
  fg.gain_increment=3;
  fg.gain/=3;
  fg.gain*=3;
  if(fg.gain > 21)fg.gain=21;
  if(fg.gain<-21)fg.gain=-21;
  sdr_att_counter++;
  return;
  }
if(ui.rx_addev_no == AIRSPY_DEVICE_CODE)
  {
  fg.gain_increment=4;
  fg.gain/=4;
  fg.gain*=4;
  if(fg.gain > 0)fg.gain=0;
  if(fg.gain<-84)fg.gain=-84;
  sdr_att_counter++;
  return;
  }
if(ui.rx_addev_no == RTL2832_DEVICE_CODE)
  {
  if(rtl2832.gain_mode == 0)
    {
    fg.gain=0;
    return;
    }
  fg.gain_increment=1;
  t1=(float)rint(0.1F*(float)old_rtl2832_gain);
  k=fg.gain-(int)t1;
  if(k == 0)return;
  if(abs(k) == fg.gain_increment)
    {
    j=0;
    m=10000;
    for(i=0; i<no_of_rtl2832_gains; i++)
      {
      if(abs(old_rtl2832_gain-rtl2832_gains[i]) < m)
        {
        m=abs(old_rtl2832_gain-rtl2832_gains[i]);
        j=i;
        }
      }
    if(k > 0)
      {
      j++;
      }
    else
      {
      j--;
      }     
    if(j < 0)j=0;
    if(j >= no_of_rtl2832_gains)j=no_of_rtl2832_gains-1;  
    }
  else
    {
    j=0;
    m=10000;
    for(i=0; i<no_of_rtl2832_gains; i++)
      {
      t1=(float)rint(0.1F*(float)rtl2832_gains[i]);
      if(abs(fg.gain-(int)t1) < m)
        {
        m=abs(fg.gain-(int)t1);
        j=i;
        }
      }
    }
  t1=(float)rint(0.1F*(float)rtl2832_gains[j]);  
  fg.gain=(int)t1;
  old_rtl2832_gain=rtl2832_gains[j];
  sdr_att_counter++;
  return;
  }
if(ui.rx_addev_no == MIRISDR_DEVICE_CODE)
  {
  fg.gain_increment=1;
  k=fg.gain-old_mirics_gain;
  if(k == 0)return;
  if(abs(k) == fg.gain_increment)
    {
    j=0;
    m=10000;
    for(i=0; i<no_of_mirics_gains; i++)
      {
      if(abs(old_mirics_gain-mirics_gains[i]) < m)
        {
        m=abs(old_mirics_gain-mirics_gains[i]);
        j=i;
        }
      }
    if(k > 0)
      {
      j++;
      }
    else
      {
      j--;
      }     
    if(j < 0)j=0;
    if(j >= no_of_mirics_gains)j=no_of_mirics_gains-1;  
    }
  else
    {
    j=0;
    m=10000;
    for(i=0; i<no_of_mirics_gains; i++)
      {
      if(abs(fg.gain-mirics_gains[i]) < m)
        {
        m=abs(fg.gain-mirics_gains[i]);
        j=i;
        }
      }
    }  
  fg.gain=mirics_gains[j];
  old_mirics_gain=mirics_gains[j];
  sdr_att_counter++;
  return;
  }
if(ui.rx_addev_no == NETAFEDRI_DEVICE_CODE)
  {
  fg.gain_increment=3;
  fg.gain+=10;
  fg.gain/=3;
  fg.gain*=3;
  fg.gain-=10;
  if(fg.gain > 35)fg.gain=35;
  if(fg.gain<-10)fg.gain=-10;
  sdr_att_counter++;
  return;
  }
if(ui.rx_addev_no == FDMS1_DEVICE_CODE)
  {
  fg.gain_increment=20;
  if(fg.gain > -10)
    {
    fg.gain=0;
    }
  else
    {
    fg.gain=-20;
    }
  sdr_att_counter++;
  return;
  }
switch (ui.rx_soundcard_radio)
  {
  case RX_SOUNDCARD_RADIO_WSE:
// bit0 to bit 2 for amplifier/bypass/-10 dB.
// bit0-bit2 = 6 => -10dB
// bit0-bit2 = 3 => 0dB
// bit0-bit2 = 5 => +10dB
//
// bit3 for 5 dB attenuator.
// bit3 = 0 => -5dB
// bit3 = 1 =>  0dB
  wse_rx_amp_control_ser();
  break;

  case RX_SOUNDCARD_RADIO_SI570:
  fg.gain=0;
  break;

  case RX_SOUNDCARD_RADIO_ELEKTOR:
  elektor_rx_amp_control();
  break;

  case RX_SOUNDCARD_RADIO_FCDPROPLUS:
  fcdproplus_rx_amp_control();
  break;
  
  case RX_SOUNDCARD_RADIO_AFEDRI_USB:
  afedriusb_rx_amp_control();
  break;
  }
}


void users_set_band_no(void){}
void users_eme(void){}

void set_hardware_rx_frequency(void)
{
float t1;
int i;
if(diskread_flag >= 2)
  {
  sc[SC_WG_FQ_SCALE]++;
  return;
  }
if(ui.use_extio != 0)
  {
  if(fg.passband_increment < 0.0001 || fg.passband_increment > 1.5)
    {   
    fg.passband_increment=0.01;
    }
  if(ui.use_extio < 0)
    {  
    fg.passband_direction = -1;
    }
  else
    {
    fg.passband_direction = 1;
    }      
  fft1_direction = fg.passband_direction;
  update_extio_rx_freq();
  }
if(rx_mode < MODE_TXTEST)users_set_band_no();
if(ui.rx_addev_no == SDR14_DEVICE_CODE ||
   ui.rx_addev_no == SDRIQ_DEVICE_CODE )
  {
  t1=(float)fg.passband_center;
  fg.passband_direction=1;
  if(fg.passband_center > adjusted_sdr_clock)
    {
    i=(int)(fg.passband_center/adjusted_sdr_clock);
    t1=(float)(fg.passband_center-i*adjusted_sdr_clock);
    }
  if(t1 > 0.5F*adjusted_sdr_clock)
    {
    t1=(float)adjusted_sdr_clock-t1;
    fg.passband_direction=-1;
    }
  fg.passband_increment=0.1;
  sdr_nco_counter++;
  goto setfq_x;
  }
if(ui.rx_addev_no == SDRIP_DEVICE_CODE ||
   ui.rx_addev_no == PERSEUS_DEVICE_CODE ||
   ui.rx_addev_no == EXCALIBUR_DEVICE_CODE ||
   ui.rx_addev_no == RTL2832_DEVICE_CODE ||
   ui.rx_addev_no == MIRISDR_DEVICE_CODE ||
   ui.rx_addev_no == BLADERF_DEVICE_CODE ||
   ui.rx_addev_no == PCIE9842_DEVICE_CODE ||
   ui.rx_addev_no == OPENHPSDR_DEVICE_CODE ||
   ui.rx_addev_no == FDMS1_DEVICE_CODE ||
   ui.rx_addev_no == NETAFEDRI_DEVICE_CODE)
  {
  fg.passband_direction=1;
  sdr_nco_counter++;
  goto setfq_x;
  }
if(ui.rx_addev_no == AIRSPY_DEVICE_CODE)
  {
  fg.passband_direction=1;
  if(airspy.real_mode==1 && FFT1_CURMODE == 2)fg.passband_direction=-1;
  sdr_nco_counter++;
  goto setfq_x;
  }
if(ui.rx_addev_no == AIRSPYHF_DEVICE_CODE)
  {
  fg.passband_direction=1;
  sdr_nco_counter++;
  goto setfq_x;
  }
fg.passband_direction=1;
switch (ui.rx_soundcard_radio)
  {
  case RX_SOUNDCARD_RADIO_WSE:
  wse_rx_freq_control_ser();
  break;

  case RX_SOUNDCARD_RADIO_UNDEFINED:
  break;
  
  case RX_SOUNDCARD_RADIO_UNDEFINED_REVERSED:
  fg.passband_direction=-1;
  break;

  case RX_SOUNDCARD_RADIO_SI570:
  fg.passband_direction=si570.rx_passband_direction;
  si570_rx_freq_control();
  break;

  case RX_SOUNDCARD_RADIO_SOFT66:
  soft66_rx_freq_control();
  break;

  case RX_SOUNDCARD_RADIO_ELEKTOR:
  elektor_rx_freq_control();
  break;

  case RX_SOUNDCARD_RADIO_FCDPROPLUS:
  fcdproplus_rx_freq_control();
  break;

  case RX_SOUNDCARD_RADIO_AFEDRI_USB:
  afedriusb_rx_freq_control();
  break;
  }
setfq_x:;
if( (ui.converter_mode & CONVERTER_USE) != 0)
  {
  fg.passband_direction*=ui_converter_direction;  
  }
fft1_direction=fg.passband_direction;
check_filtercorr_direction();
}


//=============================end routines from wse_sdrxx.c

// Define your parameters in this structure.
// Save it to a file and recover it from init_users_control_window(void)

void init_users_control_window(void)
{

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

void clear_hware_data_ser(void)
{
int i;
// Set -1 in device_data. 
// A data word is max 31 bit, the sign bit is a flag
// saying the data is not valid.
for(i=0; i<256; i++)
  {
  device_data[i]=-1;
  device_name[i]=undefmsg_ser;
  }
device_name[RX10700]=device_name_rx10700_ser;
device_name[RX70]=device_name_rx70_ser;
device_name[RX144]=device_name_rx144_ser;
device_name[RXHFA]=device_name_rxhfa_ser;
device_name[RXHFA_GAIN]=device_name_rxhfa_gain_ser;
device_name[TX10700]=device_name_tx10700_ser;
device_name[TX70]=device_name_tx70_ser;
device_name[TXHFA]=device_name_txhfa144_ser;
rx_hware_init_flag=1;
hware_no_of_updates=-1;
hware_flag=0;
hware_time=current_time();
fg_old_band=-1;
if(ui.rx_addev_no == SDR14_DEVICE_CODE ||
   ui.rx_addev_no == SDRIQ_DEVICE_CODE ||
   ui.rx_addev_no == PERSEUS_DEVICE_CODE ||
   ui.rx_addev_no == SDRIP_DEVICE_CODE )
  {
  rx_hware_init_flag=0;
  }
}

// **********************************************************
//                  open and close serial ports
// **********************************************************

void users_close_devices(void)
{
//below added by w3sz
lir_close_serport();

}

void users_open_devices(void)
{
Fp=fopen(*w3szfile,"r");
if(Fp == NULL)
  {
// Set initial values if there is no file.
  serport_number = 5;
  portno = serport_number;   
  serport_baudrate = 9600;
  serport_stopbits = 0;  // 0 is one stop bit; 1 is two stop bits
  serport_rts_mode = 0;
  }
else
  {
  fscanf(Fp,"%d %d %d %d",&serport_number,&serport_baudrate,&serport_stopbits,&serport_rts_mode);
  fclose(Fp);
  printf("Read comportfile values");
  portno = serport_number;  
  }

//below added by w3sz
// 1=COM1 or /dev/ttyS0 , 2=COM2....
// baudrate 110 to 57600
// 1=two stopbits, 0=one stopbit
serport_open_status=lir_open_serport(serport_number, 
     serport_baudrate, serport_stopbits, serport_rts_mode);      
	
serport_open_status = serport_open_status;	
	
}


void userdefined_u(void)
{
	
}

///below is routine to change serial port number; this is done for Windows only

void userdefined_q(void)
{
#if (OSNUM == OSNUM_WINDOWS)
char s[80];
char ch = ' ';
fflush(stdin);
#endif
portno = serport_number;

//change the name of the serial port
#if (OSNUM == OSNUM_WINDOWS)
sprintf(&serport_name[7],"%d",serport_number);
sprintf(s,"COM port is currently %d ",serport_number);
puts(s);
printf("Please enter desired COM port number\n"); 

scanf("%d", &portno);

sprintf(strw3sz, "Thank You. You have specified COM Port Number %d",portno);
puts(strw3sz);
printf("Do you want to set the COM Port Number to this value?\n");

scanf("%s",&ch); 
if((ch == 'y') || (ch =='Y'))
{
sprintf(s,"COM Port Number was %d",serport_number);
puts(s);
sprintf(s,"Requested COM Port Number is %d",portno);
puts(s);
serport_number = portno;
sprintf(&serport_name[7],"%d",serport_number);
sprintf(s,"COM Port Number will be %d",serport_number);
puts(s);
}
else
{
sprintf(s,"COM Port Number will remain unchanged and equal to %d",serport_number);
puts(s);	
}

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


// save COM Port Number
// to the parameterfile on disk

Fp=fopen(*w3szfile,"w");
fprintf(Fp,"%d\r\n%d\r\n%d\r\n%d\r\n",serport_number,serport_baudrate,serport_stopbits,serport_rts_mode);
printf("Wrote serial port values to comportfile\n");
fclose(Fp);

users_close_devices();
users_open_devices();

}

int read_wse_parameters_ser(void)
{
return read_sdrpar(wse_parfil_name_ser, MAX_WSE_PARM, 
                                     wse_parm_text_ser, (int*)((void*)&wse));
}

void update_users_rx_frequency(void){};



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
// Use hware_flag to direct calls to control_hware_ser() when cpu time 
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
// Use hware_flag to direct calls to control_hware_ser() when cpu time 
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
