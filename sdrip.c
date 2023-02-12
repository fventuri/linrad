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

// Structure for SDR-IP parameters
typedef struct {
int ip10;
int ip11;
int ip12;
int ip13;
int ip20;
int ip21;
int ip22;
int ip23;
int port1;
int port2;
int rate_dividend;
int format;
int udp_size;
int dither;
int adgain;
int rf_filter;
int connect;
int clock_adjust;
int check;
}P_SDRIP;
#define MAX_SDRIP_PARM 19

P_SDRIP sdrip;

char *sdrip_parfil_name="par_sdrip";
char *sdrip_parm_text[MAX_SDRIP_PARM]={"IP1_1",
                                       "IP1_2",
                                       "IP1_3",
                                       "IP1_4",
                                       "IP2_1",
                                       "IP2_2",
                                       "IP2_3",
                                       "IP2_4",
                                       "Port1",
                                       "Port2",
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
FD sdrip1_fd;
struct sockaddr_in sdrip1_addr;
FD sdrip2_fd;
struct sockaddr_in sdrip2_addr;

int verify_sdrip_parameters(void)
{
float t1;
if(sdrip.ip10 < 0 || sdrip.ip10 > 255 ||
   sdrip.ip11 < 0 || sdrip.ip11 > 255 ||
   sdrip.ip12 < 0 || sdrip.ip12 > 255 ||
   sdrip.ip13 < 0 || sdrip.ip13 > 255 ||
   sdrip.port1 < 0 || sdrip.port1 > 65535 ||
   sdrip.udp_size < 0 || sdrip.udp_size > 1 ||
   sdrip.dither < 0 || sdrip.dither > 1 ||
   sdrip.adgain < 0 || sdrip.adgain > 1 ||
   sdrip.rf_filter < 0 || sdrip.rf_filter > 1 ||
   sdrip.connect < 0 || sdrip.connect > 1 ||
   abs(sdrip.clock_adjust) > 200000 ||
   sdrip.check != SDRIP_PAR_VERNR)
  {
  return FALSE;
  }
if(sdrip.port2 >= 0 && sdrip.port2 <= 65535)
  {
  if(sdrip.ip20 < 0 || sdrip.ip20 > 255 ||
     sdrip.ip21 < 0 || sdrip.ip21 > 255 ||
     sdrip.ip22 < 0 || sdrip.ip22 > 255 ||
     sdrip.ip23 < 0 || sdrip.ip23 > 255)
    {
    return FALSE;
    }
  }
if(sdrip.format != 16 && sdrip.format != 24)return FALSE;
t1=0.1*SDRIP_SAMPLING_CLOCK/sdrip.rate_dividend;
if(t1 < 32000)return FALSE;
if(sdrip.format == 16)
  {
  if(t1 > 2000001.)return FALSE;
  }
else
  {
  if(t1 > 1333334.0)return FALSE;
  }
return TRUE;
}


int display_sdrip_parm_info(int *line)
{
char s[80];
int errcod;
errcod=read_sdrpar(sdrip_parfil_name, MAX_SDRIP_PARM, 
                                     sdrip_parm_text, (int*)((void*)&sdrip));
if(errcod == 0)
  {
  settextcolor(7);
  sprintf(s,"IP address         = %d.%d.%d.%d", 
                          sdrip.ip10,sdrip.ip11,sdrip.ip12,sdrip.ip13);
  if(sdrip.port2 != -1)sprintf(&s[strlen(s)],"      %d.%d.%d.%d", 
                            sdrip.ip20,sdrip.ip21,sdrip.ip22,sdrip.ip23);
  lir_text(24,line[0],s);
  line[0]++;
  SNDLOG"\n%s",s);
  sprintf(s,"Port               = %d",sdrip.port1);
  if(sdrip.port2 != -1)sprintf(&s[strlen(s)],"          %d",sdrip.port2);     
  lir_text(24,line[0],s);
  line[0]++;
  sprintf(s,"Sampling speed     = %.0f kHz   (dividend=%d)  adjust %d ppb",
          8000.0/sdrip.rate_dividend,sdrip.rate_dividend,sdrip.clock_adjust);
  lir_text(24,line[0],s);
  line[0]++;
  sprintf(s,"Bits per sample    = %d",sdrip.format);
  lir_text(24,line[0],s);
  line[0]++;
  sprintf(s,"UDP packet size    = %s",pkgsize[sdrip.udp_size]);
  lir_text(24,line[0],s);
  line[0]++;
  sprintf(s,"Dither             = %s",onoff[sdrip.dither]);
  lir_text(24,line[0],s);
  line[0]++;
  sprintf(s,"A/D gain           = %s",adgains[sdrip.adgain]);
  lir_text(24,line[0],s);
  line[0]++;
  sprintf(s,"RF filter          = %s",rffil[sdrip.rf_filter]);
  lir_text(24,line[0],s);
  line[0]++;
  sprintf(s,"Input connection   = %s",inp_connect[sdrip.connect]);
  lir_text(24,line[0],s);
  }
return (errcod);
}


