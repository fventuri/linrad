//  Copyright (c) <2013> <Mike. J. Keehan>
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
//
//
//
////////////////////////////////////////////////////////
//
//  fcdpp.h     header file for the FCDPP Linrad driver
//
//
//  This file is based on FCDHidCmd.h from Howard Long's
//  FCHid2002 program.
//
//  M. J. Keehan, 25th Feb 2013  (mike@keehan.net)
//
////////////////////////////////////////////////////////


#define FCD_HID_CMD_QUERY              1 // Returns string with "FCDAPP version"

#define FCD_HID_CMD_SET_FREQUENCY    100 // Send with 3 byte unsigned little endian frequency in kHz.
#define FCD_HID_CMD_SET_FREQUENCY_HZ 101 // Send with 4 byte unsigned little endian frequency in Hz,
                                         // returns with actual frequency set in Hz
#define FCD_HID_CMD_GET_FREQUENCY_HZ 102 // Returns 4 byte unsigned little endian frequency in Hz.

#define FCD_HID_CMD_SET_LNA_GAIN     110 // Send one byte, 1 on, 0 off
#define FCD_HID_CMD_SET_RF_FILTER    113 // Send one byte enum, see TUNERRFFILTERENUM
#define FCD_HID_CMD_SET_MIXER_GAIN   114 // Send one byte, 1 on, 0 off
#define FCD_HID_CMD_SET_IF_GAIN      117 // Send one byte value, valid value 0 to 59 (dB)
#define FCD_HID_CMD_SET_IF_FILTER    122 // Send one byte enum, see TUNERIFFILTERENUM
#define FCD_HID_CMD_SET_BIAS_TEE     126 // Send one byte, 1 on, 0 off

#define FCD_HID_CMD_GET_LNA_GAIN     150 // Returns one byte, 1 on, 0 off
#define FCD_HID_CMD_GET_RF_FILTER    153 // Returns one byte enum, see TUNERRFFILTERENUM
#define FCD_HID_CMD_GET_MIXER_GAIN   154 // Returns one byte, 1 on, 0 off
#define FCD_HID_CMD_GET_IF_GAIN      157 // Returns one byte value, valid value 0 to 59 (dB)
#define FCD_HID_CMD_GET_IF_FILTER    162 // Returns one byte enum, see TUNERIFFILTERENUM
#define FCD_HID_CMD_GET_BIAS_TEE     166 // Returns one byte, 1 on, 0 off

#define FCD_RESET                    255 // Reset to bootloader

typedef enum 
{
  TRFE_0_4=0,
  TRFE_4_8=1,
  TRFE_8_16=2,
  TRFE_16_32=3,
  TRFE_32_75=4,
  TRFE_75_125=5,
  TRFE_125_250=6,
  TRFE_145=7,
  TRFE_410_875=8,
  TRFE_435=9,
  TRFE_875_2000=10
} TUNERRFFILTERENUM;

typedef enum
{
  TIFE_200KHZ=0,
  TIFE_300KHZ=1,
  TIFE_600KHZ=2,
  TIFE_1536KHZ=3,
  TIFE_5MHZ=4,
  TIFE_6MHZ=5,
  TIFE_7MHZ=6,
  TIFE_8MHZ=7
} TUNERIFFILTERENUM;



