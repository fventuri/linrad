//  soft66.c   V0.1
//  Linrad's 'host control' routines for Soft66 series SDR receivers
//  Uses libsoft66 by Thomas Horsten <thomas@horsten.com>
//  
//  There is currently no provision for listing devices or
//  selecting one based on serial number, although the library
//  supports these features. Instead you can set the environment
//  variable SOFT66_SERIAL to the correct serial number and
//  the correct one will be opened.
//
//  Expect more features to be added, this is a raw first cut,
//  just to get frequency control!
//
//  This code is based on si570.c which in turn is based on 
//  ausbsoftrock.c by Andrew Nilsson (andrew.nilsson@gmail.com),
//  powerSwitch.c by Christian Starkjohann,
//  and usbtemp.c by Mathias Dalheimer of Objective Development Software GmbH (2005)
//  (see http://www.obdev.at/products/vusb/index.html )
//
//  Use at your own risk and enjoy
//  Thomas
//
//  UPDATE HISTORY
//  06 November 2010	Created

#include "osnum.h"
#include "loadusb.h"
#include <string.h>
#include "uidef.h" 
#include "screendef.h"
#include "fft1def.h"
#include "options.h"

#if OSNUM == OSNUM_WINDOWS
#include <windows.h>
#include "wscreen.h"
#ifdef interface
#undef interface
#endif
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif




#define SOFT66_SERIAL_ENV "SOFT66_SERIAL"
#define SOFT66_VID_ENV "SOFT66_VID"
#define SOFT66_PID_ENV "SOFT66_PID"

#define SOFT66_VID_DEFAULT 0x0403
#define SOFT66_PID_DEFAULT 0x6001


enum soft66_model {
	SOFT66_MODEL_UNKNOWN = 0x0,
	SOFT66_MODEL_SOFT66ADD = 0x1,
};

struct soft66_context {
	int model;
	unsigned int chipid;
	unsigned int freq_min;
	unsigned int freq_max;
	unsigned int mclk;

	struct ftdi_context *ftdi;
};


//struct soft66_context *soft66_new(void);
typedef struct soft66_context* (*p_soft66_new)(void);
typedef void (*p_soft66_free)(struct soft66_context *ctx);
typedef int (*p_soft66_set_frequency)(struct soft66_context *ctx, int freq, int noreset);
typedef int (*p_soft66_open_device)(struct soft66_context *ctx, 
                                       int vid, int pid, const char *serial);
p_soft66_new soft66_new;
p_soft66_set_frequency soft66_set_frequency;
p_soft66_open_device soft66_open_device;
p_soft66_free soft66_free;

int soft66_library_flag;



#define MAX_SOFT66_PARM            1

static char *soft66_parfil_name="par_soft66";
static char *soft66_parm_text[MAX_SOFT66_PARM]={"Power down on exit"
}; 

typedef struct {
	int PowerDownOnExit;
}P_SOFT66;
P_SOFT66 soft66;
struct soft66_context *soft66_handle = NULL;
int soft66_needs_reset = 0;

#if(OSNUM == OSNUM_WINDOWS)
#include <windows.h>
HANDLE soft66_libhandle;

void load_soft66_library(void)
{
char soft66_dllname[80];
int info;
if(soft66_library_flag)return;
info=0;
sprintf(soft66_dllname,"%ssoft66.dll",DLLDIR);
soft66_libhandle=LoadLibraryEx(soft66_dllname, NULL, 
                                            LOAD_WITH_ALTERED_SEARCH_PATH);
if(!soft66_libhandle)goto soft66_load_error;
info=1;
soft66_new=(p_soft66_new)(void*)GetProcAddress(soft66_libhandle, "soft66_new");
if(!soft66_new)goto soft66_sym_error;
soft66_set_frequency=(p_soft66_set_frequency)
                        (void*)GetProcAddress(soft66_libhandle, "soft66_set_frequency");
if(!soft66_set_frequency)goto soft66_sym_error;
soft66_open_device=(p_soft66_open_device)
                            (void*)GetProcAddress(soft66_libhandle, "soft66_open_device");
if(!soft66_open_device)goto soft66_sym_error;
soft66_free=(p_soft66_free)(void*)GetProcAddress(soft66_libhandle, "soft66_free");
if(!soft66_free)goto soft66_sym_error;
soft66_library_flag=TRUE;
if(!soft66_new)goto soft66_sym_error;
return;
soft66_sym_error:;
FreeLibrary(soft66_libhandle);
soft66_load_error:;
library_error_screen(soft66_dllname,info);
}

void unload_soft66_library(void)
{
if(!soft66_library_flag)return;
FreeLibrary(soft66_libhandle);
soft66_library_flag=FALSE;
}
#endif


#if(OSNUM == OSNUM_LINUX)
#include <dlfcn.h>
void *soft66_libhandle;

void load_soft66_library(void)
{ 
int info;
info=0;
if(soft66_library_flag)return;
soft66_libhandle=dlopen(SOFT66_LIBNAME, RTLD_LAZY);
if(!soft66_libhandle)goto soft66_load_error;
info=1;
soft66_new=(p_soft66_new)dlsym(soft66_libhandle, "soft66_new");
if(dlerror() != 0)goto soft66_sym_error;
soft66_set_frequency=(p_soft66_set_frequency)
                        dlsym(soft66_libhandle, "soft66_set_frequency");
soft66_open_device=(p_soft66_open_device)
                            dlsym(soft66_libhandle, "soft66_open_device");
soft66_free=(p_soft66_free)dlsym(soft66_libhandle, "soft66_free");
soft66_library_flag=TRUE;
return;
soft66_sym_error:;
dlclose(soft66_libhandle);
soft66_load_error:;
library_error_screen(SOFT66_LIBNAME,info);
}

