// Copyright (c) <2014> <Leif Asbrink>
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
#include "loadusb.h"
#if(OSNUM == OSNUM_LINUX)
#include "options.h"
#include <dlfcn.h>
#include <string.h>  
#include <sys/types.h>
#include <ctype.h>
#include "uidef.h"
#include "screendef.h"
#include "fft1def.h"
#include "usbdefs.h"
#include "vernr.h"
#include "thrdef.h"
#include "lscreen.h"

#define ELAD_VENDOR_ID 5921
#define FDMS1_PRODUCT_ID 1552
#define VENDOR_NAME_ELAD "FDM S1 USB FW v1.0"
#define PRODUCT_NAME_FDMS1 "FDM S1 USB FW V1.0"
#define FDMS1_MAX_RATES 6
libusb_device_handle  *fdms1_usb_handle;
libusb_context *elad_ctx; 
int fdms1_open_flag;
void *elad_handle;
void *elad_init_handle;

int64_t samprates[FDMS1_MAX_RATES]={192000,
                                     384000,
                                     768000,
                                    1536000,
                                    3072000,
                                    6144000};

#define MAX_FDMS1_PARM 5
char *fdms1_parm_text[MAX_FDMS1_PARM]={"Sampling_rate",
                                       "Enable filter",
                                       "Swap IQ",
                                       "Freq adjust",
                                       "Check"};
char* fdms1_parfil_name="par_fdms1";
                                       
typedef struct {
int sampling_rate_no;
int filter_enable;
int swap_iq;
int freq_adjust;
int check;
}P_FDMS1;
P_FDMS1 fdms1;                                       

typedef int (*p_fdms1_hw_init)(libusb_device_handle* handle);
typedef int (*p_cmd_extio)(libusb_device_handle* handle, int cmd, 
                                                         int* data, int);
typedef void (*p_set_elad_lp30)(libusb_device_handle* handle, int *cmd);
typedef void (*p_set_elad_att20)(libusb_device_handle* handle, int *cmd);
typedef void (*p_set_elad_iqswap)(libusb_device_handle* handle, int *cmd);
typedef int (*p_set_elad_hwlo)(libusb_device_handle* handle, int64_t *freq);
typedef int (*p_elad_open_hw)(libusb_device_handle* handle, int64_t rate);
typedef void (*p_elad_start_fifo)(libusb_device_handle* handle);
typedef void (*p_elad_stop_fifo)(libusb_device_handle* handle);

p_fdms1_hw_init fdms1_hw_init;
p_cmd_extio cmd_extio;
p_set_elad_lp30 set_elad_lp30;
p_set_elad_att20 set_elad_att20;
p_set_elad_iqswap set_elad_iqswap;
p_set_elad_hwlo set_elad_hwlo;
p_elad_open_hw elad_open_hw;
p_elad_start_fifo elad_start_fifo;
p_elad_stop_fifo elad_stop_fifo;

void open_elad_fdms1(int firmware_flag);
void close_elad_fdms1(void);

void set_fdms1_att(void)
{
int att;
if(fg.gain > -10)
  {
  att=1;
  }
else
  {
  att=0;
  }  
set_elad_att20(fdms1_usb_handle, &att);
}

void set_fdms1_frequency(void)
{
int64_t fq;
fq=(int64_t)(1000000.*fg.passband_center);
fg_truncation_error=fq-1000000.*fg.passband_center;
set_elad_hwlo(fdms1_usb_handle, &fq);
}

static void fdms1_callback(struct libusb_transfer *xfer)
{
if(LIBUSB_TRANSFER_COMPLETED == xfer->status &&
  thread_command_flag[THREAD_FDMS1_STARTER] == THRFLAG_ACTIVE)
  {
  memcpy(&timf1_char[timf1p_sdr],xfer->buffer,xfer->actual_length);  
  timf1p_sdr=(timf1p_sdr+xfer->actual_length)&timf1_bytemask;
  if(((timf1p_sdr-timf1p_pa+timf1_bytes)&timf1_bytemask) >=
              (int)snd[RXAD].block_bytes)lir_set_event(EVENT_FDMS1_INPUT);
  libusb_submit_transfer(xfer);
  }
}

