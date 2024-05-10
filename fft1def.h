
#define FFT1_CURMODE fft1_version[fft1mode][genparm[FIRST_FFT_VERNR]]
#define FFT1_BCKCURMODE fft1_back_version[ui.rx_rf_channels-1][genparm[FIRST_BCKFFT_VERNR]]

#define TXTEST_PEAK_REDRAW_COUNT 10

#define FFT1_NETSEND_MAXBUF 32

// Length of buffer indicators shown when "T" is pressed.
#define INDICATOR_SIZE 128

// Buffer pointers for the fft1 buffer.
// They all point to the same memory location but we change
// the data format during processing so it is convenient to have 
// different pointers defined, one for each type.
#define MAX_FFT1_BATCH_N 4

extern int fft1_skip_flag;
extern int fft1_use_gpu;
extern int timf1_mute;
extern int timf1_mute_counter;
extern int fft1_liminfo_cnt;
extern int fft2_liminfo_cnt;
extern double diskread_block_counter;
extern double file_start_block;
extern double file_stop_block;

extern double fft1_show_time;
extern int change_fft1_flag;
extern int slowcorr_tot_avgnum;
extern int latest_wg_spectrum;
extern int latest_wg_spectrum_shown;
extern float wg_corrtot_yfac_power;
extern float wg_corr_yfac_power;
extern int fft1_correlation_flag;
extern int yieldflag_timf2_fft1;
extern int yieldflag_wdsp_fft1;
extern int timf1_indicator_block;
extern int fft1_indicator_block;
extern int timf2_indicator_block;
extern int fft2n_indicator_block;
extern int timf3_indicator_block;
extern int indicator_first_pixel;
extern int fft3_indicator_block;
extern int baseb_indicator_block;
extern int daout_indicator_block;
extern int dabuf_indicator_block;
extern int indicator_ypix;
extern size_t *fft1_handle;

extern float *reg_noise;
extern float *reg_min;
extern float *reg_ston;
extern int *reg_first_point;
extern int *reg_length;

extern COSIN_TABLE *fft1tab;
extern D_COSIN_TABLE *d_fft1tab;
extern COSIN_TABLE *fft1_backtab;
extern unsigned short int *fft1_permute;
extern unsigned int *fft1_bigpermute;
extern float *fft1_window;
extern double *d_fft1_window;
extern float *fft1_sumsq;
extern float *fft1_slowsum;
extern float *fft1_power;
extern float *fft1_powersum;
extern float *fft1_corrsum;
extern float *fft1_slowcorr;
extern double *fft1_slowcorr_tot;

extern TWOCHAN_POWER *fft1_xypower;
extern TWOCHAN_POWER *fft1_xysum;
extern float *fftw_tmp;
extern float *timf2_tmp;
extern float *fftf_tmp;
extern float *fftt_tmp;
extern char *rawsave_tmp;
extern char *rawsave_tmp_net;
extern char *rawsave_tmp_disk;
extern char  *fft1_char;
extern short int *fft1_short_int;
extern int   *fft1_int;
extern float *fft1_float;
extern float *fft1_foldcorr;
extern short int *fft1_spectrum;
extern short int *fft1_corr_spectrum;
extern short int *fft1_corr_spectrum_tot;
extern float *fft1_filtercorr;
extern float *fft1_desired;
extern float *liminfo;
extern unsigned char *liminfo_wait; 
extern float *liminfo_group_min;
extern short int *fft1_split_shi;
extern float *fft1_split_float;
extern MMX_COSIN_TABLE *fft1_mmxcosin;
extern float *fft1_inverted_window;
extern short int *fft1_inverted_mmxwin;
extern unsigned short int *fft1_back_scramble;
extern short int *fft1_backbuffer;
extern char *fft1_netsend_buffer;

extern short int *wg_waterf;
extern int wg_waterf_size;
extern int wg_waterf_ptr;
extern int local_wg_waterf_ptr;
extern WATERF_TIMES *wg_waterf_times;
extern int wg_waterf_ptr2;
extern int local_wg_yborder;
extern int fft1_waterfall_flag;
extern int ui_converter_direction;
extern double converter_offset_mhz;

extern int wg_refresh_flag;
extern double wg_refresh_time;
extern int fft1_costab_size,fft1_permute_size,fft1_window_size;
extern int fft1_tmp_bytes;
extern int timf2pow_size;
extern int timf2pow_mask;

extern int timf3_totsiz;
extern float fft2_interleave_ratio, fftx_blocktime;


