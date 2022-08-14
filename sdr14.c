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
#include <string.h>
#include <fcntl.h>
#include "uidef.h"
#include "thrdef.h"
#include "sdrdef.h"
#include "fft1def.h"
#include "screendef.h"
#include "hwaredef.h"
#include "vernr.h"
#include "options.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif


float sdr_shape_factor=0.97;
#define MAX_SDR14_SPEED 3000000

P_SDR14 sdr14;

char *sdr14_parfil_name="par_sdr14";

char *sdr14_parm_text[MAX_SDR14_PARM] = {"M_CIC2",
                                         "M_CIC5",
                                         "M_RCF",
                                         "OL_RCF",
                                         "Sampl. clk adjust",
                                         "Att",
                                         "Inp.sel",
                                         "sernum1",
                                         "sernum2",
                                         "sernum3",
                                         "device code",
                                         "Check"};


#define M_RCF sdr14.m_rcf
#define M_CIC5 sdr14.m_cic5
#define M_CIC2 sdr14.m_cic2

#if (OSNUM == OSNUM_LINUX)
int sdr14_scan_usb(void);
#endif


void open_sdr14(void);
void lir_sdr14_write(char *s, int bytes);
int lir_sdr14_read(char *s, int bytes);

int display_sdr14_parm_info(int *line)
{
char s[80];
int errcod;
errcod=read_sdrpar(sdr14_parfil_name, MAX_SDR14_PARM, 
                                     sdr14_parm_text, (int*)((void*)&sdr14));
if(errcod == 0)
  {
  settextcolor(7);
  sprintf(s,"m cic2             = %i  ", sdr14.m_cic2);
  line[0]+=1;
  lir_text(24,line[0],s);
  sprintf(s,"m cic5             = %i  ", sdr14.m_cic5);
  line[0]+=1;
  lir_text(24,line[0],s);
  sprintf(s,"m rcf              = %i  ", sdr14.m_rcf);
  line[0]+=1;
  lir_text(24,line[0],s);
  sprintf(s,"ol rcf             = %i  ", sdr14.ol_rcf);
  line[0]+=1;
  lir_text(24,line[0],s);
  sprintf(s,"clock adjust       = %i  ", sdr14.clock_adjust);
  line[0]+=1;
  lir_text(24,line[0],s);
  sprintf(s,"input              = %i  ", sdr14.input);
  line[0]+=1;
  lir_text(24,line[0],s);
  }
return (errcod);
}

int sdr_destructive_control(char *msg)
{
char *s;
int i, ir;
unsigned short int msg_header, shi;
s=(char*)&msg_header;
i=0;
while(i<2)
  {
  ir=lir_sdr14_read(&s[i],2-i);
  if(ir<0)return 0;
  i+=ir;
  }
if(msg_header >= 80)
  {
err:;
  i=lir_sdr14_read(timf1_char,8192);
  msg[0]=0;
  if(i==-1)i=0;
  return -i;
  }
else
  {
  if(msg_header <= 4)goto err;
  msg_header-=4;
  ir=lir_sdr14_read((char*)&shi,2);
  if(ir<0)return 0;
  if(msg_header < 70)
    {
    ir=lir_sdr14_read(msg,msg_header);
    if(ir<0)return 0;
    }
  else
    {
    msg_header-=70;
    ir=lir_sdr14_read(timf1_char,msg_header);
    if(ir<0)return 0;
    }
  }
return shi;
}

void sdr_target_name(char *ss)
{
int i,j;
char s[4]={4,32,1,0};
j=0;
retry:;
j++;
lir_sdr14_write(s,4);
i=sdr_destructive_control(ss);
if(i==1 || j>3)return;
goto retry;
}

void sdr_target_serial_number(char *ss)
{
char s[4]={4,32,2,0};
lir_sdr14_write(s,4);
sdr_destructive_control(ss);
}

void sdr_target_interface_version(char *ss)
{
char s[4]={4,32,3,0};
lir_sdr14_write(s,4);
sdr_destructive_control(ss);
}

void sdr_target_firmware_version(char *ss)
{
char s[5]={5,32,4,0,1};
lir_sdr14_write(s,5);
sdr_destructive_control(ss);
}

void sdr_target_boot_version(char *ss)
{
char s[5]={5,32,4,0,0};
lir_sdr14_write(s,5);
sdr_destructive_control(ss);
}

char sdr14cmd[10];