void fdms1_starter(void)
{
int i;
int elad_block_size;
int no_of_buffers;
struct libusb_transfer **xfer;
unsigned char **xfer_buf;
struct timeval tv_nb={0, 2000};
elad_block_size=snd[RXAD].block_bytes/4;
if(elad_block_size < 512)elad_block_size=512;
no_of_buffers=(int)((0.1*ui.rx_ad_speed)/elad_block_size);
if(no_of_buffers < 4)no_of_buffers=4;
xfer=malloc(no_of_buffers*sizeof(struct libusb_transfer *));
for(i=0; i<no_of_buffers; i++)xfer[i]=libusb_alloc_transfer(0);
xfer_buf=malloc(no_of_buffers*sizeof(unsigned char *));
xfer_buf[0]=malloc(no_of_buffers*elad_block_size);
for(i=1; i<no_of_buffers; i++)
xfer_buf[i]=xfer_buf[i-1]+elad_block_size;
elad_open_hw(fdms1_usb_handle, samprates[fdms1.sampling_rate_no]);
for(i = 0; i < no_of_buffers; i++)
  {
  lir_sleep(1000);
  libusb_fill_bulk_transfer(xfer[i],
                            fdms1_usb_handle,
                            0x86,
                            xfer_buf[i],
                            elad_block_size,
                            (libusb_transfer_cb_fn)fdms1_callback,
                            &i,
                            0);
  libusb_submit_transfer(xfer[i]);
  }
set_fdms1_att();
set_fdms1_frequency();
set_elad_lp30(fdms1_usb_handle, &fdms1.filter_enable);
elad_start_fifo(fdms1_usb_handle);
libusb_handle_events_timeout(elad_ctx, &tv_nb);
thread_status_flag[THREAD_FDMS1_STARTER]=THRFLAG_ACTIVE;
while(!kill_all_flag && thread_command_flag[THREAD_FDMS1_STARTER]
                                                          == THRFLAG_ACTIVE)
  {
  libusb_handle_events_timeout(elad_ctx, &tv_nb);
  }
for(i=0; i<no_of_buffers; i++)
  {
  libusb_cancel_transfer(xfer[i]);
  }
lir_sleep(20000);
elad_stop_fifo(fdms1_usb_handle);
free(xfer_buf[0]);
free(xfer_buf);
for(i=0; i<no_of_buffers; i++)libusb_free_transfer(xfer[i]);
free(xfer);
}

void fdms1_input(void)
{
int rxin_local_workload_reset;
int i, errcod, samp;
char s[80];
double dt1, read_start_time, total_reads;
int timing_loop_counter,timing_loop_counter_max,initial_skip_flag;
int local_att_counter;
int local_nco_counter;
float t1;
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_FDMS1_INPUT);
#endif
thread_status_flag[THREAD_FDMS1_INPUT]=THRFLAG_ACTIVE;
local_att_counter=sdr_att_counter;
local_nco_counter=sdr_nco_counter;
i=read_sdrpar(fdms1_parfil_name, MAX_FDMS1_PARM, 
                                     fdms1_parm_text, (int*)((void*)&fdms1));
errcod=1396;
if(i != 0)goto fdms1_init_error_exit;
if( fdms1.check != FDMS1_PAR_VERNR ||
    fdms1.sampling_rate_no < 0 || fdms1.sampling_rate_no >= FDMS1_MAX_RATES ||
    abs(fdms1.freq_adjust) > 30000)goto fdms1_init_error_exit;
t1=((rint)((samprates[fdms1.sampling_rate_no]*
                     (100000000L+(double)fdms1.freq_adjust))/100000000L));
samp=(int)t1;
errcod=1397;
if(samp != ui.rx_ad_speed)goto fdms1_init_error_exit;
timf1_sampling_speed=ui.rx_ad_speed;
dt1=current_time();
elad_handle=NULL;
errcod=0;
while(!elad_handle)
  {
  open_elad_fdms1(TRUE);
  if(!elad_handle)
    {
    sprintf(s,"Waiting %.2f", current_time()-dt1);
    lir_text(3,5,s);
    lir_refresh_screen();
    if(kill_all_flag)goto fdms1_init_error_exit;
    lir_sleep(100000);
    }
  }
// ****************************************************
// We have a connection to the FDM-S1
check_filtercorr_direction();
fft1_block_timing();
if(thread_command_flag[THREAD_SCREEN]!=THRFLAG_NOT_ACTIVE)
  {
  while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
    {
    if(thread_command_flag[THREAD_FDMS1_INPUT] ==
                                           THRFLAG_KILL)goto fdms1_error_exit;
    lir_sleep(10000);
    }
  }
linrad_thread_create(THREAD_FDMS1_STARTER);
#include "timing_setup.c"
while(!kill_all_flag && 
       thread_status_flag[THREAD_FDMS1_STARTER] != THRFLAG_ACTIVE && 
       thread_command_flag[THREAD_FDMS1_INPUT] != THRFLAG_KILL)
  {
  lir_sleep(20000);
  }
