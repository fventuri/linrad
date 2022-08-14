// usb2lpt.c   V3.48.0
// THESE FUNCTIONS PERFORM THE IO TO THE USB2LPT 1.6 DEVICE
// based on V-USB driver by OBJECTIVE DEVELOPMENT Software GmbH
// 	see: http://www.obdev.at/products/vusb/index.html
// 	and  http://vusb.wikidot.com/
// and  the firmware usb2lpt6.c for USB2LPT 1.6 by Henrik Haftmann 
// 	see  http://www-user.tu-chemnitz.de/~heha/bastelecke/Rund%20um%20den%20PC/USB2LPT/index.html.en
// Pierre Vanhoucke / ON5GN

//UPDATE HISTORY
//       10 aug  2010  creation date   
//       4  feb  2013  Ported from libusb-0.1 to libusb-1.0

#include "osnum.h"
#include "loadusb.h"
#include "options.h"

#include <sys/types.h>
#include <string.h>  
#include "uidef.h"
#include "screendef.h"
#include "usbdefs.h"
#include "sdrdef.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

#define USBDEV_SHARED_VENDOR_ID   0x16C0    // VOTI    VID
#define USBDEV_SHARED_PRODUCT_ID   0x06B3   // USB2LPT PID 
#define USB_ERROR_NOTFOUND  1
#define USB_ERROR_SYSTEM    2
#define USB_ERROR_ACCESS    2
#define USB_ERROR_IO        3
#define MAX_USB_ERR_CNT     6
#define DDR_OUT 0x00FF
        
usb_dev_handle *usb2lpt_handle0;
libusb_device_handle *usb2lpt_handle1;

void out_firmware1 (unsigned char port, unsigned char data) 
 { 
	int nBytes;
	unsigned char in_byte[1];
	int err_cnt; 
	err_cnt =0;   
out_again:; 
	nBytes = libusb_control_transfer
	  (
	  usb2lpt_handle1, 
	  LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN, // request-type
	  0x92,             //  request_id parameter
	  port + data*256,  //  value parameter: first byte = portnumber, second byte = data
	  0,                //  index parameter 
	  (unsigned char *)in_byte,  //  pointer to  buffer
	  sizeof(in_byte), 
	  5000);            //  timeoutInMilliseconds

	if (nBytes != 0)
	  {
	  err_cnt ++;
	  if(err_cnt < MAX_USB_ERR_CNT)
	    {
 	   lir_sleep(1000); // settling time
 	   goto out_again; 
 	   }
 	 else
	    {
	    printf("usb2lpt.c: Error during firmware setup \n"); 
	    }
	  }
 }  

int usbOpenDevice1(void)
  {
        libusb_context *ctx = NULL; 	        
        libusb_device **devs;
        libusb_device *dev;
        libusb_device_handle *dev_handle =NULL;
        int     errorCode = USB_ERROR_NOTFOUND;
        unsigned char    buffer[256];
        int r,i;
        int usb_open_flag;
        ssize_t cnt;
        r = libusb_init(&ctx);
	        if (r < 0)    
                                {
	                        printf("failed to init libusb rc=%i\n",r);
                                errorCode= USB_ERROR_SYSTEM;
	                        return errorCode;
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
   	                struct libusb_device_descriptor desc;
	                r = libusb_get_device_descriptor(dev, &desc);
	                if (r < 0) 
                                {
	                        printf("failed to get device descriptor rc=%i\n",r);
                                errorCode= USB_ERROR_SYSTEM;
	                        return errorCode;
	                        }

                       if (desc.idVendor  ==  USBDEV_SHARED_VENDOR_ID &&
                           desc.idProduct ==  USBDEV_SHARED_PRODUCT_ID )
                          { 
                          usb_open_flag =libusb_open(dev, &dev_handle);

                          if (usb_open_flag ==0) 
                            {
                            libusb_get_string_descriptor_ascii 
                                  (dev_handle,
                                   desc.iManufacturer,   
                                   buffer,             
                                   sizeof(buffer)); 
                            if(strcmp((const char *)buffer, "haftmann#software" ) == 0) 
                              {
                              libusb_get_string_descriptor_ascii 
                                  (dev_handle,
                                   desc.iProduct,   
                                   buffer,             
                                   sizeof(buffer));
                              if(strcmp((const char *)buffer, "USB2LPT low-speed adapter" ) == 0) 
                                {
                                usb2lpt_handle1 = dev_handle;
                                errorCode = 0;
                                break;
                               }
                             }
                          }
                          else errorCode= USB_ERROR_IO;
                      }
                   }
        return errorCode;
  }