void load_ad6620(void)
{
short int *hed;
hed=(short int*)sdr14cmd;
lir_sdr14_write(sdr14cmd,0x1ff&hed[0]);
}

void sdr_set_frequency(void)
{
int i;
float t1;
short int *address;
int *dat;
unsigned short int *msg_header;
msg_header=(unsigned short int*)(&sdr14cmd[0]);
dat=(int*)(&sdr14cmd[5]);
address=(short int*)(&sdr14cmd[2]);
msg_header[0]=10;
address[0]=0x20;
sdr14cmd[4]=0;
t1=fg.passband_center;
if(fg.passband_center > adjusted_sdr_clock)
  {
  i=fg.passband_center/adjusted_sdr_clock;
  t1=fg.passband_center-i*adjusted_sdr_clock;
  }
if(t1 > 0.5*adjusted_sdr_clock)
  {
  t1=adjusted_sdr_clock-t1;
  }
t1*=SDR14_SAMPLING_CLOCK/adjusted_sdr_clock;
dat[0]=rint(t1*1000000);
fg_truncation_error=t1*1000000-dat[0];
sdr14cmd[9]=1;
lir_sdr14_write(sdr14cmd,10);
}

void set_sdr14_att(void)
{
float t1;
unsigned short int *msg_header;
short int *address;
address=(short int*)(&sdr14cmd[2]);
msg_header=(unsigned short int*)(&sdr14cmd[0]);
msg_header[0]=6;
address[0]=0x38;
if(ui.rx_addev_no==SDR14_DEVICE_CODE)
  {
  sdr14cmd[4]=0;
  sdr14cmd[5]=fg.gain;
  }
if(ui.rx_addev_no==SDRIQ_DEVICE_CODE)
  {
  sdr14cmd[4]=1;
  if(fg.gain > -sdr14.input)
    {
// Gain is in the range 0 to -sdr14.input dB.
    t1=-0.05*fg.gain;
    t1=pow(10,t1);
    t1=127/t1;
    sdr14cmd[5]=t1+0.5;
    sdr14cmd[5]&=0x7f;
    }
  else
    {
    t1=-0.05*(fg.gain+10);
    t1=pow(10,t1);
    t1=127/t1;
    sdr14cmd[5]=t1+0.5;
    sdr14cmd[5]|=0x80;
    }
  }
lir_sdr14_write(sdr14cmd,6);
}

void sdr_set_cic2(int mcic2)
{
int i,scale;
int *is;
short int *address;
address=(short int*)(&sdr14cmd[2]);
i=mcic2>>1;
i=i*i;
scale=0;
while(i!=0)
  {
  i>>=1;
  scale++;
  }
if(scale > 6)scale=6;
address[0]=0x305;
is=(int*)&sdr14cmd[4];
is[0]=scale;
load_ad6620();
address[0]=0x306;
is[0]=mcic2-1;
load_ad6620();
}

void sdr_set_cic5(int mcic5)
{
int i,scale;
int *is;
short int *address;
address=(short int*)(&sdr14cmd[2]);
i=mcic5>>1;
i=i*i*i*i*i;
scale=0;
while(i!=0)
  {
  i>>=1;
  scale++;
  }
if(scale > 20)scale=20;
address[0]=0x307;
is=(int*)&sdr14cmd[4];
is[0]=scale;
load_ad6620();
address[0]=0x308;
is[0]=mcic5-1;
load_ad6620();
}

void sdr_set_m_rcf(int m)
{
int *is;
short int *address;
address=(short int*)(&sdr14cmd[2]);
address[0]=0x30a;
is=(int*)&sdr14cmd[4];
is[0]=m-1;
load_ad6620();
}

void sdr_set_ol_rcf(void)
{
int *is;
short int *address;
address=(short int*)(&sdr14cmd[2]);
address[0]=0x309;
is=(int*)&sdr14cmd[4];
is[0]=sdr14.ol_rcf;
load_ad6620();
}

void symmetry_adapt(void)
{
double *ww;
int i;
double t2;
ww=(double*)timf1_float;
// The window function has to be a symmetric function with
// imaginary parts=zero.
for(i=1; i<128; i++)
  {
  t2=0.5*(ww[2*i]+ww[2*(256-i)]);
  ww[2*i]=t2;
  ww[2*(256-i)]=t2;
  }
for(i=0; i<256; i++)ww[2*i+1]=0;
}

