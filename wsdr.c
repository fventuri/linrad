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
#include "thrdef.h"
#include "wdef.h"
#include "fft1def.h"
#include "sdrdef.h"
#include "options.h"
#include "screendef.h"
#include "wscreen.h"

#define FT_OPEN_BY_SERIAL_NUMBER    1
#define FT_OPEN_BY_DESCRIPTION      2

#define FT_OK 0
#define FT_INVALID_HANDLE 1
#define FT_DEVICE_NOT_FOUND 2
#define FT_DEVICE_NOT_OPENED 3
#define FT_IO_ERROR 4
#define FT_INSUFFICIENT_RESOURCES 5
#define FT_INVALID_PARAMETER 6
#define FT_INVALID_BAUD_RATE 7
#define FT_DEVICE_NOT_OPENED_FOR_ERASE 8
#define FT_DEVICE_NOT_OPENED_FOR_WRITE 9
#define FT_FAILED_TO_WRITE_DEVICE 10
#define FT_EEPROM_READ_FAILED 11
#define FT_EEPROM_WRITE_FAILED 12
#define FT_EEPROM_ERASE_FAILED 13
#define	FT_EEPROM_NOT_PRESENT 14
#define	FT_EEPROM_NOT_PROGRAMMED 15
#define	FT_INVALID_ARGS 16
#define FT_NOT_SUPPORTED 17
#define	FT_OTHER_ERROR 18


typedef PVOID	FT_HANDLE;
typedef ULONG	FT_STATUS;

typedef FT_STATUS (WINAPI *sdr14_Open_Ex)(PVOID, DWORD, FT_HANDLE *);
typedef FT_STATUS (WINAPI *sdr14_Read)(FT_HANDLE, LPVOID, DWORD, LPDWORD);
typedef FT_STATUS (WINAPI *sdr14_Write)(FT_HANDLE, LPVOID, DWORD, LPDWORD);
typedef FT_STATUS (WINAPI *sdr14_close)(FT_HANDLE);
typedef FT_STATUS (WINAPI *sdr14_SetTimeouts)(FT_HANDLE, DWORD, DWORD);
typedef FT_STATUS (WINAPI *sdr14_SetResetPipeRetryCount)(FT_HANDLE,DWORD);
typedef FT_STATUS (WINAPI *sdr14_ResetDevice)(FT_HANDLE);
typedef FT_STATUS (WINAPI *sdr14_GetQueueStatus)(FT_HANDLE, LPDWORD);



#define UINT8 unsigned char
#define INT16 short int
#define UINT16 unsigned short int
#include "g31ddcapi.h"

char *unable_to_load="Unable to load ";

sdr14_Write p_sdr14_Write;
sdr14_Read p_sdr14_Read;
sdr14_close p_sdr14_Close;
sdr14_SetTimeouts p_sdr14_SetTimeouts;
sdr14_SetResetPipeRetryCount p_sdr14_SetResetPipeRetryCount;
sdr14_ResetDevice p_sdr14_ResetDevice; 
sdr14_GetQueueStatus p_sdr14_GetQueueStatus;

FT_HANDLE sdr14_handle;

HINSTANCE h_sdr14;

HMODULE h_excalibur_api;
INT32 h_excalibur_device;
G31DDC_OPEN_DEVICE p_excalibur_open_device;
G31DDC_CLOSE_DEVICE p_excalibur_close_device;
G31DDC_GET_DEVICE_LIST p_excalibur_device_list;
G31DDC_GET_DEVICE_INFO p_excalibur_device_info;
G31DDC_GET_DDC_INFO p_excalibur_ddc_info;
G31DDC_SET_DDC1 p_excalibur_set_ddc1;
G31DDC_SET_DDC1_FREQUENCY p_excalibur_set_ddc1_frequency;
G31DDC_START_DDC1 p_excalibur_start_ddc1;
G31DDC_SET_MW_FILTER p_excalibur_set_mw_filter;
G31DDC_SET_ATTENUATOR p_excalibur_set_attenuator;
G31DDC_SET_CALLBACKS p_excalibur_set_callbacks;
G31DDC_SET_POWER p_excalibur_set_power;
G31DDC_SET_DITHERING p_excalibur_set_dithering;
G31DDC_START_IF p_excalibur_start_if;
G31DDC_STOP_IF p_excalibur_stop_if;
G31DDC_STOP_DDC1 p_excalibur_stop_ddc1;


