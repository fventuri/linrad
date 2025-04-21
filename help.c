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
#else
  #include <unistd.h>
#endif
#include "uidef.h"
#include "fft1def.h"
#include "screendef.h"
#include "seldef.h"
#include "sigdef.h"
#include "thrdef.h"
#include "vernr.h"
#include "keyboard_def.h"
#include "options.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
extern HANDLE savefile_handle;
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

unsigned char show_scro_color;
FILE *msg_file;
extern fpos_t file_startpos;
extern fpos_t file_playpos;




// Define fseeko and ftello. Some old distributions, e.g. RedHat 9
// have the functions but not in the headers.
#ifndef fseeko
int fseeko(FILE *stream, off_t offset, int whence);
#endif
#ifndef ftello
off_t ftello(FILE *stream);
#endif       
       

void show_scro(int ix1,int iy1,int ix2,int iy2)
{
unsigned char cl;
if(ix1 < 0 || ix1 >= screen_width)return; 
if(ix2 < 0 || ix2 >= screen_width)return; 
if(show_scro_color==0)cl=12; else cl=10;
lir_hline(ix1,iy1,ix2,cl);
if(show_scro_color==2)cl=12; else cl=10;
lir_hline(ix1,iy2,ix2,cl);
if(show_scro_color==1)cl=12; else cl=10;
lir_line(ix1,iy1,ix1,iy2,cl);
if(show_scro_color==3)cl=12; else cl=10;
lir_line(ix2,iy1,ix2,iy2,cl);
}

char *screensave;

void init_screensave(void)
{
if(screensave_flag)return;
screensave=malloc(screen_width*screen_height);
if( screensave == NULL)
  {
  lirerr(1035);
  return;
  }
screensave_flag=TRUE;
pause_thread(THREAD_SCREEN);
lir_getbox(0,0,screen_width,screen_height,(size_t*)screensave);
lir_sched_yield();
}

void close_screensave(void)
{
lir_putbox(0,0,screen_width,screen_height,(size_t*)screensave);
free(screensave);
screensave_flag=FALSE;
lir_sched_yield();
resume_thread(THREAD_SCREEN);
}

