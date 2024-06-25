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



// *********************************************************************
// *  This software uses the Perseus API which is defined through the  *
// *  following four files of the Microtelecom Software Defined Radio  *
// *  Developer Kit (SDRK):                                            *
// *  main.cpp, perseusdll.cpp, perseusdefs.h, perseusdll.h            *
// *  To use the Perseus hardware you need the perseususb.dll file     *
// *  You also need at least one of the following files:               *
// *  perseus125k24v21.sbs                                             *
// *  perseus250k24v21.sbs                                             *
// *  perseus500k24v21.sbs                                             *
// *  perseus1m24v21.sbs                                               *
// *  perseus2m24v21.sbs                                               *
// *  The dll and the sbs files are copyright protected and only       *
// *  available at the official Microtelecom Internet site:            *
// *  http://www.microtelecom.it                                       *
// *********************************************************************
// *   Under linux the GNU licensed libperseus_sdr library is used.    *
// *********************************************************************

#include "osnum.h"
#include "globdef.h"
#include <string.h>
#ifdef __FreeBSD__ 
#include <sys/time.h>
#endif
#ifdef __NetBSD__ 
#include <sys/time.h>
#endif
#include "uidef.h"
#include "vernr.h"
#include "thrdef.h"
#include "sdrdef.h"
#include "fft1def.h"
#include "screendef.h"
#include "options.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

// Values returned by FirmwareDownload()
#define IHEX_DOWNLOAD_OK              0 //FX2 Firmware successfully downloaded
#define IHEX_DOWNLOAD_FILENOTFOUND    1 //FX2 Firmware file not found
#define IHEX_DOWNLOAD_IOERROR         2 //USB IO Error
#define IHEX_DOWNLOAD_INVALIDHEXREC   3 //Invalid HEX Record
#define IHEX_DOWNLOAD_INVALIDEXTREC   4 //Invalide Extended HEX Record

// Values returned by FpgaConfig()
#define FPGA_CONFIG_OK              0 //FPGA successfully configured
#define FPGA_CONFIG_FILENOTFOUND    1 //FPGA bitstream file not found
#define FPGA_CONFIG_IOERROR         2 //USB IO Error
#define FPGA_CONFIG_FWNOTLOADED     3 //FX2 Firmware not loaded

// Perseus SIO (serial I/O) interface definitions

// Receiver control bit masks (ctl field of SIOCTL data structure)
#define PERSEUS_SIO_FIFOEN    0x01 //Enable inbound data USB endpoint
#define PERSEUS_SIO_DITHER    0x02 //Enable ADC dither generator
#define PERSEUS_SIO_GAINHIGH  0x04 //Enable ADC preamplifier
#define PERSEUS_SIO_RSVD      0x08 //For Microtelecm internal use only
#define PERSEUS_SIO_TESTFIFO  0x10 //For Microtelecm internal use only
#define PERSEUS_SIO_ADCBYPASS 0x20 //For Microtelecm internal use only
#define PERSEUS_SIO_ADCATT60  0x40 //For Microtelecm internal use only
#define PERSEUS_SIO_SCRAMBLE  0x80 //For Microtelecm internal use only

#define OLD_USB_BLOCKSIZE (6*170)
#define NEW_USB_BLOCKSIZE (6*1024)

// SIO Receiver Control data structure
#pragma pack(1)
typedef struct {
unsigned char ctl; // Receiver control bit mask
int freg;          // Receiver DDC LO frequency register
} SIOCTL;
#pragma pack()

// Structure for Perseus parameters
typedef struct {
int rate_no;
int clock_adjust;
int presel;
int preamp;
int dither;
int check;
}P_PERSEUS;
#define MAX_PERSEUS_PARM 6


char *perseus_parfil_name="par_perseus";
int perseus_bytesperbuf;
int usb_blocksize;
int callback_running;
char *perseus_parm_text[MAX_PERSEUS_PARM]={"Rate no",           //0
                                           "Sampl. clk adjust", //1
                                           "Preselector",       //2
                                           "Preamp",            //3
                                           "Dither",            //4
                                           "Check"};            //5
#define MAX_PERSEUS_FILTER 10
float perseus_filter_cutoff[MAX_PERSEUS_FILTER+1]={
                  1.7,
                  2.1,
                  3.0,
                  4.2,
                  6.0,
                  8.4,
                 12.0,
                 17.0,
                 24.0,
                 32.0,
                 BIGFLOAT};

// Data structure which holds controls for the receiver ADC and DDC

// Perseus EEPROM Definitions and data structures
#define ADDR_EEPROM_PRODID 8
#pragma pack(1)
typedef struct {
unsigned short int sn;       // Receiver Serial Number
unsigned short int prodcode; // Microtelecom Product Code
unsigned char hwrel;         // Product release
unsigned char hwver;         // Product version
unsigned char signature[6];  // Microtelecom Original Product Signature
} eeprom_prodid;
#pragma pack()

int no_of_perseus_rates;
#define PERSEUS_RATES_BUFSIZE 20
double perseus_rates[PERSEUS_RATES_BUFSIZE];
P_PERSEUS pers;




#if(OSNUM == OSNUM_WINDOWS)
#include "wdef.h"
#if defined(interface)
#undef interface
#endif
extern char *unable_to_load;

// Microtelecom original FPGA bitstream signature
typedef struct {
unsigned char sz[6];
} Key48;

typedef int   (WINAPI *PtrToGetDLLVersion)(void);
typedef int   (WINAPI *PtrToOpen)(void);
typedef int   (WINAPI *PtrToClose)(void);
typedef char* (WINAPI *PtrToGetDeviceName)(void);
typedef int   (WINAPI *PtrToEepromFunc)(UCHAR *, DWORD, UCHAR);
typedef int   (WINAPI *PtrToFwDownload)(char* fname);
typedef int   (WINAPI *PtrToFpgaConfig)(char* fname, Key48 *pKey);
typedef int   (WINAPI *PtrToSetAttPresel)(unsigned char idSet);
typedef int   (WINAPI *PtrToSio)(void *ctl, int nbytes);
typedef int   (WINAPI *PInputCallback)(char *pBuf, int nothing);
typedef int   (WINAPI *PtrToStartAsyncInput)(int dwBufferSize, 
                           PInputCallback pInputCallback, void *pExtra);
typedef int  (WINAPI *PtrToStopAsyncInput)(void);

#define MAX_PERSEUS_RATES 10
double rate_factor[MAX_PERSEUS_RATES]={1666.6666667,  //48kHz
                                       840.0,         //95.238 kHz
                                       832.0,         //96.154 kHz
                                       640.0,         //125 kHz
                                       416.0,         //192.308 kHz
                                       320.0,         //250 kHz
                                       160.0,         //500 kHz
                                       80.0,          //1 MHz
                                       50.0,          //1.6 MHz
                                       40.0};         //2 MHz