G31DDC_DEVICE_INFO excalibur_dev_info;

void excalibur_errexit(char *s)
{
clear_screen();
lir_text(10,5,s);
SNDLOG"\n%s",s);  
lir_text(10,7,press_any_key);
await_keyboard();
}

int load_excalibur_library(char* buf)
{
char excalibur_dllname[80];
G31DDC_DEVICE_INFO *excalibur_device_list;
char s[120];
int i, j, count;
sprintf(excalibur_dllname,"%sG31DDCAPI.dll",DLLDIR);
h_excalibur_api=LoadLibraryEx(excalibur_dllname, NULL, 
                                                LOAD_WITH_ALTERED_SEARCH_PATH);
if(h_excalibur_api==NULL)
  {
  sprintf(s,"Could not load %s",excalibur_dllname);
  excalibur_errexit(s);
  return 0;
  }
if(write_log)
  {  
  sprintf(s,"%s successfully loaded.",excalibur_dllname);
  lir_text(10,3,s);
  SNDLOG"\n%s",s);
  }
p_excalibur_device_list=
         (G31DDC_GET_DEVICE_LIST)(void*)GetProcAddress(h_excalibur_api,"GetDeviceList");
if(p_excalibur_device_list == NULL)
  {
  sprintf(s,"Error in %s",excalibur_dllname);
  excalibur_errexit(s);
retzer:;  
  FreeLibrary(h_excalibur_api);
  return 0;
  }
count=p_excalibur_device_list(NULL,0);      
if(count == 0)goto retzer;
if(count >MAX_EXCALIBUR_DEVICES)count=MAX_EXCALIBUR_DEVICES;
excalibur_device_list=malloc(count*sizeof(G31DDC_DEVICE_INFO));
if(excalibur_device_list == NULL)
  {
  lirerr(631952);
  goto retzer;
  }
count=p_excalibur_device_list(excalibur_device_list,
                                  count*sizeof(G31DDC_DEVICE_INFO));
for(i=0; i<count; i++)
  {
  for(j=0; j<9; j++)
    {
    buf[9*i+j]=excalibur_device_list[i].SerialNumber[j];
    }
  }
free(excalibur_device_list);
FreeLibrary(h_excalibur_api);
return count;  
}  

void open_excalibur(void)
{
char excalibur_dllname[80];
int i;
char sernum[16];
char s[120];
FILE *excalibur_sernum_file;
excalibur_sernum_file=fopen(excalibur_sernum_filename,"r");
if(excalibur_sernum_file == NULL)
  {
  lirerr(1333);
  return;
  }
i=fscanf(excalibur_sernum_file,"%9s",sernum);
fclose(excalibur_sernum_file);
if(i==0)
  {
  lirerr(1334);
  return;
  }
sprintf(excalibur_dllname,"%sG31DDCAPI.dll",DLLDIR);
h_excalibur_api=LoadLibraryEx(excalibur_dllname, NULL, 
                                                LOAD_WITH_ALTERED_SEARCH_PATH);
if(h_excalibur_api==NULL)
  {
  sprintf(s,"%s%s",unable_to_load, excalibur_dllname);
  excalibur_errexit(s);
  return;
  }
if(write_log)
  {  
  sprintf(s,"%s successfully loaded.",excalibur_dllname);
  lir_text(10,3,s);
  SNDLOG"\n%s",s);
  }
p_excalibur_open_device=
          (G31DDC_OPEN_DEVICE)(void*)GetProcAddress(h_excalibur_api,"OpenDevice");