void open_sdrip1(void)
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
memset(&sdrip1_addr, 0, sizeof(sdrip1_addr)); 
sdrip1_addr.sin_family=AF_INET;
sdrip1_addr.sin_port=htons(sdrip.port1);
ip=(unsigned char*)&sdrip1_addr.sin_addr.s_addr;
ip[0]=sdrip.ip10;
ip[1]=sdrip.ip11;
ip[2]=sdrip.ip12;
ip[3]=sdrip.ip13;
if ((sdrip1_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVSOCK) 
  {
  lirerr(1249);
  return;
  }
j=connect(sdrip1_fd, (struct sockaddr *)&sdrip1_addr, sizeof(sdrip1_addr));
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
  j=connect(sdrip1_fd, (struct sockaddr *)&sdrip1_addr, sizeof(sdrip1_addr));
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

void open_sdrip2(void)
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
memset(&sdrip2_addr, 0, sizeof(sdrip2_addr)); 
sdrip2_addr.sin_family=AF_INET;
sdrip2_addr.sin_port=htons(sdrip.port2);
ip=(unsigned char*)&sdrip2_addr.sin_addr.s_addr;
ip[0]=sdrip.ip20;
ip[1]=sdrip.ip21;
ip[2]=sdrip.ip22;
ip[3]=sdrip.ip23;
if ((sdrip2_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVSOCK) 
  {
  lirerr(1249);
  return;
  }
j=connect(sdrip2_fd, (struct sockaddr *)&sdrip2_addr, sizeof(sdrip2_addr));
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
  j=connect(sdrip2_fd, (struct sockaddr *)&sdrip2_addr, sizeof(sdrip2_addr));
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

void sdrip_setup_header(int *line)
{
char s[80];
clear_screen();
line[0]=2;
settextcolor(13);
lir_text(20,line[0],"Installation routine for SDR-IP");
settextcolor(7);
line[0]+=2;
sprintf(s,"IP address = %d.%d.%d.%d",sdrip.ip10,sdrip.ip11,sdrip.ip12,sdrip.ip13);
lir_text(10,line[0],s);
line[0]++;
sprintf(s,"Port = %d",sdrip.port1);
lir_text(16,line[0],s);
line[0]+=2;
lir_refresh_screen();
}

void sdrip_setup_header2(int *line)
{
char s[80];
clear_screen();
line[0]=3;
sprintf(s,"Second IP address = %d.%d.%d.%d",sdrip.ip20,sdrip.ip21,sdrip.ip22,sdrip.ip23);
lir_text(10,line[0],s);
line[0]++;
if(sdrip.port2 == -1)sdrip.port2=50001;
sprintf(s,"Second port = %d",sdrip.port2);
lir_text(16,line[0],s);
line[0]+=2;
lir_refresh_screen();
}

void close_sdrip(void)
{
if(sdrip1_fd != INVSOCK)CLOSE_FD(sdrip1_fd);
if(sdrip2_fd != INVSOCK)CLOSE_FD(sdrip2_fd);
sdr=-1;
sdrip1_fd=INVSOCK;
sdrip2_fd=INVSOCK;
#if(OSNUM == OSNUM_WINDOWS)
WSACleanup();
#endif    
}

void show_sdrip_info(FD fd, int *line)
{
char msg[20], s[128], textbuf[80];
float t1;
msg[0]=0x04;
msg[1]=0x20;
msg[2]=0x01;
msg[3]=0x00;
net_write(fd,msg,4);
lir_sleep(100000);
recv(fd,textbuf,80,0);
sprintf(s,"Target Name = %s",&textbuf[4]);
lir_text(9,line[0],s);  
line[0]++;
lir_refresh_screen();
msg[2]=0x02;
net_write(fd,msg,4);
lir_sleep(100000);
recv(fd,textbuf,80,0);
sprintf(s,"Serial number = %s",&textbuf[4]);
lir_text(7,line[0],s);  
line[0]++;
lir_refresh_screen();
msg[2]=0x03;
net_write(fd,msg,4);
lir_sleep(100000);
recv(fd,textbuf,80,0);
t1=256*textbuf[5]+textbuf[4];
sprintf(s,"Interface version = %f",0.01*t1);
lir_text(3,line[0],s);  
line[0]++;
lir_refresh_screen();
msg[2]=0x04;
net_write(fd,msg,4);
lir_sleep(100000);
recv(fd,textbuf,80,0);
t1=256*textbuf[5]+textbuf[4];
sprintf(s,"Boot code version = %f",0.01*t1);
lir_text(3,line[0],s);  
line[0]++;
lir_refresh_screen();
msg[3]=0x01;
net_write(fd,msg,4);
lir_sleep(100000);
recv(fd,textbuf,80,0);
t1=256*textbuf[5]+textbuf[4];
sprintf(s,"Firmware version = %f",0.01*t1);
lir_text(4,line[0],s);  
line[0]++;
lir_refresh_screen();
msg[3]=0x02;
net_write(fd,msg,4);
lir_sleep(100000);
recv(fd,textbuf,80,0);
t1=256*textbuf[5]+textbuf[4];
sprintf(s,"Hardware version = %f",0.01*t1);
lir_text(4,line[0],s);  
line[0]++;
lir_refresh_screen();
msg[2]=0x09;
msg[3]=0x00;
net_write(fd,msg,4);
lir_sleep(100000);
recv(fd,textbuf,80,0);
t1=256*textbuf[5]+textbuf[4];
sprintf(s,"Product ID [%x][%x][%x][%x]",
                  textbuf[4],textbuf[5],textbuf[6],textbuf[7]);
lir_text(4,line[0],s);  
line[0]+=2;
}

void init_sdrip(void)
{
int i, errcod, line, length;
char s[80], textbuf[80];
unsigned b0, b1, b2, b3;
unsigned char c;
FILE *sdrip_file;
int *sdr_pi;
sdrip1_fd=INVSOCK;
sdrip2_fd=INVSOCK;
errcod=read_sdrpar(sdrip_parfil_name, MAX_SDRIP_PARM, 
                                     sdrip_parm_text, (int*)((void*)&sdrip));
if(errcod != 0)
  {
  sdrip.ip10=10;
  sdrip.ip11=0;
  sdrip.ip12=0;
  sdrip.ip13=93;
  sdrip.ip20=10;
  sdrip.ip21=0;
  sdrip.ip22=0;
  sdrip.ip23=94;
  sdrip.port1=50000;
  sdrip.port2=-1;
  sdrip.rate_dividend=100;
  sdrip.format=24;
  sdrip.udp_size=0;
  sdrip.dither=0;
  sdrip.adgain=0;
  sdrip.connect=0;
  sdrip.clock_adjust=0;  
  sdrip.check=SDRIP_PAR_VERNR;
  }
restart:;  
sdrip_setup_header(&line);
lir_text(0,line,"Press MENU button on the SDR-IP and set 'TCP/UDP Settings' to");
line++;
lir_text(0,line,"'Manual Both Same' with address and port as specified above");
line++;
lir_text(0,line,"or change address and port to agree with your SDR-IP");
line+=4;
lir_text(12,line,"A=Change IP address.");
line++;
lir_text(12,line,"B=Change port.");
line++;
lir_text(12,line,"C=Connect to the SDR-IP");
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
  sdrip.ip10=b0;
  sdrip.ip11=b1;
  sdrip.ip12=b2;
  sdrip.ip13=b3;  
  break;
  
  case 'B':
  lir_text(10,line,"New port number:");
  sdrip.port1=lir_get_integer(28,line,5,0,65535);
  break;
  
  case 'C':
  clear_screen();
  open_sdrip1();
  if(sdr == 0)goto verify_open;
  break;
  
  case 'X':
  return;
  }
if(kill_all_flag)return;  
goto restart;
verify_open:;
sdrip_setup_header(&line);
show_sdrip_info(sdrip1_fd, &line);
lir_text(10,line,"Use this hardware? (Y/N)");
lir_inkey=0;
while(lir_inkey != 'Y')
  {
  if(kill_all_flag)goto sdrip_errexit;
  await_processed_keyboard();
  if(lir_inkey == 'N')goto restart;
  }
setpar:;  
sdrip_setup_header(&line);
lir_text(0,line,"Set global parameters for the SDR-IP");
line++;
sprintf(s,"A => Rate dividend = %d (for a sampling rate of %.0f kHz)",
                             sdrip.rate_dividend, 8000.0/sdrip.rate_dividend);
lir_text(0,line,s);
line++;
sprintf(s,"B => Bits per sample = %d",sdrip.format);
lir_text(0,line,s);
line++;
sprintf(s,"C => UDP packet size = %s",pkgsize[sdrip.udp_size]);
lir_text(0,line,s);
line++;
sprintf(s,"D => Dither %s",onoff[sdrip.dither]);
lir_text(0,line,s);
line++;
sprintf(s,"E => A/D gain %s",adgains[sdrip.adgain]);
lir_text(0,line,s);
line++;
sprintf(s,"F => Frequency correction = %d ppb",sdrip.clock_adjust);
lir_text(0,line,s);
line++;
sprintf(s,"H => Input connection %s",inp_connect[sdrip.connect]);
lir_text(0,line,s);
line++;
sprintf(s,"X => Save settings in %s and exit",sdrip_parfil_name);
lir_text(0,line,s);
line++;
await_processed_keyboard();
if(kill_all_flag)goto sdrip_errexit;
switch (lir_inkey)
  {
  case 'A':
  if(sdrip.format == 16)
    {
    i=4;
    }
  else
    {
    i=6;
    }  
  sprintf(s,"Enter new rate dividend (%d to 250) =>",i);
  lir_text(0,line+1,s);
  sdrip.rate_dividend=lir_get_integer(37,line+1,3,i,250);
  clear_lines(line+1,line+1);
  break;
  
  case 'B':
  if(sdrip.format == 16)
    {
    sdrip.format=24;
    }
  else
    {
    sdrip.format=16;
    }  
  break;
  
  case 'C':
  sdrip.udp_size^=1;
  break;
  
  case 'D':
  sdrip.dither^=1;
  break;
  
  case 'E':
  sdrip.adgain^=1;
  break;
  
  case 'F':
  sprintf(s,"Enter new frequency correction in ppb =>");
  lir_text(0,line+1,s);
  sdrip.clock_adjust=lir_get_integer(strlen(s)+2,line+1,7,-200000,200000);
  clear_lines(line+1,line+1);
  break;
  
  case 'H':
  sdrip.connect^=1;
  break;
  
  case 'X':
  adjusted_sdr_clock=1+0.000000001*sdrip.clock_adjust;
  ui.rx_ad_speed=adjusted_sdr_clock*
                            0.1*SDRIP_SAMPLING_CLOCK/sdrip.rate_dividend;
  ui.rx_input_mode=IQ_DATA+DIGITAL_IQ;
  if(sdrip.format == 24)ui.rx_input_mode+=DWORD_INPUT;
  ui.rx_rf_channels=1;
  ui.rx_ad_channels=2;
  ui.rx_admode=0;
  sdrip.check=SDRIP_PAR_VERNR;
  sdrip_file=fopen(sdrip_parfil_name,"w");
  if(sdrip_file == NULL)
    {
sdrip_errexit:;
    close_sdrip();
    clear_screen();
    return;
    }
  clear_screen();
  lir_text(5,5,"Do you want to use a second SDR-IP to get two RF channels?");
  lir_text(5,7,"(Y/N)=>");
use_2nd:;
  await_processed_keyboard();
  if(kill_all_flag)goto init_x;
  if(lir_inkey == 'N')
    {
    sdrip.port2=-1;
    goto init_y;
    }
  if(lir_inkey != 'Y')goto use_2nd;
select_2nd:;
  clear_screen();
  sdrip_setup_header2(&line);
  lir_text(12,line,"A=Change second IP address.");
  line++;
  lir_text(12,line,"B=Change second port.");
  line++;
  lir_text(12,line,"C=Connect to the second SDR-IP");
  line++;
  lir_text(12,line,"X=Skip the second SDR-IP.");
  line+=2;
  await_processed_keyboard();
  if(kill_all_flag)goto init_x;
  switch (lir_inkey)
    {
    case 'A':
specify2:
    lir_text(10,line,"Specify new second IP address:");
    length=lir_get_text(40, line, textbuf);
    if(kill_all_flag) return;
// validate format of IP address 
    if ((length < 7) || (length > 15))  goto format_error;
    if (strspn(textbuf, "0123456789.") < strlen(textbuf)) goto format_error;
    if (sscanf(textbuf, "%3u.%3u.%3u.%3u%c", &b0, &b1, &b2, &b3, &c) != 4)
                                                        goto format_error2;
    if ((b0 | b1 | b2 | b3) > 255)
      {
format_error2:;
      lir_text(10,line+1,"IP address format-error: Try again ");
      goto specify2; 
      }
    sdrip.ip20=b0;
    sdrip.ip21=b1;
    sdrip.ip22=b2;
    sdrip.ip23=b3;  
    break;
  
    case 'B':
    lir_text(10,line,"New port number:");
    sdrip.port2=lir_get_integer(28,line,5,0,65535);
    break;
  
    case 'C':
    clear_screen();
    open_sdrip2();
    if(sdr == 0)goto verify_open2;
    break;
  
    case 'X':
    sdrip.port2=-1;
    goto init_y;
    }
  goto select_2nd;
verify_open2:;
  sdrip_setup_header2(&line);
  show_sdrip_info(sdrip2_fd, &line);
  lir_text(10,line,"Y=Use this hardware.");
  line++;
  lir_text(10,line,"N=Use just one SDR-IP unit.");
  lir_inkey=0;
  while(lir_inkey != 'Y')
    {
    if(kill_all_flag)goto sdrip_errexit;
    await_processed_keyboard();
    if(lir_inkey == 'N')
      {
      sdrip.port2=-1;
      goto init_y;
      }
    }
  ui.rx_rf_channels=2;
  ui.rx_ad_channels=4;
  ui.rx_input_mode|=TWO_CHANNELS;
init_y:;
  sdr_pi=(int*)(&sdrip);
  for(i=0; i<MAX_SDRIP_PARM; i++)
    {
    fprintf(sdrip_file,"%s [%d]\n",sdrip_parm_text[i],sdr_pi[i]);
    }
  parfile_end(sdrip_file);
  ui.rx_addev_no=SDRIP_DEVICE_CODE;
init_x:;  
  close_sdrip();
  return;
  }
goto setpar;   
}

void set_sdrip_att(void)
{
char msg[20], textbuf[80];
msg[0]=0x06;
msg[1]=0x00;
msg[2]=0x38;
msg[3]=0x00;
msg[4]=0x00;
msg[5]=fg.gain;
net_write(sdrip1_fd,msg,6);
if(sdrip.port2 != -1)net_write(sdrip2_fd,msg,6);
lir_sleep(100000);
recv(sdrip1_fd,textbuf,80,0);
if(sdrip.port2 != -1)recv(sdrip2_fd,textbuf,80,0);
}

void set_sdrip_frequency(void)
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
if(dt1 < 0 || dt1 > SDRIP_SAMPLING_CLOCK*adjusted_sdr_clock/2)
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
net_write(sdrip1_fd,msg,10);
if(sdrip.port2 != -1)net_write(sdrip2_fd,msg,10);
lir_sleep(100000);
recv(sdrip1_fd,textbuf,80,0);
if(sdrip.port2 != -1)recv(sdrip2_fd,textbuf,80,0);
msg[4]=0x01;
net_write(sdrip1_fd,msg,10);
if(sdrip.port2 != -1)net_write(sdrip2_fd,msg,10);
lir_sleep(100000);
recv(sdrip1_fd,textbuf,80,0);
if(sdrip.port2 != -1)recv(sdrip2_fd,textbuf,80,0);
}

