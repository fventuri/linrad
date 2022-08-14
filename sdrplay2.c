// Copyright (c) 2019 @ Davide Gerhard IV3CVE
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

/*************
 *
 * Implementation require version 2.13 of
 * the binary library; doesn't work with the 3.*
 *
 * API documentation
 * https://www.sdrplay.com/docs/SDRplay_SDR_API_Specification.pdf
 *
 * AGC documentation
 * https://sdrplay.com/docs/SDRplay_AGC_technote_r2p0.pdf
 *
 * TODO
 *  - test RSPduo and others (tested only with RSP1a)
 *  - improve AGC/LNA
 *
 *************/


#include "osnum.h"
#include "globdef.h"
#include <signal.h>
#include <string.h>
#include "uidef.h"
#include "sdrdef.h"
#include "fft1def.h"
#include "screendef.h"
#include "thrdef.h"
#include "vernr.h"
#include "hwaredef.h"
#include "options.h"
#include <unistd.h>

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif
extern void make_freq_graph(int clear_old);


/*********** from mirsdrapi-rsp.h version 2.13 ******************/
#define MIR_SDR_API_VERSION   (float)(2.13)

typedef enum
  {
   mir_sdr_Success            = 0,
   mir_sdr_Fail               = 1,
   mir_sdr_InvalidParam       = 2,
   mir_sdr_OutOfRange         = 3,
   mir_sdr_GainUpdateError    = 4,
   mir_sdr_RfUpdateError      = 5,
   mir_sdr_FsUpdateError      = 6,
   mir_sdr_HwError            = 7,
   mir_sdr_AliasingError      = 8,
   mir_sdr_AlreadyInitialised = 9,
   mir_sdr_NotInitialised     = 10,
   mir_sdr_NotEnabled         = 11,
   mir_sdr_HwVerError         = 12,
   mir_sdr_OutOfMemError      = 13,
   mir_sdr_HwRemoved          = 14
  } mir_sdr_ErrT;

typedef enum
  {
   mir_sdr_BW_Undefined = 0,
   mir_sdr_BW_0_200     = 200,
   mir_sdr_BW_0_300     = 300,
   mir_sdr_BW_0_600     = 600,
   mir_sdr_BW_1_536     = 1536,
   mir_sdr_BW_5_000     = 5000,
   mir_sdr_BW_6_000     = 6000,
   mir_sdr_BW_7_000     = 7000,
   mir_sdr_BW_8_000     = 8000
  } mir_sdr_Bw_MHzT;

typedef enum
  {
   mir_sdr_IF_Undefined = -1,
   mir_sdr_IF_Zero      = 0,
   mir_sdr_IF_0_450     = 450,
   mir_sdr_IF_1_620     = 1620,
   mir_sdr_IF_2_048     = 2048
  } mir_sdr_If_kHzT;

typedef enum
  {
   mir_sdr_ISOCH = 0,
   mir_sdr_BULK  = 1
  } mir_sdr_TransferModeT;

typedef enum
  {
   mir_sdr_CHANGE_NONE    = 0x00,
   mir_sdr_CHANGE_GR      = 0x01,
   mir_sdr_CHANGE_FS_FREQ = 0x02,
   mir_sdr_CHANGE_RF_FREQ = 0x04,
   mir_sdr_CHANGE_BW_TYPE = 0x08,
   mir_sdr_CHANGE_IF_TYPE = 0x10,
   mir_sdr_CHANGE_LO_MODE = 0x20,
   mir_sdr_CHANGE_AM_PORT = 0x40
  } mir_sdr_ReasonForReinitT;

typedef enum
  {
   mir_sdr_AGC_DISABLE  = 0,
   mir_sdr_AGC_100HZ    = 1,
   mir_sdr_AGC_50HZ     = 2,
   mir_sdr_AGC_5HZ      = 3
  } mir_sdr_AgcControlT;

typedef enum
  {
   mir_sdr_USE_SET_GR                = 0,
   mir_sdr_USE_SET_GR_ALT_MODE       = 1,
   mir_sdr_USE_RSP_SET_GR            = 2
  } mir_sdr_SetGrModeT;

typedef enum
  {
   mir_sdr_LO_Undefined = 0,
   mir_sdr_LO_Auto      = 1,
   mir_sdr_LO_120MHz    = 2,
   mir_sdr_LO_144MHz    = 3,
   mir_sdr_LO_168MHz    = 4
  } mir_sdr_LoModeT;

typedef struct
{
  char *SerNo;
  char *DevNm;
  unsigned char hwVer;
  unsigned char devAvail;
} mir_sdr_DeviceT;

typedef enum
  {
   mir_sdr_RSPII_ANTENNA_A = 5,
   mir_sdr_RSPII_ANTENNA_B = 6
  } mir_sdr_RSPII_AntennaSelectT;

typedef enum
  {
   mir_sdr_rspDuo_Tuner_1 = 1,
   mir_sdr_rspDuo_Tuner_2 = 2,
  } mir_sdr_rspDuo_TunerSelT;

typedef enum
  {
   mir_sdr_GAIN_MESSAGE_START_ID = 0x80000000,
   mir_sdr_ADC_OVERLOAD_DETECTED = mir_sdr_GAIN_MESSAGE_START_ID + 1,
   mir_sdr_ADC_OVERLOAD_CORRECTED = mir_sdr_GAIN_MESSAGE_START_ID + 2
  } mir_sdr_GainMessageIdT;
/**************************************************/


// REMOVE to avoid error during development
#define UNUSED(x) (void)(x)

// if driver debug is enabled or not
#define SDRPLAY2_DEBUG 0

// how many devices we can discover
#define MAX_RSP_DEVICES 4

/*
 * sample frequency in KHz
 * values between 2MHz and 10MHz are permitted. Decimation can be used
 * to obtain lower sample rates.
 */
#define SDRPLAY2_MIN_RATE 2000
#define SDRPLAY2_MAX_RATE 10000

// maximum values for AGC set point
#define SDRPLAY2_AGC_SETPOINT_MIN -60
#define SDRPLAY2_AGC_SETPOINT_MAX 0

// maximum and minimum frequency available KHz
#define SDRPLAY2_MIN_FREQ 1
#define SDRPLAY2_MAX_FREQ 2000000

#define SDRPLAY2_RSP1_TYPE 1
#define SDRPLAY2_RSP1_NAME "RSP1"
#define SDRPLAY2_RSP2_TYPE 2
#define SDRPLAY2_RSP2_NAME "RSPII"
#define SDRPLAY2_RSPduo_TYPE 3
#define SDRPLAY2_RSPduo_NAME "RSPduo"
#define SDRPLAY2_RSP1a_TYPE 253 // >253
#define SDRPLAY2_RSP1a_NAME "RSP1a"

// internal structure for sdrplay2 parameters
typedef struct {
  int serial;          // converted from HEX to INT with strtol()
  int freq_adjust;     // ppm
  int sample_rate;     // KHz 2000 - 10000
  int sample_rate_out; // KHz 0.5, 1, 2048 or 2000-1000
  int if_freq;         // KHz mir_sdr_If_kHzT
  int if_bandwidth;    // KHz mir_sdr_Bw_MHzT
  int transfer_type;   // USB transfer mode: iso or bulk
  int hw_flavour;      // 1 = RSP1, 2 = RSP2/RSP2pro, 3 = RSPduo, >253 = RSP1a
  int lna_state;       // LNA status
  int bias_t;          // 0/1 disable/enable bias-t
  int notch_fm;        // 0/1 disable/enable FM notch >50dB 85 - 100MHz
  int notch_dab;       // 0/1 disable/enable DAB notch >30dB 165 - 230MHz
  int agc_mode;        // mir_sdr_AgcControlT
  int agc_setpoint;    // AGC Setpoint (dBfs)
  int decimation;      // 0/1 disable/enable decimation
  int dec_value;       // decimation value
  int dc_offset;       // 0/1 disable/enable DC offset control
  int dc_mode;         // DC offset correction mode
  int dc_track_time;   // time period over which the DC offset is tracked when one-shot
  int iq_correction;   // 0/1 disable/enable iq correction
  int antenna;         // mir_sdr_RSPII_AntennaSelectT for RSP2/RSP2Pro
  int tuner;           // mir_sdr_rspDuo_TunerSelT for RSPduo
  int external_ref;    // 0/1 disable/enable external reference RSP2/RSP2Pro/RSPduo
  int am_port;         // 0/1 disable/enable AM port for RSP2/RSPduo
  int check;           // ABI/API compatibility version
}P_SDRPLAY2;
#define SDRPLAY2_PARM_NUM 25

P_SDRPLAY2 sdrplay2 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

char *sdrplay2_parfil_name="par_sdrplay2";

char *sdrplay2_parm_text[SDRPLAY2_PARM_NUM]={
                                             "Serial",
                                             "Freq adjust",
                                             "Sampling rate",
                                             "Sampling rate output",
                                             "IF freq",
                                             "IF bandwidth",
                                             "USB transfer",
                                             "HW flavour",
                                             "LNA state",
                                             "Bias-T",
                                             "Notch FM",
                                             "Notch DAB",
                                             "AGC Mode",
                                             "AGC Setpoint",
                                             "Decimation",
                                             "Decimation value",
                                             "DC Offset",
                                             "DC mode",
                                             "DC track time",
                                             "IQ Correction",
                                             "Antenna",
                                             "Tuner",
                                             "External Reference",
                                             "AM Port",
                                             "Check"
};

