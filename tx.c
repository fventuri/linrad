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
#include "uidef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "fft3def.h"
#include "sigdef.h"
#include "screendef.h"
#include "seldef.h"
#include "thrdef.h"
#include "txdef.h"
#include "vernr.h"
#include "hwaredef.h"
#include "options.h"
#include "keyboard_def.h"
#include "bufbars.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#include "fcntl.h"
#endif

#define PILOT_TONE_CONTROL 0.00000000001

int radar_cnt;
int radar_pulse_cnt;
int radar_cnt_max;
int txout_p0;

int tx_wrblock_maxcnt;
int tx_wrblock_no;
extern fpos_t file_startpos;
FILE *txfile;

#define PTT_OFF 0
#define PTT_ON 1
#define PTT_AWAITING_ON 2
#define PTT_AWAITING_OFF 4

void contract_tx_spectrum(float *z)
{
int i, k;
k=2*tx_lowest_bin;
for(i=0; i<k; i++)tx_oscilloscope_tmp[i]=0;
for(i=2*tx_filter_ia1; i<micfft_block; i++)
  {
  tx_oscilloscope_tmp[k]=z[i];
  k++;
  }
i=0;
while(i <= tx_highest_bin)
  {
  tx_oscilloscope_tmp[k]=z[i];
  k++;
  i++;
  }
while(k < micdisplay_fftsize)
  {
  tx_oscilloscope_tmp[k]=0;
  i++;
  }
}

int do_tx_open(void)
{  
#if OSNUM == OSNUM_LINUX
char s[80];
int rx_ad_bytes;
if((ui.rx_input_mode&DWORD_INPUT) == 0)
  {
  rx_ad_bytes=2;
  }
else
  {
  rx_ad_bytes=4;
  }
if(ui.rx_addev_no == ui.tx_dadev_no && ui.rx_admode == O_RDWR )
  {
  if(ui.tx_da_bytes != rx_ad_bytes || 
     ui.tx_da_channels != ui.rx_ad_channels ||
     ui.tx_da_speed != ui.rx_ad_speed)
    {
    clear_screen();
    lir_text(5,5,
         "RXAD and TXDA set to use the same soundcard in RDWR mode");
    lir_text(5,7,"Not possible since formats differ. Check setup.");
    lir_text(5,9,press_any_key);


sprintf(s,"tx_da_bytes %d  rx_ad_bytes %d",ui.tx_da_bytes, rx_ad_bytes);
lir_text(0,11,s);
sprintf(s,"tx_da_channels %d  rx_ad_channels %d",
                                    ui.tx_da_channels, ui.rx_ad_channels);
lir_text(0,12,s);
sprintf(s,"tx_da_speed %d rx_ad_speed %d",
                         ui.tx_da_speed, ui.rx_ad_speed);
lir_text(0,13,s);

    await_processed_keyboard();
    return FALSE;
    }
  snd[RXAD]=snd[TXDA];
  open_rx_sndin(FALSE);
// we do not set the flag,(snd[RXAD].open_flag) because data will not be used.
  snd[TXDA]=snd[RXAD];  
  }
#endif  
open_tx_sndout();
if(txda_status != LIR_OK)
  {
  lirerr(1407);
  }
if(kill_all_flag)return FALSE;
txout_pa=0;
txout_px=0;
snd[TXDA].open_flag=CALLBACK_CMD_ACTIVE;
tx_pilot_tone=pow(10.0,-0.05*ui.tx_pilot_tone_db);
fgetpos(txfile,&file_startpos);
return TRUE;
}

double txphase=0;

void  fix_24bit_data(int *buf)
{
signed char *ch;
int ip, cp;
ch=(signed char*) buf;
//i=fread(&rdbuf4[txout_pa], 1, 3*snd[TXDA].block_bytes/4, txfile);
ip=snd[TXDA].block_bytes/4-1;
cp=3*ip;
while(ip >= 0)
  {
  ch[4*ip+3]=ch[cp+2];
  ch[4*ip+2]=ch[cp+1];
  ch[4*ip+1]=ch[cp];
  if(ch[4*ip+3]>=0)
    {
    ch[4*ip]=0;
    }
  else
    {
    ch[4*ip]=-1;
    }
buf[ip]*=3;
  if( (ip&2)==0)
    {
    buf[ip]+=tx_pilot_tone;
    }
  else  
    {
    buf[ip]-=tx_pilot_tone;
    }
  ip--;
  cp-=3;
  }


/*
for(ip=0; ip<snd[TXDA].block_bytes/4; ip+=2)
  {
  buf[ip  ]=0x7f000000*cos(txphase);
  buf[ip+1]=0x7f000000*sin(txphase);
  if( (ip&2)==0)
    {
    buf[ip]+=tx_pilot_tone;
    buf[ip+1]-=tx_pilot_tone;
    }
  else  
    {
    buf[ip]-=tx_pilot_tone;
    buf[ip+1]+=tx_pilot_tone;
    }
  txphase+=0.5;
  if(txphase > PI_L)txphase-=2*PI_L;
  }
*/



}

void disk2tx(void)
{
char wavfilenam[256];
char s[300];
int i, k, errnr;
int bufsize, blksize, bufmask;
int line, chunk_size;
short int ishort, format_tag;
short int * rdbuf2;
int * rdbuf4;
clear_screen();
lir_text(5,5,"Specify file name of file to transmit");
lir_text(5,7,"->");
i=lir_get_filename(8,7,wavfilenam);
if(i==0 || kill_all_flag)return;
txfile=fopen(wavfilenam, "rb");
if(txfile == NULL)
  {
  sprintf(s,"Could not open %s",wavfilenam);
  lir_text(5,10,s);
  lir_text(5,11,press_any_key);
  await_processed_keyboard();
  return;
  }
line=10;  
// Read the WAV file header.
// First 4 bytes should be "RIFF"
k=fread(&i,sizeof(int),1,txfile);
errnr=0;
if(k !=1)
  {
  errnr=1;
headerr:;  
  sprintf(s,"Error in .wav file header [%d]",errnr);  
  lir_text(5,line,s);
  line +=2;
  lir_text(5,line,press_any_key);
  await_processed_keyboard();
  goto txfile_x;
  }
if(i != 0x46464952)
  {
  errnr=2;
  goto headerr;
  }
// Read file size (we do not need it)
k=fread(&i,sizeof(int),1,txfile);
if(k!=1)
  {
  errnr=3;
  goto headerr;
  }
// Now we should read "WAVE"
k=fread(&i,sizeof(int),1,txfile);
errnr=4;
if(k !=1 || i != 0x45564157)goto headerr;
// Now we should read "fmt "
k=fread(&i,sizeof(int),1,txfile);
errnr=5;
if(k !=1 || i != 0x20746d66)goto headerr;
// read the size of the format chunk.
k=fread(&chunk_size,sizeof(int),1,txfile);
errnr=6;
if(k !=1 )goto headerr; 
// read the type of data (Format Tag).
k=fread(&format_tag,sizeof(short int),1,txfile);
errnr=7;
if(k !=1 )goto headerr; 
if(format_tag != 1 && format_tag != 3)
  {
  lir_text(5,line,"Unknown wFormatTag.");
  line++;
  goto headerr;
  }
// Read no of channels
k=fread(&ishort,sizeof(short int),1,txfile);
errnr=8;
if(k !=1 )goto headerr; 
if(ishort != ui.tx_da_channels)
  {
  lir_text(5,line,"The number of channels does not agree with your TX setup.");
  line++;
  goto headerr;
  }
// Read the sampling speed.
k=fread(&i,sizeof(int),1,txfile);
errnr=9;
if(k !=1 )goto headerr; 
if(i != ui.tx_da_speed)
  {
  lir_text(5,line,"The sampling speed does not agree with your TX setup.");
  line++;
  goto headerr;
  }
// Read average bytes per second (do not care what it is)
errnr=10;
k=fread(&i,sizeof(int),1,txfile);
if(k !=1 )goto headerr; 
// Read block align to get 8 or 16 bit format.
k=fread(&ishort,sizeof(short int),1,txfile);
errnr=11;
if(k !=1 )goto headerr; 
ishort/=ui.tx_da_channels;
// We use rx_ad_bytes only in RDWR mode with OSS.
// Then the possible formats are 16 bit and 32 bit only.
switch (ishort)
  {
  case 1:
// byte int input  
  errnr=12;
  if(format_tag != 1)goto headerr;
  tx_disk_mode=BYTE_INPUT; 
  break;
  
  case 2:
// 16 bit int input
  errnr=13;
  if(format_tag != 1)goto headerr;
  tx_disk_mode=0;
  break;
  
  case 3:
// 24 bit int input  
  errnr=14;
  if(format_tag != 1)goto headerr;
  tx_disk_mode=BYTE_INPUT+DWORD_INPUT;
  break;
  
  case 4:  
  errnr=15;
  if(format_tag == 1)
    {
// 32 bit int input    
    tx_disk_mode=QWORD_INPUT+DWORD_INPUT;
    break;
    }
  if(format_tag != 3)goto headerr;
// 32 bit float input
  tx_disk_mode=FLOAT_INPUT+DWORD_INPUT;
  break;
  
  default:
  errnr=16;
  goto headerr;
  }
// Skip extra bytes if present.
chunk_size-=14;
skip_chunk:;
errnr=17;
while(chunk_size != 0)
  {
  k=fread(&i,1,1,txfile);
  if(k !=1 )goto headerr; 
  chunk_size--;
  }
// Read chunks until we encounter the "data" string (=0x61746164).
errnr=18;
k=fread(&i,sizeof(int),1,txfile);
if(k !=1 )goto headerr; 
errnr=19;
// Look for DATA 
if(i != 0x61746164)
  {
// Unknown. Get the size and skip.  
  k=fread(&chunk_size,sizeof(int),1,txfile);
  if(k !=1 )goto headerr; 
  goto skip_chunk;
  }
// Read how much data we have (do not care)
k=fread(&i,sizeof(int),1,txfile);
errnr=20;
if(k !=1 )goto headerr; 
snd[TXDA].framesize=ui.tx_da_bytes*ui.tx_da_channels;
snd[TXDA].block_bytes=(1000+ui.tx_da_speed)*snd[TXDA].framesize/1000;
make_power_of_two(&snd[TXDA].block_bytes);
test_keyboard();
switch(tx_disk_mode)
  {
  case 0:
  if(do_tx_open() == FALSE)goto txfile_x;
// 16 bit integers.
  rdbuf2=malloc(8*snd[TXDA].block_bytes);
  bufsize=8*snd[TXDA].block_bytes/2;
  blksize=snd[TXDA].block_bytes/2;
  bufmask=bufsize-1;
  while(txout_pa < bufsize-2*blksize)
    {
    i=fread(&rdbuf2[txout_pa], 1, snd[TXDA].block_bytes, txfile);
    txout_pa+=blksize;
    if(i != snd[TXDA].block_bytes)
      {
      clear_screen();
      lir_text(5,5,"Short read. File empty???");
      lir_text(5,9,press_any_key);
      await_processed_keyboard();
      goto txfile_x;
      }
    }
  while(lir_inkey == 0 && !kill_all_flag)
    {
    for(i=0; i<blksize; i+=2)
      {
      rdbuf2[txout_px+i]+=tx_pilot_tone;
      rdbuf2[txout_px+i+1]-=tx_pilot_tone;
      }
    lir_tx_dawrite((char*)&rdbuf2[txout_px]);
    txout_px=(txout_px+blksize)&bufmask;
    i=fread(&rdbuf2[txout_pa], snd[TXDA].block_bytes, 1, txfile);
    if(i!=snd[TXDA].block_bytes)
      {
      i/=2;
      while(i < blksize)
        {
        rdbuf2[txout_pa+i]=0;
        i++;
        }
      fsetpos(txfile,&file_startpos);
      txout_pa=(txout_pa+blksize)&bufmask;
      }    
    test_keyboard();
    }
  break;
  
  case BYTE_INPUT+DWORD_INPUT:
  if(do_tx_open() == FALSE)goto txfile_x;
// 24 bit integers.
  rdbuf4=malloc(8*snd[TXDA].block_bytes);
  bufsize=8*snd[TXDA].block_bytes/4;
  blksize=snd[TXDA].block_bytes/4;
  bufmask=bufsize-1;
  while(txout_pa < bufsize-3*blksize)
    {
    i=fread(&rdbuf4[txout_pa], 1, 3*snd[TXDA].block_bytes/4, txfile);
    if(i != 3*snd[TXDA].block_bytes/4)
      {
      clear_screen();
      lir_text(5,5,"Short read. File empty???");
      lir_text(5,9,press_any_key);
      await_processed_keyboard();
      goto txfile_x;
      }
    fix_24bit_data(&rdbuf4[txout_pa]);  
    txout_pa+=blksize;
    }
  while(lir_inkey == 0 && !kill_all_flag)
    {
    i=fread(&rdbuf4[txout_pa], 1, 3*snd[TXDA].block_bytes/4, txfile);
    fix_24bit_data(&rdbuf4[txout_pa]);  
    if(i!=3*snd[TXDA].block_bytes/4)
      {
      i/=3;
      while(i < blksize)
        {
        rdbuf4[txout_pa+i]=0;
        i++;
        }
      fsetpos(txfile,&file_startpos);
      }    
    txout_pa=(txout_pa+blksize)&bufmask;
    if(ui.tx_da_bytes == 4)
      {
      lir_tx_dawrite((char*)&rdbuf4[txout_px]);
      txout_px=(txout_px+blksize)&bufmask;      
      }
    else
      {
      lirerr(229513);
      return;
      }
    test_keyboard();
    }
  break;


  default:
  clear_screen();
  lir_text(5,5,"This .wav file format is not yet supported for TX output.");
  lir_text(5,7,press_any_key);
  await_processed_keyboard();
  goto txfile_x;
  }

lir_text(1,20,"OKOKOKOK");
await_keyboard();

txfile_x:;
fclose(txfile);  
}


