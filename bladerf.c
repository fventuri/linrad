// Copyright (c) <2019> <Leif Asbrink>
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


// For help on X11 give command: "man X Interface"
// Event masks and event definitions are in /usr/X11R6/include/X11/X.h

#include "osnum.h"
#include "globdef.h"
#include <string.h>
#include <stdbool.h>
#include "uidef.h"
#include "sdrdef.h"
#include "fft1def.h"
#include "screendef.h"
#include "thrdef.h"
#include "vernr.h"
#include "hwaredef.h"
#include "options.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

struct bladerf;
struct bladerf_stream;

#define BLADERF_ERR_UNEXPECTED  (-1)  /**< An unexpected failure occurred */
#define BLADERF_ERR_RANGE       (-2)  /**< Provided parameter is out of range */
#define BLADERF_ERR_INVAL       (-3)  /**< Invalid operation/parameter */
#define BLADERF_ERR_MEM         (-4)  /**< Memory allocation error */
#define BLADERF_ERR_IO          (-5)  /**< File/Device I/O error */
#define BLADERF_ERR_TIMEOUT     (-6)  /**< Operation timed out */
#define BLADERF_ERR_NODEV       (-7)  /**< No device(s) available */
#define BLADERF_ERR_UNSUPPORTED (-8)  /**< Operation not supported */
#define BLADERF_ERR_MISALIGNED  (-9)  /**< Misaligned flash access */
#define BLADERF_ERR_CHECKSUM    (-10) /**< Invalid checksum */

typedef enum {
    BLADERF_BACKEND_ANY,    /**< "Don't Care" -- use any available backend */
    BLADERF_BACKEND_LINUX,  /**< Linux kernel driver */
    BLADERF_BACKEND_LIBUSB  /**< libusb */
} bladerf_backend;

typedef enum {
    BLADERF_DEVICE_SPEED_UNKNOWN,
    BLADERF_DEVICE_SPEED_HIGH,
    BLADERF_DEVICE_SPEED_SUPER,
} bladerf_dev_speed;

#define BLADERF_SERIAL_LENGTH   33

struct bladerf_devinfo {
    bladerf_backend backend;    /**< Backend to use when connecting to device */
    char serial[BLADERF_SERIAL_LENGTH]; /**< Device serial number string */
    uint8_t usb_bus;            /**< Bus # device is attached to */
    uint8_t usb_addr;           /**< Device address on bus */
    unsigned int instance;      /**< Device instance or ID */
};

struct bladerf_rational_rate {
    uint64_t integer;           /**< Integer portion */
    uint64_t num;               /**< Numerator in fractional portion */
    uint64_t den;               /**< Denominator in fractional portion. This
                                     must be > 0. */
};




struct bladerf_stats {

    /** The number of times samples have been lost in the FPGA */
    uint64_t rx_overruns;

    /** The overall throughput of the device in samples/second */
    uint64_t rx_throughput;

    /**  Number of times samples have been too late to transmit to the FPGA */
    uint64_t tx_underruns;

    /** The overall throughput of the device in samples/second */
    uint64_t tx_throughput;
};

struct bladerf_version {
    uint16_t major;             /**< Major version */
    uint16_t minor;             /**< Minor version */
    uint16_t patch;             /**< Patch version */
    const char *describe;       /**< Version string with any additional suffix
                                 *   information.
                                 *
                                 *   @warning Do not attempt to modify or
                                 *            free() this string. */
};

typedef enum {
    BLADERF_FORMAT_SC16_Q12, /**< Signed, Complex 16-bit Q12.
                               *  This is the native format of the DAC data.
                               *
                               *  Samples are interleaved IQ value pairs, where
                               *  each value in the pair is an int16_t. For each
                               *  value, the data in the lower bits. The upper
                               *  bits are reserved.
                               *
                               *  When using this format, note that buffers
                               *  must be at least
                               *       2 * num_samples * sizeof(int16_t)
                               *  bytes large
                               */
} bladerf_format;

typedef enum {
    BLADERF_FPGA_UNKNOWN = 0,   /**< Unable to determine FPGA variant */
    BLADERF_FPGA_40KLE = 40,    /**< 40 kLE FPGA */
    BLADERF_FPGA_115KLE = 115   /**< 115 kLE FPGA */
} bladerf_fpga_size;

struct bladerf_metadata {
    uint32_t version;       /**< Metadata format version */
    uint64_t timestamp;     /**< Timestamp (TODO format TBD) */
};

/**
 * Loopback options
 */
typedef enum {

    /**
     * Firmware loopback inside of the FX3
     */
    BLADERF_LB_FIRMWARE = 1,

    /**
     * Baseband loopback. TXLPF output is connected to the RXVGA2 input.
     */
    BLADERF_LB_BB_TXLPF_RXVGA2,

    /**
     * Baseband loopback. TXVGA1 output is connected to the RXVGA2 input.
     */
    BLADERF_LB_BB_TXVGA1_RXVGA2,

    /**
     * Baseband loopback. TXLPF output is connected to the RXLPF input.
     */
    BLADERF_LB_BB_TXLPF_RXLPF,

    /**
     * Baseband loopback. TXVGA1 output is connected to RXLPF input.
     */
    BLADERF_LB_BB_TXVGA1_RXLPF,

    /**
     * RF loopback. The TXMIX output, through the AUX PA, is connected to the
     * output of LNA1.
     */
    BLADERF_LB_RF_LNA1,


    /**
     * RF loopback. The TXMIX output, through the AUX PA, is connected to the
     * output of LNA2.
     */
    BLADERF_LB_RF_LNA2,

    /**
     * RF loopback. The TXMIX output, through the AUX PA, is connected to the
     * output of LNA3.
     */
    BLADERF_LB_RF_LNA3,

    /**
     * Disables loopback and returns to normal operation.
     */
    BLADERF_LB_NONE

} bladerf_loopback;

typedef enum {
    BLADERF_SAMPLING_UNKNOWN,  /**< Unable to determine connection type */
    BLADERF_SAMPLING_INTERNAL, /**< Sample from RX/TX connector */
    BLADERF_SAMPLING_EXTERNAL  /**< Sample from J60 or J61 */
} bladerf_sampling;

typedef enum {
    BLADERF_LNA_GAIN_UNKNOWN,    /**< Invalid LNA gain */
    BLADERF_LNA_GAIN_BYPASS,     /**< LNA bypassed - 0dB gain */
    BLADERF_LNA_GAIN_MID,        /**< LNA Mid Gain (MAX-6dB) */
    BLADERF_LNA_GAIN_MAX         /**< LNA Max Gain */
} bladerf_lna_gain;

typedef enum {
    BLADERF_LPF_NORMAL,     /**< LPF connected and enabled */
    BLADERF_LPF_BYPASSED,   /**< LPF bypassed */
    BLADERF_LPF_DISABLED    /**< LPF disabled */
} bladerf_lpf_mode;

typedef enum
{
    BLADERF_MODULE_RX,  /**< Receive Module */
    BLADERF_MODULE_TX   /**< Transmit Module */
} bladerf_module;

/**
 * Expansion boards
 */
