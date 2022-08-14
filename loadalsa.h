

#include <string.h>

#ifndef __GNUC__
#define __inline__ inline
#endif

typedef struct _snd_pcm_info snd_pcm_info_t;
typedef struct _snd_pcm_hw_params snd_pcm_hw_params_t;

/** PCM stream (direction) */
typedef enum _snd_pcm_stream {
	/** Playback stream */
	SND_PCM_STREAM_PLAYBACK = 0,
	/** Capture stream */
	SND_PCM_STREAM_CAPTURE,
	SND_PCM_STREAM_LAST = SND_PCM_STREAM_CAPTURE
} snd_pcm_stream_t;

/** PCM access type */
typedef enum _snd_pcm_access {
	/** mmap access with simple interleaved channels */
	SND_PCM_ACCESS_MMAP_INTERLEAVED = 0,
	/** mmap access with simple non interleaved channels */
	SND_PCM_ACCESS_MMAP_NONINTERLEAVED,
	/** mmap access with complex placement */
	SND_PCM_ACCESS_MMAP_COMPLEX,
	/** snd_pcm_readi/snd_pcm_writei access */
	SND_PCM_ACCESS_RW_INTERLEAVED,
	/** snd_pcm_readn/snd_pcm_writen access */
	SND_PCM_ACCESS_RW_NONINTERLEAVED,
	SND_PCM_ACCESS_LAST = SND_PCM_ACCESS_RW_NONINTERLEAVED
} snd_pcm_access_t;


