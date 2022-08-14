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
#include "wdef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "fft3def.h"
#include "screendef.h"
#include "sigdef.h"
#include "seldef.h"
#include "blnkdef.h"
#include "caldef.h"
#include "txdef.h"
#include "vernr.h"
#include "keyboard_def.h"

HANDLE thread_handle_kill_all;
DWORD thread_id_kill_all;
HANDLE thread_handle_html_server;
CRITICAL_SECTION protect_rxin1, protect_rxin2, protect_txin;

char *dllversfilename="C:\\Linrad\\dll\\linrad-dll-version.txt";
int dll_version;
HANDLE serport;
RECT desktop_screen;
HWND linrad_hwnd;
int shift_key_status=0;
HDC screen_hdc;
HDC memory_hdc;
HANDLE thread_handle_main_menu;
DWORD thread_id_main_menu;
HANDLE thread_handle_mouse;
DWORD thread_id_mouse;
DWORD thread_result;
double internal_clock_frequency;
LONG internal_clock_offset;
HBITMAP memory_hbm;
BITMAP memory_bm;
unsigned char *mempix;
int first_mempix;
int last_mempix;
HWAVEIN hwav_rxadin1;
HWAVEIN hwav_rxadin2;
HWAVEIN hwav_txadin;
HWAVEOUT hwav_rxdaout;
HWAVEOUT hwav_txdaout;
char rxad_mme_open_flag1;
char rxad_mme_open_flag2;
char txad_mme_open_flag;
char *rx_wavein_buf;
char *tx_wavein_buf;
char *rx_waveout_buf;
char *tx_waveout_buf;
WAVEHDR *rx_wave_inhdr;
WAVEHDR *rx_wave_outhdr;
WAVEHDR *tx_wave_inhdr;
WAVEHDR *tx_wave_outhdr;
HANDLE rxin1_bufready;
HANDLE rxin2_bufready;
HANDLE txin_bufready;
size_t *rxadin1_newbuf;
size_t *rxadin2_newbuf;
size_t *txadin_newbuf;
WAVEHDR* *rxdaout_newbuf;
WAVEHDR* *txdaout_newbuf;
size_t rxadin1_newbuf_ptr;
size_t rxadin2_newbuf_ptr;
size_t txadin_newbuf_ptr;
size_t rxdaout_newbuf_ptr;
size_t txdaout_newbuf_ptr;
int parport_installed;
inpfuncPtr inp32;
oupfuncPtr oup32;
inp32query IsInpOutDriverOpen;

HANDLE CurrentProcess;
HANDLE extio_handle;


char *eme_own_info_filename={"C:\\emedir\\own_info"};
char *eme_allcalls_filename={"C:\\emedir\\allcalls.dta"};
char *eme_emedta_filename={"C:\\emedir\\eme.dta"};
char *eme_dirskd_filename={"C:\\emedir\\dir.skd"};
char *eme_dxdata_filename={"C:\\emedir\\linrad_dxdata"};
char *eme_call3_filename={"C:\\emedir\\CALL3.TXT"};
char *eme_error_report_file={"C:\\emedir\\location_errors.txt"};
LPTHREAD_START_ROUTINE thread_routines[THREAD_MAX]=
                   {winthread_rx_adinput,        //0
                    winthread_rx_raw_netinput,   //1
                    winthread_rx_fft1_netinput,  //2
                    winthread_rx_file_input,     //3
                    winthread_sdr14_input,       //4
                    winthread_rx_output,         //5
                    winthread_screen,            //6
                    winthread_tx_input,          //7
                    winthread_tx_output,         //8
                    winthread_wideband_dsp,      //9
                    winthread_narrowband_dsp,    //10
                    winthread_user_command,      //11
                    winthread_txtest,            //12
                    winthread_powtim,            //13
                    winthread_rx_adtest,         //14
                    winthread_cal_iqbalance,     //15
                    winthread_cal_interval,      //16
                    winthread_cal_filtercorr,    //17
                    winthread_tune,              //18
                    winthread_lir_server,        //19
                    winthread_perseus_input,     //20
                    winthread_radar,             //21
                    winthread_second_fft,        //22
                    winthread_timf2,             //23
                    winthread_blocking_rxout,    //24
                    winthread_syscall,           //25
                    winthread_sdrip_input,       //26
                    winthread_excalibur_input,   //27
                    winthread_hware_command,     //28
                    winthread_extio_input,       //29
                    winthread_write_raw_file,    //30
                    winthread_rtl2832_input,     //31
                    winthread_rtl_starter,       //32
                    winthread_mirics_input,      //33 
                    winthread_bladerf_input,     //34
                    winthread_pcie9842_input,    //35
                    winthread_openhpsdr_input,   //36
                    winthread_mirisdr_starter,   //37
                    winthread_bladerf_starter,   //38
                    winthread_netafedri_input,   //39
                    winthread_do_fft1c,          //40
                    winthread_fft1b,             //41
                    winthread_fft1b,             //42
                    winthread_fft1b,             //43
                    winthread_fft1b,             //44
                    winthread_fft1b,             //45
                    winthread_fft1b,             //46
                    NULL,                        //47
                    NULL,                        //48
                    winthread_airspy_input,      //49
                    winthread_cloudiq_input,     //50
                    winthread_network_send,      //51
                    winthread_airspyhf_input,    //52
                    winthread_sdrplay2_input,    //53
                    winthread_sdrplay3_input     //54
                    };

HANDLE thread_identifier[THREAD_MAX];
HANDLE lir_event[MAX_LIREVENT];
CRITICAL_SECTION windows_mutex[MAX_LIRMUTEX];
CRITICAL_SECTION screen_update_mutex;