int open_usb2lpt1(void)
 {
	int retval, cnt;
	char s[80];
	if(usb2lpt_flag)return TRUE;
	cnt=0;
	clear_screen();
        retval=load_usb1_library(TRUE);
        if(!retval)
          {
          lirerr(1429);
          return FALSE;
          }
retry:;
	retval=usbOpenDevice1();
	if(retval != 0 )
	  {	
	  sprintf(s,"Retcode:%d Could not find \"%s\" with VID=0x%x PID=0x%x",retval, 
             "USB2LPT low-speed adapter", USBDEV_SHARED_VENDOR_ID, USBDEV_SHARED_PRODUCT_ID);
	  lir_text(2,2,s);
	  if(retval == USB_ERROR_SYSTEM)   lir_text(2,4,"USB-system failed");
	  if(retval == USB_ERROR_NOTFOUND) lir_text(2,4,"Is the USB2LPT device connected ?");
#if OSNUM == OSNUM_WINDOWS
	  if(retval == USB_ERROR_IO)lir_text(2,4,"USB permission denied");
#endif
#if OSNUM == OSNUM_LINUX
	  if(retval == USB_ERROR_IO)lir_text(2,4,"USB permission denied. Use sudo or run as root.");
#endif
	  lir_text(2,6,"Press R to retry. Any other key to skip.");
	  await_processed_keyboard();
	  if(lir_inkey == 'R')
	     {
	     cnt++;
	     sprintf(s,"Retry count %d",cnt);
	     lir_text(2,8,s);
	     goto retry;
	     }
 	  clear_screen();  
	  return FALSE;                 
	  }
	clear_screen();
	lir_refresh_screen();
	out_firmware1(0x0F,0x00);	            	// Normal operation: Pullups on
	out_firmware1(0x0A,0x00);	             	// SPP-Modus
	out_firmware1(0x0D,(unsigned char)~DDR_OUT);     // Statusport      = Input  
	out_firmware1(0x0C,DDR_OUT);     	     	// Dataport        = Output
	out_firmware1(0x0E,DDR_OUT);	     	        // Controlport	   = Output
	return TRUE;
 }

void out_usb2lpt1(unsigned char data, unsigned char port) 
 { 
	int nBytes;
	unsigned char in_byte[1];
	int err_cnt;
	char s[80];
	err_cnt =0;   
out_again:; 
	nBytes = libusb_control_transfer
	   (
	   usb2lpt_handle1, 
	   LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN, // request-type
	   0x92,             //  request_id parameter
	   port + data*256,  //  value parameter:  first byte = portnumber, second byte = data
	   0,                //  index parameter 
	   (unsigned char *)in_byte,  //  pointer to  buffer
	   sizeof(in_byte), 
	   5000);            //  timeoutInMilliseconds
	if (nBytes != 0)
	  {
	  err_cnt ++;
	  if(err_cnt < MAX_USB_ERR_CNT)
	    {
	    lir_sleep(1000); // settling time
	    goto out_again; 
	    }
	  else
	    {
	    printf("usb2lpt.c: Error when writing to port=%i, data=%#4.2x, returncode=%i.\n",port,data,nBytes); 
	    settextcolor(12);
	    sprintf(s,"Error when writing to USB2LPT device: unable to control WSE converters  ");
	    lir_text(15,screen_last_line,s);
	    settextcolor(7);
	    lir_refresh_screen();
	    }
	  }
//printf("                 OUT_USB  port= %i  data=%#4.2x \n",port,data); 
//fflush(stdout); 
	return ;
 } 

unsigned char in_usb2lpt1( unsigned char port)
 {    
	int nBytes;
	unsigned char in_byte[1] ;           
	int err_cnt;
	char s[80];
	err_cnt =0;
in_again:;
	in_byte[0] = 0;
	nBytes     = 0;
	nBytes = libusb_control_transfer
  		(
  		usb2lpt_handle1, 
  		LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN, // request-type
  		0x01,   		// request_id parameter
		port, 		        // value parameter
		0,      		// index parameter 
		(unsigned char *)in_byte,      // pointer to buffer
		sizeof(in_byte), 
		 5000);                // timeoutInMilliseconds
	if (nBytes!= 1)                // error recovery
	  {
	  err_cnt ++;
	  if(err_cnt < MAX_USB_ERR_CNT)
	    {
	    lir_sleep(1000);  	      // settling time
	    goto in_again; 
	    }
	  else
	    {
	    printf("usb2lpt.c: Error when reading from port=%i, data=%#4.2x, returncode=%i.\n",port,in_byte[0],nBytes); 
	    in_byte[0] = 0;
	    settextcolor(12);
	    sprintf(s,"Error when reading from USB2LPT device: unable to control WSE converters   ");
	    lir_text(15,screen_last_line,s);
	    settextcolor(7);
	    lir_refresh_screen();
	    }
	  }
//printf("IN_USB  port= %i  data=%#4.2x \n",port,in_byte[0] );
//fflush(stdout); 
	return in_byte[0] ;
 }

