
//  si570.c   V3.48.0
//  Linrad's 'host control' routines for Si570 USB devices
//  They are designed to work with: 
//   -the 'QRP2000 USB-Controlled Synthesizer' kit from SDR-kits.net
//   -the 'Lite + USB Xtall V9.0' kit or the 'RX Ensemble Receiver' kit
//    from Tony Parks (see http://www.kb9yig.com )
//   -the Mobo v4.3.4, Mobo v3.6 and the UBW 
//
//   -the FiFi-SDR
//   
//  This code is based on usbsoftrock.c by Andrew Nilsson (andrew.nilsson@gmail.com),
//  powerSwitch.c by Christian Starkjohann,
//  and usbtemp.c by Mathias Dalheimer of Objective Development Software GmbH (2005)
//  (see http://www.obdev.at/products/vusb/index.html )
//
//  Thanks to Leif Asbrink/SM5BSZ for the support and to Sid Boyce/G3VBV for the testing.
// 
//  Use at your own risk and enjoy
//  Pierre Vanhoucke / ON5GN
//
//  GENERAL LOGIC       The frequency of the Si570 oscillator is set by the function si570_rx_freq_control(void)  
//                      using the formula:
//                      Si570_frequency = (fg.passband_center-si570.LO_freq_offset/1000)*float_LO_multiplier
//
//                      The other functions take care of the discovery of the Si570 device, its opening and closing,
//                      the saving and reading of the parameter-file and the settings of the Si570 parameters. 
//
//  UPDATE HISTORY
//
//  17 July   2010      Creation date 
//  22 August 2010	In function si570_rx_freq_control: 
//                      -Avoid overhead: do not open and close the si570 each time a new passband-center-freq is selected
//                       but use si570_open_switch.
//                      -Check returncode of setFreqByValue. If error, ask for a retry or a restart of Linrad.
//  16 Sept   2010      In function setFreqByValue: Error recovery added
//  16 March  2011      The  LO_multiplier  has been changed from integer to float (request from DL3IAE ) 
//                      The variables Si570.LO_multiplier and float_LO_multiplier contain now the 
//                      LO_multiplier * 1000000  .
//
//  25 Sept   2011      Support for FiFi-SDR added and bug corrections
//
//  30 Dec    2011      Removed blank lines from setup menu.
//
//  10 May    2012      The limits for the LO multiplier is changed. Now 0.1 to 64 
//  14 May    2012      The variable is changed from long to int to make the parameter file save it properly
//                      Si570.LO_multiplier now contains LO_multiplier*1000 and float_LO_multiplier contains 
//                      the LO_multiplier itself.
//                      si570.LO_freq_offset is now multiplied with LO_multiplier
//
//  04 Feb   2013       Converted to libusb-1.0
//  10 Nov   2014       Support for SDR-Widget


#include "osnum.h"
#include "loadusb.h"
#include "options.h"


#include <string.h>  
#include <sys/types.h>
#include <ctype.h>
#include "uidef.h"
#include "screendef.h"
#include "usbdefs.h"
#include "thrdef.h"
#include "txdef.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

#define USB_SUCCESS	        0
#define USB_ERROR_NOTFOUND      1
#define USB_ERROR_ACCESS        2
#define USB_ERROR_IO            3
#define SI570_USB_LIST_EMPY     4
#define USB_ERROR_SYSTEM        5
#define VENDOR_NAME_OBDEV	"www.obdev.at"
#define VENDOR_NAME_LENNESTADT  "www.ov-lennestadt.de"
#define VENDOR_NAME_SDR_WIDGET  "SDR-Widget"

#define PRODUCT_NAME_SDR_WIDGET "Yoyodyne SDR-Widget"
#define PRODUCT_NAME_DG8SAQ     "DG8SAQ-I2C"
#define USBDEV_SHARED_VENDOR    0x16C0  // VOTI  VID
#define USBDEV_SHARED_PRODUCT   0x05DC  // OBDEV PID 
                                        
#define REQUEST_READ_VERSION	  0x00
#define REQUEST_SET_FREQ_BY_VALUE 0x32
#define REQUEST_SET_PTT           0x50
#define MAX_USB_ERR_CNT           6

char *si570_parfile_name="par_si570";

#define FIRMWARE_DG8SAQ 1
#define FIRMWARE_FIFI_SDR 2
#define FIRMWARE_SDR_WIDGET 3 
#define MAX_FIRMWARE_TYPE 3

char *rx_serial_id; 
float float_LO_multiplier;

char        serialNumberString[20];
int         i2cAddress = 0x55;
char        work_string[80];
static int  didUsbInit = 0;
usb_dev_handle  *rx_global_si570usb0_handle = NULL;
libusb_device_handle  *rx_global_si570usb1_handle = NULL;
int si570_vernr;

void set_default_parameters_DG8SAQ(void)
{
// set frequency defaults for RX Ensemble Receiver kit
// **********************************************************************
//  From www.wb5rvz.com:
//  The RX Ensemble Receiver kit provides coverage of ham bands from 160-10m, 
//  in four different optional "super bands" (each with underlap 
//  and overlap within the parameters of the associated bandpass filter):
//  1. 160m - Continuous coverage from 1.8 to 2.0 MHz
//  2. 80m and 40m - Continuous coverage from 3.5 to 7.3 MHz
//  3. 30m, 20m, and 17m - Continuous coverage from 10.1 to 18.168 MHz
//  4. 15m, 12m, and 10m - Continuous coverage from 21.0 to 29.7 MHz
//
// **********************************************************************
si570.rx_usb_index=0;       // 0=DYNAMIC SELECTION OF SI570 USB DEVICE
si570.rx_lo_multiplier=4000;
si570.min_rx_freq=1800;
si570.max_rx_freq=29700;
si570.rx_lo_freq_offset=0;
si570.rx_passband_direction=1;
si570.libusb_version=-1;
}

void set_default_parameters_SDR_WIDGET(void)
{
si570.rx_usb_index=0;      // 0=DYNAMIC SELECTION OF SI570 USB DEVICE
si570.rx_lo_multiplier=4000;
si570.min_rx_freq=1800;
si570.max_rx_freq=29700;
si570.rx_lo_freq_offset=0;
si570.rx_passband_direction=1;
si570.libusb_version=-1;
}

void set_default_parameters_FiFiSDR(void)
{
si570.rx_usb_index=0;  // 0=DYNAMIC SELECTION OF SI570 USB DEVICE
si570.rx_lo_multiplier=4000;
si570.min_rx_freq=1800;
si570.max_rx_freq=29700;
si570.rx_lo_freq_offset=0;
si570.rx_passband_direction=1;
si570.libusb_version=-1;
}

void read_par_si570_file(void)
{
int errcod;
errcod=read_sdrpar(si570_parfile_name, MAX_SI570_PARM, 
                                     si570_parm_text, (int*)((void*)&si570));
if(errcod != 0)
  {
  si570.rx_firmware_type=FIRMWARE_DG8SAQ;
  set_default_parameters_DG8SAQ();
  goto readx;
  }
if(si570.rx_usb_index==0)
  {
  rx_serial_id=NULL;
  }
else
  {
  rx_serial_id=(char*)&si570.rx_serial1;
  }
readx:;
float_LO_multiplier=(float)si570.rx_lo_multiplier/1000;
}

void save_par_si570_file(void)
{
FILE *si570_file;
int *sdr_pi,i;
si570_file=fopen(si570_parfile_name,"w");
sdr_pi=(int*)(&si570);
for(i=0; i<MAX_SI570_PARM; i++)
  {
  fprintf(si570_file,"%s [%d]\n",si570_parm_text[i],sdr_pi[i]);
  }
parfile_end(si570_file);
}

void init_si570_freq_parms(void)
{
read_par_si570_file();
fg.passband_direction=si570.rx_passband_direction; 
tg.passband_direction=si570.tx_passband_direction; 
if (fg.passband_center < si570.min_rx_freq*.001 )
  {
  fg.passband_center = si570.min_rx_freq*.001 ;
  }
if (fg.passband_center > si570.max_rx_freq*.001 )
  {
  fg.passband_center = si570.max_rx_freq*.001 ;
  }
}

void setLongWord( int value, char * bytes)
{
  bytes[0] = value & 0xff;
  bytes[1] = ((value & 0xff00) >> 8) & 0xff;
  bytes[2] = ((value & 0xff0000) >> 16) & 0xff;
  bytes[3] = ((value & 0xff000000) >> 24) & 0xff;
} 


