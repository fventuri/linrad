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
#include "vernr.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

void open_excalibur(void);
void close_excalibur(void);
void select_excalibur_mode(int *line);
void start_excalibur_read(void);
void lir_excalibur_read(void);
void set_excalibur_frequency(void);
void set_excalibur_att(void);
void excalibur_stop(void);
int load_excalibur_library(char *serial_numbers);


char *excalibur_parfil_name="par_excalibur";


char *excalibur_parm_text[MAX_EXCALIBUR_PARM]={"Rate no",           //0
                                           "Sampl. rate",         //1
                                           "Sampl. clk adjust",   //2
                                           "MW filter maxfreq",   //3
                                           "Dither",              //4
                                           "Check"};              //5


void excalibur_input(void)
{
double rt_factor;
int i, errcod;
int rxin_local_workload_reset;
char s[80];
double dt1, read_start_time, total_reads;
int timing_loop_counter,timing_loop_counter_max,initial_skip_flag;
float t1;
int local_nco_counter, local_att_counter;
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_EXCALIBUR_INPUT);
#endif
local_nco_counter=sdr_nco_counter;
local_att_counter=sdr_att_counter;
errcod=0;
dt1=current_time();
while(sdr == -1)
  {
  open_excalibur();
  lir_sleep(30000);
  if(sdr == -1)
    {
    sprintf(s,"Waiting %.2f", current_time()-dt1);
    lir_text(0,screen_last_line,s);
    lir_refresh_screen();
    if(kill_all_flag)goto excalibur_init_error_exit;
    }
  }
// ****************************************************
// We have a connection to the Excaliobur.
i=read_sdrpar(excalibur_parfil_name, MAX_EXCALIBUR_PARM, 
                                excalibur_parm_text, (int*)((void*)&excalib));
if(i != 0)
  {
  errcod=1337;
  goto excalibur_error_exit;
  }
if( excalib.check != EXCALIBUR_PAR_VERNR ||
    excalib.rate_no < 0 ||
    abs(excalib.clock_adjust) > 10000 ||
    excalib.rate_no  >= no_of_excalibur_rates)goto excalibur_error_exit;
rt_factor=EXCALIBUR_SAMPLING_CLOCK/excalib.sampling_rate;
adjusted_sdr_clock=EXCALIBUR_SAMPLING_CLOCK+excalib.clock_adjust;
t1=ui.rx_ad_speed-adjusted_sdr_clock/rt_factor;
if(fabs(t1/ui.rx_ad_speed) > 0.0001)
  {
  errcod=1338;
  goto excalibur_error_exit;
  }
ui.rx_ad_speed=adjusted_sdr_clock/rt_factor;
fft1_block_timing();
if(thread_command_flag[THREAD_SCREEN]!=THRFLAG_NOT_ACTIVE)
  {
  while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
    {
    if(thread_command_flag[THREAD_EXCALIBUR_INPUT] ==
                                           THRFLAG_KILL)goto excalibur_error_exit;
    lir_sleep(10000);
    }
  }
start_excalibur_read();
set_excalibur_frequency();
set_excalibur_att();
thread_status_flag[THREAD_EXCALIBUR_INPUT]=THRFLAG_ACTIVE;
#include "timing_setup.c"
lir_status=LIR_OK;
while(thread_command_flag[THREAD_EXCALIBUR_INPUT] == THRFLAG_ACTIVE)
  {
#include "input_speed.c"
  if(local_nco_counter != sdr_nco_counter)
    {
    local_nco_counter=sdr_nco_counter;
    set_excalibur_frequency();
    }
  if(local_att_counter != sdr_att_counter)
    {
    local_att_counter=sdr_att_counter;
    set_excalibur_att();
    }
  lir_excalibur_read();
  if(kill_all_flag)goto excalibur_exit;
  finish_rx_read();
  if(kill_all_flag) goto excalibur_exit;
  }
excalibur_exit:;
excalibur_stop();
excalibur_error_exit:;
if(errcod != 0)lirerr(errcod);
close_excalibur();
excalibur_init_error_exit:;
thread_status_flag[THREAD_EXCALIBUR_INPUT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_EXCALIBUR_INPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}


int display_excalibur_parm_info(int *line)
{
char s[80];
int errcod;
errcod=read_sdrpar(excalibur_parfil_name, MAX_EXCALIBUR_PARM, 
                               excalibur_parm_text, (int*)((void*)&excalib));
if(errcod == 0)
  {
  settextcolor(7);
  sprintf(s,"rate number        = %i ", excalib.rate_no);
  if(excalib.sampling_rate >=1000000)
    {
    sprintf(s,"rate number        = %i (%8.3f MHz)", 
                      excalib.rate_no, 0.000001*excalib.sampling_rate);
    }
  else
    {
    sprintf(s,"rate number        = %i (%8.3f kHz)", 
                       excalib.rate_no, 0.001*excalib.sampling_rate);
    }
  lir_text(24,line[0],s);
  SNDLOG"\n%s",s);
  line[0]+=1;
  sprintf(s,"clock adjustment   = %i", excalib.clock_adjust);
  lir_text(24,line[0],s);
  SNDLOG"\n%s",s);
  line[0]+=1;
  sprintf(s,"MW filter maxfreq  = %i", excalib.mw_filter_maxfreq);
  lir_text(24,line[0],s);
  SNDLOG"\n%s",s);
  line[0]+=1;
  sprintf(s,"dither             = %i", excalib.dither);
  lir_text(24,line[0],s);
  SNDLOG"\n%s\n",s);
  }
