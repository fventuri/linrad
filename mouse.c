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
#include "uidef.h"
#include "screendef.h"
#include "keyboard_def.h"
#include "thrdef.h"


void check_mouse_actions(void)
{
int i, task_info;
if(kill_all_flag) return;
if(mouse_inhibit || mouse_task != -1)return;
if( rbutton_state == new_rbutton_state &&
    lbutton_state == new_lbutton_state )return;
task_info=0;
if(new_rbutton_state==1 && rbutton_state==0)
  {
  task_info=GRAPH_RIGHTPRESSED;
  }
else
  {  
  if(new_lbutton_state==1 && lbutton_state==0)
    {
    task_info=0;
    }
  else
    {
    goto mouseact_x;
    }
  }    
for(i=0; i<no_of_scro; i++)
  {
  if(scro[i].no != METER_GRAPH || mg_flag != 0)
    {
    if( scro[i].x1 <= mouse_x && 
        scro[i].x2 >= mouse_x &&      
        scro[i].y1 <= mouse_y && 
        scro[i].y2 >= mouse_y) 
      {
      mouse_task=scro[i].no+task_info;
      }
    }
  }  
mouseact_x:;
lir_sched_yield();
if( (mouse_task&GRAPH_MASK) > MAX_WIDEBAND_GRAPHS ||
                             (ui.network_flag&NET_RXIN_FFT1) != 0)
  {
  lir_set_event(EVENT_FFT1_READY);
  }
else
  {
  lir_set_event(EVENT_TIMF1);
  }
}

void set_button_states(void)
{
if( (mouse_task&GRAPH_RIGHTPRESSED) == 0)
  {
  if (lbutton_state==0)
    {
    if (leftpressed==BUTTON_IDLE)
      {
      leftpressed = BUTTON_PRESSED;
      lbutton_state=1;
      }
    }
  else
    {
    if(new_lbutton_state == 0)
      {
      if(leftpressed==BUTTON_PRESSED)
        {
        leftpressed = BUTTON_RELEASED;
        lbutton_state=0;
        }
      }  
    }
  }
else
  {    
  if(rbutton_state==0)
    {
    if (rightpressed==BUTTON_IDLE)
      {
      rightpressed = BUTTON_PRESSED;
      rbutton_state=1;
      }
    }
  else
    {
    if(new_rbutton_state == 0)
      {
      if(rightpressed==BUTTON_PRESSED)
        {
        rightpressed = BUTTON_RELEASED;
        rbutton_state=0;
        }
      }
    }
  }
}


void set_button_coordinates(void)
{
mouse_x=new_mouse_x;
mouse_y=new_mouse_y;
if (new_lbutton_state!=0)
  {
  mouse_lbutton_x=new_mouse_x;
  mouse_lbutton_y=new_mouse_y;
  }
if (new_rbutton_state!=0)
  {
  mouse_rbutton_x=new_mouse_x;
  mouse_rbutton_y=new_mouse_y;
  }
}

void open_mouse(void)
{
mouse_task=-1;
mouse_hide_flag=1;
unconditional_hide_mouse();
leftpressed=0;
rightpressed=0;
mouse_active_flag=0;
mouse_inhibit=FALSE;
mouse_flag=1;
}

void close_mouse(void)
{
mouse_active_flag=0;
mouse_flag=0;
}

void mouse_nothing(void)
{
if(rightpressed == BUTTON_IDLE)
  {
  if(leftpressed == BUTTON_RELEASED)
    {
    leftpressed=BUTTON_IDLE;
    mouse_active_flag=0;
    }
  }
else
  {
  if(rightpressed == BUTTON_RELEASED)
    {
    rightpressed=BUTTON_IDLE;
    mouse_active_flag=0;
    }
  }
}


void erase_numinput_txt(void)
{
int i;
pause_screen_and_hide_mouse();
settextcolor(15);
numinput_txt[0]=1;
for(i=1; i<numinput_chars; i++)numinput_txt[i]=' ';
numinput_txt[numinput_chars]=0;
lir_pixwrite(numinput_xpix,numinput_ypix,numinput_txt);  
numinput_curpos=0;
settextcolor(7);
resume_thread(THREAD_SCREEN);
}

void get_numinput_chars(void)
{
int i, flag;
flag=0;
if(lir_inkey == 10 || lir_inkey == 13)
  {
  switch (numinput_flag)
    {
    case FIXED_INT_PARM:
    sscanf(numinput_txt,"%d",&numinput_int_data);
    break;

    case FIXED_FLOAT_PARM:
    sscanf(numinput_txt,"%f",&numinput_float_data);
    break;
    
    case FIXED_DOUBLE_PARM:
    sscanf(numinput_txt,"%lf",&numinput_double_data);
    break;
    }
  numinput_flag|=DATA_READY_PARM;
  lir_inkey=0;
  return;
  } 
pause_screen_and_hide_mouse();
settextcolor(15);
if(lir_inkey == 8 || lir_inkey == 127 || lir_inkey == ARROW_LEFT_KEY)
  {
  if(numinput_curpos==0)goto gncx;
  numinput_curpos--;
  numinput_txt[numinput_curpos]=1;
  numinput_txt[numinput_curpos+1]=' ';
  lir_pixwrite(numinput_xpix,numinput_ypix,numinput_txt);  
  goto gncx;
  }
if( (lir_inkey >= '0' && lir_inkey <= '9') || numinput_flag == TEXT_PARM)
  {
  flag=1;
  }
if(numinput_curpos == 0 && lir_inkey == '-' && numinput_flag != TEXT_PARM)
  {
  flag=1;
  }
if( numinput_flag == FIXED_FLOAT_PARM || numinput_flag == FIXED_DOUBLE_PARM)
  {
  if(lir_inkey == '.' || lir_inkey == 'E')
    {
    for(i=0; i<numinput_curpos; i++)if(numinput_txt[i] == lir_inkey)goto gncx;
    flag=1;
    }
  if(numinput_curpos > 0 && lir_inkey == '-')
    {
    if(numinput_txt[numinput_curpos-1] == 'E')
      {
      flag=1;
      }
    }
  }
if(flag == 1)
  {
  numinput_txt[numinput_curpos]=lir_inkey;
  numinput_curpos++;
  }
if(numinput_curpos > numinput_chars-1)
  {
  numinput_curpos=numinput_chars-1;
  }
else
  {  
  numinput_txt[numinput_curpos]=1;
  }
numinput_txt[numinput_curpos+1]=0;
lir_pixwrite(numinput_xpix,numinput_ypix,numinput_txt);  
cursor_blink_time=current_time();
gncx:;
settextcolor(7);
resume_thread(THREAD_SCREEN);
}

