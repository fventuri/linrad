#ifndef UIDEF_H
#define UIDEF_H

#include <stdbool.h>

#define SNDLOG if(write_log)fprintf(sndlog,
#define BUTTON_IDLE 0
#define BUTTON_PRESSED 1
#define BUTTON_RELEASED 2

#define MAX_FLOWCNT 31
#define MAX_DEVNAMES 99
#define MAX_LOWSPEED 10
#define ABOVE_MAX_SPEED 768000

#define SAVEFILE_MAX_CHARS 100

#define EXTIO_COMMAND_NOTHING 0
#define EXTIO_COMMAND_LOAD_DLL 10
#define EXTIO_COMMAND_UNLOAD_DLL 11
#define EXTIO_COMMAND_START 12
#define EXTIO_COMMAND_STOP 13
#define EXTIO_COMMAND_DONE 98
#define EXTIO_COMMAND_KILL_ALL 99 

#if(OSNUM == OSNUM_LINUX)
extern void *extio_handle;
#endif

#if(OSNUM == OSNUM_WINDOWS)
#include <windows.h>
extern HANDLE extio_handle;
#endif

extern MIXER_VARIABLES mix1;
extern MIXER_VARIABLES mix2;

extern int refresh_screen_flag;
extern char savefile_parname[SAVEFILE_MAX_CHARS+4];
extern char savefile_name[SAVEFILE_MAX_CHARS+1];
extern char *rx_soundcard_radio_names[MAX_RX_SOUNDCARD_RADIO];
extern char *tx_soundcard_radio_names[MAX_TX_SOUNDCARD_RADIO];
extern int si570_ptt;
extern float fg_truncation_error;

extern int sel_ia;
extern int sel_ib;

extern int computation_pause_flag;
extern int asio_flag;
extern int screen_refresh_flag;
extern int use_tx;
extern int usb2lpt_flag;
extern double expose_time;
extern double wg_lowest_freq;
extern double wg_highest_freq;
extern int screensave_flag;
extern int usb2lpt_flag;
extern int si570_open_switch;
extern int wavfile_squelch_flag;

extern int clear_graph_flag;
extern char rotor[4];
extern int soundcard_test_block_count[MAX_IOTEST];
extern int soundcard_test_cmd_flag[MAX_IOTEST];
extern char *rx_logfile_name;
extern int first_keypress;
extern float pg_ch2_c1;
extern float pg_ch2_c2;
extern int gpu_fft1_batch_size;


extern int wg_freq_x1;
extern int wg_freq_x2;
extern int wg_freq_adjustment_mode;
extern int no_of_processors;
extern int hyperthread_flag;
extern int cpu_rotate;
extern int thread_rotate_count;
extern int64_t tickspersec;
extern int freq_from_file;
extern float workload;
extern int workload_counter;
extern int mouse_inhibit;
extern double mouse_time_wide;
extern double mouse_time_narrow;
extern int baseb_reset_counter;
extern double max_netwait_time;
extern double net_error_time;
extern int netwait_reset;
extern double time_info_time;
extern int sdr;
extern int write_log;
extern int baseb_control_flag;
extern int spurcancel_flag;
extern int spurinit_flag;
extern int autospur_point;
extern int hand_key;
extern int kill_all_flag;
extern char lbutton_state;
extern char rbutton_state;
extern char new_lbutton_state;
extern char new_rbutton_state;
extern int lir_errcod;
extern int all_threads_started;
extern int latest_listsend_time;
extern int net_no_of_errors;
extern double recent_time;
extern int internal_generator_flag;
extern double internal_generator_shift;
extern double internal_generator_phase1;
extern double internal_generator_phase2;
extern int internal_generator_key;
extern int internal_generator_att;
extern int internal_generator_noise;
extern int truncate_flag;
extern int rxin_nbits;
extern char *msg_filename;

extern int font_xsep;
extern int font_ysep;
extern int font_w;
extern int font_h;
extern int font_size;
extern int mouse_task;
extern int rxin_block_counter;
extern int rx_hware_fqshift;

extern float min_delay_time;
extern int savefile_repeat_flag;
extern int screen_type;
extern int qz;
extern int usercontrol_mode;
extern float tune_yzer;