timf1p_sdr=timf1p_pa;
thread_status_flag[THREAD_FDMS1_INPUT]=THRFLAG_ACTIVE;
lir_status=LIR_OK;
while(thread_command_flag[THREAD_FDMS1_INPUT] == THRFLAG_ACTIVE)
  {
  if(local_att_counter != sdr_att_counter)
    {
    local_att_counter=sdr_att_counter;
    set_fdms1_att();
    }
  if(local_nco_counter != sdr_nco_counter)
    {
    local_nco_counter=sdr_nco_counter;
    set_fdms1_frequency();
    }
  while(((timf1p_sdr-timf1p_pa+timf1_bytes)&timf1_bytemask) < 
                                                 (int)snd[RXAD].block_bytes)
    {
    if(kill_all_flag || 
       thread_command_flag[THREAD_FDMS1_INPUT] != THRFLAG_ACTIVE)goto fdms1_exit;
    lir_await_event(EVENT_FDMS1_INPUT);
    }
  while (!kill_all_flag && 
      ((timf1p_sdr-timf1p_pa+timf1_bytes)&timf1_bytemask) 
                                         >= (int)snd[RXAD].block_bytes)
    {
#include "input_speed.c"
    finish_rx_read();
    }
  }
fdms1_exit:;
thread_command_flag[THREAD_FDMS1_STARTER]=THRFLAG_KILL;
lir_join(THREAD_FDMS1_STARTER);
fdms1_error_exit:;
close_elad_fdms1();
fdms1_init_error_exit:;
if(errcod != 0)lirerr(errcod);
thread_status_flag[THREAD_FDMS1_INPUT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_FDMS1_INPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(10000);
  }
}