void sdrip_input(void)
{
struct sockaddr_in udp_addr;
FD udp1_fd, udp2_fd;
int i,errcod;
int rxin_local_workload_reset;
int pkg_size;
int timf1_netptr;
short int pkg_chk;
int sdrip_mode;
int *sr;
char *tmpbuf1;
char *tmpbuf2;
char s[80], msg[20], textbuf[80];
double dt1, read_start_time, total_reads;
short int *ichk1, *ichk2;
int timing_loop_counter,timing_loop_counter_max,initial_skip_flag;
float t1;
int local_att_counter;
int local_nco_counter;
short int pkg1_number, pkg2_number;
int err;
#if(OSNUM == OSNUM_WINDOWS)
char optval;
#endif
#if(OSNUM == OSNUM_LINUX)
int optval;
struct timeval timeout;
clear_thread_times(THREAD_SDRIP_INPUT);
#endif
sdrip1_fd=INVSOCK;
sdrip2_fd=INVSOCK;
udp2_fd=INVSOCK;
local_att_counter=sdr_att_counter;
local_nco_counter=sdr_nco_counter;
errcod=0;
pkg1_number=0;
pkg2_number=0;
dt1=current_time();
errcod=read_sdrpar(sdrip_parfil_name, MAX_SDRIP_PARM, 
                                     sdrip_parm_text, (int*)((void*)&sdrip));
if(errcod != 0)
  {
parerr:;  
  errcod=1170;
  goto sdrip_init_error_exit;
  }
i=verify_sdrip_parameters();
if(i == FALSE)goto parerr;
while(sdr == -1)
  {
  
  open_sdrip1();
  lir_sleep(30000);
  if(sdr == -1)
    {
    sprintf(s,"Waiting %.2f", current_time()-dt1);
    lir_text(0,screen_last_line,s);
    lir_refresh_screen();
    if(kill_all_flag)goto sdrip_init_error_exit;
    }
  }
if(sdrip.port2 != -1)
  {
  sdr=-1;
  while(sdr == -1)
    {
    open_sdrip2();
    lir_sleep(30000);
    if(sdr == -1)
      {
      sprintf(s,"Waiting %.2f", current_time()-dt1);
      lir_text(0,screen_last_line,s);
      lir_refresh_screen();
      if(kill_all_flag)goto sdrip_error_exit;
      }
    }
  }
adjusted_sdr_clock=1+0.000000001*sdrip.clock_adjust;
t1=ui.rx_ad_speed-adjusted_sdr_clock*
                        0.1*SDRIP_SAMPLING_CLOCK/sdrip.rate_dividend;
if(fabs(t1/ui.rx_ad_speed) > 0.0001)
  {
  errcod=1330;
  goto sdrip_error_exit;
  }
// Set the DDC output sampling rate.
msg[0]=0x09;
msg[1]=0x00;
msg[2]=0xb8;
msg[3]=0x00;
msg[4]=0x00;
sr=(int*)&msg[5];  
sr[0]=0.1*SDRIP_SAMPLING_CLOCK/sdrip.rate_dividend;
net_write(sdrip1_fd,msg,9);
if(sdrip.port2 != -1)net_write(sdrip2_fd,msg,9);
lir_sleep(100000);
recv(sdrip1_fd,textbuf,80,0);
if(sdrip.port2 != -1)recv(sdrip2_fd,textbuf,80,0);
// Set the RF filter mode.
msg[0]=0x06;
msg[1]=0x00;
msg[2]=0x44;
msg[3]=0x00;
msg[4]=0x00;
if(sdrip.rf_filter == 0)
  {
  msg[5]=0x00;
  }
else
  {
  msg[5]=0x0b;
  }    
net_write(sdrip1_fd,msg,6);
if(sdrip.port2 != -1)net_write(sdrip2_fd,msg,6);
lir_sleep(100000);
recv(sdrip1_fd,textbuf,80,0);
if(sdrip.port2 != -1)recv(sdrip2_fd,textbuf,80,0);
// Set the A/D mode.
msg[0]=0x06;
msg[1]=0x00;
msg[2]=0x8a;
msg[3]=0x00;
msg[4]=0x00;
if(sdrip.dither != 0)msg[4]|=1;
if(sdrip.adgain != 0)msg[4]|=2;
if(sdrip.connect != 0)msg[4]|=4;
msg[5]=0x01;
net_write(sdrip1_fd,msg,6);
if(sdrip.port2 != -1)net_write(sdrip2_fd,msg,6);
lir_sleep(100000);
recv(sdrip1_fd,textbuf,80,0);
if(sdrip.port2 != -1)recv(sdrip2_fd,textbuf,80,0);
// Set the data output packet size.
msg[0]=0x05;
msg[1]=0x00;
msg[2]=0xc4;
msg[3]=0x00;
msg[4]=sdrip.udp_size;
sdrip_mode=sdrip.udp_size;
net_write(sdrip1_fd,msg,5);
if(sdrip.port2 != -1)net_write(sdrip2_fd,msg,5);
lir_sleep(100000);
recv(sdrip1_fd,textbuf,80,0);
if(sdrip.port2 != -1)recv(sdrip2_fd,textbuf,80,0);
set_sdrip_att();
set_sdrip_frequency();
fft1_block_timing();
if(thread_command_flag[THREAD_SCREEN]!=THRFLAG_NOT_ACTIVE)
  {
  while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
    {
    if(thread_command_flag[THREAD_SDRIP_INPUT] ==
                                           THRFLAG_KILL)goto sdrip_error_exit;
    lir_sleep(10000);
    }
  }
// Set the input sync mode (Trigger mode) to 3
// Capture starts non high to low transition.
if(sdrip.port2 != -1)
  {
  msg[0]=0x08;
  msg[1]=0x00;
  msg[2]=0xb4;
  msg[3]=0x00;
  msg[4]=0x00;        // Set complex contigous data.
  msg[5]=0x03;        // set run/stop to RUN
  msg[6]=0xe8;
  msg[7]=0x03;
  net_write(sdrip1_fd,msg,8);
  net_write(sdrip2_fd,msg,8);
  lir_sleep(100000);
  }
// Set the receiver state.
msg[0]=0x08;
msg[1]=0x00;
msg[2]=0x18;
msg[3]=0x00;
msg[4]=0x80;        // Set complex contigous data.
msg[5]=0x02;        // set run/stop to RUN
if(sdrip.format == 16)
  {
  msg[6]=0x00;
  if( (ui.rx_input_mode&DWORD_INPUT) != 0)lirerr(1451);
  }
else
  {
  sdrip_mode|=2;
  msg[6]=0x80;  
  if( (ui.rx_input_mode&DWORD_INPUT) == 0)lirerr(1451);
  }
if(sdrip.port2 != -1)msg[6]|=0x3;
msg[7]=1;
net_write(sdrip1_fd,msg,8);
if(sdrip.port2 != -1)net_write(sdrip2_fd,msg,8);
lir_sleep(100000);
recv(sdrip1_fd,textbuf,80,0);
if(sdrip.port2 != -1)recv(sdrip2_fd,textbuf,80,0);
udp1_fd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
if (udp1_fd == INVSOCK) 
  {
  lirerr(1284);
  goto sdrip_exit;
  }
optval=TRUE;
err=setsockopt( udp1_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval) );
if(err != 0)
  {
  lirerr(1392);
  goto sdrip_exit;
  }
