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
#include "sigdef.h"

// Repeat defines so we may notice mistakes.
#define CW_WORDSEP 255
#define CW_SPACE 254
#define CW_DOT 253
#define CW_DASH 252

int cw_item_len[256-CW_DASH]={4,2,2,4};
char *cw_item_text[256-CW_DASH]={"WORDSEP","SPACE","DOT","DASH"};

// Store character values. 
// 243 means an error. The code is not used and no used code can
// be made by adding dots or dashes.
// 242 is an incomplete code. At least one dot or dash must be
// appended to get a valid code.

unsigned char morsascii1[2]={
 'E',//.
 'T'};//_
unsigned char morsascii2[4]={
 'I',//..
 'A',//._
 'N',//_.
 'M'};//__
unsigned char morsascii3[8]={
 'S',//...
 'U',//.._
 'R',//._.
 'W',//.__
 'D',//_..
 'K',//_._
 'G',//__.
'O'};//___
unsigned char morsascii4[16]={
 'H',//....
 'V',//..._
 'F',//.._.
 198,//..__
 'L',//._..
 196,//._._
 'P',//.__.
 'J',//.___
 'B',//_...
 'X',//_.._
 'C',//_._.
 'Y',//_.__
 'Z',//__..
 'Q',//__._
 246,//___.
245};//____
unsigned char morsascii5[32]={
 '5',//.....
 '4',//...._
 247,//..._.
 '3',//...__
 243,//.._..
 243,//.._._
 '!',//..__.
 '2',//..___
 243,//._...
 242,//._.._
 199,//._._.
 243,//._.__
 243,//.__..
 197,//.__._
 243,//.___.
 '1',//.____
 '6',//_....
 '=',//_..._
 '/',//_.._.
 243,//_..__
 243,//_._..
 242,//_._._
 '(',//_.__.
 243,//_.___
 '7',//__...
 242,//__.._
 243,//__._.
 243,//__.__
 '8',//___..
 '9',//____.
'0'};//_____
unsigned char morsascii6[64]={
 243,//......
 243,//....._
 243,//...._.
 243,//....__
 243,//..._..
 200,//..._._
 243,//...__.
 243,//...___
 243,//.._...
 243,//.._.._
 243,//.._._.
 243,//.._.__
 '?',//..__..
 243,//..__._
 243,//..___.
 243,//..____
 243,//._....
 243,//._..._
 '"',//._.._.
 243,//._..__
 243,//._._..
 '.',//._._._
 243,//._.__.
 243,//._.___
 243,//.__...
 243,//.__.._
 243,//.__._.
 243,//.__.__
 243,//.___..
 243,//.____.
 243,//._____
 243,//_.....
 '-',//_...._
 243,//_..._.
 243,//_...__
 243,//_.._..
 243,//_.._._
 243,//_..__.
 243,//_..___
 243,//_._...
 243,//_._.._
 ';',//_._._.
 243,//_._.__
 243,//_.__..
 ')',//_.__._
 243,//_.___.
 243,//_.____
 243,//__....
 243,//__..._
 243,//__.._.
 ',',//__..__
 243,//__._..
 243,//__._._
 243,//__.__.
 243,//__.___
 ':',//___...
 243,//___.._
 243,//___._.
 243,//___.__
 243,//____..
 243,//_____.
243};//______

double baseb_errmsg_time;
char *baseb_errmsg;
char fft3_level[12]="           ";
char fft3_skip[32]="            ";

unsigned char *morsascii[6]={morsascii1, morsascii2, morsascii3, 
               morsascii4, morsascii5, morsascii6};               

int bg_filter_points;
int bg_carrfilter_points;

float *squelch_info;
double squelch_turnon_time;
int squelch_on;
int bg_waterfall_blank_points;

int basebraw_test_cnt1;
int basebraw_test_cnt2;
int basebraw_test_cnt3;
int baseb_min_block;
int baseb_output_block;
int corr_afc_count;

