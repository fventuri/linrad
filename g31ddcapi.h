#ifndef __G31DDCAPI_H__
#define __G31DDCAPI_H__

#ifdef WIN32_LEAN_AND_MEAN

#include <basetyps.h>

#endif

#include <windows.h>

#define G31DDC_CLASS_ID_DEVICE_ENUMERATOR  0x7101
#define G31DDC_CLASS_ID_DEVICE             0x7102

#ifndef __G3XDDC_INTERFACE__
#define __G3XDDC_INTERFACE__

#define G3XDDC_INTERFACE_TYPE_PCIE    0
#define G3XDDC_INTERFACE_TYPE_USB     1

#endif //__G3XDDC_INTERFACE__

#ifndef __G3XDDC_FRONT_PANEL_LED__
#define __G3XDDC_FRONT_PANEL_LED__ 1

#define G3XDDC_FRONT_PANEL_LED_MODE_DIAG    0
#define G3XDDC_FRONT_PANEL_LED_MODE_ON      1
#define G3XDDC_FRONT_PANEL_LED_MODE_OFF     2

#endif //__G3XDDC_FRONT_PANEL_LED__

#ifndef __G3XDDC_OPEN__
#define __G3XDDC_OPEN__ 1

#define G3XDDC_OPEN_DEMO   ((CHAR*)0x00000000)
#define G3XDDC_OPEN_FIRST  ((CHAR*)0xFFFFFFFF)

#endif //__G3XDDC_OPEN__

#if !defined(__G3XDDC_MODES__) || __G3XDDC_MODES__<1

#undef __G3XDDC_MODES__
#define __G3XDDC_MODES__ 1

#define G3XDDC_MODE_CW               0
#define G3XDDC_MODE_AM               1
#define G3XDDC_MODE_FM               2
#define G3XDDC_MODE_LSB              4
#define G3XDDC_MODE_USB              5
#define G3XDDC_MODE_AMS              8
#define G3XDDC_MODE_DRM              18

#endif //__G3XDDC_MODES__

#define G31DDC_MODE_COUNT            7



#ifndef __G3XDDC_SIDE_BAND__
#define __G3XDDC_SIDE_BAND__ 1

#define G3XDDC_SIDE_BAND_LOWER         0x01
#define G3XDDC_SIDE_BAND_UPPER         0x02
#define G3XDDC_SIDE_BAND_BOTH          (G3XDDC_SIDE_BAND_LOWER|G3XDDC_SIDE_BAND_UPPER)

#endif //__G3XDDC_SIDE_BAND__



#if !defined(__G3XDDC_DEMODULATOR_PARAMS__) || __G3XDDC_DEMODULATOR_PARAMS__<1

#undef __G3XDDC_DEMODULATOR_PARAMS__
#define __G3XDDC_DEMODULATOR_PARAMS__ 1

#define G3XDDC_DEMODULATOR_PARAM_AMS_SIDE_BAND          0x00000001
#define G3XDDC_DEMODULATOR_PARAM_AMS_CAPTURE_RANGE      0x00000002
#define G3XDDC_DEMODULATOR_PARAM_CW_FREQUENCY           0x00000003
#define G3XDDC_DEMODULATOR_PARAM_DRM_AUDIO_SERVICE      0x00000004
#define G3XDDC_DEMODULATOR_PARAM_DRM_MULTIMEDIA_SERVICE 0x00000005

#pragma pack(push,1)

typedef struct
{
    UINT32  Tune;   //Hz
    UINT32  Lock;   //Hz
} G3XDDC_AMS_CAPTURE_RANGE;

#pragma pack(pop)

#endif //__G3XDDC_DEMODULATOR_PARAMS__



#if !defined(__G3XDDC_DEMODULATOR_STATES__) || __G3XDDC_DEMODULATOR_STATES__<1

#undef __G3XDDC_DEMODULATOR_STATES__
#define __G3XDDC_DEMODULATOR_STATES__ 1

#define G3XDDC_DEMODULATOR_STATE_AMS_LOCK               0x00000001
#define G3XDDC_DEMODULATOR_STATE_AMS_FREQUENCY          0x00000002
#define G3XDDC_DEMODULATOR_STATE_AM_DEPTH               0x00000003
#define G3XDDC_DEMODULATOR_STATE_TUNE_ERROR             0x00000004
#define G3XDDC_DEMODULATOR_STATE_DRM_STATUS             0x00000005
#define G3XDDC_DEMODULATOR_STATE_FM_DEVIATION           0x00000006

#endif //__G3XDDC_DEMODULATOR_STATES__



#ifndef __G3XDDC_DRM__
#define __G3XDDC_DRM__ 1

#define G3XDDC_DRM_STATE_MODE_NOT_DETERMINED_YET       -1
#define G3XDDC_DRM_STATE_MODE_A                        0
#define G3XDDC_DRM_STATE_MODE_B                        1
#define G3XDDC_DRM_STATE_MODE_C                        2
#define G3XDDC_DRM_STATE_MODE_D                        3

#define G3XDDC_DRM_STATE_INTERLEAVER_LONG              0       //long interleaver used (2 sec)
#define G3XDDC_DRM_STATE_INTERLEAVER_SHORT             1       //short interleaver used (400 msec)

#define G3XDDC_DRM_STATE_QAM_TYPE_STD                  0       //standard
#define G3XDDC_DRM_STATE_QAM_TYPE_HIER_SYM             1       //hierarchical symmetrical
#define G3XDDC_DRM_STATE_QAM_TYPE_HIER_MIX             2       //hierarchical mixed

#define G3XDDC_DRM_STATE_SERVICE_CONTENT_EMPTY         0x00    //service is not used/contains no data
#define G3XDDC_DRM_STATE_SERVICE_CONTENT_AUDIO         0x01    //service contains audio data
#define G3XDDC_DRM_STATE_SERVICE_CONTENT_TEXTMSG       0x02    //service contains text messages
#define G3XDDC_DRM_STATE_SERVICE_CONTENT_MULTIMEDIA    0x04    //service contains multimedia data
#define G3XDDC_DRM_STATE_SERVICE_CONTENT_DATA          0x08    //service contains application specific data

