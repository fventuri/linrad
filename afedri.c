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
#if(OSNUM == OSNUM_WINDOWS)
#define __USE_MINGW_ANSI_STDIO 1
#endif
#include "globdef.h"
#include <string.h>
#if(OSNUM == OSNUM_WINDOWS)
  #include <windows.h>
  #include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  #include "lscreen.h"
#endif

#include "uidef.h"
#include "sdrdef.h"
#include "vernr.h"
#include "screendef.h"
#include "hidapi.h"
#include "options.h"

#include <fcntl.h>
#include <string.h>

#if(OSNUM == OSNUM_WINDOWS)
#include <windows.h>
#include <winsock.h>
#define RECV_FLAG 0
#define INVSOCK INVALID_SOCKET
#define CLOSE_FD closesocket
#ifndef _MINGW_IP_MREQ1_H
typedef struct{
struct in_addr imr_multiaddr;   /* IP multicast address of group */
struct in_addr imr_interface;   /* local IP address of interface */
}IP_MREQ;
#endif
extern WSADATA wsadata;

#endif

#if(OSNUM == OSNUM_LINUX)
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#ifdef MSG_WAITALL
#define RECV_FLAG MSG_WAITALL
#else
#define RECV_FLAG 0
#endif
#define IP_MREQ struct ip_mreq
#define INVSOCK -1
#define CLOSE_FD close
#endif

#include "thrdef.h"
#include "fft1def.h"


#define DIVERSITY_MODE 1
#define DUAL_CHANNEL_MODE 2
#define DUAL_CHANNEL_MODE_OFF 0
#define DIVERSITY_INTERNAL_ADD_MODE 3
#define MSD_CLUSTER_SIZE 512
#define HID_COMMAND_MAX_SIZE 7
#define AFEDRI8201_MAIN_CLOCK 66.00e6
#define TRANSVERTER_MODE_LIMIT  1200000000
#define NCO_MULTIPLIER 4294967296.00
#define HID_MEMORY_READ_WRITE_REPORT 1
#define HID_FREQUENCY_REPORT 3
#define HID_GENERIC_REPORT 2
#define HID_GENERIC_GAIN_COMMAND 2
#define HID_GENERIC_INIT_FE_COMMAND 0xE1
#define HID_GENERIC_GET_FREQ_COMMAND 1
#define HID_GENERIC_GET_INIT_STATUS_COMMAND 4
#define HID_GENERIC_DAC_COMMAND 8
#define HID_GENERIC_VER_COMMAND 9
#define HID_GENERIC_CONSOLE_IO_COMMAND 10
#define HID_GENERIC_HID_CONSOLE_ON 11
#define HID_READ_EEPROM_COMMAND 0x55
#define HID_WRITE_EEPROM_COMMAND 0x56
#define HID_GENERIC_SAVE_DEFAULT_PARAMS_COMMAND 12
#define HID_GENERIC_GET_SR_COMMAND 14
#define HID_GENERIC_GET_DIVERSITY_MODE 15
#define HID_GENERIC_GET_SDR_IP_ADDRESS 16
#define HID_GENERIC_GET_SDR_IP_MASK 17
#define HID_GENERIC_GET_GATEWAY_IP_ADDRESS 18
#define HID_GENERIC_GET_DST_IP_ADDRESS 19
#define HID_GENERIC_GET_DST_PORT 20
#define HID_GENERIC_SAVE_SDR_IP_ADDRESS 21
#define HID_GENERIC_SAVE_SDR_IP_MASK 22
#define HID_GENERIC_SAVE_GATEWAY_IP_ADDRESS 23
#define HID_GENERIC_SAVE_DST_IP_ADDRESS 24
#define HID_GENERIC_SAVE_DST_PORT 25
#define HID_GENERIC_START_UDP_STREAM_COMMAND 26
#define HID_GENERIC_OVERLAOD_STATE 27
#define HID_GENERIC_ACK 28
#define HID_GENERIC_GET_SERIAL_NUMBER_COMMAND 29
#define HID_GENERIC_SET_SAMPLE_RATE_COMMAND 30
#define HID_GENERIC_SET_VCO_COMMAND 31
#define HID_GENERIC_GET_VCO_COMMAND 32
#define HID_GENERIC_SAVE_GAIN_TABLE_COMMAND 33
#define HID_GENERIC_GET_GAIN_TABLE_COMMAND 34
#define HID_GENERIC_SET_BPF_COMMAND 35
#define HID_GENERIC_SYNC_WORD_COMMAND 36
#define HID_GENERIC_HW_CPLD_SN_COMMAND 37
#define HID_GENERIC_IAP_COMMAND 38
#define HID_GENERIC_WRITE_HWORD_TO_FLASH 39
#define HID_GENERIC_BROADCAST_COMMAND 40
#define HID_GENERIC_GET_BROADCAST_STATE_COMMAND 41
#define HID_GENERIC_BYPASS_LPF_COMMAND 42
#define HID_GENERIC_GET_BYPASS_LPF_COMMAND 43
#define HID_GENERIC_SET_MULTICHANNEL_COMMAND 48
#define HID_GENERIC_MULTICAST_COMMAND 49
#define HID_GENERIC_SET_AFEDRI_ID_COMMAND 50
#define HID_GENERIC_GET_AFEDRI_ID_COMMAND 51
#define HID_GENERIC_SAVE_AFEDRI_ID_COMMAND 52
#define HID_GENERIC_SOFT_RESET_COMMAND 53
#define HID_GENERIC_GET_DHCP_STATE_COMMAND 54
#define HID_GENERIC_SET_DHCP_STATE_COMMAND 55
#define HID_GENERIC_SET_PHASE_SHIFT_COMMAND 56
#define HID_GENERIC_HW_CPLD2_SN_COMMAND 57
#define HID_GENERIC_I2C_SEND_COMMAND 58
#define HID_GENERIC_I2C_READ_COMMAND 59
#define HID_GENERIC_I2C_START_COND_COMMAND 60
#define HID_GENERIC_I2C_STOP_COND_COMMAND 61
#define HID_GENERIC_MULT_WRITE_JTAG_COMMAND 62
#define HID_GENERIC_GET_MAXIMUM_SR_COMMAND 63
#define HID_GENERIC_GET_MAIN_CLOCK_FREQ 67
#define HID_GENERIC_SET_USB_MODE        68
#define TCP_HID_PACKET 0x7
#define HID_DEVICE_VID 0x483
#define HID_DEVICE_PID 0x5932
#define HID_DEVICE_ATEECS_VID 0x255D
#define HID_DEVICE_ATEECS_PID 0x0001
#define HID_DEVICE_SDR_NET_PID 0x0002
#define HID_DEVICE_ATEECS_PID_6 0x0006

