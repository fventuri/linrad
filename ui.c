
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
#include "uidef.h"
#include "screendef.h"
#include "vernr.h"
#include "sigdef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "thrdef.h"
#include "caldef.h"
#include "powtdef.h"
#include "keyboard_def.h"
#include "sdrdef.h"
#include "txdef.h"
#include "options.h"
#include "seldef.h"

#if(OSNUM == OSNUM_LINUX)
#include <pthread.h>
#include <semaphore.h>
#if HAVE_OSS == 1
  #if OSSD == 1
  #include <sys/soundcard.h>
  #else
  #include <soundcard.h>
  #endif
#include <sys/ioctl.h>
#endif
#include "loadalsa.h"
#include "xdef.h"
#include "lscreen.h"
extern snd_pcm_t *rx_ad_handle, *rx_da_handle;
extern snd_pcm_t *tx_ad_handle, *tx_da_handle;

extern int  alsa_dev_seq_nmbr;
extern char alsa_dev_soundcard_name [256];
extern char alsa_dev_name [64];
extern char alsa_dev_hw_pcm_name[16];
extern char alsa_dev_plughw_pcm_name[16];

extern int  alsa_dev_min_rate;
extern int  alsa_dev_max_rate;
extern int  alsa_dev_min_channels;
extern int  alsa_dev_max_channels;
extern int  alsa_dev_min_bytes;
extern int  alsa_dev_max_bytes;
#endif

#if(OSNUM == OSNUM_WINDOWS)
#include "wdef.h"
#include "wscreen.h"
void win_semaphores(void);
#endif

void set_sdrip_att(void);
void set_sdrip_frequency(void);
void set_netafedri_att(void);
void set_netafedri_frequency(void);

// The syscall thread follows these states:
// THRFLAG_SEM_WAIT:    The syscall thread is waiting for a command to be set by the caller.
// THRFLAG_SEM_SET:     The caller acquired SEM_WAIT, the caller will set a command and raise an event. No other caller can raise an event at this point of time.
// THRFLAG_SEMCLEAR:    The syscall thread processed a command. Now the syscall thread is waiting for the caller to acknowledge it.
// THRFLAG_SEMFINISHED: The caller acknowledged an end of a command processing, and the caller raised an event.
// THRFLAG_SEM_WAIT:    The syscall thread waked up from THRFLAG_SEMFINISHED and now it is waiting for another command.
void do_syscall(void)
{
lir_set_thread_flag(THREAD_SYSCALL, THRFLAG_SEM_WAIT);
while(thread_command_flag[THREAD_SYSCALL] != THRFLAG_KILL)
  {
  lir_await_event(EVENT_SYSCALL);
  switch (thread_command_flag[THREAD_SYSCALL])
    {
    case THRFLAG_PORTAUDIO_STARTSTOP:
    portaudio_startstop();
    break;
    
    case THRFLAG_OPEN_RX_SNDIN:
    open_rx_sndin(FALSE);
    break;

    case THRFLAG_CLOSE_RX_SNDIN:
    close_rx_sndin();
    break;

    case THRFLAG_OPEN_RX_SNDOUT:
    open_rx_sndout();
    break;

    case THRFLAG_CLOSE_RX_SNDOUT:
    close_rx_sndout();
    break;

    case THRFLAG_OPEN_TX_SNDIN:
    open_tx_sndin();
    break;

    case THRFLAG_CLOSE_TX_SNDIN:
    close_tx_sndin();
    break;

    case THRFLAG_OPEN_TX_SNDOUT:
    open_tx_sndout();
    break;

    case THRFLAG_CLOSE_TX_SNDOUT:
    close_tx_sndout();
    break;

    case THRFLAG_SET_RX_IO:
    set_rx_io();
    break;

    case THRFLAG_TX_SETUP:
    tx_setup();
    break;

    case THRFLAG_PORTAUDIO_STOP:
    portaudio_stop();
    break;

    case THRFLAG_SET_SDRIP_ATT:
    set_sdrip_att();
    break;
    
    case THRFLAG_SET_SDRIP_FREQUENCY:
    set_sdrip_frequency();
    break;

    case THRFLAG_SET_NETAFEDRI_ATT:
    set_netafedri_att();
    break;
    
    case THRFLAG_SET_NETAFEDRI_FREQUENCY:
    set_netafedri_frequency();
    break;

    case THRFLAG_CALIBRATE_BLADERF_RX:
    calibrate_bladerf_rx();
    break;
    
	default:
	continue;
    }
  // Let the calling thread know we have finished processing a command.
  lir_set_thread_flag(THREAD_SYSCALL, THRFLAG_SEMCLEAR);
  // Wait until the calling thread sets the thread flag to SEM_WAIT.
  lir_await_event(EVENT_SYSCALL);
  // And let the other threads know we are ready to receive commands.
  lir_set_thread_flag(THREAD_SYSCALL, THRFLAG_SEM_WAIT);
  }
// End of the thread.
lir_set_thread_flag(THREAD_SYSCALL, THRFLAG_RETURNED);
}

void sys_func(int no)
{
// Wait until the thread flag is THRFLAG_SEM_WAIT 
// and it was set by this thread to THRFLAG_SEM_SET atomically.
while (! lir_set_thread_flag_if(THREAD_SYSCALL, THRFLAG_SEM_WAIT, THRFLAG_SEM_SET))
  {
  lir_sleep(3000);
  if(kill_all_flag)return;
  }
// The syscall thread will perform the command "no".
thread_command_flag[THREAD_SYSCALL] = no;
// Wake up the syscall thread.
lir_set_event(EVENT_SYSCALL);
// Wait until the syscall thread cleared the flag
// and it was set by this thread to THRFLAG_SEMFINISHED atomically.
while (!lir_set_thread_flag_if(THREAD_SYSCALL, THRFLAG_SEMCLEAR, THRFLAG_SEMFINISHED))
  {
  lir_sleep(3000);
  if(kill_all_flag)return;
  }
// Wake
lir_set_event(EVENT_SYSCALL);
}

bool sys_func_async_start(int no)
{
if (lir_set_thread_flag_if(THREAD_SYSCALL, THRFLAG_SEM_WAIT, THRFLAG_SEM_SET)) {
  // The "SEM_WAIT" lock was acquired, this is the only thread, which may set the command and raise the event.
  // Following line is enclosed between two memory barrier, one caused by lir_set_thread_flag_if,
  // the other caused by lir_set_event, and we are alone inside a critical section THRFLAG_SEM_SET.
  // The following line is therefore safe to call.
  thread_command_flag[THREAD_SYSCALL] = no;
  lir_set_event(EVENT_SYSCALL);
  return true;
} else
  return false;
}

bool sys_func_async_join(int no)
{
if (lir_set_thread_flag_if(THREAD_SYSCALL, THRFLAG_SEMCLEAR, THRFLAG_SEMFINISHED)) {
  // The command was processed by the syscall thread. This thread has acquired the "SEMCLEAR" lock.
  if (thread_command_flag[THREAD_SYSCALL] == no) {
    thread_command_flag[THREAD_SYSCALL] = THRFLAG_ACTIVE;
    lir_set_event(EVENT_SYSCALL);
	return true;
  }
  // The command, which finished, is not THRFLAG_SET_NETAFEDRI_ATT. Raise the critical section
  // and let someone else finish the command.
  lir_set_thread_flag(THREAD_SYSCALL, THRFLAG_SEMCLEAR);
}
return false;
}

void show_snd(int n, char* txt)
{
char *types[4]={"RXAD","RXDA","TXDA","TXAD"};
DEB"\n-----------   %s  %s  ------------------",txt,types[n]);
DEB"\nno_of_blocks %d    tot_frames %d     tot_bytes %d",
        snd[n].no_of_blocks, snd[n].tot_frames, snd[n].tot_bytes);
