
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0502
#endif

// Always include stdio.h, math.h stdlib.h stdint.h and errno.h
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

#define STDERR stderr

#define INITIAL_SKIP_FLAG_MAX 25

#define CPU_INTEL 0
#define CPU_ARM 1


#define CALLBACK_CMD_START 3
#define CALLBACK_ANSWER_AWAKE 2
#define CALLBACK_CMD_ACTIVE 1
#define CALLBACK_CMD_CLOSE 0
#define CALLBACK_ANSWER_CLOSED -1
#define CALLBACK_ERROR -2

#define TX_DA_MARGIN 0.97

// MAX_MOUSE_CURSIZE is used by svgalib and fbdev
#define MAX_MOUSE_CURSIZE 50

#define GPU_OCL 1
#define GPU_CUDA 2

// Structure for soundcard info.
typedef struct {
int no_of_blocks;
int valid_frames;
int min_valid_frames;
int valid_bytes;
int empty_frames;
int empty_bytes;
int tot_frames;
int tot_bytes;
int framesize;
int block_bytes;
int block_frames;
float interrupt_rate;
int open_flag;
}SOUNDCARD_PARM;

#define CONVERTER_USE 1
#define CONVERTER_UP 2
#define CONVERTER_LO_BELOW 4


#define RXAD 0
#define RXDA 1
#define TXDA 2
#define TXAD 3
#define MAX_IOTEST 3


#define RX_SOUNDCARD_RADIO_UNDEFINED 0
#define RX_SOUNDCARD_RADIO_UNDEFINED_REVERSED 1
#define RX_SOUNDCARD_RADIO_WSE 2
#define RX_SOUNDCARD_RADIO_SI570 3
#define RX_SOUNDCARD_RADIO_SOFT66 4
#define RX_SOUNDCARD_RADIO_ELEKTOR 5
#define RX_SOUNDCARD_RADIO_FCDPROPLUS 6
#define RX_SOUNDCARD_RADIO_AFEDRI_USB 7
#define MAX_RX_SOUNDCARD_RADIO 8

#define TX_SOUNDCARD_RADIO_UNDEFINED 0
#define TX_SOUNDCARD_RADIO_UNDEFINED_REVERSED 1
#define TX_SOUNDCARD_RADIO_WSE 2
#define TX_SOUNDCARD_RADIO_SI570 3
#define MAX_TX_SOUNDCARD_RADIO 4

#define TRUE 1
#define FALSE 0
#define YELLOW 14
#define LIGHT_GREEN 10
#define LIGHT_RED 12

#define MAX_FFT1_AVG1 9
#define FFT1_SMALL 0.00000001F

#define NO_REDRAW_BLOCKS 16
// *******************************************
// Global numerical values
#define PI_L 3.1415926535897932
#define BIGFLOAT 300000000000000000000000000000000000000.F
#define BIGDOUBLE 300000000000000000000000000000000000000.
// Signal power to the power of four may become a very large number.
// Define something small to multiply by to prevent overflows.
#define P4SCALE 0.000000000000000000001
#define P2SCALE 0.00000000001
// values for lir_status
// Memory errors are larger than LIR_OK.
// Other abnormal conditions smaller than LIR_OK.
#define LIR_OK 0
#define LIR_TUNEERR 1
#define LIR_FFT1ERR 2
#define LIR_SAVE_UIPARM 4
#define LIR_NEW_SETTINGS -1
#define LIR_SPURERR -2
#define LIR_PARERR -3
#define LIR_NEW_POL -4
#define LIR_TUNEERR2 -5
#define LIR_POWTIM -6
#define LIR_PA_FLAG_ERROR -7
#define LIR_PA_NO_SUCH_DEVICE -8
#define LIR_PA_FORMAT_NOT_SUPPORTED -9
#define LIR_PA_OPENSTREAM_FAILED -10
#define LIR_PA_START_STREAM_FAILED -11
#define LIR_DLL_FAILED -12
#define LIR_TXPAR_FAILED -13
// *******************************************
// Fixed array dimensions depend on these defines:



#define MODE_WCW 0
#define MODE_NCW 1
#define MODE_HSMS 2
#define MODE_SSB 3
#define MODE_FM 4
#define MODE_AM 5
#define MODE_QRSS 6
// Note that modes below MODE_TXTEST are allowed to call
// users tx routines, but that MODE_TXTEST and higher may
// call users_init_mode, but users_hwaredriver is responsible
// for checking rx_mode < MODE_TXTEST and not do any tx
// activities or screen writes.
#define MODE_TXTEST 7
#define MODE_RX_ADTEST 8
#define MODE_TUNE 9
#define MODE_RADAR 10
// Note that MAX_RX_MODES can be up to 12. 'L' is reserved
// for an new mode that might be desireable to add.  
#define MAX_RX_MODE 11

//******************************************************************
// Information about what device drivers to use for soundcards
// is stored in ui.use_alsa. The name has became misleading,
// but a change would make par_userint incompatible once again...
// Use the following masks for testing on ui.use_alsa
#define NATIVE_ALSA_USED 1
#define PORTAUDIO_RX_IN 2
#define PORTAUDIO_RX_OUT 4
#define PORTAUDIO_TX_IN 8
#define PORTAUDIO_TX_OUT 16
#define PORTAUDIO_DLL_VERSION (32+64+128)
#define RXAD_USE_EXTENSIBLE 256
#define RXDA_USE_EXTENSIBLE 512
#define TXAD_USE_EXTENSIBLE 1024
#define TXDA_USE_EXTENSIBLE 2048
#define SOUND_SYSTEM_DEFINED 2096
#define TXMODE_OFF 0
#define TXMODE_CW 1
#define TXMODE_SSB 2

