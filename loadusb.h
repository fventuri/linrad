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

// this file contains definitions from libusb-win32 (usb.h),
// libusb-1.0 (libusb.h) and libftdi (ftdi.h)
// The complete packages can be downloaded here:
// http://sm5bsz.com/linuxdsp/install/compile/wincompile_nasmplus.htm

#include "globdef.h"

#if OSNUM == OSNUM_LINUX
#include <limits.h>
#include <sys/time.h>
#define LIBUSB_CALL
#endif

#if OSNUM == OSNUM_WINDOWS
#include <windows.h>
#include <pshpack1.h>
#include <poppack.h>
#ifdef interface
#undef interface
#endif
#define LIBUSB_CALL WINAPI
#endif

struct usb_dev_handle;
typedef struct usb_dev_handle usb_dev_handle;
struct usb_device;
struct usb_bus;
int  usbGetStringAscii(usb_dev_handle *dev, int my_index, 
                                          int langid, char *buf, int buflen);
int select_libusb_version(char* name, char* lib0, char* lib1);

#define USB_TYPE_VENDOR			(0x02 << 5)
#define USB_RECIP_DEVICE		0x00
#define USB_ENDPOINT_IN			0x80
#define USB_TYPE_VENDOR			(0x02 << 5)
#define USB_REQ_GET_DESCRIPTOR		0x06
#define USB_DT_STRING			0x03
#define USB_ENDPOINT_OUT		0x00



/** FTDI chip type */
enum ftdi_chip_type { TYPE_AM=0, TYPE_BM=1, TYPE_2232C=2, TYPE_R=3, TYPE_2232H=4, TYPE_4232H=5 };

/** MPSSE bitbang modes */
enum ftdi_mpsse_mode
{
    BITMODE_RESET  = 0x00,    /**< switch off bitbang mode, back to regular serial/FIFO */
    BITMODE_BITBANG= 0x01,    /**< classical asynchronous bitbang mode, introduced with B-type chips */
    BITMODE_MPSSE  = 0x02,    /**< MPSSE mode, available on 2232x chips */
    BITMODE_SYNCBB = 0x04,    /**< synchronous bitbang mode, available on 2232x and R-type chips  */
    BITMODE_MCU    = 0x08,    /**< MCU Host Bus Emulation mode, available on 2232x chips */
                              /* CPU-style fifo mode gets set via EEPROM */
    BITMODE_OPTO   = 0x10,    /**< Fast Opto-Isolated Serial Interface Mode, available on 2232x chips  */
    BITMODE_CBUS   = 0x20,    /**< Bitbang on CBUS pins of R-type chips, configure in EEPROM before */
    BITMODE_SYNCFF = 0x40,    /**< Single Channel Synchronous FIFO mode, available on 2232H chips */
};

enum ftdi_module_detach_mode
{
    AUTO_DETACH_SIO_MODULE = 0,
    DONT_DETACH_SIO_MODULE = 1
};


/**
    \brief Main context structure for all libftdi functions.

    Do not access directly if possible.
*/
struct ftdi_context
{
    /* USB specific */
    /** libusb's usb_dev_handle */
    struct usb_dev_handle *usb_dev;
    /** usb read timeout */
    int usb_read_timeout;
    /** usb write timeout */
    int usb_write_timeout;

    /* FTDI specific */
    /** FTDI chip type */
    enum ftdi_chip_type type;
    /** baudrate */
    int baudrate;
    /** bitbang mode state */
    unsigned char bitbang_enabled;
    /** pointer to read buffer for ftdi_read_data */
    unsigned char *readbuffer;
    /** read buffer offset */
    unsigned int readbuffer_offset;
    /** number of remaining data in internal read buffer */
    unsigned int readbuffer_remaining;
    /** read buffer chunk size */
    unsigned int readbuffer_chunksize;
    /** write buffer chunk size */
    unsigned int writebuffer_chunksize;
    /** maximum packet size. Needed for filtering modem status bytes every n packets. */
    unsigned int max_packet_size;

    /* FTDI FT2232C requirecments */
    /** FT2232C interface number: 0 or 1 */
    int interface;   /* 0 or 1 */
    /** FT2232C index number: 1 or 2 */
    int index;       /* 1 or 2 */
    /* Endpoints */
    /** FT2232C end points: 1 or 2 */
    int in_ep;
    int out_ep;      /* 1 or 2 */

    /** Bitbang mode. 1: (default) Normal bitbang mode, 2: FT2232C SPI bitbang mode */
    unsigned char bitbang_mode;

    /** EEPROM size. Default is 128 bytes for 232BM and 245BM chips */
    int eeprom_size;