typedef enum {
    BLADERF_XB_NONE = 0,    /**< No expansion boards attached */
    BLADERF_XB_100,         /**< XB-100 GPIO expansion board.
                             *   This device is not yet supported in
                             *   libbladeRF, and is here as a placeholder
                             *   for future support. */
    BLADERF_XB_200          /**< XB-200 Transverter board */
} bladerf_xb;

/**
 * XB-200 filter selection options
 */
typedef enum {
    /** 50-54 MHz (6 meter band) filterbank */
    BLADERF_XB200_50M = 0,

    /** 144-148 MHz (2 meter band) filterbank */
    BLADERF_XB200_144M,

    /**
     * 222-225 MHz (1.25 meter band) filterbank.
     *
     * Note that this filter option is technically wider, covering 206-235 MHz.
     */
    BLADERF_XB200_222M,

    /**
     * This option enables the RX/TX module's custom filter bank path across the
     * associated FILT and FILT-ANT SMA connectors on the XB-200 board.
     *
     * For reception, it is often possible to simply connect the RXFILT and
     * RXFILT-ANT connectors with an SMA cable (effectively, "no filter"). This
     * allows for reception of signals outside of the frequency range of the
     * on-board filters, with some potential trade-off in signal quality.
     *
     * For transmission, <b>always</b> use an appropriate filter on the custom
     * filter path to avoid spurious emissions.
     *
     */
    BLADERF_XB200_CUSTOM,

    /**
     * When this option is selected, the other filter options are automatically
     * selected depending on the RX or TX module's current frequency, based upon
     * the 1dB points of the on-board filters.  For frequencies outside the
     * range of the on-board filters, the custom path is selected.
     */
    BLADERF_XB200_AUTO_1DB,

    /**
     * When this option is selected, the other filter options are automatically
     * selected depending on the RX or TX module's current frequency, based upon
     * the 3dB points of the on-board filters.  For frequencies outside the
     * range of the on-board filters, the custom path is selected.
     */
    BLADERF_XB200_AUTO_3DB
} bladerf_xb200_filter;

typedef enum
{
    BLADERF_DC_CAL_LPF_TUNING,
    BLADERF_DC_CAL_TX_LPF,
    BLADERF_DC_CAL_RX_LPF,
    BLADERF_DC_CAL_RXVGA2
} bladerf_cal_module;


typedef enum {
    BLADERF_LOG_LEVEL_VERBOSE,  /**< Verbose level logging */
    BLADERF_LOG_LEVEL_DEBUG,    /**< Debug level logging */
    BLADERF_LOG_LEVEL_INFO,     /**< Information level logging */
    BLADERF_LOG_LEVEL_WARNING,  /**< Warning level logging */
    BLADERF_LOG_LEVEL_ERROR,    /**< Error level logging */
    BLADERF_LOG_LEVEL_CRITICAL, /**< Fatal error level logging */
    BLADERF_LOG_LEVEL_SILENT    /**< No output */
} bladerf_log_level;

typedef void *(*bladerf_stream_cb)(struct bladerf *dev,
                                   struct bladerf_stream *stream,
                                   struct bladerf_metadata *meta,
                                   void *samples,
                                   size_t num_samples,
                                   void *user_data);


#if(OSNUM == OSNUM_WINDOWS)
HANDLE bladerf_libhandle;
#endif

#if(OSNUM == OSNUM_LINUX)
#include <dlfcn.h>
void *bladerf_libhandle;
#endif

typedef int (*p_bladerf_get_device_list)(struct bladerf_devinfo **devices);
typedef void (*p_bladerf_free_device_list)(struct bladerf_devinfo *devices);
typedef int (*p_bladerf_open_with_devinfo)(struct bladerf **device,
                                        struct bladerf_devinfo *devinfo);
typedef void (*p_bladerf_close)(struct bladerf *device);
typedef int (*p_bladerf_enable_module)(struct bladerf *dev,
                                   bladerf_module m, bool enable);
typedef int (*p_bladerf_set_sample_rate)(struct bladerf *dev,
                                      bladerf_module module,
                                      unsigned int rate,
                                      unsigned int *actual);
typedef int (*p_bladerf_set_sampling)(struct bladerf *dev,
                                   bladerf_sampling sampling);
typedef int (*p_bladerf_set_lna_gain)(struct bladerf *dev, bladerf_lna_gain gain);
typedef int (*p_bladerf_set_rxvga1)(struct bladerf *dev, int gain);
typedef int (*p_bladerf_set_rxvga2)(struct bladerf *dev, int gain);
typedef int (*p_bladerf_set_bandwidth)(struct bladerf *dev, bladerf_module module,
                                    unsigned int bandwidth,
                                    unsigned int *actual);
typedef int (*p_bladerf_select_band)(struct bladerf *dev, bladerf_module module,
                                  unsigned int frequency);
typedef int (*p_bladerf_set_lpf_mode)(struct bladerf *dev, bladerf_module module,
                                   bladerf_lpf_mode mode);
typedef int (*p_bladerf_set_frequency)(struct bladerf *dev,
                                    bladerf_module module,
                                    unsigned int frequency);
typedef int (*p_bladerf_init_stream)(struct bladerf_stream **stream,
                                  struct bladerf *dev,
                                  bladerf_stream_cb callback,
                                  void ***buffers,
                                  size_t num_buffers,
                                  bladerf_format format,
                                  size_t num_samples,
                                  size_t num_transfers,
                                  void *user_data);
typedef int (*p_bladerf_stream)(struct bladerf_stream *stream,
                             bladerf_module module);
typedef void (*p_bladerf_deinit_stream)(struct bladerf_stream *stream);
typedef int (*p_bladerf_get_fpga_size)(struct bladerf *dev,
                                    bladerf_fpga_size *size);
typedef int (*p_bladerf_fw_version)(struct bladerf *dev,
                                 struct bladerf_version *version);
typedef int (*p_bladerf_is_fpga_configured)(struct bladerf *dev);
typedef int (*p_bladerf_fpga_version)(struct bladerf *dev,
                                   struct bladerf_version *version);
typedef const char * (*p_bladerf_strerror)(int error);
typedef void (*p_bladerf_version)(struct bladerf_version *version);
typedef int (*p_bladerf_calibrate_dc)(struct bladerf *dev,
                                   bladerf_cal_module module);
typedef int (*p_bladerf_expansion_attach)(struct bladerf *dev,
                                                   bladerf_xb xb);
typedef int (*p_bladerf_xb200_set_filterbank)(struct bladerf *dev,
                                 bladerf_module module,
                                 bladerf_xb200_filter filter);
typedef int (*p_bladerf_set_loopback)(struct bladerf *dev, bladerf_loopback l);


