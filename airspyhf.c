/*
Copyright (c) 2013-2018, Youssef Touil <youssef@airspy.com>
Copyright (c) 2013-2017, Ian Gilmour <ian@sdrsharp.com>
Copyright (c) 2013-2017, Benjamin Vernoux <bvernoux@airspy.com>
Copyright (c) 2013, Michael Ossmann <mike@ossmann.com>
Copyright (c) 2012, Jared Boone <jared@sharebrained.com>

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

		Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
		Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the 
		documentation and/or other materials provided with the distribution.
		Neither the name of Airspy HF+ nor the names of its contributors may be used to endorse or promote products derived from this software
		without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef __AIRSPYHF_H__
#define __AIRSPYHF_H__

#include <stdint.h>

#define AIRSPYHF_VERSION "1.3.0"
#define AIRSPYHF_VER_MAJOR 1
#define AIRSPYHF_VER_MINOR 3
#define AIRSPYHF_VER_REVISION 0

#define AIRSPYHF_ENDPOINT_IN (1)

#if defined(_WIN32) && !defined(STATIC_AIRSPYHFPLUS)
	 #define ADD_EXPORTS
	 
//	 You should define ADD_EXPORTS *only* when building the DLL. 
	#ifdef ADD_EXPORTS
		#define ADDAPI __declspec(dllexport)
	#else
		#define ADDAPI __declspec(dllimport)
	#endif

// Define calling convention in one place, for convenience. 
	#define ADDCALL __cdecl

#else // _WIN32 not defined.

	// Define with no value on non-Windows OSes.
	#define ADDAPI
	#define ADDCALL

#endif

#ifdef __cplusplus
extern "C"
{
#endif

enum airspyhf_error
{
	AIRSPYHF_SUCCESS = 0,
	AIRSPYHF_ERROR = -1
};

typedef struct {
	float re;
	float im;
} airspyhf_complex_float_t;

typedef struct {
	uint32_t part_id;
	uint32_t serial_no[4];
} airspyhf_read_partid_serialno_t;

enum airspyhf_board_id
{
	AIRSPYHF_BOARD_ID_UNKNOWN_AIRSPYHF = 0,
	AIRSPYHF_BOARD_ID_AIRSPYHF_REV_A = 1,
	AIRSPYHF_BOARD_ID_INVALID = 0xFF,
};

typedef enum
{
	AIRSPYHF_USER_OUTPUT_0 = 0,
	AIRSPYHF_USER_OUTPUT_1 = 1,
	AIRSPYHF_USER_OUTPUT_2 = 2,
	AIRSPYHF_USER_OUTPUT_3 = 3
} airspyhf_user_output_t;

typedef enum
{
	AIRSPYHF_USER_OUTPUT_LOW	= 0,
	AIRSPYHF_USER_OUTPUT_HIGH	= 1
} airspyhf_user_output_state_t;

typedef struct airspyhf_device airspyhf_device_t;

typedef struct {
	airspyhf_device_t* device;
	void* ctx;
	airspyhf_complex_float_t* samples;
	int sample_count;
	uint64_t dropped_samples;
} airspyhf_transfer_t;

typedef struct {
	uint32_t major_version;
	uint32_t minor_version;
	uint32_t revision;
} airspyhf_lib_version_t;

#define MAX_VERSION_STRING_SIZE (64)

typedef int (*airspyhf_sample_block_cb_fn) (airspyhf_transfer_t* transfer_fn);

extern ADDAPI void ADDCALL airspyhf_lib_version(airspyhf_lib_version_t* lib_version);
extern ADDAPI int ADDCALL airspyhf_list_devices(uint64_t *serials, int count);
extern ADDAPI int ADDCALL airspyhf_open(airspyhf_device_t** device);
extern ADDAPI int ADDCALL airspyhf_open_sn(airspyhf_device_t** device, uint64_t serial_number);
extern ADDAPI int ADDCALL airspyhf_close(airspyhf_device_t* device);
extern ADDAPI int ADDCALL airspyhf_start(airspyhf_device_t* device, airspyhf_sample_block_cb_fn callback, void* ctx);
extern ADDAPI int ADDCALL airspyhf_stop(airspyhf_device_t* device);
extern ADDAPI int ADDCALL airspyhf_is_streaming(airspyhf_device_t* device);
extern ADDAPI int ADDCALL airspyhf_set_freq(airspyhf_device_t* device, const uint32_t freq_hz);
// Enables/Disables the IQ Correction, IF shift and Fine Tuning. 
extern ADDAPI int ADDCALL airspyhf_set_lib_dsp(airspyhf_device_t* device, const uint8_t flag); 
extern ADDAPI int ADDCALL airspyhf_get_samplerates(airspyhf_device_t* device, uint32_t* buffer, const uint32_t len);
extern ADDAPI int ADDCALL airspyhf_set_samplerate(airspyhf_device_t* device, uint32_t samplerate);
extern ADDAPI int ADDCALL airspyhf_get_calibration(airspyhf_device_t* device, int32_t* ppb);
extern ADDAPI int ADDCALL airspyhf_set_calibration(airspyhf_device_t* device, int32_t ppb);
extern ADDAPI int ADDCALL airspyhf_set_optimal_iq_correction_point(airspyhf_device_t* device, float w);
extern ADDAPI int ADDCALL airspyhf_iq_balancer_configure(airspyhf_device_t* device, int buffers_to_skip, int fft_integration, int fft_overlap, int correlation_integration);
extern ADDAPI int ADDCALL airspyhf_flash_calibration(airspyhf_device_t* device);
extern ADDAPI int ADDCALL airspyhf_board_partid_serialno_read(airspyhf_device_t* device, airspyhf_read_partid_serialno_t* read_partid_serialno);
extern ADDAPI int ADDCALL airspyhf_version_string_read(airspyhf_device_t* device, char* version, uint8_t length);
extern ADDAPI int ADDCALL airspyhf_set_user_output(airspyhf_device_t* device, airspyhf_user_output_t pin, airspyhf_user_output_state_t value);
extern ADDAPI int ADDCALL airspyhf_set_hf_agc(airspyhf_device_t* device, uint8_t flag);
// Possible values: 0..8 Range: 0..48 dB Attenuation with 6 dB steps extern ADDAPI int ADDCALL airspyhf_set_hf_agc_threshold(airspyhf_device_t* device, uint8_t flag);
extern ADDAPI int ADDCALL airspyhf_set_hf_att(airspyhf_device_t* device, uint8_t value); 
extern ADDAPI int ADDCALL airspyhf_set_hf_lna(airspyhf_device_t* device, uint8_t flag);

#ifdef __cplusplus
}
#endif

#endif//__AIRSPYHF_H__
*/

