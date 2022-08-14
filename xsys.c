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
#include <semaphore.h>
#include <pthread.h>
#include <ctype.h>
#include <string.h>
#if HAVE_OSS == 1
  #if OSSD == 1
  #include <sys/soundcard.h>
  #else
  #include <soundcard.h>
  #endif
#include <sys/ioctl.h>
#endif

#include "globdef.h"
#include "thrdef.h"
#include "uidef.h"
#include "screendef.h"
#include "xdef.h"
#include "keyboard_def.h"
#include "vernr.h"
#include "sdrdef.h"
#include "options.h"
#include "lscreen.h"

#if HAVE_X11 == 1
extern GC xgc;
extern XImage *ximage;
extern Display *xdis;
extern Window xwin;
extern Colormap lir_colormap;


typedef struct {
unsigned short int red;
unsigned short int green;
unsigned short int blue;
unsigned int pixel;
short int flag;
float total;
}PIXINFO;
/*
int first_mempix_copy;
int last_mempix_copy;
char first_mempix_flag;
char last_mempix_flag;
*/

void thread_refresh_screen(void)
{
int l1, h, h1, n;
while(refresh_screen_flag)
  {
  lir_await_event(EVENT_REFRESH_SCREEN);
  if(last_mempix >= first_mempix )
    {
    first_mempix_flag=0;
    last_mempix_flag=0;
    l1=first_mempix/screen_width;
    h=last_mempix/screen_width-l1+1;
    if(h>screen_height)h=screen_height;
    first_mempix=0x7fffffff;
    last_mempix=0;
    lir_sched_yield();
    if(first_mempix_flag != 0)first_mempix=first_mempix_copy;
    if(last_mempix_flag != 0)last_mempix=last_mempix_copy;
    while(h>0)
      {
      n=(5*h)/screen_height;
      if(n > 1)
        {
        h1=h/n;
        }
      else
        {
        h1=h;
        }
      if(screen_type != SCREEN_TYPE_X11_SHM)
        {
        XPutImage(xdis, xwin, xgc, ximage, 0, l1, 0, l1, screen_width, h1);
        }
#if HAVE_SHM == 1
      else
        {
        XShmPutImage(xdis, xwin, xgc, ximage, 0, l1, 0, l1, screen_width, h1,FALSE);
        }
#endif
      l1+=h1;
      h-=h1;
      if(h > 0)lir_sched_yield();
      }
#if AVOID_XFLUSH == TRUE    
#if USE_XFLUSH == 1
    XFlush(xdis);
#endif
#else
    XFlush(xdis);
#endif
    }
  }
}  
#endif