/** PCM sample format */
typedef enum _snd_pcm_format {
	/** Unknown */
	SND_PCM_FORMAT_UNKNOWN = -1,
	/** Signed 8 bit */
	SND_PCM_FORMAT_S8 = 0,
	/** Unsigned 8 bit */
	SND_PCM_FORMAT_U8,
	/** Signed 16 bit Little Endian */
	SND_PCM_FORMAT_S16_LE,
	/** Signed 16 bit Big Endian */
	SND_PCM_FORMAT_S16_BE,
	/** Unsigned 16 bit Little Endian */
	SND_PCM_FORMAT_U16_LE,
	/** Unsigned 16 bit Big Endian */
	SND_PCM_FORMAT_U16_BE,
	/** Signed 24 bit Little Endian using low three bytes in 32-bit word */
	SND_PCM_FORMAT_S24_LE,
	/** Signed 24 bit Big Endian using low three bytes in 32-bit word */
	SND_PCM_FORMAT_S24_BE,
	/** Unsigned 24 bit Little Endian using low three bytes in 32-bit word */
	SND_PCM_FORMAT_U24_LE,
	/** Unsigned 24 bit Big Endian using low three bytes in 32-bit word */
	SND_PCM_FORMAT_U24_BE,
	/** Signed 32 bit Little Endian */
	SND_PCM_FORMAT_S32_LE,
	/** Signed 32 bit Big Endian */
	SND_PCM_FORMAT_S32_BE,
	/** Unsigned 32 bit Little Endian */
	SND_PCM_FORMAT_U32_LE,
	/** Unsigned 32 bit Big Endian */
	SND_PCM_FORMAT_U32_BE,
	/** Float 32 bit Little Endian, Range -1.0 to 1.0 */
	SND_PCM_FORMAT_FLOAT_LE,
	/** Float 32 bit Big Endian, Range -1.0 to 1.0 */
	SND_PCM_FORMAT_FLOAT_BE,
	/** Float 64 bit Little Endian, Range -1.0 to 1.0 */
	SND_PCM_FORMAT_FLOAT64_LE,
	/** Float 64 bit Big Endian, Range -1.0 to 1.0 */
	SND_PCM_FORMAT_FLOAT64_BE,
	/** IEC-958 Little Endian */
	SND_PCM_FORMAT_IEC958_SUBFRAME_LE,
	/** IEC-958 Big Endian */
	SND_PCM_FORMAT_IEC958_SUBFRAME_BE,
	/** Mu-Law */
	SND_PCM_FORMAT_MU_LAW,
	/** A-Law */
	SND_PCM_FORMAT_A_LAW,
	/** Ima-ADPCM */
	SND_PCM_FORMAT_IMA_ADPCM,
	/** MPEG */
	SND_PCM_FORMAT_MPEG,
	/** GSM */
	SND_PCM_FORMAT_GSM,
	/** Special */
	SND_PCM_FORMAT_SPECIAL = 31,
	/** Signed 24bit Little Endian in 3bytes format */
	SND_PCM_FORMAT_S24_3LE = 32,
	/** Signed 24bit Big Endian in 3bytes format */
	SND_PCM_FORMAT_S24_3BE,
	/** Unsigned 24bit Little Endian in 3bytes format */
	SND_PCM_FORMAT_U24_3LE,
	/** Unsigned 24bit Big Endian in 3bytes format */
	SND_PCM_FORMAT_U24_3BE,
	/** Signed 20bit Little Endian in 3bytes format */
	SND_PCM_FORMAT_S20_3LE,
	/** Signed 20bit Big Endian in 3bytes format */
	SND_PCM_FORMAT_S20_3BE,
	/** Unsigned 20bit Little Endian in 3bytes format */
	SND_PCM_FORMAT_U20_3LE,
	/** Unsigned 20bit Big Endian in 3bytes format */
	SND_PCM_FORMAT_U20_3BE,
	/** Signed 18bit Little Endian in 3bytes format */
	SND_PCM_FORMAT_S18_3LE,
	/** Signed 18bit Big Endian in 3bytes format */
	SND_PCM_FORMAT_S18_3BE,
	/** Unsigned 18bit Little Endian in 3bytes format */
	SND_PCM_FORMAT_U18_3LE,
	/** Unsigned 18bit Big Endian in 3bytes format */
	SND_PCM_FORMAT_U18_3BE,
	/* G.723 (ADPCM) 24 kbit/s, 8 samples in 3 bytes */
	SND_PCM_FORMAT_G723_24,
	/* G.723 (ADPCM) 24 kbit/s, 1 sample in 1 byte */
	SND_PCM_FORMAT_G723_24_1B,
	/* G.723 (ADPCM) 40 kbit/s, 8 samples in 3 bytes */
	SND_PCM_FORMAT_G723_40,
	/* G.723 (ADPCM) 40 kbit/s, 1 sample in 1 byte */
	SND_PCM_FORMAT_G723_40_1B,
	/* Direct Stream Digital (DSD) in 1-byte samples (x8) */
	SND_PCM_FORMAT_DSD_U8,
	/* Direct Stream Digital (DSD) in 2-byte samples (x16) */
	SND_PCM_FORMAT_DSD_U16_LE,
	/* Direct Stream Digital (DSD) in 4-byte samples (x32) */
	SND_PCM_FORMAT_DSD_U32_LE,
	/* Direct Stream Digital (DSD) in 2-byte samples (x16) */
	SND_PCM_FORMAT_DSD_U16_BE,
	/* Direct Stream Digital (DSD) in 4-byte samples (x32) */
	SND_PCM_FORMAT_DSD_U32_BE,
	SND_PCM_FORMAT_LAST = SND_PCM_FORMAT_DSD_U32_BE,

#ifndef __BYTE_ORDER
#define __BYTE_ORDER 1234
#endif

#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1234
#endif
#ifndef EBADFD 
#define EBADFD 77
#endif


#if __BYTE_ORDER == __LITTLE_ENDIAN
	/** Signed 16 bit CPU endian */
	SND_PCM_FORMAT_S16 = SND_PCM_FORMAT_S16_LE,
	/** Unsigned 16 bit CPU endian */
	SND_PCM_FORMAT_U16 = SND_PCM_FORMAT_U16_LE,
	/** Signed 24 bit CPU endian */
	SND_PCM_FORMAT_S24 = SND_PCM_FORMAT_S24_LE,
	/** Unsigned 24 bit CPU endian */
	SND_PCM_FORMAT_U24 = SND_PCM_FORMAT_U24_LE,
	/** Signed 32 bit CPU endian */
	SND_PCM_FORMAT_S32 = SND_PCM_FORMAT_S32_LE,
	/** Unsigned 32 bit CPU endian */
	SND_PCM_FORMAT_U32 = SND_PCM_FORMAT_U32_LE,
	/** Float 32 bit CPU endian */
	SND_PCM_FORMAT_FLOAT = SND_PCM_FORMAT_FLOAT_LE,
	/** Float 64 bit CPU endian */
	SND_PCM_FORMAT_FLOAT64 = SND_PCM_FORMAT_FLOAT64_LE,
	/** IEC-958 CPU Endian */
	SND_PCM_FORMAT_IEC958_SUBFRAME = SND_PCM_FORMAT_IEC958_SUBFRAME_LE
#elif __BYTE_ORDER == __BIG_ENDIAN
	/** Signed 16 bit CPU endian */
	SND_PCM_FORMAT_S16 = SND_PCM_FORMAT_S16_BE,
	/** Unsigned 16 bit CPU endian */
	SND_PCM_FORMAT_U16 = SND_PCM_FORMAT_U16_BE,
	/** Signed 24 bit CPU endian */
	SND_PCM_FORMAT_S24 = SND_PCM_FORMAT_S24_BE,
	/** Unsigned 24 bit CPU endian */
	SND_PCM_FORMAT_U24 = SND_PCM_FORMAT_U24_BE,
	/** Signed 32 bit CPU endian */
	SND_PCM_FORMAT_S32 = SND_PCM_FORMAT_S32_BE,
	/** Unsigned 32 bit CPU endian */
	SND_PCM_FORMAT_U32 = SND_PCM_FORMAT_U32_BE,
	/** Float 32 bit CPU endian */
	SND_PCM_FORMAT_FLOAT = SND_PCM_FORMAT_FLOAT_BE,
	/** Float 64 bit CPU endian */
	SND_PCM_FORMAT_FLOAT64 = SND_PCM_FORMAT_FLOAT64_BE,
	/** IEC-958 CPU Endian */
	SND_PCM_FORMAT_IEC958_SUBFRAME = SND_PCM_FORMAT_IEC958_SUBFRAME_BE
#else
#error "Unknown endian"
#endif
} snd_pcm_format_t;