void edit_diskread_times(void)
{
char s[80];
int i, j, k, blk, line;
double filetime1, filetime2, blk_factor;
uint64_t fptr_pa;
uint64_t fptr_px;
uint64_t fptr_pb;
#if OSNUM == OSNUM_WINDOWS
BY_HANDLE_FILE_INFORMATION FileInformation;
#endif
double dt1;
int diskread_timofday1;
int diskread_timofday2;
fptr_pa=0;
fptr_px=0;
diskread_pause_flag=1;
init_screensave();
if(kill_all_flag)return;
lir_sleep(50000);
while(thread_status_flag[THREAD_SCREEN] != THRFLAG_IDLE)
  {
  lir_set_event(EVENT_SCREEN);
  lir_sleep(20000);
  }
clear_screen();
for(i=0;i<wg_waterf_size;i++)wg_waterf[i]=(short int)0x8000;
settextcolor(15);
line=3;
lir_text(10,line,"EDIT times for playback of recorded file. (X to return)");
line +=2;
settextcolor(14);
lir_text(20,line,savefile_name);
line+=3;
settextcolor(7);
filetime1=diskread_time;
i=rint(filetime1);
j=i/(24*3600);
sprintf(s,"Day since Epoch=%d",j);
lir_text(0,line,s);
line++;
i%=24*3600;
diskread_timofday1=i;
j=i/3600; 
i%=3600;
sprintf(s,"File starts at: %02d.%02d.%02d",j,i/60,i%60);
lir_text(0,line,s);
line++;
#if OSNUM == OSNUM_LINUX
fseek(save_rd_file, 0, SEEK_END);
fptr_pa=ftello(save_rd_file);
if(fsetpos(save_rd_file,&file_startpos) != 0)lirerr(296143);
fptr_px=ftello(save_rd_file);
#endif
#if OSNUM == OSNUM_WINDOWS
i= GetFileInformationByHandle(savefile_handle, &FileInformation);
if(i==0)lirerr(452975);
fptr_pa = (uint64_t)FileInformation.nFileSizeLow | ((uint64_t)FileInformation.nFileSizeHigh) <<32;
if(fsetpos(save_rd_file,&file_startpos) != 0)lirerr(296143);
fptr_px=ftell(save_rd_file);
#endif
if( (ui.rx_input_mode&DWORD_INPUT) == 0)
  {
  if( (ui.rx_input_mode&BYTE_INPUT) != 0)
    {
    blk=snd[RXAD].block_bytes>>1;
    }
  else
    { 
    blk=snd[RXAD].block_bytes;
    }
  }
else
  {  
  if( (ui.rx_input_mode&(BYTE_INPUT+FLOAT_INPUT)) != 0)
    {
    if( (ui.rx_input_mode&FLOAT_INPUT) == 0)
      {
      blk=3*(snd[RXAD].block_bytes>>2);
      }
    else
      {
      blk=snd[RXAD].block_bytes;
      }   
    }
  else
    {
    blk=save_rw_bytes;
    }
  }
filetime2=fptr_pa-fptr_px;
blk_factor=snd[RXAD].block_bytes;
blk_factor/=blk;
filetime2*=blk_factor;
filetime2=diskread_timofday1+filetime2/(snd[RXAD].framesize*ui.rx_ad_speed);
i=rint(filetime2);
//i%=24*3600;
diskread_timofday2=i;
if(diskread_timofday2 < diskread_timofday1)diskread_timofday2+=24*3600;
j=i/3600;
i%=3600;
sprintf(s,"ends at: %02d.%02d.%02d",j,i/60,i%60);
lir_text(7,line,s);
line+=2;
get_start:;
clear_lines(line,line+3);
lir_text(0,line,"Start playback at (hh.mm.ss):");
i=lir_get_filename(31,line,s);
if(kill_all_flag)goto end_edit;
if(i == 0)
  {
  dt1=0;
  clear_lines(line,line);
  lir_text(0,line,"Starting from beginning of file"); 
  goto start_0;
  }
for(j=0; j<i; j++)
  {
  if(s[j]!='.' && (s[j] < '0' || s[j] > '9'))
    {
errstart:;
    settextcolor(12);
    lir_text(10,line+2, "ERROR");
    lir_text(10,line+3,press_any_key);
    settextcolor(7);
    to_upper_await_keyboard();
    if(kill_all_flag)return;
    if(lir_inkey == 'X')goto end_edit;
    goto get_start;
    }
  }
sscanf(s,"%02d.%02d.%02d",&i,&j,&k);
if( i < 0 ||
    j < 0 ||
    j > 60 ||
    k < 0 ||
    k > 60)goto errstart;
dt1=(i*60+j)*60+k;
if(dt1 < diskread_timofday1)dt1+=24*3600;
if(dt1 >= diskread_timofday2)goto errstart;
dt1-=diskread_timofday1;
if(dt1 < 0)lirerr(32919);
dt1*=snd[RXAD].framesize*ui.rx_ad_speed/blk_factor;
start_0:;
// The data in the file starts at fptr_px and the last
// position in the file is fptr_pa. The corresponding times
// are diskread_timofday1 and diskread_timofday2
// Set fptr_pb to point at the selected start time.  
fptr_pb=rint(dt1/blk);
diskread_block_counter=fptr_pb;
file_start_block=diskread_block_counter;
fptr_pb*=blk;
fptr_pb+=fptr_px;
#if OSNUM == OSNUM_LINUX
fseeko(save_rd_file, fptr_pb, SEEK_SET);
#endif
#if OSNUM == OSNUM_WINDOWS
_lseeki64(_fileno(save_rd_file), fptr_pb, SEEK_SET);
#endif
fgetpos(save_rd_file,&file_playpos);
lir_sched_yield();
line+=2;
get_stop:;
clear_lines(line,line+3);
lir_text(0,line,"Stop playback at (hh.mm.ss):");
i=lir_get_filename(30,line,s);
if(kill_all_flag)goto end_edit;
if(i == 0)
  {
last_block:;  
  file_stop_block=-1;
  goto end_edit;
  }
for(j=0; j<i; j++)
  {
  if(s[j]!='.' && (s[j] < '0' || s[j] > '9'))
    {
errstop:;
    settextcolor(12);
    lir_text(10,line+2, "ERROR");
    lir_text(10,line+3,press_any_key);
    settextcolor(7);
    to_upper_await_keyboard();
    if(kill_all_flag)return;
    if(lir_inkey == 'X')goto end_edit;
    goto get_stop;
    }
  }
sscanf(s,"%02d.%02d.%02d",&i,&j,&k);
if( i < 0 ||
    j < 0 ||
    j > 60 ||
    k < 0 ||
    k > 60)goto errstop;
dt1=(i*60+j)*60+k;
if(dt1 < diskread_timofday1)dt1+=24*3600;
if(dt1 >= diskread_timofday2)goto last_block;
dt1-=diskread_timofday1;
if(dt1 < 0)lirerr(32929);
dt1*=snd[RXAD].framesize*ui.rx_ad_speed;
file_stop_block=rint(dt1/snd[RXAD].block_bytes);
i=diskread_time+file_stop_block*snd[RXAD].block_frames/ui.rx_ad_speed;
i%=24*3600;
j=i/3600;
i%=3600;
sprintf(s,"%02d.%02d.%02d",j,i/60,i%60);
end_edit:;
close_screensave();
lir_refresh_screen();
diskread_pause_flag=0;
lir_sched_yield();
}