float tx_total_delay(void)
{
// Get the time spanned by all buffered data.
// See tx_ssb_buftim() for details.
int i,k,m;
float t1;
if ( (ui.use_alsa&PORTAUDIO_TX_IN) == 0)
  {
  i=lir_tx_input_samples();
  }
else
  {
  i=0;
  }
i+=(mictimf_pa-mictimf_px+mictimf_bufsiz)&mictimf_mask;
if(tx_mode == TXMODE_CW)
  {
  k=((mic_key_pa-mic_key_px+mic_key_bufsiz)&mic_key_mask);
  }
else
  {
  k=0;
  }  
t1=(float)i;
if(tx_mode == TXMODE_SSB)
  {
  k=((micfft_pa-micfft_px+micfft_bufsiz)&micfft_mask);
  k+=((cliptimf_pa-cliptimf_px+micfft_bufsiz)&micfft_mask);
  t1+=(float)k/(2.F*(float)mic_to_spproc_resample_ratio);
  k=((clipfft_pa-clipfft_px+alc_bufsiz)&alc_mask)/2;
  k+=((alctimf_pa-((int)(alctimf_fx))+4+alc_bufsiz)&alc_mask);
  t1+=(float)k/tx_pre_resample;
  }
t1+=(float)k*tx_pre_resample;
m=lir_tx_output_samples();
if(tx_min_output_samples > m)tx_min_output_samples=m;
if( (ui.use_alsa&PORTAUDIO_TX_OUT) == 0)
  {  
  m+=((txout_pa+txout_p0-txout_px+txout_bufsiz)&txout_mask)/ui.tx_da_channels;
  }
t1+=(float)m/tx_resample_ratio;
t1/=(float)ui.tx_ad_speed;
return t1;
}

void tx_ssb_buftim(char cc)
{
char s[256];
int i;
float t1,t2,t3,t4,t5,t6,t7,t8;
// Show time span of buffered data.
// First microphone signal. Real data at ui.tx_ad_speed
s[0]=0;
t1=0;
if(cc > '9')sprintf(s,"\n%c ",cc);
if ( (ui.use_alsa&PORTAUDIO_TX_IN) == 0)
  {
  i=lir_tx_input_samples();
  t1=(float)(i)/(float)ui.tx_ad_speed;
  sprintf(&s[strlen(s)],"in %.3f",t1);
  }
i=(mictimf_pa-mictimf_px+mictimf_bufsiz)&mictimf_mask;
t2=(float)(i/ui.tx_ad_channels)/(float)ui.tx_ad_speed;
sprintf(&s[strlen(s)]," m.timf %.3f",t2);
// The speech processing rate may be higher or lower.
// It is adjusted for a frequency span that is N times
// larger than the bandwidth of the selected filter.
// rate conversion: mic_to_spproc_resample_ratio
// mic_to_spproc_resample_ratio=mic_fftsize/micsize;  
// Transforms overlap by 50 % and complex data has half the
// sampling speed of real data.
i=((micfft_pa-micfft_px+micfft_bufsiz)&micfft_mask);
t3=(float)i/((float)ui.tx_ad_speed*4*mic_to_spproc_resample_ratio);
sprintf(&s[strlen(s)]," m.fft %.3f",t3);
i=((cliptimf_pa-cliptimf_px+micfft_bufsiz)&micfft_mask);
t4=(float)i/((float)ui.tx_ad_speed*2*mic_to_spproc_resample_ratio);
sprintf(&s[strlen(s)]," cl.timf %.3f",t4); 
// The alc may have yet another sampling rate.
// tx_pre_resample=alc_fftsize/mic_fftsize
// Transforms overlap by 50 %
i=((clipfft_pa-clipfft_px+alc_bufsiz)&alc_mask);
t5=(float)i/((float)ui.tx_ad_speed*2*tx_pre_resample);
sprintf(&s[strlen(s)]," cl.fft %.3f",t5);
i=((alctimf_pa-((int)(alctimf_fx))+4+alc_bufsiz)&alc_mask);
t6=(float)i/((float)ui.tx_ad_speed*tx_pre_resample);
sprintf(&s[strlen(s)]," alctimf %.3f",t6);
if( (ui.use_alsa&PORTAUDIO_TX_OUT) == 0)
  {
  i=((txout_pa+txout_p0-txout_px+txout_bufsiz)&txout_mask);
  i/=ui.tx_da_channels;
  t8=(float)i/ui.tx_da_speed;
  sprintf(&s[strlen(s)]," outbuf %.3f",t8);
  }
else 
  {
  t8=0;
  }  
i=lir_tx_output_samples();
if(tx_min_output_samples > i)tx_min_output_samples=i;
t7=(float)i/ui.tx_da_speed;
sprintf(&s[strlen(s)]," out %.3f",t7);
sprintf(&s[strlen(s)]," tot %.3f",t1+t2+t3+t4+t5+t6+t7+t8);
sprintf(&s[strlen(s)]," [%.3f]",(float)tx_wsamps/(float)ui.tx_ad_speed);
if(cc <= '9')
  {
  lir_text(0,cc-'0',s);
  }
else
  {
  fprintf( dmp,"%s",s);
  }  
}

void tx_cw_buftim(char cc)
{
float t5, t1, t2, t3, t4, speed;
char s[150];
int i;
// Show time span of buffered data.
// First microphone signal. Real data at ui.tx_ad_speed
s[0]=0;
t1=0;
speed=(float)ui.tx_ad_speed;
if(cc > '9')sprintf(s,"\n%c ",cc);
if ( (ui.use_alsa&PORTAUDIO_TX_IN) == 0)
  {
  i=lir_tx_input_samples();
  t1=(float)i/speed;
  sprintf(&s[strlen(s)],"AD %.5f",t1);
  }
i=(mictimf_pa-mictimf_px+mictimf_bufsiz)&mictimf_mask;
t5=(float)i/speed;
sprintf(&s[strlen(s)]," m.timf %.5f",t5);
t2=(float)((mic_key_pa-mic_key_px+mic_key_bufsiz)&mic_key_mask);
t2*=(float)tx_pre_resample/speed;
sprintf(&s[strlen(s)]," mic_key %.5f",t2);
t3=(float)((txout_pa+txout_p0-txout_px+txout_bufsiz)&txout_mask);
t3/=speed*tx_resample_ratio*ui.tx_da_channels;
sprintf(&s[strlen(s)]," txout %.5f",t3);
i=lir_tx_output_samples();
if(tx_min_output_samples > i)tx_min_output_samples=i;
t4=(float)i/(speed*tx_resample_ratio);
sprintf(&s[strlen(s)]," DA %.5f",t4);
sprintf(&s[strlen(s)]," MIN %.5f",
                      (float)tx_min_output_samples/(speed*tx_resample_ratio));
sprintf(&s[strlen(s)]," [%.5f]",(float)tx_wsamps/(float)ui.tx_ad_speed);
sprintf(&s[strlen(s)]," tot %.5f",t1+t2+t3+t4+t5);
if(cc <= '9')
  {
  lir_text(0,cc-'0',s);
  }
else
  {
  fprintf( dmp,"%s",s);
  }  
}

void do_cw_keying(void)
{
int i, k, nn, mm;
float amplitude, pilot_ampl;
float r1, r2, tot_resamp;;
double dt1;
pilot_ampl=0;
dt1=1/sqrt(tx_daout_sin*tx_daout_sin+tx_daout_cos*tx_daout_cos);
tx_daout_sin*=dt1;
tx_daout_cos*=dt1;
// Find out how many points we should place in the output buffer.
mm=(mic_key_pa-mic_key_px+mic_key_bufsiz)&mic_key_mask;
tot_resamp=tx_resample_ratio*tx_pre_resample;
hware_hand_key();
repeat:;
r2=1.0F/tot_resamp;
r1=(mm-mic_key_fx)/r2;
nn=r1;
for(i=0; i<nn; i++)
  {
  mic_key_fx+=r2;
  if(mic_key_fx > 1)
    {
    mic_key_fx-=1;
    mic_key_px=(mic_key_px+1)&mic_key_mask;
    if(tx_mic_key[mic_key_px] > 1)
      {
      tone_key=1;
      }
    else
      {
      tone_key=0;
      }
    }  
  if(tx_waveform_pointer==0 || 
                tx_waveform_pointer==tx_cw_waveform_points)
    {
    if(rx_mode == MODE_RADAR)
      {
      tot_key=tone_key;
      }
    else
      {
      tot_key=(hand_key&txcw.enable_hand_key)|
             (tone_key&txcw.enable_tone_key);
      }
    }
  if(tx_waveform_pointer==0)
    {
    pilot_ampl=0;
    }
  else
    {
    pilot_ampl=tx_pilot_tone;
    }
  amplitude=txout_waveform[tx_waveform_pointer];
  tx_pilot_tone=-tx_pilot_tone;
  if(tot_key)
    {
    if(tx_waveform_pointer == 0 && old_cw_rise_time != txcw.rise_time)
      {
      make_tx_cw_waveform();
      }
    if(tx_waveform_pointer < tx_cw_waveform_points)tx_waveform_pointer++;
    }
  else
    {
    if(tx_waveform_pointer > 0)
      {
      tx_waveform_pointer--;
      }
// Do not continue the pilot tone after the signal is zero.
    if(amplitude < PILOT_TONE_CONTROL)pilot_ampl=0;
    }
  dt1=tx_daout_cos;
  tx_daout_cos= tx_daout_cos*tx_daout_phstep_cos+tx_daout_sin*tx_daout_phstep_sin;
  tx_daout_sin= tx_daout_sin*tx_daout_phstep_cos-dt1*tx_daout_phstep_sin;
  amplitude*=tx_output_amplitude*tx_onoff_flag;
  txout[txout_pa+txout_p0  ]=amplitude*tx_daout_cos+pilot_ampl;
  txout[txout_pa+txout_p0+1]=amplitude*tx_daout_sin-pilot_ampl;
  txout_p0+=2;
  if(txout_p0 == txout_fftsize)
    {
    tot_key=(hand_key&txcw.enable_hand_key)|
             (tone_key&txcw.enable_tone_key);
    if( tot_key && tx_onoff_flag)
      {
      ptt_state=TRUE;
      }
    else
      {
      ptt_state=FALSE;
      }
// We want to change PTT state when the current sample leaves the loudspeaker.
// Currently txout_px-i is the sample leaving.
// We want txout_pa-txout_px+i samples to run out before disabling PTT
    lir_sched_yield();
    i=lir_tx_output_samples();
    if(tx_min_output_samples > i)tx_min_output_samples=i;
    if(old_ptt_state == PTT_AWAITING_ON)
      {
      ptt_on_wait=(ptt_on_wait-txout_fftsize+txout_bufsiz)&txout_mask;
//      k=(ptt_on_wait+txout_pa-txout_px-2*txout_fftsize+txout_bufsiz)&txout_mask;
      if(ptt_on_wait > txout_bufsiz/2)
        {
        hware_set_ptt(TRUE);
        old_ptt_state=PTT_ON;
        }
      }
    if(old_ptt_state == PTT_OFF && ptt_state == TRUE)
      {
      ptt_on_wait=(txout_pa-txout_px+i+txout_fftsize+txout_bufsiz)&txout_mask;  
      old_ptt_state=PTT_AWAITING_ON;
      }


    if(old_ptt_state == PTT_AWAITING_OFF)
      {
      ptt_off_wait=(ptt_off_wait-txout_fftsize+txout_bufsiz)&txout_mask;
      k=(ptt_off_wait+txout_pa-txout_px+i+txout_bufsiz)&txout_mask;
      if(k > txout_bufsiz/2)
        {
        hware_set_ptt(FALSE);
        old_ptt_state=PTT_OFF;
        }
      }
    if(old_ptt_state == PTT_ON && ptt_state == FALSE && tx_waveform_pointer==0)
      {
      ptt_off_wait=(txout_pa-txout_px+i+2*txout_fftsize+txout_bufsiz)&txout_mask;  
      old_ptt_state=PTT_AWAITING_OFF;
      }
// Compute the total time from tx input to tx output and
// express it as samples of the tx input clock and store in tx_wsamps. 
    tx_wsamps=lir_tx_input_samples();
    k=(mic_key_pa-mic_key_px+mic_key_bufsiz)&mic_key_mask;
    tx_wsamps+=k*tx_pre_resample;

#if BUFBARS == TRUE
    if(timinfo_flag!=0)
      {
      i=(txout_pa+txout_p0-txout_px+txout_bufsiz)&txout_mask;
      i/=txout_indicator_block;
      show_bufbar(TX_BUFBAR_CW_TXOUT,i);
      }
#endif
    tx_send_to_da();
    txout_p0=0;
    txout_pa=(txout_pa+txout_fftsize)&txout_mask;
    goto new_resamp;
    }
  }  
new_resamp:;  
mm=(mic_key_pa-mic_key_px+mic_key_bufsiz)&mic_key_mask;
i=lir_tx_output_samples();
if(tx_min_output_samples > i)tx_min_output_samples=i;
lir_sched_yield();
if(i+snd[TXDA].block_frames > snd[TXDA].tot_frames)return;
if(mm > 1 )goto repeat;
}