#define VADDRESS_MAIN_CLOCK_FREQ_LOW_HALFWORD   0x0000
#define VADDRESS_MAIN_CLOCK_FREQ_HIGH_HALFWORD  0x0001

#define HIDCMD_USB_SIZE 5
#define HIDCMD_NET_SIZE 7

void net_write(FD fd, void *buf, int size);
int hidcmd_size;

// Structure for Afedri USB parameters
typedef struct {
int vid;
int pid;
int sernum1;
int sernum2;
int sernum3;
int sernum4;
int freq_adjust;
int vga_type;
int check;
}P_AFEDRIUSB;
#define MAX_AFEDRIUSB_PARM 9

// Structure for Afedri NET parameters
typedef struct {
int ip0;
int ip1;
int ip2;
int ip3;
int port;
int sampling_rate;
int format;
int udp_size;
int clock_adjust;
int channels;
int vga_type;
int multistream;
int check;
}P_NETAFEDRI;
#define MAX_NETAFEDRI_PARM 13

P_NETAFEDRI netafedri;

char *netafedri_parfil_name="par_netafedri";
char *netafedri_parm_text[MAX_NETAFEDRI_PARM]={"IP1",
                                       "IP2",
                                       "IP3",
                                       "IP4",
                                       "Port",
                                       "Sampling rate",
                                       "Format",
                                       "UDP size",
                                       "Clock adjust",
                                       "Channels",
                                       "VGA type",
                                       "Multistream",
                                       "Check"};
             
// These are defined in sdrip.c
extern char *pkgsize[2];
extern char *onoff[2];
char *vga_types[]={"AD8330","AD8369"};
#define MAX_VGA_TYPES 2
FD netafedri_fd;
struct sockaddr_in netafedri_addr;

char *afedriusb_parm_text[MAX_AFEDRIUSB_PARM]={"VID",
                                               "PID",
                                               "Serno1",
                                               "Serno2",
                                               "Serno3",
                                               "Serno4",
                                               "Freq adjust",
                                               "VGA type",
                                               "Check"};
P_AFEDRIUSB afedriusb;
char *afedriusb_parfil_name="par_afedriusb";



#define NON_BLOCKING_HID_IO
#define MAX_AFEDRI_TYPES 4

int vids[MAX_AFEDRI_TYPES]={HID_DEVICE_ATEECS_VID,
                            HID_DEVICE_ATEECS_VID,
                            HID_DEVICE_VID,
                            HID_DEVICE_ATEECS_VID};

int pids[MAX_AFEDRI_TYPES]={HID_DEVICE_SDR_NET_PID,
                            HID_DEVICE_ATEECS_PID_6,
                            HID_DEVICE_PID,
                            HID_DEVICE_ATEECS_PID};
hid_device *fe_handle=NULL;

int cmd_sizes[MAX_AFEDRI_TYPES]={7,7,5,5};
#define MAX_UNITS 10

void xafedriusb_command(unsigned char* report)
{
hid_write(fe_handle, (const unsigned char *)report, hidcmd_size);
hid_read_timeout(fe_handle, (unsigned char *)report, hidcmd_size,100);
}

void afedriusb_command(unsigned char* report)
{
#if(OSNUM == OSNUM_WINDOWS)
int i;
i=0;
retry:;
i++;
if(hid_write(fe_handle, (const unsigned char *)report, hidcmd_size) == -1)
  {
  if(hidcmd_size==HIDCMD_NET_SIZE)
    {
    hidcmd_size=HIDCMD_USB_SIZE;
    }
  else
    {
    hidcmd_size=HIDCMD_NET_SIZE;
    }
  if(i < 4)goto retry;
  }  
#else
hid_write(fe_handle, (const unsigned char *)report, hidcmd_size);
#endif
hid_read_timeout(fe_handle, report, hidcmd_size,100);
}

void afedriusb_rx_amp_control(void)
{
int rf_gain;
int fe_gain;
int i;
unsigned char report[HID_COMMAND_MAX_SIZE];
#if (OSNUM == OSNUM_LINUX) && defined(__linux__)
load_udev_library();
if(!udev_library_flag)
  {
  lirerr(999999);
  return;
  }
#endif
switch (afedriusb.vga_type)
  {
  case 0:
  fg.gain_increment = 1;
// Limit gain to -10 to +41dB.
  if(fg.gain < -10)fg.gain = -10;
  if(fg.gain > 41)fg.gain = 41;
  if(fg.gain >= 35)
    {
    rf_gain=35;
    fe_gain=fg.gain-35;
    }
  else
    {
    rf_gain=fg.gain;
    fe_gain=0;
    }  
  report[0]=HID_GENERIC_REPORT;
  report[1]=HID_GENERIC_DAC_COMMAND & 0xFF;
  report[2]= ((rf_gain + 10) * 8) / 3;
  report[3]= 0;
  for(i=4; i < HID_COMMAND_MAX_SIZE;  i++) report[i]=0;
  afedriusb_command(report);
  report[0] = HID_GENERIC_REPORT;
  report[1]=  HID_GENERIC_GAIN_COMMAND & 0xFF;
  report[2]= fe_gain+1;
  for(i=3; i < HID_COMMAND_MAX_SIZE;  i++) report[i]=0;
  afedriusb_command(report);
  break;

  case 1:
  fg.gain_increment = 3;
// Limit gain to -10 to +41dB.
  if(fg.gain < -10)fg.gain = -10;
  if(fg.gain > 47)fg.gain = 47;
  fg.gain+=10;
  fg.gain/=3;
  fg.gain*=3;
  fg.gain-=10;
  fe_gain=0;
  if(fg.gain >= 35)
    {
    rf_gain=35;
    switch (fg.gain)
      {
      case 38:
      fe_gain=2;
      break;

      case 41:
      fe_gain=3;
      break;

      case 44:
      fe_gain=5;
      break;

      case 47:
      fe_gain=6;
      break;
      }
    }
  else
    {
    rf_gain=fg.gain;
    fe_gain=0;
    }
  report[0]=HID_GENERIC_REPORT;
  report[1]=HID_GENERIC_DAC_COMMAND & 0xFF;
  report[2]= ((rf_gain + 10) * 8) / 3;
  report[3]= 0;
  for(i=4; i < HID_COMMAND_MAX_SIZE;  i++) report[i]=0;
  afedriusb_command(report);
  report[0] = HID_GENERIC_REPORT;
  report[1]=  HID_GENERIC_GAIN_COMMAND & 0xFF;
  report[2]= fe_gain+1;
  for(i=3; i < HID_COMMAND_MAX_SIZE;  i++) report[i]=0;
  afedriusb_command(report);
  break;
  }  
}