DEB"\nframesize %d     block_bytes %d    block_frames %d",
      snd[n].framesize, snd[n].block_bytes, snd[n].block_frames);
DEB"\ninterrupt_rate %f     open_flag %d",
       snd[n].interrupt_rate, snd[n].open_flag);
}


void update_snd(int sound_type)
{
int i, k;
#if(OSNUM == OSNUM_LINUX)
int err;
#endif
#if(OSNUM == OSNUM_WINDOWS)
WAVEHDR *whdr;
int j;
#endif
i=0;
buftest:;  
// This routine updates all the buffer info abut one sound device
// It replaces several routines in Linrad versions 03.08 and
// earlier.
// ********************************************************************
// ********************************************************************
//typedef struct audio_buf_info  in OSS
//	int fragments;     / # of available fragments (partially usend ones not counted)/
//	int fragstotal;    / Total # of fragments allocated in buffer/
//	int fragsize;      / Size of a fragment in bytes /
//	int bytes;         / Available space in bytes (includes partially used fragments) /
//
// A fragment in OSS is called a period in ALSA
// Unit of measurement in OSS for fragments is bytes
// Unit of measurement in ALSA  for periods and buffersizes is frames
//
// frame(alsa) == sample(oss) is the number of bytes for one sample.
// snd[xxx].framesize=#channels * #bytes for the selected format
//
// block(linrad) == period(alsa) == fragment(oss) is the block size 
// that we believe the hardware DMA is using. We try to read and write
// to our devices one full block at a time.
// snd[xxx].block_bytes=the number of bytes in one block/period/fragment.
// snd[xxx].block_frames=the number of frames in one block/period/fragment. 
// 

/*
  rx_da_info.fragments=(int)frame_avail/(int) snd[RXDA].block_frames;
  rx_da_info.fragstotal=((int)snd[RXDA].tot_frames)/(int) snd[RXDA].block_frames;
  rx_da_info.fragsize=(int)snd[RXDA].block_frames*snd[RXDA].framesize;
  rx_da_info.bytes=(int)frame_avail*snd[RXDA].framesize;
*/


// ********************************************************
/*
// Structure for soundcard info.
typedef struct {
int no_of_blocks;
int valid_frames;
int min_valid_frames;
int valid_bytes;
int empty_frames;
int empty_bytes;
int tot_frames;
int tot_bytes;
int framesize;
int block_bytes;
int block_frames;
float interrupt_rate;
int open_flag;
}SOUNDCARD_PARM;
*/
// Count how many free blocks or frames we have.
k=0;
switch (sound_type)
  {
  case RXDA:
  if(rx_audio_out == -1) return;
  if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_ACTIVE)
    {
    if( (ui.use_alsa&PORTAUDIO_RX_OUT) != 0)
      {
      snd[sound_type].empty_frames=
                   snd[sound_type].tot_frames-snd[sound_type].block_frames;
      k=snd[sound_type].no_of_blocks-1;
      break;
      }
#if(OSNUM == OSNUM_WINDOWS)
    for(j=0; j< snd[RXDA].no_of_blocks; j++)
      {
      whdr=rxdaout_newbuf[j];
      if( (whdr[0].dwFlags&WHDR_INQUEUE) == 0)k++;
      }
    snd[sound_type].empty_frames=k*snd[sound_type].block_frames;
    break;
#else
    if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
      {
      if(!alsa_library_flag)
        {
        lirerr(269113);
        return;
        }
//Try to synchronize stream position with hardware
      err=snd_pcm_hwsync(rx_da_handle);
//Get # of frames available (for read or write.)
      if(err != 0)
        {
        k=0;
        }
      else
        {  
        k=(int)snd_pcm_avail_update(rx_da_handle);
        }
      if(k < 0)k=0;
      snd[sound_type].empty_frames=k;
      }
    else
#if(HAVE_OSS == 1)
      {
      if(ioctl(rx_audio_out,SNDCTL_DSP_GETOSPACE, &rx_da_info) == -1)
        {
        k=0;
        }
      else
        {
        k=rx_da_info.bytes;
        }
      snd[sound_type].empty_frames=k/snd[sound_type].framesize;
      }
#endif
#endif
//************************************************************
// The number of bytes we can write is incorrectly zero sometimes
// in 4Front OSS. Maybe it could happen in other systems as well.
// wait a little and see if we get a correct value to return.
    if(k == 0 && (sound_type == RXDA || sound_type == TXDA) )
      {
      lir_sleep(200);
      if(kill_all_flag)return;
      i++;
      if(i<3)goto buftest;
      }
    }
  else
    {
    snd[sound_type].empty_frames=snd[sound_type].tot_frames;
    }   
  break;
  
  default:
  lirerr(234177);
  break;
  }
snd[sound_type].empty_bytes=
                  snd[sound_type].empty_frames*snd[sound_type].framesize;
snd[sound_type].valid_frames=
                snd[sound_type].tot_frames-snd[sound_type].empty_frames;
snd[sound_type].valid_bytes=
                 snd[sound_type].tot_bytes-snd[sound_type].empty_bytes;
// ***************************************************************
if(snd[sound_type].min_valid_frames > snd[sound_type].valid_frames)
               snd[sound_type].min_valid_frames=snd[sound_type].valid_frames;
}




double q_time;
static double q_sum_time=0;
static int q_sumnum=0;
static int q_reset=-1;

// Use qt0() and qt3() to get average timing for code sections
// Press Z to clear averages.
void qt0(void)
{
q_time=current_time();
}

void qt3(void)
{
char s[80];
double dd;
if(q_reset != workload_reset_flag)
  {
  q_reset=workload_reset_flag;
  q_sum_time=0;
  q_sumnum=0;
  return;
  } 
dd=current_time();
q_sum_time+=dd-q_time;
q_sumnum++;
if(q_sumnum%32==0)
  {
  sprintf(s,"[%f] ",q_sum_time/q_sumnum);
  qq(s); // OK. Part of qt3.
  }
}

void qt1(char *cc)
{
double dd;
dd=current_time();
DEB"\n%s %f",cc,dd-q_time);
q_time=dd;
}

void qt2(char *cc)
{
double dd;
dd=current_time();
DEB"\n%s %f",cc,dd-q_time);
}

void kill_all(void)
{
int i,j,k;
fflush(NULL);
kill_all_flag=TRUE;
lir_sleep(300000);
DEB"\n\nEnter kill_all()");
// Sleep for a while to make sure that the first error
// code (if any) has been saved by lirerr.
// When we start to stop threads we may induce new errors
// and they must not overwrite the original error code
// which is ensured by lirerr watching kill_all_flag.
// *******************************************************
close_network_sockets();
k=0;
if(dmp != NULL)
  {
  fflush(dmp);
  lir_sync();
  }
for(i=0; i<THREAD_MAX; i++)
  {
  lir_sched_yield();
  if(i != THREAD_SYSCALL && thread_command_flag[i] != THRFLAG_NOT_ACTIVE)
    {
    if(thread_waitsem[i] != -2)
      {
      k++;
      thread_command_flag[i]=THRFLAG_KILL;
      if(thread_waitsem[i] != -1)
        {
        lir_set_event(thread_waitsem[i]);
        }
      if(i == THREAD_RX_OUTPUT)
        {
        lir_set_event(EVENT_BASEB);
        }
      lir_sleep(100);
      }
    }
  }
