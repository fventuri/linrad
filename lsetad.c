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

#include "globdef.h"
#include "uidef.h"
#include "sigdef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "fft3def.h"
#include "screendef.h"
#include "sdrdef.h"
#include "vernr.h"
#include "thrdef.h"
#include "hwaredef.h"
#include "xdef.h"
#include "txdef.h"
#include "options.h"
#include "keyboard_def.h"
#include "padef.h"
#include "lscreen.h"


#define ABOVE_MAX_SPEED 768000
#define MAX_COLUMN 80

#define DEVFLAG_R 1
#define DEVFLAG_W 2
#define DEVFLAG_RW 4
#if HAVE_OSS == 1
char dev_name[sizeof(SND_DEV)+2];
#endif
int devmodes[3]={O_RDONLY,O_WRONLY,O_RDWR};
char *devmode_txt[3]={"RDONLY","WRONLY","RDWR"};
#define DEVMODE_RDONLY 0
#define DEVMODE_WRONLY 1
#define DEVMODE_RDWR 2

double round(double x);
void thread_test_rxad(void);
void thread_test_rxda(void);
void thread_test_txda(void);

pthread_t thread_identifier_soundcard_test[MAX_IOTEST];
ROUTINE thread_iotest[MAX_IOTEST]={thread_test_rxad,     //0
                                   thread_test_rxda,     //1
                                   thread_test_txda};    //2

int dev_flag[MAX_DEVNAMES];
int dev_rd_channels[MAX_DEVNAMES];
int dev_max_rd_speed[MAX_DEVNAMES];
int dev_min_rd_speed[MAX_DEVNAMES];
int dev_rd_bytes[MAX_DEVNAMES];
int dev_wr_channels[MAX_DEVNAMES];
int dev_max_wr_speed[MAX_DEVNAMES];
int dev_min_wr_speed[MAX_DEVNAMES];
int dev_wr_bytes[MAX_DEVNAMES];
char blanks[60];

char *wish_anyway="Do you wish to try to use it anyway (Y/N)?";
char *intrate_string=" interrupt rate: ";
int low_speeds[MAX_LOWSPEED]={1,5000,6000,8000,11025,16000,
                              22050,24000,48000,96000};

int zrw; // untested return value for OSS
void fix_prio(int no);

#include "loadalsa.h"

#define MAX_ALSADEV 24

int  alsa_dev_seq_nmbr;
char alsa_dev_soundcard_name [256];
char alsa_dev_name [64];
char alsa_dev_hw_pcm_name[MAX_ALSADEV];
char alsa_dev_plughw_pcm_name[MAX_ALSADEV];

unsigned int alsa_dev_min_rate[MAX_ALSADEV];
unsigned int alsa_dev_max_rate[MAX_ALSADEV];
int  alsa_dev_min_channels;
int  alsa_dev_max_channels;
int  alsa_dev_min_bytes;
int  alsa_dev_max_bytes;

void sndlog_sync(void)
{
fflush(sndlog);
//sync();
}

void edit_soundcard_name(char *s)
{
int i,j,k,l;
// We are called with a string with a high number of blanks in the text.
// minimise the blanks and limit total text string length to max 51
i=(int)strlen(s);
j=0;
again:;
while(s[j] != ' ')j++;
if (j >= i) goto finish;
k=j;
while(s[k] == ' ')k++;
if (k >= i) goto finish;
l=k;
while(s[l] != ' ' && l < i)
  {
  j=j+1;
  s[j]=s[l];
  if (j != l)s[l]=' ';
  l++;
  }
if (l<i) goto again;
finish:;
j++;
if(j>51)j=51;
s[j]='\0';
}

int alsa_get_dev_names(int n)
{
//This routine returns  alsa_dev_hw_pcm_name
//               	alsa_dev_plughw_pcm_name
//			alsa_dev_soundcard_name
//			alsa_dev_name
//                      alsa_dev_seq_nmbr
//for a valid sequence number n on input.
//This routine is not "thread safe"
//n  must be positive and less than  MAX_ALSADEV

//Returncode is negative if n invalid,
//                       if alsa errors
//                       if no device found for input sequence number.
snd_ctl_t *handle;
snd_ctl_card_info_t *info;
snd_pcm_info_t *pcminfo;

int err, idx, dev, dev_seq_nmbr;
char str[128];
int retcode;

alsa_dev_seq_nmbr=0;
sprintf(alsa_dev_hw_pcm_name ," ");
sprintf(alsa_dev_plughw_pcm_name," ");
sprintf(alsa_dev_soundcard_name," ");
sprintf(alsa_dev_name," ");
idx=-1;
dev_seq_nmbr=0;
retcode=-10;
if(n < 0 || n >= MAX_ALSADEV)
  {
  retcode=-6;
  goto exit_3;
  }
snd_ctl_card_info_malloc(&info);
snd_pcm_info_malloc(&pcminfo);
while (1)  // loop trough all cards
  {
  err=snd_card_next(&idx);
  if(err < 0)
    {
    SNDLOG"alsa_get_dev_names:snd_card_next error: %s\n", snd_strerror(err));
    retcode=-5;
    goto exit_2;
    }
  if(idx < 0)  // no more cards available
    {
    SNDLOG"alsa_get_dev_pcm_name:end of available card/devices reached\n");
    retcode=-1;
    goto exit_2;
    }
  sprintf(str, "hw:CARD=%i", idx);
  SNDLOG"Testing %s\n",str);
  err=snd_ctl_open( &handle, str, 0);
  if(err < 0)
    {
    SNDLOG"alsa_get_dev_names:snd_ctl_open error: %s\n", snd_strerror(err));
    retcode=-4;
    goto exit_2;
    }
  err=snd_ctl_card_info(handle, info);
  if(err < 0)
    {
    SNDLOG"alsa_get_dev_names:HW info error: %s\n", snd_strerror(err));
    retcode=-3;
    goto exit_1;
    }
  dev=-1;
  while (1)  // loop trough all devices for this card
    {
    err=snd_ctl_pcm_next_device(handle, &dev);
    if(err < 0)
      {
      SNDLOG"alsa_get_dev_names:PCM next device error: %s\n", snd_strerror(err));
      retcode=-2;
      goto exit_1;
      }
    if(dev < 0)      // no more devices available for this card, get next card
        {
        snd_ctl_close(handle);
        break;
        }
// check first if device has  valid  pcminfo,
//if not, go to next device without incrementing dev seq nmbr
      snd_pcm_info_set_device(pcminfo, (unsigned int)dev);
      snd_pcm_info_set_subdevice(pcminfo, 0);
      snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_CAPTURE);
      if((err=snd_ctl_pcm_info(handle, pcminfo))<0)
	{
        snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_PLAYBACK);
        if((err=snd_ctl_pcm_info(handle, pcminfo))<0) goto next_device;
	}
//if valid, check if match with n
    if(dev_seq_nmbr == n)
      {
// if match with n, get all device name-info and exit loop
      alsa_dev_seq_nmbr=dev_seq_nmbr;
      sprintf(alsa_dev_hw_pcm_name ,"hw:%i,%i", idx,dev);
      sprintf(alsa_dev_plughw_pcm_name,"plughw:%i,%i", idx,dev);
      snd_pcm_info_set_device(pcminfo, (unsigned int)dev);
      snd_pcm_info_set_subdevice(pcminfo, 0);
      snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_CAPTURE);
      err=snd_ctl_pcm_info(handle, pcminfo);
      if(err==0)
        {
        sprintf(alsa_dev_name," %s, %s",snd_ctl_card_info_get_name(info),snd_pcm_info_get_id(pcminfo));
        }
      else
        {
        snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_PLAYBACK);
        err=snd_ctl_pcm_info(handle, pcminfo);
        if(err==0)
          {
          sprintf(alsa_dev_name," %s, %s",snd_ctl_card_info_get_name(info),snd_pcm_info_get_id(pcminfo));
          }
        else
          {
          SNDLOG"alsa_get_dev_names:snd_ctl_pcm_info error: %s\n", snd_strerror(err));
          retcode=-7;
          goto exit_1;
          }
        }
      sprintf(alsa_dev_soundcard_name," %s",snd_ctl_card_info_get_longname(info));
      edit_soundcard_name(alsa_dev_soundcard_name);
      SNDLOG"device found: alsa_dev_plughw_pcm_name=%s \n", alsa_dev_plughw_pcm_name);
      retcode=0;
      goto exit_1;
      }
// if no match with n, increment  seq nbr of valid devices
    dev_seq_nmbr= dev_seq_nmbr+1;
//get next device
    next_device:;
    }  // end while loop through devices
  }    // end while loop through cards
exit_1:;
snd_ctl_close(handle);
exit_2:;
snd_ctl_card_info_free(info);
snd_pcm_info_free (pcminfo);
exit_3:;
SNDLOG"(alsa_get_dev_pcm_name: n=%i, retcode =%d) \n\n",n,retcode);
return retcode;
}


int  alsar_get_dev_pcm_names (int n,
                              void *al_dev_plughw_pcm_name,
                              void *al_dev_hw_pcm_name  )
{
//Threadsafe version of alsa_get_dev_names
//with limited functionality:
//This  routine returns only dev_plughw_pcm_name and dev_hw_pcm_name
//for a valid sequence number n on input.
//n  must be positive and less than  MAX_ALSADEV
//Returncode is negative if n invalid,
//                       if alsa errors
//                       if no device found for input sequence number.
snd_ctl_t *handle;
snd_ctl_card_info_t *info;
snd_pcm_info_t *pcminfo;
int err, idx, dev, dev_seq_nmbr;
char str[128];
int retcode ;
retcode=-10;
alsa_dev_seq_nmbr=0;
sprintf((char *)(al_dev_plughw_pcm_name)," ");
sprintf((char *)(al_dev_hw_pcm_name)," ");
if(n < 0 || n >= MAX_ALSADEV)
  {
  retcode=-6;
  goto exit_3;
  }
idx=-1;
dev_seq_nmbr=0;
snd_ctl_card_info_malloc(&info);
snd_pcm_info_malloc(&pcminfo);
while (1)      // loop through cards
  {
  err=snd_card_next(&idx);
  if(err < 0)
    {
    SNDLOG"alsar_get_dev_pcm_name:snd_card_next error: %s\n", snd_strerror(err));
    retcode=-5;
    goto exit_2;
    }
  if(idx < 0)  // no more cards available
    {
    SNDLOG"alsar_get_dev_pcm_name:end of available card/devices reached\n");
    retcode=-1;
    goto exit_2;
    }
  sprintf(str, "hw:CARD=%i", idx);
  SNDLOG"Testing %s\n",str);
  err=snd_ctl_open( &handle, str, 0);
  if(err < 0)
    {
    SNDLOG"alsar_get_dev_pcm_name:snd_ctl_open error: %s\n", snd_strerror(err));
    retcode=-4;
    goto exit_2;
    }
  err=snd_ctl_card_info(handle, info);
  if(err < 0)
    {
    SNDLOG"alsar_get_dev_pcm_name:HW info error: %s\n", snd_strerror(err));
    retcode=-3;
    goto exit_1;
    }
  dev=-1;
  while (1) // loop through all devices for this card
    {
    err=snd_ctl_pcm_next_device(handle, &dev);
    if(err < 0)
      {
      SNDLOG"alsar_get_dev_pcm_name:PCM next device error: %s\n", snd_strerror(err));
      retcode=-2;
      goto exit_1;
      }
    if(dev < 0)   // no more devices available for this card
      {
      snd_ctl_close(handle);
      break;
      }
// check first if device has  valid  pcminfo,
//if not, go to next device without incrementing dev seq nmbr
    snd_pcm_info_set_device(pcminfo, (unsigned int)dev);
    snd_pcm_info_set_subdevice(pcminfo, 0);
    snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_CAPTURE);
    err=snd_ctl_pcm_info(handle, pcminfo);
    if(err<0)
      {
      snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_PLAYBACK);
      err=snd_ctl_pcm_info(handle, pcminfo);
      if(err<0) goto next_device;
      }

//if valid, check if match with n
    if(dev_seq_nmbr == n)
      {
// if match with n, get  limited device name-info and exit loop
      alsa_dev_seq_nmbr=dev_seq_nmbr;
      sprintf((char*)(al_dev_hw_pcm_name) ,"hw:%i,%i", idx,dev);
      sprintf((char *)(al_dev_plughw_pcm_name),"plughw:%i,%i", idx,dev);
      SNDLOG"device found: dev_plughw_pcm_name=%s\n",((char *)(al_dev_plughw_pcm_name)));
      retcode=0;
      goto exit_1;
      }
// if no match with n, increment  seq nmbr of valid devices
    dev_seq_nmbr= dev_seq_nmbr+1;
// get next_device
    next_device:;
    }  // end while loop through devices
  }    // end while loop through cards
exit_1:;
snd_ctl_close(handle);
exit_2:;
snd_ctl_card_info_free(info);
snd_pcm_info_free (pcminfo);
exit_3:;
SNDLOG"(alsar_get_dev_pcm_name: n=%i, retcode =%d) \n\n",n,retcode);
return retcode;
}



int  alsa_get_dev_native_capabilities(int n,int mode)
{
// This single threaded routine returns the native hw capabilities of a given
// soundcard/device n, in either capture or playback mode (mode input parameter).
// This is done by opening the device with a pcm_name=hw:n,0  and
// querying hw_params
// Returncode is negative if
//                         device does not exist
//                         alsa error

snd_pcm_t *wk_handle;
snd_pcm_hw_params_t *hw_params;
unsigned int val;
int err, dir;

dir=0;
alsa_dev_min_rate[n]=0;
alsa_dev_max_rate[n]=0;
alsa_dev_min_channels=0;
alsa_dev_max_channels=0;
alsa_dev_min_bytes=0;
alsa_dev_max_bytes=0;

if((err=alsa_get_dev_names (n)) < 0 ) {
SNDLOG"alsa_get_dev_native_capabilities: alsa_get_dev_id failed\n" );
return -1;
}
if((err=snd_pcm_open(&wk_handle, alsa_dev_hw_pcm_name, mode, 0)) < 0) {
SNDLOG"alsa_get_dev_native_capabilities: open failed,%s\n",alsa_dev_hw_pcm_name );
return -2;
}
if((err=snd_pcm_hw_params_malloc(&hw_params)) < 0) {
SNDLOG"alsa_get_dev_native_capabilities: snd_pcm_hw_params_malloc failed\n" );
snd_pcm_close(wk_handle);
return -3;
}
if((err=snd_pcm_hw_params_any(wk_handle, hw_params)) < 0) {
SNDLOG"alsa_get_dev_native_capabilities: snd_pcm_hw_params_any failed\n" );
snd_pcm_hw_params_free (hw_params);
snd_pcm_close(wk_handle);
return -4;
}
// get sampling rate
if((err= snd_pcm_hw_params_get_rate_max (hw_params, &val, &dir)) < 0){
SNDLOG"alsa_get_dev_native_capabilities: snd_pcm_hw_params_get_rate_max failed\n" );
snd_pcm_hw_params_free (hw_params);
snd_pcm_close(wk_handle);
return -5;
}
alsa_dev_max_rate[n]=(int)val;

if((err= snd_pcm_hw_params_get_rate_min (hw_params, &val, &dir)) < 0) {
SNDLOG"alsa_get_dev_native_capabilities: snd_pcm_hw_params_get_rate_min failed\n" );
snd_pcm_hw_params_free (hw_params);
snd_pcm_close(wk_handle);
return -6;
}
alsa_dev_min_rate[n]=(int)val;

// get # channels
if((err= snd_pcm_hw_params_get_channels_max(hw_params, &val)) < 0) {
SNDLOG"alsa_get_dev_native_capabilities: snd_pcm_hw_params_get_channels_max failed\n" );
snd_pcm_hw_params_free (hw_params);
snd_pcm_close(wk_handle);
return -7;
}
alsa_dev_max_channels=(int)val;

if((err= snd_pcm_hw_params_get_channels_min(hw_params, &val)) < 0){
SNDLOG"alsa_get_dev_native_capabilities: snd_pcm_hw_params_get_channels_min failed\n" );
snd_pcm_hw_params_free (hw_params);
snd_pcm_close(wk_handle);
return -8;
}
alsa_dev_min_channels=(int)val;

// get  min # of bytes
if((err=snd_pcm_hw_params_test_format (wk_handle, hw_params,SND_PCM_FORMAT_U8)) == 0)
  {
  alsa_dev_min_bytes=1;
  }
else
  {
  if((err=snd_pcm_hw_params_test_format (wk_handle, hw_params,SND_PCM_FORMAT_S16_LE)) == 0)
    {
    alsa_dev_min_bytes=2;
    }
  else
    {
    if((err=snd_pcm_hw_params_test_format (wk_handle, hw_params,SND_PCM_FORMAT_S32_LE)) == 0)
      {
      alsa_dev_min_bytes=4;
      }
    else {
         SNDLOG"alsa_get_dev_native_capabilities: snd_pcm_hw_params_test_format failed\n" );
         //probably an alsa error, assume 2 bytes
         alsa_dev_min_bytes=2;
         }
    }
  }
// get  max # of bytes
if((err=snd_pcm_hw_params_test_format (wk_handle, hw_params,SND_PCM_FORMAT_S32_LE)) == 0)
  {
  alsa_dev_max_bytes=4;
  }
else
  {
  if((err=snd_pcm_hw_params_test_format (wk_handle, hw_params,SND_PCM_FORMAT_S16_LE)) == 0)
    {
    alsa_dev_max_bytes=2;
    }
  else
    {
    if((err=snd_pcm_hw_params_test_format (wk_handle, hw_params,SND_PCM_FORMAT_U8)) == 0)
      {
      alsa_dev_max_bytes=1;
      }
    else{
        SNDLOG"alsa_get_dev_native_capabilities: snd_pcm_hw_params_test_format failed\n" );
        //probably an alsa error, assume 2 bytes
        alsa_dev_max_bytes=2;
        }
    }
  }
snd_pcm_hw_params_free (hw_params);
snd_pcm_close(wk_handle);
return 0;
}

int  alsar_get_dev_native_capabilities(int n,
                                 int mode,
                                 int *al_dev_min_rate,
                                 int *al_dev_max_rate,
				 int *al_dev_min_channels,
				 int *al_dev_max_channels,
                                 int *al_dev_min_bytes,
                                 int *al_dev_max_bytes
					)
{
// threadsafe version of alsa_get_dev_native_capabilities
// This routine returns the native hw capabilities of a given
// soundcard/device n, in capture or playback mode (mode input parameter).
// This is done by opening the device with a pcm_name=hw:n,0  and
// querying hw_params
// Returncode is negative if
//                         device does not exist
//                         alsa error

snd_pcm_t *wk_handle;
snd_pcm_hw_params_t *hw_params;
unsigned int val;
int err, dir;
char dev_plughw_pcm_name[MAX_ALSADEV];
char dev_hw_pcm_name[MAX_ALSADEV];

dir=0;

*al_dev_min_rate=0;
*al_dev_max_rate=0;
*al_dev_min_channels=0;
*al_dev_max_channels=0;
*al_dev_min_bytes=0;
*al_dev_max_bytes=0;

//check if valid device
if (( alsar_get_dev_pcm_names(n,dev_plughw_pcm_name,dev_hw_pcm_name))< 0 )
{
SNDLOG"alsar_get_dev_native_capabilities: alsar_get_dev_plughw_pcm_name failed\n" );
return -1;
}
if((err=snd_pcm_open(&wk_handle, dev_hw_pcm_name, mode, 0)) < 0) {
SNDLOG"alsar_get_dev_native_capabilities: open failed,%s\n",dev_hw_pcm_name );
return -2;
}
if((err=snd_pcm_hw_params_malloc (&hw_params)) < 0) {
SNDLOG"alsar_get_dev_native_capabilities: snd_pcm_hw_params_malloc failed\n" );
snd_pcm_close(wk_handle);
return -3;
}
if((err=snd_pcm_hw_params_any(wk_handle, hw_params)) < 0) {
SNDLOG"alsar_get_dev_native_capabilities: snd_pcm_hw_params_any failed\n" );
snd_pcm_hw_params_free (hw_params);
snd_pcm_close(wk_handle);
return -4;
}
// get max sampling rate
if((err= snd_pcm_hw_params_get_rate_max (hw_params, &val, &dir)) < 0){
SNDLOG"alsar_get_dev_native_capabilities: snd_pcm_hw_params_get_rate_max failed\n" );
snd_pcm_hw_params_free (hw_params);
snd_pcm_close(wk_handle);
return -5;
}
*al_dev_max_rate=(int)val;

// get min sampling rate
if((err= snd_pcm_hw_params_get_rate_min (hw_params, &val, &dir)) < 0) {
SNDLOG"alsar_get_dev_native_capabilities: snd_pcm_hw_params_get_rate_min failed\n" );
snd_pcm_hw_params_free (hw_params);
snd_pcm_close(wk_handle);
return -6;
}
*al_dev_min_rate=(int)val;

// get max # channels
if((err= snd_pcm_hw_params_get_channels_max(hw_params, &val)) < 0) {
SNDLOG"alsar_get_dev_native_capabilities: snd_pcm_hw_params_get_channels_max failed\n" );
snd_pcm_hw_params_free (hw_params);
snd_pcm_close(wk_handle);
return -4;
}
*al_dev_max_channels=(int)val;

// get min # channels
if((err= snd_pcm_hw_params_get_channels_min(hw_params, &val)) < 0){
SNDLOG"alsar_get_dev_native_capabilities: snd_pcm_hw_params_get_channels_min failed\n" );
snd_pcm_hw_params_free (hw_params);
snd_pcm_close(wk_handle);
return -8;
}
*al_dev_min_channels=(int)val;

// get  min # of bytes
if((err=snd_pcm_hw_params_test_format (wk_handle, hw_params,SND_PCM_FORMAT_U8)) == 0)
  {
  *al_dev_min_bytes=1;
  }
else
  {
  if((err=snd_pcm_hw_params_test_format (wk_handle, hw_params,SND_PCM_FORMAT_S16_LE)) == 0)
    {
    *al_dev_min_bytes=2;
    }
  else
    {
    if((err=snd_pcm_hw_params_test_format (wk_handle, hw_params,SND_PCM_FORMAT_S32_LE)) == 0)
      {
      *al_dev_min_bytes=4;
      }
    else {
         SNDLOG"alsar_get_dev_native_capabilities: snd_pcm_hw_params_test_format failed\n" );
         //probably an alsa error, assume 2 bytes
         *al_dev_min_bytes=2;
         }
    }
  }
// get  max # of bytes
if((err=snd_pcm_hw_params_test_format (wk_handle, hw_params,SND_PCM_FORMAT_S32_LE)) == 0)
  {
  *al_dev_max_bytes=4;
  }
else
  {
  if((err=snd_pcm_hw_params_test_format (wk_handle, hw_params,SND_PCM_FORMAT_S16_LE)) == 0)
    {
    *al_dev_max_bytes=2;
    }
  else
    {
    if((err=snd_pcm_hw_params_test_format (wk_handle, hw_params,SND_PCM_FORMAT_U8)) == 0)
      {
      *al_dev_max_bytes=1;
      }
    else{
        SNDLOG"alsar_get_dev_native_capabilities: snd_pcm_hw_params_test_format failed\n" );
        //probably an alsa error, assume 2 bytes
        *al_dev_max_bytes=2;
        }
    }
  }
snd_pcm_hw_params_free (hw_params);
snd_pcm_close(wk_handle);
return 0;
}

int alsa_get_native_samplerate (int n,
                                int mode,
                                int *line,
                                unsigned int *returned_sampling_rate)
{
(void) line;
(void) returned_sampling_rate;
int err,i,k,id;
unsigned int val;
char s[80];
char dev_hw_pcm_name[MAX_ALSADEV];
char dev_plughw_pcm_name[MAX_ALSADEV];
unsigned int  native_sampling_rate[MAX_ALSADEV];
snd_pcm_t *wk_handle;
snd_pcm_hw_params_t *hw_params;
// 0 terminated  list
static unsigned int standardSampleRates[] = {8000, 9600, 
        11025, 12000, 16000, 22050, 24000, 32000,
        44100, 48000, 50000,88200, 96000, 119466, 192000,
        224000, 448000, 0};
unsigned int sample_rates[32];
unsigned int smin, smax;
err=alsar_get_dev_pcm_names(n,dev_plughw_pcm_name,dev_hw_pcm_name);
if (err!=0) return -1; 
err=snd_pcm_hw_params_malloc (&hw_params);
if (err!=0) return -2; 
err=snd_pcm_open(&wk_handle, dev_hw_pcm_name, mode, 0);
if (err!=0)
  { 
  snd_pcm_hw_params_free (hw_params);
  return -3;
  } 
clear_screen();
line[0]=2;
sprintf(s,"This is a native  ALSA device.");
lir_text(1,line[0], s);
line[0]+=1;
sprintf(s,"Select sampling rate by line number from list:");
lir_text(1,line[0], s);
line[0]+=2;
i=0;
smin=alsa_dev_min_rate[n];
smax=alsa_dev_max_rate[n];
if(smin == smax)
  {
  smin=0;
  }   
while(standardSampleRates[i] != 0 )
  {
  if(smax == standardSampleRates[i])smax=0;
  if(smin == standardSampleRates[i])smin=0;
  sample_rates[i]=standardSampleRates[i];
  i++;
  }
if(smin != 0)
  {
  sample_rates[i]=smin;
  i++;
  }
if(smax > 0)
  {
  sample_rates[i]=smax;
  i++;
  }
sample_rates[i]=0;
i=0;
k=0;
while(sample_rates[i] != 0 )
   {
   val=(unsigned int)sample_rates[i];
   err=snd_pcm_hw_params_any(wk_handle, hw_params);
   if (err!=0)
     {
     snd_pcm_close(wk_handle); 
     snd_pcm_hw_params_free (hw_params);
     return -4;
     }
   err=snd_pcm_hw_params_set_rate(wk_handle,hw_params,val, 0);
   if(err == 0)
     {
     sprintf(s,"%2d -> %6d ",k,val);
     lir_text(1,line[0],s);
     native_sampling_rate[k]= val;
     line[0]++;
     k++;
     }
    i++;
   }
line[0]++;
sprintf(s,"Enter line number >");
lir_text(1,line[0],s);
id=lir_get_integer(21,line[0],2, 0, k-1);
*returned_sampling_rate=native_sampling_rate[id];
snd_pcm_close(wk_handle); 
snd_pcm_hw_params_free (hw_params);
return 0;
}

