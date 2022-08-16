// Copyright (c) 2022 @ Franco Venturi K4VZ
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
#include "fft1def.h"
#include "screendef.h"
#include "sdrdef.h"
#include "thrdef.h"
#include "vernr.h"
#include "options.h"
#include <string.h>

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif


/*********** from sdplay_api.h version 3.X *******************/
#if SDRPLAY3PAR_VERNR == 307
#define SDRPLAY_API_VERSION   (float)(3.07)
#elif SDRPLAY3PAR_VERNR == 309
#define SDRPLAY_API_VERSION   (float)(3.09)
#elif SDRPLAY3PAR_VERNR == 310
#define SDRPLAY_API_VERSION   (float)(3.10)
#endif

// API Constants
#define SDRPLAY3_MAX_DEVICES                  (16)
#define SDRPLAY3_MAX_TUNERS_PER_DEVICE        (2)

#define SDRPLAY3_MAX_SER_NO_LEN               (64)
#define SDRPLAY3_MAX_ROOT_NM_LEN              (32)

#define SDRPLAY3_RSP1_ID                      (1)
#define SDRPLAY3_RSP1A_ID                     (255)
#define SDRPLAY3_RSP2_ID                      (2)
#define SDRPLAY3_RSPduo_ID                    (3)
#define SDRPLAY3_RSPdx_ID                     (4)

// Enum types
typedef enum
{
  sdrplay_api_Success               = 0,
  sdrplay_api_Fail                  = 1,
  sdrplay_api_InvalidParam          = 2,
  sdrplay_api_OutOfRange            = 3,
  sdrplay_api_GainUpdateError       = 4,
  sdrplay_api_RfUpdateError         = 5,
  sdrplay_api_FsUpdateError         = 6,
  sdrplay_api_HwError               = 7,
  sdrplay_api_AliasingError         = 8,
  sdrplay_api_AlreadyInitialised    = 9,
  sdrplay_api_NotInitialised        = 10,
  sdrplay_api_NotEnabled            = 11,
  sdrplay_api_HwVerError            = 12,
  sdrplay_api_OutOfMemError         = 13,
  sdrplay_api_ServiceNotResponding  = 14,
  sdrplay_api_StartPending          = 15,
  sdrplay_api_StopPending           = 16,
  sdrplay_api_InvalidMode           = 17,
  sdrplay_api_FailedVerification1   = 18,
  sdrplay_api_FailedVerification2   = 19,
  sdrplay_api_FailedVerification3   = 20,
  sdrplay_api_FailedVerification4   = 21,
  sdrplay_api_FailedVerification5   = 22,
  sdrplay_api_FailedVerification6   = 23,
  sdrplay_api_InvalidServiceVersion = 24
} sdrplay_api_ErrT;

typedef enum
{
  sdrplay_api_DbgLvl_Disable = 0,
  sdrplay_api_DbgLvl_Verbose = 1,
  sdrplay_api_DbgLvl_Warning = 2,
  sdrplay_api_DbgLvl_Error   = 3,
  sdrplay_api_DbgLvl_Message = 4
} sdrplay_api_DbgLvl_t;

typedef enum
{
    sdrplay_api_RspDuoMode_Unknown      = 0,
    sdrplay_api_RspDuoMode_Single_Tuner = 1,
    sdrplay_api_RspDuoMode_Dual_Tuner   = 2,
    sdrplay_api_RspDuoMode_Master       = 4,
    sdrplay_api_RspDuoMode_Slave        = 8,
} sdrplay_api_RspDuoModeT;

// Tuner parameter enums
typedef enum
{
    sdrplay_api_BW_Undefined = 0,
    sdrplay_api_BW_0_200     = 200,
    sdrplay_api_BW_0_300     = 300,
    sdrplay_api_BW_0_600     = 600,
    sdrplay_api_BW_1_536     = 1536,
    sdrplay_api_BW_5_000     = 5000,
    sdrplay_api_BW_6_000     = 6000,
    sdrplay_api_BW_7_000     = 7000,
    sdrplay_api_BW_8_000     = 8000
} sdrplay_api_Bw_MHzT;

typedef enum
{
    sdrplay_api_IF_Undefined = -1,
    sdrplay_api_IF_Zero      = 0,
    sdrplay_api_IF_0_450     = 450,
    sdrplay_api_IF_1_620     = 1620,
    sdrplay_api_IF_2_048     = 2048
} sdrplay_api_If_kHzT;

typedef enum
{
    sdrplay_api_LO_Undefined = 0,
    sdrplay_api_LO_Auto      = 1,
    sdrplay_api_LO_120MHz    = 2,
    sdrplay_api_LO_144MHz    = 3,
    sdrplay_api_LO_168MHz    = 4
} sdrplay_api_LoModeT;

typedef enum
{
    sdrplay_api_EXTENDED_MIN_GR = 0,
    sdrplay_api_NORMAL_MIN_GR   = 20
} sdrplay_api_MinGainReductionT;

typedef enum
{
    sdrplay_api_Tuner_Neither  = 0,
    sdrplay_api_Tuner_A        = 1,
    sdrplay_api_Tuner_B        = 2,
    sdrplay_api_Tuner_Both     = 3,
} sdrplay_api_TunerSelectT;

// Control parameter enums
typedef enum
{
    sdrplay_api_AGC_DISABLE  = 0,
    sdrplay_api_AGC_100HZ    = 1,
    sdrplay_api_AGC_50HZ     = 2,
    sdrplay_api_AGC_5HZ      = 3,
    sdrplay_api_AGC_CTRL_EN  = 4
} sdrplay_api_AgcControlT;

typedef enum
{
    sdrplay_api_ADSB_DECIMATION                  = 0,
    sdrplay_api_ADSB_NO_DECIMATION_LOWPASS       = 1,
    sdrplay_api_ADSB_NO_DECIMATION_BANDPASS_2MHZ = 2,
    sdrplay_api_ADSB_NO_DECIMATION_BANDPASS_3MHZ = 3
} sdrplay_api_AdsbModeT;


#if(OSNUM == OSNUM_LINUX)
typedef void *HANDLE;
#endif

// Device structure
typedef struct
{
    char SerNo[SDRPLAY3_MAX_SER_NO_LEN];
    unsigned char hwVer;
    sdrplay_api_TunerSelectT tuner;
    sdrplay_api_RspDuoModeT rspDuoMode;
#if SDRPLAY3PAR_VERNR >= 309
    unsigned char valid;
#endif
    double rspDuoSampleFreq;
    HANDLE dev;
} sdrplay_api_DeviceT;

// Dev parameter enums
typedef enum
{
    sdrplay_api_ISOCH = 0,
    sdrplay_api_BULK  = 1
} sdrplay_api_TransferModeT;

// Dev parameter structs
typedef struct 
{
    double fsHz;
    unsigned char syncUpdate;
    unsigned char reCal;
} sdrplay_api_FsFreqT;

typedef struct 
{
    unsigned int sampleNum;
    unsigned int period;
} sdrplay_api_SyncUpdateT;

typedef struct 
{
    unsigned char resetGainUpdate;
    unsigned char resetRfUpdate;
    unsigned char resetFsUpdate;
} sdrplay_api_ResetFlagsT;   

// RSP1A parameter structs
typedef struct
{
    unsigned char rfNotchEnable;
    unsigned char rfDabNotchEnable;
} sdrplay_api_Rsp1aParamsT;

typedef struct
{
    unsigned char biasTEnable;
} sdrplay_api_Rsp1aTunerParamsT;

// RSP2 parameter enums
typedef enum
{
    sdrplay_api_Rsp2_ANTENNA_A = 5,
    sdrplay_api_Rsp2_ANTENNA_B = 6,
} sdrplay_api_Rsp2_AntennaSelectT;

typedef enum
{
    sdrplay_api_Rsp2_AMPORT_1 = 1,
    sdrplay_api_Rsp2_AMPORT_2 = 0,
} sdrplay_api_Rsp2_AmPortSelectT;

// RSP2 parameter structs
typedef struct
{
    unsigned char extRefOutputEn;
} sdrplay_api_Rsp2ParamsT;

typedef struct
{
    unsigned char biasTEnable;
    sdrplay_api_Rsp2_AmPortSelectT amPortSel;
    sdrplay_api_Rsp2_AntennaSelectT antennaSel;
    unsigned char rfNotchEnable;
} sdrplay_api_Rsp2TunerParamsT;

typedef enum
{
    sdrplay_api_RspDuo_AMPORT_1 = 1,
    sdrplay_api_RspDuo_AMPORT_2 = 0,
} sdrplay_api_RspDuo_AmPortSelectT;

// RSPduo parameter structs
typedef struct
{
    int extRefOutputEn;
} sdrplay_api_RspDuoParamsT;

typedef struct
{
    unsigned char biasTEnable;
    sdrplay_api_RspDuo_AmPortSelectT tuner1AmPortSel;
    unsigned char tuner1AmNotchEnable;
    unsigned char rfNotchEnable;
    unsigned char rfDabNotchEnable;
} sdrplay_api_RspDuoTunerParamsT;

// RSPdx parameter enums
typedef enum
{
    sdrplay_api_RspDx_ANTENNA_A = 0,
    sdrplay_api_RspDx_ANTENNA_B = 1,
    sdrplay_api_RspDx_ANTENNA_C = 2,
} sdrplay_api_RspDx_AntennaSelectT;

typedef enum
{
    sdrplay_api_RspDx_HDRMODE_BW_0_200  = 0,
    sdrplay_api_RspDx_HDRMODE_BW_0_500  = 1,
    sdrplay_api_RspDx_HDRMODE_BW_1_200  = 2,
    sdrplay_api_RspDx_HDRMODE_BW_1_700  = 3,
} sdrplay_api_RspDx_HdrModeBwT;

// RSPdx parameter structs
typedef struct
{
    unsigned char hdrEnable;
    unsigned char biasTEnable;
    sdrplay_api_RspDx_AntennaSelectT antennaSel;
    unsigned char rfNotchEnable;
    unsigned char rfDabNotchEnable;
} sdrplay_api_RspDxParamsT;

typedef struct
{
    sdrplay_api_RspDx_HdrModeBwT hdrBw;
} sdrplay_api_RspDxTunerParamsT;

typedef struct
{
    double ppm;
    sdrplay_api_FsFreqT fsFreq;
    sdrplay_api_SyncUpdateT syncUpdate;
    sdrplay_api_ResetFlagsT resetFlags;
    sdrplay_api_TransferModeT mode;
    unsigned int samplesPerPkt;
    sdrplay_api_Rsp1aParamsT rsp1aParams;
    sdrplay_api_Rsp2ParamsT rsp2Params;
    sdrplay_api_RspDuoParamsT rspDuoParams;
    sdrplay_api_RspDxParamsT rspDxParams;
} sdrplay_api_DevParamsT;

// Tuner parameter structs
typedef struct
{
    float curr;
    float max;
    float min;
} sdrplay_api_GainValuesT;

typedef struct 
{
    int gRdB;
    unsigned char LNAstate;
    unsigned char syncUpdate;
    sdrplay_api_MinGainReductionT minGr;
    sdrplay_api_GainValuesT gainVals;
} sdrplay_api_GainT;

typedef struct 
{
    double rfHz;
    unsigned char syncUpdate;
} sdrplay_api_RfFreqT;

typedef struct 
{
    unsigned char dcCal;
    unsigned char speedUp;
    int trackTime;
    int refreshRateTime;
} sdrplay_api_DcOffsetTunerT;

typedef struct 
{
    sdrplay_api_Bw_MHzT bwType;
    sdrplay_api_If_kHzT ifType;
    sdrplay_api_LoModeT loMode;
    sdrplay_api_GainT gain;
    sdrplay_api_RfFreqT rfFreq;
    sdrplay_api_DcOffsetTunerT dcOffsetTuner;
} sdrplay_api_TunerParamsT;

// Control parameter structs
typedef struct 
{
    unsigned char DCenable;
    unsigned char IQenable;
} sdrplay_api_DcOffsetT;

typedef struct 
{
    unsigned char enable;
    unsigned char decimationFactor;
    unsigned char wideBandSignal;
} sdrplay_api_DecimationT;

typedef struct 
{
    sdrplay_api_AgcControlT enable;
    int setPoint_dBfs;
    unsigned short attack_ms;
    unsigned short decay_ms;
    unsigned short decay_delay_ms;
    unsigned short decay_threshold_dB;
    int syncUpdate;
} sdrplay_api_AgcT;

typedef struct 
{
    sdrplay_api_DcOffsetT dcOffset;
    sdrplay_api_DecimationT decimation;
    sdrplay_api_AgcT agc;
    sdrplay_api_AdsbModeT adsbMode;
} sdrplay_api_ControlParamsT;

typedef struct
{
    sdrplay_api_TunerParamsT        tunerParams;
    sdrplay_api_ControlParamsT      ctrlParams;
    sdrplay_api_Rsp1aTunerParamsT   rsp1aTunerParams;
    sdrplay_api_Rsp2TunerParamsT    rsp2TunerParams;
    sdrplay_api_RspDuoTunerParamsT  rspDuoTunerParams;
    sdrplay_api_RspDxTunerParamsT   rspDxTunerParams;
} sdrplay_api_RxChannelParamsT;

// Device parameter structure
typedef struct
{
    sdrplay_api_DevParamsT       *devParams;
    sdrplay_api_RxChannelParamsT *rxChannelA;
    sdrplay_api_RxChannelParamsT *rxChannelB;
} sdrplay_api_DeviceParamsT;

// Event callback enums
typedef enum
{
    sdrplay_api_Overload_Detected     = 0,
    sdrplay_api_Overload_Corrected    = 1,
} sdrplay_api_PowerOverloadCbEventIdT;

typedef enum
{
    sdrplay_api_MasterInitialised      = 0,
    sdrplay_api_SlaveAttached          = 1,
    sdrplay_api_SlaveDetached          = 2,
    sdrplay_api_SlaveInitialised       = 3,
    sdrplay_api_SlaveUninitialised     = 4,
    sdrplay_api_MasterDllDisappeared   = 5,
    sdrplay_api_SlaveDllDisappeared    = 6,
} sdrplay_api_RspDuoModeCbEventIdT;

typedef enum
{
    sdrplay_api_GainChange            = 0,
    sdrplay_api_PowerOverloadChange   = 1,
    sdrplay_api_DeviceRemoved         = 2,
    sdrplay_api_RspDuoModeChange      = 3,
} sdrplay_api_EventT;

// Event callback parameter structs
typedef struct 
{
    unsigned int gRdB;
    unsigned int lnaGRdB;
    double currGain;
} sdrplay_api_GainCbParamT;

typedef struct 
{
    sdrplay_api_PowerOverloadCbEventIdT powerOverloadChangeType;
} sdrplay_api_PowerOverloadCbParamT;

typedef struct 
{
    sdrplay_api_RspDuoModeCbEventIdT modeChangeType;
} sdrplay_api_RspDuoModeCbParamT;

// Event parameters overlay
typedef union
{
    sdrplay_api_GainCbParamT          gainParams;
    sdrplay_api_PowerOverloadCbParamT powerOverloadParams;
    sdrplay_api_RspDuoModeCbParamT    rspDuoModeParams;
} sdrplay_api_EventParamsT;

// Stream callback parameter structs
typedef struct 
{
    unsigned int firstSampleNum;
    int grChanged;
    int rfChanged;
    int fsChanged;
    unsigned int numSamples;
} sdrplay_api_StreamCbParamsT;

// Callback function prototypes
typedef void (*sdrplay_api_StreamCallback_t)
(
    short *si,
    short *sq,
    sdrplay_api_StreamCbParamsT *params,
    unsigned int numSamples,
    unsigned int reset,
    void *cbContext
);

typedef void (*sdrplay_api_EventCallback_t)
(
    sdrplay_api_EventT eventId,
    sdrplay_api_TunerSelectT tuner,
    sdrplay_api_EventParamsT *params,
    void *cbContext
);

typedef struct
{
    sdrplay_api_StreamCallback_t StreamACbFn;
    sdrplay_api_StreamCallback_t StreamBCbFn;
    sdrplay_api_EventCallback_t  EventCbFn;
} sdrplay_api_CallbackFnsT;

// Update settings enum
typedef enum
{
    sdrplay_api_Update_None                        = 0x00000000,
    sdrplay_api_Update_Dev_Fs                      = 0x00000001,
    sdrplay_api_Update_Dev_Ppm                     = 0x00000002,
    sdrplay_api_Update_Dev_SyncUpdate              = 0x00000004,
    sdrplay_api_Update_Dev_ResetFlags              = 0x00000008,
    sdrplay_api_Update_Rsp1a_BiasTControl          = 0x00000010,
    sdrplay_api_Update_Rsp1a_RfNotchControl        = 0x00000020,
    sdrplay_api_Update_Rsp1a_RfDabNotchControl     = 0x00000040,
    sdrplay_api_Update_Rsp2_BiasTControl           = 0x00000080,
    sdrplay_api_Update_Rsp2_AmPortSelect           = 0x00000100,
    sdrplay_api_Update_Rsp2_AntennaControl         = 0x00000200,
    sdrplay_api_Update_Rsp2_RfNotchControl         = 0x00000400,
    sdrplay_api_Update_Rsp2_ExtRefControl          = 0x00000800,
    sdrplay_api_Update_RspDuo_ExtRefControl        = 0x00001000,
    sdrplay_api_Update_Master_Spare_1              = 0x00002000,
    sdrplay_api_Update_Master_Spare_2              = 0x00004000,
    sdrplay_api_Update_Tuner_Gr                    = 0x00008000,
    sdrplay_api_Update_Tuner_GrLimits              = 0x00010000,
    sdrplay_api_Update_Tuner_Frf                   = 0x00020000,
    sdrplay_api_Update_Tuner_BwType                = 0x00040000,
    sdrplay_api_Update_Tuner_IfType                = 0x00080000,
    sdrplay_api_Update_Tuner_DcOffset              = 0x00100000,
    sdrplay_api_Update_Tuner_LoMode                = 0x00200000,
    sdrplay_api_Update_Ctrl_DCoffsetIQimbalance    = 0x00400000,
    sdrplay_api_Update_Ctrl_Decimation             = 0x00800000,
    sdrplay_api_Update_Ctrl_Agc                    = 0x01000000,
    sdrplay_api_Update_Ctrl_AdsbMode               = 0x02000000,
    sdrplay_api_Update_Ctrl_OverloadMsgAck         = 0x04000000,
    sdrplay_api_Update_RspDuo_BiasTControl         = 0x08000000,
    sdrplay_api_Update_RspDuo_AmPortSelect         = 0x10000000,
    sdrplay_api_Update_RspDuo_Tuner1AmNotchControl = 0x20000000,
    sdrplay_api_Update_RspDuo_RfNotchControl       = 0x40000000,
    sdrplay_api_Update_RspDuo_RfDabNotchControl    = 0x80000000,
} sdrplay_api_ReasonForUpdateT;

typedef enum
{
    sdrplay_api_Update_Ext1_None                   = 0x00000000,
    sdrplay_api_Update_RspDx_HdrEnable             = 0x00000001,
    sdrplay_api_Update_RspDx_BiasTControl          = 0x00000002,
    sdrplay_api_Update_RspDx_AntennaControl        = 0x00000004,
    sdrplay_api_Update_RspDx_RfNotchControl        = 0x00000008,
    sdrplay_api_Update_RspDx_RfDabNotchControl     = 0x00000010,
    sdrplay_api_Update_RspDx_HdrBw                 = 0x00000020,
} sdrplay_api_ReasonForUpdateExtension1T;


/**************************************************/

// REMOVE to avoid error during development
#define UNUSED(x) (void)(x)

// if driver debug is enabled or not
#define SDRPLAY3_DEBUG 0

// how many devices we can discover
#define MAX_RSP_DEVICES 4

/*
 * sample frequency in kHz
 * values between 2MHz and 10.66MHz are permitted. Decimation can be used
 * to obtain lower sample rates.
 */
#define SDRPLAY3_MIN_RATE 2000
#define SDRPLAY3_MAX_RATE 10660

/*
 * frequency range in kHz
 */
#define SDRPLAY3_MIN_FREQ 1
#define SDRPLAY3_MAX_FREQ 2000000

/*
 * min and max IF gain reduction in dB
 */
#define SDRPLAY3_NORMAL_MIN_GR 20
#define SDRPLAY3_MAX_BB_GR 59

// device names
#define SDRPLAY3_RSP1_NAME                    "RSP1"
#define SDRPLAY3_RSP1A_NAME                   "RSP1A"
#define SDRPLAY3_RSP2_NAME                    "RSP2"
#define SDRPLAY3_RSPduo_NAME                  "RSPduo"
#define SDRPLAY3_RSPdx_NAME                   "RSPdx"

// AGC mode names
#define SDRPLAY3_AGC_DISABLE_NAME             "Disabled"
#define SDRPLAY3_AGC_100HZ_NAME               "AGC time constant 100 Hz"
#define SDRPLAY3_AGC_50HZ_NAME                "AGC time constant 50 Hz"
#define SDRPLAY3_AGC_5HZ_NAME                 "AGC time constant 5 Hz"
#define SDRPLAY3_AGC_CTRL_EN_NAME             "custom AGC parameters"

// RSPduo mode names
#define SDRPLAY3_RSPduo_SINGLE_TUNER_NAME     "Single Tuner"
#define SDRPLAY3_RSPduo_DUAL_TUNER_NAME       "Dual Tuner"
#define SDRPLAY3_RSPduo_MASTER_MODE_NAME      "Master"
#define SDRPLAY3_RSPduo_SLAVE_MODE_NAME       "Slave"

