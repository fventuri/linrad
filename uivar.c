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
#include "uidef.h"
#include "thrdef.h"

MIXER_VARIABLES mix1;
MIXER_VARIABLES mix2;

int refresh_screen_flag;
int si570_ptt;
P_SI570 si570;

int extio_error;
int extio_speed_changed;
int extio_command_flag;
int extio_running;
int extio_show_gui;

int sdr;
int use_tx;
int first_keypress;

double wg_lowest_freq;
double wg_highest_freq;

int computation_pause_flag;
int screensave_flag;
int usb2lpt_flag;
float pg_ch2_c1;
float pg_ch2_c2;
int usb2lpt_flag;
int asio_flag;
int wg_freq_adjustment_mode;
int wg_freq_x1;
int wg_freq_x2;
double expose_time;
int no_of_processors;
int hyperthread_flag;
int cpu_rotate;
int thread_rotate_count;
int64_t tickspersec;
int freq_from_file;
float workload;
int workload_counter;
int mouse_inhibit;
double mouse_time_wide;
double mouse_time_narrow;
double time_info_time;
double max_netwait_time;
double net_error_time;
int netwait_reset;
double netstart_time;
ROUTINE par_from_keyboard_routine;
int baseb_reset_counter;
int baseb_control_flag;
int all_threads_started;
int write_log;
int spurcancel_flag;
int spurinit_flag;
int autospur_point;
double recent_time;
int internal_generator_flag;
double internal_generator_phase1;
double internal_generator_phase2;
double internal_generator_shift;
int internal_generator_key;
int internal_generator_att;
int internal_generator_noise;
int truncate_flag;
int rxin_nbits;
int hand_key;
int kill_all_flag;
char lbutton_state;
char rbutton_state;
char new_lbutton_state;
char new_rbutton_state;
int no_of_netslaves;
int netfft1_blknum;
int netraw16_blknum;
int netraw18_blknum;
int netraw24_blknum;
int nettimf2_blknum;
int netfft2_blknum;
int netbaseb_blknum;
int netbasebraw_blknum;
int basebnet_block_bytes;
int basebrawnet_block_bytes;
int next_blkptr_16;
int next_blkptr_18;
int next_blkptr_24;
int next_blkptr_fft1;
int next_blkptr_timf2;
int next_blkptr_fft2;
int next_blkptr_baseb;
int next_blkptr_basebraw;
int netsend_ptr_16;
int netsend_ptr_18;
int netsend_ptr_24;
int netsend_ptr_fft1;
int netsend_ptr_timf2;
int netsend_ptr_fft2;
int netsend_ptr_fft2;
int netsend_ptr_baseb;
int netsend_ptr_basebraw;
int latest_listsend_time;
int net_no_of_errors;
int rxin_block_counter;
int rx_hware_fqshift;
char *msg_filename;
int rx_read_time;
int wavfile_squelch_flag;


char rotor[4]={'-','/','|','\\'};
int soundcard_test_block_count[MAX_IOTEST];
int soundcard_test_cmd_flag[MAX_IOTEST];


int clear_graph_flag;
int lir_errcod;


int font_xsep;
int font_ysep;
int font_w;
int font_h;
int font_size;

int mouse_task;
float min_delay_time;
int savefile_repeat_flag;
int screen_type;
int qz;
float tune_yzer;
int usercontrol_mode;
int workload_reset_flag;
int fft2_to_fft1_ratio;
float baseband_bw_hz;
int baseband_bw_fftxpts;
int lir_inkey;
char *vga_font;
int text_width;
int text_height;
int screen_last_xpixel;
int screen_last_col;
int screen_width;
int screen_height;
int screen_totpix;
int uiparm_change_flag;
USERINT_PARM ui;
SSBPROC_PARM txssb;
CWPROC_PARM txcw;
int mouse_flag;
int mouse_hide_flag;
int mouse_x;
int new_mouse_x;
int mouse_xmax;
int mouse_xmin;
int mouse_y;
int new_mouse_y;
int mouse_ymax;
int mouse_ymin;
int mouse_cursize;
int leftpressed;
int rightpressed;
int mouse_lbutton_x;
int mouse_rbutton_x;
int mouse_lbutton_y;
int mouse_rbutton_y;
int mmx_present;
int simd_present;
int rx_audio_in;
int rx_audio_in2;
int rx_audio_out;
int sdr_att_counter;
int sdr_nco_counter;
int sel_ia;
int sel_ib;