int select_alsadev(int open_mode, int *line)
{
//display available devices for a specific mode  and return sequence number of selected device
char s[512];
int i, err;
line[0]++;
sprintf(s,"device soundcard-device-name                                device      sample-rate  channels  bytes");
SNDLOG"%s\n",s);
lir_text(0,line[0],s);
line[0]++;
sprintf(s,"seq-no              (soundcard-name)                       pcm-name      min / max   min/max  min/max");
SNDLOG"%s\n",s);
lir_text(0,line[0],s);
line[0]++;
sprintf(s,"------ --------------------------------------------------  ----------   -----------  -------  ------- " );
SNDLOG"%s\n",s);
lir_text(0,line[0],s);
line[0]++;
for(i=0; i<MAX_ALSADEV; i++)
  {
  sndlog_sync();
  err=alsa_get_dev_names(i);
  if(err == -1) goto end_alsa_get_dev_names;
  if(err == 0)
    {
    err=alsa_get_dev_native_capabilities(i, open_mode);
    if(err == 0)
      {
      line[0]++;
      if(line[0] >= screen_last_line-1)
        {
        line[0]=screen_last_line-1;
        screenerr(line);
        }
      sprintf(s,"%i", alsa_dev_seq_nmbr);
      lir_text(2,line[0],s);
      sprintf(s,"%s", alsa_dev_name);
      lir_text(6,line[0],s);
      sprintf(s,"(%s)", alsa_dev_soundcard_name);
      lir_text(20,line[0]+1,s);
      sprintf(s,"%s", alsa_dev_plughw_pcm_name);
      lir_text(59,line[0],s);
      sprintf(s,"%i/%i", alsa_dev_min_rate[i], alsa_dev_max_rate[i]);
      lir_text(72,line[0],s);
      sprintf(s,"%i/%i ", alsa_dev_min_channels, alsa_dev_max_channels);
      lir_text(86,line[0],s);
      sprintf(s,"%i/%i ", alsa_dev_min_bytes, alsa_dev_max_bytes);
      lir_text(96,line[0],s);
      sprintf(s,"     %i   %s (%s)  %s   %i/%i   %i/%i   %i/%i ",
             alsa_dev_seq_nmbr, alsa_dev_name, alsa_dev_soundcard_name,
                         alsa_dev_plughw_pcm_name, alsa_dev_min_rate[i], 
                           alsa_dev_max_rate[i],alsa_dev_min_channels, 
                            alsa_dev_max_channels, alsa_dev_min_bytes,
                                                   alsa_dev_max_bytes );
      line[0]+=2;
      if(line[0] >= screen_last_line-1)
        {
        line[0]=screen_last_line-1;
        screenerr(line);
        }
      SNDLOG"%s\n",s);
      }
    }
  }
end_alsa_get_dev_names:;
again:;
sprintf(s,"Enter desired device-seq-nmbr >" );
SNDLOG"%s\n",s);
line[0]+=2;
if(line[0] >= screen_last_line-1)
  {
  line[0]=screen_last_line-1;
  screenerr(line);
  }
lir_text(0,line[0],s);
i=lir_get_integer(33, line[0], 2, 0,MAX_ALSADEV-1);
if(kill_all_flag)return -1;
sprintf(s,"User selected  %i",i);
SNDLOG"%s\n",s);
err=alsa_get_dev_names (i);
if(err < 0)
  {
  lir_text(37,line[0],"invalid device-seq-nmbr, try again");
  sprintf(s,"invalid device-seq-nmbr, try again");
  SNDLOG"%s\n",s);
  goto again;
  }
err=alsa_get_dev_native_capabilities(i,open_mode);
if(err != 0)
  {
  lir_text(37,line[0],"incompatible capabilities, try again");
  sprintf(s,"incompatible capabilities, try again");
  SNDLOG"%s\n",s);
  goto again;
  }
lir_text(37,line[0],blanks);
line[0]=0;
clear_screen();
return i;
}

int chk_open_alsa(int no, int bytes, int speed, int channels, int mode)
{
int err;
char dev_plughw_pcm_name[MAX_ALSADEV];
char dev_hw_pcm_name[MAX_ALSADEV];

int al_dev_min_rate;
int al_dev_max_rate;
int al_dev_min_channels;
int al_dev_max_channels;
int al_dev_min_bytes;
int al_dev_max_bytes;
//check if device exists
err= alsar_get_dev_pcm_names(no,dev_plughw_pcm_name,dev_hw_pcm_name);
if( err < 0 )return -1;
//check capabilities of device
err=alsar_get_dev_native_capabilities( no,
                                 mode,
                                 &al_dev_min_rate,
                                 &al_dev_max_rate,
				 &al_dev_min_channels,
				 &al_dev_max_channels,
                                 &al_dev_min_bytes,
                                 &al_dev_max_bytes);
if( err != 0 )return -2;
if( speed < al_dev_min_rate )return -4;
if( speed > al_dev_max_rate )return -5;
if( channels > al_dev_max_channels )return -7;
if( bytes < al_dev_min_bytes )return -8;
if( bytes > al_dev_max_bytes )return -9;
return 0;
}

void alsa_errors(int err)
{
switch (err)
  {
  case EIO:
  lirerr(1169);
  return;

  case EPIPE:
  lirerr(1108);
  return;
  
  default:
  lirerr(1127);
  return;
  }
}

int alsaread(snd_pcm_t *handle, char *buf, int type)
{
snd_pcm_uframes_t frames,frames_ok;
int i, k, err, frame_time;
frames=(snd_pcm_uframes_t)snd[type].block_frames;
frame_time=1100000/snd[type].interrupt_rate;
if(frame_time < 1000)frame_time=1000;
k=0;
xx:;
err=snd_pcm_readi(handle, buf, frames);
if(kill_all_flag)return FALSE;
if((unsigned int)err != frames)
  {
  if(err == -EAGAIN)
    {
// Nothing available yet.
    lir_sleep(frame_time);
    if(kill_all_flag)return FALSE;
    goto xx;
    }    
  if(err < 0)
    {
xx1:;    
    k++;
    err= snd_pcm_recover(handle, err, 0);
    if(err == 0)goto xx;
    lir_sleep(2000);
    if(k < 3)goto xx1;
    alsa_errors(-err);
    return FALSE;      
    }
// We had a short read. Wait for the device to recover.
// Then read remaining bytes.
  frames_ok=(snd_pcm_uframes_t)err;
  k=0;
read_more:;
  if(kill_all_flag)return FALSE;
  i=(frame_time*(frames-frames_ok))/frames;
  if(i<frame_time/2)i=frame_time/2;
  lir_sleep(i); 
  err=snd_pcm_readi(handle, &buf[frames_ok*snd[type].framesize], frames-frames_ok);
  if(err == -EAGAIN)
    {
// Nothing available yet.
    goto read_more;
    }
  if(err < 0)
    {
read_more1:;
    if(kill_all_flag)return FALSE;    
    err=snd_pcm_recover(handle, err, 0);
    if(err == 0)goto read_more;
    lir_sleep(2000);
    if(k < 3)goto read_more1;
    alsa_errors(-err);
    return FALSE;      
    }
  if((snd_pcm_uframes_t)err != frames-frames_ok)
    {
    frames_ok+=(snd_pcm_uframes_t)err;
    goto read_more;
    }
  }
return TRUE;
}


void lir_tx_adread(char *buf)
{
int err;
char s[40];
int nread;
if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
  {
  err=alsaread(alsa_handle[TXAD], buf, TXAD);
  if(err == FALSE)
    {
    no_of_tx_overrun_errors++;
    sprintf(s,"TX%s%d",overrun_error_msg,no_of_tx_overrun_errors);
    wg_error(s,WGERR_TXIN);
    }
  }
else
  {
  nread=read(tx_audio_in, buf, (size_t)snd[TXAD].block_bytes);
  if(nread != snd[TXAD].block_bytes)lirerr(1283);
  }
}

void thread_rx_adinput(void)
{
double read_start_time,total_reads;
short int *rxin_isho2;
int *rxin_int1, *rxin_int2;
int rxin_local_workload_reset;
int err;
char s[40];
int i;
int timing_loop_counter,timing_loop_counter_max,initial_skip_flag;
#if OSNUM == OSNUM_LINUX
fix_prio(THREAD_RX_ADINPUT);
clear_thread_times(THREAD_RX_ADINPUT);
#endif
// Open the hware_command thread in case we are using the WSE units.
if( allow_wse_parport != 0)
  {
  linrad_thread_create(THREAD_HWARE_COMMAND);
  }
if(screen_loop_counter_max==0)screen_loop_counter_max=1;
if(thread_command_flag[THREAD_SCREEN] != THRFLAG_NOT_ACTIVE)
 {
  while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
    {
    if(thread_command_flag[THREAD_RX_ADINPUT] ==
                                           THRFLAG_KILL)goto rx_adin_init_error;
    lir_sleep(10000);
    }
  }
sys_func(THRFLAG_OPEN_RX_SNDIN);
if(kill_all_flag) goto rx_adin_init_error;
if(rxad_status != LIR_OK)goto rx_adin_init_error;
update_extio_rx_freq();
update_extio_rx_gain();
snd[RXAD].open_flag=CALLBACK_CMD_ACTIVE;
#include "timing_setup.c"
lir_status=LIR_OK;
thread_status_flag[THREAD_RX_ADINPUT]=THRFLAG_ACTIVE;
while(!kill_all_flag && 
           thread_command_flag[THREAD_RX_ADINPUT] == THRFLAG_ACTIVE)
  {
#include "input_speed.c"
  if ( (ui.use_alsa&PORTAUDIO_RX_IN) != 0)
    {
    rxin_isho=(short int*)(&timf1_char[timf1p_pa]);
    while (!kill_all_flag && timf1p_sdr == timf1p_pa)
      {
      lir_await_event(EVENT_PORTAUDIO_RXAD_READY);
      }
    }
  else
    {
    rxin_isho=(short int*)(&timf1_char[timf1p_pa]);
    if(ui.rx_addev_no < 256)
      {
      if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
        {
        err=alsaread(alsa_handle[RXAD], (char*)rxin_isho, RXAD);
        if(kill_all_flag) goto rxadin_error_exit;
        if(err == FALSE)
          {
          no_of_rx_overrun_errors++;
          sprintf(s,"RX%s%d",overrun_error_msg,no_of_rx_overrun_errors);
          wg_error(s,WGERR_RXIN);
          }
        }
      else
        {
        zrw=read(rx_audio_in, rxin_isho, (size_t)snd[RXAD].block_bytes);
        }
      }
    else
      {
      rxin_isho2=(short int*)(&timf1_char[timf1p_pa]);
      zrw=read(rx_audio_in, rxin_isho, (size_t)(snd[RXAD].block_bytes>>1));
      zrw=read(rx_audio_in2, rxin_isho2, (size_t)(snd[RXAD].block_bytes>>1));
      if( (ui.rx_input_mode&DWORD_INPUT) == 0)
        {
        for(i=snd[RXAD].block_frames-1; i>=0; i--)
          {
          rxin_isho[4*i  ]=rxin_isho[2*i  ];
          rxin_isho[4*i+1]=rxin_isho[2*i+1];
          rxin_isho[4*i+2]=rxin_isho2[2*i  ];
          rxin_isho[4*i+3]=rxin_isho2[2*i+1];
          }
        }
      else
        {
        rxin_int1=(int*)rxin_isho;
        rxin_int2=(int*)rxin_isho2;
        for(i=snd[RXAD].block_frames-1; i>=0; i--)
          {
          rxin_int1[4*i  ]=rxin_int1[2*i  ];
          rxin_int1[4*i+1]=rxin_int1[2*i+1];
          rxin_int1[4*i+2]=rxin_int2[2*i  ];
          rxin_int1[4*i+3]=rxin_int2[2*i+1];
          }
        }
      }
    }
  finish_rx_read();
  if(kill_all_flag) goto rxadin_error_exit;
  }
rxadin_error_exit:;
/*   Maybe needed with OSS in RDWR mode ????
while( rx_audio_out == rx_audio_in ||
       tx_audio_out == rx_audio_in)
  {
  lir_sleep(10000);
  }
*/  
close_rx_sndin();
rx_adin_init_error:;
if( allow_wse_parport != 0)
  {
  linrad_thread_stop_and_join(THREAD_HWARE_COMMAND);
  }
thread_status_flag[THREAD_RX_ADINPUT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_RX_ADINPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

void thread_test_rxda(void)
{
test_rxda();
}

void thread_test_txda(void)
{
test_txda();
}

void thread_test_rxad(void)
{
test_rxad();
}

int lir_tx_output_samples(void)
{
if ( (ui.use_alsa&PORTAUDIO_TX_OUT) != 0)
  {
  return ((txout_pa-txout_px+txout_bufsiz)&txout_mask)/2;
  }
int err;
char s[40];
snd_pcm_sframes_t frame_avail;
if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
  {
//Try to synchronize stream position with hardware
  err=snd_pcm_hwsync(alsa_handle[TXDA]);
  if(err != 0)
    {
    if(err == -EPIPE)       // underrun, try to recover
      {
      no_of_tx_underrun_errors++;
      sprintf(s,"TX%s%d",underrun_error_msg,no_of_tx_underrun_errors);
      wg_error(s,WGERR_TXOUT);
      snd_pcm_prepare(alsa_handle[TXDA]);
      err=snd_pcm_hwsync(alsa_handle[TXDA]);
      if(err < 0)
        {
//      lirerr(872315);  // accept error, we have to live with less accurate data in frame_avail
        }
      }
    else
      {
//    lirerr(872316);  // accept error, we have to live with less accurate data in frame_avail
      }
    }
  frame_avail= snd_pcm_avail(alsa_handle[TXDA]);
  if(frame_avail < 0)
          {
//          lirerr(872317);
          return 0;
          }
  return (int)(snd[TXDA].tot_frames-frame_avail);
  }
else
  {
#if HAVE_OSS == 1
  if(ioctl(tx_audio_out,SNDCTL_DSP_GETOSPACE, &tx_da_info) == -1)
                                                          lirerr(872313);
return snd[TXDA].tot_frames-tx_da_info.bytes/snd[TXDA].framesize;
#endif
  }
return 0;
}

int lir_tx_input_samples(void)
{
if ( (ui.use_alsa&PORTAUDIO_TX_IN) != 0)return 0;
int err;
char s[40];
snd_pcm_sframes_t frame_avail;
if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
  {
//Synchronize stream position with hardware
  err=snd_pcm_hwsync(alsa_handle[TXAD]);
    {
    if(err == -EPIPE)       // underrun, try to recover
      {
      no_of_tx_overrun_errors++;
      sprintf(s,"TX%s%d",overrun_error_msg,no_of_tx_overrun_errors);
      wg_error(s,WGERR_TXIN);
      snd_pcm_prepare(alsa_handle[TXAD]);
      err=snd_pcm_hwsync(alsa_handle[TXAD]);
      if(err < 0)
        {
//        lirerr(872318);   // recovery failed, we have to live with less accurate data in frame_avail
        }
      }
    else
      {
//      lirerr(872319);     // snd_pcm_hwsync failed with unknown error, we have to live with less accurate data in frame_avail
      }
    }
frame_avail= snd_pcm_avail(alsa_handle[TXAD]);
if(frame_avail < 0)
          {
//          lirerr(872314); //accept a lesser optimal solution
          return 0;
          }

return (int)frame_avail;

  }
else
  {
#if HAVE_OSS == 1
  if(ioctl(tx_audio_in,SNDCTL_DSP_GETISPACE, &tx_ad_info) == -1)lirerr(872314);
  return tx_ad_info.bytes/snd[TXAD].framesize;
#endif
  }
return 0;
}

void lir_empty_da_device_buffer(void)
{
if( (ui.use_alsa&PORTAUDIO_RX_OUT) != 0)
  {
  return;
  }
if(rx_audio_out == -1)
  {
  lirerr(745241);
  return;
  }
if(rx_audio_in == rx_audio_out &&
                   rx_input_thread == THREAD_RX_ADINPUT)return;
if(tx_audio_in == rx_audio_out)return;
if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
  {
  snd_pcm_drop(alsa_handle[RXDA]);
  snd_pcm_prepare(alsa_handle[RXDA]);
  }
else
  {
#if HAVE_OSS== 1  
  if(ioctl(rx_audio_out,SNDCTL_DSP_GETOSPACE, &rx_da_info) == -1)
    {
    lirerr(1077);
    return;
    }
  if(rx_da_info.fragstotal - rx_da_info.fragments <= 4)
    {
    zrw=write(rx_audio_out,&daout[daout_px],(size_t)snd[RXDA].block_bytes);
    }
  if(ioctl(rx_audio_out,SNDCTL_DSP_RESET,0)==-1)
    {
    lirerr(1205);
    return;
    }
#endif
  }
}

void lir_tx_dawrite(char *buf)
{
int err;
char s[40];
if( (ui.use_alsa&PORTAUDIO_TX_OUT) != 0)
  {
// We assume Portaudio is always OK.  
  lir_sched_yield();
  return;
  }
if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
  {
  err=snd_pcm_writei(alsa_handle[TXDA], buf,
                                 (snd_pcm_uframes_t)snd[TXDA].block_frames);
  if(err != snd[TXDA].block_frames)
    {
    if(err == -EPIPE)             // underrun try to recover
      {
      no_of_tx_underrun_errors++;
      sprintf(s,"TX%s%d",underrun_error_msg,no_of_tx_underrun_errors);
      wg_error(s,WGERR_TXOUT);
      err= snd_pcm_recover(alsa_handle[TXDA], err, 0);
      if(err < 0)
        {
        lirerr(1140);              // recovery failed
        }
      }
    else
      {
      if(err > 0 && err < (int)(snd[TXDA].block_frames ) )
        {
        lirerr(1328);                // short write
        }
      else
        {
        lirerr(1329);                // catch all other errors
        }
      }
    }
  }
else
  {
#if HAVE_OSS == 1  
  zrw=write(tx_audio_out,buf,(size_t)snd[TXDA].block_bytes);
#else
  zrw=buf[0];
#endif
  }
}

void lir_rx_dawrite(void)
{
int err;
char s[40];
int i, retry;
int wrbytes;
int bytes_written, bytes_to_write;
int cnt;
if( (ui.use_alsa&PORTAUDIO_RX_OUT) != 0)
  {
// We assume Portaudio is always OK.  
  lir_sched_yield();
  return;
  }
bytes_written=0;
bytes_to_write=snd[RXDA].block_bytes;
cnt=0;
while(bytes_to_write > 0)
  {
  if(daout_px+bytes_to_write > daout_bufmask)
    {
    wrbytes=daout_bufmask+1-daout_px;
    }
  else
    {
    wrbytes=bytes_to_write;
    }
  if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
    {
    i=wrbytes/snd[RXDA].framesize;
    err=snd_pcm_writei(alsa_handle[RXDA], &daout[daout_px],(snd_pcm_uframes_t)i);
    if(err == i)
      {
      bytes_written=wrbytes;
      }
    else
      {
      if(err < 0)
        {
        retry=0;
        if(err == -EPIPE)            // underrun, try to recover
          {
          if(count_rx_underrun_flag)
            {
            no_of_rx_underrun_errors++;
            sprintf(s,"RX%s%d",underrun_error_msg,no_of_rx_underrun_errors);
            wg_error(s,WGERR_RXOUT);
            }
recover:;
          err= snd_pcm_recover(alsa_handle[RXDA], err, 0);
          }
        if(err == -EBUSY)
          {
          retry++;
          if(retry < 15)
            {
            lir_sleep(1000);
            goto recover;
            }
          }
        if(err == -EBADFD)
          {
          close_rx_sndout();
          lir_sched_yield();
          open_rx_sndout();
          if(kill_all_flag) return;
          snd[RXDA].open_flag=CALLBACK_CMD_ACTIVE;
          }
        }
      else
        {
        if(err > 0 && err < (int) (snd[RXDA].block_frames ) )
          {
          bytes_written=err*snd[RXDA].framesize;
          }
        else
          {
          lirerr(1332);              // catch all other errors
          }
        }
      }
    }
  else
    {
#if HAVE_OSS == 1  
    if(daout_px+bytes_to_write > daout_bufmask)
      {
      wrbytes=daout_bufmask+1-daout_px;
      }
    else
      {
      wrbytes=bytes_to_write;
      }
    bytes_written=write(rx_audio_out,&daout[daout_px],(size_t)wrbytes);
    if(bytes_written < 0)
      {
      lir_sched_yield();
      if(ioctl(rx_audio_out,SNDCTL_DSP_RESET,0)==-1)
        {
        lirerr(1205001);
        return;
        }
      close_rx_sndout();  
      lir_sched_yield();
      open_rx_sndout();
      snd[RXDA].open_flag=CALLBACK_CMD_ACTIVE;
      lir_sched_yield();
      bytes_written=write(rx_audio_out,&daout[daout_px], (size_t)wrbytes);
      if(bytes_written < 0)
        {
        lirerr(1205002);
        return;
        }
      }  
#endif
    }
  daout_px=(daout_px+bytes_written)&daout_bufmask;
  bytes_to_write-=bytes_written;
  cnt++;
  if(cnt>10)
    {
    lirerr(1331);
    }
  }
}

#if HAVE_OSS == 1  
void make_devname(int nn, char* ss)
{
char s[80];
int n;
n=nn&255;
// This routine is greatly simplified July 2006 thanks to Diane Bruce, VA3DB.
sprintf(s,"%s",SND_DEV);
if(s[0] == 'u')
  {
  lirerr(1200);
  return;
  }
if(n>MAX_DEVNAMES)
  {
  lirerr(1255);
  return;
  }
if(n == MAX_DEVNAMES-1)
  {
  sprintf(ss, "%s", SND_DEV);
  }
else
  {
  sprintf(ss, "%s%d", SND_DEV, n);
  }
}
#endif

void close_rx_sndin(void)
{
int err;
if(ui.use_extio != 0 && ui.extio_type != 4)return;  
if( (ui.use_alsa&PORTAUDIO_RX_IN) != 0)
  {
  close_portaudio_rxad();
  return;
  }
if(rx_audio_in != -1)
  {
  if(rx_audio_out == rx_audio_in)
    {
    close_rx_sndout();
    }
  if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
    {
    err=snd_pcm_close(alsa_handle[RXAD]);
    if(err < 0)
      {
      lirerr(1005);
      return;
      }
     }
  else
    {
    if(close(rx_audio_in) == -1)
      {
      lirerr(1005);
      return;
      }
    }
  if(ui.rx_addev_no > 255)
    {
    if(close(rx_audio_in2) == -1)lirerr(1173);
    }
  }
rx_audio_in=-1;
}

void open_tx_sndin(void)
{
#if HAVE_OSS == 1  
char ss[sizeof(SND_DEV)+2];
int frag;
float t2;
int m, k;
#endif
float t1;
int i, j;
unsigned int val;
static snd_pcm_format_t format;
snd_pcm_hw_params_t *hw_ad_params;
int err, resample;
snd[TXAD].block_frames=snd[TXAD].block_bytes/snd[TXAD].framesize;
if(snd[TXAD].block_frames<8)snd[TXAD].block_frames=8;
snd[TXAD].interrupt_rate=(float)(ui.tx_ad_speed)/(float)snd[TXAD].block_frames;
while(snd[TXAD].interrupt_rate > ui.max_dma_rate)
  {
  snd[TXAD].interrupt_rate/=2;
  snd[TXAD].block_bytes*=2;
  snd[TXAD].block_frames*=2;
  }
while(snd[TXAD].interrupt_rate < ui.min_dma_rate && snd[TXAD].block_frames>8)
  {
  snd[TXAD].interrupt_rate*=2;
  snd[TXAD].block_bytes/=2;
  snd[TXAD].block_frames/=2;
  }
DEB"\nTXin%sDesired %f",intrate_string,snd[TXAD].interrupt_rate);
if(ui.tx_addev_no == ui.rx_dadev_no && ui.rx_damode == O_RDWR)
  {
  while(rx_audio_out == -1)
    {
    lir_sleep(10000);
    }
  tx_audio_in=rx_audio_out;
  return;
  }
if( (ui.use_alsa&PORTAUDIO_TX_IN) != 0)
  {
  open_portaudio_txad();
  return;
  }
if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
  {
 char tx_input_plughw_pcm_name  [MAX_ALSADEV];
 char tx_input_hw_pcm_name  [MAX_ALSADEV];
  err=chk_open_alsa(ui.tx_addev_no,
                    ui.tx_ad_bytes,
                    ui.tx_ad_speed,
                    ui.tx_ad_channels,
                    SND_PCM_STREAM_CAPTURE);
    if(err != 0)
      {
      lirerr(1153);  //chk_open tx_input failed
      return;
      }
//get plughw_pcm_name corresponding with ui definition
  alsar_get_dev_pcm_names(ui.tx_addev_no,tx_input_plughw_pcm_name,tx_input_hw_pcm_name);
  err=snd_pcm_open(&alsa_handle[TXAD], tx_input_plughw_pcm_name,
                                                     SND_PCM_STREAM_CAPTURE,0);
  if(err < 0)
    {
    lirerr(1333);    // open tx_input failed
    return;
    }
// use tx_audio_in as switch for compatibility reasons with the existing OSS logic */
// do not think this is used any more (May 2010)
  tx_audio_in=1;
//alsa housekeeping
  err=snd_pcm_hw_params_malloc(&hw_ad_params);
  if(err < 0)
    {
    lirerr(1272);
    return;
    }
  err=snd_pcm_hw_params_any(alsa_handle[TXAD], hw_ad_params);
  if(err < 0)
    {
    lirerr(1282);
    return;
    }
// Interleaved mode
  err=snd_pcm_hw_params_set_access(alsa_handle[TXAD], hw_ad_params,
                                             SND_PCM_ACCESS_RW_INTERLEAVED);
  if(err< 0)
    {
    lirerr(1294);
    return;
    }
// Set sample format
  if(ui.tx_ad_bytes == 2)
    {
    format=SND_PCM_FORMAT_S16_LE;
    }
  else
    {
    format=SND_PCM_FORMAT_S32_LE;
    }
  err=snd_pcm_hw_params_set_format(alsa_handle[TXAD], hw_ad_params,format);
  if(err < 0)
    {
    lirerr(1295);
    return;
    }
// Set number of ad_channels
  err=snd_pcm_hw_params_set_channels(alsa_handle[TXAD],
                       hw_ad_params, (unsigned int)ui.tx_ad_channels);
  if(err < 0)
    {
    lirerr(1296);
    return;
    }
// Set sample rate
// first set hardware resample
  resample=1;
  err=snd_pcm_hw_params_set_rate_resample(alsa_handle[TXAD],
                   hw_ad_params, (unsigned int)resample);
  if(err < 0)
    {
    lirerr(1298);
    return;
    }
// now set sample rate
  val=(unsigned int)ui.tx_ad_speed;
  err=snd_pcm_hw_params_set_rate_near(alsa_handle[TXAD],hw_ad_params, &val, 0);
  if(err < 0)
    {
    lirerr(1299);
    return;
    }
  t1=(float)(val)/(float)ui.tx_ad_speed;
  if(t1 > 1.05 || t1 < 0.95)
    {
    lirerr(1012);
    return;
    }
  ui.tx_ad_speed=(int)val;
// Set period size:
// We want the period_size to match txad.block_fragments
  i=snd[TXAD].block_frames;
  j=0;
periodset:;
  err=snd_pcm_hw_params_set_period_size(alsa_handle[TXAD],hw_ad_params,
                          (snd_pcm_uframes_t)snd[TXAD].block_frames, 0);
  if(err < 0)
    {
    if(j==0)
      {
      snd[TXAD].block_frames/=2;
      if(snd[TXAD].block_frames > 32)goto periodset;
      j=1;
      snd[TXAD].block_frames=i;
      }
    snd[TXAD].block_frames*=2;
    if(snd[TXAD].block_frames < 8192)goto periodset;
    lirerr(1318);
    return;
    }
//  Write all the parameters to the driver
  err=snd_pcm_hw_params(alsa_handle[TXAD], hw_ad_params);
  if(err < 0)
    {
    lirerr(1301);
    return;
    }
// free the memory for the hardware parameter structure
  snd_pcm_hw_params_free(hw_ad_params);
  i=snd[TXAD].block_frames;
  make_power_of_two(&i);
  if(i != (int)snd[TXAD].block_frames)
    {
    lirerr(1149);
    return;
    }
  snd[TXAD].interrupt_rate=
                     (float)(ui.tx_ad_speed)/(float)snd[TXAD].block_frames;
  DEB"\nTXin%sActual %f",intrate_string,snd[TXAD].interrupt_rate);
  }
else
  {
#if HAVE_OSS == 1  
  make_devname(ui.tx_addev_no, ss);
  if(kill_all_flag)return;
  tx_audio_in=open( ss ,O_RDONLY|O_EXCL, 0);
  if(tx_audio_in == -1)
    {
    lirerr(1260);
    return;
    }
// We want the fragment size to match snd[TXAD].block_bytes.
  frag=0;
  m=snd[TXAD].block_bytes;
  while(m>1)
    {
    m>>=1;
    frag++;
    }
  if(frag < 5)frag=5;
  frag=frag|0x7fff0000;
  if(ioctl(tx_audio_in, SNDCTL_DSP_SETFRAGMENT, &frag) == -1)
    {
    lirerr(1262);
    return;
    }
  m=AFMT_S16_LE;
#ifdef AFMT_S32_LE
  if(ui.tx_ad_bytes==4)m=AFMT_S32_LE;
#else
  ui.tx_ad_bytes=2;
#endif
  k=m;
  if(ioctl(tx_audio_in, SNDCTL_DSP_SETFMT, &m) == -1)
    {
    lirerr(231013);
    return;
    }
  if(m!=k)
    {
    lirerr(231014);
    return;
    }
// Set number of channels
  k=ui.tx_ad_channels;
  m=k;
  if(ioctl(tx_audio_in, SNDCTL_DSP_CHANNELS, &m) == -1)
    {
    lirerr(231009);
    return;
    }
  if(m != k)
    {
    lirerr(231010);
    return;
    }
  m=ui.tx_ad_speed;
  if(ioctl(tx_audio_in, SNDCTL_DSP_SPEED, &m)==-1)
    {
    lirerr(231011);
    return;
    }
  t2=(float)(m)/(float)ui.tx_ad_speed;
  if(t2 > 1.05 || t2 < 0.95)
    {
    lirerr(231012);
    return;
    }
  ui.tx_ad_speed=m;
// This has to be the last ioctl call before read or write!!
  t2=(float)(ui.tx_ad_channels*ui.tx_ad_bytes*
                               ui.tx_ad_speed)/(float)snd[TXAD].block_bytes;
  DEB"\nTXin interrupt_rate: Desired  %f",t2);
  ioctl(tx_audio_in, SNDCTL_DSP_GETBLKSIZE,&m);
  t2=(float)(ui.tx_ad_channels*ui.tx_ad_bytes*ui.tx_ad_speed)/(float)m;
  DEB"  actual %f\n",t2);
  if(ioctl(tx_audio_in,SNDCTL_DSP_GETISPACE, &tx_ad_info) == -1)
                                                          lirerr(882313);
  snd[TXAD].tot_bytes=tx_ad_info.fragstotal*tx_ad_info.fragsize;
  snd[TXAD].tot_frames=snd[TXAD].tot_bytes/snd[TXAD].framesize;
  snd[TXAD].block_bytes=tx_ad_info.fragsize;
  snd[TXAD].block_frames=snd[TXAD].block_bytes/snd[TXAD].framesize;
  snd[TXAD].no_of_blocks=snd[TXAD].tot_bytes/snd[TXAD].block_bytes;
