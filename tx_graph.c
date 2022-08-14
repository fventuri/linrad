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
#include "keyboard_def.h"
#include "padef.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

void set_default_cwproc_parms(void)
{
txcw.rise_time=10000;
txcw.enable_hand_key=0;
txcw.enable_tone_key=0;
txcw.enable_ascii=0;
txcw.radar_pulse=10;
txcw.radar_interval=25*txcw.radar_pulse;
txcw.delay_margin=15;
txcw.carrier_freq=1500;
txcw.sidetone_ampl=250;
txcw.sidetone_freq=500;
txcw.wcw_mute_on=5;
txcw.wcw_mute_off=120;
txcw.ncw_mute_on=5;
txcw.ncw_mute_off=120;
txcw.hsms_mute_on=5;
txcw.hsms_mute_off=120;
txcw.qrss_mute_on=5;
txcw.qrss_mute_off=120;
txcw.radar_mute_on=5;
txcw.radar_mute_off=120;
}

void make_txproc_filename(void)
{
switch (rx_mode)
  {
  case MODE_SSB:
  sprintf(&ssbproc_filename[11],"%02d",tg.spproc_no);
  txproc_filename=ssbproc_filename;
  tx_mode=TXMODE_SSB;
  break;

  case MODE_WCW:
  case MODE_NCW:
  case MODE_HSMS:
  case MODE_QRSS:
  case MODE_RADAR:
  sprintf(&cwproc_filename[10],"%02d",tg.spproc_no);
  txproc_filename=cwproc_filename;
  tx_mode=TXMODE_CW;
  break;
  
  default:
  tx_mode=TXMODE_OFF;
  txproc_filename=NULL;
  break;
  }
}


void save_tx_parms(char verbose)
{
int *wgi;
char s[80];
char **partext;
int i, nn, line;
FILE *txpar_file;
wgi=NULL;
nn=0;
partext=NULL;
line=4;
make_txproc_filename();
txpar_file=fopen(txproc_filename,"w");
if(txpar_file == NULL)
  {
  lirerr(381164);
  return;
  }
switch (rx_mode)
  {
  case MODE_SSB:
  wgi=(int*)(&txssb);
  txssb.check=SSBPROC_VERNR;
  partext=ssbprocparm_text;
  nn=MAX_SSBPROC_PARM;
  break;

  case MODE_WCW:
  case MODE_NCW:
  case MODE_HSMS:
  case MODE_QRSS:
  case MODE_RADAR:
  wgi=(int*)(&txcw);
  txcw.check=CWPROC_VERNR;
  partext=cwprocparm_text;
  nn=MAX_CWPROC_PARM;
  break;
  
  default:
  lirerr(918453);
  break;
  }
for(i=0; i<nn; i++)
  {
  sprintf(s,"%s [%d]",partext[i],wgi[i]);
  fprintf(txpar_file,"%s\n",s);
  if(verbose && line<screen_last_line)lir_text(5,line,s);
  line++;
  }
parfile_end(txpar_file);
if(!verbose)return;
sprintf(s,"Parameters saved to %s.   %s",txproc_filename, press_any_key);
lir_text(10,line+3,s);  
await_keyboard();
}

void select_txout_format(int *line)
{
line[0]++;
lir_text(3,line[0],"Select tx format");
line[0]+=2;
lir_text(7,line[0],"1=Single channel (normal audio)");
line[0]++;
lir_text(7,line[0],"2=Two channels (I and Q, frequency shifted)");
line[0]++;
get_txout_dachan:;
await_processed_keyboard();
if(kill_all_flag)return;
if(lir_inkey=='1')
  {
  ui.tx_da_channels=1;
  }
else
  {
  if(lir_inkey=='2')
    {
    ui.tx_da_channels=2;
    }
  else
    {
    goto get_txout_dachan;
    }
  }
}



