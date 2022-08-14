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
#ifdef _MSC_VER
  #include <sys/types.h>
#endif
#include <time.h> 
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include "uidef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "fft3def.h"
#include "seldef.h"
#include "sigdef.h"
#include "screendef.h"
#include "vernr.h"
#include "thrdef.h"
#include "keyboard_def.h"
#include "blnkdef.h"
#include "caldef.h"
#include "txdef.h"
#include "options.h"
#include "padef.h"
#include "sdrdef.h"
#include "hwaredef.h"

#if(OSNUM == OSNUM_WINDOWS)
#include <windows.h>
#include <winsock.h>
#include "wscreen.h"
#include "wdef.h"
#define INVSOCK INVALID_SOCKET
HANDLE savefile_handle;
#endif

#if(OSNUM == OSNUM_LINUX)
#include "lscreen.h"
#define INVSOCK -1
#define WORD unsigned short int
#define DWORD u_int32_t

typedef struct _SYSTEMTIME {
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
} SYSTEMTIME;

#endif

// Define fseeko and ftello. Some old distributions, e.g. RedHat 9
// have the functions but not in the headers.
#ifndef fseeko
int fseeko(FILE *stream, off_t offset, int whence);
#endif
#ifndef ftello
off_t ftello(FILE *stream);
#endif       


#define REMEMBER_UNKNOWN -1
#define REMEMBER_NOTHING -2
#define REMEMBER_PERSEUS -3
#define REMEMBER_SDR14 -4


int remember_proprietery_chunk[2];



#if (USERS_EXTRA_PRESENT == 1)
extern float users_extra_update_interval;
extern double users_extra_time;
void users_extra(void);
#endif

#define MAX_FILES 256
#define MAX_NAMLEN 70

#define DISKSAVE_X_SIZE 48*text_width
#define DISKSAVE_Y_SIZE 7*text_height    
#define DISKSAVE_SCREEN_SIZE (DISKSAVE_X_SIZE*DISKSAVE_Y_SIZE)
#define DEBMEM 64


typedef struct rcvr_hdr{
  char           chunkID[4];         // ="rcvr" (chunk perseus beta0.2)
  uint32_t          chunkSize;          // chunk length
  uint32_t          nCenterFrequencyHz; // center frequency
  uint32_t          SamplingRateIdx;    // 0=125K, 1=250K, 2=500K, 3=1M, 4=2M
  time_t          timeStart;          // start time of the recording (time(&timeStart))
  unsigned short wAttenId;           // 0=0dB, 1=10dB, 2=20dB, 3=30dB
  char           bAdcPresel;         // 0=Presel Off, 1=Presel On
  char           bAdcPreamp;         // 0=ADC Preamp Off, 1=ADC Preamp ON
  char           bAdcDither;         // 0=ADC Dither Off, 1=ADC Dither ON
  char           bSpare;             // for future use (default = 0)
  char           rsrvd[16];          // for future use (default = 000..0)
  char           extra[16];          // QS1R and others may use longer chunks
  }RCVR;

// "auxi" chunk as used in the SpectraVue WAV files
typedef struct auxi_hdr{
  char       chunkID[4];  // ="auxi" (chunk rfspace)
  uint32_t   chunkSize;   // chunk length
  SYSTEMTIME StartTime;
  SYSTEMTIME StopTime;
  DWORD      CenterFreq;  //receiver center frequency
  DWORD      ADFrequency; //A/D sample frequency before downsampling
  DWORD      IFFrequency; //IF freq if an external down converter is used
  DWORD      Bandwidth;   //displayable BW if you want to limit the display to less than Nyquist band
  DWORD      IQOffset;    //DC offset of the I and Q channels in 1/1000's of a count
  DWORD      Unused1;
  DWORD      Unused2;
  }AUXI;
RCVR perseus_hdr;
AUXI sdr14_hdr;


void write_wav_header(int begin);
int open_savefile(char *s);
fpos_t wav_wrpos;
int sel_parm;
int sel_line;

void init_os_independent_globals(void)
{
FD *net_fds;
int i;
wg_waterf_zero_time=0;
net_fds=(FD*)(&netfd);
allow_wse_parport=0;
keyboard_buffer_ptr=0;
keyboard_buffer_used=0;
mouse_flag=0;
lbutton_state=0;
rbutton_state=0;
new_lbutton_state=0;
new_rbutton_state=0;
// Make sure we know that no memory space is reserved yet.
wg_waterf=NULL;
bg_waterf=NULL;
fft3_handle=NULL;
hires_handle=NULL;
fft1_handle=NULL;
blanker_handle=NULL;
baseband_handle=NULL;
afc_handle=NULL;
calmem_handle=NULL;
txmem_handle=NULL;
siganal_handle=NULL;
vga_font=NULL;
dx=NULL;
// and that analog io is closed
rx_audio_in=-1;
rx_audio_out=-1;
sdr=-1;
snd[RXDA].block_bytes=0;
snd[TXDA].block_bytes=0;
tx_audio_in=-1;
tx_audio_out=-1;
tx_flag=0;
// Clear flags
workload_reset_flag=0;
kill_all_flag=0;
lir_errcod=0;
for(i=0; i<THREAD_MAX; i++)
  {
  thread_command_flag[i]=THRFLAG_NOT_ACTIVE;  
  thread_status_flag[i]=THRFLAG_NOT_ACTIVE;  
  }
threads_running=FALSE;
write_log=FALSE;
tx_hware_fqshift=0;
rx_hware_fqshift=0;
no_of_rx_overrun_errors=0;
no_of_rx_underrun_errors=0;
no_of_tx_overrun_errors=0;
no_of_tx_underrun_errors=0;
count_rx_underrun_flag=FALSE;
for(i=0; i<MAX_NET_FD; i++)
  {
  net_fds[i]=INVSOCK;
  }
portaudio_active_flag=FALSE;
screensave_flag=FALSE;
usb2lpt_flag=FALSE;
//internal_generator_flag=1;
//internal_generator_noise=26;
//internal_generator_att=12;
internal_generator_flag=0;
internal_generator_noise=0;
internal_generator_att=0;
truncate_flag=0;
extio_handle=NULL;
extio_running=0;
refresh_screen_flag=TRUE;
ftdi_library_flag=FALSE;
libusb1_library_flag=FALSE;
libusb0_library_flag=FALSE;
rtlsdr_library_flag=FALSE;
mirisdr_library_flag=FALSE;
udev_library_flag=FALSE;
#if(OSNUM == OSNUM_LINUX)
alsa_library_flag=FALSE;
#endif
usb2lpt_flag=FALSE;
old_fdms1_ratenum=-1;
fg_truncation_error=0;
mg_meter_file=NULL;
sg_inhibit_count=MAX_SG_INHIBIT_COUNT;
corr_afc_count=MAX_CORR_AFC_COUNT;
}
 
int skip_calibration(void)
{
int rdbuf[10], chkbuf[10];
int i, mm;
if( (save_init_flag&1) == 1)
  {
// The raw file contains calibration data for fft1_filtercorr.
  i=fread(rdbuf, sizeof(int),10,save_rd_file);
  if(i != 10)goto exx;
  if(rdbuf[7] == 0)
    {
// The filtercorr function was saved in the frequency domain in 
// rdbuf[1] points.
    mm=rdbuf[1];
    }
  else
    {
// The correction function was stored in the time domain in rdbuf[7] points
    mm=rdbuf[7];
    }
  for(i=0; i<mm; i++)
    {
    if((int)fread(timf1_char, sizeof(float), 2*rdbuf[6]+1, save_rd_file)
                                                        != 2*rdbuf[6]+1)goto exx;
    }
  if(10 != fread(chkbuf, sizeof(int),10,save_rd_file))goto exx;
  for(i=0; i<10; i++)
    {
    if(rdbuf[i]!=chkbuf[i])
      {
exx:;
      lir_text(1,7,"ERROR. File corrupted");
      return FALSE;
      }
    }
  }
if( (save_init_flag&2) == 2)
  {
  i=fread(rdbuf, sizeof(int),10,save_rd_file);
  if(i != 10)goto exx;
  bal_segments=rdbuf[0];
  mm=rdbuf[3]*rdbuf[0];
  for(i=0; i<mm; i++)
    {
    if(fread(timf1_char, sizeof(float), 8, save_rd_file) !=8)goto exx;
    }
  if(fread(chkbuf, sizeof(int),10,save_rd_file)!=10)goto exx;
  for(i=0; i<10; i++)
    {
    if(rdbuf[i]!=chkbuf[i])goto exx;
    }
  }
return TRUE;
}
 
void check_line(int *line)
{
if(line[0] >= screen_last_line)
  {
  clear_lines(screen_last_line,screen_last_line);
  settextcolor(12);
  lir_text(2,screen_last_line,"SCREEN TOO LOW FOR SELECTED FONT SIZE");
  settextcolor(7);
  line[0]=screen_last_line-1;
  clear_lines(line[0],line[0]);
  }
}


void awake_screen(void)
{
lir_set_event(EVENT_SCREEN);
if(no_of_processors > 1)
  {
  screen_loop_counter=screen_loop_counter_max;
  }
}