p_excalibur_close_device=
          (G31DDC_CLOSE_DEVICE)(void*)GetProcAddress(h_excalibur_api,"CloseDevice");
if(p_excalibur_open_device == NULL || p_excalibur_close_device == NULL )
  {
  sprintf(s,"Error in %s",excalibur_dllname);
  excalibur_errexit(s);
  return;
  }
h_excalibur_device=p_excalibur_open_device(sernum);
if(h_excalibur_device  < 0)
  {
  lirerr(1335);
apierr:;
  FreeLibrary(h_excalibur_api);
  return;
  }
p_excalibur_device_info=
    (G31DDC_GET_DEVICE_INFO)(void*)GetProcAddress(h_excalibur_api,"GetDeviceInfo"); 
if(p_excalibur_device_info == NULL)
  {
apierr2:;
  lirerr(1336);
  p_excalibur_close_device(h_excalibur_device);
  goto apierr;
  }
p_excalibur_set_ddc1=
          (G31DDC_SET_DDC1)(void*)GetProcAddress(h_excalibur_api,"SetDDC1"); 
if(p_excalibur_set_ddc1 == NULL)goto apierr2;
p_excalibur_set_ddc1_frequency=(G31DDC_SET_DDC1_FREQUENCY)
                         (void*)GetProcAddress(h_excalibur_api,"SetDDC1Frequency"); 
if(p_excalibur_set_ddc1_frequency == NULL)goto apierr2;
p_excalibur_start_ddc1=
          (G31DDC_START_DDC1)(void*)GetProcAddress(h_excalibur_api,"StartDDC1");
if(p_excalibur_start_ddc1 == NULL)goto apierr2;
p_excalibur_set_mw_filter=
         (G31DDC_SET_MW_FILTER)(void*)GetProcAddress(h_excalibur_api,"SetMWFilter");
if(p_excalibur_set_mw_filter == NULL)goto apierr2;
p_excalibur_set_attenuator=
       (G31DDC_SET_ATTENUATOR)(void*)GetProcAddress(h_excalibur_api,"SetAttenuator");
if(p_excalibur_set_attenuator == NULL)goto apierr2;
p_excalibur_set_callbacks=
        (G31DDC_SET_CALLBACKS)(void*)GetProcAddress(h_excalibur_api,"SetCallbacks");
if(p_excalibur_set_callbacks == NULL)goto apierr2;
p_excalibur_set_power=
            (G31DDC_SET_POWER)(void*)GetProcAddress(h_excalibur_api,"SetPower");
if(p_excalibur_set_power == NULL)goto apierr2;
p_excalibur_set_dithering=
         (G31DDC_SET_DITHERING)(void*)GetProcAddress(h_excalibur_api,"SetDithering");
p_excalibur_start_if=
                 (G31DDC_START_IF)(void*)GetProcAddress(h_excalibur_api,"StartIF");

p_excalibur_stop_if=(G31DDC_STOP_IF)(void*)GetProcAddress(h_excalibur_api,"StopIF");
if(p_excalibur_stop_if == NULL)goto apierr2;
p_excalibur_stop_ddc1=
                (G31DDC_STOP_DDC1)(void*)GetProcAddress(h_excalibur_api,"StopDDC1");
if(p_excalibur_stop_ddc1 == NULL)goto apierr2;
p_excalibur_set_power(h_excalibur_device, TRUE); 
p_excalibur_device_info(h_excalibur_device, &excalibur_dev_info,
                                                 sizeof(G31DDC_DEVICE_INFO));
p_excalibur_ddc_info=
        (G31DDC_GET_DDC_INFO)(void*)GetProcAddress(h_excalibur_api,"GetDDCInfo"); 
if(p_excalibur_ddc_info == NULL)goto apierr2;
no_of_excalibur_rates=excalibur_dev_info.DDCTypeCount;
sdr=0;
}

