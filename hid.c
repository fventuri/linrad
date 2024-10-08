
#include "osnum.h"
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <errno.h>
#include <stdlib.h>

/* Unix */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <poll.h>

/* Linux */
#ifndef _HIDRAW_H
#define _HIDRAW_H

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/ioccom.h>
#define _IOC_READ IOC_OUT
#define _IOC_WRITE IOC_IN
typedef uint8_t u8;   
typedef uint16_t u16; 
typedef uint32_t u32; 
typedef u_int32_t uint32_t;
typedef int16_t __s16;
#endif
#ifdef __NetBSD__
#include <sys/types.h>
#include <sys/ioccom.h>
#define _IOC_READ IOC_OUT
#define _IOC_WRITE IOC_IN
typedef uint8_t u8;   
typedef uint16_t u16; 
typedef uint32_t u32; 
typedef u_int32_t uint32_t;
typedef int16_t __s16;
#endif

#ifndef __HID_H
#define __HID_H

/*
 *  Copyright (c) 1999 Andreas Gal
 *  Copyright (c) 2000-2001 Vojtech Pavlik
 *  Copyright (c) 2006-2007 Jiri Kosina
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Should you need to contact me, the author, you can do so either by
 * e-mail - mail your message to <vojtech@ucw.cz>, or by paper mail:
 * Vojtech Pavlik, Simunkova 1594, Prague 8, 182 00 Czech Republic
 */

/*
 * USB HID (Human Interface Device) interface class code
 */

#define USB_INTERFACE_CLASS_HID		3

/*
 * USB HID interface subclass and protocol codes
 */

#define USB_INTERFACE_SUBCLASS_BOOT	1
#define USB_INTERFACE_PROTOCOL_KEYBOARD	1
#define USB_INTERFACE_PROTOCOL_MOUSE	2

/*
 * HID class requests
 */

#define HID_REQ_GET_REPORT		0x01
#define HID_REQ_GET_IDLE		0x02
#define HID_REQ_GET_PROTOCOL		0x03
#define HID_REQ_SET_REPORT		0x09
#define HID_REQ_SET_IDLE		0x0A
#define HID_REQ_SET_PROTOCOL		0x0B

/*
 * HID class descriptor types
 */

#define HID_DT_HID			(USB_TYPE_CLASS | 0x01)
#define HID_DT_REPORT			(USB_TYPE_CLASS | 0x02)
#define HID_DT_PHYSICAL			(USB_TYPE_CLASS | 0x03)

#define HID_MAX_DESCRIPTOR_SIZE		4096

#ifdef __KERNEL__

#include <sys/types.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/mod_devicetable.h> /* hid_device_id */
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/input.h>

/*
 * We parse each description item into this structure. Short items data
 * values are expanded to 32-bit signed int, long items contain a pointer
 * into the data area.
 */

struct hid_item {
	unsigned  format;
	__u8      size;
	__u8      type;
	__u8      tag;
	union {
	    __u8   u8;
	    __s8   s8;
	    __u16  u16;
	    __s16  s16;
	    __u32  u32;
	    __s32  s32;
	    __u8  *longdata;
	} data;
};

/*
 * HID report item format
 */

#define HID_ITEM_FORMAT_SHORT	0
#define HID_ITEM_FORMAT_LONG	1

/*
 * Special tag indicating long items
 */

#define HID_ITEM_TAG_LONG	15

/*
 * HID report descriptor item type (prefix bit 2,3)
 */

#define HID_ITEM_TYPE_MAIN		0
#define HID_ITEM_TYPE_GLOBAL		1
#define HID_ITEM_TYPE_LOCAL		2
#define HID_ITEM_TYPE_RESERVED		3

/*
 * HID report descriptor main item tags
 */

#define HID_MAIN_ITEM_TAG_INPUT			8
#define HID_MAIN_ITEM_TAG_OUTPUT		9
#define HID_MAIN_ITEM_TAG_FEATURE		11
#define HID_MAIN_ITEM_TAG_BEGIN_COLLECTION	10
#define HID_MAIN_ITEM_TAG_END_COLLECTION	12

/*
 * HID report descriptor main item contents
 */

#define HID_MAIN_ITEM_CONSTANT		0x001
#define HID_MAIN_ITEM_VARIABLE		0x002
#define HID_MAIN_ITEM_RELATIVE		0x004
#define HID_MAIN_ITEM_WRAP		0x008
#define HID_MAIN_ITEM_NONLINEAR		0x010
#define HID_MAIN_ITEM_NO_PREFERRED	0x020
#define HID_MAIN_ITEM_NULL_STATE	0x040
#define HID_MAIN_ITEM_VOLATILE		0x080
#define HID_MAIN_ITEM_BUFFERED_BYTE	0x100

/*
 * HID report descriptor collection item types
 */

#define HID_COLLECTION_PHYSICAL		0
#define HID_COLLECTION_APPLICATION	1
#define HID_COLLECTION_LOGICAL		2

/*
 * HID report descriptor global item tags
 */

#define HID_GLOBAL_ITEM_TAG_USAGE_PAGE		0
#define HID_GLOBAL_ITEM_TAG_LOGICAL_MINIMUM	1
#define HID_GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM	2
#define HID_GLOBAL_ITEM_TAG_PHYSICAL_MINIMUM	3
#define HID_GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUM	4
#define HID_GLOBAL_ITEM_TAG_UNIT_EXPONENT	5
#define HID_GLOBAL_ITEM_TAG_UNIT		6
#define HID_GLOBAL_ITEM_TAG_REPORT_SIZE		7
#define HID_GLOBAL_ITEM_TAG_REPORT_ID		8
#define HID_GLOBAL_ITEM_TAG_REPORT_COUNT	9
#define HID_GLOBAL_ITEM_TAG_PUSH		10
#define HID_GLOBAL_ITEM_TAG_POP			11

/*
 * HID report descriptor local item tags
 */

#define HID_LOCAL_ITEM_TAG_USAGE		0
#define HID_LOCAL_ITEM_TAG_USAGE_MINIMUM	1
#define HID_LOCAL_ITEM_TAG_USAGE_MAXIMUM	2
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_INDEX	3
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_MINIMUM	4
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_MAXIMUM	5
#define HID_LOCAL_ITEM_TAG_STRING_INDEX		7
#define HID_LOCAL_ITEM_TAG_STRING_MINIMUM	8
#define HID_LOCAL_ITEM_TAG_STRING_MAXIMUM	9
#define HID_LOCAL_ITEM_TAG_DELIMITER		10

/*
 * HID usage tables
 */

#define HID_USAGE_PAGE		0xffff0000

#define HID_UP_UNDEFINED	0x00000000
#define HID_UP_GENDESK		0x00010000
#define HID_UP_SIMULATION	0x00020000
#define HID_UP_KEYBOARD		0x00070000
#define HID_UP_LED		0x00080000
#define HID_UP_BUTTON		0x00090000
#define HID_UP_ORDINAL		0x000a0000
#define HID_UP_CONSUMER		0x000c0000
#define HID_UP_DIGITIZER	0x000d0000
#define HID_UP_PID		0x000f0000
#define HID_UP_HPVENDOR         0xff7f0000
#define HID_UP_MSVENDOR		0xff000000
#define HID_UP_CUSTOM		0x00ff0000
#define HID_UP_LOGIVENDOR	0xffbc0000