#define MAX_COLOR_SCALE 23
#define MAX_SCRO 50
#define LLSQ_MAXPAR 25
#define MAX_ADCHAN 4
#define MAX_MIX1 8           // No of different signals to process simultaneously
#define MAX_AD_CHANNELS 4
#define MAX_RX_CHANNELS 2
#define DAOUT_MAXTIME 1.
#define SPUR_N 3
#define SPUR_SIZE (1<<SPUR_N)
#define NO_OF_SPUR_SPECTRA 256
#define MAX_FFT1_ARRAYS 85
#define MAX_FFT3_ARRAYS 35
#define MAX_BASEB_ARRAYS 40
#define CG_MAXTRACE 24
#define MAX_HIRES_ARRAYS  8
#define MAX_AFC_ARRAYS 20
#define MAX_TXMEM_ARRAYS 40
#define MAX_BLANKER_ARRAYS 15
#define MAX_RADAR_ARRAYS 15
#define MAX_SIGANAL_ARRAYS 40
#define USR_NORMAL_RX 0
#define USR_TXTEST 1
#define USR_POWTIM 2
#define USR_ADTEST 3
#define USR_IQ_BALANCE 4
#define USR_CAL_INTERVAL 5
#define USR_CAL_FILTERCORR 6
#define USR_TUNE 7

#define UNDEFINED_DEVICE_CODE -1
#define DISABLED_DEVICE_CODE -2
#define NETWORK_DEVICE_CODE -3
#define EXTIO_DEVICE_CODE -4
#define SPECIFIC_DEVICE_CODES 9999
#define SDR14_DEVICE_CODE (SPECIFIC_DEVICE_CODES+1)
#define SDRIQ_DEVICE_CODE (SPECIFIC_DEVICE_CODES+2)
#define PERSEUS_DEVICE_CODE (SPECIFIC_DEVICE_CODES+3)
#define SDRIP_DEVICE_CODE (SPECIFIC_DEVICE_CODES+4)
#define EXCALIBUR_DEVICE_CODE (SPECIFIC_DEVICE_CODES+5)
#define RTL2832_DEVICE_CODE (SPECIFIC_DEVICE_CODES+6)
#define MIRISDR_DEVICE_CODE (SPECIFIC_DEVICE_CODES+7)
#define BLADERF_DEVICE_CODE (SPECIFIC_DEVICE_CODES+8)
#define PCIE9842_DEVICE_CODE (SPECIFIC_DEVICE_CODES+9)
#define OPENHPSDR_DEVICE_CODE (SPECIFIC_DEVICE_CODES+10)
#define NETAFEDRI_DEVICE_CODE (SPECIFIC_DEVICE_CODES+11)
#define FDMS1_DEVICE_CODE (SPECIFIC_DEVICE_CODES+12)
#define AIRSPY_DEVICE_CODE (SPECIFIC_DEVICE_CODES+13)
#define CLOUDIQ_DEVICE_CODE (SPECIFIC_DEVICE_CODES+14)
#define AIRSPYHF_DEVICE_CODE (SPECIFIC_DEVICE_CODES+15)
#define SDRPLAY2_DEVICE_CODE (SPECIFIC_DEVICE_CODES+16)
#define SDRPLAY3_DEVICE_CODE (SPECIFIC_DEVICE_CODES+17)

// Positions for runtime error messages in the main spectrum.
#define WGERR_DASYNC 0
#define WGERR_ADSPEED 1
#define WGERR_HWARE 2
#define WGERR_MEMORY 2
#define WGERR_SCREEN 2
#define WGERR_RXIN 3
#define WGERR_TIMF1_OVERLOAD 3
#define WGERR_TIMF2_OVERLOAD 4
#define WGERR_FFT3_OVERLOAD 0
#define WGERR_BASEB_OVERLOAD 2
#define WGERR_RXOUT 4
#define WGERR_TXIN 5
#define WGERR_TXOUT 6
#define WGERR_NETWR 7
#define WGERR_SOUNDCARD 0


#define NET_RXOUT_RAW16    1
#define NET_RXOUT_RAW18    2
#define NET_RXOUT_RAW24    4
#define NET_RXOUT_FFT1     8
#define NET_RXOUT_TIMF2    16
#define NET_RXOUT_FFT2     32
#define NET_RXOUT_BASEB    64
#define NET_RXOUT_BASEBRAW 128
#define NET_RX_OUTPUT     0x0000ffff
#define NET_RX_INPUT      0xffff0000
#define NET_RXIN_RAW16    0x00010000
#define NET_RXIN_RAW18    0x00020000
#define NET_RXIN_RAW24    0x00040000
#define NET_RXIN_FFT1     0x00080000
#define NET_RXIN_BASEB    0x00100000
#define NET_RXIN_BASEBRAW 0x00200000
#define NET_RXIN_TIMF2    0x00400000

