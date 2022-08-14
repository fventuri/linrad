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
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "globdef.h"
#include "uidef.h"
#include "sdrdef.h"

#define MAX_SDR14_DRIVER_NAMES 4
char *sdr14_driver_names[MAX_SDR14_DRIVER_NAMES]=
                            { "/dev/linrad_ft245",
                              "/dev/ft245?",
                              "/dev/ttyUSB?",
                              "/dev/usb/ttyUSB?"};


void start_excalibur_read(void){}
void lir_excalibur_read(void){}
void set_excalibur_frequency(void){}
void excalibur_stop(void){}

#define MAX_SDR14_DEVICES 6
int sdr14_scan_usb(void);
typedef struct {
int sernum1;
int sernum2;
int sernum3;
int device_code;
}SDR14_DEVICEINFO;

SDR14_DEVICEINFO sdr14_device[MAX_SDR14_DEVICES]; 
int detected_device_no;


void sdr14_scan_result(int *line, char* dev, int verify)
{
char s[256], ss[80], sn[80];
int *isn;
int len;
int j, code;
char csn[12];
isn = (int*)(void*)&csn[0];
sdr_target_name(sn);
code=UNDEFINED_DEVICE_CODE;
if( strcmp(sdr14_name_string,sn) == 0)
  {
  code=SDR14_DEVICE_CODE;
  } 
if( strcmp(sdriq_name_string,sn) == 0)
  {
  code=SDRIQ_DEVICE_CODE;
  } 
if(code != UNDEFINED_DEVICE_CODE)
  {
  sdr_target_serial_number(ss);
  len=(int)strlen(ss)-1;
  j=11;
  while(len >= 0 && j >= 0)
    {
    csn[j]=ss[len];
    j--;
    len--;
    }
  while(j >= 0)
    {
    csn[j]=0;
    j--;
    }
  if(verify)
    {
    if(  sdr14.sernum1 == isn[0] &&
         sdr14.sernum2 == isn[1] &&
         sdr14.sernum3 == isn[2] &&
         sdr14.device_code == code)
      {
      return;
      }   
    }
  else
    {
    sdr14_device[line[0]].sernum1=isn[0];
    sdr14_device[line[0]].sernum2=isn[1];
    sdr14_device[line[0]].sernum3=isn[2];
    sdr14_device[line[0]].device_code=code;
    sprintf(s,"Line:%d:  Name:%s    Serial no:%s    Device %s",line[0],sn,ss,dev);
    SNDLOG"\n%s",s);
    lir_text(5,2+line[0],s);
    line[0]++;
    if(line[0] >= MAX_SDR14_DEVICES)
      {
      close_sdr14();
      lirerr(1125);
      return;
      }
    }
  }
close_sdr14();
}

int sdr14_scan_usb(void)
{
char s[80];
int i, j, selected_device, line;
size_t len;
selected_device=-1;
line=0;
for(i=0; i<MAX_SDR14_DRIVER_NAMES; i++)
  {
  len=strlen(sdr14_driver_names[i]);
  if(sdr14_driver_names[i][len-1] != '?')
    {
    SNDLOG"\nTrying to open %s",sdr14_driver_names[i]);
    sdr=open( sdr14_driver_names[i] ,O_RDWR );
    if(sdr != -1)
      {
      sdr14_scan_result(&line, sdr14_driver_names[i], FALSE);
      if(kill_all_flag)return -1;
      close_sdr14();
      }
    }
  else
    {
    for(j=0; j<8; j++)
      {
      strcpy(s,sdr14_driver_names[i]);
      s[len-1]='0'+j;
      SNDLOG"\nTrying to open %s",s);
      if(write_log)fflush(sndlog);
      sdr=open(s ,O_RDWR );
      if(sdr != -1)
        {
        sdr14_scan_result(&line, s, FALSE);
        if(kill_all_flag)return -1;
        close_sdr14();
        }
      }
    }  
  }  
clear_lines(10,12);
if(line > 0)
  {
  lir_text(0,line+4,"Select device by line number");
  selected_device=lir_get_integer(29,line+4,1,0,line-1);
  sdr14.sernum1=sdr14_device[selected_device].sernum1;
  sdr14.sernum2=sdr14_device[selected_device].sernum2;
  sdr14.sernum3=sdr14_device[selected_device].sernum3;
  sdr14.device_code=sdr14_device[selected_device].device_code;
  ui.rx_addev_no=sdr14_device[selected_device].device_code;
  }
return selected_device;
}

void open_sdr14(void)
{
char s[80];
int i, j, line;
size_t len;
line=0;
for(i=0; i<MAX_SDR14_DRIVER_NAMES; i++)
  {
  len=strlen(sdr14_driver_names[i]);
  if(sdr14_driver_names[i][len-1] != '?')
    {
    SNDLOG"\nTrying to open %s",sdr14_driver_names[i]);
    if(write_log)fflush(sndlog);
    sdr=open( sdr14_driver_names[i] ,O_RDWR );
    if(sdr != -1)
      {
      sdr14_scan_result(&line, sdr14_driver_names[i], TRUE);
      if(kill_all_flag)return;
      }
    if(sdr != -1)return;  
    }
  else
    {
    for(j=0; j<8; j++)
      {
      strcpy(s,sdr14_driver_names[i]);
      s[len-1]='0'+j;
      SNDLOG"\nTrying to open %s",s);
      if(write_log)fflush(sndlog);
      sdr=open(s ,O_RDWR );
      if(sdr != -1)
        {
        sdr14_scan_result(&line, s, TRUE);
        if(kill_all_flag)return;
        }
      if(sdr != -1)return;  
      }
    }  
  }  
}




int load_excalibur_library(char* buf)
{
(void)buf;
return 0;
}


void open_excalibur(void)
{
}

void close_excalibur(void)
{
}

void select_excalibur_mode(int *line)
{
(void)line;
}

void set_excalibur_att(void)
{
/*
int i;
i=-fg.gain;
//ig31_device->lpVtbl->SetAttenuator(ig31_device, i);
*/
}

void lir_sdr14_write(char *s, int bytes)
{
int i, totwr;
totwr=0;
do_write:;
i=write(sdr,&s[totwr],bytes-totwr);
if(i < 0)return;
totwr+=i;
if(totwr == bytes)return;
lir_sched_yield();
goto do_write;
}

int lir_sdr14_read(char *s,int bytes)
{
int i, totread;
totread=0;
do_read:;
i=read(sdr, &s[totread], bytes-totread);
if(i < 0)return i;
totread+=i;
if(totread == bytes)return bytes;
lir_sched_yield();
goto do_read;
}

void close_sdr14(void)
{
int i;
i=fcntl(sdr, F_GETFD);
i|=O_NONBLOCK;
fcntl(sdr,F_SETFD,i);
lir_sleep(10000);
close(sdr);
sdr=-1;
}