#define HID_USAGE		0x0000ffff

#define HID_GD_POINTER		0x00010001
#define HID_GD_MOUSE		0x00010002
#define HID_GD_JOYSTICK		0x00010004
#define HID_GD_GAMEPAD		0x00010005
#define HID_GD_KEYBOARD		0x00010006
#define HID_GD_KEYPAD		0x00010007
#define HID_GD_MULTIAXIS	0x00010008
#define HID_GD_X		0x00010030
#define HID_GD_Y		0x00010031
#define HID_GD_Z		0x00010032
#define HID_GD_RX		0x00010033
#define HID_GD_RY		0x00010034
#define HID_GD_RZ		0x00010035
#define HID_GD_SLIDER		0x00010036
#define HID_GD_DIAL		0x00010037
#define HID_GD_WHEEL		0x00010038
#define HID_GD_HATSWITCH	0x00010039
#define HID_GD_BUFFER		0x0001003a
#define HID_GD_BYTECOUNT	0x0001003b
#define HID_GD_MOTION		0x0001003c
#define HID_GD_START		0x0001003d
#define HID_GD_SELECT		0x0001003e
#define HID_GD_VX		0x00010040
#define HID_GD_VY		0x00010041
#define HID_GD_VZ		0x00010042
#define HID_GD_VBRX		0x00010043
#define HID_GD_VBRY		0x00010044
#define HID_GD_VBRZ		0x00010045
#define HID_GD_VNO		0x00010046
#define HID_GD_FEATURE		0x00010047
#define HID_GD_UP		0x00010090
#define HID_GD_DOWN		0x00010091
#define HID_GD_RIGHT		0x00010092
#define HID_GD_LEFT		0x00010093

#define HID_DG_DIGITIZER	0x000d0001
#define HID_DG_PEN		0x000d0002
#define HID_DG_LIGHTPEN		0x000d0003
#define HID_DG_TOUCHSCREEN	0x000d0004
#define HID_DG_TOUCHPAD		0x000d0005
#define HID_DG_STYLUS		0x000d0020
#define HID_DG_PUCK		0x000d0021
#define HID_DG_FINGER		0x000d0022
#define HID_DG_TIPPRESSURE	0x000d0030
#define HID_DG_BARRELPRESSURE	0x000d0031
#define HID_DG_INRANGE		0x000d0032
#define HID_DG_TOUCH		0x000d0033
#define HID_DG_UNTOUCH		0x000d0034
#define HID_DG_TAP		0x000d0035
#define HID_DG_TABLETFUNCTIONKEY	0x000d0039
#define HID_DG_PROGRAMCHANGEKEY	0x000d003a
#define HID_DG_INVERT		0x000d003c
#define HID_DG_TIPSWITCH	0x000d0042
#define HID_DG_TIPSWITCH2	0x000d0043
#define HID_DG_BARRELSWITCH	0x000d0044
#define HID_DG_ERASER		0x000d0045
#define HID_DG_TABLETPICK	0x000d0046
/*
 * as of May 20, 2009 the usages below are not yet in the official USB spec
 * but are being pushed by Microsft as described in their paper "Digitizer
 * Drivers for Windows Touch and Pen-Based Computers"
 */
#define HID_DG_CONFIDENCE	0x000d0047
#define HID_DG_WIDTH		0x000d0048
#define HID_DG_HEIGHT		0x000d0049
#define HID_DG_CONTACTID	0x000d0051
#define HID_DG_INPUTMODE	0x000d0052
#define HID_DG_DEVICEINDEX	0x000d0053
#define HID_DG_CONTACTCOUNT	0x000d0054
#define HID_DG_CONTACTMAX	0x000d0055

/*
 * HID report types --- Ouch! HID spec says 1 2 3!
 */

#define HID_INPUT_REPORT	0
#define HID_OUTPUT_REPORT	1
#define HID_FEATURE_REPORT	2

/*
 * HID connect requests
 */

#define HID_CONNECT_HIDINPUT		0x01
#define HID_CONNECT_HIDINPUT_FORCE	0x02
#define HID_CONNECT_HIDRAW		0x04
#define HID_CONNECT_HIDDEV		0x08
#define HID_CONNECT_HIDDEV_FORCE	0x10
#define HID_CONNECT_FF			0x20
#define HID_CONNECT_DEFAULT	(HID_CONNECT_HIDINPUT|HID_CONNECT_HIDRAW| \
		HID_CONNECT_HIDDEV|HID_CONNECT_FF)

/*
 * HID device quirks.
 */

/* 
 * Increase this if you need to configure more HID quirks at module load time
 */
#define MAX_USBHID_BOOT_QUIRKS 4

#define HID_QUIRK_INVERT			0x00000001
#define HID_QUIRK_NOTOUCH			0x00000002
#define HID_QUIRK_IGNORE			0x00000004
#define HID_QUIRK_NOGET				0x00000008
#define HID_QUIRK_BADPAD			0x00000020
#define HID_QUIRK_MULTI_INPUT			0x00000040
#define HID_QUIRK_SKIP_OUTPUT_REPORTS		0x00010000
#define HID_QUIRK_FULLSPEED_INTERVAL		0x10000000
#define HID_QUIRK_NO_INIT_REPORTS		0x20000000

/*
 * This is the global environment of the parser. This information is
 * persistent for main-items. The global environment can be saved and
 * restored with PUSH/POP statements.
 */

struct hid_global {
	unsigned usage_page;
	__s32    logical_minimum;
	__s32    logical_maximum;
	__s32    physical_minimum;
	__s32    physical_maximum;
	__s32    unit_exponent;
	unsigned unit;
	unsigned report_id;
	unsigned report_size;
	unsigned report_count;
};

/*
 * This is the local environment. It is persistent up the next main-item.
 */

#define HID_MAX_USAGES			12288
#define HID_DEFAULT_NUM_COLLECTIONS	16

struct hid_local {
	unsigned usage[HID_MAX_USAGES]; /* usage array */
	unsigned collection_index[HID_MAX_USAGES]; /* collection index array */
	unsigned usage_index;
	unsigned usage_minimum;
	unsigned delimiter_depth;
	unsigned delimiter_branch;
};

/*
 * This is the collection stack. We climb up the stack to determine
 * application and function of each field.
 */

struct hid_collection {
	unsigned type;
	unsigned usage;
	unsigned level;
};

struct hid_usage {
	unsigned  hid;			/* hid usage code */
	unsigned  collection_index;	/* index into collection array */
	/* hidinput data */
	__u16     code;			/* input driver code */
	__u8      type;			/* input driver type */
	__s8	  hat_min;		/* hat switch fun */
	__s8	  hat_max;		/* ditto */
	__s8	  hat_dir;		/* ditto */
};