#endif
  }
}

void close_tx_sndin(void)
{
int err;
if(tx_audio_in == -1)return;
if( (ui.use_alsa&PORTAUDIO_TX_IN) != 0)
  {
  close_portaudio_txad();
  tx_audio_in=-1;
  return;
  }
if(ui.tx_addev_no == ui.rx_dadev_no && ui.rx_damode == O_RDWR)
  {
  tx_audio_in=-1;
  return;
  }
if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
  {
  err=snd_pcm_close(alsa_handle[TXAD]);
  if(err < 0)
    {
    lirerr(1005);
    return;
    }
   }
else
  {
#if HAVE_OSS == 1  
  if(close(tx_audio_in)==-1)lirerr(231037);
  tx_audio_in=-1;
#endif
  }
}

void open_tx_sndout(void)
{
unsigned int val;
static snd_pcm_format_t format;
snd_pcm_hw_params_t *hw_da_params;
snd_pcm_uframes_t block_frames, tot_frames;
int dir, err, resample;
float t1;
#if HAVE_OSS == 1  
int frag;
char ss[sizeof(SND_DEV)+2];
int m, k;
float t2;
#endif
snd[TXDA].block_frames=snd[TXDA].block_bytes/snd[TXDA].framesize;
if(snd[TXDA].block_frames<8)
  {
  snd[TXDA].block_frames=8;
  snd[TXDA].block_bytes=snd[TXDA].block_frames*snd[TXDA].framesize;
  }
snd[TXDA].interrupt_rate=(float)(ui.tx_da_speed)/(float)snd[TXDA].block_frames;
while(snd[TXDA].interrupt_rate > ui.max_dma_rate)
  {
  snd[TXDA].interrupt_rate/=2;
  snd[TXDA].block_bytes*=2;
  snd[TXDA].block_frames*=2;
  }
while(snd[TXDA].interrupt_rate < ui.min_dma_rate && snd[TXDA].block_frames>8)
  {
  snd[TXDA].interrupt_rate*=2;
  snd[TXDA].block_bytes/=2;
  snd[TXDA].block_frames/=2;
  }
DEB"\nTXout%sDesired %f",intrate_string,snd[TXDA].interrupt_rate);
if( (ui.use_alsa&PORTAUDIO_TX_OUT) != 0)
  {
  open_portaudio_txda();
  return;
  }
if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
  {
  char tx_out_plughw_pcm_name[MAX_ALSADEV];
  char tx_out_hw_pcm_name[MAX_ALSADEV];

  err=chk_open_alsa(ui.tx_dadev_no,
                    ui.tx_da_bytes,
                    ui.tx_da_speed,
                    ui.tx_da_channels,
                    SND_PCM_STREAM_PLAYBACK);
    if(err != 0)
      {
      lirerr(1151);
      return;
      }
//get plughw_pcm_name corresponding with ui definition
  alsar_get_dev_pcm_names(ui.tx_dadev_no,tx_out_plughw_pcm_name,tx_out_hw_pcm_name);
  if((err=snd_pcm_open(&alsa_handle[TXDA], tx_out_plughw_pcm_name,
                                         SND_PCM_STREAM_PLAYBACK, 0)) < 0)
    {
    lirerr(1304);
    return;
    }
// use tx_audio_out as switch for compatibility reasons with the existing OSS logic
// do not think this is used any more (May 2010)
  tx_audio_out=1;
//*********************************************************************
//******************** SET PLAYBACK PARAMETERS THE ALSA WAY ******
//*********************************************************************
  err=snd_pcm_hw_params_malloc (&hw_da_params);
  if(err < 0)
    {
    lirerr(1305);
    return;
    }
  err=snd_pcm_hw_params_any(alsa_handle[TXDA], hw_da_params);
  if(err < 0)
    {
    lirerr(1306);
    return;
    }
// Interleaved mode
  err=snd_pcm_hw_params_set_access(alsa_handle[TXDA], hw_da_params,
                                         SND_PCM_ACCESS_RW_INTERLEAVED);
  if(err < 0)
    {
    lirerr(1307);
    return;
    }
// Set sample format
  if(ui.tx_da_bytes == 2)
    {
    format=SND_PCM_FORMAT_S16_LE;
    }
  else
    {
    format=SND_PCM_FORMAT_S32_LE;
    }
  err=snd_pcm_hw_params_set_format(alsa_handle[TXDA], hw_da_params,format);
  if(err < 0)
    {
    lirerr(1308);
    return;
    }
// Set number of da_channels
  err=snd_pcm_hw_params_set_channels(alsa_handle[TXDA], 
                              hw_da_params, (unsigned int)ui.tx_da_channels);
  if(err <0)
    {
    lirerr(1309);
    return;
    }
// Set sample rate
// first enable  hardware resample
  resample=1;
  err=snd_pcm_hw_params_set_rate_resample(alsa_handle[TXDA], 
                                         hw_da_params, (unsigned int)resample);
  if(err < 0)
    {
    lirerr(1310);
    return;
    }
// now set sample rate
  val=(unsigned int)ui.tx_da_speed;
  if((err =snd_pcm_hw_params_set_rate_near(
                                   alsa_handle[TXDA],hw_da_params, &val, 0)<0))
    {
    lirerr(1311);
    return;
    }
  t1=(float)(val)/(float)ui.tx_da_speed;
  if(t1 > 1.05 || t1 < 0.95)
    {
    lirerr(1023);
    return;
    }
  ui.tx_da_speed=(int)val;
  dir=0;
  block_frames=snd[TXDA].block_frames;
// obtained value is <,=,> target-value, according value of  dir (-1,0,1)
  err=snd_pcm_hw_params_set_period_size_near(alsa_handle[TXDA], hw_da_params,
                                                        &block_frames, &dir);
  if(err < 0)
    {
    lirerr(1312);
    return;
    }
// check obtained  period size
  err=snd_pcm_hw_params_get_period_size(hw_da_params, &block_frames, &dir);
  if(err < 0)
    {
    DEB"\nopen_tx_sndout:Unable to get period size for playback: %s\n",
                                                       snd_strerror(err));
    lirerr(12953);
    return;
    }
  snd[TXDA].block_frames=(int)block_frames;;
  make_power_of_two(&snd[TXDA].block_frames);
  snd[TXDA].block_bytes=snd[TXDA].block_frames*snd[TXDA].framesize;
  snd[TXDA].interrupt_rate=(float)(ui.tx_da_speed)/(float)snd[TXDA].block_frames;
// set the hw parameters
  err=snd_pcm_hw_params(alsa_handle[TXDA], hw_da_params);
  if(err < 0)
    {
    lirerr(1313);
    return;
    }
// free the memory for the hardware parameter struct
  snd_pcm_hw_params_free (hw_da_params);
  snd[TXDA].block_frames= snd[TXDA].block_bytes/snd[TXDA].framesize;
  block_frames=snd[TXDA].block_frames;
  tot_frames=snd[TXDA].tot_frames;
// get buffersize
  err=snd_pcm_get_params ( alsa_handle[TXDA], &tot_frames, &block_frames);
  if(err < 0)
      {
      lirerr(1313);
      return;
      }
  snd[TXDA].block_frames=(int)block_frames;
  snd[TXDA].tot_frames=(int)tot_frames;
  snd[TXDA].tot_bytes=snd[TXDA].tot_frames*snd[TXDA].framesize;
  snd[TXDA].no_of_blocks=snd[TXDA].tot_bytes/snd[TXDA].block_frames;
  goto ok_exit;
  }
else
  {
#if HAVE_OSS == 1  
 if(ui.tx_enable != 0 && ui.rx_addev_no == ui.tx_dadev_no 
                                         && ui.rx_admode == O_RDWR )
    {
    m=0;
    while(rx_audio_in == -1)
      {
      m++;
      lir_sleep(30000);
      if(m>100)
        {
        lirerr(32963);
        }
      }
    snd[TXDA]=snd[RXAD];
    tx_audio_out=rx_audio_in;
    }
  else
    {
    make_devname(ui.tx_dadev_no, ss);
    if(kill_all_flag)return;
    tx_audio_out=open( ss, O_WRONLY|O_NONBLOCK|O_EXCL, 0);
    if(tx_audio_out == -1)
      {
      lirerr(1079);
      return;
      }
    frag=0;
    m=snd[TXDA].block_bytes;
    while(m>1)
      {
      m>>=1;
      frag++;
      }
    if(frag < 5)frag=5;
    frag=frag|0x7fff0000;
    if(ioctl(tx_audio_out, SNDCTL_DSP_SETFRAGMENT, &frag) == -1)
      {
      lirerr(991262);
      return;
      }
    m=AFMT_S16_LE;
#ifdef AFMT_S32_LE
    if(ui.tx_da_bytes==4)m=AFMT_S32_LE;
#else
    ui.tx_da_bytes=2;
#endif
    k=m;
    if(ioctl(tx_audio_out, SNDCTL_DSP_SETFMT, &m) == -1)
      {
      lirerr(231013);
      return;
      }
    if(m!=k)
      {
      lirerr(231014);
      return;
      }
// Set number of channels
    k=ui.tx_da_channels;
    m=k;
    if(ioctl(tx_audio_out, SNDCTL_DSP_CHANNELS, &m) == -1)
      {
      lirerr(231009);
      return;
      }
    if(m != k)
      {
      lirerr(231010);
      return;
      }
    m=ui.tx_da_speed;
    if(ioctl(tx_audio_out, SNDCTL_DSP_SPEED, &m)==-1)
      {
      lirerr(231011);
      return;
      }
    t2=(float)(m)/(float)ui.tx_da_speed;
    if(t2 > 1.05 || t2 < 0.95)
      {
      lirerr(231112);
      return;
      }
    ui.tx_da_speed=m;
    }
  if(ioctl(tx_audio_out,SNDCTL_DSP_GETOSPACE, &tx_da_info) == -1)
                                                          lirerr(872313);
  snd[TXDA].tot_bytes=tx_da_info.fragstotal*tx_da_info.fragsize;
  snd[TXDA].tot_frames=snd[TXDA].tot_bytes/snd[TXDA].framesize;
  snd[TXDA].block_bytes=tx_da_info.fragsize;
  snd[TXDA].block_frames=snd[TXDA].block_bytes/snd[TXDA].framesize;
  snd[TXDA].no_of_blocks=snd[TXDA].tot_bytes/snd[TXDA].block_bytes;
  snd[TXDA].interrupt_rate=(float)(ui.tx_da_speed)/(float)snd[TXDA].block_frames;
#endif
  }
ok_exit:;
DEB"\nTXout%sActual %f",intrate_string,snd[TXDA].interrupt_rate);
}

void close_tx_sndout(void)
{
int err;
if(tx_audio_out == -1)return;
if( (ui.use_alsa&PORTAUDIO_TX_OUT) != 0)
  {
  close_portaudio_txda();
  goto closed;
  }
if(ui.tx_dadev_no == ui.rx_addev_no && ui.rx_admode == O_RDWR)
  {
  goto closed;
  }
if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
  {
  snd_pcm_drop(alsa_handle[TXDA]);
  err= snd_pcm_close(alsa_handle[TXDA]);
  if(err < 0)lirerr(1006);
  }
else
  {
#if HAVE_OSS == 1  
  if(ioctl(tx_audio_out,SNDCTL_DSP_RESET,0)==-1)
    {
    lirerr(1288);
    }
  if(close(tx_audio_out)==-1)lirerr(1287);
#endif
  }
closed:;
tx_audio_out=-1;
}

void close_rx_sndout(void)
{
int err;
if(rx_audio_out == -1)
  {
  return;
  }
if( (ui.use_alsa&PORTAUDIO_RX_OUT) != 0)
  {
  close_portaudio_rxda();
  goto closed;
  }
if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
  {
  snd_pcm_drop(alsa_handle[RXDA]);
  err= snd_pcm_close(alsa_handle[RXDA]);
  if(err < 0)lirerr(1006);
  }
else
  {
#if HAVE_OSS == 1  
  if(rx_audio_out != rx_audio_in)
    {
    if(ioctl(rx_audio_out,SNDCTL_DSP_RESET,0)==-1)
      {
      lirerr(1026);
      }
    if(close(rx_audio_out)==-1)lirerr(1006);
    }
#endif
  }
closed:;
rx_audio_out=-1;
}

int open_rx_sndin(int report_errors)
{
#if HAVE_OSS == 1  
char ss[sizeof(SND_DEV)+2];
audio_buf_info ad_info;
int i, j, frag;
float t2;
#endif
int ir;
snd_pcm_format_t format;
snd_pcm_hw_params_t *hw_ad_params;
int k, err, resample;
unsigned int val;
float t1;
ir=report_errors;
rxad_status=LIR_OK;
if(ui.use_extio != 0 && ui.extio_type != 4)return 0;  
if( (rx_audio_in) != -1)
  {
  if(ui.rx_admode == O_RDWR)return 0;
  lirerr(1000);
  return 0;
  }
if( (ui.use_alsa&PORTAUDIO_RX_IN) != 0)
  {
  open_portaudio_rxad();
  return 0;
  }
if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
  {
  char rx_sndin_plughw_pcm_name  [MAX_ALSADEV];
  char rx_sndin_hw_pcm_name  [MAX_ALSADEV];
  ir=2;
  if( (ui.rx_input_mode&DWORD_INPUT) != 0)ir=4;
// We must call alsa_get_dev_names first. Otherwise chk_open_alsa
// fails in a 32 bit executable on a 64 bit operating system
// with a message libasound_module_conf_pulse.so can not be loaded
  alsa_get_dev_names (ui.rx_addev_no);
  err=chk_open_alsa(ui.rx_addev_no,
                    ir,
                    ui.rx_ad_speed,
                    ui.rx_ad_channels,
                    SND_PCM_STREAM_CAPTURE);
  if(err != 0)
    {
    lirerr(1152);  // chkopen rx_sndin failed
    return 0;
    }
//get plughw_pcm_name corresponding with ui definition
  alsar_get_dev_pcm_names(ui.rx_addev_no,rx_sndin_plughw_pcm_name,rx_sndin_hw_pcm_name );
  err=snd_pcm_open(&alsa_handle[RXAD], rx_sndin_plughw_pcm_name,
                                                 SND_PCM_STREAM_CAPTURE, 0);
  if(err < 0)
    {
    lirerr(1271);  // open rx_sndin failed
    return 0;
    }
// use rx_audio_in as switch for compatibility reasons with the existing OSS logic */
  rx_audio_in=2;
//alsa housekeeping
  err=snd_pcm_hw_params_malloc(&hw_ad_params);
  if(err < 0)
    {
    lirerr(1272);
    return 0;
    }
  err=snd_pcm_hw_params_any(alsa_handle[RXAD], hw_ad_params);
  if(err < 0)
    {
    lirerr(1282);
    return 0;
    }
// Interleaved mode
  err=snd_pcm_hw_params_set_access(alsa_handle[RXAD], hw_ad_params,
                                              SND_PCM_ACCESS_RW_INTERLEAVED);
  if(err < 0)
    {
    lirerr(1294);
    return 0;
    }
// Set sample format
  format=SND_PCM_FORMAT_S16_LE;
  if( (ui.rx_input_mode&DWORD_INPUT) != 0) format=SND_PCM_FORMAT_S32_LE;
  err=snd_pcm_hw_params_set_format (alsa_handle[RXAD], hw_ad_params,format);
  if(err < 0)
    {
    lirerr(1295);
    return 0;
    }
// Set number of ad_channels
  err=snd_pcm_hw_params_set_channels(alsa_handle[RXAD], hw_ad_params, 
                                          (unsigned int)ui.rx_ad_channels);
  if(err < 0)
    {
    lirerr(1296);
    return 0;
    }
// Set sample rate
// first disable  hardware resample
  resample=0;
  err=snd_pcm_hw_params_set_rate_resample(alsa_handle[RXAD],
                                          hw_ad_params,
                                          (unsigned int)resample);
  if(err < 0)
    {
    lirerr(1298);
    return 0;
    }
// now set sample rate
  val=(unsigned int)ui.rx_ad_speed;
  err=snd_pcm_hw_params_set_rate_near(alsa_handle[RXAD],hw_ad_params, &val, 0);
  if(err<0)
    {
    lirerr(1299);
    return 0;
    }
  t1=(float)(val)/(float)ui.rx_ad_speed;
  if(t1 > 1.05 || t1 < 0.95)
    {
    lirerr(1012);
    return 0;
    }
  ui.rx_ad_speed=(int)val;
// Set period size:
// We want the period_size to match snd[RXAD].block_frames
  val=(unsigned int)snd[RXAD].block_frames;
  k=1;
periodset:;
  err=snd_pcm_hw_params_set_period_size(alsa_handle[RXAD],hw_ad_params, val, 0);
  if(err<0)
    {
    if(k==1)
      {
      val/=2;
      if(val > 32)goto periodset;
      }
    k*=2;
    val=(unsigned int)(k*snd[RXAD].block_frames);
    if(k<=64)goto periodset;
    lirerr(1300);
    return 0;
    }
//  Write all the parameters to the driver
  if((err=snd_pcm_hw_params(alsa_handle[RXAD], hw_ad_params))< 0)
    {
    lirerr(1301);
    return 0;
    }
// free the memory for the hardware parameter structure
  snd_pcm_hw_params_free (hw_ad_params);

  snd[RXAD].block_frames=snd[RXAD].block_bytes/snd[RXAD].framesize;
  return 0;
  }
else
  {
#if HAVE_OSS == 1
  make_devname(ui.rx_addev_no, ss);
  if(kill_all_flag)return 0;
  rx_audio_in=open(ss ,ui.rx_admode|O_EXCL , 0);
  if(rx_audio_in == -1)
    {
    lirerr(1007);
    return 0;
    }
  if(ui.rx_addev_no > 255)
    {
    i=ui.rx_addev_no/255-1;
    make_devname(i, ss);
    if(kill_all_flag)return 0;
    rx_audio_in2=open( ss ,ui.rx_admode|O_EXCL , 0);
    if(rx_audio_in2 == -1)
      {
      lirerr(1174);
      return 0;
      }
    }
// We want the fragment size to match snd[RXAD].block_bytes.
  frag=0;
  i=snd[RXAD].block_bytes>>1;
  while(i>0)
    {
    i>>=1;
    frag++;
    }
  if(frag < 4)frag=4;
  if(ui.rx_addev_no > 255)
    {
    frag--;
    if(frag < 4)frag=4;
    frag=frag|0x7fff0000;
    if(ioctl(rx_audio_in2, SNDCTL_DSP_SETFRAGMENT, &frag) == -1)
      {
      lirerr(1175);
      return 0;
      }
    }
  frag=frag|0x7fff0000;
  if(ioctl(rx_audio_in, SNDCTL_DSP_SETFRAGMENT, &frag) == -1)
    {
    lirerr(1008);
    return 0;
    }
  i=AFMT_S16_LE;
#ifdef AFMT_S32_LE
  if( (ui.rx_input_mode&DWORD_INPUT) != 0)i=AFMT_S32_LE;
#else
  if( (ui.rx_input_mode&DWORD_INPUT) != 0)
    {
    lirerr(1246);
    return 0;
    }
#endif
  j=i;
  if(ioctl(rx_audio_in, SNDCTL_DSP_SETFMT, &i) == -1)
    {
    lirerr(1013);
    return 0;
    }
  if(i!=j)
    {
    lirerr(1014);
    return 0;
    }
  if(ui.rx_addev_no > 255)
    {
    j=i;
    if(ioctl(rx_audio_in2, SNDCTL_DSP_SETFMT, &i) == -1)
      {
      lirerr(1176);
      return 0;
      }
    if(i!=j)
      {
      lirerr(1177);
      return 0;
      }
    }
// Set number of ad_channels
  j=ui.rx_ad_channels;
  if(ui.rx_addev_no > 255)j=2;
  i=j;
  if(ioctl(rx_audio_in, SNDCTL_DSP_CHANNELS, &i) == -1)
    {
    lirerr(1009);
    return 0;
    }
  if(i != j)
    {
    lirerr(1010);
    return 0;
    }
  if(ui.rx_addev_no > 255)
    {
    i=j;
    if(ioctl(rx_audio_in2, SNDCTL_DSP_CHANNELS, &i) == -1)
      {
      lirerr(1178);
      return 0;
      }
    if(i != j)
      {
      lirerr(1179);
      return 0;
      }
    }
  i=ui.rx_ad_speed;
  if(ioctl(rx_audio_in, SNDCTL_DSP_SPEED, &i)==-1)
    {
    lirerr(1011);
    return 0;
    }
  t2=(float)(i)/(float)ui.rx_ad_speed;
  if(t2 > 1.05 || t2 < 0.95)
    {
    lirerr(1012);
    return 0;
    }
  ui.rx_ad_speed=i;
  if(ui.rx_addev_no > 255)
    {
    if(ioctl(rx_audio_in2, SNDCTL_DSP_SPEED, &i)==-1)
      {
      lirerr(1180);
      return 0;
      }
    if(i != ui.rx_ad_speed)
      {
      lirerr(1181);
      return 0;
      }
// Now that both devices are configured to be equal, synchronize them.
    i=0;
    if(ioctl(rx_audio_in,SNDCTL_DSP_SETTRIGGER,&i)==-1)
      {
      lirerr(1183);
      return 0;
      }
    i=0;
    if(ioctl(rx_audio_in2,SNDCTL_DSP_SETTRIGGER,&i)==-1)
      {
      lirerr(1184);
      return 0;
      }
    i=PCM_ENABLE_INPUT;
    if(ioctl(rx_audio_in,SNDCTL_DSP_SETTRIGGER,&i)==-1)
      {
      lirerr(1183);
      return 0;
      }
    i=PCM_ENABLE_INPUT;
    if(ioctl(rx_audio_in2,SNDCTL_DSP_SETTRIGGER,&i)==-1)
      {
      lirerr(1184);
      return 0;
      }
    }
  if(ioctl(rx_audio_in, SNDCTL_DSP_GETISPACE, &ad_info) == -1)
    {
    lirerr(1109);
    return 0;
    }
  overrun_limit=ad_info.fragstotal-1;
  if(ui.rx_addev_no > 255)
    {
    if(ioctl(rx_audio_in2, SNDCTL_DSP_GETISPACE, &ad_info) == -1)
      {
      lirerr(1109);
      return 0;
      }
    if(overrun_limit != ad_info.fragstotal-1)
      {
      lirerr(1109);
      }
    }
// This has to be the last ioctl call before read or write!!
  ioctl(rx_audio_in,SNDCTL_DSP_GETBLKSIZE,&i);
  snd[RXAD].block_frames=snd[RXAD].block_bytes/snd[RXAD].framesize;
#endif
  }
return 0;  
}