void excalibur_stop(void)
{
p_excalibur_stop_ddc1(h_excalibur_device);
p_excalibur_stop_if(h_excalibur_device);
}

void close_excalibur(void)
{
p_excalibur_set_callbacks(h_excalibur_device, NULL,0);
p_excalibur_set_power(h_excalibur_device, FALSE);
p_excalibur_close_device(h_excalibur_device);
FreeLibrary(h_excalibur_api);
sdr=-1;
}



void WINAPI excalibur_if_callback(
                   CONST SHORT *Buffer,
                   UINT32 NumberOfSamples,
                   WORD MaxADCAmplitude,
                   UINT32 ADCSamplingRate,
                   DWORD_PTR UserData)
{
(void)MaxADCAmplitude;
(void)Buffer;
(void)NumberOfSamples;
(void)ADCSamplingRate;
(void)UserData;
//if(MaxADCAmplitude > ad_maxamp[0])
//  {
//  ad_maxamp[0]=MaxADCAmplitude;
//  }
}


void WINAPI excalibur_ddc1_callback(
                   CONST VOID *Buffer,
                   UINT32 NumberOfSamples,
                   UINT32 BitsPerSample,
                   DWORD_PTR UserData)
{
// This callback gets called whenever a buffer is ready
// from the USB interface
(void)UserData;
int *ibuf, *obuf;
short int *sh_ibuf, *sh_obuf;
unsigned int i;
if(BitsPerSample == 32)
  {
  ibuf=(int*)Buffer;
  for(i=0; i<2*NumberOfSamples; i+=2)
    {
    obuf=(int*)&timf1_char[timf1p_sdr];
    obuf[0]=ibuf[i  ];
    obuf[1]=ibuf[i+1];
    timf1p_sdr=(timf1p_sdr+8)&timf1_bytemask;
    }  
  }
else
  {
  sh_ibuf=(short int*)Buffer;
  for(i=0; i<2*NumberOfSamples; i+=2)
    {
    sh_obuf=(short int*)&timf1_char[timf1p_sdr];
    sh_obuf[0]=sh_ibuf[i  ];
    sh_obuf[1]=sh_ibuf[i+1];
    timf1p_sdr=(timf1p_sdr+4)&timf1_bytemask;
    }  
  }  
if(((timf1p_sdr-timf1p_pa+timf1_bytes)&timf1_bytemask) >=
              (int)snd[RXAD].block_bytes)
  {
  SetEvent(rxin1_bufready);
  }
}

void start_excalibur_read(void)
{
int err;
G31DDC_CALLBACKS callbacks;
callbacks.IFCallback=(G31DDC_IF_CALLBACK)excalibur_if_callback;
callbacks.DDC1StreamCallback=
                          (G31DDC_DDC1_STREAM_CALLBACK)excalibur_ddc1_callback;
p_excalibur_set_ddc1(h_excalibur_device,excalib.rate_no);
p_excalibur_set_callbacks(h_excalibur_device, &callbacks,0);
rxin1_bufready=CreateEvent( NULL, FALSE, FALSE, NULL);
if(rxin1_bufready== NULL)
  {
  close_excalibur();
  lirerr(1223);
  return;
  }
p_excalibur_set_dithering(h_excalibur_device, excalib.dither);
timf1p_sdr=timf1p_pa;
lir_sched_yield();
err=p_excalibur_start_ddc1(h_excalibur_device, snd[RXAD].block_frames);
if(err == 0)
  {
  lirerr(61125);
  return;
  }
err=p_excalibur_start_if(h_excalibur_device,500);
if(err == 0)lirerr(61126);
}