#define NETMSG_OWN_FREQ 41651
#define NETMSG_SUBSLAVE_FREQ 41652
#define NETMSG_CAL_REQUEST 41653
#define NETMSG_CALIQ_REQUEST 41654
#define NETMSG_FFT1INFO_REQUEST 41655
#define NETMSG_MODE_REQUEST 41656
#define NETMSG_REMOVE_SUBSLAVE 41567
#define NETMSG_BASEBMODE_REQUEST 41658
#define NETMSG_BASEBRAWMODE_REQUEST 41659
#define NETMSG_SET_MASTER_FREQUENCY 41660
#define NETMSG_TIMF2MODE_REQUEST 41661

#define EXTIO_INVERTED 1
#define EXTIO_NON_INVERTED 2
#define EXTIO_USE_SOUNDCARD 16

typedef void (*ROUTINE) (void);
#define PERMDEB fprintf(dmp,
#define DEB if(dmp != NULL)PERMDEB
#define XZ xz
// *******************************************
// Audio board and disk file parameter definitions
#define DWORD_INPUT 1
#define TWO_CHANNELS 2
#define IQ_DATA 4
#define BYTE_INPUT 8
#define NO_DUPLEX 16
#define DIGITAL_IQ 32
#define FLOAT_INPUT 64
#define QWORD_INPUT 128
#define MODEPARM_MAX 256
// *******************************************
// Processing parameter definitions
#define FIRST_FFT_BANDWIDTH 0
#define FIRST_FFT_SINPOW 1
#define FIRST_FFT_VERNR 2
#define FIRST_FFT_NO_OF_THREADS 3
#define FFT1_STORAGE_TIME 4
#define FIRST_FFT_GAIN 5
#define WG_WATERF_BLANKED_PERCENT 6
#define FFT1_CORRELATION_SPECTRUM 7
#define SECOND_FFT_ENABLE 8

#define FIRST_BCKFFT_VERNR 9
#define SELLIM_MAXLEVEL 10
#define FIRST_BCKFFT_ATT_N 11
#define SECOND_FFT_NINC 12
#define SECOND_FFT_SINPOW 13
#define SECOND_FFT_VERNR 14
#define SECOND_FFT_ATT_N 15
#define FFT2_STORAGE_TIME 16

#define AFC_ENABLE 17
#define AFC_LOCK_RANGE 18
#define AFC_MAX_DRIFT 19
#define CW_DECODE_ENABLE 20
#define MAX_NO_OF_SPURS 21
#define SPUR_TIMECONSTANT 22

#define MIX1_BANDWIDTH_REDUCTION_N 23
#define MIX1_NO_OF_CHANNELS 24
#define THIRD_FFT_SINPOW 25
#define BASEBAND_STORAGE_TIME 26
#define OUTPUT_DELAY_MARGIN 27
#define DA_OUTPUT_SPEED 28

#define OUTPUT_MODE 29

#define AMPLITUDE_EXPAND_EXPONENT 30
#define BG_WATERF_BLANKED_PERCENT 31
#define BG_METER_FILE_AVERAGES 32
#define MAX_GENPARM 33

// *******************************************************
// Defines for state machine in fft2.c
#define FFT2_NOT_ACTIVE 0
#define FFT2_B 10
#define FFT2_C 11
#define FFT2_MMXB 20
#define FFT2_MMXC 21
#define FFT2_ELIMINATE_SPURS 104
#define FFT2_WATERFALL_LINE 1001
#define FFT2_TEST_FINISH 9999
#define FFT2_COMPLETE -1
// **********************************************
#define CALIQ 2
#define CALAMP 1

// **********************************************
#define SCREEN_TYPE_X11 1
#define SCREEN_TYPE_X11_SHM 2
#define SCREEN_TYPE_SVGALIB 4
#define SCREEN_TYPE_FBDEV 8
#define SCREEN_TYPE_WINDOWS 16

// *******************************************

#if(OSNUM == OSNUM_WINDOWS)
#define FD uint64_t
#else
#define FD int
#endif


// Structure for Si570
typedef struct {
int libusb_version;
int rx_firmware_type;  // 1=DG8SAQ/PE0FKO 2=FiFi SDR 3=SDR-Widget
int rx_usb_index;
int min_rx_freq;
int max_rx_freq;
int rx_lo_multiplier;
int rx_lo_freq_offset;
int rx_passband_direction;
int rx_freq_adjust;
int rx_serial1;
int rx_serial2;
int rx_serial3;
int rx_serial4;
int tx_firmware_type;
int tx_usb_index;
int min_tx_freq;
int max_tx_freq;
int tx_lo_multiplier;
int tx_lo_freq_offset;
int tx_passband_direction;
int tx_freq_adjust;
int tx_serial1;
int tx_serial2;
int tx_serial3;
int tx_serial4;
}P_SI570;
extern P_SI570 si570;
#define MAX_SI570_PARM            25
extern char *si570_parm_text[MAX_SI570_PARM];

// Structure for transmitter parameters in ssb mode
typedef struct {
int minfreq;
int maxfreq;
int slope;
int bass;
int treble;
int mic_gain;
int mic_agc_time;
int mic_f_threshold;
int mic_t_threshold;
int rf1_gain;
int alc_time;
int micblock_time;
int delay_margin;
int frequency_shift;
int check;
}SSBPROC_PARM;
#define MAX_SSBPROC_PARM 15
extern char *ssbprocparm_text[MAX_SSBPROC_PARM];