unsigned short readVersion1(libusb_device_handle *handle) 
{
// Read  firmware version for DG8SAQ/PE0FKO device
  unsigned short version;
  int nBytes = 0;
  nBytes = libusb_control_transfer
              (handle,
               LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN,
               REQUEST_READ_VERSION,
               0x0E00,
               0,
               (unsigned char *) &version,
               sizeof(version),
               500);


  if (nBytes == 2) 
    {
//  printf("Firmware version     : %d.%d\n", (version & 0xFF00) >> 8, version & 0xFF);
    return version;
    } 
  else
    {
//  printf("Firmware version     : UNKNOWN\n");
    return 0;
    }
}


int readVersion2_1(libusb_device_handle *handle, int *svn) 
{
// Read  firmware version FiFi-SDR
    int error;
    int nummer;
    error = libusb_control_transfer
       (handle,
        LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN,
        0xAB,          // FiFi-SDR Extra-Befehle (lesen) 
        0,             // wValue 
        0,             // wIndex = 0 --> Versions-Nummer 
        (unsigned char *)&nummer,
        4,             // wLength 
        100            // timeout 
        );


    if (error < 0) return error;
    *svn = nummer;
    return 0;
}

  libusb_context *ctx = NULL; 
unsigned char Si570usbOpenDevice1(libusb_device_handle **device, 
                                                           char *usbSerialID) 
{
  libusb_device **devs;
  libusb_device *dev;
  libusb_device_handle *dev_handle =NULL;
  struct libusb_device_descriptor desc;
  int     errorCode = USB_ERROR_NOTFOUND;
  unsigned char buffer[256];
  int r,i;
  int usb_open_flag;
  ssize_t cnt;
  init_si570_freq_parms();
  if(!didUsbInit)
    {
    didUsbInit = 1;
    r = libusb_init(&ctx);
    if (r < 0)    
      {
      printf("failed to init libusb rc=%i\n",r);
      errorCode= USB_ERROR_SYSTEM;
      return errorCode;
      }
    }
  cnt = libusb_get_device_list(ctx, &devs);
  if (cnt < 0)   
    {
    printf("failed to get device list rc=%i\n",(int)cnt);
    errorCode= USB_ERROR_SYSTEM;
    return errorCode;
    }
  i=0;
  while ((dev = devs[i++]) != NULL) 
    {
    r = libusb_get_device_descriptor(dev, &desc);
    if (r < 0) 
      {
      printf("failed to get device descriptor rc=%i\n",r);
      errorCode= USB_ERROR_SYSTEM;
      return errorCode;
      }
    if (desc.idVendor  ==  USBDEV_SHARED_VENDOR  &&   // VOTI  VID
        desc.idProduct ==  USBDEV_SHARED_PRODUCT )    // OBDEV PID
      {
      usb_open_flag =libusb_open(dev, &dev_handle);
//    printf("usb_open_flag retc= %i\n",usb_open_flag);
      if(usb_open_flag == 0) 
        {
        if(si570.rx_firmware_type == FIRMWARE_DG8SAQ)
          { 
//        printf("firmware type 1\n");
          libusb_get_string_descriptor_ascii 
            (dev_handle,
             desc.iManufacturer,   
             buffer,             
             sizeof(buffer)); 
           if(strcmp((const char *)buffer, VENDOR_NAME_OBDEV ) == 0) 
             {
             libusb_get_string_descriptor_ascii 
               (dev_handle,
                desc.iProduct,   
                buffer,             
                sizeof(buffer));
             if(strcmp((const char *)buffer, PRODUCT_NAME_DG8SAQ ) == 0) 
               {
               libusb_get_string_descriptor_ascii 
                 (dev_handle,
                  desc.iSerialNumber,   
                  buffer,             
                  sizeof(buffer));
               if((usbSerialID==NULL) || (strcmp((const char *)buffer, usbSerialID ) == 0))
                 {
                 *device = dev_handle;
                 errorCode = 0;
                 si570_open_switch =1; 
                 break;
                 }
               else 
                 {
                 libusb_close(dev_handle);
                 }                
               }
             else 
               {
               libusb_close(dev_handle);
               } 
             }
           else 
             {
             libusb_close(dev_handle);
             }
          }
        if (si570.rx_firmware_type == FIRMWARE_FIFI_SDR)
          { 
//        printf("firmware type 2\n");
          libusb_get_string_descriptor_ascii 
            (dev_handle,
             desc.iManufacturer,   
             buffer,             
             sizeof(buffer)); 
          if((strcmp((const char *)buffer, VENDOR_NAME_LENNESTADT ) == 0) | 
                                       (strcmp((const char *)buffer, "028" ) == 0))
            {
            libusb_get_string_descriptor_ascii 
              (dev_handle,
               desc.iProduct,   
               buffer,             
               sizeof(buffer));
            if(strcmp((const char *)buffer, "FiFi-SDR" ) == 0) 
              {
              libusb_get_string_descriptor_ascii 
                (dev_handle,
                 desc.iSerialNumber,   
                 buffer,             
                 sizeof(buffer));
              if((usbSerialID==NULL) || (strcmp((const char *)buffer, usbSerialID ) == 0))
                 {
                 *device = dev_handle;
                 errorCode = 0;
                 si570_open_switch =1;
                 break;
                 }
              else 
                 {
                 libusb_close(dev_handle);
                 } 
               }
            else 
              {
              libusb_close(dev_handle);
              }
            }
          else 
            {
            libusb_close(dev_handle);
            }
          }
        if (si570.rx_firmware_type == FIRMWARE_SDR_WIDGET)
          { 
//        printf("firmware type 1\n");
          libusb_get_string_descriptor_ascii 
            (dev_handle,
             desc.iManufacturer,   
             buffer,             
             sizeof(buffer)); 
           if(strcmp((const char *)buffer, VENDOR_NAME_SDR_WIDGET ) == 0) 
             {
             libusb_get_string_descriptor_ascii 
               (dev_handle,
                desc.iProduct,   
                buffer,             
                sizeof(buffer));
             if(strcmp((const char *)buffer, PRODUCT_NAME_SDR_WIDGET ) == 0) 
               {
               libusb_get_string_descriptor_ascii 
                 (dev_handle,
                  desc.iSerialNumber,   
                  buffer,             
                  sizeof(buffer));
               if((usbSerialID==NULL) || (strcmp((const char *)buffer, usbSerialID ) == 0))
                 {
                 *device = dev_handle;
                 errorCode = 0;
                 si570_open_switch =1; 
                 break;
                 }
               else 
                 {
                 libusb_close(dev_handle);
                 }                
               }
             else 
               {
               libusb_close(dev_handle);
               } 
             }
           else 
             {
             libusb_close(dev_handle);
             }
          }
        }
      else errorCode= USB_ERROR_IO;
      }
    } // END WHILE
   libusb_free_device_list(devs,1);
   return errorCode;
}

/*
Command 0x50:
-------------
Set PTT (PB4) I/O line and read CW key level from the PB5 (CW Key_1) and PB1 (CW Key_2).
In case of the enabled ABPF no change of PTT I/O line will be done and no read of the CW key's are
done. The command will return (in case of enabled ABPF) for both CW key's a open status (bits are 1).
The returnd bit value is bit 5 (0x20) for CW key_1 and bit 1 (0x02) for CW key_2, the other bits are zero.

Parameters:
    requesttype:    USB_ENDPOINT_IN
    req~uest:         0x50
    value:           Output BOOL to user output PTT
    index:           0
    bytes:           pointer to 1 byte variable CW Key's
    size:            1
*/


int srSetPTTGetCWInp1(libusb_device_handle * handle)
{
int retval;
uint8_t key;
if (si570.tx_firmware_type != FIRMWARE_DG8SAQ)return FALSE;
retval=libusb_control_transfer
  (
  handle,
  LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN,
  REQUEST_SET_PTT,
  si570_ptt,
  0,
  (unsigned char*)&key,
  sizeof(uint8_t),
  500);
if(retval < 0)
  {
  hand_key=0;
  return FALSE;
  }
if(key == 34)
  {
  hand_key=FALSE;
  }
else
  {    
  hand_key=TRUE;
  }
return TRUE;
}