#define G3XDDC_DRM_STATE_AUDIO_CODING_AAC              0       //audio coding is AAC
#define G3XDDC_DRM_STATE_AUDIO_CODING_CELP             1       //audio coding is CELP
#define G3XDDC_DRM_STATE_AUDIO_CODING_HVXC             2       //audio coding is HVXC
#define G3XDDC_DRM_STATE_AUDIO_CODING_RFU              3       //reserved for future use

#define G3XDDC_DRM_STATE_AUDIO_MODE_AAC_MONO           0       //mono
#define G3XDDC_DRM_STATE_AUDIO_MODE_AAC_PARAM_STEREO   1       //parametric stereo
#define G3XDDC_DRM_STATE_AUDIO_MODE_AAC_STEREO         2       //stereo
#define G3XDDC_DRM_STATE_AUDIO_MODE_AAC_RFU            3       //reserved for future use
	
#define G3XDDC_DRM_STATE_AUDIO_MODE_CELP_NO_CRC        0       //audio data is without CRC
#define G3XDDC_DRM_STATE_AUDIO_MODE_CELP_CRC           1       //CRC used
#define G3XDDC_DRM_STATE_AUDIO_MODE_CELP_RFU_10        2       //reserved for future use
#define G3XDDC_DRM_STATE_AUDIO_MODE_CELP_RFU_11        3       //reserved for future use
	
#define G3XDDC_DRM_STATE_AUDIO_MODE_HVXC_RFU_00        0       //reserved for future use
#define G3XDDC_DRM_STATE_AUDIO_MODE_HVXC_RFU_01        1       //reserved for future use
#define G3XDDC_DRM_STATE_AUDIO_MODE_HVXC_RFU_10        2       //reserved for future use
#define G3XDDC_DRM_STATE_AUDIO_MODE_HVXC_RFU_11        3       //reserved for future use

#pragma pack(push,1)

typedef struct
{
    BOOL            Valid;                          //entries below are invalid/valid

    struct
    {
        BOOL        SyncFound;
        BOOL        FACDecoded;
        BOOL        SDCDecoded;
        BOOL        AudioDecoded;
        SHORT       NumberOfAudioFrames;            //-1 if not available
        SHORT       NumberOfAudioErrors;            //-1 if not available	
    } DecodingState;


    INT32           Mode;                           //G3XDDC_DRM_STATE_MODE_*
	double          RFBandwidth;                    //occupied RF bandwith in kHz, 0 = invalid or information not (yet) available
    BYTE            Interleaver;                    //G3XDDC_DRM_STATE_INTERLEAVER_*
    SHORT           SDCQam;                         //coding of the SDC (QAM), 0 = not (yet) available
    SHORT           MSCQam;                         //coding of the MSC (QAM), 0 = not (yet) available
    BYTE            MSCQamType;                     //G3XDDC_DRM_STATE_QAM_TYPE_* , used QAM coding for the MSC
    double          CoderateH;				        //used coderate for hierachical coding,  values <= 0 indicate not available or not used
    double          CoderateA;				        //used coderate for protection level A,  values <= 0 indicate not available or not used
    double          CoderateB;				        //used coderate for protection level B,  values <= 0 indicate not available or not used
    double          EstimatedSNR;			        //estimated SNR in dB of the decoded signal
    WCHAR           TextMessage[128 + 1 + 16];      //text message containing max. 128 byte text + string termination + max. 16 linebreaks
   
    struct
    {
        BYTE        Content;				        //G3XDDC_DRM_STATE_SERVICE_CONTENT_*
        WCHAR       DynamicLabel[256];		        //string containing dynamic label of the service, zero terminated
        WCHAR       Country[256];			        //string containing the signaled country for this service
        WCHAR       Language[256];			        //string containing the signaled language for this service
        WCHAR       ProgramType[256];		        //string containing the signaled program type for this service
        double      AudioBitrate;			        //data rate for the audio content (0 if not available)
        double      TextMsgBitrate;			        //data rate for the text message content (0 if not available)
        double      MultimediaBitrate;		        //data rate for the multimedia content (0 if not available)
        double      DataBitrate;			        //data rate for the data content (0 if not available)
    } ServiceInfo[4];

    struct
    {
        BOOL        Valid;                          //audio decoder information valid/invalid, when invalid all other entries of the struct contain no valid data
        BYTE        AudioCoding;                    //G3XDDC_DRM_STATE_AUDIO_CODING_*        
        BOOL        SBR;                            //SBR used/not used (TRUE/FALSE)
        INT32       AudioMode;                      //depeding on the audio coding, DRM_STATE_AUDIO_MODE_*
    } AudioDecoderInfo[4];
} G3XDDC_DRM_STATUS;

#pragma pack(pop)

#endif //__G3XDDC_DRM__


#ifndef __G3XDDC_CREATE_INSTANCE__
#define __G3XDDC_CREATE_INSTANCE__ 1

typedef BOOL (__stdcall *G3XDDCAPI_CREATE_INSTANCE)(UINT32 ClassId,PVOID *Interface);

#endif //__G3XDDC_CREATE_INSTANCE__


#pragma pack(push,1)

typedef struct
{
    CHAR        DevicePath[MAX_PATH];
    BYTE        InterfaceType;
    CHAR        SerialNumber[9];
    UINT32      ChannelCount;
    UINT32      DDCTypeCount;
} G31DDC_DEVICE_INFO;


#ifndef __G3XDDC_DDC_INFO__
#define __G3XDDC_DDC_INFO__ 1

typedef struct
{
    UINT32      SampleRate;
    UINT32      Bandwidth;
    UINT32      BitsPerSample;
} G3XDDC_DDC_INFO;

#endif //__G3XDDC_DDC_INFO__

#undef INTERFACE
#define INTERFACE IG31DDCDeviceEnumerator