int check_tx_devices(void)
{
if(ui.tx_addev_no < 0 ||
   ui.tx_addev_no >= MAX_PA_DEVICES ||
   ui.tx_dadev_no < 0 ||
   ui.tx_dadev_no >= MAX_PA_DEVICES ||
   ui.tx_da_channels < 1 ||
   ui.tx_da_channels > 2 ||
   ui.tx_da_bytes < 2 || 
   ui.tx_da_bytes > 4 ||
   ui.tx_ad_bytes < 2 ||
   ui.tx_ad_bytes > 4 ) return FALSE;
return TRUE;
}


int read_txpar_file(void)
{
int i, j, k, nn;
int *wgi;
int rdbytes;
float t1;
FILE *txpar_file;
char *testbuff;
char **partext;
partext=NULL;
wgi=NULL;
nn=0;
if(check_tx_devices() == FALSE)return FALSE;
testbuff = malloc(4096);
if(testbuff == NULL)
  {
  lirerr(1286);
  return FALSE;
  }
for(i=0; i<4096; i++)testbuff[i]=0;  
make_txproc_filename();
if(txproc_filename == NULL)return FALSE;
txpar_file=fopen(txproc_filename,"r");
if(txpar_file != NULL)
  {
  rdbytes=fread(testbuff,1,4095,txpar_file);
  fclose(txpar_file);
  if(rdbytes >= 4095)goto reterr;
  switch (rx_mode)
    {
    case MODE_SSB:
    wgi=(int*)(&txssb);
    partext=ssbprocparm_text;
    nn=MAX_SSBPROC_PARM;
    break;

    case MODE_WCW:
    case MODE_NCW:
    case MODE_HSMS:
    case MODE_QRSS:
    case MODE_RADAR:
    wgi=(int*)(&txcw);
    partext=cwprocparm_text;
    nn=MAX_CWPROC_PARM;
    break;
  
    default:
    lirerr(918453);
    break;
    }
  k=0;
  for(i=0; i<nn; i++)
    {
    while( (testbuff[k]==' ' || testbuff[k]== '\n' ) && k<rdbytes)k++;
    j=0;
    while(testbuff[k]== partext[i][j] && k<rdbytes)
      {
      k++;
      j++;
      } 
    if(partext[i][j] != 0)goto reterr;    
    while(testbuff[k]!='[' && k<rdbytes)k++;
    if(k>=rdbytes)goto reterr;
    sscanf(&testbuff[k],"[%d]",&wgi[i]);
    while(testbuff[k]!='\n' && k<rdbytes)k++;
    if(k>=rdbytes)goto reterr;
    }
  switch (rx_mode)
    {
    case MODE_SSB:
    if(txssb.check!=SSBPROC_VERNR ||
       txssb.minfreq < SSB_MINFQ_LOW || 
       txssb.minfreq > SSB_MINFQ_HIGH ||
       txssb.maxfreq < SSB_MAXFQ_LOW || 
       txssb.maxfreq > SSB_MAXFQ_HIGH ||
       txssb.slope > SSB_MAXSLOPE || 
       txssb.slope < SSB_MINSLOPE ||
       txssb.bass > SSB_MAXBASS || 
       txssb.bass < SSB_MINBASS ||
       txssb.treble > SSB_MAXTREBLE || 
       txssb.treble < SSB_MINTREBLE ||
       txssb.mic_f_threshold < 0 ||
       txssb.mic_f_threshold > SSB_MAX_MICF  ||
       txssb.mic_t_threshold < 0 ||
       txssb.mic_t_threshold > SSB_MAX_MICT ||
       txssb.mic_gain < 0 ||
       txssb.mic_gain > SSB_MAX_MICGAIN ||
       txssb.mic_agc_time < 0 ||
       txssb.mic_agc_time > SSB_MAX_MICAGC_TIME ||
       txssb.alc_time < 0 ||
       txssb.alc_time > SSB_MAX_ALC_TIME ||
       txssb.rf1_gain < SSB_MIN_RF1_GAIN ||
       txssb.rf1_gain > SSB_MAX_RF1_GAIN ||
       txssb.micblock_time > SSB_MAX_MICBLOCK_TIME ||
       txssb.micblock_time < SSB_MIN_MICBLOCK_TIME ||
       txssb.delay_margin < SSB_MIN_DELAY_MARGIN ||
       txssb.delay_margin > SSB_MAX_DELAY_MARGIN )
      {
      goto reterr;
      }
    break;

    case MODE_WCW:
    mute_on=txcw.wcw_mute_on;
    mute_off=txcw.wcw_mute_off;
    goto cw;

    case MODE_NCW:
    mute_on=txcw.ncw_mute_on;
    mute_off=txcw.ncw_mute_off;
    goto cw;
    
    case MODE_HSMS:
    mute_on=txcw.hsms_mute_on;
    mute_off=txcw.hsms_mute_off;
    goto cw;
    
    case MODE_QRSS:
    mute_on=txcw.qrss_mute_on;
    mute_off=txcw.qrss_mute_off;
    goto cw;
    
    case MODE_RADAR:
    if( txcw.check != CWPROC_VERNR || 
        txcw.rise_time < 1000*CW_MIN_TIME_CONSTANT ||
        txcw.rise_time > 1000*CW_MAX_TIME_CONSTANT ||
        txcw.radar_pulse < 0.0001*txcw.rise_time ||
        txcw.radar_pulse > 9999 ||
        txcw.radar_interval < 4*txcw.radar_pulse ||
        txcw.radar_interval > 99999 ||
        txcw.delay_margin < CW_MIN_DELAY_MARGIN ||
        txcw.delay_margin > CW_MAX_DELAY_MARGIN )
      {
      goto reterr;
      }
    mute_on=txcw.radar_mute_on;
    mute_off=txcw.radar_mute_off;
cw:;    
    sidetone_cos=txcw.sidetone_ampl;
    sidetone_sin=0;
    t1=(2*PI_L*txcw.sidetone_freq)/genparm[DA_OUTPUT_SPEED];
    sidetone_phstep_cos=cos(t1);
    sidetone_phstep_sin=sin(t1);
    break;
    }
  }  
else
  {
reterr:;
  free(testbuff);  
  return FALSE;
  }  
free(testbuff);  
return TRUE;
}