int setFreqByValue1(libusb_device_handle * handle, double frequency)
{

// WINDOWS DOC  from PE0FKO:
// 
// Command 0x32:
// -------------
// Set the oscillator frequency by value. The frequency is formatted in MHz
// as 11.21 bits value. 
// The "automatic band pass filter selection", "smooth tune", "one side calibration" and
// the "frequency subtract multiply" are all done in this function. (if enabled in the firmware)
// 
// Default:    None
// 
// Parameters:
//     requesttype:    USB_ENDPOINT_OUT
//     request:         0x32
//     value:           0
//     index:           0
//     bytes:           pointer 32 bits integer
//     size:            4
//
// Code sample:
//     uint32_t iFreq;
//     double   dFreq;
// 
//     dFreq = 30.123456; // MHz
//     iFreq = (uint32_t)( dFreq * (1UL << 21) )
//     r = usbCtrlMsgOUT(0x32, 0, 0, (char *)&iFreq, sizeof(iFreq));
//     if (r < 0) Error
//*********************************************************************
//
// LINUX DOC from FiFi-SDR (libusb-1.0)
// 
// VCO schreiben
// bool softrock_write_vco (struct libusb_device_handle *sdr, double freq)
// {
//    int error;
//    uint32_t freq1121;
//    freq1121 = _11_21(4.0 * freq);
//    error = libusb_control_transfer(
//        sdr,
//        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
//        0x32, //* Set current frequency 
//        0, //* wValue 
//        0, //* wIndex 
//        (unsigned char *)&freq1121,
//        4, //* wLength 
//        100 //* timeout 
//        );
//    if (error < 0)
//    {
//        print_usb_error (error);
//        return false;
//    }
//**************************************************************************
	
  char   buffer[4];

  int    value = 0x700 + i2cAddress;
  int    retval;
  int    err_cnt;
  if (si570.rx_firmware_type == FIRMWARE_FIFI_SDR)
    {
    value = 0x00;
    }
  err_cnt =0;   
set_again:;  
  setLongWord(rint(frequency * 2097152.0), buffer);  //   2097152=2^21
  retval=libusb_control_transfer
    (
    handle,
    LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT,
    REQUEST_SET_FREQ_BY_VALUE,
    value,
    0,
    (unsigned char *) buffer,
    sizeof(buffer),
    500);
  if (retval != 4)
    {
    err_cnt ++;
    if(err_cnt < MAX_USB_ERR_CNT)
      {
      lir_sleep(500); // settling time
      goto set_again; 
      }
    else
      {
      printf("si570.c: Error when setting frequency, returncode=%i\n",retval); 
 //     printf("usb error message: %s\n", usb_strerror());
      }
    }
  return retval;
}

unsigned char Si570usbListDevices1(int *line, char *serial_id)
{
//  For SI570usb devices with si570.rx_firmware_type==1, we list all available devices with their SerialID
//  This will allow us to select a specific device afterwards.
//  This function is not invoked for devices with si570.rx_firmware_type==2 
//  because they all have the same SerialID 
libusb_device **devs;
libusb_device *dev;
libusb_device_handle *dev_handle =NULL;
int     errorCode = USB_ERROR_NOTFOUND;
int                 k=0;
char                s[512];
char                s1[120];
int                 id;
unsigned short      version;
char                serialIDS[16][20];
unsigned char    buffer[256];
int r,i;
int usb_open_flag;
ssize_t cnt;
if(!didUsbInit)
  {
  didUsbInit = 1;
  r = libusb_init(&ctx);
  if (r < 0)    
    {
    printf("failed to init libusb rc=%i\n",r);
    errorCode= USB_ERROR_SYSTEM;
    return errorCode;
    }
  }
lir_text(6,line[0],"Select a 'fixed predefined Si570 USB device' from list:");
cnt = libusb_get_device_list(ctx, &devs);
if (cnt < 0)   
  {
  printf("failed to get device list rc=%i\n",(int)cnt);
  errorCode= USB_ERROR_SYSTEM;
  return errorCode;
  }
i=0; 
while ((dev = devs[i++]) != NULL) 
  {
  struct libusb_device_descriptor desc;
  r = libusb_get_device_descriptor(dev, &desc);
  if (r < 0) 
    {
    printf("failed to get device descriptor rc=%i\n",r);
    errorCode= USB_ERROR_SYSTEM;
    return errorCode;
    }
  if (desc.idVendor  ==  USBDEV_SHARED_VENDOR  &&    // VOTI  VID
        desc.idProduct ==  USBDEV_SHARED_PRODUCT )     // OBDEV PID
    {
    usb_open_flag =libusb_open(dev, &dev_handle);
    if (usb_open_flag == 0)
      {
      libusb_get_string_descriptor_ascii(dev_handle,
                             desc.iManufacturer, buffer, sizeof(buffer)); 
      if(strcmp((const char *)buffer, VENDOR_NAME_OBDEV ) == 0) 
        {
        libusb_get_string_descriptor_ascii(dev_handle,
                                    desc.iProduct, buffer, sizeof(buffer));
        if(strcmp((const char *)buffer, PRODUCT_NAME_DG8SAQ ) == 0) 
          {
          libusb_get_string_descriptor_ascii(dev_handle,
                         desc.iSerialNumber, buffer,sizeof(buffer));
//           printf("seen serialNumberString :%s\n", (const char *)buffer);
          version=readVersion1(dev_handle);
          sprintf(s1,"( Firmware version %d.%d )  ", (version & 0xFF00) >> 8, version & 0xFF);
          sprintf(s,"%2d ->  serialID = %s %s", k, (const char *)buffer, s1);
          sprintf(serialIDS[k],"%s", (const char *)buffer);
          lir_text(6,line[0],s);
          line[0]++;
          k++; 
          if (k == 16) // array for serialIDS is full
            {
            libusb_close(dev_handle);
            goto exit;   
            }
          }
        else 
          {
          libusb_close(dev_handle);
          }
        }
      else 
        {
        if(strcmp((const char *)buffer, VENDOR_NAME_SDR_WIDGET ) == 0) 
          {
          libusb_get_string_descriptor_ascii(dev_handle,
                                      desc.iProduct, buffer, sizeof(buffer));
          if(strcmp((const char *)buffer, PRODUCT_NAME_SDR_WIDGET ) == 0) 
            {
            libusb_get_string_descriptor_ascii(dev_handle,
                         desc.iSerialNumber, buffer,sizeof(buffer));
//           printf("seen serialNumberString :%s\n", (const char *)buffer);
            version=readVersion1(dev_handle);
            sprintf(s1,"( Firmware version %d.%d )  ", (version & 0xFF00) >> 8, version & 0xFF);
            sprintf(s,"%2d ->  serialID = %s %s", k, (const char *)buffer, s1);
            sprintf(serialIDS[k],"%s", (const char *)buffer);
            lir_text(6,line[0],s);
            line[0]++;
            k++; 
            if (k == 16) // array for serialIDS is full
              {
              libusb_close(dev_handle);
              goto exit;   
              }
            }
          else 
            {
            libusb_close(dev_handle);
            }
          }
        else 
          {
          libusb_close(dev_handle);
          }
        }
      }
    else
      {
      errorCode=USB_ERROR_IO;
      }
    }
  } // END WHILE
if (k ==0 )
  {
  settextcolor(12);
  lir_text(6,line[0],
  "No Si570 USB devices found                                        ");
  settextcolor(7);
  line[0]++;
  return SI570_USB_LIST_EMPY;
  }
exit: 
line[0]++;
sprintf(s,"Enter line number >");
lir_text(6,line[0],s);
id=lir_get_integer(26,line[0],2, 0, k-1);
errorCode = USB_SUCCESS; 
if(kill_all_flag) return errorCode;
i=0;
while(serialIDS[id][i] !=0 && i<16)
  {
  serial_id[i]=serialIDS[id][i];
  i++;
  }
while(i<16)
  {
  serial_id[i]=0;
  i++;
  }
libusb_free_device_list(devs,1);
return errorCode;
}

unsigned short readVersion0(usb_dev_handle *handle) 
{
// Read  firmware version DG8SAQ/PE0FKO device
  unsigned short version;
  int nBytes = 0;
  nBytes = usb_control_msg(handle,
                           USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                           REQUEST_READ_VERSION,
                           0x0E00,
                           0,
                           (char *) &version,
                           sizeof(version),
                           500);

  if (nBytes == 2) 
    {
//    printf("Firmware version     : %d.%d\n", (version & 0xFF00) >> 8, version & 0xFF);
    return version;
    } 
  else
    {
//    printf("Firmware version     : UNKNOWN\n");
    return 0;
    }
}


int readVersion2_0(usb_dev_handle *handle, int *svn) 
{
// Read  firmware version FiFi-SDR
    int error;
    int nummer;
    error = usb_control_msg(
        handle,
        USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
        0xAB,          // FiFi-SDR Extra-Befehle (lesen) 
        0,             // wValue 
        0,             // wIndex = 0 --> Versions-Nummer 
        (char *)&nummer,
        4,             // wLength 
        100            // timeout 
        );

    if (error < 0) return error;
    *svn = nummer;
    return 0;
}