float keying_spectrum_pos[KEYING_SPECTRUM_MAX];
float keying_spectrum_ampl[KEYING_SPECTRUM_MAX];
int keying_spectrum_ptr;
int keying_spectrum_max;
int keying_spectrum_cnt;
float keying_spectrum_f1;
int bg_no_of_notches;
int bg_current_notch;
int bg_notch_pos[MAX_BG_NOTCHES];
int bg_notch_width[MAX_BG_NOTCHES];

int cg_oscw;
int cg_osc_points;
float rds_f1;
float rds_f2;
float rds_phase;
float rds_power;

int min_daout_samps;

float fm0_ph1;
float fm0_ph2;
float fm1_ph1;
float fm1_ph2;
float fm2_ph1;
float fm2_ph2;
float *fm_lowpass;
int fmfil_rise;
int fmfil_len;
float fm_lowpass_ampsum;
float *fm_audiofil_fir;
int fm_audiofil_size;
int fm_audiofil_n;
int fm_audiofil_points;
float *fmfil70_fir;
int fmfil70_size;
int fmfil70_n;
int fmfil70_points;
int fm1_resample_ratio;
int fm1_pa;
int fm1_px;
float *fm1_all;
float fm1_sampling_speed;
int fm1_size;
int fm1_mask;
int bg_agc_hang_pts;
int output_mode_x;
int output_mode_y;
float *fmfil55_fir;
int fmfil55_size;
int fmfil55_n;
int fmfil55_points;
float *fmfil_rds_fir;
int fmfil_rds_size;
int fmfil_rds_n;
int fmfil_rds_points;
float *fmfil_fir;
int fmfil_size;
int fmfil_n;
int fmfil_points;
int fm_n;
int fm_size;  
unsigned short int fm_permute[32];
COSIN_TABLE fm_tab[16];
float fm_win[32];

size_t *baseband_handle;
float *baseb_out;
float *baseb;
double *d_baseb;
float *baseb_raw;

float *baseb_fm_phase;
float *baseb_fm_sumchan;
float *baseb_fm_diffchan;
float *baseb_fm_rdsraw;
float *baseb_fm_composite;

float *baseb_fm_demod;
float *baseb_fm_demod_low;
float *baseb_fm_demod_high;
float *fm_pilot_tone;
float *baseb_fm_pil2;
float *baseb_fm_pil2det;
float *baseb_fm_pil3;
float *baseb_fm_pil3det;
int fm_pilot_size;

float *baseb_wb_raw;
float *baseb_raw_orthog;
float *baseb_carrier;
double *d_baseb_carrier;
float *baseb_carrier_ampl;
float *baseb_totpwr;
float *baseb_envelope;
float *baseb_upthreshold;
float *baseb_threshold;
float *baseb_fit;
float *baseb_tmp;
float *baseb_agc_level;
float *baseb_agc_det;
short int *baseb_ramp;
float *baseb_sho1;
float *baseb_sho2;
float carrfil_weight;
float bgfil_weight;
double d_carrfil_weight;
double d_bgfil_weight;
float *baseb_clock;

size_t *allan_handle;
int vg_siz;
double *vg_phase;
int *vg_tau;
double *vg_asum;
double *vg_acorrsum;
double *vg_hsum;
double *vg_hcorrsum;
double *vg_asum_ampl;
double *vg_acorrsum_ampl;
double *vg_hsum_ampl;
double *vg_hcorrsum_ampl;
double vg_reset_time;

int *vg_start_pointer;
int *vg_n;
int *vg_sumno;
int vg_no_of_tau;
int vg_first_xpixel;
int vg_last_xpixel;
float vg_xpix_per_decade;
float vg_ypix_per_decade;
int vg_yt;
int vg_yb;
double vg_basebfreq1;
double vg_basebfreq2;
double vg_interchannel_phase;
short int *vg_y1pix;
short int *vg_y2pix;
short int *vg_ycpix;
short int *vg_decimal_xpixel;
short int *vg_decimal_ypixel;

