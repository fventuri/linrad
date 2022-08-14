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
#include "fft2def.h"

int yieldflag_fft2_fft2;
float *fft2_float;
unsigned short int *fft2_permute;
float *fft2_window;
COSIN_TABLE *fft2_tab;
short int *fft2_short_int;
unsigned int *fft2_bigpermute;
MMX_COSIN_TABLE *fft2_mmxcosin;
short int *fft2_mmxwin;                       

float *hg_fft2_pwrsum;
float *hg_fft2_pwr;
float *hg_fft2_pol;
int hg_floatypix;
int make_fft2_status;


float *fft2_power_float;
float *fft2_powersum_float;
int *fft2_power_int;


TWOCHAN_POWER *fft2_xypower;
TWOCHAN_POWER *fft2_xysum;

int fft2_n;
int fft2_size;
int fft2_interleave_points;
int max_fft2n;
int fft2_pa;
int fft2_pt;
int fft2_na;
int fft2_nb;
int fft2_nc;
int fft2_nm;
int fft2_nx;
int fft2n_mask;
int fft2_blockbytes;
int fft2_totbytes;
int fft2_mask;
int fft2_chunk_n;
int fft2_chunk_counter;
int fft2_inc;
int fft2_m1;
int fft2_m2;
int fft2_new_points;
int fft2_att_limit;
int hgwat_first_xpoint;
int hgwat_xpoints_per_pixel;
int hgwat_pixels_per_xpoint;
int fft2_maxamp[MAX_RX_CHANNELS];

float fft2_blocktime;
float fft2_bandwidth;

float fft2_wtb;
float fft2_wttim;  