void open_rx_sndout(void)
{
snd_pcm_hw_params_t *hw_da_params;
snd_pcm_uframes_t block_frames, tot_frames;
static snd_pcm_format_t format;
int err, resample, dir;
unsigned int val;
int alsa_retries;
char rx_out_plughw_pcm_name  [MAX_ALSADEV];
char rx_out_hw_pcm_name  [MAX_ALSADEV];
float t1;
int i;
#if HAVE_OSS == 1
char cc;
int frag;
char ss[sizeof(SND_DEV)+2];
char* tmpbuf;
int j;
#endif
rxda_status=LIR_OK;
snd[RXDA].framesize=rx_daout_channels*rx_daout_bytes;
t1=min_delay_time;
i=4;
if(diskread_flag >= 2)i=1;
if(t1 < fft3_blocktime/(float)i)
  {
  t1=fft3_blocktime/(float)i;
  }
if(t1 > 1)t1=1;
snd[RXDA].block_frames=(int)(0.75F*t1*(float)genparm[DA_OUTPUT_SPEED]);
make_power_of_two(&snd[RXDA].block_frames);
// make the minimum block 8 frames
if(snd[RXDA].block_frames < 8)snd[RXDA].block_frames=8;
make_power_of_two(&snd[RXDA].block_frames);
snd[RXDA].interrupt_rate=(float)(genparm[DA_OUTPUT_SPEED])/
                                      (float)snd[RXDA].block_frames;
while(snd[RXDA].interrupt_rate < ui.min_dma_rate && snd[RXDA].block_frames>8)
  {
  snd[RXDA].interrupt_rate*=2;
  snd[RXDA].block_frames/=2;
  }
while(snd[RXDA].interrupt_rate > ui.max_dma_rate)
  {
  snd[RXDA].interrupt_rate/=2;
  snd[RXDA].block_frames*=2;
  }
DEB"\nRXout interrupt_rate: Desired %f",snd[RXDA].interrupt_rate);
// Allocate buffers for minimum 0.3 seconds.
snd[RXDA].block_bytes=snd[RXDA].block_frames*snd[RXDA].framesize;
snd[RXDA].no_of_blocks=(int)(0.3F*snd[RXDA].interrupt_rate);
make_power_of_two(&snd[RXDA].no_of_blocks);
if(snd[RXDA].no_of_blocks < 8)snd[RXDA].no_of_blocks=8;
snd[RXDA].tot_bytes=snd[RXDA].no_of_blocks*snd[RXDA].block_bytes;
snd[RXDA].tot_frames=snd[RXDA].no_of_blocks*snd[RXDA].block_frames;
if(ui.rx_dadev_no == DISABLED_DEVICE_CODE)return;
if( (ui.use_alsa&PORTAUDIO_RX_OUT) != 0)
  {
  open_portaudio_rxda();
  return;
  }
if( rx_audio_out != -1)
  {
  lirerr(1015);
  return;
  }
if( (ui.rx_input_mode&NO_DUPLEX) != 0 && rx_audio_in != -1)return;
if(ui.rx_addev_no == ui.rx_dadev_no && (ui.rx_admode&O_RDWR) == O_RDWR )
  {
  if(rx_audio_in == -1 )return;
  snd[RXDA]=snd[RXAD];
  rx_audio_out=rx_audio_in;
  }
else
  {
//*********************************************************************
//********************  OPEN ALSA PLAYBACK DEVICE *********************
//*********************************************************************
  alsa_retries=0;
  if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
    {
// We have to call this first, otherwise linrad would exit after this error:
// (snd_config_hooks_call) Cannot open shared library 
// libasound_module_conf_pulse.so (32 bit xlinrad under 64 bit debian wheezy)
retry_alsa:;
    err=alsa_get_dev_names (ui.rx_dadev_no);
//get plughw_pcm_name corresponding with ui definition
    alsar_get_dev_pcm_names(ui.rx_dadev_no,rx_out_plughw_pcm_name,rx_out_hw_pcm_name );
    err=snd_pcm_open(&alsa_handle[RXDA], rx_out_plughw_pcm_name,
                                                 SND_PCM_STREAM_PLAYBACK, 0);
    if(err < 0)
      {
      lirerr(1304);
      return;
      }
// use rx_audio_out as switch for compatibility reasons with the existing OSS logic
    rx_audio_out=1; //(int)alsa_handle[RXDA];
    }
  else
    {
#if HAVE_OSS == 1
    make_devname(ui.rx_dadev_no, ss);
    if(kill_all_flag)return;
    rx_audio_out=open( ss ,ui.rx_damode|O_EXCL , 0);
#endif
    }
  if(rx_audio_out == -1)
    {
    if(diskread_flag==2)
      {
      lirerr(1191);
      }
    else
      {
      lirerr(1017);
      }
    return;
    }
  if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
    {
//*********************************************************************
//******************** SET PLAYBACK PARAMETERS THE ALSA WAY ******
//*********************************************************************
    err=snd_pcm_hw_params_malloc (&hw_da_params);
    if(err < 0)
      {
      lirerr(1305);
      return;
      }
    err=snd_pcm_hw_params_any(alsa_handle[RXDA], hw_da_params);
    if(err < 0)
      {
      lirerr(1306);
      return;
      }
// Interleaved mode
    err=snd_pcm_hw_params_set_access(alsa_handle[RXDA], hw_da_params,
                                           SND_PCM_ACCESS_RW_INTERLEAVED);
    if(err < 0)
      {
      lirerr(1307);
      return;
      }
// Set sample format
    if(rx_daout_bytes == 1)
      {
      format=SND_PCM_FORMAT_U8;
      }
    else
      {
      format=SND_PCM_FORMAT_S16_LE;
      }
    err=snd_pcm_hw_params_set_format(alsa_handle[RXDA], hw_da_params,format);
    if(err < 0)
      {
      lirerr(1308);
      return;
      }
// Set number of da_channels
    err=snd_pcm_hw_params_set_channels(alsa_handle[RXDA], 
                                       hw_da_params, 
                                       (unsigned int)rx_daout_channels);
    if(err <0 )
      {
      lirerr(1309);
      return;
      }
// Set sample rate
// first disable  hardware resample
    resample=0;
    err=snd_pcm_hw_params_set_rate_resample(
                                   alsa_handle[RXDA],
                                   hw_da_params, 
                                   (unsigned int)resample);
    if(err < 0)
      {
      lirerr(1310);
      return;
      }
// now set sample rate
    val=(unsigned int)genparm[DA_OUTPUT_SPEED];
    err=snd_pcm_hw_params_set_rate_near(alsa_handle[RXDA],hw_da_params, &val, 0);
    if(err < 0)
      {
      lirerr(1311);
      return;
      }
    t1=(float)(val)/(float)genparm[DA_OUTPUT_SPEED];
    if(t1 > 1.05 || t1 < 0.95)
      {
      lirerr(1023);
      return;
      }
    genparm[DA_OUTPUT_SPEED]=(int)val;
    dir=0;
// obtained value is <,=,> target-value, according value of  dir (-1,0,1)
    block_frames=snd[RXDA].block_frames;
    err=snd_pcm_hw_params_set_period_size_near(alsa_handle[RXDA], hw_da_params,
                                    &block_frames, &dir);
    if(err < 0)
      {
      lirerr(1312);
      return;
      }
// check obtained  period size
    err=snd_pcm_hw_params_get_period_size(hw_da_params, &block_frames, &dir);
    if(err < 0)
      {
      DEB"\nopen_rx_sndout:Unable to get period size for playback: %s\n",
                                                       snd_strerror(err));
      lirerr(53841);
      return;
      }
// set the hw parameters
    err=snd_pcm_hw_params(alsa_handle[RXDA], hw_da_params);
    if(err < 0)
      {
      lirerr(1313);
      return;
      }
// free the memory for the hardware parameter struct
    snd_pcm_hw_params_free(hw_da_params);
// get buffersize
    tot_frames=snd[RXDA].tot_frames;
    err=snd_pcm_get_params ( alsa_handle[RXDA], &tot_frames, &block_frames);
    snd[RXDA].tot_frames=tot_frames;
    snd[RXDA].tot_bytes=snd[RXDA].tot_frames*snd[RXDA].framesize;
    snd[RXDA].no_of_blocks=snd[RXDA].tot_frames/snd[RXDA].framesize;
    if(err < 0)
      {
      lirerr(1313);
      return;
      }
    if( (int)block_frames != snd[RXDA].block_frames)
      {
      if( (int)block_frames < snd[RXDA].block_frames)
        {
        while( (int)block_frames < snd[RXDA].block_frames)
          {
          snd[RXDA].interrupt_rate*=2;
          snd[RXDA].block_frames/=2;
          snd[RXDA].block_bytes/=2;
          }
        if(snd[RXDA].block_frames < 8)
          {
          lirerr(28665);
          return;
          }
        }
      else
        {
        while( (int)block_frames > snd[RXDA].block_frames)
          {
          snd[RXDA].interrupt_rate/=2;
          snd[RXDA].block_frames*=2;
          snd[RXDA].block_bytes*=2;
          }
        }
      block_frames=snd[RXDA].block_frames;
      lir_sleep(100000);
      snd_pcm_drop(alsa_handle[RXDA]);
      err= snd_pcm_close(alsa_handle[RXDA]);
      if(err < 0)lirerr(5961006);
      if(alsa_retries == 0)goto retry_alsa;
      }
    goto alsacont;
    }
  else
    {
#if HAVE_OSS == 1
    frag=-1;
    i=snd[RXDA].block_bytes;
    while(i>0)
      {
      i>>=1;
      frag++;
      }
    if(frag < 4)frag=4;
    frag|=0x7fff0000;
    if(ioctl(rx_audio_out,SNDCTL_DSP_SETFRAGMENT,&frag)==-1)
      {
      lirerr(1018);
      return;
      }
    if(rx_daout_bytes == 1)
      {
      i=AFMT_U8;
      }
    else
      {
      i=AFMT_S16_LE;
      }
    j=i;
    if(ioctl(rx_audio_out, SNDCTL_DSP_SETFMT, &i) == -1)
      {
      lirerr(1020);
      return;
      }
    if(i != j)
      {
      lirerr(1021);
      return;
      }
    i=rx_daout_channels;
    if(ioctl(rx_audio_out, SNDCTL_DSP_CHANNELS, &i) == -1)
      {
      lirerr(1019);
      return;
      }
    if(i != rx_daout_channels)
      {
      lirerr(1099);
      return;
      }
    i=genparm[DA_OUTPUT_SPEED];
    if(i > 0)
      {
      if(ioctl(rx_audio_out, SNDCTL_DSP_SPEED, &i)==-1)
        {
        lirerr(1022);
        return;
        }
      t1=(float)(i)/(float)genparm[DA_OUTPUT_SPEED];
      if(t1 > 1.05 || t1 < 0.95)
        {
        lirerr(1023);
        return;
        }
      genparm[DA_OUTPUT_SPEED]=i;
      }
#endif
    }
  }
if( (ui.use_alsa&PORTAUDIO_RX_OUT) != 0)
  {
  return;
  }
#if HAVE_OSS == 1
// We have to write to the output because
// 4Front OSS requires a write before it will return
// correct buffer information with the ioctl call.
tmpbuf=malloc((size_t)snd[RXDA].block_bytes);
if(rx_daout_bytes == 1)
  {
  cc=(char)0x80;
  }
else
  {
  cc=0;
  }
memset(tmpbuf,cc,(size_t)snd[RXDA].block_bytes);
zrw=write(rx_audio_out,tmpbuf,(size_t)snd[RXDA].block_bytes);
zrw=write(rx_audio_out,tmpbuf,(size_t)snd[RXDA].block_bytes);
free(tmpbuf);
if(ioctl(rx_audio_out,SNDCTL_DSP_GETOSPACE, &rx_da_info) == -1)lirerr(856432);
snd[RXDA].tot_bytes=rx_da_info.fragstotal*rx_da_info.fragsize;
snd[RXDA].tot_frames=snd[RXDA].tot_bytes/snd[RXDA].framesize;
if(rx_da_info.fragsize != snd[RXDA].block_bytes)
  {
  snd[RXDA].block_bytes=rx_da_info.fragsize;
  }
snd[RXDA].block_frames=snd[RXDA].block_bytes/snd[RXDA].framesize;
#endif
alsacont:;
snd[RXDA].interrupt_rate=(float)
                     (snd[RXDA].framesize*genparm[DA_OUTPUT_SPEED])/
                      (float)(snd[RXDA].block_bytes);
DEB"  Actual %f  \n",snd[RXDA].interrupt_rate);
snd[RXDA].no_of_blocks=snd[RXDA].tot_bytes/snd[RXDA].block_bytes;
lir_sched_yield();
//lir_sleep(5000000);
}

#define SDI_COL 78

void display_sdi(void)
{
FILE *sdifile;
char *p;
char work_line [80];
int  sdi_screen_line_counter;
char sdi_intro_msg [26]= "SOUND DRIVER INFORMATION:" ;
char sdi_error_msg [46]= "No OSS or ALSA sound-driver information found";
char sndstat_filename [25];
char sndlog_sep [80];
if(screen_last_col < SDI_COL+30)return;
memset (sndlog_sep,'-',sizeof(sndlog_sep));
sndlog_sep[0]='\n';
sndlog_sep[79]='\0';
SNDLOG "%s",sndlog_sep);
SNDLOG"\n%s ", sdi_intro_msg );
settextcolor(YELLOW);
lir_text(SDI_COL, 0, sdi_intro_msg );
sdi_screen_line_counter=1;
// Look for OSS-driver info
strcpy (sndstat_filename,"/dev/sndstat");
sdifile=fopen ( sndstat_filename,"r");
// if OSS not active, try  ALSA
if(sdifile==NULL)
  {
  strcpy (sndstat_filename,"/proc/asound/oss/sndstat");
  sdifile=fopen (sndstat_filename,"r");
  }
if(sdifile!=NULL)
  {
  SNDLOG"\n(output of 'cat %s' command): \n\n",sndstat_filename );
  settextcolor(LIGHT_GREEN);
  while(fgets(work_line, sizeof(work_line)-1, sdifile) != NULL &&
                                      sdi_screen_line_counter < screen_last_line)
    {
// check if work_line is ready for output processing
    p=strchr(work_line, '\n');
    if(p != NULL)p[0]=0;
    if(*work_line != '\0')
      {
// display work_line only if it is not blank
// and there are enough lines available on the screen
// and there are at least 40 characters available on a line
      if(sdi_screen_line_counter < screen_last_line &&
                                               (screen_last_col - 40)>= 80)
        {
        sdi_screen_line_counter++;
        lir_text (SDI_COL, sdi_screen_line_counter, work_line);
        }
// write  to soundboard_init.log
      SNDLOG"%s\n",work_line);
      }
    }
  fclose(sdifile);
  }
else
  {
  settextcolor(LIGHT_RED);
  lir_text( SDI_COL, sdi_screen_line_counter++, sdi_error_msg);
  SNDLOG"\n%s",sdi_error_msg );
  }
SNDLOG"%s",sndlog_sep);
sndlog_sync();
settextcolor(7);
}

#if HAVE_OSS == 1
void ossdev_show(char *ss, int no, int minrate, int maxrate,
                                           int chan, int byte, char *modes)
{
int i, j, k;
sprintf(ss," %02d  %s                 ",no, dev_name);
sprintf(&ss[22],"%d/%d                ",minrate,maxrate);
i=40;
k=1;
while(k < 8)
  {
  if(chan&k)
    {
    sprintf(&ss[i],"%d               ",k);
    i+=2;
    }
  k*=2;
  }
k=1;
i=49;
while(k < 8)
  {
  if(byte&k)
    {
    sprintf(&ss[i],"%d               ",k);
    i+=2;
    }
  k*=2;
  }
if(modes == NULL)
  {
  i=57;
  for(j=0; j<3; j++)
    {
    k=1<<j;
    if( (dev_flag[no]&k) != 0)
      {
      sprintf(&ss[i],"%s ",devmode_txt[j]);
      while(ss[i] != 0)i++;
      }
    }
  }
else
  {
  sprintf(&ss[57],"%s ",modes);
  }
}

void log_output_mode(int no)
{
SNDLOG"\n%s opened as %s ",dev_name, devmode_txt[no]);
sndlog_sync();
}


int select_rdwr_mode(int *line, int no, int mode)
{
int m, k;
char s[80];
if(mode == O_RDONLY)
  {
  k=DEVFLAG_R;
  }
else
  {
  k=DEVFLAG_W;
  }
m=mode;
if( (dev_flag[no]&DEVFLAG_RW) != 0)
  {
  if( (dev_flag[no]&k) != 0)
    {
    line[0]++;
gt_rdo:;
    sprintf(s,"Open %s as RDWR? (Y/N) =>",dev_name);
    lir_text(10,line[0],s);
    await_processed_keyboard();
    if(kill_all_flag)return 0;
    if(lir_inkey == 'Y')
      {
      line[0]++;
      settextcolor(14);
      lir_text(0,line[0],
        "RDWR mode is limited and less reliable.  Are you sure? (Y/N) =>");
      await_processed_keyboard();
      settextcolor(7);
      if(kill_all_flag)return 0;
      if(lir_inkey == 'Y')
        {
        goto rdwr;
        }
      clear_lines(line[0],line[0]);
      line[0]--;
      goto gt_rdo;
      }
    if(lir_inkey != 'N')goto gt_rdo;
    SNDLOG"\nUser selected RDONLY");
    }
  else
    {
rdwr:;
    m=O_RDWR;
    SNDLOG"\nUser selected RDWR");
    }
  line[0]++;
  }
else
  {
  SNDLOG"\nRDWR not available");
  }
return m;
}

void show_oss_presel(int *line, int suggested_dev)
{
char s[80];
clear_lines(0,0);
line[0]++;
make_devname(suggested_dev, dev_name);
if(kill_all_flag)return;
sprintf(s,"%s auto-selected. There is no other alternative.", dev_name);
lir_text(0,line[0],s);
line[0]++;
}

void select_ossdev(int *line, int sound_type)
{
int iohandle;
int caps;
int i, j, k, m, n, nn;
char ss[3*MAX_COLUMN];
int mode, rdwr_fmt, device_no;
int rdwr_min_speed;
int rdwr_max_speed;
audio_buf_info ad_info;
char *rdtest_buffer;
int rdwr_channels, suggested_dev;
int dev_rdwr_channels[MAX_DEVNAMES];
int dev_max_rdwr_speed[MAX_DEVNAMES];
int dev_min_rdwr_speed[MAX_DEVNAMES];
int dev_rdwr_bytes[MAX_DEVNAMES];
int column, maxcolumn, bottom_line;
int devflag_val;
char s[256];
char color[3];
int h_test;
int h_test2;
h_test=-1;
h_test2=-1;
rdtest_buffer=malloc(2048);
if(rdtest_buffer == NULL)
  {
  lirerr(1044);
  return;
  }
n=0;
suggested_dev=-1;
maxcolumn=screen_last_xpixel/text_width-1;
if(maxcolumn > MAX_COLUMN)maxcolumn=MAX_COLUMN;
bottom_line=0;
line[0]++;
sprintf(s,"Dev     device          sample-rate    channels  bytes       modes");
SNDLOG"%s\n",s);
lir_text(0,line[0],s);
line[0]++;
sprintf(s," no      file           min / max");
SNDLOG"%s\n",s);
lir_text(0, line[0],s);
line[0]++;
sprintf(s," --  ---------------  --------------   -------  -------  -------------------" );
SNDLOG"%s\n",s);
lir_text(0,line[0],s);
line[0]++;
for(device_no=0; device_no<MAX_DEVNAMES; device_no++)
  {
  lir_sched_yield();
  dev_flag[device_no]=0;
  make_devname(device_no, dev_name);
  SNDLOG"Checking %s\n",dev_name);
  if(kill_all_flag)goto sel_ossdev_skip;
  if(device_no == (ui.rx_addev_no&255) && ui.rx_admode == O_RDWR)
    {
    if(ui.tx_enable != 0)
      {
      if(ui.rx_dadev_no != device_no && ui.tx_dadev_no != device_no)
        {
        k=2;
        if(  (ui.rx_input_mode&DWORD_INPUT) )k=4;
        ossdev_show(ss, device_no, ui.rx_ad_speed, ui.rx_ad_speed,
                      ui.rx_ad_channels, k,"RDWR used by RXin");
        settextcolor(10);
        line[0]++;
        lir_text(0,line[0],ss);
        lir_refresh_screen();
        dev_flag[device_no]=DEVFLAG_RW;
        n++;
        suggested_dev=device_no;
        }
      else
        { 
        dev_flag[device_no]=0;
        }
      goto used_rdwr;
      }
    if(sound_type == TXAD &&
       device_no == ui.rx_dadev_no &&
       ui.rx_damode == O_RDWR)
      {
      ossdev_show(ss, device_no, ui.rx_min_da_speed, ui.rx_max_da_speed,
                      ui.rx_min_da_channels, 2,"RDWR used by RXout");
      settextcolor(10);
      line[0] ++;
      lir_text(0,line[0],ss);
      lir_refresh_screen();
      dev_flag[device_no]=DEVFLAG_RW;
      goto used_rdwr;
      }
    }  
  sndlog_sync();
  for(mode=0; mode<3; mode++)
    {
    rdwr_max_speed=0;
    rdwr_min_speed=0;
    devflag_val=1<<mode;
    sprintf(s,
    "Trying to open %s %s. Device defective if system hangs here. Read %s   ",
    dev_name, devmode_txt[mode],rx_logfile_name);
    sndlog_sync();
    settextcolor(12);
    lir_text(0,line[0]+1,s);
    lir_refresh_screen();
    settextcolor(7);
    iohandle=open( dev_name, devmodes[mode]|O_NONBLOCK|O_EXCL, 0);
    clear_lines(line[0]+1,line[0]+1);
    if(iohandle != -1)
      {
#ifdef SNDCTL_DSP_COOKEDMODE
      i=0;
      ioctl(iohandle, SNDCTL_DSP_COOKEDMODE, &i);
#endif
      for(nn=0; nn<80; nn++)SNDLOG"-");
      log_output_mode(mode);
      rdwr_fmt=0;
      rdwr_channels=0;
// Check for 16 bits
      if( ioctl(iohandle, SNDCTL_DSP_GETFMTS, &m) == -1)
        {
        SNDLOG"\nioctl(GETFMTS)  failed");
baddev:; 
        sndlog_sync();
        devflag_val=0;
        goto  cls;
        }
      if( (m&AFMT_U8) ==0)
        {
        SNDLOG"\n8bit format NOT supported");
        }
      else
        {
        SNDLOG"\n8bit format supported");
        rdwr_fmt|=1;
        }
      if( (m&AFMT_S16_LE) == 0)
        {
        SNDLOG"\n16bit format NOT supported");
        }
      else
        {
        SNDLOG"\n16bit format supported");
        rdwr_fmt|=2;
        }
#ifdef AFMT_S32_LE
// Check for 32 bits
      if( (m&AFMT_S32_LE) != 0)
        {
        rdwr_fmt|=4;
        devflag_val=1<<mode;
        SNDLOG"\n32bit format supported");
        }
      else
        {
        SNDLOG"\n32bit format NOT supported");
        }
#endif
      sndlog_sync();
      if(rdwr_fmt == 0)
        {
        devflag_val=0;
        goto cls;
        }
// Find max no of channels.
// We never want more than 4 input channels!!
      rdwr_channels=0;
      m=4;
      while(m > 0)
        {
        k=m;
        j=ioctl(iohandle, SNDCTL_DSP_CHANNELS, &k);
        if(j != -1 && k == m)rdwr_channels|=k;
        m/=2;
        }
      if(rdwr_channels == 0)
        { 
        k=0;
        SNDLOG"\nMax no of channels = 0");
        goto baddev;
        }
      else
        {
        SNDLOG"\nChannels = %d",rdwr_channels);
        }
      sndlog_sync();
//
// To allow a separate #ifdef __FreeBSD__
// - Diane Bruce VA3DB, Sept 6, 2006
//
#ifdef __linux__
// Find maximum speed
      rdwr_max_speed=ABOVE_MAX_SPEED;
      if(ioctl(iohandle, SNDCTL_DSP_SPEED, &rdwr_max_speed)==-1)
        {
        SNDLOG"\nioctl(SPEED)  failed (max)");
        goto baddev;
        } 
      SNDLOG"\nMax speed %d Hz",rdwr_max_speed);
// Find minimum speed
      i=0; 
      while(i<MAX_LOWSPEED)
        {
        rdwr_min_speed=low_speeds[i];
        if(ioctl(iohandle, SNDCTL_DSP_SPEED, &rdwr_min_speed)!=-1)
                                                                  goto minok;
        i++; 
        }
      lirerr(1040);
      goto sel_ossdev_skip;
minok:;
#endif
//
// This code finds min max speed of sound card for FreeBSD
// It probably works for other BSDs as well
// Diane Bruce, db@db.net VA3DB
// Sept 7, 2006
//
#ifdef __FreeBSD__
        {
        snd_capabilities capab;
        int status;
        bzero((void *)&capab, sizeof(capab));
        status=ioctl(iohandle, AIOGCAP, &capab);
        if(status < 0)goto sel_ossdev_skip;
        rdwr_min_speed=capab.rate_min;
        rdwr_max_speed=capab.rate_max;
        SNDLOG"\nMax sampling speed %d", rdwr_max_speed);
        }
#endif
      SNDLOG"\nMin sampling speed = %d Hz",rdwr_min_speed);
      sndlog_sync();
      if(sound_type == RXAD)
        {
//devmode_txt[3]={"RDONLY","WRONLY","RDWR"};
        if(mode != DEVMODE_WRONLY)
          {
          if(ioctl(iohandle, SNDCTL_DSP_GETISPACE, &ad_info) == -1)
            {
            SNDLOG"\nioctl(GETISPACE)  failed");
            goto baddev;
            }
// If it is  opened RDWR, check we have a chance to write while reading
// Some devices allow write or read in rdwr mode - but not simultaneously.
          if(mode == DEVMODE_RDWR)
            {
            zrw=read(iohandle, rdtest_buffer, 128);
            if(ioctl(iohandle,SNDCTL_DSP_GETOSPACE, &ad_info) == -1)
               {
              SNDLOG"\nioctl(GETOSPACE)  failed while reading");
              goto baddev;
              }
            }
          }
        }
cls:;
      if(close(iohandle)==-1)
        { 
        sndlog_sync();
        lirerr(1037);
        goto sel_ossdev_skip;
        }
      if(devflag_val != 0)
        {
        if(sound_type == RXAD || sound_type == TXAD)
          {
          if(devflag_val != 2)
            {
            suggested_dev=device_no;
            n++;
            }
          }
        if( sound_type == RXDA || sound_type == TXDA)
          {
          if(devflag_val != 1)
            {
            suggested_dev=device_no;
            n++;
            }
          }
        dev_flag[device_no]|=devflag_val;
        switch (mode)
          {
          case DEVMODE_RDONLY:
          dev_rd_channels[device_no]=rdwr_channels;
          dev_max_rd_speed[device_no]=rdwr_max_speed;
          dev_min_rd_speed[device_no]=rdwr_min_speed;
          dev_rd_bytes[device_no]=rdwr_fmt;
          break;

           case DEVMODE_WRONLY:
          dev_wr_channels[device_no]=rdwr_channels;
          dev_max_wr_speed[device_no]=rdwr_max_speed;
          dev_min_wr_speed[device_no]=rdwr_min_speed;
          dev_wr_bytes[device_no]=rdwr_fmt;
          break;

          case DEVMODE_RDWR:
          dev_rdwr_channels[device_no]=rdwr_channels;
           dev_max_rdwr_speed[device_no]=rdwr_max_speed;
          dev_min_rdwr_speed[device_no]=rdwr_min_speed;
          dev_rdwr_bytes[device_no]=rdwr_fmt;
          break;
          }
        }
      SNDLOG"\n");
      }
    }
  if(dev_flag[device_no] != 0)
    {
    SNDLOG"\nFLAG= %d",dev_flag[device_no]);
