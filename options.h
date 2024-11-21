// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!                                                         !! 
// !!  Default is FALSE for all debug and manipulate options. !!
// !!                                                         !!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// ************************************************************
//                ----  Debug tool  ----
// Set DUMPFILE to TRUE to get debug info in the files dmp,dmp1,dmp2 and dmp3.
// Use xz("string"); to get traces from code execution
// or make explicit writes to dmp. xz() will call
// fflush() and sync() and thus it contains information
// just before a crash that would otherwise not have been 
// physically written to the disk. 
// If you trace crashes, use fprintf(dmp,"%d.....",variables)
// followed by xz(" "); to physically write to disk.
// (Do not use xz() if you want fast processing)

// WARNING!! enabling dump file will skip testing of some
// portaudio devices. Those devices cause a crash on assert error
// due to a bug in ALSA. See pa.c 

//#define DUMPFILE TRUE
#define DUMPFILE FALSE

// Functions deb_timing_info, xx,qt0,qt1,qt2,qq,qq1,qq2 and qq3 
// may be helpful.
// ************************************************************
//    --- Set the colour of crearscreen. ---
// Normally clear_screen() gives a black screen. By setting 
// something else one can get a memory function into spectrum graphs.
// This is implemented only for 24 bit screens under Linux X11
#define CLRSCR_COLOR 0
//#define CLRSCR_COLOR 7
// ************************************************************
//    ----   Avoid Debian bug 622533 (X11 in 64 bit squeeze sid)  ----
// There is a bug in 64 bit Debian squeeze that might never be corrected.
// The bug is in the package libx11-6 which is 2:1.3.3-3 in squeeze.
// The bug is corrected in Debian sid with libx11-6=2:1.4.3-1
// The new libx11 will not get into squeeze, but a targetted fix might
// be implemented if it will be considered safe enough.
// If you set AVOID_XFLUSH to TRUE linrad will never call XFlush
// when compiled under kernels more recent than 2.6.27
// This option could make graphics faster when modern kernels
// are used on slow computers. There is a risk however that the
// screen will not always update properly.
#define AVOID_XFLUSH FALSE
//#define AVOID_XFLUSH TRUE
// ************************************************************
//       --- Do not use xkblib even if it is installed  -----
//  Useful with VNC to remote controlled linrad receiver
//  XKeycodeToKeysym() is deprecated so this option might not
//  always work.
#define DO_NOT_USE_XKBLIB FALSE
//#define DO_NOT_USE_XKBLIB TRUE
// ************************************************************
//  When using the Linux Framebuffer in terminal mode the gpm
//  text cursor may be present (and annoying) Specify the full
//  location of your gpmctl file here
#define GPMCTL "/dev/gpmctl"
// ************************************************************
//               ----   File structure  ----
// Note that the user can override these standard directories
// by specifying a full path.
// GIFDIR is where screen dumps are saved
// WAVDIR is where output audio files are saved
// RAWDIR is where input raw data files are saved
// ALLANDIR is where frequency data from the Allan graph is saved.

#if OSNUM == OSNUM_LINUX
#if DARWIN == 1
#define GIFDIR "/linrad_data/"
#define WAVDIR "/linrad_data/"
#define RAWDIR "/linrad_data/"
#define ALLANDIR "/linrad_data/"
#else
#define GIFDIR "/home/linrad_data/"
#define WAVDIR "/home/linrad_data/"
#define RAWDIR "/home/linrad_data/"
#define ALLANDIR "/home/linrad_data/"
#endif
#endif

#if OSNUM == OSNUM_WINDOWS
# define GIFDIR "C:\\linrad_data\\"
# define WAVDIR "C:\\linrad_data\\"
# define RAWDIR "C:\\linrad_data\\"
#define ALLANDIR "C:\\linrad_data\\"
# if IA64 == 0
#  define DLLDIR "C:\\linrad\\dll\\"
#  else
#  define DLLDIR "C:\\linrad\\dll\\x64\\"
# endif
#endif
// Define one or more ALLAN files if you want to produce input
// for TimeLab from Linrad
//#define ALLANFILE_CH1 TRUE
#define ALLANFILE_CH1 FALSE
//#define ALLANFILE_CH2 TRUE
#define ALLANFILE_CH2 FALSE  
//#define ALLANFILE_DIFF TRUE  
#define ALLANFILE_DIFF FALSE  
// ************************************************************
//               ----   Configuration   ----
// The coherent graph which also contains the S-meter is rectangular
// and it is set to a multiple of the font width in size.
// The side length will affect the number of digits for the S-meter
// and the screen space used for this window
#define COH_SIDE 7
// ************************************************************
//                Show currently used hardware on screen last line
//#define SHOW_HARDWARE TRUE
#define SHOW_HARDWARE FALSE
// ************************************************************
//                ----  Debug tool  ----
// When using the valgrind debugger one gets error messages that incorrectly
// say that various memory areas are not initialized. This option will
// fill memory with zeroes to satisfy the valgrind debugger.
// The increased cpu load is small or negligible, but this option
// should be set to off anyway because it is just a way of handling
// a bug in the valgrind debugger.
//#define VALGRIND TRUE
#define VALGRIND FALSE
// Symbol tables make the executable 3 times larger. You can save
// about 2 megabytes by excluding them. Could be a good thing
// if you run several instances of Linrad on a machine with
// a small RAM.
// Under Mac OSX symbol tables can not be suppressed.
#define SYMBOL_TABLES FALSE
//#define SYMBOL_TABLES TRUE
// Note that you must run ./configure again after changing the
// symbol table option
// ************************************************************
//                ----  Debug tool  ----
// Set PERS_DEBUG to 1 to get error messages from libusb and the
// intrface routines in Linrad. These errors would appear on stdout
// when a Perseus is used under Linux only.