void select_excalibur_mode(int *line)
{
unsigned int i, j, k;
G3XDDC_DDC_INFO info;
char s[80];
p_excalibur_ddc_info=
        (G31DDC_GET_DDC_INFO)(void*)GetProcAddress(h_excalibur_api,"GetDDCInfo"); 
if(p_excalibur_ddc_info == NULL)
  {
  lirerr(1336);
  return;
  }
lir_text(0,line[0],"No  Sampl.Rate   Bandwidth    Bits");
line[0]++;
k=no_of_excalibur_rates-1;
for(i=0; i<=k; i++)
  {
  j=p_excalibur_ddc_info(h_excalibur_device, i, &info);
  if(j != 0 && (ui.operator_skil != OPERATOR_SKIL_NEWCOMER ||
                                              info.SampleRate <=500000))
    {
    sprintf(s,"%2d    %7d     %7d      %2d",i,info.SampleRate,
                      info.Bandwidth, info.BitsPerSample);
    lir_text(0,line[0],s);
    line[0]++;
    }
  else
    {
    k=i-1;
    }  
  }
line[0]+=2;
lir_text(0,line[0],"Select sampling rate (DDC mode) by line number");
excalib.rate_no=lir_get_integer(47, line[0], 2, 0, k); 
if(kill_all_flag)return;
p_excalibur_ddc_info(h_excalibur_device, excalib.rate_no, &info);
excalib.sampling_rate=info.SampleRate;
ui.rx_addev_no=EXCALIBUR_DEVICE_CODE;
ui.rx_input_mode=IQ_DATA+DIGITAL_IQ;
if(info.BitsPerSample == 32)ui.rx_input_mode+=DWORD_INPUT;
ui.rx_rf_channels=1;
ui.rx_ad_channels=2;
ui.rx_admode=0;

line[0]+=2;
}

void set_excalibur_frequency(void)
{
double dt1;
UINT32 fq_ctl;
dt1=fg.passband_center*1000000+0.5;
if(dt1 < 0 || dt1 > adjusted_sdr_clock/2)
  {
  fg.passband_center=7.05;
  dt1=fg.passband_center*1000000;
  }
fq_ctl=round(dt1);
fg_truncation_error=fq_ctl-dt1;
p_excalibur_set_ddc1_frequency(h_excalibur_device, fq_ctl);
if(1000000.*fg.passband_center-ui.rx_ad_speed/2 < excalib.mw_filter_maxfreq)
  {
  p_excalibur_set_mw_filter(h_excalibur_device, FALSE);
  }
else
  {
  p_excalibur_set_mw_filter(h_excalibur_device, TRUE);
  }
}

void set_excalibur_att(void)
{
int i;
fg.gain/=3;
fg.gain*=3;
i=-fg.gain;
p_excalibur_set_attenuator(h_excalibur_device, i);
}

void lir_excalibur_read(void)
{
lir_sched_yield();
while(((timf1p_sdr-timf1p_pa+timf1_bytes)&timf1_bytemask) < (int)snd[RXAD].block_bytes)
  {
  WaitForSingleObject(rxin1_bufready, INFINITE);
  }
}

//   ***********************  SDR-14 and SDR-IQ *************************

void lir_sdr14_write(char *s, int bytes)
{
DWORD written;
int totwr, cnt;
FT_STATUS err;
totwr=0;
cnt=0;
do_write:;
err=p_sdr14_Write(sdr14_handle,&s[totwr],bytes-totwr, &written);
if(err != FT_OK)
  {
  return;
  }
totwr+=written;
if(totwr == bytes)
  {
  return;
  }
lir_sched_yield();
cnt++;
if(cnt < 25)goto do_write;
}

int lir_sdr14_read(char *s,int bytes)
{
DWORD i;
int cnt, totread;
FT_STATUS err;
cnt=0;
wait:;
err=p_sdr14_GetQueueStatus(sdr14_handle, &i);
if( (int)i < bytes)
  {
  lir_sleep(5000);
  cnt++;
  if(cnt < 50)goto wait;
  }
totread=0;
cnt=0;
do_read:;
err=p_sdr14_Read(sdr14_handle, &s[totread], bytes-totread, &i);
if(kill_all_flag || err != FT_OK)
  {
  return -1;
  }
totread+=i;
if(totread == bytes)return bytes;
lir_sched_yield();
cnt++;
if(cnt<25)goto do_read;  
return -1;
}