DECLARE_INTERFACE(IG31DDCDeviceEnumerator)
{
    STDMETHOD_(LONG,AddRef)(THIS) PURE;
    STDMETHOD_(LONG,Release)(THIS) PURE;

    STDMETHOD_(BOOL,Enumerate)(THIS) PURE;
    STDMETHOD_(UINT32,GetCount)(THIS) PURE;
    STDMETHOD_(BOOL,GetDeviceInfo)(THIS_ UINT32 DeviceIndex,G31DDC_DEVICE_INFO *DeviceInfo,UINT32 BufferLength) PURE;
};

#undef INTERFACE
#define INTERFACE IG31DDCDeviceCallback

struct IG31DDCDevice;

DECLARE_INTERFACE(IG31DDCDeviceCallback)
{
    STDMETHOD_(VOID,G31DDC_IFCallback)(THIS_ struct IG31DDCDevice *Device,CONST SHORT *Buffer,UINT32 NumberOfSamples,WORD MaxADCAmplitude,UINT32 ADCSamplingRate) PURE;
    STDMETHOD_(VOID,G31DDC_DDC1StreamCallback)(THIS_ struct IG31DDCDevice *Device,CONST VOID *Buffer,UINT32 NumberOfSamples,UINT32 BitsPerSample) PURE;
    STDMETHOD_(BOOL,G31DDC_DDC1PlaybackStreamCallback)(THIS_ struct IG31DDCDevice *Device,VOID *Buffer,UINT32 NumberOfSamples,UINT32 BitsPerSample) PURE;
    STDMETHOD_(VOID,G31DDC_DDC2StreamCallback)(THIS_ struct IG31DDCDevice *Device,UINT32 Channel,CONST FLOAT *Buffer,UINT32 NumberOfSamples) PURE;        
    //SlevelPeak and SlevelRMS are in Volts
    //to convert SlevelRMS to power in dBm use SlevelRMS_dBm=10.0*log10(SlevelRMS*SlevelRMS/50.0*1000.0); where 50.0 is receiver input impendance, 1000.0 is coefficents to convert Watts to mili Watts
    STDMETHOD_(VOID,G31DDC_DDC2PreprocessedStreamCallback)(THIS_ struct IG31DDCDevice *Device,UINT32 Channel,CONST FLOAT *Buffer,UINT32 NumberOfSamples,FLOAT SlevelPeak,FLOAT SlevelRMS) PURE;        
    STDMETHOD_(VOID,G31DDC_AudioStreamCallback)(THIS_ struct IG31DDCDevice *Device,UINT32 Channel,CONST FLOAT *Buffer,CONST FLOAT *BufferFiltered,UINT32 NumberOfSamples) PURE;
    STDMETHOD_(BOOL,G31DDC_AudioPlaybackStreamCallback)(THIS_ struct IG31DDCDevice *Device,UINT32 Channel,FLOAT *Buffer,UINT32 NumberOfSamples) PURE;
};

#undef INTERFACE
#define INTERFACE IG31DDCDevice