void out_firmware0 (unsigned char port, unsigned char data) 
{ 
int nBytes;
unsigned char   in_byte[1];
int err_cnt; 
err_cnt =0;   
out_again:;    
nBytes = usb_control_msg
  (
  usb2lpt_handle0, 
  USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, // request-type
  0x92,             //  request_id parameter
  port + data*256,  //  value parameter: first byte = portnumber, second byte = data
  0,                //  index parameter 
  (char *)in_byte,  //  pointer to  buffer
  sizeof(in_byte), 
  5000);            //  timeoutInMilliseconds
if (nBytes != 0)
  {
  err_cnt ++;
  if(err_cnt < MAX_USB_ERR_CNT)
    {
    lir_sleep(1000); // settling time
    goto out_again; 
    }
  else
    {
    printf("usb2lpt.c: Error during firmware setup \n"); 
    printf("usb error message: %s\n", usb_strerror());
    }
  }
}  


int usbOpenDevice0(usb_dev_handle **device, int vendor, char *vendorName, 
                                               int product, char *productName)
{
struct usb_bus      *bus;
struct usb_device   *dev;
int                 errorCode = USB_ERROR_NOTFOUND;
static int          didUsbInit = 0;
char    string[256];
int     len;
if(!didUsbInit)
  {
  didUsbInit = 1;
  usb_init();
  }
usb_find_busses();
usb_find_devices();
for(bus=usb_get_busses(); bus; bus=bus->next)
  {
  for(dev=bus->devices; dev; dev=dev->next)
    {
    if(dev->descriptor.idVendor == vendor && 
                                         dev->descriptor.idProduct == product)
      {
// we need to open the device in order to query strings 
      usb2lpt_handle0 = usb_open(dev);       
      if(!usb2lpt_handle0)
        {
        errorCode = USB_ERROR_ACCESS;
        printf("usb2lpt.c: Warning: cannot open USB device. \n");
        printf("usb error message:%s\n", usb_strerror());
        continue;
        }
// name does not matter if strings are NULL
      if(vendorName == NULL && productName == NULL) break;  
// now check whether the names match:
      len = usbGetStringAscii(usb2lpt_handle0, 
              dev->descriptor.iManufacturer, 0x0409, string, sizeof(string));
      if(len < 0)
        {
        errorCode = USB_ERROR_IO;
        printf("usb2lpt.c: Warning: cannot query manufacturer for device.\n");
        printf("usb error message:%s\n", usb_strerror());
        }
      else
        {
        errorCode = USB_ERROR_NOTFOUND;
// printf("seen device from vendor ->%s<-\n", string); 
        if(strcmp(string, vendorName) == 0)
          {
          len = usbGetStringAscii(usb2lpt_handle0, dev->descriptor.iProduct, 
                                                 0x0409, string, sizeof(string));
          if(len < 0)
            {
            errorCode = USB_ERROR_IO;
            printf("usb2lpt.c: Warning: cannot query product for device.\n");
            printf("usb error message:%s\n", usb_strerror());
            }
          else
            {
            errorCode = USB_ERROR_NOTFOUND;
           // printf("seen product ->%s<-\n", string); 
            if(strcmp(string, productName) == 0) break;
            }
          }
        }
      usb_close(usb2lpt_handle0);
      usb2lpt_handle0 = NULL;
      }
    }
  if(usb2lpt_handle0)  break;
  }
if(usb2lpt_handle0 != NULL)
  {
  errorCode = 0;
  *device = usb2lpt_handle0;
  }
return errorCode;
}

