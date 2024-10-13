#define AG_FRANGE_MAX 5000.
#define AG_FRANGE_MIN 0.01
#define AG_MAKEP 61  // odd number!!!
#define AG_DIGITS 6
#define AG_CURVNUM 4
#define SPUR_WIDTH (SPUR_SIZE-1)
#define STON_YFAC 5*text_height
#define MAX_BASEBFILTER_WIDTH 0.85F

extern int skip_siganal;
extern int yieldflag_ndsp_mix1;
extern int freq_readout_x1;
extern int freq_readout_x2;
extern int freq_readout_y1;
extern int netslaves_selfreq[MAX_NETSLAVES];
extern float netslaves_time[MAX_NETSLAVES];
extern int netfreq_list[MAX_FREQLIST];
extern float netfreq_time[MAX_FREQLIST];
extern int netfreq_curx[MAX_FREQLIST];
extern int new_netfreq_curx[MAX_FREQLIST];

extern int enable_resamp_iir5;
extern IIR5_PARMS iir_a, iir_b;
extern double xva1[6],yva1[6];
extern double xvb1[6],yvb1[6];
extern double xva2[6],yva2[6];
extern double xvb2[6],yvb2[6];
extern double xv1[4], yv1[4];
extern double xv2[4], yv2[4];
extern double xv3[4], yv3[4];
extern double xv4[4], yv4[4];
extern double out1, out2, old_out1, old_out2;
extern double out3, out4, old_out3, old_out4;

extern int spursearch_sum_counter;
extern float *spursearch_powersum;
extern TWOCHAN_POWER *spursearch_xysum;
extern float *spursearch_spectrum;
extern char *baseb_netsend_buffer;
extern int basebnet_mask;
extern int basebnet_size;
extern int basebnet_pa;
extern int basebnet_px;

extern int allow_audio;
extern int audio_status;
extern char *basebraw_netsend_buffer;
extern int basebrawnet_mask;
extern int basebrawnet_size;
extern int basebrawnet_pa;
extern int basebrawnet_px;
extern float basebraw_sampling_speed;
extern int basebraw_ad_channels;
extern int basebraw_rf_channels;
extern int basebraw_mode;
extern int basebraw_passband_direction;
extern int spur_search_first_point;
extern int spur_search_last_point;
extern int fftx_na;
extern int fftx_nc;
extern int fftx_nm;
extern int fftx_nx;
extern int fftx_ny;
extern int fftx_nf1;
extern int fftx_nf2;
extern int max_fftxn;
extern int fftxn_mask;
extern int fftx_size;
extern float *fftx_pwr;
extern float *fftx;
extern TWOCHAN_POWER *fftx_xypower;
extern int ffts_na;
extern int ffts_nm;
extern float *spur_table;
extern short int *spur_table_mmx;
extern int *spur_location;
extern int *spur_flag;
extern float *spur_power;
extern float *spur_d0pha;
extern float *spur_d1pha;
extern float *spur_d2pha;
extern float *spur_avgd2;
extern float *spur_pol;
extern float *spur_spectra;
extern float *spur_freq;
extern float *sp_sig;
extern float *sp_pha;
extern float *sp_der;
extern float *sp_tmp;
extern float *spur_signal;
extern float *spur_ampl;
extern float *spur_noise;
extern int *spur_ind; 
extern float sp_c1,sp_c2,sp_c3;
extern int sp_avgnum;
extern int sp_numsub;
extern float sp_d0;
extern float sp_d1;
extern float sp_d2;
extern float spur_weiold;
extern float spur_weinew;
extern float spur_linefit;
extern TWOCHAN_POWER spur_pxy[SPUR_WIDTH];