void set_default_spproc_parms(void)
{
txssb.minfreq=200;
txssb.maxfreq=2600;
txssb.slope=6;
txssb.bass=-8;
txssb.treble=0;
txssb.mic_gain=5;
txssb.mic_agc_time=100;
txssb.mic_f_threshold=15;
txssb.mic_t_threshold=0;
txssb.rf1_gain=6;
txssb.alc_time=25;
txssb.micblock_time=10;
txssb.delay_margin=10;
}

void check_tg_borders(void)
{
current_graph_minh=TG_VSIZ;
current_graph_minw=TG_HSIZ;
check_graph_placement((void*)(&tg));
set_graph_minwidth((void*)(&tg));
}

void new_ssbproc_no(void)
{
int oldno;
oldno=tg.spproc_no;
tg.spproc_no=numinput_int_data;
if(read_txpar_file() == FALSE)
  {
  tg.spproc_no=oldno;
  read_txpar_file();
  }
tg_basebfreq_hz=txcw.carrier_freq;
set_hardware_tx_frequency();
make_modepar_file(GRAPHTYPE_TG);
sc[SC_SHOW_TX_FQ]++;
}   

void copy_rxfreq_to_tx(void)
{
tg.freq=0.001*hwfreq;
set_hardware_tx_frequency();
sc[SC_SHOW_TX_FQ]++;
make_modepar_file(GRAPHTYPE_TG);
}


void new_tx_frequency(void)
{
tg.freq=numinput_float_data;
set_hardware_tx_frequency();
sc[SC_SHOW_TX_FQ]++;
make_modepar_file(GRAPHTYPE_TG);
}