void open_elad_fdms1(int firmware_flag)
{
int i, k, r;
char s[80],ss[80];
libusb_device *dev;
libusb_device **devs;
libusb_device_handle *dev_handle =NULL;
struct libusb_device_descriptor desc;
ssize_t cnt;  
unsigned char buffer[256];
elad_handle=NULL;
elad_init_handle=NULL;
fdms1_usb_handle=NULL;
load_usb1_library(TRUE);
if(!libusb1_library_flag)return;
r=libusb_init(&elad_ctx);
if (r < 0)    
  {
  sprintf(s,"Failed to init libusb rc=%i",r);
errmsg:;
  lir_text(5,5,s);
  lir_text(5,5,press_any_key);
  await_processed_keyboard();
  unload_usb1_library();
  return;
  }
cnt=libusb_get_device_list(elad_ctx, &devs);
if (cnt < 0)   
  {
  sprintf(s,"Failed to get device list rc=%i",(int)cnt);
  goto errmsg;
  }
i=0;
while ((dev = devs[i++]) != NULL) 
  {
  r=libusb_get_device_descriptor(dev, &desc);
  if(r < 0) 
    {
    sprintf(s,"Failed to get device descriptor rc=%i",r);
    goto errmsg;
    }
  if(desc.idVendor == ELAD_VENDOR_ID && desc.idProduct == FDMS1_PRODUCT_ID)
    {
    fdms1_open_flag=libusb_open(dev, &dev_handle);
    if(fdms1_open_flag == 0) 
      {
      libusb_get_string_descriptor_ascii(dev_handle, desc.iManufacturer,   
                                                      buffer, sizeof(buffer)); 
      if(strcmp((const char *)buffer, VENDOR_NAME_ELAD) == 0) 
        {
        libusb_get_string_descriptor_ascii(dev_handle, desc.iProduct,   
                                                   buffer, sizeof(buffer));
        if(strcmp((const char *)buffer, PRODUCT_NAME_FDMS1) == 0) 
          {
          fdms1_usb_handle=dev_handle;
          break;
          }
        else 
          {
          libusb_close(dev_handle);
          }
        }
      }
    } 
  }
libusb_free_device_list(devs,1);
if(!dev_handle)
  {
  clear_screen();
  lir_text(5,5,"No FDM-S1 unit found on the USB system");
  lir_text(5,6,"Please check cables and try again");
  lir_text(13,8,press_any_key);
  await_processed_keyboard(); 
  return;
  }
elad_handle=dlopen(FDMS1_LIBNAME, RTLD_NOW);
if(!elad_handle)
  {
  library_error_screen(FDMS1_LIBNAME, 0);
err1:;
  libusb_close(dev_handle);
  fdms1_usb_handle=NULL;
  unload_usb1_library();
  return;
  }
cmd_extio=(p_cmd_extio)dlsym(elad_handle, "Cmd_ExtIO");
if(!cmd_extio)
  {
symerr:;
  library_error_screen(FDMS1_LIBNAME, 1);
  dlclose(elad_handle);
  goto err1;
  }  
set_elad_lp30=(p_set_elad_lp30)dlsym(elad_handle, "set_en_ext_io_LP30");
if(!set_elad_lp30)goto symerr;
set_elad_att20=(p_set_elad_att20)dlsym(elad_handle, "set_en_ext_io_ATT20");
if(!set_elad_att20)goto symerr;
set_elad_iqswap=(p_set_elad_iqswap)dlsym(elad_handle, "set_en_ext_io_SWAPIQ");
if(!set_elad_iqswap)goto symerr;
set_elad_hwlo=(p_set_elad_hwlo)dlsym(elad_handle, "SetHWLO");
if(!set_elad_hwlo)goto symerr;
elad_open_hw=(p_elad_open_hw)dlsym(elad_handle, "OpenHW");
if(!elad_open_hw)goto symerr;
elad_start_fifo=(p_elad_start_fifo)dlsym(elad_handle, "StartFIFO");
if(!elad_start_fifo)goto symerr;
elad_stop_fifo=(p_elad_stop_fifo)dlsym(elad_handle, "StopFIFO");
if(!elad_stop_fifo)goto symerr;

if(firmware_flag)
  {
  sprintf(s,"%s",FDMS1_LIBNAME);
  i=strlen(s);
  i--;
  while(!(s[i] == '.' && s[i+1] == 's') && i>0)i--;
  if(i<5)
    {
    lirerr(126753);
    return;
    }
#if IA64 == 1
  #if DARWIN == 1
    sprintf(ss,"init_%lld",samprates[fdms1.sampling_rate_no]);
  #else  
    sprintf(ss,"init_%ld",samprates[fdms1.sampling_rate_no]);
  #endif
#else
  sprintf(ss,"init_%lld",samprates[fdms1.sampling_rate_no]);
#endif
  k=strlen(ss);
  while(s[i-1] != 0)  
    {
    ss[k]=s[i];  
    k++;
    i++;
    }
  i=strlen(s);
  i--;
  while(!(s[i] == 'c' && s[i+1] == 't') && i>0)i--;
  if(i<5)
    {
    lirerr(126753);
    return;
    }
  k=0;
  while(ss[k] != 0)  
    {
    s[i]=ss[k];  
    k++;
    i++;
    }
  s[i]=0; 
  elad_init_handle=dlopen(s, RTLD_NOW);
  if(!elad_init_handle)
    {
    library_error_screen(s, 0);
    goto err1;
    }
  fdms1_hw_init=(p_fdms1_hw_init)dlsym(elad_init_handle, "fdms1_hw_init");
  if(fdms1_hw_init == NULL)
    {
    library_error_screen(s, 1);
    dlclose(elad_init_handle);
    goto err1;
    }
  old_fdms1_ratenum=fdms1.sampling_rate_no;    
  i=fdms1_hw_init(fdms1_usb_handle);
  dlclose(elad_init_handle);
  if(i == 0)
    {
    lirerr(1395);
    close_elad_fdms1();
    }
  }
}

void close_elad_fdms1(void)
{
if(elad_handle)dlclose(elad_handle);
libusb_close(fdms1_usb_handle);
unload_usb1_library();
}

int display_fdms1_parm_info(int *line)
{
char s[80];
char ny[2]={'N','Y'};
int errcod;
errcod=read_sdrpar(fdms1_parfil_name, MAX_FDMS1_PARM, 
                               fdms1_parm_text, (int*)((void*)&fdms1));
if(errcod == 0)
  {
  settextcolor(7);
#if IA64 == 1
#if DARWIN == 0
  sprintf(s,"Sampling rate      = %ld, Xtal adjust = %.0f ppb",  
                                       samprates[fdms1.sampling_rate_no],
                                                  10.*fdms1.freq_adjust);
#else
  sprintf(s,"Sampling rate      = %lld, Xtal adjust = %.0f ppb",  
                                       samprates[fdms1.sampling_rate_no],
                                                  10.*fdms1.freq_adjust);
#endif                                                  
#else
  sprintf(s,"Sampling rate      = %lld, Xtal adjust = %.0f ppb",  
                                       samprates[fdms1.sampling_rate_no],
                                                  10.*fdms1.freq_adjust);
#endif
  lir_text(24,line[0],s);
  SNDLOG"\n%s",s);
  line[0]++;
  sprintf(s,"LP filter enabled  = %c, IQ swap = %c",
                                 ny[fdms1.filter_enable^1], ny[fdms1.swap_iq]);
  lir_text(24,line[0],s);
  SNDLOG"\n%s",s);
  line[0]++;
  }