#include "osnum.h"
#include "globdef.h"
#include <string.h>
#include "uidef.h"
#include "fft1def.h"
#include "screendef.h"
#include "vernr.h"
#include "thrdef.h"
#include "hwaredef.h"
#include "options.h"
#include "sdrdef.h"
#include "seldef.h"

typedef struct airspyhf_device airspyhf_device_t;
struct airspyhf_device;

typedef struct {
	float re;
	float im;
} airspyhf_complex_float_t;

typedef struct {
	uint32_t part_id;
	uint32_t serial_no[4];
} airspyhf_read_partid_serialno_t;

typedef struct {
	airspyhf_device_t* device;
	void* ctx;
	airspyhf_complex_float_t* samples;
	int sample_count;
	uint64_t dropped_samples;
} airspyhf_transfer_t;

typedef struct {
	uint32_t major_version;
	uint32_t minor_version;
	uint32_t revision;
} airspyhf_lib_version_t;



typedef int (*airspyhf_sample_block_cb_fn) 
                                    (airspyhf_transfer_t* transfer_fn);


#define MAX_AIRSPYHF_DEVICES 8
#define AIRSPYHF_SUCCESS 0
#define AIRSPYHF_ERROR -1
#define MAX_VERSION_STRING_SIZE 64


typedef void (*p_airspyhf_lib_version)(airspyhf_lib_version_t* lib_version);
typedef int  (*p_airspyhf_version_string_read)(struct airspyhf_device* device,
                                             char* version, uint8_t length);
typedef int (*p_airspyhf_open)(struct airspyhf_device** device);
typedef int (*p_airspyhf_close)(struct airspyhf_device* device);
typedef int (*p_airspyhf_board_partid_serialno_read)
                           (struct airspyhf_device* device, 
                               airspyhf_read_partid_serialno_t* id);
typedef int (*p_airspyhf_get_samplerates)(struct airspyhf_device* device, 
                                    uint32_t* buffer, const uint32_t len);
typedef int (*p_airspyhf_set_samplerate)(struct airspyhf_device* device, 
                                                           uint32_t rate);
typedef int (*p_airspyhf_start)(struct airspyhf_device* device, 
                      airspyhf_sample_block_cb_fn callback, void* ctx);
typedef int (*p_airspyhf_set_freq)(struct airspyhf_device* device, 
                                                const uint32_t freq_hz);
typedef int (*p_airspyhf_stop)(airspyhf_device_t* device);
typedef int (*p_airspyhf_set_optimal_iq_correction_point)(airspyhf_device_t* device, float w);
typedef int (*p_airspyhf_iq_balancer_configure)(airspyhf_device_t* device, 
                int buffers_to_skip, int fft_integration, int fft_overlap, 
                int correlation_integration);
typedef int (*p_airspyhf_set_lib_dsp)(airspyhf_device_t* device, const uint8_t flag); 
typedef int (*p_airspyhf_set_hf_agc)(airspyhf_device_t* device, uint8_t flag);
typedef int (*p_airspyhf_set_hf_agc_threshold)(airspyhf_device_t* device, uint8_t flag);
// Possible hf att values: 0..8 Range: 0..48 dB Attenuation with 6 dB steps extern 
typedef int (*p_airspyhf_set_hf_att)(airspyhf_device_t* device, uint8_t value); 
typedef int (*p_airspyhf_set_hf_lna)(airspyhf_device_t* device, uint8_t flag);

                              
p_airspyhf_open airspyhf_open;
p_airspyhf_close airspyhf_close;
p_airspyhf_board_partid_serialno_read airspyhf_board_partid_serialno_read;
p_airspyhf_get_samplerates airspyhf_get_samplerates;
p_airspyhf_set_samplerate airspyhf_set_samplerate;
p_airspyhf_start airspyhf_start;
p_airspyhf_stop airspyhf_stop;
p_airspyhf_set_freq airspyhf_set_freq;
p_airspyhf_set_optimal_iq_correction_point airspyhf_set_optimal_iq_correction_point;
p_airspyhf_iq_balancer_configure airspyhf_iq_balancer_configure;
p_airspyhf_set_lib_dsp airspyhf_set_lib_dsp;
p_airspyhf_set_hf_agc airspyhf_set_hf_agc;
p_airspyhf_set_hf_agc_threshold airspyhf_set_hf_agc_threshold;
// Possible hf att values: 0..8 Range: 0..48 dB Attenuation with 6 dB steps extern 
p_airspyhf_set_hf_att airspyhf_set_hf_att;
p_airspyhf_set_hf_lna airspyhf_set_hf_lna;
p_airspyhf_lib_version airspyhf_lib_version;
p_airspyhf_version_string_read airspyhf_version_string_read;


