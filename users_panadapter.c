// Copyright (c) <2016> <Daniel Estevez>
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


// DESCRIPTION
//
//  When using Linrad as a panadapter, the SDR receiver is tuned to the IF
//  frequency of the radio used. Thus, the frequency display in Linrad is no
//  longer correct. This code corrects the frequency display in Linrad by
//  reading the dial frequency of the radio via CAT periodically (using rigctld,
//  from Hamlib).

// INSTALLATION
//
// To install under Linux for Linux:
//   download lirxx-xx.tbz and extract it to your linrad directory
//   rename users_panadapter.c to users_hwaredriver.c and rename
//   users_panadapter_ex.c to users_extra.c in your linrad directory
//   run ./configure
//   then the Makefile has to be edited to link against Hamlib
//   (this will probably change in future versions of Linrad)
//   to modify the Makefile, the easiest way is to replace the line
//   CC=gcc
//   by
//   CC=gcc -lhamlib
//   (of course hamlib needs to be installed in the system)
//   finally run make xlinrad (or make linrad) from your linrad
//   directory
//
// To generate a windows version under Linux: (not tested yet)
//   install mingw32 from Leif's website
//   download lirxx-xx.tbz and extract it to your linrad directory
//   rename users_panadapter.c to users_hwaredriver.c and rename
//   users_panadapter_ex.c to users_extra.c in your linrad directory
//   run ./configure
//   then the Makefile has to be edited to link against Hamlib
//   (this will probably change in future versions of Linrad)
//   (of course hamlib needs to be installed in the system)
//   finally run make linrad.exe from your linrad directory
//
// To generate a windows version under Windows: (not tested yet)
//   Install mingw and nasm from Leif's website in the C:\MinGW  directory
//   Download lirxx-xx.zip and unpack the linrad sources in your linrad
//   directory
//   rename users_panadapter.c to users_hwaredriver.c and rename
//   users_panadapter_ex.c to users_extra.c in your linrad directory
//   Start a MS-DOS Command Window on your computer: click the Windows Start
//   menu and enter the "cmd.exe" command into the run box
//   In the MS-DOS Command Window issue a "cd" command to your linrad directory
//   followed by a "configure.exe"
//   then the Makefile has to be edited to link against Hamlib
//   (this will probably change in future versions of Linrad)
//   (of course hamlib needs to be installed in the system)
//   finally run "make.bat"

// SETUP
//
//  First you need to measure the IF frequency of your radio. To do this, tune
//  an AM station or another strong carrier on your radio (in AM mode) and look
//  at the IF output of the radio with your SDR receiver. The frequency to which
//  the AM carrier gets converted is the IF frequency. For my FT-817ND it is
//  68.330130MHz.
//
//  Then you need to set up the frequency converter in Linrad. This is the E key
//  in the U menu (A/D and D/A setup for RX). Use the IF frequency you have
//  measured before as the LO frequency of the converter.  For my FT-817ND, I
//  set up an Upconverter with LO frequency of 68.330130MHz which is above the
//  IF frequency.  The way to choose and check the proper parameters for the
//  converter is as follows: Set up the converter, tune to an AM station or
//  carrier on your radio (in AM mode). Run Linrad with the converter enabled,
//  tune to 0MHz in Linrad and look at the waterfall. The carrier should be at
//  0MHz exactly.  The sidebands should go in the proper direction: stations
//  above the tuned station should appear on positive frequencies and stations
//  below the tuned station should appear on negative frequencies.

// USAGE
//
//  You need to run rigctld to control your radio using CAT. To check that
//  rigctld is working properly, run "rigctl -m 2 f". This should return the
//  current dial frequency on your radio.
//
//  When rigctld is running, you can now run Linrad and check that the frequency
//  display is correct and moves accordingly as you tune around with your radio.
//
//  Tips:
//
//  If your SDR receiver has a DC spike, this will be very near the centre of
//  the passband of your radio (it is not exact because the SDR receiver cannot
//  tune in steps of 1Hz or so). You can use this to help you locate the station
//  you have tuned.
//
//  If the IF output of your radio gives more bandwith that your SDR receiver
//  has (for instance, if you tap the FT-817ND before the crystal filter, your
//  IF tap will have several MHz of bandwidth), then you can use the frequency
//  control box in Linrad to move around in frequency. When you want to return
//  to the centre of the passband (with the DC spike very near the centre), the
//  easiest way is set your radio to AM mode, dial an exact frequency on your
//  radio and enter the same dial frequency into the frequency box in Linrad.