return errcod;
}

void init_elad_fdms1(void)
{
int i, line;
char s[80];
int data;
float t1;
int* sdr_pi;
FILE* fdms1_par_file;
clear_screen();
settextcolor(14);
lir_text(15,0,"Setup for ELAD FDM-S1");
settextcolor(7);
line=4;
for(i=0; i<FDMS1_MAX_RATES; i++)
  {
  sprintf(s,"%2d %4d kHz",i,(int)(samprates[i]/1000));
  lir_text(8,line,s);
  line++;
  }
line++;
sprintf(s,"Select sampling rate by line number 0 to %d =>",FDMS1_MAX_RATES-1);
lir_text(5,line,s);
i=strlen(s);
fdms1.sampling_rate_no=lir_get_integer(i+6,line,1,0,FDMS1_MAX_RATES-1);
line+=2;
settextcolor(13);
if(old_fdms1_ratenum != -1 && old_fdms1_ratenum != fdms1.sampling_rate_no)
  {
  sprintf(s,"Sampling rate changed from %d to %d kHz",
                     (int)(samprates[old_fdms1_ratenum]/1000),
                         (int)(samprates[fdms1.sampling_rate_no]/1000));
  lir_text(2,line,s);
  line++;
  lir_text(2,line,"You must exit and restart Linrad before you can run at the");
  line++;
  lir_text(2,line,"new sampling rate.");
  line+=2;
  }

lir_text(2,line,
         "Do not unplug the USB connector until after exit from Linrad if");
line++;
lir_text(2,line,"you have been running the FDM-S1 under Linux");
line+=2;
lir_text(2,line,"Note that you may have to exit Linrad and restart after");
line++;
lir_text(2,line,"having left the processing screen and that this makes");
line++;
lir_text(2,line,"calibration impossible");
line+=2;
lir_text(2,line,
        "Also note that it takes about 10 seconds for the FDM-S1 to start.");
settextcolor(7);
line+=2;
lir_text(10,line,press_any_key);
await_processed_keyboard();
if(kill_all_flag)return;
settextcolor(7);
line+=2;
open_elad_fdms1(FALSE);
if(!fdms1_usb_handle || kill_all_flag)return;
clear_lines(line,line);
cmd_extio(fdms1_usb_handle,0x9,&data,0);
sprintf(s,"Serial number %d",data);
lir_text(5,line,s);
line+=2;
lir_text(5,line,"Enable 30 MHz low pass filter ? (Y/N)");
line+=2;
fdms1.filter_enable=1;
getfilt:;
await_processed_keyboard();
if(kill_all_flag)goto init_x;
if(lir_inkey == 'Y')
  {
  fdms1.filter_enable=0;
  }
else
  {
  if(lir_inkey != 'N')
    {
    goto getfilt;
    }
  }
lir_text(5,line,"Swap I and Q ? (Y/N)");
line+=2;
fdms1.swap_iq=0;
getswap:;
await_processed_keyboard();
if(kill_all_flag)goto init_x;
if(lir_inkey == 'Y')
  {
  fdms1.swap_iq=1;
  }
else
  {
  if(lir_inkey != 'N')
    {
    goto getswap;
    }
  }
fdms1_par_file=fopen(fdms1_parfil_name,"w");
if(!fdms1_par_file)
  {
  lirerr(277745);
  goto init_x;
  }
sdr_pi=(int*)(&fdms1);
fdms1.check=FDMS1_PAR_VERNR;
for(i=0; i<MAX_FDMS1_PARM; i++)
  {
  fprintf(fdms1_par_file,"%s [%d]\n",fdms1_parm_text[i],sdr_pi[i]);
  }
parfile_end(fdms1_par_file);
ui.rx_addev_no=FDMS1_DEVICE_CODE;
t1=(rint)((samprates[fdms1.sampling_rate_no]*
                     (100000000L+(double)fdms1.freq_adjust))/100000000L);
ui.rx_ad_speed=(int)t1;
ui.rx_input_mode=IQ_DATA;
if(ui.rx_ad_speed<4500000)ui.rx_input_mode+=DWORD_INPUT;
ui.rx_rf_channels=1;
ui.rx_ad_channels=2;
ui.rx_admode=0;
init_x:;
close_elad_fdms1();
}
#endif