// One device may be different hardwares for input and output.
// Put them on different lines if characteristics differ.
    k=0;
    j=0;
// Store equal pairs in k
// bit 0=1 => READ==WRITE
// bit 1=1 => READ==RDWR
// bit 2=1 => WRITE==RDWR
    if( (dev_flag[device_no]&(DEVFLAG_R+DEVFLAG_W)) == DEVFLAG_R+DEVFLAG_W)
      {
      j|=1;
      if(dev_rd_channels[device_no]==dev_wr_channels[device_no] &&
         dev_max_rd_speed[device_no]==dev_max_wr_speed[device_no] &&
         dev_min_rd_speed[device_no]==dev_min_wr_speed[device_no] &&
         dev_rd_bytes[device_no]==dev_wr_bytes[device_no]) k|=1;
      } 
    if( (dev_flag[device_no]&(DEVFLAG_R+DEVFLAG_RW)) == DEVFLAG_R+DEVFLAG_RW)
      {
      j|=2;
      if(dev_rd_channels[device_no]==dev_rdwr_channels[device_no] &&
         dev_max_rd_speed[device_no]==dev_max_rdwr_speed[device_no] &&
         dev_min_rd_speed[device_no]==dev_min_rdwr_speed[device_no] &&
         dev_rd_bytes[device_no]==dev_rdwr_bytes[device_no])k|=2;
      }
    if( (dev_flag[device_no]&(DEVFLAG_W+DEVFLAG_RW)) == DEVFLAG_W+DEVFLAG_RW)
      {
      j|=4;
      if(dev_wr_channels[device_no]==dev_rdwr_channels[device_no] &&
         dev_max_wr_speed[device_no]==dev_max_rdwr_speed[device_no] &&
         dev_min_wr_speed[device_no]==dev_min_rdwr_speed[device_no] &&
         dev_wr_bytes[device_no]==dev_rdwr_bytes[device_no])k|=4;
      }
    SNDLOG"\nj=%d   k=%d",j,k);
    if(j == k)
      {
// There are no differences between devices. Put everything on one line.
      m=1;
      if( (dev_flag[device_no]&DEVFLAG_R) == DEVFLAG_R)
        { 
        ossdev_show(ss, device_no,
                        dev_min_rd_speed[device_no],
                        dev_max_rd_speed[device_no],
                        dev_rd_channels[device_no],
                        dev_rd_bytes[device_no],NULL);
        }
      else
        {
        if( (dev_flag[device_no]&DEVFLAG_W) == DEVFLAG_W)
          {
          ossdev_show(ss, device_no,
                         dev_min_wr_speed[device_no],
                         dev_max_wr_speed[device_no],
                         dev_wr_channels[device_no],
                         dev_wr_bytes[device_no],NULL);
          }
        }
      if(sound_type == RXAD || sound_type == TXAD)
        {
        if( (dev_flag[device_no]&(DEVFLAG_R+DEVFLAG_RW)) == 0)
          {
          color[0]=0x17;
          }
        else
          {
          color[0]=0x1c;
          }
        }
      else
        {
        if( (dev_flag[device_no]&(DEVFLAG_W+DEVFLAG_RW)) == 0)
          {
          color[0]=0x17;
          }
        else
          {
          color[0]=0x1c;
          }
        }
      }
    else
      { 
// There are differences.
// We can not arrive here unless there are at least two modes
// and consequently j can not be zero.
      if( (j&1) == 0)
        {
// We do not have both read and write so it is one of them plus rdwr.
// This does not seem right, but we present the info we get to the user.
        color[0]=0x1c;
        m=2;
        ossdev_show(ss, device_no,
                        dev_min_rdwr_speed[device_no],
                        dev_max_rdwr_speed[device_no],
                        dev_rdwr_channels[device_no],
                        dev_rdwr_bytes[device_no],devmode_txt[2]);
        if( (dev_flag[device_no]&DEVFLAG_R) == 0)
          {
          if(sound_type == RXAD || sound_type == TXAD)
            {
            color[1]=0x17;
            }
          else
            {
            color[1]=0x1c;
            }
          ossdev_show(&ss[maxcolumn], device_no,
                          dev_min_wr_speed[device_no],
                          dev_max_wr_speed[device_no],
                          dev_wr_channels[device_no],
                          dev_wr_bytes[device_no],devmode_txt[1]);
          }
        else
          {
          if(sound_type == RXAD || sound_type == TXAD)
            {
            color[1]=0x1c;
            }
          else
            {
            color[1]=0x17;
            }
          ossdev_show(&ss[maxcolumn], device_no,
                          dev_min_rd_speed[device_no],
                          dev_max_rd_speed[device_no],
                          dev_rd_channels[device_no],
                          dev_rd_bytes[device_no],devmode_txt[0]);
          }
        }
      else
        {
// There is both read and write.
        if( (dev_flag[device_no]&DEVFLAG_RW) == 0)
          {
// but there is no rdwr
          m=2;
          if(sound_type == RXAD || sound_type == TXAD)
            {
            color[1]=0x17;
            color[0]=0x1c;
            }
          else
            {
            color[1]=0x1c;
            color[0]=0x17;
            }
          ossdev_show(&ss[maxcolumn], device_no,
                          dev_min_wr_speed[device_no],
                          dev_max_wr_speed[device_no],
                          dev_wr_channels[device_no],
                          dev_wr_bytes[device_no],devmode_txt[1]);
          ossdev_show(ss, device_no,
                          dev_min_rd_speed[device_no],
                          dev_max_rd_speed[device_no],
                          dev_rd_channels[device_no],
                          dev_rd_bytes[device_no],devmode_txt[0]);
          }
        else
          {
// There is rdwr, read and write.
          if(k == 0)
            {
// They are  all different
            m=3;
            if(sound_type == RXAD || sound_type == TXAD)
              {
              color[2]=0x17;
              color[1]=0x1c;
              }
            else
              {
              color[2]=0x1c;
              color[1]=0x17;
              }
            ossdev_show(&ss[2*maxcolumn], device_no,
                          dev_min_wr_speed[device_no],
                          dev_max_wr_speed[device_no],
                           dev_wr_channels[device_no],
                          dev_wr_bytes[device_no],devmode_txt[1]);
            ossdev_show(&ss[maxcolumn], device_no,
                          dev_min_rd_speed[device_no],
                          dev_max_rd_speed[device_no],
                          dev_rd_channels[device_no],
                          dev_rd_bytes[device_no],devmode_txt[0]);
            color[0]=0x1c;
            ossdev_show(ss, device_no,
                          dev_min_rdwr_speed[device_no],
                          dev_max_rdwr_speed[device_no],
                          dev_rdwr_channels[device_no],
                          dev_rdwr_bytes[device_no],devmode_txt[2]);
            }
          else
            {
// rdwr equals read or write.
            m=2;
            if( (k&4) == 0)
              {
            if(sound_type == RXAD || sound_type == TXAD)
              {
              color[1]=0x17;
              color[0]=0x1c;
              }
            else
              {
              color[1]=0x1c;
              color[0]=0x1c;
              }
              ossdev_show(&ss[maxcolumn], device_no,
                           dev_min_wr_speed[device_no],
                           dev_max_wr_speed[device_no],
                           dev_wr_channels[device_no],
                           dev_wr_bytes[device_no],devmode_txt[1]);
              ossdev_show(ss, device_no,
                           dev_min_rd_speed[device_no],
                           dev_max_rd_speed[device_no],
                           dev_rd_channels[device_no],
                           dev_rd_bytes[device_no],
                           devmode_txt[0]);
              strcat(ss,devmode_txt[2]);
              }
             else
              {
              if(sound_type == RXAD || sound_type == TXAD)
                {
                color[1]=0x17;
                color[0]=0x1c;
                }
              else
                {
                color[1]=0x1c;
                color[0]=0x1c;
                }
              ossdev_show(&ss[maxcolumn], device_no,
                      dev_min_wr_speed[device_no],
                      dev_max_wr_speed[device_no],
                      dev_wr_channels[device_no], dev_wr_bytes[device_no],
                      devmode_txt[1]);
              strcat(ss,devmode_txt[2]);
               ossdev_show(ss, device_no,
                      dev_min_rd_speed[device_no],
                      dev_max_rd_speed[device_no],
                      dev_rd_channels[device_no],
                      dev_rd_bytes[device_no],
                      devmode_txt[0]);
              }
            }
          }
        }
      }
    while (m >0)
      { 
      m--;
      line[0]++;
      if(line[0] >= screen_last_line-1)
        {
        for(column=0; column<maxcolumn; column++)s[column]=' ';
        s[maxcolumn]=0;
        lir_text(0,line[0],s);
        settextcolor(14);
        lir_text(10,line[0],"(press ENTER to continue)");
        settextcolor(7);
        await_keyboard();
        if(kill_all_flag) goto sel_ossdev_skip;
        lir_text(0,line[0],s);
        bottom_line=screen_last_line-1;
        line[0]=1;
        }
      column=0;
      while(ss[m*maxcolumn+column]!=0)column++;
      while(column < maxcolumn)
        { 
        ss[m*maxcolumn+column]=' ';
        column++;
        }
      ss[(m+1)*maxcolumn-1]=0;
      settextcolor((unsigned char)color[m]);
      lir_text(0,line[0],&ss[m*maxcolumn]);
      lir_refresh_screen();
      SNDLOG"\n%s    color=%d m=%d",&ss[m*maxcolumn],color[m],m);
      }
    SNDLOG"\n");
    }
used_rdwr: ;
  }
settextcolor(7);
line[0]++;
clear_lines(line[0],line[0]);
if(bottom_line > line[0])line[0]=bottom_line;
display_sdi();
if(sound_type == TXDA)
  {
  if(n == 0)
    {
    ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
    lir_text(0,line[0],"No device found for Tx output");
    lir_text(2,line[0]+1,"ERROR (press any key)");
    await_keyboard();
    goto sel_ossdev_skip;
    }
  line[0]++;
  lir_text(10,line[0],"=>");
get_tx_dadev:;
  ui.tx_dadev_no=lir_get_integer(13, line[0], 3, 0,MAX_DEVNAMES-1);
  if(kill_all_flag) goto sel_ossdev_skip;
  if( (dev_flag[ui.tx_dadev_no]&(DEVFLAG_W+DEVFLAG_RW)) == 0)
    {
    settextcolor(15);
    lir_text(29,line[0],"???");
    lir_text(25,line[0]+1,"ERROR (press any key)");
    await_keyboard();
    if(kill_all_flag) goto sel_ossdev_skip;
    clear_lines(line[0]+1,line[0]+1);
    goto  get_tx_dadev;
    }
  line[0]+=2;
  }
if(sound_type == RXDA)
  {
  if(n  == 0)
    {
    if(soundcard_test_cmd_flag[RXAD] != THRFLAG_ACTIVE)
      {
      line[0]++;
      lir_text(0,line[0],"No usable device found");
      goto sel_ossdev_skip;
      }
    if(line[0] > screen_last_line-6)line[0]=screen_last_line-6;
    lir_text(0,line[0],
           "No device found for 1/2 byte output while input is open.");
    lir_text(0,line[0]+1,
            "You may use Linrad to watch spectra and record raw data.");
    lir_text(0,line[0]+2,
           "Linrad will now look for devices that can be used to");
    lir_text(0,line[0]+3,
           "output sound while processing recorded data.");
    lir_text(0,line[0]+4,press_any_key);
    SNDLOG"\nNo output device found while input is open.");
    await_keyboard();
    clear_lines(line[0],line[0]+4);
    goto sel_ossdev_skip;
    }
  if(n == 1)
    { 
    ui.rx_dadev_no=suggested_dev;
    show_oss_presel(line,suggested_dev);
    }
  if(n > 1)
    {
    line[0]++;
    lir_text(10,line[0],"=>");
get_rx_dadev:;
    display_sdi() ;
    ui.rx_dadev_no=lir_get_integer(13, line[0], 3, 0,MAX_DEVNAMES-1);
    if(kill_all_flag)goto sel_ossdev_skip;
    if( (dev_flag[ui.rx_dadev_no]&(DEVFLAG_W+DEVFLAG_RW)) == 0)
      {
      settextcolor(15);
      lir_text(51,line[0],"???");
      lir_text(35,line[0]+1,"ERROR (press any key)");
      await_keyboard();
      if(kill_all_flag) goto sel_ossdev_skip;
      clear_lines(line[0],line[0]+1);
      goto get_rx_dadev;
      }
    }
  goto sel_ossdev_skip;
  }
if(sound_type == TXAD)
  {
  if(n == 0)
    {
    lir_text(0,line[0],"No device found for microphone input.");
    line[0]++;
    lir_text(2,line[0]+1,"ERROR (press any key)");
    await_keyboard();
    goto sel_ossdev_skip;
    }
  line[0]++;
  lir_text(10,line[0],"=>");
  get_micdev:;
  ui.tx_addev_no=lir_get_integer(13, line[0], 3, 0,MAX_DEVNAMES-1);
  if( (dev_flag[ui.tx_addev_no]&(DEVFLAG_R+DEVFLAG_RW)) == 0)
    {
    settextcolor(15);
    lir_text(37,line[0],"???");
    lir_text(25,line[0]+1,"ERROR (press any key)");
    await_keyboard();
    if(kill_all_flag)goto sel_ossdev_skip;
    clear_lines(line[0]+1,line[0]+1);
    goto get_micdev;
    }
  goto sel_ossdev_skip;
  }
if(sound_type  == RXAD)
  {
  if(n == 0)
    {
    if(line[0] > screen_last_line-5)
      {
      line[0]=screen_last_line-5;
      clear_lines(line[0], screen_last_line);
      }
    lir_text(0,line[0],"No OSS sound device found (for 2/4 byte input).");
    line[0]++;
    line[0]+=2;
    lir_text(0,line[0],press_f1_for_info);    
    line[0]++;
    lir_text(20,line[0],press_any_key);
    await_processed_keyboard();
    if(lir_inkey == F1_KEY || lir_inkey == '!')
      {
      help_message(332);
      }
    SNDLOG"\nNo input device found");
    ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
    goto sel_ossdev_skip;
    }
  if(n == 1)
    {
    ui.rx_addev_no=suggested_dev;
    show_oss_presel(line,suggested_dev);
    }
  if(n > 1) 
    {
    line[0]++;
    lir_text(10,line[0],"=>");
get_rx_addev:;
    display_sdi();
    ui.rx_addev_no=lir_get_integer(13, line[0], 3, 0,MAX_DEVNAMES-1);
    if(kill_all_flag)goto sel_ossdev_skip;
    if( (dev_flag[ui.rx_addev_no]&(DEVFLAG_R+DEVFLAG_RW)) == 0)
      {
      settextcolor(15);
      lir_text(51,line[0],"???");
      lir_text(35 ,line[0]+1,"ERROR (press any key)");
      await_keyboard();
      if(kill_all_flag) goto sel_ossdev_skip;
      clear_lines(line[0],line[0]+1);
      goto get_rx_addev;
      }
    }
  make_devname(ui.rx_addev_no, dev_name);
  if(kill_all_flag)goto sel_ossdev_skip;
  SNDLOG"\n\n %s selected by user for input.",dev_name);
  SNDLOG"   dev_flag=%d",dev_flag[ui.rx_addev_no]);
  sndlog_sync();
// High end users might need two devices to read four A/D channels.
// This is an option introduced April 2004 to allow two receive
// channels with the Lynx Two card (model A).
// Do not confus e other users by asking for a second input device
// unless the selected device has 32 bits but only allows 2 channels.
  if(dev_rd_bytes[ui.rx_addev_no] >= 4 &&
     dev_rd_channels[ui.rx_addev_no]==2 &&
     (dev_flag[ui.rx_addev_no]&DEVFLAG_R) != 0)
    {
    SNDLOG"\n\n User selected 4 byte device with max two channels.");
    j=0;
    clear_lines(1,line[0]);
    line[0]=1;
    for(i=0; i<MAX_DEVNAMES; i++)
      {
      if(i != ui.rx_addev_no &&
         dev_max_rd_speed[i] == dev_max_rd_speed[ui.rx_addev_no] &&
         dev_rd_bytes[i] >= 4 &&
         (dev_flag[i]&DEVFLAG_R) != 0)
        {
// This device can also read 32 bits at the same speed.
// Check if it  can be opened simultaneously in read mode.
        if(j==0)
          {
          line[0]++;
          lir_text(5,line[0],"This device allows two channels only.");
          line[0]++;
          lir_text(5,line[0],"Do you want to use a second device to run two");
          line[0]++;
          lir_text(5,line[0],"receive channels with four audio channels ?");
          line[0]++;
          lir_text(5,line[0],"(Y/N)=>");
          line[0]++;
gt_dual:;
          await_processed_keyboard();
          line[0]+=2;
          if( kill_all_flag) goto sel_ossdev_skip;
          if(lir_inkey == 'N')
            {
            SNDLOG"\nUser skipped dual devices");
            goto skip_dual_devices;
            }
          if(lir_inkey != 'Y')goto gt_dual;
          SNDLOG"\nUser wants to try to open two read devices");
          make_devname(ui.rx_addev_no, dev_name);
          if(kill_all_flag)goto sel_ossdev_skip;
          h_test=open( dev_name, O_RDONLY|O_EXCL, 0);
          if(h_test == -1)
            {
            lirerr(1102);
            goto sel_ossdev_skip;
            }
#ifdef SNDCTL_DSP_COOKEDMODE
          k=0;
          ioctl(h_test, SNDCTL_DSP_COOKEDMODE, &k);
#endif
// Check for DSP_CAP_TRIGGER capability
          if(ioctl(h_test, SNDCTL_DSP_GETCAPS,&caps) == -1)
            {
            lirerr(1182);
            goto sel_ossdev_skip;
            }
          if( (caps&DSP_CAP_TRIGGER) == 0)
            {
            SNDLOG"\nDevice does not have DSP_CAP_TRIGGER");
            close(h_test);
             goto skip_dual_devices;
            }
          j=2;
          if(ioctl(h_test, SNDCTL_DSP_CHANNELS, &j) == -1)
            {
            lirerr(1185);
            goto sel_ossdev_skip;
            }
          if(j != 2)
            {
            lirerr(1186);
            goto sel_ossdev_skip;
            }
          }
        make_devname(i, dev_name);
        if(kill_all_flag)goto sel_ossdev_skip;
        h_test2=open( dev_name, O_RDONLY|O_NONBLOCK|O_EXCL, 0);
        if(h_test2 != -1)
           {
#ifdef SNDCTL_DSP_COOKEDMODE
          k=0;
          ioctl(h_test2, SNDCTL_DSP_COOKEDMODE, &k);
#endif
          line[0]++;
          SNDLOG"\n%s opened as second read device",dev_name);
          sprintf(s,"%2d:%12s  %7d - %7d Hz   %d Chan.   %d bit",
                    i,dev_name,
                    dev_min_rd_speed[i],
                    dev_max_rd_speed[i],
                    dev_rd_channels[i],
                     dev_rd_bytes[i]);
          lir_text(0,line[0],s);
          close(h_test2);
          dev_flag[i]|=256;
          }
        }
      }
    if(j == 0)goto skip_dual_devices;
    settextcolor(14);
    lir_text(0,line[0]+1,"Select second input device by Dev no.");
get_addev2:;
    settextcolor(7);
    i=lir_get_integer(51, line[0]+1, 3, 0,MAX_DEVNAMES-1);
    if(kill_all_flag)goto sel_ossdev_skip;
    if( (256 & dev_flag[i]) == 0)
      { 
      settextcolor(15);
      lir_text(40,line[0],"???");
      lir_text(35,line[0]+1,"ERROR (press any key)");
      await_keyboard();
      if(kill_all_flag) goto sel_ossdev_skip;
      clear_lines(line[0]+1,line[0]+1);
      settextcolor(7);
      goto get_addev2;
      }
// We store the second A/D device in the second byte of ad_device_no
    make_devname(i, dev_name);
    if(kill_all_flag)goto sel_ossdev_skip;
    ui.rx_addev_no+=256*(i+1);
    dev_flag[i]&=255;
    h_test2=open( dev_name, O_RDONLY|O_NONBLOCK|O_EXCL, 0);
    if(h_test2 == -1)
      {
      lirerr(1172);
      goto sel_ossdev_skip;
      }
#ifdef SNDCTL_DSP_COOKEDMODE
    k=0;
    ioctl(h_test2, SNDCTL_DSP_COOKEDMODE, &k);
#endif
// Check for DSP_CAP_TRIGGER capability
    if(ioctl(h_test2, SNDCTL_DSP_GETCAPS,&caps) == -1)
      {
      lirerr(1182);
      goto sel_ossdev_skip;
       }
    if( (caps&DSP_CAP_TRIGGER) == 0)
      {
      SNDLOG"\nSecond input device does not have DSP_CAP_TRIGGER");
      close(h_test);
      close(h_test2);
      goto skip_dual_devices;
      }
    SNDLOG"\n %s is second input device.",dev_name);
    ui.rx_input_mode=TWO_CHANNELS+IQ_DATA;
    ui.rx_rf_channels=2;
    ui.rx_ad_channels=4;
    clear_screen();
    goto continue_dual;
skip_dual_devices:;
    clear_screen();
    }
// Now that we decided what device to use for input, open it and let
// the user decide how to use it.
  if(ui.rx_addev_no != UNDEFINED_DEVICE_CODE)
    {
    make_devname(ui.rx_addev_no, dev_name);
    if(kill_all_flag)goto sel_ossdev_skip;
    sndlog_sync();
    line[0]++;
    ui.rx_admode=select_rdwr_mode(line, ui.rx_addev_no, O_RDONLY);
    if(kill_all_flag) goto sel_ossdev_skip;
    sndlog_sync();
    }
  clear_screen();
  line[0]=0;
  mode=0;
  if(ui.rx_admode == O_RDWR)mode=2;
  sprintf(s,"For analog input: %s opened in %s mode",
                                               dev_name,devmode_txt[mode]);
  lir_text(0,line[0],s);
  line[0]+=2;
  rx_audio_in=open( dev_name, ui.rx_admode|O_EXCL, 0);
  if(rx_audio_in == -1)
     {
    lirerr(1038);
    goto sel_ossdev_skip;
    }
continue_dual:;
  clear_lines(10,11+j);
#if HAVE_OSS == 1
  sprintf(s,"Use ossmix");
#else
  sprintf(s,"Use system mixer program");
#endif
  column=0;
  while(s[column] != 0)column++;
  sprintf(&s[column]," to select input and");
  lir_text(0,line[0],s);
  line[0]++;
  lir_text(0,line[0],"to disable direct connection from input to output etc.");
  line[0]+=2;
   lir_text(0,line[0],"In case selection of sampling frequencies is not what you");
  line[0]++;
  lir_text(0,line[0],
        "expect, check README files for your board - there may be ways");
  line[0]++;
  lir_text(0,line[0],"to reconfigure the hardware. (Under OSS, run ossmix)");
  }
sel_ossdev_skip:;
free(rdtest_buffer);
line[0]+=2;
}
#endif


void set_rx_dadev_parms(int *line)
{
if(ui.rx_dadev_no == UNDEFINED_DEVICE_CODE)return;
ui.rx_min_da_channels=1;
ui.rx_max_da_channels=2;
if( (dev_wr_channels[ui.rx_dadev_no]&3) == 0)
   {
  lir_text(0,line[0],
            "Error: Device must have 1 or 2 channels (or both).");
  line[0]++;
  lir_text(0,line[0],wish_anyway);
anyway:;
  await_processed_keyboard();
  if(lir_inkey=='N')
    {
    ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
    return;
    }
  if(lir_inkey != 'Y')goto anyway;
  }
else
  {
  if( (dev_wr_channels[ui.rx_dadev_no]&1) == 0)ui.rx_min_da_channels=2;
  if( (dev_wr_channels[ui.rx_dadev_no]&2) == 0)ui.rx_max_da_channels=1;
  }
ui.rx_min_da_speed=dev_min_wr_speed[ui.rx_dadev_no];
ui.rx_max_da_speed=dev_max_wr_speed[ui.rx_dadev_no];
ui.rx_min_da_bytes=1;
if( (dev_wr_bytes[ui.rx_dadev_no]&1) == 0)ui.rx_min_da_bytes=2;
ui.rx_max_da_bytes=ui.rx_min_da_bytes;
if( (dev_wr_bytes[ui.rx_dadev_no]&2) != 0)ui.rx_max_da_bytes=2;
}

void stop_iotest(int mode)
{
soundcard_test_cmd_flag[mode]=THRFLAG_KILL;
pthread_join(thread_identifier_soundcard_test[mode],0);
}

int start_iotest(int *line, int mode)
{
char *mode_text[3]={"RX INPUT", "RX OUTPUT", "TX OUTPUT"};
char s[80];
int i;
// To make sure that the devices we select for tx or for rx output can
// be used while the rx input is running we start a separate thread that
// uses the rx input so we will fail on devices that do not
// allow simultaneous read and write.
soundcard_test_block_count[mode]=-1;
soundcard_test_cmd_flag[mode]=THRFLAG_ACTIVE;
pthread_create(&thread_identifier_soundcard_test[mode],
                                   NULL,(void*)thread_iotest[mode], NULL);
i=0;
while(i < 200 && soundcard_test_block_count[mode] == -1)
  {
  lir_sleep(10000);
  if(kill_all_flag)return FALSE;
  i++;
  }
if(i >= 200)
  {
  sprintf(s,"COULD NOT START THE %s",mode_text[mode]);
  goto msg;
  }
i=0;
while(i < 100 && soundcard_test_block_count[mode] < 8)
  {
  lir_sleep(10000);
  if(kill_all_flag)return FALSE;
  i++;
  }
if(i >= 100)
  {
  sprintf(s,"FAILED TO RUN THE %s",mode_text[mode]);
msg:;
  line[0]+=2;
  settextcolor(12);
  lir_text(0,line[0],s);
  line[0]++;
  if(mode == RXAD)
    {
    lir_text(0,line[0],"Set a working input or force no soundcard by");
    line[0]++;
    lir_text(0,line[0],"selecting network input, then soundcard select.");
    line[0]++;
    }
  settextcolor(7);
  lir_text(0,line[0],press_any_key);
  await_keyboard();
  line[0]+=2;
  return FALSE;
  }
settextcolor(7);
return TRUE;
}

void select_sound_system(void)
{
clear_screen();
lir_text(5,5,"Select sound system");
lir_text(5,7,"0=ALSA");
lir_text(5,8,"1=OSS");
getsys:;
await_keyboard();
if(kill_all_flag)return;
if(lir_inkey == '0')
  {
  ui.use_alsa|=NATIVE_ALSA_USED;
  }
else
  {  
  if(lir_inkey == '1')
    {
    ui.use_alsa&=~NATIVE_ALSA_USED;
    }
  else
    {
    goto getsys;
    }  
  }
ui.use_alsa|=SOUND_SYSTEM_DEFINED;
}

