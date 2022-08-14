
#define CLOUDIQ_SAMPLING_CLOCK 122880000.0
#define SDRIP_SAMPLING_CLOCK 80000000.0
#define SDR14_SAMPLING_CLOCK 66.66666666
#define PERSEUS_SAMPLING_CLOCK 80000000.0
#define EXCALIBUR_SAMPLING_CLOCK 100000000.0

// Structure for Airspy parameters
typedef struct {
int ratenum;
unsigned int sernum11;
unsigned int sernum12;
unsigned int sernum21;
unsigned int sernum22;
int freq_adjust;
unsigned int gain_mode;
unsigned int bandwidth_no;
int bias_t;
int channels;
int real_mode;
int check;
}P_AIRSPY;  
#define MAX_AIRSPY_PARM 12
extern P_AIRSPY airspy;

// Structure for Airspy HF+ parameters
typedef struct {
int ratenum;
unsigned int sernum1;
unsigned int sernum2;
int freq_adjust;
int dsp;
int hf_agc;
int hf_agc_threshold;
int hf_att;
int hf_lna;
int buffers_to_skip;
int fft_integration;
int fft_overlap;
int correlation_integration;
int check;
}P_AIRSPYHF;
#define MAX_AIRSPYHF_PARM 14
extern P_AIRSPYHF airspyhf;

extern float airspyhf_old_point;


// Structure for SDR-14 parameters
typedef struct {
int m_cic2;
int m_cic5;
int m_rcf;
int ol_rcf;
int clock_adjust;
int att;
int input;
int sernum1;
int sernum2;
int sernum3;
int device_code;
int check;
}P_SDR14;
#define MAX_SDR14_PARM 12
extern P_SDR14 sdr14;

// Structure for RTL2832 parameters
typedef struct {
int sampling_speed;
int sernum1;
int sernum2;
int sernum3;
int gain_mode;
int freq_adjust;
int direct_sampling;
int bandwidth;
int check;
}P_RTL2832;
#define MAX_RTL2832_PARM 9
extern P_RTL2832 rtl2832;

extern int no_of_excalibur_rates;
extern char *excalibur_sernum_filename;

extern double adjusted_sdr_clock;
extern char *sdr14_name_string;
extern char *perseus_name_string;
extern char *sdriq_name_string;
extern char *sdrip_name_string;
extern char *excalibur_name_string;
extern char *rtl2832_name_string;
extern char *mirics_name_string;
extern char *sdrplay2_name_string;
extern char *sdrplay3_name_string;
extern char *bladerf_name_string;
extern char *pcie9842_name_string;
extern char *openhpsdr_name_string;
extern char *netafedri_name_string;
extern char *cloudiq_name_string;
extern char *fdms1_name_string;
extern char *airspy_name_string;
extern char *airspyhf_name_string;


// Structure for Excalibur parameters
typedef struct {
int rate_no;
int sampling_rate;
int clock_adjust;
int mw_filter_maxfreq;
int dither;
int check;
}P_EXCALIBUR;
#define MAX_EXCALIBUR_PARM 6
extern P_EXCALIBUR excalib;
#define MAX_EXCALIBUR_DEVICES 16


typedef struct {
int parport;
int parport_pin;
int libusb_version;
int rx2500_fqcorr;
int rx10700_fqcorr[4];
int rx70_fqcorr[5];
int rx144_fqcorr[4];
int rxhfa_fqcorr[5];
int tx2500_fqcorr;
int tx10700_fqcorr[4];
int tx70_fqcorr[5];
int tx144_fqcorr[4];
int txhfa_fqcorr[5];
}WSE_PARM;
#define MAX_WSE_PARM 41

extern WSE_PARM wse;

extern char *pkgsize[2];
extern char *onoff[2];
extern char *lowhigh[2];
extern char *adgains[2];
extern char *inp_connect[2];
extern char *rffil[2];



extern int *rtl2832_gains;
extern int no_of_rtl2832_gains;
extern int old_rtl2832_gain;
extern int *mirics_gains;
extern int no_of_mirics_gains;
extern int old_mirics_gain;

void close_sdr14(void);
void close_perseus(void);
void init_sdr14(void);
void init_perseus(void);
void init_sdrip(void);
void init_cloudiq(void);
void init_excalibur(void);
int display_perseus_parm_info(int *line);
int display_sdr14_parm_info(int *line);
int display_sdrip_parm_info(int *line);
int display_excalibur_parm_info(int *line);
int display_rtl2832_parm_info(int *line);
int display_mirics_parm_info(int *line);
int display_sdrplay2_parm_info(int *line);
int display_sdrplay3_parm_info(int *line);
int display_bladerf_parm_info(int *line);
int display_pcie9842_parm_info(int *line);
int display_openhpsdr_parm_info(int *line);
int display_netafedri_parm_info(int *line);
int display_cloudiq_parm_info(int *line);
int display_fdms1_parm_info(int *line);
void sdr_target_name(char *ss);
void sdr_target_serial_number(char *ss);
void init_rtl2832(void);
void init_mirics(void);
void init_sdrplay2(void);
void init_sdrplay3(void);
void init_bladerf(void);
void init_openhpsdr(void);
void init_pcie9842(void);
void open_afedriusb_control(void);
void close_afedriusb_control(void);
void init_airspy(void);
int display_airspy_parm_info(int *line);
void init_airspyhf(void);
int display_airspyhf_parm_info(int *line);