extern int workload_reset_flag;
extern int graphics_flag;
extern int audio_flag;
extern FILE *dmp;
extern FILE *dmp1;
extern FILE *sndlog;
extern FILE *wav_file;
extern FILE *allan_file_ch1;
extern FILE *allan_file_ch2;
extern FILE *allan_file_diff;

extern int mmx_present;
extern int simd_present;
extern ROUTINE par_from_keyboard_routine;

extern float baseband_bw_hz;
extern int baseband_bw_fftxpts;
extern int fft2_to_fft1_ratio;

extern int rx_read_time;
extern int no_of_netslaves;
extern char network_filename[];
extern int netsend_fd;
extern int netrec_fd;
extern int netraw16_blknum;
extern int netraw18_blknum;
extern int netraw24_blknum;
extern int netfft1_blknum;
extern int nettimf2_blknum;
extern int netfft2_blknum;
extern int netbaseb_blknum;
extern int netbasebraw_blknum;
extern int next_blkptr_16;
extern int next_blkptr_18;
extern int next_blkptr_24;
extern int next_blkptr_fft1;
extern int next_blkptr_timf2;
extern int next_blkptr_fft2;
extern int next_blkptr_baseb;
extern int next_blkptr_basebraw;
extern int netsend_ptr_16;
extern int netsend_ptr_18;
extern int netsend_ptr_24;
extern int netsend_ptr_fft1;
extern int netsend_ptr_timf2;
extern int netsend_ptr_fft2;
extern int netsend_ptr_baseb;
extern int netsend_ptr_basebraw;
extern int basebnet_block_bytes;
extern int basebrawnet_block_bytes;

extern int uiparm_change_flag;
extern USERINT_PARM ui;
extern SSBPROC_PARM txssb;
extern CWPROC_PARM txcw;
extern int lir_inkey;
extern char *vga_font;
#if(OSNUM == OSNUM_WINDOWS)
// If fonts_by_GDI is set, the fonts are rendered by the Windows GDI routines.
// In that case the font_color_GDI carries an index of a current font color.
extern bool fonts_by_GDI;
extern int font_color_GDI;
#endif
extern int text_width;
extern int text_height;
extern int screen_last_xpixel;
extern int screen_last_col;
extern int screen_width;
extern int screen_height;
extern int screen_totpix;
extern char userint_filename[];
extern int mouse_flag;
extern int mouse_hide_flag;
extern int mouse_x;
extern int new_mouse_x;
extern int mouse_xmax;
extern int mouse_xmin;
extern int mouse_y;
extern int new_mouse_y;
extern int mouse_ymax;
extern int mouse_ymin;
extern int mouse_cursize;
extern int leftpressed;
extern int rightpressed;
extern int mouse_lbutton_x;
extern int mouse_rbutton_x;
extern int mouse_lbutton_y;
extern int mouse_rbutton_y;
extern int rx_audio_in;
extern int rx_audio_in2;
extern int rx_audio_out;

extern SOUNDCARD_PARM snd[4];

extern int mixer;
extern float total_wttim;
extern int rx_daout_bytes;
extern int rx_daout_channels;
extern int timinfo_flag;
extern int ampinfo_flag;
extern float measured_da_speed;
extern float measured_ad_speed;
extern double diskread_time;
extern double eme_time;
extern int dasync_counter;
extern float dasync_sum;
extern float dasync_avg1;
extern float dasync_avg2;
extern float dasync_avg3;
extern double dasync_time;
extern double dasync_avgtime;
extern double da_start_time;
extern int overrun_count;
extern char savefile_parname[];
extern char savefile_name[];
extern char press_f1_for_info[];
extern size_t memalloc_no;
extern size_t memalloc_max;
extern MEM_INF *memalloc_mem;
extern DXDATA *dx;
extern int timf3_oscilloscope_limit;
extern int no_of_rx_overrun_errors;
extern int no_of_rx_underrun_errors;
extern int no_of_tx_overrun_errors;
extern int no_of_tx_underrun_errors;
extern int count_rx_underrun_flag;
extern double netstart_time;


