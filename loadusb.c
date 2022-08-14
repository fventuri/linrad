
#include "osnum.h"
#include "loadusb.h"
#include <string.h>
#include "uidef.h"
#include "options.h"

p_ftdi_init ftdi_init;
p_ftdi_write_data ftdi_write_data;
p_ftdi_set_baudrate ftdi_set_baudrate;
p_ftdi_set_bitmode ftdi_set_bitmode;
p_ftdi_read_data ftdi_read_data;
p_ftdi_free ftdi_free;
p_ftdi_usb_find_all ftdi_usb_find_all;
p_ftdi_get_error_string ftdi_get_error_string;
p_ftdi_new ftdi_new;
p_ftdi_usb_open ftdi_usb_open;
p_ftdi_write_data_set_chunksize ftdi_write_data_set_chunksize;
p_ftdi_deinit ftdi_deinit;
p_ftdi_read_pins ftdi_read_pins;
p_ftdi_usb_close ftdi_usb_close;
p_ftdi_usb_get_strings ftdi_usb_get_strings;
p_ftdi_usb_open_dev ftdi_usb_open_dev;
p_ftdi_list_free ftdi_list_free=NULL;

p_libusb_control_transfer libusb_control_transfer;
p_libusb_get_string_descriptor_ascii libusb_get_string_descriptor_ascii;
p_libusb_close libusb_close;
p_libusb_open libusb_open;
p_libusb_init libusb_init;
p_libusb_get_device_list libusb_get_device_list;
p_libusb_get_device_descriptor libusb_get_device_descriptor;
p_libusb_free_device_list libusb_free_device_list;
/* not used (yet?)
p_libusb_reset_device libusb_reset_device;
p_libusb_exit libusb_exit;
p_libusb_bulk_transfer libusb_bulk_transfer;
p_libusb_release_interface libusb_release_interface;
p_libusb_claim_interface libusb_claim_interface;
p_libusb_set_interface_alt_setting libusb_set_interface_alt_setting;
*/



p_usb_strerror usb_strerror;
p_usb_init usb_init;
p_usb_find_busses usb_find_busses;
p_usb_find_devices usb_find_devices;
p_usb_get_busses usb_get_busses; 
p_usb_close usb_close;
p_usb_control_msg usb_control_msg;
p_usb_open usb_open;
p_libusb_alloc_transfer libusb_alloc_transfer;
p_libusb_submit_transfer libusb_submit_transfer;
p_libusb_handle_events_timeout libusb_handle_events_timeout;
p_libusb_free_transfer libusb_free_transfer;
p_libusb_cancel_transfer libusb_cancel_transfer;


int ftdi_library_flag;
int libusb1_library_flag;
int libusb0_library_flag;

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include <dlfcn.h>
#include "lscreen.h"
#endif