struct hid_input;

struct hid_field {
	unsigned  physical;		/* physical usage for this field */
	unsigned  logical;		/* logical usage for this field */
	unsigned  application;		/* application usage for this field */
	struct hid_usage *usage;	/* usage table for this function */
	unsigned  maxusage;		/* maximum usage index */
	unsigned  flags;		/* main-item flags (i.e. volatile,array,constant) */
	unsigned  report_offset;	/* bit offset in the report */
	unsigned  report_size;		/* size of this field in the report */
	unsigned  report_count;		/* number of this field in the report */
	unsigned  report_type;		/* (input,output,feature) */
	__s32    *value;		/* last known value(s) */
	__s32     logical_minimum;
	__s32     logical_maximum;
	__s32     physical_minimum;
	__s32     physical_maximum;
	__s32     unit_exponent;
	unsigned  unit;
	struct hid_report *report;	/* associated report */
	unsigned index;			/* index into report->field[] */
	/* hidinput data */
	struct hid_input *hidinput;	/* associated input structure */
	__u16 dpad;			/* dpad input code */
};

#define HID_MAX_FIELDS 64

struct hid_report {
	struct list_head list;
	unsigned id;					/* id of this report */
	unsigned type;					/* report type */
	struct hid_field *field[HID_MAX_FIELDS];	/* fields of the report */
	unsigned maxfield;				/* maximum valid field index */
	unsigned size;					/* size of the report (bits) */
	struct hid_device *device;			/* associated device */
};

struct hid_report_enum {
	unsigned numbered;
	struct list_head report_list;
	struct hid_report *report_id_hash[256];
};

#define HID_REPORT_TYPES 3

#define HID_MIN_BUFFER_SIZE	64		/* make sure there is at least a packet size of space */
#define HID_MAX_BUFFER_SIZE	4096		/* 4kb */
#define HID_CONTROL_FIFO_SIZE	256		/* to init devices with >100 reports */
#define HID_OUTPUT_FIFO_SIZE	64

struct hid_control_fifo {
	unsigned char dir;
	struct hid_report *report;
	char *raw_report;
};

struct hid_output_fifo {
	struct hid_report *report;
	char *raw_report;
};

#define HID_CLAIMED_INPUT	1
#define HID_CLAIMED_HIDDEV	2
#define HID_CLAIMED_HIDRAW	4

#define HID_STAT_ADDED		1
#define HID_STAT_PARSED		2

struct hid_input {
	struct list_head list;
	struct hid_report *report;
	struct input_dev *input;
};

enum hid_type {
	HID_TYPE_OTHER = 0,
	HID_TYPE_USBMOUSE
};

struct hid_driver;
struct hid_ll_driver;

struct hid_device {							/* device report descriptor */
	__u8 *rdesc;
	unsigned rsize;
	struct hid_collection *collection;				/* List of HID collections */
	unsigned collection_size;					/* Number of allocated hid_collections */
	unsigned maxcollection;						/* Number of parsed collections */
	unsigned maxapplication;					/* Number of applications */
	__u16 bus;							/* BUS ID */
	__u32 vendor;							/* Vendor ID */
	__u32 product;							/* Product ID */
	__u32 version;							/* HID version */
	enum hid_type type;						/* device type (mouse, kbd, ...) */
	unsigned country;						/* HID country */
	struct hid_report_enum report_enum[HID_REPORT_TYPES];

	struct device dev;						/* device */
	struct hid_driver *driver;
	struct hid_ll_driver *ll_driver;

	unsigned int status;						/* see STAT flags above */
	unsigned claimed;						/* Claimed by hidinput, hiddev? */
	unsigned quirks;						/* Various quirks the device can pull on us */

	struct list_head inputs;					/* The list of inputs */
	void *hiddev;							/* The hiddev structure */
	void *hidraw;
	int minor;							/* Hiddev minor number */

	int open;							/* is the device open by anyone? */
	char name[128];							/* Device name */
	char phys[64];							/* Device physical location */
	char uniq[64];							/* Device unique identifier (serial #) */

	void *driver_data;

	/* temporary hid_ff handling (until moved to the drivers) */
	int (*ff_init)(struct hid_device *);

	/* hiddev event handler */
	int (*hiddev_connect)(struct hid_device *, unsigned int);
	void (*hiddev_disconnect)(struct hid_device *);
	void (*hiddev_hid_event) (struct hid_device *, struct hid_field *field,
				  struct hid_usage *, __s32);
	void (*hiddev_report_event) (struct hid_device *, struct hid_report *);

	/* handler for raw output data, used by hidraw */
	int (*hid_output_raw_report) (struct hid_device *, __u8 *, size_t);

	/* debugging support via debugfs */
	unsigned short debug;
	struct dentry *debug_dir;
	struct dentry *debug_rdesc;
	struct dentry *debug_events;
	struct list_head debug_list;
	wait_queue_head_t debug_wait;
};

static inline void *hid_get_drvdata(struct hid_device *hdev)
{
	return dev_get_drvdata(&hdev->dev);
}

static inline void hid_set_drvdata(struct hid_device *hdev, void *data)
{
	dev_set_drvdata(&hdev->dev, data);
}

#define HID_GLOBAL_STACK_SIZE 4
#define HID_COLLECTION_STACK_SIZE 4

struct hid_parser {
	struct hid_global     global;
	struct hid_global     global_stack[HID_GLOBAL_STACK_SIZE];
	unsigned              global_stack_ptr;
	struct hid_local      local;
	unsigned              collection_stack[HID_COLLECTION_STACK_SIZE];
	unsigned              collection_stack_ptr;
	struct hid_device    *device;
};

struct hid_class_descriptor {
	__u8  bDescriptorType;
	__le16 wDescriptorLength;
} __attribute__ ((packed));

struct hid_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__le16 bcdHID;
	__u8  bCountryCode;
	__u8  bNumDescriptors;

	struct hid_class_descriptor desc[1];
} __attribute__ ((packed));

#define HID_DEVICE(b, ven, prod) \
	.bus = (b), \
	.vendor = (ven), .product = (prod)

#define HID_USB_DEVICE(ven, prod)	HID_DEVICE(BUS_USB, ven, prod)
#define HID_BLUETOOTH_DEVICE(ven, prod)	HID_DEVICE(BUS_BLUETOOTH, ven, prod)

#define HID_REPORT_ID(rep) \
	.report_type = (rep)
#define HID_USAGE_ID(uhid, utype, ucode) \
	.usage_hid = (uhid), .usage_type = (utype), .usage_code = (ucode)
/* we don't want to catch types and codes equal to 0 */
#define HID_TERMINATOR		(HID_ANY_ID - 1)

struct hid_report_id {
	__u32 report_type;
};
struct hid_usage_id {
	__u32 usage_hid;
	__u32 usage_type;
	__u32 usage_code;
};