char *perseus_bitstream_names[MAX_PERSEUS_RATES]={
                 "perseus48k24v31",
                 "perseus95k24v31",
                 "perseus96k24v31",
                 "perseus125k24v21",
                 "perseus192k24v31",
                 "perseus250k24v21",
                 "perseus500k24v21",
                 "perseus1m24v21",
                 "perseus1d6m24v21",
                 "perseus2m24v21"};

extern char *perseus_bitstream_names[MAX_PERSEUS_RATES];

static HINSTANCE h_pers=NULL;

PtrToGetDLLVersion		m_pGetDLLVersion;
PtrToOpen		    	m_pOpen;
PtrToClose			    m_pClose;
PtrToGetDeviceName		m_pGetDeviceName;
PtrToGetDeviceName		m_pGetFriendlyDeviceName;
PtrToEepromFunc			m_pEepromRead;
PtrToFwDownload			m_pFwDownload;
PtrToFpgaConfig			m_pFpgaConfig;
PtrToSetAttPresel		m_pSetAttenuator;
PtrToSetAttPresel		m_pSetPreselector;
PtrToSio                m_pSio;
PtrToStartAsyncInput	m_pStartAsyncInput;
PtrToStopAsyncInput     m_pStopAsyncInput;

SIOCTL sioctl;
extern float perseus_filter_cutoff[MAX_PERSEUS_FILTER+1];


Key48 perseus48k24v31_signature= {{0xE4, 0x37, 0x73, 0x7B, 0x9A, 0x6A }};
Key48 perseus95k24v31_signature= {{0x22, 0x33, 0xB8, 0xEA, 0x2C, 0xBC }};
Key48 perseus96k24v31_signature= {{0x39, 0x18, 0x79, 0xF5, 0x90, 0x8C }};
Key48 perseus125k24v21_signature={{0x45, 0x7C, 0xD4, 0x3A, 0x0E, 0xD4 }};
Key48 perseus192k24v31_signature={{0x7B, 0x39, 0x26, 0x75, 0x9F, 0x7B }};
Key48 perseus250k24v21_signature={{0xCF, 0x4B, 0x9F, 0x02, 0xB2, 0x1E }};
Key48 perseus500k24v21_signature={{0xF2, 0x9E, 0x67, 0x49, 0x51, 0x6B }};
Key48 perseus1m24v21_signature=  {{0xC5, 0xC1, 0x96, 0x4D, 0xA8, 0x05 }};
Key48 perseus1d6m24v21_signature={{0xA6, 0x1B, 0xC0, 0x65, 0xC6, 0xF3 }};
Key48 perseus2m24v21_signature=  {{0xDE, 0xEA, 0x57, 0x52, 0x14, 0x35 }};

Key48 *perseus_bitstream_signatures[MAX_PERSEUS_RATES]={
                               &perseus48k24v31_signature,
                               &perseus95k24v31_signature,
                               &perseus96k24v31_signature,
                               &perseus125k24v21_signature,
                               &perseus192k24v31_signature,
                               &perseus250k24v21_signature,
                               &perseus500k24v21_signature,
                               &perseus1m24v21_signature,
                               &perseus1d6m24v21_signature,
                               &perseus2m24v21_signature};

int perseus_dll_version1;
int perseus_dll_version2;

int WINAPI perseus_callback(char *pBuf, int nothing)
{
register char c1;
int i;
callback_running=TRUE;
c1=0;
// This callback gets called whenever a buffer is ready
// from the USB interface
for(i=0; i<perseus_bytesperbuf; i+=6)
  {
  timf1_char[timf1p_sdr+3]=pBuf[i+2];
  c1=pBuf[i+2]&0x80;
  timf1_char[timf1p_sdr+2]=pBuf[i+1];
  timf1_char[timf1p_sdr+1]=pBuf[i  ];
  c1|=pBuf[i];
  timf1_char[timf1p_sdr  ]=0;
  timf1_char[timf1p_sdr+7]=pBuf[i+5];
  timf1_char[timf1p_sdr+6]=pBuf[i+4];
  timf1_char[timf1p_sdr+5]=pBuf[i+3];
  timf1_char[timf1p_sdr+4]=0;
  timf1p_sdr=(timf1p_sdr+8)&timf1_bytemask;
  }
if( (c1&1) != 0)
  {
  rx_ad_maxamp[0]=0x7fff;
  rx_ad_maxamp[1]=0x7fff;
  }
SetEvent(rxin1_bufready);
return nothing;
}

void lir_perseus_read(void)
{

if(((timf1p_sdr-timf1p_pa+timf1_bytes)&timf1_bytemask) < (int)snd[RXAD].block_bytes)
  {
  while(!kill_all_flag && 
        WaitForSingleObject(rxin1_bufready, 200) == WAIT_TIMEOUT &&
        thread_command_flag[THREAD_PERSEUS_INPUT] == THRFLAG_ACTIVE);
  }
}

int perseus_load_proprietary_code(int rate_no)
{
int i;
// Download the standard Microtelecom FX2 firmware to the Perseus receiver
// IMPORTANT NOTE:
// The parameter passed to PerseusFirmwareDownload() must be NULL
// for the Perseus hardware to be operated safely.
// Permanent damages to the receiver can result if a third-party firmware
// is downloaded to the Perseus FX2 controller. You're advised.
//
// During the execution of the PerseusFirmwareDownload() function
// the Perseus USB hardware is renumerated. This causes the function
// to be blocking for a while. It is suggested to put an hourglass cursor
// before calling this function if the code has to be developed for a real
// Window application (and removing it when it returns)
i=(*m_pFwDownload)(NULL);
if(i != IHEX_DOWNLOAD_OK && i != IHEX_DOWNLOAD_IOERROR)
  {
  switch (i)
    {
//    case IHEX_DOWNLOAD_IOERROR:
//    return 1083;

    case IHEX_DOWNLOAD_FILENOTFOUND:
    return 1084;

    case IHEX_DOWNLOAD_INVALIDHEXREC:
    return 1085;

    case IHEX_DOWNLOAD_INVALIDEXTREC:
    return 1089;

    default:
    return 1090;
    }
  }
// Download the FPGA bitstream to the Perseus' FPGA
// There are three bitstream provided with the current software version
// one for each of the output sample rate provided by the receiver
// (125, 250, 500, 1000 or 2000 kS/s)
// Each FPGA bistream (.sbs file) has an original Microtelecom
// signature which must be provided as the second parameter to the
// function PerseusFpgaConfig() to prevent the configuration of the
// FPGA with non authorized, third-party, code which could damage
// the receiver hardware.
// The bitstream files should be located in the same directory of
// the executing software for things to work.
i=(*m_pFpgaConfig)( perseus_bitstream_names[rate_no],
                    perseus_bitstream_signatures[rate_no]);
if(i != FPGA_CONFIG_OK)
  {
  switch (i)
    {
    case FPGA_CONFIG_FILENOTFOUND:
    return 1092;

    case FPGA_CONFIG_IOERROR:
    return 1096;

    default:
    return 1097;
    }
  }
return 0;
}

