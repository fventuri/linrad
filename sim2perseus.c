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
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include "globdef.h"
#include "uidef.h"



#if(OSNUM == OSNUM_WINDOWS)
#include <windows.h>
#endif


#if(OSNUM == OSNUM_LINUX)
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

typedef struct rcvr_hdr{
  char           chunkID[4];         // ="rcvr" (chunk perseus beta0.2)
  int32_t        chunkSize;          // chunk length
  int32_t        nCenterFrequencyHz; // center frequency
  int32_t        SamplingRateIdx;    // 0=125K, 1=250K, 2=500K, 3=1M, 4=2M
  time_t         timeStart;          // start time of the recording (time(&timeStart))
  unsigned short wAttenId;           // 0=0dB, 1=10dB, 2=20dB, 3=30dB
  char           bAdcPresel;         // 0=Presel Off, 1=Presel On
  char           bAdcPreamp;         // 0=ADC Preamp Off, 1=ADC Preamp ON
  char           bAdcDither;         // 0=ADC Dither Off, 1=ADC Dither ON
  char           bSpare;             // for future use (default = 0)
  char           rsrvd[16];          // for future use (default = 000..0)
  char           extra[16];          // QS1R and others may use longer chunks
  }RCVR;

#define PERSEUS_HEADER_SIZE 42

RCVR perseus_hdr;