void set_rx_io(void)
{
float t1;
char *tmptxt;
char *tmptxt1;
int dev_max_channels;
int dev_min_channels;
int i, j, k;
int n1, n2, m1, m2;
int latency;
#if HAVE_OSS == 1 
int speed_warning;
int min, max;
int trySpeed, deltaSpeed;
int rdwr_max_speed;
int column;
#endif
int line, refline;
char s[1024];
char s1[2048];
char *ss;
unsigned int new_sample_rate;
int err;
char pa_device_name[128];
char pa_device_hostapi  [128];
double pa_device_max_speed;
double pa_device_min_speed;
int pa_device_max_bytes;
int pa_device_min_bytes;
int pa_device_max_channels;
int pa_device_min_channels;
dev_max_channels=1;
dev_min_channels=1;
refline=0;
sndlog=fopen(rx_logfile_name, "w");
if(sndlog == NULL)
  {
  lirerr(1016);
  return;
  }
line=3;  
#if DARWIN == 1
ui.use_alsa|=SOUND_SYSTEM_DEFINED;
#endif
if(ui.use_alsa & 
  (PORTAUDIO_RX_IN+PORTAUDIO_RX_OUT+PORTAUDIO_TX_IN+PORTAUDIO_TX_OUT))
  {
  i=1;
  if(portaudio_active_flag==FALSE)i=portaudio_start(200);
  if(kill_all_flag)goto set_rx_io_errexit;
  if(!i)
    {
    clear_screen();
    lir_text(5,5,"Portaudio is selected but can not start.");
    lir_text(5,6,"Maybe a missing dll file?");
    lir_text(5,7,"Try to re-select RX input or press ESC");
    lir_text(10,10,press_any_key);
    await_processed_keyboard();
    if(kill_all_flag)goto set_rx_io_errexit;
    ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
    ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
    ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
    ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
    ui.use_alsa&=~(PORTAUDIO_RX_IN+PORTAUDIO_RX_OUT+
                                        PORTAUDIO_TX_IN+PORTAUDIO_TX_OUT);
    ui.use_alsa&=~SOUND_SYSTEM_DEFINED;
    }
  }    

if((ui.use_alsa & SOUND_SYSTEM_DEFINED) ==0)select_sound_system();
if(kill_all_flag)goto set_rx_io_errexit;
write_log=TRUE;
if(ui.rx_addev_no != DISABLED_DEVICE_CODE)
  {
  if(ui.rx_ad_channels == 0 && ui.rx_dadev_no != DISABLED_DEVICE_CODE)
    {
    ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
    }
  if(ui.rx_max_da_channels == 0)
    {
    ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
    }
  }  
begin_rx_set_io:;
clear_screen();
settextcolor(14);
line=0;
lir_text(2,line,"CURRENT A/D and D/A SETUP FOR RX");
if(ui.use_extio != 0)
  {
  get_extio_name(&s[0]);
  lir_text(35,line,"using");
  lir_text(41,line,s);
  if(ui.extio_type == 4)
    {
    lir_text((int)(42+strlen(s)),line,"with soundcard input.");
    }
  else
    {
    if(ui.extio_type >= 3 && ui.extio_type <= 7)
      {
      goto display_rx_out;
      }
    else
      {
      lirerr(1237);
      goto set_rx_io_errexit;
      }  
    }        
  }  
line+=2;
settextcolor(7);
// ******************************************
// Start by writing what sound system we use 
// ******************************************
lir_text(2,line,"The current sound system is:");
settextcolor(10);
if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
  {
  lir_text(31,line,"native ALSA");
  SNDLOG"\nsound system = native ALSA ");
  settextcolor(7);
  load_alsa_library();
  if(!alsa_library_flag)goto io_menu;
  }
else
  {
#if HAVE_OSS == 1
#if OSSD == 1
  lir_text(31,line,"alsa-oss");
  SNDLOG"\nsound system = alsa-oss emulation ");
#else
  lir_text(31,line,"4Front OSS");
  SNDLOG"\nsound system = 4Front OSS ");
#endif  
#else
  lir_text(31,line,"OSS not available");
  SNDLOG"\nOSS not available"); 
#endif
  unload_alsa_library();
  }
line+=2;
settextcolor(7);
if( show_rx_input_settings(&line) )goto display_rx_out;
// check for input soundcards
if(ui.rx_addev_no == UNDEFINED_DEVICE_CODE)
  {
  settextcolor(12);
  sprintf(s,"NOT YET SELECTED: (Select Menu Option 'A')");
  lir_text(24,line,s);
  SNDLOG"\n%s",s);
  goto display_rx_out;
  }
latency=0;
if ( (ui.use_alsa&PORTAUDIO_RX_IN) != 0)
  {
  if(portaudio_active_flag==FALSE)portaudio_start(100);
  err=pa_get_device_info (ui.rx_addev_no,
                          RXAD,
                          &pa_device_name,
                          &pa_device_hostapi,
			  &pa_device_max_speed,
			  &pa_device_min_speed,
			  &pa_device_max_bytes,
			  &pa_device_min_bytes,
			  &pa_device_max_channels,
			  &pa_device_min_channels );
  if (err >= 0)
    {
    sprintf(s,"SOUNDCARD device   = %s ", pa_device_name);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    line+=1;
    sprintf(s,"device number      = %d", ui.rx_addev_no);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    line+=1;
    sprintf(s,"hostapi            = Portaudio (%s)", pa_device_hostapi);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    latency=ui.rx_ad_latency;
    }
  else
    {
    sprintf (s,"PORTAUDIO device number %d : INVALID DEVICE ",ui.rx_addev_no );
    settextcolor(12);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
    goto  set_rx_io_ask;
    }
  }
if(( (ui.use_alsa&NATIVE_ALSA_USED)!=0) && ((ui.use_alsa&PORTAUDIO_RX_IN) == 0))
  {
  err=alsa_get_dev_names (ui.rx_addev_no);
  if(err < 0)
    {
    sprintf(s,"SOUNDCARD device number %i : INVALID DEVICE ", ui.rx_addev_no );
    settextcolor(12);
    lir_text(24,line,s);
    SNDLOG"\n%s\n",s);
    ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
    goto display_rx_out;
    }
  else
    {
    sprintf(s,"SOUNDCARD          =%s",alsa_dev_soundcard_name);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    sprintf(s,"device             =%s (%s)",alsa_dev_name,alsa_dev_plughw_pcm_name);
    line+=1;
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    sprintf(s,"device number      = %i", alsa_dev_seq_nmbr);
    line+=1;
    lir_text(24,line,s);
    line+=1;
    sprintf(s,"hostapi            = Native ALSA");
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    }
  }
else
  {
  if((ui.use_alsa&PORTAUDIO_RX_IN) == 0)
    {
#if HAVE_OSS == 1
// no validation available for alsa-oss or OSS
    sprintf(dev_name,"%s",SND_DEV);
    if(dev_name[0] != 'u')make_devname(ui.rx_addev_no, dev_name);
    if(kill_all_flag)goto set_rx_io_errexit;
    sprintf(s,"SOUNDCARD device   = %s",dev_name);
    if( (ui.rx_admode&O_RDWR) == O_RDWR )
      {
      sprintf(&s[strlen(s)],"  RDWR");
      }
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
#else
    lir_text(24,line,"ERROR: oss device but oss not installed.");
#endif
    }
  }
line++;
settextcolor(7);
sprintf(s,"associated radio   = %s",rx_soundcard_radio_names[ui.rx_soundcard_radio]);    
lir_text(24,line,s);
line++;
if(ui.rx_soundcard_radio == RX_SOUNDCARD_RADIO_WSE)
  {
  sprintf(s,"Port=%d Pin =%d",wse.parport,wse.parport_pin);
  lir_text(50,line,s);
  if (wse.parport==USB2LPT16_PORT_NUMBER)
    {
    settextcolor(10);
    if(libusb1_library_flag == TRUE || libusb0_library_flag == TRUE) 
      {
      sprintf(s,"USB2LPT 1.6 SELECTED");
      lir_text(70,line,s);
      }
    else
      {
      clear_lines(line,line);
      lir_text(15,line,"For USB2LTP you must install libusb or libusb-1.0");  
      }
    settextcolor(7);
    }
  line++;
  }
sprintf(s,"sample rate        = %i",ui.rx_ad_speed );
if(latency)sprintf(&s[strlen(s)],", Latency %d",latency);
SNDLOG"\n%s",s);
lir_text(24,line,s);
line++;
sprintf(s,"No of input bytes  = 2 (16 bits)");
if( (ui.rx_input_mode&DWORD_INPUT) != 0)
  {
  sprintf(&s[21],"4 (32 bits)");
  }
SNDLOG"\n%s",s);
lir_text(24,line,s);
line+=1;
tmptxt=" ";
tmptxt1=" ";
if( ((ui.rx_input_mode&TWO_CHANNELS) == 0)&&((ui.rx_input_mode&IQ_DATA) == 0))
  {
  tmptxt="radio interface    = One Rx channel, one audio channels ";
  tmptxt1="                     (normal audio)";
  }
if( ((ui.rx_input_mode&TWO_CHANNELS) == 0)&&((ui.rx_input_mode&IQ_DATA) != 0))
  {
  tmptxt="radio interface    = One Rx channel, two audio channels ";
  sprintf(s,"                     (direct conversion, time shift=%d)",
                      ui.sample_shift );
  tmptxt1=s;
  }
if( ((ui.rx_input_mode&TWO_CHANNELS) != 0)&&((ui.rx_input_mode&IQ_DATA) != 0))
  {
  tmptxt="radio interface    = Two Rx channels, four audio channels ";
  tmptxt1="                     (direct conversion, adaptive polarization)";
  }
if( ((ui.rx_input_mode&TWO_CHANNELS) != 0)&&((ui.rx_input_mode&IQ_DATA) == 0))
  {
  tmptxt="radio interface    = Two Rx channels, two audio channels ";
  tmptxt1="                     (normal audio, adaptive polarization)";
  }
lir_text(24,line,tmptxt);
SNDLOG"\n%s",tmptxt);
line+=1;
lir_text(24,line,tmptxt1);
SNDLOG"\n%s\n",tmptxt1);
display_rx_out:;
// ************************************************************************************
// Show the current RX output settings on the screen and do some validation if possible
// ************************************************************************************
settextcolor(7);
line+=2;
lir_text(2,line,"Linrad RX output to:");
SNDLOG"\nSETUP FOR RX OUTPUT IS NOW:\n");
settextcolor(10);
//check if network output mode
s[0]='\0';
if((ui.network_flag & NET_RXOUT_RAW16) != 0)  strcat ( s,"RAW16 ");
if((ui.network_flag & NET_RXOUT_RAW18) != 0)  strcat ( s,"RAW18 ");
if((ui.network_flag & NET_RXOUT_RAW24) != 0)  strcat ( s,"RAW24 ");
if((ui.network_flag & NET_RXOUT_FFT1)  != 0)  strcat ( s,"FFT1 ");
if((ui.network_flag & NET_RXOUT_TIMF2) != 0)  strcat ( s,"TIMF2 ");
if((ui.network_flag & NET_RXOUT_FFT2)  != 0)  strcat ( s,"FFT2 ");
if((ui.network_flag & NET_RXOUT_BASEB)  != 0)  strcat ( s,"BASEB16 ");
if((ui.network_flag & NET_RXOUT_BASEBRAW) != 0)  strcat ( s,"BASEB24 ");
if (strlen(s) != 0)
  {
  sprintf(s1,"NETWORK");
  lir_text(24,line,s1);
  SNDLOG"\n%s",s1);
  settextcolor(7);
  line+=1;
  sprintf(s1,"send format        = %s",s);
  lir_text(24,line,s1);
  SNDLOG"\n%s\n",s1);
  line+=2;
  settextcolor(10);
//disable output soundcard if not yet selected
  if(ui.rx_dadev_no == UNDEFINED_DEVICE_CODE) ui.rx_dadev_no=DISABLED_DEVICE_CODE;
  }
//check for output soundcards
latency=0;
if(ui.rx_dadev_no < 0)
  {
  if(ui.rx_dadev_no == UNDEFINED_DEVICE_CODE)
    {
    settextcolor(12);
    sprintf(s,"NOT YET SELECTED:  ");
    if(ui.rx_addev_no == UNDEFINED_DEVICE_CODE)
      {
      sprintf(&s[strlen(s)],"(Select input first.)");
      } 
    else
      { 
      sprintf(&s[strlen(s)]," (Select Menu Option 'B')");
      }
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    goto set_rx_io_ask;
    }
  else
    {
    ui.rx_dadev_no=DISABLED_DEVICE_CODE;
    sprintf(s,"DISABLED");
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    goto set_rx_io_ask;
    }
  }
else
  {
  if( (ui.use_alsa&PORTAUDIO_RX_OUT) != 0)
    {
    if(portaudio_active_flag==FALSE)portaudio_start(101);
    err=pa_get_device_info (ui.rx_dadev_no,
                            RXDA,
                            &pa_device_name,
                            &pa_device_hostapi,
                            &pa_device_max_speed,
                            &pa_device_min_speed,
                            &pa_device_max_bytes,
                            &pa_device_min_bytes,
                            &pa_device_max_channels,
                            &pa_device_min_channels);
    if (err == 0 )
      {
      sprintf(s,"SOUNDCARD device   = %s  ", pa_device_name );
      lir_text(24,line,s);
      SNDLOG"\n%s",s);
      line++;
      sprintf(s,"device  number     = %d", ui.rx_dadev_no);
      lir_text(24,line,s);
      SNDLOG"\n%s",s);
      line++;
      sprintf(s,"hostapi            = Portaudio (%s) ", pa_device_hostapi);
      lir_text(24,line,s);
      SNDLOG"\n%s",s);
      ui.rx_min_da_speed=(int)pa_device_min_speed;
      ui.rx_max_da_speed=(int)pa_device_max_speed;
      ui.rx_min_da_bytes=pa_device_min_bytes;
      ui.rx_max_da_bytes=pa_device_max_bytes;
      ui.rx_min_da_channels=pa_device_min_channels;
      ui.rx_max_da_channels=pa_device_max_channels;
      latency=ui.rx_da_latency;
      }
    else
      {
      sprintf(s,"INVALID DEVICE (Portaudio device number= %d )", ui.rx_dadev_no );
      settextcolor(12);
      lir_text(24,line,s);
      SNDLOG"\n%s\n",s);
      ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
      goto  set_rx_io_ask;
      }
    }
  else
    {
    if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
      {
      err=alsa_get_dev_names (ui.rx_dadev_no);
      if(err < 0)
        {
        sprintf(s,"SOUNDCARD device number %i : INVALID DEVICE ",
                                                            ui.rx_dadev_no );
        settextcolor(12);
        lir_text(24,line,s);
        SNDLOG"\n%s\n",s);
        ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
        goto  set_rx_io_ask;
        }
      else
        {
        sprintf(s,"SOUNDCARD          =%s",alsa_dev_soundcard_name);
        lir_text(24,line,s);
        line++;
        SNDLOG"\n%s",s);
        sprintf(s,"device             =%s (%s)",alsa_dev_name,alsa_dev_plughw_pcm_name);
        lir_text(24,line,s);
        line++;
        SNDLOG"\n%s",s);
        sprintf(s,"device number      = %i",alsa_dev_seq_nmbr);
        lir_text(24,line,s);
        line++;
        SNDLOG"\n%s",s);
        sprintf(s,"hostapi            = Native ALSA");
        lir_text(24,line,s);
        SNDLOG"\n%s",s);
        settextcolor(7);
        }
      }
    else
      {
#if HAVE_OSS == 1 
      if(dev_name[0] != 'u')make_devname(ui.rx_dadev_no, dev_name);
      if(kill_all_flag)goto set_rx_io_errexit;
      sprintf(s,"SOUNDCARD device   = %s",dev_name);
      if(ui.rx_addev_no == ui.rx_dadev_no && (ui.rx_admode&O_RDWR) == O_RDWR )
        {
        sprintf(&s[strlen(s)],"  RDWR");
        }
      lir_text(24,line,s);
      SNDLOG"\n%s",s);
      settextcolor(7);
#else
      lir_text(24,line,"ERROR");
#endif
      }
    }
  }
line++;
settextcolor(7);
sprintf(s,"min da sample rate = %i",ui.rx_min_da_speed);
SNDLOG"\n%s",s);
lir_text(24,line,s);
line++;
sprintf(s,"max da sample rate = %i ", ui.rx_max_da_speed);
SNDLOG"\n%s",s);
lir_text(24,line,s);
line++;
if ( (ui.use_alsa&PORTAUDIO_RX_OUT) != 0)
  {
  sprintf(s,"Latency factor     = %i ",ui.rx_da_latency );
  SNDLOG"\n%s",s);
  lir_text(24,line,s);
  line++;
  }
sprintf(s,"min da bytes       = %i",ui.rx_min_da_bytes);
SNDLOG"\n%s",s);
lir_text(24,line,s);
line++;
sprintf(s,"max da bytes       = %i ", ui.rx_max_da_bytes);
SNDLOG"\n%s",s);
lir_text(24,line,s);
line++;
sprintf(s,"min da channels    = %i",ui.rx_min_da_channels);
SNDLOG"\n%s",s);
lir_text(24,line,s);
line++;
sprintf(s,"max da channels    = %i", ui.rx_max_da_channels);
SNDLOG"\n%s",s);
lir_text(24,line,s);
set_rx_io_ask:;
settextcolor(7);
line+=2;
if(ui.converter_mode != 0)
  {
  compute_converter_parameters();
  settextcolor(10);
  line+=2;  
  if( (ui.converter_mode&CONVERTER_UP) !=0 )
    {
    lir_text(2,line,"UP");
    ss="IF";
    j=4;
    }
  else
    {
    lir_text(2,line,"DOWN");
    ss="signal";
    j=6;
    }   
  lir_text(j,line,"CONVERTER:");
  settextcolor(7);
  sprintf(s,"LO frequency %.6f",converter_offset_mhz);
  i=0;
  while(s[i] != 0)i++;
  while(s[i-1] == '0')i--;
  if(s[i-1] == '.')i--;
  sprintf(&s[i]," MHz.  LO is ");
  if( (ui.converter_mode&CONVERTER_LO_BELOW) != 0)
    {
    strcat(s,"below ");
    }
  else
    {
    strcat(s,"above ");
    }
  strcat(s,ss);
  strcat(s," frequency.");
  lir_text(j+11,line,s);    
  }
line+=2;
sprintf(s,"DMA rate   min=%i    max=%i" ,ui.min_dma_rate, ui.max_dma_rate);
SNDLOG"\n%s\n",s);
lir_text(2,line,s);
// *******************************************************************
// Ask whether the user wants to change something and act accordingly.
// *******************************************************************
io_menu:;
line+=2;
sndlog_sync();
lir_text(2,line,"A = Change input settings and reset all other soundcard settings");
line++;
lir_text(7,line,"if a soundcard is selected as input.");
line++;
lir_text(2,line,"B = Change the output soundcard settings.");
line++;
if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
  {
  lir_text(2,line,"C = Change min and max dma rate .");
  line++;
  }
if(ui.rx_soundcard_radio != RX_SOUNDCARD_RADIO_UNDEFINED &&
   ui.rx_soundcard_radio != RX_SOUNDCARD_RADIO_UNDEFINED_REVERSED)
  {
  lir_text(2,line,"D = Set parameters for soundcard radio hardware.");
  line++;
  }
if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
  {
  lir_text(2,line,"E = Enable/Disable frequency converter and set LO.");
  line++;
  }
lir_text(2,line,"Z = Disable the output soundcard.");
line++;
lir_text(2,line,"Y = Select sound system. OSS or ALSA and reset all soundcard settings.");
line++;
check_line(&line);
lir_text(2,line,"X = To main menu.");
await_processed_keyboard();
if(kill_all_flag)goto set_rx_io_errexit;
clear_screen();
switch(lir_inkey)
  {
  case 'A':
// ************************* CHANGE INPUT SETTINGS ************************************ //
  ui.sample_shift=0;
  line=2;
  SNDLOG"\nUSER STARTED THE SELECTION OF A NEW INPUT DEVICE \n");
  if(ui.use_extio != 0)
    {
    if(extio_handle != NULL)
      { 
      command_extio_library(EXTIO_COMMAND_UNLOAD_DLL);
      }
    ui.use_extio=0;
    }
  ui.rx_addev_no =UNDEFINED_DEVICE_CODE;
  lir_text(10,line,"SELECT HARDWARE FOR INPUT");
  line+=2;
  lir_text(4,line,"A = Soundcard");
  line++;
  lir_text(4,line,"B = SDR-14 or SDR-IQ");
  line++;
  lir_text(4,line,"C = Perseus");
  line++;
  lir_text(4,line,"D = SDR-IP");
  line++;
  lir_text(4,line,"E = Excalibur");
  line++;
  lir_text(4,line,"F = libExtIO hardwares");
  line++;
  lir_text(4,line,"H = RTL2832 USB dongle");
  line++;
  lir_text(4,line,"I = SDRplay or Mirics USB dongle");
  line++;
  lir_text(4,line,"J = bladeRF");
  line++;
  lir_text(4,line,"K = PCIe 9842");
  line++;
  lir_text(4,line,"L = OpenHPSDR");
  line++;
  lir_text(4,line,"M = Afedri-NET");
  line++;
  lir_text(4,line,"N = ELAD FDM-S1");
  line++;
  lir_text(4,line,"O = Airspy");
  line++;
  lir_text(4,line,"P = CloudIQ");
  line++;
  lir_text(4,line,"Q = Airspy HF+");
  line++;
  lir_text(4,line,"S = SDRplay ver2 (RSP1a/RSPII/RSPduo)");
  line++;
  lir_text(4,line,"T = SDRplay ver3 (RSP1/RSP1A/RSP2/RSPduo/RSPdx)");
  line++;
  if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
    {
    lir_text(4,line,"Y = Network");
    line++;
    }
  lir_text(4,line,"Z = Disable (Disk input allowed)");
  await_processed_keyboard();
  if(kill_all_flag)goto set_rx_io_errexit;
  clear_screen();
  ui.rx_soundcard_radio=0;
  ui.use_extio=0;
// reset the output soundcard and the tx soundcards.
  ui.use_alsa&=~(PORTAUDIO_RX_IN+PORTAUDIO_RX_OUT+
                 PORTAUDIO_TX_IN+PORTAUDIO_TX_OUT);
  ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
  ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
  ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
  ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
  switch (lir_inkey)
    {
    case 'A':
use_sndcard:;
    clear_screen();
    if(!alsa_library_flag && (ui.use_alsa&NATIVE_ALSA_USED) != 0)
      {
      settextcolor(12);
      lir_text(10,1,"ALSA not loaded");
      settextcolor(7);
      line=2;
      goto io_menu; 
      }
#if DARWIN != 1
    lir_text(10,5,
    "Use Portaudio (allows all/slow devices) for rx input? (Y/N) =>");
    lir_text(15,7,press_f1_for_info);
    await_processed_keyboard();
    if(kill_all_flag)goto set_rxin_exit;
    if(lir_inkey == F1_KEY || lir_inkey == '!')
      {
      help_message(343);
      goto use_sndcard;
      }
    if(lir_inkey == 'Y')
      {
      ui.use_alsa|=PORTAUDIO_RX_IN;
      if(portaudio_active_flag==FALSE)portaudio_start(102);
      }
    else
      {
      if(lir_inkey != 'N')
        {
        goto use_sndcard;
        }
      }
    i=TRUE;  
    if(portaudio_active_flag==FALSE)i=portaudio_start(103);
    if(kill_all_flag)goto set_rxin_exit;
    if(!i)goto use_sndcard;
    clear_screen();
#else
    ui.use_alsa|=PORTAUDIO_RX_IN;
    i=TRUE;
    if(portaudio_active_flag==FALSE)i=portaudio_start(104);
    if(kill_all_flag)goto set_rxin_exit;
    if(!i)
      {
      lir_text(10,5,"Could not start Portaudio.");
      lir_text(12,5,press_any_key);
      await_keyboard();
      goto set_rxin_exit;
      }
#endif
//get a 'portaudio' selection list
    line=1;
    if( (ui.use_alsa&PORTAUDIO_RX_IN) != 0)
      {
      set_portaudio(RXAD, &line);
      goto set_rxin_exit;
      }
// look for input soundcards
    sprintf(s,"SELECT SOUNDCARD DEVICE FOR RX INPUT FROM LIST:");
    SNDLOG"%s\n",s);
    line=1;
    settextcolor(14);
    lir_text(2,line,s);
    settextcolor(7);
    line++;
    if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
      {
      ui.rx_addev_no=select_alsadev(SND_PCM_STREAM_CAPTURE, &line);
      if(kill_all_flag)goto set_rx_io_errexit;
      alsa_get_native_samplerate(ui.rx_addev_no,SND_PCM_STREAM_CAPTURE,&line,&new_sample_rate);
      if(kill_all_flag)goto set_rx_io_errexit;
      ui.rx_ad_speed=(int)new_sample_rate;
//get number of bytes and check
      if(alsa_dev_max_bytes > 2)
        {
        if(alsa_dev_max_bytes != 4)
          {
          lirerr(872341);
          goto set_rx_io_errexit;
          }
        if(alsa_dev_min_bytes == 4)
          {
          ui.rx_input_mode=DWORD_INPUT;
          }
        else
          {
          if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
            {
            sprintf(s,"Enter number of input bytes (2 or 4) >");
select_number_input_bytes:;
            SNDLOG"%s\n",s);
            lir_text(0,line,s);
            i=lir_get_integer(39, line, 1,2,4);
            if(kill_all_flag)goto set_rx_io_errexit;
            sprintf(s,"User selected  %i",i);
            SNDLOG"%s\n",s);
            switch (i)
              {
              case 2:
              ui.rx_input_mode=0;
              break;

              case 4:
              ui.rx_input_mode=DWORD_INPUT;
              break;

              default:
              settextcolor(12);
              sprintf(s,"invalid selection, try again");
              SNDLOG"%s\n",s);
              lir_text(44,line,s);
              settextcolor(7);
              goto select_number_input_bytes;
              }
            }
          else
            {
            ui.rx_input_mode=DWORD_INPUT;
            }
          }
        }
      else
        {
        ui.rx_input_mode=0;
        SNDLOG"\n\n");
        }
      dev_min_channels=1;
      dev_max_channels=alsa_dev_max_channels;
      if(dev_max_channels > 4)dev_max_channels=4;
//  With native alsa, always set ui.rx_admode to 0
      ui.rx_admode=0;
      line+=2;
      }
    else
