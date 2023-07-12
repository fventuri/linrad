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
#include <ctype.h>
#include <string.h>
#if(OSNUM == OSNUM_WINDOWS)
#include "wscreen.h"
#include <winsock.h>
#define INVSOCK INVALID_SOCKET
extern HANDLE savefile_handle;
#endif

#if(OSNUM == OSNUM_LINUX)
#define INVSOCK -1
#include "loadalsa.h"
#include "lscreen.h"
int alsa_get_native_samplerate(int n,int mode,int *line,unsigned int *new_sampling_rate);
#endif

#if HAVE_CUFFT == 1
extern int test_cuda(void);
#endif

#include "uidef.h" 
#include "fft1def.h"
#include "fft2def.h"
#include "screendef.h"
#include "powtdef.h"
#include "vernr.h"
#include "sigdef.h"
#include "seldef.h"
#include "thrdef.h"
#include "sdrdef.h"
#include "caldef.h"
#include "keyboard_def.h"
#include "txdef.h"
#include "options.h"
#include "hwaredef.h"
#include "fft3def.h"
#include "padef.h"
#if OPENCL_PRESENT == 1
#include "oclprogs.h"
#endif
extern int tune_bytes;
#if (USERS_EXTRA_PRESENT == 1)
extern void init_users_extra(void);
#endif

char *parfilnam;
int uiparm_save[MAX_UIPARM];
int dirflag, iqflag;
int first_txproc_no;
int zzr;

void enable_extio(void)
{
if(diskread_flag == 0 && ui.use_extio != 0)
  {
  command_extio_library(EXTIO_COMMAND_LOAD_DLL);
  first_check_extio();
  if(ui.use_extio == 0)
    {
    lir_status=LIR_SAVE_UIPARM;
    return;
    }
  if(extio_handle==NULL)
    {
    lirerr(42977);
    return;
    }
  extio_show_gui=TRUE;  
  command_extio_library(EXTIO_COMMAND_START);
  }  
}

void disable_extio(void)
{
if(extio_running)command_extio_library(EXTIO_COMMAND_STOP);
if(extio_handle != NULL)
  {
  clear_screen();
  lir_text(3,3,"UNLOADING ExtIO LIBRARY");   
  lir_refresh_screen();
  lir_sleep(100000);
  command_extio_library(EXTIO_COMMAND_UNLOAD_DLL);
  }  
}

void radar_routine(void)
{
char s[80];
float t1;
int i, k, m;
use_bfo=1;
usercontrol_mode=USR_NORMAL_RX;
get_wideband_sizes();
if(kill_all_flag)return;
get_buffers(1);
if(kill_all_flag || lir_status != LIR_OK)return;
if(!freq_from_file)
  {
  read_freq_control_data();
  }
check_filtercorr_direction();
init_semaphores();
init_wide_graph();
if( genparm[SECOND_FFT_ENABLE] != 0 )init_blanker();
if(kill_all_flag || lir_status != LIR_OK) goto radar_x;
if(genparm[SECOND_FFT_ENABLE] != 0)
  {
  init_hires_graph();
  if(kill_all_flag || lir_status != LIR_OK) goto radar_x;
  }
if(genparm[AFC_ENABLE] != 0 && genparm[AFC_LOCK_RANGE] != 0)
  {
  init_afc_graph();
  if(kill_all_flag || lir_status != LIR_OK) goto radar_x;
  }
if(ui.rx_rf_channels == 2)
  {
  init_pol_graph();
  if(kill_all_flag || lir_status != LIR_OK) goto radar_x;
  }
init_radar_graph();
if(kill_all_flag || lir_status != LIR_OK) goto radar_x;
if(!freq_from_file && (ui.network_flag & NET_RX_INPUT) == 0)
  {
  init_freq_control();
  if(kill_all_flag || lir_status != LIR_OK) goto radar_x;
  }
if(use_tx != 0)init_tx_graph();
if(lir_status != LIR_OK) goto radar_x;
// Make sure users_init_mode is the last window(s) we open.
// the avoid collission routine does not know where
// users windows are placed.
if(ui.rx_soundcard_radio == RX_SOUNDCARD_RADIO_ELEKTOR &&
       diskread_flag == 0)
  {
  if(init_elektor_control_window() != EXIT_SUCCESS)ui.rx_soundcard_radio=0;
  }
if(ui.rx_soundcard_radio == RX_SOUNDCARD_RADIO_FCDPROPLUS &&
       diskread_flag == 0)
  {
  init_fcdproplus_control_window();  
  }
if(ui.rx_soundcard_radio == RX_SOUNDCARD_RADIO_AFEDRI_USB &&
       diskread_flag == 0)
  {
  open_afedriusb_control();  
  }
phasing_init_mode();
users_init_mode();
if(kill_all_flag || lir_status != LIR_OK) goto radar_x;
show_name_and_size();
linrad_thread_create(rx_input_thread);
lir_sleep(100000);
if(lir_status != LIR_OK)
  {
  lir_join(rx_input_thread);
  goto radar_x;
  }
if(kill_all_flag) goto radar_x;
if((ui.network_flag&NET_RX_OUTPUT)!=0)linrad_thread_create(THREAD_NETWORK_SEND);
linrad_thread_create(THREAD_USER_COMMAND);
if(kill_all_flag) goto radar_x;
linrad_thread_create(THREAD_RADAR);
if(kill_all_flag) goto radar_x;
linrad_thread_create(THREAD_WIDEBAND_DSP);
if(kill_all_flag) goto radar_x;
linrad_thread_create(THREAD_SCREEN);
if(kill_all_flag) goto radar_x;
if((ui.network_flag&NET_RX_OUTPUT)!=0)
  {
  linrad_thread_create(THREAD_LIR_SERVER);
  if(kill_all_flag) goto radar_x;
  }
if(use_tx != 0 )
  {
  init_txmem_cwproc();
  if(kill_all_flag)goto radar_x;
  linrad_thread_create(THREAD_TX_INPUT);
  if(kill_all_flag) goto radar_x;
  linrad_thread_create(THREAD_TX_OUTPUT);
  if(kill_all_flag) goto radar_x;
  }
fft1_waterfall_flag=1;
lir_refresh_screen();
all_threads_started=TRUE; 
lir_sleep(50000);
rxin_block_counter=0;
i=0;
k=0;
t1=50000+2000000./snd[RXAD].interrupt_rate;
if(diskread_flag < 2)
  {         
  while(!kill_all_flag &&
                    thread_status_flag[THREAD_USER_COMMAND]==THRFLAG_ACTIVE)
    {
    m=30;
    while(!kill_all_flag && m>0 && rxin_block_counter == i)
      {
      m--;
      lir_sleep(t1);
      }
   if(rxin_block_counter == i && i!=0)
      {
      sprintf(s,"No input %d",k);
      lir_text(1,screen_last_line,s);
      lir_refresh_screen();
      }
    k++;  
    if(k==10000)k=1;
    i=rxin_block_counter;
    }
  m=30;
  while(!kill_all_flag && m>0 && rxin_block_counter == i)
    {
    m--;
    lir_sleep(t1);
    }
  if(rxin_block_counter == i && i!=0)  
    {
    sprintf(s,"No input %d",k);
    lir_text(1,screen_last_line,s);
    lir_refresh_screen();
    lirerr(1284);
    }
  }
lir_join(THREAD_USER_COMMAND);
if(use_tx != 0)
  {
  linrad_thread_stop_and_join(THREAD_TX_INPUT);
  linrad_thread_stop_and_join(THREAD_TX_OUTPUT);
  close_tx_sndin();
  close_tx_sndout();
  if(txmem_handle != NULL)free(txmem_handle);
  txmem_handle=NULL;
  lir_close_event(EVENT_TX_INPUT);
  }
if((ui.network_flag&NET_RX_OUTPUT)!=0)
                          linrad_thread_stop_and_join(THREAD_NETWORK_SEND);
linrad_thread_stop_and_join(rx_input_thread);
linrad_thread_stop_and_join(THREAD_SCREEN);      
linrad_thread_stop_and_join(THREAD_RADAR);
if((ui.network_flag&NET_RX_OUTPUT)!=0)
  {
  linrad_thread_stop_and_join(THREAD_LIR_SERVER);
  } 
linrad_thread_stop_and_join(THREAD_WIDEBAND_DSP);
radar_x:;
if(ui.rx_soundcard_radio == RX_SOUNDCARD_RADIO_AFEDRI_USB &&
       diskread_flag == 0)
  {
  close_afedriusb_control();  
  }
free_semaphores();
}

void tune_wse_routine(void)
{
usercontrol_mode=USR_TUNE;
genparm[SECOND_FFT_ENABLE]=0;
get_wideband_sizes();
if(kill_all_flag)return;
clear_screen();
if(     (ui.rx_input_mode&IQ_DATA) == 0 ||
              (ui.rx_rf_channels == 1) ||
                 ui.rx_ad_speed != 96000)
  {
  settextcolor(14);
  lir_text(10,10,"Incorrect soundcard settings");
  lir_text(5,12,"Set ""Two Rx channels I/Q data"", 96 kHz and 32 bit.");
  settextcolor(7);
  lir_text(13,14,press_any_key);
  await_processed_keyboard();
  lir_status=LIR_TUNEERR;
  return;
   } 
get_buffers(1);
if(kill_all_flag || lir_status != LIR_OK)return;
// Collect data during TUNETIME seconds.
#define TUNETIME 0.1
tune_bytes=96000*TUNETIME;
if(tune_bytes < 2*fft1_size)tune_bytes=2*fft1_size;
tune_bytes*=snd[RXAD].framesize;
tune_bytes/=timf1_blockbytes;
tune_bytes++;
tune_bytes*=timf1_blockbytes;
if(tune_bytes < 2*fft1_size*(int)snd[RXAD].framesize)
  {
  lir_text(5,5,"Set fft1 bandwidth higher.");
  lir_text(5,7,press_any_key);
  await_processed_keyboard();
  return;
  }  
if(fft1_desired[0] > .99 || bal_updflag == 1)
  {
  settextcolor(14);
  lir_text(3,10,"Calibrate the RX2500 (Read z_CALIBRATE.txt)");
  settextcolor(7);
  lir_text(5,13,press_any_key);
  await_processed_keyboard();
  return;
  }
lir_inkey=0;
init_semaphores();
linrad_thread_create(rx_input_thread);
lir_sleep(100000);
if(lir_status != LIR_OK)
  {
  lir_join(rx_input_thread);
  goto tune_x;
  }
if(kill_all_flag) goto tune_x;
if((ui.network_flag&NET_RX_OUTPUT)!=0)linrad_thread_create(THREAD_NETWORK_SEND);
linrad_thread_create(THREAD_TUNE);
if(kill_all_flag) goto tune_x;
linrad_thread_create(THREAD_USER_COMMAND);
lir_sleep(50000);
lir_refresh_screen();
all_threads_started=TRUE; 
lir_join(THREAD_USER_COMMAND);
linrad_thread_stop_and_join(rx_input_thread);
if((ui.network_flag&NET_RX_OUTPUT)!=0)
                          linrad_thread_stop_and_join(THREAD_NETWORK_SEND);
linrad_thread_stop_and_join(THREAD_TUNE);
tune_x:;
free_semaphores();
}


void rx_adtest_routine(void)
{
usercontrol_mode=USR_ADTEST;
genparm[SECOND_FFT_ENABLE]=0;
get_wideband_sizes();
if(kill_all_flag)return;
get_buffers(1);
if(kill_all_flag || lir_status != LIR_OK)return;
init_semaphores();
linrad_thread_create(rx_input_thread);
lir_sleep(100000);
if(lir_status != LIR_OK)
  {
  lir_join(rx_input_thread);
  goto adtest_x;
  }
if((ui.network_flag&NET_RX_OUTPUT)!=0)linrad_thread_create(THREAD_NETWORK_SEND);
if(kill_all_flag) goto adtest_x;
linrad_thread_create(THREAD_USER_COMMAND);
if(kill_all_flag) goto adtest_x;
linrad_thread_create(THREAD_RX_ADTEST);
lir_sleep(50000);
lir_refresh_screen();
all_threads_started=TRUE; 
lir_join(THREAD_USER_COMMAND);
if((ui.network_flag&NET_RX_OUTPUT)!=0)
                          linrad_thread_stop_and_join(THREAD_NETWORK_SEND);
linrad_thread_stop_and_join(rx_input_thread);
linrad_thread_stop_and_join(THREAD_RX_ADTEST);
adtest_x:;
free_semaphores();
}