#if(OSNUM == OSNUM_WINDOWS)
win_semaphores();
#endif
DEB"\nremaining no of threads=%d",k);    
if(k==0)goto ok_exit;
j=-5;
fprintf( stderr,"\n");
while(j<10)
  {
  j++;
  if(j > 0)fprintf( stderr,"%d ",j);
  fflush(NULL);
  lir_sleep(10000);  
  k=0;
  for(i=0; i<THREAD_MAX; i++)
    {
    if(i != THREAD_SYSCALL)
      {
      if(thread_status_flag[i] == THRFLAG_RETURNED  ||
         thread_status_flag[i] == THRFLAG_NOT_ACTIVE)
        {
        thread_command_flag[i]=THRFLAG_NOT_ACTIVE;
        }
      else
        {  
        if(thread_command_flag[i] != THRFLAG_NOT_ACTIVE)
          {
          k++;
          if(thread_waitsem[i] != -2)
            {
            if(thread_waitsem[i] != -1)
              {
              lir_set_event(thread_waitsem[i]);
              }
            if(i == THREAD_RX_OUTPUT)
              {
              lir_set_event(EVENT_BASEB);
              }
            thread_command_flag[i]=THRFLAG_KILL;
            }
          }
        }
      }
    }  
  if(k==0)
    {
ok_exit:;
    threads_running=FALSE;
    DEB"\n\nALL THREADS STOPPED");
    fflush(NULL);
    return;
    }
  }
if(dmp != 0)
  {
  PERMDEB"\n\nTHREAD(S) FAIL TO STOP");
  for(i=0; i<THREAD_MAX; i++)
    {
    PERMDEB"\nThr=%d status=%d cmd %d",i,thread_status_flag[i],
                                              thread_command_flag[i]);
    }  
  PERMDEB"\n");
  fflush(dmp);
  }
// Stay here until ctrlC or kill button in Windows or X11
show_errmsg(2);
i=0;
lir_refresh_screen();
zz:;
i++;
lir_sleep(10000);
if(i > 500)abort();
goto zz;
}

void pause_thread(int no)
{
int i;
lir_sched_yield();
if( thread_command_flag[no]==THRFLAG_NOT_ACTIVE ||
    thread_status_flag[no]==THRFLAG_RETURNED || kill_all_flag)return;
i=0;
if(thread_command_flag[no]==THRFLAG_IDLE)
  {
  while(i<100 && thread_command_flag[no] == THRFLAG_IDLE)
    {
    lir_sleep(5000); 
    if(kill_all_flag)return;
    i++;
    }
  if(i >= 100)
    {
    lirerr(1224);
    return;
    }
  }
thread_command_flag[no]=THRFLAG_IDLE;
if( no ==THREAD_RX_OUTPUT)
  {
  lir_set_event(EVENT_BASEB);
  }
for(i=0; i<4; i++)
  {
  if( (ui.network_flag&NET_RXIN_FFT1) != 0 && no==THREAD_SCREEN &&
                    thread_status_flag[THREAD_SCREEN] == THRFLAG_SEM_WAIT)
    {
    awake_screen();
    }
  lir_sched_yield();
  if(thread_status_flag[no] == THRFLAG_IDLE)return;
  }
while(thread_status_flag[no] != THRFLAG_IDLE  && !kill_all_flag)
  {
  if( no==THREAD_SCREEN &&
                    thread_status_flag[THREAD_SCREEN] == THRFLAG_SEM_WAIT)
    {
    awake_screen();
    }
  if( no ==THREAD_RX_OUTPUT)
    {
    lir_set_event(EVENT_BASEB);
    }
  lir_sleep(3000);
  }
}

void resume_thread(int no)
{
// Make sure the input is allowed to take care of its data
// before heavy processing is released.
// This is occasionally needed for the sdr-14.
lir_sched_yield();
if(thread_command_flag[no]!=THRFLAG_IDLE || kill_all_flag)return;
thread_command_flag[no]=THRFLAG_ACTIVE;
}

void pause_screen_and_hide_mouse(void)
{
pause_thread(THREAD_SCREEN);
unconditional_hide_mouse();
}

void linrad_thread_stop_and_join(int no)
{
int k;
if(thread_command_flag[no] == THRFLAG_NOT_ACTIVE)
  {
  if(kill_all_flag)return;
  lirerr(765934);
  return;
  }
thread_command_flag[no]=THRFLAG_KILL;
if(thread_waitsem[no] != -1)
  {
  lir_set_event(thread_waitsem[no]);
  }
if(no == THREAD_RX_OUTPUT)lir_set_event(EVENT_BASEB); 
lir_sched_yield();
k=0;
while(thread_status_flag[no] != THRFLAG_RETURNED)
  {
  if(thread_waitsem[no] != -1)
    {
    lir_set_event(thread_waitsem[no]);
    }
  lir_sleep(3000);
  k++;
  if(no==THREAD_SDR14_INPUT)
    {
    if(k==100 && sdr!=-1)close_sdr14();
    }
  if(no==THREAD_PERSEUS_INPUT)
    {
    if(k==1000 && sdr!=-1)close_perseus();
    }
  }
thread_command_flag[no]=THRFLAG_NOT_ACTIVE;
lir_join(no);
thread_status_flag[no]=THRFLAG_NOT_ACTIVE;
}

void lir_text(int x, int y, char *txt)
{
char s[512];
int i,j;
if(x<0 || y<0 || y > screen_last_line)return;
i=0;
while(txt[i] != 0)i++;
if(x+i <= screen_last_col)
  {
  lir_pixwrite(x*text_width, y*text_height+1,txt);
  }
else 
  {
  j=1+screen_last_col-x;
  if(j > 0)
    {
    for(i=0; i<j; i++)s[i]=txt[i];
    s[j]=0;
    lir_pixwrite(x*text_width, y*text_height,s);
    }
  }  
}

void lir_pixwrite(int x0, int y, char *txt)
{
int i, x;
unsigned int k;
#if(OSNUM == OSNUM_WINDOWS)
if (lir_pixwrite_GDI(x0, y, txt))
  return;
#endif
i=-1;
if(x0>=0)
  {
  if(y>=0)
    {
    if(y+text_height-font_ysep < screen_height)
      {
      i=0;
      x=x0;
      while(txt[i] != 0)
        {
        k=(unsigned char)(txt[i]);
        if(x > screen_width)goto errx;
        lir_putbox(x,
                   y,
                   font_w,
                   font_h,
                   (size_t*)(&vga_font[(int)k*font_size]));
        i++;
        x+=text_width;
        } 
      return;
      }
    }
  }      
errx:;
fprintf( stderr,"\nTrying to write text outside screen.");
fprintf( stderr,"\nText=[%s] x=%d y=%d",txt,x0,y);
}

void h_line_error(int x1, int y, int x2)
{
fprintf( stderr,
   "\nTrying to write horizontal line outside screen x=%d to %d, y=%d",
       x1,x2,y);
}

void lir_setcross(int x, int y, unsigned char color)
{
lir_setpixel(x,y,color);
lir_setpixel(x+1,y+1,color);
lir_setpixel(x+1,y-1,color);
lir_setpixel(x-1,y+1,color);
lir_setpixel(x-1,y-1,color);
}

void setpixel_error(int x, int y)
{
fprintf( stderr, "\nTrying to set pixel outside screen x=%d, y=%d",x, y);
}

void getpixel_error(int x, int y)
{
fprintf( stderr, "\nTrying to set pixel outside screen x=%d, y=%d",x, y);
}

void putbox_error(int x, int y, int w, int h)
{
fprintf( stderr, 
      "\nTrying to put box outside screen x=%d, y=%d, width=%d height=%d",
                                                                  x, y, w, h);
}