SOUNDCARD_PARM snd[4];

int mixer;
int rx_channels;
int twice_rxchan;
FILE *dmp;
FILE *dmp1;
FILE *dmp2;
FILE *dmp3;
FILE *sndlog;
FILE *wav_file;
FILE *allan_file_ch1;
FILE *allan_file_ch2;
FILE *allan_file_diff;
float total_wttim;
float min_wttim;
int rx_daout_bytes;
int rx_daout_channels;
int timinfo_flag;
int ampinfo_flag;
float measured_da_speed;
float measured_ad_speed;
int rx_mode;
int use_bfo;
double diskread_time;
double eme_time;
int dasync_counter;
float dasync_sum;
float dasync_avg1;
float dasync_avg2;
float dasync_avg3;
double dasync_time;
double dasync_avgtime;
double da_start_time;
int overrun_count;
int wav_write_flag;
int allan_write_flag;
int wav_read_flag;
int audio_dump_flag;
int diskread_pause_flag;
int current_graph_minh;
int current_graph_minw;
int calibrate_flag;
double old_passband_center;
int swmmx_fft1;
int swmmx_fft2;
int swfloat;
int sw_onechan;
size_t memalloc_no;
size_t memalloc_max;
MEM_INF *memalloc_mem;
int eme_flag;
int lir_status;
int rxad_status;
int txad_status;
int txda_status;
int rxda_status;
int allow_wse_parport;
int no_of_rx_overrun_errors;
int no_of_rx_underrun_errors;
int no_of_tx_overrun_errors;
int no_of_tx_underrun_errors;
int count_rx_underrun_flag;
int show_map65;
float hg_map65_gain;
int old_fdms1_ratenum;
int si570_open_switch;
int file_rate_correction;
int file_center_correction;
float fg_truncation_error;
DXDATA *dx;
MEM_INF fft1mem[MAX_FFT1_ARRAYS];
MEM_INF fft3mem[MAX_FFT3_ARRAYS];
MEM_INF basebmem[MAX_BASEB_ARRAYS];
MEM_INF hiresmem[MAX_HIRES_ARRAYS];
MEM_INF afcmem[MAX_AFC_ARRAYS];
MEM_INF txmem[MAX_TXMEM_ARRAYS];
MEM_INF blankermem[MAX_BLANKER_ARRAYS];
MEM_INF radarmem[MAX_RADAR_ARRAYS];
MEM_INF siganalmem[MAX_SIGANAL_ARRAYS];
MEM_INF allanmem[MAX_ALLAN_ARRAYS];
double flowcnt[MAX_FLOWCNT];
double flowtime;
double old_flowtime;
int mailbox[64];
int udev_library_flag;
int gpu_fft1_batch_size;

NET_RX_STRUCT net_rxdata_16;
NET_RX_STRUCT net_rxdata_18;
NET_RX_STRUCT net_rxdata_24;
NET_RX_STRUCT net_rxdata_fft1;
NET_RX_STRUCT net_rxdata_timf2;
NET_RX_STRUCT net_rxdata_fft2;
NET_RX_STRUCT net_rxdata_baseb;
NET_RX_STRUCT net_rxdata_basebraw;

SLAVE_MESSAGE slave_msg;

char savefile_parname[SAVEFILE_MAX_CHARS+4];  //chars + _ag + ending zero.
char savefile_name[SAVEFILE_MAX_CHARS+1];

NET_FD netfd;
char press_f1_for_info[]="Press F1 or '!' for more info.";
char userint_filename[]="par_userint";
char network_filename[]="par_network";
char *rx_logfile_name="soundboard_rx_init.log";
char *rxpar_filenames[MAX_RX_MODE]={"par_wcw",
                         "par_cw",
                         "par_hsms",
                         "par_ssb",
                         "par_fm",
                         "par_am",
                         "par_qrss",
                         "par_txtest",
                         "par_test",
                         "par_tune",
                         "par_radar"};