// RSPduo tuner names
#define SDRPLAY3_RSPduo_TUNER_A_NAME          "Tuner A"
#define SDRPLAY3_RSPduo_TUNER_B_NAME          "Tuner B"
#define SDRPLAY3_RSPduo_BOTH_TUNERS_NAME      "Both Tuners"

// flags to manage the sdrplay3 status
typedef enum
{
  sdrplay3_None_Flag                = 0,
  sdrplay3_Library_Loaded_Flag      = 1,
  sdrplay3_API_Opened_Flag          = 2,
  sdrplay3_Device_Selected_Flag     = 4,
  sdrplay3_RspDuo_Selected_Flag     = 8
} sdrplay3_Status_t;
static sdrplay3_Status_t sdrplay3_status = sdrplay3_None_Flag;

// internal sdrplay3 device structure and useful pointers
static sdrplay_api_DeviceT device;
static sdrplay_api_DeviceParamsT *device_params = 0;
static sdrplay_api_RxChannelParamsT *rx_channel_params = 0;

// Structure for SDRplay3 parameters
typedef struct {
  int sernum1;
  int sernum2;
  int sernum3;
  int sernum4;
  int hw_version;
  int rspduo_mode;
  int tuner;
  int rspduo_sample_freq;
  int sample_rate;
  int decimation;
  int if_freq;
  int if_bandwidth;
  int antenna;
  int rf_lna_state;
  int rf_lna_state_freq;
  int rf_lna_state_max;
  int if_agc;
  int if_gain_reduction;
  int if_agc_setpoint_dbfs;
  int if_agc_attack_ms;
  int if_agc_decay_ms;
  int if_agc_decay_delay_ms;
  int if_agc_decay_threshold_db;
  int freq_corr_ppb;
  int dc_offset_corr;
  int iq_imbalance_corr;
  int usb_transfer_mode;
  int notch_filters;
  int bias_t;
  int rspdx_hdr_mode_and_bandwidth;
  int check;
} P_SDRPLAY3;

#define MAX_SDRPLAY3_PARM 31
#define RX2_OFFSET 10000

static P_SDRPLAY3 sdrplay3_parms;

/*
 * stop reading data from I/Q callback
 * 1 = return without processing data
 * 0 = process data
 */
int sdrplay3_cancel_flag;
// borrowed from airspy.c
extern int timf1p_sdr2;

// used on wse_sdrxx.c for gain
int sdrplay3_max_gain = 0;

static int ad_overload_flag = 0;

static char *sdrplay3_parm_text[MAX_SDRPLAY3_PARM] = {
  "Serno1",
  "Serno2",
  "Serno3",
  "Serno4",
  "HwVer",
  "RSPduo mode",
  "Tuner",
  "RSPduo sample freq",
  "Sample rate",
  "Decimation",
  "IF freq",
  "IF bandwidth",
  "Antenna",
  "RF LNA state",
  "RF LNA state freq",
  "RF LNA state max",
  "IF AGC",
  "IF gain reduction",
  "IF AGC setPoint dBfs",
  "IF AGC attack ms",
  "IF AGC decay ms",
  "IF AGC decay delay ms",
  "IF AGC decay threshold dB",
  "Freq corr ppb",
  "DC offset corr",
  "IQ imbalance corr",
  "USB transfer mode",
  "Notch filters",
  "Bias-T",
  "RSPdx HDR mode and bandwidth",
  "Check"
};

static char *sdrplay3_parfil_name = "par_sdrplay3";

/***** load library ******/
typedef sdrplay_api_ErrT (*p_sdrplay_api_Open)(void);
typedef sdrplay_api_ErrT (*p_sdrplay_api_Close)(void);
typedef sdrplay_api_ErrT (*p_sdrplay_api_ApiVersion)(float *apiVer);
typedef sdrplay_api_ErrT (*p_sdrplay_api_DebugEnable)(HANDLE dev, sdrplay_api_DbgLvl_t dbgLvl);
typedef sdrplay_api_ErrT (*p_sdrplay_api_LockDeviceApi)(void);
typedef sdrplay_api_ErrT (*p_sdrplay_api_UnlockDeviceApi)(void);
typedef sdrplay_api_ErrT (*p_sdrplay_api_GetDevices)(sdrplay_api_DeviceT *devices, unsigned int *numDevs, unsigned int maxDevs);
typedef sdrplay_api_ErrT (*p_sdrplay_api_SelectDevice)(sdrplay_api_DeviceT *device_);
typedef sdrplay_api_ErrT (*p_sdrplay_api_ReleaseDevice)(sdrplay_api_DeviceT *device_);
typedef sdrplay_api_ErrT (*p_sdrplay_api_GetDeviceParams)(HANDLE dev, sdrplay_api_DeviceParamsT **deviceParams);
typedef sdrplay_api_ErrT (*p_sdrplay_api_Init)(HANDLE dev, sdrplay_api_CallbackFnsT *callbackFns, void *cbContext);
typedef sdrplay_api_ErrT (*p_sdrplay_api_Uninit)(HANDLE dev);
typedef sdrplay_api_ErrT (*p_sdrplay_api_Update)(HANDLE dev, sdrplay_api_TunerSelectT tuner, sdrplay_api_ReasonForUpdateT reasonForUpdate, sdrplay_api_ReasonForUpdateExtension1T reasonForUpdateExt1);

static p_sdrplay_api_Open sdrplay_api_Open;
static p_sdrplay_api_Open sdrplay_api_Close;
static p_sdrplay_api_ApiVersion sdrplay_api_ApiVersion;
static p_sdrplay_api_DebugEnable sdrplay_api_DebugEnable;
static p_sdrplay_api_LockDeviceApi sdrplay_api_LockDeviceApi;
static p_sdrplay_api_UnlockDeviceApi sdrplay_api_UnlockDeviceApi;
static p_sdrplay_api_GetDevices sdrplay_api_GetDevices;
static p_sdrplay_api_SelectDevice sdrplay_api_SelectDevice;
static p_sdrplay_api_ReleaseDevice sdrplay_api_ReleaseDevice;
static p_sdrplay_api_GetDeviceParams sdrplay_api_GetDeviceParams;
static p_sdrplay_api_Init sdrplay_api_Init;
static p_sdrplay_api_Uninit sdrplay_api_Uninit;
static p_sdrplay_api_Update sdrplay_api_Update;