typedef struct _snd_ctl_card_info snd_ctl_card_info_t;
typedef struct _snd_ctl snd_ctl_t;
typedef struct _snd_pcm snd_pcm_t;
typedef unsigned long long snd_pcm_uframes_t;
typedef long long snd_pcm_sframes_t;
size_t snd_pcm_hw_params_sizeof(void);
typedef const char* (*p_snd_strerror)(int errnum);
typedef int (*p_snd_pcm_open)(snd_pcm_t **pcm, const char *name, 
		 snd_pcm_stream_t stream, int mode);
typedef int (*p_snd_pcm_close)(snd_pcm_t *pcm);
typedef int (*p_snd_pcm_hw_params)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
typedef int (*p_snd_pcm_prepare)(snd_pcm_t *pcm);
typedef int (*p_snd_pcm_drop)(snd_pcm_t *pcm);
typedef int (*p_snd_pcm_hwsync)(snd_pcm_t *pcm);
typedef snd_pcm_sframes_t (*p_snd_pcm_avail_update)(snd_pcm_t *pcm);
typedef snd_pcm_sframes_t (*p_snd_pcm_writei)(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size);
typedef snd_pcm_sframes_t (*p_snd_pcm_readi)(snd_pcm_t *pcm, void *buffer, snd_pcm_uframes_t size);
typedef int (*p_snd_pcm_recover)(snd_pcm_t *pcm, int err, int silent);
typedef int (*p_snd_pcm_get_params)(snd_pcm_t *pcm,
                                snd_pcm_uframes_t *buffer_size,
                                   snd_pcm_uframes_t *period_size);
