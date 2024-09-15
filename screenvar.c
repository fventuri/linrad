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
#include "screendef.h"

unsigned int sc[MAX_SC];
unsigned int sd[MAX_SC];
int screen_loop_counter;
int screen_loop_counter_max;
double phasing_time;


BUTTONS wgbutt[MAX_WGBUTT];
BUTTONS hgbutt[MAX_HGBUTT];
BUTTONS bgbutt[MAX_BGBUTT];
BUTTONS agbutt[MAX_AGBUTT];
BUTTONS pgbutt[MAX_PGBUTT];
BUTTONS cgbutt[MAX_CGBUTT];
BUTTONS egbutt[MAX_EGBUTT];
BUTTONS fgbutt[MAX_FGBUTT];
BUTTONS mgbutt[MAX_MGBUTT];
BUTTONS tgbutt[MAX_TGBUTT];
BUTTONS rgbutt[MAX_RGBUTT];
BUTTONS sgbutt[MAX_SGBUTT];
BUTTONS xgbutt[MAX_XGBUTT];
BUTTONS vgbutt[MAX_VGBUTT];
BUTTONS vgfbutt[MAX_VGFBUTT];

AG_PARMS ag;
BG_PARMS bg;
CG_PARMS cg;
EG_PARMS eg;
FG_PARMS fg;
HG_PARMS hg;
MG_PARMS mg;
PG_PARMS pg;
PG_PARMS dpg;
RG_PARMS rg;
SG_PARMS sg;
TG_PARMS tg;
VG_PARMS vg;
VGF_PARMS vgf;
WG_PARMS wg;
XG_PARMS xg; 
GPU_PARMS gpu;
NET_PARMS net;

SCREEN_OBJECT scro[MAX_SCRO];

WATERF_TIMES *bg_waterf_times;


ROUTINE current_mouse_activity;

char numinput_txt[MAX_TEXTPAR_CHARS+1];

unsigned char button_color;
int no_of_scro;

int wg_flag;
int hg_flag;
int ag_flag;
int bg_flag;
int pg_flag;
int cg_flag;
int mg_flag;
int eg_flag;
int fg_flag;
int tg_flag;
int rg_flag;
int sg_flag;
int xg_flag;
int vg_flag;
int vgf_flag;

int wg_fft_avg2num;
int wg_first_xpixel;
int wg_last_xpixel;
float wg_yfac_power;
float wg_yfac_log;
float wg_hz_per_pixel;
float wg_first_frequency;
double frequency_scale_offset;
double frequency_scale_offset_hz;
double rx_passband_center_mhz;

int wg_xpixels;
float wg_db_per_pixel;
int wg_timestamp_counter;
int waterfall_yield_interval;

int bg_flatpoints;
int bg_curvpoints;
int bg_6db_points;
float bg_noise_bw;
int bg_60db_points;
int bg_120db_points;
int bg_first_xpixel;
int bg_last_xpixel;
int bg_first_xpoint;
int bg_xpoints;
int bg_xpixels;
int bg_timestamp_counter;
int bg_ymax;
int bg_y3;
int bg_y4;
int bg_y2;
int bg_y1;
int bg_y0;
int bg_avg_counter;
int bg_yborder_max;
int bg_yborder_min;
int bg_vol_x1;
int bg_vol_x2;
float bg_hz_per_pixel;

int hg_size;
int hg_first_point;
int hg_last_point;
int hg_first_xpixel;
int hg_last_xpixel;
int hg_xpix1;
int hg_first_fq;
int hg_curx;
int hg_center;
int hg_cury0;
int hg_cury1;
int hg_cury2;
int hg_stonbars_ytop;
int hg_y0;
int hg_redraw_counter;
int hg_ymax;
float hg_db_per_pixel;
float hg_hz_per_pixel;
float hg_yfac_log;
float hg_yfac_power;
int hg_powersum_recalc;
int hg_recalc_pointer;
int hg_ston1_y;
int hg_ston2_y;
int hg_ston1_yold;
int hg_ston2_yold;
int hg_ston_y0;
int hg_ston1_x1;
int hg_ston1_x2;
int hg_ston2_x1;
int hg_ston2_x2;
int afc_curx;


int pg_x0;
int pg_y0;
int pg_oldx;
int pg_oldy;
float pg_pol_angle;
float pg_b;
double show_pol_time;
double update_meter_time;

int cg_x0;
int cg_y0;
int cg_y1;
int cg_chirpx;
int cg_chirpy;
int cg_oldx;
int cg_oldy;

int eg_hsiz;
int eg_vsiz;
int eg_oldx;
int eg_oldy;
int eme_active_flag;

int fg_oldx;
int fg_oldy;
int fg_yborder;

int tg_oldx;
int tg_oldy;
int tg_yborder;
int tg_old_band;

size_t fftx_totmem;
size_t baseband_totmem;
size_t afc_totmem;
size_t hires_totmem;
size_t fft3_totmem;
size_t radar_totmem;
size_t siganal_totmem;
size_t allan_totmem;

int bg_waterf_sum_counter;
float *bg_waterf_sum;
int bg_waterf_yinc;
int bg_waterf_y1;
int bg_waterf_y2;
int bg_waterf_y;
float bg_waterf_cfac;
float bg_waterf_czer;
int bg_waterf_block;
int bg_waterf_lines;
float bg_waterf_yfac;