void txtest_routine(void)
{
int i;
usercontrol_mode=USR_TXTEST;
if(ui.network_flag != 0 || diskread_flag != 0)
  {
  lirerr(1158);
  return;
   }
if(  (ui.rx_input_mode&IQ_DATA) != 0)
  {
  ui.rx_ad_channels=2;
  } 
else
  {
  ui.rx_ad_channels=1;
  }
ui.rx_input_mode&=-1-TWO_CHANNELS;
ui.rx_rf_channels=1;
genparm[AFC_ENABLE]=0;
genparm[SECOND_FFT_ENABLE]=0;
get_wideband_sizes();
if(kill_all_flag) return;
// Open the graph windows on the screen.
// Note that they must be opened in this order because
// Each window is placed outside the previous ones and the init
// routines assumes the order below and does not check for all possible
// conflicts.
get_buffers(1);
if(kill_all_flag)return;
if(lir_status != LIR_OK)return;
read_freq_control_data();
init_semaphores();
init_wide_graph();
if(kill_all_flag) goto txtest_x;
for(i=0; i<txtest_no_of_segs*wg.spek_avgnum; i++)txtest_power[i]=0;
if(lir_status != LIR_OK)goto txtest_x;
if(!freq_from_file && (ui.network_flag & NET_RX_INPUT) == 0)
  {
  init_freq_control();
  }
// Make sure users_init_mode is the last window(s) we open.
// the avoid collission routine does not know where
// users windows are placed.
if(ui.rx_soundcard_radio == RX_SOUNDCARD_RADIO_ELEKTOR  &&
       diskread_flag == 0)
  {
  if(init_elektor_control_window() != EXIT_SUCCESS)ui.rx_soundcard_radio=0;
  }
if(ui.rx_soundcard_radio == RX_SOUNDCARD_RADIO_FCDPROPLUS  &&
       diskread_flag == 0)
  {
  init_fcdproplus_control_window();  
  }
if(ui.rx_soundcard_radio == RX_SOUNDCARD_RADIO_AFEDRI_USB &&
       diskread_flag == 0)
  {
  open_afedriusb_control();  
  }
phasing_init_mode();
users_init_mode();
show_name_and_size();
settextcolor(7);
new_baseb_flag=-1;
ampinfo_flag=1;
linrad_thread_create(rx_input_thread);
lir_sleep(100000);
if(lir_status != LIR_OK)
  {
  lir_join(rx_input_thread);
  goto txtest_x;
  }
if((ui.network_flag&NET_RX_OUTPUT)!=0)linrad_thread_create(THREAD_NETWORK_SEND);
if(kill_all_flag) goto txtest_x;
linrad_thread_create(THREAD_USER_COMMAND);
if(kill_all_flag) goto txtest_x;
linrad_thread_create(THREAD_TXTEST);
if(kill_all_flag) goto txtest_x;
linrad_thread_create(THREAD_WIDEBAND_DSP);
if(kill_all_flag) goto txtest_x;
linrad_thread_create(THREAD_SCREEN);
fft1_waterfall_flag=1;
lir_refresh_screen();
all_threads_started=TRUE; 
lir_join(THREAD_USER_COMMAND);
if((ui.network_flag&NET_RX_OUTPUT)!=0)
                          linrad_thread_stop_and_join(THREAD_NETWORK_SEND);
linrad_thread_stop_and_join(rx_input_thread);
linrad_thread_stop_and_join(THREAD_WIDEBAND_DSP);
linrad_thread_stop_and_join(THREAD_TXTEST);
linrad_thread_stop_and_join(THREAD_SCREEN);
free_semaphores();
if(kill_all_flag)return;
if(lir_status == LIR_POWTIM)
  {
  usercontrol_mode=USR_POWTIM;
  free_buffers();
  if(kill_all_flag)return;
  clear_screen();
  genparm[SECOND_FFT_ENABLE]=1;
  genparm[FIRST_BCKFFT_VERNR]=0;
  genparm[SECOND_FFT_VERNR]=0;
  genparm[FIRST_FFT_SINPOW] = 2;
  genparm[SECOND_FFT_SINPOW] = 2;
  genparm[AFC_ENABLE]=0;
  get_wideband_sizes();
  if(kill_all_flag)return;
  get_buffers(1);
  if(kill_all_flag)return;
  set_fft1_endpoints();
  if(kill_all_flag)return;
  if(lir_status != LIR_POWTIM)return;
  genparm[SECOND_FFT_ENABLE]=0;
  fft1_waterfall_flag=0;
  init_semaphores();
  linrad_thread_create(rx_input_thread);
  i=0;
  while(i<200 && lir_status != LIR_OK)
    {
    i++;
    lir_sleep(50000);
    }
  if(lir_status != LIR_OK)
    {
    lir_join(rx_input_thread);
    goto txtest_x;
    }
  if(kill_all_flag) goto txtest_x;
  if((ui.network_flag&NET_RX_OUTPUT)!=0)linrad_thread_create(THREAD_NETWORK_SEND);
  linrad_thread_create(THREAD_USER_COMMAND);
  if(kill_all_flag) goto txtest_x;
  linrad_thread_create(THREAD_POWTIM);
  if(kill_all_flag) goto txtest_x;
  linrad_thread_create(THREAD_WIDEBAND_DSP);
  lir_sleep(50000);
  fft1_waterfall_flag=0;
  lir_refresh_screen();
  all_threads_started=TRUE;
  lir_join(THREAD_USER_COMMAND);
  linrad_thread_stop_and_join(THREAD_POWTIM);
if((ui.network_flag&NET_RX_OUTPUT)!=0)
                          linrad_thread_stop_and_join(THREAD_NETWORK_SEND);
  linrad_thread_stop_and_join(rx_input_thread);
  linrad_thread_stop_and_join(THREAD_WIDEBAND_DSP);
  clear_keyboard();
  }
else
  {
  return;
  }
txtest_x:;
if(ui.rx_soundcard_radio == RX_SOUNDCARD_RADIO_AFEDRI_USB &&
       diskread_flag == 0)
  {
  close_afedriusb_control();  
  }
free_semaphores();
}


void normal_rx_routine(void)
{
char s[256];
int i, k, m;
int local_block_cnt;
int no_input_flag;
int wlcnt, wlcnt_max, local_workload_reset; 
double total_time1, total_time2;
double cpu_time1, cpu_time2;
lir_set_title(rxmodes[rx_mode]);
workload_counter=0;
workload=-1;
computation_pause_flag=0;
usercontrol_mode=USR_NORMAL_RX;
#if OSNUM == OSNUM_LINUX
for (i = 0; i<THREAD_MAX; i++)thread_pid[i] = 0;
#endif
init_semaphores();
enable_extio();
if(kill_all_flag) goto normal_rx_x;
if(ui.network_flag)
  {
  init_network();
  if(kill_all_flag) goto normal_rx_x;
  if( (ui.network_flag & (NET_RXOUT_FFT2+NET_RXOUT_TIMF2)) != 0 &&
                                         genparm[SECOND_FFT_ENABLE] == 0)
    {
    lirerr(1281);
    goto normal_rx_x;
    }
  }
else
  {
  get_wideband_sizes();
  if(kill_all_flag) goto normal_rx_x;
  get_buffers(1);
  netfd.rec_rx=INVSOCK;
  }
if(kill_all_flag || lir_status != LIR_OK) goto normal_rx_x;
if(!freq_from_file)
  {
  read_freq_control_data();
  }
check_filtercorr_direction();
init_wide_graph();
if( genparm[SECOND_FFT_ENABLE] != 0 )init_blanker();
if(kill_all_flag || lir_status != LIR_OK) goto normal_rx_x;
if(genparm[SECOND_FFT_ENABLE] != 0)
  {
  init_hires_graph();
  if(kill_all_flag || lir_status != LIR_OK) goto normal_rx_x;
  }
if(genparm[AFC_ENABLE] != 0 && genparm[AFC_LOCK_RANGE] != 0)
  {
  init_afc_graph();
  if(kill_all_flag || lir_status != LIR_OK) goto normal_rx_x;
  }
if(ui.rx_rf_channels == 2)
  {
  init_pol_graph();
  if(kill_all_flag || lir_status != LIR_OK) goto normal_rx_x;
  }
init_baseband_graph();
if(kill_all_flag || lir_status != LIR_OK) goto normal_rx_x;
init_coherent_graph();
if(kill_all_flag || lir_status != LIR_OK) goto normal_rx_x;
if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER && eme_flag != 0)
  {
  init_eme_graph();
  if(kill_all_flag || lir_status != LIR_OK) goto normal_rx_x;
  }
if(!freq_from_file && (ui.network_flag & NET_RX_INPUT) == 0)
  {
  init_freq_control();
  if(kill_all_flag || lir_status != LIR_OK) goto normal_rx_x;
  }
if(fft1_correlation_flag==2)
  {
  init_siganal_graph();
  }
if(use_tx != 0)init_tx_graph();
if(lir_status != LIR_OK) goto normal_rx_x;
// Make sure users_init_mode is the last window(s) we open.
// the avoid collission routine does not know where
// users windows are placed.
if(ui.rx_soundcard_radio == RX_SOUNDCARD_RADIO_ELEKTOR  &&
       diskread_flag == 0)
  {
  if(init_elektor_control_window() != EXIT_SUCCESS)ui.rx_soundcard_radio=0;
  }
if(ui.rx_soundcard_radio == RX_SOUNDCARD_RADIO_FCDPROPLUS  &&
       diskread_flag == 0)
  {
  init_fcdproplus_control_window();  
  }
if(ui.rx_soundcard_radio == RX_SOUNDCARD_RADIO_AFEDRI_USB &&
       diskread_flag == 0)
  {
  open_afedriusb_control();  
  }
phasing_init_mode();
users_init_mode();
if(kill_all_flag || lir_status != LIR_OK) goto normal_rx_x;
show_name_and_size();
lir_refresh_screen();
fft3_show_time=current_time();
fft1_show_time=fft3_show_time;
sys_func(THRFLAG_PORTAUDIO_STARTSTOP);
if(kill_all_flag) goto normal_rx_x;
linrad_thread_create(THREAD_USER_COMMAND);
if(kill_all_flag) goto normal_rx_x;
linrad_thread_create(THREAD_NARROWBAND_DSP);
if(kill_all_flag) goto normal_rx_x;
linrad_thread_create(THREAD_WIDEBAND_DSP);
if(kill_all_flag) goto normal_rx_x;
linrad_thread_create(THREAD_SCREEN);
if(kill_all_flag) goto normal_rx_x;
if((ui.network_flag&NET_RX_OUTPUT)!=0)
  {
  linrad_thread_create(THREAD_LIR_SERVER);
  if(kill_all_flag) goto normal_rx_x;
  }
// Make sure that all the threads are running before
// opening inputs in order to avoid overrun errors.
// Some system calls they might do could cause overrun
// errors if the input were open.
while(thread_status_flag[THREAD_RX_OUTPUT]!=THRFLAG_SEMCLEAR &&
          thread_status_flag[THREAD_USER_COMMAND]!=THRFLAG_ACTIVE &&
          thread_status_flag[THREAD_NARROWBAND_DSP]!=THRFLAG_SEM_WAIT &&
          thread_status_flag[THREAD_WIDEBAND_DSP]!=THRFLAG_ACTIVE )
  {
  if(kill_all_flag) goto normal_rx_x;
  lir_sleep(3000);
  }
mailbox[0]=0;
mailbox[1]=0;
mailbox[2]=0;
if(kill_all_flag) goto normal_rx_x;
linrad_thread_create(rx_input_thread);
i=0;
while(!kill_all_flag && 
              thread_status_flag[rx_input_thread] != THRFLAG_ACTIVE)
  {
  lir_sleep(10000);
  i++;
  if(i>1000)
    {
    lirerr(1450);
    goto normal_rx_x;
    }
  }
if(kill_all_flag) goto normal_rx_x;
if((ui.network_flag&NET_RX_OUTPUT)!=0)linrad_thread_create(THREAD_NETWORK_SEND);
display_rx_input_source(s);
linrad_thread_create(THREAD_RX_OUTPUT);
i=0;
while(!kill_all_flag && 
              thread_status_flag[THREAD_RX_OUTPUT] != THRFLAG_ACTIVE)
  {
  lir_sleep(3000);
  i++;
  if(i>1000)
    {
    lirerr(116711);
    }
  }
if(kill_all_flag) goto normal_rx_x;
thread_command_flag[THREAD_RX_OUTPUT]=THRFLAG_IDLE;
i=0;
while(!kill_all_flag && thread_status_flag[THREAD_RX_OUTPUT] != THRFLAG_IDLE)
  {
  lir_sleep(20000);
  i++;
  if(i>2000)
    {
    lirerr(1167);
    goto normal_rx_x;
    }
  }
if(kill_all_flag) goto normal_rx_x;
thread_command_flag[THREAD_RX_OUTPUT]=THRFLAG_SEMCLEAR;
while(thread_status_flag[THREAD_RX_OUTPUT]!=THRFLAG_SEMCLEAR)
  {
  if(kill_all_flag) goto normal_rx_x;
  lir_sleep(3000);
  }
#if SHOW_HARDWARE == TRUE
lir_text(25,screen_last_line,s);
#endif
lir_sleep(100000);
if(lir_status != LIR_OK)
  {
  linrad_thread_stop_and_join(THREAD_USER_COMMAND);
  goto normal_rx_join;
  }
if(kill_all_flag) goto normal_rx_x;
if(use_tx != 0 )
  {
  switch (rx_mode)
    {
    case MODE_SSB:
    init_txmem_spproc();
    break;

    case MODE_WCW:
    case MODE_NCW:
    case MODE_HSMS:
    case MODE_QRSS:
    init_txmem_cwproc();
    break;

    case MODE_RADAR:
    init_txmem_cwproc();
    break;
    }
  if(kill_all_flag)goto normal_rx_x;
  linrad_thread_create(THREAD_TX_OUTPUT);
  while(thread_status_flag[THREAD_TX_OUTPUT]!=THRFLAG_ACTIVE)
    {
    if(kill_all_flag) goto normal_rx_x;
    lir_sleep(3000);
    }
  if(kill_all_flag) goto normal_rx_x;
  lir_refresh_screen();
  linrad_thread_create(THREAD_TX_INPUT);
  while(thread_status_flag[THREAD_TX_INPUT]!=THRFLAG_ACTIVE)
    {
    if(kill_all_flag) goto normal_rx_x;
    lir_sleep(3000);
    }
  }
lir_refresh_screen();
lir_sched_yield();
#if OSNUM == OSNUM_WINDOWS
if(ui.timer_resolution > 0)timeBeginPeriod(ui.timer_resolution);
#endif
fft1_waterfall_flag=1;
setup_thread_affinities();
lir_sleep(50000);
if(kill_all_flag) goto normal_rx_x;
lir_refresh_screen();
all_threads_started=TRUE; 
while(!kill_all_flag && 
               thread_status_flag[rx_input_thread]!=THRFLAG_ACTIVE)
  {
  lir_sleep(3000);
  }