DECLARE_INTERFACE(IG31DDCDevice)
{
    STDMETHOD_(LONG,AddRef)(THIS) PURE;
    STDMETHOD_(LONG,Release)(THIS) PURE;

    STDMETHOD_(BOOL,Open)(THIS_ CONST CHAR *DevicePath) PURE;
    STDMETHOD_(VOID,Close)(THIS) PURE;
    STDMETHOD_(BOOL,IsOpen)(THIS) PURE;

    STDMETHOD_(BOOL,IsConnected)(THIS) PURE;

    STDMETHOD_(VOID,SetCallback)(THIS_ IG31DDCDeviceCallback *Callback) PURE;
    STDMETHOD_(IG31DDCDeviceCallback*,GetCallback)(THIS) PURE;

    STDMETHOD_(BOOL,GetDeviceInfo)(THIS_ G31DDC_DEVICE_INFO *Info,UINT32 BufferLength) PURE;

    STDMETHOD_(BOOL,SetPower)(THIS_ BOOL Power) PURE;
    STDMETHOD_(BOOL,GetPower)(THIS_ BOOL *Power) PURE;

    STDMETHOD_(BOOL,SetAttenuator)(THIS_ UINT32 Attenuator) PURE;
    STDMETHOD_(BOOL,GetAttenuator)(THIS_ UINT32 *Attenuator) PURE;

    STDMETHOD_(BOOL,SetDithering)(THIS_ BOOL Enabled) PURE;
    STDMETHOD_(BOOL,GetDithering)(THIS_ BOOL *Enabled) PURE;    

    STDMETHOD_(BOOL,SetMWFilter)(THIS_ BOOL Enabled) PURE;
    STDMETHOD_(BOOL,GetMWFilter)(THIS_ BOOL *Enabled) PURE;

    STDMETHOD_(BOOL,SetLED)(THIS_ UINT32 LEDMode) PURE;
    STDMETHOD_(BOOL,GetLED)(THIS_ UINT32 *LEDMode) PURE;

    STDMETHOD_(BOOL,SetInverted)(THIS_ BOOL Inverted) PURE;
    STDMETHOD_(BOOL,GetInverted)(THIS_ BOOL *Inverted) PURE;        

    STDMETHOD_(BOOL,SetADCNoiseBlanker)(THIS_ BOOL Enabled) PURE;
    STDMETHOD_(BOOL,GetADCNoiseBlanker)(THIS_ BOOL *Enabled) PURE;
    STDMETHOD_(BOOL,SetADCNoiseBlankerThreshold)(THIS_ UINT16 Enabled) PURE;
    STDMETHOD_(BOOL,GetADCNoiseBlankerThreshold)(THIS_ UINT16 *Enabled) PURE;

    STDMETHOD_(BOOL,StartIF)(THIS_ WORD Period) PURE;
    STDMETHOD_(BOOL,StopIF)(THIS) PURE;

    STDMETHOD_(BOOL,GetDDCInfo)(THIS_ UINT32 DDCTypeIndex,G3XDDC_DDC_INFO *Info) PURE;
    STDMETHOD_(BOOL,GetDDC1Count)(THIS_ UINT32 *Count) PURE;
    STDMETHOD_(BOOL,SetDDC1)(THIS_ UINT32 DDCTypeIndex) PURE;
    STDMETHOD_(BOOL,GetDDC1)(THIS_ UINT32 *DDCTypeIndex,G3XDDC_DDC_INFO *DDCInfo) PURE;

    STDMETHOD_(BOOL,StartDDC1)(THIS_ UINT32 SamplesPerBuffer) PURE;
    STDMETHOD_(BOOL,StopDDC1)(THIS) PURE;
    STDMETHOD_(BOOL,StartDDC1Playback)(THIS_ UINT32 SamplesPerBuffer,UINT32 BitsPerSample) PURE; //BitsPerSample: 0 - default by DDC Info, 16 or 32
    STDMETHOD_(BOOL,PauseDDC1Playback)(THIS) PURE;
    STDMETHOD_(BOOL,ResumeDDC1Playback)(THIS) PURE;   

    STDMETHOD_(BOOL,SetDDC1Frequency)(THIS_ UINT32 Frequency) PURE;
    STDMETHOD_(BOOL,GetDDC1Frequency)(THIS_ UINT32 *Frequency) PURE;

    STDMETHOD_(BOOL,GetDDC2)(THIS_ UINT32 *DDCTypeIndex,G3XDDC_DDC_INFO *DDCInfo) PURE;
    STDMETHOD_(BOOL,StartDDC2)(THIS_ UINT32 Channel,UINT32 SamplesPerBuffer) PURE;
    STDMETHOD_(BOOL,StopDDC2)(THIS_ UINT32 Channel) PURE;

    STDMETHOD_(BOOL,SetDDC2Frequency)(THIS_ UINT32 Channel,INT32 Frequency) PURE;
    STDMETHOD_(BOOL,GetDDC2Frequency)(THIS_ UINT32 Channel,INT32 *Frequency) PURE;

    STDMETHOD_(BOOL,SetDDC2NoiseBlanker)(THIS_ UINT32 Channel,BOOL Enabled) PURE;
    STDMETHOD_(BOOL,GetDDC2NoiseBlanker)(THIS_ UINT32 Channel,BOOL *Enabled) PURE;
    STDMETHOD_(BOOL,SetDDC2NoiseBlankerThreshold)(THIS_ UINT32 Channel,double Threshold) PURE;
    STDMETHOD_(BOOL,GetDDC2NoiseBlankerThreshold)(THIS_ UINT32 Channel,double *Threshold) PURE;
    STDMETHOD_(BOOL,GetDDC2NoiseBlankerExcessValue)(THIS_ UINT32 Channel,double *Value) PURE;

    STDMETHOD_(BOOL,GetSignalLevel)(THIS_ UINT32 Channel,FLOAT *Peak,FLOAT *RMS) PURE;

    STDMETHOD_(BOOL,SetAGC)(THIS_ UINT32 Channel,BOOL Enabled) PURE;
    STDMETHOD_(BOOL,GetAGC)(THIS_ UINT32 Channel,BOOL *Enabled) PURE;
    STDMETHOD_(BOOL,SetAGCParams)(THIS_ UINT32 Channel,double AttackTime,double DecayTime,double ReferenceLevel) PURE;
    STDMETHOD_(BOOL,GetAGCParams)(THIS_ UINT32 Channel,double *AttackTime,double *DecayTime,double *ReferenceLevel) PURE;
    STDMETHOD_(BOOL,SetMaxAGCGain)(THIS_ UINT32 Channel,double MaxGain) PURE;
    STDMETHOD_(BOOL,GetMaxAGCGain)(THIS_ UINT32 Channel,double *MaxGain) PURE;
    STDMETHOD_(BOOL,SetGain)(THIS_ UINT32 Channel,double Gain) PURE;
    STDMETHOD_(BOOL,GetGain)(THIS_ UINT32 Channel,double *Gain) PURE;
    STDMETHOD_(BOOL,GetCurrentGain)(THIS_ UINT32 Channel,double *Gain) PURE;

    STDMETHOD_(BOOL,SetNotchFilter)(THIS_ UINT32 Channel,BOOL Enabled) PURE;
    STDMETHOD_(BOOL,GetNotchFilter)(THIS_ UINT32 Channel,BOOL *Enabled) PURE;
    STDMETHOD_(BOOL,SetNotchFilterFrequency)(THIS_ UINT32 Channel,INT32 Frequency) PURE;
    STDMETHOD_(BOOL,GetNotchFilterFrequency)(THIS_ UINT32 Channel,INT32 *Frequency) PURE;
    STDMETHOD_(BOOL,SetNotchFilterBandwidth)(THIS_ UINT32 Channel,UINT32 Bandwidth) PURE;
    STDMETHOD_(BOOL,GetNotchFilterBandwidth)(THIS_ UINT32 Channel,UINT32 *Bandwidth) PURE;
    STDMETHOD_(BOOL,SetNotchFilterLength)(THIS_ UINT32 Channel,UINT32 Length) PURE;         //Length: min=64,max=32768,step=64
    STDMETHOD_(BOOL,GetNotchFilterLength)(THIS_ UINT32 Channel,UINT32 *Length) PURE;


    STDMETHOD_(BOOL,SetDemodulatorMode)(THIS_ UINT32 Channel,UINT32 Mode) PURE;
    STDMETHOD_(BOOL,GetDemodulatorMode)(THIS_ UINT32 Channel,UINT32 *Mode) PURE;
    STDMETHOD_(BOOL,SetDemodulatorFrequency)(THIS_ UINT32 Channel,INT32 Frequency) PURE;
    STDMETHOD_(BOOL,GetDemodulatorFrequency)(THIS_ UINT32 Channel,INT32 *Frequency) PURE;
    STDMETHOD_(BOOL,SetDemodulatorFilterBandwidth)(THIS_ UINT32 Channel,UINT32 Bandwidth) PURE;
    STDMETHOD_(BOOL,GetDemodulatorFilterBandwidth)(THIS_ UINT32 Channel,UINT32 *Bandwidth) PURE;
    STDMETHOD_(BOOL,SetDemodulatorFilterShift)(THIS_ UINT32 Channel,INT32 Shift) PURE;
    STDMETHOD_(BOOL,GetDemodulatorFilterShift)(THIS_ UINT32 Channel,INT32 *Shift) PURE;
    STDMETHOD_(BOOL,SetDemodulatorFilterLength)(THIS_ UINT32 Channel,UINT32 Length) PURE;   //Length: min=64,max=32768,step=64
    STDMETHOD_(BOOL,GetDemodulatorFilterLength)(THIS_ UINT32 Channel,UINT32 *Length) PURE;
    STDMETHOD_(BOOL,SetDemodulatorParam)(THIS_ UINT32 Channel,UINT32 Code,CONST VOID *Buffer,UINT32 BufferSize) PURE;
    STDMETHOD_(BOOL,GetDemodulatorParam)(THIS_ UINT32 Channel,UINT32 Code,VOID *Buffer,UINT32 BufferSize) PURE;
    STDMETHOD_(BOOL,GetDemodulatorState)(THIS_ UINT32 Channel,UINT32 Code,VOID *Buffer,UINT32 BufferSize) PURE;  

    STDMETHOD_(BOOL,StartAudio)(THIS_ UINT32 Channel,UINT32 SamplesPerBuffer) PURE;
    STDMETHOD_(BOOL,StopAudio)(THIS_ UINT32 Channel) PURE;
    STDMETHOD_(BOOL,StartAudioPlayback)(THIS_ UINT32 Channel,UINT32 SamplesPerBuffer) PURE;
    STDMETHOD_(BOOL,PauseAudioPlayback)(THIS_ UINT32 Channel) PURE;
    STDMETHOD_(BOOL,ResumeAudioPlayback)(THIS_ UINT32 Channel) PURE;
    STDMETHOD_(BOOL,SetAudioFilter)(THIS_ UINT32 Channel,BOOL Enabled) PURE;
    STDMETHOD_(BOOL,GetAudioFilter)(THIS_ UINT32 Channel,BOOL *Enabled) PURE;
    STDMETHOD_(BOOL,SetAudioFilterParams)(THIS_ UINT32 Channel,UINT32 CutOffLow,UINT32 CutOffHigh,double Deemphasis) PURE;    
    STDMETHOD_(BOOL,GetAudioFilterParams)(THIS_ UINT32 Channel,UINT32 *CutOffLow,UINT32 *CutOffHigh,double *Deemphasis) PURE;    
    STDMETHOD_(BOOL,SetAudioFilterLength)(THIS_ UINT32 Channel,UINT32 Length) PURE;         //Length: min=64,max=32768,step64
    STDMETHOD_(BOOL,GetAudioFilterLength)(THIS_ UINT32 Channel,UINT32 *Length) PURE;
    STDMETHOD_(BOOL,SetAudioGain)(THIS_ UINT32 Channel,double Gain) PURE;
    STDMETHOD_(BOOL,GetAudioGain)(THIS_ UINT32 Channel,double *Gain) PURE;  

    STDMETHOD_(BOOL,SetDRMKey)(THIS_ CONST CHAR *DRMKeyFileDirectory) PURE;
    STDMETHOD_(BOOL,IsDRMUnlocked)(THIS) PURE;

    STDMETHOD_(BOOL,GetSpectrumCompensation)(THIS_ UINT64 CenterFrequency,UINT32 Width,FLOAT *Buffer,UINT32 Count) PURE;

    STDMETHOD_(BOOL,SetFrequency)(THIS_ UINT32 Channel,UINT32 Frequency) PURE;
    STDMETHOD_(BOOL,GetFrequency)(THIS_ UINT32 Channel,UINT32 *Frequency) PURE;
};

