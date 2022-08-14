

typedef struct {
signed int n;
signed int size;
float rest;
float avgpwr;
float avgmax;
} BLANKER_CONTROL_INFO;


#define BLN_INFO_SIZE 7 //6
#define MAX_REFPULSES 256 //128
extern unsigned char chan_color[2];
extern BLANKER_CONTROL_INFO bln[];
extern signed int largest_blnfit;
extern size_t *blanker_handle;
extern signed int timf2_noise_floor;
extern float timf2_despiked_pwr[];
extern float timf2_despiked_pwrinc[2];
extern signed int blanker_pulsewidth;
extern signed int refpul_n;
extern signed int refpul_size;
extern signed int timf2_noise_floor_avgnum;
extern signed int blanker_info_update_counter;
extern signed int blanker_info_update_interval;
extern signed int blnfit_range;
extern signed int blnclear_range;
extern float clever_blanker_rate;
extern float stupid_blanker_rate;

extern float blanker_pol_c1,blanker_pol_c2,blanker_pol_c3;
extern signed int timf2_show_pointer;
extern float blanker_phaserot;
extern float *blanker_refpulse;
extern float *blanker_phasefunc;
extern float *blanker_input;
extern signed int *blanker_pulindex;
extern signed char *blanker_flag;