/**
 * struct hid_driver
 * @name: driver name (e.g. "Footech_bar-wheel")
 * @id_table: which devices is this driver for (must be non-NULL for probe
 * 	      to be called)
 * @dyn_list: list of dynamically added device ids
 * @dyn_lock: lock protecting @dyn_list
 * @probe: new device inserted
 * @remove: device removed (NULL if not a hot-plug capable driver)
 * @report_table: on which reports to call raw_event (NULL means all)
 * @raw_event: if report in report_table, this hook is called (NULL means nop)
 * @usage_table: on which events to call event (NULL means all)
 * @event: if usage in usage_table, this hook is called (NULL means nop)
 * @report_fixup: called before report descriptor parsing (NULL means nop)
 * @input_mapping: invoked on input registering before mapping an usage
 * @input_mapped: invoked on input registering after mapping an usage
 *
 * raw_event and event should return 0 on no action performed, 1 when no
 * further processing should be done and negative on error
 *
 * input_mapping shall return a negative value to completely ignore this usage
 * (e.g. doubled or invalid usage), zero to continue with parsing of this
 * usage by generic code (no special handling needed) or positive to skip
 * generic parsing (needed special handling which was done in the hook already)
 * input_mapped shall return negative to inform the layer that this usage
 * should not be considered for further processing or zero to notify that
 * no processing was performed and should be done in a generic manner
 * Both these functions may be NULL which means the same behavior as returning
 * zero from them.
 */
struct hid_driver {
	char *name;
	const struct hid_device_id *id_table;

	struct list_head dyn_list;
	spinlock_t dyn_lock;

	int (*probe)(struct hid_device *dev, const struct hid_device_id *id);
	void (*remove)(struct hid_device *dev);

	const struct hid_report_id *report_table;
	int (*raw_event)(struct hid_device *hdev, struct hid_report *report,
			u8 *data, int size);
	const struct hid_usage_id *usage_table;
	int (*event)(struct hid_device *hdev, struct hid_field *field,
			struct hid_usage *usage, __s32 value);

	void (*report_fixup)(struct hid_device *hdev, __u8 *buf,
			unsigned int size);

	int (*input_mapping)(struct hid_device *hdev,
			struct hid_input *hidinput, struct hid_field *field,
			struct hid_usage *usage, unsigned long **bit, int *max);
	int (*input_mapped)(struct hid_device *hdev,
			struct hid_input *hidinput, struct hid_field *field,
			struct hid_usage *usage, unsigned long **bit, int *max);

/* private: */
	struct device_driver driver;
};

/**
 * hid_ll_driver - low level driver callbacks
 * @start: called on probe to start the device
 * @stop: called on remove
 * @open: called by input layer on open
 * @close: called by input layer on close
 * @hidinput_input_event: event input event (e.g. ff or leds)
 * @parse: this method is called only once to parse the device data,
 *	   shouldn't allocate anything to not leak memory
 */
struct hid_ll_driver {
	int (*start)(struct hid_device *hdev);
	void (*stop)(struct hid_device *hdev);

	int (*open)(struct hid_device *hdev);
	void (*close)(struct hid_device *hdev);

	int (*power)(struct hid_device *hdev, int level);

	int (*hidinput_input_event) (struct input_dev *idev, unsigned int type,
			unsigned int code, int value);

	int (*parse)(struct hid_device *hdev);
};

#define	PM_HINT_FULLON	1<<5
#define PM_HINT_NORMAL	1<<1

/* Applications from HID Usage Tables 4/8/99 Version 1.1 */
/* We ignore a few input applications that are not widely used */
#define IS_INPUT_APPLICATION(a) (((a >= 0x00010000) && (a <= 0x00010008)) || (a == 0x00010080) || (a == 0x000c0001) || (a == 0x000d0002))

/* HID core API */

extern int hid_debug;

extern int hid_add_device(struct hid_device *);
extern void hid_destroy_device(struct hid_device *);

extern int __must_check __hid_register_driver(struct hid_driver *,
		struct module *, const char *mod_name);
static inline int __must_check hid_register_driver(struct hid_driver *driver)
{
	return __hid_register_driver(driver, THIS_MODULE, KBUILD_MODNAME);
}
extern void hid_unregister_driver(struct hid_driver *);

extern void hidinput_hid_event(struct hid_device *, struct hid_field *, struct hid_usage *, __s32);
extern void hidinput_report_event(struct hid_device *hid, struct hid_report *report);
extern int hidinput_connect(struct hid_device *hid, unsigned int force);
extern void hidinput_disconnect(struct hid_device *);

int hid_set_field(struct hid_field *, unsigned, __s32);
int hid_input_report(struct hid_device *, int type, u8 *, int, int);
int hidinput_find_field(struct hid_device *hid, unsigned int type, unsigned int code, struct hid_field **field);
void hid_output_report(struct hid_report *report, __u8 *data);
struct hid_device *hid_allocate_device(void);
int hid_parse_report(struct hid_device *hid, __u8 *start, unsigned size);
int hid_check_keys_pressed(struct hid_device *hid);
int hid_connect(struct hid_device *hid, unsigned int connect_mask);
void hid_disconnect(struct hid_device *hid);

/**
 * hid_map_usage - map usage input bits
 *
 * @hidinput: hidinput which we are interested in
 * @usage: usage to fill in
 * @bit: pointer to input->{}bit (out parameter)
 * @max: maximal valid usage->code to consider later (out parameter)
 * @type: input event type (EV_KEY, EV_REL, ...)
 * @c: code which corresponds to this usage and type
 */
static inline void hid_map_usage(struct hid_input *hidinput,
		struct hid_usage *usage, unsigned long **bit, int *max,
		__u8 type, __u16 c)
{
	struct input_dev *input = hidinput->input;

	usage->type = type;
	usage->code = c;

	switch (type) {
	case EV_ABS:
		*bit = input->absbit;
		*max = ABS_MAX;
		break;
	case EV_REL:
		*bit = input->relbit;
		*max = REL_MAX;
		break;
	case EV_KEY:
		*bit = input->keybit;
		*max = KEY_MAX;
		break;
	case EV_LED:
		*bit = input->ledbit;
		*max = LED_MAX;
		break;
	}
}

/**
 * hid_map_usage_clear - map usage input bits and clear the input bit
 *
 * The same as hid_map_usage, except the @c bit is also cleared in supported
 * bits (@bit).
 */
static inline void hid_map_usage_clear(struct hid_input *hidinput,
		struct hid_usage *usage, long **bit, int *max,
		__u8 type, __u16 c)
{
	hid_map_usage(hidinput, usage, bit, max, type, c);
	clear_bit(c, *bit);
}

/**
 * hid_parse - parse HW reports
 *
 * @hdev: hid device
 *
 * Call this from probe after you set up the device (if needed). Your
 * report_fixup will be called (if non-NULL) after reading raw report from
 * device before passing it to hid layer for real parsing.
 */
static inline int __must_check hid_parse(struct hid_device *hdev)
{
	int ret;

	if (hdev->status & HID_STAT_PARSED)
		return 0;

	ret = hdev->ll_driver->parse(hdev);
	if (!ret)
		hdev->status |= HID_STAT_PARSED;

	return ret;
}