// Structure for transmitter parameters in CW modes
typedef struct {
int rise_time;
int enable_hand_key;
int enable_tone_key;
int enable_ascii;
int radar_interval;
int radar_pulse;
int delay_margin;
int carrier_freq;
int sidetone_ampl;
int sidetone_freq;
int wcw_mute_on;
int wcw_mute_off;
int ncw_mute_on;
int ncw_mute_off;
int hsms_mute_on;
int hsms_mute_off;
int qrss_mute_on;
int qrss_mute_off;
int radar_mute_on;
int radar_mute_off;
int check;
}CWPROC_PARM;
#define MAX_CWPROC_PARM 21
extern char *cwprocparm_text[MAX_CWPROC_PARM];

// *******************************************
// Structure for printing time stamps on waterfall graphs
typedef struct {
int line;
char text[12];
}WATERF_TIMES;

// *******************************************
// Structure for 5th order Butterworth low pass filter
typedef struct {
double gain;
double c0;
double c1;
double c2;
double c3;
double c4;
}IIR5_PARMS;

// *******************************************
// Structure for user interface parameters selected by operator
typedef struct {
int vga_mode;
int screen_width_factor;
int screen_height_factor;
int font_scale;
int mouse_speed;
int max_dma_rate;
int process_priority;
int use_alsa;
int rx_input_mode;
int rx_rf_channels;
int rx_ad_channels;
int rx_ad_speed;
int rx_addev_no;
int rx_admode;
int rx_damode;
int rx_dadev_no;
int rx_min_da_speed;
int rx_max_da_speed;
int rx_max_da_channels;
int rx_max_da_bytes;
int rx_min_da_channels;
int rx_min_da_bytes;
int rx_soundcard_radio;
unsigned int converter_hz;
unsigned int converter_mhz;
int converter_mode;
unsigned int network_flag;
int tx_ad_speed;
int tx_da_speed;
int tx_addev_no;
int tx_dadev_no;
int tx_da_channels;
int tx_ad_channels;
int tx_da_bytes;
int tx_ad_bytes;
int tx_enable;
int tx_pilot_tone_db;
int tx_pilot_tone_prestart;
int tx_soundcard_radio;
int operator_skil;
int max_blocked_cpus;
unsigned int timer_resolution;
int autostart;
int rx_ad_latency;
int rx_da_latency;
int tx_ad_latency;
int tx_da_latency;
int sample_shift;
int min_dma_rate;
int use_extio;
int extio_type;
int transceiver_mode;
int ptt_control;
int bkg_color;
int check;
}USERINT_PARM;
#define MAX_UIPARM 55
extern char *uiparm_text[MAX_UIPARM];

#define DEFAULT_MIN_DMA_RATE 30
#define DEFAULT_MAX_DMA_RATE 300
#define MIN_DMA_RATE 10
#define MAX_DMA_RATE 5000
#define DMA_MIN_DIGITS 2
#define DMA_MAX_DIGITS 4
#define MIN_DMA_RATE_EXP 1
#define MAX_DMA_RATE_EXP 100000
#define DMA_MIN_DIGITS_EXP 1
#define DMA_MAX_DIGITS_EXP 6


#define OPERATOR_SKIL_NEWCOMER 1
#define OPERATOR_SKIL_NORMAL 2
#define OPERATOR_SKIL_EXPERT 3


// *******************************************
// Screen object parameters for mouse and screen drawing.
// Each screen object is defined by it's number, scro[].no
// For each type scro[].type the number scro[].no is unique and
// defines what to do in the event of a mouse click.

// Definitions for type = GRAPH
#define WIDE_GRAPH 1
#define HIRES_GRAPH 2
#define TRANSMIT_GRAPH 3
#define FREQ_GRAPH 4
#define RADAR_GRAPH 5
#define MAX_WIDEBAND_GRAPHS 10
#define AFC_GRAPH 11
#define BASEBAND_GRAPH 12
#define POL_GRAPH 13
#define COH_GRAPH 14
#define EME_GRAPH 15
#define METER_GRAPH 16
#define ELEKTOR_GRAPH 17
#define FCDPROPLUS_GRAPH 18
#define PHASING_GRAPH 19
#define SIGANAL_GRAPH 20
#define GRAPH_RIGHTPRESSED 128
#define GRAPH_MASK 0x8000007f

// Definitions for parameter input routines under mouse control
#define FIXED_INT_PARM 3
#define TEXT_PARM 4
#define FIXED_FLOAT_PARM 5
#define FIXED_DOUBLE_PARM 6
#define DATA_READY_PARM 128
#define MAX_TEXTPAR_CHARS 32
#define EG_DX_CHARS 12
#define EG_LOC_CHARS 6