char newcomer_rx_modes[MAX_RX_MODE]={0,1,0,1,1,10,0,0,0,0};

char *rxmodes[MAX_RX_MODE]={"Weak signal CW",
                 "Normal CW",
                 "Meteor scatter CW",
                 "SSB",
                 "FM",
                 "AM",
                 "QRSS CW",
                 "TX TEST",
                 "SOUNDCARD TEST MODE",
                 "ANALOG HARDWARE TUNE",
                 "RADAR"};

#define A MAX_MIX1
#define B MAX_FFT1_VERNR
#define C MAX_FFT1_BCKVERNR
#define G MAX_FFT2_VERNR
#define K 12000

#define D 2000
#define E 1000000
#define F 11025
#define H 200
#define M 8000
#define N 10000
#define P 48000
#define A1 100
#define B5 5000
#define Q MAX_FFT1_THREADS
int genparm[MAX_GENPARM+2];
//        f  f  f w c 2      b  b           e      A e   s    m        D     d e  b b
//    f f f  f  f g o n b l  a  w f f  f  s n   A  F n m p  m i f  b   A  D  e x  g g
//  f f f t  t  t s r d a i  c  f f f  w  t a   F  C . a u  i x f  a   m  A  f p  . .
//  f t t 1  1  1 a r f c m  k  a t t  d  2 b   C  d M x r  x 1 t  s   a  s  . a  b m
//  t 1 1 t  s  a t s f k .  f  c 2 2  a  t l   l  r o s t  1 c 3  t   r  p  m n  l e
//  1 w v h  t  m l p t v m  f  t w v  t  i A   o  i r p i  r h w  i   g  e  o d  a t
//  b i e r  o  p i e e e a  t  o i e  t  m F   c  f s u m  e a i  m   i  e  d e  n .
//  w n r s  r  l m k n r x  N  r n r  N  e C   k  t e r e  d n n  e   n  d  e r  k f
//                        1  1  1 1 1  1  1 1   1  1 2 2 2  2 2 2  2   2  2  2 3    i
//  0 1 2 3  4  5 6 7 8 9 0  1  2 3 4  5  6 7   8  9 0 1 2  3 4 5  6   7  8  9 0    l
int genparm_min[MAX_GENPARM]=
 {  0,0,0,0, 0,1, 0,0,0,0,H, 2, 0,0,0, 2, 0,0,  0, 0,0,0,1, 0,1,1, 2,  0,B5, 0,0, 0,0};
int genparm_max[MAX_GENPARM]=
 {  E,9,B,Q, D,E,99,3,1,C,E,99,10,4,G,16, D,2,800,B5,1,E,H,12,A,9, P,  N, E, H,9,99,9};