#if(OSNUM == OSNUM_WINDOWS)
#include <windows.h>
static HANDLE sdrplay3_libhandle;
static int load_library(void) {
  char sdrplay3_dllname[80];
  snprintf(sdrplay3_dllname, 80, "%ssdrplay_api", DLLDIR);
  sdrplay3_libhandle = LoadLibraryEx(sdrplay3_dllname, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
  return sdrplay3_libhandle ? TRUE : FALSE;
}
static void *get_function_address(const char *name) {
  return (void *) GetProcAddress(sdrplay3_libhandle, name);
}
static void unload_library(void)
{
  if (sdrplay3_status & sdrplay3_Library_Loaded_Flag)
    FreeLibrary(sdrplay3_libhandle);
  sdrplay3_status &= ~sdrplay3_Library_Loaded_Flag;
}
static void load_error(int info)
{
  char sdrplay3_dllname[80];
  snprintf(sdrplay3_dllname, 80, "%ssdrplay_api", DLLDIR);
  library_error_screen(sdrplay3_dllname, info);
}
#endif

#if(OSNUM == OSNUM_LINUX)
#include <dlfcn.h>
static void *sdrplay3_libhandle;
static int load_library(void) {
  sdrplay3_libhandle = dlopen(SDRPLAY3_LIBNAME, RTLD_NOW);
  return sdrplay3_libhandle ? 1 : 0;
}
static void *get_function_address(const char *name) {
  return (void *) dlsym(sdrplay3_libhandle, name);
}
static void unload_library(void)
{
  if (sdrplay3_status & sdrplay3_Library_Loaded_Flag)
    dlclose(sdrplay3_libhandle);
  sdrplay3_status &= ~sdrplay3_Library_Loaded_Flag;
}
static void load_error(int info)
{
  library_error_screen(SDRPLAY3_LIBNAME, info);
}
#endif


/************** internal functions ************************/
static void load_sdrplay3_library(void)
{
  int info;
  if (sdrplay3_status & sdrplay3_Library_Loaded_Flag)
    return;
  info = 0;
  if (!load_library())
    goto sdrplay3_load_error;
  info = 1;

  sdrplay_api_Open = (p_sdrplay_api_Open) get_function_address("sdrplay_api_Open");
  if (!sdrplay_api_Open) goto sdrplay3_sym_error;
  sdrplay_api_Close = (p_sdrplay_api_Close) get_function_address("sdrplay_api_Close");
  if (!sdrplay_api_Close) goto sdrplay3_sym_error;

  sdrplay_api_ApiVersion = (p_sdrplay_api_ApiVersion) get_function_address("sdrplay_api_ApiVersion");
  if (!sdrplay_api_ApiVersion) goto sdrplay3_sym_error;

  sdrplay_api_DebugEnable = (p_sdrplay_api_DebugEnable) get_function_address("sdrplay_api_DebugEnable");
  if (!sdrplay_api_DebugEnable) goto sdrplay3_sym_error;

  sdrplay_api_LockDeviceApi = (p_sdrplay_api_LockDeviceApi) get_function_address("sdrplay_api_LockDeviceApi");
  if (!sdrplay_api_LockDeviceApi) goto sdrplay3_sym_error;
  sdrplay_api_UnlockDeviceApi = (p_sdrplay_api_UnlockDeviceApi) get_function_address("sdrplay_api_UnlockDeviceApi");
  if (!sdrplay_api_UnlockDeviceApi) goto sdrplay3_sym_error;

  sdrplay_api_GetDevices = (p_sdrplay_api_GetDevices) get_function_address("sdrplay_api_GetDevices");
  if (!sdrplay_api_GetDevices) goto sdrplay3_sym_error;
  sdrplay_api_SelectDevice = (p_sdrplay_api_SelectDevice) get_function_address("sdrplay_api_SelectDevice");
  if (!sdrplay_api_SelectDevice) goto sdrplay3_sym_error;
  sdrplay_api_ReleaseDevice = (p_sdrplay_api_ReleaseDevice) get_function_address("sdrplay_api_ReleaseDevice");
  if (!sdrplay_api_ReleaseDevice) goto sdrplay3_sym_error;

  sdrplay_api_GetDeviceParams = (p_sdrplay_api_GetDeviceParams) get_function_address("sdrplay_api_GetDeviceParams");
  if (!sdrplay_api_GetDeviceParams) goto sdrplay3_sym_error;

  sdrplay_api_Init = (p_sdrplay_api_Init) get_function_address("sdrplay_api_Init");
  if (!sdrplay_api_Init) goto sdrplay3_sym_error;
  sdrplay_api_Uninit = (p_sdrplay_api_Uninit) get_function_address("sdrplay_api_Uninit");
  if (!sdrplay_api_Uninit) goto sdrplay3_sym_error;

  sdrplay_api_Update = (p_sdrplay_api_Update) get_function_address("sdrplay_api_Update");
  if (!sdrplay_api_Update) goto sdrplay3_sym_error;

  sdrplay3_status |= sdrplay3_Library_Loaded_Flag;
  return;

sdrplay3_sym_error:
  unload_library();
sdrplay3_load_error:
  load_error(info);
  return;
}

static int sdrplay3_open_api(void)
{
  int errcod;
  sdrplay_api_ErrT err;
  float apiVer;

  if (sdrplay3_status & sdrplay3_API_Opened_Flag)
    return 0;
  errcod = 2610;
  load_sdrplay3_library();
  if (!(sdrplay3_status & sdrplay3_Library_Loaded_Flag))
    return errcod;

  errcod = 2611;
  err = sdrplay_api_Open();
  if (err != sdrplay_api_Success)
    goto unload_sdrplay3_library;

  // check version
  errcod = 2612;
  err = sdrplay_api_ApiVersion(&apiVer);
  if (err != sdrplay_api_Success)
    goto close_sdrplay3_api;
  errcod = 2613;
  if (apiVer != SDRPLAY_API_VERSION)
    goto close_sdrplay3_api;

  // enable debug output
  errcod = 2614;
  err = sdrplay_api_DebugEnable(NULL, SDRPLAY3_DEBUG);
  if (err != sdrplay_api_Success)
    goto close_sdrplay3_api;

  sdrplay3_status |= sdrplay3_API_Opened_Flag;
  return 0;

close_sdrplay3_api:
  sdrplay_api_Close();
  sdrplay3_status &= ~sdrplay3_API_Opened_Flag;
unload_sdrplay3_library:
  unload_library();
  return errcod;
}

static void sdrplay3_display_open_api_error(int errcod)
{
  char s[120];
  char *errmsg;

  if (errcod == 0)
    return;
  if (errcod == 2610)
    return;
  switch (errcod) {
    case 2611:
      errmsg = "sdrplay3: sdrplay_api_Open() failed.";
      break;
    case 2612:
      errmsg = "sdrplay3: sdrplay_api_ApiVersion() failed.";
      break;
    case 2613:
      errmsg = "sdrplay3: library version doesn't match with include.";
      break;
    case 2614:
      errmsg = "sdrplay3: sdrplay_api_DebugEnable() failed.";
      break;
    default:
      snprintf(s, 120, "sddrplay3: unknown failure - error code %d", errcod);
      errmsg = s;
      break;
  }
  lir_text(3, 2, errmsg);
  lir_text(5, 7, press_any_key);
  await_keyboard();
}

static int sdrplay3_select_if_frequency(int *line);

static void sdrplay3_select_rspduo(int *line)
{
  char s[120];
  char *rspduo_mode_name = 0;
  char *rspduo_tuner_name = 0;

  if (device.rspDuoMode == sdrplay_api_RspDuoMode_Single_Tuner) {
    rspduo_mode_name = SDRPLAY3_RSPduo_SINGLE_TUNER_NAME;
  } else if (device.rspDuoMode == sdrplay_api_RspDuoMode_Dual_Tuner) {
    rspduo_mode_name = SDRPLAY3_RSPduo_DUAL_TUNER_NAME;
  } else if (device.rspDuoMode == sdrplay_api_RspDuoMode_Master) {
    rspduo_mode_name = SDRPLAY3_RSPduo_MASTER_MODE_NAME;
  } else if (device.rspDuoMode == sdrplay_api_RspDuoMode_Slave) {
    rspduo_mode_name = SDRPLAY3_RSPduo_SLAVE_MODE_NAME;
  }
  line[0] += 2;
  if (rspduo_mode_name != 0) {
    snprintf(s, 120, "RSPduo mode autoselected: %s", rspduo_mode_name);
    lir_text(3, line[0], s);
  } else {
    if (device.rspDuoMode & sdrplay_api_RspDuoMode_Single_Tuner) {
      snprintf(s, 120, "  S=%s", SDRPLAY3_RSPduo_SINGLE_TUNER_NAME);
      lir_text(3, line[0], s);
      line[0]++;
    }
    if (device.rspDuoMode & sdrplay_api_RspDuoMode_Dual_Tuner) {
      snprintf(s, 120, "  D=%s", SDRPLAY3_RSPduo_DUAL_TUNER_NAME);
      lir_text(3, line[0], s);
      line[0]++;
    }
    if (device.rspDuoMode & sdrplay_api_RspDuoMode_Master) {
      snprintf(s, 120, "  M=%s", SDRPLAY3_RSPduo_MASTER_MODE_NAME);
      lir_text(3, line[0], s);
      line[0]++;
    }
    line[0]++;
    lir_text(3, line[0], "Select RSPduo mode (S/D/M) =>");
    for (;;) {
      await_processed_keyboard();
      if (kill_all_flag)
        return;
      switch (lir_inkey) {
        case 'S':
          if (device.rspDuoMode & sdrplay_api_RspDuoMode_Single_Tuner)
            device.rspDuoMode = sdrplay_api_RspDuoMode_Single_Tuner;
          break;
        case 'D':
          if (device.rspDuoMode & sdrplay_api_RspDuoMode_Dual_Tuner)
            device.rspDuoMode = sdrplay_api_RspDuoMode_Dual_Tuner;
          break;
        case 'M':
          if (device.rspDuoMode & sdrplay_api_RspDuoMode_Master)
            device.rspDuoMode = sdrplay_api_RspDuoMode_Master;
          break;
        default:
          continue;
      }
      break;
    }
  }

  if (device.tuner == sdrplay_api_Tuner_A) {
    rspduo_tuner_name = SDRPLAY3_RSPduo_TUNER_A_NAME;
  } else if (device.tuner == sdrplay_api_Tuner_B) {
    rspduo_tuner_name = SDRPLAY3_RSPduo_TUNER_B_NAME;
  } else if (device.tuner == sdrplay_api_Tuner_Both && device.rspDuoMode == sdrplay_api_RspDuoMode_Dual_Tuner) {
    rspduo_tuner_name = SDRPLAY3_RSPduo_BOTH_TUNERS_NAME;
  }
  line[0] += 2;
  if (rspduo_tuner_name != 0) {
    snprintf(s, 120, "RSPduo tuner autoselected: %s", rspduo_tuner_name);
    lir_text(3, line[0], s);
  } else {
    if (device.tuner & sdrplay_api_Tuner_A) {
      snprintf(s, 120, "A=%s", SDRPLAY3_RSPduo_TUNER_A_NAME);
      lir_text(3, line[0], s);
      line[0]++;
    }
    if (device.tuner & sdrplay_api_Tuner_B) {
      snprintf(s, 120, "B=%s", SDRPLAY3_RSPduo_TUNER_B_NAME);
      lir_text(3, line[0], s);
      line[0]++;
    }
    line[0]++;
    lir_text(3, line[0], "Select RSPduo tuner (A/B) =>");
    for (;;) {
      await_processed_keyboard();
      if (kill_all_flag)
        return;
      switch (lir_inkey) {
        case 'A':
          device.tuner = sdrplay_api_Tuner_A;
          break;
        case 'B':
          device.tuner = sdrplay_api_Tuner_B;
          break;
        default:
          continue;
      }
      break;
    }
  }

  if (sdrplay3_select_if_frequency(line) != 0)
    return;

  sdrplay3_status |= sdrplay3_RspDuo_Selected_Flag;
  return;
}

static void sdrplay3_select_device(int *line)
{
  unsigned int i;
  char s[120];
  sdrplay_api_ErrT err;
  unsigned int max_devs = MAX_RSP_DEVICES;
  sdrplay_api_DeviceT devices[MAX_RSP_DEVICES];
  unsigned int num_devs = 0;
  char *device_name = "UNKNOWN DEVICE";

  /* search sdrplay3 device */
  lir_text(3, 2, "SEARCHING");
  err = sdrplay_api_LockDeviceApi();
  if (err != sdrplay_api_Success) {
    lir_text(3, 2, "sdrplay3: sdrplay_api_LockDeviceApi() failed.");
    lir_text(5, 7, press_any_key);
    await_keyboard();
    goto close_sdrplay3_api;
  }

  err = sdrplay_api_GetDevices(devices, &num_devs, max_devs);
  if (err != sdrplay_api_Success) {
    lir_text(3, 2, "sdrplay3: sdrplay_api_GetDevices() failed.");
    lir_text(5, 7, press_any_key);
    await_keyboard();
    goto unlock_sdrplay3_device_api;
  }
  clear_lines(2, 2);
  if (num_devs == 0) {
    lir_text(5, 5, "SDRPlay3 not found.");
    lir_text(5, 7, press_any_key);
    await_keyboard();
    goto unlock_sdrplay3_device_api;
  }

  line[0] = 2;
  for (i = 0; i < num_devs; i++) {
    if (devices[i].hwVer == SDRPLAY3_RSP1_ID) {
      device_name = SDRPLAY3_RSP1_NAME;
    } else if (devices[i].hwVer == SDRPLAY3_RSP1A_ID) {
      device_name = SDRPLAY3_RSP1A_NAME;
    } else if (devices[i].hwVer == SDRPLAY3_RSP2_ID) {
      device_name = SDRPLAY3_RSP2_NAME;
    } else if (devices[i].hwVer == SDRPLAY3_RSPduo_ID) {
      device_name = SDRPLAY3_RSPduo_NAME;
    } else if (devices[i].hwVer == SDRPLAY3_RSPdx_ID) {
      device_name = SDRPLAY3_RSPdx_NAME;
    }
    snprintf(s, 120, " %2d: %s   MFG: %s   SN: %.64s", i, device_name,
             "SDRplay", devices[i].SerNo);
    lir_text(3, line[0], s);
    line[0]++;
  }
  line[0]++;

  if (num_devs == 1) {
    lir_text(3, line[0], "Device autoselected.");
    i = 0;
  } else {
    lir_text(3, line[0], "Select device by line number:");
    i = lir_get_integer(33, line[0], 2, 0, num_devs - 1);
  }
  if (kill_all_flag)
    goto unlock_sdrplay3_device_api;

  device = devices[i];

  // RSPduo mode selection
  if (device.hwVer == SDRPLAY3_RSPduo_ID) {
    sdrplay3_select_rspduo(line);
    if (!(sdrplay3_status & sdrplay3_RspDuo_Selected_Flag))
      goto unlock_sdrplay3_device_api;
  }

  // select device
  err = sdrplay_api_SelectDevice(&device);
  if (err != sdrplay_api_Success) {
    lir_text(3, 2, "sdrplay3: sdrplay_api_SelectDevice() failed.");
    lir_text(5, 7, press_any_key);
    await_keyboard();
    goto unlock_sdrplay3_device_api;
  }

  err = sdrplay_api_UnlockDeviceApi();
  if (err != sdrplay_api_Success) {
    lir_text(3, 2, "sdrplay3: sdrplay_api_UnlockDeviceApi() failed.");
    lir_text(5, 7, press_any_key);
    await_keyboard();
    goto release_sdrplay3_device;
  }

  err = sdrplay_api_GetDeviceParams(device.dev, &device_params);
  if (err != sdrplay_api_Success) {
    lir_text(3, 2, "sdrplay3: sdrplay_api_GetDeviceParams() failed.");
    lir_text(5, 7, press_any_key);
    await_keyboard();
    goto release_sdrplay3_device;
  }

  if (device.tuner == sdrplay_api_Tuner_Neither || device.tuner == sdrplay_api_Tuner_A) {
    rx_channel_params = device_params->rxChannelA;
  } else if (device.tuner == sdrplay_api_Tuner_B) {
    rx_channel_params = device_params->rxChannelB;
  } else if (device.tuner == sdrplay_api_Tuner_Both) {
    rx_channel_params = device_params->rxChannelA;
  }

  sdrplay3_status |= sdrplay3_Device_Selected_Flag;
  return;

release_sdrplay3_device:
  sdrplay_api_ReleaseDevice(&device);
unlock_sdrplay3_device_api:
  sdrplay_api_UnlockDeviceApi();
close_sdrplay3_api:
  sdrplay_api_Close();
  sdrplay3_status &= ~sdrplay3_API_Opened_Flag;
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
static int sdrplay3_select_samplerate(int *line)
{
  char s[1024];
  int decimation;
  int sample_rates_index_max;
  int sample_rate_index;

  line[0] += 2;
  if (device.hwVer == SDRPLAY3_RSPduo_ID &&
      (device.rspDuoMode == sdrplay_api_RspDuoMode_Dual_Tuner ||
       device.rspDuoMode == sdrplay_api_RspDuoMode_Master ||
       device.rspDuoMode == sdrplay_api_RspDuoMode_Slave)) {
    decimation = 32;
    for (sample_rates_index_max = 0; decimation > 0; sample_rates_index_max++) {
      snprintf(s, 1024, " %2d: sample rate %g kHz", sample_rates_index_max, 2000.0 / decimation);
      decimation /= 2;
      lir_text(3, line[0], s);
      line[0]++;
    }
    sample_rates_index_max--;
    line[0]++;
    lir_text(3, line[0], "Select sample rate by line number =>");
    sample_rate_index = lir_get_integer(40, line[0], 2, 0,  sample_rates_index_max);
    if (kill_all_flag)
      return -1;
    decimation = 1;
    for (; sample_rates_index_max > sample_rate_index; sample_rates_index_max--)
      decimation *= 2;
    if (device_params->devParams)
      device_params->devParams->fsFreq.fsHz = device.rspDuoSampleFreq;
  } else {
    snprintf(s, 1024, "Select sampling rate in kHz (%g to %g) =>",
             SDRPLAY3_MIN_RATE / 32., (double)(SDRPLAY3_MAX_RATE));
    lir_text(3, line[0], s);
    float samplerate = lir_get_float(strlen(s) + 4, line[0], 5, SDRPLAY3_MIN_RATE / 32.0, SDRPLAY3_MAX_RATE);
    if (kill_all_flag)
      return -1;
    decimation = 1;
    while (samplerate < SDRPLAY3_MIN_RATE) {
      decimation *= 2;
      samplerate *= 2.;
    }
    device_params->devParams->fsFreq.fsHz = samplerate * 1000.0;
  }
  if (decimation > 1)
    rx_channel_params->ctrlParams.decimation.enable = 1;
  else
    rx_channel_params->ctrlParams.decimation.enable = 0;
  rx_channel_params->ctrlParams.decimation.decimationFactor = decimation;
  return 0;
}

static double sdrplay3_get_samplerate(void)
{
  double samplerate;

  samplerate = 0.0;
  if (rx_channel_params->tunerParams.ifType == sdrplay_api_IF_Zero) {
    samplerate = device_params->devParams->fsFreq.fsHz;
  } else if (rx_channel_params->tunerParams.ifType == sdrplay_api_IF_1_620) {
    samplerate = 2e6;
  } else if (rx_channel_params->tunerParams.ifType == sdrplay_api_IF_2_048) {
    samplerate = 2e6;
  }
  if (rx_channel_params->ctrlParams.decimation.enable)
    samplerate /= rx_channel_params->ctrlParams.decimation.decimationFactor;
  return samplerate;
}

static int sdrplay3_select_if_frequency(int *line)
{
  char s[120];
  sdrplay_api_If_kHzT ifType;

  if (!(sdrplay3_status & sdrplay3_Device_Selected_Flag)) {
    if (device.hwVer == SDRPLAY3_RSPduo_ID &&
        (device.rspDuoMode == sdrplay_api_RspDuoMode_Dual_Tuner ||
         device.rspDuoMode == sdrplay_api_RspDuoMode_Master)) {
      line[0] += 2;
      lir_text(3, line[0], "A=1620 kHz");
      line[0]++;
      lir_text(3, line[0], "B=2048 kHz");
      line[0] += 2;
      lir_text(3, line[0], "Select IF frequency (A/B) =>");
      ifType = 0;
      for (;;) {
        await_processed_keyboard();
        if (kill_all_flag)
          return -1;
        switch (lir_inkey) {
          case 'A':
            device.rspDuoSampleFreq = 6e6;
            ifType = sdrplay_api_IF_1_620;
            break;
          case 'B':
            device.rspDuoSampleFreq = 8e6;
            ifType = sdrplay_api_IF_2_048;
            break;
          default:
            continue;
        }
        break;
      }
      line[0]++;
      snprintf(s, 120, "Selected IF frequency is %d kHz", ifType);
      lir_text(3, line[0], s);
    }
    return 0;
  }

  // device select (normal) case
  if (device.hwVer == SDRPLAY3_RSPduo_ID &&
      (device.rspDuoMode == sdrplay_api_RspDuoMode_Dual_Tuner ||
       device.rspDuoMode == sdrplay_api_RspDuoMode_Master ||
       device.rspDuoMode == sdrplay_api_RspDuoMode_Slave)) {
    if (device.rspDuoSampleFreq == 6e6) {
      rx_channel_params->tunerParams.ifType = sdrplay_api_IF_1_620;
    } else if (device.rspDuoSampleFreq == 8e6) {
      rx_channel_params->tunerParams.ifType = sdrplay_api_IF_2_048;
    }
    return 0;
  }

  line[0] += 2;
  lir_text(3, line[0], "A=Zero IF");
  line[0]++;
  lir_text(3, line[0], "B=450 kHz");
  line[0]++;
  lir_text(3, line[0], "C=1620 kHz");
  line[0]++;
  lir_text(3, line[0], "D=2048 kHz");
  line[0] += 2;
  lir_text(3, line[0], "Select IF frequency (A to D) =>");
  for (;;) {
    await_processed_keyboard();
    if (kill_all_flag)
      return -1;
    switch (lir_inkey) {
      case 'A':
        rx_channel_params->tunerParams.ifType = sdrplay_api_IF_Zero;
        break;
      case 'B':
        rx_channel_params->tunerParams.ifType = sdrplay_api_IF_0_450;
        break;
      case 'C':
        rx_channel_params->tunerParams.ifType = sdrplay_api_IF_1_620;
        break;
      case 'D':
        rx_channel_params->tunerParams.ifType = sdrplay_api_IF_2_048;
        break;
      default:
        continue;
    }
    break;
  }
  line[0]++;
  snprintf(s, 120, "Selected IF frequency is %d kHz", rx_channel_params->tunerParams.ifType);
  lir_text(3, line[0], s);
  return 0;
}

static int sdrplay3_select_bandwidth(int *line)
{
  char s[120];
  double samplerate;
  char max_bandwidth;

  samplerate = sdrplay3_get_samplerate();

  line[0] += 2;
  if (samplerate < 300e3) {
    rx_channel_params->tunerParams.bwType = sdrplay_api_BW_0_200;
    snprintf(s, 120, "IF bandwidth autoselected: %d kHz", rx_channel_params->tunerParams.bwType);
    lir_text(3, line[0], s);
    return 0;
  }

  // list the possible bandwidths that are <= of the samplerate
  lir_text(3, line[0], "A=200 kHz");
  max_bandwidth = 'A';
  if (samplerate >= 300e3) {
    line[0]++;
    lir_text(3, line[0], "B=300 kHz");
    max_bandwidth = 'B';
  }
  if (samplerate >= 600e3) {
    line[0]++;
    lir_text(3, line[0], "C=600 kHz");
    max_bandwidth = 'C';
  }
  if (samplerate >= 1536e3) {
    line[0]++;
    lir_text(3, line[0], "D=1536 kHz");
    max_bandwidth = 'D';
  }
  if (samplerate >= 5000e3) {
    line[0]++;
    lir_text(3, line[0], "E=5 MHz");
    max_bandwidth = 'E';
  }
  if (samplerate >= 6000e3) {
    line[0]++;
    lir_text(3, line[0], "F=6 MHz");
    max_bandwidth = 'F';
  }
  if (samplerate >= 7000e3) {
    line[0]++;
    lir_text(3, line[0], "G=7 MHz");
    max_bandwidth = 'G';
  }
  if (samplerate >= 8000e3) {
    line[0]++;
    lir_text(3, line[0], "H=8 MHz");
    max_bandwidth = 'H';
  }
  line[0] += 2;
  snprintf(s, 120, "Selected IF bandwidth (A to %c) =>", max_bandwidth);
  lir_text(3, line[0], s);
  for (;;) {
    await_processed_keyboard();
    if (kill_all_flag)
      return -1;
    if (lir_inkey > max_bandwidth)
      continue;
    switch (lir_inkey) {
      case 'A':
        rx_channel_params->tunerParams.bwType = sdrplay_api_BW_0_200;
        break;
      case 'B':
        rx_channel_params->tunerParams.bwType = sdrplay_api_BW_0_300;
        break;
      case 'C':
        rx_channel_params->tunerParams.bwType = sdrplay_api_BW_0_600;
        break;
      case 'D':
        rx_channel_params->tunerParams.bwType = sdrplay_api_BW_1_536;
        break;
      case 'E':
        rx_channel_params->tunerParams.bwType = sdrplay_api_BW_5_000;
        break;
      case 'F':
        rx_channel_params->tunerParams.bwType = sdrplay_api_BW_6_000;
        break;
      case 'G':
        rx_channel_params->tunerParams.bwType = sdrplay_api_BW_7_000;
        break;
      case 'H':
        rx_channel_params->tunerParams.bwType = sdrplay_api_BW_8_000;
        break;
      default:
        continue;
    }
    break;
  }
  line[0]++;
  snprintf(s, 120, "Selected IF bandwidth is %d kHz", rx_channel_params->tunerParams.bwType);
  lir_text(3, line[0], s);
  return 0;
}

static int sdrplay3_select_antenna(int *line)
{
  if (device.hwVer == SDRPLAY3_RSP2_ID) {
    line[0] += 2;
    lir_text(3, line[0], "A=Antenna A");
    line[0]++;
    lir_text(3, line[0], "B=Antenna B");
    line[0]++;
    lir_text(3, line[0], "Z=Hi-Z");
    line[0] += 2;
    lir_text(3, line[0], "Select antenna (A/B/Z) =>");
    for (;;) {
      await_processed_keyboard();
      if (kill_all_flag)
        return -1;
      switch (lir_inkey) {
        case 'A':
          rx_channel_params->rsp2TunerParams.antennaSel = sdrplay_api_Rsp2_ANTENNA_A;
          rx_channel_params->rsp2TunerParams.amPortSel = sdrplay_api_Rsp2_AMPORT_2;
          line[0]++;
          lir_text(3, line[0], "Selected antenna is Antenna A");
          break;
        case 'B':
          rx_channel_params->rsp2TunerParams.antennaSel = sdrplay_api_Rsp2_ANTENNA_B;
          rx_channel_params->rsp2TunerParams.amPortSel = sdrplay_api_Rsp2_AMPORT_2;
          line[0]++;
          lir_text(3, line[0], "Selected antenna is Antenna B");
          break;
        case 'Z':
          rx_channel_params->rsp2TunerParams.antennaSel = sdrplay_api_Rsp2_ANTENNA_A;
          rx_channel_params->rsp2TunerParams.amPortSel = sdrplay_api_Rsp2_AMPORT_1;
          line[0]++;
          lir_text(3, line[0], "Selected antenna is Hi-Z");
          break;
        default:
          continue;
      }
      break;
    }
  } else if (device.hwVer == SDRPLAY3_RSPduo_ID) {
    if (rx_channel_params == device_params->rxChannelA) {
      line[0] += 2;
      lir_text(3, line[0], "L=Tuner 1 50ohm (low impedence)");
      line[0]++;
      lir_text(3, line[0], "Z=High Z");
      line[0] += 2;
      lir_text(3, line[0], "Select antenna (L/Z) =>");
      for (;;) {
        await_processed_keyboard();
        if (kill_all_flag)
          return -1;
        switch (lir_inkey) {
          case 'L':
            rx_channel_params->rspDuoTunerParams.tuner1AmPortSel = sdrplay_api_RspDuo_AMPORT_2;
            line[0]++;
            lir_text(3, line[0], "Selected antenna is Tuner 1 50ohm");
            break;
          case 'Z':
            rx_channel_params->rspDuoTunerParams.tuner1AmPortSel = sdrplay_api_RspDuo_AMPORT_1;
            line[0]++;
            lir_text(3, line[0], "Selected antenna is High Z");
            break;
          default:
            continue;
        }
        break;
      }
    }
  } else if (device.hwVer == SDRPLAY3_RSPdx_ID) {
    line[0] += 2;
    lir_text(3, line[0], "A=Antenna A");
    line[0]++;
    lir_text(3, line[0], "B=Antenna B");
    line[0]++;
    lir_text(3, line[0], "C=Antenna C");
    line[0] += 2;
    lir_text(3, line[0], "Select antenna (A/B/C) =>");
    for (;;) {
      await_processed_keyboard();
      if (kill_all_flag)
        return -1;
      switch (lir_inkey) {
        case 'A':
          device_params->devParams->rspDxParams.antennaSel = sdrplay_api_RspDx_ANTENNA_A;
          line[0]++;
          lir_text(3, line[0], "Selected antenna is Antenna A");
          break;
        case 'B':
          device_params->devParams->rspDxParams.antennaSel = sdrplay_api_RspDx_ANTENNA_B;
          line[0]++;
          lir_text(3, line[0], "Selected antenna is Antenna B");
          break;
        case 'C':
          device_params->devParams->rspDxParams.antennaSel = sdrplay_api_RspDx_ANTENNA_C;
          line[0]++;
          lir_text(3, line[0], "Selected antenna is Antenna C");
          break;
        default:
          continue;
      }
      break;
    }
  }
  return 0;
}

static int sdrplay3_select_rspdx_hdr_mode(int *line)
{
  char s[120];
  int hdr_bandwidth;

  line[0] += 2;
  lir_text(3, line[0], "Enable HDR mode? (Y/N) =>");
  for (;;) {
    await_processed_keyboard();
    if (kill_all_flag)
      return -1;
    if (lir_inkey == 'N') {
      device_params->devParams->rspDxParams.hdrEnable = 0;
      line[0]++;
      lir_text(3, line[0], "RSPdx HDR mode is disabled");
      break;
    } else if (lir_inkey == 'Y') {
      device_params->devParams->rspDxParams.hdrEnable = 1;
      line[0]++;
      lir_text(3, line[0], "RSPdx HDR mode is enabled");
      line[0] += 2;
      lir_text(3, line[0], "A=HDR bandwidth 200 kHz");
      line[0]++;
      lir_text(3, line[0], "B=HDR bandwidth 500 kHz");
      line[0]++;
      lir_text(3, line[0], "C=HDR bandwidth 1200 kHz");
      line[0]++;
      lir_text(3, line[0], "D=HDR bandwidth 1700 kHz");
      line[0] += 2;
      lir_text(3, line[0], "Select HDR bandwidth (A to D) =>");
      hdr_bandwidth = 0;
      for (;;) {
        await_processed_keyboard();
        if (kill_all_flag)
          return -1;
        switch (lir_inkey) {
          case 'A':
            rx_channel_params->rspDxTunerParams.hdrBw = sdrplay_api_RspDx_HDRMODE_BW_0_200;
            hdr_bandwidth = 200;
            break;
          case 'B':
            rx_channel_params->rspDxTunerParams.hdrBw = sdrplay_api_RspDx_HDRMODE_BW_0_500;
            hdr_bandwidth = 500;
            break;
          case 'C':
            rx_channel_params->rspDxTunerParams.hdrBw = sdrplay_api_RspDx_HDRMODE_BW_1_200;
            hdr_bandwidth = 1200;
            break;
          case 'D':
            rx_channel_params->rspDxTunerParams.hdrBw = sdrplay_api_RspDx_HDRMODE_BW_1_700;
            hdr_bandwidth = 1700;
            break;
          default:
            continue;
        }
        break;
      }
      line[0]++;
      snprintf(s, 120, "Selected HDR bandwidth is %d kHz", hdr_bandwidth);
      lir_text(3, line[0], s);
      break;
    }
  }
  return 0;
}

/*********************************************************************
 * RF LNA state tables                                               *
 * from SDRplay API Specification Guide (Gain Reduction Tables)      *
 *********************************************************************/
/* RSP1 */
static int num_lna_states_rsp1_0_420 = 4;
static uint8_t lna_states_rsp1_0_420[] = { 0, 24, 19, 43 };
static int num_lna_states_rsp1_420_1000 = 4;
static uint8_t lna_states_rsp1_420_1000[] = { 0, 7, 19, 26 };
static int num_lna_states_rsp1_1000_2000 = 4;
static uint8_t lna_states_rsp1_1000_2000[] = { 0, 5, 19, 24 };
//static double freq_rsp1_1000_2000 = 1200e6;

/* RSP1A */
static int num_lna_states_rsp1a_0_60 = 7;
static uint8_t lna_states_rsp1a_0_60[] = { 0, 6, 12, 18, 37, 42, 61 };
static int num_lna_states_rsp1a_60_420 = 10;
static uint8_t lna_states_rsp1a_60_420[] = { 0, 6, 12, 18, 20, 26, 32, 38, 57, 62 };
static int num_lna_states_rsp1a_420_1000 = 10;
static uint8_t lna_states_rsp1a_420_1000[] = { 0, 7, 13, 19, 20, 27, 33, 39, 45, 64 };
static int num_lna_states_rsp1a_1000_2000 = 9;
static uint8_t lna_states_rsp1a_1000_2000[] = { 0, 6, 12, 20, 26, 32, 38, 43, 62 };

/* RSP2 */
static int num_lna_states_rsp2_0_420 = 9;
static uint8_t lna_states_rsp2_0_420[] = { 0, 10, 15, 21, 24, 34, 39, 45, 64 };
static int num_lna_states_rsp2_420_1000 = 6;
static uint8_t lna_states_rsp2_420_1000[] = { 0, 7, 10, 17, 22, 41 };
static int num_lna_states_rsp2_1000_2000 = 6;
static uint8_t lna_states_rsp2_1000_2000[] = { 0, 5, 21, 15, 15, 34 };
static int num_lna_states_rsp2_0_60_hiz = 5;
static uint8_t lna_states_rsp2_0_60_hiz[] = { 0, 6, 12, 18, 37 };

/* RSPduo */
static int num_lna_states_rspduo_0_60 = 7;
static uint8_t lna_states_rspduo_0_60[] = { 0, 6, 12, 18, 37, 42, 61 };
static int num_lna_states_rspduo_60_420 = 10;
static uint8_t lna_states_rspduo_60_420[] = { 0, 6, 12, 18, 20, 26, 32, 38, 57, 62 };
static int num_lna_states_rspduo_420_1000 = 10;
static uint8_t lna_states_rspduo_420_1000[] = { 0, 7, 13, 19, 20, 27, 33, 39, 45, 64 };
static int num_lna_states_rspduo_1000_2000 = 9;
static uint8_t lna_states_rspduo_1000_2000[] = { 0, 6, 12, 20, 26, 32, 38, 43, 62 };
static int num_lna_states_rspduo_0_60_hiz = 5;
static uint8_t lna_states_rspduo_0_60_hiz[] = { 0, 6, 12, 18, 37 };

/* RSPdx */
static int num_lna_states_rspdx_0_2_hdr = 22;
static uint8_t lna_states_rspdx_0_2_hdr[] = { 0, 3, 6, 9, 12, 15, 18, 21, 24, 25, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57, 60 };
static int num_lna_states_rspdx_0_12 = 19;
static uint8_t lna_states_rspdx_0_12[] = { 0, 3, 6, 9, 12, 15, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57, 60 };
#if SDRPLAY3PAR_VERNR == 307
static int num_lna_states_rspdx_12_60 = 20;
static uint8_t lna_states_rspdx_12_60[] = { 0, 3, 6, 9, 12, 15, 18, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57, 60 };
#elif SDRPLAY3PAR_VERNR >= 309
static int num_lna_states_rspdx_12_50 = 20;
static uint8_t lna_states_rspdx_12_50[] = { 0, 3, 6, 9, 12, 15, 18, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57, 60 };
static int num_lna_states_rspdx_50_60 = 25;
static uint8_t lna_states_rspdx_50_60[] = { 0, 3, 6, 9, 12, 20, 23, 26, 29, 32, 35, 38, 44, 47, 50, 53, 56, 59, 62, 65, 68, 71, 74, 77, 80 };
#endif
static int num_lna_states_rspdx_60_250 = 27;
static uint8_t lna_states_rspdx_60_250[] = { 0, 3, 6, 9, 12, 15, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57, 60, 63, 66, 69, 72, 75, 78, 81, 84 };
static int num_lna_states_rspdx_250_420 = 28;
static uint8_t lna_states_rspdx_250_420[] = { 0, 3, 6, 9, 12, 15, 18, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57, 60, 63, 66, 69, 72, 75, 78, 81, 84 };
static int num_lna_states_rspdx_420_1000 = 21;
static uint8_t lna_states_rspdx_420_1000[] = { 0, 7, 10, 13, 16, 19, 22, 25, 31, 34, 37, 40, 43, 46, 49, 52, 55, 58, 61, 64, 67 };
static int num_lna_states_rspdx_1000_2000 = 19;
static uint8_t lna_states_rspdx_1000_2000[] = { 0, 5, 8, 11, 14, 17, 20, 32, 35, 38, 41, 44, 47, 50, 53, 56, 59, 62, 65 };

static int sdrplay3_select_rf_gain_band_rsp1(int *line)
{
  double freq;

  line[0] += 2;
  lir_text(3, line[0], "A=RF gain reduction band 0-420 MHz");
  line[0]++;
  lir_text(3, line[0], "B=RF gain reduction band 420-1000 MHz");
  line[0]++;
  lir_text(3, line[0], "C=RF gain reduction band 1000-2000 MHz");
  line[0] += 2;
  lir_text(3, line[0], "Select RF gain reduction band (A-C) =>");
  freq = -1;
  for (;;) {
    await_processed_keyboard();
    if (kill_all_flag)
      return -1;
    switch (lir_inkey) {
      case 'A':
        freq = 200e6;
        break;
      case 'B':
        freq = 600e6;
        break;
      case 'C':
        freq = 1200e6;
        break;
      default:
        continue;
    }
    break;
  }
  return freq;
}

static double sdrplay3_select_rf_gain_band_rsp1a(int *line)
{
  double freq;

  line[0] += 2;
  lir_text(3, line[0], "A=RF gain reduction band 0-60 MHz");
  line[0]++;
  lir_text(3, line[0], "B=RF gain reduction band 60-420 MHz");
  line[0]++;
  lir_text(3, line[0], "C=RF gain reduction band 420-1000 MHz");
  line[0]++;
  lir_text(3, line[0], "D=RF gain reduction band 1000-2000 MHz");
  line[0] += 2;
  lir_text(3, line[0], "Select RF gain reduction band (A-D) =>");
  freq = -1;
  for (;;) {
    await_processed_keyboard();
    if (kill_all_flag)
      return -1;
    switch (lir_inkey) {
      case 'A':
        freq = 30e6;
        break;
      case 'B':
        freq = 200e6;
        break;
      case 'C':
        freq = 600e6;
        break;
      case 'D':
        freq = 1200e6;
        break;
      default:
        continue;
    }
    break;
  }
  return freq;
}

static double sdrplay3_select_rf_gain_band_rsp2(int *line)
{
  double freq;

  if (rx_channel_params->rsp2TunerParams.antennaSel == sdrplay_api_Rsp2_ANTENNA_A &&
      rx_channel_params->rsp2TunerParams.amPortSel == sdrplay_api_Rsp2_AMPORT_1) {
    freq = 30e6;
  } else {
    line[0] += 2;
    lir_text(3, line[0], "A=RF gain reduction band 0-420 MHz");
    line[0]++;
    lir_text(3, line[0], "B=RF gain reduction band 420-1000 MHz");
    line[0]++;
    lir_text(3, line[0], "C=RF gain reduction band 1000-2000 MHz");
    line[0] += 2;
    lir_text(3, line[0], "Select RF gain reduction band (A-C) =>");
    freq = -1;
    for (;;) {
      await_processed_keyboard();
      if (kill_all_flag)
        return -1;
      switch (lir_inkey) {
        case 'A':
          freq = 200e6;
          break;
        case 'B':
          freq = 600e6;
          break;
        case 'C':
          freq = 1200e6;
          break;
        default:
          continue;
      }
      break;
    }
  }
  return freq;
}

static double sdrplay3_select_rf_gain_band_rspduo(int *line)
{
  double freq;

  if (rx_channel_params == device_params->rxChannelA &&
      rx_channel_params->rspDuoTunerParams.tuner1AmPortSel == sdrplay_api_RspDuo_AMPORT_1) {
    freq = 30e6;
  } else {
    line[0] += 2;
    lir_text(3, line[0], "A=RF gain reduction band 0-60 MHz");
    line[0]++;
    lir_text(3, line[0], "B=RF gain reduction band 60-420 MHz");
    line[0]++;
    lir_text(3, line[0], "C=RF gain reduction band 420-1000 MHz");
    line[0]++;
    lir_text(3, line[0], "D=RF gain reduction band 1000-2000 MHz");
    line[0] += 2;
    lir_text(3, line[0], "Select RF gain reduction band (A-D) =>");
    freq = -1;
    for (;;) {
      await_processed_keyboard();
      if (kill_all_flag)
        return -1;
      switch (lir_inkey) {
        case 'A':
          freq = 30e6;
          break;
        case 'B':
          freq = 200e6;
          break;
        case 'C':
          freq = 600e6;
          break;
        case 'D':
          freq = 1200e6;
          break;
        default:
          continue;
      }
      break;
    }
  }
  return freq;
}

static double sdrplay3_select_rf_gain_band_rspdx(int *line)
{
  double freq;

  if (device_params->devParams->rspDxParams.hdrEnable) {
    freq = 1e6;
  } else {
    line[0] += 2;
    lir_text(3, line[0], "A=RF gain reduction band 0-12 MHz");
    line[0]++;
#if SDRPLAY3PAR_VERNR == 307
    lir_text(3, line[0], "B=RF gain reduction band 12-60 MHz");
    line[0]++;
    lir_text(3, line[0], "C=RF gain reduction band 60-250 MHz");
    line[0]++;
    lir_text(3, line[0], "D=RF gain reduction band 250-420 MHz");
    line[0]++;
    lir_text(3, line[0], "E=RF gain reduction band 420-1000 MHz");
    line[0]++;
    lir_text(3, line[0], "F=RF gain reduction band 1000-2000 MHz");
    line[0] += 2;
    lir_text(3, line[0], "Select RF gain reduction band (A-F) =>");
#elif SDRPLAY3PAR_VERNR >= 309
    lir_text(3, line[0], "B=RF gain reduction band 12-50 MHz");
    line[0]++;
    lir_text(3, line[0], "C=RF gain reduction band 50-60 MHz");
    line[0]++;
    lir_text(3, line[0], "D=RF gain reduction band 60-250 MHz");
    line[0]++;
    lir_text(3, line[0], "E=RF gain reduction band 250-420 MHz");
    line[0]++;
    lir_text(3, line[0], "F=RF gain reduction band 420-1000 MHz");
    line[0]++;
    lir_text(3, line[0], "G=RF gain reduction band 1000-2000 MHz");
    line[0] += 2;
    lir_text(3, line[0], "Select RF gain reduction band (A-G) =>");
#endif
    freq = -1;
    for (;;) {
      await_processed_keyboard();
      if (kill_all_flag)
        return -1;
      switch (lir_inkey) {
        case 'A':
          freq = 10e6;
          break;
        case 'B':
          freq = 30e6;
          break;
#if SDRPLAY3PAR_VERNR == 307
        case 'C':
          freq = 200e6;
          break;
        case 'D':
          freq = 350e6;
          break;
        case 'E':
          freq = 600e6;
          break;
        case 'F':
          freq = 1200e6;
          break;
#elif SDRPLAY3PAR_VERNR >= 309
        case 'C':
          freq = 55e6;
          break;
        case 'D':
          freq = 200e6;
          break;
        case 'E':
          freq = 350e6;
          break;
        case 'F':
          freq = 600e6;
          break;
        case 'G':
          freq = 1200e6;
          break;
#endif
        default:
          continue;
      }
      break;
    }
  }
  return freq;
}

static uint8_t *sdrplay3_find_lna_states(double freq, int *p_num_lna_states)
{
  if (device.hwVer == SDRPLAY3_RSP1_ID) {
    if (freq < 420e6) {
      *p_num_lna_states = num_lna_states_rsp1_0_420;
      return lna_states_rsp1_0_420;
    } else if (freq < 1000e6) {
      *p_num_lna_states = num_lna_states_rsp1_420_1000;
      return lna_states_rsp1_420_1000;
    } else {
      *p_num_lna_states = num_lna_states_rsp1_1000_2000;
      return lna_states_rsp1_1000_2000;
    }
  } else if (device.hwVer == SDRPLAY3_RSP1A_ID) {
    if (freq < 60e6) {
      *p_num_lna_states = num_lna_states_rsp1a_0_60;
      return lna_states_rsp1a_0_60;
    } else if (freq < 420e6) {
      *p_num_lna_states = num_lna_states_rsp1a_60_420;
      return lna_states_rsp1a_60_420;
    } else if (freq < 1000e6) {
      *p_num_lna_states = num_lna_states_rsp1a_420_1000;
      return lna_states_rsp1a_420_1000;
    } else {
      *p_num_lna_states = num_lna_states_rsp1a_1000_2000;
      return lna_states_rsp1a_1000_2000;
    }
  } else if (device.hwVer == SDRPLAY3_RSP2_ID) {
    if (freq < 60e6 &&
        rx_channel_params->rsp2TunerParams.antennaSel == sdrplay_api_Rsp2_ANTENNA_A &&
        rx_channel_params->rsp2TunerParams.amPortSel == sdrplay_api_Rsp2_AMPORT_1) {
      *p_num_lna_states = num_lna_states_rsp2_0_60_hiz;
      return lna_states_rsp2_0_60_hiz;
    } else if (freq < 420e6) {
      *p_num_lna_states = num_lna_states_rsp2_0_420;
      return lna_states_rsp2_0_420;
    } else if (freq < 1000e6) {
      *p_num_lna_states = num_lna_states_rsp2_420_1000;
      return lna_states_rsp2_420_1000;
    } else {
      *p_num_lna_states = num_lna_states_rsp2_1000_2000;
      return lna_states_rsp2_1000_2000;
    }
  } else if (device.hwVer == SDRPLAY3_RSPduo_ID) {
    if (freq < 60e6 &&
        rx_channel_params == device_params->rxChannelA &&
        rx_channel_params->rspDuoTunerParams.tuner1AmPortSel == sdrplay_api_RspDuo_AMPORT_1) {
      *p_num_lna_states = num_lna_states_rspduo_0_60_hiz;
      return lna_states_rspduo_0_60_hiz;
    } else if (freq < 60e6) {
      *p_num_lna_states = num_lna_states_rspduo_0_60;
      return lna_states_rspduo_0_60;
    } else if (freq < 420e6) {
      *p_num_lna_states = num_lna_states_rspduo_60_420;
      return lna_states_rspduo_60_420;
    } else if (freq < 1000e6) {
      *p_num_lna_states = num_lna_states_rspduo_420_1000;
      return lna_states_rspduo_420_1000;
    } else {
      *p_num_lna_states = num_lna_states_rspduo_1000_2000;
      return lna_states_rspduo_1000_2000;
    }
  } else if (device.hwVer == SDRPLAY3_RSPdx_ID) {
    if (freq < 2e6 &&
        device_params->devParams->rspDxParams.hdrEnable) {
      *p_num_lna_states = num_lna_states_rspdx_0_2_hdr;
      return lna_states_rspdx_0_2_hdr;
    } else if (freq < 12e6) {
      *p_num_lna_states = num_lna_states_rspdx_0_12;
      return lna_states_rspdx_0_12;
#if SDRPLAY3PAR_VERNR == 307
    } else if (freq < 60e6) {
      *p_num_lna_states = num_lna_states_rspdx_12_60;
      return lna_states_rspdx_12_60;
#elif SDRPLAY3PAR_VERNR >= 309
    } else if (freq < 50e6) {
      *p_num_lna_states = num_lna_states_rspdx_12_50;
      return lna_states_rspdx_12_50;
    } else if (freq < 60e6) {
      *p_num_lna_states = num_lna_states_rspdx_50_60;
      return lna_states_rspdx_50_60;
#endif
    } else if (freq < 250e6) {
      *p_num_lna_states = num_lna_states_rspdx_60_250;
      return lna_states_rspdx_60_250;
    } else if (freq < 420e6) {
      *p_num_lna_states = num_lna_states_rspdx_250_420;
      return lna_states_rspdx_250_420;
    } else if (freq < 1000e6) {
      *p_num_lna_states = num_lna_states_rspdx_420_1000;
      return lna_states_rspdx_420_1000;
    } else {
      *p_num_lna_states = num_lna_states_rspdx_1000_2000;
      return lna_states_rspdx_1000_2000;
    }
  }
  // unknown RSP model; return NULL
  *p_num_lna_states = 0;
  return NULL;
}

static int sdrplay3_rf_att_to_lna_state(int gain_reduction, int num_lna_states, uint8_t *lna_states)
{
  int i;
  int lna_state;
  int delta;
  int min_delta;

  // find the closest LNA state
  lna_state = -1;
  min_delta = 1000;
  for (i = 0; i < num_lna_states; i++) {
    delta = abs(lna_states[i] - gain_reduction);
    if (delta < min_delta) {
      lna_state = i;
      min_delta = delta;
    }
  }
  return lna_state;
}

static void sdrplay3_select_lna_state(int num_lna_states, uint8_t *lna_states, double freq, int *line)
{
  char s[120];
  int gain_reduction;
  int lna_state;

  line[0] += 2;
  snprintf(s, 120, "Select RF gain reduction in dB (%d-%d) =>", lna_states[0], lna_states[num_lna_states-1]);
  lir_text(3, line[0], s);
  gain_reduction = lir_get_integer(44, line[0], 2, lna_states[0], lna_states[num_lna_states-1]);
  if (kill_all_flag)
    return;
  lna_state = sdrplay3_rf_att_to_lna_state(gain_reduction, num_lna_states, lna_states);
  // we need to set the frequency to same value within the band, otherwise
  // the selected LNA state might be invalid
  rx_channel_params->tunerParams.rfFreq.rfHz = freq;
  rx_channel_params->tunerParams.gain.LNAstate = lna_state;
  // also store the maximum allowed value for LNA state (to be used in wse_sdrxx.c)
  sdrplay3_max_gain = lna_states[num_lna_states - 1];
  return;
}

static int sdrplay3_select_rf_gain(int *line)
{
  char s[120];
  double freq;
  int num_lna_states;
  uint8_t *lna_states;
  int lna_state_max;
  int lna_state;

  line[0] += 2;
  lir_text(3, line[0], "D=RF gain reduction in dB");
  line[0]++;
  lir_text(3, line[0], "L=RF gain reduction as LNA state");
  line[0] += 2;
  lir_text(3, line[0], "Select RF gain reduction mode (D/L) =>");
  freq = 0;
  for (;;) {
    await_processed_keyboard();
    if (kill_all_flag)
      return -1;
    if (lir_inkey == 'D') {
      if (device.hwVer == SDRPLAY3_RSP1_ID) {
        freq = sdrplay3_select_rf_gain_band_rsp1(line);
      } else if (device.hwVer == SDRPLAY3_RSP1A_ID) {
        freq = sdrplay3_select_rf_gain_band_rsp1a(line);
      } else if (device.hwVer == SDRPLAY3_RSP2_ID) {
        freq = sdrplay3_select_rf_gain_band_rsp2(line);
      } else if (device.hwVer == SDRPLAY3_RSPduo_ID) {
        freq = sdrplay3_select_rf_gain_band_rspduo(line);
      } else if (device.hwVer == SDRPLAY3_RSPdx_ID) {
        freq = sdrplay3_select_rf_gain_band_rspdx(line);
      }
      if (kill_all_flag)
        return -1;
      lna_states = sdrplay3_find_lna_states(freq, &num_lna_states);
      sdrplay3_select_lna_state(num_lna_states, lna_states, freq, line);
      if (kill_all_flag)
        return -1;
      line[0]++;
      snprintf(s, 120, "Selected LNA state is %d", rx_channel_params->tunerParams.gain.LNAstate);
      lir_text(3, line[0], s);
      break;
    } else if (lir_inkey == 'L') {
      lna_state_max = 0;
      if (device.hwVer == SDRPLAY3_RSP1_ID) {
        lna_state_max = 3;
        sdrplay3_max_gain = 43;
      } else if (device.hwVer == SDRPLAY3_RSP1A_ID) {
        lna_state_max = 9;
        sdrplay3_max_gain = 64;
      } else if (device.hwVer == SDRPLAY3_RSP2_ID) {
        lna_state_max = 8;
        sdrplay3_max_gain = 64;
      } else if (device.hwVer == SDRPLAY3_RSPduo_ID) {
        lna_state_max = 9;
        sdrplay3_max_gain = 64;
      } else if (device.hwVer == SDRPLAY3_RSPdx_ID) {
        lna_state_max = 27;
        sdrplay3_max_gain = 84;
      }
      line[0] += 2;
      snprintf(s, 120, "Select LNA state (0-%d) =>", lna_state_max);
      lir_text(3, line[0], s);
      lna_state = lir_get_integer(29, line[0], 2, 0, lna_state_max);
      if (kill_all_flag)
        return -1;
      // no checks here - assume the user knows what they are doing with LNA state
      rx_channel_params->tunerParams.gain.LNAstate = lna_state;
      break;
    }
  }
  return 0;
}

static int sdrplay3_select_if_gain(int *line)
{
  char s[120];
  int gain_reduction;
  int setPoint_dBfs;
  int attack_ms;
  int decay_ms;
  int decay_delay_ms;
  int decay_threshold_dB;
  char *agc_mode_name = 0;

  line[0] += 2;
  lir_text(3, line[0], "Enable AGC? (Y/N) =>");
  for (;;) {
    await_processed_keyboard();
    if (kill_all_flag)
      return -1;
    if (lir_inkey == 'N') {
      rx_channel_params->ctrlParams.agc.enable = sdrplay_api_AGC_DISABLE;
      line[0] += 2;
      snprintf(s, 120, "Select IF gain reduction in dB (%d-%d) =>", SDRPLAY3_NORMAL_MIN_GR, SDRPLAY3_MAX_BB_GR);
      lir_text(3, line[0], s);
      gain_reduction = lir_get_integer(45, line[0], 2, SDRPLAY3_NORMAL_MIN_GR, SDRPLAY3_MAX_BB_GR);
      if (kill_all_flag)
        return -1;
      rx_channel_params->tunerParams.gain.gRdB = gain_reduction;
      line[0]++;
      snprintf(s, 120, "Selected IF gain reduction is %d dB (AGC is disabled)", rx_channel_params->tunerParams.gain.gRdB);
      lir_text(3, line[0], s);
      break;
    } else if (lir_inkey == 'Y') {
      line[0] += 2;
      snprintf(s, 120, "A=%s", SDRPLAY3_AGC_5HZ_NAME);
      lir_text(3, line[0], s);
      line[0]++;
      snprintf(s, 120, "B=%s", SDRPLAY3_AGC_50HZ_NAME);
      lir_text(3, line[0], s);
      line[0]++;
      snprintf(s, 120, "C=%s", SDRPLAY3_AGC_100HZ_NAME);
      lir_text(3, line[0], s);
      line[0]++;
      snprintf(s, 120, "D=%s", SDRPLAY3_AGC_CTRL_EN_NAME);
      lir_text(3, line[0], s);
      line[0] += 2;
      lir_text(3, line[0], "Select AGC parameters (A to D) =>");
      for (;;) {
        await_processed_keyboard();
        if (kill_all_flag)
          return -1;
        switch (lir_inkey) {
          case 'A':
            rx_channel_params->ctrlParams.agc.enable = sdrplay_api_AGC_5HZ;
            break;
          case 'B':
            rx_channel_params->ctrlParams.agc.enable = sdrplay_api_AGC_50HZ;
            break;
          case 'C':
            rx_channel_params->ctrlParams.agc.enable = sdrplay_api_AGC_100HZ;
            break;
          case 'D':
            rx_channel_params->ctrlParams.agc.enable = sdrplay_api_AGC_CTRL_EN;
            break;
          default:
            continue;
        }
        break;
      }
      if (rx_channel_params->ctrlParams.agc.enable == sdrplay_api_AGC_100HZ) {
        agc_mode_name = SDRPLAY3_AGC_100HZ_NAME;
      } else if (rx_channel_params->ctrlParams.agc.enable == sdrplay_api_AGC_50HZ) {
        agc_mode_name = SDRPLAY3_AGC_50HZ_NAME;
      } else if (rx_channel_params->ctrlParams.agc.enable == sdrplay_api_AGC_5HZ) {
        agc_mode_name = SDRPLAY3_AGC_5HZ_NAME;
      } else if (rx_channel_params->ctrlParams.agc.enable == sdrplay_api_AGC_CTRL_EN) {
        agc_mode_name = SDRPLAY3_AGC_CTRL_EN_NAME;
      }
      line[0]++;
      snprintf(s, 120, "AGC is enabled - %s", agc_mode_name);
      lir_text(3, line[0], s);

      if (rx_channel_params->ctrlParams.agc.enable == sdrplay_api_AGC_CTRL_EN) {
        line[0] += 2;
        lir_text(3, line[0], "Select AGC set point in dBFS (-100 to 0) =>");
        setPoint_dBfs = lir_get_integer(47, line[0], 4, -100, 0);
        if (kill_all_flag)
          return -1;
        rx_channel_params->ctrlParams.agc.setPoint_dBfs = setPoint_dBfs;
        line[0]++;
        lir_text(3, line[0], "Select AGC attack time in ms (0 to 1000) =>");
        attack_ms = lir_get_integer(47, line[0], 4, 0, 1000);
        if (kill_all_flag)
          return -1;
        rx_channel_params->ctrlParams.agc.attack_ms = attack_ms;
        line[0]++;
        lir_text(3, line[0], "Select AGC decay time in ms (0 to 1000) =>");
        decay_ms = lir_get_integer(46, line[0], 4, 0, 1000);
        if (kill_all_flag)
          return -1;
        rx_channel_params->ctrlParams.agc.decay_ms = decay_ms;
        line[0]++;
        lir_text(3, line[0], "Select AGC decay delay in ms (0 to 1000) =>");
        decay_delay_ms = lir_get_integer(47, line[0], 4, 0, 1000);
        if (kill_all_flag)
          return -1;
        rx_channel_params->ctrlParams.agc.decay_delay_ms = decay_delay_ms;
        line[0]++;
        lir_text(3, line[0], "Select AGC decay threshold in dB (0 to 100) =>");
        decay_threshold_dB = lir_get_integer(50, line[0], 3, 0, 100);
        if (kill_all_flag)
          return -1;
        rx_channel_params->ctrlParams.agc.decay_threshold_dB = decay_threshold_dB;
      }
      break;
    }
  }
  return 0;
}

static int sdrplay3_select_frequency_correction(int *line)
{
  float ppm;

  if (!device_params->devParams)
    return 0;

  line[0] += 2;
  lir_text(3, line[0], "Select frequency correction in ppm =>");
  ppm = lir_get_float(41, line[0], 9, -300.F, +300.F);
  if (kill_all_flag)
    return -1;
  device_params->devParams->ppm = ppm;
  return 0;
}

static int sdrplay3_select_dc_offset_correction(int *line)
{
  line[0] += 2;
  lir_text(3, line[0], "Enable DC offset correction? (Y/N) =>");
  for (;;) {
    await_processed_keyboard();
    if (kill_all_flag)
      return -1;
    if (lir_inkey == 'N') {
      rx_channel_params->ctrlParams.dcOffset.DCenable = 0;
      line[0]++;
      lir_text(3, line[0], "DC offset correction is disabled");
      break;
    } else if (lir_inkey == 'Y') {
      rx_channel_params->ctrlParams.dcOffset.DCenable = 1;
      line[0]++;
      lir_text(3, line[0], "DC offset correction is enabled");
      break;
    }
  }
  return 0;
}

static int sdrplay3_select_iq_imbalance_correction(int *line)
{
  line[0] += 2;
  lir_text(3, line[0], "Enable IQ imbalance correction? (Y/N) =>");
  for (;;) {
    await_processed_keyboard();
    if (kill_all_flag)
      return -1;
    if (lir_inkey == 'N') {
      rx_channel_params->ctrlParams.dcOffset.IQenable = 0;
      line[0]++;
      lir_text(3, line[0], "IQ imbalance correction is disabled");
      break;
    } else if (lir_inkey == 'Y') {
      rx_channel_params->ctrlParams.dcOffset.DCenable = 1;
      rx_channel_params->ctrlParams.dcOffset.IQenable = 1;
      line[0]++;
      lir_text(3, line[0], "IQ imbalance correction is enabled");
      break;
    }
  }
  return 0;
}

static int sdrplay3_select_usb_transfer_mode(int *line)
{
  if (!device_params->devParams)
    return 0;

  line[0] += 2;
  lir_text(3, line[0], "Select USB transfer mode ([I]sochronous/[B]ulk): ");
  for (;;) {
    await_processed_keyboard();
    if (kill_all_flag)
      return -1;
    if (lir_inkey == 'I') {
      device_params->devParams->mode = sdrplay_api_ISOCH;
      line[0]++;
      lir_text(3, line[0], "Selected USB transfer mode Isochronous");
      break;
    } else if (lir_inkey == 'B') {
      device_params->devParams->mode = sdrplay_api_BULK;
      line[0]++;
      lir_text(3, line[0], "Selected USB transfer mode Bulk");
      break;
    }
  }
  return 0;
}

static int sdrplay3_select_notch_filters(int *line)
{
  uint8_t rf_notch;
  uint8_t dab_notch;

  if (device.hwVer == SDRPLAY3_RSP1A_ID || device.hwVer == SDRPLAY3_RSP2_ID ||
      device.hwVer == SDRPLAY3_RSPduo_ID || device.hwVer == SDRPLAY3_RSPdx_ID) {
    line[0] += 2;
    lir_text(3, line[0], "Enable RF notch filter? (Y/N) =>");
    for (;;) {
      await_processed_keyboard();
      if (kill_all_flag)
        return -1;
      if (lir_inkey == 'N') {
        rf_notch = 0;
        line[0]++;
        lir_text(3, line[0], "RF notch filter is disabled");
        break;
      } else if (lir_inkey == 'Y') {
        rf_notch = 1;
        line[0]++;
        lir_text(3, line[0], "RF notch filter is enabled");
        break;
      }
    }
    if (device.hwVer == SDRPLAY3_RSP1A_ID) {
      device_params->devParams->rsp1aParams.rfNotchEnable = rf_notch;
    } else if (device.hwVer == SDRPLAY3_RSP2_ID) {
      rx_channel_params->rsp2TunerParams.rfNotchEnable = rf_notch;
    } else if (device.hwVer == SDRPLAY3_RSPduo_ID) {
      rx_channel_params->rspDuoTunerParams.rfNotchEnable = rf_notch;
    } else if (device.hwVer == SDRPLAY3_RSPdx_ID) {
      device_params->devParams->rspDxParams.rfNotchEnable = rf_notch;
    }
  }

  if (device.hwVer == SDRPLAY3_RSP1A_ID || device.hwVer == SDRPLAY3_RSPduo_ID ||
      device.hwVer == SDRPLAY3_RSPdx_ID) {
    line[0] += 2;
    lir_text(3, line[0], "Enable DAB notch filter? (Y/N) =>");
    for (;;) {
      await_processed_keyboard();
      if (kill_all_flag)
        return -1;
      if (lir_inkey == 'N') {
        dab_notch = 0;
        line[0]++;
        lir_text(3, line[0], "DAB notch filter is disabled");
        break;
      } else if (lir_inkey == 'Y') {
        dab_notch = 1;
        line[0]++;
        lir_text(3, line[0], "DAB notch filter is enabled");
        break;
      }
    }
    if (device.hwVer == SDRPLAY3_RSP1A_ID) {
      device_params->devParams->rsp1aParams.rfDabNotchEnable = dab_notch;
    } else if (device.hwVer == SDRPLAY3_RSPduo_ID) {
      rx_channel_params->rspDuoTunerParams.rfDabNotchEnable = dab_notch;
    } else if (device.hwVer == SDRPLAY3_RSPdx_ID) {
      device_params->devParams->rspDxParams.rfDabNotchEnable = dab_notch;
    }
  }

  if (device.hwVer == SDRPLAY3_RSPduo_ID && rx_channel_params == device_params->rxChannelA) {
    line[0] += 2;
    lir_text(3, line[0], "Enable AM notch filter? (Y/N) =>");
    for (;;) {
      await_processed_keyboard();
      if (kill_all_flag)
        return -1;
      if (lir_inkey == 'N') {
        rx_channel_params->rspDuoTunerParams.tuner1AmNotchEnable = 0;
        line[0]++;
        lir_text(3, line[0], "AM notch filter is disabled");
        break;
      } else if (lir_inkey == 'Y') {
        rx_channel_params->rspDuoTunerParams.tuner1AmNotchEnable = 1;
        line[0]++;
        lir_text(3, line[0], "AM notch filter is enabled");
        break;
      }
    }
  }
  return 0;
}

static int sdrplay3_select_bias_t(int *line)
{
  uint8_t biasT;

  if (device.hwVer == SDRPLAY3_RSP1A_ID || device.hwVer == SDRPLAY3_RSP2_ID ||
      (device.hwVer == SDRPLAY3_RSPduo_ID && rx_channel_params == device_params->rxChannelB) ||
      device.hwVer == SDRPLAY3_RSPdx_ID) {
    line[0] += 2;
    lir_text(3, line[0], "Enable Bias-T? (Y/N) =>");
    for (;;) {
      await_processed_keyboard();
      if (kill_all_flag)
        return -1;
      if (lir_inkey == 'N') {
        biasT = 0;
        line[0]++;
        lir_text(3, line[0], "Bias-T is disabled");
        break;
      } else if (lir_inkey == 'Y') {
        biasT = 1;
        line[0]++;
        lir_text(3, line[0], "Bias-T is enabled");
        break;
      }
    }
    if (device.hwVer == SDRPLAY3_RSP1A_ID) {
      rx_channel_params->rsp1aTunerParams.biasTEnable = biasT;
    } else if (device.hwVer == SDRPLAY3_RSP2_ID) {
      rx_channel_params->rsp2TunerParams.biasTEnable = biasT;
    } else if (device.hwVer == SDRPLAY3_RSPduo_ID) {
      rx_channel_params->rspDuoTunerParams.biasTEnable = biasT;
    } else if (device.hwVer == SDRPLAY3_RSPdx_ID) {
      device_params->devParams->rspDxParams.biasTEnable = biasT;
    }
  }
  return 0;
}

static int sdrplay3_update_rx2_setting(sdrplay_api_ReasonForUpdateT reason_for_update, int *line)
{
  char s[120];
  sdrplay_api_ErrT err;

  err = sdrplay_api_Update(device.dev, sdrplay_api_Tuner_B, reason_for_update, sdrplay_api_Update_Ext1_None);
  if (err != sdrplay_api_Success) {
    line[0]++;
    snprintf(s, 120, "sdrplay3: sdrplay_api_Update(%d) failed.", reason_for_update);
    lir_text(3, line[0], s);
    line[0] +=2;
    lir_text(5, line[0], press_any_key);
    await_keyboard();
    return -1;
  }
  return 0;
}

static int sdrplay3_select_rx2_bandwidth(int *line)
{
  int ret;

  rx_channel_params = device_params->rxChannelB;
  ret = sdrplay3_select_bandwidth(line);
  rx_channel_params = device_params->rxChannelA;
  if (ret != 0)
    return ret;
  ret = sdrplay3_update_rx2_setting(sdrplay_api_Update_Tuner_BwType, line);
  if (ret != 0)
    return ret;
  return 0;
}

static int sdrplay3_select_rx2_rf_gain(int *line)
{
  int ret;
  rx_channel_params = device_params->rxChannelB;
  ret = sdrplay3_select_rf_gain(line);
  rx_channel_params = device_params->rxChannelA;
  if (ret != 0)
    return ret;
  ret = sdrplay3_update_rx2_setting(sdrplay_api_Update_Tuner_Gr | sdrplay_api_Update_Tuner_Frf, line);
  if (ret != 0)
    return ret;
  return 0;
}

static int sdrplay3_select_rx2_if_gain(int *line)
{
  int ret;
  rx_channel_params = device_params->rxChannelB;
  ret = sdrplay3_select_if_gain(line);
  rx_channel_params = device_params->rxChannelA;
  if (ret != 0)
    return ret;
  ret = sdrplay3_update_rx2_setting(sdrplay_api_Update_Tuner_Gr | sdrplay_api_Update_Ctrl_Agc, line);
  if (ret != 0)
    return ret;
  return 0;
}

static int sdrplay3_select_rx2_dc_iq_correction(int *line)
{
  int ret;
  rx_channel_params = device_params->rxChannelB;
  ret = sdrplay3_select_dc_offset_correction(line);
  rx_channel_params = device_params->rxChannelA;
  if (ret != 0)
    return ret;

  rx_channel_params = device_params->rxChannelB;
  ret = sdrplay3_select_iq_imbalance_correction(line);
  rx_channel_params = device_params->rxChannelA;
  if (ret != 0)
    return ret;

  ret = sdrplay3_update_rx2_setting(sdrplay_api_Update_Ctrl_DCoffsetIQimbalance, line);
  if (ret != 0)
    return ret;
  return 0;
}

static int sdrplay3_select_rx2_notch_filters(int *line)
{
  int ret;
  rx_channel_params = device_params->rxChannelB;
  ret = sdrplay3_select_notch_filters(line);
  rx_channel_params = device_params->rxChannelA;
  if (ret != 0)
    return ret;
  ret = sdrplay3_update_rx2_setting(sdrplay_api_Update_RspDuo_RfNotchControl | sdrplay_api_Update_RspDuo_RfDabNotchControl, line);
  if (ret != 0)
    return ret;
  return 0;
}

static int sdrplay3_enable_rx2_bias_t(int *line)
{
  int ret;
  device_params->rxChannelB->rspDuoTunerParams.biasTEnable = 1;
  ret = sdrplay3_update_rx2_setting(sdrplay_api_Update_RspDuo_BiasTControl, line);
  if (ret != 0)
    return ret;
  return 0;
}

static int sdrplay3_select_rx2_settings(int *line)
{
  for (;;) {
    clear_screen();
    line[0] = 2;
    lir_text(3, line[0], "RSPduo Dual Tuner - Settings for RX #2");
    line[0] += 2;
    lir_text(3, line[0], "NOTE: select only the RX #2 settings that are different from RX #1");
    line[0] += 2;
    lir_text(3, line[0], "A=Select different bandwidth for RX #2");
    line[0]++;
    lir_text(3, line[0], "B=Select different RF gain for RX #2");
    line[0]++;
    lir_text(3, line[0], "C=Select different IF gain/AGC for RX #2");
    line[0]++;
    lir_text(3, line[0], "D=Select different DC offset/IQ imbalance correction for RX #2");
    line[0]++;
    lir_text(3, line[0], "E=Select different notch filters for RX #2");
    line[0]++;
    lir_text(3, line[0], "F=Enable Bias-T for RX #2");
    line[0]++;
    lir_text(3, line[0], "X=Exit to previous menu");
    line[0] += 2;
    lir_text(3, line[0], "Select setting to change for RX #2 (A to F or X) =>");
    for (;;) {
      await_processed_keyboard();
      if (kill_all_flag)
        return -1;
      switch (lir_inkey) {
        case 'A':
          if (sdrplay3_select_rx2_bandwidth(line) != 0)
            return -1;
          break;
        case 'B':
          if (sdrplay3_select_rx2_rf_gain(line) != 0)
            return -1;
          break;
        case 'C':
          if (sdrplay3_select_rx2_if_gain(line) != 0)
            return -1;
          break;
        case 'D':
          if (sdrplay3_select_rx2_dc_iq_correction(line) != 0)
            return -1;
          break;
        case 'E':
          if (sdrplay3_select_rx2_notch_filters(line) != 0)
            return -1;
          break;
        case 'F':
          if (sdrplay3_enable_rx2_bias_t(line) != 0)
            return -1;
          break;
        case 'X':
          break;
        default:
          continue;
      }
      break;
    }
    if (lir_inkey == 'X')
      break;
  }
  rx_channel_params = device_params->rxChannelA;
  return 0;
}

static void sdrplay3_export_rx2_params(void)
{
  sdrplay_api_RxChannelParamsT *rx2_channel_params = device_params->rxChannelB;

  // fill SDRplay3 RX #2 parameters
  sdrplay3_parms.if_bandwidth += RX2_OFFSET * rx2_channel_params->tunerParams.bwType;
  sdrplay3_parms.rf_lna_state += RX2_OFFSET * rx2_channel_params->tunerParams.gain.LNAstate;
  sdrplay3_parms.rf_lna_state_freq += RX2_OFFSET * (rx2_channel_params->tunerParams.rfFreq.rfHz / 1e6);
  sdrplay3_parms.rf_lna_state_max += RX2_OFFSET * sdrplay3_max_gain;
  sdrplay3_parms.if_agc += RX2_OFFSET * rx2_channel_params->ctrlParams.agc.enable;
  sdrplay3_parms.if_gain_reduction += RX2_OFFSET * rx2_channel_params->tunerParams.gain.gRdB;
  sdrplay3_parms.if_agc_setpoint_dbfs += RX2_OFFSET * -(rx2_channel_params->ctrlParams.agc.setPoint_dBfs);
  sdrplay3_parms.if_agc_attack_ms += RX2_OFFSET * rx2_channel_params->ctrlParams.agc.attack_ms;
  sdrplay3_parms.if_agc_decay_ms += RX2_OFFSET * rx2_channel_params->ctrlParams.agc.decay_ms;
  sdrplay3_parms.if_agc_decay_delay_ms += RX2_OFFSET * rx2_channel_params->ctrlParams.agc.decay_delay_ms;
  sdrplay3_parms.if_agc_decay_threshold_db += RX2_OFFSET * rx2_channel_params->ctrlParams.agc.decay_threshold_dB;
  sdrplay3_parms.dc_offset_corr += RX2_OFFSET * rx2_channel_params->ctrlParams.dcOffset.DCenable;
  sdrplay3_parms.iq_imbalance_corr += RX2_OFFSET * rx2_channel_params->ctrlParams.dcOffset.IQenable;
  sdrplay3_parms.notch_filters += RX2_OFFSET * 100 * (rx2_channel_params->rspDuoTunerParams.rfNotchEnable +
              100 * rx2_channel_params->rspDuoTunerParams.rfDabNotchEnable);
  sdrplay3_parms.bias_t += RX2_OFFSET * rx2_channel_params->rspDuoTunerParams.biasTEnable;
}

static int sdrplay3_save_configuration(char *filename)
{
  FILE *sdrplay3_file;
  char s[120];
  int *ise = (int *)(void *)s;
  int *sdr_pi;
  int i;

  // fill SDRplay3 parameter structure first
  memset(s, 0, sizeof(s));
  snprintf(s, 16, "%.15s", device.SerNo);
  sdrplay3_parms.sernum1 = ise[0];
  sdrplay3_parms.sernum2 = ise[1];
  sdrplay3_parms.sernum3 = ise[2];
  sdrplay3_parms.sernum4 = ise[3];
  sdrplay3_parms.hw_version = device.hwVer;
  sdrplay3_parms.rspduo_mode = device.rspDuoMode;
  sdrplay3_parms.tuner = device.tuner;
  sdrplay3_parms.rspduo_sample_freq = device.rspDuoSampleFreq;
  if (device_params->devParams)
    sdrplay3_parms.sample_rate = device_params->devParams->fsFreq.fsHz;
  else
    sdrplay3_parms.sample_rate = -1;
  sdrplay3_parms.decimation = rx_channel_params->ctrlParams.decimation.enable ?
              rx_channel_params->ctrlParams.decimation.decimationFactor : 1;
  sdrplay3_parms.if_freq = rx_channel_params->tunerParams.ifType;
  sdrplay3_parms.if_bandwidth = rx_channel_params->tunerParams.bwType;
  if (device.hwVer == SDRPLAY3_RSP2_ID) {
    sdrplay3_parms.antenna = rx_channel_params->rsp2TunerParams.antennaSel +
              100 * rx_channel_params->rsp2TunerParams.amPortSel;
  } else if (device.hwVer == SDRPLAY3_RSPduo_ID) {
    sdrplay3_parms.antenna = device.tuner +
              100 * rx_channel_params->rspDuoTunerParams.tuner1AmPortSel;
  } else if (device.hwVer == SDRPLAY3_RSPdx_ID) {
    sdrplay3_parms.antenna = device_params->devParams->rspDxParams.antennaSel;
  } else {
    sdrplay3_parms.antenna = 0;
  }
  sdrplay3_parms.rf_lna_state = rx_channel_params->tunerParams.gain.LNAstate;
  sdrplay3_parms.rf_lna_state_freq = rx_channel_params->tunerParams.rfFreq.rfHz / 1e6;
  sdrplay3_parms.rf_lna_state_max = sdrplay3_max_gain;
  sdrplay3_parms.if_agc = rx_channel_params->ctrlParams.agc.enable;
  sdrplay3_parms.if_gain_reduction = rx_channel_params->tunerParams.gain.gRdB;
  sdrplay3_parms.if_agc_setpoint_dbfs = -(rx_channel_params->ctrlParams.agc.setPoint_dBfs);
  sdrplay3_parms.if_agc_attack_ms = rx_channel_params->ctrlParams.agc.attack_ms;
  sdrplay3_parms.if_agc_decay_ms = rx_channel_params->ctrlParams.agc.decay_ms;
  sdrplay3_parms.if_agc_decay_delay_ms = rx_channel_params->ctrlParams.agc.decay_delay_ms;
  sdrplay3_parms.if_agc_decay_threshold_db = rx_channel_params->ctrlParams.agc.decay_threshold_dB;
  if (device_params->devParams)
    sdrplay3_parms.freq_corr_ppb = 1000.0 * device_params->devParams->ppm;
  else
    sdrplay3_parms.freq_corr_ppb = -1;
  sdrplay3_parms.dc_offset_corr = rx_channel_params->ctrlParams.dcOffset.DCenable;
  sdrplay3_parms.iq_imbalance_corr = rx_channel_params->ctrlParams.dcOffset.IQenable;
  if (device_params->devParams)
    sdrplay3_parms.usb_transfer_mode = device_params->devParams->mode;
  else
    sdrplay3_parms.usb_transfer_mode = -1;
  if (device.hwVer == SDRPLAY3_RSP1A_ID) {
    sdrplay3_parms.notch_filters = device_params->devParams->rsp1aParams.rfNotchEnable +
              100 * device_params->devParams->rsp1aParams.rfDabNotchEnable;
  } else if (device.hwVer == SDRPLAY3_RSP2_ID) {
    sdrplay3_parms.notch_filters = rx_channel_params->rsp2TunerParams.rfNotchEnable;
  } else if (device.hwVer == SDRPLAY3_RSPduo_ID) {
    sdrplay3_parms.notch_filters = rx_channel_params->rspDuoTunerParams.rfNotchEnable +
              100 * rx_channel_params->rspDuoTunerParams.rfDabNotchEnable +
              10000 * rx_channel_params->rspDuoTunerParams.tuner1AmNotchEnable;
  } else if (device.hwVer == SDRPLAY3_RSPdx_ID) {
    sdrplay3_parms.notch_filters = device_params->devParams->rspDxParams.rfNotchEnable +
              100 *device_params->devParams->rspDxParams.rfDabNotchEnable;
  } else {
    sdrplay3_parms.notch_filters = 0;
  }
  if (device.hwVer == SDRPLAY3_RSP1A_ID) {
    sdrplay3_parms.bias_t = rx_channel_params->rsp1aTunerParams.biasTEnable;
  } else if (device.hwVer == SDRPLAY3_RSP2_ID) {
    sdrplay3_parms.bias_t = rx_channel_params->rsp2TunerParams.biasTEnable;
  } else if (device.hwVer == SDRPLAY3_RSPduo_ID) {
    sdrplay3_parms.bias_t = rx_channel_params->rspDuoTunerParams.biasTEnable;
  } else if (device.hwVer == SDRPLAY3_RSPdx_ID) {
    sdrplay3_parms.bias_t = device_params->devParams->rspDxParams.biasTEnable;
  } else {
    sdrplay3_parms.bias_t = 0;
  }
  if (device.hwVer == SDRPLAY3_RSPdx_ID) {
    sdrplay3_parms.rspdx_hdr_mode_and_bandwidth =
                  device_params->devParams->rspDxParams.hdrEnable ?
                  (int)(rx_channel_params->rspDxTunerParams.hdrBw) : -1;
  } else {
    sdrplay3_parms.rspdx_hdr_mode_and_bandwidth = -1;
  }
  sdrplay3_parms.check = SDRPLAY3PAR_VERNR;

  if (device.hwVer == SDRPLAY3_RSPduo_ID && device.tuner == sdrplay_api_Tuner_Both)
    sdrplay3_export_rx2_params();

  // write parameters
  sdrplay3_file = fopen(filename, "w");
  if (sdrplay3_file == NULL) {
    lirerr(2602);
    return -1;
  }
  sdr_pi = (int *)(&sdrplay3_parms);
  for (i = 0; i < MAX_SDRPLAY3_PARM; i++) {
    fprintf(sdrplay3_file, "%s [%d]\n", sdrplay3_parm_text[i], sdr_pi[i]);
  }
  parfile_end(sdrplay3_file);
  return 0;
}

static void sdrplay3_update_ui(void)
{
  float t1;

  ui.rx_addev_no = SDRPLAY3_DEVICE_CODE;
  if (device_params->devParams)
    t1 = (rint)(sdrplay3_get_samplerate() *
                (1.0 + device_params->devParams->ppm / 1000000.0));
  else
    t1 = (rint)(sdrplay3_get_samplerate());
  ui.rx_ad_speed = (int)t1;
  ui.rx_input_mode = IQ_DATA | DIGITAL_IQ;
  if (device.hwVer == SDRPLAY3_RSPduo_ID && device.tuner == sdrplay_api_Tuner_Both) {
    ui.rx_rf_channels = 2;
    ui.rx_ad_channels = 4;
    ui.rx_input_mode |= TWO_CHANNELS;
  } else {
    ui.rx_rf_channels = 1;
    ui.rx_ad_channels = 2;
  }
  ui.rx_admode = 0;
  return;
}

/************** public functions ************************/
void init_sdrplay3(void)
{
  int line = 0;
  sdrplay_api_ErrT err;
  sdrplay_api_CallbackFnsT callbacks;
  int errcod;

  errcod = sdrplay3_open_api();
  if (errcod || !(sdrplay3_status & sdrplay3_API_Opened_Flag)) {
    sdrplay3_display_open_api_error(errcod);
    return;
  }

  sdrplay3_select_device(&line);
  if (!(sdrplay3_status & sdrplay3_Device_Selected_Flag))
    return;

  if (sdrplay3_select_samplerate(&line) != 0)
    goto release_sdrplay3_device;

  if (sdrplay3_select_if_frequency(&line) != 0)
    goto release_sdrplay3_device;

  if (sdrplay3_select_bandwidth(&line) != 0)
    goto release_sdrplay3_device;

  line += 2;
  lir_text(5, line, "Press any key to continue to the next screen");
  await_keyboard();
  clear_screen();
  line = 2;

  if (sdrplay3_select_antenna(&line) != 0)
    goto release_sdrplay3_device;

  if (device.hwVer == SDRPLAY3_RSPdx_ID) {
    if (sdrplay3_select_rspdx_hdr_mode(&line) != 0)
      goto release_sdrplay3_device;
  }
  if (sdrplay3_select_rf_gain(&line) != 0)
    goto release_sdrplay3_device;

  if (sdrplay3_select_if_gain(&line) != 0)
    goto release_sdrplay3_device;

  line += 2;
  lir_text(5, line, "Press any key to continue to the next screen");
  await_keyboard();
  clear_screen();
  line = 2;

  if (sdrplay3_select_frequency_correction(&line) != 0)
    goto release_sdrplay3_device;

  if (sdrplay3_select_dc_offset_correction(&line) != 0)
    goto release_sdrplay3_device;

  if (sdrplay3_select_iq_imbalance_correction(&line) != 0)
    goto release_sdrplay3_device;

  if (sdrplay3_select_usb_transfer_mode(&line) != 0)
    goto release_sdrplay3_device;

  if (sdrplay3_select_notch_filters(&line) != 0)
    goto release_sdrplay3_device;

  if (sdrplay3_select_bias_t(&line) != 0)
    goto release_sdrplay3_device;

  // try to start streaming before saving the select configuration to disk
  callbacks.StreamACbFn = NULL;
  callbacks.StreamBCbFn = NULL;
  callbacks.EventCbFn = NULL;
  err = sdrplay_api_Init(device.dev, &callbacks, NULL);
  if (err != sdrplay_api_Success) {
    lir_text(3, 2, "sdrplay3: sdrplay_api_Init() failed.");
    lir_text(5, 7, press_any_key);
    await_keyboard();
    goto release_sdrplay3_device;
  }

  if (device.hwVer == SDRPLAY3_RSPduo_ID && device.tuner == sdrplay_api_Tuner_Both) {
    line += 2;
    lir_text(5, line, "Press any key to continue to RSPduo dual tuner settings for RX #2");
    await_keyboard();
    clear_screen();
    line = 2;
    line += 2;
    lir_text(3, line, "RSPduo Dual Tuner - Settings for RX #2");
    if (sdrplay3_select_rx2_settings(&line) != 0) {
      sdrplay_api_Uninit(device.dev);
      goto release_sdrplay3_device;
    }
  }

  err = sdrplay_api_Uninit(device.dev);
  if (err != sdrplay_api_Success) {
    lir_text(3, 2, "sdrplay3: sdrplay_api_Uninit() failed.");
    lir_text(5, 7, press_any_key);
    await_keyboard();
    goto release_sdrplay3_device;
  }

  sdrplay3_save_configuration(sdrplay3_parfil_name);

  sdrplay3_update_ui();

  // all done - close everything down
release_sdrplay3_device:
  err = sdrplay_api_LockDeviceApi();
  if (err != sdrplay_api_Success) {
    lir_text(3, 2, "sdrplay3: sdrplay_api_LockDeviceApi() failed.");
    lir_text(5, 7, press_any_key);
    await_keyboard();
  }
  err = sdrplay_api_ReleaseDevice(&device);
  if (err != sdrplay_api_Success) {
    lir_text(3, 2, "sdrplay3: sdrplay_api_ReleaseDevice() failed.");
    lir_text(5, 7, press_any_key);
    await_keyboard();
  }
  err = sdrplay_api_UnlockDeviceApi();
  if (err != sdrplay_api_Success) {
    lir_text(3, 2, "sdrplay3: sdrplay_api_UnlockDeviceApi() failed.");
    lir_text(5, 7, press_any_key);
    await_keyboard();
  }
  err = sdrplay_api_Close();
  sdrplay3_status &= ~sdrplay3_API_Opened_Flag;
  if (err != sdrplay_api_Success) {
    lir_text(3, 2, "sdrplay3: sdrplay_api_Close() failed.");
    lir_text(5, 7, press_any_key);
    await_keyboard();
  }
  unload_library();

  return;
}

static int sdrplay3_set_device(double dt1)
{
  char s[120];
  unsigned int i;
  int errcod;
  int found;
  sdrplay_api_ErrT err;
  unsigned int max_devs = MAX_RSP_DEVICES;
  sdrplay_api_DeviceT devices[MAX_RSP_DEVICES];
  unsigned int num_devs = 0;
  char serial[16];
  int *ise = (int *)(void *)serial;

  ise[0] = sdrplay3_parms.sernum1;
  ise[1] = sdrplay3_parms.sernum2;
  ise[2] = sdrplay3_parms.sernum3;
  ise[3] = sdrplay3_parms.sernum4;

  /* get sdrplay3 devices */
  for (;;) {
    errcod = 2620;
    err = sdrplay_api_LockDeviceApi();
    if (err != sdrplay_api_Success)
      goto close_sdrplay3_api;

    errcod = 2621;
    err = sdrplay_api_GetDevices(devices, &num_devs, max_devs);
    if (err != sdrplay_api_Success)
      goto unlock_sdrplay3_device_api;
    errcod = 2622;
    if (num_devs == 0)
      goto unlock_sdrplay3_device_api;

    found = 0;
    for (i = 0; i < num_devs; i++) {
      if (devices[i].hwVer == sdrplay3_parms.hw_version &&
          strcmp(devices[i].SerNo, serial) == 0) {
        device = devices[i];
        found = 1;
      }
    }
    if (found) {
      errcod = 0;
      break;
    }

    sprintf(s,"SDRplay RSP not found %.2f", current_time() - dt1);
    lir_text(3, 5, s);
    lir_refresh_screen();
    if (kill_all_flag)
      goto unlock_sdrplay3_device_api;

    err = sdrplay_api_UnlockDeviceApi();
    lir_sleep(300000);
  }

  // set RSPduo mode
  if (device.hwVer == SDRPLAY3_RSPduo_ID) {
    device.rspDuoMode = sdrplay3_parms.rspduo_mode;
    device.tuner = sdrplay3_parms.tuner;
    device.rspDuoSampleFreq = sdrplay3_parms.rspduo_sample_freq;
  }

  // select device
  errcod = 2623;
  err = sdrplay_api_SelectDevice(&device);
  if (err != sdrplay_api_Success)
    goto unlock_sdrplay3_device_api;

  errcod = 2624;
  err = sdrplay_api_UnlockDeviceApi();
  if (err != sdrplay_api_Success)
    goto release_sdrplay3_device;

  errcod = 2625;
  err = sdrplay_api_GetDeviceParams(device.dev, &device_params);
  if (err != sdrplay_api_Success)
    goto release_sdrplay3_device;

  if (device.tuner == sdrplay_api_Tuner_Neither || device.tuner == sdrplay_api_Tuner_A) {
    rx_channel_params = device_params->rxChannelA;
  } else if (device.tuner == sdrplay_api_Tuner_B) {
    rx_channel_params = device_params->rxChannelB;
  } else if (device.tuner == sdrplay_api_Tuner_Both) {
    rx_channel_params = device_params->rxChannelA;
  }

  sdrplay3_status |= sdrplay3_Device_Selected_Flag;
  return 0;

release_sdrplay3_device:
  sdrplay_api_ReleaseDevice(&device);
unlock_sdrplay3_device_api:
  sdrplay_api_UnlockDeviceApi();
close_sdrplay3_api:
  sdrplay_api_Close();
  sdrplay3_status &= ~sdrplay3_API_Opened_Flag;
  return errcod;
}

static void sdrplay3_set_samplerate(void)
{
  if (device_params->devParams)
    device_params->devParams->fsFreq.fsHz = sdrplay3_parms.sample_rate;
  if (sdrplay3_parms.decimation > 1) {
    rx_channel_params->ctrlParams.decimation.enable = 1;
    rx_channel_params->ctrlParams.decimation.decimationFactor = sdrplay3_parms.decimation;
  } else {
    rx_channel_params->ctrlParams.decimation.enable = 0;
    rx_channel_params->ctrlParams.decimation.decimationFactor = 1;
  }
  return;
}

static void sdrplay3_set_if_frequency(void)
{
  rx_channel_params->tunerParams.ifType = sdrplay3_parms.if_freq;
  return;
}

static void sdrplay3_set_bandwidth(void)
{
  rx_channel_params->tunerParams.bwType = sdrplay3_parms.if_bandwidth % RX2_OFFSET;
  return;
}

static int sdrplay3_set_antenna(void)
{
  int errcod;

  errcod = 0;
  if (device.hwVer == SDRPLAY3_RSP2_ID) {
    rx_channel_params->rsp2TunerParams.antennaSel = sdrplay3_parms.antenna % 100;
    rx_channel_params->rsp2TunerParams.amPortSel = sdrplay3_parms.antenna / 100;
  } else if (device.hwVer == SDRPLAY3_RSPduo_ID) {
    if (sdrplay3_parms.antenna % 100 != (int)(device.tuner)) {
      errcod = 2630;
    } else {
      rx_channel_params->rspDuoTunerParams.tuner1AmPortSel = sdrplay3_parms.antenna / 100;
    }
  } else if (device.hwVer == SDRPLAY3_RSPdx_ID) {
    device_params->devParams->rspDxParams.antennaSel = sdrplay3_parms.antenna;
  }
  return errcod;
}

static void sdrplay3_set_rspdx_hdr_mode(void)
{
  if (sdrplay3_parms.rspdx_hdr_mode_and_bandwidth == -1) {
    device_params->devParams->rspDxParams.hdrEnable = 0;
  } else {
    device_params->devParams->rspDxParams.hdrEnable = 1;
    rx_channel_params->rspDxTunerParams.hdrBw = sdrplay3_parms.rspdx_hdr_mode_and_bandwidth;
  }
  return;
}

static void sdrplay3_set_rf_gain(void)
{
  rx_channel_params->tunerParams.gain.LNAstate = sdrplay3_parms.rf_lna_state % RX2_OFFSET;
  rx_channel_params->tunerParams.rfFreq.rfHz = (sdrplay3_parms.rf_lna_state_freq % RX2_OFFSET) * 1e6;
  sdrplay3_max_gain = sdrplay3_parms.rf_lna_state_max % RX2_OFFSET;
  return;
}

static void sdrplay3_set_if_gain(void)
{
  rx_channel_params->ctrlParams.agc.enable = sdrplay3_parms.if_agc % RX2_OFFSET;
  rx_channel_params->tunerParams.gain.gRdB = sdrplay3_parms.if_gain_reduction % RX2_OFFSET;
  rx_channel_params->ctrlParams.agc.setPoint_dBfs = -(sdrplay3_parms.if_agc_setpoint_dbfs % RX2_OFFSET);
  rx_channel_params->ctrlParams.agc.attack_ms = sdrplay3_parms.if_agc_attack_ms % RX2_OFFSET;
  rx_channel_params->ctrlParams.agc.decay_ms = sdrplay3_parms.if_agc_decay_ms % RX2_OFFSET;
  rx_channel_params->ctrlParams.agc.decay_delay_ms = sdrplay3_parms.if_agc_decay_delay_ms % RX2_OFFSET;
  rx_channel_params->ctrlParams.agc.decay_threshold_dB = sdrplay3_parms.if_agc_decay_threshold_db % RX2_OFFSET;
  return;
}

static void sdrplay3_set_frequency_correction(void)
{
  if (device_params->devParams)
    device_params->devParams->ppm = (double)(sdrplay3_parms.freq_corr_ppb) / 1000.0;
  return;
}

static void sdrplay3_set_dc_offset_correction(void)
{
  rx_channel_params->ctrlParams.dcOffset.DCenable = sdrplay3_parms.dc_offset_corr % RX2_OFFSET;
  return;
}

static void sdrplay3_set_iq_imbalance_correction(void)
{
  rx_channel_params->ctrlParams.dcOffset.IQenable = sdrplay3_parms.iq_imbalance_corr % RX2_OFFSET;
  if (rx_channel_params->ctrlParams.dcOffset.IQenable)
    rx_channel_params->ctrlParams.dcOffset.DCenable = 1;
  return;
}

static void sdrplay3_set_usb_transfer_mode(void)
{
  if (device_params->devParams)
    device_params->devParams->mode = sdrplay3_parms.usb_transfer_mode;
  return;
}

static void sdrplay3_set_notch_filters(void)
{
  if (device.hwVer == SDRPLAY3_RSP1A_ID) {
    device_params->devParams->rsp1aParams.rfNotchEnable = sdrplay3_parms.notch_filters % 100;
    device_params->devParams->rsp1aParams.rfDabNotchEnable = sdrplay3_parms.notch_filters / 100;
  } else if (device.hwVer == SDRPLAY3_RSP2_ID) {
    rx_channel_params->rsp2TunerParams.rfNotchEnable = sdrplay3_parms.notch_filters;
  } else if (device.hwVer == SDRPLAY3_RSPduo_ID) {
    rx_channel_params->rspDuoTunerParams.rfNotchEnable = sdrplay3_parms.notch_filters % 100;
    rx_channel_params->rspDuoTunerParams.rfDabNotchEnable = (sdrplay3_parms.notch_filters / 100) % 100;
    rx_channel_params->rspDuoTunerParams.tuner1AmNotchEnable = (sdrplay3_parms.notch_filters / 10000) % 100;
  } else if (device.hwVer == SDRPLAY3_RSPdx_ID) {
    device_params->devParams->rspDxParams.rfNotchEnable = sdrplay3_parms.notch_filters % 100;
    device_params->devParams->rspDxParams.rfDabNotchEnable = sdrplay3_parms.notch_filters / 100;
  }
  return;
}

static void sdrplay3_set_bias_t(void)
{
  if (device.hwVer == SDRPLAY3_RSP1A_ID) {
    rx_channel_params->rsp1aTunerParams.biasTEnable = sdrplay3_parms.bias_t;
  } else if (device.hwVer == SDRPLAY3_RSP2_ID) {
    rx_channel_params->rsp2TunerParams.biasTEnable = sdrplay3_parms.bias_t;
  } else if (device.hwVer == SDRPLAY3_RSPduo_ID) {
    rx_channel_params->rspDuoTunerParams.biasTEnable = sdrplay3_parms.bias_t % RX2_OFFSET;
  } else if (device.hwVer == SDRPLAY3_RSPdx_ID) {
    device_params->devParams->rspDxParams.biasTEnable = sdrplay3_parms.bias_t;
  }
  return;
}

static void sdrplay3_rx_callback(short *si, short *sq, sdrplay_api_StreamCbParamsT *params, unsigned int numSamples, unsigned int reset, void *cbContext)
{
  UNUSED(params);
  UNUSED(reset);
  UNUSED(cbContext);

  unsigned int i;
  short int *iz;

  if (sdrplay3_cancel_flag)
    return;

  if (!(device.tuner == sdrplay_api_Tuner_Both)) {
    for (i = 0; i < numSamples; i++) {
      iz = (short int*)(&timf1_char[timf1p_sdr]);
      iz[0] = si[i];
      iz[1] = sq[i];
      timf1p_sdr = (timf1p_sdr + 4) & timf1_bytemask;
    }
    if (((timf1p_sdr - timf1p_pa + timf1_bytes) & timf1_bytemask) >=
                                                      snd[RXAD].block_bytes) {
      lir_set_event(EVENT_HWARE1_RXREADY);
    }
  } else {
    for (i = 0; i < numSamples; i++) {
      iz = (short int*)(&timf1_char[timf1p_sdr]);
      iz[0] = si[i];
      iz[1] = sq[i];
      timf1p_sdr = (timf1p_sdr + 8) & timf1_bytemask;
    }
    if (((timf1p_sdr - timf1p_pa + timf1_bytes) & timf1_bytemask) >=
                                                      snd[RXAD].block_bytes &&
        ((timf1p_sdr2 - timf1p_pa + timf1_bytes) & timf1_bytemask) >=
                                                      snd[RXAD].block_bytes) {
      lir_set_event(EVENT_HWARE1_RXREADY);
    }
  }

  return;
}

static void sdrplay3_rx2_callback(short *si, short *sq, sdrplay_api_StreamCbParamsT *params, unsigned int numSamples, unsigned int reset, void *cbContext)
{
  UNUSED(params);
  UNUSED(reset);
  UNUSED(cbContext);

  unsigned int i;
  short int *iz;

  if (sdrplay3_cancel_flag)
    return;

  for (i = 0; i < numSamples; i++) {
    iz = (short int*)(&timf1_char[timf1p_sdr2]);
    iz[2] = si[i];
    iz[3] = sq[i];
    timf1p_sdr2 = (timf1p_sdr2 + 8) & timf1_bytemask;
  }

  if (((timf1p_sdr - timf1p_pa + timf1_bytes) & timf1_bytemask) >=
                                                    snd[RXAD].block_bytes &&
      ((timf1p_sdr2 - timf1p_pa + timf1_bytes) & timf1_bytemask) >=
                                                    snd[RXAD].block_bytes) {
    lir_set_event(EVENT_HWARE1_RXREADY);
  }

  return;
}

static void sdrplay3_event_callback(sdrplay_api_EventT eventId, sdrplay_api_TunerSelectT tuner, sdrplay_api_EventParamsT *params, void *cbContext)
{
  UNUSED(tuner);
  UNUSED(cbContext);

  if (eventId == sdrplay_api_PowerOverloadChange) {
    sdrplay_api_PowerOverloadCbEventIdT powerOverloadChangeType = params->powerOverloadParams.powerOverloadChangeType;
    if (powerOverloadChangeType == sdrplay_api_Overload_Detected) {
      sdrplay_api_Update(device.dev, device.tuner, sdrplay_api_Update_Ctrl_OverloadMsgAck, sdrplay_api_Update_Ext1_None);
      ad_overload_flag = 1;
    } else if (powerOverloadChangeType == sdrplay_api_Overload_Corrected) {
      sdrplay_api_Update(device.dev, device.tuner, sdrplay_api_Update_Ctrl_OverloadMsgAck, sdrplay_api_Update_Ext1_None);
      ad_overload_flag = 0;
    }
  } else if (eventId == sdrplay_api_DeviceRemoved) {
    kill_all_flag = 1;
  } else if (eventId == sdrplay_api_RspDuoModeChange) {
    if (params->rspDuoModeParams.modeChangeType == sdrplay_api_MasterDllDisappeared) {
      kill_all_flag = 1;
    }
  }
}

static int start_streaming(void)
{
  sdrplay_api_ErrT err;
  int errcod;
  sdrplay_api_CallbackFnsT callbacks;

  errcod = 2640;
  callbacks.StreamACbFn = sdrplay3_rx_callback;
  if (device.hwVer == SDRPLAY3_RSPduo_ID && device.tuner == sdrplay_api_Tuner_Both)
    callbacks.StreamBCbFn = sdrplay3_rx2_callback;
  else
    callbacks.StreamBCbFn = NULL;
  callbacks.EventCbFn = sdrplay3_event_callback;
  err = sdrplay_api_Init(device.dev, &callbacks, NULL);
  if (err != sdrplay_api_Success)
    return errcod;
  return 0;
}

static int sdrplay3_update_rx2_setting_quiet(sdrplay_api_ReasonForUpdateT reason_for_update)
{
  sdrplay_api_ErrT err;

  err = sdrplay_api_Update(device.dev, sdrplay_api_Tuner_B, reason_for_update, sdrplay_api_Update_Ext1_None);
  if (err != sdrplay_api_Success)
    return -1;
  return 0;
}

static int sdrplay3_set_rx2_bandwidth(void)
{
  int errcod;
  sdrplay_api_Bw_MHzT bandwidth;

  bandwidth = sdrplay3_parms.if_bandwidth / RX2_OFFSET;
  if (device_params->rxChannelB->tunerParams.bwType == bandwidth)
    return 0;
  errcod = 2650;
  device_params->rxChannelB->tunerParams.bwType = bandwidth;
  if (sdrplay3_update_rx2_setting_quiet(sdrplay_api_Update_Tuner_BwType) != 0)
    return errcod;
  return 0;
}

static int sdrplay3_set_rx2_rf_gain(void)
{
  int errcod;
  int lna_state;
  double lna_state_freq;
  sdrplay_api_ReasonForUpdateT reason_for_update;

  lna_state = sdrplay3_parms.rf_lna_state / RX2_OFFSET;
  lna_state_freq = (sdrplay3_parms.rf_lna_state_freq / RX2_OFFSET) * 1e6;
  if (device_params->rxChannelB->tunerParams.gain.LNAstate == lna_state && device_params->rxChannelB->tunerParams.rfFreq.rfHz == lna_state_freq)
    return 0;
  errcod = 2651;
  reason_for_update = sdrplay_api_Update_None;
  if (device_params->rxChannelB->tunerParams.gain.LNAstate != lna_state) {
    device_params->rxChannelB->tunerParams.gain.LNAstate = lna_state;
    reason_for_update |= sdrplay_api_Update_Tuner_Gr;
  }
  if (device_params->rxChannelB->tunerParams.rfFreq.rfHz != lna_state_freq) {
    device_params->rxChannelB->tunerParams.rfFreq.rfHz = lna_state_freq;
    reason_for_update |= sdrplay_api_Update_Tuner_Frf;
  }
  if (sdrplay3_update_rx2_setting_quiet(reason_for_update) != 0)
    return errcod;
  sdrplay3_max_gain = sdrplay3_parms.rf_lna_state_max / RX2_OFFSET;
  return 0;
}

static int sdrplay3_set_rx2_if_gain(void)
{
  int errcod;
  sdrplay_api_AgcControlT if_agc;
  int if_gain_reduction;
  int if_agc_setpoint_dbfs;
  int if_agc_attack_ms;
  int if_agc_decay_ms;
  int if_agc_decay_delay_ms;
  int if_agc_decay_threshold_db;

  if_agc = sdrplay3_parms.if_agc / RX2_OFFSET;
  if_gain_reduction = sdrplay3_parms.if_gain_reduction / RX2_OFFSET;
  if_agc_setpoint_dbfs = -(sdrplay3_parms.if_agc_setpoint_dbfs / RX2_OFFSET);
  if_agc_attack_ms = sdrplay3_parms.if_agc_attack_ms / RX2_OFFSET;
  if_agc_decay_ms = sdrplay3_parms.if_agc_decay_ms / RX2_OFFSET;
  if_agc_decay_delay_ms = sdrplay3_parms.if_agc_decay_delay_ms / RX2_OFFSET;
  if_agc_decay_threshold_db = sdrplay3_parms.if_agc_decay_threshold_db / RX2_OFFSET;
  if (if_agc == sdrplay_api_AGC_DISABLE) {
    if (device_params->rxChannelB->tunerParams.gain.gRdB == if_gain_reduction)
      return 0;
    errcod = 2652;
    device_params->rxChannelB->tunerParams.gain.gRdB = if_gain_reduction;
    if (sdrplay3_update_rx2_setting_quiet(sdrplay_api_Update_Tuner_Gr) != 0)
      return errcod;
    return 0;
  } else {
    if (device_params->rxChannelB->ctrlParams.agc.enable == if_agc &&
        device_params->rxChannelB->ctrlParams.agc.setPoint_dBfs == if_agc_setpoint_dbfs &&
        device_params->rxChannelB->ctrlParams.agc.attack_ms == if_agc_attack_ms &&
        device_params->rxChannelB->ctrlParams.agc.decay_ms == if_agc_decay_ms &&
        device_params->rxChannelB->ctrlParams.agc.decay_delay_ms == if_agc_decay_delay_ms &&
        device_params->rxChannelB->ctrlParams.agc.decay_threshold_dB == if_agc_decay_threshold_db)
      return 0;
    errcod = 2653;
    device_params->rxChannelB->ctrlParams.agc.enable = if_agc;
    device_params->rxChannelB->ctrlParams.agc.setPoint_dBfs = if_agc_setpoint_dbfs;
    device_params->rxChannelB->ctrlParams.agc.attack_ms = if_agc_attack_ms;
    device_params->rxChannelB->ctrlParams.agc.decay_ms = if_agc_decay_ms;
    device_params->rxChannelB->ctrlParams.agc.decay_delay_ms = if_agc_decay_delay_ms;
    device_params->rxChannelB->ctrlParams.agc.decay_threshold_dB = if_agc_decay_threshold_db;
    if (sdrplay3_update_rx2_setting_quiet(sdrplay_api_Update_Ctrl_Agc) != 0)
      return errcod;
    return 0;
  }
}

static int sdrplay3_set_rx2_dc_offset_correction(void)
{
  int errcod;
  int dc_offset_corr;

  dc_offset_corr = sdrplay3_parms.dc_offset_corr / RX2_OFFSET;
  if (device_params->rxChannelB->ctrlParams.dcOffset.DCenable == dc_offset_corr)
    return 0;
  errcod = 2654;
  device_params->rxChannelB->ctrlParams.dcOffset.DCenable = dc_offset_corr;
  if (sdrplay3_update_rx2_setting_quiet(sdrplay_api_Update_Tuner_DcOffset) != 0)
    return errcod;
  return 0;
}


static int sdrplay3_set_rx2_iq_imbalance_correction(void)
{
  int errcod;
  int iq_imbalance_corr;

  iq_imbalance_corr = sdrplay3_parms.iq_imbalance_corr / RX2_OFFSET;
  if (device_params->rxChannelB->ctrlParams.dcOffset.IQenable == iq_imbalance_corr)
    return 0;
  errcod = 2655;
  device_params->rxChannelB->ctrlParams.dcOffset.IQenable = iq_imbalance_corr;
  if (device_params->rxChannelB->ctrlParams.dcOffset.IQenable)
    device_params->rxChannelB->ctrlParams.dcOffset.DCenable = 1;
  if (sdrplay3_update_rx2_setting_quiet(sdrplay_api_Update_Tuner_DcOffset) != 0)
    return errcod;
  return 0;
}

static int sdrplay3_set_rx2_notch_filters(void)
{
  int errcod;
  int rf_notch_enable;
  int rf_dab_notch_enable;
  sdrplay_api_ReasonForUpdateT reason_for_update;

  rf_notch_enable = (sdrplay3_parms.notch_filters / (RX2_OFFSET * 100)) % 100;
  rf_dab_notch_enable = (sdrplay3_parms.notch_filters / (RX2_OFFSET * 100)) / 100;
  if (device_params->rxChannelB->rspDuoTunerParams.rfNotchEnable == rf_notch_enable &&
      device_params->rxChannelB->rspDuoTunerParams.rfDabNotchEnable == rf_dab_notch_enable)
    return 0;
  errcod = 2656;
  reason_for_update = sdrplay_api_Update_None;
  if (device_params->rxChannelB->rspDuoTunerParams.rfNotchEnable != rf_notch_enable) {
    device_params->rxChannelB->rspDuoTunerParams.rfNotchEnable = rf_notch_enable;
    reason_for_update |= sdrplay_api_Update_RspDuo_RfNotchControl;
  }
  if (device_params->rxChannelB->rspDuoTunerParams.rfDabNotchEnable != rf_dab_notch_enable) {
    device_params->rxChannelB->rspDuoTunerParams.rfDabNotchEnable = rf_dab_notch_enable;
    reason_for_update |= sdrplay_api_Update_RspDuo_RfDabNotchControl;
  }
  if (sdrplay3_update_rx2_setting_quiet(reason_for_update) != 0)
    return errcod;
  return 0;
}

static int sdrplay3_set_rx2_bias_t(void)
{
  int errcod;
  int bias_t_enable;

  bias_t_enable = sdrplay3_parms.bias_t / RX2_OFFSET;
  if (device_params->rxChannelB->rspDuoTunerParams.biasTEnable == bias_t_enable)
    return 0;
  errcod = 2657;
  device_params->rxChannelB->rspDuoTunerParams.biasTEnable = bias_t_enable;
  if (sdrplay3_update_rx2_setting_quiet(sdrplay_api_Update_RspDuo_BiasTControl) != 0)
    return errcod;
  return 0;
}

static int sdrplay3_set_rx2_settings(void)
{
  int errcod;

  errcod = sdrplay3_set_rx2_bandwidth();
  if (errcod)
    return errcod;
  errcod = sdrplay3_set_rx2_rf_gain();
  if (errcod)
    return errcod;
  errcod = sdrplay3_set_rx2_if_gain();
  if (errcod)
    return errcod;
  errcod = sdrplay3_set_rx2_dc_offset_correction();
  if (errcod)
    return errcod;
  errcod = sdrplay3_set_rx2_iq_imbalance_correction();
  if (errcod)
    return errcod;
  errcod = sdrplay3_set_rx2_notch_filters();
  if (errcod)
    return errcod;
  errcod = sdrplay3_set_rx2_bias_t();
  if (errcod)
    return errcod;

  return 0;
}

static int set_sdrplay3_att(void)
{
  sdrplay_api_ErrT err;
  int errcod;
  double freq;
  int num_lna_states;
  uint8_t *lna_states;
  int lna_state;

  freq = rx_channel_params->tunerParams.rfFreq.rfHz;
  lna_states = sdrplay3_find_lna_states(freq, &num_lna_states);
  sdrplay3_max_gain = lna_states[num_lna_states - 1];
  lna_state = sdrplay3_rf_att_to_lna_state(fg.gain, num_lna_states, lna_states);
  if (rx_channel_params->tunerParams.gain.LNAstate != lna_state) {
    errcod = 2670;
    rx_channel_params->tunerParams.gain.LNAstate = lna_state;
    err = sdrplay_api_Update(device.dev, device.tuner, sdrplay_api_Update_Tuner_Gr, sdrplay_api_Update_Ext1_None);
    if (err != sdrplay_api_Success)
      return errcod;
  }
  return 0;
}

static int set_sdrplay3_frequency(void)
{
  sdrplay_api_ErrT err;
  int errcod;
  double frequency;
  double freq_corr;

  frequency = fg.passband_center * 1e6;
  frequency = fmax(frequency, SDRPLAY3_MIN_FREQ * 1e3);
  frequency = fmin(frequency, SDRPLAY3_MAX_FREQ * 1e3);
  freq_corr = frequency * sdrplay3_parms.freq_corr_ppb * 1e-9;
  frequency += freq_corr;
  errcod = 2671;
  rx_channel_params->tunerParams.rfFreq.rfHz = frequency;
  err = sdrplay_api_Update(device.dev, device.tuner, sdrplay_api_Update_Tuner_Frf, sdrplay_api_Update_Ext1_None);
  if (err != sdrplay_api_Success)
    return errcod;
  return 0;
}

void sdrplay3_input(void)
{
  sdrplay_api_ErrT err;
  int local_att_counter;
  int local_nco_counter;
  double dt1;
  double read_start_time;
  double total_reads;
  int ret;
  int errcod;
  int rxin_local_workload_reset;
  int timing_loop_counter;
  int timing_loop_counter_max;
  int initial_skip_flag;
  int local_ad_overload_flag;

  lir_init_event(EVENT_HWARE1_RXREADY);
#if OSNUM == OSNUM_LINUX
  clear_thread_times(THREAD_SDRPLAY3_INPUT);
#endif
  // subtract 1 to force set_sdrplay3_att() and set_sdrplay3_freq() to be
  // called right after we start streaming
  local_att_counter = sdr_att_counter - 1;
  local_nco_counter = sdr_nco_counter - 1;
  dt1 = current_time();
  timf1p_sdr = timf1p_pa;
  timf1p_sdr2 = timf1p_pa;
  local_ad_overload_flag = 0;

  ret = read_sdrpar(sdrplay3_parfil_name, MAX_SDRPLAY3_PARM,
                    sdrplay3_parm_text, (int*)((void*)&sdrplay3_parms));
  errcod = 2600;
  if (ret != 0 || sdrplay3_parms.check != SDRPLAY3PAR_VERNR)
    goto sdrplay3_input_error_exit;

  sdrplay3_cancel_flag = 1;

  errcod = 2601;
  load_sdrplay3_library();
  if (!(sdrplay3_status & sdrplay3_Library_Loaded_Flag))
    goto sdrplay3_input_error_exit;

  errcod = sdrplay3_open_api();
  if (errcod || !(sdrplay3_status & sdrplay3_API_Opened_Flag))
    goto sdrplay3_input_error_exit;

  errcod = sdrplay3_set_device(dt1);
  if (errcod)
    goto sdrplay3_input_error_exit;

  sdrplay3_set_samplerate();
  sdrplay3_set_if_frequency();
  sdrplay3_set_bandwidth();
  errcod = sdrplay3_set_antenna();
  if (errcod)
    goto sdrplay3_release_device;
  if (device.hwVer == SDRPLAY3_RSPdx_ID)
    sdrplay3_set_rspdx_hdr_mode();
  sdrplay3_set_rf_gain();
  sdrplay3_set_if_gain();
  sdrplay3_set_frequency_correction();
  sdrplay3_set_dc_offset_correction();
  sdrplay3_set_iq_imbalance_correction();
  sdrplay3_set_usb_transfer_mode();
  sdrplay3_set_notch_filters();
  sdrplay3_set_bias_t();

  fft1_block_timing();
  if (thread_command_flag[THREAD_SCREEN] != THRFLAG_NOT_ACTIVE) {
    while (thread_status_flag[THREAD_SCREEN] != THRFLAG_ACTIVE &&
           thread_status_flag[THREAD_SCREEN] != THRFLAG_IDLE &&
           thread_status_flag[THREAD_SCREEN] != THRFLAG_SEM_WAIT) {
      if (thread_command_flag[THREAD_SDRPLAY2_INPUT] == THRFLAG_KILL)
        goto sdrplay3_release_device;
      lir_sleep(10000);
    }
  }

  errcod = start_streaming();
  if (errcod)
    goto sdrplay3_release_device;
  // change the settings for the second receiver in Dual Tuner mode
  // (where they are different from the ones in the first receiver)
  if (device.hwVer == SDRPLAY3_RSPduo_ID && device.tuner == sdrplay_api_Tuner_Both) {
    errcod = sdrplay3_set_rx2_settings();
    if (errcod)
      goto sdrplay3_stop_streaming;
  }

#include "timing_setup.c"
  thread_status_flag[THREAD_SDRPLAY3_INPUT] = THRFLAG_ACTIVE;
  lir_status = LIR_OK;
  errcod = 0;
  sdrplay3_cancel_flag = 0;

  while (!kill_all_flag &&
            thread_command_flag[THREAD_SDRPLAY3_INPUT] == THRFLAG_ACTIVE) {
    if (local_att_counter != sdr_att_counter) {
      local_att_counter = sdr_att_counter;
      set_sdrplay3_att();
    }
    if (local_nco_counter != sdr_nco_counter) {
      local_nco_counter = sdr_nco_counter;
      set_sdrplay3_frequency();
    }
    if (local_ad_overload_flag != ad_overload_flag) {
      if (ad_overload_flag) {
        wg_error("AD OVERLOAD - Please increase attenuation", 0);
      } else {
        wg_error("                                         ", 0);
      }
      local_ad_overload_flag = ad_overload_flag;
    }
    lir_await_event(EVENT_HWARE1_RXREADY);
    if (kill_all_flag)
      goto sdrplay3_stop_streaming;

    if (!(device.hwVer == SDRPLAY3_RSPduo_ID && device.tuner == sdrplay_api_Tuner_Both)) {
      while (!kill_all_flag &&
 ((timf1p_pa-timf1p_sdr+timf1_bytes) & timf1_bytemask) > snd[RXAD].block_bytes) {
#include "input_speed.c"
        finish_rx_read();
        if (kill_all_flag)
          goto sdrplay3_stop_streaming;
      }
    } else {
      while (!kill_all_flag &&
 ((timf1p_pa-timf1p_sdr+timf1_bytes) & timf1_bytemask) > snd[RXAD].block_bytes &&
 ((timf1p_pa-timf1p_sdr2+timf1_bytes) & timf1_bytemask) > snd[RXAD].block_bytes) {
#include "input_speed.c"
        finish_rx_read();
        if (kill_all_flag)
          goto sdrplay3_stop_streaming;
      }
    }
  }

sdrplay3_stop_streaming:
  err = sdrplay_api_Uninit(device.dev);
  if (err != sdrplay_api_Success) {
    lirerr(2691);
    goto sdrplay3_close_api;
  }
sdrplay3_release_device:
  err = sdrplay_api_LockDeviceApi();
  if (err != sdrplay_api_Success) {
    lirerr(2692);
    goto sdrplay3_close_api;
  }
  err = sdrplay_api_ReleaseDevice(&device);
  if (err != sdrplay_api_Success) {
    lirerr(2693);
    goto sdrplay3_close_api;
  }
  err = sdrplay_api_UnlockDeviceApi();
  if (err != sdrplay_api_Success) {
    lirerr(2694);
    goto sdrplay3_close_api;
  }
sdrplay3_close_api:
  err = sdrplay_api_Close();
  sdrplay3_status &= ~sdrplay3_API_Opened_Flag;
  if (err != sdrplay_api_Success)
    lirerr(2695);
  unload_library();
sdrplay3_input_error_exit:
  sdr = -1;
  if (errcod != 0)
    lirerr(errcod);
  thread_status_flag[THREAD_SDRPLAY3_INPUT] = THRFLAG_RETURNED;
  while (thread_command_flag[THREAD_SDRPLAY3_INPUT] != THRFLAG_NOT_ACTIVE) {
    lir_sleep(1000);
  }
  lir_close_event(EVENT_HWARE1_RXREADY);
}

static void sdrplay3_display_gains(char *rx_name, int rf_lna_state, int if_agc, int if_gain_reduction, int if_agc_setpoint_dbfs, int if_agc_attack_ms, int if_agc_decay_ms, int if_agc_decay_delay_ms, int if_agc_decay_threshold_db, int *line)
{
  char s[240];
  char *agc_mode_name = 0;

  if (if_agc == sdrplay_api_AGC_DISABLE) {
    snprintf(s, 240, "%s gains - RF LNA state = %d, IF gain reduction = %d, AGC = Disabled", rx_name, rf_lna_state, if_gain_reduction);
  } else {
    if (if_agc == sdrplay_api_AGC_100HZ) {
      agc_mode_name = SDRPLAY3_AGC_100HZ_NAME;
    } else if (if_agc == sdrplay_api_AGC_50HZ) {
      agc_mode_name = SDRPLAY3_AGC_50HZ_NAME;
    } else if (if_agc == sdrplay_api_AGC_5HZ) {
      agc_mode_name = SDRPLAY3_AGC_5HZ_NAME;
    } else if (if_agc == sdrplay_api_AGC_CTRL_EN) {
      agc_mode_name = SDRPLAY3_AGC_CTRL_EN_NAME;
    }
    snprintf(s, 240, "%s gains - RF LNA state = %d, AGC = Enabled, AGC mode = %s", rx_name, rf_lna_state, agc_mode_name);
  }
  lir_text(24, line[0], s);
  SNDLOG"\n%s", s);
  line[0]++;

  if (if_agc == sdrplay_api_AGC_CTRL_EN) {
    snprintf(s, 240, "%s AGC settings - Set Point = %d dBFS, Attack Time = %d ms, Decay Time = %d ms, Decay Delay = %d ms, Decay Threshold = %d dB", rx_name, -if_agc_setpoint_dbfs, if_agc_attack_ms, if_agc_decay_ms, if_agc_decay_delay_ms, if_agc_decay_threshold_db);
    lir_text(24, line[0], s);
    SNDLOG"\n%s", s);
    line[0]++;
  }
}

