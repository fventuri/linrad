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
#include <fcntl.h>
#include <string.h>

#if(OSNUM == OSNUM_WINDOWS)
#include <windows.h>
#include <winsock.h>
#include "wscreen.h"
#define RECV_FLAG 0
#define INVSOCK INVALID_SOCKET
#define CLOSE_FD closesocket
extern WSADATA wsadata;
#endif

#if(OSNUM == OSNUM_LINUX)
#include <fcntl.h>
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
#include <unistd.h>
#include <string.h>
#include "lscreen.h"
#ifdef MSG_WAITALL
#define RECV_FLAG MSG_WAITALL
#else
#define RECV_FLAG 0
#endif
#define IP_MREQ struct ip_mreq
#define INVSOCK -1
#define CLOSE_FD close
#endif

#include "uidef.h"
#include "sdrdef.h"
#include "vernr.h"
#include "thrdef.h"
#include "fft1def.h"
#include "screendef.h"

void net_write(FD fd, void *buf, int size);

// Structure for CLOUDIQ parameters
typedef struct {
int ip0;
int ip1;
int ip2;
int ip3;
int port;
int rate_dividend;
int format;
int udp_size;
int dither;
int adgain;
int rf_filter;
int connect;
int clock_adjust;
int check;
}P_CLOUDIQ;
#define MAX_CLOUDIQ_PARM 14

P_CLOUDIQ cloudiq;

char *cloudiq_parfil_name="par_cloudiq";
char *cloudiq_parm_text[MAX_CLOUDIQ_PARM]={"IP1",
                                       "IP2",
                                       "IP3",
                                       "IP4",
                                       "Port",
                                       "Rate dividend",
                                       "Format",
                                       "UDP size",
                                       "Dither",
                                       "ADgain",
                                       "RF filter",
                                       "Connect",
                                       "Clock adjust",
                                       "Check"
                                       }; 
FD cloudiq_fd;
struct sockaddr_in cloudiq_addr;

int verify_cloudiq_parameters(void)
{
if(cloudiq.ip0 < 0 || cloudiq.ip0 > 255 ||
   cloudiq.ip1 < 0 || cloudiq.ip1 > 255 ||
   cloudiq.ip2 < 0 || cloudiq.ip2 > 255 ||
   cloudiq.ip3 < 0 || cloudiq.ip3 > 255 ||
   cloudiq.port < 0 || cloudiq.port > 65535 ||
   cloudiq.udp_size < 0 || cloudiq.udp_size > 1 ||
   cloudiq.dither < 0 || cloudiq.dither > 1 ||
   cloudiq.adgain < 0 || cloudiq.adgain > 1 ||
   cloudiq.rf_filter < 0 || cloudiq.rf_filter > 1 ||
   cloudiq.connect < 0 || cloudiq.connect > 1 ||
   abs(cloudiq.clock_adjust) > 200000 ||
   cloudiq.check != CLOUDIQ_PAR_VERNR)
  {
  return FALSE;
  }
if(cloudiq.format != 16 && cloudiq.format != 24)return FALSE;
if(cloudiq.rate_dividend < 25 || cloudiq.rate_dividend > 8191)return FALSE;
if(cloudiq.format == 16 && cloudiq.rate_dividend < 17 )return FALSE;
return TRUE;
}


int display_cloudiq_parm_info(int *line)
{
char s[80];
int errcod;
errcod=read_sdrpar(cloudiq_parfil_name, MAX_CLOUDIQ_PARM, 
                                     cloudiq_parm_text, (int*)((void*)&cloudiq));
if(errcod == 0)
  {
  settextcolor(7);
  sprintf(s,"IP address         = %d.%d.%d.%d", 
                          cloudiq.ip0,cloudiq.ip1,cloudiq.ip2,cloudiq.ip3);
  lir_text(24,line[0],s);
  line[0]++;
  SNDLOG"\n%s",s);
  sprintf(s,"Port               = %d",cloudiq.port);
  lir_text(24,line[0],s);
  line[0]++;
  sprintf(s,"Sampling speed     = %.0f kHz   (dividend=%d)  adjust %d ppb",
          0.00025*CLOUDIQ_SAMPLING_CLOCK/cloudiq.rate_dividend,
                     cloudiq.rate_dividend,cloudiq.clock_adjust);
  lir_text(24,line[0],s);
  line[0]++;
  sprintf(s,"Bits per sample    = %d",cloudiq.format);
  lir_text(24,line[0],s);
  line[0]++;
  sprintf(s,"UDP packet size    = %s",pkgsize[cloudiq.udp_size]);
  lir_text(24,line[0],s);
  line[0]++;
  sprintf(s,"Dither             = %s",onoff[cloudiq.dither]);
  lir_text(24,line[0],s);
  line[0]++;
  sprintf(s,"A/D gain           = %s",adgains[cloudiq.adgain]);
  lir_text(24,line[0],s);
  line[0]++;
  sprintf(s,"RF filter          = %s",rffil[cloudiq.rf_filter]);
  lir_text(24,line[0],s);
  line[0]++;
  sprintf(s,"Input connection   = %s",inp_connect[cloudiq.connect]);
  lir_text(24,line[0],s);
  }