int genparm_default[MAX_RX_MODE][MAX_GENPARM]={
// Weak signal CW
 { 2500,2,0,0, 4,D, 0,0,0,0,K, 6, 2,2,0, 8,20,1,150,A1,0,0,5, 6,1,2,20,200, P, 1,3, 0,0},
// Normal CW
 {10000,2,0,0, 1,D, 0,0,0,0,K, 6, 2,2,0, 7, 5,0,150,A1,0,0,5, 4,1,2, 5,100, P, 1,3, 0,0},
// Meteor scatter CW
 {30000,2,0,0, 1,D, 0,0,0,0,K, 6, 2,2,0, 7, 5,0,150,A1,0,0,5, 4,1,2, 5,100, P, 1,3, 0,0},
// SSB
 {10000,2,0,0, 1,D, 0,0,0,0,K, 6, 2,2,0, 7, 5,0,  0,A1,0,0,5, 4,1,2, 2,100, P, 1,3, 0,0},
// FM
 {10000,2,0,0, 1,D, 0,0,0,0,K, 6, 2,2,0, 7, 5,0,  0,A1,0,0,5, 2,1,2, 2,200, P, 1,3, 0,0},
// AM
 { 5000,2,0,0, 8,D, 0,0,0,0,K, 6, 0,2,0, 7, 5,0,350,40,0,0,5, 2,1,2, 2,200, P, 1,3, 0,0},
// QRSS CW
 { 2500,2,0,0, 1,D, 0,0,0,0,K, 6, 2,2,0, 7, 5,0,150,A1,0,0,5, 5,1,2, 2,500, P, 1,3, 0,0},
// Tx test
 { 5000,2,0,0, 1,D, 0,0,0,0,K, 6, 2,2,0, 7, 5,0,  0,A1,0,0,5, 4,1,2, 2,500, P, 1,3, 0,0},
// Soundcard test
 { 8300,2,0,0, 1,D, 0,0,0,0,K, 6, 2,2,0, 7, 5,0,  0,A1,0,0,5, 4,1,2, 2,500, P, 1,3, 0,0},
// Analog hardware tune
 { 8500,2,0,0, 1,D, 0,0,0,0,K, 6, 2,2,0, 7, 5,0,  0,A1,0,0,5, 4,1,2, 2,500, P, 1,3, 0,0},
// Radar
 {20000,2,0,0, 4,D, 0,0,0,0,K, 6, 2,2,0, 8, 5,0,  0,A1,0,0,5, 4,1,2, 2,500, P, 1,3, 0,0}};

char *genparm_text[MAX_GENPARM+2]={"First FFT bandwidth (Hz)",          //0
                      "First FFT window (power of sin)",                //1
                      "First forward FFT version",                      //2
                      "First FFT no of b threads",                      //3
                      "First FFT storage time (s)",                     //4
                      "First FFT amplitude",                            //5
                      "Main waterfall saturate limit",                  //6
                      "Enable correlation spectrum",                    //7
                      "Enable second FFT",                              //8
                      "First backward FFT version",                     //9
                      "Sellim maxlevel",                                //10
                      "First backward FFT att. N",                      //11
                      "Second FFT bandwidth factor in powers of 2",     //12
                      "Second FFT window (power of sin)",               //13
                      "Second forward FFT version",                     //14
                      "Second forward FFT att. N",                      //15
                      "Second FFT storage time (s)",                    //16
                      "Enable AFC/SPUR/DECODE",                         //17
                      "AFC lock range Hz",                              //18
                      "AFC max drift Hz/minute",                        //19
                      "Enable Morse decoding",                          //20
                      "Max no of spurs to cancel",                      //21
                      "Spur timeconstant (0.1sec)",                     //22
                      "First mixer bandwidth reduction in powers of 2", //23
                      "First mixer no of channels",                     //24
                      "Third FFT window (power of sin)",                //25
                      "Baseband storage time (s)",                      //26
                      "Output delay margin (ms)",                       //27
                      "Output sampling speed (Hz)",                     //28
                      "Default output mode",                            //29
                      "Audio expander exponent",                        //30
                      "Baseband waterfall saturate limit",              //31
                      "No of averages in meter.txt",                    //32 
                      "A/D speed",                                      //33
                      "Check"};


char newco_genparm[MAX_GENPARM]={1, //First FFT bandwidth (Hz)            0
                                 0, //First FFT window (power of sin)     1
                                 0, //First forward FFT version           2
                                 0, //First FFT no of threads             3
                                 1, //First FFT storage time (s)          4
                                 1, //First FFT amplitude                 5
                                 0, //Main waterfall saturate limit       6
                                 0, //Enable correlation spectra          7
                                 0, //Enable second FFT                   8
                                 0, //First backward FFT version          9
                                 0, //Sellim maxlevel                    10
                                 0, //First backward FFT att. N          11
                                 0, //Second FFT bandwidth factor...     12
                                 0, //Second FFT window (power of sin)   13
                                 0, //Second forward FFT version         14
                                 0, //Second forward FFT att. N          15
                                 0, //Second FFT storage time (s)        16
                                 0, //Enable AFC/SPUR/DECODE (2=auto     17
                                 0, //AFC lock range Hz                  18
                                 0, //AFC max drift Hz/minute            19
                                 0, //Enable Morse decoding              20
                                 0, //Max no of spurs to cancel          21
                                 0, //Spur timeconstant (0.1sec)         22
                                 1, //First mixer bandwidth reduction..  23
                                 0, //First mixer no of channels         24
                                 0, //Third FFT window (power of sin)    25
                                 1, //Baseband storage time (s)          26
                                 0, //Output delay margin (ms)           27
                                 1, //Output sampling speed (Hz)         28
                                 0, //Default output mode                29
                                 0, //Audio expander exponent            30
                                 0, //Baseband waterfall saturate limit  31
                                 0};//No of averages in s-meter.txt      32 