    /** String representation of last error */
    char *error_str;

    /** Buffer needed for async communication */
    char *async_usb_buffer;
    /** Number of URB-structures we can buffer */
    unsigned int async_usb_buffer_size;

    /** Defines behavior in case a kernel module is already attached to the device */
    enum ftdi_module_detach_mode module_detach_mode;
};

struct ftdi_device_list
{
    /** pointer to next entry */
    struct ftdi_device_list *next;
    /** pointer to libusb's usb_device */
    struct usb_device *dev;
};

typedef int (*p_ftdi_write_data)(struct ftdi_context *ftdi, 
                                       unsigned char *buf, int size);
typedef int (*p_ftdi_set_baudrate)(struct ftdi_context *ftdi, 
                                                       int baudrate);
typedef int (*p_ftdi_set_bitmode)(struct ftdi_context *ftdi, 
                           unsigned char bitmask, unsigned char mode);
typedef int (*p_ftdi_read_data)(struct ftdi_context *ftdi, 
                                        unsigned char *buf, int size);
typedef void (*p_ftdi_free)(struct ftdi_context *ftdi);
typedef int (*p_ftdi_usb_find_all)(struct ftdi_context *ftdi, 
        struct ftdi_device_list **devlist, int vendor, int product);
typedef char* (*p_ftdi_get_error_string)(struct ftdi_context *ftdi);
typedef struct ftdi_context* (*p_ftdi_new)(void);
typedef int (*p_ftdi_usb_open)(struct ftdi_context *ftdi, 
                                           int vendor, int product);
typedef int (*p_ftdi_init)(struct ftdi_context *ftdi);
typedef int (*p_ftdi_write_data_set_chunksize)(
                    struct ftdi_context *ftdi, unsigned int chunksize);
typedef void (*p_ftdi_deinit)(struct ftdi_context *ftdi);
typedef int (*p_ftdi_read_pins)(struct ftdi_context *ftdi, 
                                                  unsigned char *pins);
typedef int (*p_ftdi_usb_close)(struct ftdi_context *ftdi);
typedef int (*p_ftdi_usb_get_strings)(struct ftdi_context *ftdi, 
                  struct usb_device *dev, char * manufacturer, 
                    int mnf_len, char * description, int desc_len, 
                                       char * serial, int serial_len);
typedef int (*p_ftdi_usb_open_dev)(struct ftdi_context *ftdi, 
                                              struct usb_device *dev);
typedef void (*p_ftdi_list_free)(struct ftdi_device_list **devlist);

extern p_ftdi_write_data ftdi_write_data;
extern p_ftdi_set_baudrate ftdi_set_baudrate;
extern p_ftdi_set_bitmode ftdi_set_bitmode;
extern p_ftdi_read_data ftdi_read_data;
extern p_ftdi_free ftdi_free;
extern p_ftdi_usb_find_all ftdi_usb_find_all;
extern p_ftdi_get_error_string ftdi_get_error_string;
extern p_ftdi_new ftdi_new;
extern p_ftdi_usb_open ftdi_usb_open;
extern p_ftdi_init ftdi_init;
extern p_ftdi_write_data_set_chunksize ftdi_write_data_set_chunksize;
extern p_ftdi_deinit ftdi_deinit;
extern p_ftdi_read_pins ftdi_read_pins;
extern p_ftdi_usb_close ftdi_usb_close;
extern p_ftdi_usb_get_strings ftdi_usb_get_strings;
extern p_ftdi_usb_open_dev ftdi_usb_open_dev;
extern p_ftdi_list_free ftdi_list_free;

/** \ingroup misc
 * Recipient bits of the
 * \ref libusb_control_setup::bmRequestType "bmRequestType" field in control
 * transfers. Values 4 through 31 are reserved. */
enum libusb_request_recipient {
	/** Device */
	LIBUSB_RECIPIENT_DEVICE = 0x00,

	/** Interface */
	LIBUSB_RECIPIENT_INTERFACE = 0x01,

	/** Endpoint */
	LIBUSB_RECIPIENT_ENDPOINT = 0x02,

	/** Other */
	LIBUSB_RECIPIENT_OTHER = 0x03,
};

/** \ingroup misc
 * Request type bits of the
 * \ref libusb_control_setup::bmRequestType "bmRequestType" field in control
 * transfers. */
enum libusb_request_type {
	/** Standard */
	LIBUSB_REQUEST_TYPE_STANDARD = (0x00 << 5),

	/** Class */
	LIBUSB_REQUEST_TYPE_CLASS = (0x01 << 5),

	/** Vendor */
	LIBUSB_REQUEST_TYPE_VENDOR = (0x02 << 5),