unsigned char *wg_background;
unsigned char *bg_background;
unsigned char *hg_background;
char *hg_stonbuf;
short int *hg_spectrum;
int mouse_active_flag;
int numinput_flag;
int numinput_xdiff;
int numinput_ydiff;
int numinput_xpix;
int numinput_ypix;
int numinput_curpos;
int numinput_int_data;
int numinput_chars;
float numinput_float_data;
double numinput_double_data;
double cursor_blink_time;
float moon_az;
float moon_el;

size_t *hires_handle;
float *wg_waterf_sum;
int wg_waterf_sum_counter;
int wg_waterf_yinc;
int wg_waterf_y1;
int wg_waterf_y2;
int wg_waterf_y;
float wg_waterf_cfac;
float wg_waterf_czer;
int wg_waterf_block;
int wg_waterf_lines;
int screen_last_line;

float sg_hz_per_pixel;

short int *bg_waterf;
int max_bg_waterf_times;
int bg_waterf_ptr;
int bg_waterf_ptr2;
int bg_waterf_size;
int local_bg_waterf_ptr;
int local_bg_yborder;
int s_meter_avg_filled_flag;

char *sg_xscales[2]={"lin","log"};
char sg_modes[NO_SG_MODES]={'B','P','A','C'}; 
char ch2_phase_symbol[MAX_CH2_PHASES]={'+','-','c'};
char vg_modes[2]={'A','H'};
char vg_clears[2]={'C','S'};

char *graphtype_names[MAX_GRAPHTYPES]={"ag",
                                       "wg",
                                       "bg",
                                       "hg",
                                       "pg",
                                       "cg",
                                       "eg",
                                       "fg",
                                       "mg",
                                       "tg",
                                       "rg",
                                       "sg",
                                       "xg",
                                       "vg",
                                       "vgf",
                                       "gpu"};
                                       
char *graphtype_parptr[MAX_GRAPHTYPES]={(void*)(&ag),
                                       (void*)(&wg),
                                       (void*)(&bg),
                                       (void*)(&hg),
 //Use dpg for read/write to keep defaults while moving window.
                                       (void*)(&dpg),
                                       (void*)(&cg),
                                       (void*)(&eg),
                                       (void*)(&fg),
                                       (void*)(&mg),
                                       (void*)(&tg),
                                       (void*)(&rg),
                                       (void*)(&sg),
                                       (void*)(&xg),
                                       (void*)(&vg),
                                       (void*)(&vgf),
                                       (void*)(&gpu)};
                                       
int graphtype_max_intpar[MAX_GRAPHTYPES]={MAX_AG_INTPAR,
                                          MAX_WG_INTPAR,
                                          MAX_BG_INTPAR,
                                          MAX_HG_INTPAR,
                                          MAX_PG_INTPAR,
                                          MAX_CG_INTPAR,
                                          MAX_EG_INTPAR,
                                          MAX_FG_INTPAR,
                                          MAX_MG_INTPAR,
                                          MAX_TG_INTPAR,
                                          MAX_RG_INTPAR,
                                          MAX_SG_INTPAR,
                                          MAX_XG_INTPAR,
                                          MAX_VG_INTPAR,
                                          MAX_VGF_INTPAR,
                                          MAX_GPU_INTPAR};

char **graphtype_partexts_int[MAX_GRAPHTYPES]={ag_intpar_text,
                                               wg_intpar_text,
                                               bg_intpar_text,
                                               hg_intpar_text,
                                               pg_intpar_text,
                                               cg_intpar_text,
                                               eg_intpar_text,
                                               fg_intpar_text,
                                               mg_intpar_text,
                                               tg_intpar_text,
                                               rg_intpar_text,
                                               sg_intpar_text,
                                               xg_intpar_text,
                                               vg_intpar_text,
                                               vgf_intpar_text,
                                               ocl_intpar_text};

// Store number of float with reversed sign if the
// float variables are double precision.
int graphtype_max_floatpar[MAX_GRAPHTYPES]={MAX_AG_FLOATPAR,
                                          MAX_WG_FLOATPAR,
                                          MAX_BG_FLOATPAR,
                                          MAX_HG_FLOATPAR,
                                          MAX_PG_FLOATPAR,
                                          MAX_CG_FLOATPAR,
                                          MAX_EG_FLOATPAR,
                                          -MAX_FG_FLOATPAR,
                                          MAX_MG_FLOATPAR,
                                          -MAX_TG_FLOATPAR,
                                          MAX_RG_FLOATPAR,
                                          MAX_SG_FLOATPAR,
                                          MAX_XG_FLOATPAR,
                                          MAX_VG_FLOATPAR,
                                          MAX_VGF_FLOATPAR,
                                          MAX_GPU_FLOATPAR};

char **graphtype_partexts_float[MAX_GRAPHTYPES]={ag_floatpar_text,
                                               wg_floatpar_text,
                                               bg_floatpar_text,
                                               hg_floatpar_text,
                                               pg_floatpar_text,
                                               cg_floatpar_text,
                                               eg_floatpar_text,
                                               fg_floatpar_text,
                                               mg_floatpar_text,
                                               tg_floatpar_text,
                                               rg_floatpar_text,
                                               sg_floatpar_text,
                                               xg_floatpar_text,
                                               vg_floatpar_text,
                                               vgf_floatpar_text,
                                               ocl_floatpar_text};