char *si570_parm_text[MAX_SI570_PARM]={"Libusb version",
                                       "SI570 RX firmware",
                                       "RX USB index",
                                       "Min RX freq",
                                       "Max RX freq",
                                       "RX LO multiplier",
                                       "RX LO freq offset",
                                       "RX passband direction",
                                       "RX freq adjust ppm",
                                       "RX serial id 1",
                                       "RX serial id 2",
                                       "RX serial id 3",
                                       "RX serial id 4",
                                       "SI570 TX firmware",
                                       "TX USB index",
                                       "Min TX freq",
                                       "Max TX freq",
                                       "TX LO multiplier",
                                       "TX LO freq offset",
                                       "TX passband direction",
                                       "TX freq adjust ppm",
                                       "TX serial id 1",
                                       "TX serial id 2",
                                       "TX serial id 3",
                                       "TX serial id 4"
                                       }; 


char *ssbprocparm_text[MAX_SSBPROC_PARM]={"Lowest freq",        //0
                                          "Highest freq",       //1
                                          "Slope",              //2
                                          "Bass",               //3
                                          "Treble",             //4
                                          "Mic gain",           //5
                                          "Mic AGC time",       //6
                                          "Mic F threshold",    //7
                                          "Mic T threshold",    //8
                                          "RF1 gain",           //9
                                          "ALC time",           //10 
                                          "Block time",         //11
                                          "Delay margin",       //12          
                                          "Freq shift",         //13 
                                          "Check"};             //14

char *cwprocparm_text[MAX_CWPROC_PARM]={"Rise time",        //0
                                        "Hand key",         //1
                                        "Tone key",         //2
                                        "ASCII input",      //3
                                        "Radar interval",   //4
                                        "Radar pulse",      //5
                                        "Delay margin",     //6
                                        "Carrier freq",     //7
                                        "Sidetone ampl",    //8
                                        "Sidetone freq",    //9
                                        "wcw mute on",      //10
                                        "wcw mute off",     //11
                                        "ncw mute on",      //12
                                        "ncw mute off",     //13
                                        "ms mute on",       //14
                                        "ms mute off",      //15
                                        "qrss mute on",     //16
                                        "qrss mute off",    //17
                                        "radar mute on",    //18
                                        "radar mute off",   //19 
                                        "Check"};           //20

char *uiparm_text[MAX_UIPARM]={"vga mode",           //0
                               "Screen width (%)",   //1
                               "Screen height (%)",  //2
                               "font scale",         //3
                               "mouse speed",        //4
                               "Max DMA rate",       //5
                               "Process priority",   //6
                               "Native ALSA",        //7
                               "Rx input mode",      //8
                               "Rx rf channels",     //9
                               "Rx ad channels",     //10
                               "Rx ad speed",        //11
                               "Rx ad device no",    //12
                               "Rx ad mode",         //13
                               "Rx da mode",         //14 
                               "Rx da device no",    //15
                               "Rx min da speed",    //16
                               "Rx max da speed",    //17
                               "Rx max da channels", //18
                               "Rx max da bytes",    //19
                               "Rx min da channels", //20
                               "Rx min da bytes",    //21
                               "Rx soundcard radio", //22
                               "Converter Hz",       //23
                               "Converter MHz",      //24
                               "Converter mode",     //25
                               "network flag",       //26
                               "Tx ad speed",        //27
                               "Tx da speed",        //28
                               "Tx ad device no",    //29
                               "Tx da device no",    //30
                               "Tx da channels",     //31
                               "Tx ad channels",     //32
                               "Tx da bytes",        //33
                               "Tx ad bytes",        //34
                               "Tx enable",          //35
                               "Tx pilot tone dB",   //36
                               "Tx pilot microsec.", //37
                               "Tx soundcard radio", //38
                               "Operator skil",      //39
                               "Max blocked CPUs",   //40
                               "Timer resolution",   //41
                               "Autostart",          //42
                               "Rx ad latency",      //43
                               "Rx da latency",      //44
                               "Tx ad latency",      //45
                               "Tx da latency",      //46
                               "Sample shift",       //47
                               "Min DMA rate",       //48
                               "Use ExtIO",          //49
                               "ExtIO type",         //50
                               "Transceiver mode",   //51   
                               "PTT control",        //52
                               "Background colour",  //53
                               "check"};             //54