int vgf_freq_xpix;
int vgf_pa;
int vgf_px;
int vgf_size;
int vgf_mask;
double vgf_average_freq1;
double vgf_average_freq2;
int vgf_first_xpixel;
int vgf_last_xpixel;
int vgf_xpixels;
int vgf_yt;
int vgf_yb;
float *vgf_freq;
float *vgf_phase;
double vgf_mid_freq;
double vgf_mid_phase;
int vgf_tau;
int vgf_n;
int vgf_center_traces;

size_t *siganal_handle;
int sg_pa;
int sg_px;
int sg_interleave_points;
int sg_pixels_per_point;
int sg_points_per_pixel;
int sg_siz;
int sg_corrnum;
int sg_numpow;
int sg_y0;
int sg_mode3_ymid;
int sg_mode3_ypix;
int sg_first_xpixel;
int sg_last_xpixel;
float sg_first_logfreq;
float sg_last_logfreq;
int sg_first_logpoint;
int sg_last_logpoint;
int sg_first_logpixel;
float sg_logoffset;
float sg_log_xfac;
float sg_log_scale;
float sg_log_zerfreq;
float sg_log_intfreq;

int sg_ytop2;
double sg_reset_time;
int sg_valid;
int corrpow_cnt;
int corrpow_reset;

int fftn_tmp_size;
double *sg_fft;
float *sg_pwr;
double *sg_pwrsum;
double *sg_corrsum;
double *sg_anpn_corr;
double *sg_window;
D_COSIN_TABLE *sg_tab;
unsigned int *sg_permute;
double *sg_tmp;

short int *sg_ancspectrum;
short int *sg_pncspectrum;
short int *sg_anpncorr_ispectrum;
short int *sg_anpncorr_qspectrum;
short int *sg_oldancspectrum;
short int *sg_oldpncspectrum;
short int *sg_oldanpncorr_ispectrum;
short int *sg_oldanpncorr_qspectrum;
short int *sg_an1spectrum;
short int *sg_an2spectrum;
short int *sg_pn1spectrum;
short int *sg_pn2spectrum;

float reg_dot_power[5];
float reg_dot_re[5];
float reg_dot_im[5];
float reg_dash_power;
float reg_dash_re;
float reg_dash_im;
int dot_siz, dot_sizhalf, dash_siz, dash_sizhalf;
int cg_old_y1;
int cg_old_y2;
int cg_old_x1;
int cg_old_x2;

FILE *mg_meter_file;
int mg_meter_file_cnt;
float mg_meter_file_sum;
unsigned int mg_meter_file_tot;
double mg_meter_start_time;

int keying_spectrum_size;
float *keying_spectrum;
char *mg_behind_meter;
short int *mg_rms_ypix;
short int *mg_peak_ypix;
float *mg_rms_meter;
float *mg_peak_meter;
short int mg_first_xpixel;
short int mg_last_xpixel;
short int mg_pa;
short int mg_px;
short int mg_size;
short int mg_mask;
short int mgw_p;
short int mg_ymin;
short int mg_ymax;
int mg_stonavg_mouse;
int mg_stonavg_x1;
char mg_clear_flag;
int bg_update_filter;
double mg_dbscale_start;
double mg_scale_offset;
double mg_dbstep;
float mg_midscale;
int mg_valid;
int mg_bar;
char *mg_barbuf;
short int mg_bar_x1;
short int mg_bar_x2;
short int mg_y0;
short int mg_bar_y;

int cg_wave_start;
float cg_wave_midpoint;
float cg_wave_fit;
float cg_wave_coh_re;
float cg_wave_coh_im;
float cg_wave_raw_re;
float cg_wave_raw_im;
float cg_wave_dat;
float cg_wave_err;
float *basblock_maxpower;
float *basblock_avgpower;
char *daout;
float *cg_map;
short int *cg_traces;
float *mix2_tmp;
float *carr_tmp;
double *d_carr_tmp;
float *mi2_tmp;
double d_mi2_tmp;
float *mix2_pwr;
D_COSIN_TABLE *d_mix2_table;
float *fftn_tmp;
double *d_fftn_tmp;
MORSE_DECODE_DATA *cw;
float *cw_carrier_window;
char *bg_volbuf;
float *bg_binshape;
double *d_bg_binshape;
double *bg_ytmp;
float *dash_waveform;

