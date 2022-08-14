
#define MICBLOCK_SSBTIME 0.0205        
#define MICBLOCK_CWTIME 0.0005
#define SSB_DELAY_EXTRA 0.003

#define SSB_MAX_MICBLOCK_TIME 100
#define SSB_MIN_MICBLOCK_TIME 5
#define SSB_MIN_DELAY_MARGIN 1
#define SSB_MAX_DELAY_MARGIN 50

#define MAX_DISPLAY_TYPES 3
#define MAX_SSBPROC_FILES 99
#define MAX_CWPROC_FILES 10
#define SSB_MINFQ_LOW 50
#define SSB_MINFQ_HIGH 3000
#define SSB_MAXFQ_LOW 1000
#define SSB_MAXFQ_HIGH 8000
#define SSB_MINSLOPE -50
#define SSB_MAXSLOPE 50
#define SSB_MINBASS -50
#define SSB_MAXBASS 50
#define SSB_MINTREBLE -50
#define SSB_MAXTREBLE 50
#define SSB_MAX_MICF 120
#define SSB_MAX_MICT 120
#define SSB_MAX_MICGAIN 120
#define SSB_MAX_MICAGC_TIME 1000
#define SSB_MAX_ALC_TIME 1000
#define SSB_MIN_RF1_GAIN -50
#define SSB_MAX_RF1_GAIN 120
#define MAX_DYNRANGE 0.000001 // -60dB for weakest signal

#define TX_SETUP_AD 0
#define TX_SETUP_MUTE 1
#define TX_SETUP_MICAGC 2
#define TX_SETUP_RF1CLIP 3
#define TX_SETUP_ALC 4

#define TX_BAR_AD_LINE 10
#define TX_BAR_MUTE_LINE 12
#define TX_BAR_MICAGC_LINE 14
#define TX_BAR_RF1CLIP_LINE 16
#define TX_BAR_ALC_LINE 18
#define TXPAR_INPUT_LINE 20

#define MIC_KEY_LINE 19
#define KEY_INDICATOR_LINE (MIC_KEY_LINE+10)
#define TXCW_INPUT_LINE (KEY_INDICATOR_LINE+4)
#define CW_MAX_TIME_CONSTANT 100.0
#define CW_MIN_TIME_CONSTANT 1.0
#define CW_MIN_DELAY_MARGIN 1
#define CW_MAX_DELAY_MARGIN 25


#define TX_BAR_SIZE 256

#define MAX_DISPLAY_TYPES 3
#define DISPLAY_HOLD_SIZE 32
#define TG_HSIZ (FG_HSIZ+8*text_width)
#define TG_VSIZ (3*(text_height+3)+3)

#define MAX_SCREENCOUNT 15


extern int tx_oscilloscope_max;
extern int tx_oscilloscope_min;
extern int tx_oscilloscope_zero;
extern int tx_setup_mode;
extern double tx_oscilloscope_time;
extern float *tx_oscilloscope_tmp;
extern int micdisplay_fftsize;
extern int tx_display_mode;
extern int tx_oscilloscope_ptr;
extern float tx_oscilloscope_ypixels;
extern int tx_oscilloscope_tmp_size;
extern float *cliptimf_oscilloscope;
extern int sw_cliptimf;
extern int tx_min_output_samples;
extern float af_gain;
extern float rf_gain;
extern float tx_minlevel;
extern int tx_fft_phase_flag;