char *ag_intpar_text[MAX_AG_INTPAR]={"ytop",               //1
                                     "ybottom",            //2
                                     "xleft",              //3
                                     "xright",             //4
                                     "mode",               //5
                                     "avgnum",             //6
                                     "window",             //7
                                     "fit points",         //8
                                     "delay",              //9
                                     "check"};             //10

char *ag_floatpar_text[MAX_AG_FLOATPAR]={"min S/N",        //1
                                         "search range",   //2
                                         "lock_range",     //3
                                         "freq range"};    //4

char *rx_soundcard_radio_names[MAX_RX_SOUNDCARD_RADIO]={"Undef",    //0
                                                  "Undef reversed", //1 
                                                  "WSE",            //2
                                                  "SI570",          //3
                                                  "Soft66",         //4
                                                  "Elektor",        //5
                                                  "FCD Pro Plus",   //6
                                                  "Afedri USB"};    //7

char *tx_soundcard_radio_names[MAX_TX_SOUNDCARD_RADIO]={"Undef",    //0
                                                  "Undef reversed", //1 
                                                  "WSE",            //2
                                                  "SI570"};         //3
                                                  
char *wg_intpar_text[MAX_WG_INTPAR]={"ytop",               //1
                                     "ybottom",            //2
                                     "xleft",              //3
                                     "xright",             //4
                                     "yborder",            //5
                                     "xpoints per pixel",  //6
                                     "pixels per xpoint",  //7
                                     "first xpoint",       //8
                                     "xpoints",            //9
                                     "avg1num",            //10
                                     "spek avgnum",        //11
                                     "waterfall avgnum",   //12
                                     "spur_inhibit",       //13
                                     "check"};             //14

char *wg_floatpar_text[MAX_WG_FLOATPAR]={"yzero",          //1
                                         "yrange",         //2
                                         "wat. db zero",   //3
                                         "wat. db gain"};  //4

char *vg_intpar_text[MAX_VG_INTPAR]={"ytop",               //1
                                     "ybottom",            //2
                                     "xleft",              //3
                                     "xright",             //4
                                     "points per decade",  //5
                                     "Ymin neg pow10",     //6
                                     "Ymax neg pow10",     //7
                                     "Clear traces",       //8
                                     "Mode",               //9 
                                     "Check"};             //10
                                      
char *vg_floatpar_text[MAX_VG_FLOATPAR]={"Min tau",         //1
                                         "Max tau"};        //2

char *vgf_intpar_text[MAX_VGF_INTPAR]={"ytop",               //1
                                       "ybottom",            //2
                                       "xleft",              //3
                                       "xright",             //4
                                       "Check"};              //5

char *vgf_floatpar_text[MAX_VGF_FLOATPAR]={"Freq gain",      //1
                                           "Time step"};     //2

