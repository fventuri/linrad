#include <string.h>
#include <dlfcn.h>
#include "osnum.h"
#include "globdef.h"
#include "uidef.h"
#include "loadalsa.h"
int alsa_library_flag;
p_snd_strerror snd_strerror;
p_snd_pcm_open snd_pcm_open;
p_snd_pcm_close snd_pcm_close;
p_snd_pcm_hw_params snd_pcm_hw_params;
p_snd_pcm_prepare snd_pcm_prepare;
p_snd_pcm_drop snd_pcm_drop;
p_snd_pcm_hwsync snd_pcm_hwsync;
p_snd_pcm_avail snd_pcm_avail;
p_snd_pcm_writei snd_pcm_writei;
p_snd_pcm_readi snd_pcm_readi;
p_snd_pcm_recover snd_pcm_recover;
p_snd_pcm_get_params snd_pcm_get_params;
p_snd_pcm_info_malloc snd_pcm_info_malloc;
p_snd_pcm_info_free snd_pcm_info_free;
p_snd_pcm_info_get_id snd_pcm_info_get_id;
p_snd_pcm_info_get_name snd_pcm_info_get_name;
p_snd_pcm_info_set_device snd_pcm_info_set_device;
p_snd_pcm_info_set_subdevice snd_pcm_info_set_subdevice;
p_snd_pcm_info_set_stream snd_pcm_info_set_stream;
p_snd_pcm_hw_params_any snd_pcm_hw_params_any;
p_snd_pcm_hw_params_malloc snd_pcm_hw_params_malloc;
p_snd_pcm_hw_params_free snd_pcm_hw_params_free;
p_snd_pcm_hw_params_set_format snd_pcm_hw_params_set_format;
p_snd_pcm_hw_params_get_access snd_pcm_hw_params_get_access;
p_snd_pcm_hw_params_set_access snd_pcm_hw_params_set_access;
p_snd_pcm_hw_params_test_format snd_pcm_hw_params_test_format;
p_snd_pcm_hw_params_get_channels_min snd_pcm_hw_params_get_channels_min;
p_snd_pcm_hw_params_get_channels_max snd_pcm_hw_params_get_channels_max;
p_snd_pcm_hw_params_set_channels snd_pcm_hw_params_set_channels;
p_snd_pcm_hw_params_get_rate_min snd_pcm_hw_params_get_rate_min;
p_snd_pcm_hw_params_get_rate_max snd_pcm_hw_params_get_rate_max;
p_snd_pcm_hw_params_set_rate snd_pcm_hw_params_set_rate;
p_snd_pcm_hw_params_set_rate_near snd_pcm_hw_params_set_rate_near;
p_snd_pcm_hw_params_set_rate_resample snd_pcm_hw_params_set_rate_resample;
p_snd_pcm_hw_params_get_period_size snd_pcm_hw_params_get_period_size;
p_snd_pcm_hw_params_set_period_size snd_pcm_hw_params_set_period_size;
p_snd_pcm_hw_params_set_period_size_near snd_pcm_hw_params_set_period_size_near;
p_snd_card_next snd_card_next;
p_snd_ctl_open snd_ctl_open;
p_snd_ctl_close snd_ctl_close;
p_snd_ctl_card_info snd_ctl_card_info;
p_snd_ctl_pcm_next_device snd_ctl_pcm_next_device;
p_snd_ctl_pcm_info snd_ctl_pcm_info;
p_snd_ctl_card_info_malloc snd_ctl_card_info_malloc;
p_snd_ctl_card_info_free snd_ctl_card_info_free;
p_snd_ctl_card_info_get_id snd_ctl_card_info_get_id;
p_snd_ctl_card_info_get_name snd_ctl_card_info_get_name;
p_snd_ctl_card_info_get_longname snd_ctl_card_info_get_longname;

void *alsa_libhandle;
snd_pcm_t *alsa_handle[4];