extern int rx_mode;
extern int use_bfo;
extern int rx_channels;
extern int twice_rxchan;
extern int mailbox[];
extern char *rxpar_filenames[MAX_RX_MODE];
extern char *rxmodes[MAX_RX_MODE];
extern char newcomer_rx_modes[MAX_RX_MODE];
extern int genparm[MAX_GENPARM+2];
extern int genparm_min[MAX_GENPARM];
extern int genparm_max[MAX_GENPARM];
extern int genparm_default[MAX_RX_MODE][MAX_GENPARM];
extern char *genparm_text[MAX_GENPARM+2];
extern char newco_genparm[MAX_GENPARM];
extern char modes_man_auto[3];
extern MEM_INF fft1mem[MAX_FFT1_ARRAYS];
extern MEM_INF fft3mem[MAX_FFT3_ARRAYS];
extern MEM_INF basebmem[MAX_BASEB_ARRAYS];
extern MEM_INF hiresmem[MAX_HIRES_ARRAYS];
extern MEM_INF afcmem[MAX_AFC_ARRAYS];
extern MEM_INF txmem[MAX_TXMEM_ARRAYS];
extern MEM_INF blankermem[MAX_BLANKER_ARRAYS];
extern MEM_INF radarmem[MAX_RADAR_ARRAYS];
extern MEM_INF siganalmem[MAX_SIGANAL_ARRAYS];
extern MEM_INF allanmem[MAX_ALLAN_ARRAYS];

extern int current_graph_minh;
extern int current_graph_minw;
extern int calibrate_flag;
extern double old_passband_center;
extern int wav_write_flag;
extern int allan_write_flag;
extern int wav_read_flag;
extern int swmmx_fft2;
extern int swmmx_fft1;
extern int swfloat;
extern int sw_onechan;
extern int eme_flag;
extern char *press_any_key;
extern char *press_enter;
extern int lir_status;
extern int rxad_status;
extern int txad_status;
extern int txda_status;
extern int rxda_status;
extern int audio_dump_flag;
extern int diskread_pause_flag;
extern int allow_wse_parport;
extern char *audiomode_text[4];
extern char serport_name[];
extern char remind_parsave[];
extern char overrun_error_msg[];
extern char underrun_error_msg[];
extern char arrow_mode_char[3];

extern int sdr_att_counter;
extern int sdr_nco_counter;
extern int show_map65;
extern float hg_map65_gain;

extern int extio_running;
extern int extio_error;
extern int extio_speed_changed;
extern int extio_command_flag;
extern int extio_show_gui;
extern int ftdi_library_flag;
extern int libusb1_library_flag;
extern int libusb0_library_flag;
extern int rtlsdr_library_flag;
extern int mirisdr_library_flag;
extern int soft66_library_flag;
extern int udev_library_flag;
extern int alsa_library_flag;
extern int old_fdms1_ratenum;

extern int file_rate_correction;
extern int file_center_correction;

void fgraphics_init(void);
void lgraphics_init(void);
void save_screen_image(void);
void process_current_lir_inkey(void);
void *chk_free(void *p);
void await_keyboard(void);
void await_processed_keyboard(void);
void clear_await_keyboard(void);
void clear_keyboard(void);
void test_keyboard(void);
void to_upper_await_keyboard(void);
int to_upper(int chr);
void settextcolor(unsigned char color);
void init_font(int font_type);
void clear_lines(int i, int j);
void show_mouse(void);
void unconditional_hide_mouse(void);
void hide_mouse(int x1,int x2,int iy1,int y2);
void dsp_mouse(void);
void open_mouse(void);
void close_mouse(void);
int open_rx_sndin(int report_errors);
void open_rx_sndout(void);
void close_rx_sndin(void);
void close_rx_sndout(void);
void clear_real2iq(void);
void set_rx_io(void);
void set_tx_io(void);
void free_buffers(void);
void make_sincos(int mo, int sz, COSIN_TABLE *tab);
void make_d_sincos(int mo, int sz, D_COSIN_TABLE *tab);
void make_permute(int mo, int nz, int sz, unsigned short int *perm);
void make_bigpermute(int mo, int nz, int sz, unsigned int *perm);
void make_window(int i, int size, int n_sin, float *fft1_window);
void make_d_window(int i, int size, int n_sin, double *d_fft1_window);
void init_fft(int i,int n, int size,
                          COSIN_TABLE *tab, unsigned short int *permute);