char *usb_transfer_types[2]={"isosynchronous", "bulk"};

// structure that contains all sdrplay devices found
mir_sdr_DeviceT sdrplay2_dev[MAX_RSP_DEVICES];

// which device we are using (index)
uint8_t sdrplay2_dev_no = 0;

/*
 * stop reading data from I/Q callback
 * 1 = return without processing data
 * 0 = process data
 */
int8_t sdrplay2_cancel_flag = 1;

/* Needed by StremInit() */
// returns the number of samples that will be returned in the callback
int sdrplay2_samplesPerPacket;
// initial gain reduction in dB; returns IF gain reduction value
int sdrplay2_gRdB = 40;
//returns overall system gain reduction value
int sdrplay2_gRdBsystem = 0;

// manage if the library is loaded or not
uint8_t sdrplay2_library_flag = 0;

// used on wse_sdrxx.c for gain
int sdrplay2_max_gain = 0;

/***** load library ******/
// mir_sdr_StreamInit() callback function prototypes
typedef void (*mir_sdr_StreamCallback_t)(short *xi, short *xqq, unsigned int firstSampleNum,
                                         int grChanged, int rfChanged, int fsChanged,
                                         unsigned int numSamples, unsigned int reset,
                                         unsigned int hwRemoved, void *cbContext);
typedef void (*mir_sdr_GainChangeCallback_t)(unsigned int gRdB, unsigned int lnaGRdB, void *cbContext);

typedef mir_sdr_ErrT (*p_mir_sdr_AgcControl)(mir_sdr_AgcControlT enable, int setPoint_dBfs,
                                             int knee_dBfs, unsigned int decay_ms,
                                             unsigned int hang_ms, int syncAgcUpdate,
                                             int lagcLNAstate);
typedef mir_sdr_ErrT (*p_mir_sdr_Reinit)(int *gRdB, double fsMHz, double rfMHz, mir_sdr_Bw_MHzT bwType,
                                         mir_sdr_If_kHzT ifType, mir_sdr_LoModeT loMode, int LNAstate,
                                         int *gRdBsystem, mir_sdr_SetGrModeT setGrMode,
                                         int *samplesPerPacket, mir_sdr_ReasonForReinitT reasonForReinit);
typedef mir_sdr_ErrT (*p_mir_sdr_ApiVersion)(float *version);
typedef mir_sdr_ErrT (*p_mir_sdr_StreamInit)(int *gRdB, double fsMHz, double rfMHz, mir_sdr_Bw_MHzT bwType,
                                             mir_sdr_If_kHzT ifType, int LNAstate, int *gRdBsystem,
                                             mir_sdr_SetGrModeT setGrMode, int *samplesPerPacket,
                                             mir_sdr_StreamCallback_t StreamCbFn,
                                             mir_sdr_GainChangeCallback_t GainChangeCbFn, void *cbContext);
typedef mir_sdr_ErrT (*p_mir_sdr_StreamUninit)(void);
typedef mir_sdr_ErrT (*p_mir_sdr_DebugEnable)(unsigned int enable);
typedef mir_sdr_ErrT (*p_mir_sdr_GetDevices)(mir_sdr_DeviceT *devices, unsigned int *numDevs,
                                             unsigned int maxDevs);
typedef mir_sdr_ErrT (*p_mir_sdr_SetDeviceIdx)(unsigned int idx);
typedef mir_sdr_ErrT (*p_mir_sdr_ReleaseDeviceIdx)(void);
typedef mir_sdr_ErrT (*p_mir_sdr_GetHwVersion)(unsigned char *ver);
typedef mir_sdr_ErrT (*p_mir_sdr_SetTransferMode)(mir_sdr_TransferModeT mode);
typedef mir_sdr_ErrT (*p_mir_sdr_SetPpm)(double ppm);
typedef mir_sdr_ErrT (*p_mir_sdr_AmPortSelect)(int port);
typedef mir_sdr_ErrT (*p_mir_sdr_rsp1a_BiasT)(int enable);
typedef mir_sdr_ErrT (*p_mir_sdr_rsp1a_DabNotch)(int enable);
typedef mir_sdr_ErrT (*p_mir_sdr_rsp1a_BroadcastNotch)(int enable);
typedef mir_sdr_ErrT (*p_mir_sdr_rspDuo_TunerSel)(mir_sdr_rspDuo_TunerSelT sel);
typedef mir_sdr_ErrT (*p_mir_sdr_rspDuo_ExtRef)(int enable);
typedef mir_sdr_ErrT (*p_mir_sdr_rspDuo_BiasT)(int enable);
typedef mir_sdr_ErrT (*p_mir_sdr_rspDuo_Tuner1AmNotch)(int enable);
typedef mir_sdr_ErrT (*p_mir_sdr_rspDuo_BroadcastNotch)(int enable);
typedef mir_sdr_ErrT (*p_mir_sdr_rspDuo_DabNotch)(int enable);
typedef mir_sdr_ErrT (*p_mir_sdr_RSPII_AntennaControl)(mir_sdr_RSPII_AntennaSelectT ant_select);
typedef mir_sdr_ErrT (*p_mir_sdr_RSPII_ExternalReferenceControl)(unsigned int output_enable);
typedef mir_sdr_ErrT (*p_mir_sdr_RSPII_BiasTControl)(unsigned int enable);
typedef mir_sdr_ErrT (*p_mir_sdr_RSPII_RfNotchEnable)(unsigned int enable);
typedef mir_sdr_ErrT (*p_mir_sdr_DCoffsetIQimbalanceControl)(unsigned int DCenable, unsigned int IQenable);
typedef mir_sdr_ErrT (*p_mir_sdr_SetDcMode)(int dcCal, int speedUp);
typedef mir_sdr_ErrT (*p_mir_sdr_SetDcTrackTime)(int trackTime);
typedef mir_sdr_ErrT (*p_mir_sdr_DecimateControl)(unsigned int enable, unsigned int decimationFactor, unsigned int wideBandSignal);
typedef mir_sdr_ErrT (*p_mir_sdr_GainChangeCallbackMessageReceived)(void);

p_mir_sdr_AgcControl mir_sdr_AgcControl;
p_mir_sdr_Reinit mir_sdr_Reinit;
p_mir_sdr_ApiVersion mir_sdr_ApiVersion;
p_mir_sdr_StreamInit mir_sdr_StreamInit;
p_mir_sdr_DebugEnable mir_sdr_DebugEnable;
p_mir_sdr_GetDevices mir_sdr_GetDevices;
p_mir_sdr_SetDeviceIdx mir_sdr_SetDeviceIdx;
p_mir_sdr_ReleaseDeviceIdx mir_sdr_ReleaseDeviceIdx;
p_mir_sdr_GetHwVersion mir_sdr_GetHwVersion;
p_mir_sdr_SetTransferMode mir_sdr_SetTransferMode;
p_mir_sdr_SetPpm mir_sdr_SetPpm;
p_mir_sdr_StreamUninit mir_sdr_StreamUninit;
p_mir_sdr_rsp1a_BiasT mir_sdr_rsp1a_BiasT;
p_mir_sdr_AmPortSelect mir_sdr_AmPortSelect;
p_mir_sdr_RSPII_AntennaControl mir_sdr_RSPII_AntennaControl;
p_mir_sdr_rspDuo_TunerSel mir_sdr_rspDuo_TunerSel;
p_mir_sdr_RSPII_ExternalReferenceControl mir_sdr_RSPII_ExternalReferenceControl;
p_mir_sdr_rspDuo_ExtRef mir_sdr_rspDuo_ExtRef;
p_mir_sdr_rspDuo_BiasT mir_sdr_rspDuo_BiasT;
p_mir_sdr_RSPII_BiasTControl mir_sdr_RSPII_BiasTControl;
p_mir_sdr_rsp1a_BroadcastNotch mir_sdr_rsp1a_BroadcastNotch;
p_mir_sdr_RSPII_RfNotchEnable mir_sdr_RSPII_RfNotchEnable;
p_mir_sdr_rspDuo_Tuner1AmNotch mir_sdr_rspDuo_Tuner1AmNotch;
p_mir_sdr_rspDuo_BroadcastNotch mir_sdr_rspDuo_BroadcastNotch;
p_mir_sdr_rsp1a_DabNotch mir_sdr_rsp1a_DabNotch;
p_mir_sdr_rspDuo_DabNotch mir_sdr_rspDuo_DabNotch;
p_mir_sdr_DCoffsetIQimbalanceControl mir_sdr_DCoffsetIQimbalanceControl;
p_mir_sdr_SetDcMode mir_sdr_SetDcMode;
p_mir_sdr_SetDcTrackTime mir_sdr_SetDcTrackTime;
p_mir_sdr_DecimateControl mir_sdr_DecimateControl;
p_mir_sdr_GainChangeCallbackMessageReceived mir_sdr_GainChangeCallbackMessageReceived;

#if(OSNUM == OSNUM_WINDOWS)
#include <windows.h>
HANDLE sdrplay2_libhandle;