char *hg_intpar_text[MAX_HG_INTPAR]={"ytop",                //1
                                     "ybottom",             //2
                                     "xleft",               //3
                                     "xright",              //4
                                     "mode (dumb)",         //5
                                     "mode (clever)",       //6
                                     "timf2 display",       //7
                                     "timf2 display lines", //8
                                     "timf2 display hold",  //9
                                     "spek avgnum",         //10
                                     "limit (dumb)",        //11
                                     "limit (clever)",      //12
                                     "spek gain",           //13
                                     "spek zero",           //14
                                     "map65 gain",          //15
                                     "map65 strong",        //16
                                     "Sellim par1",         //17
                                     "Sellim par2",         //18
                                     "Sellim par3",         //19
                                     "Sellim par4",         //20
                                     "Sellim par5",         //21
                                     "Sellim par6",         //22
                                     "Sellim par7",         //23
                                     "Sellim par8",         //24
                                     "check"};              //25

char *hg_floatpar_text[MAX_HG_FLOATPAR]={"factor (dumb)",          //1
                                         "factor (clever)",        //2
                                         "sellim fft1 S/N",        //3
                                         "sellim fft2 S/N",        //4
                                         "timf2 display gain wk",  //5
                                         "timf2 display gain st"}; //6

char *bg_intpar_text[MAX_BG_INTPAR]={"ytop",                //1
                                     "ybottom",             //2
                                     "xleft",               //3
                                     "xright",              //4
                                     "yborder",             //5
                                     "fft3 avgnum",         //6
                                     "pixels/point",        //7
                                     "coh factor",          //8
                                     "delay points",        //9
                                     "AGC flag",            //10
                                     "AGC attack",          //11
                                     "AGC release",         //12
                                     "AGC hang",            //13
                                     "Waterfall avgnum",    //14
                                     "Mouse wheel step",    //15
                                     "Oscill ON",           //16
                                     "Arrow mode",          //17
                                     "Filter FIR/FFT",      //18
                                     "Filter shift",        //19
                                     "FM mode",             //20
                                     "FM subtract",         //21
                                     "FM factor",           //22
                                     "ch2 phase",           //23
                                     "Squelch level",       //24
                                     "Squelch time",        //25
                                     "Squelch point",       //26
                                     "check"};              //27

char *bg_floatpar_text[MAX_BG_FLOATPAR]={"filter flat",          //1
                                         "filter curved",        //2
                                         "yzero",                //3
                                         "yrange",               //4
                                         "dB/pixel",             //5
                                         "yfac pwr",             //6
                                         "yfac log",             //7
                                         "bandwidth",            //8
                                         "first freq",           //9
                                         "BFO freq",             //10
                                         "Output gain",          //11
                                         "Waterfall gain",       //12
                                         "Waterfall zero",       //13
                                         "Oscill gain"};         //14

char *pg_intpar_text[MAX_PG_INTPAR]={"ytop",            //1
                                     "ybottom",         //2
                                     "xleft",           //3
                                     "xright",          //4
                                     "adapt",           //5
                                     "avg",             //6
                                     "startpol",        //7
                                     "enable phasing",  //8
                                     "azimuth",         //9
                                     "size",            //10
                                     "check"};          //11

char *pg_floatpar_text[MAX_PG_FLOATPAR]={"angle",       //1
                                         "c1",          //2
                                         "c2",          //3
                                         "c3",          //4
                                         "ch2 amp",     //5
                                         "ch2_phase"};  //6

char *sg_intpar_text[MAX_SG_INTPAR]={"ytop",            //1
                                     "ybottom",         //2
                                     "xleft",           //3
                                     "xright",          //4
                                     "xscale",          //5
                                     "mode",            //6
                                     "avg",             //7
                                     "fft_n",           //8
                                     "ymax",            //9
                                     "check"};          //10
                                     
char *sg_floatpar_text[MAX_SG_FLOATPAR]={"xgain",        //1
                                         "ygain"};       //2

char *cg_intpar_text[MAX_CG_INTPAR]={"ytop",             //1
                                     "ybottom",          //2
                                     "xleft",            //3
                                     "xright",           //4
                                     "Meter graph on",   //5
                                     "Oscill on"};       //6

char *xg_intpar_text[MAX_XG_INTPAR]={"ytop",             //1
                                     "ybottom",          //2
                                     "xleft",            //3
                                     "xright"};          //4