void afedriusb_rx_freq_control(void)
{
uint64_t frequency;
int i;
unsigned char report[HID_COMMAND_MAX_SIZE];
if(fe_handle == NULL)return;
frequency=rint((fg.passband_center*(100000000-afedriusb.freq_adjust))/100);
fg_truncation_error=frequency-
                (fg.passband_center*(100000000-afedriusb.freq_adjust))/100;
report[0] = HID_FREQUENCY_REPORT;
for(i=0; i < 4;  i++)
  {
  report[i+1]= (frequency >> (i*8)) & 0xFF;
  }
for(i=5; i < HID_COMMAND_MAX_SIZE;  i++) report[i]=0;
afedriusb_command(report);
}

void clear_hid(void)
{
int i, j;
unsigned char report[HID_COMMAND_MAX_SIZE];
j=0;
for(i=0; i<64; i++)
  {
  if(hid_read_timeout(fe_handle, report, hidcmd_size,10)==0)j++;
  if(j>3)return;
  }
}

void close_afedriusb_control(void)
{
if(fe_handle == NULL)return;
clear_hid();
hid_close(fe_handle);
}

void open_afedriusb_control(void)
{
int i;
struct hid_device_info *devs, *cur_dev;
char s[80];
#if DISABLE_AFEDRI_SERIAL_NUMBER == FALSE
int *ise;
ise=(int*)s;   
#endif
#if OSNUM == OSNUM_LINUX && defined(__linux__)
load_udev_library();
if(!udev_library_flag)
  {
  lirerr(999999);
  return;
  }
#endif
i=read_sdrpar(afedriusb_parfil_name, MAX_AFEDRIUSB_PARM, 
                             afedriusb_parm_text, (int*)((void*)&afedriusb));
if(i != 0 || afedriusb.check != AFEDRIUSBPAR_VERNR)
  {
  lirerr(1389);
  goto afu_init_err;
  }
devs = hid_enumerate(afedriusb.vid, afedriusb.pid);
if(devs == NULL)
  {
  lirerr(1394);
  goto afu_init_err;
  }
cur_dev = devs;
#if DISABLE_AFEDRI_SERIAL_NUMBER == FALSE
while (cur_dev) 
  {
  sprintf(s,"%ls",cur_dev->serial_number);
  i=0;
  while(s[i]!=0)i++;
  while(i<40)
    {
    s[i]=32;
    i++;
    }
  if(afedriusb.sernum1 == ise[0] &&
     afedriusb.sernum2 == ise[1] &&
     afedriusb.sernum3 == ise[2] &&
     afedriusb.sernum4 == ise[3])goto match;
  cur_dev = cur_dev->next;
  }
lirerr(1390);
goto afu_err;
match:;
#endif
fe_handle=hid_open_path(cur_dev->path);
if(fe_handle == NULL)
  {
  clear_screen();
  sprintf(s,"ERROR: COULD NOT OPEN  %s",cur_dev->path);
  lir_text(4,8,s);
  lir_text(6,10,press_any_key);
  await_keyboard();
  lirerr(1393);
  }
else
  {
  hidcmd_size=HIDCMD_NET_SIZE;
  clear_hid();
  afedriusb_rx_freq_control();
  afedriusb_rx_amp_control();
  }
#if DISABLE_AFEDRI_SERIAL_NUMBER == FALSE
afu_err:;
#endif
hid_free_enumeration(devs);
afu_init_err:;
}

short int get_eeprom_data(unsigned char VirtAddress)
{
unsigned char report[HID_COMMAND_MAX_SIZE];
short int *dat;
int i;
dat=(short int*)&report[2];
report[0]=HID_GENERIC_REPORT;
report[1]=HID_READ_EEPROM_COMMAND;
report[2]=VirtAddress;
for(i=3; i<HID_COMMAND_MAX_SIZE; i++)report[i]=0;
afedriusb_command(report);
return dat[0];
}

/* Disable this. Does not work correctly
int get_afedriusb_main_clock(void)
{
char c[4];
int *main_clock_freq;
short int *result1;
short int *result2;
main_clock_freq=(int*)&c[0];
result1=(short int*)&c[0];
result2=(short int*)&c[2];
result1[0]=get_eeprom_data(VADDRESS_MAIN_CLOCK_FREQ_LOW_HALFWORD);
result2[0]=get_eeprom_data(VADDRESS_MAIN_CLOCK_FREQ_HIGH_HALFWORD);
return main_clock_freq[0];
}
*/