void raw2wav(void)
{
int amplitude_factor;
int j, k, bits;
int n;
int wav_wrbytes, wavfile_bytes;
int line;
char fnam[256], s[512];
int *samp;
getin:;
clear_screen();
settextcolor(14);
lir_text(10,1,"RAW to WAV file converter. (Enter to skip.)");
settextcolor(7);
lir_text(1,2,"Input file name:");
j=lir_get_filename(17,2,fnam);
if(j==0)return;
j=open_savefile(fnam);
if(j!=0)goto getin;
clear_screen();
if(ui.rx_ad_channels != 1 && ui.rx_ad_channels != 2)
  {
  fclose(save_rd_file);
#if(OSNUM == OSNUM_WINDOWS)
  CloseHandle(savefile_handle);
#endif
  lir_text(0,0,"Can only write 1 or 2 audio channels.");
  sprintf(s,"This file has %d channels.",ui.rx_ad_channels);
  lir_text(0,2,s);
  lir_text(5,3,press_any_key);
  await_keyboard();
  return;
  }
rx_daout_channels=ui.rx_ad_channels;
genparm[DA_OUTPUT_SPEED]=ui.rx_ad_speed;
snd[RXAD].block_bytes=8192;
if((ui.rx_input_mode&DWORD_INPUT) != 0)
  {
  bits=18;
  rx_daout_bytes=3;
  wav_wrbytes=(3*snd[RXAD].block_bytes)/4;
  save_rw_bytes=18*snd[RXAD].block_bytes/32;
  }
else
  {
  bits=16;
  rx_daout_bytes=2;
  wav_wrbytes=snd[RXAD].block_bytes;
  save_rw_bytes=snd[RXAD].block_bytes;
  }
timf1p_pa=0;
rxin_isho=(short int*)timf1_char;
rxin_int=(int*)timf1_char;
rxin_char=(char*)timf1_char;
rx_read_bytes=snd[RXAD].block_bytes;
timf1_char=malloc(snd[RXAD].block_bytes);
if(timf1_char == NULL)lirerr(34214);
rawsave_tmp=malloc(save_rw_bytes);
if(rawsave_tmp==NULL)lirerr(34124);
amplitude_factor=1;
sprintf(s,"Input file %s uses %d bits per sample",fnam,bits);
lir_text(0,0,s),
line=1;
wavfile_bytes=0;
if(fg.passband_direction <0 && (ui.rx_input_mode&IQ_DATA)!=0)
  {
  lir_text(0,line,"The passband direction flag is set, I and Q will be exchanged");
  line++;
  }
if((ui.rx_input_mode&DWORD_INPUT) != 0)
  {
// This file uses 18 bits per sample. Check whether more
// than 16 bits actually contain valid data.
  lir_text(0,line,"Scanning file for largest sample.");
  line++;
  if(skip_calibration() == FALSE)goto errx;
  k=0;
  while(diskread_flag != 4)
    {
    diskread_block_counter++;
    n=fread(rawsave_tmp,1,save_rw_bytes,save_rd_file);
    if(n != save_rw_bytes)
      {
      while(n != save_rw_bytes)
        {
        rawsave_tmp[n]=0;
        n++;
        }
      diskread_flag=4;
      }
    expand_rawdat();
    for(n=0; n<snd[RXAD].block_bytes; n+=4)
      {
      samp=(int*)(&timf1_char[n]);
      if(k<abs(samp[0]))k=abs(samp[0]);
      }
    sprintf(s,"Blk %.0f  (max ampl 0x%x)",diskread_block_counter,k);
    lir_text(1,line,s);
    }
  line++;
  amplitude_factor=0x7fffffff/k;
  sprintf(s,"Headroom to saturation is a factor %d",amplitude_factor);
  lir_text(1,line,s);
  line++;
  if(amplitude_factor >= 8)
    {
    amplitude_factor=8;
    lir_text(1,line,
          "The .WAV file can be written in 16 bit format (left shifted by 2)");
    line++;
    goto gtfmt_a; 
    }
  else
    {
    if(amplitude_factor==1)
      {
      j=18;
      }
    else
      {
      j=17;
      }
    sprintf(s,"The input file uses %d significant bits",j);
    lir_text(1,line,s);
    line++;  
gtfmt_a:;
    lir_text(1,line,"Do you wish to use 24 bit .WAV format (Y/N)?");
gtfmt:;
    to_upper_await_keyboard();
    if(lir_inkey == 'N')
      {
      rx_daout_bytes=2;
      wav_wrbytes=snd[RXAD].block_bytes/2;
      }
    else
      {
      if(lir_inkey != 'Y')goto gtfmt;
      }
    }
  fclose(save_rd_file);
#if(OSNUM == OSNUM_WINDOWS)
  CloseHandle(savefile_handle);
#endif
  j=open_savefile(fnam);
  if(j!=0)lirerr(954362);
  }
wavsave_start_stop(line+1);
if(wav_write_flag < 0)
  {
  wav_write_flag=0;
  return;
  }
if(wav_write_flag == 0)return;
diskread_block_counter=0;
wavfile_bytes=48;
if(skip_calibration() == FALSE)goto errx;
while(diskread_flag != 4)
  {
  diskread_block_counter++;
  if( (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    rxin_isho=(short int*)(timf1_char);
    n=fread(rxin_isho,1,snd[RXAD].block_bytes,save_rd_file);
    if(n != snd[RXAD].block_bytes)
      {
      while(n != snd[RXAD].block_bytes)
        {
        timf1_char[n]=0;
        n++;
        }
      diskread_flag=4;
      }
    }
  else
    {  
    n=fread(rawsave_tmp,1,save_rw_bytes,save_rd_file);
    if(n != save_rw_bytes)
      {
      while(n != save_rw_bytes)
        {
        rawsave_tmp[n]=0;
        n++;
        }
      diskread_flag=4;
      }
    expand_rawdat();
    k=0;
    if(rx_daout_bytes==3)
      {
      k=0;
      for(n=0; n<snd[RXAD].block_bytes; n+=4)
        {
        timf1_char[k  ]=timf1_char[n+1];
        timf1_char[k+1]=timf1_char[n+2];
        timf1_char[k+2]=timf1_char[n+3];
        k+=3;
        }
      }
    else
      {
      for(n=0; n<snd[RXAD].block_bytes; n+=4)
        {
        samp=(int*)(&timf1_char[n]);
        samp[0]*=amplitude_factor;
        timf1_char[k  ]=timf1_char[n+2];
        timf1_char[k+1]=timf1_char[n+3];
        k+=2;
        }
      }
    }
  if(fg.passband_direction < 0 && (ui.rx_input_mode&IQ_DATA)!=0)
    {
    if(rx_daout_bytes==3)
      {
      for(j=0; j<wav_wrbytes; j+=6)
        {
        k=timf1_char[j];
        timf1_char[j]=timf1_char[j+3];
        timf1_char[j+3]=k;
        k=timf1_char[j+1];
        timf1_char[j+1]=timf1_char[j+4];
        timf1_char[j+4]=k;
        k=timf1_char[j+2];
        timf1_char[j+2]=timf1_char[j+5];
        timf1_char[j+5]=k;
        }
      }
    else  
      {
      for(j=0; j<wav_wrbytes; j+=4)
        {
        k=timf1_char[j];
        timf1_char[j]=timf1_char[j+2];
        timf1_char[j+2]=k;
        k=timf1_char[j+1];
        timf1_char[j+1]=timf1_char[j+3];
        timf1_char[j+3]=k;
        }
      }
    }
  if(fwrite(timf1_char,wav_wrbytes,1,wav_file)!=1 ||
    (unsigned int)(wavfile_bytes)+(unsigned int)(wav_wrbytes) > 0x7fffffff)
    {
    wavsave_start_stop(0);
    lir_text(1,7,"ERROR on write. File too big?");
    goto errbig;
    }
  wavfile_bytes+=wav_wrbytes;
  sprintf(s,"%.0f ",diskread_block_counter);
  lir_text(1,7,s);
  }
errbig:;  
wavsave_start_stop(0);
lir_text(1,7,"Conversion sucessful.");
errx:;
if(wavfile_bytes < 100000)
  {
  sprintf(s,"%d bytes written",wavfile_bytes);
  }
else
  {
  if(wavfile_bytes < 10000000)
    {
    sprintf(s,"%.2f kilobytes written",0.001*wavfile_bytes);
    }
  else
    {
    sprintf(s,"%.2f megabytes written",0.000001*wavfile_bytes);
    }
  }
lir_text(1,9,s);
lir_text(5,11,press_any_key);  
free(timf1_char);
free(rawsave_tmp);
fclose(save_rd_file);
#if(OSNUM == OSNUM_WINDOWS)
CloseHandle(savefile_handle);
#endif
await_keyboard();
}


int open_savefile(char *s)
{
char *ss;
char sq[160];
int i;
ss=NULL;
i=0;
while(s[i] == ' ')i++;
if(s[i] == 0)
  {
  lir_text(5,4,"No file name given.");
  goto errfile_1;
  }
#if(OSNUM == OSNUM_WINDOWS)
savefile_handle=CreateFile(s,
                FILE_GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,
                NULL);
if(savefile_handle == INVALID_HANDLE_VALUE)
  {
  sprintf(sq,"Could not find %s",s);
  lir_text(5,4,sq);
  lir_text(10,5,press_any_key);
  goto errfile_1;
  }
ss="C:\\LINRAD_DATA\\TEST.RAW";
#else
ss="/home/linrad_data/test.raw";
#endif
save_rd_file = fopen(s, "rb");
if (save_rd_file == NULL)
  {
  if(errno == EFBIG)
    {
    lir_text(5,3,"File too large.");
    }
errfile:; 
  sprintf(sq,"Can not open file %s",s);
  lir_text(5,4,sq);
  lir_text(5,5,"Name must be given with full path. Example:");
  settextcolor(14);
  lir_text(5,6,ss);
  settextcolor(7);
errfile_1:;  
  await_keyboard();
  return 1;
  }
diskread_flag=2;
// Read a Linrad.raw file
// The original file format had ui.rx_input_mode as the first data item.
// Later versions use the first item as a flag to know the
// contents of the file.
i=fread(&ui.rx_input_mode,sizeof(int),1,save_rd_file);
if(i != 1)
  {
errfile_2:;
  lir_text(5,3,"File corrupted.");
  goto errfile;
  }
// In case ui.rx_input_mode is negative the file contains more data.
if(ui.rx_input_mode < 0)
  {
  remember_proprietery_chunk[0]=ui.rx_input_mode;
  switch (ui.rx_input_mode)
    {
    case REMEMBER_UNKNOWN:
    case REMEMBER_NOTHING:
    break;
    
    case REMEMBER_PERSEUS:
    i=fread(&remember_proprietery_chunk[1],sizeof(int),1,save_rd_file);
    if(i !=1 )goto errfile_2; 
    i=fread(&perseus_hdr.nCenterFrequencyHz,1,
                              remember_proprietery_chunk[1],save_rd_file);
    if(i != remember_proprietery_chunk[1])goto errfile_2; 
    break;
    
    case REMEMBER_SDR14:
    i=fread(&remember_proprietery_chunk[1],sizeof(int),1,save_rd_file);
    if(i !=1 )goto errfile_2; 
    i=fread(&sdr14_hdr.StartTime,1,
                              remember_proprietery_chunk[1],save_rd_file);
    if(i != remember_proprietery_chunk[1] )goto errfile_2; 
    break;

    default:
    lir_text(5,3,"This Linrad version is too old");
    goto errfile_2;
    }
  i=fread(&diskread_time,sizeof(double),1,save_rd_file);
  if(i!=1)goto errfile_2;
  i=fread(&fg.passband_center,sizeof(double),1,save_rd_file);
  freq_from_file=TRUE;
  if(i!=1)goto errfile_2;
  i=fread(&fg.passband_direction,sizeof(int),1,save_rd_file);
  if(i!=1)goto errfile_2;
  if(abs(fg.passband_direction) != 1)goto errfile_2; 
  fft1_direction=fg.passband_direction;
  i=fread(&ui.rx_input_mode,sizeof(int),1,save_rd_file);
  if(i!=1)goto errfile_2;
  }
else
  {
  diskread_time=0;
  fg.passband_center=0;
  fg.passband_direction=1;
  fft1_direction=fg.passband_direction;
  remember_proprietery_chunk[0]=REMEMBER_NOTHING;
  }
if(ui.rx_input_mode >= MODEPARM_MAX)goto errfile_2;
i=fread(&ui.rx_rf_channels,sizeof(int),1,save_rd_file);
if(i!=1)goto errfile_2;
if(ui.rx_rf_channels == 2)ui.rx_input_mode|=TWO_CHANNELS;
i=fread(&ui.rx_ad_channels,sizeof(int),1,save_rd_file);
if(i!=1)goto errfile_2;
if(ui.rx_ad_channels > 4 || ui.rx_ad_channels < 1)goto errfile_2;
if(ui.rx_ad_channels != ui.rx_rf_channels &&
               ui.rx_ad_channels != 2*ui.rx_rf_channels)goto errfile_2;
i=fread(&ui.rx_ad_speed,sizeof(int),1,save_rd_file);
if(i!=1)goto errfile_2;
save_init_flag=0;
i=fread(&save_init_flag,1,1,save_rd_file);
if(i!=1)goto errfile_2;
return 0;
}

void could_not_create(char *filename, int line)
{
lir_text(0,line,"Could not create file!!");
lir_text(0,line+1,filename);
lir_text(0,line+2,"Make sure directory exists and that the filename is not");
lir_text(0,line+3,"a diredctory name.");
lir_text(0,line+4,press_any_key);
clear_await_keyboard();
}


FILE *open_parfile(int type, char *mode)
{
char s[80];
int i,j;
// Find out if there is a file with parameters for the current graph.
// Make the default graph if no file is found, else use old data.
i=0;
if(savefile_parname[0]!=0)
  {
  while(savefile_parname[i]!=0)
    {
    s[i]=savefile_parname[i];
    i++;
    }
  }  
else
  {  
  while(rxpar_filenames[rx_mode][i]!=0)
    {
    s[i]=rxpar_filenames[rx_mode][i];
    i++;
    }
  }
s[i  ]='_';
i++;
j=0;
while(graphtype_names[type][j] != 0)
  {
  s[i]=graphtype_names[type][j];
  i++;
  j++;
  }
s[i]=0;
return fopen(s, mode);
}


int read_modepar_file(int type)
{
int i, j, k, max_intpar, max_floatpar;
int *wgi;
float *wgf;
double *wgd;
char *parinfo;
FILE *wgfile;
wgfile = open_parfile(type,"r");
if (wgfile == NULL)return 0;
parinfo=malloc(4096);
if(parinfo == NULL)
  {
  lirerr(1082);
  return 0;
  }
for(i=0; i<4096; i++) parinfo[i]=0;
i=fread(parinfo,1,4095,wgfile);
fclose(wgfile);
if(i == 4095)goto txt_err;
wgi=(int*)(graphtype_parptr[type]);
k=0;
max_intpar = graphtype_max_intpar[type];
max_floatpar = graphtype_max_floatpar[type];
for(i=0; i<max_intpar; i++)
  {
  while( (parinfo[k]==' ' || parinfo[k]== '\n' ) && k<4095)k++;
  j=0;
  while(parinfo[k]== graphtype_partexts_int[type][i][j] && k<4095)
    {
    k++;
    j++;
    } 
  if(graphtype_partexts_int[type][i][j] != 0)
    {
txt_err:;    
    free(parinfo);
    return 0;
    }
  while(parinfo[k]!='[' && k<4095)k++;
  if(k>=4095)goto txt_err;
  sscanf(&parinfo[k],"[%d]",&wgi[i]);
  while(parinfo[k]!='\n' && k<4095)k++;
  if(k>=4095)goto txt_err;
  }
if(max_floatpar < 0)
  {
  wgd=(double*)(&wgi[max_intpar]);
  for(i=0; i<-max_floatpar; i++)
    {
    while(parinfo[k]==' ' ||
          parinfo[k]== '\n' )k++;
    j=0;
    while(parinfo[k]== graphtype_partexts_float[type][i][j]&&k<4095)
      {
      k++;
      j++;
      } 
    if(graphtype_partexts_float[type][i][j] != 0)goto txt_err;
    while(parinfo[k]!='[' && k<4095)k++;
    if(k>=4095)goto txt_err;
    sscanf(&parinfo[k],"[%lf]",&wgd[i]);
    if(k>=4095)goto txt_err;
    while(parinfo[k]!='\n' && k<4095)k++;
    }
  }
else
  {  
  wgf=(float*)(&wgi[max_intpar]);
  for(i=0; i<max_floatpar; i++)
    {
    while(parinfo[k]==' ' || parinfo[k]== '\n' )k++;
    j=0;
    while(parinfo[k]== graphtype_partexts_float[type][i][j])
      {
      k++;
      j++;
      } 
    if(graphtype_partexts_float[type][i][j] != 0)goto txt_err;
    while(parinfo[k]!='[')k++;
    sscanf(&parinfo[k],"[%f]",&wgf[i]);
    while(parinfo[k]!='\n')k++;
    }
  }
free(parinfo);
return 1;
}


void make_modepar_file(int type)
{
int i, max_intpar, max_floatpar;
int *wgi;
float *wgf;
double *wgd;
FILE *file;
file = open_parfile(type,"w");
if (file == NULL)
  {
  lirerr(1164);
  return;
  }
wgi=(int*)(graphtype_parptr[type]);
max_intpar = graphtype_max_intpar[type];
max_floatpar = graphtype_max_floatpar[type];
for(i=0; i<max_intpar; i++)
  {
  fprintf(file,"%s [%d]\n",graphtype_partexts_int[type][i],wgi[i]);
  }
if(max_floatpar < 0)
  {
  wgd=(double*)(&wgi[max_intpar]);
  for(i=0; i<-max_floatpar; i++)
    {
    fprintf(file,"%s [%.30f]\n",graphtype_partexts_float[type][i],wgd[i]);
    }
  }
else
  {  
  wgf=(float*)(&wgi[max_intpar]);
  for(i=0; i<max_floatpar; i++)
    {
    fprintf(file,"%s [%.15f]\n",graphtype_partexts_float[type][i],wgf[i]);
    }
  }  
parfile_end(file);
}

        
void select_namlin(char *s,FILE *file)
{
char ch;
int i,k,num, line, col;
fpos_t fileptr[MAX_FILES];
clear_screen();
if(sel_line == -1)lir_text(10,0,"SELECT A NUMBER FROM THE LIST:");
num=0;
line=2;
col=0;
get_name:;
k=1;
sprintf(s,"%2d:",num);
if(fgetpos(file, &fileptr[num]))
  {
  lirerr(1118);
  return;
  }
i=2;
while(k==1 && s[i] != 10 && i<MAX_NAMLEN)
  {
  i++;
  k=fread(&s[i],1,1,file);
  if(s[i]==13)s[i]=10;
  if(i==3 && s[i]==10)goto get_name;
  }
ch=s[i];
s[i]=0;
if(i>3)
  {
  if(sel_line == -1)lir_text(col,line,s);
  while(k==1 && ch != 10 )
    {
    k=fread(&ch,1,1,file);
    if(ch==13)ch=10;
    }
  num++;
  line++;
  }
if(line>screen_last_line)
  {
  line=2;
  col+=MAX_NAMLEN +5;
  if( col+MAX_NAMLEN+5 > screen_last_col)goto listfull;
  }
if(k == 1 && num < MAX_FILES)goto get_name;
listfull:;
if(sel_line == -1)sel_line=lir_get_integer(42, 0, 3, 0,num-1);
if(kill_all_flag) return;
if(fsetpos(file, &fileptr[sel_line]))lirerr(1119);
clear_screen();
}

void get_parfile_name(char *s)
{
int i, ptr, eq;
ptr=0;
while(s[ptr]!=0 && s[ptr]!=' ')
  {
  savefile_name[ptr]=s[ptr];
  if(s[ptr]==10 || s[ptr]==13 )
    {
    savefile_parname[0]=0;
    savefile_name[ptr]=0;
    s[ptr]=0;
    return;
    }
  ptr++;
  if(ptr > SAVEFILE_MAX_CHARS)lirerr(1104);
  }
savefile_name[ptr]=0;
s[ptr]=0;
ptr++;
while(s[ptr] == ' ')ptr++;
i=0;
eq=1;
while(s[ptr]!=0 && 
      s[ptr]!=' ' && 
      s[ptr] != 10 && 
      s[ptr] != 13 && 
      i<SAVEFILE_MAX_CHARS)
  {
  savefile_parname[i]=s[ptr];
  if(s[i] != s[ptr])eq=0; 
  i++;
  ptr++;
  }
if(s[i] != ' ')eq=0;  
if(i!=0 && eq==1)lirerr(1120);  
if(i>= SAVEFILE_MAX_CHARS)lirerr(1115);  
savefile_parname[i]=0;
}

int init_wavread(int sel_file)
{
char s[256], sq[300], si[16];
FILE *file;
int i, k, n, chunk_size;
short int ishort, format_tag;
int errnr, expert_sdr2_flag;
char *tmpbuf, *tmp2;
char *sdr_console_time_string="CurrentTimeUTC=";
int console_time_string_length;
char *sdr_console_freq_string="RadioCenterFreq=";
int console_freq_string_length;
char *xml_version_string="<?xml version=\"1.0\"?>";
int xml_version_string_length;
long ll;
int j;    
clear_screen();
expert_sdr2_flag=FALSE;
file = fopen("adwav", "rb");
if (file == NULL)
  {
  file = fopen("adwav.txt", "rb");
  if (file == NULL)
    {
empty_error:;
    help_message(313);
    return 1;
    }
  }
if(sel_file != 0)
  {  
  select_namlin(s,file);
  if(kill_all_flag) return 1;
  }  
for(i=0; i<256; i++)s[i]=0;
k=fread(&s , 1, 256, file);  
fclose(file);
if(k<2)goto empty_error;
i=0;
while( i<255 && (s[i] == ' ' || s[i] == 10 || s[i] == 13))i++;
get_parfile_name(&s[i]);
if(lir_errcod != 0)return 1;
for(n=i;n<256;n++)if(s[n]==10 || s[n]==13)s[n]=0;
#if(OSNUM == OSNUM_WINDOWS)
savefile_handle=CreateFile(&s[i],
                FILE_GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,
                NULL);
if(savefile_handle == INVALID_HANDLE_VALUE)goto rdfile_x;
#endif
save_rd_file = fopen(&s[i], "rb");
if (save_rd_file == NULL)
  {
#if(OSNUM == OSNUM_WINDOWS)
rdfile_x:;
#endif
  sprintf(sq,"Could not find %s",&s[i]);
  lir_text(5,4,sq);
rdfile_x1:;
  lir_text(10,5,press_any_key);
  await_keyboard();
  return 1;
  }
diskread_time=0;
fg.passband_center=0;
// Read the WAV file header.
// First 4 bytes should be "RIFF"
si[4]=0;
k=fread(&si,sizeof(int),1,save_rd_file);
errnr=0;
if(k !=1)
  {
headerr:;  
  lir_text(5,5,s);
  sprintf(s,"Error in .wav file header [%d]",errnr);  
  lir_text(5,4,s);
  goto rdfile_x1;
  }
if(strncmp(si,"RIFF",sizeof(int)) != 0)
  {
  errnr=1;
  goto headerr;
  }
// Read file size (we do not need it)
k=fread(&i,sizeof(int),1,save_rd_file);
if(k!=1)
  {
  errnr=2;
  goto headerr;
  }
// Now we should read "WAVE"
k=fread(&si,sizeof(int),1,save_rd_file);
errnr=2;
if(k !=1 || strncmp(si,"WAVE",sizeof(int)) != 0)goto headerr;
// Now we should read "fmt "
k=fread(&si,sizeof(int),1,save_rd_file);
errnr=3;
if(k !=1 || strncmp(si,"fmt ",sizeof(int)) != 0)goto headerr;
// read the size of the format chunk.
k=fread(&chunk_size,sizeof(int),1,save_rd_file);
errnr=4;
if(k !=1 )goto headerr; 
// read the type of data (Format Tag). We only recognize PCM data!
k=fread(&format_tag,sizeof(short int),1,save_rd_file);
errnr=5;
if(k !=1 || (format_tag != 1 && format_tag != 3))
  {
  lir_text(5,3,"Unknown wFormatTag.");
  goto headerr;
  }
// Read no of channels
k=fread(&ishort,sizeof(short int),1,save_rd_file);
errnr=6;
if(k !=1 || ishort < 1 || ishort > 2)goto headerr;  
ui.rx_ad_channels=ishort;
// Read the sampling speed.
k=fread(&ui.rx_ad_speed,sizeof(int),1,save_rd_file);
errnr=7;
if(k !=1 )goto headerr; 
// Read average bytes per second (do not care what it is)
errnr=8;
k=fread(&i,sizeof(int),1,save_rd_file);
if(k !=1 )goto headerr; 
// Read block align to get 8 or 16 bit format.
k=fread(&ishort,sizeof(short int),1,save_rd_file);
errnr=9;
if(k !=1 )goto headerr; 
ishort/=ui.rx_ad_channels;
  switch (ishort)
  {
  case 1:
// byte int input  
  errnr=10;
  if(format_tag != 1)goto headerr;
  ui.rx_input_mode=BYTE_INPUT; 
  break;
  
  case 2:
// 16 bit int input
  errnr=11;
  if(format_tag != 1)goto headerr;
  ui.rx_input_mode=0;
  break;
  
  case 3:
// 24 bit int input  
  errnr=12;
  if(format_tag != 1)goto headerr;
  ui.rx_input_mode=BYTE_INPUT+DWORD_INPUT;
  break;
  
  case 4:  
  errnr=13;
  if(format_tag == 1)
    {
// 32 bit int input    
    ui.rx_input_mode=QWORD_INPUT+DWORD_INPUT;
    break;
    }
  if(format_tag != 3)goto headerr;
// 32 bit float input
  ui.rx_input_mode=FLOAT_INPUT+DWORD_INPUT;
  break;
  
  default:
  goto headerr;
  }
// Skip extra bytes if present.
chunk_size-=14;
skip_chunk:;
errnr=11;
while(chunk_size != 0)
  {
  k=fread(&i,1,1,save_rd_file);
  if(k !=1 )goto headerr; 
  chunk_size--;
  }
// Read chunks until we encounter the "data" string (=0x61746164).
// Look for Perseus or SDR-14 headers.
errnr=12;
remember_proprietery_chunk[0]=REMEMBER_NOTHING;
next_chunk:;
k=fread(&si,sizeof(int),1,save_rd_file);
if(k !=1 )goto headerr; 
errnr=13;
// test for "rcvr"
if(strncmp(si,"rcvr",sizeof(int)) == 0)
  {
  if(remember_proprietery_chunk[0]!=REMEMBER_NOTHING)
    {
    lirerr(1522318);
    return 98;
    }
  remember_proprietery_chunk[0]=REMEMBER_PERSEUS;
  k=fread(&chunk_size,sizeof(int),1,save_rd_file);
  if(k !=1 )goto headerr; 
  if(chunk_size > (int)sizeof(RCVR) )goto headerr;
  remember_proprietery_chunk[1]=chunk_size;
  k=fread(&perseus_hdr.nCenterFrequencyHz,1,chunk_size,save_rd_file);
  if(k != chunk_size)goto headerr;
  diskread_time=perseus_hdr.timeStart;
  fg.passband_center=0.000001*perseus_hdr.nCenterFrequencyHz;
  freq_from_file=TRUE;
  goto next_chunk;
  }
errnr=14; 
// test for "auxi"
if(strncmp(si,"auxi",sizeof(int)) == 0)
  {
  if(remember_proprietery_chunk[0]!=REMEMBER_NOTHING)
    {
    lirerr(1522319);
    return 99;
    }
  k=fread(&chunk_size,sizeof(int),1,save_rd_file);
  if(k !=1 )goto headerr; 
  tmpbuf=malloc(chunk_size*sizeof(char));
  if(tmpbuf == NULL)
    {
    lirerr(373896);
    return 99;
    }
  k=fread(tmpbuf,1,chunk_size,save_rd_file);
  if(k != chunk_size)
    {
headerr_free:;    
    free(tmpbuf);
    goto headerr;
    }
  k=sizeof(AUXI)-sizeof(sdr14_hdr.chunkID)-sizeof(sdr14_hdr.chunkSize);
  tmp2=(char*)&sdr14_hdr.StartTime;
  for(i=0; i<k; i++)
    {
    tmp2[i]=tmpbuf[i];
    }
// This could be XML from SDR console.
  i=1;
  k=2;
// Remove the zeroes.
  while(k < chunk_size)
    {
    tmpbuf[i]=tmpbuf[k];
    i++;
    k+=2;
    }
  xml_version_string_length=strlen(xml_version_string);
  k=strncmp(tmpbuf, xml_version_string,xml_version_string_length);
  if(k==0) 
    {
    chunk_size/=2;
    i=0;
    console_time_string_length=strlen(sdr_console_time_string);
    while(i < chunk_size-console_time_string_length)
      {
      k=strncmp(&tmpbuf[i], sdr_console_time_string, 
                                                 console_time_string_length);
      if(k==0)goto console_time;
      i++;
      }
    goto headerr_free;
console_time:;
    i+=console_time_string_length;
    while(tmpbuf[i]!=' ')i++;
    i++;
    sscanf(&tmpbuf[i],"%d:%d:%d",&i,&j,&k);
    diskread_time=3600.*i+60*j+k;
    i=0;
    console_freq_string_length=strlen(sdr_console_freq_string);
    while(i < chunk_size-console_freq_string_length)
      {
      k=strncmp(&tmpbuf[i], sdr_console_freq_string, 
                                                 console_freq_string_length);
      if(k==0)goto console_freq;
      i++;
      }
    goto headerr_free;
console_freq:;
    i+=console_freq_string_length+1;
    sscanf(&tmpbuf[i],"%ld",&ll);
    fg.passband_center=0.000001*(float)(ll);
    freq_from_file=TRUE;
    free(tmpbuf);
    goto next_chunk;
    }
  free(tmpbuf);  
  remember_proprietery_chunk[0]=REMEMBER_SDR14;
  remember_proprietery_chunk[1]=chunk_size;
  diskread_time=sdr14_hdr.StartTime.wHour*3600.+
                sdr14_hdr.StartTime.wMinute*60.+
                sdr14_hdr.StartTime.wSecond;
  fg.passband_center=0.000001*sdr14_hdr.CenterFreq;
  freq_from_file=TRUE;
  goto next_chunk;
  }
errnr=15;
// test for "esdr"
if(strncmp(si,"esdr",sizeof(int)) == 0)
  {
  expert_sdr2_flag=TRUE;
  k=fread(&chunk_size,sizeof(int),1,save_rd_file);
  if(k !=1 )goto headerr; 
  goto skip_chunk;
  }
errnr=25;
// Look for DATA 
if(strncmp(si,"data",sizeof(int)) != 0)
  {
// Unknown. Get the size and skip.  
  k=fread(&chunk_size,sizeof(int),1,save_rd_file);
  if(k !=1 )goto headerr; 
  goto skip_chunk;
  }
// Read how much data we have ( do not care)
k=fread(&i,sizeof(int),1,save_rd_file);
if(k !=1 )goto headerr; 
diskread_flag=2;
save_init_flag=0;
fg.passband_direction=0;
fft1_direction=fg.passband_direction;
if(expert_sdr2_flag)
  {
  k=fread(&chunk_size,1,1,save_rd_file);  // to read SunSDR2 files.
  ui.rx_input_mode|=DIGITAL_IQ;
  }
return 0;
}  

void parfile_end(FILE *file)
{
fprintf(file,"\nChange only between brackets.");
fprintf(file,"\nIf file has errors, Linrad will ignore file and use defaults");
fprintf(file,"\nor prompt for a complete set of new parameters\n");
fprintf(file,"\n%s",PROGRAM_NAME);
fclose(file);
}


int init_diskread(int sel_file)
{
char s[256];
FILE *file;
int i, n, kk;
if(sel_file != -1)
  {
  sel_parm=sel_file;
  sel_line=-1;
  }
kk=sel_parm;
if( wav_read_flag != 0)
  {
  i=init_wavread(kk);
  return i;
  }     
clear_screen();
file = fopen("adfile", "rb");
if (file == NULL)
  {
  file = fopen("adfile.txt", "rb");
  if (file == NULL)
    {
emptyerror:;    
    help_message(314);
    return 1;
    }
  }
if(kk != 0)
  {  
  select_namlin(s,file);
  if(kill_all_flag) return 1;
  }  
for(i=0; i<256; i++)s[i]=0;
i=fread(&s , 1, 256, file);  
fclose(file);
if(i < 2)goto emptyerror;
i=0;
while( i<255 && (s[i] == ' ' || s[i] == 10 || s[i] == 13))i++;
get_parfile_name(&s[i]);
if(lir_errcod != 0)return 1;
for(n=i;n<256;n++)if(s[n]==10 || s[n]==13)s[n]=0;
return open_savefile(&s[i]);
}  

void complete_filename(int i, char *s, char *fxt, char *dir, char *fnm)
{
#if(OSNUM == OSNUM_WINDOWS)
int j, k;
j=0;
k=0;
#endif
if( i <= 4 || (i > 4 && strcmp(&s[i-4],fxt) !=0))
  {
  strcpy(&s[i],fxt);
  }
#if(OSNUM == OSNUM_LINUX)
if(s[0] != '/' && s[0]!= '.')
#endif
#if(OSNUM == OSNUM_WINDOWS)
while(s[k]!=0)
  {
  if(s[k]==':')j=1;
  k++;
  }
if(j==0)
#endif  
  {
  sprintf(fnm,"%s%s",dir,s);  
  }
else
  {
  strcpy(fnm,s);
  }
}

void update_indicator(unsigned char color)
{
char s[3];
int i;
s[2]=0;
if(diskwrite_flag == TRUE)
  {
  s[0]='S';
  }
else
  {
  s[0]=' ';
  }
if(wav_write_flag == 1)
  {
  s[1]='W';
  }
else
  {
  s[1]=' ';
  }
settextcolor(color);
i=wg.xright-2*text_width-2;
if(i<2)i=2;
lir_pixwrite(i,wg.yborder+2,s);
if(numinput_flag != 0)
  {
  if(color == 12)settextcolor(15);
  lir_pixwrite(numinput_xpix,numinput_ypix,numinput_txt);
  }
settextcolor(7);
}

void disksave_start(void)
{
// Open or close save_rd_file or save_wr_file.
// If mode=TRUE, we operate on save_wr_file.
FILE *fc_file, *iq_file;
int i, k;
int wrbuf[10];
double dt1;
char raw_filename[160];
char s[160];
char *disksave_screencopy; 
if(diskwrite_flag == 0)
  {
  pause_thread(THREAD_SCREEN);
  disksave_screencopy=malloc(DISKSAVE_SCREEN_SIZE);
  if(disksave_screencopy == NULL)
    {
    lirerr(1047);
    return;
    }
  lir_getbox(0,0,DISKSAVE_X_SIZE,DISKSAVE_Y_SIZE,(size_t*)disksave_screencopy);
  lir_fillbox(0,0,DISKSAVE_X_SIZE,DISKSAVE_Y_SIZE,10);
  lir_text(0,0,"Enter file name for raw data.");
  lir_text(0,1,"ENTER to skip.");
  lir_text(0,2,"=>");
  i=lir_get_filename(2,2,s);          
  if(kill_all_flag) return;
  if(i==0)
    {
    diskwrite_flag=FALSE;
    }
  else 
    {
    complete_filename(i, s, ".raw", RAWDIR, raw_filename);
    save_wr_file = fopen( raw_filename, "wb");
    if(save_wr_file == NULL)
      {
errx:;
      could_not_create(raw_filename,3);
      if(kill_all_flag) return;
      diskwrite_flag=FALSE;
      }
    else
      {
// Write REMEMBER_UNKNOWN as a flag telling what version number of 
// Linrad raw data file we are writing.        
      if(diskread_flag < 2)
        {
        remember_proprietery_chunk[0]=REMEMBER_UNKNOWN;
        }     
      i=fwrite(&remember_proprietery_chunk[0],sizeof(int),1,save_wr_file); 
      if(i != 1)
        {
wrerr:;
        fclose(save_wr_file);
        goto errx;
        }
      switch(remember_proprietery_chunk[0])
        {
        case REMEMBER_NOTHING:
        case REMEMBER_UNKNOWN:
        break;


        case REMEMBER_PERSEUS:
        i=fwrite(&remember_proprietery_chunk[1],sizeof(int),1,save_wr_file);
        if(i !=1 )goto wrerr; 
        i=fwrite(&perseus_hdr.nCenterFrequencyHz,1,
                              remember_proprietery_chunk[1],save_wr_file);
        if(i != remember_proprietery_chunk[1] )goto wrerr; 
        break;
    
        case REMEMBER_SDR14:
        i=fwrite(&remember_proprietery_chunk[1],sizeof(int),1,save_wr_file);
        if(i !=1 )goto wrerr; 
        i=fwrite(&sdr14_hdr.StartTime,1,
                              remember_proprietery_chunk[1],save_wr_file);
        if(i != remember_proprietery_chunk[1] )goto wrerr; 
        break;

        default:
        lirerr(3850232);
        goto wrerr;
        }
      if(diskread_flag < 2)
        {
        i=lir_get_epoch_seconds();
        dt1=i;
        i%=24*3600;
        }
      else  
        {
        i=diskread_time+diskread_block_counter*
                                     snd[RXAD].block_frames/ui.rx_ad_speed;
        i%=24*3600;
        dt1=i;
        }
      i=fwrite(&dt1,sizeof(double),1,save_wr_file);
      if(i != 1)goto wrerr;
      i=fwrite(&fg.passband_center,sizeof(double),1,save_wr_file);
      if(i != 1)goto wrerr;
      i=fwrite(&fg.passband_direction,sizeof(int),1,save_wr_file);
      if(i != 1)goto wrerr;
// Write mode info so we know how to process data when reading 
      k=ui.rx_input_mode&(TWO_CHANNELS+DWORD_INPUT+IQ_DATA+DIGITAL_IQ);
      i=fwrite(&k,sizeof(int),1,save_wr_file);
      if(i != 1)goto wrerr;
      i=fwrite(&ui.rx_rf_channels,sizeof(int),1,save_wr_file);
      if(i != 1)goto wrerr;
      i=fwrite(&ui.rx_ad_channels,sizeof(int),1,save_wr_file);
      if(i != 1)goto wrerr;
      i=fwrite(&ui.rx_ad_speed,sizeof(int),1,save_wr_file);
      if(i != 1)goto wrerr;
      save_init_flag=0;
      iq_file=NULL;
      fc_file=NULL;
      if( (fft1_calibrate_flag&CALAMP) == CALAMP)
        {
        make_filfunc_filename(s);
        fc_file = fopen(s, "rb");
        save_init_flag=1;
        }
      if(  (ui.rx_input_mode&IQ_DATA) != 0 && 
           (fft1_calibrate_flag&CALIQ) == CALIQ)
        {
        make_iqcorr_filename(s);
        iq_file = fopen(s, "rb");
        save_init_flag+=2;
        }
      i=fwrite(&save_init_flag,1,1,save_wr_file);
      if(i != 1)goto wrerr;

      if( (save_init_flag & 1) == 1)
        {
        if(fc_file != NULL)
          {
rd1:;
          i=fread(&s,1,1,fc_file);
          if(i != 0)
            {
            i=fwrite(&s,1,1,save_wr_file);
            if(i != 1)goto wrerr;
            goto rd1;
            }
          fclose(fc_file);
          fc_file=NULL;
          }
        else
          {
          wrbuf[0]=fft1_n;
          wrbuf[1]=fft1_size;
          wrbuf[2]=rx_mode;
          wrbuf[3]=ui.rx_input_mode;
          wrbuf[4]=genparm[FIRST_FFT_VERNR];
          wrbuf[5]=ui.rx_ad_speed;
          wrbuf[6]=ui.rx_rf_channels;
          wrbuf[7]=fft1_size;
          for(i=8; i<10; i++)wrbuf[i]=0;
          i=fwrite(wrbuf, sizeof(int),10,save_wr_file);
          if(i != 10)goto wrerr;
          i=fwrite(fft1_filtercorr, twice_rxchan*sizeof(float),
                                                    fft1_size, save_wr_file);  
          if(i != fft1_size)goto wrerr;
          i=fwrite(fft1_desired,sizeof(float),fft1_size,save_wr_file);
          if(i != fft1_size)goto wrerr;
          i=fwrite(wrbuf, sizeof(int),10,save_wr_file);
          if(i!=10)goto wrerr;
          }
        }    
      if( (save_init_flag & 2) == 2)
        {
        if(iq_file != NULL)
          {
rd2:;
          i=fread(&s,1,1,iq_file);
          if(i != 0)
            {
            i=fwrite(&s,1,1,save_wr_file);
            if(i != 1)goto wrerr;
            goto rd2;
            }
          fclose(iq_file);
          iq_file=NULL;
          }
        else  
          {
          wrbuf[0]=fft1_size;
          wrbuf[1]=ui.rx_input_mode&IQ_DATA;
          wrbuf[2]=ui.rx_ad_speed;
          wrbuf[3]=ui.rx_rf_channels;
          wrbuf[4]=FOLDCORR_VERNR;
          for(i=5; i<10; i++)wrbuf[i]=0;
          i=fwrite(wrbuf, sizeof(int),10,save_wr_file);
          if(i!=10)goto wrerr;
          i=fwrite(fft1_foldcorr, twice_rxchan*sizeof(float),
                                               4*fft1_size, save_wr_file);  
// We write four times too much data for foldcorr. ööööööööö
// make a new FOLDCORR_VERNR and write future files in the öööö
// proper size.  öööö
          if(i != 4*fft1_size)goto wrerr;
          i=fwrite(wrbuf, sizeof(int),10,save_wr_file);
          if(i!=10)goto wrerr;
          }
        }
      diskwrite_flag=TRUE;
// Create a separate thread for disk writes. They can take very
// long time particularly under Windows.
      lir_init_event(EVENT_WRITE_RAW_FILE);
      linrad_thread_create(THREAD_WRITE_RAW_FILE);
        
      }
    }
  lir_putbox(0,0,DISKSAVE_X_SIZE,DISKSAVE_Y_SIZE,(size_t*)disksave_screencopy);
  free(disksave_screencopy);
  resume_thread(THREAD_SCREEN);
  }
//update_indicator();
}

void disksave_stop(void)
{
// Open or close save_rd_file or save_wr_file.
// If mode=TRUE, we operate on save_wr_file.
if(diskwrite_flag != 0)
  {
  diskwrite_flag=0;
  linrad_thread_stop_and_join(THREAD_WRITE_RAW_FILE);
  lir_close_event(EVENT_WRITE_RAW_FILE);
  }
//update_indicator();
}

void wavsave_start_stop(int line)
{
int i;
char s[160],wav_filename[160];
char *wav_write_screencopy; 
if( wav_write_flag == 0)
  {
  pause_thread(THREAD_SCREEN);
  wav_write_screencopy=malloc(DISKSAVE_SCREEN_SIZE);
  if(wav_write_screencopy == NULL)
    {
    lirerr(1031);
    return;
    }
  lir_getbox(0,line*text_height,DISKSAVE_X_SIZE,
                      DISKSAVE_Y_SIZE,(size_t*)wav_write_screencopy);
  lir_fillbox(0,line*text_height,DISKSAVE_X_SIZE,DISKSAVE_Y_SIZE,10);
  lir_text(0,line,"Enter name for audio output file");
  lir_text(0,line+1,"ENTER to skip.");
  if(wavfile_squelch_flag)
    {
    lir_text(16,line+1,"Squelch open ONLY.");
    }
  lir_text(0,line+2,"=>");
  i=lir_get_filename(2,line+2,s);          
  if(kill_all_flag) return;
  if(i != 0)
    {
    complete_filename(i, s, ".wav", WAVDIR, wav_filename);
    wav_file = fopen( wav_filename, "wb");
    if(wav_file == NULL)
      {
      could_not_create(wav_filename, line+2);
      wav_write_flag = -1;
      }
    else
      {
// Write the .wav header, but make the file size gigantic
// We will write the correct size when closing, but if
// system crashes we can fix the header and recover data.
// Speed, channels and bits will be ok. Size will indicate loss of data. 
      write_wav_header(TRUE);
      fflush(wav_file);
      lir_sync();
      wav_write_flag=1;
      }
    }
  lir_putbox(0,line*text_height,DISKSAVE_X_SIZE,
                       DISKSAVE_Y_SIZE,(size_t*)wav_write_screencopy);
  free(wav_write_screencopy);
  resume_thread(THREAD_SCREEN);
  }
else
  {
  wav_write_flag = 0;
  lir_sleep(200000);
  write_wav_header(FALSE);
  fclose(wav_file);
  lir_sync();
  }
lir_refresh_screen();
//update_indicator();
}

void write_wav_header(int begin)
{
int i;
// Write the header for a .wav file using the current output
// settings. 
// First point to start of file.
off_t filesize;
off_t ii;
if(begin == TRUE)
  {
  if(fgetpos(wav_file, &wav_wrpos))
    {
    lirerr(1114);
    return;
    }
  filesize=0x7fffffff;  
  }
else
  {
  fseek(wav_file, 0, SEEK_END);
#if OSNUM == OSNUM_LINUX
  filesize=ftello(wav_file);
#endif
#if OSNUM == OSNUM_WINDOWS
  filesize=ftell(wav_file);
#endif
  if(fsetpos(wav_file, &wav_wrpos) != 0)
    {
    lirerr(1111);
    return;
    }
  }
// First 4 bytes should be "RIFF"
if(fwrite("RIFF",sizeof(int),1,wav_file)!= 1)
  {
headerr:;
  lirerr(1112);
  return;
  }
// Now file size in bytes -8
// The format chunk uses 16 bytes. 
ii=filesize-8;
if(ii > 0x7fffffff)ii=0x7fffffff;
if(fwrite(&ii,sizeof(int),1,wav_file)!= 1)goto headerr;
// The next code word pair is "WAVEfmt "
if(fwrite("WAVEfmt ",sizeof(int),2,wav_file)!= 2)goto headerr;
// Write the size of the format chunk = 16
// ******************************************************
// pos 16-19
i=16;
if(fwrite(&i,sizeof(int),1,wav_file)!= 1)goto headerr;
// ******************************************************
// pos 20-21
// Write the type of data (Format Tag = 1 for PCM data)
i=1;
if(fwrite(&i,sizeof(short int),1,wav_file)!=1)goto headerr;
// ******************************************************
// pos 22-23
// Write no of channels
if(fwrite(&rx_daout_channels,sizeof(short int),1,wav_file)!=1)goto headerr;
// ******************************************************
// pos 24-27
// Write the output speed. 
if(fwrite(&genparm[DA_OUTPUT_SPEED],sizeof(int),1,wav_file)!=1)goto headerr;
// ******************************************************
// pos 28-31
// Write average bytes per second.
i=genparm[DA_OUTPUT_SPEED]*rx_daout_bytes*rx_daout_channels;
if(fwrite(&i,sizeof(int),1,wav_file)!=1)goto headerr;
// ******************************************************
// pos 32-33
// Write block align.
i=rx_daout_channels*rx_daout_bytes;
if(fwrite(&i,sizeof(short int),1,wav_file)!=1)goto headerr;
// ******************************************************
// pos 34-35
// Write bits per sample
i=8*rx_daout_bytes;
if(fwrite(&i,sizeof(short int),1,wav_file)!=1)goto headerr;
// ******************************************************
// Write the proprietary chunk for Perseus, SDR-14 etc.
if(diskread_flag != 0)
  {
  switch(remember_proprietery_chunk[0])
    {
    case REMEMBER_NOTHING:
    case REMEMBER_UNKNOWN:
    break;

    case REMEMBER_PERSEUS:
    if(fwrite("rcvr",sizeof(int),1,wav_file)!=1)goto headerr;
    if(fwrite(&remember_proprietery_chunk[1],sizeof(int),1,wav_file)!=1)
                                                                 goto headerr; 
    i=fwrite(&perseus_hdr.nCenterFrequencyHz,1,
                                    remember_proprietery_chunk[1],wav_file);
    if(i != remember_proprietery_chunk[1]) goto headerr; 
    break;

    case REMEMBER_SDR14:
    if(fwrite("auxi",sizeof(int),1,wav_file)!=1)goto headerr;
    if(fwrite(&remember_proprietery_chunk[1],sizeof(int),1,wav_file)!=1)
                                                                 goto headerr; 
    i=fwrite(&sdr14_hdr.StartTime,1, remember_proprietery_chunk[1], wav_file);
    if(i != remember_proprietery_chunk[1]) goto headerr; 
    break;

    default:
    lirerr(5699231);
    return;
    }  
  }
// Now write the code word "data"
if(fwrite("data",sizeof(int),1,wav_file)!=1)goto headerr;
// And finally the size of the data block
ii= filesize-44;
if(ii > 0x7fffffff)ii=0x7fffffff;
if(fwrite(&ii,sizeof(int),1,wav_file)!=1)goto headerr;
}

void init_memalloc(MEM_INF *mm, size_t max)
{
memalloc_no=0;
memalloc_max=max;
memalloc_mem=mm;
}

void mem(int num, void *pointer, unsigned int size, int scratch_size)
{
if(lir_errcod !=0)return;
// Skip if outside array. Error code will come on return from memalloc.
if(memalloc_no>=memalloc_max)goto skip;
memalloc_mem[memalloc_no].pointer=pointer;
memalloc_mem[memalloc_no].size=(size+15)&((size_t)-16);
memalloc_mem[memalloc_no].scratch_size=((scratch_size+15)&((size_t)-16));
memalloc_mem[memalloc_no].num=num;
skip:;
memalloc_no++;
}


// Declare this function size_t if really big arrays become needed.
size_t memalloc( size_t **hh, char *s)
{
int j;
size_t totbytes;
size_t i, k, mask;
char *x;
size_t *ptr;
size_t **handle;
double dt;
handle=(size_t**)hh;
if(memalloc_no >= memalloc_max)
  {
  DEB"memalloc_no=%d %s",(unsigned int)memalloc_no,s);
  lirerr(1136);
  }
if(lir_errcod != 0)return 0;
memalloc_mem[memalloc_no].pointer=s;
memalloc_mem[memalloc_no].size=-1;
totbytes=16+DEBMEM;
dt=totbytes;
for(i=0; i<memalloc_no; i++)
  {
  totbytes+=memalloc_mem[i].size+memalloc_mem[i].scratch_size+DEBMEM;
  dt+=memalloc_mem[i].size+memalloc_mem[i].scratch_size+DEBMEM;
  }
if(fabs(dt-totbytes) > 100)return 0;
DEB"%s: %3.1f Megabytes(%d arrays)\n",s,
                             totbytes*0.000001,(unsigned int)memalloc_no);