void shift_128(void)
{
double *ww;
int i;
double t1;
ww=(double*)timf1_float;
for(i=0; i<128;i++)
  {
  t1=ww[2*i];
  ww[2*i]=ww[2*(i+128)];
  ww[2*(i+128)]=t1;
  }
}

void clear_small(float limit)
{
int i;
double *ww;
float t1;
ww=(double*)timf1_float;
// Clear all points that are below limit.
t1=0;
for(i=0; i<256; i++)
  {
  if(t1<fabs(ww[2*i]))t1=fabs(ww[2*i]);
  }
t1*=limit;
for(i=0; i<256; i++)
  {
  if(fabs(ww[2*i]) < limit) ww[2*i]=0;
  }
}

void round_to_integer(int limit)
{
int i, j;
double *ww;
ww=(double*)timf1_float;
// Clear all points that are below 0.5 bit in our 20 bit words.
for(i=0; i<256; i++)
  {
  if(fabs(ww[2*i]) > limit)
    {
// Round to the nearest integer.
    j=fabs(ww[2*i])+0.5;
    if( ww[2*i] < 0) j=-j;
    ww[2*i]=j;
    }
  }
}

void sdr14_stop(void)
{
double dt1;
int i, ir;
unsigned short int *msg_header;
short int *address;
msg_header=(unsigned short int*)(&sdr14cmd[0]);
address=(short int*)(&sdr14cmd[2]);
msg_header[0]=8;
address[0]=0x18;
sdr14cmd[4]=0x81;  //Use AD6620 and RF amplifier.
sdr14cmd[5]=1;     //Set to STOP
sdr14cmd[6]=0;     //Continous mode
sdr14cmd[7]=0;     //Does not matter in continous mode
lir_sdr14_write(sdr14cmd,8);
dt1=current_time();
end_loop:;
i=lir_sdr14_read((char*)msg_header,2);
if(i<0)goto errclose;
while(i!=2)
  {
  lir_sched_yield();
  ir=lir_sdr14_read(&sdr14cmd[i],2-i);
  if(ir<0)goto errclose;
  i+=ir;
  if(current_time() -dt1 > 0.1)goto errclose;
  }
if((0xffff&msg_header[0]) == (unsigned short int)0x8000)
  {
  i=lir_sdr14_read(timf1_char,8192);
  if(i<0)goto errclose;
  while(i!=8192)
    {
    lir_sleep(2000);
    ir=lir_sdr14_read(timf1_char,8192-i);
    if(ir<0)goto errclose;
    i+=ir;
    if(current_time() -dt1 > 0.1)goto errclose;
    }
  }
else
  {
  if(i <= 8)
    {
    ir=lir_sdr14_read(&sdr14cmd[2],i);
    if(address[0]==0x18)goto errclose;
    }
  else
    {
    ir=lir_sdr14_read(timf1_char,8192);
    }
  if(ir<0)goto errclose;
  }
if(current_time() -dt1 < 0.1)goto end_loop;
errclose:;
close_sdr14();
}

void sdr14_input(void)
{
int rxin_local_workload_reset;
int *is;
char s[80];
D_COSIN_TABLE *local_fft_table;
unsigned int *local_fft_permute;
double dt1, read_start_time,total_reads;
double *ww;
int timing_loop_counter,timing_loop_counter_max,initial_skip_flag;
int i, j, nn, ntaps;
int ia, ib, errcod;
short int *isho;
unsigned short int *msg_header;
short int *address;
int *windat;
int local_att_counter;
int local_nco_counter;
char ackmsg[3]={3,0x60,0};
float t1;
windat=(int*)(&sdr14cmd[4]);
msg_header=(unsigned short int*)(&sdr14cmd[0]);
address=(short int*)(&sdr14cmd[2]);
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_SDR14_INPUT);
#endif
no_of_rx_overrun_errors=0;
begin:;
local_att_counter=sdr_att_counter;
local_nco_counter=sdr_nco_counter;
j=0;
dt1=current_time();
errcod=read_sdrpar(sdr14_parfil_name, MAX_SDR14_PARM, 
                                     sdr14_parm_text, (int*)((void*)&sdr14));
if(errcod != 0 || sdr14.check != SDR14PAR_VERNR)goto sdr14_init_error_exit;
while(sdr == -1)
  {
  open_sdr14();
  lir_sleep(30000);
  if(sdr == -1)
    {
    sprintf(s,"Waiting %.2f", current_time()-dt1);
    lir_text(0,screen_last_line,s);
    lir_refresh_screen();
    if(kill_all_flag)goto sdr14_init_error_exit;
    }
  }
