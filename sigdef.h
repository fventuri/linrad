
#define MAX_FM_FFTSIZE 32
#define MAX_FM_FFTN 5

#define CWDETECT_CLEARED 0
#define CWDETECT_SEARCH_SPEED 1
#define CWDETECT_ERROR 4

#define CWDETECT_WAVEFORM_ESTABLISHED 5
#define CWDETECT_SOME_PARTS_FITTED 6
#define CWDETECT_LIMITS_FOUND 10 //test for flag>=
#define CWDETECT_REGION_INITIATED 11
#define CWDETECT_REGION_WAVEFORM_OK 20
#define CWDETECT_SOME_ASCII_FITTED 21
#define CWDETECT_DEBUG_STOP 99
#define CWDETECT_DEBUG_IDLE 100

// We evaluate the noise floor from narrow filters outside
// the baseband. Define the number of such filters to use on each
// side, the number of points for each filter and the separation between
// the filters.
#define NOISE_FILTERS 2
#define NOISE_POINTS 5
#define NOISE_SEP 2

// Parameter for colour scale of cw speed statistcdisplay
#define CG_STATISTICS_F1 1.0
// Parameter for the total time to average statistics over.
#define CG_STATISTICS_F3 10
#define CG_STATISTICS_F2 (CG_STATISTICS_F3/CG_STATISTICS_F1)

// Max length of Morse decoding oscilloscope traces.
#define MAX_CG_OSCW 768


// Structure for Morse decoding data.
typedef struct {
unsigned char type;    // Part type (CW_DOT, CW_SPACE,....A,B,C,...)
unsigned char unkn;    // Number of unknown states before this part
unsigned int len;      // Length of character in states 
float midpoint;
float sep;
float coh_re;
float coh_im;
float raw_re;
float raw_im;
float tmp;
}MORSE_DECODE_DATA;

// Defines for MORSE_DECODE_DATA.type (cw.type)
// Note that CW_DASH must have the lowest value.
// The data in cw_item_len[255-CW_DASH]; must agree with these definitions!

#define CW_WORDSEP 255
#define CW_SPACE 254
#define CW_DOT 253
#define CW_DASH 252


// The number of dots and dashes we can use in the detect routine
// is computed from the maximum number of dots the buffer can hold
// multiplied by this factor
#define MAX_CW_PARTS 0.25

// Parameters that define detect accuracy.
#define DASH_DETECT_LIMIT 0.82
#define DASH_MISFIT_LIMIT 0.7
#define DOT_DETECT_LIMIT 0.88
#define DOT_MISFIT_LIMIT 0.75
// The error in the detect waveform is given by the S/N ratio.
// This is the limit at which we decide that the keying speed is 
// found accurately enough so we can switch from looking for
// the keying speed to actually start to look for code parts.
#define CW_WAVEFORM_MINSTON 8




// Defines for the Morse detect oscilloscope.
#define COH_GRAPH_ACTIVE -1000


#define POLEVAL_SIZE 32
typedef struct {
float a2;
float b2;
float re;
float im;
} POLCOFF;

#define KEYING_SPECTRUM_MAX 4

// for the meter fraph
#define DB2DBM 106
void mg_compute_stonavg(void);


extern int baseb_min_block;
extern int baseb_output_block;
#define MAX_CORR_AFC_COUNT 15
#define MAX_SG_INHIBIT_COUNT 7
extern int corr_afc_count;
extern int sg_inhibit_count;
extern int sg_enable_flag;

extern int basebraw_test_cnt1;
extern int basebraw_test_cnt2;
extern int basebraw_test_cnt3;

extern float rds_f1;
extern float rds_f2;
extern float rds_phase;
extern float rds_power;
extern int bg_filter_points;
extern float *squelch_info;
extern double squelch_turnon_time;
extern int squelch_on;
extern int bg_waterfall_blank_points;

extern float keying_spectrum_pos[KEYING_SPECTRUM_MAX];
extern float keying_spectrum_ampl[KEYING_SPECTRUM_MAX];
extern int keying_spectrum_ptr;
extern int keying_spectrum_max;
extern int keying_spectrum_cnt;
extern float keying_spectrum_f1;
extern int min_daout_samps;