void fillbox_error(int x, int y, int w, int h)
{
fprintf( stderr, 
      "\nTrying to fill box outside screen x=%d, y=%d, width=%d height=%d",
                                                                  x, y, w, h);
}

void aw_keyb(void)
{
while(keyboard_buffer_ptr == keyboard_buffer_used)
  {
  if(keyboard_buffer_ptr == keyboard_buffer_used)
    {
    lir_await_event(EVENT_KEYBOARD);
    if(kill_all_flag) return;
    }
  }

lir_inkey = keyboard_buffer[keyboard_buffer_used];
keyboard_buffer_used=(keyboard_buffer_used+1)&(KEYBOARD_BUFFER_SIZE-1);
lir_sleep(10000);
}

void await_keyboard(void)
{
lir_refresh_screen();
aw_keyb();
}

void init_semaphores(void)
{
int i;
for(i=0; i<EVENT_AUTOINIT_MAX; i++)lir_init_event(i);
}

void free_semaphores(void)
{
int i;
// For some bizarre reason we have to wait here.
// Windows XP will crash when EVENT_SCREEN is closed
// even though all threads using it have been closed. (???)
// This problemoccurs on some platforms when input is
// from a disk file.
lir_sleep(50000);
//if(FBDEV == 0)fflush(NULL);
lir_sleep(50000);
for(i=0; i<EVENT_AUTOINIT_MAX; i++)lir_close_event(i);
}

void do_xshift(int dir)
{
float t1;
if(cal_domain == 0)
  {
  t1=(float)fft2_size;
  }
else
  {
  t1=(float)fft1_size;
  }
t1*=0.1F*cal_xgain;
cal_xshift=(int)((float)cal_xshift+t1*(float)dir);
}


