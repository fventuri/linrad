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
#include "uidef.h"
#include "hwaredef.h"
#include "fft1def.h"
#include "screendef.h"
#include "seldef.h"
#include "sdrdef.h"
#include "thrdef.h"
#include "txdef.h"

int hware_flag;
int fg_new_band;

char hware_error_flag;
double hware_time;
int wse_parport_status;
int wse_parport_control;
int wse_parport_ack;
int wse_parport_ack_sign;

#if (LUSERS_ROUTINES_PRESENT == 1 && OSNUM == OSNUM_LINUX)
#include <semaphore.h>
extern int serport;
#include "lscreen.h"
#include "users_hwaredriver.c"
#else
#if (WUSERS_ROUTINES_PRESENT == 1 && OSNUM == OSNUM_WINDOWS)
#include "windef.h"
#include "wscreen.h"
extern HANDLE serport;
#include "wusers_hwaredriver.c"
#else

void mouse_on_users_graph(void){}
void init_users_control_window(void){}
void users_init_mode(void){}
void users_eme(void){}
void userdefined_u(void){}
void userdefined_q(void){}
void users_set_band_no(void){}
void update_users_rx_frequency(void){};




// **********************************************************
//                  open and close
// **********************************************************

void users_close_devices(void)
{
// WSE units do not use the serial port.
// and there is nothing to close.
}

void users_open_devices(void)
{
}

#include "wse_sdrxx.c"

#endif
#endif
