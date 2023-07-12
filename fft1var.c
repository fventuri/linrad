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
#include "fft1def.h"

// Define setup info for each fft version.
//typedef struct {
//unsigned char window;
//unsigned char permute;
//unsigned char max_n;
//unsigned char mmx;
//unsigned char simd;
//unsigned char ocl;
//unsigned char real2complex;
//unsigned char parall_fft;    
//unsigned char doub;
//char *text;
//} FFT_SETUP_INFO;


FFT_SETUP_INFO fft_cntrl[MAX_FFT_VERSIONS]={
{4,0,15,0,1,0,0,2,0,         "Twin Radix 4 DIT SIMD"},            //0
{4,0,15,0,0,0,1,1,0,         "Twin Radix 4 DIT C real"},          //1
{2,2,14,0,0,0,0,1,0,         "Split radix DIT C real"},           //2
{4,0,15,0,0,0,1,2,0,         "Quad Radix 4 DIT C real"},          //3
{4,0,15,0,1,0,1,2,0,         "Quad Radix 4 DIT SIMD real"},       //4
{4,0,15,0,0,0,0,2,0,         "Twin Radix 4 DIT"},                 //5
{4,0,15,0,0,0,0,1,0,         "Radix 4 DIT C"},                    //6  
{1,1,15,0,0,0,0,1,0,         "Radix 2 DIF C"},                    //7
{1,1,15,0,0,0,0,1,0,         "Radix 2 DIF ASM"},                  //8
{1,1,15,0,0,0,0,1,0,         "Twin radix 2 DIF ASM"},             //9
{4,0,15,0,0,0,0,1,0,         "Twin Radix 4 DIT C"},               //10
{4,0,15,0,1,0,0,1,0,         "Twin Radix 4 DIT SIMD"},            //11
{4,0,15,1,0,0,0,1,0,         "Twin Radix 4 DIT MMX"},             //12
{4,0,15,0,0,0,0,1,0,         "Quad Radix 4 DIT C"},               //13
{4,0,15,1,0,0,0,1,0,         "Quad Radix 4 DIT MMX"},             //14
{4,0,15,0,0,0,0,1,0,         "Radix 2 DIF C"},                    //15
{4,0,15,0,0,0,0,1,0,         "Twin Radix 4 DIT C"},               //16
{4,0,15,1,0,0,0,1,0,         "Radix 4 DIT MMX"},                  //17
{1,0,15,0,0,GPU_OCL,0,1,0,   "clFFT with OpenCL"},                //18 
{1,0,15,0,0,GPU_CUDA,0,1,0,  "CUDA"},                             //19
{1,1,31,0,0,0,0,1,1,         "Double precision"}                  //20
                          }; 
// For fft1 there are 4 input modes as defined by the 
// value of ui.input_mode&(TWO_CHANNELS+IQ_DATA)/2
// Define the index to fft_cntrl for as many fft versions
// as are implemented in each case.
// *************
// permute = 1 => DIF, Permute complex data after transform
// permute = 2 => DIT, permute and rearrange before computing transform

int fft1_version[4][MAX_FFT1_VERNR]={
//        0   1   2   3   4   5   6   7  
         {2,  1,  3,  4, -1, -1, -1, -1},     // 1 chan normal audio    
         {2,  1,  3,  4, -1, -1, -1, -1},     // 2 chan normal audio    
         {7,  8,  6,  5,  0, 18, 19, -1},     // 1 chan direct conversion (IQ)     
         {7,  8,  9,  6, 10, 11, 20, -1}};    // 2 chan direct conversion (IQ)     

int fft1_back_version[2][MAX_FFT1_BCKVERNR]={
//        0   1   2    3    
        {10, 12, -1,  -1},      // 1 chan     
        {13, 14, -1 , -1}};     // 2 chan     

int fft2_version[2][MAX_FFT2_VERNR]={
//        0   1   2   3    
        {15,  6, 17, -1 },     // 1 chan     
        {15, 16, 12, -1 }};    // 2 chan      

double fft1_show_time;


int timf1_mute;
int timf1_mute_counter;
int fft1_liminfo_cnt;
int fft2_liminfo_cnt;
int fft1_correlation_flag;
short int *rxin_isho;
int *rxin_int;
char *rxin_char;