rxin_block_counter=0;
i=0;
k=0;
wlcnt_max=1+snd[RXAD].interrupt_rate;
wlcnt=wlcnt_max;
local_workload_reset=workload_reset_flag;
lir_system_times(&cpu_time1, &total_time1);
#if OSNUM == OSNUM_LINUX
current_time();
for(i=0; i<THREAD_MAX; i++)
  {
  thread_tottim1[i]=recent_time;
  thread_cputim1[i]=0;
  }
#endif
local_block_cnt=0;
no_input_flag=FALSE;
while( !kill_all_flag &&
               thread_status_flag[THREAD_USER_COMMAND]==THRFLAG_ACTIVE &&
               thread_status_flag[rx_input_thread] != THRFLAG_RETURNED)
  {
  m=20;
  lir_sleep(50000);
  while(!kill_all_flag && m>0 && rxin_block_counter == local_block_cnt)
    {
    m--;
    if(local_workload_reset != workload_reset_flag)goto loadprt;
    lir_sleep(50000);
    }
  if( (diskread_flag < 2 || diskread_pause_flag==0) &&
        rxin_block_counter == local_block_cnt/* && local_block_cnt==0*/)
    {
    wlcnt=wlcnt_max;
    sprintf(s,"No input %d",k);
    no_input_flag=TRUE;
    lir_text(20,screen_last_line,s);
    lir_refresh_screen();
    }
  if(diskwrite_flag)
    {
    lir_sync();
    }
  wlcnt-=rxin_block_counter-local_block_cnt;
  if(wlcnt <= 0)
    {
    if(no_input_flag)
      {
      memset(s,' ',20);
      s[20]=0;
      lir_text(20,screen_last_line,s);
      no_input_flag=FALSE;
      }
loadprt:;
    fix_thread_affinities();
    wlcnt=wlcnt_max;
// *******************************************************************
// Compute the workload. We should arrive here at a rate of about 1 Hz.
// Modern Linux kernels (2.6.8 or 2.6.9 and later) as well as Windows
// allow us to compute the work load from here.
    current_time();
    for(i=0; i<THREAD_MAX; i++)
      {
#if OSNUM == OSNUM_LINUX
      if(thread_pid[i] != 0)
#endif
#if OSNUM == OSNUM_WINDOWS
      if(thread_command_flag[i]!=THRFLAG_NOT_ACTIVE)
#endif
        {
        thread_tottim2[i]=recent_time;
        thread_cputim2[i]=lir_get_thread_time(i);
        thread_workload[i]=100*(thread_cputim2[i]-thread_cputim1[i])/
                                  (thread_tottim2[i]-thread_tottim1[i]);
        }  
      }        
    lir_system_times(&cpu_time2, &total_time2);
    workload=100*(cpu_time2-cpu_time1)/(total_time2-total_time1);
#if OSNUM == OSNUM_LINUX
    workload/=no_of_processors;
#endif
    if(workload<0)workload=0;
#if OSNUM == OSNUM_LINUX
    lir_fix_bug(1);
#endif
    if(local_workload_reset != workload_reset_flag)
      {
      local_workload_reset=workload_reset_flag;
      total_time1=total_time2;
      cpu_time1=cpu_time2;
      for(i=0; i<THREAD_MAX; i++)
        {
        if(thread_command_flag[i]!=THRFLAG_NOT_ACTIVE)
          {
          thread_tottim1[i]=thread_tottim2[i];
          thread_cputim1[i]=thread_cputim2[i];
          }  
        }  
      }
    workload_counter++;
    awake_screen();
    }
  k++;  
  if(k==10000)k=1;
  local_block_cnt=rxin_block_counter;
  }
lir_join(THREAD_USER_COMMAND);
if(diskwrite_flag != 0)disksave_stop();
#if OSNUM == OSNUM_WINDOWS
if(ui.timer_resolution > 0)timeEndPeriod(ui.timer_resolution);
#endif
if(use_tx != 0)
  {
  linrad_thread_stop_and_join(THREAD_TX_INPUT);
  linrad_thread_stop_and_join(THREAD_TX_OUTPUT);
  close_tx_sndin();
  close_tx_sndout();
  if(txmem_handle != NULL)free(txmem_handle);
  txmem_handle=NULL;
  lir_close_event(EVENT_TX_INPUT);
  }
normal_rx_join:;
linrad_thread_stop_and_join(THREAD_RX_OUTPUT);
if((ui.network_flag&NET_RX_OUTPUT)!=0)
                          linrad_thread_stop_and_join(THREAD_NETWORK_SEND);
linrad_thread_stop_and_join(rx_input_thread);
linrad_thread_stop_and_join(THREAD_SCREEN);      
linrad_thread_stop_and_join(THREAD_NARROWBAND_DSP);
if((ui.network_flag&NET_RX_OUTPUT)!=0)
  {
  linrad_thread_stop_and_join(THREAD_LIR_SERVER);
  } 
linrad_thread_stop_and_join(THREAD_WIDEBAND_DSP);
normal_rx_x:;
if(ui.rx_soundcard_radio == RX_SOUNDCARD_RADIO_AFEDRI_USB &&
       diskread_flag == 0)
  {
  close_afedriusb_control();  
  }
if(!kill_all_flag)disable_extio();
free_semaphores();
}

void prompt_reason(char *s)
{
clear_screen();
lir_text(5,8,"You are prompted to the parameter selection screens");
lir_text(15,10,"for the following reason:");
lir_text(5,13,s);
lir_text(20,16,press_any_key);
clear_keyboard();
await_keyboard();
}

void check_output_no_of_channels(void)
{
int i;
i=1+((genparm[OUTPUT_MODE]>>1)&1);
if(i<ui.rx_min_da_channels)i=ui.rx_min_da_channels;
if(i>ui.rx_max_da_channels)i=ui.rx_max_da_channels;
i--;
genparm[OUTPUT_MODE]&=-3;
genparm[OUTPUT_MODE]|=i<<1;            
}

void fix_limits(int *k, int *m, int parnum)
{
int i;
k[0]=genparm_max[parnum];
m[0]=genparm_min[parnum];
if(parnum == FIRST_FFT_VERNR)
  {
  k[0]=0;
  fft1mode=(ui.rx_input_mode&(TWO_CHANNELS+IQ_DATA))/2;
  while( fft1_version[fft1mode][k[0]+1] >= 0 &&
                                k[0] < MAX_FFT1_VERNR-1)k[0]++;
  if(simd_present == 0)
    {
    while( fft_cntrl[fft1_version[fft1mode][k[0]]].simd != 0)k[0]--;
    }
  if(mmx_present == 0)
    {
    while( fft_cntrl[fft1_version[fft1mode][k[0]]].mmx != 0)k[0]--;
    }
  }
if(parnum == FIRST_BCKFFT_VERNR)
  {
  i=0;  
  while(fft1_back_version[ui.rx_rf_channels-1][i+1] > 0 &&
                              i < MAX_FFT1_BCKVERNR-1)i++;
  if(mmx_present == 0)
    {
    while(i > 0 && fft_cntrl[fft1_back_version[ui.rx_rf_channels-1][i]].mmx != 0)i--;
    }
  k[0]=i;
  }
if(parnum == FIRST_BCKFFT_ATT_N)
  {
  k[0]=(fft1_n-4)&0xfffe;
  }
if(parnum == SECOND_FFT_ATT_N)
  {
  k[0]=fft2_n-2;
  }
if(parnum == SECOND_FFT_VERNR)
  {
  i=0;
  while( i < MAX_FFT2_VERNR-1 &&
         fft2_version[ui.rx_rf_channels-1][i+1] > 0)i++;
  if(mmx_present == 0)
    {
    while(i > 0 && fft_cntrl[fft2_version[ui.rx_rf_channels-1][i]].mmx != 0)i--;
    }
  k[0]=i;  
  }
if(parnum == MIX1_BANDWIDTH_REDUCTION_N)
  {
  if(genparm[SECOND_FFT_ENABLE] == 0)
    {
    k[0]=fft1_n-3;
    }
  else
    {
    k[0]=fft2_n-3;
    }  
  }
if(parnum == DA_OUTPUT_SPEED)
  {
  m[0]=ui.rx_min_da_speed;
  k[0]=ui.rx_max_da_speed;
  }
if(parnum == MAX_NO_OF_SPURS)
  {
  if(genparm[AFC_ENABLE]==0)
    {
    k[0]=0;
    }
  else
    {  
    if(genparm[SECOND_FFT_ENABLE] == 0)
      {
      k[0]=2*fft1_size/SPUR_WIDTH;
      }
    else
      {
      k[0]=2*fft2_size/SPUR_WIDTH;
      }
    }  
  }
if(parnum == SPUR_TIMECONSTANT)
  {
  if(genparm[SECOND_FFT_ENABLE] == 0)
    {
    k[0]=5*genparm[FFT1_STORAGE_TIME];
    }
  else
    {
    k[0]=5*genparm[FFT2_STORAGE_TIME];
    }
  }
// In case fft sizes are not set. 
if(k[0] < m[0] || m[0]<genparm_min[parnum] || k[0]>genparm_max[parnum])
  { 
  k[0]=genparm_max[parnum];
  m[0]=genparm_min[parnum];
  }
}

void modify_parms(char *line1, int first, int last)
{
unsigned int new_sample_rate;
char s[80];
int line1_len, line;
int i, j, k, m, no, mouse_line,parnum;
line1_len=strlen(line1)+8;
no=last-first+1;
start:;
hide_mouse(0,screen_width,0,screen_height);
clear_screen();
settextcolor(14);
lir_text(5,1, line1);
settextcolor(7);
// Make sure fft1_n and fft2_n are defined and 
// in agreement with current parameters.
get_wideband_sizes();
if(kill_all_flag)return;
if( first <= FIRST_FFT_SINPOW && last >= FIRST_FFT_SINPOW)
  {
  sprintf(s,"fft1 size=%d (Bw=%fHz) %s",fft1_size,fft1_bandwidth,
                                              fft_cntrl[FFT1_CURMODE].text);
  lir_text(line1_len,1,s);
  }    
if( first <= SECOND_FFT_SINPOW && last >= SECOND_FFT_SINPOW)
  {
  sprintf(s,"fft2 size=%d (Bw=%fHz)",fft2_size,fft2_bandwidth);
  lir_text(line1_len,1,s);
  }    
if(kill_all_flag) return;
if( first <= DA_OUTPUT_SPEED && last >= DA_OUTPUT_SPEED)
  {
  if(genparm[DA_OUTPUT_SPEED] < ui.rx_min_da_speed)
                              genparm[DA_OUTPUT_SPEED]=ui.rx_min_da_speed;
  
  if(genparm[DA_OUTPUT_SPEED] > ui.rx_max_da_speed)
                             genparm[DA_OUTPUT_SPEED]=ui.rx_max_da_speed; 
 
  }
if( first <= CW_DECODE_ENABLE && last >= CW_DECODE_ENABLE)
  {
  if(rx_mode==MODE_WCW || rx_mode==MODE_NCW || rx_mode==MODE_HSMS)
    {
    if(genparm[CW_DECODE_ENABLE] != 0)
      {
      settextcolor(12);
      lir_text(1,14,"WARNING: The Morse decode routines are incomplete.");
      lir_text(1,15,"They will not produce any useful output and may cause");
      lir_text(1,16,"a program crasch. Use only for development and perhaps");
      lir_text(1,17,"for some evaluation of chirp and other keying defects");
      lir_text(1,18,"with the coherent graph oscilloscope.");
      }
    }
  else
    {
    genparm[CW_DECODE_ENABLE]=0;
    }
  }
if( first <= FIRST_BCKFFT_ATT_N && last >= FIRST_BCKFFT_ATT_N)
  {
  k=(fft1_n-4)&0xfffe;
  if(genparm[FIRST_BCKFFT_ATT_N]>k)genparm[FIRST_BCKFFT_ATT_N]=k;
  if(genparm[FIRST_BCKFFT_ATT_N]<0)genparm[FIRST_BCKFFT_ATT_N]=
                                             genparm_min[FIRST_BCKFFT_ATT_N];
  }
if( first <= SECOND_FFT_ATT_N && last >= SECOND_FFT_ATT_N)
  {
  k=fft2_n-2;
  if(genparm[SECOND_FFT_ATT_N]>k)genparm[SECOND_FFT_ATT_N]=k;
  }
settextcolor(7);
line=0;
for(i=0; i<no; i++)
  {
  j=i+first;
  if(ui.operator_skil == OPERATOR_SKIL_NEWCOMER && newco_genparm[j] ==0)
    {
    settextcolor(8);
    }
  else
    {
    settextcolor(7);
    }
  if((ui.rx_input_mode&TWO_CHANNELS) == 0 && j==FFT1_CORRELATION_SPECTRUM)
    {
    settextcolor(8);
    }
  if(j == FIRST_FFT_BANDWIDTH)
    {
    sprintf(s,"%s [%.2f] ",genparm_text[j],0.01F*(float)genparm[j]);
    }
  else
    {  
    sprintf(s,"%s [%d] ",genparm_text[j],genparm[j]);
    }
  lir_text(1,3+i, s);
  }
lir_text(1,5+no,"Use left mouse button to select line");  
lir_text(1,3+no,"CONTINUE");  
settextcolor(15);
show_mouse();
modloop:;
if( new_mouse_x!=mouse_x || new_mouse_y!=mouse_y)
  {
  lir_move_mouse_cursor();
  show_mouse();
  }
lir_refresh_screen();
lir_sleep(10000);
if(new_lbutton_state==1)lbutton_state=1;
if(new_lbutton_state==0 && lbutton_state==1)
  {
  lbutton_state=0;
  mouse_line=mouse_y/text_height-3;
  if(mouse_line == no)goto loopx;
  if(mouse_line >= 0 && mouse_line <no)
    {
agn:;
    parnum=mouse_line+first;
    clear_screen();
    if((ui.rx_input_mode&TWO_CHANNELS) == 0 && 
                                         parnum ==FFT1_CORRELATION_SPECTRUM)
      {
      goto start;
      }
    settextcolor(14);
    lir_text(5,1, line1);
    line=3;
    settextcolor(15);
    if(parnum == FIRST_FFT_BANDWIDTH)
      {
      sprintf(s,"Old value = %.2f",0.01F*(float)genparm[parnum]);
      }
    else
      {
      sprintf(s,"Old value = %d",genparm[parnum]);
      }
    lir_text(1,line, s);
    line++;
    lir_text(1,line,"Enter new value for:");
    line++;
    fix_limits(&k, &m, parnum);
    if(parnum == FIRST_FFT_BANDWIDTH)
      {
      sprintf(s," %s (%.2f to %.2f)",genparm_text[parnum], 
                                        0.01F*(float)m,0.01F*(float)k);
      }
    else
      {  
      sprintf(s," %s (%d to %d)",genparm_text[parnum], m,k);
      }
    lir_text(1,line, s);
    i=line+1;
    line+=4;
    msg_filename="help.lir";
    write_from_msg_file(&line, 201+mouse_line+first, TRUE, HELP_VERNR);
    if(parnum == DA_OUTPUT_SPEED && ui.rx_dadev_no != DISABLED_DEVICE_CODE)
      {
#if(OSNUM == OSNUM_LINUX)
      if (( (ui.use_alsa&NATIVE_ALSA_USED) != 0)  && ((ui.use_alsa&PORTAUDIO_RX_OUT)== 0) )
        {
        alsa_get_native_samplerate(ui.rx_dadev_no,SND_PCM_STREAM_PLAYBACK,&line,&new_sample_rate);
        genparm[parnum]=new_sample_rate;
        goto modify_parms_next;
        } 
      else
#endif
        {
        if( (ui.use_alsa&PORTAUDIO_RX_OUT) != 0 && 
             ui.rx_dadev_no != DISABLED_DEVICE_CODE)
          {
          pa_get_valid_samplerate(ui.rx_dadev_no,RXDA,&line,&new_sample_rate);
          genparm[parnum]=new_sample_rate;
          goto modify_parms_next;
          }
        }  
      }
    lir_text(7,i,"=>");
    if(parnum == FIRST_FFT_BANDWIDTH)
      {
      genparm[parnum]=(int)(100.F*lir_get_float(10, i, 8, 
                                              0.01F*(float)m,0.01F*(float)k));
      }
    else
      {
      genparm[parnum]=lir_get_integer(10, i, 8, m,k);
      }
    if(parnum == FIRST_FFT_VERNR)
      {
#if OPENCL_PRESENT == 1
      if(!ocl_active)
        {
        if(fft_cntrl[fft1_version[fft1mode][genparm[parnum]]].gpu == GPU_OCL)
          {
          goto agn;
          }
        }
#else
      if(fft_cntrl[fft1_version[fft1mode][genparm[parnum]]].gpu == GPU_OCL)
        {
        goto agn;
        }
#endif
#if HAVE_CUFFT != 1
      if(fft_cntrl[fft1_version[fft1mode][genparm[parnum]]].gpu == GPU_CUDA)
        {
        goto agn;
        }
#endif
      } 
modify_parms_next:;
    if(kill_all_flag) return;
    if(parnum == OUTPUT_MODE)check_output_no_of_channels();
    goto start;
    }
  }
test_keyboard();
if(kill_all_flag) return;
if(lir_inkey != 0)
  {
  process_current_lir_inkey();
  if(kill_all_flag) return;
  }
if(lir_inkey == F1_KEY || lir_inkey == '!')
  {
  mouse_line=mouse_y/text_height-3;
  if(mouse_line >= 0 && mouse_line <no)
    {
    help_message(201+mouse_line+first);
    }
  else
    {
    help_message(200);
    }
  if(kill_all_flag) return;
  goto start;
  }
if(lir_inkey != 10 && lir_inkey!= 'X')goto modloop;
loopx:;
}