char *xg_floatpar_text[MAX_XG_FLOATPAR]={"Ampl ratio",     //1
                                         "Phase shift",    //2
                                         "M1 ampl",        //3
                                         "M1 phase",       //4 
                                         "M2 ampl",        //5
                                         "M2 phase",       //6 
                                         "M3 ampl",        //7
                                         "M3 phase"};      //8 

char *cg_floatpar_text[MAX_CG_FLOATPAR+1]={""};

char *mg_intpar_text[MAX_MG_INTPAR]={"ytop",             //1
                                     "ybottom",          //2
                                     "xleft",            //3
                                     "xright",           //4
                                     "Scale type",       //5
                                     "Avgnum",           //6
                                     "Tracks",           //7
                                     "Check"};           //8

char *mg_floatpar_text[MAX_MG_FLOATPAR]={"ygain",             //1
                                         "yzero",             //2
                                         "Cal dBm",           //3
                                         "Cal S/N",           //4 
                                         "Cal S/N sigshift",  //5 
                                         "Cal dbhz",          //6 
                                         "Cal S-units"};      //7

char *eg_intpar_text[MAX_EG_INTPAR]={"ytop",            //1
                                     "ybottom",         //2
                                     "xleft",           //3
                                     "xright",          //4
                                     "minim"};          //5

char *eg_floatpar_text[MAX_EG_FLOATPAR+1]={""};


char *fg_intpar_text[MAX_FG_INTPAR]={"ytop",            //1
                                     "ybottom",         //2
                                     "xleft",           //3
                                     "xright",          //4
                                     "yborder",         //5
                                     "direction",       //6
                                     "gain",            //7
                                     "gain_inc"};       //8

char *fg_floatpar_text[MAX_FG_FLOATPAR]={"fq inc",      //1
                                         "Center freq", //2
                                         "Tune freq"};  //3

char *tg_intpar_text[MAX_TG_INTPAR]={"ytop",            //1
                                     "ybottom",         //2
                                     "xleft",           //3
                                     "xright",          //4
                                     "spproc no",       //5
                                     "direction"};      //6

char *tg_floatpar_text[MAX_TG_FLOATPAR]={"level dB",    //1
                                         "freq",        //2
                                         "fq inc",      //3
                                         "band fq"};    //4

char *net_intpar_text[MAX_NET_INTPAR]={"send group",       //1
                                       "receive group",    //2
                                       "port",             //3
                                       "send_raw",         //4
                                       "send_fft1",        //5
                                       "send_fft2",        //6
                                       "send_timf2",       //7
                                       "send_baseb16",     //8
                                       "send_baseb24",     //9
                                       "receive_raw",      //10
                                       "receive_fft1",     //11
                                       "receive_baseb16",  //12
                                       "receive_baseb24",  //13
                                       "receive_timf2",    //14
                                       "check"};           //15

char *rg_intpar_text[MAX_RG_INTPAR]={"ytop",            //1
                                     "ybottom",         //2
                                     "xleft",           //3
                                     "xright"           //4
                                     };

char *rg_floatpar_text[MAX_RG_FLOATPAR]={"Time",        //1
                                         "Zero",        //2
                                         "Gain"         //3
                                         };

char *ocl_intpar_text[MAX_GPU_INTPAR]={"fft1 platform",
                                       "fft1_device",
                                       "fft1 batch N"
                                      };

char *ocl_floatpar_text[MAX_GPU_FLOATPAR+1]={""};



char modes_man_auto[]={'-','A','M'};
char arrow_mode_char[3]={'T', 'P','B'};
char *press_any_key={"Press any key"};
char *press_enter={"Press ENTER"};
char *audiomode_text[4]={
         "One RF, one audio channel (normal audio)",
         "One RF, two audio channels (direct conversion)",
 "Two RF, two audio channels (normal audio, adaptive polarization)",
 "Two RF, four audio channels (direct conversion, adaptive polarization)"};
char remind_parsave[]="(Do not forget to save with ""W"" on the main menu)";
char overrun_error_msg[]=" OVERRUN_ERROR ";
char underrun_error_msg[]=" UNDERRUN_ERROR ";

