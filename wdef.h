#pragma once

#ifndef __MINGW32__
#include <Windows.h>
#endif

#include <windows.h>
#define NUMBER_HOME       36
#define NUMBER_UP         38
#define NUMBER_PGUP       33
#define NUMBER_LEFT       37
#define NUMBER_RIGHT      39
#define NUMBER_END        35
#define NUMBER_DOWN       40
#define NUMBER_PGDN       34
#define NUMBER_INSERT     45
#define NUMBER_DELETE     46
#define FUNCTION_F1		  112
#define FUNCTION_F2		  113
#define FUNCTION_F3		  114
#define FUNCTION_F4		  115
#define FUNCTION_F5		  116
#define FUNCTION_F6		  117
#define FUNCTION_F7		  118
#define FUNCTION_F8		  119
#define FUNCTION_F9		  120
#define FUNCTION_F11		  122
#define FUNCTION_F12		  123
#define FUNCTION_SHIFT             16

#define MAX_WAV_DEVICES 32
// Max interrupt rate is 1kHz. We want 0.1 seconds minimum
// to stay in the output device so we may need many buffers.

extern DWORD WINAPI winthread_kill_all(PVOID arg);
extern HANDLE thread_handle_kill_all;
extern HANDLE thread_handle_html_server;
extern DWORD thread_id_kill_all;
extern RECT desktop_screen;
extern HWND linrad_hwnd;
extern int shift_key_status;
extern HDC screen_hdc;
extern HDC memory_hdc;
extern HANDLE thread_handle_main_menu;
extern DWORD thread_id_main_menu;
extern HANDLE thread_handle_mouse;
extern DWORD thread_id_mouse;
extern DWORD thread_result;
extern double internal_clock_frequency;
extern LONG internal_clock_offset;
extern HBITMAP memory_hbm;
extern BITMAP memory_bm;
extern unsigned char *mempix;
extern int first_mempix;
extern int last_mempix;
extern HWAVEIN hwav_rxadin1;
extern HWAVEIN hwav_rxadin2;
extern HWAVEIN hwav_txadin;
extern HWAVEOUT hwav_rxdaout;
extern HWAVEOUT hwav_txdaout;
extern char rxad_mme_open_flag1;
extern char rxad_mme_open_flag2;
extern char txad_mme_open_flag;
extern char *rx_wavein_buf;
extern char *tx_wavein_buf;
extern char *rx_waveout_buf;
extern char *tx_waveout_buf;
extern WAVEHDR *rx_wave_inhdr;
extern WAVEHDR *rx_wave_outhdr;
extern WAVEHDR *tx_wave_inhdr;
extern WAVEHDR *tx_wave_outhdr;
extern HANDLE rxin1_bufready;
extern HANDLE rxin2_bufready;
extern HANDLE txin_bufready;
extern size_t *rxadin1_newbuf;
extern size_t *rxadin2_newbuf;
extern size_t *txadin_newbuf;
extern WAVEHDR* *rxdaout_newbuf;
extern WAVEHDR* *txdaout_newbuf;
extern size_t rxadin1_newbuf_ptr;
extern size_t rxadin2_newbuf_ptr;
extern size_t txadin_newbuf_ptr;
extern size_t rxdaout_newbuf_ptr;
extern size_t txdaout_newbuf_ptr;
extern HANDLE lir_event[];
extern CRITICAL_SECTION protect_event[];
extern CRITICAL_SECTION windows_mutex[MAX_LIRMUTEX];
extern CRITICAL_SECTION screen_update_mutex;
extern CRITICAL_SECTION protect_rxin1, protect_rxin2, protect_txin;

extern HANDLE thread_identifier[];
extern int parport_installed;
extern HANDLE serport;

typedef DWORD (WINAPI *WIN_ROUTINE) (PVOID arg);
extern LPTHREAD_START_ROUTINE thread_routines[];

typedef short (WINAPI *inpfuncPtr)(short portaddr);
typedef void (WINAPI *oupfuncPtr)(short portaddr, short datum);
typedef BOOL (WINAPI *inp32query)(void);
extern inpfuncPtr inp32;
extern oupfuncPtr oup32;
extern inp32query IsInpOutDriverOpen;

extern HANDLE CurrentProcess;