/**
 * hid_hw_start - start underlaying HW
 *
 * @hdev: hid device
 * @connect_mask: which outputs to connect, see HID_CONNECT_*
 *
 * Call this in probe function *after* hid_parse. This will setup HW buffers
 * and start the device (if not deffered to device open). hid_hw_stop must be
 * called if this was successfull.
 */
static inline int __must_check hid_hw_start(struct hid_device *hdev,
		unsigned int connect_mask)
{
	int ret = hdev->ll_driver->start(hdev);
	if (ret || !connect_mask)
		return ret;
	ret = hid_connect(hdev, connect_mask);
	if (ret)
		hdev->ll_driver->stop(hdev);
	return ret;
}

/**
 * hid_hw_stop - stop underlaying HW
 *
 * @hdev: hid device
 *
 * This is usually called from remove function or from probe when something
 * failed and hid_hw_start was called already.
 */
static inline void hid_hw_stop(struct hid_device *hdev)
{
	hid_disconnect(hdev);
	hdev->ll_driver->stop(hdev);
}

void hid_report_raw_event(struct hid_device *hid, int type, u8 *data, int size,
		int interrupt);

extern int hid_generic_init(void);
extern void hid_generic_exit(void);

/* HID quirks API */
u32 usbhid_lookup_quirk(const u16 idVendor, const u16 idProduct);
int usbhid_quirks_init(char **quirks_param);
void usbhid_quirks_exit(void);
void usbhid_set_leds(struct hid_device *hid);

#ifdef CONFIG_HID_PID
int hid_pidff_init(struct hid_device *hid);
#else
#define hid_pidff_init NULL
#endif

