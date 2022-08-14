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
#include "thrdef.h"
#include "xdef.h"

unsigned char *mempix_char;
unsigned short int *mempix_shi;
int first_mempix;
int last_mempix;
int color_depth;
unsigned char *xpalette;
int mouse_wheel_flag;
#if HAVE_X11 == 1 
pthread_t thread_identifier_process_event;
pthread_t thread_identifier_refresh_screen;
int process_event_flag;
int expose_event_done;
int shift_key_status;
int alt_key_status;


GC xgc;
XImage *ximage;
Display *xdis;
Window xwin;
Colormap lir_colormap;

#endif