p_bladerf_get_device_list bladerf_get_device_list;
p_bladerf_free_device_list bladerf_free_device_list;
p_bladerf_open_with_devinfo bladerf_open_with_devinfo;
p_bladerf_close bladerf_close;
p_bladerf_enable_module bladerf_enable_module;
p_bladerf_set_sample_rate bladerf_set_sample_rate;
p_bladerf_set_sampling bladerf_set_sampling;
p_bladerf_set_lna_gain bladerf_set_lna_gain;
p_bladerf_set_rxvga1 bladerf_set_rxvga1;
p_bladerf_set_rxvga2 bladerf_set_rxvga2;
p_bladerf_set_bandwidth bladerf_set_bandwidth;
p_bladerf_select_band bladerf_select_band;
p_bladerf_set_lpf_mode bladerf_set_lpf_mode;
p_bladerf_set_frequency bladerf_set_frequency;
p_bladerf_init_stream bladerf_init_stream;
p_bladerf_stream bladerf_stream;
p_bladerf_deinit_stream bladerf_deinit_stream;
p_bladerf_get_fpga_size bladerf_get_fpga_size;
p_bladerf_fw_version bladerf_fw_version;
p_bladerf_is_fpga_configured bladerf_is_fpga_configured;
p_bladerf_fpga_version bladerf_fpga_version;
p_bladerf_strerror bladerf_strerror;
p_bladerf_version bladerf_version;
p_bladerf_calibrate_dc bladerf_calibrate_dc;
p_bladerf_expansion_attach bladerf_expansion_attach;
p_bladerf_xb200_set_filterbank bladerf_xb200_set_filterbank;
p_bladerf_set_loopback bladerf_set_loopback;


// Structure for BladeRF parameters
typedef struct {
int rx_sampling_speed;
int rx_resampling_factor;
int sernum1;
int sernum2;
int sernum3;
int sernum4;
int sernum5;
int sernum6;
int sernum7;
int sernum8;
int freq_adjust;
int sampling;
int bandwidth;
int gain_mode;
int use_expansion;
int check;
}P_BLADERF;
#define MAX_BLADERF_PARM 16



P_BLADERF bladerf;

char *bladerf_parm_text[MAX_BLADERF_PARM]={"Rx sampling speed",
                                     "Rx resampling factor",
                                     "Serno1",
                                     "Serno2",
                                     "Serno3",
                                     "Serno4",
                                     "Serno5",
                                     "Serno6",
                                     "Serno7",
                                     "Serno8",
                                     "Freq adjust",
                                     "Sampling",
                                     "Bandwidth",
                                     "Gain mode",
                                     "Use expansion",
                                     "Check"};

char *bladerf_parfil_name="par_bladerf";

#define MIN_BLADERF_SAMP_RATE 79000
#define MAX_BLADERF_SAMP_RATE 40000000

struct bladerf *dev_bladerf;
int shutdown_rx_bladerf;


int usb_idx;
int usb_no_of_buffers;
void** usb_buffers;

#ifndef BLADERF_FORMAT_SC16_Q11
#define BLADERF_FORMAT_SC16_Q11 BLADERF_FORMAT_SC16_Q12 
#endif

#if(OSNUM == OSNUM_LINUX)
int load_bladerf_library(void)
{
int info;
info=0;
bladerf_libhandle=dlopen(BLADERF_LIBNAME, RTLD_LAZY);
if(bladerf_libhandle == NULL)goto load_error; 
info=1;
bladerf_get_device_list=(p_bladerf_get_device_list)
                          dlsym(bladerf_libhandle, "bladerf_get_device_list");
if(dlerror() != 0)goto sym_error;
bladerf_free_device_list=(p_bladerf_free_device_list)
                         dlsym(bladerf_libhandle, "bladerf_free_device_list");
if(dlerror() != 0)goto sym_error;
bladerf_open_with_devinfo=(p_bladerf_open_with_devinfo)
                        dlsym(bladerf_libhandle, "bladerf_open_with_devinfo");
if(dlerror() != 0)goto sym_error;
bladerf_close=(p_bladerf_close)dlsym(bladerf_libhandle, "bladerf_close");
if(dlerror() != 0)goto sym_error;
bladerf_enable_module=(p_bladerf_enable_module)
                           dlsym(bladerf_libhandle, "bladerf_enable_module");
if(dlerror() != 0)goto sym_error;
bladerf_set_sample_rate=(p_bladerf_set_sample_rate)
                        dlsym(bladerf_libhandle, "bladerf_set_sample_rate");
if(dlerror() != 0)goto sym_error;
bladerf_set_sampling=(p_bladerf_set_sampling)
                             dlsym(bladerf_libhandle, "bladerf_set_sampling");
if(dlerror() != 0)goto sym_error;
bladerf_set_lna_gain=(p_bladerf_set_lna_gain)
                            dlsym(bladerf_libhandle, "bladerf_set_lna_gain");
if(dlerror() != 0)goto sym_error;
bladerf_set_rxvga1=(p_bladerf_set_rxvga1)
                             dlsym(bladerf_libhandle, "bladerf_set_rxvga1");
if(dlerror() != 0)goto sym_error;
bladerf_set_rxvga2=(p_bladerf_set_rxvga2)
                            dlsym(bladerf_libhandle, "bladerf_set_rxvga2");
if(dlerror() != 0)goto sym_error;
bladerf_set_bandwidth=(p_bladerf_set_bandwidth)
                            dlsym(bladerf_libhandle, "bladerf_set_bandwidth");
if(dlerror() != 0)goto sym_error;
bladerf_select_band=(p_bladerf_select_band)
                             dlsym(bladerf_libhandle, "bladerf_select_band");
if(dlerror() != 0)goto sym_error;
bladerf_set_lpf_mode=(p_bladerf_set_lpf_mode)
                            dlsym(bladerf_libhandle, "bladerf_set_lpf_mode");
if(dlerror() != 0)goto sym_error;
bladerf_set_frequency=(p_bladerf_set_frequency)
                           dlsym(bladerf_libhandle, "bladerf_set_frequency");
if(dlerror() != 0)goto sym_error;
bladerf_init_stream=(p_bladerf_init_stream)dlsym
                                  (bladerf_libhandle, "bladerf_init_stream");
if(dlerror() != 0)goto sym_error;
bladerf_stream=(p_bladerf_stream)dlsym(bladerf_libhandle, "bladerf_stream");
if(dlerror() != 0)goto sym_error;
bladerf_deinit_stream=(p_bladerf_deinit_stream)
                            dlsym(bladerf_libhandle, "bladerf_deinit_stream");
if(dlerror() != 0)goto sym_error;
bladerf_get_fpga_size=(p_bladerf_get_fpga_size)dlsym
                                  (bladerf_libhandle, "bladerf_get_fpga_size");
if(dlerror() != 0)goto sym_error;
bladerf_fw_version=(p_bladerf_fw_version)
                            dlsym(bladerf_libhandle, "bladerf_fw_version");
if(dlerror() != 0)goto sym_error;
bladerf_is_fpga_configured=(p_bladerf_is_fpga_configured)
                       dlsym(bladerf_libhandle, "bladerf_is_fpga_configured");
if(dlerror() != 0)goto sym_error;
bladerf_fpga_version=(p_bladerf_fpga_version)dlsym
                                  (bladerf_libhandle, "bladerf_fpga_version");
if(dlerror() != 0)goto sym_error;
bladerf_strerror=(p_bladerf_strerror)
                                  dlsym(bladerf_libhandle, "bladerf_strerror");
if(dlerror() != 0)goto sym_error;
bladerf_version=(p_bladerf_version)dlsym(bladerf_libhandle, "bladerf_version");
if(dlerror() != 0)goto sym_error;
bladerf_calibrate_dc=(p_bladerf_calibrate_dc)
                              dlsym(bladerf_libhandle, "bladerf_calibrate_dc");
if(dlerror() != 0)goto sym_error;
bladerf_expansion_attach=(p_bladerf_expansion_attach)
                  dlsym(bladerf_libhandle, "bladerf_expansion_attach");
bladerf_xb200_set_filterbank=(p_bladerf_xb200_set_filterbank) 
                  dlsym(bladerf_libhandle, "bladerf_xb200_set_filterbank");
bladerf_set_loopback=(p_bladerf_set_loopback)
                  dlsym(bladerf_libhandle, "bladerf_set_loopback");
if(dlerror() != 0)goto sym_error;
return 0;
sym_error:;
dlclose(bladerf_libhandle);
load_error:;
library_error_screen(BLADERF_LIBNAME,info);
return -1;
}