unsigned char Si570usbOpenDevice0(usb_dev_handle **device, char *usbSerialID)
{
  struct usb_bus      *bus;
  struct usb_device   *dev;
  usb_dev_handle      *handle = NULL;
  unsigned char       errorCode = USB_ERROR_NOTFOUND;
  char                string[256];
  int                 len;
  int  vendor        = USBDEV_SHARED_VENDOR;
  char *vendorName   = VENDOR_NAME_OBDEV;
  int  product       = USBDEV_SHARED_PRODUCT;
  char *productName  = PRODUCT_NAME_DG8SAQ;
  init_si570_freq_parms();
  if (si570.rx_firmware_type == FIRMWARE_FIFI_SDR)
// FiFi-SDR DOC
// Vendor-ID: 16C0 (hexadezimal) / 5824 (dezimal)
// Device-ID: 05DC (hexadezimal) / 1500 (dezimal)
// Hersteller/Vendor Name: "www.ov-lennestadt.de"  or "O28"
// Produkt/Device Name: <leer>
// Si570 I2C-Adresse: beliebig (wird automatisch von der SDR-Firmware erkannt)
//
    {
    vendor       = USBDEV_SHARED_VENDOR;
    product      = USBDEV_SHARED_PRODUCT;
    productName  = " ";
    vendorName   = VENDOR_NAME_LENNESTADT;
    usbSerialID  = NULL;
//printf("Firmware_type  = %i  \n",si570.rx_firmware_type); 
//fflush(stdout); 
    }
  if (si570.rx_firmware_type == FIRMWARE_SDR_WIDGET)
// SDR-Widget
    {
    vendorName   = VENDOR_NAME_SDR_WIDGET;
    productName  = PRODUCT_NAME_SDR_WIDGET;
    }
  if(!didUsbInit)
    {
    didUsbInit = 1;
    usb_init();
    }
  usb_find_busses();
  usb_find_devices();
  for(bus=usb_get_busses(); bus; bus=bus->next){
    for(dev=bus->devices; dev; dev=dev->next){
      if(dev->descriptor.idVendor == vendor && dev->descriptor.idProduct == product)
        {
//printf("vendor   %i product %i found \n",vendor,product); 
//fflush(stdout); 
        handle = usb_open(dev); // we need to open the device in order to query strings 
        if(!handle)
          {

          errorCode = USB_ERROR_ACCESS;
          printf("si570.c: Warning: cannot open Si570-USB device:\n");
          printf("usb error message: %s\n",usb_strerror());
          continue;
          }
//printf("open OK \n"); 
//fflush(stdout); 
        // now check whether the names match
        len = usbGetStringAscii(handle, dev->descriptor.iManufacturer, 0x0409, string, sizeof(string));
        if(len < 0)
          {
          errorCode = USB_ERROR_IO;
          printf("si570.c: Warning: cannot query manufacturer for Si570-USB device:\n");
          printf("usb error message: %s\n",usb_strerror());
          }
        else
          {
          errorCode = USB_ERROR_NOTFOUND;
//printf("seen device from vendor ->%s<-\n", string);
//fflush(stdout); 
          if((strcmp(string, vendorName) == 0) | ((strcmp(string, "O28")== 0) && 
                                                     (si570.rx_firmware_type == FIRMWARE_FIFI_SDR)) )
            {
            if (si570.rx_firmware_type == FIRMWARE_FIFI_SDR)
              { // do not check product-name and serial-number for FiFi-SDR
              errorCode =0;
              break;
              }
            len = usbGetStringAscii(handle, dev->descriptor.iProduct, 0x0409, string, sizeof(string));
            if(len < 0)
              {
              errorCode = USB_ERROR_IO;
              printf("si570.c: Warning: cannot query product for Si570-USB device: \n");
              printf("usb error message: %s\n",usb_strerror());
              }
            else
              {
              errorCode = USB_ERROR_NOTFOUND;
//printf("seen product ->%s<-\n", string); 
//fflush(stdout);
              if(strcmp(string, productName) == 0) 
                {
		len = usbGetStringAscii(handle, dev->descriptor.iSerialNumber, 0x0409, 
                      serialNumberString, sizeof(serialNumberString));
		if (len < 0) 
                  {
		  errorCode = USB_ERROR_IO;
		  printf("si570.c: Warning: cannot query serial number for Si570-USB device: \n");
                  printf("usb error message: %s\n",usb_strerror());
		  }
                else
                  {
//printf("seen serial_number ->%s<-\n", serialNumberString);
//fflush(stdout);
		  errorCode = USB_ERROR_NOTFOUND;
		  if ((usbSerialID == NULL) || (strcmp(serialNumberString, usbSerialID) == 0))
                    {
//                    printf("\nOpen Si570 USB device: OK\n");
//                    printf("usbSerialID          : %s\n",serialNumberString);
		    break;
		    }
		   }
	         }
               }
             }
           }
        usb_close(handle);
        handle = NULL;
      }
    }
    if(handle) break;
  }
  if(handle != NULL)
    {
    errorCode = USB_SUCCESS;
    *device = handle;
    }
  return errorCode;
}

/*
Command 0x50:
-------------
Set PTT (PB4) I/O line and read CW key level from the PB5 (CW Key_1) and PB1 (CW Key_2).
In case of the enabled ABPF no change of PTT I/O line will be done and no read of the CW key's are
done. The command will return (in case of enabled ABPF) for both CW key's a open status (bits are 1).
The returnd bit value is bit 5 (0x20) for CW key_1 and bit 1 (0x02) for CW key_2, the other bits are zero.

Parameters:
    requesttype:    USB_ENDPOINT_IN
    request:         0x50
    value:           Output BOOL to user output PTT
    index:           0
    bytes:           pointer to 1 byte variable CW Key's
    size:            1
*/

int srSetPTTGetCWInp0(usb_dev_handle * handle)
{
int retval;
uint8_t key;
//char s[80];
//sprintf(s,"ptt %d",si570_ptt);
//lir_text(0,25,s);
if (si570.rx_firmware_type != FIRMWARE_DG8SAQ)return FALSE;
retval=usb_control_msg
  (
  handle, 
  USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
  REQUEST_SET_PTT,
  si570_ptt,
  0,
  (char*)&key,
  sizeof(uint8_t),
  500);
if(retval < 0)
  {
  hand_key=0;
  return FALSE;
  }
if(key == 34)
  {
  hand_key=FALSE;
  }
else
  {    
  hand_key=TRUE;
  }
return TRUE;
}

int setFreqByValue0(usb_dev_handle * handle, double frequency)
{

// WINDOWS DOC  from PE0FKO:
// 
// Command 0x32:
// -------------
// Set the oscillator frequency by value. The frequency is formatted in MHz
// as 11.21 bits value. 
// The "automatic band pass filter selection", "smooth tune", "one side calibration" and
// the "frequency subtract multiply" are all done in this function. (if enabled in the firmware)
// 
// Default:    None
// 
// Parameters:
//     requesttype:    USB_ENDPOINT_OUT
//     request:         0x32
//     value:           0
//     index:           0
//     bytes:           pointer 32 bits integer
//     size:            4
//
// Code sample:
//     uint32_t iFreq;
//     double   dFreq;
// 
//     dFreq = 30.123456; // MHz
//     iFreq = (uint32_t)( dFreq * (1UL << 21) )
//     r = usbCtrlMsgOUT(0x32, 0, 0, (char *)&iFreq, sizeof(iFreq));
//     if (r < 0) Error
//*********************************************************************
//
// LINUX DOC from FiFi-SDR (libusb-1.0)
// 
// VCO schreiben
// bool softrock_write_vco (struct libusb_device_handle *sdr, double freq)
// {
//    int error;
//    uint32_t freq1121;
//    freq1121 = _11_21(4.0 * freq);
//    error = libusb_control_transfer(
//        sdr,
//        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
//        0x32, //* Set current frequency 
//        0, //* wValue 
//        0, //* wIndex 
//        (unsigned char *)&freq1121,
//        4, //* wLength 
//        100 //* timeout 
//        );
//    if (error < 0)
//    {
//        print_usb_error (error);
//        return false;
//    }
//**************************************************************************
	
  char   buffer[4];
  int    value;
  int    retval;
  int    err_cnt; 
  if (si570.rx_firmware_type == FIRMWARE_FIFI_SDR)
    {
    value = 0x00;
    }
  else
    {
    value = 0x700 + i2cAddress;;
    }
  err_cnt =0;   
set_again:;  
  setLongWord(rint(frequency * 2097152.0), buffer);  //   2097152=2^21 
  retval=usb_control_msg
    (
    handle, 
    USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
    REQUEST_SET_FREQ_BY_VALUE,
    value,
    0,
    buffer,
    sizeof(buffer),
    500);
  if (retval != 4)
    {
    err_cnt ++;
    if(err_cnt < MAX_USB_ERR_CNT)
      {
      lir_sleep(500); // settling time
      goto set_again; 
      }
    else
      {
      printf("si570.c: Error when setting frequency, returncode=%i\n",retval); 
      printf("usb error message: %s\n", usb_strerror());
      }
    }
  return retval;
}