#define dbg_hid(format, arg...) if (hid_debug) \
				printk(KERN_DEBUG "%s: " format ,\
				__FILE__ , ## arg)
#define err_hid(format, arg...) printk(KERN_ERR "%s: " format "\n" , \
		__FILE__ , ## arg)
#endif /* HID_FF */

#endif

#ifdef __FreeBSD__
#include <sys/types.h>
typedef uint8_t __u8; 
typedef uint32_t __u32;
#elif defined(__NetBSD__)
#include <sys/types.h>
typedef uint8_t __u8; 
typedef uint32_t __u32;
#else
#include <linux/types.h>
#endif
  
struct hidraw_report_descriptor {
	__u32 size;
	__u8 value[HID_MAX_DESCRIPTOR_SIZE];
};

struct hidraw_devinfo {
	__u32 bustype;
	__s16 vendor;
	__s16 product;
};

/* ioctl interface */
#define HIDIOCGRDESCSIZE	_IOR('H', 0x01, int)
#define HIDIOCGRDESC		_IOR('H', 0x02, struct hidraw_report_descriptor)
#define HIDIOCGRAWINFO		_IOR('H', 0x03, struct hidraw_devinfo)
#define HIDIOCGRAWNAME(len)     _IOC(_IOC_READ, 'H', 0x04, len)
#define HIDIOCGRAWPHYS(len)     _IOC(_IOC_READ, 'H', 0x05, len)

#define HIDRAW_FIRST_MINOR 0
#define HIDRAW_MAX_DEVICES 64
/* number of reports to buffer */
#define HIDRAW_BUFFER_SIZE 64


/* kernel-only API declarations */
#ifdef __KERNEL__

struct hidraw {
	unsigned int minor;
	int exist;
	int open;
	wait_queue_head_t wait;
	struct hid_device *hid;
	struct device *dev;
	struct list_head list;
};

struct hidraw_report {
	__u8 *value;
	int len;
};

struct hidraw_list {
	struct hidraw_report buffer[HIDRAW_BUFFER_SIZE];
	int head;
	int tail;
	struct fasync_struct *fasync;
	struct hidraw *hidraw;
	struct list_head node;
	struct mutex read_mutex;
};

#ifdef CONFIG_HIDRAW
int hidraw_init(void);
void hidraw_exit(void);
void hidraw_report_event(struct hid_device *, u8 *, int);
int hidraw_connect(struct hid_device *);
void hidraw_disconnect(struct hid_device *);
#else
static inline int hidraw_init(void) { return 0; }
static inline void hidraw_exit(void) { }
static inline void hidraw_report_event(struct hid_device *hid, u8 *data, int len) { }
static inline int hidraw_connect(struct hid_device *hid) { return -1; }
static inline void hidraw_disconnect(struct hid_device *hid) { }
#endif

#endif

#endif

/* Definitions from linux/hidraw.h. Since these are new, some distros
   may not have header files which contain them. */
#ifndef HIDIOCSFEATURE
#define HIDIOCSFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x06, len)
#endif
#ifndef HIDIOCGFEATURE
#define HIDIOCGFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x07, len)
#endif

#if !defined(__FreeBSD__) && !defined(__NetBSD__)
#include <linux/version.h>
#endif
 
#include <dlfcn.h>
#include "globdef.h"
#include "uidef.h"
#include "hidapi.h"

struct udev;
struct udev_list_entry;
struct udev_device;
struct udev_enumerate;

typedef struct udev* (*p_udev_ref)(struct udev *udev);
typedef struct udev* (*p_udev_unref)(struct udev *udev);
typedef struct udev* (*p_udev_new)(void);
typedef struct udev_list_entry* (*p_udev_list_entry_get_next)(struct udev_list_entry *list_entry);
typedef struct udev_list_entry* (*p_udev_list_entry_get_by_name)(struct udev_list_entry *list_entry, const char *name);
typedef const char* (*p_udev_list_entry_get_name)(struct udev_list_entry *list_entry);
typedef const char* (*p_udev_list_entry_get_value)(struct udev_list_entry *list_entry);
typedef struct udev_device* (*p_udev_device_unref)(struct udev_device *udev_device);
typedef struct udev_device* (*p_udev_device_new_from_syspath)(struct udev *udev, const char *syspath);
typedef struct udev_device* (*p_udev_device_new_from_devnum)(struct udev *udev, char type, dev_t devnum);
typedef struct udev_device* (*p_udev_device_get_parent_with_subsystem_devtype)(
            struct udev_device *udev_device, const char *subsystem, 
                    const char *devtype);
typedef const char* (*p_udev_device_get_devnode)(struct udev_device *udev_device);
typedef const char* (*p_udev_device_get_sysattr_value)(struct udev_device *udev_device, const char *sysattr);

typedef struct udev_enumerate* (*p_udev_enumerate_unref)(struct udev_enumerate *udev_enumerate);
typedef struct udev_enumerate* (*p_udev_enumerate_new)(struct udev *udev);
typedef int (*p_udev_enumerate_add_match_subsystem)(struct udev_enumerate *udev_enumerate, const char *subsystem);
typedef int (*p_udev_enumerate_scan_devices)(struct udev_enumerate *udev_enumerate);
typedef struct udev_list_entry* (*p_udev_enumerate_get_list_entry)(struct udev_enumerate *udev_enumerate);

p_udev_new udev_new;
p_udev_ref udev_ref;
p_udev_unref udev_unref; 
p_udev_list_entry_get_next udev_list_entry_get_next;
p_udev_list_entry_get_by_name udev_list_entry_get_by_name;
p_udev_list_entry_get_name udev_list_entry_get_name;
p_udev_list_entry_get_value udev_list_entry_get_value;
p_udev_device_unref udev_device_unref;
p_udev_device_new_from_syspath udev_device_new_from_syspath;
p_udev_device_new_from_devnum udev_device_new_from_devnum;
p_udev_device_get_parent_with_subsystem_devtype udev_device_get_parent_with_subsystem_devtype;
p_udev_device_get_devnode udev_device_get_devnode;
p_udev_enumerate_unref udev_enumerate_unref;
p_udev_device_get_sysattr_value udev_device_get_sysattr_value ;
p_udev_enumerate_new udev_enumerate_new;
p_udev_enumerate_add_match_subsystem udev_enumerate_add_match_subsystem;
p_udev_enumerate_scan_devices udev_enumerate_scan_devices;
p_udev_enumerate_get_list_entry udev_enumerate_get_list_entry;

#define udev_list_entry_foreach(list_entry, first_entry) \
        for (list_entry = first_entry; \
             list_entry != NULL; \
             list_entry = udev_list_entry_get_next(list_entry))

struct hid_device_ {
	int device_handle;
	int blocking;
	int uses_numbered_reports;
};

static __u32 kernel_version = 0;

void *udev_libhandle;

void load_udev_library(void)
{
int info;
info=0;
if(udev_library_flag)return;
udev_libhandle=dlopen(UDEV_LIBNAME, RTLD_LAZY);
if(!udev_libhandle)goto udev_load_error;
info=1;
udev_new=(p_udev_new)dlsym(udev_libhandle, "udev_new");
if(udev_new == NULL)goto udev_sym_error;
udev_ref=(p_udev_ref)dlsym(udev_libhandle, "udev_ref"); 
if(udev_ref == NULL)goto udev_sym_error;
udev_unref=(p_udev_unref)dlsym(udev_libhandle, "udev_unref"); 
if(udev_unref == NULL)goto udev_sym_error;
udev_list_entry_get_next=(p_udev_list_entry_get_next)
                          dlsym(udev_libhandle, "udev_list_entry_get_next");  
if(udev_list_entry_get_next == NULL)goto udev_sym_error;
udev_list_entry_get_by_name=(p_udev_list_entry_get_by_name)
                       dlsym(udev_libhandle, "udev_list_entry_get_by_name"); 
if(udev_list_entry_get_by_name == NULL)goto udev_sym_error;
udev_list_entry_get_name=(p_udev_list_entry_get_name)
                          dlsym(udev_libhandle, "udev_list_entry_get_name"); 
if(udev_list_entry_get_name == NULL)goto udev_sym_error;
udev_list_entry_get_value=(p_udev_list_entry_get_value)
                         dlsym(udev_libhandle, "udev_list_entry_get_value"); 
if(udev_list_entry_get_value == NULL)goto udev_sym_error;
udev_device_unref=(p_udev_device_unref)
                                 dlsym(udev_libhandle, "udev_device_unref"); 
if(udev_device_unref == NULL)goto udev_sym_error;
udev_device_new_from_syspath=(p_udev_device_new_from_syspath)
                      dlsym(udev_libhandle, "udev_device_new_from_syspath"); 
if(udev_device_new_from_syspath == NULL)goto udev_sym_error;
udev_device_new_from_devnum=(p_udev_device_new_from_devnum)
                       dlsym(udev_libhandle, "udev_device_new_from_devnum"); 
if(udev_device_new_from_devnum == NULL)goto udev_sym_error;
udev_device_get_parent_with_subsystem_devtype=
     (p_udev_device_get_parent_with_subsystem_devtype)
       dlsym(udev_libhandle, "udev_device_get_parent_with_subsystem_devtype"); 
if(udev_device_get_parent_with_subsystem_devtype == NULL)goto udev_sym_error;
udev_device_get_devnode=(p_udev_device_get_devnode)
                           dlsym(udev_libhandle, "udev_device_get_devnode"); 
if(udev_device_get_devnode == NULL)goto udev_sym_error;
udev_enumerate_unref=(p_udev_enumerate_unref)
                              dlsym(udev_libhandle, "udev_enumerate_unref"); 
if(udev_enumerate_unref == NULL)goto udev_sym_error;
udev_device_get_sysattr_value=(p_udev_device_get_sysattr_value)
                     dlsym(udev_libhandle, "udev_device_get_sysattr_value"); 
if(udev_device_get_sysattr_value == NULL)goto udev_sym_error;
udev_enumerate_new=(p_udev_enumerate_new)
                                dlsym(udev_libhandle, "udev_enumerate_new"); 
if(udev_enumerate_new == NULL)goto udev_sym_error;
udev_enumerate_add_match_subsystem=(p_udev_enumerate_add_match_subsystem)
               dlsym(udev_libhandle, "udev_enumerate_add_match_subsystem"); 
if(udev_enumerate_add_match_subsystem == NULL)goto udev_sym_error;
udev_enumerate_scan_devices=(p_udev_enumerate_scan_devices)
                      dlsym(udev_libhandle, "udev_enumerate_scan_devices"); 
if(udev_enumerate_scan_devices == NULL)goto udev_sym_error;
udev_enumerate_get_list_entry=(p_udev_enumerate_get_list_entry)
                    dlsym(udev_libhandle, "udev_enumerate_get_list_entry"); 
if(udev_enumerate_get_list_entry == NULL)goto udev_sym_error;
udev_library_flag=TRUE;
return;
udev_sym_error:;
dlclose(udev_libhandle);
udev_load_error:;
library_error_screen(UDEV_LIBNAME,info);
}

void unload_udev_library(void)
{
if(!udev_library_flag)return;
dlclose(udev_libhandle);
udev_library_flag=FALSE;
}

hid_device *new_hid_device(void)
{
	hid_device *dev = calloc(1, sizeof(hid_device));
	dev->device_handle = -1;
	dev->blocking = 1;
	dev->uses_numbered_reports = 0;

	return dev;
}

/*
static void register_error(hid_device *device, const char *op)
{
}
*/

/* Get an attribute value from a udev_device and return it as a whar_t
   string. The returned string must be freed with free() when done.*/
static wchar_t *copy_udev_string(struct udev_device *dev, const char *udev_name)
{
	const char *str;
	wchar_t *ret = NULL;
	str = udev_device_get_sysattr_value(dev, udev_name);
	if (str) {
		/* Convert the string from UTF-8 to wchar_t */
		size_t wlen = mbstowcs(NULL, str, 0);
		ret = calloc(wlen+1, sizeof(wchar_t));
		mbstowcs(ret, str, wlen+1);
		ret[wlen] = 0x0000;
	}
	
	return ret;
}

/* uses_numbered_reports() returns 1 if report_descriptor describes a device
   which contains numbered reports. */ 
static int uses_numbered_reports(__u8 *report_descriptor, __u32 size) {
	int i = 0;
	int size_code;
	int data_len, key_size;
	
	while (i < (int)size) {
		int key = report_descriptor[i];

		/* Check for the Report ID key */
		if (key == 0x85/*Report ID*/) {
			/* This device has a Report ID, which means it uses
			   numbered reports. */
			return 1;
		}
		
		//printf("key: %02hhx\n", key);
		
		if ((key & 0xf0) == 0xf0) {
			/* This is a Long Item. The next byte contains the
			   length of the data section (value) for this key.
			   See the HID specification, version 1.11, section
			   6.2.2.3, titled "Long Items." */
			if (i+1 < (int)size)
				data_len = report_descriptor[i+1];
			else
				data_len = 0; /* malformed report */
			key_size = 3;
		}
		else {
			/* This is a Short Item. The bottom two bits of the
			   key contain the size code for the data section
			   (value) for this key.  Refer to the HID
			   specification, version 1.11, section 6.2.2.2,
			   titled "Short Items." */
			size_code = key & 0x3;
			switch (size_code) {
			case 0:
			case 1:
			case 2:
				data_len = size_code;
				break;
			case 3:
				data_len = 4;
				break;
			default:
				/* Can't ever happen since size_code is & 0x3 */
				data_len = 0;
				break;
			};
			key_size = 1;
		}
		
		/* Skip over this key and it's associated data */
		i += data_len + key_size;
	}
	
	/* Didn't find a Report ID key. Device doesn't use numbered reports. */
	return 0;
}

static int get_device_string(hid_device *dev, const char *key, wchar_t *string, size_t maxlen)
{
	struct udev *udev;
	struct udev_device *udev_dev, *parent;
	struct stat s;
	int ret = -1;
	
	setlocale(LC_ALL,"");

	/* Create the udev object */
	udev = udev_new();
	if (!udev) {
		printf("Can't create udev\n");
		return -1;
	}

	/* Get the dev_t (major/minor numbers) from the file handle. */
	fstat(dev->device_handle, &s);
	/* Open a udev device from the dev_t. 'c' means character device. */
	udev_dev = udev_device_new_from_devnum(udev, 'c', s.st_rdev);
	if (udev_dev) {
		const char *str;
		/* Find the parent USB Device */
		parent = udev_device_get_parent_with_subsystem_devtype(
		       udev_dev,
		       "usb",
		       "usb_device");
		if (parent) {
			str = udev_device_get_sysattr_value(parent, key);
			if (str) {
				/* Convert the string from UTF-8 to wchar_t */
				ret = mbstowcs(string, str, maxlen);
				goto end;
			}
		}
	}

end:
	udev_device_unref(udev_dev);
	// parent doesn't need to be (and can't be) unref'd.
	// I'm not sure why, but it'll throw double-free() errors.
	udev_unref(udev);

	return ret;
}

int HID_API_EXPORT hid_init(void)
{
	/* Nothing to do for this in the Linux/hidraw implementation. */
	return 0;
}

int HID_API_EXPORT hid_exit(void)
{
	/* Nothing to do for this in the Linux/hidraw implementation. */
	return 0;
}

struct hid_device_info  HID_API_EXPORT *hid_enumerate(unsigned short vendor_id, unsigned short product_id)
{
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	
	struct hid_device_info *root = NULL; // return object
	struct hid_device_info *cur_dev = NULL;

// Do not call setlocale here - will make the decimal point
// a comma in EU countries.	
//	setlocale(LC_ALL,"");

	/* Create the udev object */
	udev = udev_new();
	if (!udev) {
		printf("Can't create udev\n");
		return NULL;
	}

	/* Create a list of the devices in the 'hidraw' subsystem. */
	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "hidraw");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);
	/* For each item, see if it matches the vid/pid, and if so
	   create a udev_device record for it */
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *sysfs_path;
		const char *dev_path;
		const char *str;
		struct udev_device *hid_dev; // The device's HID udev node.
		struct udev_device *dev; // The actual hardware device.
		struct udev_device *intf_dev; // The device's interface (in the USB sense).
		unsigned short dev_vid;
		unsigned short dev_pid;
		
		/* Get the filename of the /sys entry for the device
		   and create a udev_device object (dev) representing it */
		sysfs_path = udev_list_entry_get_name(dev_list_entry);
		hid_dev = udev_device_new_from_syspath(udev, sysfs_path);
		dev_path = udev_device_get_devnode(hid_dev);
		
		/* The device pointed to by hid_dev contains information about
		   the hidraw device. In order to get information about the
		   USB device, get the parent device with the
		   subsystem/devtype pair of "usb"/"usb_device". This will
		   be several levels up the tree, but the function will find
		   it.*/
		dev = udev_device_get_parent_with_subsystem_devtype(
		       hid_dev,
		       "usb",
		       "usb_device");
		if (!dev) {
			/* Unable to find parent usb device. */
			goto next;
		}

		/* Get the VID/PID of the device */
		str = udev_device_get_sysattr_value(dev,"idVendor");
		dev_vid = (str)? strtol(str, NULL, 16): 0x0;
		str = udev_device_get_sysattr_value(dev, "idProduct");
		dev_pid = (str)? strtol(str, NULL, 16): 0x0;

		/* Check the VID/PID against the arguments */
		if ((vendor_id == 0x0 && product_id == 0x0) ||
		    (vendor_id == dev_vid && product_id == dev_pid)) {
			struct hid_device_info *tmp;
			size_t len;

		    	/* VID/PID match. Create the record. */
			tmp = malloc(sizeof(struct hid_device_info));
			if (cur_dev) {
				cur_dev->next = tmp;
			}
			else {
				root = tmp;
			}
			cur_dev = tmp;
			
			/* Fill out the record */
			cur_dev->next = NULL;
			str = dev_path;
			if (str) {
				len = strlen(str);
				cur_dev->path = calloc(len+1, sizeof(char));
				strcpy(cur_dev->path, str);
			}
			else
				cur_dev->path = NULL;
			
			/* Serial Number */
			cur_dev->serial_number
				= copy_udev_string(dev, "serial");

			/* Manufacturer and Product strings */
			cur_dev->manufacturer_string
				= copy_udev_string(dev, "manufacturer");
			cur_dev->product_string
				= copy_udev_string(dev, "product");
			
			/* VID/PID */
			cur_dev->vendor_id = dev_vid;
			cur_dev->product_id = dev_pid;

			/* Release Number */
			str = udev_device_get_sysattr_value(dev, "bcdDevice");
			cur_dev->release_number = (str)? strtol(str, NULL, 16): 0x0;
			
			/* Interface Number */
			cur_dev->interface_number = -1;
			/* Get a handle to the interface's udev node. */
			intf_dev = udev_device_get_parent_with_subsystem_devtype(
				   hid_dev,
				   "usb",
				   "usb_interface");
			if (intf_dev) {
				str = udev_device_get_sysattr_value(intf_dev, "bInterfaceNumber");
				cur_dev->interface_number = (str)? strtol(str, NULL, 16): -1;
			}
		}
		else
			goto next;

	next:
		udev_device_unref(hid_dev);
		/* dev and intf_dev don't need to be (and can't be)
		   unref()d.  It will cause a double-free() error.  I'm not
		   sure why.  */
	}
	/* Free the enumerator and udev objects. */
	udev_enumerate_unref(enumerate);
	udev_unref(udev);
	
	return root;
}