handle[0]=(size_t*)(malloc(totbytes+16));
if(handle[0] == 0)return 0;
mask=(size_t)-16;
k=((size_t)(handle[0])+15)&mask;
for(i=0; i<memalloc_no; i++)
  {
  x=(char*)(k);
  k+=DEBMEM;
  for (j=0; j<DEBMEM; j++)x[j]=j&0xff;
  k+=memalloc_mem[i].scratch_size;
  ptr=memalloc_mem[i].pointer;
  ptr[0]=k;
  k+=memalloc_mem[i].size;
  }
x=(char*)(k);
k+=DEBMEM;
for (j=0; j<DEBMEM; j++)x[j]=j&0xff;
return totbytes;
}

void memcheck(int callno, MEM_INF *mm, size_t **hh)
{
char s[80];
int fl, i, j, errflag;
unsigned char *x;
size_t k, mask;
size_t *handle;
handle=(size_t*)hh;
mask=(size_t)-16;
k=(handle[0]+15)&mask;
i=0;
errflag=0;
fl=mm[i].size;
while(fl != -1)
  {
  x=(unsigned char*)(k);
  k+=DEBMEM;
  for (j=0; j<DEBMEM; j++)
    {
    if( x[j] != (j&0xff) )
      {
      errflag=1;
      DEB"\nMEMORY ERROR mm=%d[%d]  data is%d(%d)  Call no %d",
                                            mm[i].num,j,x[j],j&0xff,callno);
      printf("\nMEMORY ERROR mm=%d[%d]  data is%d (%d)  Call no %d",
                                            mm[i].num,j,x[j],j&0xff,callno);
      } 
    }
  fl=mm[i].size;
  k+=mm[i].size;
  k+=mm[i].scratch_size;
  i++;
  }