unsigned char Si570usbListDevices0(int *line, char *serial_id)
{
int i;
(void) line;
struct usb_bus      *bus;
struct usb_device   *dev;
usb_dev_handle      *handle = NULL;
unsigned char       errorCode     = USB_ERROR_NOTFOUND;
int                 vendor        = USBDEV_SHARED_VENDOR;
char                *vendorName   = VENDOR_NAME_OBDEV;
int                 product       = USBDEV_SHARED_PRODUCT;
char                *productName  = PRODUCT_NAME_DG8SAQ;
int                 k=0;
int                 id;
char                s[256];
char                s1[120];
char                string[256];
int                 len;
unsigned short      version;
char                serialIDS[16][20];
if (si570.rx_firmware_type == FIRMWARE_SDR_WIDGET)
// SDR-Widget
  {
  vendorName   = VENDOR_NAME_SDR_WIDGET;
  productName  = PRODUCT_NAME_SDR_WIDGET;
  }
if(!didUsbInit)
  {
  didUsbInit = 1;
  usb_init();
  }
lir_text(6,line[0],"Select a 'fixed predefined Si570 USB device' from list:");
line[0]+=2;
usb_find_busses();
usb_find_devices();
for(bus=usb_get_busses(); bus; bus=bus->next)
  {
  for(dev=bus->devices; dev; dev=dev->next)
    {
    if(dev->descriptor.idVendor == vendor && dev->descriptor.idProduct == product)
      {
      handle = usb_open(dev); // we need to open the device in order to query strings 
      if(!handle)
        {
        errorCode = USB_ERROR_ACCESS;
        printf("si570.c: ListDevices, Warning, cannot open USB device.\n");
        printf("usb error message: %s\n",usb_strerror());
        continue;
        }
// now check whether the names match
      len = usbGetStringAscii(handle, dev->descriptor.iManufacturer, 0x0409, string, sizeof(string));
      if(len < 0)
        {
        errorCode = USB_ERROR_IO;
        printf( "si570.c: ListDevices, Warning, cannot query manufacturer for device.\n");
        printf("usb error message: %s\n",usb_strerror());
        }
      else
        {
        errorCode = USB_ERROR_NOTFOUND;
 //          printf("seen device from vendor ->%s<-\n", string); 
        if(strcmp(string, vendorName) == 0)
          {
          len = usbGetStringAscii(handle, dev->descriptor.iProduct, 0x0409, string, sizeof(string));
          if(len < 0)
            {
            errorCode = USB_ERROR_IO;
            printf("si570.c: ListDevices, Warning, cannot query product for device.\n");
            printf("usb error message: %s\n",usb_strerror());
            }
          else
            {
            errorCode = USB_ERROR_NOTFOUND;
//               printf("seen product ->%s<-\n", string); 
            if(strcmp(string, productName) == 0) 
              {
              len = usbGetStringAscii(handle, dev->descriptor.iSerialNumber, 0x0409, 
              serialNumberString, sizeof(serialNumberString));
              if (len < 0) 
                {
                errorCode = USB_ERROR_IO;
                printf("si570.c: ListDevices, Warning, cannot query serial number for device.\n");
                printf("usb error message: %s\n",usb_strerror());
                }
              else
                {
                errorCode = USB_ERROR_NOTFOUND;
 //                 printf("seen serialNumberString :%s\n", serialNumberString);
                version=readVersion0(handle);
                sprintf(s1,"( Firmware version %d.%d )", (version & 0xFF00) >> 8, version & 0xFF);
                sprintf(s,"%2d ->  serialID = %s %s", k, serialNumberString, s1);
                sprintf(serialIDS[k],"%s", serialNumberString);
                lir_text(6,line[0],s);
                line[0]++;
                k++; 
                if (k == 16) goto exit;  // array for serialIDS is full   
                }
              }
            }
          }
        }
      usb_close(handle);
      handle = NULL;
      }
    }
  if(handle) break;
  }
exit:
if(handle != NULL)
  {
  errorCode = USB_SUCCESS;
  usb_close(handle);
  }
if (k ==0 )
  {
  line[0]=line[0]-2;
  settextcolor(12);
  lir_text(6,line[0],
  "No Si570 USB devices found                                        ");
  settextcolor(7);
  line[0]++;
  return SI570_USB_LIST_EMPY;
  }
line[0]++;
sprintf(s,"Enter line number >");
lir_text(6,line[0],s);
id=lir_get_integer(26,line[0],2, 0, k-1);
if(kill_all_flag) return errorCode;
i=0;
while(serialIDS[id][i] !=0 && i<16)
  {
  serial_id[i]=serialIDS[id][i];
  i++;
  }
while(i<16)
  {
  serial_id[i]=0;
  i++;
  }
return errorCode;
}

//  End Linrad's 'host control' routines for Si570 USB devices

void init_si570_setup(void)
{
read_par_si570_file();
if(si570.libusb_version < 0)
  {
  si570_vernr=-1;
#if OSNUM == OSNUM_LINUX
  si570_vernr=select_libusb_version("SI570", LIBUSB0_LIBNAME, LIBUSB1_LIBNAME);
#endif
#if OSNUM == OSNUM_WINDOWS
  si570_vernr=select_libusb_version("SI570", "libusb0.dll","libusb-1.0.dll");
#endif
  si570.libusb_version=si570_vernr;    
  }
else
  {
  if(si570.libusb_version == 1)load_usb1_library(TRUE);
  if(si570.libusb_version == 0)load_usb0_library(TRUE);
  }
if (si570_open_switch == 1)
  {   
  if(si570.libusb_version == 1)  
    {
    libusb_close(rx_global_si570usb1_handle);
    rx_global_si570usb1_handle=NULL;
    }
  else  
    {
    usb_close(rx_global_si570usb0_handle);
    rx_global_si570usb0_handle=NULL;
    }
  }
save_par_si570_file();
}

void si570_tx_setup(void)
{
int line;
init_si570_setup();
si570_open_switch=0;
clear_screen();
ui.transceiver_mode=-1;  
line=8;
if(ui.rx_soundcard_radio == RX_SOUNDCARD_RADIO_SI570 &&
   si570.rx_firmware_type == FIRMWARE_DG8SAQ)
  {
  lir_text(2,line,"SI570 with DG8SAQ/PEOFKO firmware selected for RX.");
  line++;
  lir_text(3,line,"Do you want to use the same unit for TX? (Y/N)");
  line+=3;
ifsame:;
  await_processed_keyboard();
  if(kill_all_flag)return;
  if(lir_inkey == 'Y')
    {
    if(si570.rx_firmware_type != FIRMWARE_DG8SAQ)
      {
      lir_text(3,line,
      "Transmitter implemented only for DG8SAQ/PE0FKO firmware");
      line++;
      lir_text(3,line,
      "Receiver uses a different firmware.");
      goto err;
      }
    si570.tx_firmware_type=FIRMWARE_DG8SAQ;
    lir_text(3,line,"Two modes are possible for Tx frequency:");
    line+=2;
    lir_text(3,line,"1 = LO fixed (by Rx), Tx audio variable.");
    line++;
    lir_text(3,line,"2 = Tx audio fixed, LO(Si570) different for Rx and Tx.");
    line+=2;
    lir_text(3,line,"Select 2 if using more than 1 W. Use a lowpass filter between");
    line++;
    lir_text(3,line,"soundcard and radio to eliminate noise with mode 2 above 1 W."); 
    line+=2;
    lir_text(15,line,"=>");
    ui.transceiver_mode=lir_get_integer(18,line,1,1,2);
    goto set_dir;
    }
  if(lir_inkey != 'N')goto ifsame;  
  if(rx_serial_id == NULL)
    {
    lir_text(3,line,
      "To use different si570 chips for Rx and Tx you must set the Rx");
    line++;  
    lir_text(3,line,"selection-mode to STATIC");
err:;
    line+=2;
    lir_text(3,line,"Use 'U' in the main menu, then 'D'.");
    line+=2;
    lir_text(3,line,press_any_key);
    await_processed_keyboard();
    return;
    }       
  ui.transceiver_mode=0;  
  }
else
  {
  ui.transceiver_mode=0;  
  }
set_dir:;
lir_text(6,line,"Tx passband direction ( 1 or -1 ) >");
line+=2;
si570.tx_passband_direction=lir_get_integer(42, line, 2, -1, 1);
if(kill_all_flag) return;
if (si570.tx_passband_direction ==0) goto set_dir;
ui.ptt_control=0;
if(ui.transceiver_mode == 1)
  {
  lir_text(6,line,"Use pilot tone to control PTT? (Y/N) =>");
  if(kill_all_flag) return;
pttq:;
  await_processed_keyboard();
  if(lir_inkey == 'Y')
    {
    ui.ptt_control=1;
    }
  else
    {    
    if(lir_inkey != 'N')goto pttq;
    }
  }  
save_par_si570_file();
}