void
load_sdrplay2_library(void)
{
  char sdrplay2_dllname[80];
  int info;
  if(sdrplay2_library_flag)return;
  info=0;
  sprintf(sdrplay2_dllname,"%smir_sdr_api",DLLDIR);
  sdrplay2_libhandle=LoadLibraryEx(sdrplay2_dllname, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
  if(!sdrplay2_libhandle) goto sdrplay2_load_error;
  info=1;

  mir_sdr_AgcControl=(p_mir_sdr_AgcControl)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_AgcControl");
  if(!mir_sdr_AgcControl) goto sdrplay2_sym_error;
  mir_sdr_Reinit=(p_mir_sdr_Reinit)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_Reinit");
  if(!mir_sdr_Reinit) goto sdrplay2_sym_error;
  mir_sdr_ApiVersion=(p_mir_sdr_ApiVersion)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_ApiVersion");
  if(!mir_sdr_ApiVersion) goto sdrplay2_sym_error;
  mir_sdr_StreamInit=(p_mir_sdr_StreamInit)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_StreamInit");
  if(!mir_sdr_StreamInit) goto sdrplay2_sym_error;
  mir_sdr_DebugEnable=(p_mir_sdr_DebugEnable)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_DebugEnable");
  if(!mir_sdr_DebugEnable) goto sdrplay2_sym_error;
  mir_sdr_GetDevices=(p_mir_sdr_GetDevices)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_GetDevices");
  if(!mir_sdr_GetDevices) goto sdrplay2_sym_error;
  mir_sdr_SetDeviceIdx=(p_mir_sdr_SetDeviceIdx)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_SetDeviceIdx");
  if(!mir_sdr_SetDeviceIdx) goto sdrplay2_sym_error;
  mir_sdr_ReleaseDeviceIdx=(p_mir_sdr_ReleaseDeviceIdx)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_ReleaseDeviceIdx");
  if(!mir_sdr_ReleaseDeviceIdx) goto sdrplay2_sym_error;
  mir_sdr_GetHwVersion=(p_mir_sdr_GetHwVersion)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_GetHwVersion");
  if(!mir_sdr_GetHwVersion) goto sdrplay2_sym_error;
  mir_sdr_SetTransferMode=(p_mir_sdr_SetTransferMode)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_SetTransferMode");
  if(!mir_sdr_SetTransferMode) goto sdrplay2_sym_error;
  mir_sdr_SetPpm=(p_mir_sdr_SetPpm)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_SetPpm");
  if(!mir_sdr_SetPpm) goto sdrplay2_sym_error;
  mir_sdr_StreamUninit=(p_mir_sdr_StreamUninit)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_StreamUninit");
  if(!mir_sdr_StreamUninit) goto sdrplay2_sym_error;
  mir_sdr_rsp1a_BiasT=(p_mir_sdr_rsp1a_BiasT)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_rsp1a_BiasT");
  if(!mir_sdr_rsp1a_BiasT) goto sdrplay2_sym_error;
  mir_sdr_AmPortSelect=(p_mir_sdr_AmPortSelect)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_AmPortSelect");
  if(!mir_sdr_AmPortSelect) goto sdrplay2_sym_error;
  mir_sdr_RSPII_AntennaControl=(p_mir_sdr_RSPII_AntennaControl)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_RSPII_AntennaControl");
  if(!mir_sdr_RSPII_AntennaControl) goto sdrplay2_sym_error;
  mir_sdr_rspDuo_TunerSel=(p_mir_sdr_rspDuo_TunerSel)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_rspDuo_TunerSel");
  if(!mir_sdr_rspDuo_TunerSel) goto sdrplay2_sym_error;
  mir_sdr_RSPII_ExternalReferenceControl=(p_mir_sdr_RSPII_ExternalReferenceControl)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_RSPII_ExternalReferenceControl");
  if(!mir_sdr_RSPII_ExternalReferenceControl) goto sdrplay2_sym_error;
  mir_sdr_rspDuo_ExtRef=(p_mir_sdr_rspDuo_ExtRef)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_rspDuo_ExtRef");
  if(!mir_sdr_rspDuo_ExtRef) goto sdrplay2_sym_error;
  mir_sdr_rspDuo_BiasT=(p_mir_sdr_rspDuo_BiasT)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_rspDuo_BiasT");
  if(!mir_sdr_rspDuo_BiasT) goto sdrplay2_sym_error;
  mir_sdr_RSPII_BiasTControl=(p_mir_sdr_RSPII_BiasTControl)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_RSPII_BiasTControl");
  if(!mir_sdr_RSPII_BiasTControl) goto sdrplay2_sym_error;
  mir_sdr_rsp1a_BroadcastNotch=(p_mir_sdr_rsp1a_BroadcastNotch)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_rsp1a_BroadcastNotch");
  if(!mir_sdr_rsp1a_BroadcastNotch) goto sdrplay2_sym_error;
  mir_sdr_RSPII_RfNotchEnable=(p_mir_sdr_RSPII_RfNotchEnable)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_RSPII_RfNotchEnable");
  if(!mir_sdr_RSPII_RfNotchEnable) goto sdrplay2_sym_error;
  mir_sdr_rspDuo_Tuner1AmNotch=(p_mir_sdr_rspDuo_Tuner1AmNotch)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_rspDuo_Tuner1AmNotch");
  if(!mir_sdr_rspDuo_Tuner1AmNotch) goto sdrplay2_sym_error;
  mir_sdr_rspDuo_BroadcastNotch=(p_mir_sdr_rspDuo_BroadcastNotch)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_rspDuo_BroadcastNotch");
  if(!mir_sdr_rspDuo_BroadcastNotch) goto sdrplay2_sym_error;
  mir_sdr_rsp1a_DabNotch=(p_mir_sdr_rsp1a_DabNotch)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_rsp1a_DabNotch");
  if(!mir_sdr_rsp1a_DabNotch) goto sdrplay2_sym_error;
  mir_sdr_rspDuo_DabNotch=(p_mir_sdr_rspDuo_DabNotch)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_rspDuo_DabNotch");
  if(!mir_sdr_rspDuo_DabNotch) goto sdrplay2_sym_error;
  mir_sdr_DCoffsetIQimbalanceControl=(p_mir_sdr_DCoffsetIQimbalanceControl)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_DCoffsetIQimbalanceControl");
  if(!mir_sdr_DCoffsetIQimbalanceControl) goto sdrplay2_sym_error;
  mir_sdr_SetDcMode=(p_mir_sdr_SetDcMode)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_SetDcMode");
  if(!mir_sdr_SetDcMode) goto sdrplay2_sym_error;
  mir_sdr_SetDcTrackTime=(p_mir_sdr_SetDcTrackTime)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_SetDcTrackTime");
  if(!mir_sdr_SetDcTrackTime) goto sdrplay2_sym_error;
  mir_sdr_DecimateControl=(p_mir_sdr_DecimateControl)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_DecimateControl");
  if(!mir_sdr_DecimateControl) goto sdrplay2_sym_error;
  mir_sdr_GainChangeCallbackMessageReceived=(p_mir_sdr_GainChangeCallbackMessageReceived)(void*)GetProcAddress(sdrplay2_libhandle, "mir_sdr_GainChangeCallbackMessageReceived");
  if(!mir_sdr_GainChangeCallbackMessageReceived) goto sdrplay2_sym_error;

  sdrplay2_library_flag=TRUE;
  return;
 sdrplay2_sym_error:;
  FreeLibrary(sdrplay2_libhandle);
 sdrplay2_load_error:;
  library_error_screen(sdrplay2_dllname,info);
  return;
}

void
unload_sdrplay2_library(void)
{
  if(!sdrplay2_library_flag)return;
  FreeLibrary(sdrplay2_libhandle);
  sdrplay2_library_flag=FALSE;
}
#endif

#if(OSNUM == OSNUM_LINUX)
#include <dlfcn.h>
void *sdrplay2_libhandle;

void
load_sdrplay2_library(void)
{
  int info;
  if(sdrplay2_library_flag)return;
  info=0;
  sdrplay2_libhandle=dlopen(SDRPLAY2_LIBNAME, RTLD_NOW);
  if(!sdrplay2_libhandle)goto sdrplay2_load_error;
  info=1;

  mir_sdr_AgcControl=(p_mir_sdr_AgcControl)dlsym(sdrplay2_libhandle, "mir_sdr_AgcControl");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_Reinit=(p_mir_sdr_Reinit)dlsym(sdrplay2_libhandle, "mir_sdr_Reinit");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_ApiVersion=(p_mir_sdr_ApiVersion)dlsym(sdrplay2_libhandle, "mir_sdr_ApiVersion");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_StreamInit=(p_mir_sdr_StreamInit)dlsym(sdrplay2_libhandle, "mir_sdr_StreamInit");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_DebugEnable=(p_mir_sdr_DebugEnable)dlsym(sdrplay2_libhandle, "mir_sdr_DebugEnable");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_GetDevices=(p_mir_sdr_GetDevices)dlsym(sdrplay2_libhandle, "mir_sdr_GetDevices");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_SetDeviceIdx=(p_mir_sdr_SetDeviceIdx)dlsym(sdrplay2_libhandle, "mir_sdr_SetDeviceIdx");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_ReleaseDeviceIdx=(p_mir_sdr_ReleaseDeviceIdx)dlsym(sdrplay2_libhandle, "mir_sdr_ReleaseDeviceIdx");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_GetHwVersion=(p_mir_sdr_GetHwVersion)dlsym(sdrplay2_libhandle, "mir_sdr_GetHwVersion");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_SetTransferMode=(p_mir_sdr_SetTransferMode)dlsym(sdrplay2_libhandle, "mir_sdr_SetTransferMode");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_SetPpm=(p_mir_sdr_SetPpm)dlsym(sdrplay2_libhandle, "mir_sdr_SetPpm");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_StreamUninit=(p_mir_sdr_StreamUninit)dlsym(sdrplay2_libhandle, "mir_sdr_StreamUninit");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_rsp1a_BiasT=(p_mir_sdr_rsp1a_BiasT)dlsym(sdrplay2_libhandle, "mir_sdr_rsp1a_BiasT");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_AmPortSelect=(p_mir_sdr_AmPortSelect)dlsym(sdrplay2_libhandle, "mir_sdr_AmPortSelect");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_RSPII_AntennaControl=(p_mir_sdr_RSPII_AntennaControl)dlsym(sdrplay2_libhandle, "mir_sdr_RSPII_AntennaControl");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_rspDuo_TunerSel=(p_mir_sdr_rspDuo_TunerSel)dlsym(sdrplay2_libhandle, "mir_sdr_rspDuo_TunerSel");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_RSPII_ExternalReferenceControl=(p_mir_sdr_RSPII_ExternalReferenceControl)dlsym(sdrplay2_libhandle, "mir_sdr_RSPII_ExternalReferenceControl");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_rspDuo_ExtRef=(p_mir_sdr_rspDuo_ExtRef)dlsym(sdrplay2_libhandle, "mir_sdr_rspDuo_ExtRef");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_rspDuo_BiasT=(p_mir_sdr_rspDuo_BiasT)dlsym(sdrplay2_libhandle, "mir_sdr_rspDuo_BiasT");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_RSPII_BiasTControl=(p_mir_sdr_RSPII_BiasTControl)dlsym(sdrplay2_libhandle, "mir_sdr_RSPII_BiasTControl");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_rsp1a_BroadcastNotch=(p_mir_sdr_rsp1a_BroadcastNotch)dlsym(sdrplay2_libhandle, "mir_sdr_rsp1a_BroadcastNotch");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_RSPII_RfNotchEnable=(p_mir_sdr_RSPII_RfNotchEnable)dlsym(sdrplay2_libhandle, "mir_sdr_RSPII_RfNotchEnable");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_rspDuo_Tuner1AmNotch=(p_mir_sdr_rspDuo_Tuner1AmNotch)dlsym(sdrplay2_libhandle, "mir_sdr_rspDuo_Tuner1AmNotch");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_rspDuo_BroadcastNotch=(p_mir_sdr_rspDuo_BroadcastNotch)dlsym(sdrplay2_libhandle, "mir_sdr_rspDuo_BroadcastNotch");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_rsp1a_DabNotch=(p_mir_sdr_rsp1a_DabNotch)dlsym(sdrplay2_libhandle, "mir_sdr_rsp1a_DabNotch");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_rspDuo_DabNotch=(p_mir_sdr_rspDuo_DabNotch)dlsym(sdrplay2_libhandle, "mir_sdr_rspDuo_DabNotch");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_DCoffsetIQimbalanceControl=(p_mir_sdr_DCoffsetIQimbalanceControl)dlsym(sdrplay2_libhandle, "mir_sdr_DCoffsetIQimbalanceControl");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_SetDcMode=(p_mir_sdr_SetDcMode)dlsym(sdrplay2_libhandle, "mir_sdr_SetDcMode");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_SetDcTrackTime=(p_mir_sdr_SetDcTrackTime)dlsym(sdrplay2_libhandle, "mir_sdr_SetDcTrackTime");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_DecimateControl=(p_mir_sdr_DecimateControl)dlsym(sdrplay2_libhandle, "mir_sdr_DecimateControl");
  if(dlerror() != 0) goto sdrplay2_sym_error;
  mir_sdr_GainChangeCallbackMessageReceived=(p_mir_sdr_GainChangeCallbackMessageReceived)dlsym(sdrplay2_libhandle, "mir_sdr_GainChangeCallbackMessageReceived");
  if(dlerror() != 0) goto sdrplay2_sym_error;

  sdrplay2_library_flag=TRUE;
  return;

 sdrplay2_sym_error:
  dlclose(sdrplay2_libhandle);
 sdrplay2_load_error:
  library_error_screen(SDRPLAY2_LIBNAME,info);
}

void
unload_sdrplay2_library(void)
{
  if(sdrplay2_library_flag) {
    dlclose(sdrplay2_libhandle);
    sdrplay2_library_flag=FALSE;
  }
}
#endif
/************************/

void
sdrplay2_setBwEnumForRate(int rate)
{
  if (sdrplay2.if_freq == mir_sdr_IF_Zero)
    {
      if ((rate >= 200) && (rate < 300))
        sdrplay2.if_bandwidth = mir_sdr_BW_0_200;
      else if ((rate >= 300) && (rate < 600))
        sdrplay2.if_bandwidth = mir_sdr_BW_0_300;
      else if ((rate >= 600) && (rate < 1536))
        sdrplay2.if_bandwidth = mir_sdr_BW_0_600;
      else if ((rate >= 1536) && (rate < 5000))
        sdrplay2.if_bandwidth = mir_sdr_BW_1_536;
      else if ((rate >= 5000) && (rate < 6000))
        sdrplay2.if_bandwidth = mir_sdr_BW_5_000;
      else if ((rate >= 6000) && (rate < 7000))
        sdrplay2.if_bandwidth = mir_sdr_BW_6_000;
      else if ((rate >= 7000) && (rate < 8000))
        sdrplay2.if_bandwidth = mir_sdr_BW_7_000;
      else
        sdrplay2.if_bandwidth = mir_sdr_BW_8_000;
    }
  else if ((sdrplay2.if_freq == mir_sdr_IF_0_450) || (sdrplay2.if_freq == mir_sdr_IF_1_620))
    {
      if ((rate >= 200) && (rate < 500))
        sdrplay2.if_bandwidth = mir_sdr_BW_0_200;
      else if ((rate >= 500) && (rate < 1000))
        sdrplay2.if_bandwidth = mir_sdr_BW_0_300;
      else
        sdrplay2.if_bandwidth = mir_sdr_BW_0_600;
    }
  else
    {
      if ((rate >= 200) && (rate < 500))
        sdrplay2.if_bandwidth = mir_sdr_BW_0_200;
      else if ((rate >= 500) && (rate < 1000))
        sdrplay2.if_bandwidth = mir_sdr_BW_0_300;
      else if ((rate >= 1000) && (rate < 1536))
        sdrplay2.if_bandwidth = mir_sdr_BW_0_600;
      else
        sdrplay2.if_bandwidth = mir_sdr_BW_1_536;
    }
}

void
sdrplay2_setSampleRateDecimation(int rate)
{
  if (sdrplay2.if_freq == mir_sdr_IF_2_048)
    {
      if (rate == 2048)
        {
          sdrplay2.decimation = 1;
          sdrplay2.dec_value = 4;
          sdrplay2.sample_rate = 8192;
          sdrplay2.sample_rate_out = 2048;
        }
      else
        {
          sdrplay2.decimation = 0;
          sdrplay2.dec_value = 1;
          sdrplay2.sample_rate = rate;
          sdrplay2.sample_rate_out = rate/2;
        }
    }
  else if (sdrplay2.if_freq == mir_sdr_IF_0_450)
    {
      if
        (rate == 1000)
        {
          sdrplay2.decimation = 1;
          sdrplay2.dec_value = 2;
          sdrplay2.sample_rate = 2000;
          sdrplay2.sample_rate_out = 1000;
        }
      else if (rate == 500)
        {
          sdrplay2.decimation = 1;
          sdrplay2.dec_value = 4;
          sdrplay2.sample_rate = 2000;
          sdrplay2.sample_rate_out = 500;
        }
      else
        {
          sdrplay2.decimation = 0;
          sdrplay2.dec_value = 1;
          sdrplay2.sample_rate = rate;
          sdrplay2.sample_rate_out = rate/2;
        }
    }
  else if (sdrplay2.if_freq == mir_sdr_IF_Zero)
    {
      if ((rate >= 200)  && (rate < 500))
        {
          sdrplay2.decimation = 1;
          sdrplay2.dec_value = 8;
          sdrplay2.sample_rate = 2000;
          sdrplay2.sample_rate_out = 250;
        }
      else if ((rate >= 500)  && (rate < 1000))
        {
          sdrplay2.decimation = 1;
          sdrplay2.dec_value = 4;
          sdrplay2.sample_rate = 2000;
          sdrplay2.sample_rate_out = 500;
        }
      else if ((rate >= 1000) && (rate < 2000))
        {
          sdrplay2.decimation = 1;
          sdrplay2.dec_value = 2;
          sdrplay2.sample_rate = 2000;
          sdrplay2.sample_rate_out = 1000;
        }
      else
        {
          sdrplay2.decimation = 0;
          sdrplay2.dec_value = 1;
          sdrplay2.sample_rate = rate;
          sdrplay2.sample_rate_out = rate;
        }
    }
}

static void
sdrplay2_gain_callback(unsigned int _gRdB, unsigned int _lnaGRdB, void *_cbContext)
{
  UNUSED(_lnaGRdB);
  UNUSED(_cbContext);

  // alert when AGC overload
  if (_gRdB == mir_sdr_ADC_OVERLOAD_DETECTED || _gRdB == mir_sdr_ADC_OVERLOAD_CORRECTED)
    {
    mir_sdr_GainChangeCallbackMessageReceived();
// Set ad max amplitude for the A/D margin to become zero.
// Users will see it they run with 'A' pressed to show amplitude margins.
    rx_ad_maxamp[0]=0x7fff;
    rx_ad_maxamp[1]=0x7fff;
    }
}

static void
sdrplay2_callback(short *xi, short *xqq, unsigned int firstSampleNum, int grChanged, int rfChanged,
                  int fsChanged, unsigned int numSamples, unsigned int reset,
                  unsigned int hwRemoved, void *cbContext)
{
  UNUSED(firstSampleNum);
  UNUSED(grChanged);
  UNUSED(rfChanged);
  UNUSED(fsChanged);
  UNUSED(reset);
  UNUSED(hwRemoved);
  UNUSED(cbContext);

  short *iz;
  unsigned int i;

  if(hwRemoved)
    {
      kill_all_flag=1;
      return;
    }

  if(sdrplay2_cancel_flag)
    return;

  for(i=0; i < numSamples; i++)
    {
      iz=(short int*)&timf1_char[timf1p_sdr];
      iz[0]=xi[i];
      iz[1]=xqq[i];
      timf1p_sdr=(timf1p_sdr+4)&timf1_bytemask;
    }

  if( ((timf1p_sdr-timf1p_pa+timf1_bytes)&timf1_bytemask) >=
      snd[RXAD].block_bytes)
    {
      lir_set_event(EVENT_HWARE1_RXREADY);
    }
}

void
set_sdrplay2_att(void)
{
  sdrplay2.lna_state = fg.gain;
  if (sdrplay2.agc_mode != mir_sdr_AGC_DISABLE)
    mir_sdr_AgcControl(sdrplay2.agc_mode, sdrplay2.agc_setpoint, 0, 0, 0, 0, sdrplay2.lna_state);
  else
    mir_sdr_Reinit(&sdrplay2_gRdB, 0.0, 0.0, mir_sdr_BW_Undefined,
                   mir_sdr_IF_Undefined, mir_sdr_LO_Undefined,
                   sdrplay2.lna_state, &sdrplay2_gRdBsystem,
                   mir_sdr_USE_RSP_SET_GR, &sdrplay2_samplesPerPacket,
                   mir_sdr_CHANGE_GR);
}

// callable only when Stream is active
void
set_sdrplay2_frequency(void)
{
  // should be on wse_sdrxx.c but we use macro and nobody
  // use this checks on that file
  if (fg.passband_center*1000 < SDRPLAY2_MIN_FREQ)
    fg.passband_center = SDRPLAY2_MIN_FREQ/1000;
  else if (fg.passband_center*1000 > SDRPLAY2_MAX_FREQ)
    fg.passband_center = SDRPLAY2_MAX_FREQ/1000;
  else
    mir_sdr_Reinit(&sdrplay2_gRdB, 0.0, fg.passband_center, mir_sdr_BW_Undefined,
                   mir_sdr_IF_Undefined, mir_sdr_LO_Undefined, sdrplay2.lna_state,
                   &sdrplay2_gRdBsystem, mir_sdr_USE_RSP_SET_GR,
                   &sdrplay2_samplesPerPacket, mir_sdr_CHANGE_RF_FREQ);
}

void
sdrplay2_input(void)
{
  unsigned int i;
  int errcod;
  char s[256];
  int local_att_counter;
  int local_nco_counter;
  int rxin_local_workload_reset;
  int timing_loop_counter,timing_loop_counter_max,initial_skip_flag;
  double read_start_time, total_reads;
  sdrplay2_cancel_flag=1;

  lir_init_event(EVENT_HWARE1_RXREADY);
#if OSNUM == OSNUM_LINUX
  clear_thread_times(THREAD_SDRPLAY2_INPUT);
#endif
  // set immediately the basic value
  local_att_counter=sdr_att_counter;
  local_nco_counter=sdr_nco_counter;
  timf1p_sdr=timf1p_pa;

  errcod=read_sdrpar(sdrplay2_parfil_name, SDRPLAY2_PARM_NUM,
                     sdrplay2_parm_text, (int*)((void*)&sdrplay2));

  if (sdrplay2.check != SDRPLAY2PAR_VERNR)
    {
      lirerr(1465);
      goto sdrplay2_init_error_exit3;
    }

  // load the external library
  load_sdrplay2_library();
  if(!sdrplay2_library_flag)
    {
      lirerr(1414);
      goto sdrplay2_init_error_exit3;
    }

  float ver;
  mir_sdr_ApiVersion(&ver);
  if (ver < MIR_SDR_API_VERSION)
    {
      lirerr(1471);
      goto sdrplay2_init_error_exit3;
    }

  // enable debug output
  mir_sdr_DebugEnable(SDRPLAY2_DEBUG);

  unsigned int devices_num;
  if(mir_sdr_GetDevices(&sdrplay2_dev[0], &devices_num, MAX_RSP_DEVICES) != mir_sdr_Success)
    {
      lirerr(1466);
      goto sdrplay2_init_error_exit3;
    }
  for(i = 0; i < devices_num; i++)
    {
      if((int)strtol(sdrplay2_dev[i].SerNo, NULL, 16) == sdrplay2.serial)
        {
          // we found the device
          sdrplay2_dev_no = i;
          if(mir_sdr_SetDeviceIdx(sdrplay2_dev_no) != mir_sdr_Success)
            {
#if(OSNUM == OSNUM_WINDOWS)
              lirerr(1468);
#else
              lirerr(1469);
#endif
              goto sdrplay2_init_error_exit3;
            }
          break;
        }

    }

  // device not found
  if(i == devices_num)
    {
      sprintf(s,"SDRPlay2 USB not found");
      lir_text(3,5,s);
      lir_refresh_screen();
      if(kill_all_flag)
        goto sdrplay2_init_error_exit3;
      lir_sleep(100000);
      goto sdrplay2_init_error_exit3;
    }

  // configure the max gain available for the device
  if (sdrplay2.hw_flavour == SDRPLAY2_RSP1_TYPE)
    sdrplay2_max_gain = 3;
  else if (sdrplay2.hw_flavour == SDRPLAY2_RSP2_TYPE)
    sdrplay2_max_gain = 8;
  else
    sdrplay2_max_gain = 9;

  // set transfer mode
  if(mir_sdr_SetTransferMode(sdrplay2.transfer_type))
        goto sdrplay2_init_error_exit2;

  // set ppm
  if(mir_sdr_SetPpm(sdrplay2.freq_adjust))
    goto sdrplay2_init_error_exit2;

  // set AGC
  if(mir_sdr_AgcControl(sdrplay2.agc_mode, sdrplay2.agc_setpoint, 0, 0, 0, 0, sdrplay2.lna_state))
    goto sdrplay2_init_error_exit2;

  // the default is 15 we want to move the value to our default
  // TODO check if there is a cleaner way
  if(fg.gain == 15)
    {
      fg.gain = sdrplay2.lna_state;
      make_freq_graph(TRUE);
    }

  // set external reference
  if(sdrplay2.hw_flavour == SDRPLAY2_RSP2_TYPE &&
     (mir_sdr_RSPII_ExternalReferenceControl(sdrplay2.external_ref) != mir_sdr_Success))
        goto sdrplay2_init_error_exit2;
  if(sdrplay2.hw_flavour == SDRPLAY2_RSPduo_TYPE &&
     (mir_sdr_rspDuo_ExtRef(sdrplay2.external_ref) != mir_sdr_Success))
    goto sdrplay2_init_error_exit2;

  // enable AM port
  if((sdrplay2.hw_flavour == SDRPLAY2_RSP2_TYPE || sdrplay2.hw_flavour == SDRPLAY2_RSPduo_TYPE) &&
     (mir_sdr_AmPortSelect(sdrplay2.am_port) != mir_sdr_Success))
    goto sdrplay2_init_error_exit2;

  // bias-t
  if(sdrplay2.hw_flavour > SDRPLAY2_RSP1a_TYPE &&
     (mir_sdr_rsp1a_BiasT(sdrplay2.bias_t) != mir_sdr_Success))
       goto sdrplay2_init_error_exit2;
     else if(sdrplay2.hw_flavour == SDRPLAY2_RSP2_TYPE &&
             (mir_sdr_RSPII_BiasTControl(sdrplay2.bias_t) != mir_sdr_Success))
       goto sdrplay2_init_error_exit2;
     else if(sdrplay2.hw_flavour == SDRPLAY2_RSPduo_TYPE &&
             (mir_sdr_rspDuo_BiasT(sdrplay2.bias_t) != mir_sdr_Success))
       goto sdrplay2_init_error_exit2;

     // Notch
     if(sdrplay2.hw_flavour > SDRPLAY2_RSP1a_TYPE)
       {
         if ((mir_sdr_rsp1a_BroadcastNotch(sdrplay2.notch_fm) != mir_sdr_Success) ||
             (mir_sdr_rsp1a_DabNotch(sdrplay2.notch_dab) != mir_sdr_Success))
           goto sdrplay2_init_error_exit2;
       }
  else if(sdrplay2.hw_flavour == SDRPLAY2_RSP2_TYPE)
    {
      if (mir_sdr_RSPII_RfNotchEnable(sdrplay2.notch_fm) != mir_sdr_Success)
        goto sdrplay2_init_error_exit2;

    }
  else if(sdrplay2.hw_flavour == SDRPLAY2_RSPduo_TYPE)
    {
      if (sdrplay2.tuner == mir_sdr_rspDuo_Tuner_1 && sdrplay2.am_port == 1 &&
          (mir_sdr_rspDuo_Tuner1AmNotch(sdrplay2.notch_fm) != mir_sdr_Success))
        goto sdrplay2_init_error_exit2;
      else if (sdrplay2.am_port == 0 &&
               (mir_sdr_rspDuo_BroadcastNotch(sdrplay2.notch_fm) != mir_sdr_Success))
        goto sdrplay2_init_error_exit2;
      if (mir_sdr_rspDuo_DabNotch(sdrplay2.notch_dab) != mir_sdr_Success)
        goto sdrplay2_init_error_exit2;
    }

  // DC offset and IQ imbalance
  if(mir_sdr_DCoffsetIQimbalanceControl(sdrplay2.dc_offset, sdrplay2.iq_correction)
     != mir_sdr_Success)
    goto sdrplay2_init_error_exit2;

  fft1_block_timing();
  if(thread_command_flag[THREAD_SCREEN]!=THRFLAG_NOT_ACTIVE)
    {
      while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
            thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
            thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
        {
          if(thread_command_flag[THREAD_SDRPLAY2_INPUT] == THRFLAG_KILL)
            goto sdrplay2_init_error_exit2;
          lir_sleep(10000);
        }
    }

  // initialize and start the I/Q stream
  if((errcod = mir_sdr_StreamInit(
                                  &sdrplay2_gRdB,
                                  sdrplay2.sample_rate/1e3,
                                  (double)(fg.passband_center*(100000000.-sdrplay2.freq_adjust)/100)/1e6,
                                  sdrplay2.if_bandwidth,
                                  sdrplay2.if_freq,
                                  sdrplay2.lna_state,
                                  &sdrplay2_gRdBsystem,
                                  mir_sdr_USE_RSP_SET_GR,
                                  &sdrplay2_samplesPerPacket,
                                  sdrplay2_callback,
                                  sdrplay2_gain_callback,
                                  (void *)NULL
                                  )) != mir_sdr_Success)
    {
      lirerr(1467);
      goto sdrplay2_init_error_exit2;
    }

  if(sdrplay2.dc_offset)
    {
      if(mir_sdr_SetDcMode(sdrplay2.dc_mode, 0) != mir_sdr_Success)
        goto sdrplay2_init_error_exit;
      if(mir_sdr_SetDcTrackTime(sdrplay2.dc_track_time) != mir_sdr_Success)
        goto sdrplay2_init_error_exit;
    }

  if(sdrplay2.decimation && sdrplay2.if_freq == mir_sdr_IF_Zero)
    mir_sdr_DecimateControl(sdrplay2.decimation, sdrplay2.dec_value, 1);

#include "timing_setup.c"
  thread_status_flag[THREAD_SDRPLAY2_INPUT]=THRFLAG_ACTIVE;
  lir_status=LIR_OK;
  sdrplay2_cancel_flag=0;
  while(!kill_all_flag &&
        thread_command_flag[THREAD_SDRPLAY2_INPUT] == THRFLAG_ACTIVE)
    {
      if(local_att_counter != sdr_att_counter)
        {
          local_att_counter=sdr_att_counter;
          set_sdrplay2_att();
        }
      if(local_nco_counter != sdr_nco_counter)
        {
          local_nco_counter=sdr_nco_counter;
          set_sdrplay2_frequency();
        }
      lir_await_event(EVENT_HWARE1_RXREADY);
      if(kill_all_flag)
        goto sdrplay2_init_error_exit;
      while (!kill_all_flag && ((timf1p_pa-timf1p_sdr+timf1_bytes)&
                                timf1_bytemask)>snd[RXAD].block_bytes)
        {
#include "input_speed.c"
          finish_rx_read();
        }
    }

 sdrplay2_init_error_exit:
  if(mir_sdr_StreamUninit() != mir_sdr_Success)
    {
      lirerr(1467);
      goto sdrplay2_init_error_exit3;
    }
  sdrplay2_cancel_flag=1;
 sdrplay2_init_error_exit2:;
  if(mir_sdr_ReleaseDeviceIdx() != mir_sdr_Success)
    {
      lirerr(1467);
    }
 sdrplay2_init_error_exit3:;
  unload_sdrplay2_library();
  thread_status_flag[THREAD_SDRPLAY2_INPUT]=THRFLAG_RETURNED;
  while(thread_command_flag[THREAD_SDRPLAY2_INPUT] != THRFLAG_NOT_ACTIVE)
    {
      lir_sleep(1000);
    }
  lir_close_event(EVENT_HWARE1_RXREADY);
}


int
display_sdrplay2_parm_info(int *line)
{
  char s[80];
  int errcod;

  errcod=read_sdrpar(sdrplay2_parfil_name, SDRPLAY2_PARM_NUM,
                     sdrplay2_parm_text, (int*)((void*)&sdrplay2));
  if(errcod == 0)
    {
      settextcolor(7);
      char device_name[30] = "";
      if(sdrplay2.hw_flavour == SDRPLAY2_RSP1_TYPE)
        strncpy(device_name, SDRPLAY2_RSP1_NAME, sizeof(device_name));
      else if(sdrplay2.hw_flavour == SDRPLAY2_RSP2_TYPE)
        strncpy(device_name, SDRPLAY2_RSP2_NAME, sizeof(device_name));
      else if(sdrplay2.hw_flavour == SDRPLAY2_RSPduo_TYPE)
        strncpy(device_name, SDRPLAY2_RSPduo_NAME, sizeof(device_name));
      else if(sdrplay2.hw_flavour > SDRPLAY2_RSP1a_TYPE)
        strncpy(device_name, SDRPLAY2_RSP1a_NAME, sizeof(device_name));

      sprintf(s,"Device             = %s, Serial = %X",
              device_name, sdrplay2.serial);
      lir_text(24,line[0],s);
      SNDLOG"\n%s",s);
  line[0]++;
  sprintf(s,"IF frequency       = %d kHz, Xtal adjust = %.0f ppb",
          sdrplay2.if_freq, 10.*sdrplay2.freq_adjust);
  lir_text(24,line[0],s);
  SNDLOG"\n%s",s);
line[0]++;
sprintf(s,"Sampling rate      = %d KHz, IF bandwidth = %d kHz",
        sdrplay2.sample_rate,
        sdrplay2.if_bandwidth);
lir_text(24,line[0],s);
      SNDLOG"\n%s",s);