void init_d_fft(int i,int n, int size,
                         D_COSIN_TABLE *tab, unsigned int *permute);
void init_big_fft(int i,int n, int size,
                          COSIN_TABLE *tab, unsigned int *permute);

void init_mmxfft(int size, MMX_COSIN_TABLE *tab);
void make_mmxwindow(int mo, int size, int n, short int *x);
void fftback(int sz, int n, float *x,
             COSIN_TABLE *sc, unsigned short int *pm, int yieldflag);
void big_fftback(int sz, int n, float *x,
             COSIN_TABLE *sc, unsigned int *pm, int yieldflag);
void dual_fftback(int sz, int n, float *x,
                     COSIN_TABLE *sc, unsigned short int *pm, int yieldflag);
void d_dual_fftback(int sz, int n, double *x,
                  D_COSIN_TABLE *sc, unsigned short int *pm, int yieldflag);
void fftforward(int sz, int n, float *x,
                     COSIN_TABLE *sc, unsigned short int *pm, int yieldflag);
void big_fftforward(int sz, int n, float *x,
                          COSIN_TABLE *sc, unsigned int *pm, int yieldflag);
void d_fftback(int sz, int n, double *x,
                                  D_COSIN_TABLE *sc, unsigned int *pm);
void d_fftforward(int sz, int n, double *x,
                   D_COSIN_TABLE *sc, unsigned int *pm);
void fft_iqshift(int size, float *x);
void fft_real_to_hermitian( float *z, int size, int n, COSIN_TABLE *tab);
void asmbulk_of_dual_dif(int size, int n, float *x,
                                         COSIN_TABLE *sincos, int yieldflag);
void asmbulk_of_dif(int size, int n, float *x,
                                        COSIN_TABLE *sincos, int yieldflag);
void bulk_of_dif(int size, int n, float *x, COSIN_TABLE *sincos, int yieldflag);
void d_bulk_of_dif(int size, int n, double *x, D_COSIN_TABLE *sincos, int yieldflag);
void bulk_of_dual_dif(int size, int n, float *x, COSIN_TABLE *sincos, int yieldflag);
void d_bulk_of_dual_dif(int size, int n, double *x, D_COSIN_TABLE *sincos, int yieldflag);
void bulk_of_dit(int size, int n, float *x, COSIN_TABLE *sincos, int yieldflag);
void bulk_of_dual_dit(int size, int n, float *x, COSIN_TABLE *sincos, int yieldflag);
void simdbulk_of_dual_dit(int size, int n, float *x, COSIN_TABLE *sincos);
int check_mmx(void);
void get_buffers(int filldat);
void get_wideband_sizes(void);
int adjust_scale(double *step);
int llsq2(void);
int llsq1(void);
int make_power_of_two(int *i);
int lir_get_filename(int  x, int y, char *name);
int lir_get_integer(int  x, int y, int len, int min, int max);
double lir_get_float(int  x, int y, int len, double min, double max);
void xz(char *s);
void xq(char *s);
void qq(char *s);
void qq1(char *s);
void qq2(char *s);
void tt0(void);
void tt(char *s);
float tt1(void);

void lir_setcross(int x, int y, unsigned char color);
void disksave_start(void);
void disksave_stop(void);
void wavsave_start_stop(int line);