void load_alsa_library(void)
{
int info;
info=0;
if(alsa_library_flag)return;
alsa_libhandle=dlopen(ALSA_LIBNAME, RTLD_LAZY);
if(!alsa_libhandle)goto alsa_load_error;
info=1;
snd_strerror=(p_snd_strerror)dlsym(alsa_libhandle, "snd_strerror");
if(snd_strerror == NULL)goto alsa_sym_error;
snd_pcm_open=(p_snd_pcm_open)dlsym(alsa_libhandle, "snd_pcm_open");
if(snd_pcm_open == NULL)goto alsa_sym_error;
snd_pcm_close=(p_snd_pcm_close)dlsym(alsa_libhandle, "snd_pcm_close");
if(snd_pcm_close == NULL)goto alsa_sym_error;
snd_pcm_hw_params=(p_snd_pcm_hw_params)dlsym(alsa_libhandle, "snd_pcm_hw_params");
if(snd_pcm_hw_params == NULL)goto alsa_sym_error;
snd_pcm_prepare=(p_snd_pcm_prepare)dlsym(alsa_libhandle, "snd_pcm_prepare");
if(snd_pcm_prepare == NULL)goto alsa_sym_error;
snd_pcm_drop=(p_snd_pcm_drop)dlsym(alsa_libhandle, "snd_pcm_drop");
if(snd_pcm_drop == NULL)goto alsa_sym_error;
snd_pcm_hwsync=(p_snd_pcm_hwsync)dlsym(alsa_libhandle, "snd_pcm_hwsync");
if(snd_pcm_hwsync == NULL)goto alsa_sym_error;
snd_pcm_avail=(p_snd_pcm_avail)dlsym(alsa_libhandle, "snd_pcm_avail");
if(snd_pcm_avail == NULL)goto alsa_sym_error;
snd_pcm_writei=(p_snd_pcm_writei)dlsym(alsa_libhandle, "snd_pcm_writei");
if(snd_pcm_writei == NULL)goto alsa_sym_error;
snd_pcm_readi=(p_snd_pcm_readi)dlsym(alsa_libhandle, "snd_pcm_readi");
if(snd_pcm_readi == NULL)goto alsa_sym_error;
snd_pcm_recover=(p_snd_pcm_recover)dlsym(alsa_libhandle, "snd_pcm_recover");
if(snd_pcm_recover == NULL)goto alsa_sym_error;
snd_pcm_get_params=(p_snd_pcm_get_params)dlsym(alsa_libhandle, "snd_pcm_get_params");
if(snd_pcm_get_params == NULL)goto alsa_sym_error;
snd_pcm_info_malloc=(p_snd_pcm_info_malloc)dlsym(alsa_libhandle, "snd_pcm_info_malloc");
if(snd_pcm_info_malloc == NULL)goto alsa_sym_error;
snd_pcm_info_free=(p_snd_pcm_info_free)dlsym(alsa_libhandle, "snd_pcm_info_free");
if(snd_pcm_info_free == NULL)goto alsa_sym_error;
snd_pcm_info_get_id=(p_snd_pcm_info_get_id)dlsym(alsa_libhandle, "snd_pcm_info_get_id");
if(snd_pcm_info_get_id == NULL)goto alsa_sym_error;
snd_pcm_info_get_name=(p_snd_pcm_info_get_name)dlsym(alsa_libhandle, "snd_pcm_info_get_name");
if(snd_pcm_info_get_name == NULL)goto alsa_sym_error;
snd_pcm_info_set_device=(p_snd_pcm_info_set_device)dlsym(alsa_libhandle, "snd_pcm_info_set_device");
if(snd_pcm_info_set_device == NULL)goto alsa_sym_error;
snd_pcm_info_set_subdevice=(p_snd_pcm_info_set_subdevice)dlsym(alsa_libhandle, "snd_pcm_info_set_subdevice");
if(snd_pcm_info_set_subdevice == NULL)goto alsa_sym_error;
snd_pcm_info_set_stream=(p_snd_pcm_info_set_stream)dlsym(alsa_libhandle, "snd_pcm_info_set_stream");
if(snd_pcm_info_set_stream == NULL)goto alsa_sym_error;
snd_pcm_hw_params_any=(p_snd_pcm_hw_params_any)dlsym(alsa_libhandle, "snd_pcm_hw_params_any");
if(snd_pcm_hw_params_any == NULL)goto alsa_sym_error;
snd_pcm_hw_params_malloc=(p_snd_pcm_hw_params_malloc)dlsym(alsa_libhandle, "snd_pcm_hw_params_malloc");
if(snd_pcm_hw_params_malloc == NULL)goto alsa_sym_error;
snd_pcm_hw_params_free=(p_snd_pcm_hw_params_free)dlsym(alsa_libhandle, "snd_pcm_hw_params_free");
if(snd_pcm_hw_params_free == NULL)goto alsa_sym_error;
snd_pcm_hw_params_set_format=(p_snd_pcm_hw_params_set_format)dlsym(alsa_libhandle, "snd_pcm_hw_params_set_format");
if(snd_pcm_hw_params_set_format == NULL)goto alsa_sym_error;
snd_pcm_hw_params_get_access=(p_snd_pcm_hw_params_get_access)dlsym(alsa_libhandle, "snd_pcm_hw_params_get_access");
if(snd_pcm_hw_params_get_access == NULL)goto alsa_sym_error;
snd_pcm_hw_params_set_access=(p_snd_pcm_hw_params_set_access)dlsym(alsa_libhandle, "snd_pcm_hw_params_set_access");
if(snd_pcm_hw_params_set_access == NULL)goto alsa_sym_error;
snd_pcm_hw_params_test_format=(p_snd_pcm_hw_params_test_format)dlsym(alsa_libhandle, "snd_pcm_hw_params_test_format");
if(snd_pcm_hw_params_test_format == NULL)goto alsa_sym_error;
snd_pcm_hw_params_get_channels_min=(p_snd_pcm_hw_params_get_channels_min)dlsym(alsa_libhandle, "snd_pcm_hw_params_get_channels_min");
if(snd_pcm_hw_params_get_channels_min == NULL)goto alsa_sym_error;
snd_pcm_hw_params_get_channels_max=(p_snd_pcm_hw_params_get_channels_max)dlsym(alsa_libhandle, "snd_pcm_hw_params_get_channels_max");
if(snd_pcm_hw_params_get_channels_max == NULL)goto alsa_sym_error;
snd_pcm_hw_params_set_channels=(p_snd_pcm_hw_params_set_channels)dlsym(alsa_libhandle, "snd_pcm_hw_params_set_channels");
if(snd_pcm_hw_params_set_channels == NULL)goto alsa_sym_error;
snd_pcm_hw_params_get_rate_min=(p_snd_pcm_hw_params_get_rate_min)dlsym(alsa_libhandle, "snd_pcm_hw_params_get_rate_min");
if(snd_pcm_hw_params_get_rate_min == NULL)goto alsa_sym_error;
snd_pcm_hw_params_get_rate_max=(p_snd_pcm_hw_params_get_rate_max)dlsym(alsa_libhandle, "snd_pcm_hw_params_get_rate_max");
if(snd_pcm_hw_params_get_rate_max == NULL)goto alsa_sym_error;
snd_pcm_hw_params_set_rate=(p_snd_pcm_hw_params_set_rate)dlsym(alsa_libhandle, "snd_pcm_hw_params_set_rate");
if(snd_pcm_hw_params_set_rate == NULL)goto alsa_sym_error;
snd_pcm_hw_params_set_rate_near=(p_snd_pcm_hw_params_set_rate_near)dlsym(alsa_libhandle, "snd_pcm_hw_params_set_rate_near");
if(snd_pcm_hw_params_set_rate_near == NULL)goto alsa_sym_error;
snd_pcm_hw_params_set_rate_resample=(p_snd_pcm_hw_params_set_rate_resample)dlsym(alsa_libhandle, "snd_pcm_hw_params_set_rate_resample");
if(snd_pcm_hw_params_set_rate_resample == NULL)goto alsa_sym_error;
snd_pcm_hw_params_get_period_size=(p_snd_pcm_hw_params_get_period_size)dlsym(alsa_libhandle, "snd_pcm_hw_params_get_period_size");
if(snd_pcm_hw_params_get_period_size == NULL)goto alsa_sym_error;
snd_pcm_hw_params_set_period_size=(p_snd_pcm_hw_params_set_period_size)dlsym(alsa_libhandle, "snd_pcm_hw_params_set_period_size");
if(snd_pcm_hw_params_set_period_size == NULL)goto alsa_sym_error;
snd_pcm_hw_params_set_period_size_near=(p_snd_pcm_hw_params_set_period_size_near)dlsym(alsa_libhandle, "snd_pcm_hw_params_set_period_size_near");
if(snd_pcm_hw_params_set_period_size_near == NULL)goto alsa_sym_error;
snd_card_next=(p_snd_card_next)dlsym(alsa_libhandle, "snd_card_next");
if(snd_card_next == NULL)goto alsa_sym_error;
snd_ctl_open=(p_snd_ctl_open)dlsym(alsa_libhandle, "snd_ctl_open");
if(snd_ctl_open == NULL)goto alsa_sym_error;
snd_ctl_close=(p_snd_ctl_close)dlsym(alsa_libhandle, "snd_ctl_close");
if(snd_ctl_close == NULL)goto alsa_sym_error;
snd_ctl_card_info=(p_snd_ctl_card_info)dlsym(alsa_libhandle, "snd_ctl_card_info");
if(snd_ctl_card_info == NULL)goto alsa_sym_error;
snd_ctl_pcm_next_device=(p_snd_ctl_pcm_next_device)dlsym(alsa_libhandle, "snd_ctl_pcm_next_device");
if(snd_ctl_pcm_next_device == NULL)goto alsa_sym_error;
snd_ctl_pcm_info=(p_snd_ctl_pcm_info)dlsym(alsa_libhandle, "snd_ctl_pcm_info");
if(snd_ctl_pcm_info == NULL)goto alsa_sym_error;
snd_ctl_card_info_malloc=(p_snd_ctl_card_info_malloc)dlsym(alsa_libhandle, "snd_ctl_card_info_malloc");
if(snd_ctl_card_info_malloc == NULL)goto alsa_sym_error;
snd_ctl_card_info_free=(p_snd_ctl_card_info_free)dlsym(alsa_libhandle, "snd_ctl_card_info_free");
if(snd_ctl_card_info_free == NULL)goto alsa_sym_error;
snd_ctl_card_info_get_id=(p_snd_ctl_card_info_get_id)dlsym(alsa_libhandle, "snd_ctl_card_info_get_id");
if(snd_ctl_card_info_get_id == NULL)goto alsa_sym_error;
snd_ctl_card_info_get_name=(p_snd_ctl_card_info_get_name)
                        dlsym(alsa_libhandle, "snd_ctl_card_info_get_name");
if(snd_ctl_card_info_get_name == NULL)goto alsa_sym_error;
snd_ctl_card_info_get_longname=(p_snd_ctl_card_info_get_longname)
                        dlsym(alsa_libhandle, "snd_ctl_card_info_get_longname");
if(snd_ctl_card_info_get_longname == NULL)goto alsa_sym_error;
alsa_library_flag=TRUE;
return;
alsa_sym_error:;
dlclose(alsa_libhandle);
alsa_load_error:;
library_error_screen(ALSA_LIBNAME,info);
}

void unload_alsa_library(void)
{
if(!alsa_library_flag)return;
dlclose(alsa_libhandle);
alsa_library_flag=FALSE;
}