return (errcod);
}

void init_excalibur(void)
{
char *serial_numbers[MAX_EXCALIBUR_DEVICES][9];
FILE *excalibur_file, *excalibur_sernum_file;
double rt_factor;
int count, i, line;
int *sdr_pi;
char *looking;
char s[2000];
looking="Looking for an Excalibur on the USB port.";
settextcolor(12);
lir_text(10,1,looking);
SNDLOG"\n%s",looking);
lir_text(10,2,"Reset CPU, USB and SDR hardware if system hangs here.");
lir_refresh_screen();
settextcolor(14);
count=load_excalibur_library((char*)serial_numbers);
settextcolor(7);
if(count==0)
  {
  sprintf(s,"Did not find any Excalibur hardware.");
  SNDLOG"\n%s\n",s);
  lir_text(10,8,s);
  lir_text(12,9,press_any_key);
  await_processed_keyboard();
  return;
  }
clear_lines(1,2);
lir_refresh_screen();
line=7;
if(count == 1)
  {
  lir_text(0,line,"There is only one Excalibur on the system.");
  line++;
  sprintf(s,"The serial number is %s and it is auto-selected.",
                                               (char*)serial_numbers[0]);
  lir_text(0,line,s);                                               
  line++;
  i=0;
  }
else
  {
  for(i=0; i<count; i++)
    {
    sprintf(s,"%02d    serial=%s",i,(char*)serial_numbers[i]);
    lir_text(0,line,s);
    line++;
    }
  line++;  
  lir_text(0,line,"Select the unit to use by its line number.");
  i=lir_get_integer(44,line,2,0,count);
  if(kill_all_flag)return;
  }
excalibur_sernum_file=fopen(excalibur_sernum_filename,"w");
if(excalibur_sernum_file == NULL)
  {
  lirerr(281667);
  return;
  }
fprintf(excalibur_sernum_file,"%s",(char*)serial_numbers[i]);
fclose(excalibur_sernum_file);
open_excalibur();
if(kill_all_flag)return;
settextcolor(7);
if(sdr != -1)
  {
  clear_lines(1,2);
  lir_refresh_screen();
  line=10;
  select_excalibur_mode(&line);
  if(kill_all_flag)goto excalibur_errexit;
  if(ui.operator_skil == OPERATOR_SKIL_NEWCOMER)
    {
    excalib.clock_adjust=0;
    }
  else
    {
    sprintf(s,"Set sampling clock shift (Hz))");
    lir_text(5,line,s);
    excalib.clock_adjust=lir_get_integer(37, line, 5,-10000,10000);
    if(kill_all_flag)goto excalibur_errexit;
    line++;
    }
  rt_factor=EXCALIBUR_SAMPLING_CLOCK/excalib.sampling_rate;
  adjusted_sdr_clock=EXCALIBUR_SAMPLING_CLOCK+excalib.clock_adjust;
  ui.rx_ad_speed=adjusted_sdr_clock/rt_factor;
  if(ui.operator_skil == OPERATOR_SKIL_NEWCOMER)
    {
    excalib.mw_filter_maxfreq=1800000;
    excalib.dither=1;
    }
  else
    {
    lir_text(5,line,"Max freq (Hz) without MW filter ?");
    excalib. mw_filter_maxfreq=lir_get_integer(37, line, 7, 0, 50000000);
    if(kill_all_flag)goto excalibur_errexit;
    line++;
    lir_text(5,line,"Enable dither ? (0/1)");
    excalib.dither=lir_get_integer(27, line, 1, 0, 1);
    if(kill_all_flag)goto excalibur_errexit;
    line++;
    }
  excalib.check=EXCALIBUR_PAR_VERNR;
  excalibur_file=fopen(excalibur_parfil_name,"w");
  if(excalibur_file == NULL)
    {
excalibur_errexit:;
    close_excalibur();
    clear_screen();
    ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
    return;
    }
  sdr_pi=(int*)(&excalib);
  for(i=0; i<MAX_EXCALIBUR_PARM; i++)
    {
    fprintf(excalibur_file,"%s [%d]\n",excalibur_parm_text[i],sdr_pi[i]);
    }
  parfile_end(excalibur_file);
  close_excalibur();
  }
else
  {
  SNDLOG"\nopen_excalibur failed.\n");
  }
}


