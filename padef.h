

#define MAX_PA_DEVICES 160

extern int portaudio_active_flag;
extern char *sndtype[4];

void set_portaudio(int sound_type, int *line);
void close_portaudio_rxda(void);
void open_portaudio_rxda(void);
void close_portaudio_rxad(void);
void open_portaudio_rxad(void);
void close_portaudio_txda(void);
void open_portaudio_txda(void);
void close_portaudio_txad(void);
void open_portaudio_txad(void);
int pa_get_device_info (int n,
                        int sound_type,
                        void *pa_device_name,
                        void *pa_device_hostapi,
			double *pa_device_max_speed,
			double *pa_device_min_speed,
			int *pa_device_max_bytes,
			int *pa_device_min_bytes,
			int *pa_device_max_channels,
			int *pa_device_min_channels );
int pa_get_valid_samplerate(int n, int mode, int *line,unsigned int *new_sample_rate);