float *reg_noise;
float *reg_min;
float *reg_ston;
int *reg_first_point;
int *reg_length;

COSIN_TABLE *fft1tab;
D_COSIN_TABLE *d_fft1tab;
COSIN_TABLE *fft1_backtab;
unsigned short int *fft1_permute;
unsigned int *fft1_bigpermute;
float *fft1_corrsum;
float *fft1_slowcorr;
double *fft1_slowcorr_tot;
int correlation_reset_flag;
int slowcorr_tot_avgnum;
float wg_corrtot_yfac_power;
float wg_corr_yfac_power;
int latest_wg_spectrum;
int latest_wg_spectrum_shown;
float *fft1_window;
double *d_fft1_window;
float *fft1_sumsq;
float *fft1_slowsum;
float *fft1_power;
float *fft1_powersum;
TWOCHAN_POWER *fft1_xypower;
TWOCHAN_POWER *fft1_xysum;
float *fftw_tmp;
float *timf2_tmp;
float *fftf_tmp;
float *fftt_tmp;
char *rawsave_tmp;
char *rawsave_tmp_net;
char *rawsave_tmp_disk;
float *fft1_foldcorr;
size_t *fft1_handle;
char  *fft1_char;
short int *fft1_short_int;
int   *fft1_int;
float *fft1_float;
short int *fft1_spectrum;
short int *fft1_corr_spectrum;
short int *fft1_corr_spectrum_tot;
float *fft1_filtercorr;
float *fft1_desired;
float *liminfo;
char *fft1_netsend_buffer;
unsigned char *liminfo_wait; 
float *liminfo_group_min;
short int *fft1_split_shi;
float *fft1_split_float;
MMX_COSIN_TABLE *fft1_mmxcosin;
float *fft1_inverted_window;
short int *fft1_inverted_mmxwin;
unsigned short int *fft1_back_scramble;
short int *fft1_backbuffer;
char *timf1_char;
float *timf1_float;
int *timf1_int;
short int *timf1_short_int;
unsigned int *timf2_pwr_int;
float *timf2_pwr_float;
short int *timf2pix;
int yieldflag_wdsp_fft1;
int yieldflag_timf2_fft1;
int ui_converter_direction;
double converter_offset_mhz;



float *wg_waterf_yfac;
short int *wg_waterf;
int wg_waterf_size;
int wg_waterf_ptr;
int local_wg_waterf_ptr;
WATERF_TIMES *wg_waterf_times;
int max_wg_waterf_times;
int wg_waterf_ptr2;
int local_wg_yborder;
int fft1_waterfall_flag;
int change_fft1_flag;
int fft1_use_gpu;

int wg_refresh_flag;
double wg_refresh_time;

int fft1_costab_size,fft1_permute_size,fft1_window_size;
int fft1_tmp_bytes;
int timf2pow_size;
int timf2pow_mask;
int timf3_totsiz;
float fft2_interleave_ratio, fftx_blocktime;

double diskread_block_counter;
double file_start_block;
double file_stop_block;

int timf1_indicator_block;
int fft1_indicator_block;
int timf2_indicator_block;
int fft2n_indicator_block;
int timf3_indicator_block;
int fft3_indicator_block;
int baseb_indicator_block;
int daout_indicator_block;
int dabuf_indicator_block;
int indicator_first_pixel;
int indicator_ypix;
int fft1corr_reset_flag;
int fft1_n;             
int fft1_size;          
int fft1_block;
int fft1_mulblock;
int fft1_muln;
int fft1_blockbytes;
int fft1_interleave_points;
float fft1_interleave_ratio;
int fft1_new_points;
float fft1_blocktime;
int fft1_mask;  
int fft1_bytes;      
int fft1_bufmask;       
int fft1net_pa;
int fft1net_px;
int fft1net_size;
int fft1net_mask;
int fft1_pa;
int fft1_pb;
int fft1_px;
int fft1_na;
int fft1_nb;
int fft1_nc;
int fft1_nm;
int fft1_nx;
int fft1afc_flag;
double wg_waterf_zero_time;
int no_of_fft1b;