void user_command(void)
{
int i, m, flag;
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_USER_COMMAND);
#endif
thread_status_flag[THREAD_USER_COMMAND]=THRFLAG_ACTIVE;
while(thread_command_flag[THREAD_USER_COMMAND] == THRFLAG_ACTIVE)
  {
// **********************************************
// Get input from the user and do whatever.......
  if( usercontrol_mode != USR_NORMAL_RX)lir_refresh_screen();
  aw_keyb();
  process_current_lir_inkey();
  lir_sleep(5000);
  if(kill_all_flag)
    {
    goto user_error_exit;
    }
  if(numinput_flag != TEXT_PARM)
    {
    if(lir_inkey == 'X')
      {
      i=0;
      DEB"\n'X' Pressed");
      set_button_states();
      while(rightpressed != BUTTON_IDLE || leftpressed != BUTTON_IDLE ||
          mouse_task != -1)
        {
        i++;
        if(i > 50)
          {
          clear_screen();
          lir_text(10,10,"WAITING FOR MOUSE BUTTON TO BE RELEASED");
          }
        lir_sleep(20000);
        set_button_states();
        mouse_nothing();
        }
      goto user_exit;
      }
    if(lir_inkey == 'Z') 
      {
      workload_reset_flag++;
      goto ignore;
      }
    if(lir_inkey == ALT_Z_KEY) 
      {
      fft1corr_reset_flag++;
      goto ignore;
      }
    }
  if(numinput_flag != 0)
    {
    get_numinput_chars();
    }
  else
    {
    switch(usercontrol_mode)
      {
      case USR_NORMAL_RX:
      switch (lir_inkey)
        {
        case 'A':
        pause_thread(THREAD_SCREEN);
        ampinfo_flag^=1;
        amp_info_texts();
        timinfo_flag=0;
        resume_thread(THREAD_SCREEN);
        workload_reset_flag++;
        break;

        case '+':
        m=bg.wheel_stepn;
        m++;
        if(m>30)m=30;
        bg.wheel_stepn=m;
        make_modepar_file(GRAPHTYPE_BG);
        sc[SC_SHOW_WHEEL]++;
        break;

        case '-':
        m=bg.wheel_stepn;
        m--;
        if(m<-30)m=-30;
        if( (genparm[AFC_ENABLE]==0 || genparm[AFC_LOCK_RANGE]==0) && m<0)m=0;
        bg.wheel_stepn=m;
        make_modepar_file(GRAPHTYPE_BG);
        sc[SC_SHOW_WHEEL]++;
        break;

        case 'E':
        if(genparm[AFC_ENABLE] == 1 && genparm[MAX_NO_OF_SPURS] != 0)
                                                          spurinit_flag++;
        break;

        case 'F':
        if(diskread_flag >= 2  && thread_status_flag[THREAD_RX_FILE_INPUT]
                                     == THRFLAG_ACTIVE)edit_diskread_times();
        break;
      
        case 'C':
        if(genparm[AFC_ENABLE] != 0)spurcancel_flag++;
        break;

        case 'Q':
        userdefined_q();
        break;

        case 'S':
        if(abs(fg.passband_direction) != 1)break;
        if(diskwrite_flag == 0)
          {
          disksave_start();
          }
        else
          {  
          disksave_stop();
          }
        break;

        case 'T':
        timinfo_flag^=1;
        if(timinfo_flag!=0)time_info_time=current_time();
        pause_thread(THREAD_SCREEN);
        timing_info_texts();
        resume_thread(THREAD_SCREEN);
        workload_reset_flag++;
        break;

        case 'U':
        userdefined_u();
        break;

        case ALT_W_KEY:
        wavfile_squelch_flag=1;
        goto wavcont;
        
        case 'W':
        wavfile_squelch_flag=0;
wavcont:;
        wavsave_start_stop(0);
        if( wav_write_flag < 0)wav_write_flag=0;
        break;      
    
        case F1_KEY:
        case '!':
        help_screen_objects();
        break;

        case F2_KEY:
        if(s_meter_avgnum <0)
          {
          s_meter_avg=0;
          s_meter_avgnum=0;
          }
        else
          {
          s_meter_avgnum=-1;
          }
        break;

        case F3_KEY:
        audio_dump_flag^=1;
        audio_dump_flag&=1;
        lir_fillbox(0,0,6,6,(unsigned char)(12*audio_dump_flag));
        lir_refresh_screen();
        break;

        case F5_KEY:
        computation_pause_flag^=1;
        computation_pause_flag&=1;
        lir_fillbox(16,0,6,6,(unsigned char)(13*computation_pause_flag));
        break;


        case F8_KEY:
        internal_generator_att++;
        if(internal_generator_att > 12)internal_generator_att=0;
        break;

        case F9_KEY:
        truncate_flag<<=1;
        truncate_flag++;
        if( (ui.rx_input_mode&DWORD_INPUT) == 0)
          {
          if(truncate_flag > 0x3ff)truncate_flag=0;
          }
        else
          {
          if(truncate_flag > 0x3ffff)truncate_flag=0;
          }
        break;

        case F11_KEY:
        internal_generator_flag^=1;
        break;

        case F7_KEY:
        internal_generator_noise++;
        if( (ui.rx_input_mode&DWORD_INPUT) == 0)
          {
          if(internal_generator_noise > 16)internal_generator_noise=0;
          }
        else
          {
          if(internal_generator_noise <10)internal_generator_noise=10;
          if(internal_generator_noise > 48)internal_generator_noise=0;
          }
        break;

        case SHIFT_F3_KEY:
        diskread_pause_flag^=1;
        diskread_pause_flag&=1;
        lir_fillbox(8,0,6,6,(unsigned char)(14*diskread_pause_flag));
        lir_refresh_screen();
        break;
      
        case ARROW_UP_KEY:
        if(genparm[CW_DECODE_ENABLE] != 0)
          {
          i=cg_osc_offset_inc;
          cg_osc_offset_inc=(int)((float)(cg_osc_offset_inc)*1.3F);
          if(i==cg_osc_offset_inc)cg_osc_offset_inc++;
          if(cg_osc_offset_inc>(baseband_size<<3) )
                                          cg_osc_offset_inc=baseband_size<<3;
          pause_thread(THREAD_SCREEN);
          shift_cg_osc_track(0);
          resume_thread(THREAD_SCREEN);
          }
        else
          {
          bg.filter_flat=(float)(bg_hz_per_pixel*(bg_flatpoints*1.2+2));
          bg_update_filter++;
          }
        break;
        
        case ARROW_DOWN_KEY:
        if(genparm[CW_DECODE_ENABLE] != 0)
          {
          i=cg_osc_offset_inc;
          cg_osc_offset_inc=(int)((float)(cg_osc_offset_inc)/1.3F);
          if(cg_osc_offset_inc==0)cg_osc_offset_inc=1;
          pause_thread(THREAD_SCREEN);
          shift_cg_osc_track(0);
          resume_thread(THREAD_SCREEN);
          }
        else
          {
          if(bg_flatpoints > 1)
            {
            bg.filter_flat=bg_hz_per_pixel*((float)(bg_flatpoints)*0.85F-2);
            bg_update_filter++;
            }
          }
        break;

        case ARROW_RIGHT_KEY:
        if(genparm[CW_DECODE_ENABLE] != 0)
          {
          pause_thread(THREAD_SCREEN);
          shift_cg_osc_track(cg_osc_offset_inc);
          resume_thread(THREAD_SCREEN);
          }
        else
          {
          step_rx_frequency(1);
          }             
        break;
        
        case ARROW_LEFT_KEY:
        if(genparm[CW_DECODE_ENABLE] != 0)
          {
          pause_thread(THREAD_SCREEN);
          shift_cg_osc_track(-cg_osc_offset_inc);
          resume_thread(THREAD_SCREEN);
          }
        else
          {
          step_rx_frequency(-1);
          }
        break;

        case ' ':
        copy_rxfreq_to_tx();
        break;

        case 'V':
        case 'B':
        case 'N':
        copy_txfreq_to_rx();
        break;

        case 'R':
        fg.tune_frequency=mix1_selfreq[0];
        make_modepar_file(GRAPHTYPE_FG);
        break;
        
        case 'M':
        bg.horiz_arrow_mode++;
        if(bg.horiz_arrow_mode > 2)bg.horiz_arrow_mode=0;
        make_modepar_file(GRAPHTYPE_BG);
        sc[SC_BG_BUTTONS]++;
        break;

        }
      break;  

      case USR_TXTEST:
      switch (lir_inkey)
        {
        case F4_KEY:
        lir_status=LIR_POWTIM;
        goto user_exit;      
        }
      break;

      case USR_POWTIM:
      switch (lir_inkey)
        {
        case '+':
        powtim_gain*=2;
        break;
      
        case '-':
        powtim_gain/=1.8F;
        break;

        case ARROW_UP_KEY:
        powtim_fgain*=2;
        break;
      
        case ARROW_DOWN_KEY:
        powtim_fgain/=1.8F;
        break;

        case 'D':
        if(powtim_xstep < 5)
          {
          powtim_xstep--;
          }
        else
          {
          powtim_xstep=(int)(powtim_xstep/1.5);
          }
        if(powtim_xstep<1)powtim_xstep=1;     
        break;
      
        case 'I':
        if(powtim_xstep < 5)
          {
          powtim_xstep++;
          }
        else
          {
          powtim_xstep*=2;
          }
        if(powtim_xstep>timf2pow_mask/16)powtim_xstep=timf2pow_mask/16;  
        break;

        case 'L':
        powtim_displaymode^=1;
        break;

        case 'W':
        powtim_displaymode^=2;
        break;

        case 'M':
        powtim_trigmode++;
        if(powtim_trigmode >= POWTIM_TRIGMODES)powtim_trigmode=0;
        break;

        case 'P':
        powtim_pause_flag=1;
        break;
        
        case 'R':
        powtim_pause_flag=0;
        break;

        if(powtim_pause_flag != 0)
          {
          case ARROW_RIGHT_KEY:
          timf2_pn2=(timf2_pn2-screen_width*powtim_xstep/4+timf2_mask+1)&timf2_mask;
          break;

          case ARROW_LEFT_KEY:
          timf2_pn2=(timf2_pn2+screen_width*powtim_xstep/5)&timf2_mask;
          break;
          }        
        }
      if(kill_all_flag) goto user_error_exit;
      powtim_parmwrite();
      if(powtim_pause_flag != 0) powtim_screen();
      break;

      case USR_ADTEST:
      adtest_new++;
      switch (lir_inkey)
        {
        case '+':
        if(adtest_scale < BIGFLOAT/2)adtest_scale*=2;
        break;

        case '-':
        if(adtest_scale > 2/BIGFLOAT)adtest_scale/=2;
        break;

        case 'C':
        if(ui.rx_rf_channels == 2)adtest_channel^=1;
        break;

        case 'M':
        if(ui.rx_rf_channels == 2)adtest_iq_mode^=1;
        break;

        case 'P':
        powtim_pause_flag=1;
        break;
        
        case 'R':
        powtim_pause_flag=0;
        break;

        case 'W':
        powtim_displaymode^=1;
        break;
        }
      break;
        
      case USR_IQ_BALANCE:
      pause_thread(THREAD_CAL_IQBALANCE);
      caliq_clear_flag=TRUE;
      switch (lir_inkey)
        {
        case '+':
        bal_segments*=2;
        break;
          
        case '-':
        bal_segments/=2;
        break;
          
        case 'S':
        write_iq_foldcorr();
        break;
          
        case 'U':
        update_iq_foldcorr();
        break;
          
        case 'C':
        clear_iq_foldcorr();
        break;

        case F1_KEY:
        case '!':
        help_message(311);
        caliq_clear_flag=FALSE;
        break;

        default:
        caliq_clear_flag=FALSE;
        break;
        }
      clear_keyboard();
      resume_thread(THREAD_CAL_IQBALANCE);
      break;

      case USR_CAL_INTERVAL:
      pause_thread(THREAD_CAL_INTERVAL);
      switch (lir_inkey)
        {
        case '+':
        cal_ygain*=2;
        break;
          
        case '-':
        cal_ygain/=2;
        break;

        case 'E':
        cal_xgain*=2;
        break;
          
        case 'C':
        cal_xgain/=2;
        break;
        
        case F1_KEY:
        case '!':
        help_message(304);
        break;
        
        case 10:
        DEB"\nENTER Pressed");
        usercontrol_mode=USR_CAL_FILTERCORR;
        goto user_exit;
        }
      clear_keyboard();
      resume_thread(THREAD_CAL_INTERVAL);
      break;

      case USR_CAL_FILTERCORR:
      pause_thread(THREAD_CAL_FILTERCORR);
      flag=THRFLAG_ACTIVE;  
      switch (lir_inkey)
        {
        case '+':
        cal_ygain*=2;
        break;
          
        case '-':
        cal_ygain/=2;
        break;

        case 'A':
        clear_fft1_filtercorr();
        make_cal_fft1_filtercorr();
        flag=THRFLAG_RESET;
        break;

        case 'E':
        cal_xgain*=2;
        break;
          
        case 'C':
        cal_xgain/=2;
        break;

        case 'S':
        write_filcorr(-1);
        break;
          
        case 'T':
        cal_domain++;
        if(cal_domain > MAX_CAL_DEBUG)cal_domain=0;
        break;

        case 'I':
        break;
          
        case 'U':
        if(cal_buf4[0]>=0)
          {
          if(cal_update_ram() == 0)
            {
            linrad_thread_stop_and_join(THREAD_CAL_FILTERCORR);
            goto user_exit;
            }
          flag=THRFLAG_RESET;
          }
        break;

        case ARROW_LEFT_KEY:
        do_xshift(1);
        break;
        
        case ARROW_RIGHT_KEY:
        do_xshift(-1);
        break;

        case F1_KEY:
        case '!':
        help_message(305);
        flag=THRFLAG_RESET;
        break;

        }
      thread_command_flag[THREAD_CAL_FILTERCORR]=(char)flag;
      while(thread_status_flag[THREAD_CAL_FILTERCORR] != THRFLAG_ACTIVE)
        {
        lir_sleep(2000);
        if(kill_all_flag) goto user_error_exit;
        }
      break;

      case USR_TUNE:
      if(thread_command_flag[THREAD_TUNE]==THRFLAG_ACTIVE)
        {
        thread_command_flag[THREAD_TUNE]=THRFLAG_IDLE;
        while(thread_status_flag[THREAD_TUNE]!=THRFLAG_IDLE)
          {
          lir_sleep(5000);
          if(kill_all_flag) goto user_error_exit;
          }
        switch(lir_inkey)
          {
          case '+':
          tune_yzer-=100;
          break;

          case '-':
          tune_yzer+=100;
          break;
          } 
        thread_command_flag[THREAD_TUNE]=THRFLAG_ACTIVE;
        while(thread_status_flag[THREAD_TUNE]!=THRFLAG_ACTIVE)
          {
          lir_sleep(5000);
          if(kill_all_flag) goto user_error_exit;
          }
        }  
      break;
      }
    }