void tx_send_to_da(void)
{
int i, p0, pb;
float t1, t2;
if( (ui.use_alsa&PORTAUDIO_TX_OUT) == 0)
  {
#if BUFBARS == TRUE
  if(timinfo_flag!=0)
    {
    i=lir_tx_output_samples();
    i/=txda_indicator_block;
    show_bufbar(TX_BUFBAR_CW_DA,i);
    }
#endif
  while( 2*((txout_pa+txout_p0-txout_px+txout_bufsiz)&txout_mask)/ui.tx_da_channels > 
                        snd[TXDA].block_frames &&
                   ((txout_pa+txout_p0-txout_px+txout_bufsiz)&txout_mask) < 
                                             txout_bufsiz/2)                        

    {
    p0=txout_px;
    switch(tx_output_mode)
      {
      case 0:
// one channel 16 bit data
      pb=txout_px;
      for(i=0; i<snd[TXDA].block_frames; i++)
        {
        t1=txout[txout_px];
        tx_out_shi[pb]=TX_DA_MARGIN*0x7fff*t1;
        txout_px=(txout_px+2)&txout_mask;  
        pb=(pb+1)&txout_mask;  
        }
      lir_tx_dawrite((char*)&tx_out_shi[p0]);
      break;
  
      case 1:
// two channels 16 bit data
      for(i=0; i<snd[TXDA].block_frames; i++)
        {
        t1=txout[txout_px  ];
        t2=txout[txout_px+1];
        tx_out_shi[txout_px]=TX_DA_MARGIN*0x7fff*t2;
        tx_out_shi[txout_px+1]=TX_DA_MARGIN*0x7fff*t1;
        txout_px=(txout_px+2)&txout_mask;  
        }
      lir_tx_dawrite((char*)&tx_out_shi[p0]);
      break;

      case 2:
// one channel 32 bit data
      pb=txout_px;
      for(i=0; i<snd[TXDA].block_frames; i++)
        {
        t1=txout[txout_px];
        tx_out_int[pb]=TX_DA_MARGIN*0x7fff0000*t1;
        txout_px=(txout_px+2)&txout_mask;  
        pb=(pb+1)&txout_mask;  
        }
      lir_tx_dawrite((char*)&tx_out_int[p0]);
      break;
  
      case 3:
// two channels 32 bit data
      for(i=0; i<snd[TXDA].block_frames; i++)
        {
        t1=txout[txout_px  ];
        t2=txout[txout_px+1];
        tx_out_int[txout_px  ]=TX_DA_MARGIN*0x7fff0000*t2;
        tx_out_int[txout_px+1]=TX_DA_MARGIN*0x7fff0000*t1;
        txout_px=(txout_px+2)&txout_mask;  
        }
      lir_tx_dawrite((char*)&tx_out_int[p0]);
      break;
      }
    }
  }
lir_sched_yield();
t1=tx_total_delay();
tx_wttim_sum+=t1;
tx_wrblock_no++;
if(tx_mode == MODE_SSB)
{
if(tx_wrblock_no == tx_wrblock_maxcnt)
  {
  tx_wttim_sum/=(float)tx_wrblock_maxcnt;
  if(tx_output_flag == 1)
    {
    tx_output_flag=2;
    tx_ideal_delay=tx_wttim_sum;
    tx_min_output_samples=1000000;
    }
  else
    { 
    t1=tx_wttim_sum-tx_ideal_delay;
    t2=t1-tx_wttim_diff;
    tx_wttim_diff=t1;
// We have accumulated an error of t1 seconds while writing 
// tx_wrblock_maxcnt data blocks. Each block spans a time of
// txout_sizhalf/txssb.output speed so the relative error is
// X=t1/(tx_wrblock_maxcnt*txout_sizhalf/txssb.output speed);
// Therefore we should multiply the output 
// speed by (1-X)
// Use half the correction and add in a small part of t1,
// the deviation from the initial time delay.
    new_tx_resample_ratio=tx_resample_ratio*(1-0.5*(t2+0.2*t1)*
                           ui.tx_da_speed/(tx_wrblock_maxcnt*txout_sizhalf));    
    tx_resample_ratio=0.9*tx_resample_ratio+0.1*new_tx_resample_ratio;
    }  
  tx_wttim_sum=0;
  tx_wrblock_no=0;
  }
}
else
{
if(tx_wrblock_no == tx_wrblock_maxcnt)
  {
  tx_wttim_sum/=(float)tx_wrblock_maxcnt;
  t1=tx_wttim_sum-tx_ideal_delay;
  t2=t1-tx_wttim_diff;
  tx_wttim_diff=t1;
// We have accumulated an error of t1 seconds while writing 
// tx_wrblock_maxcnt data blocks. Each block spans a time of
// txout_sizhalf/txssb.output speed so the relative error is
// X=t1/(tx_wrblock_maxcnt*txout_sizhalf/txssb.output speed);
// Therefore we should multiply the output 
// speed by (1-X)
// Use the correction and add in a part of t1,
// the deviation from the initial time delay.
  new_tx_resample_ratio=tx_resample_ratio*(1-(t2+0.5*t1)*
                           ui.tx_da_speed/(tx_wrblock_maxcnt*txout_sizhalf));    
  tx_resample_ratio=0.8*tx_resample_ratio+0.2*new_tx_resample_ratio;
  tx_wttim_sum=0;
  tx_wrblock_no=0;
  }

}

}