int ad_wts;
float fft1_wtb;
float ad_wttim;
float fft1_wttim;  
float timf2_wtb;
float timf2_wttim;  
int fft1_sumsq_mask;
int fft1_sumsq_bufsize;
int fft1_sumsq_counter;
int fft1_sumsq_pa;
int radar_fft1_sumsq_pa;
int fft1_sumsq_pwg;
int fft1mode;
int fft1_calibrate_flag;
int max_fft1_sumsq;
int input_counter;
int fft1_sumsq_recalc;
int fft1_first_point;
int fft1_first_sym_point;
int fft1_first_sumsq_point;
int fft1_last_point;
int fft1_last_inband;
int fft1_first_inband;
int fft1_last_sym_point;
int fft1_last_sumsq_point;
int fft1back_lowgain_n;
int max_fft1n;
int fft1n_mask;
int fft1_sumsq_tot;
int fft1_direction;
int fft1_filtercorr_direction;
float fft1_bandwidth;
float fft1_hz_per_point;
float fft1_filtercorr_start;
int liminfo_groups;
int liminfo_group_points;
int fft1_lowlevel_points;
float fft1_lowlevel_fraction;
float liminfo_amplitude_factor;
float fft1_desired_totsum;
float fft1_noise_floor;
int ad_bufmargin;
int wg_xpix1;
int wg_waterfall_blank_points;

float txtest_decayfac2;
float txtest_decayfac1;
int txtest_last_xpix;
int txtest_peak_redraw;
float txtest_saturated;

short int *txtest_ypeak;
short int *txtest_ypeak_decay;
short int *txtest_yavg;
float *txtest_power;
float *txtest_powersum;
short int *txtest_old_yavg;
short int *txtest_old_ypeak;
short int *fft1_old_spectrum;
short int *txtest_old_ypeak_decay;
float *txtest_peak_power;
float *txtest_peak_power_decay;
// Variables related to saving raw data on disk
int save_init_flag;
FILE *save_wr_file;
FILE *save_rd_file;
int diskwrite_flag;
int diskread_flag;
int save_rw_bytes;

float timf1_sampling_speed;
int timf1_bytes;
int timf1_bytemask;
int timf1_blockbytes;
int timf1_usebytes;
int rx_read_bytes;
int timf1p_pa;
int timf1p_px;
int timf1p_pb;
int timf1p_pc_net;
int timf1p_pc_disk;
int timf1p_sdr;

short int *timf2_shi;
float *timf2_float;
float *timf2_blockpower;
int timf2_blockpower_pa;
int timf2_blockpower_px;
int timf2_blockpower_size;
int timf2_blockpower_mask;
int timf2_pa;
int timf2_pb;
int timf2_pc;
int timf2_pn1;
int timf2_pn2;
int timf2_pt;
int timf2p_fit;
int timf2_px;
int timf2_mask;
int timf2_neg;
int timf2_size;
int timf2_blockpower_block;
int timf2_input_block;
int timf2_output_block;
int timf2_oscilloscope_counter;
int timf2_oscilloscope_interval;
int timf2_oscilloscope_maxpoint;
int timf2_ovfl;
int timf2_fitted_pulses;
int timf2_cleared_points;
int timf2_blanker_points;
int rx_ad_maxamp[MAX_AD_CHANNELS];
int timf2_maxamp[2*MAX_RX_CHANNELS];
int fft1_maxamp[2*MAX_RX_CHANNELS];
int timf2_y0[16];
int timf2_ymax[16];
int timf2_ymin[16];
int timf2_hg_x[2];
int timf2_hg_y[2];
float timf2_hg_xfac;
float timf2_hg_xlog;
int timf2_hg_xmin;
int timf2_hg_xmax;
int timf2_hg_yh;
int timf2_hg_stux;
int timf2_hg_clex;
int timf2_hg_qex;
int overrun_limit;

float timf2_oscilloscope_maxval_float;
float timf2_oscilloscope_powermax_float;
unsigned int timf2_oscilloscope_maxval_uint;
unsigned int timf2_oscilloscope_powermax_uint;

int txtest_no_of_segs;
float txtest_pixinc;
int txtest_pntinc;
int txtest_first_xpix;
int txtest_spek_no;
int txtest_spek_p0;
int txtest_first_point;
int txtest_show_p0;
float txtest_yfac;

int wg_first_point;
int wg_last_point;

