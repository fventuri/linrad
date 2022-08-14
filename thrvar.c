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
#if OSNUM == OSNUM_WINDOWS
#include <windows.h>
#endif
#include "thrdef.h"


volatile int32_t thread_command_flag[THREAD_MAX];
volatile int32_t thread_status_flag[THREAD_MAX];
char mouse_thread_flag;
float thread_workload[THREAD_MAX];
double thread_tottim1[THREAD_MAX];
double thread_cputim1[THREAD_MAX];
double thread_tottim2[THREAD_MAX];
double thread_cputim2[THREAD_MAX];
int thread_pid[THREAD_MAX];


int rx_input_thread;
int threads_running;
int ampinfo_reset;

int thread_waitsem[THREAD_MAX]={
                    -1,              //0 = THREAD_RX_ADINPUT
                    -1,              //1 = THREAD_RX_RAW_NETINPUT
                    -1,              //2 = THREAD_RX_FFT1_NETINPUT
                    -1,              //3 = THREAD_RX_FILE_INPUT
                    -1,              //4 = THREAD_SDR14_INPUT
             EVENT_RX_START_DA,      //5 = THREAD_RX_OUTPUT
             EVENT_SCREEN,           //6 = THREAD_SCREEN
                    -1,              //7 = THREAD_TX_INPUT
             EVENT_TX_INPUT,         //8 = THREAD_TX_OUTPUT
             EVENT_TIMF1,            //9 = THREAD_WIDEBAND_DSP
             EVENT_FFT1_READY,       //10 = THREAD_NARROWBAND_DSP
             EVENT_KEYBOARD,         //11 = THREAD_USER_COMMAND
             EVENT_FFT1_READY,       //12 = THREAD_TXTEST
             EVENT_FFT1_READY,       //13 = THREAD_POWTIM
             EVENT_TIMF1,            //14 = THREAD_RX_ADTEST
             EVENT_FFT1_READY,       //15 = THREAD_CAL_IQBALANCE
             EVENT_TIMF1,            //16 = THREAD_CAL_INTERVAL
             EVENT_FFT1_READY,       //17 = THREAD_CAL_FILTERCORR
             EVENT_TIMF1,            //18 = THREAD_TUNE
                    -1,              //19 = THREAD_LIR_SERVER 
             EVENT_PERSEUS_INPUT,    //20 = THREAD_PERSEUS_INPUT
             EVENT_FFT1_READY,       //21 = THREAD_RADAR             
             EVENT_FFT2,             //22 = THREAD_SECOND_FFT
             EVENT_TIMF2,            //23 = THREAD_TIMF2
             EVENT_BLOCKING_RXOUT,   //24 = THREAD_BLOCKING_RXOUT
             EVENT_SYSCALL,          //25 = THREAD_SYSCALL
                    -1,              //26 = THREAD_SDRIP_INPUT
                    -1,              //27 = THREAD_EXCALIBUR_INPUT
                    -1,              //28 = THREAD_HWARE_COMMAND
             EVENT_EXTIO_RXREADY,    //29 = THREAD_EXTIO_INPUT
             EVENT_WRITE_RAW_FILE,   //30 = THREAD_WRITE_RAW_FILE
             EVENT_HWARE1_RXREADY,   //31 = THREAD_RTL2832_INPUT
                    -2,              //32 = THREAD_RTL_STARTER
             EVENT_HWARE1_RXREADY,   //33 = THREAD_MIRISDR_INPUT
             EVENT_HWARE1_RXREADY,   //34 = THREAD_BLADERF_INPUT
             EVENT_HWARE1_RXREADY,   //35 = THREAD_PCIE9842_INPUT
                    -1,              //36 = THREAD_OPENHPSDR_INPUT
                    -2,              //37 = THREAD_MIRISDR_STARTER
                    -2,              //38 = THREAD_BLADERF_STARTER
                    -1,              //39 = THREAD NETAFEDRI_INPUT 
             EVENT_DO_FFT1C,         //40 = THREAD_DO_FFT1C
             EVENT_DO_FFT1B1,        //41 = THREAD_FFT1B1
             EVENT_DO_FFT1B2,        //42 = THREAD_FFT1B2
             EVENT_DO_FFT1B3,        //43 = THREAD_FFT1B3
             EVENT_DO_FFT1B4,        //44 = THREAD_FFT1B4
             EVENT_DO_FFT1B5,        //45 = THREAD_FFT1B5
             EVENT_DO_FFT1B6,        //46 = THREAD_FFT1B6
             EVENT_FDMS1_INPUT,      //47 = THREAD_FDMS1_INPUT
                    -2,              //48 = THREAD_FDMS1_STARTER
             EVENT_HWARE1_RXREADY,   //49 = THREAD_AIRSPY_INPUT
                    -1,              //50 = THREAD_CLOUDIQ_INPUT
             EVENT_NETWORK_SEND,     //51 = THREAD_NETWORK_SEND
             EVENT_HWARE1_RXREADY,   //52 = THREAD_AIRSPYHF_INPUT
             EVENT_HWARE1_RXREADY,   //53 = THREAD_SDRPLAY2_INPUT
             EVENT_HWARE1_RXREADY,   //54 = THREAD_SDRPLAY3_INPUT
                     };