if(errflag != 0)
  {
  if(fl!=-1)
    {
    sprintf(s,"\nUnknown");
    }
  else
    {
    sprintf(s,"%s",(char*)mm[i-1].pointer);
    }  
  DEB"\nError in %s\n",s);
  printf("\nError in %s\n",s);
  lirerr(1240);
  }
}

void show_name_and_size(void)
{
int i,imax;
char s[120];
float fftx_,base_,afc_,hires_,fft3_;
if(wg.yborder-3.5*text_height<wg.ytop)return;
fftx_=0.000001*fftx_totmem;
base_=0.000001*baseband_totmem;
afc_= 0.000001*afc_totmem;
fft3_=0.000001*fft3_totmem;
hires_=0.000001*hires_totmem;
sprintf(s,"%s %.1f Mbytes. fft1,fft2=%.1f  hires %.1f \
 fft3=%.1f  afc=%.1f  bas=%.1f)",
           PROGRAM_NAME,(fftx_+base_+afc_+hires_+fft3_),
           fftx_,hires_,fft3_,afc_,base_);
imax=(wg.xright-wg.xleft)/text_width-6;
if(imax > 120)imax=120;
i=0;
while(s[i]!=0  && i<imax)i++;
s[i]=0; 
settextcolor(15);
lir_pixwrite(wg.xleft+4*text_width,wg.yborder-2*text_height,s);
settextcolor(7);
}