sdr_target_name(s);
if(ui.rx_addev_no==SDR14_DEVICE_CODE)
  {
  if(strcmp(sdr14_name_string,s) != 0)goto sdr14_init_error_exit;
  }
if(ui.rx_addev_no==SDRIQ_DEVICE_CODE)
  {
  if(strcmp(sdriq_name_string,s) != 0)goto sdr14_init_error_exit;
  }
// ****************************************************
// We have a connection to the SDR-14.
// Set status to stop
msg_header[0]=8;
address[0]=0x18;
sdr14cmd[4]=0x81;  //Use AD6620 and RF amplifier.
sdr14cmd[5]=1;     //Set to STOP
sdr14cmd[6]=0;     //Continous mode
sdr14cmd[7]=0;     //Does not matter in continous mode
lir_sdr14_write(sdr14cmd,8);
i=1;
while(i==0 || i!=0x18)
  {
  i=sdr_destructive_control(s);
  if(kill_all_flag)goto sdr14_init_error_exit;
  }
if( abs(sdr14.clock_adjust) > 10000)goto sdr14_init_error_exit;
adjusted_sdr_clock=SDR14_SAMPLING_CLOCK+0.000001*sdr14.clock_adjust,
t1=ui.rx_ad_speed-1000000.*adjusted_sdr_clock/
                                     (sdr14.m_cic2*sdr14.m_cic5*sdr14.m_rcf);
if(fabs(t1/ui.rx_ad_speed) > 0.01)
  {
  lirerr(1345);
  goto sdr14_init_error_exit;
  }
ui.rx_ad_speed=1000000.*adjusted_sdr_clock/(M_CIC2*M_CIC5*M_RCF);
if(ui.rx_ad_speed > MAX_SDR14_SPEED)
  {
  lirerr(1165);
  goto normal_exit;
  }
// Program the AD6620.
set_sdr14_att();
i=1;
while(i==0 || i!=0x38)
  {
  i=sdr_destructive_control(s);
  if(kill_all_flag)goto sdr14_init_error_exit;
  }
// Set the frequency
sdr_set_frequency();
i=1;
while(i==0 || i!=0x20)
  {
  i=sdr_destructive_control(s);
  if(kill_all_flag)goto sdr14_init_error_exit;
  }
msg_header[0]=0xa009;
// Set MODE CONTROL REGISTER to 1 (soft reset)
address[0]=0x300;
sdr14cmd[4]=1;
load_ad6620();
// Data sheet says we should write 0 here.
address[0]=0x30d;
is=(int*)&sdr14cmd[4];
is[0]=0;
load_ad6620();
ww=(double*)timf1_float;
for(i=0; i<512; i++)ww[i]=0;
// Set up tables for a 256 point fft
local_fft_permute=(unsigned int*)(&ww[512]);
local_fft_table=(D_COSIN_TABLE*)(&timf1_float[2048+32]);
init_d_fft(0,8, 256, local_fft_table, local_fft_permute);
ntaps=M_RCF*M_CIC5*M_CIC2/2;
if(ntaps > 256)ntaps=256;
// Construct a filter function for the FIR filter in ntaps points.
// The gaussian function is the function that minimises the
// pulse response for a given bandwidth.
// The array of FIR filter coefficients is the pulse response of a FIR filter
// and as a first step we construct the widest acceptable time
// function as a single gaussian.
t1=0.25/0x7ffff;
t1=pow(exp(1.),t1);
t1=0.5*sqrt(t1/ntaps);
ww[0]=1;
for(i=1; i<128; i++)
  {
  dt1=i*t1;
  dt1=exp(-dt1*dt1);
  ww[2*i]=dt1;
  ww[2*(256-i)]=dt1;
  }
symmetry_adapt();
// Go to the frequency domain where we get the frequency response
// of the longest acceptable time function.
d_fftforward(256,8,ww,local_fft_table, local_fft_permute);
symmetry_adapt();
clear_small(0.000001);
nn=0;
while(ww[2*nn]>0.3162*ww[0])nn++;
// nn now points to the -10 dB point.
// Construct a rectangular filter in the frequency domain
// by summing several frequency shifted gaussian filters
// of the now established shape.
// Make the filter flat over a center portion that is 1/M_RCF points wide.
nn=128./M_RCF-nn;
// nn is now half the range over which we want to sum up gaussians.
shift_128();
for(i=nn; i<256-nn; i++)
  {
  ia=i-nn;
  ib=i+nn;
  t1=0;
  for(j=ia; j<=ib; j++)
    {
    t1+=ww[2*j];
    }
  ww[2*i+1]=t1;
  }