// Definitions for type WIDE_GRAPH
#define WG_TOP 0
#define WG_BOTTOM 1
#define WG_LEFT 2
#define WG_RIGHT 3
#define WG_BORDER 4
#define WG_YSCALE_EXPAND 5
#define WG_YSCALE_CONTRACT 6
#define WG_YZERO_DECREASE 7
#define WG_YZERO_INCREASE 8
#define WG_FQMIN_DECREASE 9
#define WG_FQMIN_INCREASE 10
#define WG_FQMAX_DECREASE 11
#define WG_FQMAX_INCREASE 12
#define WG_AVG1NUM 13
#define WG_FFT1_AVGNUM 14
#define WG_WATERF_AVGNUM 15
#define WG_WATERF_ZERO 16
#define WG_WATERF_GAIN 17
#define WG_SPUR_TOGGLE 18
#define WG_FREQ_ADJUSTMENT_MODE 19
#define WG_LOWEST_FREQ 20
#define WG_HIGHEST_FREQ 21
#define MAX_WGBUTT 22

// Definitions for type HIRES_GRAPH
#define HG_TOP 0
#define HG_BOTTOM 1
#define HG_LEFT 2
#define HG_RIGHT 3
#define HG_BLN_STUPID 4
#define HG_BLN_CLEVER 5
#define HG_TIMF2_STATUS 6
#define HG_TIMF2_WK_INC 7
#define HG_TIMF2_WK_DEC 8
#define HG_TIMF2_ST_INC 9
#define HG_TIMF2_ST_DEC 10
#define HG_TIMF2_LINES 11
#define HG_TIMF2_HOLD 12
#define HG_FFT2_AVGNUM 13
#define HG_SPECTRUM_ZERO 14
#define HG_SPECTRUM_GAIN 15
#define HG_MAP65_GAIN 16
#define HG_MAP65_STRONG 17
#define HG_SELLIM_PAR1 18
#define HG_SELLIM_PAR2 19
#define HG_SELLIM_PAR2 19
#define HG_SELLIM_PAR3 20
#define HG_SELLIM_PAR4 21
#define HG_SELLIM_PAR5 22
#define HG_SELLIM_PAR6 23
#define HG_SELLIM_PAR7 24
#define HG_SELLIM_PAR8 25
#define MAX_HGBUTT 26

// Definitions for type BASEBAND_GRAPH
#define BG_TOP 0
#define BG_BOTTOM 1
#define BG_LEFT 2
#define BG_RIGHT 3
#define BG_YSCALE_EXPAND 4
#define BG_YSCALE_CONTRACT 5
#define BG_YZERO_DECREASE 6
#define BG_YZERO_INCREASE 7
#define BG_RESOLUTION_DECREASE 8
#define BG_RESOLUTION_INCREASE 9
#define BG_OSCILLOSCOPE 10
#define BG_OSC_INCREASE 11
#define BG_OSC_DECREASE 12
#define BG_PIX_PER_PNT_INC 13
#define BG_PIX_PER_PNT_DEC 14
#define BG_TOGGLE_EXPANDER 15
#define BG_TOGGLE_COHERENT 16
#define BG_TOGGLE_PHASING 17
#define BG_TOGGLE_CHANNELS 18
#define BG_TOGGLE_BYTES 19
#define BG_TOGGLE_TWOPOL 20
#define BG_SEL_COHFAC 21
#define BG_SEL_DELPNTS 22
#define BG_SEL_FFT3AVGNUM 23
#define BG_TOGGLE_AGC 24
#define BG_SEL_AGC_ATTACK 25
#define BG_SEL_AGC_RELEASE 26
#define BG_SEL_AGC_HANG 27
#define BG_YBORDER 28
#define BG_WATERF_ZERO 29
#define BG_WATERF_GAIN 30
#define BG_WATERF_AVGNUM 31
#define BG_HORIZ_ARROW_MODE 32
#define BG_MIXER_MODE 33
#define BG_FILTER_SHIFT 34
#define BG_NOTCH_NO 35
#define BG_NOTCH_POS 36
#define BG_NOTCH_WIDTH 37
#define BG_TOGGLE_FM_MODE 38
#define BG_TOGGLE_FM_SUBTRACT 39
#define BG_SEL_FM_AUDIO_BW 40
#define BG_TOGGLE_CH2_PHASE 41
#define BG_SQUELCH_TIME 42
#define BG_SQUELCH_POINT 43
#define BG_SQUELCH_LEVEL 44

#define MAX_BGBUTT 45

#define MAX_BG_NOTCHES 9

// Definitions for type AFC_GRAPH
#define AG_TOP 0
#define AG_BOTTOM 1
#define AG_LEFT 2
#define AG_RIGHT 3
#define AG_FQSCALE_EXPAND 4
#define AG_FQSCALE_CONTRACT 5
#define AG_MANAUTO 6
#define AG_WINTOGGLE 7
#define AG_SEL_AVGNUM 8
#define AG_SEL_FIT 9
#define AG_SEL_DELAY 10
#define MAX_AGBUTT 11

// Definitions for type POL_GRAPH
#define PG_TOP 0
#define PG_BOTTOM 1
#define PG_LEFT 2
#define PG_RIGHT 3
#define PG_ANGLE 4
#define PG_CIRC 5
#define PG_AUTO 6
#define PG_AVGNUM 7
#define MAX_PGBUTT 8

