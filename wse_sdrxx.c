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



extern char hware_outdat;
extern int sdrplay2_max_gain;
extern int sdrplay3_max_gain;

void hware_command(void)
{
#if OSNUM == OSNUM_LINUX
clear_thread_times(THREAD_HWARE_COMMAND);
#endif
thread_status_flag[THREAD_HWARE_COMMAND]=THRFLAG_ACTIVE;
while(!kill_all_flag && 
         thread_command_flag[THREAD_HWARE_COMMAND]==THRFLAG_ACTIVE)
  {
  lir_sleep(30000);
  while(hware_flag != 0)
    {
    control_hware();
    lir_sleep(3000);
    }
  }
thread_status_flag[THREAD_HWARE_COMMAND]=THRFLAG_RETURNED;
while(!kill_all_flag &&
            thread_command_flag[THREAD_HWARE_COMMAND] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

void set_hardware_tx_frequency(void)
{
if(ui.tx_dadev_no < SPECIFIC_DEVICE_CODES)
  {
  switch (ui.tx_soundcard_radio)
    {
    case TX_SOUNDCARD_RADIO_WSE:
    wse_tx_freq_control();
    break;
    
    case TX_SOUNDCARD_RADIO_SI570:
// Set the transmitter carrier frequency to tg.freq. 
    tg.passband_direction=si570.tx_passband_direction;
    tg.band_increment = 0.00001;
    switch (ui.transceiver_mode )
      {
      case 0:
// The transmitter has its own Si570
      break;
      
      case 1:
// The Si570 is common to Rx and Tx. Keep the Si570 frequency constant
// and change tg_basebfreq_hz as needed.
      tg_basebfreq_hz=1000000*(fg.passband_center-tg.freq);
      if(tg_basebfreq_hz > (float)ui.tx_da_speed*0.4F)
        {
        tg_basebfreq_hz=(float)ui.tx_da_speed*0.35F;
        tg.freq=fg.passband_center-tg_basebfreq_hz/1000000.F;
        }
      if(tg_basebfreq_hz < -(float)ui.tx_da_speed*0.4F)
        {
        tg_basebfreq_hz=-(float)ui.tx_da_speed*0.35F;
        tg.freq=fg.passband_center-tg_basebfreq_hz/1000000.F;
        }
      break;
      
      case 2:
// The Si570 is common to Rx and Tx. Keep the audio tone, tg_basebfreq_hz 
// constant and change the Si570 between Rx and tx.
      tx_passband_center=tg.freq+tg_basebfreq_hz/1000000.F;
      break;
      }
    make_tx_phstep();
    break;
    }
  }
else
  {
  switch (ui.tx_dadev_no)
    {
    case BLADERF_DEVICE_CODE:
    break;

    case OPENHPSDR_DEVICE_CODE:
    break;
    }
  }  
}

void hware_hand_key(void)
{
int i;
// This routine is called from the rx_input_thread.
// It is responsible for reading the hardware and setting
// hand_key to TRUE or FALSE depending on whether the hand key
// is depressed or not.
if(ui.tx_dadev_no < SPECIFIC_DEVICE_CODES)
  {
  switch (ui.tx_soundcard_radio)
    {
    case TX_SOUNDCARD_RADIO_WSE:
// disable the polling of the parallelport when USB2LPT is selected 
// (otherwise we get RX OVERRUN ERRORS )
    if(( wse.parport != 0 && allow_wse_parport) && 
                               wse.parport != USB2LPT16_PORT_NUMBER )
      {
      i=lir_inb(wse_parport_status)&WSE_PARPORT_MORSE_KEY;
      if(i==0)
        {
        hand_key=TRUE;
        }
      else
        {
        hand_key=FALSE;
        }
      }
    break;
    
    case TX_SOUNDCARD_RADIO_SI570:
    if(ui.ptt_control == 1)
      {
// We use the pilot tone to control PTT.       
      si570_ptt=0;
      }
    si570_get_ptt();
    break;
    }
  }
else
  {
  switch (ui.tx_dadev_no)  
    {
    case BLADERF_DEVICE_CODE:
    break;

    case OPENHPSDR_DEVICE_CODE:
    break;
    }
  }  
}  

void hware_set_ptt(int state)
{
if(ui.tx_dadev_no < SPECIFIC_DEVICE_CODES)
  {
  switch (ui.tx_soundcard_radio)
    {
    case TX_SOUNDCARD_RADIO_WSE:
    if( wse.parport != 0 && allow_wse_parport)
      {
      lir_mutex_lock(MUTEX_PARPORT);
      if(state == TRUE)
        {
        hware_outdat&=-1-WSE_PARPORT_PTT;
        }
      else
        {
        hware_outdat|=WSE_PARPORT_PTT;
        }
      lir_mutex_unlock(MUTEX_PARPORT);
      lir_outb(hware_outdat,wse_parport_control);
      }
    break;
    
    case TX_SOUNDCARD_RADIO_SI570:
    if(ui.ptt_control == 1)
      {
// We use the pilot tone to control PTT.       
      si570_ptt=0;
      }
    else
      {  
      if(si570_ptt != state)
        {
        si570_ptt=state;
        si570_set_ptt();
        }
      }
    break;
    }
  }
else
  {
  switch (ui.tx_dadev_no)  
    {
    case BLADERF_DEVICE_CODE:
    break;

    case OPENHPSDR_DEVICE_CODE:
    break;
    }
  }  
    
}

void set_hardware_rx_gain(void)
{
int i, j, k, m;
float t1;
if(diskread_flag >= 2)return;
if(ui.use_extio != 0)
  {
  update_extio_rx_gain();
  return;
  }
if(ui.rx_addev_no == SDR14_DEVICE_CODE)
  {
  fg.gain_increment=10;
  fg.gain/=10;
  fg.gain*=10;
  if(fg.gain > 0)fg.gain=0;
  if(fg.gain<-30)fg.gain=-30;
  sdr_att_counter++;
  return;
  }
if(ui.rx_addev_no == SDRIQ_DEVICE_CODE)
  {
  fg.gain_increment=1;
  if(fg.gain > 0)fg.gain=0;
  if(fg.gain<-36)fg.gain=-36;
  sdr_att_counter++;
  return;
  }
if(ui.rx_addev_no == SDRIP_DEVICE_CODE)
  {
  fg.gain_increment=10;
  fg.gain/=10;
  fg.gain*=10;
  if(fg.gain > 0)fg.gain=0;
  if(fg.gain<-30)fg.gain=-30;
  sdr_att_counter++;
  return;
  }
if(ui.rx_addev_no == PERSEUS_DEVICE_CODE)
  {
  fg.gain_increment=10;
  fg.gain/=10;
  fg.gain*=10;
  if(fg.gain > 0)fg.gain=0;
  if(fg.gain<-30)fg.gain=-30;
  sdr_att_counter++;
  return;
  }
if(ui.rx_addev_no == EXCALIBUR_DEVICE_CODE)
  {
  fg.gain_increment=3;
  fg.gain/=3;
  fg.gain*=3;
  if(fg.gain > 0)fg.gain=0;
  if(fg.gain<-21)fg.gain=-21;
  sdr_att_counter++;
  return;
  }
if(ui.rx_addev_no == BLADERF_DEVICE_CODE)
  {
  fg.gain_increment=3;
  fg.gain/=3;
  fg.gain*=3;
  if(fg.gain > 21)fg.gain=21;
  if(fg.gain<-21)fg.gain=-21;
  sdr_att_counter++;
  return;
  }
if(ui.rx_addev_no == AIRSPY_DEVICE_CODE)
  {
  fg.gain_increment=4;
  fg.gain/=4;
  fg.gain*=4;
  if(fg.gain > 0)fg.gain=0;
  if(fg.gain<-84)fg.gain=-84;
  sdr_att_counter++;
  return;
  }
if(ui.rx_addev_no == AIRSPYHF_DEVICE_CODE)
  {
  if(airspyhf.hf_agc == 1)
    {
    fg.gain_increment=0;
    fg.gain=0;
    return;
    }
  fg.gain_increment=6;
  fg.gain/=6;
  fg.gain*=6;
  if(fg.gain > 0)fg.gain=0;
  if(fg.gain<-48)fg.gain=-48;
  sdr_att_counter++;
  return;
  }
if(ui.rx_addev_no == RTL2832_DEVICE_CODE)
  {
  if(rtl2832.gain_mode == 0)
    {
    fg.gain=0;
    return;
    }
  fg.gain_increment=1;
  t1=(float)rint(0.1F*(float)old_rtl2832_gain);
  k=fg.gain-(int)t1;
  if(k == 0)return;
  if(abs(k) == fg.gain_increment)
    {
    j=0;
    m=10000;
    for(i=0; i<no_of_rtl2832_gains; i++)
      {
      if(abs(old_rtl2832_gain-rtl2832_gains[i]) < m)
        {
        m=abs(old_rtl2832_gain-rtl2832_gains[i]);
        j=i;
        }
      }
    if(k > 0)
      {
      j++;
      }
    else
      {
      j--;
      }     
    if(j < 0)j=0;
    if(j >= no_of_rtl2832_gains)j=no_of_rtl2832_gains-1;  
    }
  else
    {
    j=0;
    m=10000;
    for(i=0; i<no_of_rtl2832_gains; i++)
      {
      t1=(float)rint(0.1F*(float)rtl2832_gains[i]);
      if(abs(fg.gain-(int)t1) < m)
        {
        m=abs(fg.gain-(int)t1);
        j=i;
        }
      }
    }
  t1=(float)rint(0.1F*(float)rtl2832_gains[j]);  
  fg.gain=(int)t1;
  old_rtl2832_gain=rtl2832_gains[j];
  sdr_att_counter++;
  return;
  }
if(ui.rx_addev_no == MIRISDR_DEVICE_CODE)
  {
  fg.gain_increment=1;
  k=fg.gain-old_mirics_gain;
  if(k == 0)return;
  if(abs(k) == fg.gain_increment)
    {
    j=0;
    m=10000;
    for(i=0; i<no_of_mirics_gains; i++)
      {
      if(abs(old_mirics_gain-mirics_gains[i]) < m)
        {
        m=abs(old_mirics_gain-mirics_gains[i]);
        j=i;
        }
      }
    if(k > 0)
      {
      j++;
      }
    else
      {
      j--;
      }     
    if(j < 0)j=0;
    if(j >= no_of_mirics_gains)j=no_of_mirics_gains-1;  
    }
  else
    {
    j=0;
    m=10000;
    for(i=0; i<no_of_mirics_gains; i++)
      {
      if(abs(fg.gain-mirics_gains[i]) < m)
        {
        m=abs(fg.gain-mirics_gains[i]);
        j=i;
        }
      }
    }  
  fg.gain=mirics_gains[j];
  old_mirics_gain=mirics_gains[j];
  sdr_att_counter++;
  return;
  }
if(ui.rx_addev_no == SDRPLAY2_DEVICE_CODE)
  {
    fg.gain_increment=1;
    // the max value is different for each device type
    if(fg.gain > sdrplay2_max_gain)
        fg.gain = sdrplay2_max_gain;
    if(fg.gain < 0)
      fg.gain = 0;
    sdr_att_counter++;
    return;
  }
if(ui.rx_addev_no == SDRPLAY3_DEVICE_CODE)
  {
    fg.gain_increment=1;
    // the max value is different for each device type
    if(fg.gain > sdrplay3_max_gain)
        fg.gain = sdrplay3_max_gain;
    if(fg.gain < 0)
      fg.gain = 0;
    sdr_att_counter++;
    return;
  }
if(ui.rx_addev_no == NETAFEDRI_DEVICE_CODE)
  {
  fg.gain_increment=3;
  fg.gain+=10;
  fg.gain/=3;
  fg.gain*=3;
  fg.gain-=10;
  if(fg.gain > 35)fg.gain=35;
  if(fg.gain<-10)fg.gain=-10;
  sdr_att_counter++;
  return;
  }
if(ui.rx_addev_no == FDMS1_DEVICE_CODE)
  {
  fg.gain_increment=20;
  if(fg.gain > -10)
    {
    fg.gain=0;
    }
  else
    {
    fg.gain=-20;
    }
  sdr_att_counter++;
  return;
  }
switch (ui.rx_soundcard_radio)
  {
  case RX_SOUNDCARD_RADIO_WSE:
// bit0 to bit 2 for amplifier/bypass/-10 dB.
// bit0-bit2 = 6 => -10dB
// bit0-bit2 = 3 => 0dB
// bit0-bit2 = 5 => +10dB
//
// bit3 for 5 dB attenuator.
// bit3 = 0 => -5dB
// bit3 = 1 =>  0dB
  wse_rx_amp_control();
  break;

  case RX_SOUNDCARD_RADIO_SI570:
  fg.gain=0;
  break;

  case RX_SOUNDCARD_RADIO_ELEKTOR:
  elektor_rx_amp_control();
  break;

  case RX_SOUNDCARD_RADIO_FCDPROPLUS:
  fcdproplus_rx_amp_control();
  break;
  
  case RX_SOUNDCARD_RADIO_AFEDRI_USB:
  afedriusb_rx_amp_control();
  break;
  }
}

void set_hardware_rx_frequency(void)
{
float t1;
int i;
if(diskread_flag >= 2)
  {
  sc[SC_WG_FQ_SCALE]++;
  return;
  }
if(ui.use_extio != 0)
  {
  if(fg.passband_increment < 0.0001 || fg.passband_increment > 1.5)
    {   
    fg.passband_increment=0.01;
    }
  if(ui.use_extio < 0)
    {  
    fg.passband_direction = -1;
    }
  else
    {
    fg.passband_direction = 1;
    }      
  fft1_direction = fg.passband_direction;
  update_extio_rx_freq();
  }
if(rx_mode < MODE_TXTEST)users_set_band_no();
if(ui.rx_addev_no == SDR14_DEVICE_CODE ||
   ui.rx_addev_no == SDRIQ_DEVICE_CODE )
  {
  t1=(float)fg.passband_center;
  fg.passband_direction=1;
  if(fg.passband_center > adjusted_sdr_clock)
    {
    i=(int)(fg.passband_center/adjusted_sdr_clock);
    t1=(float)(fg.passband_center-i*adjusted_sdr_clock);
    }
  if(t1 > 0.5F*adjusted_sdr_clock)
    {
    t1=(float)adjusted_sdr_clock-t1;
    fg.passband_direction=-1;
    }
  fg.passband_increment=0.1;
  sdr_nco_counter++;
  goto setfq_x;
  }
if(ui.rx_addev_no == SDRIP_DEVICE_CODE ||
   ui.rx_addev_no == PERSEUS_DEVICE_CODE ||
   ui.rx_addev_no == EXCALIBUR_DEVICE_CODE ||
   ui.rx_addev_no == RTL2832_DEVICE_CODE ||
   ui.rx_addev_no == MIRISDR_DEVICE_CODE ||
   ui.rx_addev_no == SDRPLAY2_DEVICE_CODE ||
   ui.rx_addev_no == SDRPLAY3_DEVICE_CODE ||
   ui.rx_addev_no == BLADERF_DEVICE_CODE ||
   ui.rx_addev_no == PCIE9842_DEVICE_CODE ||
   ui.rx_addev_no == OPENHPSDR_DEVICE_CODE ||
   ui.rx_addev_no == FDMS1_DEVICE_CODE ||
   ui.rx_addev_no == NETAFEDRI_DEVICE_CODE)
  {
  fg.passband_direction=1;
  sdr_nco_counter++;
  goto setfq_x;
  }
if(ui.rx_addev_no == AIRSPY_DEVICE_CODE)
  {
  fg.passband_direction=1;
  if(airspy.real_mode==1 && FFT1_CURMODE == 2)fg.passband_direction=-1;
  sdr_nco_counter++;
  goto setfq_x;
  }
if(ui.rx_addev_no == AIRSPYHF_DEVICE_CODE)
  {
  fg.passband_direction=1;
  sdr_nco_counter++;
  goto setfq_x;
  }
fg.passband_direction=1;
switch (ui.rx_soundcard_radio)
  {
  case RX_SOUNDCARD_RADIO_WSE:
  wse_rx_freq_control();
  break;

  case RX_SOUNDCARD_RADIO_UNDEFINED:
  break;
  
  case RX_SOUNDCARD_RADIO_UNDEFINED_REVERSED:
  fg.passband_direction=-1;
  break;

  case RX_SOUNDCARD_RADIO_SI570:
  fg.passband_direction=si570.rx_passband_direction;
  si570_rx_freq_control();
  break;

  case RX_SOUNDCARD_RADIO_SOFT66:
  soft66_rx_freq_control();
  break;

  case RX_SOUNDCARD_RADIO_ELEKTOR:
  elektor_rx_freq_control();
  break;

  case RX_SOUNDCARD_RADIO_FCDPROPLUS:
  fcdproplus_rx_freq_control();
  break;

  case RX_SOUNDCARD_RADIO_AFEDRI_USB:
  afedriusb_rx_freq_control();
  break;
  }
setfq_x:;
if( (ui.converter_mode & CONVERTER_USE) != 0)
  {
  fg.passband_direction*=ui_converter_direction;  
  }
fft1_direction=fg.passband_direction;
check_filtercorr_direction();
}