void new_tx_signal_level(void)
{
tg.level_db=numinput_float_data;
if(tg.level_db<0)tg.level_db=0;
if(tg.level_db>120)tg.level_db=120;
tx_output_amplitude=pow(10.0,-0.05*tg.level_db);
sc[SC_SHOW_TX_FQ]++;
make_modepar_file(GRAPHTYPE_TG);
}

void new_mute_on(void)
{
mute_on=numinput_int_data;
switch (rx_mode)
  {
  case MODE_SSB:
  break;

  case MODE_WCW:
  txcw.wcw_mute_on=mute_on;
  break;

  case MODE_NCW:
  txcw.ncw_mute_on=mute_on;
  break;    

  case MODE_HSMS:
  txcw.hsms_mute_on=mute_on;
  break;    

  case MODE_QRSS:
  txcw.qrss_mute_on=mute_on;
  break;    

  case MODE_RADAR:
  txcw.radar_mute_on=mute_on;
  break;
  }
sc[SC_SHOW_TX_FQ]++;
save_tx_parms(FALSE);
}

void new_mute_off(void)
{
mute_off=numinput_int_data;
switch (rx_mode)
  {
  case MODE_SSB:
  break;

  case MODE_WCW:
  txcw.wcw_mute_off=mute_off;
  break;

  case MODE_NCW:
  txcw.ncw_mute_off=mute_off;
  break;    

  case MODE_HSMS:
  txcw.hsms_mute_off=mute_off;
  break;    

  case MODE_QRSS:
  txcw.qrss_mute_off=mute_off;
  break;    

  case MODE_RADAR:
  txcw.radar_mute_off=mute_off;
  break;
  }
sc[SC_SHOW_TX_FQ]++;
save_tx_parms(FALSE);
}

void help_on_tx_graph(void)
{
int msg_no;
int event_no;
// Set msg invalid in case we are not in any select area.
msg_no=-1;
for(event_no=0; event_no<MAX_TGBUTT; event_no++)
  {
  if( tgbutt[event_no].x1 <= mouse_x && 
      tgbutt[event_no].x2 >= mouse_x &&      
      tgbutt[event_no].y1 <= mouse_y && 
      tgbutt[event_no].y2 >= mouse_y) 
    {
    switch (event_no)
      {
      case TG_TOP:
      case TG_BOTTOM:
      case TG_LEFT:
      case TG_RIGHT:
      msg_no=101;
      break;

      case TG_INCREASE_FQ: 
      msg_no=104;
      break;
  
      case TG_DECREASE_FQ:
      msg_no=103;
      break;

      case TG_NEW_TX_FREQUENCY:
      msg_no=105;
      break;

      case TG_SET_SIGNAL_LEVEL:
      msg_no=107;
      break;

      case TG_CHANGE_TXPAR_FILE_NO:
      msg_no=106;
      break;
      }
    }  
  }
help_message(msg_no);
}