void welcome_msg(void)
{
printf("\nUse W to create a new %s file after setup.\n",userint_filename);
printf("\nNote that the following keys have a special meaning in Linrad:");
printf("\nESC = terminate Linrad");
printf("\n X  = Skip whatever process you are in and get one level");
printf("\n      upwards in Linrads menu tree.(Not everywhere!!)");
printf("\n G  = Make a .gif file with a screen dump of your current screen.");
printf("\n\n\n -----------  GLOBAL PARAMETERS SETUP -------------");
printf("\n\n       Press one of the following keys:\n");
printf("\n'N' for NEWCOMER mode.");
printf("\n'S' for normal mode.");
printf("\n'E' for expert mode.");
printf("\n\nThen %s\n",press_enter);
}

int read_sdrpar(char *file_name, int max_parm, char **parm_text, int *par)
{
char *testbuff;
FILE *file;
int i, j, k;
file=fopen(file_name,"r");
// Read control parameters from par_xxxxxx
if(file == NULL)return -1;
testbuff = malloc(4096);
if(testbuff == NULL)
  {
  fclose(file);
  lirerr(1003);
  return -99;
  }
for(i=0; i<4096; i++)testbuff[i]=0;
i=fread(testbuff,1,4095,file);
fclose(file);
if(i >= 4095)
  {
  free(testbuff);
  return -2;
  }
k=0;
for(i=0; i<max_parm; i++)
  {
  while( (testbuff[k]==' ' || testbuff[k]== '\n' ) && k<4095)k++;
  j=0;
  while(testbuff[k]== parm_text[i][j] && k<4095)
    {
    k++;
    j++;
    }
  if(parm_text[i][j] != 0)goto lineerr;
  while(testbuff[k]!='[' && k<4095)k++;
  if(k>=4095)goto lineerr;
  sscanf(&testbuff[k],"[%d]",&par[i]);
  while(testbuff[k]!='\n' && k<4095)k++;
  if(k>=4095)goto lineerr;
  }
free(testbuff);  
return 0;
lineerr:;
free(testbuff);
return i+1;  
}