void  HID_API_EXPORT hid_free_enumeration(struct hid_device_info *devs)
{
	struct hid_device_info *d = devs;
	while (d) {
		struct hid_device_info *next = d->next;
		free(d->path);
		free(d->serial_number);
		free(d->manufacturer_string);
		free(d->product_string);
		free(d);
		d = next;
	}
}

hid_device * hid_open(unsigned short vendor_id, unsigned short product_id, wchar_t *serial_number)
{
	struct hid_device_info *devs, *cur_dev;
	const char *path_to_open = NULL;
	hid_device *handle = NULL;
	
	devs = hid_enumerate(vendor_id, product_id);
	cur_dev = devs;
	while (cur_dev) {
		if (cur_dev->vendor_id == vendor_id &&
		    cur_dev->product_id == product_id) {
			if (serial_number) {
				if (wcscmp(serial_number, cur_dev->serial_number) == 0) {
					path_to_open = cur_dev->path;
					break;
				}
			}
			else {
				path_to_open = cur_dev->path;
				break;
			}
		}
		cur_dev = cur_dev->next;
	}

	if (path_to_open) {
		/* Open the device */
		handle = hid_open_path(path_to_open);
	}

	hid_free_enumeration(devs);
	
	return handle;
}

hid_device * HID_API_EXPORT hid_open_path(const char *path)
{
hid_device *dev = NULL;
dev = new_hid_device();
if (kernel_version == 0) 
  {
  struct utsname name;
  int major, minor, release;
  int ret;
  uname(&name);
  ret = sscanf(name.release, "%d.%d.%d", &major, &minor, &release);
  if (ret == 3) 
    {
    kernel_version = major << 16 | minor << 8 | release;
//printf("Kernel Version: %d\n", kernel_version);
    }
  else
    {
    ret = sscanf(name.release, "%d.%d-%d", &major, &minor, &release);
    if (ret == 3) 
      {
      kernel_version = major << 16 | minor << 8 | release;
//printf("Kernel Version: %d\n", kernel_version);
      }
    else 
      {
      printf("Couldn't sscanf() version string %s (see hid.c)\n", name.release);
      }
    }
  }  


	// OPEN HERE //
	dev->device_handle = open(path, O_RDWR);

	// If we have a good handle, return it.
	if (dev->device_handle > 0) {

		/* Get the report descriptor */
		int res, desc_size = 0;
		struct hidraw_report_descriptor rpt_desc;

		memset(&rpt_desc, 0x0, sizeof(rpt_desc));

		/* Get Report Descriptor Size */
		res = ioctl(dev->device_handle, HIDIOCGRDESCSIZE, &desc_size);
		if (res < 0)
			perror("HIDIOCGRDESCSIZE");


		/* Get Report Descriptor */
		rpt_desc.size = desc_size;
		res = ioctl(dev->device_handle, HIDIOCGRDESC, &rpt_desc);
		if (res < 0) {
			perror("HIDIOCGRDESC");
		} else {
			/* Determine if this device uses numbered reports. */
			dev->uses_numbered_reports =
				uses_numbered_reports(rpt_desc.value,
				                      rpt_desc.size);
		}
		
		return dev;
	}
	else {
		// Unable to open any devices.
		free(dev);
		return NULL;
	}
}