// Definitions for type PHASING_GRAPH
#define XG_TOP 0
#define XG_BOTTOM 1
#define XG_LEFT 2
#define XG_RIGHT 3
#define XG_INCREASE_PAR1 4
#define XG_DECREASE_PAR1 5
#define XG_INCREASE_PAR2 6
#define XG_DECREASE_PAR2 7
#define XG_INCREASE_PAR1_F 8
#define XG_DECREASE_PAR1_F 9
#define XG_INCREASE_PAR2_F 10
#define XG_DECREASE_PAR2_F 11
#define XG_DO_A 12
#define XG_DO_B 13
#define XG_DO_C 14
#define XG_DO_D 15
#define XG_DO_1 16
#define XG_DO_2 17
#define XG_DO_3 18
#define XG_COPY 19
#define XG_MEM_1 20
#define XG_MEM_2 21
#define XG_MEM_3 22
#define MAX_XGBUTT 23

// Definitions for type siganal_graph
#define SG_TOP 0
#define SG_BOTTOM 1
#define SG_LEFT 2
#define SG_RIGHT 3
#define SG_NEW_FFT_N 4
#define SG_NEW_YMAX 5
#define SG_NEW_YGAIN 6
#define SG_NEW_XGAIN 7
#define SG_NEW_AVGNUM 8
#define SG_NEW_MODE 9
#define MAX_SGBUTT 10

// Definitions for type COH_GRAPH
#define CG_TOP 0
#define CG_BOTTOM 1
#define CG_LEFT 2
#define CG_RIGHT 3
#define CG_OSCILLOSCOPE 4
#define CG_METER_GRAPH 5
#define MAX_CGBUTT 6

// Definitions for type AEME_GRAPH
#define EG_TOP 0
#define EG_BOTTOM 1
#define EG_LEFT 2
#define EG_RIGHT 3
#define EG_MINIMISE 4
#define EG_LOC 5
#define EG_DX 6
#define MAX_EGBUTT 7

// Definitions for type FREQ_GRAPH
#define FG_TOP 0
#define FG_BOTTOM 1
#define FG_LEFT 2
#define FG_RIGHT 3
#define FG_INCREASE_FQ 4
#define FG_DECREASE_FQ 5
#define FG_INCREASE_GAIN 6
#define FG_DECREASE_GAIN 7
#define MAX_FGBUTT 8

// Definitions for type (S)METER_GRAPH
#define MG_TOP 0
#define MG_BOTTOM 1
#define MG_LEFT 2
#define MG_RIGHT 3
#define MG_INCREASE_AVGN 4
#define MG_DECREASE_AVGN 5
#define MG_INCREASE_GAIN 6
#define MG_DECREASE_GAIN 7
#define MG_INCREASE_YREF 8
#define MG_DECREASE_YREF 9
#define MG_CHANGE_CAL 10
#define MG_CHANGE_TYPE 11
#define MG_CHANGE_TRACKS 12
#define MG_SCALE_STON_SIGSHIFT 13
#define MAX_MGBUTT 14

// Definitions for type TRANSMIT_GRAPH

#define TG_TOP 0
#define TG_BOTTOM 1
#define TG_LEFT 2
#define TG_RIGHT 3
#define TG_INCREASE_FQ 4
#define TG_DECREASE_FQ 5
#define TG_CHANGE_TXPAR_FILE_NO 6
#define TG_NEW_TX_FREQUENCY 7
#define TG_SET_SIGNAL_LEVEL 8
#define TG_ONOFF 9
#define TG_RADAR_INTERVAL 10
#define TG_MUTE_ON 11
#define TG_MUTE_OFF 12
#define MAX_TGBUTT 13

// Definitions for type RADAR_GRAPH

#define RG_TOP 0
#define RG_BOTTOM 1
#define RG_LEFT 2
#define RG_RIGHT 3
#define RG_TIME 4
#define RG_ZERO 5
#define RG_GAIN 6
#define MAX_RGBUTT 7



// Structure for the eme database.
#define CALLSIGN_CHARS 12
typedef struct{
char call[CALLSIGN_CHARS];
float lon;
float lat;
}DXDATA;

// Structure for mouse buttons
typedef struct {
int x1;
int x2;
int y1;
int y2;
}BUTTONS;

// Structure to remember screen positions
// for processing parameter change boxes
typedef struct{
int no;
int type;
int x;
int y;
}SAVPARM;

// Structure for a screen object on which the mouse can operate.
typedef struct {
int no;
int x1;
int x2;
int y1;
int y2;
}SCREEN_OBJECT;

// Structure for the AFC graph
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int mode_control;
int avgnum;
int window;
int fit_points;
int delay;
int check;
float minston;
float search_range;
float lock_range;
float frange;
} AG_PARMS;
#define MAX_AG_INTPAR 10
extern char *ag_intpar_text[MAX_AG_INTPAR];
#define MAX_AG_FLOATPAR 4
extern char *ag_floatpar_text[MAX_AG_FLOATPAR];

// Structure for the wide graph.
// Waterfall and full dynamic range spectrum
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int yborder;
int xpoints_per_pixel;
int pixels_per_xpoint;
int first_xpoint;
int xpoints;
int fft_avg1num;
int spek_avgnum;
int waterfall_avgnum;
int spur_inhibit;
int check;
float yzero;
float yrange;
float waterfall_db_zero;
float waterfall_db_gain;
} WG_PARMS;
#define MAX_WG_INTPAR 14
extern char *wg_intpar_text[MAX_WG_INTPAR];
#define MAX_WG_FLOATPAR 4
extern char *wg_floatpar_text[MAX_WG_FLOATPAR];