int make_new_signal(int ss, float fq);
void make_wg_file(void);
void make_wg_yfac(void);
void make_hg_file(void);
void baseband_fq_scale(void);
void hires_fq_scale(void);
void make_hg_yscale(void);
void new_fft2_averages(void);
void wide_graph_add_signal(void);
void make_fft3_all(void);
void fft3_mix2(void);
void init_blanker(void);
void new_hg_pol(void);
void timf3_oscilloscope(void);
void wg_waterf_line(void);
void bg_waterf_line(void);
void clear_fft1_filtercorr(void);
void init_fft1_filtercorr(void);
void set_general_parms(char *mode);
void make_rx_audio_output(void);
void change_fft3_avgnum(void);
void make_ad_wttim(void);
void make_fft2_wttim(void);
void make_audio_signal(void);
void make_timing_info(void);
void show_amp_info(void);
void amp_info_texts(void);
void show_timing_info(void);
void timing_info_texts(void);
void deb_timing_info(char *txt);
void update_bar(int x1, int x2, int yzer,
                   int newy, int oldy, unsigned char color, char *buf);
void parabolic_fit(float *amp, float *pos, float yy1, float yy2, float yy3);
void set_daout_parms(void);
void parfile_end(FILE *file);
void lir_text(int x, int y, char *txt);
void lir_pixwrite(int x, int y, char *s);
#if(OSNUM == OSNUM_WINDOWS)
// If fonts_by_GDI is set, then the text output is
// rendered by GDI routines.
extern bool lir_pixwrite_GDI(int x, int y, char *s);
#endif
void help_message(int msg_no);
void help_screen_objects(void);
void select_pol_default(void);
void init_spur_elimination(void);
void cancel_spur_elimination(void);
void eliminate_spurs(void);
void init_spur_spectra(void);
float average_curvature(float *z, int siz);
float average_slope(float *z, int siz);
void spur_phase_parameters(void);
void complex_lowpass(float *zin, float *zout, int nn, int siz);
void remove_phasejumps(float *z, int siz);
int spur_phase_lock(int nx);
int make_spur_freq(void);
void make_spur_pol(void);
void spurspek_norm(void);
int spur_relock(void);
void refine_pll_parameters(void);
int verify_spur_pll(void);
void shift_spur_table(int j);
void coherent_cw_detect(void);
void clear_coherent(void);
void clear_cg_traces(void);
void init_memalloc(MEM_INF *mem, size_t max);
void mem(int num, void *pointer, size_t size, int scratch_size);
size_t memalloc( size_t **handle, char *s);
void memcheck(int n, MEM_INF *mm, size_t **handle);
void ncw_rx(void);
double current_time(void);
void show_name_and_size(void);
int init_wavread(int m);
int init_diskread(int m);
void init_eme_database(void);
void read_eme_database(void);
void init_eme_graph(void);
void calculate_moon_data(void);
void init_freq_control(void);
void erase_numinput_txt(void);
void read_freq_control_data(void);
void wide_fq_scale(void);
void users_open_devices(void);
void users_close_devices(void);
void users_init_mode(void);
void init_phasing_window(void);
void expand_fft1_filtercorr(int cal_n, int cal_size);
void make_filcorrstart(void);
void userdefined_u(void);
void userdefined_q(void);
void check_mouse_actions(void);
void rx_audio_output_start(void);
int read_modepar_file(int type);
void make_modepar_file(int type);
void show_button(BUTTONS *butt, char *s);
void show_wtrfbutton(BUTTONS *butt, char *s);
void txtest_init(void);
void clear_hware_data(void);
void hware_interface_test(void);
void tune(void);
void frequency_readout(void);
void err_restart_da(int errcod);
int tx_setup(void);
void do_nothing(int n);
void await_mouse_event(void);
void graph_borders(WG_PARMS *a,unsigned char color);
void dual_graph_borders(WG_PARMS *a,unsigned char color);
void main_menu(void);
void lir_unlock_mem(void);
int lir_lock_mem(void);
void update_iq_foldcorr(void);
void clear_iq_foldcorr(void);
void lir_join(int no);
void lir_sleep(int us);
void lir_sync(void);
int lir_get_epoch_seconds(void);
void lir_system_times(double *cpu_time, double *total_time);
void rx_file_input(void);
void screen_routine(void);
char lir_inb(int port);
void lir_outb(char bytedat, int port);
float lir_random(void);
void lir_srandom(void);
void narrowband_dsp(void);
void wideband_dsp(void);
void do_fft1c(void);
void do_fft1b(void);
void second_fft(void);
void timf2_routine(void);
void rx_output(void);
void network_send(void);
int lir_tx_output_bytes(void);
void lir_empty_da_device_buffer(void);
int lir_parport_permission(int port);
void sdr14_input(void);
void perseus_input(void);
void sdrip_input(void);
void cloudiq_input(void);
void netafedri_input(void);
void excalibur_input(void);
void rtl2832_input(void);
void mirics_input(void);
void sdrplay2_input(void);
void sdrplay3_input(void);
void bladerf_input(void);
void pcie9842_input(void);
void openhpsdr_input(void);