void si570_rx_setup(void)
{
char s[160];
char s1[80];
char* libname;
int  retval;
int  line, select_line;
init_si570_setup();
start_setup:
clear_screen();
settextcolor(14);
lir_text(2,1,"CURRENT SETUP FOR RX 'SI570 USB DEVICE' ");
settextcolor(7);
line= 3;
if (si570.rx_firmware_type == FIRMWARE_DG8SAQ)
  {
  sprintf(s,"SI570 firmware-type               =");
  lir_text(2,line,s);
  settextcolor(14);
  sprintf(s,"DG8SAQ/PEOFKO firmware");
  lir_text(38,line,s);
  settextcolor(7);
  line++;
  }
if (si570.rx_firmware_type == FIRMWARE_FIFI_SDR)
  {
  sprintf(s,"SI570 firmware-type               =");
  lir_text(2,line,s);
  settextcolor(14);
  sprintf(s,"FiFi-SDR firmware");
  lir_text(38,line,s);
  settextcolor(7);
  line++;
  }
if (si570.rx_firmware_type == FIRMWARE_SDR_WIDGET)
  {
  sprintf(s,"SI570 firmware-type               =");
  lir_text(2,line,s);
  settextcolor(14);
  sprintf(s,"SDR-Widget firmware");
  lir_text(38,line,s);
  settextcolor(7);
  line++;
  }
lir_text(2,line,"Run-time library");
libname="Not installed";
#if OSNUM == OSNUM_LINUX
if(si570.libusb_version == 0)
  {
  libname=LIBUSB0_LIBNAME;
  }
if(si570.libusb_version == 1)
  {
  libname=LIBUSB1_LIBNAME;
  }
#endif
#if OSNUM == OSNUM_WINDOWS
if(si570.libusb_version == 0)
  {
  libname="libusb0.dll";
  }
if(si570.libusb_version == 1)
  {
  libname="libusb-1.0.dll";
  }
#endif
sprintf(s,"= %s",libname);
lir_text(36,line,s);
line++;  
if (si570.rx_firmware_type == FIRMWARE_DG8SAQ || 
    si570.rx_firmware_type == FIRMWARE_SDR_WIDGET)
  {
  sprintf(s,"USB serialID / Firmware version   =");
  lir_text(2,line,s);
  lir_refresh_screen();
  if(si570.libusb_version == 1)  
    {
    retval=Si570usbOpenDevice1(&rx_global_si570usb1_handle, rx_serial_id);
    }
  else
    {
    retval=Si570usbOpenDevice0(&rx_global_si570usb0_handle, rx_serial_id);
    }
  if (rx_serial_id != NULL)       // STATIC
    {
    if(retval == 0)
      {
      if(si570.libusb_version == 1)  
        {
        retval=readVersion1(rx_global_si570usb1_handle);
        sprintf(s1,"%d.%d", (retval & 0xFF00) >> 8, retval & 0xFF);
        libusb_close(rx_global_si570usb1_handle);
        rx_global_si570usb1_handle=NULL;
        }
      else
        {
        retval=readVersion0(rx_global_si570usb0_handle);
        sprintf(s1,"%d.%d", (retval & 0xFF00) >> 8, retval & 0xFF);
        usb_close(rx_global_si570usb0_handle);
        rx_global_si570usb0_handle=NULL;
        }
      settextcolor(10);
      sprintf(s,"%s / Firmware version %s",rx_serial_id,s1);
      lir_text(38,line,s);
      settextcolor(7);
      line+=2;
      }
    else
      {
      settextcolor(12);
      sprintf(s,"%s",rx_serial_id);
      lir_text(38,line,s);
      sprintf(s,"could not open device, device probably not connected");
      lir_text(58,line,s);
      settextcolor(7);
      line+=2;
      }
    sprintf(s,"USB device selection mode         = ");
    lir_text(2,line,s);
    settextcolor(10);
    sprintf(s,"STATIC ( => Linrad uses a fixed predefined device )");
    lir_text(38,line,s);
    settextcolor(7);
    line+=2;
    }
  else      //DYNAMIC  (rx_serial_id == NULL) 
    {
    if(retval != 0)
      {
      sprintf(s,"Could not detect a Si570 USB device, probably NO device connected");
      settextcolor(12);
      lir_text(38,line,s);
      settextcolor(7);
      line++;
      }
    else
      {
      if(si570.libusb_version == 1)  
        {
        retval=readVersion1(rx_global_si570usb1_handle);
        libusb_close(rx_global_si570usb1_handle);
        rx_global_si570usb1_handle=NULL;
        }
      else
        {
        retval=readVersion0(rx_global_si570usb0_handle);
        usb_close(rx_global_si570usb0_handle);
        rx_global_si570usb0_handle=NULL;
        }
      sprintf(s1,"%d.%d", (retval & 0xFF00) >> 8, retval & 0xFF);
      sprintf(s,"%s / Firmware version %s",serialNumberString, s1);
      settextcolor(10);
      lir_text(38,line,s);
      settextcolor(7);
      line++;
      }
    sprintf(s,"USB device selection mode         = ");
    lir_text(2,line,s);
    settextcolor(10);
    sprintf(s,"DYNAMIC ( => Linrad uses the first detected device )");
    lir_text(38,line,s);
    settextcolor(7);
    line+=2;
    }
  }
if (si570.rx_firmware_type == FIRMWARE_FIFI_SDR)
  {
  sprintf(s,"Firmware version                  =");
  lir_text(2,line,s);  
  if(si570.libusb_version == 1)  
    {
    retval=Si570usbOpenDevice1(&rx_global_si570usb1_handle, rx_serial_id);
    }
  else
    {
    retval=Si570usbOpenDevice0(&rx_global_si570usb0_handle, rx_serial_id);
    }
  if(retval == 0)
    {
    int svn = 0;
    if(si570.libusb_version == 1)  
      {
      retval=readVersion2_1(rx_global_si570usb1_handle,&svn); 
      libusb_close(rx_global_si570usb1_handle);
      rx_global_si570usb1_handle=NULL;
      }
    else
      {
      retval=readVersion2_0(rx_global_si570usb0_handle,&svn); 
      usb_close(rx_global_si570usb0_handle);
      rx_global_si570usb0_handle=NULL;
      }
    if(retval == 0)
      {
      sprintf (s1,"(SVN) %i", svn );  
      settextcolor(10);
      lir_text(38,line,s1);
      settextcolor(7);
      line+=2;
      } 
    }
  else
    {
    settextcolor(12);
    sprintf(s,"could not open FiFi-SDR device, probably not connected");
    lir_text(38,line,s);
    settextcolor(7);
    line+=2;
    }
  }
sprintf(s,"Si570 min receive frequency (Khz) = ");
lir_text(2,line,s);
settextcolor(10);
sprintf(s,"%i Khz",si570.min_rx_freq);
lir_text(38,line,s);
settextcolor(7);
line++;
sprintf(s,"Si570 max receive frequency (Khz) = ");
lir_text(2,line,s);
settextcolor(10);
sprintf(s,"%i Khz",si570.max_rx_freq);
lir_text(38,line,s);
settextcolor(7);
line++;
sprintf(s,"Si570 Rx passband direction       = ");
lir_text(2,line,s);
settextcolor(10);
sprintf(s,"%i", si570.rx_passband_direction );   
lir_text(38,line,s);
settextcolor(7);
line+=2;
sprintf(s,"Si570 LO frequency multiplier     = ");
lir_text(2,line,s);
settextcolor(10);
sprintf(s,"%6.6f",float_LO_multiplier);
lir_text(38,line,s);
settextcolor(7);
line++;
sprintf(s,"Si570 LO frequency error (ppb)    = ");
lir_text(2,line,s);
settextcolor(10);
sprintf(s,"%d",si570.rx_freq_adjust);
lir_text(38,line,s);
settextcolor(7);
line++;
sprintf(s,"Si570 LO frequency offset (Khz)   = ");
lir_text(2,line,s);
settextcolor(10);
sprintf(s,"%i Khz",si570.rx_lo_freq_offset);
lir_text(38,line,s);

settextcolor(7);
line++;
sprintf(s,"Si570 resulting LO range (Mhz)    = ");
lir_text(2,line,s);
settextcolor(10);

sprintf(s,"from %3.6f to %3.6f Mhz",
               (double)(si570.min_rx_freq-si570.rx_lo_freq_offset)*(float_LO_multiplier/1000), 
               (double)(si570.max_rx_freq-si570.rx_lo_freq_offset)*(float_LO_multiplier/1000));
lir_text(38,line,s);
line++;
settextcolor(7);
// error checking
int si570_freq_error=0;
if ((si570.min_rx_freq-si570.rx_lo_freq_offset)*float_LO_multiplier < 3500 )
  {
  si570_freq_error=1;
  line++;
  settextcolor(12);
  lir_text(38,line,"Invalid 'from' frequency: should be >= 3500 Khz ! ");
  settextcolor(7);
  }
if ((si570.min_rx_freq-si570.rx_lo_freq_offset)*float_LO_multiplier > 945000 )
  {
  si570_freq_error=1;
  line++;
  settextcolor(12);
  lir_text(38,line,"Invalid 'from' frequency:   should be <= 945000 Khz ! ");
  settextcolor(7);
  }
 if ((si570.max_rx_freq-si570.rx_lo_freq_offset)*float_LO_multiplier < 3500 )
 {
 si570_freq_error=1;
 line++;
 settextcolor(12);
 lir_text(38,line,"Invalid 'to' frequency: should be >= 3500 Khz ! ");
 settextcolor(7);
 }
if ((si570.max_rx_freq-si570.rx_lo_freq_offset)*float_LO_multiplier > 945000 )
 {
 si570_freq_error=1;
 line++;
 settextcolor(12);
 lir_text(38,line,"Invalid 'to' frequency: should be <= 945000 Khz ! ");
 settextcolor(7);
 }
if (si570_freq_error==1)
  {
  line++;
  settextcolor(12);
  lir_text(38,line,"CHECK FREQUENCY PARAMETERS");
  settextcolor(7);
  }
if (si570_freq_error==0)
  { 
  if ((si570.max_rx_freq-si570.rx_lo_freq_offset)*float_LO_multiplier > 210000 )
   { // warning
   line++;
   settextcolor(13);
   lir_text(38,line,"Make sure your Si570 chip can handle frequencies > 210000 Khz !");
   settextcolor(7);
   }
  }
// display menu options
  line+=4;
  select_line=line;
process_menu_options:;
  settextcolor(7);
  lir_text(2,line,"SELECT MENU OPTION TO CHANGE SETUP PARAMETERS OR TO EXIT:");
  line+=2;
  settextcolor(7);
  lir_text(2,line,"A = Change SI570 firmware-type (DG8SAQ,FiFi-SDR,SDR-Widget).");
  line++;
  if ( si570.rx_firmware_type == FIRMWARE_DG8SAQ || 
       si570.rx_firmware_type == FIRMWARE_SDR_WIDGET)
    {
    lir_text(2,line,"B = Set selection-mode to DYNAMIC.");
    line++;
    lir_text(2,line,"C = Set selection-mode to STATIC and select a 'predefined Si570 USB device'.");
    line++;
    }
  line++;
  lir_text(2,line,"D = Change MIN RECEIVE frequency.");
  line++;
  lir_text(2,line,"E = Change MAX RECEIVE frequency.");
  line++;
  lir_text(2,line,"F = Change run-time library.");
  line++;
  lir_text(2,line,"H = Change PASSBAND DIRECTION.");
  line++;
  line++;
  lir_text(2,line,"I = Change LO FREQUENCY MULTIPLIER.");
  line++;
  lir_text(2,line,"J = Change LO FREQUENCY OFFSET.");
  line++;
  lir_text(2,line,"K = Change LO FREQUENCY ERROR.");
  line++;
  line++;
  lir_text(2,line,"L = RESTORE default parameters.");
  line++;
  lir_text(2,line,"X = EXIT.");

//process menu options
  await_processed_keyboard();
  if(kill_all_flag)return;
  clear_lines(select_line,line);
  line=select_line;
  switch(lir_inkey)
  {
  case 'A':  //Change SI570 firmware-type
  si570.rx_firmware_type++;
  if(si570.rx_firmware_type >MAX_FIRMWARE_TYPE)si570.rx_firmware_type=FIRMWARE_DG8SAQ;
  save_par_si570_file();
  break;

  case 'B':  //Set selection-mode to DYNAMIC
  if ( si570.rx_firmware_type == FIRMWARE_DG8SAQ || 
       si570.rx_firmware_type == FIRMWARE_SDR_WIDGET)
    {
    rx_serial_id = NULL;
    si570.rx_usb_index=0;
    save_par_si570_file();
    }
  break;

  case 'C':  //Set selection-mode to STATIC
  if ( si570.rx_firmware_type == FIRMWARE_DG8SAQ || 
       si570.rx_firmware_type == FIRMWARE_SDR_WIDGET)
    {
    line= line+4;
    settextcolor(11);
    lir_text(6,line,
    "SETTING USB DEVICE SELECTION MODE TO STATIC:");
    line+=1;
    lir_text(6,line,
    "Looking for available Si570 USB devices:");
    line+=2;
    settextcolor(7);
    if(si570.libusb_version == 1)  
      {
      retval=Si570usbListDevices1(&line,(char*)&si570.rx_serial1);
      }
    else
      {
      retval=Si570usbListDevices0(&line,(char*)&si570.rx_serial1);
      }
    if (retval == SI570_USB_LIST_EMPY )
      {
      line= line+2;
      lir_text(6,line,"Hit any key to go back ");
      await_processed_keyboard();
      goto start_setup;
      } 
   if (retval == USB_ERROR_IO )
      {
      line= line+2;
      settextcolor(12);
      lir_text(6,line,"USB IO-ERROR, Hit any key to go back                            ");
      settextcolor(7);
      await_processed_keyboard();
      goto start_setup;
      } 
    si570.rx_usb_index=1;
    save_par_si570_file();
    }
  break;

  case 'D':  //Change MIN RECEIVE frequency
  line=line+4;
  settextcolor(11);
  lir_text(6,line,"CHANGING MIN RECEIVE FREQUENCY:");
  line+=1;
  lir_text(6,line,
    "Make sure that ((min receive frequency * LO freq multiplier) - LO freq offset) >= 3500 Khz ");
  line+=1;
  lir_text(6,line,
    "3500 Khz being the lower frequency limit for Si570 devices.");
  line+=2;
  settextcolor(7);
  lir_text(6,line,"Enter new value for min receive frequency in Khz >");
  si570.min_rx_freq=lir_get_integer(57, line, 6, 0, 945000);
  if(kill_all_flag) return;
  if (si570.min_rx_freq >= si570.max_rx_freq ) si570.min_rx_freq = si570.max_rx_freq ;
  save_par_si570_file();
  break;

  case 'E':  //Change MAX RECEIVE frequency
  line=line+4;
  settextcolor(11);
  lir_text(6,line,
    "CHANGING MAX RECEIVE FREQUENCY:");
  line+=1;
  lir_text(6,line,
    "Make sure that ((max receive frequency * LO freq multiplier) - LO freq offset) ");
  line+=1;
  lir_text(6,line,
    "is less than the upper frequency limit of your Si570 device.");
  line+=2;
  settextcolor(7);
  lir_text(6,line,"Enter new value for max receive frequency in Khz >");
  si570.max_rx_freq=lir_get_integer(57, line, 6, 0, 945000);
  if(kill_all_flag) return;
  if (si570.max_rx_freq <= si570.min_rx_freq ) si570.max_rx_freq = si570.min_rx_freq ;
  save_par_si570_file();
  break;

  case 'F':  // Change run-time library
  unload_usb1_library();
  unload_usb0_library();
  didUsbInit=0;
#if OSNUM == OSNUM_LINUX
  si570_vernr=select_libusb_version("SI570", LIBUSB0_LIBNAME, LIBUSB1_LIBNAME);
#endif
#if OSNUM == OSNUM_WINDOWS
  si570_vernr=select_libusb_version("SI570", "libusb0.dll","libusb-1.0.dll");
#endif
  if(si570.libusb_version != si570_vernr)
    {
    si570.libusb_version=si570_vernr;
    save_par_si570_file();
    }
  goto start_setup;

  case 'H':  // Change PASSBAND DIRECTION
  line=line+4;
  settextcolor(11);
  lir_text(6,line,
    "CHANGING PASSBAND DIRECTION:");
  line+=1;
  lir_text(6,line,
    "Toggling between 1 and -1  allows to reverse the spectrum displayed on the waterfall graph");
  line+=2;
  settextcolor(7);
again_rx_passband_direction:;
  lir_text(6,line,"Enter new value for passband direction ( 1 or -1 ) >");
  si570.rx_passband_direction=lir_get_integer(60, line, 2, -1, 1);
  if(kill_all_flag) return;
  if (si570.rx_passband_direction ==0) goto again_rx_passband_direction;
  save_par_si570_file();
  break;

  case 'I':
  line=line+4;
  settextcolor(11);
  lir_text(6,line,
    "CHANGING LO MULTIPLIER:");
  line+=1;
  lir_text(6,line,"Use 4 for Softrock");
  line++;
  lir_text(10,line,"2 for UHFSDR and IQ+");
  line ++;
  lir_text(10,line,"1 for LO without frequency divider.");
  line+=2;
  settextcolor(7);
  lir_text(6,line,"Enter new LO multiplier (between 0.1 and  64.0) >");
  float_LO_multiplier=lir_get_float(56, line, 8, 0.1, 64);
  if(kill_all_flag) return;
  si570.rx_lo_multiplier= float_LO_multiplier*1000;
  save_par_si570_file();
  goto start_setup;

  case 'J':
  line=line+4;
  settextcolor(11);
  lir_text(6,line, "CHANGING LO FREQUENCY OFFSET:");
  line+=1;
  lir_text(6,line,
    "Use 0 for receivers with zero IF like Softrock, or plus or minus the IF-value for non-zero-IF receivers ");
  line+=2;
  settextcolor(7);
  lir_text(6,line,"Enter new value for LO frequency offset in Khz >");
  si570.rx_lo_freq_offset=lir_get_integer(55, line, 7, -945000, 945000);
  if(kill_all_flag) return;
  save_par_si570_file();
  break;

  case 'K':  //Set frequency error in ppm.
  line=line+4;
  settextcolor(11);
  lir_text(6,line,
    "CHANGING crystal frequency error in parts per billion:");
  line+=2;
  settextcolor(7);
  lir_text(6,line,"Enter new value for error in ppb >");
  si570.rx_freq_adjust=lir_get_integer(41, line, 8, -50000000, 5000000);
  if(kill_all_flag) return;
  save_par_si570_file();
  break;
  
  case 'L':  //restore default parameters
  if(si570.rx_firmware_type == FIRMWARE_DG8SAQ)set_default_parameters_DG8SAQ();
  if(si570.rx_firmware_type == FIRMWARE_FIFI_SDR)set_default_parameters_FiFiSDR();
  if(si570.rx_firmware_type == FIRMWARE_SDR_WIDGET)set_default_parameters_SDR_WIDGET();
  save_par_si570_file();
  break;

  case 'X':
  si570_open_switch=0;
  return;

  default:
  goto process_menu_options;
  }
goto start_setup;
}

