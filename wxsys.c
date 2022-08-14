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
#include "thrdef.h"
#include "keyboard_def.h"

void wxmouse(void)
{
mouse_thread_flag=THRFLAG_ACTIVE;
while(mouse_thread_flag==THRFLAG_ACTIVE)
  {
// **********************************************
// Wait until a mouse event occurs. 
  lir_await_event(EVENT_MOUSE);
  if( thread_status_flag[THREAD_SCREEN] == THRFLAG_SEM_WAIT ||
      thread_status_flag[THREAD_SCREEN] == THRFLAG_ACTIVE ||
      thread_status_flag[THREAD_SCREEN] == THRFLAG_IDLE) 
    {
    lir_move_mouse_cursor();
    check_mouse_actions();
    }
  }
mouse_thread_flag=THRFLAG_RETURNED;  
}

void lir_remove_mouse_thread(void)
{
if(mouse_thread_flag == THRFLAG_NOT_ACTIVE)return;
mouse_thread_flag=THRFLAG_KILL;
while(mouse_thread_flag != THRFLAG_RETURNED)
  {
  lir_set_event(EVENT_MOUSE);
  lir_sleep(1000);
  }
}

void store_in_kbdbuf(int c)
{
if(c==13)c=10;
keyboard_buffer[keyboard_buffer_ptr]=c;
keyboard_buffer_ptr=(keyboard_buffer_ptr+1)&(KEYBOARD_BUFFER_SIZE-1);
lir_sleep(10000);  
lir_set_event(EVENT_KEYBOARD);
}