int perseus_eeprom_read(unsigned char *buf, int addr, unsigned char count)
{
return (*m_pEepromRead)(buf, addr, count);
}

char *perseus_frname(void)
{
return (*m_pGetFriendlyDeviceName)();
}

void perseus_store_att(unsigned char idAtt)
{
m_pSetAttenuator(idAtt);
}


void perseus_store_presel(unsigned char idPresel)
{
m_pSetPreselector(idPresel);
}

void set_perseus_frequency(void)
{
unsigned char sl;
double dt1;
sl=MAX_PERSEUS_FILTER;
if(pers.presel !=0 )
  {
  sl=0;
// select filter according to frequency.
  while(perseus_filter_cutoff[sl]<fg.passband_center)sl++;
  }
perseus_store_presel(sl);
dt1=fg.passband_center*1000000;
if(dt1 < 0 || dt1 > adjusted_sdr_clock/2)
  {
  fg.passband_center=7.05;
  dt1=fg.passband_center*1000000;
  }
sioctl.freg=4.294967296E9*dt1/adjusted_sdr_clock;
m_pSio(&sioctl,sizeof(sioctl));
}

void start_perseus_read(void)
{
int i;
sdr=-1;
rxin1_bufready=CreateEvent( NULL, FALSE, FALSE, NULL);
if(rxin1_bufready== NULL)
  {
  close_perseus();
  lirerr(1223);
  return;
  }
perseus_bytesperbuf=snd[RXAD].block_bytes/usb_blocksize;
if(perseus_bytesperbuf == 0)perseus_bytesperbuf=1;
if(perseus_bytesperbuf > 16)perseus_bytesperbuf=16;
perseus_bytesperbuf*=usb_blocksize;
timf1p_sdr=timf1p_pa;
lir_sched_yield();
i=m_pStartAsyncInput(perseus_bytesperbuf, perseus_callback, 0);
if(i == FALSE)
  {
  close_perseus();
  lirerr(1098);
  return;
  }
// Enable the Perseus data FIFO through its SIO interface
// setting the PERSEUS_SIO_FIFOEN bit in the ctl field of
// the (global) sioctl variable.
sioctl.ctl |=PERSEUS_SIO_FIFOEN;
m_pSio(&sioctl,sizeof(sioctl));
sdr=0;
}

void perseus_stop(void)
{
if(callback_running)m_pStopAsyncInput();
sioctl.ctl &=~PERSEUS_SIO_FIFOEN;
m_pSio(&sioctl,sizeof(sioctl));
lir_sleep(10000);
CloseHandle(rxin1_bufready); 
}

void close_perseus(void)
{
m_pClose();
FreeLibrary(h_pers);
h_pers=NULL;
sdr=-1;
}

void open_perseus(void)
{
int j, k;
char perseus_dllname[80];
char s[80];
sprintf(perseus_dllname,"%sperseususb.dll",DLLDIR);
if(h_pers == NULL)
  {
  h_pers = LoadLibraryEx(perseus_dllname, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
  if(h_pers == NULL)
    {
    clear_screen();
    sprintf(s,"%s%s",unable_to_load, perseus_dllname);
    SNDLOG"\n%s",s);
    lir_text(3,2,s);
    lir_text(3,3,press_any_key);
    await_keyboard();
    return;
    }
  m_pGetDLLVersion	= (PtrToGetDLLVersion)(void*)GetProcAddress(h_pers,"GetDLLVersion");
  m_pOpen                 = (PtrToOpen)(void*)GetProcAddress(h_pers,"Open");
  m_pClose                = (PtrToClose)(void*)GetProcAddress(h_pers,"Close");
  m_pGetDeviceName	= (PtrToGetDeviceName)(void*)GetProcAddress(h_pers,"GetDeviceName");
  m_pGetFriendlyDeviceName= (PtrToGetDeviceName)(void*)GetProcAddress(h_pers,"GetFriendlyDeviceName");
  m_pEepromRead		= (PtrToEepromFunc)(void*)GetProcAddress(h_pers,"EepromRead");
  m_pFwDownload           = (PtrToFwDownload)(void*)GetProcAddress(h_pers,"FirmwareDownload");
  m_pFpgaConfig		= (PtrToFpgaConfig)(void*)GetProcAddress(h_pers,"FpgaConfig");
  m_pSetAttenuator	= (PtrToSetAttPresel)(void*)GetProcAddress(h_pers,"SetAttenuator");
  m_pSetPreselector	= (PtrToSetAttPresel)(void*)GetProcAddress(h_pers,"SetPreselector");
  m_pSio			= (PtrToSio)(void*)GetProcAddress(h_pers,"Sio");
  m_pStartAsyncInput	= (PtrToStartAsyncInput)(void*)GetProcAddress(h_pers,"StartAsyncInput");
  m_pStopAsyncInput	= (PtrToStopAsyncInput)(void*)GetProcAddress(h_pers,"StopAsyncInput");
  SNDLOG"\nperseususb.dll loaded");
  if( m_pGetDLLVersion == NULL ||
      m_pOpen == NULL ||
      m_pGetDeviceName == NULL ||
      m_pGetFriendlyDeviceName == NULL ||
      m_pEepromRead == NULL ||
      m_pFwDownload == NULL ||
      m_pFpgaConfig == NULL ||
      m_pSetAttenuator == NULL ||
      m_pSetPreselector == NULL ||
      m_pSio  == NULL ||
      m_pStartAsyncInput == NULL ||
      m_pStopAsyncInput == NULL
      )
    {
    SNDLOG"\nDLL function(s) missing");
free_pdll:;
    lir_text(3,2,"The Perseus seems to not be properly installed");
    FreeLibrary(h_pers);
    h_pers=NULL;
    return;
    }
  j = m_pGetDLLVersion();
  if (!j)
    {
    SNDLOG"\nDLLversion failed");
    goto free_pdll;
    }
  k=j>>16;
  j&=0xffff;
  SNDLOG" vers %d.%d",k,j);
  perseus_dll_version1=k;
  perseus_dll_version2=j;
  j=m_pOpen();
  if(j==FALSE)
    {
    SNDLOG"\nPerseus receiver not detected");
    if(write_log)fflush(sndlog);
    goto free_pdll;
    }
  }