ignore:;
  if(kill_all_flag) goto user_error_exit;
  }
user_exit:;
thread_status_flag[THREAD_USER_COMMAND]=THRFLAG_RETURNED;
return;
user_error_exit:;
if(dmp!=NULL)fflush(dmp);
thread_status_flag[THREAD_USER_COMMAND]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_USER_COMMAND] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }  
}

void xz(char *s)
{
if(dmp == NULL)
  {
  lirerr(1074);
  lir_sleep(100000);
  return;
  }
PERMDEB" %s\n",s);
fflush(dmp);
lir_sync();
}

void xq(char *s)
{
if(dmp == NULL)
  {
  lirerr(1074);
  lir_sleep(100000);
  return;
  }
fprintf( stderr," %s\n",s);
fflush(stderr);
lir_sync();
}

void test_keyboard(void)
{
if(kill_all_flag!=0)return;
if(keyboard_buffer_ptr != keyboard_buffer_used)
  {
  lir_await_event(EVENT_KEYBOARD);
  lir_inkey = keyboard_buffer[keyboard_buffer_used];
  keyboard_buffer_used=(keyboard_buffer_used+1)&(KEYBOARD_BUFFER_SIZE-1);
  }
else
  {
  lir_inkey=0;  
  }
}


void clear_keyboard(void)
{
test_keyboard();
if(kill_all_flag) return;
while(lir_inkey != 0)
  {
  test_keyboard();
  if(kill_all_flag) return;
  }
}


void clear_await_keyboard(void)
{
clear_keyboard();
if(kill_all_flag) return;
await_keyboard();
}



int qnqcnt=0;
int qnqcnt1=0;
int qnqcnt2=0;
#define QQLINE 0
void qq(char *s)
{
char w[80];
sprintf(w,"%s%3d",s,qnqcnt);
lir_text(5,QQLINE,w);
lir_refresh_screen();
qnqcnt++;
if(qnqcnt > 999)qnqcnt=0;
}

void qq1(char *s)
{
char w[80];
sprintf(w,"%s%3d",s,qnqcnt1);
lir_text(15,QQLINE,w);
lir_refresh_screen();
qnqcnt1++;
if(qnqcnt1 > 999)qnqcnt1=0;
}

void qq2(char *s)
{
char w[80];
sprintf(w,"%s%3d",s,qnqcnt2);
lir_text(25,QQLINE,w);
lir_refresh_screen();
qnqcnt2++;
if(qnqcnt2 > 999)qnqcnt2=0;
}

void show_button(BUTTONS *butt, char *s)
{
int ix1, ix2, iy1, iy2;
ix1=butt[0].x1;
ix2=butt[0].x2;
iy1=butt[0].y1;
iy2=butt[0].y2;
if( ix1 < 0 ||
    ix1 >= screen_width ||
    iy1 < 0 ||
    iy2 >= screen_height )
  {
  return;
  }
lir_hline(ix1,iy1,ix2,button_color);
if(kill_all_flag) return;
lir_hline(ix1,iy2,ix2,button_color);
if(kill_all_flag) return;
lir_line(ix1,iy1,ix1,iy2,button_color);
if(kill_all_flag) return;
lir_line(ix2,iy1,ix2,iy2,button_color);
if(kill_all_flag) return;
lir_pixwrite(ix1+3,iy1+2,s);
}


void update_bar(int x1, int x2, int yzer, int newy, 
                int oldy, unsigned char color,char* buf)
{
int i, k, height, width;
if(newy == oldy)return;
if(oldy < 0)oldy=yzer;
width=x2-x1+1;
if(newy < oldy)
  {
  height=oldy-newy+1;
  lir_getbox(x1,newy,width,height,(size_t*)buf);
  k=width*height;
  for(i=0; i<k; i++)
    {
    if(buf[i] != 7) 
      {
      buf[i]=(char)color;
      }
    }
  lir_putbox(x1,newy,width,height, (size_t*)buf);
  }
else
  {
  height=newy-oldy+1;
  lir_getbox(x1,oldy,width,height, (size_t*)buf);
  k=width*height;
  for(i=0; i<k; i++)
    {
    if(buf[i] != 7) 
      {
      buf[i]=0;
      }
    }
  lir_putbox(x1,oldy,width,height, (size_t*)buf);
  }
}

int make_power_of_two( int *i)
{
int k;
k=-1;
i[0]+=i[0]-1;
while(i[0] != 0)
  {
  i[0]/=2;
  k++;
  }
i[0]=1<<k;
return k;
}

void *chk_free(void *p)
{
if(p==NULL)return NULL;
free(p);
return NULL;
}

void process_current_lir_inkey(void)
{
if(lir_inkey == 13)lir_inkey = 10;
lir_inkey=toupper(lir_inkey);
if(lir_inkey == 'G')
  {
  if(fft1_handle==NULL)
    {
    save_screen_image();
    }
  else
    {  
    if( numinput_flag != TEXT_PARM)
      {
      save_screen_image();
      lir_inkey=0;
      }
    }
  }
}



int lir_get_filename(int  x, int y, char *name)
{
int len;
len=0;
next_char:;
name[len]=0;
lir_text(x,y,name);
await_keyboard();
if(kill_all_flag)return 0;
if(lir_inkey == 10 || lir_inkey == 13) return len;
if(lir_inkey == 8 || 
   lir_inkey == ARROW_LEFT_KEY ||
   lir_inkey == 127)
  {
  if(len==0)goto next_char;
  name[len-1]=' ';
  lir_text(x,y,name);  
  len--;
  goto next_char;
  }
if(  (lir_inkey >=  '.' && lir_inkey <= '9') ||
     (lir_inkey >= 'A' && lir_inkey <= 'Z') ||
     lir_inkey == '_' || 
     lir_inkey == '-' || 
     lir_inkey == '+' || 
#if(OSNUM == OSNUM_WINDOWS)
     lir_inkey == ':' || 
     lir_inkey == '\\' || 
#endif
     (lir_inkey >= 'a' && lir_inkey <= 'z'))
  {
  name[len]=(char)lir_inkey;
  if(len<79)len++;
  }
goto next_char;  
}