void set_general_parms(char *mode)
{
char s[80];
sprintf(s,"%s: Rx channels=%d",mode,ui.rx_rf_channels);
if(lir_status < LIR_OK)goto bufreduce;
if((ui.rx_input_mode&TWO_CHANNELS) == 0)genparm[FFT1_CORRELATION_SPECTRUM]=0;
setfft1:;
if(kill_all_flag) return;
modify_parms(s, 0, SECOND_FFT_ENABLE);
if(kill_all_flag) return;
if(lir_inkey == 'X')return;
// Make sure fft1_n and fft2_n are defined and that we can
// allocate memory.
get_wideband_sizes();
if(kill_all_flag) return;
get_buffers(0);
if(kill_all_flag) return;
if(fft1_handle != NULL)fft1_handle=chk_free(fft1_handle);
if(lir_status != LIR_OK)goto bufreduce;
if(genparm[SECOND_FFT_ENABLE]==1)
  {
  modify_parms(s, FIRST_BCKFFT_VERNR, FFT2_STORAGE_TIME);
  if(kill_all_flag) return;
  if(lir_inkey == 'X')return;
  get_wideband_sizes();
  if(kill_all_flag) return;
  get_buffers(0);
  if(kill_all_flag) return;
  if(fft1_handle != NULL)fft1_handle=chk_free(fft1_handle);
  if(lir_status != LIR_OK)
    {
bufreduce:;
    clear_screen();
    settextcolor(15);
    switch (lir_status)
      {
      case LIR_FFT1ERR:
      lir_text(5,5,"Out of memory !!!");
      lir_text(10,10,"Storage times are set to minimum.");
      settextcolor(14);
      lir_text(10,13,"Check memory allocations in waterfall window");
      lir_text(10,14,"to decide how much you may increase storage times.");
      genparm[FFT1_STORAGE_TIME]=genparm_min[FFT1_STORAGE_TIME];
      genparm[FFT2_STORAGE_TIME]=genparm_min[FFT2_STORAGE_TIME];
      genparm[BASEBAND_STORAGE_TIME]=genparm_min[BASEBAND_STORAGE_TIME];
      lir_status=LIR_OK;
      break;
      
      case LIR_SPURERR:
      sprintf(s,"fft1 storage time too short for spur removal");
      if(genparm[SECOND_FFT_ENABLE] != 0)s[3]='2';
      lir_text(7,7,s);
      lir_text(7,8,"Spur removal disabled");
      genparm[MAX_NO_OF_SPURS]=0;
      lir_status=LIR_OK;
      break;

      case LIR_NEW_SETTINGS:
      goto setfft1;
      }      
    settextcolor(7);
    lir_text(10,17,"Press ESC to quit, any other key to continue");
    await_processed_keyboard();
    if(kill_all_flag) return;
    goto setfft1;
    }
  }
if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
  {
  modify_parms(s, AFC_ENABLE, AFC_ENABLE);
  if(kill_all_flag) return;
  if(lir_inkey == 'X')return;
  if(genparm[AFC_ENABLE] != 0)modify_parms(s, AFC_ENABLE+1, SPUR_TIMECONSTANT);
  if(kill_all_flag) return;
  if(lir_inkey == 'X')return;
  }
modify_parms(s, MIX1_BANDWIDTH_REDUCTION_N, MAX_GENPARM-1);
if(kill_all_flag) return;
if(lir_inkey == 'X')return;
clear_screen();
}