void ensure_usb_open(void)
{
int  retval;
double dt1;
char s[125];
read_par_si570_file();
if(si570.libusb_version == 1)  
  {
  retval=load_usb1_library(TRUE);
  if(!retval)
    {
    lirerr(1429);
    return;
    }
  retval=Si570usbOpenDevice1(&rx_global_si570usb1_handle, rx_serial_id);
  }
else
  {
  retval=load_usb0_library(TRUE);
  if(!retval)
    {
    lirerr(1429);
    return;
    }
  retval=Si570usbOpenDevice0(&rx_global_si570usb0_handle, rx_serial_id);
  }
dt1=current_time();
while(retval !=0)
  {
  clear_lines ( screen_last_line, screen_last_line );
  settextcolor(12);
  if (rx_serial_id == NULL)
    {
    sprintf(s,"No Si570 USB device found, please connect a device: Waiting %.2f                 ",
                                                             current_time()-dt1);
    }
  else
    {
    sprintf(s,
      "No Si570 USB device with serialID = %s found, please connect this device: Waiting %.2f",
                                            rx_serial_id, current_time()-dt1);
    }
  lir_text(0,screen_last_line,s);
  settextcolor(7);
  lir_refresh_screen();
  lir_sleep(50000);
  if(si570.libusb_version == 1)  
    {
    retval=Si570usbOpenDevice1(&rx_global_si570usb1_handle, rx_serial_id);
    }
  else
    {
    retval=Si570usbOpenDevice0(&rx_global_si570usb0_handle, rx_serial_id);
    }
  if(kill_all_flag)return; 
  if(retval == 0)
    {
    clear_lines ( screen_last_line, screen_last_line );
    lir_refresh_screen();
    }
  }
si570_open_switch=1;
}  