void library_error_screen(char* libname, int info)
{
char s[200];
int i, line;
#if(OSNUM == OSNUM_LINUX)
char* pkg;
char *ss;
#endif
#if(OSNUM == OSNUM_WINDOWS)
DWORD winerr;
#endif
settextcolor(7);
line=5;
lir_status=LIR_DLL_FAILED;
lir_sched_yield();
clear_screen();
sprintf(s,"Could not load library %s",libname);
lir_text(5,line,s);
line+=2;
if(info == 0)
  {
#if(OSNUM == OSNUM_LINUX)
  ss=dlerror();
  if(ss)lir_text(5,line,ss);
  line+=2;
  lir_text(5,line,"Did you run ./configure after installing this library?");
  line+=2;
#endif
#if(OSNUM == OSNUM_WINDOWS)
  winerr=GetLastError();
  switch (winerr)
    {
    case 0x7e:
    sprintf(s,"Module not found.");
    break;

    case 0xc1:
#if IA64 == 1
    sprintf(s,"Not a valid Win64 application or the module tries to");
    lir_text(5,line,s);
    line++;
    sprintf(s,"load another module which is not a Win64 application.");
#else
    sprintf(s,"Not a valid Win32 application or the module tries to");
    lir_text(5,line,s);
    line++;
    sprintf(s,"load another module which is not a Win32 application.");
#endif
    break;

    default:
    sprintf(s,"Call to LoadLibrary returned error code 0x%lx",winerr);
    }
  lir_text(5,line,s);
#endif
  }
else
  {
#if(OSNUM == OSNUM_LINUX)
  ss=dlerror();
  if(ss)lir_text(5,line,ss);
  line+=2;
#endif
  lir_text(5,line,"Library not compatible with Linrad. Maybe too old?");
  }
line+=2;
#if(OSNUM == OSNUM_LINUX)
#if IA64 == 0
i=32;
#else
i=64;
#endif
pkg=libname;
//                                               help        //1
//                                               x11         //2
//                                               xext        //3
//                                               svgalib     //4
if(strcmp(libname,"libasound.so")==0)pkg="ALSA";             //5
if(strcmp(libname,"libusb-1.0.so")==0)pkg="libusb1";         //6
if(strcmp(libname,"libperseus-sdr.so")==0)pkg="perseus";     //7
if(strcmp(libname,"librtlsdr.so")==0)pkg="rtlsdr";           //8
if(strcmp(libname,"libmirisdr.so")==0)pkg="mirics";          //9
if(strcmp(libname,"libbladeRF.so")==0)pkg="bladerf";         //10
//                                        libwd_dask.so      //11
if(strcmp(libname,"libfdms1_hw_ctrl.so.1.0")==0)pkg="FDM-S1";//12
if(strcmp(libname,"libairspy.so")==0)pkg="airspy";           //13
if(strcmp(libname,"libsoft66.so")==0)pkg="soft66";           //14
if(strcmp(libname,"libudev.so")==0)pkg="udev";               //15
if(strcmp(libname,"libportaudio.so")==0)pkg="portaudio";     //16
if(strcmp(libname,"libairspyhf.so")==0)pkg="airspyhf";       //13



#if BUILD == 0
lir_text(5,line,"Run ");
settextcolor(12);
sprintf(s,"./configure --with-%s-%d ", pkg, i);
lir_text(9,line,s);
settextcolor(7);
lir_text(9+strlen(s),line,"to get install instructions for");
line++;
lir_text(5,line,"this particular library or run ");
settextcolor(12);
lir_text(37,line,"./configure --with-help ");
settextcolor(7);
lir_text(61,line,"to get");
line++;
lir_text(5,line,"install instructions for all currently un-installed packages");
line+=2;
sprintf(s,"Repeat the configure command until %s (%dbit) is no longer 'Not present'",
                                                                       libname,i); 
lir_text(5,line,s);
line+=2;
#if IA64 == 0
lir_text(5,line,"Finally run make xlinrad");
#else
lir_text(5,line,"Finally run make xlinrad64."); 
#endif
#else
lir_text(5,line,"Run ");
settextcolor(12);
sprintf(s,"../configure --with-%s-%d ", pkg, i);
lir_text(9,line,s);
settextcolor(7);
lir_text(9+strlen(s),line,"to get install instructions for");
line++;
lir_text(5,line,"this particular library or run ../configure --with-help to get");
line++;
lir_text(5,line,"install instructions for all currently un-installed packages");
line+=2;
sprintf(s,"Repeat the configure command until %s (%dbit) is no longer 'Not present'",
                                                                       libname,i);
lir_text(5,line,s);
line+=2;
line+=2;
lir_text(5,line,"After the installation, run cmake .. ");
lir_text(5,line,"Finally run make"); 
#endif
line+=3;
#endif
lir_text(9,line,press_any_key);
i=lir_inkey;
await_processed_keyboard();
lir_inkey=i;
clear_screen();
}

#if(OSNUM == OSNUM_WINDOWS)
HANDLE ftdi_libhandle;
HANDLE libusb1_libhandle;
HANDLE libusb0_libhandle;