char *airspyhf_parfil_name="par_airspyhf";
struct airspyhf_device *dev_airspyhf;
int airspyhf_library_flag;
int airspyhf_cancel_flag;

P_AIRSPYHF airspyhf;


char *airspyhf_parm_text[MAX_AIRSPYHF_PARM]={"Ratenum",
                                         "Serno1",
                                         "Serno2",
                                         "Freq adjust",
                                         "DSP",
                                         "HF AGC",
                                         "HF Threshold",
                                         "HF ATT",
                                         "HF LNA",
                                         "Buffers to skip",
                                         "FFT integration",
                                         "FFT overlap",
                                         "Correlation integration",
                                         "Check"};



#if(OSNUM == OSNUM_WINDOWS)
#include <windows.h>
#include "wscreen.h"
HANDLE airspyhf_libhandle;


void load_airspyhf_library(void)
{
char airspyhf_dllname[80];
int info;
if(airspyhf_library_flag)return;
info=0;
sprintf(airspyhf_dllname,"%sairspyhf.dll",DLLDIR);
airspyhf_libhandle=LoadLibraryEx(airspyhf_dllname, NULL, 
                                                LOAD_WITH_ALTERED_SEARCH_PATH);
if(!airspyhf_libhandle)goto airspyhf_load_error;
info=1;
airspyhf_open=(p_airspyhf_open)(void*)GetProcAddress(airspyhf_libhandle,
                                                           "airspyhf_open");
if(!airspyhf_open)goto airspyhf_sym_error;
airspyhf_lib_version=(p_airspyhf_lib_version)(void*)GetProcAddress(
                        airspyhf_libhandle,"airspyhf_lib_version");
if(!airspyhf_lib_version)goto airspyhf_sym_error;
airspyhf_version_string_read=(p_airspyhf_version_string_read)(void*)
            GetProcAddress(airspyhf_libhandle, "airspyhf_version_string_read");
if(!airspyhf_version_string_read)goto airspyhf_sym_error;
airspyhf_board_partid_serialno_read=(p_airspyhf_board_partid_serialno_read) 
               (void*)GetProcAddress(airspyhf_libhandle, 
                                    "airspyhf_board_partid_serialno_read");
if(!airspyhf_board_partid_serialno_read)goto airspyhf_sym_error;
airspyhf_close=(p_airspyhf_close)(void*)GetProcAddress(airspyhf_libhandle, "airspyhf_close");
if(!airspyhf_close)goto airspyhf_sym_error;
airspyhf_get_samplerates=(p_airspyhf_get_samplerates)(void*)GetProcAddress(airspyhf_libhandle, 
                                                   "airspyhf_get_samplerates");
if(!airspyhf_get_samplerates)goto airspyhf_sym_error;
airspyhf_set_samplerate=(p_airspyhf_set_samplerate)(void*)GetProcAddress(airspyhf_libhandle, 
                                                   "airspyhf_set_samplerate");
if(!airspyhf_set_samplerate)goto airspyhf_sym_error;
airspyhf_start=(p_airspyhf_start)(void*)GetProcAddress(airspyhf_libhandle, 
                                                       "airspyhf_start");
if(!airspyhf_start)goto airspyhf_sym_error;
airspyhf_stop=(p_airspyhf_stop)(void*)GetProcAddress(airspyhf_libhandle, 
                                                       "airspyhf_stop");
if(!airspyhf_stop)goto airspyhf_sym_error;
airspyhf_set_freq=(p_airspyhf_set_freq)(void*)GetProcAddress(airspyhf_libhandle, 
                                                       "airspyhf_set_freq");
if(!airspyhf_set_freq)goto airspyhf_sym_error;
airspyhf_set_optimal_iq_correction_point=(p_airspyhf_set_optimal_iq_correction_point)
        (void*)GetProcAddress(airspyhf_libhandle, "airspyhf_set_optimal_iq_correction_point");
airspyhf_iq_balancer_configure=(p_airspyhf_iq_balancer_configure)(void*)GetProcAddress(
             airspyhf_libhandle,"airspyhf_iq_balancer_configure"); 
airspyhf_set_lib_dsp=(p_airspyhf_set_lib_dsp)(void*)GetProcAddress(airspyhf_libhandle,
                                                    "airspyhf_set_lib_dsp");
airspyhf_set_hf_agc=(p_airspyhf_set_hf_agc)(void*)GetProcAddress(airspyhf_libhandle,
                                                       "airspyhf_set_hf_agc");
airspyhf_set_hf_agc_threshold=(p_airspyhf_set_hf_agc_threshold)
            (void*)GetProcAddress(airspyhf_libhandle,"airspyhf_set_hf_agc_threshold");
airspyhf_set_hf_att=(p_airspyhf_set_hf_att)(void*)GetProcAddress(airspyhf_libhandle,
                                                       "airspyhf_set_hf_att");
airspyhf_set_hf_lna=(p_airspyhf_set_hf_lna)(void*)GetProcAddress(airspyhf_libhandle,
                                                         "airspyhf_set_hf_lna");
airspyhf_library_flag=TRUE;
return;
airspyhf_sym_error:;

FreeLibrary(airspyhf_libhandle);
airspyhf_load_error:;
library_error_screen(airspyhf_dllname,info);
}

void unload_airspyhf_library(void)
{
if(!airspyhf_library_flag)return;
FreeLibrary(airspyhf_libhandle);
airspyhf_library_flag=FALSE;
}
#endif


#if(OSNUM == OSNUM_LINUX)
#include <dlfcn.h>
#include "lscreen.h"
static void *airspyhf_libhandle=NULL;