void afedriusb_setup(void)
{
FILE *afedriusb_file;
int type, line, unit;
char s[128];
int i, k;
int *ise, *sdr_pi;
char *ss;
// int main_clock;
// double dt1;
// Enumerate and print the HID devices on the system
struct hid_device_info *devs, *cur_dev;
int afedri_vid[MAX_UNITS];
int afedri_pid[MAX_UNITS];
char afedri_sernum[MAX_UNITS][40];
line=2;
k=0;
clear_screen();
#if OSNUM == OSNUM_LINUX && defined(__linux__)
load_udev_library();
if(!udev_library_flag)return;
#endif
settextcolor(14);
lir_text(0,line,
    "Use an appropriate software tool from the manufacturer");
line++;
lir_text(0,line,
    "to configure your hardware");
settextcolor(7);
line+=4;
for(type=0; type<MAX_AFEDRI_TYPES; type++)
  {
  devs = hid_enumerate(vids[type], pids[type]);
  cur_dev = devs;
  while (cur_dev) 
    {
    sprintf(s,"%2d: Product %ls",k,cur_dev->product_string);
    lir_text(2,line,s);
    sprintf(s,"Type: %04hx %04hx",cur_dev->vendor_id, cur_dev->product_id);
    lir_text(36,line,s);
    afedri_vid[k]=cur_dev->vendor_id;
    afedri_pid[k]=cur_dev->product_id;
    sprintf(s,"Serial: %ls",cur_dev->serial_number);
    lir_text(53,line,s);
    sprintf(afedri_sernum[k],"%ls               ",cur_dev->serial_number);
    k++;
    line++;
    if(k >= MAX_UNITS)goto list_end;
    cur_dev = cur_dev->next;
    }
  hid_free_enumeration(devs);
  }
list_end:;  
line++;  
if(k == 0)
  {
  lir_text(5,line,"No Afedri device found on the USB system.");
  lir_text(5,line+2,press_any_key);
  await_processed_keyboard();
  return;
  }
if(k == 1)
  {
  lir_text(5,line,"Unit autoselected.");
  unit=0;
  }
else
  {
  lir_text(5,line,"Select radio hardware by line number. =>");
  unit=lir_get_integer(46,line,2,0,k-1);
  }
line+=2;
afedriusb.vid=afedri_vid[unit];
afedriusb.pid=afedri_pid[unit];
ise=(int*)afedri_sernum[unit];
ss=(char*)afedri_sernum[unit];
i=0;
while(ss[i]!=0)i++;
while(i<40)
  {
  ss[i]=32;
  i++;
  }
afedriusb.sernum1=ise[0];
afedriusb.sernum2=ise[1];
afedriusb.sernum3=ise[2];
afedriusb.sernum4=ise[3];
afedriusb.check=AFEDRIUSBPAR_VERNR;
afedriusb_file=fopen(afedriusb_parfil_name,"w");
for(i=0; i<MAX_VGA_TYPES; i++)
  {
  sprintf(s,"%d:  %s",i,vga_types[i]);
  lir_text(0,line,s);
  line++;
  }
line ++; 
lir_text(5,line,"Select VGA type by line number.");
afedriusb.vga_type=lir_get_integer(37,line,1,0,MAX_VGA_TYPES-1);
line+=2;
lir_text(5,line,"LO frequency error in ppb =>");
afedriusb.freq_adjust=lir_get_integer(34,line,7,-500000,500000);
if(afedriusb_file == NULL)
  {
  lirerr(384268);
  return;
  }
sdr_pi=(int*)(&afedriusb);
for(i=0; i<MAX_AFEDRIUSB_PARM; i++)
  {
  fprintf(afedriusb_file,"%s [%d]\n",afedriusb_parm_text[i],sdr_pi[i]);
  }
parfile_end(afedriusb_file);
open_afedriusb_control();
// main_clock=get_afedriusb_main_clock();
close_afedriusb_control();
/* Disable. Does not work properly.
dt1=(float)main_clock/ui.rx_ad_speed;
line+=2;
i=rint(dt1);
sprintf(s,"main clock %d",main_clock);
lir_text(0,line,s);
line+=2;
if(i>0)
  {
  dt1=main_clock/i;
  sprintf(s,"ADC clock = %.3f MH z  Divisor = %d",0.000001*(float)main_clock,i);
  lir_text(0,line,s);
  line++;
  sprintf(s,"Nominal sampling rate %d  True sampling rate %.1f",
                                                 ui.rx_ad_speed,rint(dt1));
  lir_text(0,line,s);
  line+=2;
  sprintf(s,"%s",controlbox);
  }
else
  {
  sprintf(s,"%s",controlbox);
  lir_text(0,line,s);
  line++;
  sprintf(s,"save Main Clock to its correct value and");    
  lir_text(0,line,s);
  }
line++;
sprintf(s,"save mode and USB sampling rate.");
lir_text(0,line,s);
line++;
sprintf(s,"Remember to reset your Afedri box after any change.");
lir_text(0,line,s);

line+=2;
lir_text(5,line,press_any_key);
await_processed_keyboard();
*/
}

int verify_netafedri_parameters(void)
{
if(netafedri.ip0 < 0 || netafedri.ip0 > 255 ||
   netafedri.ip1 < 0 || netafedri.ip1 > 255 ||
   netafedri.ip2 < 0 || netafedri.ip2 > 255 ||
   netafedri.ip3 < 0 || netafedri.ip3 > 255 ||
   netafedri.port < 0 || netafedri.port > 65535 ||
   netafedri.udp_size < 0 || netafedri.udp_size > 1 ||
   netafedri.vga_type < 0 || netafedri.vga_type > 1 ||
   abs(netafedri.clock_adjust) > 200000 ||
   netafedri.channels < 1 || netafedri.channels > 2 ||
   netafedri.multistream < 0 || netafedri.multistream > 1 ||
   (netafedri.multistream != 0 && netafedri.channels == 2) ||
   netafedri.check != NETAFEDRI_PAR_VERNR)
  {
  return FALSE;
  }
else
  {
  if(netafedri.format != 16 && netafedri.format != 24)return FALSE;
  if(netafedri.sampling_rate < 32000)return FALSE;
  if(netafedri.sampling_rate > 200000000)return FALSE;
  return TRUE;
  }
}

int display_netafedri_parm_info(int *line)
{
char s[80];
int errcod;
errcod=read_sdrpar(netafedri_parfil_name, MAX_NETAFEDRI_PARM,
                                     netafedri_parm_text, (int*)((void*)&netafedri));
if(errcod == 0)
  {
  settextcolor(7);
  sprintf(s,"IP address       = %d.%d.%d.%d",
                          netafedri.ip0,netafedri.ip1,netafedri.ip2,netafedri.ip3);
  lir_text(24,line[0],s);
  line[0]++;
  SNDLOG"\n%s",s);
  sprintf(s,"Port             = %d",netafedri.port);
  lir_text(24,line[0],s);
  line[0]++;
  sprintf(s,"Sampling speed   = %d Hz Correction = %d ppb ", 
                              netafedri.sampling_rate,netafedri.clock_adjust);
  lir_text(24,line[0],s);
  line[0]++;
  sprintf(s,"Bits per sample  = %d  ",netafedri.format);
  if(netafedri.multistream == 0)
    {
    sprintf(&s[strlen(s)],"Channels =%d", netafedri.channels);
    }
  else
    {
    sprintf(&s[strlen(s)],"Multistream ( 1 channel)");
    }  
  lir_text(24,line[0],s);
  line[0]++;
  sprintf(s,"UDP packet size  = %s",pkgsize[netafedri.udp_size]);
  lir_text(24,line[0],s);
  line[0]++;
  sprintf(s,"VGA type         = %s",vga_types[netafedri.vga_type]);
  lir_text(24,line[0],s);
  line[0]++;
  }
return (errcod);
}