#define TX_DISPLAY_MICTIMF '1'
#define TX_DISPLAY_MICSPECTR '2'
#define TX_DISPLAY_MICSPECTR_FILTERED '3'
#define TX_DISPLAY_MICSPECTR_FILTALL '4'
#define TX_DISPLAY_MUTE_T '1'
#define TX_DISPLAY_MUTE_F '2'
#define TX_DISPLAY_AGC '5'
#define TX_DISPLAY_CLIP '6'
#define TX_DISPLAY_CLIPFFT_WIDE '1'
#define TX_DISPLAY_CLIPFFT_NARROW '2'
#define TX_DISPLAY_CLIPFFT_WIDE_FILT '3'
#define TX_DISPLAY_CLIPFFT_NARROW_FILT '4'
#define TX_DISPLAY_FILTER '5'
#define TX_DISPLAY_ALCTIMF_RAW '1'
#define TX_DISPLAY_ALCTIMF '2'
#define TX_DISPLAY_ALC_FACTOR '3'
#define TX_DISPLAY_ALC_SPECTRUM '4'
#define TX_DISPLAY_RESAMP '5'
#define TX_DISPLAY_TXOUT_SPECTRUM '6'
#define TX_DISPLAY_TXOUT_TIMF '7'

extern double tx_passband_center;
extern float mic_to_spproc_resample_ratio;
extern int tx_onoff_flag;
extern double tg_basebfreq_hz;
extern int mic_key_pa;
extern int mic_key_px;
extern int screen_count;
extern int tx_lowest_bin;
extern int tx_highest_bin;
extern char *clipfft_mute;
extern char *cliptimf_mute;
extern char *alctimf_mute;
extern float *tx_mic_key;
extern float micfft_winfac;
extern int tg_old_y1;
extern int tg_old_y2;
extern int tg_old_x1;
extern int tg_old_x2;
extern float tx_hware_fqshift;
extern float tx_ssbproc_fqshift;
extern int mic_key_bufsiz;
extern int mic_key_block;
extern int mic_key_mask;
extern int txda_indicator_block;
extern int mic_key_indicator_block;
extern int mictimf_indicator_block;
extern int alc_indicator_block;
extern int txout_indicator_block;
extern int old_cw_rise_time;
extern double tx_daout_cos;
extern double tx_daout_sin;
extern double tx_daout_phstep_sin;
extern double tx_daout_phstep_cos;
extern float tx_ideal_delay;
extern float tx_ssb_resamp_block_factor;
extern float rg_old_time;
extern float rg_old_gain;
extern float rg_old_zero;

extern int tx_graph_scro;
extern int tx_flag;
extern int txmem_size;
extern int tx_ad_maxamp;
extern int tx_input_mode;
extern float tx_resamp_ampfac1;
extern float tx_resamp_ampfac2;
extern size_t *txmem_handle;
extern int tx_audio_in;
extern int tx_audio_out;
extern short int *txtrace;
extern float txad_hz_per_bin;
extern float txtrace_gain;
extern int txtrace_height;
extern float micfft_bin_minpower_ref;
extern float micfft_bin_minpower;
extern float micfft_minpower_ref;
extern float micfft_minpower;
extern float *mic_filter;
extern int *tx_out_int;

extern short int *tx_out_shi;
extern float tx_agc_factor;
extern float tx_agc_decay;
extern float txpwr_decay;
extern float tx_forwardpwr;
extern float tx_backpwr;
extern int tx_output_flag;
extern int tx_ph1;
extern float tx_resample_ratio;
extern float new_tx_resample_ratio;
extern float tx_output_upsample;
extern int tx_resamp_pa;
extern int tx_resamp_px;
extern int tx_filter_ia1;
extern int tx_filter_ia2;
extern int tx_filter_ia3;
extern int tx_filter_ib1;
extern int tx_filter_ib2;
extern int tx_filter_ib3;
extern char display_color[MAX_DISPLAY_TYPES];
extern int tx_pre_resample;
extern int tx_output_mode;
extern int tx_show_siz;
extern int tx_wsamps;
extern float tx_wttim;
extern float tx_wttim_sum;
extern float tx_wttim_ref;
extern float tx_wttim_diff;
extern int tx_setup_flag;