int load_ftdi_library(void)
{
char ftdi_dllname[80];
int info;
if(ftdi_library_flag)return 0;
info=0;
sprintf(ftdi_dllname,"%slibftdi.dll",DLLDIR);
ftdi_libhandle=LoadLibraryEx(ftdi_dllname, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
if(ftdi_libhandle == NULL)goto ftdi_load_error;
info=1;
ftdi_write_data=(p_ftdi_write_data)(void*)GetProcAddress(ftdi_libhandle, 
                                                         "ftdi_write_data");
if(!ftdi_write_data)goto ftdi_sym_error;
ftdi_set_baudrate=(p_ftdi_set_baudrate)(void*)GetProcAddress(ftdi_libhandle, 
                                                       "ftdi_set_baudrate");
if(!ftdi_set_baudrate)goto ftdi_sym_error;
ftdi_set_bitmode=(p_ftdi_set_bitmode)(void*)GetProcAddress(ftdi_libhandle, 
                                                         "ftdi_set_bitmode");
if(!ftdi_set_bitmode)goto ftdi_sym_error;
ftdi_read_data=(p_ftdi_read_data)(void*)GetProcAddress(ftdi_libhandle, 
                                                           "ftdi_read_data");
if(!ftdi_read_data)goto ftdi_sym_error;
ftdi_free=(p_ftdi_free)(void*)GetProcAddress(ftdi_libhandle, "ftdi_free");
if(!ftdi_free)goto ftdi_sym_error;
ftdi_usb_find_all=(p_ftdi_usb_find_all)(void*)GetProcAddress(ftdi_libhandle, 
                                                     "ftdi_usb_find_all");
if(!ftdi_usb_find_all)goto ftdi_sym_error;
ftdi_get_error_string=(void*)(char*)(void*)GetProcAddress(ftdi_libhandle, 
                                                 "ftdi_get_error_string");
if(!ftdi_get_error_string)goto ftdi_sym_error;
ftdi_new=(void*)(struct ftdi_context*)(void*)GetProcAddress(ftdi_libhandle, 
                                                               "ftdi_new");
if(!ftdi_new)goto ftdi_sym_error;
ftdi_usb_open=(p_ftdi_usb_open)(void*)GetProcAddress(ftdi_libhandle, "ftdi_usb_open");
if(!ftdi_usb_open)goto ftdi_sym_error;
ftdi_init=(p_ftdi_init)(void*)GetProcAddress(ftdi_libhandle, "ftdi_init");
if(!ftdi_init)goto ftdi_sym_error;
ftdi_write_data_set_chunksize=(p_ftdi_write_data_set_chunksize)
             (void*)GetProcAddress(ftdi_libhandle, "ftdi_write_data_set_chunksize");
if(!ftdi_write_data_set_chunksize)goto ftdi_sym_error;
ftdi_deinit=(p_ftdi_deinit)(void*)GetProcAddress(ftdi_libhandle, "ftdi_deinit");
if(!ftdi_deinit)goto ftdi_sym_error;
ftdi_read_pins=(p_ftdi_read_pins)(void*)GetProcAddress(ftdi_libhandle, 
                                                          "ftdi_read_pins");
if(!ftdi_read_pins)goto ftdi_sym_error;
ftdi_usb_close=(p_ftdi_usb_close)(void*)GetProcAddress(ftdi_libhandle, 
                                                           "ftdi_usb_close");
if(!ftdi_usb_close)goto ftdi_sym_error;
ftdi_usb_get_strings=(p_ftdi_usb_get_strings)(void*)GetProcAddress(ftdi_libhandle, 
                                                      "ftdi_usb_get_strings");
if(!ftdi_usb_get_strings)goto ftdi_sym_error;
ftdi_usb_open_dev=(p_ftdi_usb_open_dev)(void*)GetProcAddress(ftdi_libhandle, 
                                                        "ftdi_usb_open_dev");
if(!ftdi_usb_open_dev)goto ftdi_sym_error;
ftdi_list_free=(p_ftdi_list_free)(void*)GetProcAddress(ftdi_libhandle, 
                                                           "ftdi_list_free");
if(!ftdi_list_free)goto ftdi_sym_error;
ftdi_library_flag=TRUE;
return 0;
ftdi_sym_error:;
FreeLibrary(ftdi_libhandle);
ftdi_library_flag=FALSE;
ftdi_load_error:;
clear_screen();
library_error_screen(ftdi_dllname,info);
return -1;
}

void unload_ftdi_library(void)
{
FreeLibrary(ftdi_libhandle);
}

int load_usb0_library(int msg_flag)
{
char libusb0_dllname[80];
int info;
if(libusb0_library_flag)return TRUE;
info=0;
sprintf(libusb0_dllname,"%slibusb0.dll",DLLDIR);
libusb0_libhandle=LoadLibraryEx(libusb0_dllname, NULL, 
                                                 LOAD_WITH_ALTERED_SEARCH_PATH);
if(!libusb0_libhandle)goto libusb0_load_error;
info=1;
usb_strerror=(p_usb_strerror)(void*)GetProcAddress(libusb0_libhandle, "usb_strerror");
if(!usb_strerror)goto libusb0_sym_error;
usb_init=(p_usb_init)(void*)GetProcAddress(libusb0_libhandle, "usb_init");
if(!usb_init)goto libusb0_sym_error;
usb_find_busses=(p_usb_find_busses)(void*)GetProcAddress(libusb0_libhandle, "usb_find_busses");
if(!usb_find_busses)goto libusb0_sym_error;
usb_find_devices=(p_usb_find_devices)(void*)GetProcAddress(libusb0_libhandle, "usb_find_devices");
if(!usb_find_devices)goto libusb0_sym_error;
usb_get_busses=(p_usb_get_busses)(void*)GetProcAddress(libusb0_libhandle, "usb_get_busses");
if(!usb_get_busses)goto libusb0_sym_error;
usb_close=(p_usb_close)(void*)GetProcAddress(libusb0_libhandle, "usb_close");
if(!usb_close)goto libusb0_sym_error;
usb_control_msg=(p_usb_control_msg)(void*)GetProcAddress(libusb0_libhandle, "usb_control_msg");
if(!usb_control_msg)goto libusb0_sym_error;
usb_open=(p_usb_open)(void*)GetProcAddress(libusb0_libhandle, "usb_open");
if(!usb_open)goto libusb0_sym_error;
libusb0_library_flag=TRUE;
return TRUE;
libusb0_sym_error:
FreeLibrary(libusb0_libhandle);
libusb0_load_error:
libusb0_library_flag=FALSE;
if(msg_flag)library_error_screen(libusb0_dllname,info);
return FALSE;
}

void unload_usb0_library(void)
{
if(!libusb0_library_flag)return;
FreeLibrary(libusb0_libhandle);
libusb0_library_flag=FALSE;
}

int load_usb1_library(int msg_flag)
{
char libusb1_dllname[80];
int info;
if(libusb1_library_flag)return TRUE;
info=0;
sprintf(libusb1_dllname,"%slibusb-1.0.dll",DLLDIR);
libusb1_libhandle=LoadLibraryEx(libusb1_dllname, NULL, 
                                              LOAD_WITH_ALTERED_SEARCH_PATH);
if(!libusb1_libhandle)goto libusb1_load_error;
info=1;
libusb_init=(p_libusb_init)(void*)GetProcAddress(libusb1_libhandle, "libusb_init");
if(!libusb_init)goto libusb1_sym_error;
libusb_control_transfer=(p_libusb_control_transfer)(void*)GetProcAddress(libusb1_libhandle, "libusb_control_transfer");
if(!libusb_control_transfer)goto libusb1_sym_error;
libusb_get_string_descriptor_ascii=(p_libusb_get_string_descriptor_ascii)(void*)GetProcAddress(libusb1_libhandle, "libusb_get_string_descriptor_ascii");
if(!libusb_get_string_descriptor_ascii)goto libusb1_sym_error;
libusb_close=(p_libusb_close)(void*)GetProcAddress(libusb1_libhandle, "libusb_close");
if(!libusb_close)goto libusb1_sym_error;
libusb_open=(p_libusb_open)(void*)GetProcAddress(libusb1_libhandle, "libusb_open");
if(!libusb_open)goto libusb1_sym_error;
libusb_get_device_list=(p_libusb_get_device_list)(void*)GetProcAddress(libusb1_libhandle, "libusb_get_device_list");
if(!libusb_get_device_list)goto libusb1_sym_error;
libusb_get_device_descriptor=(p_libusb_get_device_descriptor)(void*)GetProcAddress(libusb1_libhandle, "libusb_get_device_descriptor");
if(!libusb_get_device_descriptor)goto libusb1_sym_error;
libusb_free_device_list=(p_libusb_free_device_list)(void*)GetProcAddress(libusb1_libhandle, "libusb_free_device_list");
if(!libusb_free_device_list)goto libusb1_sym_error;
libusb_alloc_transfer=(p_libusb_alloc_transfer)(void*)GetProcAddress(libusb1_libhandle, "libusb_alloc_transfer"); 
if(!libusb_alloc_transfer)goto libusb1_sym_error;
libusb_submit_transfer=(p_libusb_submit_transfer)(void*)GetProcAddress(libusb1_libhandle, "libusb_submit_transfer");
if(!libusb_submit_transfer)goto libusb1_sym_error;
libusb_handle_events_timeout=(p_libusb_handle_events_timeout)(void*)GetProcAddress(libusb1_libhandle, "libusb_handle_events_timeout"); 
if(!libusb_handle_events_timeout)goto libusb1_sym_error;
libusb_free_transfer=(p_libusb_free_transfer)(void*)GetProcAddress(libusb1_libhandle, "libusb_free_transfer"); 
if(!libusb_free_transfer)goto libusb1_sym_error;
libusb_cancel_transfer=(p_libusb_cancel_transfer)(void*)GetProcAddress(libusb1_libhandle, "libusb_cancel_transfer");
if(!libusb_cancel_transfer)goto libusb1_sym_error;
libusb1_library_flag=TRUE;
return TRUE;
libusb1_sym_error:
FreeLibrary(libusb1_libhandle);
libusb1_load_error:
libusb1_library_flag=FALSE;
if(msg_flag)library_error_screen(libusb1_dllname,info);
return FALSE;
}

void unload_usb1_library(void)
{
if(!libusb1_library_flag)return;
FreeLibrary(libusb1_libhandle);
libusb1_library_flag=FALSE;
}
#endif



#if(OSNUM == OSNUM_LINUX)
#include <dlfcn.h>
void *ftdi_libhandle;
void *libusb1_libhandle;
void *libusb0_libhandle;

int load_ftdi_library(void)
{
int info;
info=0;
if(ftdi_library_flag)return 0;
ftdi_libhandle=dlopen(FTDI_LIBNAME, RTLD_NOW|RTLD_GLOBAL);
if(!ftdi_libhandle)goto ftdi_load_error;
info=1;
ftdi_init=(p_ftdi_init)dlsym(ftdi_libhandle, "ftdi_init");
if(dlerror() != 0)goto ftdi_sym_error;
ftdi_write_data=(p_ftdi_write_data)dlsym(ftdi_libhandle, "ftdi_write_data");
if(dlerror() != 0)goto ftdi_sym_error;
ftdi_set_baudrate=(p_ftdi_set_baudrate)dlsym(ftdi_libhandle, 
                                                       "ftdi_set_baudrate");
if(dlerror() != 0)goto ftdi_sym_error;
ftdi_set_bitmode=(p_ftdi_set_bitmode)dlsym(ftdi_libhandle, "ftdi_set_bitmode");
if(dlerror() != 0)goto ftdi_sym_error;
ftdi_read_data=(p_ftdi_read_data)dlsym(ftdi_libhandle, "ftdi_read_data");
if(dlerror() != 0)goto ftdi_sym_error;
ftdi_free=(p_ftdi_free)dlsym(ftdi_libhandle, "ftdi_free");
if(dlerror() != 0)goto ftdi_sym_error;
ftdi_usb_find_all=(p_ftdi_usb_find_all)dlsym(ftdi_libhandle, 
                                                     "ftdi_usb_find_all");
if(dlerror() != 0)goto ftdi_sym_error;
ftdi_get_error_string=(void*)(char*)dlsym(ftdi_libhandle, 
                                                 "ftdi_get_error_string");
if(dlerror() != 0)goto ftdi_sym_error;
ftdi_new=(void*)(struct ftdi_context*)dlsym(ftdi_libhandle, "ftdi_new");
if(dlerror() != 0)goto ftdi_sym_error;
ftdi_usb_open=(p_ftdi_usb_open)dlsym(ftdi_libhandle, "ftdi_usb_open");
if(dlerror() != 0)goto ftdi_sym_error;
ftdi_write_data_set_chunksize=(p_ftdi_write_data_set_chunksize)
                       dlsym(ftdi_libhandle, "ftdi_write_data_set_chunksize");
if(dlerror() != 0)goto ftdi_sym_error;
ftdi_deinit=(p_ftdi_deinit)dlsym(ftdi_libhandle, "ftdi_deinit");
if(dlerror() != 0)goto ftdi_sym_error;
ftdi_read_pins=(p_ftdi_read_pins)dlsym(ftdi_libhandle, "ftdi_read_pins");
if(dlerror() != 0)goto ftdi_sym_error;
ftdi_usb_close=(p_ftdi_usb_close)dlsym(ftdi_libhandle, "ftdi_usb_close");
if(dlerror() != 0)goto ftdi_sym_error;
ftdi_usb_get_strings=(p_ftdi_usb_get_strings)dlsym(ftdi_libhandle, 
                                                      "ftdi_usb_get_strings");
if(dlerror() != 0)goto ftdi_sym_error;
ftdi_usb_open_dev=(p_ftdi_usb_open_dev)dlsym(ftdi_libhandle, 
                                                        "ftdi_usb_open_dev");
if(dlerror() != 0)goto ftdi_sym_error;
ftdi_list_free=(p_ftdi_list_free)dlsym(ftdi_libhandle, "ftdi_list_free");
if(dlerror() != 0)goto ftdi_sym_error;
ftdi_library_flag=TRUE;
return 0;
ftdi_sym_error:;
dlclose(ftdi_libhandle);
ftdi_load_error:;
library_error_screen(FTDI_LIBNAME,info);
return -1;
}

void unload_ftdi_library(void)
{
if(!ftdi_library_flag)return;
dlclose(ftdi_libhandle);
ftdi_library_flag=FALSE;
}

int load_usb1_library(int msg_flag)
{
int info;
info=0;
if(libusb1_library_flag)return TRUE;
libusb1_libhandle=dlopen(LIBUSB1_LIBNAME, RTLD_NOW|RTLD_GLOBAL);
if(!libusb1_libhandle)goto libusb1_load_error;
info=1;
libusb_init=(p_libusb_init)dlsym(libusb1_libhandle, "libusb_init");
if(dlerror() != 0)goto libusb1_sym_error;
libusb_control_transfer=(p_libusb_control_transfer)dlsym(libusb1_libhandle, "libusb_control_transfer");
if(dlerror() != 0)goto libusb1_sym_error;
libusb_get_string_descriptor_ascii=(p_libusb_get_string_descriptor_ascii)dlsym(libusb1_libhandle, "libusb_get_string_descriptor_ascii");
if(dlerror() != 0)goto libusb1_sym_error;
libusb_close=(p_libusb_close)dlsym(libusb1_libhandle, "libusb_close");
if(dlerror() != 0)goto libusb1_sym_error;
libusb_open=(p_libusb_open)dlsym(libusb1_libhandle, "libusb_open");
if(dlerror() != 0)goto libusb1_sym_error;
libusb_get_device_list=(p_libusb_get_device_list)dlsym(libusb1_libhandle, "libusb_get_device_list");
if(dlerror() != 0)goto libusb1_sym_error;
libusb_get_device_descriptor=(p_libusb_get_device_descriptor)dlsym(libusb1_libhandle, "libusb_get_device_descriptor");
if(dlerror() != 0)goto libusb1_sym_error;
libusb_free_device_list=(p_libusb_free_device_list)dlsym(libusb1_libhandle, "libusb_free_device_list");
if(dlerror() != 0)goto libusb1_sym_error;
libusb_alloc_transfer=(p_libusb_alloc_transfer)dlsym(libusb1_libhandle, "libusb_alloc_transfer"); 
if(dlerror() != 0)goto libusb1_sym_error;
libusb_submit_transfer=(p_libusb_submit_transfer)dlsym(libusb1_libhandle, "libusb_submit_transfer");
if(dlerror() != 0)goto libusb1_sym_error;
libusb_handle_events_timeout=(p_libusb_handle_events_timeout)dlsym(libusb1_libhandle, "libusb_handle_events_timeout"); 
if(dlerror() != 0)goto libusb1_sym_error;
libusb_free_transfer=(p_libusb_free_transfer)dlsym(libusb1_libhandle, "libusb_free_transfer"); 
if(dlerror() != 0)goto libusb1_sym_error;
libusb_cancel_transfer=(p_libusb_cancel_transfer)dlsym(libusb1_libhandle, "libusb_cancel_transfer");
if(dlerror() != 0)goto libusb1_sym_error;

libusb1_library_flag=TRUE;
return TRUE;
libusb1_sym_error:
dlclose(libusb1_libhandle);
libusb1_load_error:
libusb1_library_flag=FALSE;
if(msg_flag)library_error_screen(LIBUSB1_LIBNAME,info);
return FALSE;
}

void unload_usb1_library(void)
{
if(!libusb1_library_flag)return;
dlclose(libusb1_libhandle);
libusb1_library_flag=FALSE;
}

int load_usb0_library(int msg_flag)
{
int info;
info=0;
if(libusb0_library_flag)return TRUE;
libusb0_libhandle=dlopen(LIBUSB0_LIBNAME, RTLD_NOW|RTLD_GLOBAL);
if(!libusb0_libhandle)goto libusb0_load_error;
info=1;
usb_strerror=(p_usb_strerror)dlsym(libusb0_libhandle, "usb_strerror");
if(dlerror() != 0)goto libusb0_sym_error;
usb_init=(p_usb_init)dlsym(libusb0_libhandle, "usb_init");
if(dlerror() != 0)goto libusb0_sym_error;
usb_find_busses=(p_usb_find_busses)dlsym(libusb0_libhandle, "usb_find_busses");
if(dlerror() != 0)goto libusb0_sym_error;
usb_find_devices=(p_usb_find_devices)dlsym(libusb0_libhandle, "usb_find_devices");
if(dlerror() != 0)goto libusb0_sym_error;
usb_get_busses=(p_usb_get_busses)dlsym(libusb0_libhandle, "usb_get_busses");
if(dlerror() != 0)goto libusb0_sym_error;
usb_close=(p_usb_close)dlsym(libusb0_libhandle, "usb_close");
if(dlerror() != 0)goto libusb0_sym_error;
usb_control_msg=(p_usb_control_msg)dlsym(libusb0_libhandle, "usb_control_msg");
if(dlerror() != 0)goto libusb0_sym_error;
usb_open=(p_usb_open)dlsym(libusb0_libhandle, "usb_open");
if(dlerror() != 0)goto libusb0_sym_error;
libusb0_library_flag=TRUE;
return TRUE;
libusb0_sym_error:
dlclose(libusb0_libhandle);
libusb0_load_error:
libusb0_library_flag=FALSE;
if(msg_flag)library_error_screen(LIBUSB0_LIBNAME,info);
return FALSE;
}

void unload_usb0_library(void)
{
if(!libusb0_library_flag)return;
dlclose(libusb0_libhandle);
libusb0_library_flag=FALSE;
}
#endif

int  usbGetStringAscii(usb_dev_handle *dev, int my_index, 
                                           int langid, char *buf, int buflen)
{
char    buffer[256];
int     rval, i;
if((rval = usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR, 
   (USB_DT_STRING << 8) + my_index, langid, buffer, sizeof(buffer), 1000)) < 0) return rval;
if(buffer[1] != USB_DT_STRING)  return 0;
if((unsigned char)buffer[0] < rval) rval = (unsigned char)buffer[0];
rval /= 2;
// lossy conversion to ISO Latin1 
for(i=1;i<rval;i++)
  {
  if(i > buflen) break;                       // destination buffer overflow 
  buf[i-1] = buffer[2 * i];
  if(buffer[2 * i + 1] != 0)  buf[i-1] = '?'; // outside of ISO Latin1 range 
  }
buf[i-1] = 0;
return i-1;
}

