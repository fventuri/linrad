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
#include "seldef.h"

int skip_siganal;
int yieldflag_ndsp_mix1;
int freq_readout_x1;
int freq_readout_x2;
int freq_readout_y1;
int netslaves_selfreq[MAX_NETSLAVES];
float netslaves_time[MAX_NETSLAVES];
int netfreq_list[MAX_FREQLIST];
float netfreq_time[MAX_FREQLIST];
int netfreq_curx[MAX_FREQLIST];
int new_netfreq_curx[MAX_FREQLIST];

int enable_resamp_iir5;
IIR5_PARMS iir_a, iir_b;
double xva1[6],yva1[6];
double xvb1[6],yvb1[6];
double xva2[6],yva2[6];
double xvb2[6],yvb2[6];
double xv1[4], yv1[4];
double xv2[4], yv2[4];
double xv3[4], yv3[4];
double xv4[4], yv4[4];
double out1, out2, old_out1, old_out2;
double out3, out4, old_out3, old_out4;

int spursearch_sum_counter;
float *spursearch_powersum;
TWOCHAN_POWER *spursearch_xysum;
float *spursearch_spectrum;
int spur_search_first_point;
int spur_search_last_point;
int basebnet_px;
int basebnet_pa;
char *baseb_netsend_buffer;
int basebnet_mask;
int basebnet_size;

int allow_audio;
int audio_status;

char *basebraw_netsend_buffer;
int basebrawnet_mask;
int basebrawnet_size;
int basebrawnet_pa;
int basebrawnet_px;
float basebraw_sampling_speed;
int basebraw_ad_channels;
int basebraw_rf_channels;
int basebraw_mode;
int basebraw_passband_direction;
float *fftx;
float *fftx_pwr;
TWOCHAN_POWER *fftx_xypower;
size_t *afc_handle;
float *afc_spectrum;
float *afc_lowres_spectrum;
float *afct_window;
float *ag_fitted_fq;
float *ag_mix1freq;
float *ag_freq;
float *ag_ston;
TWOCHAN_POWER *afc_xysum;
char *ag_background;
short int *ag_oldpix;
char *ag_stonbuf;
char *ag_srchbuf;
char *ag_lockbuf;

float mix1_lowest_fq;
float mix1_highest_fq;

D_COSIN_TABLE *d_mix1_table;

float *mix1_fqwin;
double *d_mix1_window;
double *d_mix1_cos2win;
double *d_mix1_sin2win;
double *d_mix1_fqwin;
float *mix1_fq_mid;
float *mix1_fq_start;
float *mix1_fq_curv;
float *mix1_fq_slope;
short int *mix1_eval_avgn;
float *mix1_eval_fq;
float *mix1_eval_sigpwr;
float *mix1_eval_noise;
float *mix1_fitted_fq;

float *spur_table;
short int *spur_table_mmx;
int *spur_location;
int *spur_flag;
float *spur_power;
float *spur_d0pha;
float *spur_d1pha;
float *spur_d2pha;
float *spur_avgd2;
float *spur_pol;
float *spur_spectra;
float *spur_freq;
float *sp_sig;
float *sp_pha;
float *sp_der;
float *sp_tmp;
float *spur_signal;
float *spur_ampl;
float *spur_noise;
int *spur_ind; 
float sp_c1, sp_c2, sp_c3;
int sp_avgnum;
int sp_numsub;
int no_of_spurs;
int spur_block;
int spurno;
int spurp0;
int spur_speknum;
float spur_max_d2;
float spur_minston;
float spur_freq_factor;
TWOCHAN_POWER spur_pxy[SPUR_WIDTH];
float sp_d0;
float sp_d1;
float sp_d2;
float spur_weiold;
float spur_weinew;
float spur_linefit;
int fftx_na;
int fftx_nc;
int fftx_nm;
int fftx_nx;
int fftx_ny;
int fftx_nf1;
int fftx_nf2;
int max_fftxn;
int fftxn_mask;
int fftx_size;
int ffts_na;
int ffts_nm;

int ag_first_xpixel;
int ag_xpixels;
int ag_size;
int ag_pa;
int ag_px;
int ag_ps;
int ag_y0;
int ag_y1;
int ag_height;
int ag_old_height;
int ag_old_fit_points;
int ag_shift_xpixels;
int ag_pf1;
int ag_pf2;

float ag_first_fq;
float ag_hz_per_pixel;

int afct_delay_points;
int old_afct_delay;
int afct_avgnum;
int afct_half_avgnum;
int max_afcf_points;
int max_afc_fit;
float afcf_search_range;
float afcf_max_drift;
float afc_minston;
int afc_speknum;
int afc_half_speknum;
int afc_ib;
int afc_ia;
float afc_drift_step;
int afc_graph_filled;
int afc_tpts;

int mix1p0;
int ag_mask;
unsigned char afc_old_cursor_color;
int initial_afct_avgnum;
int ag_ss;



float fftx_points_per_hz;
double hwfreq;
double old_hwfreq;
double mix1_selfreq[MAX_MIX1];
double old_mix1_selfreq;
int mix1_curx[MAX_MIX1];
int new_mix1_curx[MAX_MIX1];
int mix1_point[MAX_MIX1]; 
float mix1_phase_step[MAX_MIX1];
float mix1_phase[MAX_MIX1];
float mix1_phase_rot[MAX_MIX1];
float mix1_old_phase[MAX_MIX1];
int mix1_old_point[MAX_MIX1];
int mix1_status[MAX_MIX1];
float mix1_good_freq[MAX_MIX1];
float afc_maxval;
float afc_noise;
float afc_noisefac;
float afc_fq;
unsigned char afc_cursor_color;
float afc_slope;
float ag_floatypix;
int ag_fpar_y0;
int ag_fpar_ytop;
int ag_ston_y;
int ag_lock_y;
int ag_srch_y;
int ag_ston_x1;
int ag_ston_x2;
int ag_lock_x1;
int ag_lock_x2;
int ag_srch_x1;
int ag_srch_x2;