extern float tx_pilot_tone;
extern float tx_output_amplitude;
extern int txout_pa;
extern int txout_px;
extern int txout_bufsiz;
extern int txout_mask;
extern float mic_key_fx;
extern float *txout_waveform;
extern int max_tx_cw_waveform_points;
extern int tx_cw_waveform_points;
extern int tone_key, tot_key;
extern int tx_waveform_pointer;
extern int old_ptt_state;
extern int ptt_state;
extern int ptt_on_wait;
extern int ptt_off_wait;
extern int tx_mode;


extern char ssbproc_filename[];
extern char cwproc_filename[];
extern char *txproc_filename;

extern float *radar_pulse_ston;
extern float *radar_pulse_noise_floor;
extern int *radar_pulse_bin;
extern int *radar_pulse_pointer;
extern size_t *radar_handle;
extern float *radar_average;
extern int radar_graph_scro;
extern int rg_old_x1;
extern int rg_old_x2;
extern int rg_old_y1;
extern int rg_old_y2;
extern int radar_bins;
extern int radar_maxlines;
extern int radar_lines;
extern int pulse_sep;
extern int pulse_bin;
extern float radar_decayfac;
extern int radar_redraw_count;
extern int radar_update_cnt;
extern float rg_cfac;
extern float rg_czer;
extern int radar_first_bin;
extern int radar_last_bin;
extern int radar_show_bins;

extern short int *tx_oscilloscope;

// ***************************************************
//      Buffer and block sizes in CW mode.
//
// ***************************************************
// ***************************************************
// The first time function, data from the soundcard. The block
// size is set for the data to span a suitable time in order to
// provide a small time delay as specified by MICBLOCK_CWTIME
// Setting a small MAX_DMA_RATE will make the blocks bigger
// and the delat larger.

// Time function 1. Microphone signal 
// sampling rate ui.tx_ad_speed
// mictimf_shi (buffer for 16 bit input)
// mictimf_int (buffer for 32 bit input)
// mictimf_adread_block (block_size =snd[TXAD].block_bytes/ui.tx_ad_bytes)
// mictimf_bufsiz (buffer size)
// mictimf_mask (mask for the mictimf buffer)

// Time function 2. Keying signal 
// sampling rate ui.tx_ad_speed/tx_pre_resample (4 to 8 kHz)
// tx_mic_key (buffer for keying signal)
// mic_key_block (spans a time near MICBLOCK_CWTIME)
// mic_key_bufsiz (buffer size)
// mic_key_mask (mask for the tx_mic_key buffer)

// Time function 3 Tx output
// sampling rate ui.tx_da_speed  (=ui.tx_ad_speed*tx_resample_ratio)
// txout (buffer for I/Q data in float format)
// txout_fftsize (block size, spans a time near MICBLOCK_CWTIME)
// txout_bufsiz (buffer size)
// txout_mask (mask for the txout buffer)
 


// ***************************************************
//      Transform sizes and associated variables.
//            Description for SSB mode
//
// ***************************************************
// The first time function, data from the soundcard. The transform
// size is set for the data to span a suitable time in order to
// get a frequency resolution and an associated time delay that
// is independent of the microphone input sampling speed.
// Size is micsize.

extern int tx_disk_mode;