#pragma pack(pop)

//non-object API

typedef VOID (__stdcall *G31DDC_IF_CALLBACK)(CONST SHORT *Buffer,UINT32 NumberOfSamples,WORD MaxADCAmplitude,UINT32 ADCSamplingRate,DWORD_PTR UserData);
typedef VOID (__stdcall *G31DDC_DDC1_STREAM_CALLBACK)(CONST VOID *Buffer,UINT32 NumberOfSamples,UINT32 BitsPerSample,DWORD_PTR UserData);
typedef BOOL (__stdcall *G31DDC_DDC1_PLAYBACK_STREAM_CALLBACK)(VOID *Buffer,UINT32 NumberOfSamples,UINT32 BitsPerSample,DWORD_PTR UserData);
typedef VOID (__stdcall *G31DDC_DDC2_STREAM_CALLBACK)(UINT32 Channel,CONST FLOAT *Buffer,UINT32 NumberOfSamples,DWORD_PTR UserData);
typedef VOID (__stdcall *G31DDC_DDC2_PREPROCESSED_STREAM_CALLBACK)(UINT32 Channel,CONST FLOAT *Buffer,UINT32 NumberOfSamples,FLOAT SlevelPeak,FLOAT SlevelRMS,DWORD_PTR UserData);
typedef VOID (__stdcall *G31DDC_AUDIO_STREAM_CALLBACK)(UINT32 Channel,CONST FLOAT *Buffer,CONST FLOAT *BufferFiltered,UINT32 NumberOfSamples,DWORD_PTR UserData);
typedef BOOL (__stdcall *G31DDC_AUDIO_PLAYBACK_STREAM_CALLBACK)(UINT32 Channel,FLOAT *Buffer,UINT32 NumberOfSamples,DWORD_PTR UserData);

#pragma pack(push,1)