line[0]++;
sprintf(s,"Decimation         = %s, Value = %d",
        sdrplay2.decimation == 1 ? "ON" : "OFF" ,
        sdrplay2.dec_value);
lir_text(24,line[0],s);
SNDLOG"\n%s",s);

line[0]++;
sprintf(s,"AGC mode           = %s, AGC set point = %d dBfs",
        sdrplay2.agc_mode == mir_sdr_AGC_DISABLE ? "OFF" :
        sdrplay2.agc_mode == mir_sdr_AGC_100HZ ? "ON 100Hz" :
        sdrplay2.agc_mode == mir_sdr_AGC_50HZ ? "ON 50Hz" :
        "ON 5Hz",
        sdrplay2.agc_setpoint);
lir_text(24,line[0],s);
SNDLOG"\n%s",s);

if(sdrplay2.hw_flavour == SDRPLAY2_RSP2_TYPE || sdrplay2.hw_flavour == SDRPLAY2_RSPduo_TYPE)
  {
    line[0]++;
    sprintf(s,"Antenna             = %d, Tuner = %d",
            sdrplay2.antenna == mir_sdr_RSPII_ANTENNA_A ? 'A' : 'B',
            sdrplay2.tuner == mir_sdr_rspDuo_Tuner_1 ? 1 : 2);
    lir_text(24,line[0],s);
SNDLOG"\n%s",s);
line[0]++;
sprintf(s,"External Reference = %d, AM Port = %d",
        sdrplay2.external_ref, sdrplay2.am_port);