// Structure for the high resolution and blanker control graph.
// Waterfall and full dynamic range spectrum
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int stupid_bln_mode;
int clever_bln_mode;
int timf2_oscilloscope;
int timf2_oscilloscope_lines;
int timf2_oscilloscope_hold;
int spek_avgnum;
unsigned int stupid_bln_limit;
unsigned int clever_bln_limit;
int spek_zero;
int spek_gain;
int map65_gain_db;
int map65_strong;
int sellim_par1;  //0,1,2
int sellim_par2;  //0,1
int sellim_par3;  //0,1
int sellim_par4;  //0,1
int sellim_par5;  //0,1,2
int sellim_par6;  // 0,1
int sellim_par7;  // 0,1
int sellim_par8;  // 0,1
int check;
float stupid_bln_factor;
float clever_bln_factor;
float blanker_ston_fft1;
float blanker_ston_fft2;
float timf2_oscilloscope_wk_gain;
float timf2_oscilloscope_st_gain;
} HG_PARMS;
#define MAX_HG_INTPAR 25
extern char *hg_intpar_text[MAX_HG_INTPAR];
#define MAX_HG_FLOATPAR 6
extern char *hg_floatpar_text[MAX_HG_FLOATPAR];


// Structure for the baseband graph
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int yborder;
int fft_avgnum;
int pixels_per_point;
int coh_factor;
int delay_points;
int agc_flag;
int agc_attack;
int agc_release;
int agc_hang;
int waterfall_avgnum;
int wheel_stepn;
int oscill_on;
int horiz_arrow_mode;
int mixer_mode;
int filter_shift;
int fm_mode;
int fm_subtract;
int fm_audio_bw;
int ch2_phase;
int squelch_level;
int squelch_time;
int squelch_point;
int check;
float filter_flat;
float filter_curv;
float yzero;
float yrange;
float db_per_pixel;
float yfac_power;
float yfac_log;
float bandwidth;
float first_frequency;
float bfo_freq;
float output_gain;
float waterfall_gain;
float waterfall_zero;
float oscill_gain;
} BG_PARMS;
#define MAX_BG_INTPAR 27
extern char *bg_intpar_text[MAX_BG_INTPAR];
#define MAX_BG_FLOATPAR 14
extern char *bg_floatpar_text[MAX_BG_FLOATPAR];

// Structure for the polarization graph
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int adapt;
int avg;
int startpol;
int enable_phasing;
int check;
float angle;
float c1;
float c2;
float c3;
float ch2_amp;
float ch2_phase;
} PG_PARMS;
#define MAX_PG_INTPAR 9
extern char *pg_intpar_text[MAX_PG_INTPAR];
#define MAX_PG_FLOATPAR 6
extern char *pg_floatpar_text[MAX_PG_FLOATPAR];

typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int mode;
int avg;
int fft_n;
int ymax;
int check;
float xgain;
float ygain;
} SG_PARMS;
#define MAX_SG_INTPAR 9
extern char *sg_intpar_text[MAX_SG_INTPAR];
#define MAX_SG_FLOATPAR 2
extern char *sg_floatpar_text[MAX_SG_FLOATPAR];



typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
float ampl_bal;
float phase_shift;
float m1ampl;
float m1phase;
float m2ampl;
float m2phase;
float m3ampl;
float m3phase;
} XG_PARMS;
#define MAX_XG_INTPAR 4
extern char *xg_intpar_text[MAX_XG_INTPAR];
#define MAX_XG_FLOATPAR 8
extern char *xg_floatpar_text[MAX_XG_FLOATPAR];


// Structure for the coherent processing graph
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int meter_graph_on;
int oscill_on;
} CG_PARMS;
#define MAX_CG_INTPAR 6
extern char *cg_intpar_text[MAX_CG_INTPAR];
#define MAX_CG_FLOATPAR 0
extern char *cg_floatpar_text[MAX_CG_FLOATPAR+1];


// Structure for the S-meter graph
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int scale_type;
int avgnum;
int tracks;
int check;
float ygain;
float yzero;
float cal_dbm;
float cal_ston;
float cal_ston_sigshift;
float cal_dbhz;
float cal_s_units;
} MG_PARMS;
#define MAX_MG_INTPAR 8
extern char *mg_intpar_text[MAX_MG_INTPAR];
#define MAX_MG_FLOATPAR 7
extern char *mg_floatpar_text[MAX_MG_FLOATPAR];


// Structure for the EME graph.
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int minimise;
} EG_PARMS;
#define MAX_EG_INTPAR 5
extern char *eg_intpar_text[MAX_EG_INTPAR];
#define MAX_EG_FLOATPAR 0
extern char *eg_floatpar_text[MAX_EG_FLOATPAR+1];

// Structure for the frequency control box.
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int yborder;
int passband_direction;
int gain;
int gain_increment;
double passband_increment;
double passband_center;
double tune_frequency;
} FG_PARMS;
#define MAX_FG_INTPAR 8
extern char *fg_intpar_text[MAX_FG_INTPAR];
#define MAX_FG_FLOATPAR 3
extern char *fg_floatpar_text[MAX_FG_FLOATPAR];