int main(int argc, char *argv[])
{
int rx_ad_speed;
short int rx_ad_channels, block_align;
int i, k, m, errnr;
unsigned int file_bytes, data_bytes, bytes_per_second;
unsigned int chunk_size;
short int format_tag;
char s[200];
char *ich;
FILE *save_rd_file, *save_wr_file;
memset((char*)&perseus_hdr,0,sizeof(perseus_hdr));
perseus_hdr.chunkID[0]='r';
perseus_hdr.chunkID[1]='c';
perseus_hdr.chunkID[2]='v';
perseus_hdr.chunkID[3]='r';
perseus_hdr.chunkSize=PERSEUS_HEADER_SIZE-8;
if(argc != 2)
  {
  printf("Give a single filename as argument to this program. (ex sim.wav)");
  exit(0);
  }
i=0;
while(i<180 && argv[1][i]!=0)i++;
argv[1][i]=0;  
sprintf(s,"perseus-%s",argv[1]);
printf("\n Convert %s to Perseus format (%s)\n\n",argv[1],s);
save_rd_file = fopen(argv[1], "rb");
if (save_rd_file == NULL)
  {
  printf("Could not open %s",argv[1]);
  exit(1);
  }
save_wr_file = fopen(s, "wb");
if (save_wr_file == NULL)
  {
  printf("Could not open %s\n",s);
  exit(1);
  }
// Read the WAV file header.
// First 4 bytes should be "RIFF"
k=fread(&i,sizeof(int),1,save_rd_file);
errnr=0;
if(k !=1)
  {
headerr:;  
  printf("Error in .wav file header [%d]",errnr);  
  exit(1);
  }
if(i != 0x46464952)
  {
  errnr=1;
  goto headerr;
  }
k=fwrite(&i,sizeof(int),1,save_wr_file);
if(k != 1)goto wrerr;
// Read file size (we do not need it)
k=fread(&file_bytes,sizeof(int),1,save_rd_file);
if(k!=1)
  {
  errnr=2;
  goto headerr;
  }
k=fwrite(&file_bytes,sizeof(int),1,save_wr_file);
if(k != 1)goto wrerr;
// Now we should read "WAVE"
k=fread(&i,sizeof(int),1,save_rd_file);
errnr=2;
if(k !=1 || i != 0x45564157)goto headerr;
k=fwrite(&i,sizeof(int),1,save_wr_file);
if(k != 1)goto wrerr;
// Now we should read "fmt "
k=fread(&i,sizeof(int),1,save_rd_file);
errnr=3;
if(k !=1 || i != 0x20746d66)goto headerr;
k=fwrite(&i,sizeof(int),1,save_wr_file);
if(k != 1)goto wrerr;
// read the size of the format chunk.
k=fread(&chunk_size,sizeof(int),1,save_rd_file);
errnr=4;
if(k !=1 )goto headerr; 
k=fwrite(&chunk_size,sizeof(int),1,save_wr_file);
if(k != 1)goto wrerr;
// read the type of data (Format Tag). We only recognize PCM data!
k=fread(&format_tag,sizeof(short int),1,save_rd_file);
errnr=5;
if(k !=1 || (format_tag != 1 && format_tag != 3))
  {
  printf("Unknown wFormatTag.");
  goto headerr;
  }
k=fwrite(&format_tag,sizeof(short int),1,save_wr_file);
if(k != 1)goto wrerr;  
// Read no of channels
k=fread(&rx_ad_channels,sizeof(short int),1,save_rd_file);
errnr=6;
if(k !=1 || rx_ad_channels < 1 || rx_ad_channels > 2)goto headerr;  
k=fwrite(&rx_ad_channels,sizeof(short int),1,save_wr_file);
if(k != 1)goto wrerr; 
// Read the sampling speed.
k=fread(&rx_ad_speed,sizeof(int),1,save_rd_file);
errnr=7;
if(k !=1 )goto headerr; 
k=fwrite(&rx_ad_speed,sizeof(int),1,save_wr_file);
if(k != 1)goto wrerr;
// Read average bytes per second (do not care what it is)
errnr=8;
k=fread(&bytes_per_second,sizeof(int),1,save_rd_file);
if(k !=1 )goto headerr; 
k=fwrite(&bytes_per_second,sizeof(int),1,save_wr_file);
if(k != 1)goto wrerr;
// Read block align to get 8 or 16 bit format .
k=fread(&block_align,sizeof(short int),1,save_rd_file);
errnr=9;
if(k !=1 )goto headerr; 
  switch (block_align/rx_ad_channels)
  {
  case 1:
  errnr=10;
  if(format_tag != 1)goto headerr;
  break;
  
  case 2:
  errnr=11;
  if(format_tag != 1)goto headerr;
  break;
  
  case 3:
  errnr=12;
  if(format_tag != 1)goto headerr;
  break;
  
  case 4:
  errnr=13;
  if(format_tag != 3)goto headerr;
  break;
  
  default:
  goto headerr;
  }
k=fwrite(&block_align,sizeof(short int),1,save_wr_file);
if(k != 1)goto wrerr;  
// Skip extra bytes if present.
chunk_size-=14;
errnr=11;
while(chunk_size != 0)
  {
  k=fread(&i,1,1,save_rd_file);
  if(k !=1 )goto headerr; 
  k=fwrite(&i,1,1,save_wr_file);
  if(k != 1)goto wrerr;  
  chunk_size--;
  }
// Read chunks until we encounter the "data" string (=0x61746164).
// Look for Perseus or SDR-14 headers.
errnr=12;
next_chunk:;
k=fread(&i,sizeof(int),1,save_rd_file);
if(k !=1 )goto headerr; 
errnr=13;
// test for "rcvr"
if(i==0x72766372)
  {
  printf("This file is already in Perseus format.");
  goto headerr;
  }
errnr=25;
// Look for DATA 
if(i != 0x61746164)
  {
// Unknown. Get the size and skip.  
  k=fread(&chunk_size,sizeof(int),1,save_rd_file);
  if(k !=1 )goto headerr; 
  while(chunk_size != 0)
    {
    k=fread(&i,1,1,save_rd_file);
    if(k !=1 )goto headerr; 
    chunk_size--;
    }
  goto next_chunk;
  }
// Read how much data we have ( do not care)
k=fread(&data_bytes,sizeof(int),1,save_rd_file);
if(k !=1 )goto headerr; 
switch(rx_ad_speed)
  {
  case 125000:
  perseus_hdr.SamplingRateIdx=0;
  break;
  
  case 250000:
  perseus_hdr.SamplingRateIdx=1;
  break;
  
  case 500000:
  perseus_hdr.SamplingRateIdx=2;
  break;
  
  case 1000000:
  perseus_hdr.SamplingRateIdx=3;
  break;
  
  case 2000000:
  perseus_hdr.SamplingRateIdx=4;
  break;
  
  default:
  printf("Unsupported sampling rate for Perseus %d Hz.",rx_ad_speed);
  goto headerr;
  }
printf("Enter the center frequency in Hz ");
scanf("%4d",&perseus_hdr.nCenterFrequencyHz);
k=fwrite(&perseus_hdr.chunkID[0],PERSEUS_HEADER_SIZE,1,save_wr_file);
if(k != 1)goto wrerr;
k=fwrite(&i,sizeof(int),1,save_wr_file);
if(k != 1)goto wrerr;
k=fwrite(&data_bytes,sizeof(int),1,save_wr_file);
if(k != 1)goto wrerr;
// (Unorthodox. Slow but easy.) read byte by byte until end of file.
ich=(char*)&i;
k=fread(&ich[1],3,1,save_rd_file);
while(k == 1)
  {
  m=fwrite(&ich[1],3,1,save_wr_file);
  if(m != 1)goto wrerr;  
  k=fread(&ich[1],3,1,save_rd_file);
  }
return 0;
wrerr:;
printf("\nERROR: Failed to write output file.\n");
return 1;
}  