void cal_filtercorr(void);
void powtim(void);
void rx_adtest(void);
void cal_iqbalance(void);
void do_cal_interval(void);
void user_command(void);
void run_tx_output(void);
void tx_input(void);
void txtest(void);
void lirerr(int errcod);
void lir_rx_dawrite(void);
void free_semaphores(void);
void init_semaphores(void);
void lir_tx_dawrite(char *buf);
void lir_tx_adread(char *buf);
void resume_thread(int no);
void pause_thread(int no);
void lir_move_mouse_cursor(void);
void mouse_nothing(void);
int lir_get_text(int  x, int y, char *txtbuf);
void set_button_states(void);
void init_os_independent_globals(void);
void win_global_uiparms(int wn);
void llin_global_uiparms(int wn);
void flin_global_uiparms(int wn);
void x_global_uiparms(int wn);
#if(OSNUM == OSNUM_WINDOWS)
// Pops up a Win32 Common Dialog to select a monospaced font.
// The font configuration is stored into par_userfont file.
void win_selectfont(void);
#endif /* OS is Windows */
void clear_wide_maxamps(void);
void simulate_keyboard(char chr);
void show_errmsg(int screen_mode);
void lir_sched_yield(void);
void check_filtercorr_direction(void);
void kill_all(void);
void lir_close_serport(void);
int lir_open_serport(int serport_number, int baudrate,int stopbit_flag,int rts_status);
int lir_read_serport(void *s, int bytes);
void pause_screen_and_hide_mouse(void);
void set_button_coordinates(void);
void verify_network(int setup);
void init_network(void);
void netsend_errors(void);
void could_not_create(char *filename, int line);
void init_genparm(int uiupd);
void use_filtercorr_fd(int cal_n, int cal_size,
                            float *cal_corr, float *cal_desired);
void use_filtercorr_td(int cal_size, float *corr, float *desired);
void use_iqcorr(void);
void lir_send_raw16(void);
void lir_send_raw18(void);
void lir_send_raw24(void);
void lir_send_fft1(void);
void lir_send_timf2(void);
void lir_send_fft2(void);
void lir_send_baseb(void);
void lir_send_basebraw(void);
void net_send_slaves_freq(void);
int ms_since_midnight(void);
void close_network_sockets(void);
int lir_write_serport(void *s, int bytes);
void step_rx_frequency(int direction);
int read_txpar_file(void);
void hware_set_ptt(int state);
void lir_mutex_init(void);
void lir_mutex_destroy(void);
void lir_mutex_lock(int no);
void lir_mutex_unlock(int no);
void init_tx_graph(void);
void set_default_spproc_parms(void);
int check_tx_devices(void);
void make_cal_fft1_filtercorr(void);
void raw2wav(void);
void mask_tophat_filter2(float *xi,float *xo, int l, int pa, int pb, int sz);
void mask_tophat_filter1(float *xi,float *xo, int l, int pa, int pb, int sz);
void complete_filename(int i, char *s, char *gif, char *dir, char *fnm);
void update_users_rx_frequency(void);
void copy_rxfreq_to_tx(void);
void copy_txfreq_to_rx(void);
void qt0(void);
void qt1(char *cc);
void qt2(char *cc);
void qt3(void);
void lir_set_title(char *txt);
void clear_button(BUTTONS *butt, int max);
void clear_bfo(void);
void make_bfo(void);
void run_radar(void);
void blocking_rxout(void);
void write_raw_file(void);
float lir_noisegen(int level);
void write_from_msg_file(int *line, int msg_no, int screen_mode, int vernr);
void newcomer_escpress(int clear);
void clear_thread_times(int no);
void make_thread_times(int no);
void change_wg_highest_freq(void);
void change_wg_lowest_freq(void);
void update_snd(int sound_type);
void welcome_msg(void);
void do_syscall(void);
void sys_func(int no);
bool sys_func_async_start(int no);
bool sys_func_async_join(int no);
int read_sdrpar(char *file_name, int max_parm, char **parm_text, int *par);
int show_rx_input_settings(int *line);