#if OSNUM == OSNUM_LINUX
timeout.tv_sec=1;
timeout.tv_usec=0;
err=setsockopt( udp1_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout) );
if(err != 0)
  {
  lirerr(1392);
  goto sdrip_exit;
  }
#endif
i=131072;
err=setsockopt(udp1_fd,SOL_SOCKET,SO_RCVBUF,(char*)&i,sizeof(int));
if(err != 0)
  {
  lirerr(1392);
  goto sdrip_exit;
  }
i=8192;
err=setsockopt(udp1_fd,SOL_SOCKET,SO_SNDBUF,(char*)&i,sizeof(int));
if(err != 0)
  {
  lirerr(1392);
  goto sdrip_exit;
  }
memset(&udp_addr, 0, sizeof(udp_addr)); 
udp_addr.sin_family=AF_INET;
udp_addr.sin_port=htons(sdrip.port1);
udp_addr.sin_addr.s_addr = htonl(INADDR_ANY );
// bind to the SDR-IP address and port.
if (bind(udp1_fd, (struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0) 
  {
  lirerr(1302);
  goto sdrip_udp_exit;
  }
if(sdrip.port2 != -1)
  {
  udp2_fd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
  if (udp2_fd == INVSOCK) 
    {
    lirerr(1284);
    goto sdrip_exit;
    }
  optval=TRUE;
  err=setsockopt( udp2_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval) );
  if(err != 0)
    {
    lirerr(1392);
    goto sdrip_exit;
    }
#if OSNUM == OSNUM_LINUX
  timeout.tv_sec=1;
  timeout.tv_usec=0;
  err=setsockopt( udp2_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout) );
  if(err != 0)
    {
    lirerr(1392);
    goto sdrip_exit;
    }
#endif
  i=131072;
  err=setsockopt(udp2_fd,SOL_SOCKET,SO_RCVBUF,(char*)&i,sizeof(int));
  if(err != 0)
     {
    lirerr(1392);
    goto sdrip_exit;
    }
  i=8192;
  err=setsockopt(udp2_fd,SOL_SOCKET,SO_SNDBUF,(char*)&i,sizeof(int));
  if(err != 0)
    {
    lirerr(1392);
    goto sdrip_exit;
    }
  memset(&udp_addr, 0, sizeof(udp_addr)); 
  udp_addr.sin_family=AF_INET;
  udp_addr.sin_port=htons(sdrip.port2);
  udp_addr.sin_addr.s_addr = htonl(INADDR_ANY );
// bind to the SDR-IP address and port.
  if (bind(udp2_fd, (struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0) 
    {
    lirerr(1302);
    goto sdrip_udp_exit;
    }
  }
tmpbuf1=malloc(1444);
if(tmpbuf1 == NULL)goto sdrip_udp_exit;
tmpbuf2=malloc(1444);
if(tmpbuf2 == NULL)goto sdrip_udp_exit;
// sdrip_mode == 0 => large UDP, 16 bit
// sdrip_mode == 1 => small UDP, 16 bit
// sdrip_mode == 2 => large UDP, 24 bit
// sdrip_mode == 3 => small UDP, 24 bit
timf1_netptr=timf1p_pa;
switch (sdrip_mode)
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
  goto sdrip_udpbuf_exit;
  }
#include "timing_setup.c"
lir_status=LIR_OK;
ichk1=(short int*)tmpbuf1;
ichk2=(short int*)tmpbuf2;
thread_status_flag[THREAD_SDRIP_INPUT]=THRFLAG_ACTIVE;
while(thread_command_flag[THREAD_SDRIP_INPUT] == THRFLAG_ACTIVE)
  {
  if(local_att_counter != sdr_att_counter &&
     ! sys_func_async_start(THRFLAG_SET_SDRIP_ATT) &&
  	   sys_func_async_join(THRFLAG_SET_SDRIP_ATT))
    local_att_counter = sdr_att_counter;
  if (local_nco_counter != sdr_nco_counter &&
	! sys_func_async_start(THRFLAG_SET_SDRIP_FREQUENCY) &&
	sys_func_async_join(THRFLAG_SET_SDRIP_FREQUENCY))
	local_nco_counter = sdr_nco_counter;
recv1_again:;
  i=recv(udp1_fd, tmpbuf1, pkg_size, 0);
  if(kill_all_flag)goto sdrip_udpbuf_exit;
  if(i < 0) goto recv1_again;
  if(ichk1[0] != pkg_chk)goto recv1_again;
// In case the package number is too low, a package has been re-sent.
// This happens on my LAN when a slow computer is connected to the
// Afedri through several switches.
// If the package is not very old, assume it has already been properly
// processed and ignore the resent package.
// The code is now implemented for SDR-IP even though I have never
// seen any resending from this hardware.
#define MAX_RESEND 25
  if( pkg1_number!=ichk1[1] && pkg1_number==0)pkg1_number=1; 
  if( pkg1_number-ichk1[1] > 0 && pkg1_number-ichk1[1] < 
                                                MAX_RESEND)goto recv1_again;  
// In case the packet number is totally wrong, just
// set pkg_number to the current package number.
  if(abs(ichk1[1]-pkg1_number) >= MAX_RESEND)pkg1_number=ichk1[1];
  if(sdrip.port2 != -1)
    {
recv2_again:;
    i=recv(udp2_fd, tmpbuf2, pkg_size, 0);
    if(kill_all_flag)goto sdrip_udpbuf_exit;
    if(i < 0) goto recv2_again;
    if(ichk2[0] != pkg_chk)goto recv2_again;
// In case the package number is too low, a package has been re-sent.
// This happens on my LAN when a slow computer is connected to the
// Afedri through several switches.
// If the package is not very old, assume it has already been properly
// processed and ignore the resent package.
// The code is now implemented for SDR-IP even though I have never
// seen any resending from this hardware.
    if( pkg2_number!=ichk1[1] && pkg2_number==0)pkg2_number=1; 
    if( pkg2_number-ichk2[1] > 0 && pkg2_number-ichk2[1] < 
                                                   MAX_RESEND)goto recv2_again;  
// In case the packet number is totally wrong, just
// set pkg_number to the current package number.
    if(abs(ichk2[1]-pkg2_number) >= MAX_RESEND)pkg2_number=ichk2[1];
// Do not worry about errors at this time
    if(sdrip.format == 16)
      {
      for(i=4; i<pkg_size; i+=4)
        {
        timf1_char[timf1_netptr  ]=tmpbuf1[i  ];
        timf1_char[timf1_netptr+1]=tmpbuf1[i+1];
        timf1_char[timf1_netptr+2]=tmpbuf1[i+2];
        timf1_char[timf1_netptr+3]=tmpbuf1[i+3];
        timf1_char[timf1_netptr+4]=tmpbuf2[i  ];
        timf1_char[timf1_netptr+5]=tmpbuf2[i+1];
        timf1_char[timf1_netptr+6]=tmpbuf2[i+2];
        timf1_char[timf1_netptr+7]=tmpbuf2[i+3];
        timf1_netptr=(timf1_netptr+8)&timf1_bytemask;
        }
      pkg1_number++;
      pkg2_number++;
      }
    else
      {
      for(i=4; i<pkg_size; i+=6)
        {
        timf1_char[timf1_netptr   ]=0;
        timf1_char[timf1_netptr+ 1]=tmpbuf1[i  ];
        timf1_char[timf1_netptr+ 2]=tmpbuf1[i+1];
        timf1_char[timf1_netptr+ 3]=tmpbuf1[i+2];
        timf1_char[timf1_netptr+ 4]=0;
        timf1_char[timf1_netptr+ 5]=tmpbuf1[i+3];
        timf1_char[timf1_netptr+ 6]=tmpbuf1[i+4];
        timf1_char[timf1_netptr+ 7]=tmpbuf1[i+5];
        timf1_char[timf1_netptr+ 8]=0;
        timf1_char[timf1_netptr+ 9]=tmpbuf2[i  ];
        timf1_char[timf1_netptr+10]=tmpbuf2[i+1];
        timf1_char[timf1_netptr+11]=tmpbuf2[i+2];
        timf1_char[timf1_netptr+12]=0;
        timf1_char[timf1_netptr+13]=tmpbuf2[i+3];
        timf1_char[timf1_netptr+14]=tmpbuf2[i+4];
        timf1_char[timf1_netptr+15]=tmpbuf2[i+5];
        timf1_netptr=(timf1_netptr+16)&timf1_bytemask;
        }
      pkg1_number++;
      pkg2_number++;
      }  
    }
  else
    {
// Fill zeroes in case the packet number does not agree
// with pkg_number and increase pkg_number until we
// reach agreement. Then fill the current data from the buffer.
    if(sdrip.format == 16)
      {
      while(pkg1_number != ichk1[1])
        {
        for(i=4; i<pkg_size; i++)
          {
          timf1_char[timf1_netptr]=0;
          timf1_netptr=(timf1_netptr+1)&timf1_bytemask;
          }
        pkg1_number++;
        }
      for(i=4; i<pkg_size; i++)
        {
        timf1_char[timf1_netptr]=tmpbuf1[i];
        timf1_netptr=(timf1_netptr+1)&timf1_bytemask;
        }
      pkg1_number++;
      }
    else
      {
      while(pkg1_number != ichk1[1])
        {
        for(i=4; i<pkg_size; i+=3)
          {
          timf1_char[timf1_netptr  ]=0;
          timf1_char[timf1_netptr+1]=0;
          timf1_char[timf1_netptr+2]=0;
          timf1_char[timf1_netptr+3]=0;
          timf1_netptr=(timf1_netptr+4)&timf1_bytemask;
          }
        pkg1_number++;
        }
      for(i=4; i<pkg_size; i+=3)
        {
        timf1_char[timf1_netptr  ]=0;
        timf1_char[timf1_netptr+1]=tmpbuf1[i  ];
        timf1_char[timf1_netptr+2]=tmpbuf1[i+1];
        timf1_char[timf1_netptr+3]=tmpbuf1[i+2];
        timf1_netptr=(timf1_netptr+4)&timf1_bytemask;
        }
      pkg1_number++;
      }
    }    
  while( ((timf1_netptr-timf1p_pa+timf1_bytes)&timf1_bytemask) > 
                                     2*snd[RXAD].block_bytes)
    {
#include "input_speed.c"
    finish_rx_read();
    }    
  if(kill_all_flag) goto sdrip_udpbuf_exit;
  }
sdrip_udpbuf_exit:;
free(tmpbuf1);  
free(tmpbuf2);  
sdrip_udp_exit:;
CLOSE_FD(udp1_fd);
if(sdrip.port2 != -1)CLOSE_FD(udp2_fd);
sdrip_exit:;
// Set the receiver state.
msg[0]=0x08;
msg[1]=0x00;
msg[2]=0x18;
msg[3]=0x00;
msg[4]=0x80;        // Set complex contigous data.
msg[5]=0x01;        // set run/stop to STOP
if(sdrip.format == 16)
  {
  msg[6]=0x00;
  }
else
  {
  msg[6]=0x80;  
  }
msg[7]=0x01;  
net_write(sdrip1_fd,msg,8);
if(sdrip.port2 != -1)net_write(sdrip2_fd,msg,8);
lir_sleep(100000);
recv(sdrip1_fd,textbuf,80,0);
if(sdrip.port2 != -1)recv(sdrip2_fd,textbuf,80,0);
sdrip_error_exit:;
close_sdrip();
sdrip_init_error_exit:;
if(errcod != 0)lirerr(errcod);
thread_status_flag[THREAD_SDRIP_INPUT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_SDRIP_INPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}