typedef int (*p_snd_pcm_info_malloc)(snd_pcm_info_t **ptr);
typedef void (*p_snd_pcm_info_free)(snd_pcm_info_t *obj);
typedef const char* (*p_snd_pcm_info_get_id)(const snd_pcm_info_t *obj);
typedef const char* (*p_snd_pcm_info_get_name)(const snd_pcm_info_t *obj);
typedef void (*p_snd_pcm_info_set_device)(snd_pcm_info_t *obj, unsigned int val);
typedef void (*p_snd_pcm_info_set_subdevice)(snd_pcm_info_t *obj, unsigned int val);
typedef void (*p_snd_pcm_info_set_stream)(snd_pcm_info_t *obj, snd_pcm_stream_t val);
typedef int (*p_snd_pcm_hw_params_any)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
typedef int (*p_snd_pcm_hw_params_malloc)(snd_pcm_hw_params_t **ptr);
typedef void (*p_snd_pcm_hw_params_free)(snd_pcm_hw_params_t *obj);
typedef int (*p_snd_pcm_hw_params_set_format)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val);
typedef int (*p_snd_pcm_hw_params_get_access)(const snd_pcm_hw_params_t *params, snd_pcm_access_t *_access);
typedef int (*p_snd_pcm_hw_params_set_access)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t _access);
typedef int (*p_snd_pcm_hw_params_test_format)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val);
typedef int (*p_snd_pcm_hw_params_get_channels_min)(const snd_pcm_hw_params_t *params, unsigned int *val);
typedef int (*p_snd_pcm_hw_params_get_channels_max)(const snd_pcm_hw_params_t *params, unsigned int *val);
typedef int (*p_snd_pcm_hw_params_set_channels)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val);
typedef int (*p_snd_pcm_hw_params_get_rate_min)(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
typedef int (*p_snd_pcm_hw_params_get_rate_max)(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
typedef int (*p_snd_pcm_hw_params_set_rate)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val, int dir);
typedef int (*p_snd_pcm_hw_params_set_rate_near)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
typedef int (*p_snd_pcm_hw_params_set_rate_resample)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val);
typedef int (*p_snd_pcm_hw_params_get_period_size)(const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *frames, int *dir);
typedef int (*p_snd_pcm_hw_params_set_period_size)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t val, int dir);
typedef int (*p_snd_pcm_hw_params_set_period_size_near)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val, int *dir);
typedef int (*p_snd_card_next)(int *card);
typedef int (*p_snd_ctl_open)(snd_ctl_t **ctl, const char *name, int mode);
typedef int (*p_snd_ctl_close)(snd_ctl_t *ctl);
typedef int (*p_snd_ctl_card_info)(snd_ctl_t *ctl, snd_ctl_card_info_t *info);
typedef int (*p_snd_ctl_pcm_next_device)(snd_ctl_t *ctl, int *device);
typedef int (*p_snd_ctl_pcm_info)(snd_ctl_t *ctl, snd_pcm_info_t * info);
typedef int (*p_snd_ctl_card_info_malloc)(snd_ctl_card_info_t **ptr);
typedef void (*p_snd_ctl_card_info_free)(snd_ctl_card_info_t *obj);
typedef const char* (*p_snd_ctl_card_info_get_id)(const snd_ctl_card_info_t *obj);
typedef const char* (*p_snd_ctl_card_info_get_name)(const snd_ctl_card_info_t *obj);
typedef const char* (*p_snd_ctl_card_info_get_longname)(const snd_ctl_card_info_t *obj);