void si570_get_ptt(void)
{
if (si570_open_switch == 0)ensure_usb_open(); 
if(kill_all_flag)return;
lir_mutex_lock(MUTEX_LIBUSB);
if(si570.libusb_version == 1)  
  {
  srSetPTTGetCWInp1(rx_global_si570usb1_handle);
  }
else
  {  
  srSetPTTGetCWInp0(rx_global_si570usb0_handle);
  }
lir_mutex_unlock(MUTEX_LIBUSB);
}

void si570_set_ptt(void)
{
double dt2;
dt2=0;
if (si570_open_switch == 0)ensure_usb_open(); 
if(kill_all_flag)return;
lir_mutex_lock(MUTEX_LIBUSB);
if(si570.libusb_version == 1)  
  {
  if(ui.transceiver_mode == 2)
    {
    if(si570_ptt)
      {
      dt2=tx_passband_center;
      }
    else
      {
      dt2=fg.passband_center*((double)1-0.000000001*si570.rx_freq_adjust);
      }
    dt2=(dt2-(double)si570.rx_lo_freq_offset/1000)*float_LO_multiplier;
    if(si570_ptt)setFreqByValue1(rx_global_si570usb1_handle,dt2);
    }   
  srSetPTTGetCWInp1(rx_global_si570usb1_handle);
  if(ui.transceiver_mode == 2)
    {
    if(!si570_ptt)setFreqByValue1(rx_global_si570usb1_handle,dt2);
    }
  }  
else
  {  
  if(ui.transceiver_mode == 2)
    {
    if(si570_ptt)
      {
      dt2=tx_passband_center;
      }
    else
      {
      dt2=fg.passband_center*((double)1-0.000000001*si570.rx_freq_adjust);
      }
    dt2=(dt2-(double)si570.rx_lo_freq_offset/1000)*float_LO_multiplier;
    if(si570_ptt)setFreqByValue0(rx_global_si570usb0_handle,dt2);
    }   
  srSetPTTGetCWInp0(rx_global_si570usb0_handle);
  if(ui.transceiver_mode == 2)
    {
    if(!si570_ptt)setFreqByValue0(rx_global_si570usb0_handle,dt2);
    }
  }
lir_mutex_unlock(MUTEX_LIBUSB);
}

void si570_rx_freq_control(void)
{
int  retval;
double dt2;
char s[125];
start_retry:;
si570_set_ptt();
if (si570_open_switch == 0)ensure_usb_open(); 
if(kill_all_flag)return;
dt2=fg.passband_center*((double)1-0.000000001*si570.rx_freq_adjust);
dt2=(dt2-(double)si570.rx_lo_freq_offset/1000)*float_LO_multiplier;
lir_mutex_lock(MUTEX_LIBUSB);
if(si570.libusb_version == 1)  
  {
  retval=setFreqByValue1(rx_global_si570usb1_handle,dt2);
  }
else
  {
  retval=setFreqByValue0(rx_global_si570usb0_handle,dt2);
  }
lir_mutex_unlock(MUTEX_LIBUSB);
if (retval != 4 ) 
  {
  clear_lines (screen_last_line, screen_last_line );
  settextcolor(12);
  sprintf(s,
        "Si570 USB ERROR, UNABLE TO SET NEW PASSBAND-CENTER-FREQUENCY: HIT ENTER TWICE TO RETRY OR ESCAPE KEY TO EXIT ");
  lir_text(15,screen_last_line,s);
  settextcolor(7);
  lir_refresh_screen();
  await_processed_keyboard();
  if(kill_all_flag)return;
  close_si570();
  goto start_retry;
  }
}

void close_si570(void)
{
if(si570.libusb_version == 1)  
  {
  libusb_close(rx_global_si570usb1_handle);
  rx_global_si570usb1_handle=NULL;
  }
else
  {  
  usb_close(rx_global_si570usb0_handle);
  rx_global_si570usb0_handle=NULL;
  }
unload_usb1_library();
unload_usb0_library();
didUsbInit=0;
si570_open_switch=0;   //0 = force open next time the si570_rx_freq_control function gets called 
}