void unload_soft66_library(void)
{
if(!soft66_library_flag)return;
dlclose(soft66_libhandle);
soft66_library_flag=FALSE;
}
#endif



static void set_default_parameters(void)
{
// SET DEFAULTS VALUES FOR Soft66
	soft66.PowerDownOnExit =0;
}

static void read_par_soft66_file(void)
{
	int errcod;
	errcod=read_sdrpar(soft66_parfil_name, MAX_SOFT66_PARM, 
			   soft66_parm_text, (int*)((void*)&soft66));
	if (errcod != 0)
	{
		FILE *soft66_file;
		int *sdr_pi,i;
		soft66_file=fopen(soft66_parfil_name,"w");
		set_default_parameters();
		sdr_pi=(int*)(&soft66);
		for(i=0; i<MAX_SOFT66_PARM; i++)
		{
			fprintf(soft66_file,"%s [%d]\n",soft66_parm_text[i],sdr_pi[i]);
		}
		parfile_end(soft66_file);
	}
}

static void save_par_soft66_file(void)
{
	FILE *soft66_file;
	int *sdr_pi,i;
	soft66_file=fopen(soft66_parfil_name,"w");
	sdr_pi=(int*)(&soft66);
	for(i=0; i<MAX_SOFT66_PARM; i++)
	{
		fprintf(soft66_file,"%s [%d]\n",soft66_parm_text[i],sdr_pi[i]);
	}
	parfile_end(soft66_file);
}

static int Soft66OpenDevice(void)
{
	int r;
	struct soft66_context *ctx = soft66_new();
	read_par_soft66_file();
	if (!ctx)
		return -1;
	r = soft66_open_device(ctx, 0, 0, NULL);
	if (r < 0) {
		soft66_free(ctx);
		ctx = NULL;
	}
	soft66_needs_reset = 1;
	soft66_handle = ctx;
	return r;
return 0;
}
/*
static int Soft66CloseDevice(void)
{
	if (soft66_handle)
		soft66_free(soft66_handle);
	soft66_handle = NULL;
	soft66_needs_reset = 0;
	return 0;
}
*/
static int setFreqByValue(unsigned int frequency)
{
	int r;
	if (!soft66_handle) {
		r = Soft66OpenDevice();
		if (r<0)
			return r;
	}
		
	r = soft66_set_frequency(soft66_handle, frequency, soft66_needs_reset);
	//printf("sfbv: %d (%d)\n", frequency, soft66_needs_reset);
	if (r < 0)
		printf("soft66.c: Error when setting frequency %d, returncode=%i\n",frequency,r); 
	soft66_needs_reset = 0;
	return r;
}

void soft66_setup(void)
{
char s[160];
int  retval;
int  line;
load_soft66_library();
if(!soft66_library_flag)return;
start_setup:
clear_screen();
settextcolor(14);
lir_text(2,1,"CURRENT SETUP FOR 'SOFT66 USB DEVICE' ");
settextcolor(7);
line= 4;
read_par_soft66_file();
retval=Soft66OpenDevice();
if(retval == 0)
  {
  settextcolor(10);
  sprintf(s,"Soft66 Model: %d", soft66_handle->model);
  lir_text(38,line,s);
  settextcolor(7);
  line+=2;
  }
else
  {
  settextcolor(12);
  sprintf(s,"Could not open device, probably not connected");
  lir_text(58,line,s);
  settextcolor(7);
  line+=2;
  }
sprintf(s,"Soft66 power down on exit = ");
lir_text(2,line,s);
settextcolor(10);
sprintf(s,"%i",soft66.PowerDownOnExit);
lir_text(38,line,s);
settextcolor(7);
line+=6;
lir_text(2,line,"A = Enable automatic power-down.");
line++;
lir_text(2,line,"B = Disable automatic power-down.");
line++;
line++;
lir_text(2,line,"X = Exit.");
setup_menu:;
await_processed_keyboard();
if(kill_all_flag)return;
switch(lir_inkey)
  {
  case 'A':
  soft66.PowerDownOnExit=0;
  save_par_soft66_file();
  goto start_setup;

  case 'B':
  soft66.PowerDownOnExit=0;
  save_par_soft66_file();
  goto start_setup;

  case 'X':
  unload_soft66_library();
  return;

  default:
  goto setup_menu;
  }
}

void soft66_rx_freq_control(void)
{
	int  retval;
	int f;

	fg.passband_direction=1;
	fft1_direction = fg.passband_direction;
	fg.passband_increment=0.050;

	if (!soft66_handle) {
		retval=Soft66OpenDevice();
		if(retval != 0 ) printf("soft66.c: Could not find Soft66 device\n");
		return;
	}

	if (fg.passband_center*1000000.0 < soft66_handle->freq_min )
	{
		fg.passband_center = soft66_handle->freq_min/1000000.0;
	} else if (fg.passband_center*1000000.0 > soft66_handle->freq_max )
	{
		fg.passband_center = soft66_handle->freq_max/1000000.0;
	}
	f = (int)(fg.passband_center*1000000.0);
	//f = (int)(fg.passband_center*1000000.0);
	retval=setFreqByValue(f);
	if (retval < 0 ) 
	{
		if(retval != 0 ) printf("soft66.c: Could not set frequency\n");
	}
}