void load_airspyhf_library(void)
{ 
int info;
info=0;
if(airspyhf_library_flag)return;
airspyhf_libhandle=dlopen(AIRSPYHF_LIBNAME, RTLD_LAZY);
if(!airspyhf_libhandle)goto airspyhf_load_error;
info=1;
airspyhf_open=(p_airspyhf_open)dlsym(airspyhf_libhandle, "airspyhf_open");
if(dlerror() != 0)goto airspyhf_sym_error;
airspyhf_lib_version=(p_airspyhf_lib_version)
                         dlsym(airspyhf_libhandle,"airspyhf_lib_version");
if(dlerror() != 0)goto airspyhf_sym_error;
airspyhf_version_string_read=(p_airspyhf_version_string_read)
                      dlsym(airspyhf_libhandle, "airspyhf_version_string_read");
if(dlerror() != 0)goto airspyhf_sym_error;
airspyhf_board_partid_serialno_read=(p_airspyhf_board_partid_serialno_read) 
               dlsym(airspyhf_libhandle, "airspyhf_board_partid_serialno_read");
if(dlerror() != 0)goto airspyhf_sym_error;
airspyhf_close=(p_airspyhf_close)dlsym(airspyhf_libhandle, "airspyhf_close");
if(dlerror() != 0)goto airspyhf_sym_error;
airspyhf_get_samplerates=(p_airspyhf_get_samplerates)dlsym(airspyhf_libhandle, 
                                                   "airspyhf_get_samplerates");
if(dlerror() != 0)goto airspyhf_sym_error;
airspyhf_set_samplerate=(p_airspyhf_set_samplerate)dlsym(airspyhf_libhandle, 
                                                   "airspyhf_set_samplerate");
if(dlerror() != 0)goto airspyhf_sym_error;
airspyhf_start=(p_airspyhf_start)dlsym(airspyhf_libhandle, 
                                                     "airspyhf_start");
if(dlerror() != 0)goto airspyhf_sym_error;
airspyhf_stop=(p_airspyhf_stop)dlsym(airspyhf_libhandle, 
                                                     "airspyhf_stop");
if(dlerror() != 0)goto airspyhf_sym_error;
airspyhf_set_freq=(p_airspyhf_set_freq)dlsym(airspyhf_libhandle, 
                                                       "airspyhf_set_freq");
if(dlerror() != 0)goto airspyhf_sym_error;
airspyhf_set_optimal_iq_correction_point=(p_airspyhf_set_optimal_iq_correction_point)
                  dlsym(airspyhf_libhandle,"airspyhf_set_optimal_iq_correction_point");
airspyhf_iq_balancer_configure=(p_airspyhf_iq_balancer_configure)
              dlsym(airspyhf_libhandle,"airspyhf_iq_balancer_configure"); 
airspyhf_set_lib_dsp=(p_airspyhf_set_lib_dsp)dlsym(airspyhf_libhandle,
                                                    "airspyhf_set_lib_dsp");
airspyhf_set_hf_agc=(p_airspyhf_set_hf_agc)dlsym(airspyhf_libhandle,
                                                       "airspyhf_set_hf_agc");
airspyhf_set_hf_agc_threshold=(p_airspyhf_set_hf_agc_threshold)
            dlsym(airspyhf_libhandle,"airspyhf_set_hf_agc_threshold");
airspyhf_set_hf_att=(p_airspyhf_set_hf_att)dlsym(airspyhf_libhandle,
                                                       "airspyhf_set_hf_att");
airspyhf_set_hf_lna=(p_airspyhf_set_hf_lna)dlsym(airspyhf_libhandle,
                                                         "airspyhf_set_hf_lna");
airspyhf_library_flag=TRUE;
return;
airspyhf_sym_error:;
dlclose(airspyhf_libhandle);
airspyhf_library_flag=FALSE;
airspyhf_load_error:;
library_error_screen(AIRSPYHF_LIBNAME,info);
}


void unload_airspyhf_library(void)
{
if(!airspyhf_library_flag)return;
dlclose(airspyhf_libhandle);
airspyhf_library_flag=FALSE;
}


#endif


int airspyhf_callback(airspyhf_transfer_t* transfer)
{
int i;
int *iz;
airspyhf_complex_float_t *ix;
if(airspyhf_cancel_flag)
  {
  if(airspyhf_cancel_flag == -1)
    {
    return -1;
    }
  else  
    {
    return -0;
    }
  }
ix=transfer->samples;
for(i=0; i<transfer->sample_count; i++)
  {
  iz=(int*)&timf1_char[timf1p_sdr];
  iz[0]=0x55000000*ix[i].re;
  iz[1]=0x55000000*ix[i].im;
  timf1p_sdr=(timf1p_sdr+8)&timf1_bytemask;
  }
if( ((timf1p_sdr-timf1p_pa+timf1_bytes)&timf1_bytemask) >= 
                                                 snd[RXAD].block_bytes)
  {
  lir_set_event(EVENT_HWARE1_RXREADY);
  }
return 0;
}

void set_airspyhf_att(void)
{
unsigned int gain_no=-fg.gain/6;
// Possible hf att values: 0..8 Range: 0..48 dB Attenuation with 6 dB steps extern 
if(airspyhf_set_hf_att != NULL)airspyhf_set_hf_att(dev_airspyhf, gain_no); 
}

void set_airspyhf_frequency(void)
{
unsigned int freq_hz;
float t1;
freq_hz=(int)(fg.passband_center*(100000000.-airspyhf.freq_adjust)/100);
fg_truncation_error=freq_hz-(int)(fg.passband_center*(100000000.-airspyhf.freq_adjust)/100);
airspyhf_set_freq(dev_airspyhf, freq_hz);
fg.passband_direction = 1;
fft1_direction = fg.passband_direction;
if(airspyhf_set_optimal_iq_correction_point) 
  {
  t1=mix1_selfreq[0];
  airspyhf_old_point=t1;
  airspyhf_set_optimal_iq_correction_point(dev_airspyhf,
                                             (t1-5000.f)/ui.rx_ad_speed-0.5f);
  }
}