void show_specific_dev(char *name_string, int *line)
{
char s[80];
sprintf(s,"SDR DEVICE         = %s",name_string);
lir_text(24,line[0],s);
line[0]++;
SNDLOG"\n%s\n",s);
}

char *netin_txt(void)
{
char *netin;
switch (ui.network_flag & NET_RX_INPUT)
  {
  case NET_RXIN_RAW16:
  netin="RAW16";
  break;

  case NET_RXIN_RAW18:
  netin="RAW18";
  break;

  case NET_RXIN_RAW24:
  netin="RAW24";
  break;

  case NET_RXIN_FFT1:
  netin="FFT1";
  break;

  case NET_RXIN_BASEB:
  netin="BASEB16";
  break;

  case NET_RXIN_BASEBRAW:
  netin="BASEB24";
  break;

  case NET_RXIN_TIMF2:
  netin="TIMF2";
  break;

  default:
  netin="ERROR";
  break;
  }
return netin;
}

int show_rx_input_settings(int *line)
{
char *netw="NETWORK";
char s[80],ss[80];
char *specific_dev;
int retcode;
lir_text(2,line[0],"Linrad RX input from:");
SNDLOG"\nSETUP FOR RX INPUT IS NOW:\n");
settextcolor(10);
//check for SDR devices
specific_dev=NULL;
retcode=0;
switch (ui.rx_addev_no)
  {
  case SDR14_DEVICE_CODE:
  show_specific_dev(sdr14_name_string, line);
  retcode=display_sdr14_parm_info(line);
  specific_dev=sdr14_name_string;
  break;
    
  case SDRIQ_DEVICE_CODE:
  show_specific_dev(sdriq_name_string, line);
  retcode=display_sdr14_parm_info(line);
  specific_dev=sdriq_name_string;
  break;
    
  case SDRIP_DEVICE_CODE:
  show_specific_dev(sdrip_name_string, line);
  retcode=display_sdrip_parm_info(line);
  specific_dev=sdrip_name_string;
  break;
    
  case PERSEUS_DEVICE_CODE:
  show_specific_dev(perseus_name_string, line);
  retcode=display_perseus_parm_info(line);
  specific_dev=perseus_name_string;
  break;

  case EXCALIBUR_DEVICE_CODE:
  show_specific_dev(excalibur_name_string, line);
  retcode=display_excalibur_parm_info(line);
  specific_dev=excalibur_name_string;
  break;

  case RTL2832_DEVICE_CODE:
  show_specific_dev(rtl2832_name_string, line);
  retcode=display_rtl2832_parm_info(line);
  specific_dev=rtl2832_name_string;
  break;

  case MIRISDR_DEVICE_CODE:
  show_specific_dev(mirics_name_string, line);
  retcode=display_mirics_parm_info(line);
  specific_dev=mirics_name_string;
  break;

  case SDRPLAY2_DEVICE_CODE:
    show_specific_dev(sdrplay2_name_string, line);
    retcode=display_sdrplay2_parm_info(line);
    specific_dev=sdrplay2_name_string;
    break;

  case SDRPLAY3_DEVICE_CODE:
    show_specific_dev(sdrplay3_name_string, line);
    retcode=display_sdrplay3_parm_info(line);
    specific_dev=sdrplay3_name_string;
    break;

#if(OSNUM == OSNUM_LINUX)
  case FDMS1_DEVICE_CODE:
  show_specific_dev(fdms1_name_string, line);
  retcode=display_fdms1_parm_info(line);
  specific_dev=fdms1_name_string;
  break;
#endif

  case BLADERF_DEVICE_CODE:
  show_specific_dev(bladerf_name_string, line);
  retcode=display_bladerf_parm_info(line);
  specific_dev=bladerf_name_string;
  break;

  case PCIE9842_DEVICE_CODE:
  show_specific_dev(pcie9842_name_string, line);
  retcode=display_pcie9842_parm_info(line);
  specific_dev=pcie9842_name_string;
  break;

  case OPENHPSDR_DEVICE_CODE:
  show_specific_dev(openhpsdr_name_string, line);
  retcode=display_openhpsdr_parm_info(line);
  specific_dev=openhpsdr_name_string;
  break;
  
  case AIRSPY_DEVICE_CODE:
  show_specific_dev(airspy_name_string, line);
  retcode=display_airspy_parm_info(line);
  specific_dev=airspy_name_string;
  break;

  case AIRSPYHF_DEVICE_CODE:
  show_specific_dev(airspyhf_name_string, line);
  retcode=display_airspyhf_parm_info(line);
  specific_dev=airspyhf_name_string;
  break;

  case NETAFEDRI_DEVICE_CODE:
  show_specific_dev(netafedri_name_string, line);
  retcode=display_netafedri_parm_info(line);
  specific_dev=netafedri_name_string;
  break;
    
  case CLOUDIQ_DEVICE_CODE:
  show_specific_dev(cloudiq_name_string, line);
  retcode=display_cloudiq_parm_info(line);
  specific_dev=cloudiq_name_string;
  break;
    
  case DISABLED_DEVICE_CODE:
  sprintf(s,"DISABLED (Input from hard disk only)");
  lir_text(24,line[0],s);
  line[0]++;
  SNDLOG"\n%s\n",s);
  return TRUE;

  case NETWORK_DEVICE_CODE:
  lir_text(24,line[0],netw);
  line[0]++;
  SNDLOG"\n%s",netw);
  settextcolor(7);
  sprintf(ss,"receive format     = %s",netin_txt());
  lir_text(24,line[0],ss);
  SNDLOG"\n%s\n",ss);
  return TRUE;
  break;

  default:
  if(ui.rx_addev_no >= SPECIFIC_DEVICE_CODES)
    {
    specific_dev="UNKNOWN SDR DEVICE";
    retcode=-99;
    settextcolor(12);
    }
  break;
  }