void cal_package(void)
{
char s[80], ss[256];
int single_run;
int i, ia, ib, ic;
float t1,t2;
calibrate_flag = 1;
// **************************************************************
// Set fft1_direction positive.
// The calibration routine does not want to know if the fft1 routine
// will invert the frequency scale.
fft1_direction=1; 
single_run=0;
// Get normal buffers with minimum for all parameters.
// Everything selectable becomes deselected.
// Save fft1 version, window and bandwidth.
ia=genparm[FIRST_FFT_VERNR];
ib=genparm[FIRST_FFT_SINPOW];
ic=genparm[FIRST_FFT_BANDWIDTH];
for(i=0; i<MAX_GENPARM; i++)genparm[i]=genparm_min[i];
// Select the correct fft version for the current rx_mode
// This is not really needed for versions above Linrad-01.xx
// because the approximate fft has been removed. None of the
// fft implementations contains a filter any more. 
genparm[FIRST_FFT_VERNR]=ia;
// Force fft1_size to be 4 times larger than specified for the current
// rx_mode by dividing bandwidth by 4.
t2=ic/4;
// Compensate for not using the specified window
// We use sin power 4 to suppress wideband noise that otherwise would
// be produced by discontinuities in matching between transform ends.
t1=pow(0.5,1.0/ib);
t2*=(1-2*asin(t1)/PI_L);
t1=pow(0.5,1.0/4);
t2/=(1-2*asin(t1)/PI_L);
genparm[FIRST_FFT_SINPOW]=4;
i=t2+0.5;  
if(i < genparm_min[FIRST_FFT_BANDWIDTH])i=genparm_min[FIRST_FFT_BANDWIDTH];
genparm[FIRST_FFT_BANDWIDTH]=i;
clear_screen();
get_wideband_sizes();
if(kill_all_flag) return;
get_buffers(1);
if(kill_all_flag) return;
if(lir_status != LIR_OK)return;
wg.first_xpoint=0;
wg.xpoints=fft1_size;
set_fft1_endpoints();
if(fft1afc_flag == 0)fft1afc_flag=-1;
init_memalloc(calmem, MAX_CAL_ARRAYS);
mem( 1,&cal_graph,2*MAX_ADCHAN*screen_width*sizeof(short int),0);
mem( 2,&cal_table,fft1_size*sizeof(COSIN_TABLE )/2,0);
mem( 3,&cal_permute,fft1_size*sizeof(int),0);
mem(22,&fft2_tab,fft1_size*sizeof(COSIN_TABLE )/2,0);
mem(23,&fft2_permute,fft1_size*sizeof(short int),0);
mem( 4,&cal_win,(1+fft1_size/2)*sizeof(float),0);
mem( 5,&cal_tmp,twice_rxchan*fft1_size*sizeof(float),0);
mem( 6,&cal_buf,twice_rxchan*fft1_size*sizeof(float),0);
mem( 7,&bal_flag,(BAL_MAX_SEG+1)*sizeof(int),0);
mem( 8,&bal_pos,BAL_AVGNUM*(BAL_MAX_SEG+1)*ui.rx_rf_channels*sizeof(int),0);
mem( 9,&bal_phsum,BAL_AVGNUM*(BAL_MAX_SEG+1)*ui.rx_rf_channels*sizeof(float),0);
mem(10,&bal_amprat,BAL_AVGNUM*(BAL_MAX_SEG+1)*ui.rx_rf_channels*sizeof(float),0);
mem(11,&contracted_iq_foldcorr,8*(BAL_MAX_SEG+1)*ui.rx_rf_channels*
                                                            sizeof(float),0);
mem(12,&cal_buf2,twice_rxchan*fft1_size*sizeof(float),0);
mem(13,&cal_buf3,twice_rxchan*fft1_size*sizeof(float),0);
mem(14,&cal_buf4,twice_rxchan*fft1_size*sizeof(float),0);
mem(15,&cal_buf5,twice_rxchan*fft1_size*sizeof(float),0);
mem(16,&cal_buf6,twice_rxchan*fft1_size*sizeof(float),0);
mem(17,&cal_buf7,twice_rxchan*fft1_size*sizeof(float),0);
mem(47,&cal_fft1_filtercorr,twice_rxchan*fft1_size*sizeof(float),0);
mem(48,&cal_fft1_desired,fft1_size*sizeof(float),0);
mem(49,&cal_fft1_sumsq,fft1_size*ui.rx_rf_channels*sizeof(float),0);
mem(50,&cal_fft1_slowsum,fft1_size*ui.rx_rf_channels*sizeof(float),0);
i=memalloc(&calmem_handle,"calmem");
if(i==0)
  {
  lirerr(1188);
  return;
  }
if(ui.rx_rf_channels == 1)
  {
  cal_ymax=.25;
  cal_yzer=.65;
  }
else
  {
  cal_ymax=.15;
  cal_yzer=.5;
  }
make_bigpermute(0, fft1_n, fft1_size, cal_permute);
make_sincos(0, fft1_size, cal_table); 
make_window(2,fft1_size/2, 4, cal_win);
iqbeg:;
single_run++;
if(single_run >=2)goto cal_skip;
fft1_pa=fft1_block;
fft1_na=1;
fft1_pb=0;
fft1_px=0;
fft1_nx=0;
timf1p_pa=snd[RXAD].block_bytes;
rxin_isho=(short int*)(&timf1_char[timf1p_pa]);
rxin_int=(int*)(&timf1_char[timf1p_pa]);
rxin_char=(char*)(&timf1_char[timf1p_pa]);
timf1p_px=0;
if(kill_all_flag) goto cal_skip;
cal_type=CAL_TYPE_MENU;
cal_initscreen();
if( (ui.rx_input_mode&IQ_DATA) != 0)
  {
  if( (ui.rx_input_mode&DIGITAL_IQ) == 0)
    {
    lir_text(1, 5,"Running in IQ mode (direct conversion receiver)");
    lir_text(1, 6,"The I/Q phase and amplitude should be calibrated before");
    lir_text(1, 7,"the total amplitude and phase response is calibrated");
    lir_text(5, 9,"A=> Calibrate I/Q phase and amplitude.");
    }
  lir_text(5,10,"B=> Calibrate total amplitude and phase");
  lir_text(5,11,"C=> Remove center discontinuity");
  lir_text(5,12,"D=> Refine amplitude and phase correction");
  lir_text(5,13,"X=> Skip");
  lir_text(5,14,"F1 or !=> Help");
get_kbd:;
  await_processed_keyboard();
  if(kill_all_flag) goto cal_skip;
  switch (lir_inkey)
    {
    case 'X':
    goto cal_skip;

    case 'A':
    if( (ui.rx_input_mode&DIGITAL_IQ) != 0)break;
    if( (fft1_calibrate_flag&CALAMP)==CALAMP)
      {
      clear_screen();
      lir_text(5,5,"The amplitudes are already calibrated.");
      make_filfunc_filename(s);
      sprintf(ss,"Exit from Linrad and remove the file %s",s);
      lir_text(1,6,ss);
      lir_text(5,8,press_any_key);
      await_keyboard();
      break;
      }
    usercontrol_mode=USR_IQ_BALANCE;
    init_semaphores();
    ampinfo_flag=1;
    if( (ui.network_flag&NET_RX_INPUT) != 0)
      {
      init_network();
      if(kill_all_flag) goto cal_skip;
      }
    linrad_thread_create(rx_input_thread);
    lir_sleep(100000);
    if(lir_status != LIR_OK)
      {
      lir_join(rx_input_thread);
      goto cal_skip_freesem;
      }
    if((ui.network_flag&NET_RX_OUTPUT)!=0)linrad_thread_create(THREAD_NETWORK_SEND);
    if(kill_all_flag) goto cal_skip_freesem;
    linrad_thread_create(THREAD_USER_COMMAND);
    if(kill_all_flag) goto iqbal;
    linrad_thread_create(THREAD_CAL_IQBALANCE);
    if(kill_all_flag) goto iqbal;
    linrad_thread_create(THREAD_WIDEBAND_DSP);
iqbal:;    
    lir_sleep(50000);
    lir_refresh_screen();
    lir_join(THREAD_USER_COMMAND);
if((ui.network_flag&NET_RX_OUTPUT)!=0)
                          linrad_thread_stop_and_join(THREAD_NETWORK_SEND);
    linrad_thread_stop_and_join(rx_input_thread);
    linrad_thread_stop_and_join(THREAD_CAL_IQBALANCE);
    linrad_thread_stop_and_join(THREAD_WIDEBAND_DSP);
    free_semaphores();
    if(kill_all_flag) goto cal_skip;
    break;

    case 'B':
    goto pulsecal;

    case 'C':
    if(remove_iq_notch() == 0)goto cal_skip;
    break;

    case 'D':
    final_filtercorr_init();
    goto cal_skip;

    case F1_KEY:
    case '!':
    help_message(302);
    break;

    default:
    goto get_kbd;
    }
  }    
else
  {
  lir_text(5, 9,"A=> Calibrate frequency response");
  lir_text(5,10,"B=> Refine amplitude and phase correction");
  lir_text(5,11,"X=> Skip");
  lir_text(5,12,"F1 or !=> Help");
get_valid_keypress:;
  await_processed_keyboard();
  if(kill_all_flag) goto cal_skip;
  switch (lir_inkey)
    {
    case 'X':
    goto cal_skip;
        
    case 'A':
    goto pulsecal;

    case 'B':
    final_filtercorr_init();
    goto cal_skip;

    case F1_KEY:
    case '!':
    help_message(303);
    break;

    default:
    goto get_valid_keypress;
    }
  }  
goto iqbeg;
pulsecal:;
enable_extio();
timf1p_pa=0;
rxin_isho=(short int*)timf1_char;
rxin_int=(int*)timf1_char;
rxin_char=(char*)timf1_char;
usercontrol_mode=USR_CAL_INTERVAL;
init_semaphores();
ampinfo_flag=1;
if( (ui.network_flag&NET_RX_INPUT) != 0)
  {
  init_network();
  if(kill_all_flag) goto cal_skip;
  }
linrad_thread_create(rx_input_thread);
lir_sleep(100000);
if(lir_status != LIR_OK)
  {
  lir_join(rx_input_thread);
  goto cal_skip_freesem;
  }
if((ui.network_flag&NET_RX_OUTPUT)!=0)linrad_thread_create(THREAD_NETWORK_SEND);
if(kill_all_flag) goto cal_skip_freesem;
linrad_thread_create(THREAD_USER_COMMAND);
if(kill_all_flag) goto calint;
linrad_thread_create(THREAD_CAL_INTERVAL);
if(kill_all_flag)goto cal_skip;
calint:;
lir_sleep(50000);
lir_refresh_screen();
lir_join(THREAD_USER_COMMAND);
if((ui.network_flag&NET_RX_OUTPUT)!=0)
                          linrad_thread_stop_and_join(THREAD_NETWORK_SEND);
linrad_thread_stop_and_join(rx_input_thread);
linrad_thread_stop_and_join(THREAD_CAL_INTERVAL);
free_semaphores();
if(kill_all_flag)goto cal_skip;
clear_screen();
lir_refresh_screen();
if(usercontrol_mode != USR_CAL_FILTERCORR)
  {
  disable_extio();
  goto iqbeg;
  }
if( (ui.rx_input_mode&IQ_DATA) == 0)cal_interval/=2;
#define ERRLINE 5
if(cal_interval > fft1_size/5)
  {
  lir_text(5,ERRLINE,"Pulse repetition frequency too low.");
  lir_text(5,ERRLINE+1,"Increase PRF or set fft1 bandwidth lower to");
  lir_text(5,ERRLINE+2,"increase transform size.");
err1:;
  lir_text(5,ERRLINE+5,press_any_key);
  await_keyboard();
  disable_extio();
  goto iqbeg;
  }  
if(cal_signal_level > 0.9)
  {
  lir_text(5,ERRLINE,"Pulses too strong (above 90%)");
  goto err1;
  }
timf1p_pa=0;
rxin_isho=(short int*)timf1_char;
rxin_int=(int*)timf1_char;
rxin_char=(char*)timf1_char;
timf1p_px=0;
init_semaphores();
if( (ui.network_flag&NET_RX_INPUT) != 0)
  {
  init_network();
  if(kill_all_flag) goto cal_skip;
  }
linrad_thread_create(rx_input_thread);
lir_sleep(100000);
if(lir_status != LIR_OK)
  {
  lir_join(rx_input_thread);
  goto cal_skip_freesem;
  }
if((ui.network_flag&NET_RX_OUTPUT)!=0)
                                linrad_thread_create(THREAD_NETWORK_SEND);
if(kill_all_flag) goto cal_skip_freesem;
linrad_thread_create(THREAD_WIDEBAND_DSP);
if(kill_all_flag) goto calfil;
linrad_thread_create(THREAD_CAL_FILTERCORR);
if(kill_all_flag) goto calfil;
linrad_thread_create(THREAD_USER_COMMAND);
calfil:;
lir_sleep(50000);
lir_refresh_screen();
lir_join(THREAD_USER_COMMAND);
if((ui.network_flag&NET_RX_OUTPUT)!=0)
                          linrad_thread_stop_and_join(THREAD_NETWORK_SEND);
linrad_thread_stop_and_join(rx_input_thread);
linrad_thread_stop_and_join(THREAD_CAL_FILTERCORR);
linrad_thread_stop_and_join(THREAD_WIDEBAND_DSP);
free_semaphores();
if(kill_all_flag)goto cal_skip;
disable_extio();
goto iqbeg;
// ********************************
cal_skip_freesem:;
free_semaphores();
cal_skip:;
disable_extio();
memcheck(199,calmem,&calmem_handle);
calmem_handle=chk_free(calmem_handle);
free_buffers();
}

void restore_uiparm(void)
{
int i;
int *uiparm;
uiparm=(int*)(&ui);
for(i=0; i<MAX_UIPARM; i++)uiparm[i]=uiparm_save[i];
}

void save_uiparm(void)
{
int i;
int *uiparm;
uiparm=(int*)(&ui);
for(i=0; i<MAX_UIPARM; i++)uiparm_save[i]=uiparm[i];
}

void set_wav_parms(void)
{
if( wav_read_flag != 0)
  {
  if(ui.rx_ad_channels == 2)
    {
    if(iqflag==1)
      {
      ui.rx_rf_channels=1;
      ui.rx_input_mode|=IQ_DATA;
      }
    else
      {
      ui.rx_input_mode|=TWO_CHANNELS;
      ui.rx_rf_channels=2;
      }
    if(dirflag == 1)
      {
      fg.passband_direction=-1;
      }
    else
      {
      fg.passband_direction=1;
      }
    }
  else
    {
    ui.rx_rf_channels=1;
    fg.passband_direction=1;
    }        
  fft1_direction=fg.passband_direction;
  }
}

void select_fft1_gpu_parms(void)
{
int i,j,line;
char s[80];
clear_screen();
line=0;
#if OPENCL_PRESENT == 1
if(fft_cntrl[FFT1_CURMODE].gpu  == GPU_OCL)
  {
  lir_text(10,line,
            "You have selected to use OpenCL and clFFT for the first FFT.");
  line+=4;
  lir_text(0,line,"Platform    Device     gflops");
  line++;
  for(i=0; i<(int)ocl_globals.platforms; i++)  
    {
    for(j=0; j<(int)ocl_globals.devices[i]; j++)
      {
      sprintf(s,    "%d          %d         %f",i,j,
                     ocl_globals.gflops[(unsigned int)i][(unsigned int)j]);
      lir_text(2,line,s);
      line++;
      }
    }
  line+=2;
  lir_text(0,line,"Get details on platforms and devices by pressing '6'");
  line++;
  lir_text(0,line,"on the main menu"); 
  line+=2;
  lir_text(0,line,"Select platform number =>");
  gpu.fft1_platform=lir_get_integer(26,line,2,0,(int)ocl_globals.platforms);  
  if(kill_all_flag)return;            
  line+=2;
  lir_text(0,line,"Select device number =>");
  gpu.fft1_device=lir_get_integer(24,line,2,0,
                    (int)ocl_globals.devices[gpu.fft1_platform]);  
  if(kill_all_flag)return;            
  line+=2;
  sprintf(s,"Select batch N 0 to %d =>",MAX_FFT1_BATCH_N);
  lir_text(0,line,s);
  gpu.fft1_batch_n=lir_get_integer(28,line,2,0,MAX_FFT1_BATCH_N);
  if(kill_all_flag)return;            
  make_modepar_file(GRAPHTYPE_GPU);
  }
#endif
#if HAVE_CUFFT == 1
if(fft_cntrl[FFT1_CURMODE].gpu  == GPU_CUDA)
  {
  lir_text(10,line,
            "You have selected to use CUDA.");
  sprintf(s,"Select batch N 0 to %d =>",MAX_FFT1_BATCH_N);
  line+=2;
  lir_text(0,line,s);
  gpu.fft1_batch_n=lir_get_integer(28,line,2,0,MAX_FFT1_BATCH_N);
  if(kill_all_flag)return;            
  make_modepar_file(GRAPHTYPE_GPU);
  }
#endif
// make sure the compiler is happy with respect to unused variables.
i=0;j=0;
sprintf(s,"%d %d",i,j);
lir_text(0,line,s);
clear_screen();
}