for(i=0; i<256; i++)
  {
  ww[2*i]=ww[2*i+1];
  }
shift_128();
symmetry_adapt();
// Take the back transform to go from the frequency domain to the time domain.
d_fftback(256,8,ww,local_fft_table, local_fft_permute);
symmetry_adapt();
// Round the big numbers to integers
t1=0x7ffff/ww[0];
for(i=0; i<256; i++)
  {
  ww[2*i]*=t1;
  }
round_to_integer(4);
shift_128();
for(i=0; i<256; i++)
  {
  address[0]=i;
  windat[0]=ww[2*i];
  load_ad6620();
  }
// Set ntaps
address[0]=0x30c;
sdr14cmd[4]=ntaps-1;
load_ad6620();
// With offset
address[0]=0x30b;
sdr14cmd[4]=128-ntaps/2;
load_ad6620();
// Disable dithering. A good operator will change the frequency
// to avoid spurs, converting them to wideband noise will degrade
// the dynamic range.
address[0]=0x301;
sdr14cmd[4]=0;  //phase dither=2, amplitude dither=4, both=6
load_ad6620();
// Set MCIC2 and the accompanying scale factor
sdr_set_cic2(M_CIC2);
sdr_set_cic5(M_CIC5);
sdr_set_m_rcf(M_RCF);
sdr_set_ol_rcf();
fft1_block_timing();
snd[RXAD].block_bytes=8192;
snd[RXAD].block_frames=snd[RXAD].block_bytes/4;
snd[RXAD].interrupt_rate=(float)(ui.rx_ad_speed)/snd[RXAD].block_frames;
// Start the AD6620
address[0]=0x300;
sdr14cmd[4]=0;
load_ad6620();
msg_header[0]=8;
address[0]=0x18;
sdr14cmd[4]=0x81;  //Use AD6620 and RF filter.
if(ui.rx_addev_no == SDR14_DEVICE_CODE && sdr14.input !=0)
  {
  sdr14cmd[4]=0x80;  //Use AD6620 without filter.
  }
sdr14cmd[5]=2;     //Set to RUN
sdr14cmd[6]=0;     //Continous mode
sdr14cmd[7]=0;     //Does not matter in continous mode
lir_sdr14_write(sdr14cmd,8);
if(thread_command_flag[THREAD_SCREEN]!=THRFLAG_NOT_ACTIVE)
  {
  while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
    {
    if(thread_command_flag[THREAD_SDR14_INPUT] ==
                                  THRFLAG_KILL)goto normal_exit;
    lir_sleep(10000);
    }
  }
thread_status_flag[THREAD_SDR14_INPUT]=THRFLAG_ACTIVE;
set_hardware_rx_frequency();
#include "timing_setup.c"
lir_status=LIR_OK;
while(thread_command_flag[THREAD_SDR14_INPUT] == THRFLAG_ACTIVE)
  {
  i=lir_sdr14_read((char*)msg_header,2);
  if(i<0)goto sdr14_error_exit;
  isho=(short int*)(&timf1_char[timf1p_pa]);
  if((0xffff&msg_header[0]) == (unsigned short int)0x8000)
    {
    if(timing_loop_counter == timing_loop_counter_max)
      {
      lir_sdr14_write(ackmsg,3);
      }
#include "input_speed.c"
    i=lir_sdr14_read((char*)isho,8192);
    if(i<0)goto sdr14_error_exit;
    finish_rx_read();
    if(kill_all_flag) goto normal_exit;
    if(local_att_counter != sdr_att_counter)
      {
      local_att_counter=sdr_att_counter;
      set_sdr14_att();
      }
    if(local_nco_counter != sdr_nco_counter)
      {
      local_nco_counter=sdr_nco_counter;
      sdr_set_frequency();
      }
    }
  else
    {
    i=(0x1fff&msg_header[0])-2;
    if(i <= 8)
      {
      i=lir_sdr14_read(&sdr14cmd[2],i);
      if(i<0)goto sdr14_error_exit;
      }
    else
      {
// Something went (very) wrong.
      no_of_rx_overrun_errors++;
      sprintf(s,"RX%s%d",overrun_error_msg,no_of_rx_overrun_errors);
      wg_error(s,WGERR_RXIN);
      sdr14_stop();
      goto begin;
      }
    }
  }