void mouse_continue_tx_graph(void)
{
int i, j;
switch (mouse_active_flag-1)
  {
  case TG_TOP:
  if(tg.ytop!=mouse_y)goto tgm;
  break;

  case TG_BOTTOM:
  if(tg.ybottom!=mouse_y)goto tgm;
  break;

  case TG_LEFT:
  if(tg.xleft!=mouse_x)goto tgm;
  break;

  case TG_RIGHT:
  if(tg.xright==mouse_x)break;
tgm:;
  pause_screen_and_hide_mouse();
  dual_graph_borders((void*)&tg,0);
  if(tg_oldx==-10000)
    {
    tg_oldx=mouse_x;
    tg_oldy=mouse_y;
    }
  else
    {
    i=mouse_x-tg_oldx;
    j=mouse_y-tg_oldy;  
    tg_oldx=mouse_x;
    tg_oldy=mouse_y;
    tg.ytop+=j;
    tg.ybottom+=j;      
    tg.xleft+=i;
    tg.xright+=i;
    check_tg_borders();
    }
  graph_borders((void*)&tg,15);
  resume_thread(THREAD_SCREEN);
  break;
    
  default:
  goto await_release;
  }
if(leftpressed == BUTTON_RELEASED)goto finish;
return;
await_release:;
if(leftpressed != BUTTON_RELEASED) return;
if(ui.network_flag != 2)
  {
  switch (mouse_active_flag-1)
    {
    case TG_INCREASE_FQ: 
    if(hware_flag == 0)
      {
      tg.freq+=tg.band_increment;
      set_hardware_tx_frequency();
      }
    break;
  
    case TG_DECREASE_FQ:
    if(hware_flag == 0)
      {
      tg.freq-=tg.band_increment;
      set_hardware_tx_frequency();
      }
    break;

    case TG_NEW_TX_FREQUENCY:
    if(hware_flag == 0)
      {
      numinput_xpix=tgbutt[TG_NEW_TX_FREQUENCY].x1+3;
      numinput_ypix=tgbutt[TG_NEW_TX_FREQUENCY].y2-text_height+1;
      numinput_chars=FREQ_MHZ_DIGITS+7;
      erase_numinput_txt();
      numinput_flag=FIXED_FLOAT_PARM;
      par_from_keyboard_routine=new_tx_frequency;
      }
    return;  
    
    case TG_SET_SIGNAL_LEVEL:
    if(hware_flag == 0)
      {
      numinput_xpix=tgbutt[TG_SET_SIGNAL_LEVEL].x1+3+4*text_width;
      numinput_ypix=tgbutt[TG_SET_SIGNAL_LEVEL].y2-text_height+1;
      numinput_chars=5;
      erase_numinput_txt();
      numinput_flag=FIXED_FLOAT_PARM;
      par_from_keyboard_routine=new_tx_signal_level;
      }
    return;
    
    case TG_CHANGE_TXPAR_FILE_NO:
    if(hware_flag == 0)
      {
      numinput_xpix=tgbutt[TG_CHANGE_TXPAR_FILE_NO].x1+3+5*text_width;
      numinput_ypix=tgbutt[TG_CHANGE_TXPAR_FILE_NO].y2-text_height+1;
      numinput_chars=2;
      erase_numinput_txt();
      numinput_flag=FIXED_INT_PARM;
      par_from_keyboard_routine=new_ssbproc_no;
      }
    return;  

    case TG_MUTE_ON:
    numinput_xpix=tgbutt[TG_MUTE_ON].x1+3+4*text_width;
    numinput_ypix=tgbutt[TG_MUTE_ON].y2-text_height;
    numinput_chars=3;
    erase_numinput_txt();
    numinput_flag=FIXED_INT_PARM;
    par_from_keyboard_routine=new_mute_on;
    return;
    
    case TG_MUTE_OFF:
    numinput_xpix=tgbutt[TG_MUTE_OFF].x1+3+3*text_width;
    numinput_ypix=tgbutt[TG_MUTE_OFF].y2-text_height;
    numinput_chars=4;
    erase_numinput_txt();
    numinput_flag=FIXED_INT_PARM;
    par_from_keyboard_routine=new_mute_off;
    return;
    
    case TG_ONOFF:
    tx_onoff_flag^=1;
    break;
        
    default:
    lirerr(872);
    break;
    }
  }  
finish:;
leftpressed=BUTTON_IDLE;  
mouse_active_flag=0;
make_tx_graph(TRUE);
tg_oldx=-10000;
}