void unload_bladerf_library(void)
{
dlclose(bladerf_libhandle);
}
#endif


#if(OSNUM == OSNUM_WINDOWS)

int load_bladerf_library(void)
{
char bladerf_dllname[80];
int info;
info=0;
sprintf(bladerf_dllname,"%sbladeRF.dll",DLLDIR);

bladerf_libhandle=LoadLibraryEx(bladerf_dllname, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
if(bladerf_libhandle == NULL)goto load_error; 
info=1;
bladerf_get_device_list=(p_bladerf_get_device_list)
                 (void*)GetProcAddress(bladerf_libhandle, "bladerf_get_device_list");
if(!bladerf_get_device_list)goto sym_error;
bladerf_free_device_list=(p_bladerf_free_device_list)
                (void*)GetProcAddress(bladerf_libhandle, "bladerf_free_device_list");
if(!bladerf_free_device_list)goto sym_error;
bladerf_open_with_devinfo=(p_bladerf_open_with_devinfo)
               (void*)GetProcAddress(bladerf_libhandle, "bladerf_open_with_devinfo");
if(!bladerf_open_with_devinfo)goto sym_error;
bladerf_close=(p_bladerf_close)
                          (void*)GetProcAddress(bladerf_libhandle, "bladerf_close");
if(!bladerf_close)goto sym_error;
bladerf_enable_module=(p_bladerf_enable_module)
                   (void*)GetProcAddress(bladerf_libhandle, "bladerf_enable_module");
if(!bladerf_enable_module)goto sym_error;
bladerf_set_sample_rate=(p_bladerf_set_sample_rate)
                 (void*)GetProcAddress(bladerf_libhandle, "bladerf_set_sample_rate");
if(!bladerf_set_sample_rate)goto sym_error;
bladerf_set_sampling=(p_bladerf_set_sampling)
                    (void*)GetProcAddress(bladerf_libhandle, "bladerf_set_sampling");
if(!bladerf_set_sampling)goto sym_error;
bladerf_set_lna_gain=(p_bladerf_set_lna_gain)
                    (void*)GetProcAddress(bladerf_libhandle, "bladerf_set_lna_gain");
if(!bladerf_set_lna_gain)goto sym_error;
bladerf_set_rxvga1=(p_bladerf_set_rxvga1)
                     (void*)GetProcAddress(bladerf_libhandle, "bladerf_set_rxvga1");
if(!bladerf_set_rxvga1)goto sym_error;
bladerf_set_rxvga2=(p_bladerf_set_rxvga2)
                    (void*)GetProcAddress(bladerf_libhandle, "bladerf_set_rxvga2");
if(!bladerf_set_rxvga2)goto sym_error;
bladerf_set_bandwidth=(p_bladerf_set_bandwidth)
                    (void*)GetProcAddress(bladerf_libhandle, "bladerf_set_bandwidth");
if(!bladerf_set_bandwidth)goto sym_error;
bladerf_select_band=(p_bladerf_select_band)
                     (void*)GetProcAddress(bladerf_libhandle, "bladerf_select_band");
if(!bladerf_select_band)goto sym_error;
bladerf_set_lpf_mode=(p_bladerf_set_lpf_mode)
                    (void*)GetProcAddress(bladerf_libhandle, "bladerf_set_lpf_mode");
if(!bladerf_set_lpf_mode)goto sym_error;
bladerf_set_frequency=(p_bladerf_set_frequency)
                   (void*)GetProcAddress(bladerf_libhandle, "bladerf_set_frequency");
if(!bladerf_set_frequency)goto sym_error;
bladerf_init_stream=(p_bladerf_init_stream)
                     (void*)GetProcAddress(bladerf_libhandle, "bladerf_init_stream");
if(!bladerf_init_stream)goto sym_error;
bladerf_stream=(p_bladerf_stream)
                         (void*)GetProcAddress(bladerf_libhandle, "bladerf_stream");
if(!bladerf_stream)goto sym_error;
bladerf_deinit_stream=(p_bladerf_deinit_stream)
                  (void*)GetProcAddress(bladerf_libhandle, "bladerf_deinit_stream");
if(!bladerf_deinit_stream)goto sym_error;
bladerf_get_fpga_size=(p_bladerf_get_fpga_size)(void*)GetProcAddress
                                (bladerf_libhandle, "bladerf_get_fpga_size");
if(!bladerf_get_fpga_size)goto sym_error;
bladerf_fw_version=(p_bladerf_fw_version)
                     (void*)GetProcAddress(bladerf_libhandle, "bladerf_fw_version");
if(!bladerf_fw_version)goto sym_error;
bladerf_is_fpga_configured=(p_bladerf_is_fpga_configured)
             (void*)GetProcAddress(bladerf_libhandle, "bladerf_is_fpga_configured");
if(!bladerf_is_fpga_configured)goto sym_error;
bladerf_fpga_version=(p_bladerf_fpga_version)
                   (void*)GetProcAddress(bladerf_libhandle, "bladerf_fpga_version");
if(!bladerf_fpga_version)goto sym_error;
bladerf_strerror=(p_bladerf_strerror)
                       (void*)GetProcAddress(bladerf_libhandle, "bladerf_strerror");
if(!bladerf_strerror)goto sym_error;
bladerf_version=(p_bladerf_version)
                        (void*)GetProcAddress(bladerf_libhandle, "bladerf_version");
if(!bladerf_version)goto sym_error;
bladerf_calibrate_dc=(p_bladerf_calibrate_dc)
                  (void*)GetProcAddress(bladerf_libhandle, "bladerf_calibrate_dc");
if(!bladerf_calibrate_dc)goto sym_error;
bladerf_expansion_attach=(p_bladerf_expansion_attach)
             (void*)GetProcAddress(bladerf_libhandle, "bladerf_expansion_attach");
bladerf_xb200_set_filterbank=(p_bladerf_xb200_set_filterbank) 
          (void*)GetProcAddress(bladerf_libhandle, "bladerf_xb200_set_filterbank");
bladerf_set_loopback=(p_bladerf_set_loopback)
                  (void*)GetProcAddress(bladerf_libhandle, "bladerf_set_loopback");

CoInitialize(NULL);
return 0;
sym_error:;
FreeLibrary(bladerf_libhandle);
load_error:;
library_error_screen(bladerf_dllname,info);
return -1;
}

void unload_bladerf_library(void)
{
FreeLibrary(bladerf_libhandle);
CoUninitialize();
}
#endif


void *bladerf_callback(struct bladerf *dev, 
                      struct bladerf_stream *stream,
                      struct bladerf_metadata *metadata, 
                      void *samples,
                      size_t num_samples, 
                      void *user_data)
{
(void)dev;
(void) stream;
(void)metadata;
(void)num_samples;
(void)user_data;
short int *buf1, *buf2;
short int x[8];
unsigned int i, n;
buf1=(short int*)samples;
switch (bladerf.rx_resampling_factor)
  {
  case 0:
  for(i=0; i<num_samples; i++)
    {
    buf2=(short int*)&timf1_char[timf1p_sdr];
    buf2[0]=buf1[2*i];
    buf2[1]=buf1[2*i+1];
    buf2[0]-=0x8000;
    buf2[1]-=0x8000;
    buf2[0]<<=4;
    buf2[1]<<=4;
    timf1p_sdr=(timf1p_sdr+4)&timf1_bytemask;
    }
  break;
  
  case 1:
  n=num_samples/2;
  for(i=0; i<n; i++)
    {
    buf2=(short int*)&timf1_char[timf1p_sdr];
    buf2[0]=buf1[4*i  ];
    buf2[1]=buf1[4*i+1];
    buf2[0]-=0x8000;
    buf2[1]-=0x8000;
    buf2[0]<<=4;
    buf2[1]<<=4;
    x[2]=buf1[4*i+2];
    x[3]=buf1[4*i+3];
    x[2]-=0x8000;
    x[3]-=0x8000;
    x[2]<<=4;
    x[3]<<=4;
    buf2[0]=buf2[0]/2+x[2]/2;
    buf2[1]=buf2[1]/2+x[3]/2;
    timf1p_sdr=(timf1p_sdr+4)&timf1_bytemask;
    }
  break;

  case 2:
  n=num_samples/4;
  for(i=0; i<n; i++)
    {
    buf2=(short int*)&timf1_char[timf1p_sdr];
    buf2[0]=buf1[8*i  ];
    buf2[1]=buf1[8*i+1];
    buf2[0]-=0x8000;
    buf2[1]-=0x8000;
    buf2[0]<<=4;
    buf2[1]<<=4;
    x[2]=buf1[8*i+2];
    x[3]=buf1[8*i+3];
    x[2]-=0x8000;
    x[3]-=0x8000;
    x[2]<<=4;
    x[3]<<=4;
    x[4]=buf1[8*i+4];
    x[5]=buf1[8*i+5];
    x[4]-=0x8000;
    x[5]-=0x8000;
    x[4]<<=4;
    x[5]<<=4;
    x[6]=buf1[8*i+4];
    x[7]=buf1[8*i+5];
    x[6]-=0x8000;
    x[7]-=0x8000;
    x[6]<<=4;
    x[7]<<=4;
    buf2[0]=buf2[0]/4+x[2]/4+x[4]/4+x[6]/4;
    buf2[1]=buf2[1]/4+x[3]/4+x[5]/4+x[7]/4;
    timf1p_sdr=(timf1p_sdr+4)&timf1_bytemask;
    }
  break;

  }  
if( ((timf1p_sdr-timf1p_pa+timf1_bytes)&timf1_bytemask) >= 
                                                     snd[RXAD].block_bytes)
  {
  lir_set_event(EVENT_HWARE1_RXREADY);
  }
if(shutdown_rx_bladerf != 0)
  {
  shutdown_rx_bladerf=2;
  fflush(NULL);
  return NULL;
  }
usb_idx=(usb_idx+1)%usb_no_of_buffers;
return usb_buffers[usb_idx];;
}

void bladerf_starter(void)
{
struct bladerf_stream * stream;
int i;
int k, no_of_buffers;
void **buffers;
k=snd[RXAD].block_bytes/4;
k=k<<bladerf.rx_resampling_factor;
if(k < 1024)k=1024;
// Allocate buffers for 50 ms minimum.
no_of_buffers=(int)((0.1*bladerf.rx_sampling_speed)/k);
// Never use less than 4 buffers.
if(no_of_buffers < 4)no_of_buffers=4;
while(thread_status_flag[THREAD_BLADERF_INPUT]!=THRFLAG_ACTIVE)
  {
  lir_sleep(10000);
  if(kill_all_flag)return;
  }
shutdown_rx_bladerf=0;
usb_idx=0;
usb_no_of_buffers=no_of_buffers;
bladerf_set_loopback(dev_bladerf, BLADERF_LB_NONE);
bladerf_enable_module(dev_bladerf, BLADERF_MODULE_RX, true);
i=bladerf_init_stream(&stream, 
                      dev_bladerf,
                      bladerf_callback,
                      &buffers,
                      no_of_buffers,
                      BLADERF_FORMAT_SC16_Q11,
                      k,
                      no_of_buffers,
                      NULL);
usb_buffers=buffers;
if(i<0)
  {
  lirerr(2376712);
  return;
  }
bladerf_stream(stream, BLADERF_MODULE_RX);
i=0;
while(i<50 && shutdown_rx_bladerf != 2)
  {
  lir_sleep(20000);
  }
bladerf_deinit_stream(stream);	
}

void calibrate_bladerf_rx(void)
{

if(bladerf.bandwidth != 0)
  {
  bladerf_calibrate_dc(dev_bladerf, BLADERF_DC_CAL_RX_LPF);
  bladerf_calibrate_dc(dev_bladerf, BLADERF_DC_CAL_LPF_TUNING);
  }
//bladerf_calibrate_dc(dev_bladerf, BLADERF_DC_CAL_RXVGA2);
}

void set_bladerf_frequency(void)
{
uint32_t frequency;
if((bladerf_sampling)bladerf.sampling == BLADERF_SAMPLING_INTERNAL)
  {
  frequency=(fg.passband_center*(100000000-bladerf.freq_adjust))/100;
  fg_truncation_error=
         frequency-(fg.passband_center*(100000000-bladerf.freq_adjust))/100;
  bladerf_set_frequency(dev_bladerf, BLADERF_MODULE_RX, frequency);
  bladerf_select_band(dev_bladerf, BLADERF_MODULE_RX, frequency);
  sys_func(THRFLAG_CALIBRATE_BLADERF_RX);
  }
} 	

void set_bladerf_att(void)
{
bladerf_lna_gain lna;
int vga2, vga1;
vga1=24;
if(bladerf.gain_mode == 0)
  {
  lna=BLADERF_LNA_GAIN_BYPASS;
  vga2=27;
  switch (fg.gain)
    {
    case 21:
    lna=BLADERF_LNA_GAIN_MAX;
    vga1=16;
    break;
    
    case 18:
    lna=BLADERF_LNA_GAIN_MID;
    vga1=24;
    break;

    case 15:
    lna=BLADERF_LNA_GAIN_MID;
    vga1=19;
    break;
    
    case 12:
    lna=BLADERF_LNA_GAIN_MID;
    vga1=14;
    break;
    
    case 9:
    vga1=30;
    break;
    
    case 6:
    vga1=28;
    break;
    
    case 3:
    vga1=28;
    vga2=24;
    break;
    
    case 0:
    vga1=28;
    vga2=21;
    break;
    
    case -3:
    vga1=26;
    vga2=21;
    break;
    
    case -6:
    vga1=22;
    vga2=21;
    break;
    
    case -9:
    vga1=17;
    vga2=21;
    break;
    
    case -12:
    vga1=12;
    vga2=21;
    break;
    
    case -15:
    vga1=7;
    vga2=21;
    break;
    
    case -18:
    vga1=7;
    vga2=18;
    break;

    case -21:
    vga1=7;
    vga2=15;
    break;    
    }
  }
else
  {
  vga2=3;
  lna=BLADERF_LNA_GAIN_MAX;
  switch (fg.gain)
    {
    case 21:
    vga2=18;
    break;

    case 18:
    vga2=15;
    break;

    case 15:
    vga2=12;
    break;

    case 12:
    vga2=9;
    break;

    case 9:
    vga2=6;
    break;

    case 6:
    break;

    case 3:
    vga1=19;
    break;

    case 0:
    vga1=13;
    break;

    case -3:
    vga1=8;
    break;

    case -6:
    lna=BLADERF_LNA_GAIN_MID;
    vga1=17;
    break;

    case -9:
    lna=BLADERF_LNA_GAIN_MID;
    vga1=11;
    break;

    case -12:
    lna=BLADERF_LNA_GAIN_MID;
    vga1=6;
    break;

    case -15:
    lna=BLADERF_LNA_GAIN_BYPASS;
    vga1=28;
    break;

    case -18:
    lna=BLADERF_LNA_GAIN_BYPASS;
    vga1=25;
    break;

    case -21:
    lna=BLADERF_LNA_GAIN_BYPASS;
    vga1=21;
    break;
    }
  }
bladerf_set_lna_gain(dev_bladerf, lna);
// vga1 range 5-33 step 1
bladerf_set_rxvga1(dev_bladerf, vga1);
// vga2 range 3-60 step 3 (above 30 not recommended.)
bladerf_set_rxvga2(dev_bladerf, vga2);
}


void bladerf_input(void)
{
int i, j;
struct bladerf_devinfo *devices = NULL;    
int rxin_local_workload_reset;
char s[128];
double dt1, read_start_time, total_reads;
int timing_loop_counter,timing_loop_counter_max,initial_skip_flag;
int local_att_counter;
int local_nco_counter;
int *ise;
int n_devices;
int idx;
float t1;
lir_init_event(EVENT_HWARE1_RXREADY);
if(load_bladerf_library() != 0)goto await_exit;
ise=(int*)(void*)s;
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_BLADERF_INPUT);
#endif
local_att_counter=sdr_att_counter;
local_nco_counter=sdr_nco_counter;
dt1=current_time();
i=read_sdrpar(bladerf_parfil_name, MAX_BLADERF_PARM, 
                                     bladerf_parm_text, (int*)((void*)&bladerf));
if(i != 0 || bladerf.check != BLADERFPAR_VERNR)
  {
  lirerr(1379);
  goto bladerf_init_error_exit;
  } 
t1=((rint)((bladerf.rx_sampling_speed*
                     (100000000L+(double)bladerf.freq_adjust))/100000000L));
if((ui.rx_ad_speed << bladerf.rx_resampling_factor) != (int)t1)
  {
  lirerr(1379);
  goto bladerf_init_error_exit;
  }
if( bladerf.check != BLADERFPAR_VERNR ||
   bladerf.rx_resampling_factor < 0 ||
   bladerf.rx_resampling_factor > 2 )
  {
  lirerr(1379);
  goto bladerf_init_error_exit;
  }
timf1p_sdr=timf1p_pa;  
j=0;

while(sdr == -1)
  {
  n_devices = bladerf_get_device_list(&devices);
  for(idx=0; idx<n_devices; idx++)
    {
    if(devices == NULL || strlen(devices[idx].serial) > 33)
      {
      lirerr(1543962);
      goto bladerf_init_error_exit;
      }
    sprintf(s,"%s",devices[idx].serial);
    if( bladerf.sernum1 == ise[0] &&
        bladerf.sernum2 == ise[1] &&
        bladerf.sernum3 == ise[2] &&
        bladerf.sernum4 == ise[3] &&
        bladerf.sernum5 == ise[4] &&
        bladerf.sernum6 == ise[5] &&
        bladerf.sernum7 == ise[6] &&
        bladerf.sernum8 == ise[7])
      { 
      sdr=bladerf_open_with_devinfo(&dev_bladerf, &devices[idx]);
      if(kill_all_flag)goto bladerf_init_error_exit;
      if(sdr >= 0)
        {
        i=bladerf_is_fpga_configured(dev_bladerf);
        if(i < 0)
          {
          lirerr(1380);
opnerr:;        
          bladerf_close(dev_bladerf);
          goto bladerf_init_error_exit;
          }
        if(i == 0)
          {
          lirerr(1381);
          goto opnerr;
          }
        i=bladerf_set_sampling(dev_bladerf, 
                                     (bladerf_sampling)bladerf.sampling);
        if(i<0) 
          {
          lirerr(1383);
          goto opnerr;
          }
        i=bladerf_set_sample_rate(dev_bladerf, BLADERF_MODULE_RX, 
                 (unsigned int)bladerf.rx_sampling_speed, (unsigned int*)&j);
        if(i<0)
          {
          lirerr(1385);
          goto opnerr;
          }
        if(abs(bladerf.rx_sampling_speed-j) != 0)
          {
          lirerr(1386);
          goto opnerr;
          }
        linrad_thread_create(THREAD_BLADERF_STARTER);
        break;
        }
      else
        {
        if(j==0)
          {
          clear_screen();
          j=1;
          }
        }
      }
    }
  sprintf(s,"Waiting %.2f", current_time()-dt1);
  lir_pixwrite(wg.xleft+4*text_width,wg.yborder-3*text_height,s);
  lir_refresh_screen();
  if(kill_all_flag)goto bladerf_init_error_exit;
  lir_sleep(100000);
  }
if((bladerf_sampling)bladerf.sampling == BLADERF_SAMPLING_INTERNAL)
  {  
  if(bladerf.bandwidth == 0)
    {
    bladerf_set_lpf_mode(dev_bladerf, BLADERF_MODULE_RX, BLADERF_LPF_BYPASSED);
//    bladerf_set_lpf_mode(dev_bladerf, BLADERF_MODULE_RX, BLADERF_LPF_DISABLED);
    }
  else
    {
    bladerf_set_lpf_mode(dev_bladerf, BLADERF_MODULE_RX, BLADERF_LPF_NORMAL);
    bladerf_set_bandwidth(dev_bladerf, BLADERF_MODULE_RX, 
                          (unsigned int)bladerf.bandwidth, (unsigned int*)&i);
    }
  }  
else
  {
  bladerf_set_lpf_mode(dev_bladerf, BLADERF_MODULE_RX, BLADERF_LPF_DISABLED);
  }
if(bladerf.use_expansion == 1)
  {
  if(bladerf_expansion_attach && bladerf_xb200_set_filterbank)
    {
    bladerf_expansion_attach(dev_bladerf, BLADERF_XB_200);
    bladerf_xb200_set_filterbank(dev_bladerf, BLADERF_MODULE_RX, 
                                                BLADERF_XB200_AUTO_3DB);
    }
  }  
if((bladerf_sampling)bladerf.sampling == BLADERF_SAMPLING_INTERNAL)
  {
  set_hardware_rx_gain();
  set_bladerf_att();
  set_bladerf_frequency();
  }
fft1_block_timing();
if(thread_command_flag[THREAD_SCREEN]!=THRFLAG_NOT_ACTIVE)
  {
  while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
    {
    if(thread_command_flag[THREAD_BLADERF_INPUT] ==
                                           THRFLAG_KILL)goto bladerf_error_exit1;
    lir_sleep(10000);
    }
  }
#include "timing_setup.c"
thread_status_flag[THREAD_BLADERF_INPUT]=THRFLAG_ACTIVE;
lir_status=LIR_OK;
while(!kill_all_flag && 
            thread_command_flag[THREAD_BLADERF_INPUT] == THRFLAG_ACTIVE)
  {
  if((bladerf_sampling)bladerf.sampling == BLADERF_SAMPLING_INTERNAL)
    {
    if(local_att_counter != sdr_att_counter)
      {
      local_att_counter=sdr_att_counter;
      set_bladerf_att();
      }
    if(local_nco_counter != sdr_nco_counter)
      {
      local_nco_counter=sdr_nco_counter;
      set_bladerf_frequency();
      }
    }  
  lir_await_event(EVENT_HWARE1_RXREADY);
  if(kill_all_flag)goto bladerf_error_exit1;
  while (!kill_all_flag && timf1p_sdr != timf1p_pa &&
              thread_command_flag[THREAD_BLADERF_INPUT] == THRFLAG_ACTIVE)
    {
#include "input_speed.c"
    finish_rx_read();
    }
  }
bladerf_error_exit1:;
shutdown_rx_bladerf=1;
i=0;
while(i<500 && shutdown_rx_bladerf != 2)
  {
  lir_sleep(2000);
  i++;
  }
lir_join(THREAD_BLADERF_STARTER);
bladerf_close(dev_bladerf);
bladerf_init_error_exit:;
unload_bladerf_library();
await_exit:;
sdr=-1;
thread_status_flag[THREAD_BLADERF_INPUT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_BLADERF_INPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
lir_close_event(EVENT_HWARE1_RXREADY);
} 

static __inline const char *backend2str(bladerf_backend b)
{
switch (b) 
  {
  case BLADERF_BACKEND_LIBUSB:
  return "libusb";

  case BLADERF_BACKEND_LINUX:
  return "Linux kernel driver";

  default:
  return "Unknown";
  }
}
                                                                    
static __inline const char *fpga_size2str(bladerf_fpga_size b)
{
switch (b) 
  {
  case BLADERF_FPGA_40KLE:
  return "40k LE";

  case BLADERF_FPGA_115KLE:
  return "115 kLE";

  default:
  return "Unknown";
  }
}



void init_bladerf(void)
{
FILE *bladerf_file;
int i, err, n_devices, devno, line, line0;
char s[120];
int *ise, *sdr_pi;
bladerf_fpga_size fpga_size;
bladerf_sampling sampling;
struct bladerf_version version, lib_version;
struct bladerf_devinfo *devices;
int j;
float t1;
if(load_bladerf_library() != 0)return;
ise=(int*)(void*)s;
devices=NULL;
n_devices = bladerf_get_device_list(&devices);
if (n_devices < 0) 
  {
  if (n_devices == BLADERF_ERR_NODEV) 
    {
    lir_text(5,5,"No bladeRF devices found.");
    } 
  else 
    {
    sprintf(s,"Failed to probe for devices: %s",bladerf_strerror(n_devices));
    lir_text(5,5,s);
    }
  lir_text(5,7,press_any_key);
  await_keyboard();
  goto unload_exit;
  }
line=2;
for(i = 0; i<n_devices; i++) 
  {
  for(j=0; j<33; j++)
  sprintf(s,"%d BladeRF:%s Ser:%s Bus/Addr: %d/%d ", 
     i, backend2str(devices[i].backend), devices[i].serial,
                                devices[i].usb_bus,devices[i].usb_addr);
  lir_text(3,line,s);
  line++;
  }
line++;
if(n_devices == 1)
  {
  lir_text(3, line, "Device autoselected");
  devno=0;
  }
else
  {  
  lir_text(3, line, "Select device by line number:");
  devno=lir_get_integer(32,line,2,0,n_devices-1);
  }
line++;
if(kill_all_flag)goto unload_exit;;
sprintf(s,"%s",devices[devno].serial);
bladerf.sernum1=ise[0];
bladerf.sernum2=ise[1];
bladerf.sernum3=ise[2];
bladerf.sernum4=ise[3];
bladerf.sernum5=ise[4];
bladerf.sernum6=ise[5];
bladerf.sernum7=ise[6];
bladerf.sernum8=ise[7];
err=bladerf_open_with_devinfo(&dev_bladerf, &devices[devno]);
bladerf_free_device_list(devices);
if(err<0)
  {
#if(OSNUM == OSNUM_WINDOWS)
  lirerr(1370);
#else
  lirerr(1369);
#endif
  goto unload_exit;;
  }
i=bladerf_is_fpga_configured(dev_bladerf);
err=bladerf_get_fpga_size(dev_bladerf, &fpga_size);
if(err<0)
  {
  lirerr(1371);
  goto errexit;
  }
if(i < 0)
  {
  lirerr(1373);
  goto errexit;
  }
if(i == 0)
  {
  if(fpga_size == BLADERF_FPGA_40KLE)
    {
    lirerr(1374);
    goto errexit;
    }
  if(fpga_size == BLADERF_FPGA_115KLE)
    {
    lirerr(1387);
    goto errexit;
  }
  lirerr(1388);
  goto errexit;
  }
err=bladerf_fpga_version(dev_bladerf, &version);
if(err < 0)
  {
  lirerr(1375);
  goto errexit;
  }
sprintf(s,"FPGA size: %s   FPGA version %d.%d.%d (%s),", 
                      fpga_size2str(fpga_size), version.major, 
                         version.minor, version.patch, version.describe);
lir_text(3,line,s);
line++;
err=bladerf_fw_version(dev_bladerf, &version);
if(err<0)
  {
  lirerr(1372);
  goto errexit;
  }
sprintf(s,"FX3 firmware version %d.%d.%d (%s),", version.major, 
                         version.minor, version.patch, version.describe);
lir_text(3,line,s);
line++;
bladerf_version (&lib_version);
sprintf(s,"libbladerf version %d.%d.%d (%s)",lib_version.major, lib_version.minor,
                               lib_version.patch,lib_version.describe);
lir_text(3,line,s);
line+=2;
sprintf(s,"Input via J61 (Y/N)");
lir_text(3,line,s);
line+=2;
getsamp:;
await_processed_keyboard();
if(kill_all_flag)goto errexit;
if(lir_inkey == 'Y')
  {
  sampling=BLADERF_SAMPLING_EXTERNAL;
  bladerf.use_expansion=0;
  }
else
  {
  if(lir_inkey != 'N')goto getsamp;  
  sampling=BLADERF_SAMPLING_INTERNAL;
  }
bladerf.sampling=(int)sampling;
err=bladerf_set_sampling(dev_bladerf, sampling);
if(err<0)
  {
  lirerr(1382);
  goto errexit;
  }
line0=line;
enter_speed:;  
sprintf(s,"Set RX sampling speed in Hz %d to %d =>",
                                    MIN_BLADERF_SAMP_RATE,MAX_BLADERF_SAMP_RATE);
lir_text(3,line,s);
bladerf.rx_sampling_speed=lir_get_integer(4+strlen(s),line,8,
                                MIN_BLADERF_SAMP_RATE, MAX_BLADERF_SAMP_RATE);
line++;
if(kill_all_flag)goto errexit;
err=bladerf_set_sample_rate(dev_bladerf, BLADERF_MODULE_RX, 
             (unsigned int)bladerf.rx_sampling_speed, 
                                  (unsigned int*)&bladerf.rx_sampling_speed);
if(err<0)
  {
  lirerr(1384);
  goto errexit;
  }
sprintf(s,"Actual RX sampling rate=%d",bladerf.rx_sampling_speed);
lir_text(3,line,s);
line+=2;
lir_text(3,line,"Enter resampling factor in powers of two (0-2) =>");
bladerf.rx_resampling_factor=lir_get_integer(53,line,1,0,4);
line+=2;
sprintf(s,"FFT bandwidth in Linrad %d", 
          bladerf.rx_sampling_speed/(1<<bladerf.rx_resampling_factor));
lir_text(3,line,s);
line+=2;
if(sampling == BLADERF_SAMPLING_INTERNAL)
  {
ask_filt:;
  lir_text(3,line,"Enable filter ? (Y/N) =>");
  await_processed_keyboard();
  line+=2;
  if(kill_all_flag)goto errexit;
  if(lir_inkey == 'N')
    {
    bladerf.bandwidth=0;
    }
  else 
    {
    if(lir_inkey != 'Y')
      {
      goto ask_filt;
      }
    sprintf(s,"Set filter bandwidth in kHz (1500 to 14000) =>");
    lir_text(3,line,s);
    bladerf.bandwidth=1000*lir_get_integer(4+strlen(s),line,6,1500,14000);
    if(kill_all_flag)goto errexit;
    line++;
    bladerf_set_bandwidth(dev_bladerf, BLADERF_MODULE_RX, (unsigned int)bladerf.bandwidth,  
                                            (unsigned int*)&bladerf.bandwidth);
    sprintf(s,"Actual bandwidth=%d",bladerf.bandwidth/1000);
    lir_text(3,line,s);
    line+=2;
speed_ok:;  
    lir_text(3,line,"Is this combination of rate and bandwidth OK? (Y/N) =>");
    await_processed_keyboard();
    line+=2;
    if(kill_all_flag)goto errexit;
    if(lir_inkey == 'N')
      {
      clear_lines(line0,line);
      line=line0;
      goto enter_speed;
      }
    else
      {
      if(lir_inkey != 'Y')
        {
        goto speed_ok;
        }
      }
    }  
  lir_text(3,line,"Select gain mode.");
  line++;
  lir_text(3,line,"0=Linearity.");
  line++;
  lir_text(3,line,"1=Sensitivity.");
  line+=2;
  lir_text(3,line,"=>");
  bladerf.gain_mode=lir_get_integer(6,line,1,0,1);
  line+=2;
  }
else
  {
  bladerf.bandwidth=0;
  bladerf.gain_mode=0;
  }  
if(line > 3*screen_last_line/4)
  {
  clear_screen();
  line=2;
  }
lir_text(3,line,"Enter xtal error in ppb =>");
bladerf.freq_adjust=0.1*lir_get_float(32,line,9,-300000.,300000.);
line+=2;
if(bladerf.sampling == BLADERF_SAMPLING_INTERNAL)
  {
  if(!bladerf_expansion_attach || !bladerf_xb200_set_filterbank)
    {
    lir_text(3,line,"The bladeRF library does not support XB-200. (Too old.)");
    bladerf.use_expansion=0;
    line+=2;
    lir_text(8,line,press_any_key);
    await_processed_keyboard();
    }
  else
    {   
    lir_text(3,line,"Use XB-200 expansion board (Y/N)");
expb:;
    await_processed_keyboard();
    if(kill_all_flag)goto errexit;
    if(lir_inkey == 'N')
      {
      bladerf.use_expansion=0;
      }
    else
      {
      if(lir_inkey != 'Y')goto expb;
      bladerf.use_expansion=1;
      }
    }      
  }
bladerf_file=fopen(bladerf_parfil_name,"w");
if(bladerf_file == NULL)
  {
  lirerr(381268);
  goto errexit;
  }
bladerf.check=BLADERFPAR_VERNR;
sdr_pi=(int*)(&bladerf);
for(i=0; i<MAX_BLADERF_PARM; i++)
  {
  fprintf(bladerf_file,"%s [%d]\n",bladerf_parm_text[i],sdr_pi[i]);
  }
parfile_end(bladerf_file);
ui.rx_addev_no=BLADERF_DEVICE_CODE;
t1=((rint)((bladerf.rx_sampling_speed*
                     (100000000L+(double)bladerf.freq_adjust))/100000000L));
ui.rx_ad_speed=(int)t1;
ui.rx_ad_speed>>=bladerf.rx_resampling_factor;
if(sampling == BLADERF_SAMPLING_INTERNAL)
  {
  ui.rx_input_mode=IQ_DATA;
  ui.rx_rf_channels=1;
  }
else
  {
  ui.rx_input_mode=0;
  ui.rx_rf_channels=2;
  }
ui.rx_ad_channels=2;
ui.rx_admode=0;
errexit:;
bladerf_close(dev_bladerf);
unload_exit:;
unload_bladerf_library();
}

int display_bladerf_parm_info(int *line)
{
char s[80], ss[256];
char *conns[]={"Unknown","SMA and tuner","J61"};
char *sp;
int errcod;
errcod=read_sdrpar(bladerf_parfil_name, MAX_BLADERF_PARM, 
                            bladerf_parm_text, (int*)((void*)&bladerf));
if(errcod == 0)
  {
  settextcolor(7);
  sprintf(s,"Sampling rate=%i Hz, Resampling=%d, FFT bandwidth=%.3f kHz  ",
            bladerf.rx_sampling_speed, bladerf.rx_resampling_factor,
        0.001*(bladerf.rx_sampling_speed/(1<<bladerf.rx_resampling_factor)));
  lir_text(24,line[0],s);
  line[0]++;
  SNDLOG"\n%s",s);
  sprintf(s,"Xtal adjust = %.0f ppb", 10.*bladerf.freq_adjust);
  lir_text(24,line[0],s);
  line[0]++;
  SNDLOG"\n%s",s);
  sprintf(s,"Connection via %s", conns[bladerf.sampling]);
  if(bladerf.sampling == BLADERF_SAMPLING_INTERNAL)
    {
    if(bladerf.bandwidth == 0)
      {
      sprintf(ss,"%s, Low pass filter bypassed.",s);  
      }
    else
      {   
      sprintf(ss,"%s, Low pass filter bandwidth %d kHz.",s,
                                                     bladerf.bandwidth/1000);
      }
    lir_text(24,line[0],ss);
    SNDLOG"\n%s",ss);
    line[0]++;
    if(bladerf.gain_mode == 0)
      {
      sp="linearity";
      }
    else
      {
      sp="sensitivity";
      }
    sprintf(s,"Gain mode = %s",sp);
    lir_text(24,line[0],s);
    SNDLOG"\n%s",s);
    line[0]++;
    if(bladerf.use_expansion == 1)
      {
      lir_text(24,line[0],"XB-200 expansion board used.");
      line[0]++;
      } 
    }
  }  
return (errcod);
}