lir_text(24,line[0],s);
SNDLOG"\n%s",s);
}

line[0]++;
sprintf(s,"DC Offset          = %d, I/Q Correction = %d",
        sdrplay2.dc_offset, sdrplay2.iq_correction);
lir_text(24,line[0],s);
SNDLOG"\n%s",s);
line[0]++;
sprintf(s,"Notch WBFM/MW      = %d, Notch DAB = %d",
        sdrplay2.notch_fm,
        sdrplay2.notch_dab);
lir_text(24,line[0],s);
SNDLOG"\n%s",s);
line[0]++;
sprintf(s,"Bias-t             = %d",
        sdrplay2.bias_t);
lir_text(24,line[0],s);
SNDLOG"\n%s",s);
line[0]++;
sprintf(s,"USB transfer       = %s",
        usb_transfer_types[sdrplay2.transfer_type]);
lir_text(24,line[0],s);
SNDLOG"\n%s",s);

line[0]++;

}
return (errcod);
}

void
init_sdrplay2(void)
{
  FILE *sdrplay2_file;
  uint8_t line = 0, i;
  char s[1024];
  float ver;

  load_sdrplay2_library();
  if(!sdrplay2_library_flag)
    return;

  mir_sdr_ApiVersion(&ver);
  if (ver != MIR_SDR_API_VERSION)
    {
      lir_text(3,2,"sdrplay2: library doesn't much with include.");
      lir_text(5,7,press_any_key);
      await_keyboard();
      return;
    }

  // enable debug output
  mir_sdr_DebugEnable(SDRPLAY2_DEBUG);

  /* search sdrplay2 device */
  lir_text(3,2,"SEARCHING");
  unsigned int devices_num = 0;
  if(mir_sdr_GetDevices(&sdrplay2_dev[0], &devices_num, MAX_RSP_DEVICES) != mir_sdr_Success)
    {
      lir_text(3,2,"sdrplay2: error on searching devices");
      lir_text(5,7,press_any_key);
      await_keyboard();
      goto exit_init2;
    }

  clear_lines(2,2);
  if(devices_num == 0)
    {
      lir_text(5,5,"SDRPlay2 not found.");
      lir_text(5,7,press_any_key);
      await_keyboard();
      goto exit_init2;
    }
  line=2;

  for(i=0; i<devices_num; i++)
    {

      if(sdrplay2_dev[i].devAvail == 1)
        {
          sprintf(s," %2d: %s   MFG:%s, SN: %s", i,
                  sdrplay2_dev[i].DevNm, "SDRPlay", sdrplay2_dev[i].SerNo);
          lir_text(3,line,s);
          line++;
        }
    }
  line++;
  if (devices_num == 1)
    {
      lir_text(3,line,"Device autoselected.");
      sdrplay2_dev_no=0;
    }
  else
    {
      lir_text(3, line, "Select device by line number: ");
      sdrplay2_dev_no=lir_get_integer(33,line,2,0,devices_num-1);
    }
  if(kill_all_flag)
    goto exit_init;

  if(mir_sdr_SetDeviceIdx(sdrplay2_dev_no) != mir_sdr_Success)
    {
      lir_text(3,2,"sdrplay2: error on opening the device");
#if(OSNUM == OSNUM_WINDOWS)
      lirerr(1468);
#else
      lirerr(1469);
#endif
      goto exit_init;
    }

  sdrplay2.serial = (int)strtol(sdrplay2_dev[sdrplay2_dev_no].SerNo, NULL, 16);
  sdrplay2.hw_flavour = (int)sdrplay2_dev[sdrplay2_dev_no].hwVer;

  // mir_sdr_USE_RSP_SET_GR
  sdrplay2.lna_state = (sdrplay2.hw_flavour == 2 ||
                        sdrplay2.hw_flavour == 3 ||
                        sdrplay2.hw_flavour > 253)? 4: 1;

  // low IF can be used only with bandwidth <= 1536 KHz
  if(sdrplay2.if_bandwidth <= mir_sdr_BW_1_536)
    {
      line+=2;
      lir_text(3,line,"A=Zero IF");
      line++;
      lir_text(3,line,"B=450 kHz");
      line++;
      lir_text(3,line,"C=1620 kHz");
      line++;
      lir_text(3,line,"D=2048 kHz");
      line++;
      lir_text(3,line,"Select IF frequency (A to D) =>");
    get_if_freq:;
      to_upper_await_keyboard();
      if(kill_all_flag) goto exit_init;
      switch(lir_inkey)
        {
        case 'A':
          sdrplay2.if_freq=mir_sdr_IF_Zero;
          break;
        case 'B':
          sdrplay2.if_freq=mir_sdr_IF_0_450;
          break;
        case 'C':
          sdrplay2.if_freq=mir_sdr_IF_1_620;
          break;
        case 'D':
          sdrplay2.if_freq=mir_sdr_IF_2_048;
          break;
        default:
          goto get_if_freq;
        }
      line++;
      sprintf(s,"Selected IF frequency is %d KHz",sdrplay2.if_freq);
      lir_text(3,line,s);
    }

  /*
   * ADC bits
   *
   * 14bit 2MSPS - 6.048MSPS
   * 12bit 6.048MSPS - 8.064MSPS
   * 10bit 8.064MSPS - 9.216MSPS
   * 8bit >9.216MSPS
   *
   */
  line+=2;
  sprintf(s,"Set sampling rate in KHz (%d to %d) or less than %d with auto-decimation =>",
          SDRPLAY2_MIN_RATE, SDRPLAY2_MAX_RATE, SDRPLAY2_MIN_RATE);
  lir_text(3,line,s);
  int t_rate = lir_get_integer(strlen(s)+2,line,5,SDRPLAY2_MIN_RATE, SDRPLAY2_MAX_RATE);
  sdrplay2_setSampleRateDecimation(t_rate);

  if(kill_all_flag)
    goto exit_init;

  line++;
  sprintf(s,"Actual sampling rate is %d KHz", sdrplay2.sample_rate);
  lir_text(3,line,s);
  if(sdrplay2.decimation)
    {
      sprintf(s,"Decimation is ON with value %d", sdrplay2.dec_value);
      lir_text(3,line,s);
    }

  // configure Bandwidth based on IF frequency and sampling rate
  sdrplay2_setBwEnumForRate(t_rate);

  line+=2;
  lir_text(3,line,"Enter xtal error in ppb =>");
  sdrplay2.freq_adjust=0.1*lir_get_float(32,line,9,-300000.,300000.);
  if(mir_sdr_SetPpm(sdrplay2.freq_adjust != mir_sdr_Success))
    goto exit_init;
  if(kill_all_flag)
    goto exit_init;
  line+=2;
#if OSNUM == OSNUM_LINUX
  lir_text(3,line,"Set transfer type isosynchronous or bulk (I/B)=>");
 get_usb_type:
  to_upper_await_keyboard();
  if(kill_all_flag) goto exit_init;
  switch(lir_inkey)
    {
    case 'I':
      sdrplay2.transfer_type=mir_sdr_ISOCH;
      break;
    case 'B':
      sdrplay2.transfer_type=mir_sdr_BULK;
      break;
    default:
      goto get_usb_type;
    }
#else
  sdrplay2.transfer_type=mir_sdr_ISOCH;
#endif
  if(mir_sdr_SetTransferMode(sdrplay2.transfer_type) != mir_sdr_Success)
    goto exit_init;
  line++;
  sprintf(s,"Selected USB transfer type %s",
          sdrplay2.transfer_type == mir_sdr_ISOCH ? usb_transfer_types[0] : usb_transfer_types[1]);
  lir_text(3,line,s);

  clear_screen();
  line=2;

  if(sdrplay2.hw_flavour == SDRPLAY2_RSP2_TYPE || sdrplay2.hw_flavour == SDRPLAY2_RSPduo_TYPE)
    {
      lir_text(3,line, "Which antenna do you want to use (A/B) =>");
    get_antenna:;
      await_processed_keyboard();
      if(kill_all_flag) goto exit_init;
      switch(lir_inkey)
        {
        case 'A':
          sdrplay2.antenna=mir_sdr_RSPII_ANTENNA_A;
          sdrplay2.tuner=mir_sdr_rspDuo_Tuner_1;
          break;
        case 'B':
          sdrplay2.antenna=mir_sdr_RSPII_ANTENNA_B;
          sdrplay2.tuner=mir_sdr_rspDuo_Tuner_2;
          break;
        default:
          goto get_antenna;
        }
      if(mir_sdr_RSPII_AntennaControl(sdrplay2.antenna) != mir_sdr_Success)
        goto exit_init;
      if(mir_sdr_rspDuo_TunerSel(sdrplay2.tuner) != mir_sdr_Success)
        goto exit_init;
      line++;
      sprintf(s,"Selected Antenna is %d",sdrplay2.antenna);
      lir_text(3,line,s);

      line+=2;
      lir_text(3,line, "Enable External Reference clock (0/1) =>");
    get_external_ref:;
      await_processed_keyboard();
      if(kill_all_flag) goto exit_init;
      switch(lir_inkey)
        {
        case '0':
          sdrplay2.external_ref=0;
          break;
        case '1':
          sdrplay2.external_ref=1;
          break;
        default:
          goto get_external_ref;
        }
      if(sdrplay2.hw_flavour == SDRPLAY2_RSP2_TYPE &&
         (mir_sdr_RSPII_ExternalReferenceControl(sdrplay2.external_ref) != mir_sdr_Success))
        goto exit_init;
      if(sdrplay2.hw_flavour == SDRPLAY2_RSPduo_TYPE &&
         (mir_sdr_rspDuo_ExtRef(sdrplay2.external_ref) != mir_sdr_Success))
        goto exit_init;
      line++;
      sprintf(s,"Selected external reference clock is %s",sdrplay2.external_ref == 1 ? "ON" : "OFF");
      lir_text(3,line,s);

      line+=2;
      lir_text(3,line,"Which AM port (0 = Antenna A or B / 1 = Hi-Z input) when requested frequency is within the AM band =>");
    get_am_port:;
      await_processed_keyboard();
      if(kill_all_flag) goto exit_init;
      switch(lir_inkey)
        {
        case '0':
          sdrplay2.am_port=0;
          break;
        case '1':
          sdrplay2.am_port=1;
          break;
        default:
          goto get_am_port;
        }
      if(mir_sdr_AmPortSelect(sdrplay2.am_port) != mir_sdr_Success)
        goto exit_init;
      line++;
      sprintf(s,"Selected AM port is %d",sdrplay2.am_port);
      lir_text(3,line,s);
    }

  // see https://sdrplay.com/docs/SDRplay_AGC_technote_r2p0.pdf
  line+=2;
  lir_text(3,line,"A=AGC disabled");
  line++;
  lir_text(3,line,"B=AGC on with 100Hz loop");
  line++;
  lir_text(3,line,"C=AGC on with 50 Hz loop");
  line++;
  lir_text(3,line,"D=AGC on with 5Hz loop");
  line++;
  lir_text(3,line,"Select AGC mode (A to D) =>");
 get_agc_mode:;
  await_processed_keyboard();
  if(kill_all_flag) goto exit_init;
  switch(lir_inkey)
    {
    case 'A':
      sdrplay2.agc_mode=mir_sdr_AGC_DISABLE;
      break;
    case 'B':
      sdrplay2.agc_mode=mir_sdr_AGC_100HZ;
      break;
    case 'C':
      sdrplay2.agc_mode=mir_sdr_AGC_50HZ;
      break;
    case 'D':
      sdrplay2.agc_mode=mir_sdr_AGC_5HZ;
      break;
    default:
      goto get_agc_mode;
    }
  line++;
  sprintf(s,"Selected AGC mode is %s",
          sdrplay2.agc_mode == mir_sdr_AGC_DISABLE ? "OFF" :
          sdrplay2.agc_mode == mir_sdr_AGC_100HZ ? "ON with 100Hz" :
          sdrplay2.agc_mode == mir_sdr_AGC_50HZ ? "ON with 50Hz" :
          "ON with 5Hz");
  lir_text(3,line,s);
  if(sdrplay2.agc_mode != mir_sdr_AGC_DISABLE)
    {
      line+=2;
      sprintf(s,"Set AGC set point in dBfs (%d to %d) | set -30 if you don't know =>",
              SDRPLAY2_AGC_SETPOINT_MIN, SDRPLAY2_AGC_SETPOINT_MAX);
      lir_text(3,line,s);
      sdrplay2.agc_setpoint=lir_get_integer(strlen(s)+2,line,5,
                                            SDRPLAY2_AGC_SETPOINT_MIN, SDRPLAY2_AGC_SETPOINT_MAX);
      if(kill_all_flag)
        goto exit_init;
    }
  if(mir_sdr_AgcControl(sdrplay2.agc_mode, sdrplay2.agc_setpoint, 0, 0, 0, 0, sdrplay2.lna_state))
    goto exit_init;

  line+=2;
  lir_text(3,line,"Enable bias-t (0/1) =>");
 get_bias_t:;
  await_processed_keyboard();
  if(kill_all_flag) goto exit_init;
  switch(lir_inkey)
       {
       case '0':
         sdrplay2.bias_t=0;
         break;
       case '1':
         sdrplay2.bias_t=1;
         break;
       default:
         goto get_bias_t;
       }
  if(sdrplay2.hw_flavour > SDRPLAY2_RSP1a_TYPE &&
     (mir_sdr_rsp1a_BiasT(sdrplay2.bias_t) != mir_sdr_Success))
    goto exit_init;
  else if(sdrplay2.hw_flavour == SDRPLAY2_RSP2_TYPE &&
          (mir_sdr_RSPII_BiasTControl(sdrplay2.bias_t) != mir_sdr_Success))
    goto exit_init;
  else if(sdrplay2.hw_flavour == SDRPLAY2_RSPduo_TYPE &&
          (mir_sdr_rspDuo_BiasT(sdrplay2.bias_t) != mir_sdr_Success))
    goto exit_init;
  line++;
  sprintf(s,"Selected bias-t is %s",sdrplay2.bias_t == 1 ? "ON" : "OFF");
  lir_text(3,line,s);


  line+=2;
  sprintf(s,"Enable WBFM/MW notch filter (0/1) =>");
  lir_text(3,line,s);
 get_notch_fm:;
  await_processed_keyboard();
  if(kill_all_flag) goto exit_init;
  switch(lir_inkey)
    {
    case '0':
      sdrplay2.notch_fm=0;
      break;
    case '1':
      sdrplay2.notch_fm=1;
      break;
    default:
      goto get_notch_fm;
    }
  if(sdrplay2.hw_flavour > SDRPLAY2_RSP1a_TYPE &&
     (mir_sdr_rsp1a_BroadcastNotch(sdrplay2.notch_fm) != mir_sdr_Success))
    goto exit_init;
  else if(sdrplay2.hw_flavour == SDRPLAY2_RSP2_TYPE &&
          (mir_sdr_RSPII_RfNotchEnable(sdrplay2.notch_fm) != mir_sdr_Success))
    goto exit_init;
  else if(sdrplay2.hw_flavour == SDRPLAY2_RSPduo_TYPE)
    {
      if (sdrplay2.tuner == mir_sdr_rspDuo_Tuner_1 && sdrplay2.am_port == 1 &&
          (mir_sdr_rspDuo_Tuner1AmNotch(sdrplay2.notch_fm) != mir_sdr_Success))
        goto exit_init;
      else if (sdrplay2.am_port == 0 &&
               (mir_sdr_rspDuo_BroadcastNotch(sdrplay2.notch_fm) != mir_sdr_Success))
        goto exit_init;
    }
  line++;
  sprintf(s,"Selected notch FM is %s",sdrplay2.notch_fm == 1 ? "ON" : "OFF");
  lir_text(3,line,s);

  if(sdrplay2.hw_flavour == SDRPLAY2_RSPduo_TYPE || sdrplay2.hw_flavour > SDRPLAY2_RSP1a_TYPE)
    {
      line+=2;
      sprintf(s,"Enable DAB notch filter (0/1) =>");
      lir_text(3,line,s);
    get_notch_dab:;
      await_processed_keyboard();
      if(kill_all_flag) goto exit_init;
      switch(lir_inkey)
        {
        case '0':
          sdrplay2.notch_dab=0;
          break;
        case '1':
          sdrplay2.notch_dab=1;
          break;
        default:
          goto get_notch_dab;
        }
      if(sdrplay2.hw_flavour > SDRPLAY2_RSP1a_TYPE)
        {
          if (mir_sdr_rsp1a_DabNotch(sdrplay2.notch_dab) != mir_sdr_Success)
            goto exit_init;
        }
      else
        if (mir_sdr_rspDuo_DabNotch(sdrplay2.notch_dab) != mir_sdr_Success)
          goto exit_init;
      if(kill_all_flag)
        goto exit_init;

      line++;
      sprintf(s,"Selected notch DAB is %s",sdrplay2.notch_dab == 1 ? "ON" : "OFF");
      lir_text(3,line,s);
    }

  line+=2;
  sprintf(s,"Enable DC offset correction (0/1) =>");
  lir_text(3,line,s);
 get_dc_offset:;
  await_processed_keyboard();
  if(kill_all_flag) goto exit_init;
  switch(lir_inkey)
    {
    case '0':
      sdrplay2.dc_offset=0;
      break;
    case '1':
      sdrplay2.dc_offset=1;
      break;
    default:
      goto get_dc_offset;
    }
  line++;
  sprintf(s,"Selected DC offset correction is %s",sdrplay2.dc_offset ==1 ? "ON" : "OFF");
  lir_text(3,line,s);
  // for the moment static
  sdrplay2.dc_mode = 4;
  sdrplay2.dc_track_time = 63;

  line+=2;
  sprintf(s,"Enable I/Q correction (0/1) =>");
  lir_text(3,line,s);
 get_iq_correction:;
  await_processed_keyboard();
  if(kill_all_flag) goto exit_init;
  switch(lir_inkey)
    {
    case '0':
      sdrplay2.iq_correction=0;
      break;
    case '1':
      sdrplay2.iq_correction=1;
      break;
    default:
      goto get_iq_correction;
    }
  if(mir_sdr_DCoffsetIQimbalanceControl(sdrplay2.dc_offset, sdrplay2.iq_correction)
     != mir_sdr_Success)
    goto exit_init;
  line++;
  sprintf(s,"Selected I/Q correction is %s", sdrplay2.iq_correction == 1 ? "ON" : "OFF");
  lir_text(3,line,s);

  sdrplay2.check=SDRPLAY2PAR_VERNR;

  // write parameters
  if((sdrplay2_file = fopen(sdrplay2_parfil_name, "w")) == NULL)
      {
        lirerr(1470);
        goto exit_init;
      }
  int *sdr_pi=(int*)(&sdrplay2);
  for(i=0; i< SDRPLAY2_PARM_NUM; i++)
    {
      fprintf(sdrplay2_file,"%s [%d]\n",sdrplay2_parm_text[i],sdr_pi[i]);
    }
  parfile_end(sdrplay2_file);

  ui.rx_addev_no = SDRPLAY2_DEVICE_CODE;
  float t1=((rint)((sdrplay2.sample_rate_out*1000*
                    (100000000L+(double)sdrplay2.freq_adjust))/100000000L));
  ui.rx_ad_speed=(int)t1;
  ui.rx_input_mode=IQ_DATA;
  if(sdrplay2.if_freq != 0)
    ui.rx_input_mode |= DIGITAL_IQ;
  ui.rx_rf_channels=1;
  ui.rx_ad_channels=2;
  ui.rx_admode=0;

 exit_init:
  mir_sdr_ReleaseDeviceIdx();
 exit_init2:
  unload_sdrplay2_library();
}
