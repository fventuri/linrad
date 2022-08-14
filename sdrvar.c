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
#include "sdrdef.h"

char *sdr14_name_string="SDR-14";
char *sdriq_name_string="SDR-IQ";
char *sdrip_name_string="SDR-IP";
char *perseus_name_string="Perseus";
char *excalibur_name_string="Excalibur";
char *rtl2832_name_string="RTL2832 USB";
char *mirics_name_string="Mirics USB";
char *sdrplay2_name_string="SDRPlay RSP1a/RSP2";
char *sdrplay3_name_string="SDRplay v3.X RSP1/RSP1A/RSP2/RSPduo/RSPdx";
char *bladerf_name_string="bladeRF";
char *pcie9842_name_string="PCIe9842";
char *openhpsdr_name_string="OpenHPSDR";
char *netafedri_name_string="Afedri NET";
char *fdms1_name_string="ELAD FDM-S1";
char *airspy_name_string="AIRSPY";
char *airspyhf_name_string="Airspy HF+";
char *cloudiq_name_string="CloudIQ";

char *pkgsize[2]={"Large","Small"};
char *onoff[2]={"OFF","ON"};
char *lowhigh[2]={"LOW","HIGH"};
char *adgains[2]={"1.0","1.5"};
char *inp_connect[2]={"Normal","Converter"};
char *rffil[2]={"Auto","Bypass"};





int *rtl2832_gains;
int no_of_rtl2832_gains;
int old_rtl2832_gain;
int *mirics_gains;
int no_of_mirics_gains;
int old_mirics_gain;
float airspyhf_old_point;


char *excalibur_sernum_filename="par_excalibur_sernum";


double adjusted_sdr_clock;
P_EXCALIBUR excalib;

int no_of_excalibur_rates;