void airspyhf_input(void)
{
int airspyhf1, local_nco_counter, local_att_counter, i, errcod;
double dt1, read_start_time, total_reads;
int no_of_airspyhf;
int res;
struct airspyhf_device* devices[MAX_AIRSPYHF_DEVICES];
airspyhf_read_partid_serialno_t id;
char s[80];
uint32_t no_of_rates;
uint32_t *rates;
float t1;
int rxin_local_workload_reset;
int timing_loop_counter,timing_loop_counter_max,initial_skip_flag;
lir_init_event(EVENT_HWARE1_RXREADY);
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_AIRSPYHF_INPUT);
#endif
local_att_counter=sdr_att_counter;
local_nco_counter=sdr_nco_counter;
dt1=current_time();
i=read_sdrpar(airspyhf_parfil_name, MAX_AIRSPYHF_PARM, 
                                  airspyhf_parm_text, (int*)((void*)&airspyhf));
errcod=1400;
if(i != 0 || airspyhf.check != AIRSPYHF_PAR_VERNR)goto airspyhf_init_error_exit;
timf1p_sdr=timf1p_pa;  
load_usb1_library(TRUE);
errcod=1402;
if(libusb1_library_flag == FALSE)goto airspyhf_init_error_exit;
load_airspyhf_library();
errcod=1403;
if(!airspyhf_library_flag)goto airspyhf_init_error_exit1;
errcod=23723;
no_of_airspyhf=0;
airspyhf1=-1;
while(!kill_all_flag)
  {
  res=AIRSPYHF_SUCCESS;
  while( res == AIRSPYHF_SUCCESS && no_of_airspyhf < MAX_AIRSPYHF_DEVICES)
    {
    devices[no_of_airspyhf]=NULL;
    res=airspyhf_open(&devices[no_of_airspyhf]);
    if(res == AIRSPYHF_SUCCESS)
      {
      res=airspyhf_board_partid_serialno_read(devices[no_of_airspyhf], &id);
      if(res == AIRSPYHF_SUCCESS)
        {
        if( airspyhf.sernum1 == id.serial_no[2] &&
            airspyhf.sernum2 == id.serial_no[3])
          {
          airspyhf1=no_of_airspyhf;
          goto open_ok;
          }
        }
      } 
    }
  sprintf(s,"Selected Airspy not found %.2f", current_time()-dt1);
  lir_text(3,5,s);
  lir_refresh_screen();
  if(kill_all_flag)goto airspyhf_init_error_exit2;
  lir_sleep(300000);
  for(i=0; i< no_of_airspyhf; i++)
    {
    if(devices[i] != NULL)airspyhf_close(devices[i]);
    }
  no_of_airspyhf=0;
  airspyhf1=-1;
  }
open_ok:;
if(kill_all_flag)goto airspyhf_init_error_exit2;
if(no_of_airspyhf > 0)
  {
  for(i=0; i< no_of_airspyhf; i++)
    {
    if(i != airspyhf1)
      {
      if(devices[i] != NULL)airspyhf_close(devices[i]);
      }
    }
  }
dev_airspyhf=devices[airspyhf1];
airspyhf_get_samplerates(dev_airspyhf, &no_of_rates, 0);
if((uint32_t)airspyhf.ratenum >= no_of_rates)
  {
  errcod=1401;
  goto airspyhf_init_error_exit3;
  }    
rates = (uint32_t *) malloc(no_of_rates * sizeof(uint32_t));
airspyhf_get_samplerates(dev_airspyhf, rates, no_of_rates);
t1=(float)rates[airspyhf.ratenum];
free(rates);
t1=((rint)((t1*(100000000L+(double)airspyhf.freq_adjust))/100000000L));
if(ui.rx_ad_speed != (int)t1)
  {
  errcod=1401;
  goto airspyhf_init_error_exit3;
  } 
res=airspyhf_set_samplerate(dev_airspyhf, airspyhf.ratenum);
if(res != AIRSPYHF_SUCCESS)
  {  
  errcod=1404;
  goto airspyhf_init_error_exit3;
  }
if(airspyhf_set_lib_dsp != NULL)
  {
  res=airspyhf_set_lib_dsp(dev_airspyhf, airspyhf.dsp);
  if(res != AIRSPYHF_SUCCESS)
    {  
    errcod=1463;
    goto airspyhf_init_error_exit3;
    }
  }  
if(airspyhf_set_hf_agc != NULL)
  {
  res=airspyhf_set_hf_agc(dev_airspyhf, airspyhf.hf_agc);
  if(res != AIRSPYHF_SUCCESS)
    {  
    errcod=1464;
    goto airspyhf_init_error_exit3;
    }
  }  
if(airspyhf.hf_agc == 1)
  {
  if(airspyhf_set_hf_agc_threshold != NULL)
    {
    airspyhf_set_hf_agc_threshold(dev_airspyhf, airspyhf.hf_agc_threshold);
    }
  }
else
  {
  if(airspyhf_set_hf_lna != NULL)
    {
    airspyhf_set_hf_lna(dev_airspyhf, airspyhf.hf_lna);
    }
  }
if(airspyhf_iq_balancer_configure != NULL)
  {
  if(airspyhf.buffers_to_skip >= 0 &&
     airspyhf.fft_integration >= 0 &&
     airspyhf.fft_overlap >= 0 &&
     airspyhf.correlation_integration >= 0)
     airspyhf_iq_balancer_configure(dev_airspyhf,
                                    airspyhf.buffers_to_skip,
                                    airspyhf.fft_integration,
                                    airspyhf.fft_overlap,
                                    airspyhf.correlation_integration);
  }