void close_sdr14(void)
{
p_sdr14_Close(sdr14_handle);
sdr=-1;
FreeLibrary(h_sdr14);
}

void open_sdr14(void)
{
char s[120];
sdr14_Open_Ex p_sdr14_OpenEx;
char sdr14_dllname[80];
sprintf(sdr14_dllname,"%sftd2xx.dll",DLLDIR);
h_sdr14=LoadLibraryEx(sdr14_dllname, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
if (h_sdr14 == NULL)
  {
  sprintf(s,"Failed to load %s",sdr14_dllname);
  goto open_sdr14_fail2;
  }
SNDLOG"File %s loaded.\n",sdr14_dllname);
p_sdr14_OpenEx=(sdr14_Open_Ex)(void*)GetProcAddress(h_sdr14, "FT_OpenEx");
if(p_sdr14_OpenEx == NULL)
  {
  sprintf(s,"Could not find FT_OpenEx");
  goto open_sdr14_fail;
  }
if(p_sdr14_OpenEx(sdr14_name_string,
                         FT_OPEN_BY_DESCRIPTION, &sdr14_handle) != FT_OK)
  {
  if(p_sdr14_OpenEx(sdriq_name_string,
                          FT_OPEN_BY_DESCRIPTION, &sdr14_handle) != FT_OK)
    {
    sprintf(s,"Could not open any of %s or %s",sdr14_name_string, sdriq_name_string);
    goto open_sdr14_fail;
    }
  else
    {
    SNDLOG"%s open OK\n",sdriq_name_string);
    }
  }
else
  {
  SNDLOG"%s open OK\n",sdr14_name_string);
  }
p_sdr14_Read=(sdr14_Read)(void*)GetProcAddress(h_sdr14, "FT_Read");
p_sdr14_Write=(sdr14_Write)(void*)GetProcAddress(h_sdr14, "FT_Write");
p_sdr14_Close=(sdr14_close)(void*)GetProcAddress(h_sdr14, "FT_Close");
p_sdr14_SetTimeouts=(sdr14_SetTimeouts)(void*)GetProcAddress(h_sdr14, "FT_SetTimeouts");
p_sdr14_SetResetPipeRetryCount=(sdr14_SetResetPipeRetryCount)
                 (void*)GetProcAddress(h_sdr14, "FT_SetResetPipeRetryCount");
p_sdr14_ResetDevice=(sdr14_ResetDevice)
                 (void*)GetProcAddress(h_sdr14, "FT_ResetDevice");
p_sdr14_GetQueueStatus=(sdr14_GetQueueStatus)
                 (void*)GetProcAddress(h_sdr14, "FT_GetQueueStatus");


if(p_sdr14_Read == NULL ||
   p_sdr14_Write == NULL ||
   p_sdr14_Close == NULL ||
   p_sdr14_SetTimeouts == NULL ||
   p_sdr14_SetResetPipeRetryCount == NULL ||
   p_sdr14_ResetDevice == NULL ||
   p_sdr14_GetQueueStatus == NULL )goto open_sdr14_fail;
if(p_sdr14_ResetDevice(sdr14_handle) != FT_OK)goto open_sdr14_fail;
if(p_sdr14_SetResetPipeRetryCount(sdr14_handle,100) != FT_OK)
                                                       goto open_sdr14_fail;
if(p_sdr14_SetTimeouts(sdr14_handle,500,500) != FT_OK)
                                                       goto open_sdr14_fail;
sdr=1;
SNDLOG"open_sdr14() sucessful.\n");
return;
open_sdr14_fail:;
FreeLibrary(h_sdr14);
open_sdr14_fail2:;
sdr=-1;
SNDLOG"%s\n",s);
SNDLOG"open_sdr14() returned an error.\n");
clear_screen();
lir_text(10,10,s);
lir_text(12,11,press_any_key);
await_processed_keyboard();
}