sioctl.ctl=0;
if(pers.preamp != 0)sioctl.ctl|=PERSEUS_SIO_GAINHIGH;
if(pers.dither != 0)sioctl.ctl|=PERSEUS_SIO_DITHER;
sdr=0;
}
#endif

#if(OSNUM == OSNUM_LINUX)
#include <dlfcn.h>

// Perseus VID/PID
#define PERSEUS_VID                     0x04B4
#define PERSEUS_PID_BLANKEEPROM         0x8613
#define PERSEUS_PID                     0x325C

// Perseus Fx2 firmware end points
#define PERSEUS_EP_CMD			0x01
#define PERSEUS_EP_STATUS		0x81
#define PERSEUS_EP_DATAIN	  	0x82

// Fx2 MCU specific vendor command defines
#define FX2_BM_VENDOR_REQUEST		0x40
#define FX2_REQUEST_FIRMWARE_LOAD	0xA0
#define FX2_ADDR_CPUCS				0xE600

// Defines and commands interpreted by the Perseus Fx2 firmware
#define PERSEUS_CMD_CONFIG_TRANSFER_SIZE 64
#define PERSEUS_EEPROMADR_PRODID 8

#define PERSEUS_CMD_FPGACONFIG	0x00
#define PERSEUS_CMD_FPGARESET	0x01
#define PERSEUS_CMD_FPGACHECK	0x02
#define PERSEUS_CMD_FPGASIO		0x03
#define PERSEUS_CMD_FX2PORTE	0x04
#define PERSEUS_CMD_EEPROMREAD	0x06
#define PERSEUS_CMD_SHUTDOWN	0x08

#define PERSEUS_NOERROR			 0
#define PERSEUS_INVALIDDEV		-1
#define PERSEUS_NULLDESCR		-2
#define PERSEUS_ALREADYOPEN		-3
#define PERSEUS_LIBUSBERR		-4
#define PERSEUS_DEVNOTOPEN		-5
#define PERSEUS_DEVCONF			-6
#define PERSEUS_DEVCLAIMINT		-7
#define PERSEUS_DEVALTINT		-8
#define PERSEUS_FNNOTAVAIL		-9
#define PERSEUS_DEVNOTFOUND		-10
#define PERSEUS_EEPROMREAD		-11
#define PERSEUS_FILENOTFOUND	-12
#define PERSEUS_IOERROR			-13
#define PERSEUS_INVALIDHEXREC	-14
#define PERSEUS_INVALIDEXTREC	-15
#define PERSEUS_FWNOTLOADED		-16
#define PERSEUS_FPGACFGERROR	-17
#define PERSEUS_FPGANOTCFGD	    -18
#define PERSEUS_ASYNCSTARTED	-19
#define PERSEUS_NOMEM			-20
#define PERSEUS_CANTCREAT		-21
#define PERSEUS_ERRPARAM		-22
#define PERSEUS_MUTEXIN			-23
#define PERSEUS_BUFFERSIZE		-24
#define PERSEUS_ATTERROR		-25

typedef struct libusb_device libusb_device;
typedef struct libusb_device_handle libusb_device_handle;

// Fx2 MCU <-> FPGA serial io data structure
typedef struct __attribute__((__packed__)) {
	uint8_t 	ctl;
	uint32_t	freg;
} fpga_sioctl;

typedef int (*perseus_input_callback)(void *buf, int buf_size, void *extra);

typedef struct perseus_input_queue_ds *perseus_input_queue_ptr;

typedef struct {
	int						 idx;
	perseus_input_queue_ptr  queue;
	struct libusb_transfer 	*transfer;
	int cancelled;
} perseus_input_transfer;

typedef struct perseus_input_queue_ds {
	int						size;
	perseus_input_transfer	*transfer_queue;
	volatile int 			cancelling;
	char					*buf;
	int						transfer_buf_size;
	int						timeout;
	perseus_input_callback 	callback_fn;
	void					*callback_extra;
	int						idx_expected;
	volatile int			completed;
	struct timeval 			start;
	struct timeval			stop;
	uint64_t		bytes_received;
} perseus_input_queue;

// Microtelecom product code for the Perseus receiver
#define PERSEUS_PRODCODE    0x8014
#define MAX_LIBUSB_BULK_BUFFER 16320
#define MAX_PERSEUS_RATES 6


typedef struct perseus_descr_ds {
		int						index;
		libusb_device		 	*device;
	    libusb_device_handle 	*handle;
		uint8_t					bus;
		uint8_t					devaddr;
		int						is_cypress_ezusb;
		int						is_preserie;
		int						firmware_downloaded;
		int						fpga_configured;
		eeprom_prodid 			product_id;
		uint8_t					frontendctl;
		double					adc_clk_freq;
		double					ddc_lo_freq;
		uint8_t					presel_flt_id;
		fpga_sioctl				sioctl;
		perseus_input_queue		input_queue;
} perseus_descr;

perseus_descr *pers_descr;
double rate_factor[PERSEUS_RATES_BUFSIZE];
char *perseus_bitstream_names[MAX_PERSEUS_RATES]={
                 "perseus95k24v31.rbs",
                 "perseus125k24v21.rbs",
                 "perseus250k24v21.rbs",
                 "perseus500k24v21.rbs",
                 "perseus1m24v21.rbs",
                 "perseus2m24v21.rbs"};


// ******* Perseus library functions that we do not use *******
// int perseus_set_adc(perseus_descr *pers_descr, int enableDither, int enablePreamp);
// int perseus_set_sampling_rate(perseus_descr *pers_descr, int new_sample_rate);
// int perseus_set_attenuator_in_db (perseus_descr *pers_descr, int new_level_in_db);
// int perseus_get_attenuator_values (perseus_descr *pers_descr, int *buf, unsigned int size);
// int perseus_set_attenuator_n (perseus_descr *pers_descr, int nlo);
// void perseus_set_debug(int level);
// *************************************************************