int open_usb2lpt0(void)
{
int retval, cnt;
char s[80];
char *usb2lpt_name_string="USB2LPT low-speed adapter";
if(usb2lpt_flag)return TRUE;
cnt=0;
clear_screen();
retval=load_usb0_library(TRUE);
if(!retval)
  {
  lirerr(1429);
  return FALSE;
  }
retry:;
retval=(usbOpenDevice0(&usb2lpt_handle0,
                      USBDEV_SHARED_VENDOR_ID,
                      "haftmann#software",
                      USBDEV_SHARED_PRODUCT_ID,
                      usb2lpt_name_string) );
if(retval != 0 )
  {
  sprintf(s,"%d Could not find \"%s\" with vid=0x%x pid=0x%x",retval, 
             usb2lpt_name_string, USBDEV_SHARED_VENDOR_ID, USBDEV_SHARED_PRODUCT_ID);
  lir_text(2,2,s);
  if(retval == USB_ERROR_NOTFOUND)
                           lir_text(2,4,"Is the USB2LPT device connected?");
#if OSNUM == OSNUM_WINDOWS
  if(retval == USB_ERROR_IO)lir_text(2,4,"USB permission denied");
#endif
#if OSNUM == OSNUM_LINUX
  if(retval == USB_ERROR_IO)lir_text(2,4,
                        "USB permission denied. Use sudo or run as root.");
#endif
  lir_text(2,6,"Press R to retry. Any other key to skip.");
  await_processed_keyboard();
  if(lir_inkey == 'R')
    {
    cnt++;
    sprintf(s,"Retry count %d",cnt);
    lir_text(2,8,s);
    goto retry;
    }
  clear_screen();  
  return FALSE;                 
  }
clear_screen();
lir_refresh_screen();
out_firmware0(0x0F,0x00);	            	// Normal operation: Pullups on
out_firmware0(0x0A,0x00);	             	// SPP-Modus
out_firmware0(0x0D,(unsigned char)~DDR_OUT);     // Statusport      = Input  
out_firmware0(0x0C,DDR_OUT);     	     	// Dataport        = Output
out_firmware0(0x0E,DDR_OUT);	     	        // Controlport	   = Output
return TRUE;
}

void out_usb2lpt0 (unsigned char data, unsigned char port) 
{ 
int nBytes;
unsigned char   in_byte[1];
int err_cnt;
char s[80];
err_cnt =0;   
out_again:;       
nBytes = usb_control_msg
  (
  usb2lpt_handle0, 
  USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, // request-type
  0x92,             //  request_id parameter
  port + data*256,  //  value parameter:  first byte = portnumber, second byte = data
  0,                //  index parameter 
  (char *)in_byte,  //  pointer to  buffer
  sizeof(in_byte), 
  5000);            //  timeoutInMilliseconds
if (nBytes != 0)
  {
  err_cnt ++;
  if(err_cnt < MAX_USB_ERR_CNT)
    {
    lir_sleep(1000); // settling time
    goto out_again; 
    }
  else
    {
    printf("usb2lpt.c: Error when writing to port=%i, data=%#4.2x, returncode=%i.\n",port,data,nBytes); 
    printf("usb error message: %s\n", usb_strerror());
    settextcolor(12);
    sprintf(s,"Error when writing to USB2LPT device: unable to control WSE converters  ");
    lir_text(15,screen_last_line,s);
    settextcolor(7);
    lir_refresh_screen();
    }
  }
//printf("                 OUT_USB  port= %i  data=%#4.2x \n",port,data); 
//fflush(stdout); 
return ;
} 

unsigned char in_usb2lpt0( unsigned char port)
{    
int nBytes;
unsigned char   in_byte[1] ;           
int err_cnt;
char s[80];
err_cnt =0;
in_again:;
in_byte[0] = 0;
nBytes     = 0;
nBytes = usb_control_msg
  (
  usb2lpt_handle0, 
  USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, // request-type
  0x01,   		// request_id parameter
  port, 		// value parameter
  0,      		// index parameter 
  (char *)in_byte,      // pointer to buffer
  sizeof(in_byte), 
  5000);                // timeoutInMilliseconds
if (nBytes!= 1)         // error recovery
  {
  err_cnt ++;
  if(err_cnt < MAX_USB_ERR_CNT)
    {
    lir_sleep(1000);  	// settling time
    goto in_again; 
    }
  else
    {
    printf("usb2lpt.c: Error when reading from port=%i, data=%#4.2x, returncode=%i.\n",port,in_byte[0],nBytes); 
    printf("usb error message: %s\n", usb_strerror());
    in_byte[0] = 0;
    settextcolor(12);
    sprintf(s,"Error when reading from USB2LPT device: unable to control WSE converters   ");
    lir_text(15,screen_last_line,s);
    settextcolor(7);
    lir_refresh_screen();
    }
  }
//printf("IN_USB  port= %i  data=%#4.2x \n",port,in_byte[0] );
//fflush(stdout); 
return in_byte[0] ;
}

int open_USB2LPT(void)
{
if(wse.libusb_version == 1)return open_usb2lpt1();
if(wse.libusb_version == 0)return open_usb2lpt0();
return FALSE;
}

void out_USB2LPT(unsigned char data, unsigned char port)
{
if(wse.libusb_version == 1)out_usb2lpt1(data, port);
if(wse.libusb_version == 0)out_usb2lpt0(data, port);
}

unsigned char in_USB2LPT( unsigned char port)
{
if(wse.libusb_version == 1)return in_usb2lpt1(port);
if(wse.libusb_version == 0)return in_usb2lpt0(port);
return 0;
}