	/** Reserved */
	LIBUSB_REQUEST_TYPE_RESERVED = (0x03 << 5)
};

/** \ingroup desc
 * Endpoint direction. Values for bit 7 of the
 * \ref libusb_endpoint_descriptor::bEndpointAddress "endpoint address" scheme.
 */
enum libusb_endpoint_direction {
	/** In: device-to-host */
	LIBUSB_ENDPOINT_IN = 0x80,

	/** Out: host-to-device */
	LIBUSB_ENDPOINT_OUT = 0x00
};



/** \ingroup desc
 * A structure representing the standard USB device descriptor. This
 * descriptor is documented in section 9.6.1 of the USB 2.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
struct libusb_device_descriptor {
	/** Size of this descriptor (in bytes) */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_DEVICE LIBUSB_DT_DEVICE in this
	 * context. */
	uint8_t  bDescriptorType;

	/** USB specification release number in binary-coded decimal. A value of
	 * 0x0200 indicates USB 2.0, 0x0110 indicates USB 1.1, etc. */
	uint16_t bcdUSB;

	/** USB-IF class code for the device. See \ref libusb_class_code. */
	uint8_t  bDeviceClass;

	/** USB-IF subclass code for the device, qualified by the bDeviceClass
	 * value */
	uint8_t  bDeviceSubClass;

	/** USB-IF protocol code for the device, qualified by the bDeviceClass and
	 * bDeviceSubClass values */
	uint8_t  bDeviceProtocol;

	/** Maximum packet size for endpoint 0 */
	uint8_t  bMaxPacketSize0;

	/** USB-IF vendor ID */
	uint16_t idVendor;

	/** USB-IF product ID */
	uint16_t idProduct;

	/** Device release number in binary-coded decimal */
	uint16_t bcdDevice;

	/** Index of string descriptor describing manufacturer */
	uint8_t  iManufacturer;

	/** Index of string descriptor describing product */
	uint8_t  iProduct;

	/** Index of string descriptor containing device serial number */
	uint8_t  iSerialNumber;

	/** Number of possible configurations */
	uint8_t  bNumConfigurations;
};


struct libusb_context;
struct libusb_device;
struct libusb_device_handle;

typedef struct libusb_context libusb_context;
typedef struct libusb_device libusb_device;
typedef struct libusb_device_handle libusb_device_handle;

/** \ingroup asyncio
 * Transfer status codes */
enum libusb_transfer_status {
	/** Transfer completed without error. Note that this does not indicate
	 * that the entire amount of requested data was transferred. */
	LIBUSB_TRANSFER_COMPLETED,

	/** Transfer failed */
	LIBUSB_TRANSFER_ERROR,

	/** Transfer timed out */
	LIBUSB_TRANSFER_TIMED_OUT,

	/** Transfer was cancelled */
	LIBUSB_TRANSFER_CANCELLED,

	/** For bulk/interrupt endpoints: halt condition detected (endpoint
	 * stalled). For control endpoints: control request not supported. */
	LIBUSB_TRANSFER_STALL,

	/** Device was disconnected */
	LIBUSB_TRANSFER_NO_DEVICE,

	/** Device sent more data than requested */
	LIBUSB_TRANSFER_OVERFLOW,
};

// Endpoint transfer type. Values for bits 0:1 of the
// \ref libusb_endpoint_descriptor::bmAttributes "endpoint attributes" field.

enum libusb_transfer_type {
	/** Control endpoint */
	LIBUSB_TRANSFER_TYPE_CONTROL = 0,

	/** Isochronous endpoint */
	LIBUSB_TRANSFER_TYPE_ISOCHRONOUS = 1,

	/** Bulk endpoint */
	LIBUSB_TRANSFER_TYPE_BULK = 2,

	/** Interrupt endpoint */
	LIBUSB_TRANSFER_TYPE_INTERRUPT = 3,

	/** Stream endpoint */
	LIBUSB_TRANSFER_TYPE_BULK_STREAM = 4,
};




/** \ingroup asyncio
 * Isochronous packet descriptor. */
struct libusb_iso_packet_descriptor {
	/** Length of data to request in this packet */
	unsigned int length;

	/** Amount of data that was actually transferred */
	unsigned int actual_length;

	/** Status code for this packet */
	enum libusb_transfer_status status;
};

struct libusb_transfer;

typedef void (*libusb_transfer_cb_fn)(struct libusb_transfer *transfer);

/** \ingroup asyncio
 * The generic USB transfer structure. The user populates this structure and
 * then submits it in order to request a transfer. After the transfer has
 * completed, the library populates the transfer with the results and passes
 * it back to the user.
 */