extern int keying_spectrum_size;
extern float *keying_spectrum;
extern int cg_oscw;
extern int cg_osc_points;
extern int bg_no_of_notches;
extern int bg_current_notch;
extern int bg_notch_pos[MAX_BG_NOTCHES];
extern int bg_notch_width[MAX_BG_NOTCHES];
extern int bg_agc_hang_pts;

extern char *mg_behind_meter;
extern short int *mg_rms_ypix;
extern short int *mg_peak_ypix;
extern float *mg_rms_meter;
extern float *mg_peak_meter;
extern short int mg_first_xpixel;
extern short int mg_last_xpixel;
extern short int mg_pa;
extern short int mg_px;
extern short int mg_size;
extern short int mg_mask;
extern short int mgw_p;
extern short int mg_ymin;
extern short int mg_ymax;
extern int mg_stonavg_mouse;
extern int mg_stonavg_x1;
extern int bg_update_filter;
extern char mg_clear_flag;
extern double mg_dbscale_start;
extern double mg_scale_offset;
extern double mg_dbstep;
extern float mg_midscale;
extern int mg_valid;
extern int mg_bar;
extern char *mg_barbuf;
extern short int mg_bar_x1;
extern short int mg_bar_x2;
extern short int mg_y0;
extern short int mg_bar_y;
extern float reg_dot_power[5];
extern float reg_dot_re[5];
extern float reg_dot_im[5];
extern float reg_dash_power;
extern float reg_dash_re;
extern float reg_dash_im;
extern int dot_siz, dot_sizhalf, dash_siz, dash_sizhalf;
extern int cg_old_y1;
extern int cg_old_y2;
extern int cg_old_x1;
extern int cg_old_x2;

extern int cw_item_len[256-CW_DASH];
extern char *cw_item_text[256-CW_DASH];
extern unsigned char *morsascii[6];
extern int cg_no_of_traces;
extern int cg_lsep;
extern unsigned char cg_color[];
extern int cg_osc_ptr;
extern int cg_osc_offset;
extern int cg_osc_offset_inc;
extern int cg_osc_shift_flag;
extern int cg_max_trlevel;
extern float cg_code_unit;
extern float cg_decay_factor;

extern int cw_waveform_max;

extern unsigned int cw_avg_points;
extern int cg_wave_start;
extern float cg_wave_midpoint;
extern float cg_wave_fit;
extern float cg_wave_coh_re;
extern float cg_wave_coh_im;
extern float cg_wave_raw_re;
extern float cg_wave_raw_im;
extern float cg_wave_dat;
extern float cg_wave_err;

extern int cg_update_interval;
extern int cg_update_count;

extern float cw_stoninv;
extern int no_of_cwdat;
extern int correlate_no_of_cwdat;
extern int cw_decoded_chars;
extern float baseband_noise_level;
extern float carrier_noise_level;
extern float *cw_carrier_window;

extern float fm0_ph1;
extern float fm0_ph2;
extern float fm1_ph1;
extern float fm1_ph2;
extern float fm2_ph1;
extern float fm2_ph2;
extern float *fmfil70_fir;
extern int fmfil70_size;
extern int fmfil70_n;
extern int fmfil70_points;
extern float *fm_audiofil_fir;
extern int fm_audiofil_size;
extern int fm_audiofil_n;
extern int fm_audiofil_points;
extern int fm1_resample_ratio;
extern float fm1_sampling_speed;
extern int fm1_pa;
extern int fm1_px;
extern float *fm1_all;
extern int fm1_size;
extern int fm1_mask;


extern float *fmfil55_fir;
extern int fmfil55_size;
extern int fmfil55_n;
extern int fmfil55_points;
extern float *fmfil_rds_fir;
extern int fmfil_rds_size;
extern int fmfil_rds_n;
extern int fmfil_rds_points;
extern float *fmfil_fir;
extern int fmfil_size;
extern int fmfil_n;
extern int fmfil_points;
extern float *fmfil2_fir;
extern int fmfil2_size;
extern int fmfil2_n;
extern int fmfil2_points;
extern int fm_n;
extern int fm_size;  
extern unsigned short int fm_permute[MAX_FM_FFTSIZE];
extern COSIN_TABLE fm_tab[MAX_FM_FFTSIZE/2];
extern float fm_win[MAX_FM_FFTSIZE];