if(kill_all_flag)return TRUE;
if(retcode != 0)
  {
  sprintf(s,"%s defined for Rx input (in %s)",
                                      specific_dev, userint_filename);
  lir_text(24,line[0],s);
  line[0]++;
  SNDLOG"\n%s\n",s);
  s[0]=0;
  if(retcode == -1)
    {
    sprintf(s,"parameter file not found.");
    }
  else
    {
    if(retcode == -2)
      {
      sprintf(s,"parameter file error: file too big.");
      }
    else
      {
      if(retcode > 0)sprintf(s,"parameter file error on line %d",retcode);
      }
    }
  if(s[0] != 0)
    {
    settextcolor(12);  
    lir_text(24,line[0],s);
    line[0]++;
    SNDLOG"\n%s\n",s);
    }
  settextcolor(7);   
  }
if(specific_dev != NULL)return TRUE;   
return FALSE;
}

void display_rx_input_source(char *retmsg)
{
char *sdrnam;
char ss[128], st[80];
sdrnam=NULL;
ss[0]=0;
switch(ui.rx_addev_no)
  {
  case SDR14_DEVICE_CODE:
  sdrnam=sdr14_name_string;
  break;

  case SDRIQ_DEVICE_CODE:
  sdrnam=sdriq_name_string;
  break;
  
  case PERSEUS_DEVICE_CODE:
  sdrnam=perseus_name_string;
  break;
 
  case SDRIP_DEVICE_CODE:
  sdrnam=sdrip_name_string;
  break;

  case EXCALIBUR_DEVICE_CODE:
  sdrnam=excalibur_name_string;
  break;
 
  case RTL2832_DEVICE_CODE:
  sdrnam=rtl2832_name_string;
  break;
 
  case MIRISDR_DEVICE_CODE:
  sdrnam=mirics_name_string;
  break;
 
  case FDMS1_DEVICE_CODE:
  sdrnam=fdms1_name_string;
  break;
 
  case BLADERF_DEVICE_CODE:
  sdrnam=bladerf_name_string;
  break;
 
  case PCIE9842_DEVICE_CODE:
  sdrnam=pcie9842_name_string;
  break;
 
  case OPENHPSDR_DEVICE_CODE:
  sdrnam=openhpsdr_name_string;
  break;
 
  case NETAFEDRI_DEVICE_CODE:
  sdrnam=netafedri_name_string;
  break;

  case AIRSPY_DEVICE_CODE:
  sdrnam=airspy_name_string;
  break;

  case AIRSPYHF_DEVICE_CODE:
  sdrnam=airspyhf_name_string;
  break;

  case DISABLED_DEVICE_CODE:
  sdrnam="Hardware disabled";
  break;

  case UNDEFINED_DEVICE_CODE:
  sdrnam="Not defined";
  break;

  case NETWORK_DEVICE_CODE:
  sdrnam="NETWORK";
  sprintf(ss,"%s",netin_txt());
  break;

  default:
  sdrnam="Soundcard";
  if(ui.use_extio != 0)
    {
    get_extio_name(&ss[0]);
    if( (ui.use_extio&EXTIO_USE_SOUNDCARD) == 0)
      {
      sdrnam="Callback";
      }
    }
  else
    {
    switch (ui.rx_soundcard_radio)
      {
      case RX_SOUNDCARD_RADIO_WSE:
      sdrnam="WSE";
      read_wse_parameters();
      allow_wse_parport=FALSE;
      if(wse.parport == 0)
        {
        sprintf(ss,"Disabled (port=0 illegal)");
        }
      else
        { 
        if(lir_parport_permission(wse.parport)==TRUE)
          {
          wse_parport_status=wse.parport+1;
          wse_parport_control=wse.parport+2;
          wse_parport_ack_sign=0;
          switch (wse.parport_pin)
            {
            case 15:
            wse_parport_ack=8;
            break;
 
            case 13:
            wse_parport_ack=16;
            break;
  
            case 10:
            wse_parport_ack=64;
            break;
    
            case 11:
            wse_parport_ack=128;
            wse_parport_ack_sign=128;
            break;
            }
#if(OSNUM == OSNUM_WINDOWS)
          if(dll_version >= 5)
            {
            if(!IsInpOutDriverOpen())
              {
              sprintf(ss,"Not open. Run Linrad once as Administrator");
              break;
              }
            }
#endif
            {
            allow_wse_parport=TRUE;
            if (wse.parport==USB2LPT16_PORT_NUMBER) 
              {
              strcpy(st,"USB2LPT");
              }
            else
              {  
              sprintf(st,"%d",wse.parport);
              }
            sprintf(ss,"Parport %s:%d",st,wse.parport_pin);
            }
          }
        else  
          {
          sprintf(ss,"WSE Parport disabled");
          }
        }  
      break;
    
      case RX_SOUNDCARD_RADIO_SI570:
      sprintf(ss,"Si570");
      break;
          
      case RX_SOUNDCARD_RADIO_SOFT66:
      sprintf(ss,"Soft66");
      break;
  
      case RX_SOUNDCARD_RADIO_ELEKTOR:
      sprintf(ss,"Elektor");
      break;
      
      case RX_SOUNDCARD_RADIO_FCDPROPLUS:
      sprintf(ss,"Funcube Pro+");
      break;
      
      case RX_SOUNDCARD_RADIO_AFEDRI_USB:
      sprintf(ss,"Afedri USB");
      break;
      
      default:
      break;
      }
    }
  }
sprintf(retmsg,"%s %s",sdrnam,ss);
}