struct libusb_transfer {
	/** Handle of the device that this transfer will be submitted to */
	libusb_device_handle *dev_handle;

	/** A bitwise OR combination of \ref libusb_transfer_flags. */
	uint8_t flags;

	/** Address of the endpoint where this transfer will be sent. */
	unsigned char endpoint;

	/** Type of the endpoint from \ref libusb_transfer_type */
	unsigned char type;

	/** Timeout for this transfer in millseconds. A value of 0 indicates no
	 * timeout. */
	unsigned int timeout;

	/** The status of the transfer. Read-only, and only for use within
	 * transfer callback function.
	 *
	 * If this is an isochronous transfer, this field may read COMPLETED even
	 * if there were errors in the frames. Use the
	 * \ref libusb_iso_packet_descriptor::status "status" field in each packet
	 * to determine if errors occurred. */
	enum libusb_transfer_status status;

	/** Length of the data buffer */
	int length;

	/** Actual length of data that was transferred. Read-only, and only for
	 * use within transfer callback function. Not valid for isochronous
	 * endpoint transfers. */
	int actual_length;

	/** Callback function. This will be invoked when the transfer completes,
	 * fails, or is cancelled. */
	libusb_transfer_cb_fn callback;

	/** User context data to pass to the callback function. */
	void *user_data;

	/** Data buffer */
	unsigned char *buffer;

	/** Number of isochronous packets. Only used for I/O with isochronous
	 * endpoints. */
	int num_iso_packets;

	/** Isochronous packet descriptors, for isochronous transfers only. */
	struct libusb_iso_packet_descriptor iso_packet_desc
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
	[] /* valid C99 code */
#else
	[0] /* non-standard, but usually working code */
#endif
	;
};

static __inline void libusb_fill_bulk_transfer(struct libusb_transfer *transfer,
	libusb_device_handle *dev_handle, unsigned char endpoint,
	unsigned char *buffer, int length, libusb_transfer_cb_fn callback,
	void *user_data, unsigned int timeout)
{
	transfer->dev_handle = dev_handle;
	transfer->endpoint = endpoint;
	transfer->type = LIBUSB_TRANSFER_TYPE_BULK;
	transfer->timeout = timeout;
	transfer->buffer = buffer;
	transfer->length = length;
	transfer->user_data = user_data;
	transfer->callback = callback;
}

typedef int (LIBUSB_CALL *p_libusb_get_string_descriptor_ascii)
        (libusb_device_handle *dev,
                 uint8_t desc_index, unsigned char *data, int length);
typedef void (LIBUSB_CALL *p_libusb_close)(libusb_device_handle *dev_handle);
typedef int (LIBUSB_CALL *p_libusb_control_transfer)(libusb_device_handle *dev_handle,
	uint8_t request_type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
	unsigned char *data, uint16_t wLength, unsigned int timeout);
typedef int (LIBUSB_CALL *p_libusb_open)(libusb_device *dev, libusb_device_handle **handle);
typedef int (LIBUSB_CALL *p_libusb_init)(libusb_context **ctx);
typedef intptr_t (LIBUSB_CALL *p_libusb_get_device_list)(libusb_context *ctx,
	libusb_device ***list);
typedef int (LIBUSB_CALL *p_libusb_get_device_descriptor)(libusb_device *dev,
	struct libusb_device_descriptor *desc);
typedef void (LIBUSB_CALL *p_libusb_free_device_list)(libusb_device **list,
	int unref_devices);
// --
typedef int (LIBUSB_CALL *p_libusb_submit_transfer)(struct libusb_transfer *transfer);
typedef int (LIBUSB_CALL *p_libusb_reset_device)(libusb_device_handle *dev);
typedef void (LIBUSB_CALL *p_libusb_exit)(libusb_context *ctx);
typedef int (LIBUSB_CALL *p_libusb_bulk_transfer)(libusb_device_handle *dev_handle,
	unsigned char endpoint, unsigned char *data, int length,
	int *actual_length, unsigned int timeout);
typedef int (LIBUSB_CALL *p_libusb_cancel_transfer)(struct libusb_transfer *transfer);
typedef int (LIBUSB_CALL *p_libusb_handle_events_timeout)(libusb_context *ctx,
	struct timeval *tv);
typedef int (LIBUSB_CALL *p_libusb_release_interface)(libusb_device_handle *dev,
	int interface_number);
typedef int (LIBUSB_CALL *p_libusb_claim_interface)(libusb_device_handle *dev,
	int interface_number);