fft1_block_timing();
if(thread_command_flag[THREAD_SCREEN]!=THRFLAG_NOT_ACTIVE)
  {
  while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
    {
    if(thread_command_flag[THREAD_AIRSPYHF_INPUT] ==
                                 THRFLAG_KILL)goto airspyhf_init_error_exit3;
    lir_sleep(10000);
    }
  }
#include "timing_setup.c"
airspyhf_cancel_flag=1;
res = airspyhf_start(dev_airspyhf, airspyhf_callback, NULL);
if(res != AIRSPYHF_SUCCESS)
  {  
  errcod=564998;
  goto airspyhf_init_error_exit3;
  }
set_airspyhf_frequency();
thread_status_flag[THREAD_AIRSPYHF_INPUT]=THRFLAG_ACTIVE;
lir_status=LIR_OK;
errcod=0;
airspyhf_cancel_flag=0;
while(!kill_all_flag && 
            thread_command_flag[THREAD_AIRSPYHF_INPUT] == THRFLAG_ACTIVE)
  {
  if(local_att_counter != sdr_att_counter)
    {
    local_att_counter=sdr_att_counter;
    set_airspyhf_att();
    }
  if(local_nco_counter != sdr_nco_counter)
    {
    local_nco_counter=sdr_nco_counter;
    set_airspyhf_frequency();
    }
  lir_await_event(EVENT_HWARE1_RXREADY);
  if(kill_all_flag)goto airspyhf_error_exit;
  while (!kill_all_flag && ((timf1p_pa-timf1p_sdr+timf1_bytes)&
                                    timf1_bytemask)>snd[RXAD].block_bytes)
    {
#include "input_speed.c"
    finish_rx_read();
    if(kill_all_flag)goto airspyhf_error_exit;
    }
  }
airspyhf_error_exit:;
airspyhf_stop(dev_airspyhf);
airspyhf_cancel_flag=-1;
airspyhf_init_error_exit3:;
airspyhf_close(dev_airspyhf);
airspyhf_init_error_exit2:;
unload_airspyhf_library();
airspyhf_init_error_exit1:;
unload_usb1_library();
airspyhf_init_error_exit:;
sdr=-1;
if(errcod != 0)lirerr(errcod);
thread_status_flag[THREAD_AIRSPYHF_INPUT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_AIRSPYHF_INPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
lir_close_event(EVENT_HWARE1_RXREADY);
} 


int display_airspyhf_parm_info(int *line)
{
char s[80];
int errcod;
float t1;
errcod=read_sdrpar(airspyhf_parfil_name, MAX_AIRSPYHF_PARM, 
                               airspyhf_parm_text, (int*)((void*)&airspyhf));
if(errcod == 0)
  {
  sprintf(s,"Serial no          = 0x%08X%08X",
                                         airspyhf.sernum1 ,airspyhf.sernum2);
  lir_text(24,line[0],s);
  SNDLOG"\n%s",s);
  line[0]++;
  t1=ui.rx_ad_speed;
  t1*=0.000001*(1-0.00000001*(double)airspyhf.freq_adjust);
  sprintf(s,"Sampling rate      = %.3f Mhz, Xtal adjust = %.0f ppb",t1,  
                                                   10.*airspyhf.freq_adjust);
  lir_text(24,line[0],s);
  SNDLOG"\n%s",s);
  line[0]++;
  sprintf(s,"HF agc %s, ",onoff[airspyhf.hf_agc]);
  if(airspyhf.hf_agc != 0)
    {
    sprintf(&s[strlen(s)],"HF AGC threshold %s",lowhigh[airspyhf.hf_agc_threshold]);
    lir_text(24,line[0],s);
    }
  else
    {
    sprintf(&s[strlen(s)],"HF LNA %s ",onoff[airspyhf.hf_lna]);
    lir_text(24,line[0],s);
    }
  SNDLOG"\n%s",s);
  line[0]++;
  }
if(airspyhf.dsp == 1)
  {
  sprintf(s,"IQ bal. parms");
  lir_text(24,line[0],s);
  if(airspyhf.buffers_to_skip >= 0 &&
     airspyhf.fft_integration >= 0 &&
     airspyhf.fft_overlap >= 0 &&
     airspyhf.correlation_integration >= 0)
    {
    sprintf(s,": bufs to skip %d",airspyhf.buffers_to_skip);
    lir_text(38,line[0],s);
    line[0]++;
    sprintf(s,": FFT integration %d",airspyhf.fft_integration);
    lir_text(38,line[0],s);
    line[0]++;
    sprintf(s,": FFT overlap %d",airspyhf.fft_overlap);
    lir_text(38,line[0],s);
    line[0]++;
    sprintf(s,": corr integration %d",airspyhf.correlation_integration);  
    lir_text(38,line[0],s);
    } 
  else
    {
    sprintf(s,"= default");
    lir_text(43,line[0],s);
    }
  line[0]++;
  }
return errcod;  
}