int display_sdrplay3_parm_info(int *line)
{
  char s[120];
  int errcod;
  char *device_name = "UNKNOWN DEVICE";
  char serial[16];
  int *ise = (int *)(void *)serial;
  char *rspduo_mode_name = 0;
  char *rspduo_tuner_name = 0;
  double sample_rate_kHz;
  int decimation;
  char *disabled_string = "Disabled";
  char *enabled_string = "Enabled";
  char *dc_offset_corr_mode_name = 0;
  char *iq_imbalance_corr_mode_name = 0;
  char *usb_transfer_mode_name = 0;
  char *rf_notch_filter_mode_name = 0;
  char *rf_dab_notch_filter_mode_name = 0;
  char *am_notch_filter_mode_name = 0;
  char *bias_t_mode_name = 0;
  int hdr_bandwidth;

  errcod = read_sdrpar(sdrplay3_parfil_name, MAX_SDRPLAY3_PARM,
                       sdrplay3_parm_text, (int*)((void*)&sdrplay3_parms));
  if (errcod != 0)
    return errcod;

  settextcolor(7);

  if (sdrplay3_parms.hw_version == SDRPLAY3_RSP1_ID) {
    device_name = SDRPLAY3_RSP1_NAME;
  } else if (sdrplay3_parms.hw_version == SDRPLAY3_RSP1A_ID) {
    device_name = SDRPLAY3_RSP1A_NAME;
  } else if (sdrplay3_parms.hw_version == SDRPLAY3_RSP2_ID) {
    device_name = SDRPLAY3_RSP2_NAME;
  } else if (sdrplay3_parms.hw_version == SDRPLAY3_RSPduo_ID) {
    device_name = SDRPLAY3_RSPduo_NAME;
  } else if (sdrplay3_parms.hw_version == SDRPLAY3_RSPdx_ID) {
    device_name = SDRPLAY3_RSPdx_NAME;
  }
  ise[0] = sdrplay3_parms.sernum1;
  ise[1] = sdrplay3_parms.sernum2;
  ise[2] = sdrplay3_parms.sernum3;
  ise[3] = sdrplay3_parms.sernum4;
  snprintf(s, 120, "MFG = SDRplay, Device = %s, Serial = %s",
           device_name, serial);
  lir_text(24, line[0], s);
  SNDLOG"\n%s", s);
  line[0]++;

  if (sdrplay3_parms.hw_version == SDRPLAY3_RSPduo_ID) {
    if (sdrplay3_parms.rspduo_mode == sdrplay_api_RspDuoMode_Single_Tuner) {
      rspduo_mode_name = SDRPLAY3_RSPduo_SINGLE_TUNER_NAME;
    } else if (sdrplay3_parms.rspduo_mode == sdrplay_api_RspDuoMode_Dual_Tuner) {
      rspduo_mode_name = SDRPLAY3_RSPduo_DUAL_TUNER_NAME;
    } else if (sdrplay3_parms.rspduo_mode == sdrplay_api_RspDuoMode_Master) {
      rspduo_mode_name = SDRPLAY3_RSPduo_MASTER_MODE_NAME;
    } else if (sdrplay3_parms.rspduo_mode == sdrplay_api_RspDuoMode_Slave) {
      rspduo_mode_name = SDRPLAY3_RSPduo_SLAVE_MODE_NAME;
    }
    if (sdrplay3_parms.tuner == sdrplay_api_Tuner_A) {
      rspduo_tuner_name = SDRPLAY3_RSPduo_TUNER_A_NAME;
    } else if (sdrplay3_parms.tuner == sdrplay_api_Tuner_B) {
      rspduo_tuner_name = SDRPLAY3_RSPduo_TUNER_B_NAME;
    } else if (sdrplay3_parms.tuner == sdrplay_api_Tuner_Both) {
      rspduo_tuner_name = SDRPLAY3_RSPduo_BOTH_TUNERS_NAME;
    }
    snprintf(s, 120, "RSPduo mode = %s, Tuner = %s, RSPduo internal s/r = %d MHz",
             rspduo_mode_name, rspduo_tuner_name, sdrplay3_parms.rspduo_sample_freq / 1000000);
    lir_text(24, line[0], s);
    SNDLOG"\n%s", s);
    line[0]++;
  }

  sample_rate_kHz = 0.0;
  if (sdrplay3_parms.if_freq == sdrplay_api_IF_Zero) {
    sample_rate_kHz = sdrplay3_parms.sample_rate / 1000;
  } else if (sdrplay3_parms.if_freq == sdrplay_api_IF_1_620) {
    sample_rate_kHz = 2e3;
  } else if (sdrplay3_parms.if_freq == sdrplay_api_IF_2_048) {
    sample_rate_kHz = 2e3;
  }
  decimation = sdrplay3_parms.decimation;
  snprintf(s, 120, "Sample rate = %.3lf kHz (Initial sample rate = %.0lf kHz, Decimation = %d)",
           sample_rate_kHz / decimation, sample_rate_kHz, decimation);
  lir_text(24, line[0], s);
  SNDLOG"\n%s", s);
  line[0]++;

  if (sdrplay3_parms.hw_version == SDRPLAY3_RSPduo_ID && sdrplay3_parms.tuner == sdrplay_api_Tuner_Both) {
    snprintf(s, 120, "IF frequency = %d kHz, Bandwidth RX #1 = %d kHz, Bandwidth RX #2 = %d kHz",
             sdrplay3_parms.if_freq, sdrplay3_parms.if_bandwidth % RX2_OFFSET,
             sdrplay3_parms.if_bandwidth / RX2_OFFSET);
  } else {
    snprintf(s, 120, "IF frequency = %d kHz, Bandwidth = %d kHz",
             sdrplay3_parms.if_freq, sdrplay3_parms.if_bandwidth);
  }
  lir_text(24, line[0], s);
  SNDLOG"\n%s", s);
  line[0]++;

  s[0] = '\0';
  if (sdrplay3_parms.hw_version == SDRPLAY3_RSP2_ID) {
    if (sdrplay3_parms.antenna % 100 == sdrplay_api_Rsp2_ANTENNA_A) {
      if (sdrplay3_parms.antenna / 100 == sdrplay_api_Rsp2_AMPORT_2) {
        snprintf(s, 120, "Antenna = Antenna A");
      } else if (sdrplay3_parms.antenna / 100 == sdrplay_api_Rsp2_AMPORT_1) {
        snprintf(s, 120, "Antenna = Hi-Z");
      }
    } else if (sdrplay3_parms.antenna % 100 == sdrplay_api_Rsp2_ANTENNA_B) {
      if (sdrplay3_parms.antenna / 100 == sdrplay_api_Rsp2_AMPORT_2) {
        snprintf(s, 120, "Antenna = Antenna B");
      }
    }
  } else if (sdrplay3_parms.hw_version == SDRPLAY3_RSPduo_ID) {
    if (sdrplay3_parms.antenna % 100 == sdrplay_api_Tuner_A) {
      if (sdrplay3_parms.antenna / 100 == sdrplay_api_RspDuo_AMPORT_2) {
        snprintf(s, 120, "Antenna = Tuner 1 50ohm");
      } else if (sdrplay3_parms.antenna / 100 == sdrplay_api_RspDuo_AMPORT_1) {
        snprintf(s, 120, "Antenna = High Z");
      }
    } else if (sdrplay3_parms.antenna % 100 == sdrplay_api_Tuner_B) {
      snprintf(s, 120, "Antenna = Tuner 2 50ohm");
    } else if (sdrplay3_parms.antenna % 100 == sdrplay_api_Tuner_Both) {
      if (sdrplay3_parms.antenna / 100 == sdrplay_api_RspDuo_AMPORT_2) {
        snprintf(s, 120, "Antenna RX #1 = Tuner 1 50ohm, Antenna RX #2 = Tuner 2 50ohm");
      } else if (sdrplay3_parms.antenna / 100 == sdrplay_api_RspDuo_AMPORT_1) {
        snprintf(s, 120, "Antenna RX #1 = High Z, Antenna RX #2 = Tuner 2 50ohm");
      }
    }
  } else if (sdrplay3_parms.hw_version == SDRPLAY3_RSPdx_ID) {
    if (sdrplay3_parms.antenna == sdrplay_api_RspDx_ANTENNA_A) {
      snprintf(s, 120, "Antenna = Antenna A");
    } else if (sdrplay3_parms.antenna == sdrplay_api_RspDx_ANTENNA_B) {
      snprintf(s, 120, "Antenna = Antenna B");
    } else if (sdrplay3_parms.antenna == sdrplay_api_RspDx_ANTENNA_C) {
      snprintf(s, 120, "Antenna = Antenna C");
    }
  }
  if (s[0] != '\0') {
    lir_text(24, line[0], s);
    SNDLOG"\n%s", s);
    line[0]++;
  }

  if (sdrplay3_parms.hw_version == SDRPLAY3_RSPduo_ID && sdrplay3_parms.tuner == sdrplay_api_Tuner_Both) {
    sdrplay3_display_gains("RX #1", sdrplay3_parms.rf_lna_state % RX2_OFFSET,
                           sdrplay3_parms.if_agc % RX2_OFFSET,
                           sdrplay3_parms.if_gain_reduction % RX2_OFFSET,
                           sdrplay3_parms.if_agc_setpoint_dbfs % RX2_OFFSET,
                           sdrplay3_parms.if_agc_attack_ms % RX2_OFFSET,
                           sdrplay3_parms.if_agc_decay_ms % RX2_OFFSET,
                           sdrplay3_parms.if_agc_decay_delay_ms % RX2_OFFSET,
                           sdrplay3_parms.if_agc_decay_threshold_db % RX2_OFFSET,
                           line);
    sdrplay3_display_gains("RX #2", sdrplay3_parms.rf_lna_state / RX2_OFFSET,
                           sdrplay3_parms.if_agc / RX2_OFFSET,
                           sdrplay3_parms.if_gain_reduction / RX2_OFFSET,
                           sdrplay3_parms.if_agc_setpoint_dbfs / RX2_OFFSET,
                           sdrplay3_parms.if_agc_attack_ms / RX2_OFFSET,
                           sdrplay3_parms.if_agc_decay_ms / RX2_OFFSET,
                           sdrplay3_parms.if_agc_decay_delay_ms / RX2_OFFSET,
                           sdrplay3_parms.if_agc_decay_threshold_db / RX2_OFFSET,
                           line);
  } else {
    sdrplay3_display_gains("RX", sdrplay3_parms.rf_lna_state,
                           sdrplay3_parms.if_agc,
                           sdrplay3_parms.if_gain_reduction,
                           sdrplay3_parms.if_agc_setpoint_dbfs,
                           sdrplay3_parms.if_agc_attack_ms,
                           sdrplay3_parms.if_agc_decay_ms,
                           sdrplay3_parms.if_agc_decay_delay_ms,
                           sdrplay3_parms.if_agc_decay_threshold_db, line);
  }

  snprintf(s, 120, "Frequency Correction = %.3lf ppm", (double) sdrplay3_parms.freq_corr_ppb / 1000.0);
  lir_text(24, line[0], s);
  SNDLOG"\n%s", s);
  line[0]++;

  if (sdrplay3_parms.hw_version == SDRPLAY3_RSPduo_ID && sdrplay3_parms.tuner == sdrplay_api_Tuner_Both) {
    if (sdrplay3_parms.dc_offset_corr % RX2_OFFSET) {
      dc_offset_corr_mode_name = enabled_string;
    } else {
      dc_offset_corr_mode_name = disabled_string;
    }
    if (sdrplay3_parms.iq_imbalance_corr % RX2_OFFSET) {
      iq_imbalance_corr_mode_name = enabled_string;
    } else {
      iq_imbalance_corr_mode_name = disabled_string;
    }
    snprintf(s, 120, "RX #1 - DC Offset Correction = %s, IQ Imbalance Correction = %s", dc_offset_corr_mode_name, iq_imbalance_corr_mode_name);
    lir_text(24, line[0], s);
    SNDLOG"\n%s", s);
    line[0]++;

    dc_offset_corr_mode_name = 0;
    iq_imbalance_corr_mode_name = 0;
    if (sdrplay3_parms.dc_offset_corr / RX2_OFFSET) {
      dc_offset_corr_mode_name = enabled_string;
    } else {
      dc_offset_corr_mode_name = disabled_string;
    }
    if (sdrplay3_parms.iq_imbalance_corr / RX2_OFFSET) {
      iq_imbalance_corr_mode_name = enabled_string;
    } else {
      iq_imbalance_corr_mode_name = disabled_string;
    }
    snprintf(s, 120, "RX #2 - DC Offset Correction = %s, IQ Imbalance Correction = %s", dc_offset_corr_mode_name, iq_imbalance_corr_mode_name);
    lir_text(24, line[0], s);
    SNDLOG"\n%s", s);
    line[0]++;
  } else {
    if (sdrplay3_parms.dc_offset_corr) {
      dc_offset_corr_mode_name = enabled_string;
    } else {
      dc_offset_corr_mode_name = disabled_string;
    }
    if (sdrplay3_parms.iq_imbalance_corr) {
      iq_imbalance_corr_mode_name = enabled_string;
    } else {
      iq_imbalance_corr_mode_name = disabled_string;
    }
    snprintf(s, 120, "DC Offset Correction = %s, IQ Imbalance Correction = %s", dc_offset_corr_mode_name, iq_imbalance_corr_mode_name);
    lir_text(24, line[0], s);
    SNDLOG"\n%s", s);
    line[0]++;
  }

  if (sdrplay3_parms.usb_transfer_mode == sdrplay_api_ISOCH) {
    usb_transfer_mode_name = "Isochronous";
  } else if (sdrplay3_parms.usb_transfer_mode == sdrplay_api_BULK) {
    usb_transfer_mode_name = "Bulk";
  }
  snprintf(s, 120, "USB Transfer Mode = %s", usb_transfer_mode_name);
  lir_text(24, line[0], s);
  SNDLOG"\n%s", s);
  line[0]++;

  if (sdrplay3_parms.notch_filters % 100) {
    rf_notch_filter_mode_name = enabled_string;
  } else {
    rf_notch_filter_mode_name = disabled_string;
  }
  if ((sdrplay3_parms.notch_filters / 100) % 100) {
    rf_dab_notch_filter_mode_name = enabled_string;
  } else {
    rf_dab_notch_filter_mode_name = disabled_string;
  }
  s[0] = '\0';
  if (sdrplay3_parms.hw_version == SDRPLAY3_RSP1A_ID) {
    snprintf(s, 120, "RF Notch Filter = %s, DAB Notch Filter = %s", rf_notch_filter_mode_name, rf_dab_notch_filter_mode_name);
  } else if (sdrplay3_parms.hw_version == SDRPLAY3_RSP2_ID) {
    snprintf(s, 120, "RF Notch Filter = %s", rf_notch_filter_mode_name);
  } else if (sdrplay3_parms.hw_version == SDRPLAY3_RSPduo_ID) {
    if (sdrplay3_parms.tuner == sdrplay_api_Tuner_A) {
      if ((sdrplay3_parms.notch_filters / 10000) % 100) {
        am_notch_filter_mode_name = enabled_string;
      } else {
        am_notch_filter_mode_name = disabled_string;
      }
      snprintf(s, 120, "RF Notch Filter = %s, DAB Notch Filter = %s, AM Notch Filter = %s", rf_notch_filter_mode_name, rf_dab_notch_filter_mode_name, am_notch_filter_mode_name);
    } else if (sdrplay3_parms.tuner == sdrplay_api_Tuner_B) {
      snprintf(s, 120, "RF Notch Filter = %s, DAB Notch Filter = %s", rf_notch_filter_mode_name, rf_dab_notch_filter_mode_name);
    } else if (sdrplay3_parms.tuner == sdrplay_api_Tuner_Both) {
      if ((sdrplay3_parms.notch_filters / 10000) % 100) {
        am_notch_filter_mode_name = enabled_string;
      } else {
        am_notch_filter_mode_name = disabled_string;
      }
      snprintf(s, 120, "RX #1 - RF Notch Filter = %s, DAB Notch Filter = %s, AM Notch Filter = %s", rf_notch_filter_mode_name, rf_dab_notch_filter_mode_name, am_notch_filter_mode_name);
      lir_text(24, line[0], s);
      SNDLOG"\n%s", s);
      line[0]++;

      rf_notch_filter_mode_name = 0;
      rf_dab_notch_filter_mode_name = 0;
      if ((sdrplay3_parms.notch_filters / RX2_OFFSET / 100) % 100) {
        rf_notch_filter_mode_name = enabled_string;
      } else {
        rf_notch_filter_mode_name = disabled_string;
      }
      if ((sdrplay3_parms.notch_filters / RX2_OFFSET / 10000) % 100) {
        rf_dab_notch_filter_mode_name = enabled_string;
      } else {
        rf_dab_notch_filter_mode_name = disabled_string;
      }
      snprintf(s, 120, "RX #2 - RF Notch Filter = %s, DAB Notch Filter = %s", rf_notch_filter_mode_name, rf_dab_notch_filter_mode_name);
    }
  } else if (sdrplay3_parms.hw_version == SDRPLAY3_RSPdx_ID) {
    snprintf(s, 120, "RF Notch Filter = %s, DAB Notch Filter = %s", rf_notch_filter_mode_name, rf_dab_notch_filter_mode_name);
  }
  if (s[0] != '\0') {
    lir_text(24, line[0], s);
    SNDLOG"\n%s", s);
    line[0]++;
  }

  s[0] = '\0';
  if (sdrplay3_parms.bias_t % 100) {
    bias_t_mode_name = enabled_string;
  } else {
    bias_t_mode_name = disabled_string;
  }
  if (sdrplay3_parms.hw_version == SDRPLAY3_RSP1A_ID) {
    snprintf(s, 120, "Bias-T = %s", bias_t_mode_name);
  } else if (sdrplay3_parms.hw_version == SDRPLAY3_RSP2_ID) {
    snprintf(s, 120, "Bias-T = %s", bias_t_mode_name);
  } else if (sdrplay3_parms.hw_version == SDRPLAY3_RSPduo_ID) {
    if (sdrplay3_parms.tuner == sdrplay_api_Tuner_B) {
      snprintf(s, 120, "Bias-T = %s", bias_t_mode_name);
    } else if (sdrplay3_parms.tuner == sdrplay_api_Tuner_Both) {
      bias_t_mode_name = 0;
      if ((sdrplay3_parms.bias_t / RX2_OFFSET) % 100) {
        bias_t_mode_name = enabled_string;
      } else {
        bias_t_mode_name = disabled_string;
      }
      snprintf(s, 120, "RX #2 Bias-T = %s", bias_t_mode_name);
    }
  } else if (sdrplay3_parms.hw_version == SDRPLAY3_RSPdx_ID) {
    snprintf(s, 120, "Bias-T = %s", bias_t_mode_name);
  }
  if (s[0] != '\0') {
    lir_text(24, line[0], s);
    SNDLOG"\n%s", s);
    line[0]++;
  }

  if (sdrplay3_parms.hw_version == SDRPLAY3_RSPdx_ID) {
    if (sdrplay3_parms.rspdx_hdr_mode_and_bandwidth == -1) {
      snprintf(s, 120, "RSPdx HDR Mode = %s", disabled_string);
    } else {
      hdr_bandwidth = 0;
      if (sdrplay3_parms.rspdx_hdr_mode_and_bandwidth == sdrplay_api_RspDx_HDRMODE_BW_0_200) {
        hdr_bandwidth = 200;
      } else if (sdrplay3_parms.rspdx_hdr_mode_and_bandwidth == sdrplay_api_RspDx_HDRMODE_BW_0_500) {
        hdr_bandwidth = 500;
      } else if (sdrplay3_parms.rspdx_hdr_mode_and_bandwidth == sdrplay_api_RspDx_HDRMODE_BW_1_200) {
        hdr_bandwidth = 1200;
      } else if (sdrplay3_parms.rspdx_hdr_mode_and_bandwidth == sdrplay_api_RspDx_HDRMODE_BW_1_700) {
        hdr_bandwidth = 1700;
      }
      snprintf(s, 120, "RSPdx HDR Mode = %s, HDR Bandwidth = %d kHz", enabled_string, hdr_bandwidth);
    }
    lir_text(24, line[0], s);
    SNDLOG"\n%s", s);
    line[0]++;
  }

  SNDLOG"\n");
  line[0]++;
  return errcod;
}