void open_netafedri(void)
{
int i, j;
unsigned char *ip;
char s[80];
sdr=-1;
#if(OSNUM == OSNUM_WINDOWS)
if(WSAStartup(2, &wsadata) != 0)
  {
  lirerr(1263);
  return;   
  }
#endif
memset(&netafedri_addr, 0, sizeof(netafedri_addr));
netafedri_addr.sin_family=AF_INET;
netafedri_addr.sin_port=htons(netafedri.port);
ip=(unsigned char*)&netafedri_addr.sin_addr.s_addr;
ip[0]=netafedri.ip0;
ip[1]=netafedri.ip1;
ip[2]=netafedri.ip2;
ip[3]=netafedri.ip3;
if ((netafedri_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVSOCK)
  {
  lirerr(1249);
  return;
  }
j=connect(netafedri_fd, (struct sockaddr *)&netafedri_addr, sizeof(netafedri_addr));
lir_sleep(30000);
i=0;
lir_inkey=0;
s[0]=0;
while(j == -1 && lir_inkey == 0)
  {
  if(i > 2)sprintf(s,"Trying to connect %d,  errno=%d",i,errno);
  lir_text(1,5,s);
  lir_refresh_screen();
  j=connect(netafedri_fd, (struct sockaddr *)&netafedri_addr, sizeof(netafedri_addr));
  test_keyboard();
  if(kill_all_flag)return;
  i++;
  lir_sleep(30000);
  }
if(lir_inkey != 0)
  {
  lir_inkey=0;
  return;
  }
sdr=0;
}

void netafedri_setup_header(int *line)
{
char s[80];
clear_screen();
line[0]=2;
settextcolor(13);
lir_text(20,line[0],"Installation routine for NET Afedri");
settextcolor(7);
line[0]+=2;
sprintf(s,"IP address = %d.%d.%d.%d",netafedri.ip0,netafedri.ip1,netafedri.ip2,netafedri.ip3);
lir_text(10,line[0],s);
line[0]++;
sprintf(s,"Port = %d",netafedri.port);
lir_text(16,line[0],s);
line[0]+=2;
lir_refresh_screen();
}

void close_netafedri(void)
{
CLOSE_FD(netafedri_fd);
sdr=-1;
}

void init_netafedri(void)
{
double dt1;
int i, errcod, line, length;
char msg[20], s[256], textbuf[128];
unsigned b0, b1, b2, b3;
unsigned char c;
float t1;
int test_rate;
int64_t *max_rate;
int64_t max_sampling_rate;
FILE *netafedri_file;
int *sdr_pi;
int *idat;
int adc_rate;
char* adc_rate_bytes;
adc_rate_bytes=(char*)&adc_rate;
errcod=read_sdrpar(netafedri_parfil_name, MAX_NETAFEDRI_PARM, 
                                     netafedri_parm_text, (int*)((void*)&netafedri));
if(errcod != 0)
  {
  netafedri.ip0=10;
  netafedri.ip1=0;
  netafedri.ip2=0;
  netafedri.ip3=92;
  netafedri.port=50005;
  netafedri.sampling_rate=250000;
  netafedri.format=16;
  netafedri.udp_size=0;
  netafedri.vga_type=1;
  netafedri.clock_adjust=0;  
  netafedri.channels=2;
  netafedri.multistream=0;
  netafedri.check=NETAFEDRI_PAR_VERNR;
  }
restart:;  
netafedri_setup_header(&line);
lir_text(0,line,"Use the Afedri tool SDR_Control.exe in Windows to");
line++;
lir_text(0,line,"set IP address and port for your hardware or change");
line++;
lir_text(0,line,"address and port to agree with your NET Afedri");
line+=4;
lir_text(12,line,"A=Change IP address.");
line++;
lir_text(12,line,"B=Change port.");
line++;
lir_text(12,line,"C=Connect to the AFEDRI-NET");
line++;
lir_text(12,line,"X=Skip and return to previous menu.");
line+=2;
await_processed_keyboard();
if(kill_all_flag)return;
switch (lir_inkey)
  {
  case 'A':
specify:
  lir_text(10,line,"Specify new IP address:");
  length=lir_get_text(40, line, textbuf);
  if(kill_all_flag) return;
// validate format of IP address 
  if ((length < 7) || (length > 15))  goto format_error;
  if (strspn(textbuf, "0123456789.") < strlen(textbuf)) goto format_error;
  if (sscanf(textbuf, "%3u.%3u.%3u.%3u%c", &b0, &b1, &b2, &b3, &c) != 4)
                                                        goto format_error;
  if ((b0 | b1 | b2 | b3) > 255)
    {
format_error:;
    lir_text(10,line+1,"IP address format-error: Try again ");
    goto specify; 
    }
  netafedri.ip0=b0;
  netafedri.ip1=b1;
  netafedri.ip2=b2;
  netafedri.ip3=b3;  
  break;

  case 'B':
  lir_text(10,line,"New port number:");
  netafedri.port=lir_get_integer(28,line,5,0,65535);
  break;

  case 'C':
  clear_screen();
  open_netafedri();
  if(sdr == 0)goto verify_open;
  break;

  case 'X':
  return;
  }
if(kill_all_flag)return;  
goto restart;
verify_open:;
netafedri_setup_header(&line);
msg[0]=0x04;
msg[1]=0x20;
msg[2]=0x01;
msg[3]=0x00;
net_write(netafedri_fd,msg,4);
lir_sleep(100000);
recv(netafedri_fd,textbuf,80,0);
sprintf(s,"Target Name = %s",&textbuf[4]);
lir_text(9,line,s);  
line++;
lir_refresh_screen();
msg[2]=0x02;
net_write(netafedri_fd,msg,4);
lir_sleep(100000);
recv(netafedri_fd,textbuf,80,0);
sprintf(s,"Serial number = %s",&textbuf[4]);
lir_text(7,line,s);  
line++;
lir_refresh_screen();
msg[2]=0x03;
net_write(netafedri_fd,msg,4);
lir_sleep(100000);
recv(netafedri_fd,textbuf,80,0);
t1=256*textbuf[5]+textbuf[4];
sprintf(s,"Interface version = %f",0.01*t1);
lir_text(3,line,s);  
line++;
lir_refresh_screen();
msg[2]=0x04;
net_write(netafedri_fd,msg,4);
lir_sleep(100000);
recv(netafedri_fd,textbuf,80,0);
t1=256*textbuf[5]+textbuf[4];
sprintf(s,"Boot code version = %f",0.01*t1);
lir_text(3,line,s);  
line++;
lir_refresh_screen();
msg[3]=0x01;
net_write(netafedri_fd,msg,4);
lir_sleep(100000);
recv(netafedri_fd,textbuf,80,0);
t1=256*textbuf[5]+textbuf[4];
sprintf(s,"Firmware version = %f",0.01*t1);
lir_text(4,line,s);  
line++;
lir_refresh_screen();
msg[3]=0x02;
net_write(netafedri_fd,msg,4);
lir_sleep(100000);
recv(netafedri_fd,textbuf,80,0);
t1=256*textbuf[5]+textbuf[4];
sprintf(s,"Hardware version = %f",0.01*t1);
lir_text(4,line,s);  
line++;
lir_refresh_screen();
msg[2]=0x09;
msg[3]=0x00;
net_write(netafedri_fd,msg,4);
lir_sleep(100000);
for(i=0; i<80; i++)textbuf[i]=0;
recv(netafedri_fd,textbuf,80,0);
sprintf(s,"Product ID       = %s (=[%d][%d][%d][%d])",&textbuf[4],
                               textbuf[4],textbuf[5],textbuf[6],textbuf[7]);
lir_text(4,line,s);
line++;
msg[0]=0x09;
msg[1]=0xe0;
msg[2]=0x02;
msg[3]=0x3f;
for(i=4; i<10; i++)msg[i]=0;
net_write(netafedri_fd,msg,9);
lir_sleep(100000);
for(i=0; i<80; i++)textbuf[i]=0;
recv(netafedri_fd,textbuf,80,0);
max_rate=(int64_t*)&textbuf[4];
max_sampling_rate=max_rate[0];
if(textbuf[3]==0x1c)max_sampling_rate=1333333;
#if OSNUM == OSNUM_WINDOWS
sprintf(s,"Max samp. rate   = %lld Hz",max_sampling_rate);
#endif
#if OSNUM == OSNUM_LINUX
#if IA64 == 0
sprintf(s,"Max samp. rate   = %lld Hz",max_sampling_rate);
#else
#if DARWIN == 0
sprintf(s,"Max samp. rate   = %ld Hz",max_sampling_rate);
#else
sprintf(s,"Max samp. rate   = %lld Hz",max_sampling_rate);
#endif
#endif
#endif
lir_text(4,line,s);
line++;
// Find the frequency of the master clock.
msg[0]=0x09;
msg[1]=0xe0;
msg[2]=0x02;
msg[3]=0x55;
for(i=4; i<10; i++)msg[i]=0;
net_write(netafedri_fd,msg,9);
lir_sleep(100000);
for(i=0; i<80; i++)textbuf[i]=0;
recv(netafedri_fd,textbuf,80,0);
adc_rate_bytes[0]=textbuf[4];
adc_rate_bytes[1]=textbuf[5];
msg[4]=1;
net_write(netafedri_fd,msg,9);
lir_sleep(100000);
for(i=0; i<80; i++)textbuf[i]=0;
recv(netafedri_fd,textbuf,80,0);
adc_rate_bytes[2]=textbuf[4];
adc_rate_bytes[3]=textbuf[5];
sprintf(s,"ADC clock        = %d Hz",adc_rate);
lir_text(4,line,s);
line+=2;
lir_text(10,line,"Use this hardware? (Y/N)");
lir_inkey=0;
while(lir_inkey != 'Y')
  {
  if(kill_all_flag)goto netafedri_errexit;
  await_processed_keyboard();
  if(lir_inkey == 'N')goto restart;
  }
test_rate=-1;  
setpar:;  
if(test_rate != netafedri.sampling_rate)
  {
// Set the DDC output sampling rate.
  msg[0]=0x09;
  msg[1]=0x00;
  msg[2]=0xb8;
  msg[3]=0x00;
  msg[4]=0x00;
  idat=(int*)&msg[5];  
  idat[0]=netafedri.sampling_rate;
  net_write(netafedri_fd,msg,9);
  lir_sleep(100000);
  recv(netafedri_fd,textbuf,80,0);
  netafedri.sampling_rate=idat[0];
  test_rate=idat[0];;
  }
netafedri_setup_header(&line);
lir_text(0,line,"Set global parameters for the NET-Afedri");
line++;
sprintf(s,"A => Sampling rate        = %d Hz)", netafedri.sampling_rate);
lir_text(0,line,s);
line++;
sprintf(s,"B => Bits per sample      = %d",netafedri.format);
lir_text(0,line,s);
line++;
sprintf(s,"C => UDP packet size      = %s",pkgsize[netafedri.udp_size]);
lir_text(0,line,s);
line++;
sprintf(s,"D => VGA type             = %s",vga_types[netafedri.vga_type]);
lir_text(0,line,s);
line++;
sprintf(s,"E => No of RF channels    = %d",netafedri.channels);
lir_text(0,line,s);
line++;
sprintf(s,"F => Frequency correction = %d ppb",netafedri.clock_adjust);
lir_text(0,line,s);
line++;
sprintf(s,"H => Multistream mode     = %d",netafedri.multistream);
lir_text(0,line,s);
line++;
sprintf(s,"X => Save settings in %s and exit",netafedri_parfil_name);
lir_text(0,line,s);
line+=3;
await_processed_keyboard();
if(kill_all_flag)goto netafedri_errexit;
switch (lir_inkey)
  {
  case 'A':
  i=16*max_sampling_rate/(netafedri.channels*netafedri.format);
  sprintf(s,"Enter new sampling rate in Hz (48000 to %d) =>",i);
  lir_text(0,line+1,s);
  i=lir_get_integer(strlen(s)+2,line+1,7,48000,i);
  dt1=(float)adc_rate/i;
  i=rint(dt1);
  dt1=adc_rate/i;
  netafedri.sampling_rate=rint(dt1);
  clear_lines(line+1,line+1);
  break;
  
  case 'B':
  if(netafedri.format == 16)
    {
    netafedri.format=24;
    i=16*max_sampling_rate/(netafedri.channels*netafedri.format);
    if(netafedri.sampling_rate > i)netafedri.sampling_rate=i;
    }
  else
    {
    netafedri.format=16;
    }  
  break;
  
  case 'C':
  netafedri.udp_size^=1;
  break;
  
  case 'D':
  netafedri.vga_type^=1;
  break;
  
  case 'E':
  netafedri.channels++;
  if(netafedri.channels > 2)netafedri.channels=1;
  i=16*max_sampling_rate/(netafedri.channels*netafedri.format);
  if(netafedri.sampling_rate > i)netafedri.sampling_rate=i;
  if(netafedri.channels == 2)netafedri.multistream=0;
  break;
  
  case 'F':
  sprintf(s,"Enter new frequency correction in ppb =>");
  lir_text(0,line+1,s);
  netafedri.clock_adjust=lir_get_integer(strlen(s)+2,line+1,7,-200000,200000);
  clear_lines(line+1,line+1);
  break;
  
  case 'H':
  netafedri.multistream++;
  netafedri.multistream&=1;
  if(netafedri.multistream != 0)netafedri.channels=1;
  break;
  
  case 'X':
  close_netafedri();
  adjusted_sdr_clock=1+0.000000001*netafedri.clock_adjust;
  ui.rx_ad_speed=adjusted_sdr_clock*netafedri.sampling_rate;
  ui.rx_input_mode=IQ_DATA+DIGITAL_IQ;
  if(netafedri.format == 24)ui.rx_input_mode+=DWORD_INPUT;
  ui.rx_rf_channels=netafedri.channels;
  if(ui.rx_rf_channels == 2)ui.rx_input_mode+=TWO_CHANNELS;
  ui.rx_ad_channels=2*ui.rx_rf_channels;
  ui.rx_admode=0;
  netafedri.check=NETAFEDRI_PAR_VERNR;
  netafedri_file=fopen(netafedri_parfil_name,"w");
  if(netafedri_file == NULL)
    {
netafedri_errexit:;
    CLOSE_FD(netafedri_fd);
#if(OSNUM == OSNUM_WINDOWS)
    WSACleanup();
#endif    
    clear_screen();
    return;
    }
  sdr_pi=(int*)(&netafedri);
  for(i=0; i<MAX_NETAFEDRI_PARM; i++)
    {
    fprintf(netafedri_file,"%s [%d]\n",netafedri_parm_text[i],sdr_pi[i]);
    }
  parfile_end(netafedri_file);
  ui.rx_addev_no=NETAFEDRI_DEVICE_CODE;
#if(OSNUM == OSNUM_WINDOWS)
  WSACleanup();
#endif    
  return;
  }
goto setpar;   
}

void set_netafedri_att(void)
{
char msg[20], textbuf[80];
msg[0]=0x06;
msg[1]=0x00;
msg[2]=0x38;
msg[3]=0x00;
msg[4]=0x00;
if(fg.gain <=35)
  {
  msg[5]=8*((fg.gain+10)/3)+4;
  }
else
  {
  msg[5]=124;
  }  
net_write(netafedri_fd,msg,6);
lir_sleep(100000);
recv(netafedri_fd,textbuf,80,0);
}


void set_netafedri_frequency(void)
{
int i;
char msg[20], textbuf[80];
uint64_t frequency;
msg[0]=0x0a;
msg[1]=0x00;
msg[2]=0x20;
msg[3]=0x00;
msg[4]=0x00;
frequency=rint(adjusted_sdr_clock*fg.passband_center*1000000);
fg_truncation_error=frequency-adjusted_sdr_clock*fg.passband_center*1000000;
for(i=0; i < 4;  i++)
  {
  msg[5+i]= (frequency >> (i*8)) & 0xFF;
  }
net_write(netafedri_fd,msg,10);
lir_sleep(100000);
recv(netafedri_fd,textbuf,80,0);
msg[4]=0x01;
net_write(netafedri_fd,msg,10);
lir_sleep(100000);
recv(netafedri_fd,textbuf,80,0);
}

void netafedri_input(void)
{
struct sockaddr_in udp_addr;
FD udp_fd;
int i,errcod;
int rxin_local_workload_reset;
int pkg_size;
int timf1_netptr;
short int pkg_chk;
int netafedri_mode;
int *sr;
char *tmpbuf;
char s[80], msg[20], textbuf[80];
double dt1, read_start_time, total_reads;
short int *ichk;
int timing_loop_counter,timing_loop_counter_max,initial_skip_flag;
float t1;
int local_att_counter;
int local_nco_counter;
short int pkg_number;
int err;
#if(OSNUM == OSNUM_WINDOWS)
char optval;
#endif
#if(OSNUM == OSNUM_LINUX)
int optval;
struct timeval timeout;
clear_thread_times(THREAD_NETAFEDRI_INPUT);
#endif
local_att_counter=sdr_att_counter;
local_nco_counter=sdr_nco_counter;
errcod=0;
pkg_number=0;
dt1=current_time();
errcod=read_sdrpar(netafedri_parfil_name, MAX_NETAFEDRI_PARM, 
                                     netafedri_parm_text, (int*)((void*)&netafedri));
if(errcod != 0)
  {
parerr:;  
  errcod=1170;
  goto netafedri_init_error_exit;
  }
i=verify_netafedri_parameters();
if(i == FALSE)goto parerr;
while(sdr == -1)
  {
  open_netafedri();
  lir_sleep(30000);
  if(sdr == -1)
    {
    sprintf(s,"Waiting %.2f", current_time()-dt1);
    lir_text(0,screen_last_line,s);
    lir_refresh_screen();
    if(kill_all_flag)goto netafedri_init_error_exit;
    }
  }
adjusted_sdr_clock=1+0.000000001*netafedri.clock_adjust;
t1=ui.rx_ad_speed-adjusted_sdr_clock*netafedri.sampling_rate;
if(fabs(t1/ui.rx_ad_speed) > 0.0001)
  {
  errcod=1330;
  goto netafedri_error_exit;
  }
// Set the number of channels.
msg[0]=0x09;
msg[1]=0xe0;
msg[2]=0x02;
msg[3]=0x30;
if(netafedri.multistream == 0)
  {
  msg[4]=netafedri.channels-1;
  }
else
  {
  msg[4]=2;
  }
for(i=5; i<9; i++)msg[i]=0;
net_write(netafedri_fd,msg,9);
lir_sleep(100000);
recv(netafedri_fd,textbuf,80,0);
// Set the DDC output sampling rate.
msg[0]=0x09;
msg[1]=0x00;
msg[2]=0xb8;
msg[3]=0x00;
msg[4]=0x00;
sr=(int*)&msg[5];  
sr[0]=netafedri.sampling_rate;
net_write(netafedri_fd,msg,9);
lir_sleep(100000);
recv(netafedri_fd,textbuf,80,0);
netafedri_mode=netafedri.udp_size;
if(netafedri.multistream == 0)
  {
// Set the data output packet size.
  msg[0]=0x05;
  msg[1]=0x00;
  msg[2]=0xc4;
  msg[3]=0x00;
  msg[4]=netafedri.udp_size;
  net_write(netafedri_fd,msg,5);
  lir_sleep(100000);
  recv(netafedri_fd,textbuf,80,0);
  } 
set_netafedri_att();
set_netafedri_frequency();
fft1_block_timing();
if(thread_command_flag[THREAD_SCREEN]!=THRFLAG_NOT_ACTIVE)
  {
  while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
    {
    if(thread_command_flag[THREAD_NETAFEDRI_INPUT] ==
                                           THRFLAG_KILL)goto netafedri_error_exit;
    lir_sleep(10000);
    }
  }
// Set the receiver state.
msg[0]=0x08;
msg[1]=0x00;
msg[2]=0x18;
msg[3]=0x00;
msg[4]=0x80;        // Set complex contigous data.
msg[5]=0x02;        // set run/stop to RUN
if(netafedri.format == 16)
  {
  msg[6]=0x00;
  if( (ui.rx_input_mode&DWORD_INPUT) != 0)lirerr(1452);
  }
else
  {
  netafedri_mode|=2;
  msg[6]=0x80;  
  if( (ui.rx_input_mode&DWORD_INPUT) == 0)lirerr(1452);
  }
msg[7]=1;
net_write(netafedri_fd,msg,8);
lir_sleep(100000);
recv(netafedri_fd,textbuf,80,0);
if ((udp_fd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) == INVSOCK) 
  {
  lirerr(1284);
  goto netafedri_exit;
  }
optval=TRUE;
err=setsockopt( udp_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval) );
if(err != 0)
  {
  lirerr(1391);
  goto netafedri_exit;
  }
#if OSNUM == OSNUM_LINUX
timeout.tv_sec=1;
timeout.tv_usec=0;
err=setsockopt( udp_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout) );
if(err != 0)
  {
  lirerr(1391);
  goto netafedri_exit;
  }