extern double da_block_counter;
extern float baseband_sampling_speed;
extern int timf1_to_baseband_speed_ratio;
extern size_t *baseband_handle;
extern int baseband_size;
extern int baseband_mask;
extern int baseband_neg;
extern int baseband_sizhalf;
extern float *baseb_out;
extern float *baseb;
extern double *d_baseb;
extern float *baseb_raw;

extern float *baseb_fm_phase;
extern float *baseb_fm_rdsraw;
extern float *baseb_fm_composite;
extern float *baseb_fm_sumchan;
extern float *baseb_fm_diffchan;
extern float *baseb_fm_demod;
extern float *baseb_fm_demod_low;
extern float *baseb_fm_demod_high;
extern float *fm_pilot_tone;
extern float *baseb_fm_pil2;
extern float *baseb_fm_pil2det;
extern float *baseb_fm_pil3;
extern float *baseb_fm_pil3det;
extern int fm_pilot_size;


extern float *baseb_wb_raw;
extern float *baseb_raw_orthog;
extern float *baseb_carrier;
extern double *d_baseb_carrier;
extern float *baseb_carrier_ampl;
extern float *baseb_totpwr;
extern float *baseb_envelope;
extern float *baseb_upthreshold;
extern float *baseb_threshold;
extern float *baseb_fit;
extern float *baseb_tmp;
extern float *baseb_agc_level;
extern float *baseb_agc_det;
extern float *basblock_maxpower;
extern float *basblock_avgpower;
extern short int *baseb_ramp;
extern float *baseb_sho1;
extern float *baseb_sho2;
extern float *cg_map;
extern short int *cg_traces;
extern float *dash_waveform;
extern float *dash_wb_waveform;
extern float *dot_wb_waveform;
extern float *baseb_clock;
extern int dash_wb_ia;
extern int dash_wb_ib;
extern int dot_wb_ia;
extern int dot_wb_ib;
extern float carrfil_weight;
extern float bgfil_weight;
extern double d_carrfil_weight;
extern double d_bgfil_weight;
extern int cg_yzer[CG_MAXTRACE];
extern int cw_ptr;
extern int cw_detect_flag;
extern int baseb_pa;
extern int baseb_pb;
extern int baseb_pc;
extern int baseb_pd;
extern int baseb_pe;
extern int baseb_pf;
extern int baseb_ps;
extern int baseb_pm;
extern int baseb_pn;
extern int baseb_py;
extern int baseb_px;
extern float baseb_fx;

extern size_t *siganal_handle;
extern int sg_pa;
extern int sg_px;
extern int sg_interleave_points;
extern int sg_pixels_per_point;
extern int sg_points_per_pixel;
extern int sg_siz;
extern int sg_corrnum;
extern int sg_numpow;
extern int sg_y0;
extern int sg_first_xpixel;
extern int sg_last_xpixel;
extern double sg_reset_time;
extern double sg_display_time;
extern int sg_valid;
extern int corrpow_cnt;

extern int fftn_tmp_size;
extern int siganal_totmem;
extern double *sg_fft;
extern float *sg_pwr;
extern double *sg_pwrsum;
extern double *sg_corrsum;
extern double *sg_window;
extern D_COSIN_TABLE *sg_tab;
extern unsigned int *sg_permute;
extern double *sg_tmp;
extern char *sg_background;
extern void do_siganal(void);

extern short int *sg_an1spectrum;
extern short int *sg_an2spectrum;
extern short int *sg_ancspectrum;
extern short int *sg_pn1spectrum;
extern short int *sg_pn2spectrum;
extern short int *sg_pncspectrum;