return (errcod);
}


void open_cloudiq(void)
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
memset(&cloudiq_addr, 0, sizeof(cloudiq_addr)); 
cloudiq_addr.sin_family=AF_INET;
cloudiq_addr.sin_port=htons(cloudiq.port);
ip=(unsigned char*)&cloudiq_addr.sin_addr.s_addr;
ip[0]=cloudiq.ip0;
ip[1]=cloudiq.ip1;
ip[2]=cloudiq.ip2;
ip[3]=cloudiq.ip3;
if ((cloudiq_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVSOCK) 
  {
  lirerr(1249);
  return;
  }
j=connect(cloudiq_fd, (struct sockaddr *)&cloudiq_addr, sizeof(cloudiq_addr));
lir_sleep(30000);
i=0;
lir_inkey=0;
s[0]=0;
while(j == -1 && lir_inkey == 0)
  {
  if(kill_all_flag)return;
  if(i > 2)sprintf(s,"Trying to connect %d,  errno=%d",i,errno);
  lir_text(1,5,s);
  lir_refresh_screen();
  j=connect(cloudiq_fd, (struct sockaddr *)&cloudiq_addr, sizeof(cloudiq_addr));
  test_keyboard();
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

void cloudiq_setup_header(int *line)
{
char s[80];
clear_screen();
line[0]=2;
settextcolor(13);
lir_text(20,line[0],"Installation routine for CloudIQ");
settextcolor(7);
line[0]+=2;
sprintf(s,"IP address = %d.%d.%d.%d",cloudiq.ip0,cloudiq.ip1,cloudiq.ip2,cloudiq.ip3);
lir_text(10,line[0],s);
line[0]++;
sprintf(s,"Port = %d",cloudiq.port);
lir_text(16,line[0],s);
line[0]+=2;
lir_refresh_screen();
}

void close_cloudiq(void)
{
CLOSE_FD(cloudiq_fd);
sdr=-1;
#if(OSNUM == OSNUM_WINDOWS)
    WSACleanup();
#endif    
}

void init_cloudiq(void)
{
int i, errcod, line, length;
char msg[20], s[128], textbuf[80];
unsigned b0, b1, b2, b3;
unsigned char c;
float t1;
FILE *cloudiq_file;
int *sdr_pi;
errcod=read_sdrpar(cloudiq_parfil_name, MAX_CLOUDIQ_PARM, 
                                     cloudiq_parm_text, (int*)((void*)&cloudiq));
if(errcod != 0)
  {
  cloudiq.ip0=10;
  cloudiq.ip1=0;
  cloudiq.ip2=0;
  cloudiq.ip3=93;
  cloudiq.port=50000;
  cloudiq.rate_dividend=100;
  cloudiq.format=24;
  cloudiq.udp_size=0;
  cloudiq.dither=0;
  cloudiq.adgain=0;
  cloudiq.connect=0;
  cloudiq.clock_adjust=0;  
  cloudiq.check=CLOUDIQ_PAR_VERNR;
  }
restart:;  
cloudiq_setup_header(&line);
lir_text(0,line,"Press MENU button on the CloudIQ and set 'TCP/UDP Settings' to");
line++;
lir_text(0,line,"'Manual Both Same' with address and port as specified above");
line++;
lir_text(0,line,"or change address and port to agree with your CloudIQ");
line+=4;
lir_text(12,line,"A=Change IP address.");
line++;
lir_text(12,line,"B=Change port.");
line++;
lir_text(12,line,"C=Connect to the CloudIQ");
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
  cloudiq.ip0=b0;
  cloudiq.ip1=b1;
  cloudiq.ip2=b2;
  cloudiq.ip3=b3;  
  break;
  
  case 'B':
  lir_text(10,line,"New port number:");
  cloudiq.port=lir_get_integer(28,line,5,0,65535);
  break;
  
  case 'C':
  clear_screen();
  open_cloudiq();
  if(sdr == 0)goto verify_open;
  break;
  
  case 'X':
  return;
  }
if(kill_all_flag)return;  
goto restart;
verify_open:;
cloudiq_setup_header(&line);
msg[0]=0x04;
msg[1]=0x20;
msg[2]=0x01;
msg[3]=0x00;
net_write(cloudiq_fd,msg,4);
lir_sleep(100000);
recv(cloudiq_fd,textbuf,80,0);
sprintf(s,"Target Name = %s",&textbuf[4]);
lir_text(9,line,s);  
line++;
lir_refresh_screen();
msg[2]=0x02;
net_write(cloudiq_fd,msg,4);
lir_sleep(100000);
recv(cloudiq_fd,textbuf,80,0);
sprintf(s,"Serial number = %s",&textbuf[4]);
lir_text(7,line,s);  
line++;
lir_refresh_screen();
msg[2]=0x03;
net_write(cloudiq_fd,msg,4);
lir_sleep(100000);
recv(cloudiq_fd,textbuf,80,0);
t1=256*textbuf[5]+textbuf[4];
sprintf(s,"Interface version = %f",0.01*t1);
lir_text(3,line,s);  
line++;
lir_refresh_screen();
msg[2]=0x04;
net_write(cloudiq_fd,msg,4);
lir_sleep(100000);
recv(cloudiq_fd,textbuf,80,0);
t1=256*textbuf[5]+textbuf[4];
sprintf(s,"Boot code version = %f",0.01*t1);
lir_text(3,line,s);  
line++;
lir_refresh_screen();
msg[3]=0x01;
net_write(cloudiq_fd,msg,4);
lir_sleep(100000);
recv(cloudiq_fd,textbuf,80,0);
t1=256*textbuf[5]+textbuf[4];
sprintf(s,"Firmware version = %f",0.01*t1);
lir_text(4,line,s);  
line++;
lir_refresh_screen();
msg[3]=0x02;
net_write(cloudiq_fd,msg,4);
lir_sleep(100000);
recv(cloudiq_fd,textbuf,80,0);
t1=256*textbuf[5]+textbuf[4];
sprintf(s,"Hardware version = %f",0.01*t1);
lir_text(4,line,s);  
line++;
lir_refresh_screen();
msg[2]=0x09;
msg[3]=0x00;
net_write(cloudiq_fd,msg,4);
lir_sleep(100000);
recv(cloudiq_fd,textbuf,80,0);
t1=256*textbuf[5]+textbuf[4];
sprintf(s,"Product ID [%x][%x][%x][%x]",
                  textbuf[4],textbuf[5],textbuf[6],textbuf[7]);
lir_text(4,line,s);  
line+=2;
lir_text(10,line,"Use this hardware? (Y/N)");
lir_inkey=0;
while(lir_inkey != 'Y')
  {
  if(kill_all_flag)goto cloudiq_errexit;
  await_processed_keyboard();
  if(lir_inkey == 'N')goto restart;
  }
setpar:;  
cloudiq_setup_header(&line);
lir_text(0,line,"Set global parameters for the CloudIQ");
line++;
sprintf(s,"A => Rate dividend = %d (for a sampling rate of %.0f kHz)",
                        cloudiq.rate_dividend, 
                        0.00025*CLOUDIQ_SAMPLING_CLOCK/cloudiq.rate_dividend);
lir_text(0,line,s);
line++;
sprintf(s,"B => Bits per sample = %d",cloudiq.format);
lir_text(0,line,s);
line++;
sprintf(s,"C => UDP packet size = %s",pkgsize[cloudiq.udp_size]);
lir_text(0,line,s);
line++;
sprintf(s,"D => Dither %s",onoff[cloudiq.dither]);
lir_text(0,line,s);
line++;
sprintf(s,"E => A/D gain %s",adgains[cloudiq.adgain]);
lir_text(0,line,s);
line++;
sprintf(s,"F => Frequency correction = %d ppb",cloudiq.clock_adjust);
lir_text(0,line,s);
line++;
sprintf(s,"H => Input connection %s",inp_connect[cloudiq.connect]);
lir_text(0,line,s);
line++;
sprintf(s,"X => Save settings in %s and exit",cloudiq_parfil_name);
lir_text(0,line,s);
line++;
await_processed_keyboard();
if(kill_all_flag)goto cloudiq_errexit;
switch (lir_inkey)
  {
  case 'A':
  if(cloudiq.format == 16)
    {
    i=4;
    }
  else
    {
    i=6;
    }  
  sprintf(s,"Enter new rate dividend (%d to 250) =>",i);
  lir_text(0,line+1,s);
  cloudiq.rate_dividend=lir_get_integer(37,line+1,3,i,250);
  clear_lines(line+1,line+1);
  break;
  
  case 'B':
  if(cloudiq.format == 16)
    {
    cloudiq.format=24;
    }
  else
    {
    cloudiq.format=16;
    }  
  break;
  
  case 'C':
  cloudiq.udp_size^=1;
  break;
  
  case 'D':
  cloudiq.dither^=1;
  break;
  
  case 'E':
  cloudiq.adgain^=1;
  break;
  
  case 'F':
  sprintf(s,"Enter new frequency correction in ppb =>");
  lir_text(0,line+1,s);
  cloudiq.clock_adjust=lir_get_integer(strlen(s)+2,line+1,7,-200000,200000);
  clear_lines(line+1,line+1);
  break;
  
  case 'H':
  cloudiq.connect^=1;
  break;
  
  case 'X':
  adjusted_sdr_clock=1+0.000000001*cloudiq.clock_adjust;
  ui.rx_ad_speed=adjusted_sdr_clock*
                      0.25*CLOUDIQ_SAMPLING_CLOCK/cloudiq.rate_dividend;
  ui.rx_input_mode=IQ_DATA+DIGITAL_IQ;
  if(cloudiq.format == 24)ui.rx_input_mode+=DWORD_INPUT;
  ui.rx_rf_channels=1;
  ui.rx_ad_channels=2;
  ui.rx_admode=0;
  cloudiq.check=CLOUDIQ_PAR_VERNR;
  cloudiq_file=fopen(cloudiq_parfil_name,"w");
  if(cloudiq_file == NULL)
    {
cloudiq_errexit:;
    close_cloudiq();
    clear_screen();
    return;
    }
  sdr_pi=(int*)(&cloudiq);
  for(i=0; i<MAX_CLOUDIQ_PARM; i++)
    {
    fprintf(cloudiq_file,"%s [%d]\n",cloudiq_parm_text[i],sdr_pi[i]);
    }
  parfile_end(cloudiq_file);
  ui.rx_addev_no=CLOUDIQ_DEVICE_CODE;
  close_cloudiq();
  return;
  }
goto setpar;   
}

void set_cloudiq_att(void)
{
char msg[20], textbuf[80];
msg[0]=0x06;
msg[1]=0x00;
msg[2]=0x38;
msg[3]=0x00;
msg[4]=0x00;
msg[5]=fg.gain;
net_write(cloudiq_fd,msg,6);
lir_sleep(100000);
recv(cloudiq_fd,textbuf,80,0);
}

void set_cloudiq_frequency(void)
{
double dt1;
char msg[20], textbuf[80];
int i;
uint64_t frequency;
msg[0]=0x0a;
msg[1]=0x00;
msg[2]=0x20;
msg[3]=0x00;
msg[4]=0x00;
dt1=fg.passband_center*1000000+0.5;
if(dt1 < 0 || dt1 > CLOUDIQ_SAMPLING_CLOCK*adjusted_sdr_clock/2)
  {
  fg.passband_center=7.05;
  }
msg[9]=0x00;
frequency=rint(adjusted_sdr_clock*fg.passband_center*1000000);
fg_truncation_error=frequency-adjusted_sdr_clock*fg.passband_center*1000000;
for(i=0; i < 4;  i++)
  {
  msg[5+i]= (frequency >> (i*8)) & 0xFF;
  }
net_write(cloudiq_fd,msg,10);
lir_sleep(100000);
recv(cloudiq_fd,textbuf,80,0);
msg[4]=0x01;
net_write(cloudiq_fd,msg,10);
lir_sleep(100000);
recv(cloudiq_fd,textbuf,80,0);
}

void cloudiq_input(void)
{
struct sockaddr_in udp_addr;
FD udp_fd;
int i,errcod;
int rxin_local_workload_reset;
int pkg_size;
int timf1_netptr;
short int pkg_chk;
int cloudiq_mode;
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
clear_thread_times(THREAD_CLOUDIQ_INPUT);
#endif
local_att_counter=sdr_att_counter;
local_nco_counter=sdr_nco_counter;
errcod=0;
pkg_number=0;
dt1=current_time();
errcod=read_sdrpar(cloudiq_parfil_name, MAX_CLOUDIQ_PARM, 
                                     cloudiq_parm_text, (int*)((void*)&cloudiq));
if(errcod != 0)
  {
parerr:;  
  errcod=1170;
  goto cloudiq_init_error_exit;
  }
i=verify_cloudiq_parameters();
if(i == FALSE)goto parerr;
while(sdr == -1)
  {
  open_cloudiq();
  lir_sleep(30000);
  if(sdr == -1)
    {
    sprintf(s,"Waiting %.2f", current_time()-dt1);
    lir_text(0,screen_last_line,s);
    lir_refresh_screen();
    if(kill_all_flag)goto cloudiq_init_error_exit;
    }
  }
adjusted_sdr_clock=1+0.000000001*cloudiq.clock_adjust;
t1=ui.rx_ad_speed-adjusted_sdr_clock*
                     0.25*CLOUDIQ_SAMPLING_CLOCK/cloudiq.rate_dividend;
if(fabs(t1/ui.rx_ad_speed) > 0.0001)
  {
  errcod=1330;
  goto cloudiq_error_exit;
  }
// Set the DDC output sampling rate.
msg[0]=0x09;
msg[1]=0x00;
msg[2]=0xb8;
msg[3]=0x00;
msg[4]=0x00;
sr=(int*)&msg[5];  
sr[0]=0.25*CLOUDIQ_SAMPLING_CLOCK/cloudiq.rate_dividend;
net_write(cloudiq_fd,msg,9);
lir_sleep(100000);
recv(cloudiq_fd,textbuf,80,0);
// Set the RF filter mode.
msg[0]=0x06;
msg[1]=0x00;
msg[2]=0x44;
msg[3]=0x00;
msg[4]=0x00;
if(cloudiq.rf_filter == 0)
  {
  msg[5]=0x00;
  }
else
  {
  msg[5]=0x0b;
  }    
net_write(cloudiq_fd,msg,6);
lir_sleep(100000);
recv(cloudiq_fd,textbuf,80,0);
// Set the A/D mode.
msg[0]=0x06;
msg[1]=0x00;
msg[2]=0x8a;
msg[3]=0x00;
msg[4]=0x00;
if(cloudiq.dither != 0)msg[4]|=1;
if(cloudiq.adgain != 0)msg[4]|=2;
if(cloudiq.connect != 0)msg[4]|=4;
msg[5]=0x01;
net_write(cloudiq_fd,msg,6);
lir_sleep(100000);
recv(cloudiq_fd,textbuf,80,0);
// Set the data output packet size.
msg[0]=0x05;
msg[1]=0x00;
msg[2]=0xc4;
msg[3]=0x00;
msg[4]=cloudiq.udp_size;
cloudiq_mode=cloudiq.udp_size;
net_write(cloudiq_fd,msg,5);
lir_sleep(100000);
recv(cloudiq_fd,textbuf,80,0);
set_cloudiq_att();
set_cloudiq_frequency();
fft1_block_timing();
if(thread_command_flag[THREAD_SCREEN]!=THRFLAG_NOT_ACTIVE)
  {
  while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
    {
    if(thread_command_flag[THREAD_CLOUDIQ_INPUT] ==
                                           THRFLAG_KILL)goto cloudiq_error_exit;
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
if(cloudiq.format == 16)
  {
  msg[6]=0x00;
  if( (ui.rx_input_mode&DWORD_INPUT) != 0)lirerr(1453);
  }
else
  {
  cloudiq_mode|=2;
  msg[6]=0x80;  
  if( (ui.rx_input_mode&DWORD_INPUT) == 0)lirerr(1453);
  }
msg[7]=1;
net_write(cloudiq_fd,msg,8);
lir_sleep(100000);
recv(cloudiq_fd,textbuf,80,0);
if ((udp_fd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) == INVSOCK) 
  {
  lirerr(1284);
  goto cloudiq_exit;
  }
optval=TRUE;
err=setsockopt( udp_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval) );
if(err != 0)
  {
  lirerr(1392);
  goto cloudiq_exit;
  }
#if OSNUM == OSNUM_LINUX
timeout.tv_sec=1;
timeout.tv_usec=0;
err=setsockopt( udp_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout) );
if(err != 0)
  {
  lirerr(1392);
  goto cloudiq_exit;
  }
#endif
i=131072;
err=setsockopt(udp_fd,SOL_SOCKET,SO_RCVBUF,(char*)&i,sizeof(int));
if(err != 0)
  {
  lirerr(1392);
  goto cloudiq_exit;
  }
i=8192;
err=setsockopt(udp_fd,SOL_SOCKET,SO_SNDBUF,(char*)&i,sizeof(int));
if(err != 0)
  {
  lirerr(1392);
  goto cloudiq_exit;
  }
memset(&udp_addr, 0, sizeof(udp_addr)); 
udp_addr.sin_family=AF_INET;
udp_addr.sin_port=htons(cloudiq.port);
udp_addr.sin_addr.s_addr = htonl(INADDR_ANY );
// bind to the CloudIQ address and port.
if (bind(udp_fd, (struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0) 
  {
  lirerr(1302);
  goto cloudiq_udp_exit;
  }
tmpbuf=malloc(1444);
if(tmpbuf == NULL)goto cloudiq_udp_exit;
// cloudiq_mode == 0 => large UDP, 16 bit
// cloudiq_mode == 1 => small UDP, 16 bit
// cloudiq_mode == 2 => large UDP, 24 bit
// cloudiq_mode == 3 => small UDP, 24 bit
timf1_netptr=timf1p_pa;
switch (cloudiq_mode)
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
  goto cloudiq_udpbuf_exit;
  }
#include "timing_setup.c"
lir_status=LIR_OK;
thread_status_flag[THREAD_CLOUDIQ_INPUT]=THRFLAG_ACTIVE;
while(thread_command_flag[THREAD_CLOUDIQ_INPUT] == THRFLAG_ACTIVE)
  {
  if(local_att_counter != sdr_att_counter)
    {
    sys_func(THRFLAG_SET_CLOUDIQ_ATT);
    local_att_counter = sdr_att_counter;
    }
  if(local_nco_counter != sdr_nco_counter)
    {
    sys_func(THRFLAG_SET_CLOUDIQ_FREQUENCY);
    local_nco_counter = sdr_nco_counter;
    }
  ichk=(short int*)tmpbuf;
recv_again:;
  i=recv(udp_fd, tmpbuf, pkg_size, 0);
  if(kill_all_flag)goto cloudiq_udpbuf_exit;
  if(i < 0) goto recv_again;
  if(ichk[0] != pkg_chk)goto recv_again;
// In case the package number is too low, a package has been re-sent.
// This happens on my LAN when a slow computer is connected to the
// Afedri through several switches.
// If the package is not very old, assume it has already been properly
// processed and ignore the resent package.
// The code is now implemented for CloudIQ even though I have never
// seen any resending from this hardware.
#define MAX_RESEND 25
  if( pkg_number-ichk[1] > 0 && pkg_number-ichk[1] < MAX_RESEND)goto recv_again;  
// In case the packet number is totally wrong, just
// set pkg_number to the current package number.
  if(abs(ichk[1]-pkg_number) >= MAX_RESEND)pkg_number=ichk[1];
// Fill zeroes in case the packet number does not agree
// with pkg_number and increase pkg_number until we
// reach agreement. Then fill the current data from the buffer.
  if(cloudiq.format == 16)
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
  if(kill_all_flag) goto cloudiq_udpbuf_exit;
  }
cloudiq_udpbuf_exit:;
free(tmpbuf);  
cloudiq_udp_exit:;
CLOSE_FD(udp_fd);
cloudiq_exit:;
// Set the receiver state.
msg[0]=0x08;
msg[1]=0x00;
msg[2]=0x18;
msg[3]=0x00;
msg[4]=0x80;        // Set complex contigous data.
msg[5]=0x01;        // set run/stop to STOP
if(cloudiq.format == 16)
  {
  msg[6]=0x00;
  }
else
  {
  msg[6]=0x80;  
  }
msg[7]=0x01;  
net_write(cloudiq_fd,msg,8);
lir_sleep(100000);
recv(cloudiq_fd,textbuf,80,0);
cloudiq_error_exit:;
close_cloudiq();
cloudiq_init_error_exit:;
if(errcod != 0)lirerr(errcod);
thread_status_flag[THREAD_CLOUDIQ_INPUT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_CLOUDIQ_INPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}