//OSS or alsa-oss
      {
#if HAVE_OSS == 1 
      column=0;
      speed_warning=0;
      if(ui.process_priority != 0)
        {
        if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
          {
          settextcolor(12);
          lir_text(0,line+2,"WARNING!! SETTING PRIORITY ABOVE 0 WILL PROBABLY CAUSE");
          lir_text(0,line+3,"YOUR SYSTEM TO FREEZE IF YOU ARE USING 4FRONT OSS");
          sprintf(s,"Press y to keep priority=%d",ui.process_priority);
          lir_text(0,line+5,s);
          lir_text(0,line+6,"Any other key to set priority to zero.");
          await_processed_keyboard();
          if(lir_inkey != 'Y')ui.process_priority=0;
          settextcolor(7);
          clear_lines(line+2,line+6);
          }
        else
          {
          ui.process_priority=0;  
          }
        }
      select_ossdev(&line, RXAD);
      if(kill_all_flag)goto set_rx_io_errexit;
      if(ui.rx_addev_no == UNDEFINED_DEVICE_CODE)goto begin_rx_set_io;
      max=dev_max_rd_speed[ui.rx_addev_no&255];
      min=dev_min_rd_speed[ui.rx_addev_no&255];
      if(max == ABOVE_MAX_SPEED || max < 1000)
        {
        max=ABOVE_MAX_SPEED;
        speed_warning=1;
        }
      sprintf(s,"Select sampling speed(%d to %d)", min,max);
      lir_text(0,line,s);
      if(speed_warning != 0)
        {
        settextcolor(12);
        lir_text(0,line+2,
         "WARNING!! device does not respond correctly to speed setting command");
        lir_text(0,line+3,
             "Incorrect (unsupported) values may cause system to hang");
        settextcolor(7);
        }
      column=0;
      while(s[column] != 0)column++;
      ui.rx_ad_speed=lir_get_integer(column+2, line, 8, min,max);
      lir_refresh_screen();
      if(kill_all_flag)goto set_rx_io_errexit;
      rdwr_max_speed=ui.rx_ad_speed;
      SNDLOG"\n%d Hz selected by user",ui.rx_ad_speed);
      line++;
      clear_lines(line,line+3);
      if(ioctl(rx_audio_in, SNDCTL_DSP_SPEED, &ui.rx_ad_speed)==-1)
        {
        lirerr(1041);
        goto set_rx_io_errexit;
        }
      if(ui.rx_ad_speed > (int)(rdwr_max_speed*1.05) ||
         ui.rx_ad_speed < (int)(rdwr_max_speed*0.95) )
        {
// The user selected something that was unacceptable to the device driver.
// Rather than just accepting whatever value goto set_rx_io_errexited,
// could be -22 (an unspecified error code) we step the desired speed
// upwards, looking for a valid speed.
// In case we fail, an attempt is made downwards as well.
// ****************************************************************
// This code is written by Ulf, SM0LCB
        SNDLOG"\nReqested speed didn't work");
// This code will search up/down from requested speed
// until the driver will respond with valid code
// Search use delta stepping of 3% from current speed
// This to speed up the search for a valid speed
// First search up in fq
        SNDLOG"\nSearch up : ");
        for( trySpeed=rdwr_max_speed, 
                       deltaSpeed=(int)((float)(rdwr_max_speed)*0.03F);
                       trySpeed<192000;
                       trySpeed+=deltaSpeed )
          {
          ui.rx_ad_speed=trySpeed;
          if(ioctl(rx_audio_in, SNDCTL_DSP_SPEED, &ui.rx_ad_speed)==-1)
            {
            lirerr(1024);
            goto set_rx_io_errexit;
            }
// test if valid speed
          if(ui.rx_ad_speed < (int)(trySpeed*1.05) &&
             ui.rx_ad_speed > (int)(trySpeed*0.95) )
            {
            SNDLOG"try speed %d Hz ", trySpeed);
            goto spok;
            }
          deltaSpeed=(int)((float)(trySpeed)*0.03F);
          }
        SNDLOG"no speed found");
// Then search down in fq
        SNDLOG"\nSearch down : ");
        for( trySpeed=rdwr_max_speed, 
                     deltaSpeed=(int)((float)(rdwr_max_speed)*0.03F);
                     trySpeed>5000;
                     trySpeed-=deltaSpeed )
          {
          ui.rx_ad_speed=trySpeed;
          if(ioctl(rx_audio_in, SNDCTL_DSP_SPEED, &ui.rx_ad_speed)==-1)
            {
            lirerr(1135);
            goto set_rx_io_errexit;
            }
// test if valid speed
          if(ui.rx_ad_speed < (int)(trySpeed*1.05) &&
             ui.rx_ad_speed > (int)(trySpeed*0.95) )
            {
            SNDLOG"try speed %d Hz ", trySpeed);
            goto spok;
            }
          deltaSpeed=(int)((float)(trySpeed)*0.03F);
          }
        SNDLOG"no speed found");
// If we didn't find any vaid speed
        SNDLOG"\nNo valid speed found !");
// Exit program
        lirerr(1134);
        goto set_rx_io_errexit;
        }
spok:;
      close(rx_audio_in);
      rx_audio_in=-1;
      SNDLOG"\n%d Hz reported by device",ui.rx_ad_speed);
      sndlog_sync();
      sprintf(s,"Input speed: %.3fkHz",0.001*(float)(ui.rx_ad_speed));
      lir_text(0,line,s);
      line++;
      if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
        {
        if( (dev_rd_bytes[ui.rx_addev_no&255]&6) == 6)
          {
          line++;
          lir_text(0,line,"Select A/D number of bytes (2 or 4):");
          line++;
selfmt:;
          await_processed_keyboard();
          if(kill_all_flag) goto set_rx_io_errexit;
          if(lir_inkey == '4')
            {
            ui.rx_input_mode=DWORD_INPUT;
            }
          else
            {
            if(lir_inkey != '2')goto selfmt;
            ui.rx_input_mode=0;
            }
          goto fmtok;
          }
        }
      if( (dev_rd_bytes[ui.rx_addev_no&255]&6) == 4)
        {
        ui.rx_input_mode=DWORD_INPUT;
        }
      else
        {
        ui.rx_input_mode=0;
        }
fmtok:;
      dev_max_channels=0;
      k=4;
      while(dev_max_channels == 0 && k> 0)
        {
        dev_max_channels|=dev_rd_channels[ui.rx_addev_no]&k;
        k/=2;
        }
      dev_min_channels=1;
#endif
      }
    if(ui.rx_addev_no < 256)
      {
      refline=line;
sel_radio:;
      line=refline;
      j=dev_max_channels;
      k=dev_min_channels;
      line++;
      lir_text(0,line,"Select radio interface:");
      line=line+k+1;
      if(j == 2)j=3;
      for(i=k; i<=j; i++)
        {
        sprintf(s,"%d: %s",i,audiomode_text[i-1]);
        lir_text(5,line,s);
        line++;
        }
      line++;
      lir_text(1,line,"F1 for help/info");
chsel:;
      await_processed_keyboard();
      if(kill_all_flag) goto set_rx_io_errexit;
      if(lir_inkey == F1_KEY || lir_inkey == '!')
        {
        help_message(89);
        clear_screen();
        goto sel_radio;
        }
      if(lir_inkey-'0'<k || lir_inkey-'0'>j)goto chsel;
      switch (lir_inkey)
        {
        case '1':
        ui.rx_rf_channels=1;
        ui.rx_ad_channels=1;
        break;

        case '2':
        ui.rx_input_mode|=IQ_DATA;
        ui.rx_rf_channels=1;
        ui.rx_ad_channels=2;
        break;

        case '3':
        ui.rx_input_mode|=TWO_CHANNELS;
        ui.rx_rf_channels=2;
        ui.rx_ad_channels=2;
        break;

        case '4':
        ui.rx_input_mode|=TWO_CHANNELS+IQ_DATA;
        ui.rx_rf_channels=2;
        ui.rx_ad_channels=4;
        break;

        default:
        goto chsel;
        }
      SNDLOG"\nui.rx_input_mode=%d  ui.rx_rf_channels=%d  ui.rx_ad_channels =%d",
                         ui.rx_input_mode, ui.rx_rf_channels, ui.rx_ad_channels);
      if(ui.rx_addev_no == ui.rx_dadev_no)ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
      if(ui.tx_enable != 0)
        { 
        if(ui.rx_addev_no == ui.tx_dadev_no)ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
        if(ui.rx_addev_no == ui.tx_addev_no)ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
        }
      }
    if(ui.rx_rf_channels == 1 && ui.rx_ad_channels == 2)
      {
      line+=2;
      lir_text(0,line,"Number of points to time shift between I and Q? (-4 to +4)");
      ui.sample_shift=lir_get_integer(60,line, 2, -4, 4);
      }
set_rxin_exit:    
    clear_screen();
    line=0;
    lir_text(9,line,"Select receiver hardware to use with soundcard.");
    line+=2;
    for(i=0; i<MAX_RX_SOUNDCARD_RADIO; i++)
      {
      sprintf(s,"%d   %s",i,rx_soundcard_radio_names[i]);
      lir_text(5,line,s);
      line++;
      if(line >= screen_last_line)screenerr(&line);
      }
    line++;
    if(line >= screen_last_line)screenerr(&line);
    lir_text(0,line,"Select by line number=>");
    ui.rx_soundcard_radio=lir_get_integer(25,line,1,0,MAX_RX_SOUNDCARD_RADIO);
    break;
    
    case 'B':
    timf1_char=malloc(0x8000);
    if(timf1_char == NULL)
      {
      lirerr(1231231);
      goto set_rx_io_errexit;
      }
    init_sdr14();
    free(timf1_char);
    break;
    
    case 'C':
    init_perseus();
    if(kill_all_flag)goto set_rx_io_errexit;
    if(sdr == -1)
      {
      clear_screen();
      lir_text(5,2,"Perseus init failed.");
      lir_text(5,3,"Is the unit connected and properly powered?");
      lir_text(7,5,press_any_key);
      await_processed_keyboard();
      }
    else
      {  
      sdr=-1;
      }
    break;
    
    case 'D':
    init_sdrip();
    break;

    case 'E':
    lir_text(5,2,"No Excalibur yet under Linux.");
    lir_text(7,4,press_any_key);
    await_processed_keyboard();
    break;

    case 'F':
    init_extio();
    if(kill_all_flag)goto set_rx_io_errexit;
    if(ui.extio_type == 4)goto use_sndcard;
    break;
    
    case 'H':
    init_rtl2832();
    break;

    case 'I':
    init_mirics();
    break;

    case 'J':
    init_bladerf();
    break;

    case 'K':
    init_pcie9842();
    break;

    case 'L':
    init_openhpsdr();
    break;

    case 'M':
    init_netafedri();
    break;
    
    case 'N':
    init_elad_fdms1();
    break;
    
    case 'O':
    init_airspy();
    break;
    
    case 'P':
    init_cloudiq();
    break;

    case 'Q':
    init_airspyhf();
    break;

    case 'S':
    init_sdrplay2();
    break;

    case 'T':
    init_sdrplay3();
    break;

    case 'Y':
    if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
      {
      ui.rx_addev_no=NETWORK_DEVICE_CODE;
      ui.network_flag=(int)((unsigned int)ui.network_flag|NET_RX_INPUT);
      verify_network(TRUE);
      }
    break;

    case 'Z':
    ui.rx_addev_no=DISABLED_DEVICE_CODE;
    break;
    }
  if(kill_all_flag)goto set_rx_io_errexit;
  if(ui.rx_addev_no == UNDEFINED_DEVICE_CODE)goto begin_rx_set_io;
  break;

  case 'B':
// ************************* CHANGE OUTPUT SETTINGS ************************************ //
  if (ui.rx_addev_no == UNDEFINED_DEVICE_CODE)
    {
    settextcolor(11);
    line+=2;
    lir_text(2,line,"Option B selected, but RX input must be set first:");
    line+=1;
    lir_text(2,line,"Hit any key to go back to this menu and select option A");
    settextcolor(7);
    await_processed_keyboard();
    if(kill_all_flag) goto set_rx_io_errexit;
    break;
    }
  SNDLOG"\nUSER STARTED THE SELECTION OF A NEW OUTPUT DEVICE \n");
  clear_screen();
  if((ui.rx_addev_no >=0 && ui.rx_addev_no < SPECIFIC_DEVICE_CODES) &&
                                        (ui.network_flag&NET_RX_INPUT) == 0)
    {
    if(start_iotest(&line,RXAD) == FALSE)
      {
      if(kill_all_flag)return;
      goto set_rxout_exit;
      }
    }
#if DARWIN != 1
pa_rxda:;
#endif
  line=3;
  ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
  ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
  ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
  ui.use_alsa&=~(PORTAUDIO_RX_OUT+PORTAUDIO_TX_IN+PORTAUDIO_TX_OUT);
#if DARWIN != 1
  lir_text(2,line,
  "Use Portaudio (allows all/slow devices) for rx output? (Y/N) =>");
  lir_text(7,line+2,press_f1_for_info);
  await_processed_keyboard();
  if(kill_all_flag) goto set_rx_io_errexit;
  if(lir_inkey == F1_KEY || lir_inkey == '!')
    {
    help_message(343);
go_pa_rxda:;
    clear_screen();
    goto pa_rxda;
    }
  if(lir_inkey == 'Y')
    {
    ui.use_alsa|=PORTAUDIO_RX_OUT;
    if(portaudio_active_flag==FALSE)i=portaudio_start(104);
    }
  else
    {
    if(lir_inkey != 'N')
      {
      goto go_pa_rxda;
      }
    }
  i=TRUE;  
  if(portaudio_active_flag==FALSE)i=portaudio_start(105);
  if(kill_all_flag)goto set_rx_io_errexit;
  if(!i)goto go_pa_rxda;
#else
  ui.use_alsa|=PORTAUDIO_RX_OUT;
  i=TRUE;
  if(portaudio_active_flag==FALSE)i=portaudio_start(106);
  if(kill_all_flag)goto set_rx_io_errexit;
  if(!i)
    {
    lir_text(10,5,"Could not start Portaudio.");
    lir_text(12,5,press_any_key);
    await_keyboard();
    goto set_rxout_exit;
    }
#endif
// Start test-thread with input soundcard device
//get a 'portaudio' selection list for output soundcards
  if( (ui.use_alsa&PORTAUDIO_RX_OUT) != 0)
    {
    set_portaudio(RXDA, &line);
    if(kill_all_flag) goto set_rx_io_errexit;
    goto set_rxout_exit;
    }
//get an 'ALSA' or 'OSS' selection list for output soundcards
  sprintf(s,"SELECT SOUNDCARD DEVICE FOR RX OUTPUT FROM LIST:");
  SNDLOG"%s\n",s);
  line=1;
  settextcolor(14);
  lir_text(2,line,s);
  settextcolor(7);
  line++;
  if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
    {
    ui.rx_dadev_no=select_alsadev(SND_PCM_STREAM_PLAYBACK, &line);
    if(kill_all_flag)goto set_rxout_exit;
    ui.rx_min_da_speed=alsa_dev_min_rate[ui.rx_dadev_no];
    ui.rx_max_da_speed=alsa_dev_max_rate[ui.rx_dadev_no];
    if(alsa_dev_min_bytes > 2 )
      {
      if(line >= screen_last_line-5)
        {
        clear_screen();
        line=0;
        }
      lir_text(0,line, "RX output only allowed with 8 or 16 bit data.");
      line++;
      lir_text(0,line, wish_anyway);
      line++;
anyw:;
      await_processed_keyboard();
      if(kill_all_flag) goto set_rx_io_errexit;
      if(lir_inkey == 'N')
        {
        ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
        lir_text(10,line,press_any_key);
        goto set_rxout_exit;
        }
      if(lir_inkey != 'Y')goto anyw;
      alsa_dev_min_bytes=1;
      }
    ui.rx_min_da_bytes=alsa_dev_min_bytes;
    ui.rx_max_da_bytes=alsa_dev_max_bytes;
    if(ui.rx_max_da_bytes > 2)
      {
      ui.rx_max_da_bytes=2;
      }
    if(alsa_dev_min_channels >=  2 )ui.rx_min_da_channels=2;
       else ui.rx_min_da_channels=1;
    if(alsa_dev_max_channels <  2 )ui.rx_max_da_channels=1;
       else ui.rx_max_da_channels=2;
    }
  else
    {
#if HAVE_OSS == 1 
    refline=line;
    select_ossdev(&line, RXDA);
    if(kill_all_flag)goto set_rx_io_errexit;
    if(ui.rx_dadev_no == (ui.rx_addev_no&255) && ui.rx_admode == O_RDWR)
      {
      if( (ui.rx_ad_channels == 4) || (ui.rx_input_mode&DWORD_INPUT) != 0)
        {
        lir_text(0,line,
        "RDWR for output not possible with 32 bit data or 4 channels.");
        line++;
        ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
        lir_text(10,line,press_any_key);
        await_processed_keyboard();
        if(kill_all_flag) goto set_rx_io_errexit;
        goto set_rxout_exit;
        }
      ui.rx_min_da_speed=ui.rx_ad_speed;
      ui.rx_max_da_speed=ui.rx_ad_speed;
      ui.rx_min_da_bytes=2;
      ui.rx_max_da_bytes=2;
      ui.rx_max_da_channels=ui.rx_ad_channels;
      ui.rx_min_da_channels=ui.rx_ad_channels;
      }
    else
      {
      set_rx_dadev_parms(&line);
      }
// Now that we decided what device to use for output, let
// the user decide how to use it. (It may become microphone input).
    if(ui.rx_dadev_no != UNDEFINED_DEVICE_CODE)
      {
      if(dev_name[0] != 'u')make_devname(ui.rx_dadev_no, dev_name);
      if(kill_all_flag)goto set_rx_io_errexit;
      sndlog_sync();
      ui.rx_damode=select_rdwr_mode(&line, ui.rx_dadev_no, O_WRONLY);
      }
#endif
    }
set_rxout_exit:;
  line++;
  if(ui.tx_enable != 0)
    { 
    if(ui.rx_dadev_no == ui.tx_dadev_no)ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
    if(ui.rx_dadev_no == ui.tx_addev_no)ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
    }
  if((ui.rx_addev_no >=0 && ui.rx_addev_no < SPECIFIC_DEVICE_CODES)&&
                                      (ui.network_flag&NET_RX_INPUT) == 0)
    {
    stop_iotest(RXAD);
    if(kill_all_flag)goto set_rx_io_errexit;
    }
//set_rxout_exit2:;
  break;

  case 'C':
// ************************ CHANGE DMA RATE  SETTINGS ******************************
  if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
    {
    line=5;
    SNDLOG"\nUSER STARTED A CHANGE OF MIN/MAX DMA RATE\n");
    sprintf(s,"Current  min, max DMA rates in Hz       : %i, %i ",
                                               ui.min_dma_rate, ui.max_dma_rate);
    lir_text(10,line,s);
    SNDLOG"%s\n",s);
    line+=2;
    if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
      {
      n1=MIN_DMA_RATE_EXP;
      m1=DMA_MIN_DIGITS_EXP;
      n2=MAX_DMA_RATE_EXP;
      m2=DMA_MAX_DIGITS_EXP;
      }
    else
      {
      n1=MIN_DMA_RATE;
      m1=DMA_MIN_DIGITS;
      n2=MAX_DMA_RATE;
      m2=DMA_MAX_DIGITS;
      }
    sprintf(s,"Enter new min DMA rate in Hz (%d-%d) > ",n1,n2);
    SNDLOG"%s\n",s);
    lir_text(10,line,s);
    i=lir_get_integer(45+m1+m2, line, m2,n1,n2);
    if(kill_all_flag)goto set_rx_io_errexit;
    sprintf(s,"User selected  %i",i);
    SNDLOG"%s\n",s);
    ui.min_dma_rate=i;
    line+=4;
    sprintf(s,"%i",i);
    m1=strlen(s);
    sprintf(s,"Enter new max DMA rate in Hz (%d-%d) > ",ui.min_dma_rate,n2);
    SNDLOG"%s\n",s);
    lir_text(10,line,s);
    i=lir_get_integer(43+m1+m2, line, m2, ui.min_dma_rate, n2);
    if(kill_all_flag)goto set_rx_io_errexit;
    sprintf(s,"User selected  %i",i);
    SNDLOG"%s\n",s);
    ui.max_dma_rate=i;
    }
  break;

  case 'D':
  switch (ui.rx_soundcard_radio)
    {
    case RX_SOUNDCARD_RADIO_WSE:
    wse_setup();
    break;
    
    case RX_SOUNDCARD_RADIO_SI570:
    si570_rx_setup();
    break;

    case RX_SOUNDCARD_RADIO_SOFT66:
    soft66_setup();
    break;

    case RX_SOUNDCARD_RADIO_ELEKTOR:
    elektor_setup();
    break;

    case RX_SOUNDCARD_RADIO_FCDPROPLUS:
    fcdproplus_setup();
    break;

    case RX_SOUNDCARD_RADIO_AFEDRI_USB:
    afedriusb_setup();
    }  
  break;  

  case 'E':
// *************************  CONVERTER SETUP FOR RX ****************************
  lir_text(5,3,"Do you wish to use a converter in front of your receiver (Y/N)?");
  ss=NULL;
get_convuse:;
  await_processed_keyboard();
  if(kill_all_flag) goto set_rx_io_errexit;
  ui.converter_mode=0;
  if(lir_inkey=='N')
    {
    ui.converter_hz=0;  
    ui.converter_mhz=0;  
    converter_offset_mhz=0;
    break;
    }
  else
    {
    if(lir_inkey != 'Y')goto get_convuse;
    }
  ui.converter_mode=CONVERTER_USE;
  lir_text(5,5,"Enter converter LO frequency in MHz:");
  converter_offset_mhz=(float)lir_get_float(42,5,14,0.001F,200000.);
  ui.converter_mhz=(unsigned int)converter_offset_mhz;
  t1=round(1000000*(converter_offset_mhz-ui.converter_mhz));
  ui.converter_hz=(int)t1;
get_updown:;
  lir_text(5,7,"Up or down conversion? (U/D)?");  
  await_processed_keyboard();
  if(kill_all_flag) goto set_rx_io_errexit;
  if(lir_inkey=='U')
    {
    ui.converter_mode|=CONVERTER_UP;
    ss="IF";
    }
  else
    {
    if(lir_inkey != 'D')goto get_updown;
    ss="signal";
    }
get_losign:;
  sprintf(s,"Is LO below %s frequency (Y/N)?",ss);
  lir_text(5,9,s);
  await_processed_keyboard();
  if(kill_all_flag) goto set_rx_io_errexit;
  if(lir_inkey=='Y')
    {
    ui.converter_mode|=CONVERTER_LO_BELOW;  
    }
  else
    {
    if(lir_inkey != 'N')goto get_losign;
    }
  compute_converter_parameters();
  break;

  case 'Z':
// ************************* DISABLE OUTPUT *************************************
  SNDLOG"\nUSER DISABLED THE OUPUT SOUNDCARD \n");
  ui.rx_dadev_no=DISABLED_DEVICE_CODE;
  ui.rx_max_da_bytes=2;
  ui.rx_max_da_channels=2;
  ui.rx_min_da_bytes=1;
  ui.rx_min_da_channels=1;
  ui.rx_min_da_speed=5000;
  ui.rx_max_da_speed=1000000;
  break;

  case 'Y':
// ************************** selectE sounde system ***************************** //
  select_sound_system();
  if(kill_all_flag)goto set_rx_io_errexit;
  ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
  ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
  ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
  ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
  ui.max_dma_rate=DEFAULT_MAX_DMA_RATE;
  ui.min_dma_rate=DEFAULT_MIN_DMA_RATE;
  goto begin_rx_set_io;

  case 'X':
// ************************* EXIT TO MAIN MENU *********************************** //
//validate input settings
  if((ui.rx_addev_no == -1) && (((unsigned int)ui.network_flag&NET_RX_INPUT) == 0))
    {
    settextcolor(12);
    line+=2;
    lir_text(2,line,"Invalid input settings: NOT YET SELECTED");
    line+=1;
    lir_text(2,line,
      "Hit any key to go back to the main menu and select a network input");
    line+=1;
    lir_text(2,line,
      "or hit U on the main menu to go back to this menu and hit A.");
    settextcolor(7);
    }
  else
    {
//validate output settings
    if(ui.rx_dadev_no == UNDEFINED_DEVICE_CODE)
      {
      settextcolor(12);
      line+=2;
      lir_text(2,line,"Invalid output settings: NOT YET SELECTED");
      line+=1;
      lir_text(2,line,
            "Hit any key to go back to this menu and select option B or Z");
      settextcolor(7);
      await_processed_keyboard();
      if(kill_all_flag) goto set_rx_io_errexit;
      break;
      }
    clear_screen();
    lir_text(2,5,remind_parsave);
    lir_text(2,7,press_any_key);
    }
  await_keyboard();
  goto set_rx_io_exit;

  default:
  break;
  }
// ******************************************************************************** //
SNDLOG"\ngoto begin_rx_set_io\n\n");
if(kill_all_flag)goto set_rx_io_errexit;
sndlog_sync();
goto begin_rx_set_io;
set_rx_io_exit:;
SNDLOG"\nNormal end");
set_rx_io_errexit:;
SNDLOG"\n");
fclose(sndlog);
if(portaudio_active_flag==TRUE)portaudio_stop(100);
write_log=FALSE;
}