extern float *mix2_tmp;
extern float *mix2_pwr;
extern D_COSIN_TABLE *d_mix2_table;
extern char *daout;
extern float *fftn_tmp;
extern double *d_fftn_tmp;
extern MORSE_DECODE_DATA *cw;
extern int daout_size;
extern int new_baseb_flag;
extern float da_resample_ratio;
extern float new_da_resample_ratio;
extern int daout_upsamp;
extern int new_daout_upsamp;
extern int daout_upsamp_n;
extern int daout_pa;
extern int daout_px;
extern int daout_bufmask;
extern int flat_xpixel;
extern int curv_xpixel;
extern int bfo_xpixel;
extern int bfo10_xpixel;
extern int bfo100_xpixel;
extern int bfo_flag;
extern int baseb_queue;
extern double rx_daout_cos;
extern double rx_daout_sin;
extern double rx_daout_phstep_sin;
extern double rx_daout_phstep_cos;

extern float daout_gain;
extern int daout_gain_y;
extern char *bg_volbuf;
extern float *bg_binshape;
extern float *bg_ytmp;
extern int baseb_channels;
extern float baseb_wts;
extern float da_wttim;  
extern float baseb_wttim;
extern float db_wttim;
extern float da_wait_time;
extern int poleval_pointer;
extern POLCOFF poleval_data[POLEVAL_SIZE];
extern int bg_filtershift_points;
extern int bg_expand;
extern int bg_coherent;
extern int bg_delay;
extern int bg_twopol;
extern float bg_expand_a;
extern float bg_expand_b;
extern float bg_agc_amplimit;
extern float bg_amplimit;
extern float bg_maxamp;
extern int bg_amp_indicator_x;
extern int bg_amp_indicator_y;
extern int da_ch2_sign;
extern int cg_size;
extern int dash_pts;
extern float dash_sumsq;
extern float cwbit_pts;
extern float refpwr;
extern int basblock_size;
extern int basblock_pa;
extern int basblock_mask;
extern int basblock_hold_points;
extern float s_meter_peak_hold;
extern float s_meter_fast_attack_slow_decay;
extern float s_meter_average_power;
extern float cg_maxavg;
extern float baseband_pwrfac;
extern float rx_agc_factor1;
extern float rx_agc_factor2;
extern float rx_agc_sumpow1;
extern float rx_agc_sumpow2;
extern float rx_agc_sumpow3;
extern float rx_agc_sumpow4;
extern float rx_agc_fastsum1;
extern float rx_agc_fastsum2;
extern float rx_agc_fast_factor1;
extern float rx_agc_fast_factor2;
extern float agc_attack_factor1;
extern float agc_attack_factor2;
extern float agc_release_factor;
extern float am_dclevel1;
extern float am_dclevel2;
extern float am_dclevel_factor1;
extern float am_dclevel_factor2;
extern int s_meter_avgnum;
extern float s_meter_avg;
extern int max_cwdat;
extern FILE *mg_meter_file;
extern int mg_meter_file_cnt;
extern float mg_meter_file_sum;
extern unsigned int mg_meter_file_tot;
extern double mg_meter_start_time;
void show_bg_maxamp(void);
void store_dash(void);
void fit_dash(void);
void shift_cg_osc_track(int shift);
float check_dash(float pos);
void set_region_envelope(void);
void set_long_region_envelope(void);
void clear_region(void);
int store_symmetry_adapted_dash(int ideal);
void update_cg_traces(void);
void detect_cw_speed(void);
void skip_old_dashes(void);
void first_find_parts(void);
void second_find_parts(void);
void cw_decode(void);
void cw_decode_region(void);
void init_cw_decode(void);
void init_cw_decode_region(void);
void seldash_cwspeed(int da, int db);
void remove_dash(void);
void insert_item(int type);
void make_bg_waterf_cfac(void);
void new_bg_waterfall_gain(void);
void new_bg_waterfall_zero(void);
void change_bg_waterf_avgnum(void);
void wb_investigate_region(float pos, int len);
void get_wb_average_dashes(void);
float collect_wb_ston(void);
void find_good_dashes(int ia, int ic, char color);
int short_region_guesses(int types);
void decoded_cwspeed(void);
void get_wb_average_dots(void);
void guess_wb_average_dots(void);
void init_basebmem(void);
void make_baseband_graph(int flag);
void init_baseband_sizes(void);
void make_bg_filter(void);
void clear_baseb_arrays(int nn,int k);
void clear_baseb(void);