void run_tx_output(void)
{
int i, k, mm, valid_bins, muted_points;
float pmax, t1, t2, r2, prat, prat1a, prat1b;
#if BUFBARS == TRUE
clear_bufbars();
#endif
tot_key=0;
int local_workload_reset_flag;
thread_status_flag[THREAD_TX_OUTPUT]=THRFLAG_ACTIVE;
// Allow the receive side to use the CPU for a while.  
if(!kill_all_flag)lir_await_event(EVENT_TX_INPUT);
sw_cliptimf=FALSE;
switch (tx_mode)
  {
  case TXMODE_SSB:
  if(!kill_all_flag)lir_await_event(EVENT_TX_INPUT);
  for(i=0; i<10; i++)
    {
    micfft_px=micfft_pa;
    lir_sched_yield();
    while(micfft_px == micfft_pa && !kill_all_flag)
      {
      if(thread_command_flag[THREAD_TX_OUTPUT] != THRFLAG_ACTIVE)goto txout_x;
      lir_await_event(EVENT_TX_INPUT);
      t1=0;
      for(k=0;k<100;k++)t1+=sin(0.0001*k);
      lir_sched_yield();
      }
    }
  micfft_px=micfft_pa;
  local_workload_reset_flag=workload_reset_flag;
// ************************* SSB processing ************************  
  while(!kill_all_flag && 
                    thread_command_flag[THREAD_TX_OUTPUT] == THRFLAG_ACTIVE)
    {
//    tx_ssb_buftim('A');
    lir_sched_yield();
    while(!kill_all_flag && micfft_pa ==  micfft_px)
      {
      if(thread_command_flag[THREAD_TX_OUTPUT] != THRFLAG_ACTIVE)goto txout_x;
      if(local_workload_reset_flag != workload_reset_flag)
        {
        local_workload_reset_flag=workload_reset_flag;
        tx_min_output_samples=snd[TXDA].tot_bytes/snd[TXDA].framesize;
        }
      lir_await_event(EVENT_TX_INPUT);
      lir_sched_yield();
      }
#if BUFBARS == TRUE
    if(timinfo_flag!=0)
      {
      i=(micfft_pa-micfft_px+micfft_bufsiz)&micfft_mask;
      i/=micfft_indicator_block;
      show_bufbar(TX_BUFBAR_MICFFT,i);
      }
#endif
    if(tx_agc_factor < 0.1)tx_agc_factor=0.1;
    if(tx_fft_phase_flag)tx_ph1*=-1;
    tx_agc_factor=tx_agc_decay*tx_agc_factor+(1-tx_agc_decay);
    micfft_bin_minpower=micfft_bin_minpower_ref*tx_agc_factor*tx_agc_factor;
    micfft_minpower=micfft_minpower_ref; // tx_agc_factor*tx_agc_factor;
    valid_bins=tx_ssb_step2(&t1);
// In case AGC has reduced the gain by more than 20 dB, set the
// AGC factor to -20 dB immediately because voice should never
// be as loud. This is to avoid the agc release time constant
// for impulse sounds caused by hitting the microphone etc.       


// ************* Step 4. *************
// Go back to the time domain and store the signal in cliptimf
// Remember that we use sin squared windows and that
// the transforms overlap with 50%.
//
// Ideally, the peak amplitude should be 1.0, the audio AGC
// should be active and keep the power level nearly constant.
// For a voice signal the waveform will differ from time to time
// and therefore the peak to average power ratio will vary.
// Compute the peak amplitude for the current transform
// and save it for the next time we arrive here.
// Use the peak amplitude for an AGC in the time domain (complex-valued).
// The Hilbert space AGC is a constant that may vary from one
// FFT block to the next one. It is equivalent with an AM modulator
// with a bandwidth corresponding to a single bin in the FFT so this
// AM modulation will not increase the bandwidth notably, but it
// will bring the RF amplitude near unity always.
//
// Finally, apply an amplitude limiter to the complex time domain signal.
// It will work as an RF limiter on the SSB signal and cause a lot of
// signal outside the passband.
    if(valid_bins!=0)
      {
      fftback(mic_fftsize, mic_fftn, &micfft[micfft_px],
                                   micfft_table, micfft_permute, FALSE);
      lir_sched_yield();
      r2=0;
      for(i=0; i<mic_fftsize; i++)
        {
        t2=micfft[micfft_px+2*i  ]*micfft[micfft_px+2*i  ]+
           micfft[+2*i+1]*micfft[micfft_px+2*i+1];
        if(t2>r2)r2=t2;
        }
      }          
    else
      {
      r2=0;
      }
//tx_ssb_buftim('b');
#if BUFBARS == TRUE
    if(timinfo_flag!=0)
      {
      i=(cliptimf_pa-cliptimf_px+micfft_bufsiz)&micfft_mask;
      i/=micfft_indicator_block;
      show_bufbar(TX_BUFBAR_CLIPTIMF,i);
      }
#endif
    muted_points=tx_ssb_step4(valid_bins,&pmax,&prat1a, &prat1b);
// The oversampling is mic_fftsize/bandwidth =siz/(siz-(ia1-ib1))
// The number of points in the oversampled signal is mic_fftsize/2
// for a single computation by tx_ssb_step4.
// so the number of un-muted points is mic_fftsize/2-muted_points.
// Mute if we have less than 5 points in the non-oversampled signal.
    if(valid_bins != 0 && mic_fftsize/2-muted_points < 5.F*t1)
      {
      cliptimf_mute[((cliptimf_pa-mic_fftsize+micfft_bufsiz)
                                    &micfft_mask)/mic_fftsize]=TRUE;
      valid_bins=0;
      }
// ************* Step 5. *************
// At this point we have applied an RF clipper by limiting power on a
// complex-valued time domain function. As a consequence we have 
// produced intermodulation products outside the desired passband.
// Go to the frequency domain and remove undesired frequencies.
#if BUFBARS == TRUE
  if(timinfo_flag!=0)
    {
    i=(clipfft_pa-clipfft_px+alc_bufsiz)&alc_mask;
    i/=alc_indicator_block;
    show_bufbar(TX_BUFBAR_CLIPFFT,i);
    }
#endif
  tx_ssb_step5();
// ************* Step 6. *************
// Go back to the time domain and store the signal in alctimf.
// Remember that we use sin squared windows and that
// the transforms overlap with 50%.
// Compute power and store the peak power with
// an exponential decay corresponding to a time constant
// of 50 milliseconds.
// We expand the fft size from mic_fftsize to tx_fftsiz2 because
// the fractional resampling that will follow this step needs
// the signal to be oversampled by a factor of four to avoid
// attenuation at high frequencies. The polynomial fitting
// works as a low pass filter and we do not want any attenuation
// at the high side of our passband.
#if BUFBARS == TRUE
    if(timinfo_flag!=0)
      {
      i=(alctimf_pa-(int)alctimf_fx+alc_bufsiz)&alc_mask;
      i/=alc_indicator_block;
      show_bufbar(TX_BUFBAR_ALCTIMF,i);
      }
#endif
    i=lir_tx_output_samples();
    if(tx_min_output_samples > i)tx_min_output_samples=i;
    if(i < snd[TXDA].tot_frames-2*snd[TXDA].block_frames)
      {    
      while(clipfft_px != clipfft_pa)
        {
        if(thread_command_flag[THREAD_TX_OUTPUT] != THRFLAG_ACTIVE)goto txout_x;
        tx_ssb_step6(&prat);
        lir_sched_yield();
        }
// ************* Step 7. *************
// Use the slowly decaying bi-directional peak power as an ALC
// but make sure we allow at least two data blocks un-processed
// to allow the backwards fall-off to become re-calculated.
// Using the slowly varying function for an ALC (= AM modulation)
// will not increase the bandwidth by more than the bandwidth
// of the modulation signal (50 ms or 20 Hz)
      while( ((alctimf_pa-alctimf_pb+alc_bufsiz)&alc_mask) >= 3*alc_fftsize)
        {
        if(thread_command_flag[THREAD_TX_OUTPUT] != THRFLAG_ACTIVE)goto txout_x;
        tx_ssb_step7(&prat);
        }
// ************* Step 8. *************
// In case we have enough signal in the buffer, start the output.
      if(thread_command_flag[THREAD_TX_OUTPUT] != THRFLAG_ACTIVE)goto txout_x;
#if BUFBARS == TRUE
    if(timinfo_flag!=0)
      {
      i=(tx_resamp_pa-tx_resamp_px+alc_bufsiz)&mictimf_mask;
      i/=alc_indicator_block;
      show_bufbar(TX_BUFBAR_RESAMP,i);
      }
#endif
      tx_ssb_step8();
      }
    }
// ******************** End of SSB processing ************************  
  break;

  case TXMODE_CW:
  mic_key_px=mic_key_pa;
  while(mic_key_px == mic_key_pa && !kill_all_flag)lir_await_event(EVENT_TX_INPUT);
  for(i=0;i<32; i++)
    {
    if(!kill_all_flag)lir_await_event(EVENT_TX_INPUT);
    }
  mic_key_px=mic_key_pa;
  for(i=0; i<txout_fftsize; i++)
    {
    txout[i]=0;;
    }
  txout_pa=(txout_px+txout_fftsize)&txout_mask;
wrmore:;  
  if(!kill_all_flag)lir_await_event(EVENT_TX_INPUT);
  tx_send_to_da();
  txout_pa=(txout_pa+txout_fftsize)&txout_mask;
  tx_min_output_samples=lir_tx_output_samples();
  do_cw_keying();    
  t1=tx_total_delay();
// t1 is the current delay from microphone top tx output.
// we want it to be tx_ideal_delay  
  if(t1 < tx_ideal_delay)goto wrmore;  
  t1-=tx_ideal_delay;
// We want to eliminate t1 from the total data in memory.
  if(mic_key_pa != mic_key_px)
    {
    k=((mic_key_pa-mic_key_px+mic_key_bufsiz)&mic_key_mask);
    t2=(float)k*(float)tx_pre_resample/(float)ui.tx_ad_speed;
    if(t2 < t1)
      {
      mic_key_px=mic_key_pa;
      }
    else
      {
      k=(int)(t1*(float)ui.tx_ad_speed/(float)tx_pre_resample);
      mic_key_px=((mic_key_px+k)&mic_key_mask);
      }
    }
  t1=tx_total_delay();
  t1-=tx_ideal_delay;
  local_workload_reset_flag=workload_reset_flag;
  while(!kill_all_flag && 
                    thread_command_flag[THREAD_TX_OUTPUT] == THRFLAG_ACTIVE)
    {
#if BUFBARS == TRUE
    if(timinfo_flag!=0)
      {
      i=(mic_key_pa-mic_key_px+mic_key_bufsiz)&mic_key_mask;
      i/=mic_key_indicator_block;
      show_bufbar(TX_BUFBAR_MIC_KEY,i);
      }
#endif
    if(timinfo_flag!=0)tx_cw_buftim('0');
    lir_sched_yield();
    mm=(mic_key_pa-mic_key_px+mic_key_bufsiz)&mic_key_mask;
    while(!kill_all_flag && mm <=1)
      {
      if(thread_command_flag[THREAD_TX_OUTPUT] != THRFLAG_ACTIVE)goto txout_x;
      if(local_workload_reset_flag != workload_reset_flag)
        {
        local_workload_reset_flag=workload_reset_flag;
        tx_min_output_samples=snd[TXDA].tot_bytes/snd[TXDA].framesize;
        }
      lir_await_event(EVENT_TX_INPUT);
      lir_sched_yield();
      mm=(mic_key_pa-mic_key_px+mic_key_bufsiz)&mic_key_mask;
      }
#if BUFBARS == TRUE
    if(timinfo_flag!=0)
      {
      i=(mic_key_pa-mic_key_px+mic_key_bufsiz)&mic_key_mask;
      i/=mic_key_indicator_block;
      show_bufbar(TX_BUFBAR_MIC_KEY,i);
      }
#endif
    do_cw_keying();    
    }
  break;
  }
txout_x:;  
thread_status_flag[THREAD_TX_OUTPUT]=THRFLAG_RETURNED;
if(old_ptt_state == PTT_ON)
  {
  old_ptt_state=PTT_OFF;
  hware_set_ptt(FALSE);
  }
while(thread_command_flag[THREAD_TX_OUTPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

void show_onoff_flag(void)
{
lir_text(0,MIC_KEY_LINE-2,"F3 to toggle output");
if(tx_onoff_flag == 0)
  {
  settextcolor(12);
  }
else
  {
  settextcolor(10);
  }
lir_text(20,MIC_KEY_LINE-2,"ON");
settextcolor(7);
lir_text(22,MIC_KEY_LINE-2,"/");
if(tx_onoff_flag == 0)
  {
  settextcolor(10);
  }
else
  {
  settextcolor(12);
  }
lir_text(23,MIC_KEY_LINE-2,"OFF");
settextcolor(7);
}

void cwproc_setup(void)
{
char color;
char *onoff[]={"Disabled", "Enabled"};
char s[256];
int i, k, pa, default_cwproc_no;
int old_tone_key, old_hand_key;
int line, isum;
float t1, t2, t3;
#if BUFBARS == TRUE
clear_bufbars();
#endif
rx_mode=MODE_NCW;
tg.spproc_no=0;
default_cwproc_no=tg.spproc_no;
if(read_txpar_file()==FALSE)
  {
  set_default_cwproc_parms();
  }
if(txcw.rise_time < 1000*CW_MIN_TIME_CONSTANT)
                         txcw.rise_time=1000*CW_MIN_TIME_CONSTANT;
if(txcw.rise_time > 1000*CW_MAX_TIME_CONSTANT)
                         txcw.rise_time=1000*CW_MAX_TIME_CONSTANT;
if(ui.tx_pilot_tone_prestart > 1000*CW_MAX_TIME_CONSTANT)
  {
  lirerr(1070);
  return;
  }
init_txmem_cwproc();
if(kill_all_flag)return;
linrad_thread_create(THREAD_TX_INPUT);
restart:;
tx_waveform_pointer=0;
if(txcw.rise_time < 1000*CW_MIN_TIME_CONSTANT)
                         txcw.rise_time=1000*CW_MIN_TIME_CONSTANT;
if(txcw.rise_time > 1000*CW_MAX_TIME_CONSTANT)
                         txcw.rise_time=1000*CW_MAX_TIME_CONSTANT;
make_tx_cw_waveform();
old_tone_key=-1;
old_hand_key=-1;
tot_key=0;
clear_screen();
lir_text(20,0,"Setup for CW keying.");
lir_text(1,2,"There are three ways to produce Morse code in Linrad:");
lir_text(1,3,"Press keys to toggle/change");
if(txcw.enable_tone_key == 0)settextcolor(7); else settextcolor(14);
sprintf(s,"'T' = Tone into the microphone input. (Fast keying)  [%s]",
                    onoff[txcw.enable_tone_key]);
line=5;
lir_text(1,line,s);
line++;
if(txcw.enable_hand_key == 0)settextcolor(7); else settextcolor(14);
sprintf(s,"'H' = Hardware Parallel port pin 13 or Si570. [%s]",
                                          onoff[txcw.enable_hand_key]);
lir_text(1,line,s);
line++;
if(txcw.enable_ascii == 0)settextcolor(7); else settextcolor(14);
sprintf(s,"'C' = Computer generated from ascii on keyboard.  [%s]",
                                          onoff[txcw.enable_ascii]);
lir_text(1,line,s);
line++;
settextcolor(7);
sprintf(s,"'A' = Rise time = %.2f ms.",txcw.rise_time*0.001);
lir_text(1,line,s);
line++;
sprintf(s,"'B' = Delay margin = %d ms.",txcw.delay_margin);
lir_text(1,line,s);
line++;
if(ui.transceiver_mode == 2)
  {
  sprintf(s,"'D' = Carrier frequency = %d Hz.",txcw.carrier_freq);
  lir_text(1,line,s);
  line++;
  }
sprintf(s,"'E' = Sidetone level = %d.",txcw.sidetone_ampl);
lir_text(1,line,s);
line++;
sprintf(s,"'F' = Sidetone frequency = %d Hz.",txcw.sidetone_freq);
lir_text(1,line,s);
line++;
make_txproc_filename();
sprintf(s,"Press 'S' to save %s, 'R' to read.  '+' or '-' to change file no.",
                                                 txproc_filename);
lir_text(0,MIC_KEY_LINE-3,s);
show_onoff_flag();
k=screen_last_col;
if(k>255)k=255;
for(i=0; i<k; i++)s[i]='-';
s[k]=0;
lir_text(0,MIC_KEY_LINE,s);
lir_text(0,MIC_KEY_LINE+8,s);
//                         0123456789012345678901234567890
lir_text(0,MIC_KEY_LINE+1,"Average       Max        Min");
lir_text(0,MIC_KEY_LINE+3,
"Set tone level and frequency for numbers well above 1.0 with key down.");
lir_text(0,MIC_KEY_LINE+4,
"Make sure the numbers are well below 1.0 with key up.");
lir_text(0,MIC_KEY_LINE+5,
"Typically Min(key down) > 10.0 and Max(key up) < 0.1.");
sprintf(s,
  "Your Nyquist frequency is %.1f kHz so you should use a keying tone",
                                                     0.0005*ui.tx_ad_speed);
lir_text(0,MIC_KEY_LINE+6,s);
lir_text(0,MIC_KEY_LINE+7,"that is a little lower than this.");
for(i=0; i<4; i++)lir_await_event(EVENT_TX_INPUT);
do_cw_keying();
i=lir_tx_output_samples();
tx_min_output_samples=i;
t1=0;
t2=0;
t3=BIGFLOAT;
isum=0;
timinfo_flag=1;
while(!kill_all_flag)
  {
  lir_await_event(EVENT_TX_INPUT);
  if(timinfo_flag!=0)tx_cw_buftim('0');
  pa=mic_key_px;
  while(pa != mic_key_pa)
    {
    isum++;
    t1+=tx_mic_key[pa];
    if(t2 < tx_mic_key[pa])t2=tx_mic_key[pa];
    if(t3 > tx_mic_key[pa])t3=tx_mic_key[pa];
    pa=(pa+1)&mic_key_mask;
    }
  if(isum > 4000)
    {
    t1/=isum;  
    sprintf(s,"%.3f",t1);
    lir_text(1,MIC_KEY_LINE+2,s);
    sprintf(s,"%.3f",t2);
    lir_text(12,MIC_KEY_LINE+2,s);
    sprintf(s,"%.3f",t3);
    lir_text(23,MIC_KEY_LINE+2,s);
    if(t1 < 1)
      {
      tone_key=0;
      }
    else
      {
      tone_key=1;
      }  
    if(tone_key != old_tone_key)
      {
      old_tone_key=tone_key;
      if(tone_key == 0)
        {
        color=7;
        }
      else
        {
        color=12;
        }
      lir_fillbox(5*text_width, KEY_INDICATOR_LINE*text_height, 10*text_width,
                                                        3*text_height,color);
      lir_text(8,KEY_INDICATOR_LINE+1,"TONE");
      lir_sched_yield();    
      }
    t1=0;
    t2=0;
    t3=BIGFLOAT;
    isum=0;
    }
  if(hand_key != old_hand_key)
    {
    old_hand_key=hand_key;
    if(hand_key == 0)
      {
      color=7;
      }
    else
      {
      color=12;
      }
    lir_fillbox(20*text_width, KEY_INDICATOR_LINE*text_height, 10*text_width,
                                                        3*text_height,color);
    lir_text(22,KEY_INDICATOR_LINE+1,"HAND");
    lir_sched_yield();    
    }
  do_cw_keying();
  lir_refresh_screen();
  test_keyboard();
  if(kill_all_flag)return;
  if(lir_inkey != 0)
    {
    process_current_lir_inkey();
    if(lir_inkey=='X')goto end_tx_setup;
    switch (lir_inkey)
      {
      case 'A':
      lir_text(0,TXCW_INPUT_LINE,"Rise time (ms):");
      t1=lir_get_float(15,TXCW_INPUT_LINE,6,
                                  CW_MIN_TIME_CONSTANT,CW_MAX_TIME_CONSTANT);
      txcw.rise_time=1000*t1;  
      break;
      
      case 'B':
      lir_text(0,TXCW_INPUT_LINE,"Output delay margin:");
      txcw.delay_margin=lir_get_integer(20,TXCW_INPUT_LINE,6,
                               CW_MIN_DELAY_MARGIN,CW_MAX_DELAY_MARGIN);
      break;

      case 'C':
      txcw.enable_ascii^=1;
      txcw.enable_ascii&=1;
      break;

      case 'D':
      if(ui.transceiver_mode == 2)
        {
        lir_text(0,TXCW_INPUT_LINE,"Carrier frequency in Hz:");
        i=ui.tx_da_speed/2;
        txcw.carrier_freq=lir_get_integer(24,TXCW_INPUT_LINE,6,-i,i);
        tg_basebfreq_hz=txcw.carrier_freq;
        set_hardware_tx_frequency();
        }
      break;

      case 'E':
      lir_text(0,TXCW_INPUT_LINE,"Sidetone level:");
      txcw.sidetone_ampl=lir_get_integer(20,TXCW_INPUT_LINE,6,0,1000);
      break;

      case 'F':
      lir_text(0,TXCW_INPUT_LINE,"Sidetone frequency:");
      txcw.sidetone_freq=lir_get_integer(20,TXCW_INPUT_LINE,6,50,10000);
      break;

      case 'T':
      txcw.enable_tone_key^=1;
      txcw.enable_tone_key&=1;
      break;
      
      case 'H':
      txcw.enable_hand_key^=1;
      txcw.enable_hand_key&=1;
      break;

      case 'S':
      clear_screen();
      save_tx_parms(TRUE);
      break;

      case 'R':
      clear_screen();
      read_txpar_file();
      tg_basebfreq_hz=txcw.carrier_freq;
      set_hardware_tx_frequency();
      break;

      case '+':
      tg.spproc_no++;
      if(tg.spproc_no > MAX_CWPROC_FILES)tg.spproc_no=MAX_CWPROC_FILES;
      break;

      case '-':
      tg.spproc_no--;
      if(tg.spproc_no < 0)tg.spproc_no=0;
      break;

      case F3_KEY:
      tx_onoff_flag++;
      tx_onoff_flag&=1;
      show_onoff_flag();
      break;
      }
    goto restart;
    }
  }
end_tx_setup:;
if(old_ptt_state == PTT_ON)
  {
  old_ptt_state=PTT_OFF;
  hware_set_ptt(FALSE);
  }
close_tx_sndout();
linrad_thread_stop_and_join(THREAD_TX_INPUT);
close_tx_sndin();
if(txmem_handle != NULL)free(txmem_handle);
txmem_handle=NULL;
lir_close_event(EVENT_TX_INPUT);
tg.spproc_no=default_cwproc_no;
}

void make_tx_phstep(void)
{
float t1;
t1=tg.passband_direction*2*PI_L*tg_basebfreq_hz/ui.tx_da_speed;
tx_daout_phstep_cos=cos(t1);
tx_daout_phstep_sin=sin(t1);
}

void init_txmem_common(float micblock_time)
{
get_tg_parms();
tx_pilot_tone=pow(10.0,-0.05*ui.tx_pilot_tone_db);
tx_output_amplitude=pow(10.0,-0.05*tg.level_db);
tx_daout_cos=1;
tx_daout_sin=0;
txout_p0=0;
micsize=ui.tx_ad_speed*micblock_time;
make_power_of_two(&micsize);
snd[TXAD].framesize=ui.tx_ad_channels*ui.tx_ad_bytes;

snd[TXAD].block_bytes=snd[TXAD].framesize*micsize;
lir_init_event(EVENT_TX_INPUT);
tx_output_flag=0;
tx_wsamps=0;
ptt_on_wait=-1;
ptt_off_wait=-1;
}

void set_txmem_sizes(float blocktime, int mode)
{
int i;
float adc_time;
mictimf_block=snd[TXAD].block_bytes/ui.tx_ad_bytes;
mictimf_adread_block=mictimf_block;
adc_time=(float)mictimf_block/(float)ui.tx_ad_speed;
while(adc_time > 1.415F*blocktime/ui.tx_ad_channels)
  {
  mictimf_block/=2;
  adc_time/=2;
  }
if(mode == MODE_SSB)
  {  
  while(adc_time < 0.706F*blocktime/ui.tx_ad_channels)
    {
    mictimf_block*=2;
    adc_time*=2;
    }
  }  
micsize=mictimf_block;
micn=0;
i=micsize;
while(i>1)
  {
  i>>=1;
  micn++;
  }
mic_sizhalf=micsize/2;
mictimf_bufsiz=16*mictimf_adread_block;
if(mictimf_bufsiz <16*mictimf_block)mictimf_bufsiz=16*mictimf_block;
while(mictimf_bufsiz < 2*screen_width)mictimf_bufsiz*=2;
mictimf_mask=mictimf_bufsiz-1;
mictimf_indicator_block=mictimf_bufsiz/INDICATOR_SIZE;
mictimf_block *= ui.tx_ad_channels;
}

void init_txmem_cwproc(void)
{
int i;
init_txmem_common(MICBLOCK_CWTIME);
if(tx_setup_flag)
  {
  open_tx_sndin();
  }
else
  {
  sys_func(THRFLAG_OPEN_TX_SNDIN);
  }  
if(kill_all_flag)return;
// Allow the input buffer to hold 80 milliseconds of data (or more)
// Keying with a tone into the microphone input should
// be done with a tone frequency near, but below the Nyquist frequency
// which is ui.tx_ad_speed/2.
// We do not want faster rise times than 1 ms for a dot length
// of 2 ms minimum or a dot rate of 250 Hz maximum.
// With at least 4 points on the rise time we need a sampling
// rate of 4kHz.
set_txmem_sizes(MICBLOCK_CWTIME, MODE_NCW);
tx_pre_resample=1;
i=1.05*ui.tx_ad_speed/2;
mic_key_block=mictimf_block;
mic_key_bufsiz=8*mictimf_adread_block;
if(rx_mode == MODE_RADAR)i/=2;
while(i > 4000)
  {
  i/=2;
  mic_key_block/=2;
  mic_key_bufsiz/=2;
  tx_pre_resample*=2;
  }
if(mic_key_bufsiz < INDICATOR_SIZE)mic_key_bufsiz=INDICATOR_SIZE;
make_power_of_two(&mic_key_bufsiz);
if(tx_pre_resample < 4 && rx_mode != MODE_RADAR)lirerr(1147);
tx_resample_ratio=(float)(ui.tx_da_speed)/ui.tx_ad_speed;
new_tx_resample_ratio=tx_resample_ratio;
mic_key_mask=mic_key_bufsiz-1;
mic_key_indicator_block=mic_key_bufsiz/INDICATOR_SIZE;
txout_fftsize=mic_key_block*tx_resample_ratio*tx_pre_resample;
make_power_of_two(&txout_fftsize);
// allocate a buffer for 0.5 seconds of data
txout_bufsiz=ui.tx_da_channels*ui.tx_da_speed/2;
make_power_of_two(&txout_bufsiz);
snd[TXDA].framesize=ui.tx_da_bytes*ui.tx_da_channels;
snd[TXDA].block_bytes=txout_fftsize*snd[TXDA].framesize;
if(tx_setup_flag)
  {
  open_tx_sndout();
  }
else
  {
  sys_func(THRFLAG_OPEN_TX_SNDOUT);
  }  
if(txda_status != LIR_OK)
  {
  lirerr(1407);
  }
if(kill_all_flag)return;
tx_ssb_resamp_block_factor=1;
txout_sizhalf=(snd[TXDA].block_bytes/snd[TXDA].framesize)/2;
txout_fftsize=2*txout_sizhalf;
if(txout_bufsiz < txout_fftsize*8)txout_bufsiz=8*txout_fftsize;
txout_mask=txout_bufsiz-1;
txout_indicator_block=txout_bufsiz/INDICATOR_SIZE;

tx_ideal_delay=1.5F/snd[TXDA].interrupt_rate+
                       1.5F/snd[TXAD].interrupt_rate+0.001*txcw.delay_margin;
max_tx_cw_waveform_points=ui.tx_da_speed*(0.0022*CW_MAX_TIME_CONSTANT);
init_memalloc(txmem, MAX_TXMEM_ARRAYS);
if(ui.tx_ad_bytes == 2)
  {
  mem(51,&mictimf_shi,mictimf_bufsiz*sizeof(short int),0);
  }
else
  {
  mem(51,&mictimf_int,mictimf_bufsiz*sizeof(int),0);
  }
mem(52,&tx_mic_key,mic_key_bufsiz*sizeof(float),0);
mem(53,&txout,txout_bufsiz*sizeof(float),0);
mem(54,&txout_waveform,max_tx_cw_waveform_points*sizeof(float),0);
if(ui.tx_da_bytes==4)
  {
  mem(55,&tx_out_int,txout_bufsiz*sizeof(int),0);
  }
else
  {  
  mem(56,&tx_out_shi,txout_bufsiz*sizeof(short int),0);
  }
txmem_size=memalloc(&txmem_handle,"txmem");
if(txmem_size == 0)
  {
  lirerr(1261);
  return;
  }
tx_wrblock_no=0;
tx_wttim_ref=0;
tx_wttim_sum=0;
tx_wttim_diff=0;
tg_basebfreq_hz=txcw.carrier_freq;
set_hardware_tx_frequency();
make_tx_phstep();
make_tx_modes();  
make_tx_cw_waveform();
mic_key_pa=0;
mic_key_px=0;
mictimf_pa=0;
mictimf_px=0;
txout_pa=0;
txout_px=0;
mic_key_fx=0;
tx_waveform_pointer=0;
tx_wrblock_maxcnt=(int)(5.F*snd[TXDA].interrupt_rate);
snd[TXAD].open_flag=CALLBACK_CMD_ACTIVE;
snd[TXDA].open_flag=CALLBACK_CMD_ACTIVE;
txda_indicator_block=snd[TXDA].tot_frames/INDICATOR_SIZE;
}

void init_txmem_spproc(void)
{
int j;
int i, k;
float t1;
int max_fftsize;
// Speech processing is done in both the time domain and in the
// frequency domain.
// The length of an FFT block determines the frequency
// resolution as well as the total time delay through
// the processes.
// Set micsize to span a suitable time and suggest the same
// time span for a dma buffer to the tx input soundcard.
t1=0.001F*(float)txssb.micblock_time;
init_txmem_common(t1);
if(tx_setup_flag)
  {
  open_tx_sndin();
  }
else
  {
  sys_func(THRFLAG_OPEN_TX_SNDIN);
  }  
if(kill_all_flag)return;
if(txad_status != LIR_OK)
  {
  lirerr(1410);
  return;
  }
// The open function may have changed the block size.
// Set micsize to fit the dma buffer we actually have.
set_txmem_sizes(t1,MODE_SSB);
max_fftsize=micsize;
txad_hz_per_bin=(0.5*ui.tx_ad_speed)/micsize;
tx_lowest_bin=txssb.minfreq/txad_hz_per_bin;
if(tx_lowest_bin > micsize/3)tx_lowest_bin=micsize/3;
if(tx_lowest_bin < 1)tx_lowest_bin=1;
tx_highest_bin=txssb.maxfreq/txad_hz_per_bin;
if(tx_highest_bin > micsize/2-2)tx_highest_bin=micsize/2-2;
if(tx_highest_bin < tx_lowest_bin+1)tx_highest_bin=tx_lowest_bin+1;
t1=(tx_highest_bin-tx_lowest_bin+1)*txad_hz_per_bin;
if(ui.tx_da_channels == 1)t1*=2;
if(t1 > 0.75*ui.tx_da_speed)
  {
  t1=ui.tx_da_speed;
  if(ui.tx_da_channels == 1)t1/=2;
  t1/=txad_hz_per_bin;
  tx_highest_bin=tx_lowest_bin+t1;
  if(tx_highest_bin > micsize/2-2)tx_highest_bin=micsize/2-2;
  }
mic_fftsize=micsize;
mic_fftn=micn;
k=1.2*(tx_highest_bin-tx_lowest_bin);
micdisplay_fftsize=k;
make_power_of_two(&micdisplay_fftsize);
// The microphone sampling speed may be low since we have no interest
// in frequencies above 2.5 kHz or so.
// An amplitude clipper on a complex-valued signal must however 
// be oversampled.
// Oversampling by 16 will place most of the overtones generated by 
// the clipping process outside the passband. Without oversampling 
// all the overtones will alias into the passband and destroy the
// clipping process.
k*=16;
while(k < mic_fftsize)
  {
  mic_fftsize/=2;
  mic_fftn--;
  }
k/=2;
while(k > mic_fftsize)
  {
  mic_fftsize*=2;
  mic_fftn++;
  }
mic_to_spproc_resample_ratio=(float)mic_fftsize/(float)micsize;  
DEB"\nmic_to_spproc_resample_ratio %f",mic_to_spproc_resample_ratio);
if(max_fftsize < mic_fftsize)max_fftsize=mic_fftsize;
mic_sizhalf=mic_fftsize/2;
micfft_block=2*mic_fftsize;
micfft_bufsiz=32*mic_fftsize;
micfft_indicator_block=micfft_bufsiz/INDICATOR_SIZE;
micfft_mask=micfft_bufsiz-1;
i=(tx_lowest_bin-tx_highest_bin)/2;
if(i>-1)i=-1;
tx_filter_ia1=mic_fftsize+i-1;
tx_filter_ib1=tx_filter_ia1-2*i-mic_fftsize+1;
k=mic_fftsize/tx_filter_ib1;
alc_fftn=mic_fftn;
alc_fftsize=mic_fftsize;
tx_pre_resample=1;
k++;
while(k<8)
  {
  tx_pre_resample*=2;
  k*=2;
  alc_fftn++;
  alc_fftsize*=2;
  }
if(max_fftsize < alc_fftsize)max_fftsize=alc_fftsize;
// We allow data to stack up in the alc buffer when the resampler
// runs too fast. Make a big buffer.
alc_bufsiz=64*alc_fftsize;
alc_mask=alc_bufsiz-1;
alc_indicator_block=alc_bufsiz/INDICATOR_SIZE;
alc_sizhalf=alc_fftsize/2;      
alc_block=2*alc_fftsize;
tx_analyze_size=8*alc_fftsize;
tx_analyze_sizhalf=tx_analyze_size/2;
tx_analyze_fftn=alc_fftn+3;
tx_resample_ratio=(float)(2*ui.tx_da_speed)/ui.tx_ad_speed;
tx_output_upsample=0.5;
txout_fftsize=micsize;
txout_fftn=micn;
while(tx_resample_ratio > 1.5)
  {
  tx_resample_ratio/=2;
  tx_output_upsample*=2;
  txout_fftsize*=2;
  txout_fftn++;
  }
snd[TXDA].framesize=ui.tx_da_bytes*ui.tx_da_channels;
snd[TXDA].block_bytes=txout_fftsize*snd[TXDA].framesize/2;
snd[TXDA].block_frames=snd[TXDA].block_bytes/snd[TXDA].framesize;
// allocate a buffer for 0.5 seconds of data
txout_bufsiz=ui.tx_da_channels*ui.tx_da_speed/2;
make_power_of_two(&txout_bufsiz);
j=snd[TXDA].block_bytes;
if(tx_setup_flag)
  {
  open_tx_sndout();
  }
else
  {
  sys_func(THRFLAG_OPEN_TX_SNDOUT);
  } 
if(kill_all_flag)return;
if(txda_status != LIR_OK)
  {
  lirerr(1411);
  return;
  }
tx_ssb_resamp_block_factor=1;
while(j < snd[TXDA].block_bytes)
  {
  j*=2;
  tx_ssb_resamp_block_factor*=2;
  }
while(j > snd[TXDA].block_bytes)
  {
  j/=2;
  tx_ssb_resamp_block_factor/=2;
  }
if(max_fftsize < txout_fftsize)max_fftsize=txout_fftsize;
if(txout_bufsiz < txout_fftsize*8)txout_bufsiz=8*txout_fftsize;
txout_mask=txout_bufsiz-1;
// Time for minimum delay data expressed as samples of txad.  
t1=1.5*mic_fftsize;  // Samples in mictimf and cliptimf.
t1+=0.5*(1+SSB_DELAY_EXTRA)*alc_fftsize/tx_pre_resample;
tx_ideal_delay=2/snd[TXDA].interrupt_rate+2*t1/ui.tx_ad_speed;
new_tx_resample_ratio=tx_resample_ratio;
txout_sizhalf=txout_fftsize/2;
if(max_fftsize < screen_width)max_fftsize=screen_width;
make_power_of_two(&max_fftsize);
init_memalloc(txmem, MAX_TXMEM_ARRAYS);
if(ui.tx_ad_bytes == 2)
  {
  mem(1,&mictimf_shi,mictimf_bufsiz*sizeof(short int),0);
  }
else
  {
  mem(1,&mictimf_int,mictimf_bufsiz*sizeof(int),0);
  }
mem(2,&micfft,micfft_bufsiz*sizeof(float),0);
mem(3,&cliptimf,micfft_bufsiz*sizeof(float),0);
mem(4,&mic_tmp,2*micsize*sizeof(float),0);
mem(5,&mic_table,micsize*sizeof(COSIN_TABLE),0);
mem(6,&mic_permute,2*micsize*sizeof(short int),0);
mem(7,&mic_win,(1+micsize)*sizeof(float),0);
mem(8,&mic_filter,mic_fftsize*sizeof(float),0);
mem(9,&micfft_table,mic_fftsize*sizeof(COSIN_TABLE)/2,0);
mem(10,&micfft_permute,mic_fftsize*sizeof(short int),0);
mem(11,&micfft_win,(2+mic_sizhalf)*sizeof(float),0);
mem(12,&clipfft,alc_bufsiz*sizeof(float),0);
mem(13,&alctimf,alc_bufsiz*sizeof(float),0);
mem(14,&alctimf_pwrf,alc_bufsiz*sizeof(float)/2,0);
mem(15,&alctimf_pwrd,alc_bufsiz*sizeof(float)/2,0);
mem(16,&tx_resamp,2*txout_fftsize*sizeof(float),0);
mem(17,&resamp_tmp,alc_fftsize*sizeof(float),0);
mem(18,&alc_permute,alc_fftsize*sizeof(short int),0);
mem(19,&alc_table,alc_sizhalf*sizeof(COSIN_TABLE),0);
mem(20,&alc_win,(1+alc_sizhalf)*sizeof(float),0);
mem(21,&txout,txout_bufsiz*sizeof(float),0);
mem(22,&txout_permute,txout_fftsize*sizeof(short int),0);
mem(23,&txout_table,txout_sizhalf*sizeof(COSIN_TABLE),0);
mem(24,&txout_tmp,txout_fftsize*sizeof(float),0);
if(ui.tx_da_bytes==4)
  {
  mem(25,&tx_out_int,txout_bufsiz*sizeof(int),0);
  }
else
  {  
  mem(25,&tx_out_shi,txout_bufsiz*sizeof(short int),0);
  }
mem(26,&cliptimf_mute,micfft_bufsiz*sizeof(char)/mic_fftsize,0);  
mem(27,&clipfft_mute,alc_bufsiz*sizeof(char)/alc_block,0);  
mem(28,&alctimf_mute,alc_bufsiz*sizeof(char)/alc_fftsize,0);  
if(tx_setup_flag == TRUE)
  {
  tx_oscilloscope_tmp_size=2*(max_fftsize+screen_width);
  make_power_of_two(&tx_oscilloscope_tmp_size);
  mem(29,&tx_oscilloscope,2*screen_width*sizeof(short int),0);
  mem(30,&tx_oscilloscope_tmp,tx_oscilloscope_tmp_size*sizeof(float),0);
  mem(31,&cliptimf_oscilloscope,micfft_bufsiz*sizeof(float),0);
  mem(32,&alctimf_raw,alc_bufsiz*sizeof(float),0);
  mem(33,&tx_analyze_table,tx_analyze_size*sizeof(COSIN_TABLE)/2,0);
  mem(34,&tx_analyze_win,(tx_analyze_size/2+1)*sizeof(float),0);
  mem(35,&tx_analyze_permute,tx_analyze_size*sizeof(short int),0);
  }
txmem_size=memalloc(&txmem_handle,"txmem");
if(txmem_size == 0)
  {
  lirerr(1261);
  return;
  }
if(tx_setup_flag == TRUE)
  {
  tx_oscilloscope_min=(TXPAR_INPUT_LINE+1)*text_height+1;
  tx_oscilloscope_max=screen_height-1;
  tx_oscilloscope_zero=(tx_oscilloscope_max+tx_oscilloscope_min)/2;
  tx_oscilloscope_min=2*tx_oscilloscope_zero-tx_oscilloscope_max;
  for(i=0; i<2*screen_width; i++)tx_oscilloscope[i]=0;
  tx_oscilloscope_time=0;
  tx_oscilloscope_ypixels=tx_oscilloscope_max-tx_oscilloscope_zero;
  init_fft(0,tx_analyze_fftn, tx_analyze_size, tx_analyze_table, 
                                                    tx_analyze_permute);
  make_window(2,tx_analyze_size/2, 8, tx_analyze_win);
  t1=3.F/(float)tx_analyze_size;
  for(i=0; i<tx_analyze_sizhalf; i++)
    {
    tx_analyze_win[i]*=t1;
    }
  }
make_permute(2, micn, micsize, mic_permute);
make_window(2,micsize, 2, mic_win);
if(ui.tx_ad_bytes != 2)
  {
  for(i=0; i<micsize; i++)mic_win[i]/=0x10000;
  }
t1=2/(1.62*0x7fff*mic_sizhalf);
t1*=(float)mic_fftsize/(float)micsize;  
for(i=0; i<micsize; i++)mic_win[i]*=t1;
make_sincos(2, micsize, mic_table); 
init_fft(0,mic_fftn, mic_fftsize, micfft_table, micfft_permute);
make_window(2,mic_sizhalf, 2, micfft_win);
micfft_winfac=1.62*mic_fftsize;
t1=1/micfft_winfac;
for(i=0; i<mic_sizhalf; i++)
  {
  micfft_win[i]*=t1;
  }
micfft_win[mic_sizhalf]=micfft_win[mic_sizhalf-1];
micfft_win[mic_sizhalf+1]=micfft_win[mic_sizhalf-1];
micfft_winfac=1/micfft_win[mic_sizhalf-1];
for(i=0; i<micfft_bufsiz; i++)
  {
  micfft[i]=0;
  cliptimf[i]=0;
  }
init_fft(0,alc_fftn, alc_fftsize, alc_table, alc_permute);
make_window(2,alc_sizhalf, 2, alc_win);
t1=1/(1.62*alc_fftsize);
for(i=0; i<alc_sizhalf; i++)
  {
  alc_win[i]*=t1;
  }
for(i=0; i<alc_bufsiz; i++)
  {
  alctimf[i]=0;
  clipfft[i]=0;
  }
for(i=0; i<alc_bufsiz/alc_block; i++)clipfft_mute[i]=0;
for(i=0; i<alc_bufsiz/2; i++)alctimf_pwrf[i]=0;
init_fft(0,txout_fftn, txout_fftsize, txout_table, txout_permute);
for(i=0; i<2*txout_fftsize; i++)tx_resamp[i]=0;
tx_filter_ia1=mic_fftsize+(tx_lowest_bin/2-tx_highest_bin/2-1);
tx_filter_ia2=alc_fftsize+(tx_lowest_bin/2-tx_highest_bin/2-1);
tx_filter_ia3=txout_fftsize+(tx_lowest_bin/2-tx_highest_bin/2-1);
tx_filter_ib1=tx_filter_ia1+tx_highest_bin-tx_lowest_bin-mic_fftsize;
tx_filter_ib2=tx_filter_ia2+tx_highest_bin-tx_lowest_bin-alc_fftsize;
tx_filter_ib3=tx_filter_ia3+tx_highest_bin-tx_lowest_bin-txout_fftsize;
tx_fft_phase_flag=(mic_fftsize-tx_filter_ia1+tx_lowest_bin)&1;
tx_ssbproc_fqshift=(float)(tx_highest_bin-tx_filter_ib1)*txad_hz_per_bin;
// Construct the microphone input filter from the
// parameters supplied by the user.
for(i=0; i<mic_fftsize; i++)mic_filter[i]=0;
// Store the desired frequency response in dB.
t1=0;
for(i=tx_lowest_bin; i<=tx_highest_bin; i++)
  {
  mic_filter[i]=t1;
  t1+=0.001*txssb.slope*txad_hz_per_bin;
  }
k=(tx_highest_bin-tx_lowest_bin)/4;
t1=0;
for(i=tx_lowest_bin+k; i>=tx_lowest_bin; i--)
  {
  t1+=0.001*txssb.bass*txad_hz_per_bin;
  mic_filter[i]+=t1;
  }
t1=0;
for(i=tx_highest_bin-k; i<=tx_highest_bin; i++)
  {
  t1+=0.001*txssb.treble*txad_hz_per_bin;
  mic_filter[i]+=t1;
  }
// Convert from dB to linear scale (amplitude) and normalize
// the filter curve.
t1=0;
for(i=tx_lowest_bin; i<=tx_highest_bin; i++)
  {
  mic_filter[i]=pow(10.,0.05*mic_filter[i]);
  t1+=mic_filter[i]*mic_filter[i];
  }
t1=1/sqrt(t1/(tx_highest_bin-tx_lowest_bin+1));
for(i=tx_lowest_bin; i<=tx_highest_bin; i++)
  {
  mic_filter[i]*=t1;
  }
mic_filter[tx_lowest_bin]*=0.5F;
mic_filter[tx_lowest_bin+1]*=0.875F;
mic_filter[tx_highest_bin]*=0.5F;
mic_filter[tx_highest_bin-1]*=0.875F;
micfft_bin_minpower_ref=pow(10.,0.1*(txssb.mic_f_threshold-100));
micfft_minpower_ref=pow(10.,0.1*(txssb.mic_t_threshold+txssb.mic_f_threshold-90));
tx_agc_factor=1;
tx_agc_decay=pow(2.718,-100.0/(txssb.mic_agc_time*txad_hz_per_bin));
txpwr_decay=pow(2.718,-2000.0/(ui.tx_ad_speed*txssb.alc_time));
af_gain=pow(10.0,0.05*txssb.mic_gain)/8;
rf_gain=pow(10.0,0.05*txssb.rf1_gain);
tx_minlevel=rf_gain*MAX_DYNRANGE*pow(10,0.05*txssb.mic_t_threshold);
tx_forwardpwr=0;
tx_backpwr=0;
tx_ph1=1;
tx_wrblock_no=0;
tx_wttim_ref=0;
tx_wttim_sum=0;
tx_wttim_diff=0;
memcheck(1,txmem,&txmem_handle);
tg_basebfreq_hz=tx_hware_fqshift+tx_ssbproc_fqshift;
make_tx_modes();
make_tx_phstep();
tx_resamp_ampfac1=1;
tx_resamp_ampfac2=1;
micfft_pa=0;
micfft_px=0;
mictimf_pa=0;
mictimf_px=0;
cliptimf_pa=0;
cliptimf_px=0;
clipfft_pa=0;
clipfft_px=0;
alctimf_pa=0;
alctimf_pb=0;
alctimf_fx=0;
txout_pa=0;
txout_px=0;
tx_wrblock_maxcnt=(int)(5.F/snd[TXDA].interrupt_rate);
snd[TXAD].open_flag=CALLBACK_CMD_ACTIVE;
snd[TXDA].open_flag=CALLBACK_CMD_ACTIVE;
txda_indicator_block=snd[TXDA].tot_frames/INDICATOR_SIZE;
DEB"\nmicsize=%d   mic_fftsize = %d   alc_fftsize =%d   txout_fftsize =%d",
                            micsize, mic_fftsize, alc_fftsize, txout_fftsize);
}

void tx_input(void)
{
int i, ia, ib, pa, pb, pc;
int k;
int refresh_counter;
int max_refresh_counter;
int twosiz, ssb_bufmin;
float *z;
float t1;
double dt1;
make_txproc_filename();
twosiz=2*micsize;
ssb_bufmin=twosiz*ui.tx_ad_channels;
pb=mictimf_px;
thread_status_flag[THREAD_TX_INPUT]=THRFLAG_ACTIVE;
// Clear the device buffer.
dt1=current_time();
while(!kill_all_flag && current_time()-dt1 < 0.3F)
  {
  if ( (ui.use_alsa&PORTAUDIO_TX_IN) != 0)
    {
    while (!kill_all_flag && mictimf_pc == mictimf_pa)
      {
      lir_await_event(EVENT_PORTAUDIO_TXAD_READY);
      }
    }
  else
    {
    if(ui.tx_ad_bytes == 2)
      {
      lir_tx_adread((char*)&mictimf_shi[mictimf_pa]);
      }
    else
      {
      lir_tx_adread((char*)&mictimf_int[mictimf_pa]); 
      }
    }  
  mictimf_pa=(mictimf_pa+mictimf_adread_block)&mictimf_mask;
  }
mictimf_px=mictimf_pa;
t1=(float)mictimf_adread_block/(float)ui.tx_ad_speed;
t1/=(float)(ui.tx_ad_channels*ui.tx_ad_bytes);
max_refresh_counter=.02F/t1;
if(max_refresh_counter==0)max_refresh_counter=1;
refresh_counter=max_refresh_counter;
while(!kill_all_flag && 
                    thread_command_flag[THREAD_TX_INPUT] == THRFLAG_ACTIVE)
  {
  if(tx_setup_flag)
    {
    refresh_counter--;
    if( refresh_counter == 0)
      {
      refresh_counter=max_refresh_counter;
      lir_refresh_screen();
      memcheck(2,txmem,&txmem_handle);
      }
    }  
#if BUFBARS == TRUE
    if(timinfo_flag!=0)
      {
      i=(mictimf_pa-mictimf_px+mictimf_bufsiz)&mictimf_mask;
      i/=mictimf_indicator_block;
      show_bufbar(TX_BUFBAR_MICTIMF,i);
      }
#endif
  if ( (ui.use_alsa&PORTAUDIO_TX_IN) != 0)
    {
    while (!kill_all_flag && mictimf_pc == mictimf_pa)
      {
      lir_await_event(EVENT_PORTAUDIO_TXAD_READY);
      }
    }
  else
    {  
    if( ((mictimf_pa-mictimf_px+mictimf_bufsiz)&mictimf_mask) < 2*ssb_bufmin)
      {
      if(ui.tx_ad_bytes == 2)
        {
        lir_tx_adread((char*)&mictimf_shi[mictimf_pa]);
        }
      else
        {
        lir_tx_adread((char*)&mictimf_int[mictimf_pa]); 
        }
      }  
    }
  mictimf_pa=(mictimf_pa+mictimf_adread_block)&mictimf_mask;
  switch (tx_mode)
    {
    case TXMODE_SSB:
    while( !kill_all_flag && 
           ((mictimf_pa-mictimf_px+mictimf_bufsiz)&mictimf_mask) >= ssb_bufmin)
      {
// Copy the input to micfft while multiplying with the window function.
      ib=twosiz-1;
      pa=mictimf_px;
      switch(tx_input_mode)
        {
        case 0:
// One channel, two bytes.        
        pb=(pa+ib)&mictimf_mask;
        for( ia=0; ia<micsize; ia++)
          {
          mic_tmp[mic_permute[ia]]=(float)(mictimf_shi[pa])*mic_win[ia];
          if(abs(mictimf_shi[pa]) > tx_ad_maxamp)
            {
            tx_ad_maxamp=abs(mictimf_shi[pa]);
            }
          mic_tmp[mic_permute[ib]]=(float)(mictimf_shi[pb])*mic_win[ia];
          if(abs(mictimf_shi[pb]) > tx_ad_maxamp)
            {
            tx_ad_maxamp=abs(mictimf_shi[pb]);
            }
          ib--; 
          pa=(pa+1)&mictimf_mask;
          pb=(pb+mictimf_mask)&mictimf_mask;
          }
        break;
        
        case 1:
// In case there are two channels for the microphone, just use one of them!!
        pb=(pa+2*ib)&mictimf_mask;
        for( ia=0; ia<micsize; ia++)
          {
          mic_tmp[mic_permute[ia]]=mictimf_shi[pa]*mic_win[ia];
          if(abs(mictimf_shi[pa]) > tx_ad_maxamp)
            {
            tx_ad_maxamp=abs(mictimf_shi[pa]);
            }
          mic_tmp[mic_permute[ib]]=mictimf_shi[pb]*mic_win[ia];
          if(abs(mictimf_shi[pb]) > tx_ad_maxamp)
            {
            tx_ad_maxamp=abs(mictimf_shi[pb]);
            }
          ib--; 
          pa=(pa+2)&mictimf_mask;
          pb=(pb+mictimf_mask-1)&mictimf_mask;
          }
        break;
        
        case 2:
        pb=(pa+ib)&mictimf_mask;
        for( ia=0; ia<micsize; ia++)
          {
          mic_tmp[mic_permute[ia]]=mictimf_int[pa]*mic_win[ia];
          if(abs(mictimf_int[pa]) > tx_ad_maxamp)
            {
            tx_ad_maxamp=abs(mictimf_int[pa]);
            }
          mic_tmp[mic_permute[ib]]=mictimf_int[pb]*mic_win[ia];
          if(abs(mictimf_int[pb]) > tx_ad_maxamp)
            {
            tx_ad_maxamp=abs(mictimf_int[pb]);
            }
          ib--; 
          pa=(pa+1)&mictimf_mask;
          pb=(pb+mictimf_mask)&mictimf_mask;
          }
        break;
        
        case 3:
// In case there are two channels for the microphone, just use one of them!!
        pb=(pa+2*ib)&mictimf_mask;
        for( ia=0; ia<micsize; ia++)
          {
          mic_tmp[mic_permute[ia]]=mictimf_int[pa]*mic_win[ia];
          if(abs(mictimf_int[pa]) > tx_ad_maxamp)
            {
            tx_ad_maxamp=abs(mictimf_int[pa]);
            }
          mic_tmp[mic_permute[ib]]=mictimf_int[pb]*mic_win[ia];
          if(abs(mictimf_int[pb]) > tx_ad_maxamp)
            {
            tx_ad_maxamp=abs(mictimf_int[pb]);
            }
          ib--; 
          pa=(pa+2)&mictimf_mask;
          pb=(pb+mictimf_mask-1)&mictimf_mask;
          }
        }
// The microphone signal should be maximum 0x7fff or 0x7fffffffffff
// depending on whether 16 or 32 bit format is selected. 
      mictimf_px=(mictimf_px+mictimf_block)&mictimf_mask;
      fft_real_to_hermitian( mic_tmp, twosiz, micn, mic_table);
// Output is {Re(z^[0]),...,Re(z^[n/2),Im(z^[n/2-1]),...,Im(z^[1]).
// Clear those parts of the spectrum that we do not want and
// transfer the rest of the points to micfft while multiplying
// with our filter function. 
// Place the spectrum center at frequency zero so we can use
// the lowest possible sampling rate in further processing steps.
      z=&micfft[micfft_pa];
      pc=2*(tx_highest_bin/2-tx_lowest_bin/2);
      k=micfft_block-pc-2;
      if( (tx_highest_bin&1) != 0 && (tx_lowest_bin&1) == 0)pc+=2;
      if( (tx_highest_bin&1) == 0 && (tx_lowest_bin&1) != 0)pc-=2;
      while(pc < k)
        {
        z[pc  ]=0;
        z[pc+1]=0;
        pc+=2;
        }
      k=tx_lowest_bin;
      pc=2*tx_filter_ia1;
      while(pc < 2*mic_fftsize)
        {
        z[pc+1]=mic_tmp[k  ]*mic_filter[k];
        z[pc  ]=mic_tmp[twosiz-k]*mic_filter[k];
        pc+=2;
        k++;
        }
      pc=0;
      while(k <= tx_highest_bin)
        {
        z[pc+1]=mic_tmp[k  ]*mic_filter[k];
        z[pc  ]=mic_tmp[twosiz-k]*mic_filter[k];
        pc+=2;
        k++;
        }
      micfft_pa=(micfft_pa+micfft_block)&micfft_mask;
      if(tx_setup_flag == TRUE)
        {
        if(tx_setup_mode == TX_SETUP_AD || tx_setup_mode == TX_SETUP_MUTE)
          {
          if(current_time()-tx_oscilloscope_time >0.2F)
            {
            if(tx_display_mode == TX_DISPLAY_MICTIMF && 
                                                 tx_setup_mode == TX_SETUP_AD)
              {          
              pa=mictimf_pa;
              pb=(pa-screen_width+mictimf_bufsiz)&mictimf_mask;
              i=0;
              switch(tx_input_mode)
                {
                case 0:
// One channel, two bytes.        
                while(pb != pa)
                  {
                  lir_setpixel(i,tx_oscilloscope[i],0);
                  tx_oscilloscope[i]=mictimf_shi[pb]>>8;
                  pb=(pb+1)&mictimf_mask;
                  i++;
                  }
                break;
        
                case 1:
// In case there are two channels for the microphone, just use one of them!!
                pb=(pb-screen_width+mictimf_bufsiz)&mictimf_mask;
                while(pb != pa)
                  {
                  lir_setpixel(i,tx_oscilloscope[i],0);
                  tx_oscilloscope[i]=mictimf_shi[pb]>>8;
                  pb=(pb+2)&mictimf_mask;
                  i++;
                  }
                break;
       
                case 2:
                while(pb != pa)
                  {
                  lir_setpixel(i,tx_oscilloscope[i],0);
                  tx_oscilloscope[i]=mictimf_int[pb]>>24;
                  pb=(pb+1)&mictimf_mask;
                  i++;
                  }
                break;
        
                case 3:
// In case there are two channels for the microphone, just use one of them!!
                pb=(pb-screen_width+mictimf_bufsiz)&mictimf_mask;
                while(pb != pa)
                  {
                  lir_setpixel(i,tx_oscilloscope[i],0);
                  tx_oscilloscope[i]=mictimf_int[pb]>>24;
                  pb=(pb+2)&mictimf_mask;
                  i++;
                  }
                break;
                }
              for(i=0; i<screen_width; i++)
                {
                tx_oscilloscope[i]*=txtrace_gain/.3;
                tx_oscilloscope[i]+=tx_oscilloscope_zero;
                if(tx_oscilloscope[i] < tx_oscilloscope_min)
                                        tx_oscilloscope[i]=tx_oscilloscope_min;
                if(tx_oscilloscope[i] > tx_oscilloscope_max)
                                        tx_oscilloscope[i]=tx_oscilloscope_max;
                lir_setpixel(i,tx_oscilloscope[i],15);
                }
              tx_oscilloscope_time=recent_time;
              }
            if(tx_display_mode == TX_DISPLAY_MICSPECTR &&
                                              tx_setup_mode == TX_SETUP_AD)
              {          
              for(i=0; i<micsize; i++)
                {
                tx_oscilloscope_tmp[2*i]=mic_tmp[i];
                tx_oscilloscope_tmp[2*i+1]=mic_tmp[twosiz-i];
                }
              display_color[0]=12;
              show_txfft(tx_oscilloscope_tmp,micfft_bin_minpower,0,micsize);          
              tx_oscilloscope_time=recent_time;
              }
            if(tx_display_mode == TX_DISPLAY_MICSPECTR_FILTERED)
              {          
              display_color[0]=10;
              contract_tx_spectrum(z);
              show_txfft(tx_oscilloscope_tmp,
                                  micfft_bin_minpower,0,micdisplay_fftsize);
              tx_oscilloscope_time=recent_time;
              }
            if(tx_setup_mode == TX_SETUP_AD && 
                             tx_display_mode == TX_DISPLAY_FILTER)
              {          
              for(i=0; i<2*tx_lowest_bin; i++)
                {
                tx_oscilloscope_tmp[i]=0;
                }
              i=tx_lowest_bin;
              pc=2*micdisplay_fftsize;
              while(i <= tx_highest_bin)
                {
                tx_oscilloscope_tmp[2*i+1]=mic_filter[i];
                tx_oscilloscope_tmp[2*i  ]=mic_filter[i];
                i++;
                }
              for(i=2*(tx_highest_bin+1); i<pc; i++)
                {
                tx_oscilloscope_tmp[i]=0;
                }
              display_color[0]=11;
              show_txfft(tx_oscilloscope_tmp,
                                  0,0,micdisplay_fftsize);
              tx_oscilloscope_time=recent_time;
              }
            if(tx_display_mode == TX_DISPLAY_MICSPECTR_FILTALL)
              {          
              display_color[0]=14;
              show_txfft(&micfft[micfft_px],micfft_bin_minpower,0,mic_fftsize);
              tx_oscilloscope_time=recent_time;
              } 
            lir_refresh_screen();
            }
          }
        }
      lir_sched_yield();
      lir_set_event(EVENT_TX_INPUT);
      }  
    break;

    case TXMODE_CW:
    if(mictimf_px != mictimf_pa)
      {
      switch(tx_input_mode)
        {
        case 0:
        while(mictimf_px != mictimf_pa)
          {
          t1=0;
          pb=mictimf_px;
          mictimf_px=(mictimf_px+1)&mictimf_mask;
          for(i=1; i<tx_pre_resample; i++)
            {
            t1+=abs(mictimf_shi[mictimf_px]-mictimf_shi[pb]);
            mictimf_px=(mictimf_px+1)&mictimf_mask;
            }
          tx_mic_key[mic_key_pa]=t1/(tx_pre_resample*1000);
          mic_key_pa=(mic_key_pa+1)&mic_key_mask;
          }
        break;

        case 1:
        while(mictimf_pa != mictimf_px)
          {
          t1=0;
          pb=mictimf_px;
          mictimf_px=(mictimf_px+2)&mictimf_mask;
          for(i=1; i<tx_pre_resample; i++)
            {
            t1+=abs(mictimf_shi[mictimf_px]-mictimf_shi[pb]);
            t1+=abs(mictimf_shi[mictimf_px+1]-mictimf_shi[pb+1]);
            pb=mictimf_px;
            mictimf_px=(mictimf_px+2)&mictimf_mask;
            }
          tx_mic_key[mic_key_pa]=t1/(tx_pre_resample*1000);
          mic_key_pa=(mic_key_pa+1)&mic_key_mask;
          }
        break;
        
        case 2:
        while(mictimf_pa != mictimf_px)
          {
          t1=0;
          pb=mictimf_px;
          mictimf_px=(mictimf_px+1)&mictimf_mask;
          for(i=0; i<tx_pre_resample; i++)
            {
            t1+=abs(mictimf_int[mictimf_px]-mictimf_int[pb]);
            pb=mictimf_px;
            mictimf_px=(mictimf_px+1)&mictimf_mask;
            }
          tx_mic_key[mic_key_pa]=(t1/tx_pre_resample)/(0x7fff*1000);
          mic_key_pa=(mic_key_pa+1)&mic_key_mask;
          }
        break;
        
        case 3:
        while(mictimf_pa != mictimf_px)
          {
          t1=0;
          pb=mictimf_px;
          mictimf_px=(mictimf_px+2)&mictimf_mask;
          for(i=0; i<tx_pre_resample; i++)
            {
            t1+=abs(mictimf_int[mictimf_px]-mictimf_int[pb]);
            t1+=abs(mictimf_shi[mictimf_px+1]-mictimf_shi[pb+1]);
            pb=mictimf_px;
            mictimf_px=(mictimf_px+2)&mictimf_mask;
            }
          tx_mic_key[mic_key_pa]=t1/(tx_pre_resample*1000);
          mic_key_pa=(mic_key_pa+1)&mic_key_mask;
          }
        break;
        }
      lir_set_event(EVENT_TX_INPUT);
      memcheck(3,txmem,&txmem_handle);
      }
    break;

    case MODE_RADAR:
    while(mictimf_px != mictimf_pa)
      {
      mictimf_px=(mictimf_px+tx_pre_resample)&mictimf_mask;
      if(radar_cnt < radar_pulse_cnt)
        {
        i=10;
        }
      else
        {
        i=0;
        }
      radar_cnt++;
      if(radar_cnt >= radar_cnt_max)radar_cnt=0;
      tx_mic_key[mic_key_pa]=i;
      mic_key_pa=(mic_key_pa+1)&mic_key_mask;
      lir_set_event(EVENT_TX_INPUT);
      memcheck(4,txmem,&txmem_handle);
      }
    break;  
    }
  }
thread_status_flag[THREAD_TX_INPUT]=THRFLAG_RETURNED;
lir_set_event(EVENT_TX_INPUT);
while(thread_command_flag[THREAD_TX_INPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

void make_tx_modes(void)
{
if(ui.tx_ad_bytes == 2)
  {
  if(ui.tx_ad_channels==1)
    {
    tx_input_mode=0;
    }
  else
    {
    tx_input_mode=1;
    }  
  }
else
  {
  if(ui.tx_ad_channels==1)
    {
    tx_input_mode=2;
    }
  else
    {
    tx_input_mode=3;
    }  
  }
if(ui.tx_da_bytes == 2)
  {
  if(ui.tx_da_channels==1)
    {
    tx_output_mode=0;
    }
  else
    {
    tx_output_mode=1;
    }  
  }
else
  {
  if(ui.tx_da_channels==1)
    {
    tx_output_mode=2;
    }
  else
    {
    tx_output_mode=3;
    }  
  }
}

void make_tx_cw_waveform(void)
{
int i, n1, n2;
double dt1, dt2;
// Use the (complementary) error function erfc(x) to get the optimum 
// shape for the rise time.
// Start at erfc(-3.3) and go to erfc(3.3). In this interval the function
// goes from  1.9999969423 to 0.0000030577 and jumping abruptly to 0 or
// 2.0 respectively corresponds to an abrupt keying of another signal
// that is 117 dB below the main signal.
n1=ui.tx_da_speed*(0.000001*ui.tx_pilot_tone_prestart);
n2=ui.tx_da_speed*(0.000001*txcw.rise_time);
n2&=-2;
tx_cw_waveform_points=n1+n2-1;
if(tx_cw_waveform_points >= max_tx_cw_waveform_points)
  {
  lirerr(1076);
  return;
  }
// Set a very small (non-zero) amplitude for use by the pilot tone
// so it will start the desired time before the actual output
// signal will start to follow the erfc function.
for(i=1; i<=n1; i++) txout_waveform[i]=0.5*PILOT_TONE_CONTROL;
dt1=3.3;
dt2=6.6/(n2-3);
for(i=n1+1; i<=tx_cw_waveform_points; i++)
  {
  txout_waveform[i]=erfc(dt1)/2;
  dt1-=dt2;
  }
txout_waveform[0]=0;
old_cw_rise_time=txcw.rise_time;
radar_cnt_max=0.001*txcw.radar_interval*ui.tx_ad_speed/tx_pre_resample;
radar_pulse_cnt=0.001*txcw.radar_pulse*ui.tx_ad_speed/tx_pre_resample;
radar_cnt=radar_cnt_max-radar_pulse_cnt;
}

int tx_setup(void)
{
int return_flag;
int line;
char s[80];
char* ena[]={"disabled", "enabled"};
return_flag=FALSE;
if(read_txpar_file() == FALSE)set_default_spproc_parms();
if(rx_input_thread == THREAD_RX_ADINPUT)
  {
  snd[RXAD].framesize=2*ui.rx_ad_channels;
  if( (ui.rx_input_mode&DWORD_INPUT) != 0) snd[RXAD].framesize*=2;
  snd[RXAD].block_frames=ui.rx_ad_speed/1000;
  if(snd[RXAD].block_frames < 4)snd[RXAD].block_frames=4;
  make_power_of_two(&snd[RXAD].block_frames);
  snd[RXAD].block_bytes=2*ui.rx_ad_channels*snd[RXAD].block_frames;
  if( (ui.rx_input_mode&DWORD_INPUT) != 0)snd[RXAD].block_bytes*=2;
  }
genparm[DA_OUTPUT_SPEED]=ui.rx_min_da_speed;
if(genparm[DA_OUTPUT_SPEED] < 8000)genparm[DA_OUTPUT_SPEED]=8000;
rx_daout_bytes=ui.rx_max_da_bytes;
rx_daout_channels=ui.rx_max_da_channels;
snd[RXDA].framesize=rx_daout_bytes*rx_daout_channels;
snd[TXDA].framesize=ui.tx_da_bytes*ui.tx_da_channels;
snd[TXDA].block_bytes=(snd[TXDA].framesize*ui.tx_da_speed)/1000;
// Define wide graph borders to get error messages on screen
wg.xright=screen_width-30;
wg.ybottom=screen_height-30;
// *******************************************************
// ***   The A/D and A/D setup is already done.
// ***   Ask the user what he wants.
// ***   The transmit functions of Linrad are in a very early stage
// ***   and there is not much to choose from yet............
// ******************************************************
repeat_menu:;
if(kill_all_flag) goto txend;
clear_screen();
sprintf(s,"Tx %s",ena[ui.tx_enable]);
lir_text(2,2,s);
lir_text(5,6,"B=Hardware setup for Tx.");
lir_text(5,7,"C=Speech processor setup.");
lir_text(5,8,"D=CW setup.");
lir_text(5,9,"E=Enable/disable Tx.");
lir_text(5,10,"F=Send disk file to Tx output");
lir_text(5,11,"X=Return to main menu");
await_processed_keyboard();
if(kill_all_flag) goto txend;
switch (lir_inkey)
  {
  case 'B':
do_tx_setup:;
  set_tx_io();
  return_flag=check_tx_devices();
  goto txend;
  
  case 'C':
  if(check_tx_devices() == FALSE)goto do_tx_setup;
  if(ui.tx_enable != 0)
    {  
    line=15;
    if(rx_input_thread == THREAD_RX_ADINPUT)
      {
      start_iotest(&line, RXAD);
      if(kill_all_flag)return FALSE;
      }
    start_iotest(&line, RXDA);
    if(kill_all_flag)return FALSE;
    }
  spproc_setup();
  if(ui.tx_enable != 0)
    {  
    stop_iotest(RXDA);
    if(rx_input_thread == THREAD_RX_ADINPUT)
      {
      stop_iotest(RXAD);
      }
    }
  break;

  case 'D':
  if(check_tx_devices() == FALSE)goto do_tx_setup;
  if(rx_input_thread == THREAD_RX_ADINPUT)
    {
    line=15;
    start_iotest(&line, RXAD);
    if(kill_all_flag)return FALSE;
    }
  start_iotest(&line, RXDA);
  if(kill_all_flag)return FALSE;
  cwproc_setup();
  stop_iotest(RXDA);
  if(rx_input_thread == THREAD_RX_ADINPUT)
    {
    stop_iotest(RXAD);
    }
  break;

  case 'E':
  get_tx_enable();
  break;

  case 'F':
  if(check_tx_devices() == FALSE)goto do_tx_setup;
  disk2tx();
  break;

  case 'X':
txend:;
  lir_inkey=0;
  return return_flag;
  }
goto repeat_menu;
}