#endif
i=131072;
err=setsockopt(udp_fd,SOL_SOCKET,SO_RCVBUF,(char*)&i,sizeof(int));
if(err != 0)
  {
  lirerr(1391);
  goto netafedri_exit;
  }
i=8192;
err=setsockopt(udp_fd,SOL_SOCKET,SO_SNDBUF,(char*)&i,sizeof(int));
if(err != 0)
  {
  lirerr(1391);
  goto netafedri_exit;
  }
memset(&udp_addr, 0, sizeof(udp_addr)); 
udp_addr.sin_family=AF_INET;
udp_addr.sin_port=htons(netafedri.port);
udp_addr.sin_addr.s_addr = htonl(INADDR_ANY );
// bind to the SDR-IP address and port.
if (bind(udp_fd, (struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0) 
  {
  lirerr(1302);
  goto netafedri_udp_exit;
  }
tmpbuf=malloc(1444);
if(tmpbuf == NULL)goto netafedri_udp_exit;
// netafedri_mode == 0 => large UDP, 16 bit
// netafedri_mode == 1 => small UDP, 16 bit
// netafedri_mode == 2 => large UDP, 24 bit
// netafedri_mode == 3 => small UDP, 24 bit
timf1_netptr=timf1p_pa;
switch (netafedri_mode)
  {
  case 0:
  pkg_size=1028;
  pkg_chk=0x8404;
  break;

  case 1:
  pkg_size=516;
  pkg_chk=0x8204;

  break;

  case 2:
  pkg_size=1444;
  pkg_chk=0x85a4;

  break;

  case 3:
  pkg_size=388;
  pkg_chk=0x8184;
  break;
  
  default:
  pkg_size=0;
  pkg_chk=0;
  lirerr(46295);
  goto netafedri_udpbuf_exit;
  }
#include "timing_setup.c"
thread_status_flag[THREAD_NETAFEDRI_INPUT]=THRFLAG_ACTIVE;
lir_status=LIR_OK;
while(thread_command_flag[THREAD_NETAFEDRI_INPUT] == THRFLAG_ACTIVE)
  {
  if(local_att_counter != sdr_att_counter &&
     ! sys_func_async_start(THRFLAG_SET_NETAFEDRI_ATT) &&
  	   sys_func_async_join(THRFLAG_SET_NETAFEDRI_ATT))
    local_att_counter = sdr_att_counter;
  if (local_nco_counter != sdr_nco_counter &&
	!sys_func_async_start(THRFLAG_SET_NETAFEDRI_FREQUENCY) &&
	sys_func_async_join(THRFLAG_SET_NETAFEDRI_FREQUENCY))
	local_nco_counter = sdr_nco_counter;
  ichk=(short int*)tmpbuf;
recv_again:;
  i=recv(udp_fd, tmpbuf, pkg_size, 0);
  if(kill_all_flag)goto netafedri_udpbuf_exit;
  if(i < 0) goto recv_again;
  if(ichk[0] != pkg_chk)goto recv_again;
// In case the package number is too low, a package has been re-sent.
// This happens on my LAN when a slow computer is connected to the
// Afedri through several switches.
// If the package is not very old, assume it has already been properly
// processed and ignore the resent package.
#define MAX_RESEND 25
  if( pkg_number-ichk[1] > 0 && pkg_number-ichk[1] < MAX_RESEND)goto recv_again;  
// In case the packet number is totally wrong, just
// set pkg_number to the current package number.
  if(abs(ichk[1]-pkg_number) >= MAX_RESEND)pkg_number=ichk[1];
// Fill zeroes in case the packet number does not agree
// with pkg_number and increase pkg_number until we
// reach agreement. Then fill the current data from the buffer.
  if(netafedri.format == 16)
    {
    while(pkg_number != ichk[1])
      {
      for(i=4; i<pkg_size; i++)
        {
        timf1_char[timf1_netptr]=0;
        timf1_netptr=(timf1_netptr+1)&timf1_bytemask;
        }
      pkg_number++;
      }
    for(i=4; i<pkg_size; i++)
      {
      timf1_char[timf1_netptr]=tmpbuf[i];
      timf1_netptr=(timf1_netptr+1)&timf1_bytemask;
      }
    pkg_number++;
    }
  else
    {
    while(pkg_number != ichk[1])
      {
      for(i=4; i<pkg_size; i+=3)
        {
        timf1_char[timf1_netptr  ]=0;
        timf1_char[timf1_netptr+1]=0;
        timf1_char[timf1_netptr+2]=0;
        timf1_char[timf1_netptr+3]=0;
        timf1_netptr=(timf1_netptr+4)&timf1_bytemask;
        }
      pkg_number++;
      }
    for(i=4; i<pkg_size; i+=3)
      {
      timf1_char[timf1_netptr  ]=0;
      timf1_char[timf1_netptr+1]=tmpbuf[i  ];
      timf1_char[timf1_netptr+2]=tmpbuf[i+1];
      timf1_char[timf1_netptr+3]=tmpbuf[i+2];
      timf1_netptr=(timf1_netptr+4)&timf1_bytemask;
      }
    pkg_number++;
    }  
  while( ((timf1_netptr-timf1p_pa+timf1_bytes)&timf1_bytemask) > 
                                     2*snd[RXAD].block_bytes)
    {
#include "input_speed.c"
    finish_rx_read();
    }    
  if(kill_all_flag) goto netafedri_udpbuf_exit;
  }
netafedri_udpbuf_exit:;
free(tmpbuf);  
netafedri_udp_exit:;
CLOSE_FD(udp_fd);
netafedri_exit:;
// Set the receiver state.
msg[0]=0x08;
msg[1]=0x00;
msg[2]=0x18;
msg[3]=0x00;
msg[4]=0x80;        // Set complex contigous data.
msg[5]=0x01;        // set run/stop to STOP
if(netafedri.format == 16)
  {
  msg[6]=0x00;
  }
else
  {
  msg[6]=0x80;  
  }
msg[7]=0x01;  
net_write(netafedri_fd,msg,8);
lir_sleep(100000);
recv(netafedri_fd,textbuf,80,0);
netafedri_error_exit:;
close_netafedri();
netafedri_init_error_exit:;
if(errcod != 0)lirerr(errcod);
thread_status_flag[THREAD_NETAFEDRI_INPUT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_NETAFEDRI_INPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}