// Structure for the Tx control box.
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int spproc_no;
int passband_direction;
double level_db;
double freq;
double band_increment;
double band_center;
} TG_PARMS;
#define MAX_TG_INTPAR 6
extern char *tg_intpar_text[MAX_TG_INTPAR];
#define MAX_TG_FLOATPAR 4
extern char *tg_floatpar_text[MAX_TG_FLOATPAR];

// Structure for the radar graph.
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
float time;
float zero;
float gain;
} RG_PARMS;
#define MAX_RG_INTPAR 4
extern char *rg_intpar_text[MAX_RG_INTPAR];
#define MAX_RG_FLOATPAR 3
extern char *rg_floatpar_text[MAX_RG_FLOATPAR];

// Structure for GPU parameters.
typedef struct {
int fft1_platform;
int fft1_device;
int fft1_batch_n;
} GPU_PARMS;
#define MAX_GPU_INTPAR 3
extern char *ocl_intpar_text[MAX_GPU_INTPAR];
#define MAX_GPU_FLOATPAR 0
extern char *ocl_floatpar_text[MAX_GPU_FLOATPAR+1];


// Structure for network parameters
typedef struct {
int send_group;
int rec_group;
int port;
unsigned int send_raw;
unsigned int send_fft1;
unsigned int send_fft2;
unsigned int send_timf2;
unsigned int send_baseb;
unsigned int send_basebraw;
unsigned int receive_raw;
unsigned int receive_fft1;
unsigned int receive_baseb;
unsigned int receive_basebraw;
unsigned int receive_timf2;
int check;
} NET_PARMS;
#define MAX_NET_INTPAR 15
extern char *net_intpar_text[MAX_NET_INTPAR];

#define MAX_NETSLAVES 16
// Structure for file descriptors used by the network
typedef struct {
FD send_rx_raw16;
FD send_rx_raw18;
FD send_rx_raw24;
FD send_rx_fft1;
FD send_rx_fft2;
FD send_rx_timf2;
FD send_rx_baseb;
FD send_rx_basebraw;
FD rec_rx;
FD master;
FD any_slave;
FD slaves[MAX_NETSLAVES];
} NET_FD;
#define MAX_NET_FD (9+MAX_NETSLAVES)
extern NET_FD netfd;
#define MAX_FREQLIST (2*MAX_NETSLAVES)

// Structure for multicasting receive data on the network.
#define NET_MULTICAST_PAYLOAD 1392 // This number must be a multiple of 48
typedef struct {

double passband_center;        //  8
int time;                      //  4
float userx_freq;              //  4
int ptr;                       //  4
unsigned short int block_no;   //  2
signed char userx_no;                 //  1
signed char passband_direction;       //  1
char buf[NET_MULTICAST_PAYLOAD];
} NET_RX_STRUCT;
extern NET_RX_STRUCT net_rxdata_16;
extern NET_RX_STRUCT net_rxdata_18;
extern NET_RX_STRUCT net_rxdata_24;
extern NET_RX_STRUCT net_rxdata_fft1;
extern NET_RX_STRUCT net_rxdata_timf2;
extern NET_RX_STRUCT net_rxdata_fft2;
extern NET_RX_STRUCT net_rxdata_baseb;
extern NET_RX_STRUCT net_rxdata_basebraw;

// Structure for messages from slaves.
typedef struct {
int type;
int frequency;
} SLAVE_MESSAGE;
extern SLAVE_MESSAGE slave_msg;




// ***********************************************
// Structure used by float fft routines
typedef struct {
float sin;
float cos;
}COSIN_TABLE;

// ***********************************************
// Structure used by d_float fft routines (double)
typedef struct {
double sin;
double cos;
}D_COSIN_TABLE;

// Structure used by MMX fft routines
typedef struct {
short int c1p;
short int s2p;
short int c3p;
short int s4m;
}MMX_COSIN_TABLE;

// Structure for mixer/decimater using backwards FFTs
typedef struct {
float *window;
float *cos2win;
float *sin2win;
unsigned short int *permute;
COSIN_TABLE *table;
unsigned int interleave_points;
unsigned int new_points;
unsigned int crossover_points;
unsigned int size;
unsigned int n;
}MIXER_VARIABLES;

// Define setup info for each fft version.
typedef struct {
unsigned char window;
unsigned char permute;
unsigned char max_n;
unsigned char mmx;
unsigned char simd;
unsigned char gpu;
unsigned char real2complex;
unsigned char parall_fft;
unsigned char doub;
char *text;
} FFT_SETUP_INFO;

typedef struct {
void *pointer;
size_t size;
size_t scratch_size;
int num;
} MEM_INF;

typedef struct {
float x2;
float y2;
float im_xy;
float re_xy;
}TWOCHAN_POWER;

# define MAX_FFT_VERSIONS 21
# define MAX_FFT1_VERNR 8
# define MAX_FFT1_BCKVERNR 4
# define MAX_FFT2_VERNR 4

// when  wse.parport is set to this value, all the I/O to the LPT is routed to the USB2LPT 1.6
# define USB2LPT16_PORT_NUMBER 16

#ifdef _MSC_VER
  #ifndef ssize_t
    #ifdef _WIN64
	  typedef  __int64 ssize_t;
	#else
	  typedef  __int32 ssize_t;
    #endif
  #endif
#endif

#define TIMF1_UNMUTED 0
#define TIMF1_MUTED 1
