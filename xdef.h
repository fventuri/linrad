
extern unsigned char *mempix_char;
extern unsigned short int *mempix_shi;
extern int first_mempix;
extern int last_mempix;
extern int first_mempix_copy;
extern int last_mempix_copy;
extern char first_mempix_flag;
extern char last_mempix_flag;
extern int mouse_wheel_flag;
extern pthread_t thread_identifier_process_event;
extern pthread_t thread_identifier_refresh_screen;

extern int process_event_flag;
extern int expose_event_done;
extern int shift_key_status;
extern int alt_key_status;
extern int color_depth;
extern unsigned char *xpalette;
void ui_setup(void);
void thread_process_event(void);
void thread_refresh_screen(void);
void lir_remove_mouse_thread(void);
void store_in_kbdbuf(int c);
void wxmouse(void);

#if HAVE_X11 == 1
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xutil.h>
#endif

#if HAVE_SHM == 1
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#if HAVE_OSS == 1
extern audio_buf_info rx_da_info;
extern audio_buf_info tx_da_info;
extern audio_buf_info tx_ad_info;
#endif


#if HAVE_SVGALIB == 1
extern struct termios termios_pp;
extern int terminal_flag;
extern int gpm_fd;
extern struct fb_cmap *fb_palette;
extern int framebuffer_handle;
extern size_t framebuffer_screensize;
#endif

extern pthread_t thread_identifier_kill_all;
extern pthread_t thread_identifier_keyboard;
extern pthread_t thread_identifier_mouse;
extern pthread_t thread_identifier_main_menu;
extern pthread_t thread_identifier_html_server;
extern pthread_t thread_identifier[THREAD_MAX];
extern pthread_mutex_t lir_event_mutex[MAX_LIREVENT];
extern int lir_event_flag[MAX_LIREVENT];
extern pthread_cond_t lir_event_cond[MAX_LIREVENT];
extern int serport;
extern char *behind_mouse;
void thread_main_menu(void);
void thread_rx_adinput(void);
void thread_rx_raw_netinput(void);
void thread_rx_fft1_netinput(void);
void thread_rx_file_input(void);
void thread_rx_output(void);
void thread_screen(void);
void thread_tx_input(void);
void thread_tx_output(void);
void thread_wideband_dsp(void);
void thread_second_fft(void);
void thread_timf2(void);
void thread_narrowband_dsp(void);
void thread_user_command(void);
void thread_txtest(void);
void thread_powtim(void);
void thread_rx_adtest(void);
void thread_cal_interval(void);
void thread_cal_filtercorr(void);
void thread_sdr14_input(void);
void thread_tune(void);
void thread_kill_all(void);
void thread_keyboard(void);
void lthread_mouse(void);
void thread_lir_server(void);
void thread_perseus_input(void);
void thread_radar(void);
void thread_blocking_rxout(void);
void thread_mix2(void);
void thread_fft3(void);
void thread_syscall(void);
void thread_sdrip_input(void);
void thread_cloudiq_input(void);
void thread_hware_command(void);
void thread_excalibur_input(void);
void thread_extio_input(void);
void thread_write_raw_file(void);
void thread_rtl2832_input(void);
void thread_rtl_starter(void);
void thread_mirics_input(void);
void thread_bladerf_input(void);
void thread_pcie9842_input(void);
void thread_openhpsdr_input(void);
void thread_mirisdr_starter(void);
void thread_bladerf_starter(void);
void thread_html_server(void);
void thread_netafedri_input(void);
void thread_do_fft1c(void);
void thread_fft1b(void);
void thread_fdms1_input(void);
void thread_fdms1_starter(void);
void thread_airspy_input(void);
void thread_network_send(void);
void thread_airspyhf_input(void);
void thread_sdrplay2_input(void);
void thread_sdrplay3_input(void);
int investigate_cpu(void);
void print_procerr(int xxprint);
extern pthread_mutex_t linux_mutex[MAX_LIRMUTEX];
extern ROUTINE thread_routine[THREAD_MAX];