normal_exit:;
if(sdr != -1)sdr14_stop();
thread_status_flag[THREAD_SDR14_INPUT]=THRFLAG_RETURNED;
while(!kill_all_flag &&
        thread_command_flag[THREAD_SDR14_INPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
return;
sdr14_error_exit:;
lirerr(1343);
goto normal_exit;
sdr14_init_error_exit:;
lirerr(1163);
goto normal_exit;
}

void init_sdr14(void)
{
char ss[80];
char sn[80];
char s[128];
int i, line;
float t1;
FILE *sdr14_file;
int *sdr_pi;
char *looking;
looking="Looking for SDR-14 or SDR-IQ on USB ports.";
// Set device_no to appropriate values if some dedicated hardware
// is selected by the user.
//  ------------------------------------------------------------
// See if there is an SDR-14 or an SDR-IQ on the system.
settextcolor(12);
lir_text(10,10,looking);
SNDLOG"\n%s",looking);
lir_text(10,11,"Reset CPU, USB and SDR hardware if system hangs here.");
lir_refresh_screen();
settextcolor(7);
line=0;
#if(OSNUM == OSNUM_LINUX)
i=sdr14_scan_usb();
if(i<0)goto sdr14_fail;
open_sdr14();
clear_screen();
sdr_target_name(sn);
goto use_sdr14;
#endif
open_sdr14();
clear_lines(10,11);
lir_refresh_screen();
if(sdr != -1)
  {
  SNDLOG"open_sdr14 sucessful.\n");
  lir_text(5,5,"An SDR is detected on the USB port.");
  lir_text(5,6,"Do you want to use it for RX input (Y/N)?");
qsdr:;
  await_processed_keyboard();
  if(kill_all_flag) goto sdr14_errexit;
  if(lir_inkey == 'Y')
    {
    sdr_target_name(sn);
    sprintf(s,"Target name: %s",sn);
    SNDLOG"%s\n",s);
    lir_text(5,10,s);
    if( strcmp(sdr14_name_string,sn) == 0)
      {
      ui.rx_addev_no=SDR14_DEVICE_CODE;
      }
    if( strcmp(sdriq_name_string,sn) == 0)
      {
      ui.rx_addev_no=SDRIQ_DEVICE_CODE;
      }
    if(ui.rx_addev_no == UNDEFINED_DEVICE_CODE)
      {
      lir_text(5,12,"Unknown hardware, can not use.");
      lir_text(5,13,press_any_key);
      await_keyboard();
      if(kill_all_flag) goto sdr14_errexit;
      clear_screen();
      }
    else
      {
      sdr_target_serial_number(ss);
      sprintf(s,"Serial no: %s",ss);
      lir_text(5,11,s);
      sdr_target_interface_version(ss);
      i=(short int)ss[0];
      sprintf(s,"Interface version: %d.%02d",i/100,i%100);
      lir_text(5,12,s);
      sdr_target_boot_version(ss);
      i=(short int)ss[0];
      sprintf(s,"PIC boot version: %d.%02d",i/100,i%100);
      lir_text(5,13,s);
      sdr_target_firmware_version(ss);
      i=(short int)ss[0];
      sprintf(s,"PIC firmware version: %d.%02d",i/100,i%100);
      lir_text(5,14,s);
      lir_text(10,16,"PRESS ANY KEY");
restart_sdr14par:;
      await_processed_keyboard();
#if(OSNUM == OSNUM_LINUX)
use_sdr14:;
#endif
      ui.rx_input_mode=IQ_DATA+DIGITAL_IQ;
      ui.rx_rf_channels=1;
      ui.rx_ad_channels=2;
      ui.rx_admode=0;
      if(kill_all_flag) goto sdr14_errexit;
      clear_screen();
      sprintf(s,"%s selected for input",sn);
      lir_text(10,line,s);
      line++;
      if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
        {
        lir_text(5,line,"Set CIC2 decimation (2 - 16)");
        sdr14.m_cic2=lir_get_integer(35, line, 2, 2,16);
        if(kill_all_flag)goto sdr14_errexit;
        t1=SDR14_SAMPLING_CLOCK/sdr14.m_cic2;
        sprintf(s,"clk=%.2f MHz",t1);
        lir_text(39,line,s);
        line++;
        lir_text(5,line,"Set CIC5 decimation (2 - 32)");
        sdr14.m_cic5=lir_get_integer(35, line, 2, 2,32);
        if(kill_all_flag)goto sdr14_errexit;
        t1/=sdr14.m_cic5;
        sprintf(s,"clk=%.4f MHz",t1);
        lir_text(39,line,s);
        line++;
        lir_text(5,line,"Set RCF decimation (2 - 32)");
        sdr14.m_rcf=lir_get_integer(35, line, 2, 2,32);
        if(kill_all_flag)goto sdr14_errexit;
        t1/=sdr14.m_rcf;
        ui.rx_ad_speed=t1*1000000;
        if(ui.rx_ad_speed > MAX_SDR14_SPEED)
          {
          line++;
          settextcolor(12);
          lir_text(5,line,
                "ERROR The sampling speed is far too high for USB 1.0");
          line++;
          lir_text(5,line,press_any_key);
          settextcolor(7);
          goto restart_sdr14par;
          }
        }
      else
        {
        sdr14.m_cic2=10;
        t1=SDR14_SAMPLING_CLOCK/sdr14.m_cic2;
        sdr14.m_cic5=10;
        t1/=sdr14.m_cic5;
        line++;
        for(i=4; i<8; i++)
          {
          sprintf(s,"RCF= %d  Speed %f kHz",i,1000*t1/i);
          lir_text(0,line,s);
          line++;
          }
        line++;
        lir_text(5,line,"Set RCF decimation (4 - 7)");
        sdr14.m_rcf=lir_get_integer(35, line, 2, 4,7);
        if(kill_all_flag)goto sdr14_errexit;
        t1/=sdr14.m_rcf;
        ui.rx_ad_speed=t1*1000000;
        }
      sprintf(s,"clk=%.2f kHz",t1*1000);
      lir_text(39,line,s);
      line++;
      lir_text(5,line,"Set RCF output shift (0 - 7)");
      sdr14.ol_rcf=lir_get_integer(35, line, 2, 0,7);
      if(kill_all_flag)goto sdr14_errexit;
      line++;
      if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
        {
        lir_text(5,line,"Set sampling clock shift (Hz)");
        sdr14.clock_adjust=lir_get_integer(35, line, 6,-10000,10000);
        if(kill_all_flag)goto sdr14_errexit;
        line++;
        }
      else
        {
        sdr14.clock_adjust=0;
        }
      adjusted_sdr_clock=SDR14_SAMPLING_CLOCK+0.000001*sdr14.clock_adjust,
      ui.rx_ad_speed=1000000.*adjusted_sdr_clock/(M_CIC2*M_CIC5*M_RCF);
      if(ui.rx_addev_no == SDR14_DEVICE_CODE)
        {
        lir_text(5,line,"Select direct input (0 - 1)");
        sdr14.input=lir_get_integer(35, line, 2, 0, 1);
        if(kill_all_flag)return;
        }
      else
        {
        if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
          {
          lir_text(5,line,"Attenuation below which to use 10dB (10 - 25)");
          sdr14.input=lir_get_integer(52, line, 2, 10, 25);
          if(kill_all_flag)return;
          }
        else
          {
          sdr14.input=20;
          }
        }
      sdr14_file=fopen(sdr14_parfil_name,"w");
      if(sdr14_file == NULL)
        {
        lirerr(381264);
sdr14_errexit:;
        close_sdr14();
        clear_screen();
        return;
        }
      sdr14.check=SDR14PAR_VERNR;
      sdr_pi=(int*)(&sdr14);
      for(i=0; i<MAX_SDR14_PARM; i++)
        {
        fprintf(sdr14_file,"%s [%d]\n",sdr14_parm_text[i],sdr_pi[i]);
        }
      parfile_end(sdr14_file);
      }
    }
  else
    {
    if(lir_inkey != 'N')goto qsdr;
    clear_screen();
    }
  close_sdr14();
  }
else
  {
#if(OSNUM == OSNUM_LINUX)
sdr14_fail:;
#endif
  SNDLOG"open_sdr14 USB failed.\n");
  clear_screen();
  lir_text(5,5,"No SDR-14 or SDR-IQ device found.");
  lir_text(5,6,"Make sure that the proper kernel module is installed");
  lir_text(8,8,press_any_key);
  await_keyboard();
  }
return;
}