extern int max_wg_waterf_times;
extern float *wg_waterf_yfac;
extern float txtest_decayfac2;
extern float txtest_decayfac1;
extern int txtest_last_xpix;
extern int txtest_peak_redraw;
extern float txtest_saturated;
extern short int *txtest_ypeak;
extern short int *txtest_ypeak_decay;
extern short int *txtest_yavg;
extern float *txtest_power;
extern float *txtest_powersum;
extern short int *txtest_old_yavg;
extern short int *txtest_old_ypeak;
extern short int *txtest_old_ypeak_decay;
extern short int *fft1_old_spectrum;
extern float *txtest_peak_power;
extern float *txtest_peak_power_decay;
extern short int *rxin_isho;
extern int *rxin_int;
extern char *rxin_char;


extern float fft1_bandwidth;
extern float fft1_hz_per_point;
extern float fft1_filtercorr_start;
void fft1_buffer_pointers(void);
void init_foldcorr(void);
void set_fft1_endpoints(void);
void normalise_fft1_filtercorr(float *xyzq);
void finish_rx_read(void);


extern int fft1_n;
extern int fft1_size;
extern int fft1_block;
extern int fft1_mulblock;
extern int fft1_muln;
extern int fft1_blockbytes;
extern int fft1_mask;  
extern int fft1_bytes;      
extern int rx_read_bytes;
extern int fft1_bufmask;       
extern int fft1net_pa;
extern int fft1net_px;
extern int fft1net_size;
extern int fft1net_mask;
extern int fft1_pa;
extern int fft1_pb;
extern int fft1_px;
extern int fft1_na;
extern int fft1_nb;
extern int fft1_nc;
extern int fft1_nm;
extern int fft1_nx;
extern int fft1afc_flag;
extern int fft1_first_point;
extern int fft1_first_sym_point;
extern int fft1_first_sumsq_point;
extern int fft1_last_point;
extern int fft1_last_inband;
extern int fft1_first_inband;
extern int fft1_last_sym_point;
extern int fft1_last_sumsq_point;
extern int fft1back_lowgain_n;
extern int fft1_filtercorr_flag;
extern int fft1_calibrate_flag;
extern int fft1_lowlevel_points;
extern float fft1_lowlevel_fraction;
extern int ad_wts;
extern int max_fft1n;
extern int fft1n_mask;
extern float fft1_wtb;
extern float ad_wttim;
extern float fft1_wttim;  
extern float timf2_wtb;
extern float timf2_wttim;  
extern int rx_ad_maxamp[];
extern int timf2_maxamp[];
extern int fft1_maxamp[];
extern float liminfo_amplitude_factor;
extern float fft1_desired_totsum;
extern int fft1_sumsq_tot;
extern int fft1_direction;
extern int fft1_filtercorr_direction;
extern int wg_xpix1;
extern int wg_waterfall_blank_points;
extern int no_of_fft1b;

extern int fft1_sumsq_mask;
extern int fft1_sumsq_bufsize;
extern int fft1_interleave_points;
extern float fft1_interleave_ratio;
extern int fft1_new_points;
extern float fft1_blocktime;
extern float fft1_noise_floor;
extern int fft1_sumsq_counter;
extern int liminfo_groups;
extern int liminfo_group_points;
extern int correlation_reset_flag;
extern int ad_bufmargin;
extern int fft1corr_reset_flag;
extern int max_fft1_sumsq;
extern int fft1_sumsq_pa;
extern int radar_fft1_sumsq_pa;
extern int fft1_sumsq_pwg;
extern int fft1_sumsq_recalc;
extern int fft1mode;


extern FILE *save_wr_file;
extern FILE *save_rd_file;
extern int save_init_flag;
extern int save_rw_bytes;
extern int diskwrite_flag;
extern int diskread_flag;

extern int fft1_version[4][MAX_FFT1_VERNR];
extern int fft1_back_version[2][MAX_FFT1_BCKVERNR];
extern FFT_SETUP_INFO fft_cntrl[MAX_FFT_VERSIONS];

extern int timf1_bytes;
extern int timf1_bytemask;
extern int timf1_blockbytes;
extern int timf1_usebytes;
extern char *timf1_char;
extern int *timf1_int;
extern short int *timf1_short_int;
extern float *timf1_float;
extern int timf1p_pa;
extern int timf1p_px;
extern int timf1p_pb;
extern int timf1p_pc_net;
extern int timf1p_pc_disk;

extern int timf1p_sdr;


extern float timf1_sampling_speed;