float *dash_wb_waveform;
float *dot_wb_waveform;
int dash_wb_ia;
int dash_wb_ib;
int dot_wb_ia;
int dot_wb_ib;


int cg_yzer[CG_MAXTRACE];
int cw_ptr;
int cw_detect_flag;
int max_cwdat;
int no_of_cwdat;
int correlate_no_of_cwdat;
int cw_decoded_chars;
float baseband_noise_level;
float carrier_noise_level;



double da_block_counter;
int poleval_pointer;
float baseband_sampling_speed;
int timf1_to_baseband_speed_ratio;
int cg_no_of_traces;
int cg_lsep;
unsigned char cg_color[CG_MAXTRACE];
int cg_osc_ptr;
int cg_osc_offset;
int cg_osc_offset_inc;
int cg_osc_shift_flag;
int cg_max_trlevel;
float cg_code_unit;
float cg_decay_factor;
int cw_waveform_max;
unsigned int cw_avg_points;
int cg_update_interval;
int cg_update_count;
float cw_stoninv;

int baseband_size;
int baseband_mask;
int baseband_neg;
int baseband_sizhalf;
int baseb_pa;
int baseb_pb;
int baseb_pc;
int baseb_pd;
int baseb_pe;
int baseb_pf;
int baseb_ps;
int baseb_pm;
int baseb_pn;
int baseb_px;
int baseb_py;
float baseb_fx;
int daout_size;
int new_baseb_flag;
float da_resample_ratio;
float new_da_resample_ratio;
int daout_upsamp;
int new_daout_upsamp;
int daout_upsamp_n;
int daout_pa;
int daout_px;
int daout_py;
int daout_bufmask;
int flat_xpixel;
int curv_xpixel;
int bfo_xpixel;
int bfo10_xpixel;
int bfo100_xpixel;
double rx_daout_cos;
double rx_daout_sin;
double rx_daout_phstep_sin;
double rx_daout_phstep_cos;

float daout_gain;
int daout_gain_y;
int bfo_flag;
int baseb_channels;
float baseb_wts;
float da_wttim;  
float baseb_wttim;
float db_wttim;
float da_wait_time;
POLCOFF poleval_data[POLEVAL_SIZE];
int bg_filtershift_points;
int bg_expand;
int bg_coherent;
int bg_delay;
int bg_twopol;
int bg_ypixels;
float bg_expand_a;
float bg_expand_b;
float bg_amplimit;
float bg_agc_amplimit;
float bg_maxamp;
int bg_amp_indicator_x;
int bg_amp_indicator_y;
int da_ch2_sign;
int cg_size;
int dash_pts;
float dash_sumsq;
float cwbit_pts;
float refpwr;
int basblock_size;
int basblock_mask;
int basblock_hold_points;
int basblock_pa;
float s_meter_peak_hold;
float s_meter_fast_attack_slow_decay;
float s_meter_average_power;
float cg_maxavg;
float baseband_pwrfac;
float rx_agc_factor1;
float rx_agc_factor2;
float rx_agc_sumpow1;
float rx_agc_sumpow2;
float rx_agc_sumpow3;
float rx_agc_sumpow4;
float rx_agc_fastsum1;
float rx_agc_fastsum2;
float rx_agc_fast_factor1;
float rx_agc_fast_factor2;
float agc_attack_factor1;
float agc_attack_factor2;
float agc_release_factor;
float am_dclevel1;
float am_dclevel2;
float am_dclevel_factor1;
float am_dclevel_factor2;
int s_meter_avgnum;
float s_meter_avg;