void newcomer_escpress(int clear)
{
if(clear == 0)
  {
  if(screensave_flag)
    {
    lir_sleep(10000);
    if(keyboard_buffer_ptr == keyboard_buffer_used)
      {
      keyboard_buffer[keyboard_buffer_ptr]=0;
      keyboard_buffer_ptr=(keyboard_buffer_ptr+1)&(KEYBOARD_BUFFER_SIZE-1);
      }
    lir_sleep(10000);  
    lir_set_event(EVENT_KEYBOARD);
    while(thread_status_flag[THREAD_SCREEN] == THRFLAG_IDLE)
      {
      lir_refresh_screen();
      lir_sleep(3000);
      if(kill_all_flag) return;
      }
    }
  init_screensave();
  if(kill_all_flag)return;
  clear_screen();
  settextcolor(15);
  lir_text(20,10,"ESC key pressed");
  settextcolor(7);
  lir_text(5,12,"Do you really want to exit from Linrad now ?  (Y/N)=>");
  }
else 
  {
  close_screensave();
  }
lir_refresh_screen();
}

void help_screen_objects(void)
{
int i, j;
#if(OSNUM == OSNUM_LINUX)
int line;
char s[80];
#endif
// Make sure we are not in a mouse function.
mouse_time_wide=current_time();
mouse_time_narrow=mouse_time_wide;
if(mouse_task != -1)return;
lir_sched_yield();
if(mouse_task != -1)return;
mouse_time_wide=current_time();
mouse_time_narrow=mouse_time_wide;
lir_sched_yield();
if(mouse_task != -1)return;
mouse_inhibit=TRUE; 
init_screensave();
if(kill_all_flag)return;
for(i=0; i<no_of_scro; i++)
  {
  if( scro[i].x1 <= mouse_x && 
      scro[i].x2 >= mouse_x &&      
      scro[i].y1 <= mouse_y && 
      scro[i].y2 >= mouse_y) 
    {
    switch (scro[i].no)
      {
      case WIDE_GRAPH:
      help_on_wide_graph();
      break;

      case HIRES_GRAPH:
      help_on_hires_graph();
      break;

      case AFC_GRAPH:
      help_on_afc_graph();
      break;

      case BASEBAND_GRAPH:
      help_on_baseband_graph();
      break;

      case POL_GRAPH:
      help_on_pol_graph();
      break;

      case COH_GRAPH:
      help_on_coherent_graph();
      break;

      case EME_GRAPH:
      help_on_eme_graph();
      break;

      case FREQ_GRAPH:
      help_on_freq_graph();
      break;

      case METER_GRAPH:
      help_on_meter_graph();
      break;

      case TRANSMIT_GRAPH:
      help_on_tx_graph();
      break;

      case RADAR_GRAPH:
      help_on_radar_graph();
      break;
      
      case SIGANAL_GRAPH:
      help_on_siganal_graph();
      break;

      case ALLAN_GRAPH:
      help_on_allan_graph();
      break;

      case PHASING_GRAPH:
      help_on_phasing_graph();
      break;
      }
    }
  }
if(kill_all_flag) return;  
if(lir_inkey != 0)
  {
  show_scro_color=0;
repeat:;
  for(i=0; i<no_of_scro; i++)
    {
    show_scro(scro[i].x1,scro[i].y1,scro[i].x2,scro[i].y2);
    switch (scro[i].no)
      {
      case WIDE_GRAPH:
      for(j=4; j<MAX_WGBUTT; j++)
        {
        show_scro(wgbutt[j].x1,wgbutt[j].y1,wgbutt[j].x2,wgbutt[j].y2);
        if(kill_all_flag) return;
        }
      break;

      case HIRES_GRAPH:
      for(j=4; j<MAX_HGBUTT; j++)
        {
        if(hgbutt[j].x1 > 0)
          { 
          show_scro(hgbutt[j].x1,hgbutt[j].y1,hgbutt[j].x2,hgbutt[j].y2);
          if(kill_all_flag) return;
          }
        }
// The control bars are separate from the buttons.
      show_scro(timf2_hg_xmin-2,timf2_hg_y[0],
                                 timf2_hg_xmax+1,timf2_hg_y[0]+text_height-1);
      if(kill_all_flag) return;
      show_scro(hg_ston1_x2+2,hg_ston_y0,hg_first_xpixel,hg_stonbars_ytop);
      if(kill_all_flag) return;
      show_scro(hg.xleft+1,hg_ston_y0, hg_ston1_x2,hg_stonbars_ytop);
      break;

      case AFC_GRAPH:
      for(j=4; j<MAX_AGBUTT; j++)
        {
        show_scro(agbutt[j].x1,agbutt[j].y1,agbutt[j].x2,agbutt[j].y2);
        if(kill_all_flag) return;
        } 
// The control bars are separate from the buttons.
      show_scro(ag_ston_x1,ag_fpar_y0,ag_ston_x2,ag_fpar_ytop);
      if(kill_all_flag) return;
      show_scro(ag_lock_x1,ag_fpar_y0,ag_lock_x2,ag_fpar_ytop);
      if(kill_all_flag) return;
      show_scro(ag_srch_x1,ag_fpar_y0,ag_srch_x2,ag_fpar_ytop);
      if(kill_all_flag) return;
      break;

      case BASEBAND_GRAPH:
      for(j=4; j<MAX_BGBUTT; j++)
        {
        show_scro(bgbutt[j].x1,bgbutt[j].y1,bgbutt[j].x2,bgbutt[j].y2);
        if(kill_all_flag) return;
        }
// The volume control bar:
      show_scro(bg_vol_x1,bg_y0,bg_vol_x2,bg_ymax);
      if(kill_all_flag) return;
      break;

      case POL_GRAPH:
      for(j=4; j<MAX_PGBUTT; j++)
        {
        show_scro(pgbutt[j].x1,pgbutt[j].y1,pgbutt[j].x2,pgbutt[j].y2);
        if(kill_all_flag) return;
        } 
      break;

      case COH_GRAPH:
      for(j=4; j<MAX_CGBUTT; j++)
        {
        show_scro(cgbutt[j].x1,cgbutt[j].y1,cgbutt[j].x2,cgbutt[j].y2);
        if(kill_all_flag) return;
        } 
      break;

      case EME_GRAPH:
      if(eme_active_flag != 0)
        {
        for(j=4; j<MAX_EGBUTT; j++)
          {
          show_scro(egbutt[j].x1,egbutt[j].y1,egbutt[j].x2,egbutt[j].y2);
          if(kill_all_flag) return;
          }
        }
      else
        {
        show_scro(egbutt[EG_MINIMISE].x1,egbutt[EG_MINIMISE].y1,
                  egbutt[EG_MINIMISE].x2,egbutt[EG_MINIMISE].y2);
        if(kill_all_flag) return;
        }           
      break;

      case FREQ_GRAPH:
      for(j=4; j<MAX_FGBUTT; j++)
        {
        show_scro(fgbutt[j].x1,fgbutt[j].y1,fgbutt[j].x2,fgbutt[j].y2);
        if(kill_all_flag) return;
        } 
      break;

      case METER_GRAPH:
      for(j=4; j<MAX_MGBUTT; j++)
        {
        show_scro(mgbutt[j].x1,mgbutt[j].y1,mgbutt[j].x2,mgbutt[j].y2);
        if(kill_all_flag) return;
        } 
      break;

      case TRANSMIT_GRAPH:
      for(j=4; j<MAX_TGBUTT; j++)
        {
        show_scro(tgbutt[j].x1,tgbutt[j].y1,tgbutt[j].x2,tgbutt[j].y2);
        if(kill_all_flag) return;
        } 
      break;
      }
    }
  show_scro_color++;
  if(show_scro_color > 3)show_scro_color=0;
  lir_refresh_screen();
  lir_sleep(400000);
  test_keyboard(); 
  if(kill_all_flag) goto help_scox; 
  process_current_lir_inkey();
  if(kill_all_flag) goto help_scox; 
  if(lir_inkey == 0)goto repeat;
  }
#if(OSNUM == OSNUM_LINUX)
if(lir_inkey == F1_KEY || lir_inkey == '!')
  {
  clear_screen();
  lir_text(15,0,"Threads within Linrad");
  lir_text(0,2,"PID");
  lir_text(10,2,"Name");
  lir_text(20,2,"No");
  line=3;
  for(i=0; i<THREAD_MAX; i++)
    {
    if(thread_command_flag[i] > THRFLAG_NOT_ACTIVE  && 
       thread_command_flag[i] < THRFLAG_RETURNED)
      {
      sprintf(s,"%d  ",thread_pid[i]);
      lir_text(0,line,s);
      lir_text(10,line,thread_names[i]);
      sprintf(s,"%2d",i);
      lir_text(20,line,s);
      line++;
      }
    }
  lir_refresh_screen();
  test_keyboard();
  process_current_lir_inkey();
  while(!kill_all_flag && lir_inkey == 0)
    {
    test_keyboard();    
    lir_sleep(100000);
    process_current_lir_inkey();
    }
  }
#endif
close_screensave();
lir_refresh_screen();
help_scox:;
resume_thread(THREAD_SCREEN);
mouse_inhibit=FALSE;
}