void init_airspyhf(void)
{
char s[80],ss[160];
int i, j, k, m, line, res, no_of_airspyhf; 
struct airspyhf_device* devices[MAX_AIRSPYHF_DEVICES];
airspyhf_lib_version_t lib;
char cind[4]={'-','/','|','\\'};
airspyhf_read_partid_serialno_t id[MAX_AIRSPYHF_DEVICES];
int devno;
float nominal_freq;
uint32_t no_of_rates;
uint32_t *rates;
FILE *airspyhf_file;
int *sdr_pi;
load_usb1_library(TRUE);
if(libusb1_library_flag == FALSE)return;
load_airspyhf_library();
if(airspyhf_library_flag == FALSE)return;
no_of_airspyhf=0;
line=5;
k=0;
for(i=0; i<MAX_AIRSPYHF_DEVICES; i++)devices[i]=NULL;
res=AIRSPYHF_ERROR;
while( (res != AIRSPYHF_SUCCESS && no_of_airspyhf == 0) || 
       (res == AIRSPYHF_SUCCESS && no_of_airspyhf < MAX_AIRSPYHF_DEVICES) )
  {
  res=airspyhf_open(&devices[no_of_airspyhf]);
  if(res == AIRSPYHF_SUCCESS)
    {
    no_of_airspyhf++;
    } 
  else
    {
    if(no_of_airspyhf == 0)
      {  
      sprintf(s,"Airspy HF+ not found %c",cind[k]);
      lir_text(10,line,s);
      lir_refresh_screen();
      lir_sleep(100000);
      test_keyboard();
      if(kill_all_flag || lir_inkey != 0)
        {
        unload_usb1_library();
        return;
        }
      k++;
      k&=3;
      }
    }
  }
clear_lines(line,line);  
for(k=0; k<no_of_airspyhf; k++)
  {
  res=AIRSPYHF_ERROR;
  if(devices[k])res=airspyhf_board_partid_serialno_read(devices[k], &id[k]);
  if(res != AIRSPYHF_SUCCESS) 
    {
    airspyhf_close(devices[k]);
    m=k+1;
    while(m < no_of_airspyhf)
      { 
      devices[m-1]=devices[m];
      m++;
      }
    no_of_airspyhf--;
    devices[no_of_airspyhf]=NULL;
    }
  }
// Windows XP may open the same device multiple times so
// a second device might not be opened.
// Maybe two devices could have the same serial number.
// do not accept that.
if( no_of_airspyhf > 1)
  {
  for(k=0; k<no_of_airspyhf-1; k++)
    {
    for(m=k+1; m<no_of_airspyhf; m++)
      {
      if(id[k].serial_no[2] == id[m].serial_no[2] && 
         id[k].serial_no[3] == id[m].serial_no[3])
        {
        airspyhf_close(devices[m]);
        i=m+1;
        while(i < no_of_airspyhf)
          { 
          devices[i-1]=devices[i];
          i++;
          }
        no_of_airspyhf--;
        devices[no_of_airspyhf]=NULL;
        } 
      }
    }  
  }   
for(k=0; k<no_of_airspyhf; k++)
  {
  sprintf(s,"%2d: serial no = 0x%08X%08X",k,id[k].serial_no[2],
                                            id[k].serial_no[3]);
  lir_text(3,line,s);
  line++;
  }   
line++;
if(no_of_airspyhf == 0)
  {
  lir_text(5,line,"No usable Airspy HF+ device found");
errexit:;
  await_processed_keyboard();
  unload_airspyhf_library();
  unload_usb1_library();
  return;
  }
else 
  { 
  if(no_of_airspyhf == 1)
    {
    lir_text(5,line,"Airspy device autoselected");
    devno=0;
    }
  else
    {
    lir_text(5,line,"Select device by line number.");
    devno=lir_get_integer(35,line,2,0,no_of_airspyhf-1);
    if(kill_all_flag)goto unload_lib;
    }
  }  
airspyhf.sernum1=id[devno].serial_no[2];
airspyhf.sernum2=id[devno].serial_no[3];
dev_airspyhf=devices[devno];
res=airspyhf_version_string_read(dev_airspyhf, s, MAX_VERSION_STRING_SIZE);
if(res == AIRSPYHF_SUCCESS)
  {
  sscanf(s,"R%d.%d.%d",&i,&j,&k);
  if(i < 3 || (i==3 && k < 2))
    {
    sprintf(ss,"Firmware version is %s. Upgrade to R3.0.2 or later",s);
    line++;
    settextcolor(12);
    lir_text(2,line,ss);
    settextcolor(7);
    }
  }
airspyhf_lib_version(&lib);

printf("\n%d  %d  %d",lib.major_version,lib.minor_version,lib.revision);
i=0;
if(lib.major_version == 0)i=1;
if(lib.major_version == 1 && lib.minor_version < 6 )i=1;
if(lib.major_version == 1 && lib.minor_version == 6 && lib.revision < 8)i=1;
  if(i==1)
  {
#if OSNUM == OSNUM_LINUX  
  sprintf(ss,"libairspyhf version is %d.%d.%d. Upgrade to 1.6.8 or later",
                             lib.major_version,lib.minor_version,lib.revision);
#endif
#if OSNUM == OSNUM_WINDOWS  
  sprintf(ss,"airspyhf.dll version is %d.%d.%d. Upgrade to 1.6.8 or later",
                             lib.major_version,lib.minor_version,lib.revision);
#endif
  line++;
  settextcolor(12);
  lir_text(2,line,ss);
  settextcolor(7);
  }  
line+=2;
airspyhf_get_samplerates(dev_airspyhf, &no_of_rates, 0);
rates = (uint32_t *) malloc(no_of_rates * sizeof(uint32_t));
airspyhf_get_samplerates(dev_airspyhf, rates, no_of_rates);
for(i=0; i<(int)no_of_rates; i++)
  {
  sprintf(s,"%2d  %d Hz",i,rates[i]);
  lir_text(2,line,s);
  line++;
  }
if(no_of_rates > 1)
  {
  line++;
  lir_text(2,line,"Select rate by line number.");
  airspyhf.ratenum=lir_get_integer(30,line,2,0,(int)no_of_rates);
  }
else
  {
  airspyhf.ratenum=0;
  }
nominal_freq=rates[airspyhf.ratenum];
res=airspyhf_set_samplerate(dev_airspyhf, airspyhf.ratenum);
if(res != AIRSPYHF_SUCCESS)
  {  
  sprintf(s,"Failed to set sample rate");
  lir_text(2,line,s);
  airspyhf_close(dev_airspyhf);
  goto errexit;
  }
line+=2;
lir_text(3,line,"Enter xtal error in ppb =>");
airspyhf.freq_adjust=0.1*lir_get_float(32,line,9,-300000.,300000.);
if(kill_all_flag)goto unload_lib;
nominal_freq=((rint)((nominal_freq*(100000000L+(double)airspyhf.freq_adjust))/100000000L));
ui.rx_ad_speed=(int)nominal_freq;
line+=2;
if(airspyhf_set_lib_dsp != NULL)
  {
  lir_text(3,line,"Use automatic mirror image suppression (Y/N) =>");
get_dsp:;
  await_processed_keyboard();
  if(kill_all_flag)goto errexit;
  if(lir_inkey == 'Y')
    {
    airspyhf.dsp=1;
    ui.rx_input_mode=IQ_DATA+FLOAT_INPUT+DWORD_INPUT+DIGITAL_IQ;
    }
  else
    {
    if(lir_inkey != 'N') goto get_dsp;  
    airspyhf.dsp=0;
    ui.rx_input_mode=IQ_DATA+FLOAT_INPUT+DWORD_INPUT;
    }
  line++;
  }
else
  {
  airspyhf.dsp=1;
  }
if(airspyhf_set_hf_agc != NULL)
  {
  lir_text(3,line,"Use HF AGC (Y/N) =>");
  line++;
get_agc:;
  await_processed_keyboard();
  if(kill_all_flag)goto errexit;
  if(lir_inkey == 'Y')
    {
    airspyhf.hf_agc=1;
    if(airspyhf_set_hf_agc_threshold != NULL)
      {
      lir_text(3,line,"Set AGC threshold high (Y/N) =>)");
get_agc_threshold:;
      await_processed_keyboard();
      if(kill_all_flag)goto errexit;
      if(lir_inkey == 'Y')
        {
        airspyhf.hf_agc_threshold=1;
        }
      else
        {
        if(lir_inkey != 'N') goto get_agc_threshold;  
        airspyhf.hf_agc_threshold=0;
        }
      }
    else
      {
      airspyhf.hf_agc_threshold=1;
      }            
    }
  else
    {
    if(lir_inkey != 'N') goto get_agc;  
    airspyhf.hf_agc=0;
    if(airspyhf_set_hf_lna != NULL)
      {
      lir_text(3,line,"Enable HF LNA (suggest Y) (Y/N)");
get_hf_lna:;
      await_processed_keyboard();
      if(kill_all_flag)goto errexit;
      if(lir_inkey == 'Y')
        {
        airspyhf.hf_lna=1;
        }
      else
        {
        if(lir_inkey != 'N') goto get_hf_lna;  
        airspyhf.hf_agc_threshold=0;
        }
      }
    else
      {
      airspyhf.hf_lna=1;
      }
    }
  line ++;
  }
else
  {
  airspyhf.hf_agc=1;
  airspyhf.hf_lna=1;
  }    
if(airspyhf_iq_balancer_configure != NULL)
  {
  lir_text(3,line,"Set non-default IQ balancing (Y/N)");
get_iq_default:;
  await_processed_keyboard();
  if(kill_all_flag)goto errexit;
  if(lir_inkey == 'Y')
    {
    line++;
    lir_text(3,line,"Buffers to skip (default=2)");
    airspyhf.buffers_to_skip=lir_get_integer(31,line,2,0,99);
    line++;
    lir_text(3,line,"FFT integration (default=4)");
    airspyhf.fft_integration=lir_get_integer(31,line,2,1,99);
    line++;
    lir_text(3,line,"FFT overlap (default=2)");
    airspyhf.fft_overlap=lir_get_integer(27,line,2,1,99);
    line++;
    lir_text(3,line,"Correlation integration (default=16)");
    airspyhf.correlation_integration=lir_get_integer(40,line,2,1,99);
    line++;
    }
  else
    {
    if(lir_inkey != 'N')
      {
      goto get_iq_default;
      }
    airspyhf.buffers_to_skip=-1;
    airspyhf.fft_integration=-1;
    airspyhf.fft_overlap=-1;
    airspyhf.correlation_integration=-1;
    }
  }
else
  {
  airspyhf.buffers_to_skip=-1;
  airspyhf.fft_integration=-1;
  airspyhf.fft_overlap=-1;
  airspyhf.correlation_integration=-1;
  }
airspyhf_file=fopen(airspyhf_parfil_name,"w");
if(airspyhf_file == NULL)
  {
  lirerr(385668);
  goto unload_lib;
  }
line+=2;
airspyhf.check=AIRSPYHF_PAR_VERNR;
sdr_pi=(int*)(&airspyhf);
for(i=0; i<MAX_AIRSPYHF_PARM; i++)
  {
  fprintf(airspyhf_file,"%s [%d]\n",airspyhf_parm_text[i],sdr_pi[i]);
  }
parfile_end(airspyhf_file);
ui.rx_addev_no=AIRSPYHF_DEVICE_CODE;
ui.rx_rf_channels=1;
ui.rx_ad_channels=2;
ui.rx_admode=0;
unload_lib:;
m=0;  
for(k=0; k<no_of_airspyhf; k++)
  {
  if(devices[k])
    {
    res=airspyhf_close(devices[k]);
    if(res != AIRSPYHF_SUCCESS)m++;
    }
  }
unload_airspyhf_library();     
unload_usb1_library();
if(m != 0)
  {  
  lirerr(1315);
  }
}

