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
#include "blnkdef.h"

unsigned char chan_color[2]={2,4};

size_t *blanker_handle;
float *blanker_refpulse;
float *blanker_phasefunc;
float *blanker_input;
signed int *blanker_pulindex;
signed char *blanker_flag;


BLANKER_CONTROL_INFO bln[BLN_INFO_SIZE];
float blanker_pol_c1;
float blanker_pol_c2;
float blanker_pol_c3;

signed int largest_blnfit;
signed int blnclear_range;
signed int blnfit_range;

signed int timf2_show_pointer;
signed int blanker_pulsewidth;
signed int refpul_n;
signed int refpul_size;
signed int timf2_noise_floor_avgnum;
signed int timf2_noise_floor;

float timf2_despiked_pwr[2];
float timf2_despiked_pwrinc[2];
float clever_blanker_rate;
float stupid_blanker_rate;
signed int blanker_info_update_counter;
signed int blanker_info_update_interval;
float blanker_phaserot;