void write_from_msg_file(int *line, int msg_no, 
                                     int screen_mode, int vernr)
{
char s[512];
char chr;
int i,j,k;
msg_file=fopen(msg_filename, "r");
if(msg_file == NULL)
  {
  sprintf(s,"Could not find %s",msg_filename);
  goto finish_err;
  }
i=fread(&s,1,6,msg_file);
if(i!=6)
  {
  sprintf(s,"File %s empty",msg_filename);
  goto finish_err_close;
  }
s[6]=0;   
sscanf(s,"%d",&k);
if(k != vernr)
  {
  sprintf(s,"Error %d. Wrong version number in %s",msg_no, msg_filename);
  goto finish_err_close;
  }
gtnum:;
i=fread(&s,1,1,msg_file);
if(i==0)
  {
  sprintf(s,"Could not find info no %d",msg_no);
  goto finish_err_close;
  }
if(s[0] != '[')goto gtnum;
j=0;
while(fread(&s[j],1,1,msg_file)==1 && s[j] != ']')
  {
  j++;
  if(j > 79)
    {
    lirerr(966572);
    goto finish_err_close;
    }
  }  
s[j]=0;
sscanf(s,"%d",&k);
if(k != msg_no)goto gtnum;
sprintf(s,"[%d]",msg_no);
j=0;
while(s[j]!=0)j++;
j--;
gtrow:; 
j++;   
i=fread(&s[j],1,1,msg_file);
if( i==1 && s[j] != 10 && s[j] != '[' && j<255 )goto gtrow;
chr=s[j];
if(chr== '[' || chr==10)
  {
  s[j]=0;
  }
else
  {  
  s[j+1]=0;
  }
if(screen_mode!=0)
  {
  lir_text(0,line[0],s);
  }
else
  {
  printf("\n%s",s);
  DEB"\n%s",s);
  }  