void init_genparm(int upd)
{
char s[256],ss[80];
FILE *file;
int  i, j, k, kmax, imin, imax;
int line;
int parwrite_flag, parfile_flag;
float t1;
char *parinfo;
parinfo=NULL;
parwrite_flag=0;
if(upd == TRUE) goto updparm;
file=NULL;
// Start by reading general parameters for the selected rx mode.
// If no parameters found, go to the parameter select routine because
// we can not guess what hardware will be in use so default parameters
// are likely to be very far from what will be needed.
parfile_flag=0;
if(savefile_parname[0] != 0)
  {
// We use disk input and there is a parameter file name supplied
// for this particular recording. 
  file = fopen(savefile_parname, "r");
  if (file == NULL)goto saverr;
  parfilnam=savefile_parname;
  parfile_flag=1;
// First read the mode flag, an extra line in the userint par file
// placed above the normal lines.
// This line specifies receive mode and whether the file should be
// repeated endlessly
// Stereo .wav files have more flags. Whether it is two RF channels
// or I and Q from direct conversion. In the latter case what 
// direction to use for the frequency scale.
  i=fread(s,1,1,file);
  if(i != 1)goto saverr_close;
  rx_mode=to_upper(s[0])-'A';  
  if(rx_mode < 0 || rx_mode >= MAX_RX_MODE)goto saverr_close;
  i=fread(s,1,1,file);
  if(i != 1)goto saverr_close;
  if(s[0]=='1')
    {
    savefile_repeat_flag=1;
    }
  else
    {
    if(s[0]!='0')goto saverr_close;
    savefile_repeat_flag=0;
    }
  if(wav_read_flag==0)goto skip_modlin;
  i=fread(s,1,1,file);
  if(i != 1)goto saverr_close;
  if(s[0]=='1')
    {
    iqflag=1;
    ui.rx_rf_channels=ui.rx_ad_channels/2;
    i=fread(s,1,1,file);
    if(i != 1)goto saverr_close;
    if(s[0]=='1')
      {
      dirflag=1;
      }
    else
      {
      if(s[0]!='0')goto saverr_close;
      dirflag=0;
      }
    }
  else
    {
    if(s[0]!='0')goto saverr_close;
    iqflag=0;
    ui.rx_rf_channels=ui.rx_ad_channels;
    }
skip_modlin:; 
// We use disk input and something was wrong with the first line.
// Read until end of line, then ask the user for the desired mode.
  i=fread(s,1,1,file);
  if(s[0]=='\n') goto fileok;
  if(s[0]=='[')
    {
    zzr=fscanf(file,"%d %d", &file_center_correction, &file_rate_correction);
    goto skip_modlin;
    }
  if(i==1)goto skip_modlin;
saverr_close:;
  fclose(file);
saverr:;
  parwrite_flag=1;
  }
if(diskread_flag == 2)
  {
  clear_screen();
  for(i=0; i<MAX_RX_MODE; i++)
    {
    sprintf(s,"%c=%s",i+'A',rxmodes[i]);
    lir_text(1, i+2,s);
    }
  settextcolor(15);
  lir_text(5, MAX_RX_MODE+3, "Select Rx mode");
  rx_mode=-1;
  while(rx_mode < 0 || rx_mode > MAX_RX_MODE)
    {
    to_upper_await_keyboard();
    if(kill_all_flag) return;
    rx_mode=lir_inkey-'A';
    }
  clear_screen();
  line=5;
  lir_text(5,line,"Repeat recording endlessly (Y/N)?");          
  line++;
savrpt:;
  to_upper_await_keyboard();
  if(kill_all_flag) return;
  if(lir_inkey=='Y')
    {
    savefile_repeat_flag=1;
    }
  else
    {
    if(lir_inkey != 'N')goto savrpt;  
    savefile_repeat_flag=0;
    }
  if( wav_read_flag != 0)
    {
    if(ui.rx_ad_channels == 2)
      {
      lir_text(5,line,"Stereo recording. Interpret as I/Q data (Y/N) ?");          
      line++;
wawiq:;
      to_upper_await_keyboard();
      if(kill_all_flag) return;
      if(lir_inkey=='Y')
        {
        iqflag=1;
        lir_text(5,line,"Invert frequency scale (Y/N) ?");
        line++;
diriq:;
        to_upper_await_keyboard();
        if(kill_all_flag) return;
        if(lir_inkey=='Y')
          {
          dirflag=1;
          }
        else
          {
          if(lir_inkey != 'N')goto diriq;  
          dirflag=0;
          }
        }
      else
        {
        if(lir_inkey != 'N')goto wawiq;  
        iqflag=0;
        fg.passband_center=0.000001*ui.rx_ad_speed/4;
        freq_from_file=TRUE;
        }
      }
    else
      {
      ui.rx_rf_channels=1;
      fg.passband_center=0.000001*ui.rx_ad_speed/4;
      freq_from_file=TRUE;
      }
    }
  lir_text(5,line,"Set center frequency correction in ppb:");
  file_center_correction=lir_get_integer(45,line,7,-900000,900000);
  line++;
  lir_text(5,line,"Set sampling rate correction in ppb:");
  file_rate_correction=lir_get_integer(42,line,7,-900000,900000);
  line++;
  if(savefile_parname[0]!=0)
    {
    file = fopen( savefile_parname, "r");
    if(file == NULL) parwrite_flag=1;
    }              
  settextcolor(7);
  }
set_wav_parms();
if(parfile_flag == 0)parfilnam=rxpar_filenames[rx_mode];
file = fopen(rxpar_filenames[rx_mode], "r");
if(file == NULL)
  {
  sprintf(s,"%s file missing",rxpar_filenames[rx_mode]);
  prompt_reason(s);
iniparm:;
  if(kill_all_flag) return;
  for(i=0; i<MAX_GENPARM; i++)
    {
    genparm[i]=genparm_default[rx_mode][i];
    }
// The default mix1 bandwidth reduction will be fine when
// the sampling rate is 96 kHz I/Q.
  t1=96000;
  if( (ui.rx_input_mode&IQ_DATA) == 0)t1*=2;
  t1=ui.rx_ad_speed/t1;
  if(t1 > 1)
    {
    while(t1>1.5)
      {
      t1*=0.5;
      genparm[MIX1_BANDWIDTH_REDUCTION_N]++;
      }
    }
  if(t1 < 1)
    {
    while(t1<0.7)
      {
      t1*=2;
      genparm[MIX1_BANDWIDTH_REDUCTION_N]--;
      }
    genparm[MIX1_BANDWIDTH_REDUCTION_N]-=make_power_of_two(&i);
    }
  fix_limits(&imax, &imin,MIX1_BANDWIDTH_REDUCTION_N);
  if(genparm[MIX1_BANDWIDTH_REDUCTION_N] > imax)
                           genparm[MIX1_BANDWIDTH_REDUCTION_N]= imax;
  if(genparm[MIX1_BANDWIDTH_REDUCTION_N] < imin)
                           genparm[MIX1_BANDWIDTH_REDUCTION_N]= imin;
  genparm[MAX_GENPARM]=ui.rx_ad_speed;
updparm:;
  if(diskread_flag == 4)
    {
    if(init_diskread(-1) != 0)
      {
      lirerr(3641);
      return;
      }
    }
  set_wav_parms();
  set_general_parms(rxmodes[rx_mode]);
  if(kill_all_flag)return;
  genparm[MAX_GENPARM+1]=GENPARM_VERNR;
  if(diskread_flag == 0)save_uiparm();
  if(savefile_parname[0]!=0)
    {
write_savefile_parms:;    
    parwrite_flag=0;
    parfilnam=savefile_parname;
    file = fopen(parfilnam, "w");
    if(file == NULL)goto wrerr;
    fprintf(file,"%c%c",rx_mode+'A',savefile_repeat_flag+'0');
    if( wav_read_flag != 0)
      {
      fprintf(file,"%c%c", iqflag+'0', dirflag+'0');
      }
    fprintf(file," [%d %d]\n", file_center_correction, file_rate_correction);
    goto wrok;   
    }
  file = fopen(parfilnam, "w");
  if(file == NULL)
    {
wrerr:;  
    clear_screen();
    could_not_create(parfilnam,0);
    return;
    }
wrok:;
  for(i=0; i<MAX_GENPARM+2; i++)
    {
    fprintf(file,"%s [%d]\n",genparm_text[i],genparm[i]);
    }
  parfile_end(file);
  file=NULL;
  }
else
  {
  parfilnam=rxpar_filenames[rx_mode];
fileok:;
  parinfo=malloc(4096);
  if(parinfo == NULL)
    {
    fclose(file);
    lirerr(1081);
    return;
    }
  for(i=0; i<4096; i++)parinfo[i]=0;
  kmax=fread(parinfo,1,4095,file);
  fclose(file);
  file=NULL;
  if(kmax >= 4095)
    {
    sprintf(ss,"Excessive file size");
daterr:;    
    sprintf(s,"%s Data error in file %s",ss,parfilnam);
    prompt_reason(s);
    goto iniparm;
    }
  k=0;
  for(i=0; i<MAX_GENPARM+2; i++)
    {
    while(k <= kmax && (parinfo[k]==' ' || parinfo[k]== '\n' ))k++;
    if(k > kmax)
      {
      sprintf(ss,"Premature end of file");
daterr_free:;
      parinfo=chk_free(parinfo);
      goto daterr;
      }
    j=0;
    while(parinfo[k]== genparm_text[i][j])
      {
      k++;
      j++;
      } 
    sprintf(ss,"Error: %s  ",genparm_text[i]);
    if(genparm_text[i][j] != 0 || k > kmax)
      {
      goto daterr_free;
      }
    while(k <= kmax && parinfo[k]!='[')k++;
    if(k > kmax)
      {
      goto daterr_free;
      }
    sscanf(&parinfo[k],"[%d]",&genparm[i]);
    while(k <= kmax && parinfo[k]!='\n')k++;
    if(k > kmax)
      {
      goto daterr_free;
      }
    }
  parinfo=chk_free(parinfo);
  if(diskread_flag == 4)
    {
    if(init_diskread(-1) != 0)
      {
      lirerr(3641);
      free(parinfo);
      return;
      }
    }
  else
    {    
    if( fabs( (float)(genparm[MAX_GENPARM] - ui.rx_ad_speed)/
             (genparm[MAX_GENPARM] + ui.rx_ad_speed)) > 0.0002)
      {
      sprintf(s,"Input sampling speed changed %d  (old=%d)",
                                       ui.rx_ad_speed, genparm[MAX_GENPARM]);
      prompt_reason(s);
      goto iniparm;
      }                                 
    }
  set_wav_parms();
  if(genparm[MAX_GENPARM+1] != (int)(GENPARM_VERNR))
    {
    prompt_reason("GENPARM version changed");
    goto iniparm;
    }
    
  if(rx_mode!=MODE_WCW && rx_mode!=MODE_NCW && rx_mode!=MODE_HSMS &&
                                         genparm[CW_DECODE_ENABLE] != 0)
    {
    prompt_reason("Mode not compatible with morse decode");
    goto iniparm;
    }
  for(i=0; i<MAX_GENPARM; i++)
    {
    if(genparm[i] < genparm_min[i] || genparm[i] > genparm_max[i])
      {
illegal_value:;
      sprintf(s,"Illegal value for %s: %d (%d to %d)",genparm_text[i],
                                   genparm[i],genparm_min[i], genparm_max[i]);
      prompt_reason(s);
      goto iniparm;
      }
    }
  fft1mode=(ui.rx_input_mode&(TWO_CHANNELS+IQ_DATA))/2;
  i=fft1_version[fft1mode][genparm[FIRST_FFT_VERNR]];
  if( i < 0 || i>=MAX_FFT_VERSIONS)
    {
    prompt_reason("FFT1 version incompatible with A/D mode");
    goto iniparm;
    }
  if(fft_cntrl[i].gpu == GPU_OCL)
    {
#if OPENCL_PRESENT != 1
    prompt_reason("Parameters say use OpenCL and clFFT - not supported by computer!");
    goto iniparm;
#else
    if(!ocl_active)
      {
      prompt_reason("OpenCL failed to start");
      goto iniparm;
      }    
    if (read_modepar_file(GRAPHTYPE_GPU) == 0 ||
            gpu.fft1_batch_n < 0 ||
                    gpu.fft1_batch_n > MAX_FFT1_BATCH_N)
      {
      select_fft1_gpu_parms();
      if(kill_all_flag)return;
      }
#endif
    }
  if(fft_cntrl[i].gpu == GPU_CUDA)
    {
#if HAVE_CUFFT == 0
    prompt_reason("Parameters say use CUDA - not supported by computer!");
    goto iniparm;
#else
    if (read_modepar_file(GRAPHTYPE_GPU) == 0 || 
        gpu.fft1_batch_n < 0 ||
        gpu.fft1_batch_n > MAX_FFT1_BATCH_N )
      {
      select_fft1_gpu_parms();
      if(kill_all_flag)return;
      }
#endif
    }
  if(simd_present == 0)
    {
    if( fft_cntrl[i].simd != 0)
      {
      prompt_reason("Parameters say use SIMD - not supported by computer!");
      goto iniparm;
      }
    }
#if IA64 != 0    
  if( fft_cntrl[i].simd != 0)
    {
    prompt_reason("Parameters say use SIMD - not implemented for 64 bit code!");
    goto iniparm;
    }
#endif
  if(genparm[SECOND_FFT_ENABLE] != 0)
    {  
    if(mmx_present == 0)
      {
      if( fft_cntrl[i].mmx != 0)
        {
nommx:;
#if IA64 == 0
        prompt_reason("Parameters say use MMX - not supported by computer!");
#else
        prompt_reason("Parameters say use MMX - not (yet?) supported on IA64!");
#endif
        goto iniparm;
        }
      }
    i=fft1_back_version[ui.rx_rf_channels-1][genparm[FIRST_BCKFFT_VERNR]];
    if( i < 0 || i>=MAX_FFT_VERSIONS)
      {
      prompt_reason("Backwards FFT1 version incompatible with no of channels");
      goto iniparm;
      }
    if(mmx_present == 0)
      {
      if( fft_cntrl[i].mmx != 0)goto nommx;
      }
    i=fft2_version[ui.rx_rf_channels-1][genparm[SECOND_FFT_VERNR]];
    if( i < 0 || i>=MAX_FFT_VERSIONS)
      {
      prompt_reason("FFT2 version incompatible with no of channels");
      goto iniparm;
      }
    if(mmx_present == 0)
      {
      if( fft_cntrl[i].mmx != 0)goto nommx;
      }
// Make sure fft1_n and fft2_n are defined.
    get_wideband_sizes();
    if(kill_all_flag) return;
    k=(fft1_n-4)&0xfffe;
    if(genparm[FIRST_BCKFFT_ATT_N]>k)
      {
      i=FIRST_BCKFFT_ATT_N;
      goto illegal_value;
      }
    k=fft2_n-2;
    if(genparm[SECOND_FFT_ATT_N]>k)
      {
      i=SECOND_FFT_ATT_N;
      goto illegal_value;
      }
    }
  if(genparm[DA_OUTPUT_SPEED] > ui.rx_max_da_speed||
               genparm[DA_OUTPUT_SPEED] < ui.rx_min_da_speed)
    {
    sprintf(s,"Output sampling speed out of range %d  (%d to %d)",
                              genparm[DA_OUTPUT_SPEED],ui.rx_min_da_speed,
                                                          ui.rx_max_da_speed);
    prompt_reason(s);
    goto iniparm;
    }
  check_output_no_of_channels();
  }
if( wav_read_flag != 0)
  {
  ui.rx_rf_channels=1;
  if(ui.rx_ad_channels == 2)
    {
    if(iqflag==1)
      {
      ui.rx_input_mode|=IQ_DATA;
      }
    else
      {
      ui.rx_rf_channels=2;
      ui.rx_input_mode|=TWO_CHANNELS;
      }
    }
  if(dirflag == 1)
    {
    fg.passband_direction=-1;
    }
  else
    {
    fg.passband_direction=1;
    }
  fft1_direction=fg.passband_direction;
  }
if(genparm[AFC_ENABLE] == 0)
  {
  genparm[CW_DECODE_ENABLE]=0;
  }
if(parwrite_flag!=0)goto write_savefile_parms;
if(diskread_flag != 0)
  {
// Apply the corrections that the user might have asked for.
  ui.rx_ad_speed-=rint((float)(ui.rx_ad_speed)*0.000000001F*
                                                       file_rate_correction);
  fg.passband_center-=fg.passband_center*0.000000001*
                            (double)file_center_correction;
  }
}


