
extern int dll_version;
extern char *dllversfilename;

void clear_screen(void);
void lir_refresh_entire_screen(void);
void lir_refresh_screen(void);
void lir_line(int x1, int yy1, int x2,int y2, unsigned char c);
void lir_hline(int x1, int y, int x2, unsigned char c);
void lir_putbox(int x, int y, int w, int h, size_t* dp);
void lir_getbox(int x, int y, int w, int h, size_t* dp);
void lir_fillbox(int x, int y,int w, int h, unsigned char c);
void lir_setpixel(int x, int y, unsigned char c);
void lir_getpalettecolor(int j, int *r, int *g, int *b);