line[0]++;
if(chr != '[' && i==1)
  {
  j=-1;
  goto gtrow;
  }   
goto finish_ok;  
finish_err_close:;       
fclose(msg_file);
finish_err:;       
if(screen_mode!=0)
  {
  lir_text(0,line[0],s);
  }
else
  {
  printf("\n%s",s);
  DEB"\n%s",s);
  }  
line[0]++;
sprintf(s,"Error[%d] No info available in %s",msg_no,msg_filename);
if(screen_mode!=0)
  {
  lir_text(0,line[0],s);
  }
else
  {
  printf("\n%s\n",s);
  DEB"\n%s",s);
  fflush(stdout);
  lir_sync();
  }  
line[0]++;
finish_ok:;
if(screen_mode == 2)
  {
  if(screen_type & (SCREEN_TYPE_SVGALIB | SCREEN_TYPE_FBDEV))
    {
    lir_text(15,line[0]+2,"Exit with ctlC");
    }
  else
    {
    lir_text(15,line[0]+2,
       "Escape by clicking with the mouse mouse on the X in the title bar");
    }  
  if(screen_type == SCREEN_TYPE_WINDOWS)
    {
    lir_text(15,line[0]+3,"or with ESC from keyboard.");
    }
  }
lir_refresh_screen();
}