DWORD WINAPI winthread_mouse(PVOID arg);
DWORD WINAPI winthread_rx_adinput(PVOID arg);
DWORD WINAPI winthread_rx_raw_netinput(PVOID arg);
DWORD WINAPI winthread_rx_fft1_netinput(PVOID arg);
DWORD WINAPI winthread_rx_file_input(PVOID arg);
DWORD WINAPI winthread_rx_output(PVOID arg);
DWORD WINAPI winthread_screen(PVOID arg);
DWORD WINAPI winthread_tx_input(PVOID arg);
DWORD WINAPI winthread_tx_output(PVOID arg);
DWORD WINAPI winthread_wideband_dsp(PVOID arg);
DWORD WINAPI winthread_second_fft(PVOID arg);
DWORD WINAPI winthread_timf2(PVOID arg);
DWORD WINAPI winthread_narrowband_dsp(PVOID arg);
DWORD WINAPI winthread_user_command(PVOID arg);
DWORD WINAPI winthread_txtest(PVOID arg);
DWORD WINAPI winthread_powtim(PVOID arg);
DWORD WINAPI winthread_rx_adtest(PVOID arg);
DWORD WINAPI winthread_cal_iqbalance(PVOID arg);
DWORD WINAPI winthread_cal_interval(PVOID arg);
DWORD WINAPI winthread_cal_filtercorr(PVOID arg);
DWORD WINAPI winthread_sdr14_input(PVOID arg);
DWORD WINAPI winthread_tune(PVOID arg);
DWORD WINAPI winthread_main_menu(PVOID arg);
DWORD WINAPI winthread_ui_update(PVOID arg);
DWORD WINAPI winthread_lir_server(PVOID arg);
DWORD WINAPI winthread_perseus_input(PVOID arg);
DWORD WINAPI winthread_radar(PVOID arg);
DWORD WINAPI winthread_blocking_rxout(PVOID arg);
DWORD WINAPI winthread_syscall(PVOID arg);
DWORD WINAPI winthread_sdrip_input(PVOID arg);
DWORD WINAPI winthread_cloudiq_input(PVOID arg);
DWORD WINAPI winthread_excalibur_input(PVOID arg);
DWORD WINAPI winthread_hware_command(PVOID arg);
DWORD WINAPI winthread_extio_input(PVOID arg);
DWORD WINAPI winthread_write_raw_file(PVOID arg);
DWORD WINAPI winthread_rtl2832_input(PVOID arg);
DWORD WINAPI winthread_rtl_starter(PVOID arg);
DWORD WINAPI winthread_mirics_input(PVOID arg);
DWORD WINAPI winthread_bladerf_input(PVOID arg);
DWORD WINAPI winthread_pcie9842_input(PVOID arg);
DWORD WINAPI winthread_openhpsdr_input(PVOID arg);
DWORD WINAPI winthread_mirisdr_starter(PVOID arg);
DWORD WINAPI winthread_bladerf_starter(PVOID arg);
DWORD WINAPI winthread_html_server(PVOID arg);
DWORD WINAPI winthread_netafedri_input(PVOID arg);
DWORD WINAPI winthread_do_fft1c(PVOID arg);
DWORD WINAPI winthread_fft1b(PVOID arg);
DWORD WINAPI winthread_airspy_input(PVOID arg);
DWORD WINAPI winthread_network_send(PVOID arg);
DWORD WINAPI winthread_airspyhf_input(PVOID arg);
DWORD WINAPI winthread_sdrplay2_input(PVOID arg);
DWORD WINAPI winthread_sdrplay3_input(PVOID arg);


LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);

#define WAVE_FORMAT_EXTENSIBLE          0xFFFE

typedef struct 
  {
  WAVEFORMATEX  Format;
  union 
    {
    WORD  wValidBitsPerSample;
    WORD  wSamplesPerBlock;
    WORD  wReserved;
    } Samples;
  DWORD   dwChannelMask;
  GUID    SubFormat;
  } 
WAVEFORMATEXTENSIBLE, *PWAVEFORMATEXTENSIBLE;

// A "GUID" for working with WAVEFORMATEXTENSIBLE 
// struct (for PCM audio stream)
static const  GUID KSDATAFORMAT_SUBTYPE_PCM = {
       0x1,0x0000,0x0010,{0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71} };
void close_rx_input(void);
void ui_setup(void);
void store_in_kbdbuf(int c);
void wxmouse(void);

// Load a font definition from par_userfont.
// If the definition is not valid, or if forceDialog is true,
// open a Win32 Common Dialog for font selection.
// Once the font has been selected, store it back into par_userfont.
extern void win_init_font_GDI(LOGFONT *logfont, bool forceDialog);

// Select the logfont into the GDI device context of the back buffer. 
extern void win_load_font_GDI(LOGFONT *logfont);