int portaudio_startstop(void);
void portaudio_stop(void);
void edit_diskread_times(void);

void lir_init_event(int no);
void lir_close_event(int no);
void lir_set_event(int no);
void lir_await_event(int no);
void wse_setup(void);
void si570_rx_setup(void);
void si570_tx_setup(void);
void elektor_setup(void);
void fcdproplus_setup(void);
void soft66_setup(void);
void wse_rx_freq_control(void);
void wse_tx_freq_control(void);
void wse_rx_amp_control(void);
void elektor_rx_amp_control(void);
void fcdproplus_rx_amp_control(void);
void afedriusb_rx_amp_control(void);
int init_elektor_control_window(void);  
void init_fcdproplus_control_window(void);  
void update_elektor_rx_frequency(void);
void update_fcdproplus_rx_frequency(void);
void si570_rx_freq_control(void);
void elektor_rx_freq_control(void);
void fcdproplus_rx_freq_control(void);
void soft66_rx_freq_control(void);
void afedriusb_rx_freq_control(void);
void compute_converter_parameters(void);
void out_USB2LPT (unsigned char data, unsigned char port);
unsigned char in_USB2LPT( unsigned char port);
void hware_command(void);
void display_rx_input_source(char *str);
int read_wse_parameters(void);
void update_indicator(unsigned char color);
void init_extio(void);
void first_check_extio(void);
void start_extio(void);
void stop_extio(void);
void update_extio_rx_gain(void);
void update_extio_rx_freq(void);
void get_extio_name(char *name);
void extio_input(void);
void get_extio_speed(void);
void confirm_extio(void);
void command_extio_library(int load);
void rtl_starter(void);
void mirisdr_starter(void);
void bladerf_starter(void);
void fdms1_starter(void);
void calibrate_bladerf_rx(void);
void si570_missing(void);
int html_server(void);
void init_netafedri(void);
void afedriusb_setup(void);
void fix_thread_affinities(void);
void setup_thread_affinities(void);
int load_ftdi_library(void);
void unload_ftdi_library(void);
void load_soft66_library(void);
void unload_soft66_library(void);
int load_usb0_library(int msg_flag);
void unload_usb0_library(void);
int load_usb1_library(int msg_flag);
void unload_usb1_library(void);
void load_rtlsdr_library(void);
void unload_rtlsdr_library(void);
int load_extio_library(void);
void unload_extio_library(void);
void load_alsa_library(void);
void unload_alsa_library(void);
void library_error_screen(char* libname, int info);
void init_elad_fdms1(void);
void fdms1_input(void);
void move_rx_frequency(float step, int mode);
void add_mix1_cursor(int num);
void airspy_input(void);
void airspyhf_input(void);
void airspy_starter1(void);
void airspy_starter2(void);
void airspyhf_starter(void);
void awake_screen(void);
int start_iotest(int *line, int mode);
void stop_iotest(int mode);
void select_txout_format(int *line);
void test_rxad(void);
void test_rxda(void);
void test_txda(void);
int get_mic_speed(int *line, int min, int max);
void close_si570(void);
void get_tx_enable(void);
void load_udev_library(void);
void si570_set_ptt(void);
void si570_get_ptt(void);
void hware_hand_key(void);
void check_line(int *line);
void get_tg_parms(void);
#if OPENCL_PRESENT == 1
int ocl_test(void);
#endif
void init_llir(void);
void init_flir(void);
void init_xlir(void);
void screenerr(int *line);
void h_line_error(int x1, int y, int x2);
void getpixel_error(int x, int y);
void setpixel_error(int x, int y);
void putbox_error(int x, int y, int w, int h);
void fillbox_error(int x, int y, int w, int h);
#endif /* UIDEF_H */