char *thread_names[THREAD_MAX]={"RxAD",  //0 = THREAD_RX_ADINPUT
                                "Rxrn",  //1 = THREAD_RX_RAW_NETINPUT
                                "Rxfn",  //2 = THREAD_RX_FFT1_NETINPUT
                                "Rxfi",  //3 = THREAD_RX_FILE_INPUT
                                "SR14",  //4 = THREAD_SDR14_INPUT
                                "RxDA",  //5 = THREAD_RX_OUTPUT
                                "Scre",  //6 = THREAD_SCREEN
                                "TxAD",  //7 = THREAD_TX_INPUT
                                "TxDA",  //8 = THREAD_TX_OUTPUT
                                "Wdsp",  //9 = THREAD_WIDEBAND_DSP
                                "Ndsp",  //10 = THREAD_NARROWBAND_DSP
                                "Cmds",  //11 = THREAD_USER_COMMAND
                                "TxTe",  //12 = THREAD_TXTEST
                                "PowT",  //13 = THREAD_POWTIM
                                "ADte",  //14 = THREAD_RX_ADTEST
                                "C_IQ",  //15 = THREAD_CAL_IQBALANCE
                                "C_in",  //16 = THREAD_CAL_INTERVAL
                                "C_fi",  //17 = THREAD_CAL_FILTERCOR
                                "Tune",  //18 = THREAD_TUNE
                                "Serv",  //19 = THREAD_LIR_SERVER 
                                "Pers",  //20 = THREAD_PERSEUS_INPUT
                                "Radr",  //21 = THREAD_RADAR
                                "fft2",  //22 = THREAD_SECOND_FFT
                                "Tf2 ",  //23 = THREAD_TIMF2
                                "RxDB",  //24 = THREAD_BLOCKING_RXOUT
                                "Sys ",  //25 = THREAD_SYSCALL
                                "SRIP",  //26 = THREAD_SDRIP_INPUT
                                "Exca",  //27 = THREAD_EXCALIBUR_INPUT
                                "Hwar",  //28 = THREAD_HWARE_COMMAND
                                "ExIO",  //29 = THREAD_EXTIO_INPUT
                                "WRAW",  //30 = THREAD_WRITE_RAW_FILE
                                "RTL ",  //31 = THREAD_RTL2832_INPUT
                                "RTLs",  //32 = THREAD_RTL_STARTER
                                "Miri",  //33 = THREAD_MIRISDR_INPUT
                                "Blad",  //34 = THREAD_BLADERF_INPUT
                                "9842",  //35 = THREAD_PCIE9842_INPUT
                                "HPSD",  //36 = THREAD_OPENHPSDR_INPUT
                                "MIRs",  //37 = THREAD_MIRISDR_STARTER
                                "Bl_s",  //38 = THREAD_BLADERF_STARTER
                                "Afed",  //39 = THREAD_NETAFEDRI_INPUT
                                "F1c1",  //40 = THREAD_DO_FFT1C
                                "F1b1",  //41 = THREAD_FFT1_B1
                                "F1b2",  //42 = THREAD_FFT1_B2
                                "F1b3",  //43 = THREAD_FFT1_B3
                                "F1b4",  //44 = THREAD_FFT1_B4
                                "F1b5",  //45 = THREAD_FFT1_B5
                                "F1b6",  //46 = THREAD_FFT1_B5
                                "Elad",  //47 = THREAD_FDMS1_INPUT
                                "FDMs",  //48 = THREAD_FDMS1_STARTER
                                "AIR ",  //49 = THREAD_AIRSPY_INPUT
                                "ClIQ",  //50 = THREAD CLOUDIQ_INPUT
                                "Send",  //51 = THREAD_NETWORK_SEND
                                "AHFP",  //52 = THREAD_AIRSPYHF_INPUT
                                "SP2i",  //53 = THREAD_SDRPLAY2_INPUT
                                "SP3i",  //53 = THREAD_SDRPLAY3_INPUT
                                 };