typedef int (*p_perseus_init)(void);
typedef int (*p_perseus_exit)(void);
typedef perseus_descr* (*p_perseus_open)(int nDev);
typedef int (*p_perseus_set_attenuator)(perseus_descr *descr, uint8_t atten_id);
typedef int (*p_perseus_close)(perseus_descr *descr);
typedef int (*p_perseus_firmware_download)(perseus_descr *descr, char *fname);
typedef int (*p_perseus_get_product_id)(perseus_descr *descr, eeprom_prodid *prodid);
typedef int (*p_perseus_set_ddc_center_freq)(perseus_descr *descr,  double center_freq_hz, int enablePresel);
typedef int (*p_perseus_start_async_input)(perseus_descr *descr, uint32_t buffersize, perseus_input_callback callback, void *cb_extra);
typedef int (*p_perseus_get_sampling_rates)(perseus_descr *descr, int *buf, unsigned int size);
typedef int (*p_perseus_set_sampling_rate_n)(perseus_descr *descr, unsigned int new_sample_rate_ordinal);
typedef int (*p_perseus_stop_async_input)(perseus_descr *descr);
typedef char* (*p_perseus_errorstr)(void);

p_perseus_init perseus_init;
p_perseus_exit perseus_exit;
p_perseus_open perseus_open;
p_perseus_set_attenuator perseus_set_attenuator;
p_perseus_close perseus_close;
p_perseus_firmware_download perseus_firmware_download;
p_perseus_get_product_id perseus_get_product_id;
p_perseus_set_ddc_center_freq perseus_set_ddc_center_freq;
p_perseus_start_async_input perseus_start_async_input;
p_perseus_get_sampling_rates perseus_get_sampling_rates;
p_perseus_set_sampling_rate_n perseus_set_sampling_rate_n;
p_perseus_stop_async_input  perseus_stop_async_input;
p_perseus_errorstr perseus_errorstr;

static void *perseus_libhandle=NULL;

int load_perseus_library(void)
{
int info;
info=0;
if(perseus_libhandle)return 0;
perseus_libhandle=dlopen(PERSEUS_LIBNAME, RTLD_LAZY);
if(!perseus_libhandle)goto perseus_load_error;
info=1;
perseus_init=(p_perseus_init)dlsym(perseus_libhandle, "perseus_init");
if(perseus_init == NULL)goto perseus_sym_error;
perseus_exit=(p_perseus_exit)dlsym(perseus_libhandle, "perseus_exit");
if(perseus_exit == NULL)goto perseus_sym_error;
perseus_open=(p_perseus_open)dlsym(perseus_libhandle, "perseus_open");
if(perseus_open == NULL)goto perseus_sym_error;
perseus_set_attenuator=(p_perseus_set_attenuator)dlsym(perseus_libhandle, "perseus_set_attenuator");
if(perseus_set_attenuator == NULL)goto perseus_sym_error;
perseus_close=(p_perseus_close)dlsym(perseus_libhandle, "perseus_close");
if(perseus_close == NULL)goto perseus_sym_error;
perseus_firmware_download=(p_perseus_firmware_download)dlsym(perseus_libhandle, "perseus_firmware_download");
if(perseus_firmware_download == NULL)goto perseus_sym_error;
perseus_get_product_id=(p_perseus_get_product_id)dlsym(perseus_libhandle, "perseus_get_product_id");
if(perseus_get_product_id == NULL)goto perseus_sym_error;
perseus_set_ddc_center_freq=(p_perseus_set_ddc_center_freq)dlsym(perseus_libhandle, "perseus_set_ddc_center_freq");
if(perseus_set_ddc_center_freq == NULL)goto perseus_sym_error;
perseus_start_async_input=(p_perseus_start_async_input)dlsym(perseus_libhandle, "perseus_start_async_input");
if(perseus_start_async_input == NULL)goto perseus_sym_error;
perseus_get_sampling_rates=(p_perseus_get_sampling_rates)dlsym(perseus_libhandle, "perseus_get_sampling_rates");
if(perseus_get_sampling_rates == NULL)goto perseus_sym_error;
perseus_set_sampling_rate_n=(p_perseus_set_sampling_rate_n)dlsym(perseus_libhandle, "perseus_set_sampling_rate_n");
if(perseus_set_sampling_rate_n == NULL)goto perseus_sym_error;
perseus_stop_async_input=(p_perseus_stop_async_input)dlsym(perseus_libhandle, "perseus_stop_async_input");
if(perseus_stop_async_input == NULL)goto perseus_sym_error;
perseus_errorstr=(p_perseus_errorstr)dlsym(perseus_libhandle, "perseus_errorstr");
if(perseus_errorstr == NULL)goto perseus_sym_error;
return 0;
perseus_sym_error:;
dlclose(perseus_libhandle);
perseus_load_error:;
library_error_screen(PERSEUS_LIBNAME,info);
return -1;
}

void unload_perseus_library(void)
{
if(!perseus_libhandle)return;
dlclose(perseus_libhandle);
perseus_libhandle=NULL;
}


void set_perseus_frequency(void)
{
double dt1;
dt1=fg.passband_center*1000000;
if(dt1 < 0 || dt1 > adjusted_sdr_clock/2)
  {
  fg.passband_center=7.05;
  dt1=fg.passband_center*1000000;
  }
dt1/=adjusted_sdr_clock/PERSEUS_SAMPLING_CLOCK;  
perseus_set_ddc_center_freq(pers_descr, dt1, pers.presel);
}

int fill_perseus_rates(void)
{
// Old versions of libperseus-sdr require a double
// so we have to allocate double space to not get a crasch
// when calling perseus_get_sampling_rates 
int pr[2*PERSEUS_RATES_BUFSIZE];
int i;
double dt1;
i=perseus_get_sampling_rates(pers_descr, pr, PERSEUS_RATES_BUFSIZE);
if(i) return -1196;
no_of_perseus_rates = 0;
while (pr[no_of_perseus_rates] > 1)
  {
  if(no_of_perseus_rates >= PERSEUS_RATES_BUFSIZE)return -1412;
  switch (pr[no_of_perseus_rates])
    {
    case 48000:
    dt1=PERSEUS_SAMPLING_CLOCK/1664;
    break;
    
    case 95000:
    dt1=PERSEUS_SAMPLING_CLOCK/840;
    break;
    
    case 96000:
    dt1=PERSEUS_SAMPLING_CLOCK/832;
    break;
    
    case 192000:
    dt1=PERSEUS_SAMPLING_CLOCK/416;
    break;
    
    default:
    dt1=(double)pr[no_of_perseus_rates];
    }
  perseus_rates[no_of_perseus_rates]=dt1;
  no_of_perseus_rates++;
  }
if(no_of_perseus_rates == 0)return -1412;
return 0;
}

void perseus_stop(void)
{
perseus_stop_async_input(pers_descr);
lir_sleep(10000);
}