extern short int *mictimf_shi;
extern int *mictimf_int;
extern int micsize;
extern int micn;
extern int mictimf_pa;
extern int mictimf_pc;
extern int mictimf_px;
extern int mictimf_block;
extern int mictimf_adread_block;
extern int mictimf_bufsiz;
extern int mictimf_mask;
extern float *mic_win;
extern COSIN_TABLE *mic_table;
extern unsigned short int *mic_permute;
extern float *mic_tmp;
// ***************************************************
// The first FFT. The transforms are likely to span a far too large
// frequency range. Only the minimum required to represent the passband
// of interest is used for the output of mic_fft     
// Size is mic_fftsize.
extern float *micfft;
extern int mic_fftsize; 
extern int mic_fftn;
extern int mic_fftsize_start;
extern int mic_fftn_start;
extern int mic_sizhalf;
extern int micfft_pa;
extern int micfft_px;
extern int micfft_block;
extern int micfft_bufsiz;
extern int micfft_mask;
extern COSIN_TABLE *micfft_table;
extern unsigned short int *micfft_permute;
extern float *micfft_win;
extern int micfft_indicator_block;
// ***************************************************
// The second time function. Here we will do clipping.
// Size is mic_fftsize.
extern float *cliptimf;
extern int cliptimf_pa;
extern int cliptimf_px;
// ***************************************************
// The second FFT. Used to clean up the spectrum.
// Size is alc_fftsize.
extern float *clipfft;
extern int clipfft_pa;
extern int clipfft_px;
// ***************************************************
// The third time function. Used for ALC.
// We will use this time function for resampling to a new sampling speed
// that will be equal to the output speed or a power of two faster or
// slower.
// The fractional resampler needs an oversampled input and therefore
// we first upsample by a power of two here.
// Size is alc_fftsize.
extern int alc_fftsize;
extern int alc_fftn;
extern int alc_sizhalf;
extern float *alctimf;
extern float *alctimf_raw;
extern float *alctimf_pwrf;
extern float *alctimf_pwrd;
extern int alctimf_pa;
extern int alctimf_pb;
extern float alctimf_fx;
extern int alc_block;
extern int alc_bufsiz;
extern int alc_mask;
extern COSIN_TABLE *alc_table;
extern unsigned short int *alc_permute;
extern float *alc_win;
// ***************************************************
// The fourth time function, the output from the fractional resampler.
// Size is alc_fftsize.
// We use transforms one by one so no buffer pointer is needed.
// The third FFT is computed in place and the size of the transform
// is changed in place to txout_fftsize.
extern float *tx_resamp;
extern float *resamp_tmp;
// ***************************************************
// The fifth time function. 
// Size is txout_fftsize.
extern int txout_fftsize;
extern int txout_fftn;
extern int txout_sizhalf;
extern float *txout;
extern float *txout_tmp;
extern COSIN_TABLE *txout_table;
extern unsigned short int *txout_permute;
// ****************************************************
extern int tx_analyze_size;
extern int tx_analyze_sizhalf;
extern int tx_analyze_fftn;
extern COSIN_TABLE *tx_analyze_table;
extern float *tx_analyze_win;
extern unsigned short int *tx_analyze_permute;
extern int mute_on;
extern int mute_off;
extern double sidetone_cos;
extern double sidetone_sin;
extern double sidetone_phstep_cos;
extern double sidetone_phstep_sin;

void show_txfft(float *z, float lim, int type, int siz);
void tx_bar(int xt,int yt,int val1, int val2);
void resample_tx_output(void);
void check_txparms(void);
void init_txmem_spproc(void);
void open_tx_sndin(void);
void close_tx_sndin(void);
void spproc_setup(void);
void cwproc_setup(void);
void save_tx_parms(char verbose);
int lir_tx_input_samples(void);
int lir_tx_output_samples(void);
void open_tx_sndout(void);
void close_tx_sndout(void);

void make_tx_graph(int clear_old);
void init_txmem_cwproc(void);
void make_txproc_filename(void);
void tx_ssb_step8(void);
float tx_ssb_step7(float *prat);
float tx_ssb_step6(float *prat);
void tx_ssb_step5(void);
int tx_ssb_step4(int n,float *t1, float *prat1, float *prat2);
void tx_ssb_step3(float *totamp);
int tx_ssb_step2(float *totpwr);
void set_default_cwproc_parms(void);
void make_tx_modes(void);
void tx_send_to_da(void);
void make_tx_cw_waveform(void);
void make_tx_phstep(void);
void init_txmem_common(float micblock_time);
void contract_tx_spectrum(float *z);
float txssb_total_delay(void);