int HID_API_EXPORT hid_write(hid_device *dev, const unsigned char *data, size_t length)
{
	int bytes_written;

	bytes_written = write(dev->device_handle, data, length);

	return bytes_written;
}


int HID_API_EXPORT hid_read_timeout(hid_device *dev, unsigned char *data, size_t length, int milliseconds)
{
	int bytes_read;

	if (milliseconds != 0) {
		/* milliseconds is -1 or > 0. In both cases, we want to
		   call poll() and wait for data to arrive. -1 means
		   INFINITE. */
		int ret;
		struct pollfd fds;

		fds.fd = dev->device_handle;
		fds.events = POLLIN;
		fds.revents = 0;
		ret = poll(&fds, 1, milliseconds);
		if (ret == -1 || ret == 0)
			/* Error or timeout */
			return ret;
	}

	bytes_read = read(dev->device_handle, data, length);
	if (bytes_read < 0 && errno == EAGAIN)
		bytes_read = 0;
#if !defined(__FreeBSD__) && !defined(__NetBSD__)
	if (bytes_read >= 0 &&
	    kernel_version < KERNEL_VERSION(2,6,34) &&
	    dev->uses_numbered_reports) {
		/* Work around a kernel bug. Chop off the first byte. */
		memmove(data, data+1, bytes_read);
		bytes_read--;
	}
#endif
	return bytes_read;
}

int HID_API_EXPORT hid_read(hid_device *dev, unsigned char *data, size_t length)
{
	return hid_read_timeout(dev, data, length, (dev->blocking)? -1: 0);
}

int HID_API_EXPORT hid_set_nonblocking(hid_device *dev, int nonblock)
{
	int flags, res;

	flags = fcntl(dev->device_handle, F_GETFL, 0);
	if (flags >= 0) {
		if (nonblock)
			res = fcntl(dev->device_handle, F_SETFL, flags | O_NONBLOCK);
		else
			res = fcntl(dev->device_handle, F_SETFL, flags & ~O_NONBLOCK);
	}
	else
		return -1;

	if (res < 0) {
		return -1;
	}
	else {
		dev->blocking = !nonblock;
		return 0; /* Success */
	}
}


int HID_API_EXPORT hid_send_feature_report(hid_device *dev, const unsigned char *data, size_t length)
{
	int res;

	res = ioctl(dev->device_handle, HIDIOCSFEATURE(length), data);
	if (res < 0)
		perror("ioctl (SFEATURE)");

	return res;
}

int HID_API_EXPORT hid_get_feature_report(hid_device *dev, unsigned char *data, size_t length)
{
	int res;

	res = ioctl(dev->device_handle, HIDIOCGFEATURE(length), data);
	if (res < 0)
		perror("ioctl (GFEATURE)");


	return res;
}


void HID_API_EXPORT hid_close(hid_device *dev)
{
	if (!dev)
		return;
	close(dev->device_handle);
	free(dev);
}


int HID_API_EXPORT_CALL hid_get_manufacturer_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
	return get_device_string(dev, "manufacturer", string, maxlen);
}

int HID_API_EXPORT_CALL hid_get_product_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
	return get_device_string(dev, "product", string, maxlen);
}

int HID_API_EXPORT_CALL hid_get_serial_number_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
	return get_device_string(dev, "serial", string, maxlen);
}

