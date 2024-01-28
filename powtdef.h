

#define RX_POWTIM_TRIGGER 340
#define RX_POWTIM_SHOW 350
#define POWTIM_TRIGMODES 4

extern float powtim_powfac;
extern float powtim_gain;
extern float powtim_fgain;
extern int powtim_xstep;
extern int powtim_trigmode;
extern int powtim_displaymode;
extern unsigned int powtim_y0;
extern unsigned int powtim_y1;
extern unsigned int powtim_y2;
extern int powtim_scalepix;
extern int powtim_pause_flag;
extern int adtest_channel;
extern int adtest_iq_mode;
extern double adtest_scale;
extern int adtest_new;

void powtim_screen(void);
void powtim_parmwrite(void);
