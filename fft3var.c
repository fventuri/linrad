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
#include "fft3def.h"

double fft3_show_time;
int yieldflag_ndsp_fft3;
int yieldflag_ndsp_mix2;
size_t *fft3_handle;
float *fft3;
double *d_fft3;
COSIN_TABLE *fft3_tab;
D_COSIN_TABLE *d_fft3_tab;
unsigned short int *fft3_permute;
unsigned int *d_fft3_permute;
float *fft3_window;
double *d_fft3_window;
float *fft3_tmp;
double *d_fft3_tmp;
float *fft3_power;
float *fft3_fqwin_inv;
double *d_fft3_fqwin_inv;
float *fft3_slowsum;
float *fft3_cleansum;
float *bg_filterfunc;
float *bg_carrfilter;
short int *fft3_spectrum;
short int *bg_filterfunc_y;
short int *bg_carrfilter_y;
float *timf3_float;
double *d_timf3_float;
int *timf3_int;
short int *timf3_graph;
int fft3_n;
int fft3_size;
int fft3_block;
int fft3_totsiz;
int fft3_mask;
float fft3_blocktime;
int fft3_pa;
int fft3_px;
int fft3_show_size;
int fft3_slowsum_recalc;
int fft3_slowsum_cnt;
int bg_show_pa;

float fft3_interleave_ratio;
int fft3_interleave_points;
int fft3_new_points;
float *basebraw_fir;
int basebraw_fir_pts;
float *basebwbraw_fir;
int basebwbraw_fir_pts;
float *basebcarr_fir;
double *d_basebcarr_fir;
int basebcarr_fir_pts;
int d_basebcarr_fir_pts;


int timf3_size;
int timf3_mask;
int timf3_y0[8];
float timf3_wttim;
float fft3_wttim;
float timf3_sampling_speed;
int timf3_osc_interval;
int timf3_block;
int timf3_pa;
int timf3_px;
int timf3_py;
int timf3_ps;
int timf3_pn;
int timf3_pc;
int timf3_pd;
int timf3_wts;
float fft3_wtb;