int lir_get_text(int  x, int y, char *txtbuf)
{
int len;
len=0;
next_char:;
txtbuf[len]='_';
txtbuf[len+1]=0;
lir_text(x,y,txtbuf);
await_keyboard();
if(kill_all_flag)return 0;
if(lir_inkey == 10 || lir_inkey == 13 )
  {
  txtbuf[len]=0;
  return len;
  }
if(lir_inkey == 8 || 
   lir_inkey == ARROW_LEFT_KEY ||
   lir_inkey == 127)
  {
  if(len==0)goto next_char;
  txtbuf[len]=' ';
  len--;
  txtbuf[len]='_';
  lir_text(x,y,txtbuf);  
  txtbuf[len+1]=0;
  goto next_char;
  }
if(lir_inkey >= ' ' && lir_inkey <= 'z')
  {
  txtbuf[len]=(char)lir_inkey;
  if(len<78)len++;
  }
goto next_char;  
}


int lir_get_integer(int  x, int y, int len, int min, int max)
{
char s[16];
int i, j, pos;
if(len < 1 || len > 15)
  {
  lirerr(8);
  return min;
  }
for(i=1; i<len; i++)s[i]=' '; 
s[0]='_';
s[len]=0;
lir_text(x,y,s);
pos=0;
digin:;
await_processed_keyboard();
if(kill_all_flag)return min;
switch (lir_inkey)
  {
  case F1_KEY:
  case '!':
  return min;

  case 10: 
  if(pos != 0)
    {
    j=atoi(s);
    s[pos]=' ';
    if(j < min)
      {
      j=min;
      sprintf(s,"%d ",min);
      }
    lir_text(x,y,s);
    return j;
    }
  break;

  case 8:
  case 127:
  case ARROW_LEFT_KEY:
  if(pos > 0)
    {
    s[pos+1]=0;
    s[pos]=' ';
    pos--;
    s[pos]='_';
    lir_text(x,y,s);
    s[pos+1]=0;
    }
  break;
  
  case '-':
  if(min < 0)
    {
    if(pos == 0)
      {
      s[pos]='-';
      pos++;
      s[pos]=0;
      }
    lir_text(x,y,s);
    }  
  break;      

  default:
  if(lir_inkey>='0' && lir_inkey <='9')
    {
    if(pos < len)
      {
      s[pos]=(char)lir_inkey;
      pos++;
      s[pos]=0;
      j=atoi(s);
      if(s[0] != '-')
        {
        if(j > max)sprintf(s,"%d",max);
        }
      else
        {  
        if(j < min)sprintf(s,"%d",min);
        }
      pos=0;
      while(s[pos] != 0 && s[pos] != ' ')pos++;
      s[pos]='_';
      j=pos+1;
      while( j< len)
        {
        s[j]=' ';
        j++;
        }
      s[len]=0;    
      lir_text(x,y,s);
      s[pos+1]=0;
      }
    }
  break;
  }
goto digin;
}

double lir_get_float(int  x, int y, int len, double min, double max)
{
char s[16];
int i, j, pos;
double t1;
if(len < 1 || len > 15)
  {
  lirerr(8);
  return min;
  }
for(i=1; i<len; i++)s[i]=' '; 
s[0]='_';
s[len]=0;
lir_text(x,y,s);
pos=0;
digin:;
await_processed_keyboard();
if(kill_all_flag)return min;
switch (lir_inkey)
  {
  case 10: 
  if(pos != 0)
    {
    t1=atof(s);
    if(t1 < min)
      {
      sprintf(s,"%f",min);
      t1=min;
      lir_text(x,y,s);
      }
    return t1;
    }
  break;
  
  case '-':
  if(pos == 0)
    {
    s[pos]='-';
    pos++;
    s[pos]=0;
    lir_text(x,y,s);
    }
  break;      

  case 8:
  case 127:
  case ARROW_LEFT_KEY:
  if(pos > 0)
    {
    s[pos]=' ';
    pos--;
    s[pos]='_';
    lir_text(x,y,s);
    }
  break;
  
  case '.':
  j=0;
  while(j < pos && s[j] != '.')j++;
  if(j < len)
    {
    s[pos]=' ';
    pos=j;
    s[pos]='.';
    goto show;
    }
  break;      
  
  default:
  if(lir_inkey>='0' && lir_inkey <='9')
    {
    if(pos < len)
      {
      s[pos]=(char)lir_inkey;
show:;
      pos++;
      s[pos]=0;
      t1=atof(s);
      if(t1 > max)sprintf(s,"%f",max);
      pos=0;
      while(s[pos] != 0 && s[pos] != ' ' && pos < len)pos++;
      s[pos]='_';
      j=pos+1;
      s[j]=0;
      lir_text(x,y,s);
      }
    }
  break;
  }
goto digin;
}

void clear_lines(int i, int j)
{
int k;
if(i>screen_last_line)i=screen_last_line;
if(j>screen_last_line)j=screen_last_line;
if(i > j)
  {
  k=i;
  i=j;
  j=k;
  }
lir_fillbox(0,i*text_height,screen_width-1,(j-i+1)*text_height,0);
}

void await_processed_keyboard(void)
{
await_keyboard();
process_current_lir_inkey();
}

int to_upper(int c)
{
if(c>96 && c<123)c-=32;
return c;
}

void to_upper_await_keyboard(void)
{
await_keyboard();
lir_inkey=toupper(lir_inkey);
}

int adjust_scale(double *step)
{
int pot,i;
double t1;
// Make step the nearest larger of 1, 2 or 5 in the same power of 10 
t1=step[0];
pot=0;
while(t1 > 10)
  {
  t1/=10;
  pot++;
  }
if(t1<0.00001)t1=.00001;  
while(t1 < 1)
  {
  t1*=10;
  pot--;
  }
if(t1 <= 2)
  {
  t1=2;
  i=2;
  goto asx;
  }
if(t1 <= 5)
  {
  t1=5;
  i=5;
  goto asx;
  }
t1=10;
i=1;
asx:;
while(pot > 0)
  {
  t1*=10;
  pot--;
  }
while(pot < 0)
  {
  t1/=10;
  pot++;
  }
step[0]=t1;
return i;
}

void clear_button(BUTTONS *butt, int max)
{
int i;
for(i=0; i<max; i++)
  {
  butt[i].x1=-1;
  butt[i].x2=-1;
  butt[i].y1=-1;
  butt[i].y2=-1;
  }  
}

void make_button(int x, int y, BUTTONS *butt, int m, char chr)
{
char s[2];
s[0]=chr;
s[1]=0;
butt[m].x1=x-text_width/2-2;
butt[m].x2=x+text_width/2+2;
butt[m].y1=y-text_height/2-1;
lir_pixwrite(butt[m].x1+3,butt[m].y1+2,s);
butt[m].y2=y+text_height/2;
lir_hline(butt[m].x1,butt[m].y1,butt[m].x2,7);
if(kill_all_flag) return;
lir_hline(butt[m].x1,butt[m].y2,butt[m].x2,7);
if(kill_all_flag) return;
lir_line(butt[m].x1,butt[m].y1,butt[m].x1,butt[m].y2,7);
if(kill_all_flag) return;
lir_line(butt[m].x2,butt[m].y1,butt[m].x2,butt[m].y2,7);
}  