// CALIBRATION OF THE SSB OFFSETS
//
//  When using an SSB mode on your radio, the LO of the radio will be shifted by
//  about 1.5KHz (in comparison to the AM mode with the same dial frequency) to
//  make the corresponding sideband fall on the centre of the IF. These offsets
//  have been calibrated on my FT-817ND. To obtain a precise frequency readout
//  in SSB mode, you should calibrate the offsets for your particular radio (The
//  offsets are defined below).
//
//  To do or check the calibration, set your radio to AM mode and tune to or
//  near an AM station or other carrier which is on a spot exact frequency.
//  Note the frequency of the carrier in the waterfall in Linrad (you should
//  zoom in the waterfall to do the calibration). The carrier should be
//  displayed on the proper frequency. Now change to some SSB mode (CW, CWR,
//  DIG, RTTY, etc.  are also SSB modes). The waterfall will shift, but the
//  frequency display should shift by the same amount and the carrier should
//  still display the same frequency. If not, note the difference and correct
//  the offset accordingly. You should do this for all the different SSB modes
//  in your radio.

#include <hamlib/rig.h>
#include <hamlib/riglist.h>

static int enable_panadapter = 0;
static RIG *rig = NULL;
static double freq = 0.0;
static rmode_t mode;

// These should be adjusted for your particular transceiver.
// The offset is the difference between the frequency that gets converted
// to the centre of the IF and the dial frequency in the radio.
#define LSB_OFFSET -1502.0
#define USB_OFFSET 1497.0
#define CW_OFFSET (USB_OFFSET-700.0)
#define CWR_OFFSET -800.0

void panadapter_update_rig_freq(void) {
  int converter_direction;
  double new_freq;
  double ssb_offset = 0.0;
  rmode_t new_mode;
  pbwidth_t width;

  if (!enable_panadapter) return;
  
  rig_get_freq(rig, RIG_VFO_CURR, &new_freq);
  rig_get_mode(rig, RIG_VFO_CURR, &new_mode, &width);

  // run updates only if there is a change in freq or mode
  if (new_freq == freq && mode == new_mode) return;
  freq = new_freq;
  mode = new_mode;

  switch (mode) {
  case RIG_MODE_USB:
  case RIG_MODE_RTTY:
  case RIG_MODE_PKTUSB:
  case RIG_MODE_ECSSUSB:
  case RIG_MODE_SAH:
    ssb_offset = USB_OFFSET;
    break;
  case RIG_MODE_LSB:
  case RIG_MODE_RTTYR:
  case RIG_MODE_PKTLSB:
  case RIG_MODE_ECSSLSB:
  case RIG_MODE_SAL:
    ssb_offset = LSB_OFFSET;
    break;
  case RIG_MODE_CW:
    ssb_offset = CW_OFFSET;
    break;
  case RIG_MODE_CWR:
    ssb_offset = CWR_OFFSET;
    break;
  default:
    ssb_offset = 0.0;
    break;
  }

  converter_direction = ui.converter_mode & CONVERTER_LO_BELOW ? 1 : -1;
  converter_offset_mhz = ui.converter_mhz + 1e-6*(ui.converter_hz - converter_direction*freq + ssb_offset);

  // needed to update frequency in linrad internals
  check_filtercorr_direction();
  sc[SC_SHOW_CENTER_FQ]++;  
}


void users_close_devices(void) {
  if (enable_panadapter) {
    rig_close(rig);
    rig_cleanup(rig);
  }
  enable_panadapter = 0;
}


void users_open_devices(void) {
  /* Enable panadapter support only when a converter is set */
  if (ui.converter_mode == 0) return;

  enable_panadapter = 1;
  
  rig_set_debug_level(RIG_DEBUG_ERR);
  rig = rig_init(RIG_MODEL_NETRIGCTL);
  rig_open(rig);
}

void userdefined_u(void){};
void userdefined_q(void){};
void update_users_rx_frequency(void){};
void users_eme(void){};
void show_user_parms(void){};
void mouse_on_users_graph(void){};
void init_users_control_window(void){};
void users_init_mode(void){};

void users_set_band_no(void){};
#include "wse_sdrxx.c"