void show_errmsg(int screen_mode)
{
char s[80];
int line;
if(lir_errcod == 0)
  {
  clear_screen();
  lir_text(3,3,"Closing.");
  lir_refresh_screen();
  return;
  }
if(screen_mode != 0)
  {
  settextcolor(15);
  clear_screen();
  }
sprintf(s,"INTERNAL ERROR: %d",lir_errcod);
if(screen_mode)
  {
  lir_text(0,2,s);
  }
else
  {
  printf("\n%s",s);
  DEB"\n%s",s);
  }  
msg_filename="errors.lir";
line=3;
write_from_msg_file(&line, lir_errcod, screen_mode, ERROR_VERNR);
lir_sleep(5000000);
}  


void help_message(int msg_no)
{
int i, line;
if(msg_no < 0)return;
msg_filename="help.lir";
line=0;
settextcolor(15);
clear_screen();
#if INTERNAL_GEN_FILTER_TEST == TRUE  
  i=1;
#else
  i=2;
#endif  
if(msg_no != 1)
  {
  write_from_msg_file(&line, msg_no, TRUE, HELP_VERNR);
  line+=2;
  lir_text(0,line,
             "Press F1 or ! for KEYBOARD COMMANDS. Any other key to exit help.");
  await_keyboard();
  if(lir_inkey != F1_KEY && lir_inkey != '!')goto ex;
  line=0;
  clear_screen();  
  }
write_from_msg_file(&line, i, TRUE, HELP_VERNR);
await_processed_keyboard();
ex:;
lir_inkey=0;
settextcolor(7);
clear_screen();
}
