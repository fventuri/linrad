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
#include <pthread.h>
#include <semaphore.h>
#if HAVE_OSS == 1
  #if OSSD == 1
  #include <sys/soundcard.h>
  #else
  #include <soundcard.h>
  #endif
#include <sys/ioctl.h>
#endif


#include "globdef.h"
#include "thrdef.h"
#include "caldef.h"
#include "xdef.h"

pthread_mutex_t linux_mutex[MAX_LIRMUTEX];

#if HAVE_OSS == 1 
audio_buf_info tx_da_info;
audio_buf_info tx_ad_info;
audio_buf_info rx_da_info;
#endif
pthread_t thread_identifier_kill_all;
pthread_t thread_identifier_keyboard;
pthread_t thread_identifier_mouse;
pthread_t thread_identifier_main_menu;
pthread_t thread_identifier_html_server;
char *behind_mouse;
pthread_t thread_identifier[THREAD_MAX];
int serport;
pthread_mutex_t lir_event_mutex[MAX_LIREVENT];
int lir_event_flag[MAX_LIREVENT];
pthread_cond_t lir_event_cond[MAX_LIREVENT];
void *extio_handle;



ROUTINE thread_routine[THREAD_MAX]={thread_rx_adinput,          //0
                                    thread_rx_raw_netinput,     //1
                                    thread_rx_fft1_netinput,    //2
                                    thread_rx_file_input,       //3
                                    thread_sdr14_input,         //4
                                    thread_rx_output,           //5
                                    thread_screen,              //6
                                    thread_tx_input,            //7
                                    thread_tx_output,           //8
                                    thread_wideband_dsp,        //9
                                    thread_narrowband_dsp,      //10
                                    thread_user_command,        //11
                                    thread_txtest,              //12
                                    thread_powtim,              //13
                                    thread_rx_adtest,           //14
                                    thread_cal_iqbalance,       //15
                                    thread_cal_interval,        //16
                                    thread_cal_filtercorr,      //17
                                    thread_tune,                //18
                                    thread_lir_server,          //19
                                    thread_perseus_input,       //20
                                    thread_radar,               //21
                                    thread_second_fft,          //22
                                    thread_timf2,               //23
                                    thread_blocking_rxout,      //24
                                    thread_syscall,             //25 
                                    thread_sdrip_input,         //26
                                    thread_excalibur_input,     //27
                                    thread_hware_command,       //28
                                    thread_extio_input,         //29
                                    thread_write_raw_file,      //30
                                    thread_rtl2832_input,       //31
                                    thread_rtl_starter,         //32
                                    thread_mirics_input,        //33 
                                    thread_bladerf_input,       //34
                                    thread_pcie9842_input,      //35
                                    thread_openhpsdr_input,     //36
                                    thread_mirisdr_starter,     //37
                                    thread_bladerf_starter,     //38
                                    thread_netafedri_input,     //39
                                    thread_do_fft1c,            //40
                                    thread_fft1b,               //41
                                    thread_fft1b,               //42
                                    thread_fft1b,               //43
                                    thread_fft1b,               //44
                                    thread_fft1b,               //45
                                    thread_fft1b,               //46
                                    thread_fdms1_input,         //47
                                    thread_fdms1_starter,       //48
                                    thread_airspy_input,        //49
                                    thread_cloudiq_input,       //50
                                    thread_network_send,        //51
                                    thread_airspyhf_input,      //52
                                    thread_sdrplay2_input,      //53
                                    thread_sdrplay3_input       //54
                                    };
char *eme_own_info_filename={"/home/emedir/own_info"};
char *eme_allcalls_filename={"/home/emedir/allcalls.dta"};
char *eme_emedta_filename={"/home/emedir/eme.dta"};
char *eme_dirskd_filename={"/home/emedir/dir.skd"};
char *eme_dxdata_filename={"/home/emedir/linrad_dxdata"};
char *eme_call3_filename={"/home/emedir/CALL3.TXT"};
char *eme_error_report_file={"/home/emedir/location_errors.txt"};