//#define PERS_DEBUG 1
#define PERS_DEBUG 0
// *************************************************************
//        ---- manipulate calibration data ----
// Extract a calibration file from save files in Linrads .raw format
// and save them as the appropriate dsp_xxx_corr file.
//#define SAVE_RAWFILE_CORR TRUE
#define SAVE_RAWFILE_CORR FALSE
// *************************************************************
//        ---- manipulate calibration data ----
// Extract a calibration file from save files in Linrads .raw format
// and save them as the appropriate dsp_xxx_iqcorr file.
//#define SAVE_RAWFILE_IQCORR TRUE
#define SAVE_RAWFILE_IQCORR FALSE
// *************************************************************
//        ---- manipulate calibration data ----
// Use calibration data from the appropriate dsp_xx_corr file
// and disregard whatever data that may be contained in a .raw file
// This will also add calibration data to a .wav file
//#define DISREGARD_RAWFILE_CORR TRUE
#define DISREGARD_RAWFILE_CORR FALSE
// **************************************************************
//        ---- manipulate calibration data ----
// Skip the mirror image calibration that is present in a .raw file.
//#define DISREGARD_RAWFILE_IQCORR TRUE
#define DISREGARD_RAWFILE_IQCORR FALSE
// ************************************************************
//                ----  Debug tool  ----
// Linrad can display buffer usage as bars on screen when "T"
// is pressed to provide timing info. This option is however
// rather time-consuming and not often helpful 
// Look for info in bufbars.h
//#define BUFBARS TRUE
#define BUFBARS FALSE
#include "bufbars.h"
// **************************************************************
//           ---- Internal signal generator  ----
// When playing recorded files Linrad can replace or modify the
// data that it reads from the disk. This data can be used to check
// the performance of filters, AGC and possibly other things that
// depend on user selected parameters in Linrad. The data can be
// saved as .raw files which can be converted to .wav files and
// subsequently used for input to other SDR software.
// Define one of the generator modes TRUE and the other ones FALSE.
#define INTERNAL_GEN_CARRIER TRUE
#define INTERNAL_GEN_FILTER_TEST FALSE
#define INTERNAL_GEN_AGC_TEST FALSE
#define INTERNAL_GEN_ADD_AGCTEST FALSE


// *************************************************************
//            ---- Debug info for calibration ----
// Set MAX_CAL_DEBUG larger than 1 to see intermediate results
// from the filter calibration routine. Add display code
// as required in calibrate.c
#define MAX_CAL_DEBUG 1

// *************************************************************
//               ----   Baseband via network  ----
// For development purposes Linrad can send various signals via the
// network for analysis in another instance of Linrad on the same
// computer or on another computer. The default is to send BASEBAND_IQ,
// the baseband signal that has passed through the filter that is 
// shown in the baseband graph. 
//
// These defines must all be present and they must associate
// different numbers to the different formats that can be sent
// over the network. 
#define BASEBAND_IQ 1
#define WFM_FM_FULL_BANDWIDTH 2
#define WFM_AM_FULL_BANDWIDTH 3
#define WFM_FM_FIRST_LOWPASS 4
#define WFM_AM_FIRST_LOWPASS 5
#define WFM_FM_FIRST_LOWPASS_RESAMP 6
#define WFM_AM_FIRST_LOWPASS_RESAMP 7
#define WFM_FM_FIRST_LOWPASS_UPSAMP 8
#define WFM_SYNTHETIZED_FROM_FIRST_LOWPASS 9
#define WFM_SUBTRACT_FROM_FIRST_LOWPASS 10
#define WFM_STEREO_LOW 11
#define WFM_STEREO_HIGH 12
#define BASEBAND_IQ_FULL_BANDWIDTH 13
#define BASEBAND_IQ_TEST 14
#define BASEBAND_IQ_MASTER_TEST 15
 
// Here we select which one of the modes to send over the network.
//#define NET_BASEBRAW_MODE BASEBAND_IQ_FULL_BANDWIDTH
//#define NET_BASEBRAW_MODE BASEBAND_IQ_MASTER_TEST
#define NET_BASEBRAW_MODE BASEBAND_IQ
// **********************************************************************
// Disable the test for a serial number in Afedri.
//#define DISABLE_AFEDRI_SERIAL_NUMBER TRUE
#define DISABLE_AFEDRI_SERIAL_NUMBER FALSE
// **********************************************************************
// Show the frequency difference between the channels in the Allan window,
// (correlation_flag = 3)
#define SHOW_ALLAN_FREQDIFF TRUE
//#define SHOW_ALLAN_FREQDIFF FALSE
// **********************************************************************