int perseus_callback(void *buf, int buf_size, void *extra)
{
int i;
register char c1;
char *pBuf;
(void)extra;
pBuf=(char*)buf;
c1=0;
// This callback gets called whenever a buffer is ready
// from the USB interface
for(i=0; i<buf_size; i+=6)
  {
  timf1_char[timf1p_sdr+3]=pBuf[i+2];
  c1=pBuf[i+2]&0x80;
  timf1_char[timf1p_sdr+2]=pBuf[i+1];
  timf1_char[timf1p_sdr+1]=pBuf[i  ];
  c1|=pBuf[i];
  timf1_char[timf1p_sdr  ]=0;
  timf1_char[timf1p_sdr+7]=pBuf[i+5];
  timf1_char[timf1p_sdr+6]=pBuf[i+4];
  timf1_char[timf1p_sdr+5]=pBuf[i+3];
  timf1_char[timf1p_sdr+4]=0;
  timf1p_sdr=(timf1p_sdr+8)&timf1_bytemask;
  }
if( (c1&1) != 0)
  {
  rx_ad_maxamp[0]=0x7fff;
  rx_ad_maxamp[1]=0x7fff;
  }
if(((timf1p_sdr-timf1p_pa+timf1_bytes)&timf1_bytemask) >=
              (int)snd[RXAD].block_bytes)lir_set_event(EVENT_PERSEUS_INPUT);
return 0;
}

void start_perseus_read(void)
{
int i;
sdr=-1;
perseus_bytesperbuf=snd[RXAD].block_bytes/usb_blocksize;
if(perseus_bytesperbuf == 0)perseus_bytesperbuf=1;
perseus_bytesperbuf*=usb_blocksize;
if(perseus_bytesperbuf > MAX_LIBUSB_BULK_BUFFER)
  {
  perseus_bytesperbuf=MAX_LIBUSB_BULK_BUFFER/usb_blocksize;
  perseus_bytesperbuf*=usb_blocksize;
  }
timf1p_sdr=timf1p_pa;
lir_sched_yield();
i=perseus_start_async_input(pers_descr,perseus_bytesperbuf, perseus_callback, NULL);
if(i<0)
  {
  if(usb_blocksize == OLD_USB_BLOCKSIZE)
    {
    usb_blocksize=NEW_USB_BLOCKSIZE;
    }
  else
    {
    usb_blocksize=OLD_USB_BLOCKSIZE;
    }
  perseus_bytesperbuf=snd[RXAD].block_bytes/usb_blocksize;
  if(perseus_bytesperbuf == 0)perseus_bytesperbuf=1;
  perseus_bytesperbuf*=usb_blocksize;
  if(perseus_bytesperbuf > MAX_LIBUSB_BULK_BUFFER)
    {
    perseus_bytesperbuf=MAX_LIBUSB_BULK_BUFFER/usb_blocksize;
    perseus_bytesperbuf*=usb_blocksize;
    }
  i=perseus_start_async_input(pers_descr,perseus_bytesperbuf, perseus_callback, NULL);
  }    
if(i < 0) 
  {
  lirerr(1195);
  return;
  }
sdr=0;
}

void lir_perseus_read(void)
{
while(!kill_all_flag && 
  ((timf1p_sdr-timf1p_pa+timf1_bytes)&timf1_bytemask) < 
                                             (int)snd[RXAD].block_bytes &&
                thread_command_flag[THREAD_PERSEUS_INPUT] == THRFLAG_ACTIVE)
  {
  lir_await_event(EVENT_PERSEUS_INPUT);
  }
lir_await_event(EVENT_PERSEUS_INPUT);
}

int perseus_load_proprietary_code(int rate_no)
{
int i;
i=perseus_set_sampling_rate_n(pers_descr, rate_no);
if(i<0) return 1194;
return 0;
}

int perseus_eeprom_read(unsigned char *buf, int addr, unsigned char count)
{
eeprom_prodid *prodid;
int i;
char s[80];
prodid=(eeprom_prodid*) buf;
(void) buf;
(void) addr;
(void) count;
i=perseus_get_product_id(pers_descr,prodid);
if(i<0)
  {
  sprintf(s,"get product id error: %s", perseus_errorstr());
  clear_screen();
  lir_text(5,5,s);
  lir_text(5,7,press_any_key);
  await_processed_keyboard();
  lirerr(1193);
  return 1;
  }
return 0;
}

void open_perseus(void)
{
int i;
if(sdr != -1)
  {
  return;
  }
load_usb1_library(TRUE);
if(libusb1_library_flag == FALSE)return;
if(load_perseus_library() != 0)return;  
if(perseus_init() == 0)
  {
  lir_text(3,3,"Could not init. Is Perseus connected with power on?");
conerr:;
  return;
  }
pers_descr=perseus_open(0);
if(pers_descr == NULL)
  {
  clear_screen();
  lir_text(3,2,"Could not open Perseus. Do you have write permission to USB?");
  lir_text(3,3,"Maybe you should use sudo (or run as root)");
  goto conerr;
  }
i=perseus_firmware_download(pers_descr,NULL);
if(i < 0)
  {
  close_perseus();
  return;
  }
sdr=0;  
}

void perseus_store_att(unsigned char idAtt)
{
perseus_set_attenuator(pers_descr, idAtt);
}

void perseus_store_presel(unsigned char idPresel)
{
(void) idPresel;
}

void close_perseus(void)
{
perseus_close(pers_descr);
lir_sleep(100000);
perseus_exit();
sdr=-1;
unload_perseus_library();
unload_usb1_library();
}

#endif

// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

int display_perseus_parm_info(int *line)
{
char s[80];
int i, errcod;
errcod=read_sdrpar(perseus_parfil_name, MAX_PERSEUS_PARM, 
                                     perseus_parm_text, (int*)((void*)&pers));
if(errcod == 0)
  {
#if(OSNUM == OSNUM_LINUX)
    if(load_perseus_library() != 0)return -99;
    fill_perseus_rates();
    for(i=0; i<no_of_perseus_rates; i++)
      {
      rate_factor[i]=PERSEUS_SAMPLING_CLOCK/perseus_rates[i];
      }
    unload_perseus_library();
#endif
#if(OSNUM == OSNUM_WINDOWS)
    for(i=0; i<MAX_PERSEUS_RATES; i++)
      {
      perseus_rates[i]=PERSEUS_SAMPLING_CLOCK/rate_factor[i];
      }
    no_of_perseus_rates=MAX_PERSEUS_RATES;
#endif
  settextcolor(7);
  sprintf(s,"rate number        = %i ", pers.rate_no);
  if(perseus_rates[pers.rate_no] > 999999.9)
    {
    sprintf(s,"rate number        = %i (%8.3f MHz)", 
                         pers.rate_no, 0.000001*perseus_rates[pers.rate_no]);
    }
  else
    {
    sprintf(s,"rate number        = %i (%8.3f kHz)", 
                         pers.rate_no, 0.001*perseus_rates[pers.rate_no]);
    }
  lir_text(24,line[0],s);
  SNDLOG"\n%s",s);
  line[0]+=1;
  sprintf(s,"clock adjustment   = %i", pers.clock_adjust);
  lir_text(24,line[0],s);
  SNDLOG"\n%s",s);
  line[0]+=1;
  sprintf(s,"preselector        = %i", pers.presel);
  lir_text(24,line[0],s);
  SNDLOG"\n%s",s);
  line[0]+=1;
  sprintf(s,"preamplifier       = %i", pers.preamp);
  lir_text(24,line[0],s);
  SNDLOG"\n%s",s);
  line[0]+=1;
  sprintf(s,"dither             = %i", pers.dither);
  lir_text(24,line[0],s);
  SNDLOG"\n%s\n",s);
  }