typedef struct
{
    G31DDC_IF_CALLBACK                          IFCallback;
    G31DDC_DDC1_STREAM_CALLBACK                 DDC1StreamCallback;
    G31DDC_DDC1_PLAYBACK_STREAM_CALLBACK        DDC1PlaybackStreamCallback;
    G31DDC_DDC2_STREAM_CALLBACK                 DDC2StreamCallback;
    G31DDC_DDC2_PREPROCESSED_STREAM_CALLBACK    DDC2PreprocessedStreamCallback;
    G31DDC_AUDIO_STREAM_CALLBACK                AudioStreamCallback;
    G31DDC_AUDIO_PLAYBACK_STREAM_CALLBACK       AudioPlaybackStreamCallback;
} G31DDC_CALLBACKS;

#pragma pack(pop)

//GetDeviceList
typedef INT32 (__stdcall *G31DDC_GET_DEVICE_LIST)(G31DDC_DEVICE_INFO *DeviceList,UINT32 BufferSize);
//OpenDevice
typedef INT32(__stdcall *G31DDC_OPEN_DEVICE)(CONST CHAR *SerialNumber);
//CloseDevice
typedef BOOL (__stdcall *G31DDC_CLOSE_DEVICE)(INT32 hDevice);
//IsDeviceConnected
typedef BOOL (__stdcall *G31DDC_IS_DEVICE_CONNECTED)(INT32 hDevice);
//GetDeviceInfo
typedef BOOL (__stdcall *G31DDC_GET_DEVICE_INFO)(INT32 hDevice,G31DDC_DEVICE_INFO *Info,UINT32 BufferLength);
//SetPower
typedef BOOL (__stdcall *G31DDC_SET_POWER)(INT32 hDevice,BOOL Power);
//GetPower
typedef BOOL (__stdcall *G31DDC_GET_POWER)(INT32 hDevice,BOOL *Power);
//SetAttenuator
typedef BOOL (__stdcall *G31DDC_SET_ATTENUATOR)(INT32 hDevice,UINT32 Attenuator);
//GetAttenuator
typedef BOOL (__stdcall *G31DDC_GET_ATTENUATOR)(INT32 hDevice,UINT32 *Attenuator);
//SetDithering
typedef BOOL (__stdcall *G31DDC_SET_DITHERING)(INT32 hDevice,BOOL Enabled);
//GetDithering
typedef BOOL (__stdcall *G31DDC_GET_DITHERING)(INT32 hDevice,BOOL *Enabled);
//SetMWFilter
typedef BOOL (__stdcall *G31DDC_SET_MW_FILTER)(INT32 hDevice,BOOL Enabled);
//GetMWFilter
typedef BOOL (__stdcall *G31DDC_GET_MW_FILTER)(INT32 hDevice,BOOL *Enabled);
//SetADCNoiseBlanker
typedef BOOL (__stdcall *G31DDC_SET_ADC_NOISE_BLANKER)(INT32 hDevice,BOOL Enabled);
//GetADCNoiseBlanker
typedef BOOL (__stdcall *G31DDC_GET_ADC_NOISE_BLANKER)(INT32 hDevice,BOOL *Enabled);
//SetADCNoiseBlankerThreshold
typedef BOOL (__stdcall *G31DDC_SET_ADC_NOISE_BLANKER_THRESHOLD)(INT32 hDevice,WORD Threshold);
//GetADCNoiseBlankerThreshold
typedef BOOL (__stdcall *G31DDC_GET_ADC_NOISE_BLANKER_THRESHOLD)(INT32 hDevice,WORD *Threshold);
//SetLED
typedef BOOL (__stdcall *G31DDC_SET_LED)(INT32 hDevice,UINT32 LEDMode);
//GetLED
typedef BOOL (__stdcall *G31DDC_GET_LED)(INT32 hDevice,UINT32 *LEDMode);
//SetInverted    
typedef BOOL (__stdcall *G31DDC_SET_INVERTED)(INT32 hDevice,BOOL Inverted);
//GetInverted    
typedef BOOL (__stdcall *G31DDC_GET_INVERTED)(INT32 hDevice,BOOL *Inverted);
//SetDDC2NoiseBlanker
typedef BOOL (__stdcall *G31DDC_SET_DDC2_NOISE_BLANKER)(INT32 hDevice,UINT32 Channel,BOOL Enabled);
//GetDDC2NoiseBlanker
typedef BOOL (__stdcall *G31DDC_GET_DDC2_NOISE_BLANKER)(INT32 hDevice,UINT32 Channel,BOOL *Enabled);
//SetDDC2NoiseBlankerThreshold
typedef BOOL (__stdcall *G31DDC_SET_DDC2_NOISE_BLANKER_THRESHOLD)(INT32 hDevice,UINT32 Channel,double Threshold);
//GetDDC2NoiseBlankerThreshold
typedef BOOL (__stdcall *G31DDC_GET_DDC2_NOISE_BLANKER_THRESHOLD)(INT32 hDevice,UINT32 Channel,double *Threshold);
//GetDDC2NoiseBlankerExcessValue
typedef BOOL (__stdcall *G31DDC_GET_DDC2_NOISE_BLANKER_EXCESS_VALUE)(INT32 hDevice,UINT32 Channel,double *Value);
//StartIF
typedef BOOL (__stdcall *G31DDC_START_IF)(INT32 hDevice,WORD Period);
//StopIF
typedef BOOL (__stdcall *G31DDC_STOP_IF)(INT32 hDevice);
//GetDDCInfo
typedef BOOL (__stdcall *G31DDC_GET_DDC_INFO)(INT32 hDevice,UINT32 DDCTypeIndex,G3XDDC_DDC_INFO *Info);
//GetDDC1Count
typedef BOOL (__stdcall *G31DDC_GET_DDC1_COUNT)(INT32 hDevice,UINT32 *Count);
//SetDDC1
typedef BOOL (__stdcall *G31DDC_SET_DDC1)(INT32 hDevice,UINT32 DDCTypeIndex);
//GetDDC1
typedef BOOL (__stdcall *G31DDC_GET_DDC1)(INT32 hDevice,UINT32 *DDCTypeIndex,G3XDDC_DDC_INFO *DDCInfo);
//StartDDC1
typedef BOOL (__stdcall *G31DDC_START_DDC1)(INT32 hDevice,UINT32 SamplesPerBuffer);
//StopDDC1
typedef BOOL (__stdcall *G31DDC_STOP_DDC1)(INT32 hDevice);
//StartDDC1Playback
typedef BOOL (__stdcall *G31DDC_START_DDC1_PLAYBACK)(INT32 hDevice,UINT32 SamplesPerBuffer,UINT32 BitsPerSample);
//PauseDDC1Playback
typedef BOOL (__stdcall *G31DDC_PAUSE_DDC1_PLAYBACK)(INT32 hDevice);
//ResumeDDC1Playback
typedef BOOL (__stdcall *G31DDC_RESUME_DDC1_PLAYBACK)(INT32 hDevice);
//SetDDC1Frequency
typedef BOOL (__stdcall *G31DDC_SET_DDC1_FREQUENCY)(INT32 hDevice,UINT32 Frequency);
//GetDDC1Frequency
typedef BOOL (__stdcall *G31DDC_GET_DDC1_FREQUENCY)(INT32 hDevice,UINT32 *Frequency);
//GetDDC2
typedef BOOL (__stdcall *G31DDC_GET_DDC2)(INT32 hDevice,UINT32 *DDCTypeIndex,G3XDDC_DDC_INFO *DDCInfo);
//StartDDC2
typedef BOOL (__stdcall *G31DDC_START_DDC2)(INT32 hDevice,UINT32 Channel,UINT32 SamplesPerBuffer);
//StopDDC2
typedef BOOL (__stdcall *G31DDC_STOP_DDC2)(INT32 hDevice,UINT32 Channel);
//SetDDC2Frequency
typedef BOOL (__stdcall *G31DDC_SET_DDC2_FREQUENCY)(INT32 hDevice,UINT32 Channel,INT32 Frequency);
//GetDDC2Frequency
typedef BOOL (__stdcall *G31DDC_GET_DDC2_FREQUENCY)(INT32 hDevice,UINT32 Channel,INT32 *Frequency);
//GetSignalLevel
typedef BOOL (__stdcall *G31DDC_GET_SIGNAL_LEVEL)(INT32 hDevice,UINT32 Channel,FLOAT *Peak,FLOAT *RMS);
//SetAGC
typedef BOOL (__stdcall *G31DDC_SET_AGC)(INT32 hDevice,UINT32 Channel,BOOL Enabled);
//GetAGC
typedef BOOL (__stdcall *G31DDC_GET_AGC)(INT32 hDevice,UINT32 Channel,BOOL *Enabled);
//SetAGCParams
typedef BOOL (__stdcall *G31DDC_SET_AGC_PARAMS)(INT32 hDevice,UINT32 Channel,double AttackTime,double DecayTime,double ReferenceLevel);
//GetAGCParams
typedef BOOL (__stdcall *G31DDC_GET_AGC_PARAMS)(INT32 hDevice,UINT32 Channel,double *AttackTime,double *DecayTime,double *ReferenceLevel);
//SetMaxAGCGain
typedef BOOL (__stdcall *G31DDC_SET_MAX_AGC_GAIN)(INT32 hDevice,UINT32 Channel,double MaxGain);
//GetMaxAGCGain
typedef BOOL (__stdcall *G31DDC_GET_MAX_AGC_GAIN)(INT32 hDevice,UINT32 Channel,double *MaxGain);
//SetGain
typedef BOOL (__stdcall *G31DDC_SET_GAIN)(INT32 hDevice,UINT32 Channel,double Gain);
//GetGain
typedef BOOL (__stdcall *G31DDC_GET_GAIN)(INT32 hDevice,UINT32 Channel,double *Gain);
//GetCurrentGain
typedef BOOL (__stdcall *G31DDC_GET_CURRENT_GAIN)(INT32 hDevice,UINT32 Channel,double *Gain);
//SetNotchFilter
typedef BOOL (__stdcall *G31DDC_SET_NOTCH_FILTER)(INT32 hDevice,UINT32 Channel,BOOL Enabled);
//GetNotchFilter
typedef BOOL (__stdcall *G31DDC_GET_NOTCH_FILTER)(INT32 hDevice,UINT32 Channel,BOOL *Enabled);
//SetNotchFilterFrequency
typedef BOOL (__stdcall *G31DDC_SET_NOTCH_FILTER_FREQUENCY)(INT32 hDevice,UINT32 Channel,INT32 Frequency);
//GetNotchFilterFrequency
typedef BOOL (__stdcall *G31DDC_GET_NOTCH_FILTER_FREQUENCY)(INT32 hDevice,UINT32 Channel,INT32 *Frequency);
//SetNotchFilterBandwidth
typedef BOOL (__stdcall *G31DDC_SET_NOTCH_FILTER_BANDWIDTH)(INT32 hDevice,UINT32 Channel,UINT32 Bandwidth);
//GetNotchFilterBandwidth
typedef BOOL (__stdcall *G31DDC_GET_NOTCH_FILTER_BANDWIDTH)(INT32 hDevice,UINT32 Channel,UINT32 *Bandwidth);
//SetNotchFilterLength
typedef BOOL (__stdcall *G31DDC_SET_NOTCH_FILTER_LENGTH)(INT32 hDevice,UINT32 Channel,UINT32 Length);
//GetNotchFilterLength
typedef BOOL (__stdcall *G31DDC_GET_NOTCH_FILTER_LENGTH)(INT32 hDevice,UINT32 Channel,UINT32 *Length);
//SetDemodulatorMode
typedef BOOL (__stdcall *G31DDC_SET_DEMODULATOR_MODE)(INT32 hDevice,UINT32 Channel,UINT32 Mode);
//GetDemodulatorMode
typedef BOOL (__stdcall *G31DDC_GET_DEMODULATOR_MODE)(INT32 hDevice,UINT32 Channel,UINT32 *Mode);
//SetDemodulatorFrequency
typedef BOOL (__stdcall *G31DDC_SET_DEMODULATOR_FREQUENCY)(INT32 hDevice,UINT32 Channel,INT32 Frequency);
//GetDemodulatorFrequency
typedef BOOL (__stdcall *G31DDC_GET_DEMODULATOR_FREQUENCY)(INT32 hDevice,UINT32 Channel,INT32 *Frequency);
//SetDemodulatorFilterBandwidth
typedef BOOL (__stdcall *G31DDC_SET_DEMODULATOR_FILTER_BANDWIDTH)(INT32 hDevice,UINT32 Channel,UINT32 Bandwidth);
//GetDemodulatorFilterBandwidth
typedef BOOL (__stdcall *G31DDC_GET_DEMODULATOR_FILTER_BANDWIDTH)(INT32 hDevice,UINT32 Channel,UINT32 *Bandwidth);
//SetDemodulatorFilterShift
typedef BOOL (__stdcall *G31DDC_SET_DEMODULATOR_FILTER_SHIFT)(INT32 hDevice,UINT32 Channel,INT32 Shift);
//GetDemodulatorFilterShift
typedef BOOL (__stdcall *G31DDC_GET_DEMODULATOR_FILTER_SHIFT)(INT32 hDevice,UINT32 Channel,INT32 *Shift);
//SetDemodulatorFilterLength
typedef BOOL (__stdcall *G31DDC_SET_DEMODULATOR_FILTER_LENGTH)(INT32 hDevice,UINT32 Channel,UINT32 Length);
//GetDemodulatorFilterLength
typedef BOOL (__stdcall *G31DDC_GET_DEMODULATOR_FILTER_LENGTH)(INT32 hDevice,UINT32 Channel,UINT32 *Length);
//SetDemodulatorParam
typedef BOOL (__stdcall *G31DDC_SET_DEMODULATOR_PARAM)(INT32 hDevice,UINT32 Channel,UINT32 Code,CONST VOID *Buffer,UINT32 BufferSize);
//GetDemodulatorParam
typedef BOOL (__stdcall *G31DDC_GET_DEMODULATOR_PARAM)(INT32 hDevice,UINT32 Channel,UINT32 Code,VOID *Buffer,UINT32 BufferSize);
//GetDemodulatorState
typedef BOOL (__stdcall *G31DDC_GET_DEMODULATOR_STATE)(INT32 hDevice,UINT32 Channel,UINT32 Code,VOID *Buffer,UINT32 BufferSize);
//StartAudio
typedef BOOL (__stdcall *G31DDC_START_AUDIO)(INT32 hDevice,UINT32 Channel,UINT32 SamplesPerBuffer);
//StopAudio
typedef BOOL (__stdcall *G31DDC_STOP_AUDIO)(INT32 hDevice,UINT32 Channel);
//StartAudioPlayback
typedef BOOL (__stdcall *G31DDC_START_AUDIO_PLAYBACK)(INT32 hDevice,UINT32 Channel,UINT32 SamplesPerBuffer);
//PauseAudioPlayback
typedef BOOL (__stdcall *G31DDC_PAUSE_AUDIO_PLAYBACK)(INT32 hDevice,UINT32 Channel);
//ResumeAudioPlayback
typedef BOOL (__stdcall *G31DDC_RESUME_AUDIO_PLAYBACK)(INT32 hDevice,UINT32 Channel);
//SetAudioFilter
typedef BOOL (__stdcall *G31DDC_SET_AUDIO_FILTER)(INT32 hDevice,UINT32 Channel,BOOL Enabled);
//GetAudioFilter
typedef BOOL (__stdcall *G31DDC_GET_AUDIO_FILTER)(INT32 hDevice,UINT32 Channel,BOOL *Enabled);
//SetAudioFilterParams
typedef BOOL (__stdcall *G31DDC_SET_AUDIO_FILTER_PARAMS)(INT32 hDevice,UINT32 Channel,UINT32 CutOffLow,UINT32 CutOffHigh,double Deemphasis);
//GetAudioFilterParams
typedef BOOL (__stdcall *G31DDC_GET_AUDIO_FILTER_PARAMS)(INT32 hDevice,UINT32 Channel,UINT32 *CutOffLow,UINT32 *CutOffHigh,double *Deemphasis);
//SetAudioFilterLength
typedef BOOL (__stdcall *G31DDC_SET_AUDIO_FILTER_LENGTH)(INT32 hDevice,UINT32 Channel,UINT32 Length);
//GetAudioFilterLength
typedef BOOL (__stdcall *G31DDC_GET_AUDIO_FILTER_LENGTH)(INT32 hDevice,UINT32 Channel,UINT32 *Length);
//SetAudioGain
typedef BOOL (__stdcall *G31DDC_SET_AUDIO_GAIN)(INT32 hDevice,UINT32 Channel,double Gain);
//GetAudioGain
typedef BOOL (__stdcall *G31DDC_GET_AUDIO_GAIN)(INT32 hDevice,UINT32 Channel,double *Gain);
//SetDRMKey
typedef BOOL (__stdcall *G31DDC_SET_DRM_KEY)(INT32 hDevice,CONST CHAR *DRMKeyFileDirectory);
//IsDRMUnlocked
typedef BOOL (__stdcall *G31DDC_IS_DRM_UNLOCKED)(INT32 hDevice);
//GetSpectrumCompensation
typedef BOOL (__stdcall *G31DDC_GET_SPECTRUM_COMPENSATION)(INT32 hDevice,UINT64 CenterFrequency,UINT32 Width,FLOAT *Buffer,UINT32 Count);
//SetFrequency
typedef BOOL (__stdcall *G31DDC_SET_FREQUENCY)(INT32 hDevice,UINT32 Channel,UINT32 Frequency);
//GetFrequency
typedef BOOL (__stdcall *G31DDC_GET_FREQUENCY)(INT32 hDevice,UINT32 Channel,UINT32 *Frequency);
//SetCallbacks
typedef BOOL (__stdcall *G31DDC_SET_CALLBACKS)(INT32 hDevice,CONST G31DDC_CALLBACKS *Callbacks,DWORD_PTR UserData);

    
#endif