void main_menu(void)
{
int rx_rf_channels;
int i, j, k, n1, n2;
int uiupd, line;
char s[256], ss[80];
int message_line;
FILE *file;
int *uiparm;
uiparm=(int*)(&ui);
rx_rf_channels=ui.rx_rf_channels;
lir_mutex_init();
eme_flag=0;
first_keypress=0;
iqflag=0;
dirflag=0;
read_eme_database();
linrad_thread_create(THREAD_SYSCALL);
if(kill_all_flag) goto menu_x;
// Save the ui parameters. 
// The user has set up the sound cards for normal operation
// with his hardware but some routines, e.g. txtest may change
// the A/D parameters and/or other parameters.
// We will always start here with the initial ui parameters.
    if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
      {
      n1=MIN_DMA_RATE_EXP;
      n2=MAX_DMA_RATE_EXP;
      }
    else
      {
      n1=MIN_DMA_RATE;
      n2=MAX_DMA_RATE;
      }
if (ui.max_dma_rate < n1 || ui.max_dma_rate > n2) 
  {
  ui.max_dma_rate=DEFAULT_MAX_DMA_RATE;
  }
if (ui.min_dma_rate < n1 || ui.min_dma_rate > n2) 
  {
  ui.min_dma_rate=DEFAULT_MIN_DMA_RATE;
  }
if ( ui.min_dma_rate > ui.max_dma_rate) 
  {
  ui.min_dma_rate=ui.max_dma_rate;
  }
compute_converter_parameters();
verify_network(FALSE);
#if OPENCL_PRESENT == 1
ocl_active=FALSE;
ocl_active=start_ocl(FALSE);
if(ocl_active)
  {
  for(i=0; i<(int)ocl_globals.platforms; i++)
    {
    for(j=0; j<(int)ocl_globals.devices[i]; j++)
      {
      ocl_compute_gflops((unsigned int)i,(unsigned int)j);
      }
    }
  }
#endif  
save_uiparm();
menu_loop:;
if(kill_all_flag)goto menu_x;
tx_setup_flag=FALSE;
lir_set_title("");
restore_uiparm();
clear_screen();
settextcolor(12);
display_rx_input_source(ss);
#if IA64 == 0
sprintf(s," 32bit");
#else
sprintf(s," 64bit");
#endif
sprintf(&s[6]," %s   %s",PROGRAM_NAME, ss);    
lir_text(8,0,s);
if((ui.network_flag&NET_RX_OUTPUT) != 0)lir_text(1,0,"NETSEND");
lir_refresh_screen();
if(kill_all_flag)goto menu_x;
line=2;
settextcolor(14);
if(ui.operator_skil == OPERATOR_SKIL_NEWCOMER)
  {
  lir_text(14,1,"newcomer mode");
  line++;
  }
if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
  {
  lir_text(14,1,"expert mode");
  line++;
  }
message_line=line;  
settextcolor(7);
button_color=7;
if(ui.rx_addev_no != DISABLED_DEVICE_CODE)
  {
  for(i=0; i<MAX_RX_MODE; i++)
    {
    if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER || newcomer_rx_modes[i] != 0)
      {
      if(ui.operator_skil == OPERATOR_SKIL_EXPERT || i != MODE_TUNE)
        {
        sprintf(s,"%c=%s",i+'A',rxmodes[i]);
        lir_text(1, line, s);
        line++;
        }
      }  
    }
  }
if((ui.network_flag&NET_RX_INPUT) == 0)
  {
  lir_text(30,message_line,"1=Process first file named in 'adfile'");
  message_line++;
  lir_text(30,message_line,"2=Process first file named in 'adwav'");
  message_line++;
  lir_text(30,message_line,"3=Select file from 'adfile'");
  message_line++;
  lir_text(30,message_line,"4=Select file from 'adwav'");
  message_line++;
  if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
    {
    lir_text(30,message_line,"5=File converter .raw to .wav");
    message_line++;
#if OPENCL_PRESENT == 1
#if(OSNUM == OSNUM_LINUX)
    lir_text(30,message_line,"6=Test OpenCL (output on terminal window)");
    message_line++;
#endif 
#endif
#if HAVE_CUFFT == 1
#if(OSNUM == OSNUM_LINUX)
    lir_text(30,message_line,"7=Test CUDA (output on terminal window)");
    message_line++;
#endif 
#endif

    }
  }
if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
  {
  lir_text(30,message_line,"T=Toggle network output");
  message_line++;
  }
if(message_line > line)line=message_line;  
line++;  
settextcolor(3);
if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
  {
  lir_text(1, line  , "M=Init moon tracking and EME database");
  line++;
  lir_text(1, line, "N=Network set up");
  line++;
  }
  lir_text(1, line, 
#if(OSNUM == OSNUM_WINDOWS)
	  fonts_by_GDI ? 
	  "S=Global parms set up (CTRL-F: Font selection)" : 
#endif
	  "S=Global parms set up");
line++;
lir_text(1, line, "U=A/D and D/A set up for RX");
line++;
if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
  {
  lir_text(1, line, "V=TX mode set up");
  line++;
  }
lir_text(1, line, "W=Save current parameters in par_userint");
line++;
if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER && 
                                   (ui.network_flag&NET_RX_INPUT) == 0)
  {
  switch (ui.rx_soundcard_radio)
    {
    case RX_SOUNDCARD_RADIO_WSE:
    lir_text(1, line, "Z=WSE hardware interface test");
    line++;
    break;
    
    default:
    break;
    }
  }
if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
  {
  lir_text(1, line, "F9=Emergency light");
  line++;
  }
lir_text(1, line, "ESC=Quit program");
settextcolor(14);
lir_text(1, line, "F1 or !=Show keyboard commands (HELP)");
line++;
if(kill_all_flag)goto menu_x;
message_line=line+1;
if(uiparm_change_flag==TRUE)
  {
  settextcolor(15);
  lir_text(5,message_line+1,"PARAMETERS NOT SAVED Press ""W"" to save");
  }
settextcolor(7);
if(kill_all_flag) goto menu_x;
#if OSNUM == OSNUM_WINDOWS
if(first_keypress == 0)
  {
  j=0;
  if(dll_version > 0)
    {
    if(dll_version == DLL_VERSION_NUMBER)
      {
      sprintf(s,"The required DLL package [%d] is installed",dll_version);
      j=1;
      }
    else
      {
#if IA64 == 0
      sprintf(s,"The DLL package [%d] is installed, ([%d] or later is recommended)"
                                 ,dll_version, DLL_VERSION_NUMBER);
#else
      if(dll_version < 3)
        {
        sprintf(s,"The DLL package [%d] is installed.This 64 bit program needs[3] or later"
                                 ,dll_version);
        }
      else
        {        
        sprintf(s,"The DLL package [%d] is installed, ([%d] or later is recommended)"
                                 ,dll_version, DLL_VERSION_NUMBER);
        }
#endif          
      }
    }
  else
    {
    sprintf(s,"Can not open %s",dllversfilename);
    }
  lir_text(5,message_line+1,s);
  if(j == 0)
    {
    lir_text(5,message_line+2,"The latest linrad dll package is available here:");
    lir_text(5,message_line+3,"http://sm5bsz.com/dll.htm");
    }
  }
#endif
if(ui.operator_skil == OPERATOR_SKIL_EXPERT && first_keypress == 0 &&
                           ui.autostart >= 'A' && ui.autostart <= 'G')
  {
  lir_inkey=ui.autostart;
  }
else
  {    
  if(first_keypress > 1)
    {
    lir_inkey=first_keypress;
    lir_sleep(300000);
    }
  else
    {
    to_upper_await_keyboard();
    if(kill_all_flag) goto menu_x;
    }
  }
first_keypress=1;
savefile_parname[0]=0;
calibrate_flag=0;
diskread_flag=0;
file_start_block=-1;
file_stop_block=-1;
diskwrite_flag=FALSE;
freq_from_file=FALSE;
#if(OSNUM == OSNUM_LINUX)
if((ui.use_alsa & SOUND_SYSTEM_DEFINED) != 0)
  {
  if( (ui.use_alsa&NATIVE_ALSA_USED) != 0)
    {
    load_alsa_library();
    if(!alsa_library_flag)goto menu_x;
    }
  else
    {
    unload_alsa_library();
    }
  }
#endif
if(ui.use_extio != 0 && ui.extio_type != 4)
  {
  rx_input_thread= THREAD_EXTIO_INPUT;
  }
else
  {  
  rx_input_thread=THREAD_RX_ADINPUT;
  if(ui.rx_addev_no == SDR14_DEVICE_CODE ||
     ui.rx_addev_no == SDRIQ_DEVICE_CODE )rx_input_thread=THREAD_SDR14_INPUT;
  if(ui.rx_addev_no == PERSEUS_DEVICE_CODE)rx_input_thread=THREAD_PERSEUS_INPUT;
  if(ui.rx_addev_no == SDRIP_DEVICE_CODE)rx_input_thread=THREAD_SDRIP_INPUT;
  if(ui.rx_addev_no == EXCALIBUR_DEVICE_CODE)
                                       rx_input_thread=THREAD_EXCALIBUR_INPUT;
  if(ui.rx_addev_no == RTL2832_DEVICE_CODE)
                                       rx_input_thread=THREAD_RTL2832_INPUT;
  if(ui.rx_addev_no == MIRISDR_DEVICE_CODE)
                                       rx_input_thread=THREAD_MIRISDR_INPUT;
  if(ui.rx_addev_no == SDRPLAY2_DEVICE_CODE)
    rx_input_thread=THREAD_SDRPLAY2_INPUT;
  if(ui.rx_addev_no == SDRPLAY3_DEVICE_CODE)
                                       rx_input_thread=THREAD_SDRPLAY3_INPUT;
  if(ui.rx_addev_no == AIRSPY_DEVICE_CODE)
                                       rx_input_thread=THREAD_AIRSPY_INPUT;
  if(ui.rx_addev_no == AIRSPYHF_DEVICE_CODE)
                                       rx_input_thread=THREAD_AIRSPYHF_INPUT;
#if(OSNUM == OSNUM_LINUX)
  if(ui.rx_addev_no == FDMS1_DEVICE_CODE)
                                       rx_input_thread=THREAD_FDMS1_INPUT;
#endif
  if(ui.rx_addev_no == BLADERF_DEVICE_CODE)
                                       rx_input_thread=THREAD_BLADERF_INPUT;
  if(ui.rx_addev_no == PCIE9842_DEVICE_CODE)
                                       rx_input_thread=THREAD_PCIE9842_INPUT;
  if(ui.rx_addev_no == OPENHPSDR_DEVICE_CODE)
                                       rx_input_thread=THREAD_OPENHPSDR_INPUT;
  if(ui.rx_addev_no == NETAFEDRI_DEVICE_CODE)
                                       rx_input_thread=THREAD_NETAFEDRI_INPUT;
  if(ui.rx_addev_no == CLOUDIQ_DEVICE_CODE)
                                       rx_input_thread=THREAD_CLOUDIQ_INPUT;
  }
if(kill_all_flag)goto menu_x;
lir_status=LIR_OK;
sys_func(THRFLAG_PORTAUDIO_STARTSTOP);
if(kill_all_flag) goto menu_x;
if(lir_status != LIR_OK)
  {
  clear_screen();
  lir_text(5,5,"Something has gone wrong with your setup.");
  lir_text(5,6,"Exit with ESC and fix your dll files or press enter");
  lir_text(5,7,"to clear soundcard settings. It would then be necessary");
  lir_text(5,8,"to re-configure all soundcards.");
  await_processed_keyboard();
  if(kill_all_flag)goto menu_x;
  ui.use_alsa=0;
  ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
  ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
  ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
  ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
  save_uiparm();
  lir_status=LIR_OK;
  goto write_parms;
  }