return (errcod);
}

void init_perseus(void)
{
int i, k;
char s[80];
char *ss;
int *sdr_pi;
int line;
char *looking;
eeprom_prodid prodid;
FILE *perseus_file;
looking="Looking for a Perseus on the USB port.";
settextcolor(12);
lir_text(10,10,looking);
SNDLOG"\n%s",looking);
lir_text(10,11,"Reset CPU, USB and SDR hardware if system hangs here.");
lir_refresh_screen();
settextcolor(14);
sdr=-1;
open_perseus();
settextcolor(7);
clear_lines(10,11);
lir_refresh_screen();
line=0;
if(sdr != -1)
  {
  lir_text(5,5,"A Perseus is detected on the USB port.");
  lir_text(5,6,"Do you want to use it for RX input (Y/N)?");
qpers:;
  await_processed_keyboard();
  if(kill_all_flag) goto pers_errexit;
  if(lir_inkey == 'Y')
    {
#if(OSNUM == OSNUM_WINDOWS)
    ss=perseus_frname();
    if(ss == NULL)goto pers_errexit;
    sprintf(s,"USB Friendly device Name is: %s",ss);
    lir_text(2,8,s);
    SNDLOG"%s\n",s);
    ss=m_pGetDeviceName();
    if(ss == NULL)goto pers_errexit;
    sprintf(s,"USB Device Name is: %s",ss);
    lir_text(2,9,s);
    SNDLOG"%s\n",s);
#else
    ss="Perseus";
#endif
    ui.rx_addev_no=PERSEUS_DEVICE_CODE;
    ui.rx_input_mode=IQ_DATA+DIGITAL_IQ+DWORD_INPUT;
    ui.rx_rf_channels=1;
    ui.rx_ad_channels=2;
    ui.rx_admode=0;
    perseus_eeprom_read((unsigned char*)&prodid,
                                       ADDR_EEPROM_PRODID, sizeof(prodid));
    if(kill_all_flag)goto pers_errexit;
    sprintf(s,"Microtelecom product code: 0x%04X", prodid.prodcode);
    lir_text(2,10,s);
    SNDLOG"%s\n",s);
    sprintf(s,"Serial Number: %05hd-%02hX%02hX-%02hX%02hX-%02hX%02hX",
				(unsigned short)prodid.sn,
				(unsigned short)prodid.signature[5],
				(unsigned short)prodid.signature[4],
				(unsigned short)prodid.signature[3],
				(unsigned short)prodid.signature[2],
				(unsigned short)prodid.signature[1],
				(unsigned short)prodid.signature[0]);
    lir_text(2,11,s);
    SNDLOG"%s\n",s);
    sprintf(s,"Hardware version/revision: %d.%d",
                         (unsigned)prodid.hwver, (unsigned)prodid.hwrel);
    lir_text(2,12,s);
    SNDLOG"%s\n",s);
    lir_text(10,16,"PRESS ANY KEY");
    await_processed_keyboard();
    if(kill_all_flag) goto pers_errexit;
    clear_screen();
    sprintf(s,"%s selected for input",ss);
    lir_text(10,line,s);
    line+=2;
#if(OSNUM == OSNUM_WINDOWS)
    for(i=0; i<MAX_PERSEUS_RATES; i++)
      {
      perseus_rates[i]=PERSEUS_SAMPLING_CLOCK/rate_factor[i];
      }
    no_of_perseus_rates=MAX_PERSEUS_RATES;
#else
    i=fill_perseus_rates();
    if(i != 0)
      {
      lirerr(-i);
      return;
      }
    for(i=0; i<no_of_perseus_rates; i++)
      {
      rate_factor[i]=PERSEUS_SAMPLING_CLOCK/perseus_rates[i];
      }
#endif    
    k=no_of_perseus_rates-1;
    if(ui.operator_skil == OPERATOR_SKIL_NEWCOMER)k-=2;
    for(i=0; i<no_of_perseus_rates; i++)
      {
      if(i>k)settextcolor(8);
      if(perseus_rates[i] > 999999.9)
        {
        sprintf(s,"%d %8.3f MHz ",i,0.000001*perseus_rates[i]);
        }
      else
        {
        sprintf(s,"%d %8.3f kHz ",i,0.001*perseus_rates[i]);
        }
      lir_text(5,line,s);
      line++;
      }
    settextcolor(7);
    line++;
    sprintf(s,"Set USB sampling rate (by number 0 to %d)",k);
    lir_text(5,line,s);
    pers.rate_no=lir_get_integer(50, line, 2, 0, k);
    if(kill_all_flag)goto pers_errexit;
    line++;
    if(ui.operator_skil == OPERATOR_SKIL_NEWCOMER)
      {
      pers.clock_adjust=0;
      }
    else
      {
      sprintf(s,"Set sampling clock shift (Hz))");
      lir_text(5,line,s);
      pers.clock_adjust=lir_get_integer(50, line, 5,-10000,10000);
      if(kill_all_flag)goto pers_errexit;
      line++;
      }
    adjusted_sdr_clock=PERSEUS_SAMPLING_CLOCK+pers.clock_adjust;
    ui.rx_ad_speed=rint(adjusted_sdr_clock/rate_factor[pers.rate_no]);
    if(ui.operator_skil == OPERATOR_SKIL_NEWCOMER)
      {
      pers.presel=1;
      pers.preamp=1;
      pers.dither=1;
      }
    else
      {
      lir_text(5,line,"Enable preselector ? (0/1)");
      pers.presel=lir_get_integer(32, line, 1, 0, 1);
      if(kill_all_flag)goto pers_errexit;
      line++;
      lir_text(5,line,"Enable preamp ? (0/1)");
      pers.preamp=lir_get_integer(27, line, 1, 0, 1);
      if(kill_all_flag)goto pers_errexit;
      line++;
      lir_text(5,line,"Enable dither ? (0/1)");
      pers.dither=lir_get_integer(27, line, 1, 0, 1);
      if(kill_all_flag)goto pers_errexit;
      line++;
      }
    pers.check=PERSEUS_PAR_VERNR;
    perseus_file=fopen(perseus_parfil_name,"w");
    if(perseus_file == NULL)
      {
pers_errexit:;
      close_perseus();
      clear_screen();
      ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
      return;
      }
    sdr_pi=(int*)(&pers);
    for(i=0; i<MAX_PERSEUS_PARM; i++)
      {
      fprintf(perseus_file,"%s [%d]\n",perseus_parm_text[i],sdr_pi[i]);
      }
    parfile_end(perseus_file);
    }
  else
    {
    if(lir_inkey != 'N')goto qpers;
    clear_screen();
    }
  close_perseus();
  sdr=0;
  }