int select_libusb_version(char* name, char* lib0, char* lib1)
{
int i;
char s[80];
int vernr;
clear_screen();
load_usb1_library(FALSE);
load_usb0_library(FALSE);
settextcolor(14);
sprintf(s,"Library selection for %s",name);
lir_text(10,1,s);
#if OSNUM == OSNUM_WINDOWS
sprintf(s,"Use zadig.exe or zadig_xp.exe to install USB drivers.");
lir_text(10,2,s);
#endif
settextcolor(7);
if(libusb1_library_flag == FALSE && libusb0_library_flag == FALSE)
  {
  lir_text(5,5,"Can not load any of libusb or libusb-1.0");
  lir_text(9,10,press_any_key);
  i=lir_inkey;
  await_processed_keyboard();
  lir_inkey=i; 
  clear_screen();
  return -1;
  }
if(libusb1_library_flag == TRUE && libusb0_library_flag == TRUE)
  {
  lir_text(5,5,"Both libusb and libusb-1.0 are available on this system.");
  lir_text(5,6,"Select which one to use:");
#if OSNUM == OSNUM_WINDOWS
  sprintf(s,"0 = %s for libusb-win32 driver.",lib0);
  lir_text(5,8,s); 
  sprintf(s,"1 = %s for for WinUSB (libusbx) driver.",lib1);
  lir_text(5,9,s);
#endif  
#if OSNUM == OSNUM_LINUX
  sprintf(s,"0 = %s",lib0);
  lir_text(5,8,s); 
  sprintf(s,"1 = %s",lib1);
  lir_text(5,9,s);
#endif  
  lir_text(5,11,"=>");
  vernr=lir_get_integer(8,11,1,0,1);
  }
else
  {
  if(libusb1_library_flag == TRUE)
    {
    lir_text(5,5,"Only libusb-1.0 is available on this system.");
#if OSNUM == OSNUM_WINDOWS
    sprintf(s,"Will use %s for for WinUSB (libusbx) driver.",lib1);
#endif
#if OSNUM == OSNUM_LINUX
    sprintf(s,"Will use %s",lib1);
#endif
    vernr=1;
    }
  else
    {
    lir_text(5,5,"Only libusb is available on this system.");
#if OSNUM == OSNUM_WINDOWS
    sprintf(s,"Will use %s for for libusb-win32 driver.",lib0);
#endif
#if OSNUM == OSNUM_LINUX
    sprintf(s,"Will use %s",lib1);
#endif
    vernr=0;
    }
  lir_text(5,7,s);
  lir_text(10,10,press_any_key);
  i=lir_inkey;
  await_processed_keyboard();
  lir_inkey=i;
  }
if(vernr == 0 && libusb1_library_flag == TRUE)unload_usb1_library();
if(vernr == 1 && libusb0_library_flag == TRUE)unload_usb0_library();
clear_screen();
return vernr;
}