void set_graph_minwidth(WG_PARMS *a)
{
int i;
i=a[0].xright-a[0].xleft;
if(i!=current_graph_minw)
  {
  a[0].xleft=(a[0].xleft+a[0].xright-current_graph_minw)/2;
  a[0].xright=a[0].xleft+current_graph_minw;
  }
if(a[0].xright>screen_last_xpixel)
  {
  a[0].xright=screen_last_xpixel;
  a[0].xleft=a[0].xright-current_graph_minw;
  }
if(a[0].xleft<0)
  {
  a[0].xleft=0;
  a[0].xright=a[0].xleft+current_graph_minw;
  }
i=a[0].ybottom-a[0].ytop;
if(i!=current_graph_minh)
  {
  a[0].ybottom=(a[0].ybottom+a[0].ytop+current_graph_minh)/2;
  a[0].ytop=a[0].ybottom-current_graph_minh;
  }  
if(a[0].ybottom>=screen_height)
  {
  a[0].ybottom=screen_height-1;
  a[0].ytop=a[0].ybottom-current_graph_minh;
  }  
if(a[0].ytop <0)
  {
  a[0].ytop=0;
  a[0].ybottom=a[0].ytop+current_graph_minh;
  }
}


void graph_mincheck(WG_PARMS *a)
{
if(a[0].ybottom-a[0].ytop < current_graph_minh)
  {      
  a[0].ybottom=(a[0].ybottom+a[0].ytop+current_graph_minh)>>1;
  a[0].ytop=a[0].ybottom-current_graph_minh;
  }
if(a[0].xright-a[0].xleft < current_graph_minw)
  {
  a[0].xright=(a[0].xright+a[0].xleft+current_graph_minw)>>1;
  a[0].xleft=a[0].xright-current_graph_minw;
  }
if(a[0].xleft<0)
  {
  a[0].xleft=0;
  if(a[0].xright<current_graph_minw)a[0].xright=current_graph_minw;
  }
if(a[0].ytop<0)
  {
  a[0].ytop=0;
  if(a[0].ybottom<current_graph_minh)a[0].ybottom=current_graph_minh;
  }
if(a[0].xright>screen_last_xpixel)
  {
  a[0].xright=screen_last_xpixel;
  if(a[0].xright-a[0].xleft<current_graph_minw)
                                  a[0].xleft=a[0].xright-current_graph_minw;
  }
if(a[0].ybottom>screen_height-1)
  {
  a[0].ybottom=screen_height-1;
  if(a[0].ybottom-a[0].ytop<current_graph_minh)
                                  a[0].ytop=a[0].ybottom-current_graph_minh;
  }
}

void avoid_graph_collision(WG_PARMS *a, WG_PARMS *b)
{
int ix,jx,iy,jy;
int la,ra,ta,ba,i;
// One border line of graph a has been changed.
// Check if graph a now is on top of graph b.
// If there is a collission, find out what border line to move which
// will cause minimum loss of graph area.
// This routine is also called every time a graph is placed on the
// screen when the program is started.
// During start, this simple procedure may fail, therefore
// some security is added at the end of this routine. 
if(a==b)return;
graph_mincheck(a);
ix=b[0].xright-a[0].xleft+1;
jx=b[0].xleft-a[0].xright+1;
if(ix*jx>0)goto check;
iy=b[0].ybottom-a[0].ytop+1;
jy=b[0].ytop-a[0].ybottom+1;
if(iy*jy>0)goto check;
la=ix*(a[0].ybottom-a[0].ytop);
ra=-jx*(a[0].ybottom-a[0].ytop);
ta=iy*(a[0].xright-a[0].xleft);
ba=-jy*(a[0].xright-a[0].xleft);
if(la<ra)
  {
  if(ta<ba)
    {
    if(la<ta)
      {
      i=a[0].xright-a[0].xleft;
      a[0].xleft=b[0].xright+1;
      a[0].xright=a[0].xleft+i;
      }
    else  
      {
      i=a[0].ybottom-a[0].ytop;
      a[0].ytop=b[0].ybottom+1;
      a[0].ybottom=a[0].ytop+i;
      }
    }
  else
    {
    if(la<ba)
      {
      i=a[0].xright-a[0].xleft;
      a[0].xleft=b[0].xright+1;
      a[0].xright=a[0].xleft+i;
      }
    else  
      {
      i=a[0].ybottom-a[0].ytop;
      a[0].ybottom=b[0].ytop-1;
      a[0].ytop=a[0].ybottom-i;
      }
    }  
  }
else
  {
  if(ta<ba)
    {
    if(ra<ta)
      {
      i=a[0].xright-a[0].xleft;
      a[0].xright=b[0].xleft-1;
      a[0].xright=a[0].xleft-i;
      }
    else  
      {
      i=a[0].ybottom-a[0].ytop;
      a[0].ytop=b[0].ybottom+1;
      a[0].ybottom=a[0].ytop+i;
      }
    }
  else
    {
    if(ra<ba)
      {
      i=a[0].xright-a[0].xleft;
      a[0].xright=b[0].xleft-1;
      a[0].xleft=a[0].xright-i;
      }
    else  
      {
      i=a[0].ybottom-a[0].ytop;
      a[0].ybottom=b[0].ytop-1;
      a[0].ytop=a[0].ybottom-i;
      }
    }  
  }
// Make sure we are always within the screen.
// Don't care if graphs overlay now.
check:;
graph_mincheck(a);
}  

void check_graph_placement(WG_PARMS *a)
{
graph_mincheck(a);
if(fg_flag)avoid_graph_collision(a,(void*)(&fg));
if(wg_flag)avoid_graph_collision(a,&wg);
if(hg_flag)avoid_graph_collision(a,(void*)(&hg));
if(bg_flag)avoid_graph_collision(a,(void*)(&bg));
if(ag_flag)avoid_graph_collision(a,(void*)(&ag));
if(cg_flag)avoid_graph_collision(a,(void*)(&cg));
if(pg_flag)avoid_graph_collision(a,(void*)(&pg));
if(eg_flag)avoid_graph_collision(a,(void*)(&eg));
if(mg_flag)avoid_graph_collision(a,(void*)(&mg));
if(tg_flag)avoid_graph_collision(a,(void*)(&tg));
if(rg_flag)avoid_graph_collision(a,(void*)(&rg));
if(sg_flag)avoid_graph_collision(a,(void*)(&sg));
if(vg_flag)avoid_graph_collision(a,(void*)(&vg));
if(xg_flag)avoid_graph_collision(a,(void*)(&xg));
}

void graph_borders(WG_PARMS *a,unsigned char color)
{
lir_hline(a[0].xleft,a[0].ytop,a[0].xright,color);
if(kill_all_flag) return;
lir_hline(a[0].xleft,a[0].ybottom,a[0].xright,color);
if(kill_all_flag) return;
lir_line(a[0].xleft,a[0].ytop,a[0].xleft,a[0].ybottom,color);
if(kill_all_flag) return;
lir_line(a[0].xright,a[0].ytop,a[0].xright,a[0].ybottom,color);
}

void dual_graph_borders(WG_PARMS *a, unsigned char color)
{
graph_borders(a,color);
if(kill_all_flag) return;
lir_hline(a[0].xleft, a[0].yborder, a[0].xright, color);
}

void compute_converter_parameters(void)
{
if( (ui.converter_mode & CONVERTER_LO_BELOW) == 0)
  { 
  ui_converter_direction=-1; 
  }
else
  { 
  ui_converter_direction=1; 
  }
converter_offset_mhz=ui.converter_mhz+0.000001*ui.converter_hz;
}

void screenerr(int *line)
{        
char s[80];
settextcolor(12);
sprintf(s,"Screen height too small. Take notes, then %s",press_any_key);
if(line[0] > screen_last_line)line[0]=screen_last_line;
lir_text(0, line[0], s);
settextcolor(7);
await_processed_keyboard();
line[0]=0;
clear_screen();
}


