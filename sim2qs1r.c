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
#if(OSNUM == OSNUM_WINDOWS)
#include <windows.h>
#endif
#include "globdef.h"
#include "uidef.h"



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
  uint32_t           chunkSize;          // chunk length
  unsigned char  data[64];
  }RCVR;

// "auxi" chunk as used in the SpectraVue WAV files
typedef struct auxi_hdr{
  char       chunkID[4];  // ="auxi" (chunk rfspace)
  uint32_t       chunkSize;   // chunk length
  SYSTEMTIME StartTime;
  SYSTEMTIME StopTime;
  DWORD      CenterFreq;  //receiver center frequency
  DWORD      ADFrequency; //A/D sample frequency before downsampling
  DWORD      IFFrequency; //IF freq if an external down converter is used
  DWORD      Bandwidth;   //displayable BW if you want to limit the display to less than Nyquist band
  DWORD      IQOffset;    //DC offset of the I and Q channels in 1/1000's of a count
  DWORD      Unused2;
  DWORD      Unused3;
  DWORD      Unused4;
  DWORD      Unused5;
  }AUXI;
RCVR perseus_hdr;
AUXI sdr14_hdr;

int main(int argc, char *argv[])
{
int rx_ad_speed;
short int rx_ad_channels, block_align;
int i, k, m, errnr;
int *qsrate;
unsigned int chunk_size;
short int format_tag;
char s[200];
FILE *save_rd_file, *save_wr_file;
unsigned char qsdata[38];
if(argc != 2)
  {
  printf("Give a single filename as argument to this program. (ex sim.wav)");
  exit(0);
  }
i=0;
while(i<180 && argv[1][i]!=0)i++;
argv[1][i]=0;  
sprintf(s,"qs1r-%s",argv[1]);
printf("\n Convert %s to QS1R format (%s)\n\n",argv[1],s);
qsdata[0]=60;
qsdata[1]=169;
qsdata[2]=185;
qsdata[3]=1;
qsdata[4]=32;
qsrate=(int*)(&qsdata[4]);
qsdata[5]=161;
qsdata[6]=7;
qsdata[7]=0;
qsdata[8]=248;
qsdata[9]=249;
qsdata[10]=29;
qsdata[11]=77;
qsdata[12]=0;
qsdata[13]=0;
qsdata[14]=0;
qsdata[15]=0;
qsdata[16]=0;
qsdata[17]=0;
qsdata[18]=0;
qsdata[19]=1;
qsdata[20]=0;
qsdata[21]=0;
qsdata[22]=113;
qsdata[23]=115;
qsdata[24]=49;
qsdata[25]=114;
qsdata[26]=32;
qsdata[27]=114;
qsdata[28]=101;
qsdata[29]=118;
qsdata[30]=100;
qsdata[31]=32;
qsdata[32]=32;
qsdata[33]=32;
qsdata[34]=32;
qsdata[35]=32;
qsdata[36]=32;
qsdata[37]=32;


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
k=fread(&i,sizeof(int),1,save_rd_file);
if(k!=1)
  {
  errnr=2;
  goto headerr;
  }
k=fwrite(&i,sizeof(int),1,save_wr_file);
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
qsrate[0]=rx_ad_speed;
if(k != 1)goto wrerr;
// Read average bytes per second (do not care what it is)
errnr=8;
k=fread(&i,sizeof(int),1,save_rd_file);
if(k !=1 )goto headerr; 
k=fwrite(&i,sizeof(int),1,save_wr_file);
if(k != 1)goto wrerr;
// Read block align to get 8 or 16 bit format.
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
skip_chunk:;
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
k=fwrite(&i,sizeof(int),1,save_wr_file);
if(k != 1)goto wrerr;
errnr=13;
// test for "rcvr"
if(i==0x72766372)
  {
  k=fread(&chunk_size,sizeof(int),1,save_rd_file);
  if(k !=1 )goto headerr; 
  k=fread(&perseus_hdr.data[0],1,chunk_size,save_rd_file);
  if(k != (int)chunk_size)goto headerr;
// Replace the perseus data with data from a qs1r recording
  chunk_size=38;
  k=fwrite(&chunk_size,sizeof(int),1,save_wr_file);
  if(k != 1)goto wrerr;
  k=fwrite(qsdata,1,chunk_size,save_wr_file);
  if(k != (int)chunk_size)goto headerr;
  goto next_chunk;
  }
/*
errnr=14;  
// test for "auxi"
if(i==0x69787561 && chunk_size == sizeof(sdr14_hdr))
  {
  k=fread(&chunk_size,sizeof(int),1,save_rd_file);
  if(k !=1 )goto headerr; 
  k=fread(&sdr14_hdr.StartTime,1,chunk_size,save_rd_file);
  if(k != chunk_size)goto headerr;
  goto next_chunk;
  }
*/
errnr=25;
// Look for DATA 
if(i != 0x61746164)
  {
// Unknown. Get the size and skip.  
  k=fread(&chunk_size,sizeof(int),1,save_rd_file);
  if(k !=1 )goto headerr; 
  k=fwrite(&chunk_size,sizeof(int),1,save_wr_file);
  if(k != 1)goto wrerr;
  goto skip_chunk;
  }
// Read how much data we have ( do not care)
k=fread(&i,sizeof(int),1,save_rd_file);
if(k !=1 )goto headerr; 
k=fwrite(&i,sizeof(int),1,save_wr_file);
if(k != 1)goto wrerr;
// (Unorthodox. Slow but easy.) read byte by byte until end of file.
k=fread(&i,1,1,save_rd_file);
while(k == 1)
  {
  m=fwrite(&i,1,1,save_wr_file);
  if(m != 1)goto wrerr;  
  k=fread(&i,1,1,save_rd_file);
  }
return 0;
wrerr:;
printf("\nERROR: Failed to write output file.\n");
return 1;
}  
