
#define FFT2_CURMODE fft2_version[ui.rx_rf_channels-1][genparm[SECOND_FFT_VERNR]]


extern int yieldflag_fft2_fft2;
extern int fft2_n;
extern int fft2_size;
extern int fft2_interleave_points;
extern int max_fft2n;
extern int fft2_na;
extern int fft2_nb;
extern int fft2_nc;
extern int fft2_nm;
extern int fft2_pa;
extern int fft2_pt;
extern int fft2_nx;
extern int fft2n_mask;
extern int fft2_blockbytes;
extern int fft2_totbytes;
extern int fft2_mask;
extern int fft2_chunk_n;
extern int fft2_chunk_counter;
extern int fft2_inc;
extern int fft2_m1;
extern int fft2_m2;
extern int fft2_att_limit;
extern int hgwat_first_xpoint;
extern int hgwat_xpoints_per_pixel;
extern int hgwat_pixels_per_xpoint;
extern float fft2_blocktime;
extern int fft2_new_points;
extern float fft2_wtb;
extern float fft2_wttim;  
extern int fft2_maxamp[];

extern float *fft2_float;
extern unsigned short int *fft2_permute;
extern float *fft2_window;
extern COSIN_TABLE *fft2_tab;
extern float *fft2_power_float;
extern float *fft2_powersum_float;
extern float fft2_bandwidth;

extern short int *fft2_short_int;
extern unsigned int *fft2_bigpermute;
extern MMX_COSIN_TABLE *fft2_mmxcosin;
extern short int *fft2_mmxwin;                       
extern int make_fft2_status;



extern float *hg_fft2_pwrsum;
extern float *hg_fft2_pwr;
extern float *hg_fft2_pol;
extern int *fft2_power_int;
extern TWOCHAN_POWER *fft2_xypower;
extern TWOCHAN_POWER *fft2_xysum;
extern int fft2_version[2][MAX_FFT2_VERNR];
extern int hg_floatypix;

void make_fft2(void);
void fft2_mix1_fixed(void);
void fft2_mix1_afc(void);
void first_noise_blanker(void);
void fft2_update_liminfo(void);
void update_hg_spectrum(void);
void make_hires_graph(int flag);
void make_hires_valid(void);