typedef void (LIBUSB_CALL *p_libusb_free_transfer)(struct libusb_transfer *transfer);
typedef int (LIBUSB_CALL *p_libusb_set_interface_alt_setting)(libusb_device_handle *dev,
	int interface_number, int alternate_setting);
typedef struct libusb_transfer* (LIBUSB_CALL *p_libusb_alloc_transfer)(int iso_packets);

extern p_libusb_control_transfer libusb_control_transfer;
extern p_libusb_get_string_descriptor_ascii libusb_get_string_descriptor_ascii;
extern p_libusb_close libusb_close;
extern p_libusb_open libusb_open;
extern p_libusb_init libusb_init;
extern p_libusb_get_device_list libusb_get_device_list;
extern p_libusb_get_device_descriptor libusb_get_device_descriptor;
extern p_libusb_free_device_list libusb_free_device_list;
extern p_libusb_submit_transfer libusb_submit_transfer;
extern p_libusb_reset_device libusb_reset_device;
extern p_libusb_exit libusb_exit;
extern p_libusb_bulk_transfer libusb_bulk_transfer;
extern p_libusb_cancel_transfer libusb_cancel_transfer;
extern p_libusb_handle_events_timeout libusb_handle_events_timeout;
extern p_libusb_release_interface libusb_release_interface;
extern p_libusb_claim_interface libusb_claim_interface;
extern p_libusb_free_transfer libusb_free_transfer;
extern p_libusb_set_interface_alt_setting libusb_set_interface_alt_setting;
extern p_libusb_alloc_transfer libusb_alloc_transfer;

#if OSNUM == OSNUM_WINDOWS
#define LIBUSB_PATH_MAX 512
struct usb_device_descriptor
{
unsigned char  bLength;
unsigned char  bDescriptorType;
unsigned short bcdUSB;
unsigned char  bDeviceClass;
unsigned char  bDeviceSubClass;
unsigned char  bDeviceProtocol;
unsigned char  bMaxPacketSize0;
unsigned short idVendor;
unsigned short idProduct;
unsigned short bcdDevice;
unsigned char  iManufacturer;
unsigned char  iProduct;
unsigned char  iSerialNumber;
unsigned char  bNumConfigurations;
};

struct usb_device
{
struct usb_device *next, *prev;
char filename[LIBUSB_PATH_MAX];
struct usb_bus *bus;
struct usb_device_descriptor descriptor;
struct usb_config_descriptor *config;
void *dev;
unsigned char devnum;
unsigned char num_children;
struct usb_device **children;
};

struct usb_bus
{
struct usb_bus *next, *prev;
char dirname[LIBUSB_PATH_MAX];
struct usb_device *devices;
unsigned long long location;
struct usb_device *root_dev;
};
#endif

#if OSNUM == OSNUM_LINUX
struct usb_device_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;
	uint16_t bcdUSB;
	uint8_t  bDeviceClass;
	uint8_t  bDeviceSubClass;
	uint8_t  bDeviceProtocol;
	uint8_t  bMaxPacketSize0;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t  iManufacturer;
	uint8_t  iProduct;
	uint8_t  iSerialNumber;
	uint8_t  bNumConfigurations;
} __attribute__ ((packed));

struct usb_device {
  struct usb_device *next, *prev;

  char filename[PATH_MAX + 1];

  struct usb_bus *bus;

  struct usb_device_descriptor descriptor;
  struct usb_config_descriptor *config;

  void *dev;		/* Darwin support */

  uint8_t devnum;

  unsigned char num_children;
  struct usb_device **children;
};


struct usb_bus {
  struct usb_bus *next, *prev;
  char dirname[PATH_MAX + 1];
  struct usb_device *devices;
  uint32_t location;
  struct usb_device *root_dev;
};
#endif

typedef char* (*p_usb_strerror)(void);
typedef void (*p_usb_init)(void);
typedef int (*p_usb_find_busses)(void);
typedef int (*p_usb_find_devices)(void);
typedef struct usb_bus* (*p_usb_get_busses)(void);
typedef int (*p_usb_close)(usb_dev_handle *dev);
typedef int (*p_usb_control_msg)(usb_dev_handle *dev, int requesttype, int request,
	int value, int iindex, char *bytes, int size, int timeout);
typedef usb_dev_handle* (*p_usb_open)(struct usb_device *dev);

extern p_usb_strerror usb_strerror;
extern p_usb_init usb_init;
extern p_usb_find_busses usb_find_busses;
extern p_usb_find_devices usb_find_devices;
extern p_usb_get_busses usb_get_busses; 
extern p_usb_close usb_close;
extern p_usb_control_msg usb_control_msg;
extern p_usb_open usb_open;