extern int no_of_spurs;
extern int spur_block;
extern int spurno;
extern int spurp0;
extern int spur_speknum;
extern float spur_max_d2;
extern float spur_minston;
extern float spur_freq_factor;
extern int ag_first_xpixel;
extern int ag_xpixels;
extern int ag_size;
extern int ag_pa;
extern int ag_ps;
extern int ag_px;
extern int ag_y0;
extern int ag_y1;
extern int ag_height;
extern int ag_old_height;
extern int ag_old_fit_points;
extern int ag_pf1;
extern int ag_pf2;


extern float ag_first_fq;
extern float ag_hz_per_pixel;
extern float ag_floatypix;
extern int ag_fpar_y0;
extern int ag_fpar_ytop;
extern int ag_ston_y;
extern int ag_lock_y;
extern int ag_srch_y;
extern int ag_ston_x1;
extern int ag_ston_x2;
extern int ag_lock_x1;
extern int ag_lock_x2;
extern int ag_srch_x1;
extern int ag_srch_x2;
extern size_t *afc_handle;
extern float *afc_spectrum;
extern float *afc_lowres_spectrum;
extern float *afct_window;
extern float *ag_fitted_fq;
extern float *ag_mix1freq;
extern float *ag_freq;
extern float *ag_ston;
extern TWOCHAN_POWER *afc_xysum;
extern char *ag_background;
extern short int *ag_oldpix;
extern char *ag_stonbuf;
extern char *ag_srchbuf;
extern char *ag_lockbuf;

extern int ag_shift_xpixels;
extern int afct_delay_points;
extern int old_afct_delay;
extern int afct_avgnum;
extern int afct_half_avgnum;
extern int max_afcf_points;
extern int max_afc_fit;
extern float afcf_search_range;
extern float afcf_max_drift;
extern float afc_minston;
extern float afc_slope;
extern int afc_speknum;
extern int afc_half_speknum;
extern int afc_ib;
extern int afc_ia;
extern float afc_drift_step;
extern int afc_graph_filled;
extern float afc_maxval;
extern float afc_noise;
extern float afc_noisefac;
extern float afc_fq;
extern unsigned char afc_cursor_color;
extern int afc_tpts;
extern int mix1p0;
extern int ag_mask;
extern unsigned char afc_old_cursor_color;
extern int initial_afct_avgnum;
extern int ag_ss;

extern float mix1_lowest_fq;
extern float mix1_highest_fq;

extern float *mix1_fqwin;
extern double *d_mix1_window;
extern double *d_mix1_fqwin;
extern double *d_mix1_cos2win;
extern double *d_mix1_sin2win;
extern D_COSIN_TABLE *d_mix1_table;
extern float *mix1_fq_mid;
extern float *mix1_fq_start;
extern float *mix1_fq_curv;
extern float *mix1_fq_slope;
extern short int *mix1_eval_avgn;
extern float *mix1_eval_fq;
extern float *mix1_eval_sigpwr;
extern float *mix1_eval_noise;
extern float *mix1_fitted_fq;

extern double mix1_selfreq[MAX_MIX1];
extern double old_mix1_selfreq;
extern int mix1_curx[MAX_MIX1];
extern int new_mix1_curx[MAX_MIX1];
extern int mix1_point[MAX_MIX1]; 
extern float mix1_phase_step[MAX_MIX1];
extern float mix1_phase[MAX_MIX1];
extern float mix1_phase_rot[MAX_MIX1];
extern float mix1_old_phase[MAX_MIX1];
extern int mix1_old_point[MAX_MIX1];
extern int mix1_status[MAX_MIX1];
extern float mix1_good_freq[MAX_MIX1];


extern float fftx_points_per_hz;
extern double hwfreq;
extern double old_hwfreq;


void make_afc(void);
void make_ag_point(int np, int no_of_points);
void afc_cursor(void);
void collect_initial_spectrum(void);
void make_afct_avgnum(void);
int make_afc_signoi(void);
void make_afct_window(int new_avgn);
void fill_afc_graph(void);
void remove_spur(int nnspur);
void spursearch_spectrum_cleanup(void);