void set_tx_io(void)
{
char *tmptxt;
int i, k;
int dev_valid;
int err;
int line;
char s[512];
char *tx_logfile_name="soundboard_tx_init.log";
char pa_device_name[128];
char pa_device_hostapi  [128];
double pa_device_max_speed;
double pa_device_min_speed;
int soundcard_error_flag;
int pa_device_max_bytes;
int pa_device_min_bytes;
int pa_device_max_channels;
int pa_device_min_channels;
clear_screen();
if(((unsigned int)ui.network_flag&NET_RX_INPUT) != 0)
  {
  lir_text(0,0,"Input from the network is currently selected.");
  goto not_now;
  }
if( ((ui.rx_ad_channels == 0 || ui.rx_addev_no == UNDEFINED_DEVICE_CODE)
                               && ui.rx_addev_no != DISABLED_DEVICE_CODE) ||
    ((ui.rx_max_da_channels == 0 || ui.rx_dadev_no == UNDEFINED_DEVICE_CODE)
                               && ui.rx_dadev_no != DISABLED_DEVICE_CODE))
  {
  lir_text(0,0,"The RX input and output must be set up before the TX.");
not_now:;
  lir_text(0,1,"TX is now disabled.");
  ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
  ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
  lir_text(10,3,press_any_key);
  await_processed_keyboard();
  return;
  }
if(ui.tx_ad_channels == 0)ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
if(ui.tx_da_channels == 0)ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
sndlog=fopen(tx_logfile_name, "w");
if(sndlog == NULL)
  {
  lirerr(1016);
  return;
  }
write_log=TRUE;
begin_tx_set_io:;
clear_screen();
if(kill_all_flag)goto set_tx_io_errexit;
lir_text(0,0,"The current setting is:");
lir_text(0,1,"Use RX setup menu to toggle between native ALSA and alsa-oss.");
settextcolor(10);
if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
  {
  lir_text(24,0,"native ALSA");
  }
else
  {
  lir_text(24,0,"alsa-oss   ");
  }
sprintf(s,"ALSA");
settextcolor(7);
lir_text(37,0,s);
clear_lines(2,4);
line=4;
// **************************************************************
// Show the current TX output settings on the screen.
// **************************************************************
lir_text(0,line,"Linrad TX output from:");
dev_valid=FALSE;
settextcolor(10);
sprintf(s,"UNKNOWN (no=%d)",ui.tx_dadev_no);
tmptxt=s;
if(ui.tx_dadev_no == UNDEFINED_DEVICE_CODE)
  {
  tmptxt="NOT YET SELECTED";
  goto show_txda;
  }
if( (ui.use_alsa&PORTAUDIO_TX_OUT) != 0)
  {
  portaudio_start(110);
  err=pa_get_device_info (ui.tx_dadev_no,
                        TXDA,
                        &pa_device_name,
                        &pa_device_hostapi,
                        &pa_device_max_speed,
                        &pa_device_min_speed,
                        &pa_device_max_bytes,
                        &pa_device_min_bytes,
                        &pa_device_max_channels,
                        &pa_device_min_channels);
  if (err == 0 )
    {
    sprintf(s,"SOUNDCARD device   = %s  ", pa_device_name );
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    line++;
    sprintf(s,"device  number     = %d", ui.tx_dadev_no);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    line++;
    sprintf(s,"hostapi            = Portaudio (%s) ", pa_device_hostapi);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    tmptxt=NULL;
    dev_valid=TRUE;
    }
  else
    {
    sprintf(s,"INVALID DEVICE (Portaudio device number= %d )", ui.tx_dadev_no );
    tmptxt=s;
    ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
    }
  }
else
  {
  if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
    {
    err=alsa_get_dev_names (ui.tx_dadev_no);
    if(err < 0)
      {
      sprintf(s,"invalid card/device  ( alsa-device-seq-nmbr = %i )",
                                                          ui.tx_dadev_no );
      tmptxt=s;
      }
    else
      {
      sprintf(s,"Soundcard =%s",alsa_dev_soundcard_name);
      lir_text(24,line,s);
      settextcolor(7);
      line++;
      SNDLOG"Initial TXout setting: %s\n",s);
      sprintf(s,"Dev name = %s",alsa_dev_name);
      lir_text(24,line,s);
      line++;
      SNDLOG"%s\n",s);
      sprintf(s,"Pcm name = %s",alsa_dev_plughw_pcm_name);
      lir_text(24,line,s);
      line++;
      SNDLOG"%s\n",s);
      sprintf(s,"Seq nmbr = %i",alsa_dev_seq_nmbr);
      lir_text(24,line,s);
      SNDLOG"%s\n",s);
      tmptxt=NULL;
      dev_valid=TRUE;
      }
    }
  else
    {
#if HAVE_OSS == 1 
    if(dev_name[0] != 'u')make_devname(ui.tx_dadev_no, dev_name);
    if(kill_all_flag)goto set_tx_io_errexit;
    sprintf(s,"Device name =%s",dev_name);
    tmptxt=s;
    dev_valid=TRUE;
#endif
    }
  }  
show_txda:;
if(kill_all_flag)goto set_tx_io_errexit;
if(tmptxt)
  {
  lir_text(24,line,tmptxt);
  settextcolor(7);
  }
line++;
settextcolor(7);
if(dev_valid)
  {
  sprintf(s,"sample rate        = %i ",ui.tx_da_speed );
  SNDLOG"%s\n",s);
  lir_text(24,line,s);
  line++;
  sprintf(s,"No of output bytes = %d (%d bits)",
                ui.tx_da_bytes, 8*ui.tx_da_bytes);
  SNDLOG"%s\n",s);
  lir_text(24,line,s);
  line++;
  sprintf(s,"No of output chans = %d", ui.tx_da_channels);
  SNDLOG"%s\n",s);
  lir_text(24,line,s);
  line++;

  sprintf(s,"Associated radio   = %s",tx_soundcard_radio_names[ui.tx_soundcard_radio]);
  if(ui.transceiver_mode)sprintf(&s[strlen(s)],"  TRANSCEIVER MODE.  Si570 freq");
  if(ui.transceiver_mode==1)sprintf(&s[strlen(s)]," fixed.");
  if(ui.transceiver_mode==2)sprintf(&s[strlen(s)]," variable.");
  SNDLOG"%s\n",s);
  lir_text(24,line,s);
  line++;
  
  if(ui.transceiver_mode==1)
    {
    sprintf(s,"PTT control        =");
    if(ui.ptt_control == 0)sprintf(&s[strlen(s)]," USB."); 
    if(ui.ptt_control == 1)sprintf(&s[strlen(s)]," Pilot tone."); 
    SNDLOG"%s\n",s);
    lir_text(24,line,s);
    line++;
    }

  }
line+=2;
// **************************************************************
// Show the current TX input settings on the screen.
// **************************************************************
lir_text(0,line,"Linrad TX input to:");
settextcolor(10);
sprintf(s,"UNKNOWN (no=%d)",ui.tx_addev_no);
tmptxt=s;
dev_valid=FALSE;
if(ui.tx_addev_no == UNDEFINED_DEVICE_CODE)
  {
  tmptxt="NOT YET SELECTED";
  goto show_txad;
  }
if ( (ui.use_alsa&PORTAUDIO_TX_IN) != 0)
  {
  if(portaudio_active_flag==FALSE)i=portaudio_start(111);
  err=pa_get_device_info (ui.tx_addev_no,
                          TXAD,
                          &pa_device_name,
                          &pa_device_hostapi,
			  &pa_device_max_speed,
			  &pa_device_min_speed,
			  &pa_device_max_bytes,
			  &pa_device_min_bytes,
			  &pa_device_max_channels,
			  &pa_device_min_channels );
  if (err >= 0)
    {
    sprintf(s,"SOUNDCARD device   = %s ", pa_device_name);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    line+=1;
    sprintf(s,"device number      = %d", ui.tx_addev_no);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    line+=1;
    sprintf(s,"hostapi            = Portaudio (%s)", pa_device_hostapi);
    lir_text(24,line,s);
    SNDLOG"\n%s",s);
    dev_valid=TRUE;
    tmptxt=NULL;
    }
  else
    {
    sprintf (s,"PORTAUDIO device number %d : INVALID DEVICE ",ui.tx_addev_no );
    tmptxt=s;
    ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
    }
  line++;
  settextcolor(7);
  goto show_settings;
  }
if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
  {
  err=alsa_get_dev_names (ui.tx_addev_no);
  if(err < 0)
    {
    sprintf(s,"invalid card/device  ( alsa-device-seq-nmbr = %i )",
                                                          ui.tx_addev_no );
    tmptxt=s;
    }
  else
    {
    sprintf(s,"Soundcard =%s",alsa_dev_soundcard_name);
    lir_text(24,line,s);
    settextcolor(7);
    line++;
    SNDLOG"Initial TXin setting: %s\n",s);
    sprintf(s,"Dev name = %s",alsa_dev_name);
    lir_text(24,line,s);
    line++;
    SNDLOG"%s\n",s);
    sprintf(s,"Pcm name = %s",alsa_dev_plughw_pcm_name);
    lir_text(24,line,s);
    line++;
    SNDLOG"%s\n",s);
    sprintf(s,"Seq nmbr = %i",alsa_dev_seq_nmbr);
    lir_text(24,line,s);
    SNDLOG"%s\n",s);
    dev_valid=TRUE;
    tmptxt=NULL;
    }
  }
else
  {
#if HAVE_OSS == 1 
  if(dev_name[0] != 'u')make_devname(ui.tx_addev_no, dev_name);
  sprintf(s,"Device name =%s",dev_name);
  tmptxt=s;
  dev_valid=TRUE;
#endif
  }
show_txad:;  
if(kill_all_flag)goto set_tx_io_errexit;
if(tmptxt)
  {
  lir_text(24,line,tmptxt);
  }
line++;
show_settings:;
settextcolor(7);
if(dev_valid)
  {
  sprintf(s,"sample rate        = %i ",ui.tx_ad_speed);
  SNDLOG"%s\n",s);
  lir_text(24,line,s);
  line++;
  sprintf(s,"No of input bytes  = %d (%d bits)",
                ui.tx_ad_bytes, 8*ui.tx_ad_bytes);
  SNDLOG"%s\n",s);
  lir_text(24,line,s);
  line++;
  sprintf(s,"No of input chans  = %d", ui.tx_ad_channels);
  SNDLOG"%s\n",s);
  lir_text(24,line,s);
  }
line+=2;
sndlog_sync();
// **************************************************************
// Ask whether the user wants to change something and act accordingly.
// **************************************************************
settextcolor(7);
lir_text(0,line,"A = Change the output settings.");
line++;
lir_text(0,line,"B = Change the input settings.");
line++;
if(ui.tx_soundcard_radio > TX_SOUNDCARD_RADIO_UNDEFINED_REVERSED)
  {
  lir_text(0,line,"C = Change parameters for soundcard radio.");
  line++;
  }
lir_text(0,line,"X = To main menu.");
setup_menu:;
await_processed_keyboard();
if(kill_all_flag)goto set_tx_io_errexit;
switch(lir_inkey)
  {
  case 'A':
  ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
  ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
  clear_screen();
  line=2;
  lir_text(10,line,"SELECT HARDWARE FOR TX");
  line+=2;
  lir_text(4,line,"A = Soundcard");
  line++;
  lir_text(4,line,"B = bladeRF");
  line++;
  lir_text(4,line,"C = OpenHPSDR");
  await_processed_keyboard();
  if(kill_all_flag)goto set_tx_io_errexit;
  clear_screen();
  ui.tx_soundcard_radio=0;
  line=2;
  switch (lir_inkey)
    {
    case 'A':
    if(ui.tx_enable != 0)
      {
      if((ui.rx_addev_no >=0 && ui.rx_addev_no < SPECIFIC_DEVICE_CODES) &&
                                          (ui.network_flag&NET_RX_INPUT) == 0)
        {
        if(start_iotest(&line,RXAD) == FALSE)
          {
          if(kill_all_flag)goto set_tx_io_errexit;
          goto set_txout_exit2;
          }
        }
      if(ui.rx_dadev_no >=0 && ui.rx_dadev_no < SPECIFIC_DEVICE_CODES)
        {
        if(start_iotest(&line,RXDA) == FALSE)
          {
          if(kill_all_flag)goto set_tx_io_errexit;
          goto set_txout_exit;
          }
        }
      }  
#if DARWIN != 1
sel_tx_output:;
#endif
    ui.use_alsa&=~(PORTAUDIO_TX_IN+PORTAUDIO_TX_OUT);
    ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
    ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
#if DARWIN != 1
    lir_text(10,5,"Use Portaudio for tx output? (Y/N) =>");
    lir_text(15,7,press_f1_for_info);
    await_processed_keyboard();
    if(kill_all_flag)goto set_tx_io_errexit;;
    if(lir_inkey == F1_KEY || lir_inkey == '!')
      {
      help_message(343);
go_sel_tx_output:;
      clear_screen();
      goto sel_tx_output;
      }
    if(lir_inkey == 'Y')
      {
      ui.use_alsa|=PORTAUDIO_TX_OUT;
      }
    else
      {
      if(lir_inkey != 'N')
        {
        goto go_sel_tx_output;
        }
      } 
    i=TRUE;  
    if(portaudio_active_flag==FALSE)i=portaudio_start(112);
    if(kill_all_flag)goto set_tx_io_errexit;;
    if(!i)goto go_sel_tx_output;
#else
    ui.use_alsa|=PORTAUDIO_TX_OUT;
    i=TRUE;  
    if(portaudio_active_flag==FALSE)i=portaudio_start(113);
    if(kill_all_flag)goto set_tx_io_errexit;
    if(!i)
      {
      lir_text(10,5,"Could not start Portaudio.");
      lir_text(12,5,press_any_key);
      await_keyboard();
      goto set_txout_exit;
      }
#endif
    clear_screen();
    line=1;
    if( (ui.use_alsa&PORTAUDIO_TX_OUT) != 0)
      {
      set_portaudio(TXDA, &line);
      goto set_txout_exit;
      }
    sprintf(s,"SELECT SOUNDCARD/DEVICE FOR TX OUTPUT FROM LIST:");
    SNDLOG"%s\n",s);
    lir_text(0,line,s);
    line++;
    if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
      {
      ui.tx_dadev_no=select_alsadev(SND_PCM_STREAM_PLAYBACK, &line);
      if(kill_all_flag)goto set_txout_exit;
      if(alsa_dev_min_rate[ui.tx_dadev_no] != 
                                      alsa_dev_max_rate[ui.tx_dadev_no])
        {
        sprintf(s,"Select Tx D/A speed %d to %d",
                                   alsa_dev_min_rate[ui.tx_dadev_no], 
                                   alsa_dev_max_rate[ui.tx_dadev_no]);
        lir_text(5,line,s);
        k=0;
        while(s[k] != 0)k++;
        ui.tx_da_speed=lir_get_integer(k+7, line, 6,
                                   alsa_dev_min_rate[ui.tx_dadev_no], 
                                   alsa_dev_max_rate[ui.tx_dadev_no]);
        if(kill_all_flag)goto set_txout_exit;
        line++;
        }
      else
        {
        ui.tx_da_speed=alsa_dev_min_rate[ui.tx_dadev_no];
        }
      select_txout_format(&line);
      if(kill_all_flag)goto set_txout_exit;
      if(alsa_dev_max_bytes > 2)
        {
        if(alsa_dev_max_bytes != 4)
          {
          lirerr(327761);
          goto set_tx_io_errexit;
          }
        if(alsa_dev_min_bytes == 4)
          {
          ui.tx_da_bytes=4;
          }
        else
          {
          line++;
          lir_text(5,line,"Select tx no of bytes (2 or 4)");
alsa_get_tx_dabytes:;
          ui.tx_da_bytes=lir_get_integer(36,line,2,2,4);
          if(ui.tx_da_bytes==3)goto alsa_get_tx_dabytes;
          }
        }
      else
        {
        ui.tx_da_bytes=2;
        }
      }
    else
      {
#if HAVE_OSS == 1 
      select_ossdev(&line, TXDA);
      if(kill_all_flag)goto set_tx_io_errexit;
      if(ui.tx_enable != 0)
        {
        if(ui.tx_dadev_no == (ui.rx_addev_no&255) && ui.rx_admode == O_RDWR)
          {
          if(ui.tx_dadev_no == ui.rx_dadev_no)
            {
            lir_text(line,0,"Device used for RX input and output.");
            goto nodup;
            }
          if(ui.rx_ad_channels == 4)
            {
            lir_text(0,line,"RDWR for TX output not possible with 4 channels.");
nodup:;
            line++;
            ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
            lir_text(10,line,press_any_key);
            await_processed_keyboard();
            goto set_txout_exit;
            }
//set speed
          ui.tx_da_speed=ui.rx_ad_speed;
          ui.tx_da_bytes=2;
          if( (ui.rx_input_mode&DWORD_INPUT) != 0)ui.tx_da_bytes=4;
          ui.tx_da_channels=2;
          }
        }
      else
        {
        if(dev_min_wr_speed[ui.tx_dadev_no] != dev_max_wr_speed[ui.tx_dadev_no])
          {
          sprintf(s,"Select Tx D/A speed %d to %d",
            dev_min_wr_speed[ui.tx_dadev_no], dev_max_wr_speed[ui.tx_dadev_no]);
          lir_text(5,line,s);
          k=0;
          while(s[k] != 0)k++;
          ui.tx_da_speed=lir_get_integer(k+7, line, 6,
            dev_min_wr_speed[ui.tx_dadev_no], dev_max_wr_speed[ui.tx_dadev_no]);
          line++;
          }
        else
          {
          ui.tx_da_speed=dev_min_wr_speed[ui.tx_dadev_no];
          }
// Rely on the driver to allow one or two channels.
// 4Front returns 4 for a Delta44 in cooked mode so
// the "clever" code does not work.
        dev_wr_channels[ui.tx_dadev_no]=3;
        select_txout_format(&line);
        if( (dev_wr_bytes[ui.tx_dadev_no]&6) == 6)
          {
          line++;
          lir_text(5,line,"Select tx no of bytes (2 or 4)");
get_tx_dabytes:;
          ui.tx_da_bytes=lir_get_integer(36,line,2,2,4);
          if(ui.tx_da_bytes==3)goto get_tx_dabytes;
          }
        else
          {
          ui.tx_da_bytes=dev_wr_bytes[ui.tx_dadev_no]&6;
          }
        }
#endif
      }
set_txout_exit:;
    if(ui.tx_enable != 0)
      {
      if(ui.rx_dadev_no >=0 && ui.rx_dadev_no < SPECIFIC_DEVICE_CODES)
        {
        stop_iotest(RXDA);
        }
set_txout_exit2:;
      if((ui.rx_addev_no >=0 && ui.rx_addev_no < SPECIFIC_DEVICE_CODES) &&
                                         (ui.network_flag&NET_RX_INPUT) == 0)
        {
        stop_iotest(RXAD);
        }
      }  
    clear_screen();
    line=4;
    lir_text(9,line,"Select transmitter hardware to use with soundcard.");
    line+=3;
    for(i=0; i<MAX_TX_SOUNDCARD_RADIO; i++)
      {
      sprintf(s,"%d   %s",i,tx_soundcard_radio_names[i]);
      lir_text(5,line,s);
      line++;
      }
    line++;
    lir_text(0,line,"Select by line number=>");
    ui.tx_soundcard_radio=lir_get_integer(25,line,1,0,MAX_TX_SOUNDCARD_RADIO);
    break;

    case 'B':
    lir_text(0,line,"NOT YET IMPLEMENTED");
    lir_text(0,line+2,press_any_key);
    await_processed_keyboard();
    break;

    case 'C':
    lir_text(0,line,"NOT YET IMPLEMENTED");
    lir_text(0,line+2,press_any_key);
    await_processed_keyboard();
    break;
    }
  break;

  case 'B':
  clear_screen();
  line=2;
  soundcard_error_flag=TRUE;
  if(ui.tx_dadev_no == UNDEFINED_DEVICE_CODE)
    {
    lir_text(0,line,"Set tx output device first.");
    line+=2;
    lir_text(10,line,press_any_key);
    await_processed_keyboard();
    break;
    }
  if(ui.tx_enable != 0)
    {
    if((ui.rx_addev_no >=0 && ui.rx_addev_no < SPECIFIC_DEVICE_CODES) &&
                                        (ui.network_flag&NET_RX_INPUT) == 0)
      {
      if(start_iotest(&line,RXAD) == FALSE)
        {
        if(kill_all_flag)goto set_tx_io_errexit;
        goto set_txin_exit2;
        }
      }
    if(ui.rx_dadev_no >=0 && ui.rx_dadev_no < SPECIFIC_DEVICE_CODES)
      {
      if(start_iotest(&line,RXDA) == FALSE)
        {
        if(kill_all_flag)goto set_tx_io_errexit;
        goto set_txin_exit1;
        }
      }
    }
  if(ui.tx_dadev_no >=0 && ui.tx_dadev_no < SPECIFIC_DEVICE_CODES)
    {
    if(start_iotest(&line,TXDA) == FALSE)
      {
      if(kill_all_flag)goto set_tx_io_errexit;
      goto set_txin_exit;
      }
    }
  soundcard_error_flag=FALSE;
#if DARWIN != 1
sel_tx_input:;
#endif
  ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
  ui.use_alsa&=~PORTAUDIO_TX_IN;
#if DARWIN != 1
  lir_text(10,5,"Use Portaudio for tx input? (Y/N) =>");
  lir_text(15,7,press_f1_for_info);
  await_processed_keyboard();
  if(kill_all_flag)goto set_tx_io_errexit;;
  if(lir_inkey == F1_KEY || lir_inkey == '!')
    {
    help_message(343);
go_sel_tx_input:;
    clear_screen();
    goto sel_tx_input;
    }
  if(lir_inkey == 'Y')
    {
    ui.use_alsa|=PORTAUDIO_TX_IN;
    }
  else
    {
    if(lir_inkey != 'N')
      {
      goto go_sel_tx_input;
      }
    } 
  i=TRUE;  
  if(portaudio_active_flag==FALSE)i=portaudio_start(114);
  if(kill_all_flag)goto set_tx_io_errexit;;
  if(!i)goto go_sel_tx_input;
#else
   ui.use_alsa|=PORTAUDIO_TX_IN;
   i=TRUE;
  if(portaudio_active_flag==FALSE)i=portaudio_start(115);
   if(kill_all_flag)goto set_tx_io_errexit;;
   if(!i)
     {
     lir_text(10,5,"Could not start Portaudio.");
     lir_text(12,5,press_any_key);
     await_keyboard();
     goto set_txin_exit;
     }
#endif
  clear_screen();
  line=1;
  if( (ui.use_alsa&PORTAUDIO_TX_IN) != 0)
    {
    set_portaudio(TXAD, &line);
    goto set_txin_exit;
    }
  sprintf(s,"SELECT SOUNDCARD/DEVICE FOR TX INPUT FROM LIST:");
  SNDLOG"%s\n",s);
  lir_text(0,line,s);
  line++;
  if( (ui.use_alsa&NATIVE_ALSA_USED)!=0)
    {
    ui.tx_addev_no=select_alsadev(SND_PCM_STREAM_CAPTURE, &line);
    if(kill_all_flag)goto set_tx_io_errexit;
    sprintf(s,"Enter sample-rate >");
    lir_text(0,line,s);
    SNDLOG"%s\n",s);
    ui.tx_ad_speed=lir_get_integer(21, line, 6, 
                                  alsa_dev_min_rate[ui.tx_addev_no],
                                  alsa_dev_max_rate[ui.tx_addev_no]);
    if(kill_all_flag)goto set_tx_io_errexit;
    line++;
    sprintf(s,"User selected  %i",ui.tx_ad_speed);
    SNDLOG"%s\n\n",s);
//get number of bytes and check
    if(alsa_dev_max_bytes > 2)
      {
      sprintf(s,"Enter number of input bytes (2 or 4) >");
select_number_input_bytes:;
      SNDLOG"%s\n",s);
      lir_text(0,line,s);
      ui.tx_ad_bytes=lir_get_integer(39, line, 1,2,4);
      if(kill_all_flag)goto set_tx_io_errexit;
      if(ui.tx_ad_bytes==3)
        {
        settextcolor(12);
        sprintf(s,"invalid selection, try again");
        SNDLOG"%s\n",s);
        lir_text(44,line,s);
        settextcolor(7);
        goto select_number_input_bytes;
        }
      line++;
      }
    else
      {
      ui.tx_ad_bytes=2;
      SNDLOG"\n\n");
      }
    ui.tx_ad_channels=1;
    line+=2;
    }
  else
    {
#if HAVE_OSS == 1 
    select_ossdev(&line, TXAD);
    if(kill_all_flag)goto set_tx_io_errexit;
    if(ui.tx_addev_no == ui.rx_dadev_no && ui.rx_damode == O_RDWR)
      {
      if(ui.rx_max_da_bytes != 2)
        {
        ui.tx_addev_no=UNDEFINED_DEVICE_CODE;
        lir_text(0,line,"Useless device. Only 8 bit.");
        }
      else
        {
        lir_text(0,line,"Rx output parameters modified for RDWR usage");
        ui.rx_min_da_bytes=2;
        ui.tx_ad_bytes=2;
        if(ui.rx_max_da_channels == 2)
          {
          ui.rx_min_da_channels=2;
          ui.tx_ad_channels=2;
          }
        else
          {
          ui.rx_max_da_channels=1;
          ui.tx_ad_channels=1;
          }
        ui.tx_ad_speed=get_mic_speed(&line, ui.rx_min_da_speed,
                                                        ui.rx_max_da_speed);
        ui.rx_min_da_speed=ui.tx_ad_speed;
        ui.rx_max_da_speed=ui.tx_ad_speed;
        }
      line++;
      lir_text(5,line,press_any_key);
      await_processed_keyboard();
      goto set_txin_exit;
      }
// Use one channel if the device can do both one and two channels.
    if( (3 & dev_rd_channels[ui.tx_addev_no]) == 3)
      {
      ui.tx_ad_channels=1;
      }
    else
      {
      ui.tx_ad_channels=3&dev_rd_channels[ui.tx_addev_no];
      }
    if( (dev_rd_channels[ui.tx_addev_no]&3) == 3)
      {
      line++;
      lir_text(0,line,"Select microphone no of bytes (2 or 4)");
get_micbytes:;
      ui.tx_ad_bytes=lir_get_integer(39,line,2,2,4);
      if(ui.tx_ad_bytes==3)goto get_micbytes;
      line++;
      }
    else
      {
      ui.tx_ad_bytes=dev_rd_channels[ui.tx_addev_no]&3;
      }
    if(dev_min_rd_speed[ui.tx_addev_no] != dev_max_rd_speed[ui.tx_addev_no])
      {
      ui.tx_ad_speed=get_mic_speed(&line, dev_min_rd_speed[ui.tx_addev_no],
                                         dev_max_rd_speed[ui.tx_addev_no]);
      }
    else
      {
      ui.tx_ad_speed=dev_min_rd_speed[ui.tx_addev_no];
      }
#endif
    }
set_txin_exit:;
    if(ui.tx_dadev_no >=0 && ui.tx_dadev_no < SPECIFIC_DEVICE_CODES)
    {
    stop_iotest(TXDA);
    }
set_txin_exit1:;
  if(ui.tx_enable != 0)
    {
    if(ui.rx_dadev_no >=0 && ui.rx_dadev_no < SPECIFIC_DEVICE_CODES)
      {
      stop_iotest(RXDA);
      }
set_txin_exit2:;
    if((ui.rx_addev_no >=0 && ui.rx_addev_no < SPECIFIC_DEVICE_CODES) &&
                                        (ui.network_flag&NET_RX_INPUT) == 0)
      {
      stop_iotest(RXAD);
      }
    }  
  if(!soundcard_error_flag)
    {
    line=5;
    clear_screen();
    lir_text(0,line,"The tx pilot tone may control fast Rx/Tx switching.");
    line++;
    lir_text(0,line,"Set pilot tone level in dB below signal (50-130)");
    ui.tx_pilot_tone_db=lir_get_integer(49, line, 3, 50, 130);
    line+=2;
    lir_text(0,line,"Set prestart in microseconds (0-100000)");
    ui.tx_pilot_tone_prestart=lir_get_integer(39, line, 6, 0, 100000);
    line+=2;
    settextcolor(15);
    lir_text(5,line,remind_parsave);
    settextcolor(7);
    line+=2;
    lir_text(5,line,press_any_key);
    await_processed_keyboard();
    }
  break;

  case 'C':
  switch (ui.tx_soundcard_radio)
    {
    case TX_SOUNDCARD_RADIO_SI570:
    si570_tx_setup();
    break;
    }
  break;
  
  case 'X':
  goto set_tx_io_exit;

  default:
  goto setup_menu;
  }
SNDLOG"goto begin_tx_set_io\n\n");
sndlog_sync();
goto begin_tx_set_io;
set_tx_io_exit:;
SNDLOG"\nNormal end");
set_tx_io_errexit:;
SNDLOG"\n");
fclose(sndlog);
if(portaudio_active_flag==TRUE)portaudio_stop(110);
write_log=FALSE;
}