void mouse_on_tx_graph(void)
{
int event_no;
// First find out is we are on a button or border line.
for(event_no=0; event_no<MAX_TGBUTT; event_no++)
  {
  if( tgbutt[event_no].x1 <= mouse_x && 
      tgbutt[event_no].x2 >= mouse_x &&      
      tgbutt[event_no].y1 <= mouse_y && 
      tgbutt[event_no].y2 >= mouse_y) 
    {
    tg_old_y1=tg.ytop;
    tg_old_y2=tg.ybottom;
    tg_old_x1=tg.xleft;
    tg_old_x2=tg.xright;
    mouse_active_flag=1+event_no;
    current_mouse_activity=mouse_continue_tx_graph;
    return;
    }
  }
// Not button or border.
current_mouse_activity=mouse_nothing;
mouse_active_flag=1;
}


void make_tx_graph(int clear_old)
{
pause_thread(THREAD_SCREEN);
if(clear_old)
  {
  hide_mouse(tg_old_x1,tg_old_x2,tg_old_y1,tg_old_y2);
  lir_fillbox(tg_old_x1,tg_old_y1,tg_old_x2-tg_old_x1+1,
                                                    tg_old_y2-tg_old_y1+1,0);
  }
tg_flag=1;
check_tg_borders();
clear_button(tgbutt, MAX_TGBUTT);
hide_mouse(tg.xleft,tg.xright,tg.ytop,tg.ybottom);  
scro[tx_graph_scro].no=TRANSMIT_GRAPH;
scro[tx_graph_scro].x1=tg.xleft;
scro[tx_graph_scro].x2=tg.xright;
scro[tx_graph_scro].y1=tg.ytop;
scro[tx_graph_scro].y2=tg.ybottom;
tgbutt[TG_LEFT].x1=tg.xleft;
tgbutt[TG_LEFT].x2=tg.xleft;
tgbutt[TG_LEFT].y1=tg.ytop;
tgbutt[TG_LEFT].y2=tg.ybottom;
tgbutt[TG_RIGHT].x1=tg.xright;
tgbutt[TG_RIGHT].x2=tg.xright;
tgbutt[TG_RIGHT].y1=tg.ytop;
tgbutt[TG_RIGHT].y2=tg.ybottom;
tgbutt[TG_TOP].x1=tg.xleft;
tgbutt[TG_TOP].x2=tg.xright;
tgbutt[TG_TOP].y1=tg.ytop;
tgbutt[TG_TOP].y2=tg.ytop;
tgbutt[TG_BOTTOM].x1=tg.xleft;
tgbutt[TG_BOTTOM].x2=tg.xright;
tgbutt[TG_BOTTOM].y1=tg.ybottom;
tgbutt[TG_BOTTOM].y2=tg.ybottom;
// Draw the border lines
graph_borders((void*)&tg,7);
tg_oldx=-10000;
settextcolor(7);
make_button(tg.xleft+text_width,tg.ybottom-text_height/2-2,
                                       tgbutt,TG_DECREASE_FQ,25);
tgbutt[TG_NEW_TX_FREQUENCY].x1=tgbutt[TG_DECREASE_FQ].x2+0.5*text_width-2;
tgbutt[TG_NEW_TX_FREQUENCY].x2=tgbutt[TG_NEW_TX_FREQUENCY].x1+
                                      (FREQ_MHZ_DIGITS+9)*text_width;
tgbutt[TG_NEW_TX_FREQUENCY].y1=tgbutt[TG_DECREASE_FQ].y1;
tgbutt[TG_NEW_TX_FREQUENCY].y2=tgbutt[TG_DECREASE_FQ].y2;
make_button(tgbutt[TG_NEW_TX_FREQUENCY].x2+text_width,
                      tg.ybottom-text_height/2-2, tgbutt,TG_INCREASE_FQ,24); 
if(rx_mode == MODE_RADAR)
  {
  tgbutt[TG_RADAR_INTERVAL].x1=tgbutt[TG_INCREASE_FQ].x2+2;
  tgbutt[TG_RADAR_INTERVAL].x2=tgbutt[TG_RADAR_INTERVAL].x1+5.5*text_width;
  tgbutt[TG_RADAR_INTERVAL].y1=tgbutt[TG_DECREASE_FQ].y1;
  tgbutt[TG_RADAR_INTERVAL].y2=tgbutt[TG_DECREASE_FQ].y2;
  }
tgbutt[TG_CHANGE_TXPAR_FILE_NO].x1=tg.xleft+2;
tgbutt[TG_CHANGE_TXPAR_FILE_NO].x2=tg.xleft+2+(15*text_width)/2;
tgbutt[TG_CHANGE_TXPAR_FILE_NO].y2=tgbutt[TG_DECREASE_FQ].y1-2;
tgbutt[TG_CHANGE_TXPAR_FILE_NO].y1=tgbutt[TG_CHANGE_TXPAR_FILE_NO].y2-text_height-1;

tgbutt[TG_SET_SIGNAL_LEVEL].x1=tgbutt[TG_CHANGE_TXPAR_FILE_NO].x2+text_width/2;
tgbutt[TG_SET_SIGNAL_LEVEL].x2=tgbutt[TG_SET_SIGNAL_LEVEL].x1+(25*text_width)/2;
tgbutt[TG_SET_SIGNAL_LEVEL].y1=tgbutt[TG_CHANGE_TXPAR_FILE_NO].y1;
tgbutt[TG_SET_SIGNAL_LEVEL].y2=tgbutt[TG_CHANGE_TXPAR_FILE_NO].y2;

tgbutt[TG_ONOFF].x1=tg.xleft+2;
tgbutt[TG_ONOFF].x2=tg.xleft+2+3.5*text_width;
tgbutt[TG_ONOFF].y2=tgbutt[TG_CHANGE_TXPAR_FILE_NO].y1-2;
tgbutt[TG_ONOFF].y1=tgbutt[TG_ONOFF].y2-text_height-2;

tgbutt[TG_MUTE_ON].x1=tgbutt[TG_ONOFF].x2+2;
tgbutt[TG_MUTE_ON].x2=tgbutt[TG_MUTE_ON].x1+(15*text_width)/2;
tgbutt[TG_MUTE_ON].y1=tgbutt[TG_ONOFF].y1;
tgbutt[TG_MUTE_ON].y2=tgbutt[TG_ONOFF].y2;

tgbutt[TG_MUTE_OFF].x1=tgbutt[TG_MUTE_ON].x2+2;
tgbutt[TG_MUTE_OFF].x2=tgbutt[TG_MUTE_OFF].x1+(15*text_width)/2;;
tgbutt[TG_MUTE_OFF].y1=tgbutt[TG_ONOFF].y1;
tgbutt[TG_MUTE_OFF].y2=tgbutt[TG_ONOFF].y2;

resume_thread(THREAD_SCREEN);
sc[SC_SHOW_TX_FQ]++;
make_modepar_file(GRAPHTYPE_TG);
}

void get_tg_parms(void)
{
if (read_modepar_file(GRAPHTYPE_TG) == 0)
  {
  tg.xright=screen_width-text_width;;
  tg.xleft=tg.xright-TG_HSIZ;
  tg.ybottom=screen_height-4*text_height;
  tg.ytop=tg.ybottom-TG_VSIZ;
  tg.spproc_no=0;
  tg.passband_direction=1;
  tg.level_db=0;
  tg.band_increment=.01;
  tg.band_center=7.05;
  }
if(tg.spproc_no<0)tg.spproc_no=0;
if(tg.spproc_no>MAX_SSBPROC_FILES)tg.spproc_no=MAX_SSBPROC_FILES;
}

void init_tx_graph(void)
{
get_tg_parms();
tx_graph_scro=no_of_scro;
tx_onoff_flag=0;
make_tx_graph(FALSE);
no_of_scro++;
if(no_of_scro >= MAX_SCRO)lirerr(89);
if(read_txpar_file()==FALSE)
  {
  lirerr(1428);
  return;
  }
set_hardware_tx_frequency();
}