ampinfo_reset=workload_reset_flag-1;
curv_xpixel=0;
flat_xpixel=0;
parfilnam=NULL;
wg_freq_x1=-1;
wav_read_flag=0;
switch ( lir_inkey )
  {
  case 'T':
  if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
    {
    if( (ui.network_flag&NET_RX_OUTPUT) == 0)
      {
      ui.network_flag|=NET_RX_OUTPUT;
      }
    else
      {
      ui.network_flag&=NET_RX_INPUT;
      }
    verify_network(FALSE);
    save_uiparm();
    uiparm_change_flag=TRUE;
    }
  break;
  
  case 'M':
  if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
    {
    init_eme_database();
    if(kill_all_flag) goto menu_x;
    }
  break;

  case F1_KEY:
  case '!':
  help_message(1);
  if(kill_all_flag) goto menu_x;
  break;

  case F8_KEY:
  if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
    {
    clear_screen();
    lir_text(1,0,"Currently selected font (see fonts.c)");
    for(i=0; i<16; i++)
      {
      for(j=0; j<16; j++)
        {
        k=16*i+j;
        sprintf(s,"%c[%2x]",k,k);
        lir_text(j*8,i+2,s);
        }
      }  
    while(!kill_all_flag)
      {
      await_keyboard();  
      sprintf(s,"lir_inkey=%d =0x%02x",lir_inkey,lir_inkey);
      clear_lines(22,22);
      lir_text(1,22,s);
      }
    }
  break;
    
  case F9_KEY:
  if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
    {
    lir_fillbox(0,0,screen_width,screen_height,15);
    await_keyboard();
    }
  break;

  case '1':
  case '3':
  if((ui.network_flag&NET_RX_INPUT) == 0)
    {
    if(ui.rx_dadev_no == -1)goto setad;
    rx_input_thread=THREAD_RX_FILE_INPUT;
    if(init_diskread(lir_inkey-'1') == 0)
      {
      goto do_pc_radio;
      }
    }
  break;

  case '2':
  case '4':
  if((ui.network_flag&NET_RX_INPUT) == 0)
    {
    if(ui.rx_dadev_no == -1)goto setad;
    wav_read_flag=1;
    rx_input_thread=THREAD_RX_FILE_INPUT;
    if(init_diskread(lir_inkey-'2') == 0)
      {
      goto do_pc_radio;
      }
    }
  break;

  case '5':
  if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
    {
    wav_read_flag=0;
    raw2wav();
    }
  break;

#if OPENCL_PRESENT == 1
  case '6':
  printf( "\nocl_test=%d\n\n",ocl_test());
  fflush(NULL);
  break;
#endif

#if HAVE_CUFFT == 1
  case '7':
  printf( "\ncuFFT =%d\n\n",test_cuda());
  fflush(NULL);
  break;
#endif

  case 'U':
setad:;
  sys_func(THRFLAG_SET_RX_IO);
  if(kill_all_flag) goto menu_x;
  verify_network(FALSE);
  save_uiparm();
  uiparm_change_flag=TRUE;
  break; 

  case 'V':
  if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
    {
    tx_setup_flag=TRUE;
    sys_func(THRFLAG_TX_SETUP);
    tx_setup_flag=FALSE;
    if(kill_all_flag) goto menu_x;
    save_uiparm();
    uiparm_change_flag=TRUE;
    }
  break; 

  case 'Z':
  if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
    {
    switch (ui.rx_soundcard_radio)
      {
      case RX_SOUNDCARD_RADIO_WSE:
      hware_interface_test();
      if(kill_all_flag) goto menu_x;
      break;
      
      default:
      break;
      }
    }
  break;
    
  case 'N':
  if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
    {
    ui.network_flag=NET_RX_INPUT;
    verify_network(TRUE);
    if(kill_all_flag) goto menu_x;
    save_uiparm();
    uiparm_change_flag=TRUE;
    }
  break;

  case 'S':
  switch (screen_type)
    {
#if OSNUM == OSNUM_LINUX
    case SCREEN_TYPE_SVGALIB:
    llin_global_uiparms(1);
    break;
      
    case SCREEN_TYPE_FBDEV:
    flin_global_uiparms(1);
    break;
      
    case SCREEN_TYPE_X11:
    case SCREEN_TYPE_X11_SHM:
    x_global_uiparms(1);
    break;
#endif

#if OSNUM == OSNUM_WINDOWS
    case SCREEN_TYPE_WINDOWS:
    win_global_uiparms(1);
    break;
#endif
    }
  if(kill_all_flag) goto menu_x;
  save_uiparm();
  uiparm_change_flag=TRUE;
  break;

#if(OSNUM == OSNUM_WINDOWS)
  case 6:
    // ctrl-F: Font selection
	win_selectfont();
  break;
#endif /* OS is Windows */

  case 'W':
write_parms:;  
  file = fopen(userint_filename, "w");
  if (file == NULL)
    {
    lirerr(1029);
    goto menu_x;
    }
  ui.check=UI_VERNR;  
  for(i=0; i<MAX_UIPARM; i++)
    {
    fprintf(file,"%s [%d]\n",uiparm_text[i],uiparm[i]);
    }
  parfile_end(file);
  if(kill_all_flag) goto menu_x;
  settextcolor(15);
  clear_lines(message_line,screen_last_line);
  lir_text(1, message_line, "User interface setup saved");
  k=message_line;
  for(i=0; i<MAX_UIPARM-1; i+=2)
    {
    if( k < screen_last_line)
      {
      k++;
      sprintf(s,"%s [%d] ",uiparm_text[i],uiparm[i]);
      lir_text(5,k, s);
      sprintf(s,"%s [%d] ",uiparm_text[i+1],uiparm[i+1]);
      lir_text(40,k, s);
      }
    }
  if(i < MAX_UIPARM-1)
    {
    i++;
    sprintf(s,"%s [%d] ",uiparm_text[i],uiparm[i]);
    if( (message_line+i+1) <= screen_last_line)lir_text(5,message_line+i+1, s);
    }      
  settextcolor(7);
  await_processed_keyboard();
  if(kill_all_flag) goto menu_x;
  first_keypress=lir_inkey;
  uiparm_change_flag=FALSE;
  goto menu_loop;

  default:
  if(ui.rx_addev_no == DISABLED_DEVICE_CODE)break; 
  if(ui.rx_addev_no == UNDEFINED_DEVICE_CODE)goto setad;
  rx_mode=(lir_inkey-'A');
  if( rx_mode < 0 || rx_mode>=MAX_RX_MODE)goto menu_loop;
  if(ui.operator_skil == OPERATOR_SKIL_NEWCOMER && newcomer_rx_modes[rx_mode]==0)goto menu_loop;
  if(ui.operator_skil != OPERATOR_SKIL_EXPERT && rx_mode == MODE_TUNE)goto menu_loop;
  if( (ui.network_flag&NET_RX_INPUT)==0 && ui.rx_input_mode < 0)goto setad;
do_pc_radio:;
  if(ui.rx_dadev_no == DISABLED_DEVICE_CODE)
    {
    ui.rx_max_da_bytes=2;
    ui.rx_max_da_channels=2;
    ui.rx_min_da_bytes=1;
    ui.rx_min_da_channels=1;
    ui.rx_min_da_speed=5000;
    ui.rx_max_da_speed=1000000;
    }
  if(ui.rx_max_da_channels==0)goto setad;
  if( ui.rx_min_da_speed < 1000  ||  
     ui.rx_max_da_speed < ui.rx_min_da_speed)
    {
    clear_screen();
    lir_text(3,3,"ERROR: illegal limits for Rx D/A speed");
    lir_text(5,5,press_any_key);
    await_keyboard();
    goto setad;
    }
  calibrate_flag=0;
  parfilnam=NULL;
  open_mouse();
  uiupd=FALSE;
  if((ui.network_flag&NET_RX_INPUT) == 0)
    {
updparm:;
    init_genparm(uiupd);
    if(kill_all_flag) goto menu_x;
//    init_genparm(FALSE);  //why was this added in r805??
// comment "for oclprogs" ??   Removed in r992 july 2021
    }
  if(si570_open_switch != 0)close_si570();
  clear_screen();
  settextcolor(7);
#if (USERS_EXTRA_PRESENT == 1)
  init_users_extra();
#endif
  lir_status=LIR_OK;
  rxda_status=LIR_OK;
  rxad_status=LIR_OK;
  txad_status=LIR_OK;
  txda_status=LIR_OK;
  fft1_waterfall_flag=0;
  all_threads_started=FALSE; 
  cg.oscill_on=0;
  first_txproc_no=tg.spproc_no;
  use_tx=ui.tx_enable;
  if(diskread_flag >= 2 ||
     ui.tx_addev_no == UNDEFINED_DEVICE_CODE ||
     ui.tx_dadev_no == UNDEFINED_DEVICE_CODE)use_tx=0;
  if(!ftdi_library_flag)
    {
    if( ui.rx_soundcard_radio == RX_SOUNDCARD_RADIO_ELEKTOR ||
        ui.rx_soundcard_radio == RX_SOUNDCARD_RADIO_SOFT66)
      {
      i=load_ftdi_library();
      if(i != 0)goto menu_loop;
      }
    if(ui.rx_soundcard_radio == RX_SOUNDCARD_RADIO_SOFT66)
      {
      load_soft66_library();
      if(!soft66_library_flag)goto menu_loop;
      }
    }
  switch (rx_mode)
    {
    case MODE_WCW:
    case MODE_NCW:
    case MODE_HSMS:
    case MODE_SSB:
    case MODE_QRSS:
    use_bfo=1;
    goto do_normal_rx;

    case MODE_FM:
    case MODE_AM:
    use_bfo=0;
do_normal_rx:;
    i=ui.rx_ad_speed;
    normal_rx_routine();
    if(kill_all_flag)goto menu_x;
    if(i != ui.rx_ad_speed)
      {
      save_uiparm();
      uiparm_change_flag=TRUE;
      }
    if(lir_status == LIR_DLL_FAILED)goto menu_loop;
    break;

    case MODE_TXTEST:
    txtest_routine();
    if(lir_status == LIR_DLL_FAILED)goto menu_loop;
    if(kill_all_flag)goto menu_x;
    goto txtest_exit;

// Soundcard test mode
    case MODE_RX_ADTEST:
    rx_adtest_routine();
    if(lir_status == LIR_DLL_FAILED)goto menu_loop;
    break;

// Tune WSE hardware.  
    case MODE_TUNE:
    tune_wse_routine();
    if(lir_status == LIR_DLL_FAILED)goto menu_loop;
    break;

    case MODE_RADAR:
    radar_routine();
    if(lir_status == LIR_DLL_FAILED)goto menu_loop;
    break;

    default:
    lirerr(436);
    } 
  if(kill_all_flag) goto menu_x;
  close_network_sockets();
  rx_rf_channels=ui.rx_rf_channels;
  if( (diskread_flag&(2+4+8)) != 0)
    {
    restore_uiparm();
    diskread_flag=4;
    fclose(save_rd_file);
#if(OSNUM == OSNUM_WINDOWS)
    CloseHandle(savefile_handle);
#endif
    save_rd_file=NULL;
    }
txtest_exit:;
  free_buffers();
// *******************************************************************
// ******************* main exit from processing modes ***************
  if(rxda_status != LIR_OK)
    {
    clear_screen();
    lir_text(5,5,"Soundcard for Rx output disabled");
    lir_text(5,6,"Could not be opened at specified speed");
    lir_text(5,7,"In duplex mode the same speed may be necessary for");
    lir_text(5,8,"output and input. The device speed may be locked");
    lir_text(5,9,"by Linrad input or some other application.");
    lir_text(10,12,press_any_key);
    await_keyboard();
    if(kill_all_flag) goto menu_x;
    lir_status = LIR_OK;
    goto menu_loop;
    }
  switch(lir_status)
    {
    case LIR_TUNEERR:
    goto menu_loop;

    case LIR_SAVE_UIPARM:
    save_uiparm();
    goto menu_loop;

    case LIR_FFT1ERR:
    clear_screen();
    lir_text(5,5,"Out of memory. Try less demanding parameters");
    lir_text(5,8,press_any_key);
    await_keyboard();
    if(kill_all_flag) goto menu_x;
go_updparm:;
    uiupd=TRUE;
    restore_uiparm();
    goto updparm;

    case LIR_NEW_POL:
    clear_screen();
    select_pol_default();
    if(kill_all_flag) goto menu_x;
    lir_status = LIR_OK;
    goto do_pc_radio;

    case LIR_POWTIM:
    goto do_pc_radio;  
    }
  if(lir_status < LIR_OK)
    {
    goto go_updparm;
    }  
  fft1_waterfall_flag=0;
wt_kbd:;
  clear_screen();
  line=0;
  settextcolor(12);
  lir_text(25,line,PROGRAM_NAME);
  if(diskread_flag >=2 )
    {
    settextcolor(14);
    lir_text(40,line,savefile_name);
    }
  line+=2;  
  settextcolor(15);
  sprintf(s,"F1 = Info about the %s mode",rxmodes[rx_mode]);  
  lir_text(11,line,s);
  line++;
  sprintf(s,"B = Back to %s without change",rxmodes[rx_mode]);
  lir_text(12,line,s);
  line++;
  lir_text(12,line,"P = Change parameters");
  line++;
  i=fft1_version[fft1mode][genparm[FIRST_FFT_VERNR]];
  if(fft_cntrl[i].gpu != 0)
    {
    lir_text(12,line,"S = Change GPU parameters");
    line++;
    }
  lir_text(12,line,"C = Calibrate");
  line++;
  if(rx_rf_channels == 2)
    {
    lir_text(12,line,"D = Select Pol. default");
    line++;
    }
  line++;
  settextcolor(7);
  sprintf(s,"Current parameters (file: %s)",parfilnam);
  lir_text(2,line,s);
  line++;
  for(i=0; i<MAX_GENPARM+1; i++)
    {
    sprintf(s,"%s [%d] ",genparm_text[i],genparm[i]);
    if( line <= screen_last_line)lir_text(2,line, s);
    line++;
    }
  await_processed_keyboard();
  if(kill_all_flag) goto menu_x;
  switch (lir_inkey)
    {
    case 'B':
    goto do_pc_radio;

    case 'C':
    if((ui.network_flag&NET_RX_INPUT) == 0)
      {
      init_genparm(FALSE);
      if(kill_all_flag) goto menu_x;
      }
    cal_package();
    lir_status=LIR_OK;
    goto menu_loop;

    case 'D':
    if(rx_rf_channels == 2)select_pol_default();
    if(kill_all_flag) goto menu_x;
    break;

    case 'P':
    uiupd=TRUE;
    goto updparm;

    case 'S':
    select_fft1_gpu_parms();
    if(kill_all_flag) goto menu_x;
    break;

    case F1_KEY:
    case '!':
    help_message(280+rx_mode);
    break;

    case 'X':
    goto menu_loop;
    }
  goto wt_kbd;
  }
if(!kill_all_flag)goto menu_loop;
menu_x:;
if(mg_meter_file != NULL)fclose(mg_meter_file);
close_mouse();
free_buffers();
close_network_sockets();
if(portaudio_active_flag)sys_func(THRFLAG_PORTAUDIO_STOP);
linrad_thread_stop_and_join(THREAD_SYSCALL);
lir_mutex_destroy();
show_errmsg(1);
if(lir_errcod != 0)
  {
  lir_inkey=1;
  while(lir_inkey == 1)
    {
    await_keyboard();
    }
  }
command_extio_library(EXTIO_COMMAND_KILL_ALL);
unload_ftdi_library();
unload_soft66_library();
#if(OSNUM == OSNUM_LINUX)
unload_alsa_library();
#endif
#if OPENCL_PRESENT == 1
if(ocl_active)stop_ocl();
#endif
}