else
  {
  clear_screen();
  lir_text(3,3,"Could not open Perseus.");
  lir_text(3,5,"Is the USB cable connected and the unit powered?");
  line=7;
#if OSNUM == OSNUM_WINDOWS
  lir_text(3,line,"Are drive routines installed?");
  line+=2;
  lir_text(3,line,"Make sure the latest dll package is installed.");
  line++;
  lir_text(3,line,"Look here: http://sm5bsz.com/dll.htm .");
#endif  
  SNDLOG"\nopen_perseus failed.\n");
  lir_text(8,line+4,press_any_key);
  await_processed_keyboard();
  }
return;
}

void set_perseus_att(void)
{
int i;
i=-fg.gain/10;
perseus_store_att(i);
}

void perseus_input(void)
{
int rxin_local_workload_reset;
int i, j, errcod;
char s[80];
double dt1, read_start_time, total_reads;
int timing_loop_counter,timing_loop_counter_max,initial_skip_flag;
float t1;
int local_att_counter;
int local_nco_counter;
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_PERSEUS_INPUT);
#endif
callback_running=FALSE;
local_att_counter=sdr_att_counter;
local_nco_counter=sdr_nco_counter;
dt1=current_time();
j=0;
while(sdr == -1)
  {
  open_perseus();
  if(sdr == -1)
    {
    sprintf(s,"Waiting %.2f", current_time()-dt1);
    lir_text(3,5,s);
    lir_refresh_screen();
    if(kill_all_flag)goto perseus_init_error_exit;
    lir_sleep(100000);
    if(j==0)
      {
      clear_screen();
      j=1;
      }
    }
  }
errcod=1365;
#if OSNUM != OSNUM_WINDOWS
if(!libusb1_library_flag)goto perseus_error_exit; 
#endif
// ****************************************************
// We have a connection to the Perseus.
i=read_sdrpar(perseus_parfil_name, MAX_PERSEUS_PARM, 
                                     perseus_parm_text, (int*)((void*)&pers));
errcod=1080;
if(i != 0)goto perseus_error_exit;
if( pers.check != PERSEUS_PAR_VERNR ||
    pers.rate_no < 0 ||
    abs(pers.clock_adjust) > 10000)goto perseus_error_exit;
#if(OSNUM == OSNUM_WINDOWS)
if(pers.rate_no  >= MAX_PERSEUS_RATES)goto perseus_error_exit;
if(perseus_dll_version1 == 3 && (perseus_dll_version2 == 5 ||
                                 perseus_dll_version2 == 6))
  {                                 
  usb_blocksize=OLD_USB_BLOCKSIZE;
  }
else
  {                                 
  usb_blocksize=NEW_USB_BLOCKSIZE;
  }
fprintf( stderr,"\nperseususb.dll version = %d.%d",perseus_dll_version1,
                                                  perseus_dll_version2);  
#endif
#if(OSNUM == OSNUM_LINUX)
fill_perseus_rates();
if(pers.rate_no  >= no_of_perseus_rates)goto perseus_error_exit;
for(i=0; i<no_of_perseus_rates; i++)
  {
  rate_factor[i]=PERSEUS_SAMPLING_CLOCK/perseus_rates[i];
  }
if(no_of_perseus_rates <=9)
  {
  usb_blocksize=OLD_USB_BLOCKSIZE;
  }
else
  {
  usb_blocksize=NEW_USB_BLOCKSIZE;
  }
#endif    
errcod=0;
adjusted_sdr_clock=PERSEUS_SAMPLING_CLOCK+pers.clock_adjust;
t1=ui.rx_ad_speed-adjusted_sdr_clock/rate_factor[pers.rate_no];
if(fabs(t1/ui.rx_ad_speed) > 0.0001)
  {
  errcod=1091;
  goto perseus_error_exit;
  }
errcod=perseus_load_proprietary_code(pers.rate_no);
if(errcod != 0)goto perseus_error_exit;
ui.rx_ad_speed=adjusted_sdr_clock/rate_factor[pers.rate_no];
timf1_sampling_speed=adjusted_sdr_clock/rate_factor[pers.rate_no];
check_filtercorr_direction();
set_perseus_att();
set_perseus_frequency();
fft1_block_timing();
if(thread_command_flag[THREAD_SCREEN]!=THRFLAG_NOT_ACTIVE)
  {
  while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
    {
    if(thread_command_flag[THREAD_PERSEUS_INPUT] ==
                                           THRFLAG_KILL)goto perseus_error_exit;
    lir_sleep(10000);
    }
  }
start_perseus_read();
if(sdr != 0) goto perseus_error_exit;
lir_status=LIR_OK;
#include "timing_setup.c"
thread_status_flag[THREAD_PERSEUS_INPUT]=THRFLAG_ACTIVE;
while(thread_command_flag[THREAD_PERSEUS_INPUT] == THRFLAG_ACTIVE)
  {
  if(local_att_counter != sdr_att_counter)
    {
    local_att_counter=sdr_att_counter;
    set_perseus_att();
    }
  if(local_nco_counter != sdr_nco_counter)
    {
    local_nco_counter=sdr_nco_counter;
    set_perseus_frequency();
    }
  lir_perseus_read();
  if(kill_all_flag)goto perseus_exit;
  while (!kill_all_flag && 
      ((timf1p_sdr-timf1p_pa+timf1_bytes)&timf1_bytemask) 
                                         >= (int)snd[RXAD].block_bytes)
    {
#include "input_speed.c"
    finish_rx_read();
    }
  }
perseus_exit:;
perseus_stop();
perseus_error_exit:;
if(!kill_all_flag && errcod != 0)lirerr(errcod);
close_perseus();
perseus_init_error_exit:;
thread_status_flag[THREAD_PERSEUS_INPUT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_PERSEUS_INPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