extern int alsa_library_flag;
extern p_snd_strerror snd_strerror;
extern p_snd_pcm_open snd_pcm_open;
extern p_snd_pcm_close snd_pcm_close;
extern p_snd_pcm_hw_params snd_pcm_hw_params;
extern p_snd_pcm_prepare snd_pcm_prepare;
extern p_snd_pcm_drop snd_pcm_drop;
extern p_snd_pcm_hwsync snd_pcm_hwsync;
extern p_snd_pcm_avail_update snd_pcm_avail_update;
extern p_snd_pcm_writei snd_pcm_writei;
extern p_snd_pcm_readi snd_pcm_readi;
extern p_snd_pcm_recover snd_pcm_recover;
extern p_snd_pcm_get_params snd_pcm_get_params;
extern p_snd_pcm_info_malloc snd_pcm_info_malloc;
extern p_snd_pcm_info_free snd_pcm_info_free;
extern p_snd_pcm_info_get_id snd_pcm_info_get_id;
extern p_snd_pcm_info_get_name snd_pcm_info_get_name;
extern p_snd_pcm_info_set_device snd_pcm_info_set_device;
extern p_snd_pcm_info_set_subdevice snd_pcm_info_set_subdevice;
extern p_snd_pcm_info_set_stream snd_pcm_info_set_stream;
extern p_snd_pcm_hw_params_any snd_pcm_hw_params_any;
extern p_snd_pcm_hw_params_malloc snd_pcm_hw_params_malloc;
extern p_snd_pcm_hw_params_free snd_pcm_hw_params_free;
extern p_snd_pcm_hw_params_set_format snd_pcm_hw_params_set_format;
extern p_snd_pcm_hw_params_get_access snd_pcm_hw_params_get_access;
extern p_snd_pcm_hw_params_set_access snd_pcm_hw_params_set_access;
extern p_snd_pcm_hw_params_test_format snd_pcm_hw_params_test_format;
extern p_snd_pcm_hw_params_get_channels_min snd_pcm_hw_params_get_channels_min;
extern p_snd_pcm_hw_params_get_channels_max snd_pcm_hw_params_get_channels_max;
extern p_snd_pcm_hw_params_set_channels snd_pcm_hw_params_set_channels;
extern p_snd_pcm_hw_params_get_rate_min snd_pcm_hw_params_get_rate_min;
extern p_snd_pcm_hw_params_get_rate_max snd_pcm_hw_params_get_rate_max;
extern p_snd_pcm_hw_params_set_rate snd_pcm_hw_params_set_rate;
extern p_snd_pcm_hw_params_set_rate_near snd_pcm_hw_params_set_rate_near;
extern p_snd_pcm_hw_params_set_rate_resample snd_pcm_hw_params_set_rate_resample;
extern p_snd_pcm_hw_params_get_period_size snd_pcm_hw_params_get_period_size;
extern p_snd_pcm_hw_params_set_period_size snd_pcm_hw_params_set_period_size;
extern p_snd_pcm_hw_params_set_period_size_near snd_pcm_hw_params_set_period_size_near;
extern p_snd_card_next snd_card_next;
extern p_snd_ctl_open snd_ctl_open;
extern p_snd_ctl_close snd_ctl_close;
extern p_snd_ctl_card_info snd_ctl_card_info;
extern p_snd_ctl_pcm_next_device snd_ctl_pcm_next_device;
extern p_snd_ctl_pcm_info snd_ctl_pcm_info;
extern p_snd_ctl_card_info_malloc snd_ctl_card_info_malloc;
extern p_snd_ctl_card_info_free snd_ctl_card_info_free;
extern p_snd_ctl_card_info_get_id snd_ctl_card_info_get_id;
extern p_snd_ctl_card_info_get_name snd_ctl_card_info_get_name;
extern p_snd_ctl_card_info_get_longname snd_ctl_card_info_get_longname;
