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
#include "caldef.h"

char *cal_type_text[CAL_TYPE_MAX]={
                     "Refine filter calibration",
                     "Collect pulses to calibrate filter",
                     "I/Q balance calibration",
                     "I/Q balance calibration, data in ram",
                     "I/Q balance calibration, collect data",
                     "Adjust pulse generator for filter calibration",
                     "Set desired shape for filter calibration",
                     "Remove center discontinuity. Set exclude",
                     "Remove center discontinuity results",
                     "Interpolate across center discontinuity",
                     "Calibration routines"};

int cal_type;


short int *cal_graph;
unsigned int *cal_permute;
COSIN_TABLE *cal_table; 
float *cal_win;
float *cal_buf;
float *cal_buf2;
float *cal_buf3;
float *cal_buf4;
float *cal_buf5;
float *cal_buf6;
float *cal_buf7;
float *cal_tmp;
float *cal_fft1_desired;
float *cal_fft1_filtercorr;
float *cal_fft1_sumsq;
float *cal_fft1_slowsum;

int *bal_flag;
int *bal_pos;
float *bal_phsum;
float *bal_amprat;
float *contracted_iq_foldcorr;
MEM_INF calmem[MAX_CAL_ARRAYS];
size_t *calmem_handle;
int bal_updflag;
int bal_segments;
int bal_screen;
float cal_ymax;
float cal_yzer;
float cal_xgain;
int cal_xshift;
float cal_ygain;
int cal_lowedge;
int cal_midlim;
int cal_domain;
float cal_interval;
float cal_signal_level;
int cal_fft1_n;
int cal_fft1_size;
int caliq_clear_flag;