extern short int *timf2_shi;
extern float *timf2_float;
extern float *timf2_blockpower;
extern int timf2_blockpower_block;
extern int timf2_blockpower_pa;
extern int timf2_blockpower_px;
extern int timf2_blockpower_size;
extern int timf2_blockpower_mask;
extern int timf2_pa;
extern int timf2_pb;
extern int timf2_pc;
extern int timf2_pn1;
extern int timf2_pn2;
extern int timf2_pt;
extern int timf2p_fit;
extern int timf2_px;
extern int timf2_mask;
extern int timf2_neg;
extern int timf2_size;
extern int timf2_input_block;
extern int timf2_output_block;
extern int timf2_oscilloscope_counter;
extern int timf2_oscilloscope_interval;
extern int timf2_oscilloscope_maxpoint;
extern int timf2_ovfl;
extern int timf2_fitted_pulses;
extern int timf2_cleared_points;
extern int timf2_blanker_points;
extern int timf2_ymin[16];
extern int timf2_ymax[16];
extern int timf2_y0[16];
extern unsigned int *timf2_pwr_int;
extern float *timf2_pwr_float;
extern int timf2_hg_xmin;
extern int timf2_hg_xmax;
extern int timf2_hg_y[2];
extern int timf2_hg_yh;
extern int timf2_hg_x[2];
extern int timf2_hg_stux;
extern int timf2_hg_clex;
extern int timf2_hg_qex;
extern float timf2_hg_xfac;
extern float timf2_hg_xlog;
extern int overrun_limit;
extern int wg_last_point;
extern int wg_first_point;


extern short int *timf2pix;
extern float timf2_oscilloscope_maxval_float;
extern float timf2_oscilloscope_powermax_float;
extern unsigned int timf2_oscilloscope_maxval_uint;
extern unsigned int timf2_oscilloscope_powermax_uint;
extern double wg_waterf_zero_time;
extern int txtest_no_of_segs;
extern float txtest_pixinc;
extern int txtest_pntinc;
extern int txtest_first_xpix;
extern int txtest_spek_no;
extern int txtest_spek_p0;
extern int txtest_first_point;
extern int txtest_show_p0;
extern float txtest_yfac;


void simd1_16_win(int timf1p_ref, float *out);
void simd1_16_nowin(int timf1p_ref, float *out);
void simd1_32_win(int timf1p_ref, float *out);
void simd1_32_nowin(int timf1p_ref, float *out);
void simd1_16_win_real(int timf1p_ref, float *out);
void simd1_16_nowin_real(int timf1p_ref, float *out);
void simd1_32_win_real(int timf1p_ref, float *out);
void simd1_32_nowin_real(int timf1p_ref, float *out);
void fft1_update_liminfo(void);
void compress_rawdat_disk(void);
void compress_rawdat_net(void);
void expand_rawdat(void);
void fft1_approx_dif_two(void);
void fft1_approx_dif_one(void);
void fft1_approx_dit_one(void);
void fft1_approx_dit_two(void);
void fft1_reherm_dit_one(int timf1p_ref, float *out, float *tmp);
void fft1_reherm_dit_two(int timf1p_ref, float *out, float *tmp);
void fft1_complex_two(void);
void make_timf2(void);
void show_timf2(void);
void split_one(void);
void split_two(void);
void mmx_fft1back_two(void);
void mmx_fft1back_one(void);
void fft1back_one(void);
void fft1back_two(void);
void fft1back_fp_finish(void);
void fft1back_mmx_finish(void);
void fft1_mix1_fixed(void);
void fft1_mix1_afc(void);
void fft1_waterfall(void);
void read_rawdat(int debughelp);
void fft1_b(int timf1p_ref, float *out, float *tmp, int gpu_handle_number);
void fft1_c(void);
void make_filfunc_filename(char *s);
void make_iqcorr_filename(char *s);
void check_fft1_timing(void);
void make_wg_waterf_cfac(void);
void clear_around_wgbutt(int n);
void change_fft1avgnum(void);
void change_waterfall_avgnum(void);
void change_wg_waterfall_zero(void);
void change_wg_waterfall_gain(void);
void fft1_block_timing(void);
void update_fft1_slowsum(void);
void show_txtest_spectrum(void);
void resize_filtercorr_td_to_fd(int to_fd, int size_in, float *buf_in, 
                       int n_out, int size_out, float *buf_out);
void convert_filtercorr_fd_to_td(int n, int siz, float *buf);
void resize_fft1_desired(int siz_in, float *buf_in, 
                         int siz_out, float *buf_out);
void clear_fft1_correlation(void);
