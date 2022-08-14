
#include "osnum.h"
#if HAVE_SVGALIB == 1
  #include <vga.h>
  #include <vgagl.h>
  #include <vgamouse.h>
  #include <sys/ioctl.h>
  #endif
#include <ctype.h>
#include <unistd.h>
#include <sys/mman.h>
#include <termios.h>
#include <sys/resource.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>
#include <fcntl.h>
#ifndef __FreeBSD__ 
#if DARWIN == 0
#include <linux/fb.h>
#endif
#endif
#include <sys/mman.h>
#include <locale.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdarg.h>     
#if HAVE_OSS == 1
  #if OSSD == 1
  #include <sys/soundcard.h>
  #else
  #include <soundcard.h>
  #endif
#include <sys/ioctl.h>
#endif

#include "globdef.h"
#include "thrdef.h"
#include "uidef.h"
#include "screendef.h"
#include "xdef.h"
#include "keyboard_def.h"
#include "vernr.h"
#include "sdrdef.h"
#include "options.h"
#include "lscreen.h"

//  X11
#if HAVE_X11 == 1
extern GC xgc;
extern XImage *ximage;
extern Display *xdis;
extern Window xwin;
extern Colormap lir_colormap;
#endif
int framebuffer_handle;
char *framebuffer_pointer;
size_t framebuffer_screensize;
int first_mempix_copy;
int last_mempix_copy;
char first_mempix_flag;
char last_mempix_flag;


// FBDEV
typedef struct {
unsigned short int red;
unsigned short int green;
unsigned short int blue;
unsigned int pixel;
short int flag;
float total;
}PIXINFO;

struct termios termios_pp;
int terminal_flag;
int gpm_fd;
struct fb_cmap *fb_palette;

// X11


//||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void xlir_fix_bug(int bug)
{
#if HAVE_X11 == 1
XEvent ev;
switch (bug)
  {
  case 0:
  return;
  
  case 1:
  ev.type = Expose;
  ev.xexpose.display = xdis;
  ev.xexpose.window = xwin;
  ev.xexpose.send_event = FALSE;
  ev.xexpose.count=256;
  ev.xexpose.x = 16;
  ev.xexpose.y = 16;
  ev.xexpose.width = 16;
  ev.xexpose.height = 16;
  ev.xexpose.serial = 0;
  if(screen_type != SCREEN_TYPE_X11_SHM)
    {
    XSendEvent(xdis, xwin, FALSE, ExposureMask, &ev);
    }
#if HAVE_SHM == 1
  else
    {
    XSendEvent(xdis, xwin, FALSE, ExposureMask, &ev);
    }
#endif
  return;

  default:
  return;
  }
#else
(void)bug;
#endif
}

void lir_set_title(char *s)
{
#if HAVE_X11 == 1
if(!(screen_type & 
     (SCREEN_TYPE_X11+SCREEN_TYPE_X11_SHM+SCREEN_TYPE_WINDOWS)))return;
char *window_name;
char wn[80];
XTextProperty window_title_property;
// Set the name of our window
// This is the string to be translated into a property. 
// translate the given string into an X property.
#if IA64 == 0
sprintf(wn," 32bit");
#else
sprintf(wn," 64bit");
#endif
if(screen_type != SCREEN_TYPE_X11_SHM)
  {
  sprintf(&wn[6]," %s %s",PROGRAM_NAME,s);
  }
else
  {
  sprintf(&wn[6]," MIT SHM %s %s",PROGRAM_NAME,s);
  }
window_name=wn;
XStringListToTextProperty(&window_name, 1, &window_title_property);
XSetWMName(xdis, xwin, &window_title_property);
XFree(window_title_property.value);
#else
(void)s;
#endif
}

void xlir_fillbox(int x, int y,int w, int h, unsigned char c)
{
int i, j, k;
unsigned short int *ipalette;
k=(x+y*screen_width);
if(k<0 || k+(h-1)*screen_width+w > screen_totpix)
  {
  fillbox_error(x,y,w,h);
  return;
  }
if(first_mempix > k)
  {
  first_mempix_copy=k;
  first_mempix=k;
  first_mempix_flag=1;
  }
switch (color_depth)
  {
  case 24:
  for(j=0; j<h; j++)
    {
    for(i=0; i<w; i++)
      {
      mempix_char[4*(k+i)  ]=svga_palette[3*c  ];
      mempix_char[4*(k+i)+1]=svga_palette[3*c+1];
      mempix_char[4*(k+i)+2]=svga_palette[3*c+2];
      }
    k+=screen_width;
    }
  break;
      
  case 16:
  ipalette=(unsigned short int*)(&svga_palette[0]);
  for(j=0; j<h; j++)
    {
    for(i=0; i<w; i++)
      {
      mempix_shi[k+i]=ipalette[c];
      }
    k+=screen_width;
    }
  break;

  case 8:
  for(j=0; j<h; j++)
    {
    for(i=0; i<w; i++)
      {
      mempix_char[k+i]=xpalette[c];
      }
    k+=screen_width;
    }
  break;
  }    
k-=screen_width-w;
if(last_mempix < k)
  {
  last_mempix_copy=k;
  last_mempix=k;
  last_mempix_flag=1;
  }
}

void xlir_getbox(int x, int y, int w, int h, size_t* dp)
{
unsigned char *savmem;
int i, j, k, m;
unsigned char c,c1,c2,c3;
unsigned short int *ipalette;
k=x+y*screen_width;
savmem=(unsigned char*)dp;
m=0;
c=0;
c1=0;
c2=0;
switch (color_depth)
  {
  case 24:
  for(j=0; j<h; j++)
    {
    for(i=0; i<w; i++)
      {
      if(svga_palette[3*c  ] == mempix_char[4*(k+i)  ] &&
         svga_palette[3*c+1] == mempix_char[4*(k+i)+1] &&
         svga_palette[3*c+2] == mempix_char[4*(k+i)+2])goto store_24;
      if(svga_palette[3*c1  ] == mempix_char[4*(k+i)  ] &&
         svga_palette[3*c1+1] == mempix_char[4*(k+i)+1] &&
         svga_palette[3*c1+2] == mempix_char[4*(k+i)+2])
        {
        c3=c;
        c=c1;
        c1=c;
        goto store_24;
        }
      if(svga_palette[3*c2  ] == mempix_char[4*(k+i)  ] &&
         svga_palette[3*c2+1] == mempix_char[4*(k+i)+1] &&
         svga_palette[3*c2+2] == mempix_char[4*(k+i)+2])
        {
        c3=c;
        c=c2;
        c1=c;
        c2=c3;
        goto store_24;
        }
      c2=c1;
      c1=c;
      while(svga_palette[3*c  ] != mempix_char[4*(k+i)  ] ||
            svga_palette[3*c+1] != mempix_char[4*(k+i)+1] ||
            svga_palette[3*c+2] != mempix_char[4*(k+i)+2])
        {
        c++;
        if(c>=MAX_SVGA_PALETTE)c=0;
        if(c==c1)
          {
          lirerr(1248);
          return;
          }
        }
store_24:;
      savmem[m]=c;
      m++;
      }
    k+=screen_width;
    }
  break;

  case 16:
  ipalette=(unsigned short int*)(&svga_palette[0]);
  for(j=0; j<h; j++)
    {
    for(i=0; i<w; i++)
      {
      if(ipalette[c] == mempix_shi[(k+i)]) goto store_16;
      if(ipalette[c1] == mempix_shi[(k+i)])
        {
        c3=c;
        c=c1;
        c1=c;
        goto store_16;
        }
      if(ipalette[c2] == mempix_shi[(k+i)])
        {
        c3=c;
        c=c2;
        c1=c;
        c2=c3;
        goto store_16;
        }
      c2=c1;
      c1=c;
      while(ipalette[c] != mempix_shi[(k+i)])
        {
        c++;
        if(c>=MAX_SVGA_PALETTE)c=0;
        if(c==c1)
          {
          lirerr(1248);
          return;
          }
        }
store_16:;
      savmem[m]=c;
      m++;
      }
    k+=screen_width;
    }
  break;

  case 8:
  for(j=0; j<h; j++)
    {
    for(i=0; i<w; i++)
      {
      if(xpalette[c] == mempix_char[(k+i)]) goto store_8;
      if(xpalette[c1] == mempix_char[(k+i)])
        {
        c3=c;
        c=c1;
        c1=c;
        goto store_8;
        }
      if(xpalette[c2] == mempix_char[(k+i)])
        {
        c3=c;
        c=c2;
        c1=c;
        c2=c3;
        goto store_8;
        }
      c2=c1;
      c1=c;
      while(xpalette[c] != mempix_char[(k+i)])
        {
        c++;
        if(c>=MAX_SVGA_PALETTE)c=0;
        if(c==c1)
          {
          lirerr(1248);
          return;
          }
        }
store_8:;
      savmem[m]=c;
      m++;
      }
    k+=screen_width;
    }
  break;
  }
}

void xlir_putbox(int x, int y, int w, int h, size_t* dp)
{
unsigned char *savmem;
int i, j, k, m;
unsigned short int *ipalette;
k=(x+y*screen_width);
if(k<0 || k+(h-1)*screen_width+w-1 >= screen_totpix)
  {
  putbox_error(x,y,w,h);
  return;
  }
if(first_mempix > k)
  {
  first_mempix_copy=k;
  first_mempix=k;
  first_mempix_flag=1;
  }
m=0;
savmem=(unsigned char*)dp;
switch (color_depth)
  {
  case 24:
  for(j=0; j<h; j++)
    {  
    for(i=0; i<w; i++)
      {
      mempix_char[4*(k+i)  ]=svga_palette[3*savmem[m]  ];
      mempix_char[4*(k+i)+1]=svga_palette[3*savmem[m]+1];
      mempix_char[4*(k+i)+2]=svga_palette[3*savmem[m]+2];
      m++;
      }
    k+=screen_width;
    }
  break;
  
  case 16:
  ipalette=(unsigned short int*)(&svga_palette[0]);
  for(j=0; j<h; j++)
    {  
    for(i=0; i<w; i++)
      {
      mempix_shi[k+i]=ipalette[savmem[m]];
      m++;
      }
    k+=screen_width;
    }
  break;

  case 8:
  for(j=0; j<h; j++)
    {  
    for(i=0; i<w; i++)
      {
      mempix_char[k+i]=xpalette[savmem[m]];
      m++;
      }
    k+=screen_width;
    }
  }  
if(last_mempix < k)
  {
  last_mempix_copy=k;
  last_mempix=k;
  last_mempix_flag=1;
  }
}

void xlir_hline(int x1, int y, int x2, unsigned char c)
{
int i, ia, ib;
unsigned short int *ipalette;
ia=y*screen_width;
if(x1 <= x2)
  {
  ib=ia+x2;
  ia+=x1;
  }
else
  {
  ib=ia+x1;
  ia+=x2;
  }
if(ia < 0 || ib >= screen_totpix)
  {
  h_line_error(x1, y, x2);
  return;
  }
switch (color_depth)
  {
  case 24:
  for(i=ia; i<=ib; i++)
    {
    mempix_char[4*i  ]=svga_palette[3*c  ];
    mempix_char[4*i+1]=svga_palette[3*c+1];
    mempix_char[4*i+2]=svga_palette[3*c+2];
    }
  break;
  
  case 16:
  ipalette=(unsigned short int*)(&svga_palette[0]);
  for(i=ia; i<=ib; i++)
    {
    mempix_shi[i]=ipalette[c];
    }
  break;

  case 8:
  for(i=ia; i<=ib; i++)
    {
    mempix_char[i]=xpalette[c];
    }
  break;
  }
if(first_mempix > ia)
  {
  first_mempix_copy=ia;
  first_mempix=ia;
  first_mempix_flag=1;
  }
if(last_mempix < ib)
  {
  last_mempix_copy=ib;
  last_mempix=ib;
  last_mempix_flag=1;
  }
}

void xlir_line(int x1, int yy1, int x2,int y2, unsigned char c)
{
int ia;
int i,j,k;
int xd, yd;
float t1,t2,delt;
unsigned short int *ipalette;
if(x1 < 0)x1=0;
if(x1 >= screen_width)x1=screen_width-1;
if(x2 < 0)x2=0;
if(x2 >= screen_width)x2=screen_width-1;
if(yy1 < 0)yy1=0;
if(yy1 >= screen_height)yy1=screen_height-1;
if(y2 < 0)y2=0;
if(y2 >= screen_height)y2=screen_height-1;
xd=x2-x1;
yd=y2-yy1;
if(yd==0)
  {
  if(xd==0)
    {
    lir_setpixel(x1,yy1,c);
    }
  else
    {
    lir_hline(x1,yy1,x2,c);
    }
  return;
  }
if(abs(xd)>=abs(yd))
  {
  if(xd>=0)
    {
    k=1;
    }
  else
    {
    k=-1;
    }  
  if(yd >= 0)
    {
    delt=0.5;
    }
  else
    {  
    delt=-0.5;
    }
  t1=yy1;
  t2=(float)(yd)/abs(xd);
  i=x1-k;
  switch (color_depth)
    {
    case 24:
    while(i!=x2)
      {
      i+=k;
      j=t1+delt;
      ia=i+j*(screen_width);
      mempix_char[4*ia  ]=svga_palette[3*c  ];
      mempix_char[4*ia+1]=svga_palette[3*c+1];
      mempix_char[4*ia+2]=svga_palette[3*c+2];
      if(first_mempix > ia)
        {
        first_mempix_copy=ia;
        first_mempix=ia;
        first_mempix_flag=1;
        }
      if(last_mempix < ia)
        {
        last_mempix_copy=ia;
        last_mempix=ia;
        last_mempix_flag=1;
        }
      t1+=t2;
      }
    break;
    
    case 16:
    ipalette=(unsigned short int*)(&svga_palette[0]);
    while(i!=x2)
      {
      i+=k;
      j=t1+delt;
      ia=i+j*(screen_width);
      mempix_shi[ia  ]=ipalette[c  ];
      if(first_mempix > ia)
        {
        first_mempix_copy=ia;
        first_mempix=ia;
        first_mempix_flag=1;
        }
      if(last_mempix < ia)
        {
        last_mempix_copy=ia;
        last_mempix=ia;
        last_mempix_flag=1;
        }
      t1+=t2;
      }
    break;
    
    case 8:
    while(i!=x2)
      {
      i+=k;
      j=t1+delt;
      ia=i+j*(screen_width);
      mempix_char[ia  ]=xpalette[c  ];
      if(first_mempix > ia)
        {
        first_mempix_copy=ia;
        first_mempix=ia;
        first_mempix_flag=1;
        }
      if(last_mempix < ia)
        {
        last_mempix_copy=ia;
        last_mempix=ia;
        last_mempix_flag=1;
        }
      t1+=t2;
      }
    break;
        
    }  
  }  
else
  {
  if(yd>=0)
    {
    k=1;
    }
  else
    {
    k=-1;
    } 
  if(xd >= 0)
    {
    delt=0.5;
    }
  else
    {  
    delt=-0.5;
    }
  t1=x1;
  t2=(float)(xd)/abs(yd);
  i=yy1-k;
  switch (color_depth)
    {
    case 24:
    while(i!=y2)
      {
      i+=k;
      j=t1+delt;
      ia=j+i*(screen_width);
      mempix_char[4*ia  ]=svga_palette[3*c  ];
      mempix_char[4*ia+1]=svga_palette[3*c+1];
      mempix_char[4*ia+2]=svga_palette[3*c+2];
      if(first_mempix > ia)
        {
        first_mempix_copy=ia;
        first_mempix=ia;
        first_mempix_flag=1;
        }
      if(last_mempix < ia)
        {
        last_mempix_copy=ia;
        last_mempix=ia;
        last_mempix_flag=1;
        }
      t1+=t2;
      }
    break;
    
    case 16:
    ipalette=(unsigned short int*)(&svga_palette[0]);
    while(i!=y2)
      {
      i+=k;
      j=t1+delt;
      ia=j+i*(screen_width);
      mempix_shi[ia  ]=ipalette[c  ];
      if(first_mempix > ia)
        {
        first_mempix_copy=ia;
        first_mempix=ia;
        first_mempix_flag=1;
        }
      if(last_mempix < ia)
        {
        last_mempix_copy=ia;
        last_mempix=ia;
        last_mempix_flag=1;
        }
      t1+=t2;
      }
    break;

    case 8:
    while(i!=y2)
      {
      i+=k;
      j=t1+delt;
      ia=j+i*(screen_width);
      mempix_char[ia  ]=xpalette[c  ];
      if(first_mempix > ia)
        {
        first_mempix_copy=ia;
        first_mempix=ia;
        first_mempix_flag=1;
        }
      if(last_mempix < ia)
        {
        last_mempix_copy=ia;
        last_mempix=ia;
        last_mempix_flag=1;
        }
      t1+=t2;
      }
    break;
    }    
  }  
}

void xclear_screen(void)
{
int i, k;
switch (color_depth)
  {
  case 24:
  k=4*screen_width*screen_height;
  for(i=0; i<k; i+=4)
    {
    mempix_char[i]=svga_palette[3*CLRSCR_COLOR];
    mempix_char[i+1]=svga_palette[3*CLRSCR_COLOR+1];
    mempix_char[i+2]=svga_palette[3*CLRSCR_COLOR+2];
    mempix_char[i+3]=255;
    }
  break;

  case 16:
  k=screen_width*screen_height;
  for(i=0; i<k; i++)
    {
    mempix_shi[i]=0;
    }
  break;
  
  case 8:
  k=screen_width*screen_height;
  for(i=0; i<k; i++)
    {
    mempix_char[i]=xpalette[0];
    }
  }
lir_refresh_entire_screen();
lir_refresh_screen();
}

void xlir_refresh_entire_screen(void)
{
first_mempix_copy=0;
first_mempix=0;
first_mempix_flag=1;
last_mempix_copy=screen_totpix-1;
last_mempix=screen_totpix-1;
last_mempix_flag=1;
}

void xlir_refresh_screen(void)
{
lir_set_event(EVENT_REFRESH_SCREEN);
}

void xlir_getpalettecolor(int j, int *r, int *g, int *b)
{
int k;
unsigned short int *ipalette;
switch (color_depth)
  {
  case 24:
  b[0]=(int)(svga_palette[3*j  ])>>2;
  g[0]=(int)(svga_palette[3*j+1])>>2;
  r[0]=(int)(svga_palette[3*j+2])>>2;
  break;

  case 16:
  ipalette=(unsigned short int*)(&svga_palette[0]);
  k=ipalette[j];
  k<<=1;
  b[0]=k&0x3f;
  k>>=6;
  g[0]=k&0x3f;
  k>>=5;
  r[0]=k;
  break;

  case 8:
  b[0]=(int)(svga_palette[3*j  ]);
  g[0]=(int)(svga_palette[3*j+1]);
  r[0]=(int)(svga_palette[3*j+2]);
  break;
  }
}

void xlir_setpixel(int x, int y, unsigned char c)
{
int ia;
unsigned short int *ipalette;
ia=x+y*screen_width;
if(ia < 0 || ia >= screen_totpix)
  {
  setpixel_error(x, y);;
  return;
  }
switch (color_depth)
  {
  case 24:
  mempix_char[4*ia  ]=svga_palette[3*c  ];
  mempix_char[4*ia+1]=svga_palette[3*c+1];
  mempix_char[4*ia+2]=svga_palette[3*c+2];
  break;
  
  case 16:
  ipalette=(unsigned short int*)(&svga_palette[0]);
  mempix_shi[ia]=ipalette[c];
  break;

  case 8:
  mempix_char[ia]=xpalette[c];
  break;
  }
if(first_mempix > ia)
  {
  first_mempix_copy=ia;
  first_mempix=ia;
  first_mempix_flag=1;
  }
if(last_mempix < ia)
  {
  last_mempix_copy=ia;
  last_mempix=ia;
  last_mempix_flag=1;
  }
}




// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//  SVGALIB

int llir_getpixel(int x, int y)
{
#if HAVE_SVGALIB == 1
return gl_getpixel(x,y);
#else
(void)x;
(void)y;
return 0;
#endif
}

void lgraphics_init(void)
{
#if HAVE_SVGALIB == 1
int i;
vga_setmode(ui.vga_mode);
gl_setcontextvga(ui.vga_mode);
if(BYTESPERPIXEL != 1)
  {
  lirerr(1243);
  return;
  }
screen_width=WIDTH;
screen_height=HEIGHT;
if(screen_width < 640 || screen_height < 480)
  {
  lirerr(1242);
  return;
  }
for(i=0; i<256;i++)
  {
  gl_setpalettecolor(i, svga_palette[3*i+2],
                        svga_palette[3*i+1],
                        svga_palette[3*i  ]);
  }
init_font(ui.font_scale);
#endif
}

void llir_fix_bug(int bug)
{
(void) bug;
}

void llin_global_uiparms(int wn)
{
#if HAVE_SVGALIB == 1
char ss[80];
int line;
char s[80];
int *modflag;
int i, grp, maxprio;
vga_modeinfo *info;
// ************************************************
// *****   Select screen number for svgalib  ******
// ************************************************
if(wn!=0)
  {
  if( ui.operator_skil == OPERATOR_SKIL_NEWCOMER)
    {
    clear_screen();
    lir_text(0,4,"You are now in NEWCOMER mode.");
    lir_text(0,6,"Press 'Y' to change to NORMAL mode or 'N' to");
    lir_text(0,7,"stay in newcomer mode.");
ask_newco:;
    await_processed_keyboard();
    if(lir_inkey == 'N')goto stay_newco;
    if(lir_inkey != 'Y')goto ask_newco;
    ui.operator_skil=OPERATOR_SKIL_NORMAL;
    }
stay_newco:;
  if( ui.operator_skil == OPERATOR_SKIL_NORMAL)
    {
    clear_screen();
    lir_text(0,7,"You are now in NORMAL mode.");
    lir_text(0,9," 'Y' to change to EXPERT mode or 'N' to");
    lir_text(0,10,"stay in normal mode.");
ask_normal:;
    await_processed_keyboard();
    if(lir_inkey == 'N')goto stay_normal;
    if(lir_inkey != 'Y')goto ask_normal;
    ui.operator_skil=OPERATOR_SKIL_EXPERT;
    }
stay_normal:;
  }
modflag = malloc((GLASTMODE+1) * sizeof(int));
if(modflag == NULL)
  {
  if(wn==0)
    {
    exit(0);
    }
  else
    {  
    lirerr(1034);
    return;
    }
  }
modflag[0]=0;
line=0;
sprintf(s,"Choose one of the following video modes:");
if(wn==0)
  {
  for(i=0; i<32; i++)printf("\n");
  printf("%s",s);
  }
else
  {
  clear_screen();
  settextcolor(15);
  lir_text(0,line,s);
  settextcolor(7);
  line++;
  }  
grp=0;
for (i = 1; i <= GLASTMODE; i++)
  {
  if (vga_hasmode(i))
    {
    info = vga_getmodeinfo(i);
    if (info->colors == 256)
      {
      modflag[0]++;
      modflag[i]=1;
      sprintf(s,"%5d: %dx%d       ",i, info->width, info->height);
      s[20]=0;
      if(wn==0)
        { 
        if(grp==0)printf("\n");
        printf("%s",s); 
        }
      else  
        {
        lir_text(20*grp,line,s);
        }
      grp++;
      if(grp >= 3)
        {
        grp=0;
        line++;
        if(line > screen_last_line-1)line=screen_last_line-1;
        }
      }
    else
      {
      modflag[i]=0;
      }  
    }
  }
if(wn==0)printf("\n");
line++;
if(modflag[0] == 0)
  {
  sprintf(s,"No 256 color graphics found.");
  if(wn==0)
    {
    printf("%s\n",s);
    exit(0);
    }
  else
    {
    lirerr(352198);
    return;
    }
  }        
gtmode:;
sprintf(s,"Enter mode number.");
if(wn==0)
  {
  printf("%s Then %s\n",s,press_enter);
  fflush(stdout);
  lir_sleep(100000);
  if(lir_inkey==0)exit(0);
  clear_keyboard();
  i=0;
  ss[0]=' ';
  ss[1]=0;
  while(ss[i]!=10)
    {
    if(i<10)i++;
    await_keyboard();
    if(lir_inkey==0)exit(0);
    ss[i]=lir_inkey;
    ss[i+1]=0;
    }
  sscanf(ss,"%d", &ui.vga_mode);
  }  
else
  {
  lir_text(12,line,"=>");
  ui.vga_mode=lir_get_integer(15,line,3,0,GLASTMODE);
  }
sprintf(s,"Error: Mode number out of range");
if(ui.vga_mode <0 || ui.vga_mode > GLASTMODE)goto moderr;
if (modflag[ui.vga_mode] == 0)
  {
moderr:;  
  if(wn==0)
    {
    printf("%s\n\n",s);
    }
  goto gtmode;
  }     
free(modflag);
// *******************************************************
if(wn!=0)
  {
  line=0;
  clear_screen();
  }
if(ui.operator_skil == OPERATOR_SKIL_NEWCOMER)
  {
  ui.font_scale=2;
  }
else
  {
  sprintf(s,"Enter font scale (1 to 5)"); 
fntc:;
  if(wn==0)
    {
    printf("\n%s\n=>",s); 
    await_keyboard();
    if(lir_inkey==0)exit(0);
    }
  else
    {
    lir_text(0,line,s);
    await_keyboard();
    if(kill_all_flag) return;
    line++;
    if(line>=screen_last_line)line=screen_last_line;
    }
  if(lir_inkey < '1' || lir_inkey > '5')goto fntc;
  ui.font_scale=lir_inkey-'0';
  if(lir_errcod)return;
  }
lgraphics_init();
if(lir_errcod)return;
clear_screen();
settextcolor(15);
lir_text(0,2,"Mouse speed reduction factor:");
ui.mouse_speed=lir_get_integer(30,2,3,1,999);
ui.max_blocked_cpus=0;
ui.process_priority=0;
wse.parport=0;
wse.parport_pin=0;
if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
  {
  if(no_of_processors > 1)
    {
    clear_screen();
    sprintf(s,"This system has % d processors.",no_of_processors);
    lir_text(0,3,s);
    lir_text(0,4,"How many do you allow Linrad to block?");
    lir_text(0,5,
        "If you run several instances of Linrad on one multiprocessor");
    lir_text(0,6,"platform it may be a bad idea to allow the total number");
    lir_text(0,7,"of blocked CPUs to be more that the total number less one.");        
    lir_text(5,9,"=>");
    ui.max_blocked_cpus=lir_get_integer(8,9,2,0,no_of_processors-1);
    }    
prio:;
  clear_screen();
  sprintf(s,"Set process priority (0=NORMAL to "); 
  if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
    {
    strcat(s,"3=REALTIME)");
    maxprio=3;
    }
  else  
    {
    strcat(s,"2=HIGH)");
    maxprio=2;
    }
  lir_text(0,7,s);
  to_upper_await_keyboard();
  if(lir_inkey >='0' && lir_inkey <= maxprio+'0')
    {
    ui.process_priority=lir_inkey-'0';
    }
  else
    {
    goto prio;
    }
  if(ui.process_priority==2)
    {
    sprintf(s,"Are you sure? (Y/N)");
    clear_screen();
    lir_text(0,3,s);
    to_upper_await_keyboard();
    if(lir_inkey=='Y')goto prio_x;
    goto prio;
    }
  if(ui.process_priority==3)
    {
    sprintf(s,"Hmmm, you are the expert so you know what might happen....");
    sprintf(ss,"Are you really sure? (Y/N)");
    clear_screen();
    lir_text(0,3,s);
    lir_text(0,5,ss);
    to_upper_await_keyboard();
    if(lir_inkey=='Y')goto prio_x;
    goto prio;
    }
prio_x:;
  if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
    {
    get_autostart:;    
    clear_screen();
    lir_text(0,3,
             "Set autostart: Z=none,A=WCW,B=CW,C=MS,D=SSB,E=FM,F=AM,G=QRSS");
    to_upper_await_keyboard();
    }
  if(lir_inkey == 'Z')
    {
    ui.autostart=0;
    }
  else
    {
    if(lir_inkey < 'A' || lir_inkey > 'G')goto get_autostart;
    ui.autostart=lir_inkey;
    }
  }  
if(ui.screen_width_factor <= 0)ui.screen_width_factor=96;
if(ui.screen_height_factor <= 0)ui.screen_height_factor=92;
clear_screen();
lir_text(0,6,
          "Do not forget to save your parameters with 'W' in the main menu");
lir_text(5,8,press_any_key);
await_keyboard();
clear_screen();
settextcolor(7);
#else
(void)wn;
#endif
}

void llir_fillbox(int x, int y,int w, int h, unsigned char c)
{
#if HAVE_SVGALIB == 1
if(x < 0 || 
   w < 0 || 
   x+w > WIDTH ||
   y < 0 ||
   h < 0 || 
   y+h > HEIGHT)
     {
     fillbox_error(x,y,w,h);
     return;
     }
gl_fillbox(x,y,w,h,c);
#else
(void)x;
(void)y;
(void)w;
(void)h;
(void)c;
#endif
}

void llir_getbox(int x, int y, int w, int h, size_t* dp)
{
#if HAVE_SVGALIB == 1
gl_getbox(x,y,w,h,dp);
#else
(void)x;
(void)y;
(void)w;
(void)h;
(void)dp;
#endif
}

void llir_putbox(int x, int y, int w, int h, size_t* dp)
{
#if HAVE_SVGALIB == 1
int k;
k=(x+y*screen_width);
if(k<0 || k+(h-1)*screen_width+w-1 >= screen_totpix)
  {
  putbox_error(x,y,w,h);
  return;
  }
gl_putbox(x,y,w,h,dp);
#else
(void)x;
(void)y;
(void)w;
(void)h;
(void)dp;
#endif
}

void llir_hline(int x1, int y, int x2, unsigned char c)
{
#if HAVE_SVGALIB == 1
if( x1 < 0 ||
    x2 < 0 ||
    y < 0 ||
    x1>=WIDTH ||
    x2>=WIDTH ||
    y >= HEIGHT)
      {
      h_line_error(x1, y, x2);
      return;
      }
gl_hline( x1, y, x2, c);
#else
(void)x1;
(void)y;
(void)x2;
(void)c;
#endif
}

void llir_line(int x1, int yy1, int x2,int y2, unsigned char c)
{
#if HAVE_SVGALIB == 1
if( x1 < 0 )  lirerr(99201);
if( x2 < 0 )lirerr(99202);
if( yy1 < 0 )lirerr(99203);
if( y2 < 0 )lirerr(99204);
if( x1>=WIDTH )lirerr(99205);
if( x2>=WIDTH )lirerr(99206);
if( yy1 >= HEIGHT)lirerr(99207);
if( y2 >= HEIGHT)lirerr(99208);
gl_line( x1, yy1, x2, y2, c);
#else
(void)x1;
(void)yy1;
(void)x2;
(void)y2;
(void)c;
#endif
}

void lclear_screen(void)
{
#if HAVE_SVGALIB == 1
vga_clear();
#endif
}

void llir_getpalettecolor(int j, int *r, int *g, int *b)
{
#if HAVE_SVGALIB == 1
gl_getpalettecolor(j, r, g, b);
#else
(void)j;
(void)r;
(void)g;
(void)b;
#endif
}

void llir_setpixel(int x, int y, unsigned char c)
{
#if HAVE_SVGALIB == 1
if( x < 0 )
  {
  setpixel_error(x, y);;
  return;
  }
else
  {  
  if( y < 0 )
    {
    setpixel_error(x, y);;
    return;
    }
  else
    {
    if( x >= WIDTH )
      {
      setpixel_error(x, y);;
      return;
      }
    else
      {
      if( y >= HEIGHT)
        {
        setpixel_error(x, y);;
        return;
        }
      else
        {
        gl_setpixel( x, y, (int)c);
        }
      }
    }
  }
#else
(void)x;
(void)y;
(void)c;
#endif
}

void llir_refresh_screen(void)
{
// This is used by Windows and X11 to force a copy from memory
// to the screen. svgalib is fast enough to allow
// direct screen updates. (SetPixel under Windows is extremely slow)
}

void llir_refresh_entire_screen(void)
{
// This is used by X11 to force a copy from memory
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
void x_global_uiparms(int wn)
{
char s[80],ss[80];
char sr[80],st[80],su[80];
char chr;
int line, maxprio;
line=2;
if(ui.vga_mode==0)ui.vga_mode=12;
if(ui.mouse_speed <=0 || ui.mouse_speed> 120)ui.mouse_speed=8;
if(ui.vga_mode < 10 || ui.vga_mode > 256)ui.vga_mode=10;
ui.process_priority=1;
if(wn != 0)
  { 
  if(ui.operator_skil == OPERATOR_SKIL_NEWCOMER)
    {
    clear_screen();
    lir_text(0,4,"You are now in NEWCOMER mode.");
    lir_text(0,6,"Press 'Y' to change to NORMAL mode or 'N' to");
    lir_text(0,7,"stay in newcomer mode.");
ask_newco:;
    await_processed_keyboard();
    if(lir_inkey == 'N')goto stay_newco;
    if(lir_inkey != 'Y')goto ask_newco;
    ui.operator_skil=OPERATOR_SKIL_NORMAL;
    }
stay_newco:;
  if(ui.operator_skil == OPERATOR_SKIL_NORMAL)
    {
    clear_screen();
    lir_text(0,7,"You are now in NORMAL mode.");
    lir_text(0,9,"Press 'Y' to change to EXPERT mode or 'N' to");
    lir_text(0,10,"stay in normal mode.");
ask_normal:;
    await_processed_keyboard();
    if(lir_inkey == 'N')goto stay_normal;
    if(lir_inkey != 'Y')goto ask_normal;
    ui.operator_skil=OPERATOR_SKIL_EXPERT;
    }
stay_normal:;
  clear_screen();
  }
if(ui.operator_skil == OPERATOR_SKIL_NEWCOMER)
  {
  ui.font_scale=2;
  wse.parport=0;
  wse.parport_pin=0;
  ui.max_blocked_cpus=0;
  }
else
  {  
  sprintf(s,"Font scale (1 to 5):"); 
  if(wn==0)
    {
fntc:;
    printf("\n%s\n=>",s); 
    fflush(NULL);
    while(fgets(ss,8,stdin)==NULL)lir_sleep(10000);
    lir_inkey=to_upper(ss[0]);
    if(lir_inkey < '1' || lir_inkey > '5')goto fntc;
    ui.font_scale=lir_inkey-'0';
    }
  else
    {
    lir_text(0,line,s);
    ui.font_scale=lir_get_integer(21,line,2,1,5);
    if(kill_all_flag) return;
    line++;
    }
  if(no_of_processors > 1)
    {
    sprintf(s,"This system has % d processors.",no_of_processors);
    sprintf(ss,"How many do you allow Linrad to block?");
    sprintf(sr,
        "If you run several instances of Linrad on one multiprocessor");
    sprintf(st,"platform it may be a bad idea to allow the total number");
    sprintf(su,"of blocked CPUs to be more that the total number less one.");        
    if(wn==0)
      {
      printf("\n%s",s); 
      printf("\n%s",ss); 
      printf("\n%s",sr); 
      printf("\n%s",st); 
      printf("\n%s\n\n=>",su); 
      fflush(NULL);
      while(fgets(ss,8,stdin)==NULL)lir_sleep(10000);
      sscanf(ss,"%d", &ui.max_blocked_cpus);
      if(ui.max_blocked_cpus <0)ui.max_blocked_cpus=0;
      if(ui.max_blocked_cpus >=no_of_processors)
                                          ui.max_blocked_cpus=no_of_processors;
      }
    else
      {
      line++;
      lir_text(0,line,s);
      line++;
      lir_text(0,line,ss);
      line++;
      lir_text(0,line,sr);
      line++;
      lir_text(0,line,st);
      line++;
      lir_text(0,line,su);
      line+=2;
      lir_text(0,line,"=>");
      ui.max_blocked_cpus=lir_get_integer(3,line,2,0,no_of_processors-1);
      line+=2;
      }
    }    
  else
    {
    ui.max_blocked_cpus=0;
    }  
  }
sprintf(s,"You can specify the screen size in pixels or as a percentage");
sprintf(sr,"of the entire area of all your screens. Enter Y for sizes in"); 
sprintf(st,"pixels or N for sizes as %% (Y/N)=>");
screen_sel:;
if(wn == 0)
  {
  printf("\n%s",s); 
  printf("\n%s",sr); 
  printf("\n%s",st); 
  fflush(NULL);
  while(fgets(ss,8,stdin)==NULL)lir_sleep(10000);
  sscanf(ss,"%c", &chr);
  }
else
  {
  clear_lines(line,line);
  lir_text(0,line,s);
  line++;
  lir_text(0,line,sr);
  line++;
  lir_text(0,line,st);
  line++;
  await_keyboard();
  chr=lir_inkey;
  if(kill_all_flag) return;
  }
chr=toupper(chr);  
switch (chr)
  {
  case 'N':    
  sprintf(s,"Percentage of screen width to use(25 to 100):"); 
parport_wfac:;
  if(wn==0)
    {
    printf("\n%s\n=>",s); 
    fflush(NULL);
    while(fgets(ss,8,stdin)==NULL)lir_sleep(10000);
    sscanf(ss,"%d", &ui.screen_width_factor);
    if(ui.screen_width_factor < 25 ||
       ui.screen_width_factor > 100)goto parport_wfac;
    }
  else
    {
    clear_lines(line,line);
    lir_text(0,line,s);
    ui.screen_width_factor=lir_get_integer(47,line,3,25,100);
    if(kill_all_flag) return;
    line++;
    }
  sprintf(s,"Percentage of screen height to use (25 to 100):"); 
parport_hfac:;
  if(wn==0)
    {
    printf("\n%s\n=>",s); 
    fflush(NULL);
    while(fgets(ss,8,stdin)==NULL)lir_sleep(10000);
    sscanf(ss,"%d", &ui.screen_height_factor);
    if(ui.screen_height_factor < 25 ||
       ui.screen_height_factor > 100)goto parport_hfac;
    }
  else
    {
    lir_text(0,line,s);
    ui.screen_height_factor=lir_get_integer(49,line,3,25,100);
    if(kill_all_flag) return;
    clear_screen();
    }
  break;
  
  case 'Y':    
  sprintf(s,"Screen width in pixels (500 to 10000):"); 
parport_wpix:;
  if(wn==0)
    {
    printf("\n%s\n=>",s); 
    fflush(NULL);
    while(fgets(ss,8,stdin)==NULL)lir_sleep(10000);
    sscanf(ss,"%d", &ui.screen_width_factor);
    if(ui.screen_width_factor < 500 ||
       ui.screen_width_factor > 10000)goto parport_wpix;
    }
  else
    {
    clear_lines(line,line);
    lir_text(0,line,s);
    ui.screen_width_factor=lir_get_integer(38,line,5,500,10000);
    if(kill_all_flag) return;
    line++;
    }
  ui.screen_width_factor=-ui.screen_width_factor;
  sprintf(s,"Screen height in pixels (400 to 10000):"); 
parport_hpix:;
  if(wn==0)
    {
    printf("\n%s\n=>",s); 
    fflush(NULL);
    while(fgets(ss,8,stdin)==NULL)lir_sleep(10000);
    sscanf(ss,"%d", &ui.screen_height_factor);
    if(ui.screen_height_factor < 400 ||
       ui.screen_height_factor > 10000)goto parport_hpix;
    }
  else
    {
    lir_text(0,line,s);
    ui.screen_height_factor=lir_get_integer(39,line,5,400,10000);
    if(kill_all_flag) return;
    clear_screen();
    }
  ui.screen_height_factor=-ui.screen_height_factor;
  break;
  
  default:
  goto screen_sel;
  }
if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
  {
  prio:;
  sprintf(s,"Set process priority (0=NORMAL to "); 
  if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
    {
    strcat(s,"3=REALTIME)");
    maxprio=3;
    }
  else  
    {
    strcat(s,"2=HIGH)");
    maxprio=2;
    }
  if(wn==0)
    {
    printf("\n%s, then press Enter: ",s); 
    fflush(NULL);
    while(fgets(ss,8,stdin)==NULL)lir_sleep(10000);
    lir_inkey=to_upper(ss[0]);
    }  
  else
    {
    clear_screen();
    lir_text(0,7,s);
    to_upper_await_keyboard();
    }
  if(lir_inkey >='0' && lir_inkey <= maxprio+'0')
    {
    ui.process_priority=lir_inkey-'0';
    }
  else
    {
    goto prio;
    }
  if(ui.process_priority==2)
    {
    sprintf(s,"Are you sure? (Y/N)");
    if(wn==0)
      {
      printf("\n%s\n=>",s); 
      fflush(NULL);
      while(fgets(ss,8,stdin)==NULL)lir_sleep(10000);
      if(to_upper(ss[0])=='Y')goto prio_x;
      goto prio;
      }
    else
      {
      clear_screen();
      lir_text(0,3,s);
      to_upper_await_keyboard();
      if(lir_inkey=='Y')goto prio_xc;
      goto prio;
      }
    }
  if(ui.process_priority==3)
    {
    sprintf(s,"Hmmm, you are the expert so you know what might happen....");
    sprintf(ss,"Are you really sure? (Y/N)");
    if(wn==0)
      {
      printf("\n%s\n=>",s); 
      printf("\n%s\n=>",ss); 
      fflush(NULL);
      while(fgets(ss,8,stdin)==NULL)lir_sleep(10000);
      if(to_upper(ss[0])=='Y')goto prio_x;
      goto prio;
      }
    else
      {
      clear_screen();
      lir_text(0,3,s);
      lir_text(0,5,ss);
      to_upper_await_keyboard();
      if(lir_inkey=='Y')goto prio_xc;
      goto prio;
      }
    }
prio_xc:;
  if(wn!=0)clear_screen();
  }
prio_x:;  
if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
  {
  sprintf(s,"Set autostart: Z=none,A=WCW,B=CW,C=MS,D=SSB,E=FM,F=AM,G=QRSS");
get_autostart:;    
  if(wn==0)
    {
    printf("\n%s\n=>",s); 
    fflush(NULL);
    while(fgets(ss,8,stdin)==NULL)lir_sleep(10000);
    }         
  else
    {
    lir_text(0,6,s);
    lir_text(0,8,"=>");
    to_upper_await_keyboard();
    sprintf(ss,"%c",lir_inkey);
    lir_text(3,8,ss);
    line=8;
    }
  lir_inkey=to_upper(ss[0]);
  if(lir_inkey == 'Z')
    {
    ui.autostart=0;
    }
  else
    {
    if(lir_inkey < 'A' || lir_inkey > 'G')goto get_autostart;
    ui.autostart=lir_inkey;
    }
  }  
if(wn == 0)
  {
  printf("\n\nLinrad will now open another window.");
  printf("\nMinimize this window and click on the new window to continue.");
  printf(
      "\n\nDo not forget to save your parameters with 'W' in the main menu");
  fflush(NULL);
  }
else  
  { 
  line+=4;
  lir_text(0,line,"Save the new parameters with W in the main menu");
  line++;
  lir_text(0,line,"Then restart Linrad for changes to take effect.");
  line++;
  lir_text(10,line,press_any_key);
  await_keyboard();
  }
}



//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//    FBDEV

void fgraphics_init(void)
{
#if HAVE_SVGALIB == 1
int i, k;
unsigned short int *ipalette;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
short unsigned int red[MAX_SVGA_PALETTE];
short unsigned int green[MAX_SVGA_PALETTE];
short unsigned int blue[MAX_SVGA_PALETTE];
short unsigned int transp[MAX_SVGA_PALETTE];
struct sockaddr_un addr;
struct MouseCaps caps;
struct termios termios_p;
framebuffer_screensize=0;
framebuffer_handle = open("/dev/fb0", O_RDWR);
if (!framebuffer_handle) 
  {
  lirerr(1349);
  return;
  }
if (ioctl(framebuffer_handle, FBIOGET_VSCREENINFO, &vinfo)) 
  {
  lirerr(1351);
  return;
  }
color_depth=vinfo.bits_per_pixel;
framebuffer_screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
mempix_char = mmap(0, framebuffer_screensize, PROT_READ | PROT_WRITE, MAP_SHARED, framebuffer_handle, 0);
if ((ssize_t)mempix_char == -1) 
  {
  lirerr(1355);
  return;
  }
color_depth=vinfo.bits_per_pixel;
printf("color_depth=%d",color_depth);
tcgetattr(0, &termios_pp);
memcpy(&termios_p, &termios_pp, sizeof(termios_p));
terminal_flag=TRUE;
termios_p.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                                      | INLCR | IGNCR | ICRNL | IXON);
termios_p.c_oflag &= ~OPOST;
termios_p.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
termios_p.c_cflag &= ~(CSIZE | PARENB);
termios_p.c_cflag |= CS8;
tcsetattr(0, TCSANOW, &termios_p);
printf( "\e[?25l\n " ); // hide the cursor
// Open gpmctl. This will hide the gpm cursor.
gpm_fd=socket(AF_UNIX,SOCK_STREAM,0);
if(gpm_fd >= 0)
  {
  memset(&addr,0,sizeof(addr));
  addr.sun_family=AF_UNIX;
  strcpy(addr.sun_path, GPMCTL);
  i=sizeof(addr.sun_family)+strlen(GPMCTL);
  if(connect(gpm_fd,(struct sockaddr *)(&addr),i)<0 ) 
    {
    close(gpm_fd);
    gpm_fd=open(GPMCTL,O_RDWR);
    }
  }  
screen_width=vinfo.xres;
screen_height=vinfo.yres;
screen_totpix=screen_width*screen_height;
i=vga_getmousetype();
if(i == MOUSE_NONE)
  {
  lirerr(1354);
  return;
  }
mouse_init("/dev/mouse",i,MOUSE_DEFAULTSAMPLERATE);
mouse_setdefaulteventhandler();
mouse_setxrange(0, screen_width-1);
mouse_setyrange(0, screen_height-1);
mouse_setwrap(MOUSE_NOWRAP);
lir_status=LIR_OK;
// Set up mouse and system parameters in case it has not been done already.
mouse_cursize=screen_height/80;
if(mouse_cursize > MAX_MOUSE_CURSIZE)mouse_cursize=MAX_MOUSE_CURSIZE;
if(ui.mouse_speed < 1)ui.mouse_speed=1;
if(ui.mouse_speed > 999)ui.mouse_speed=999;
mouse_setscale(ui.mouse_speed);
mouse_x=screen_width/2;
mouse_y=screen_height/2;
new_mouse_x=screen_width/2;
new_mouse_y=screen_height/2;
mouse_setposition( mouse_x, mouse_y);
if(mouse_getcaps(&caps)) 
  {
// Failed! Old library version... Check the mouse type.
  i=vga_getmousetype() & MOUSE_TYPE_MASK;
  if(i == MOUSE_INTELLIMOUSE || i==MOUSE_IMPS2)
    {
    mouse_wheel_flag=1;
    }
  else
    {
    mouse_wheel_flag=0;
    }
  }
else
  {
// If this is a mouse_wheel_flag mouse, interpret rx as a wheel
  mouse_wheel_flag = ((caps.info & MOUSE_INFO_WHEEL) != 0);
  }
if(mouse_wheel_flag)
  {
  mouse_setrange_6d(0,0, 0,0, 0, 0, -180,180, 0,0, 0,0, MOUSE_RXDIM);
  }
if(screen_width < 640 || screen_height < 480)
  {
  lirerr(1242);
  return;
  }
switch (color_depth)
  {
  case 32:
  case 24:
  for(i=0; i<screen_totpix*4; i++)mempix_char[i]=0;
// ******************************************************************
// Rearrange the palette. It was designed for svgalib under Linux
  for(i=0; i<3*MAX_SVGA_PALETTE; i++)
    {
    svga_palette[i]=(unsigned char)(svga_palette[i]<<2);
    if(svga_palette[i] != 0) svga_palette[i]|=3;
    }
  break;
  
  case 16:
  mempix_shi=(unsigned short int*)mempix_char;
  for(i=0; i<screen_totpix; i++)mempix_shi[i]=0;
// ******************************************************************
  ipalette=(unsigned short int*)(&svga_palette[0]);
  for(i=0; i<256; i++)
    {
    k=svga_palette[3*i+2];
    k&=0xfffe;
    k<<=5;
    k|=svga_palette[3*i+1];
    k&=0xfffe;
    k<<=6;
    k|=svga_palette[3*i  ];
    k>>=1;
    ipalette[i]=(short unsigned int)k;
    }
  break;

  case 8:
  if (ioctl(framebuffer_handle, FBIOGET_FSCREENINFO, &finfo)) 
    {
    lirerr(1350);
    return;
    }
  fb_palette=malloc(256*sizeof(fb_palette));
  if(finfo.visual == FB_VISUAL_DIRECTCOLOR) 
    {
    if (ioctl(framebuffer_handle, FBIOGETCMAP , &fb_palette)) 
      {
      free(fb_palette);
      lirerr(1352);
      return;
      }
    }
  fb_palette->start=0;
  fb_palette->len=MAX_SVGA_PALETTE;
  for(i=0; i<MAX_SVGA_PALETTE; i++)
    {
    red[i]=(short unsigned int)(svga_palette[3*i+2]);
    green[i]=(short unsigned int)(svga_palette[3*i+1]);
    blue[i]=(short unsigned int)(svga_palette[3*i  ]);
    transp[i]=0;
    }
  fb_palette->red=red;  
  fb_palette->green=green;  
  fb_palette->blue=blue;  
  fb_palette->transp=transp;  

  vinfo.activate=FB_ACTIVATE_ALL|FB_CHANGE_CMAP_VBL;
  ioctl(framebuffer_handle, FBIOPUT_VSCREENINFO, &vinfo);

  i=ioctl(framebuffer_handle, FBIOPUTCMAP , &fb_palette); 
printf("\nioctl FBIOPUTCMAP returned %d\n",i);
  break;

  default:
  printf("\nUnknown color depth: %d\n",color_depth);
  lirerr(1353);
  return;
  } 
#endif
}

void flin_global_uiparms(int wn)
{
char ss[80];
int line;
char s[80];
int maxprio;
clear_keyboard();
line=0;
if(wn!=0)
  {
  if( ui.operator_skil == OPERATOR_SKIL_NEWCOMER)
    {
    clear_screen();
    lir_text(0,4,"You are now in NEWCOMER mode.");
    lir_text(0,6,"Press 'Y' to change to NORMAL mode or 'N' to");
    lir_text(0,7,"stay in newcomer mode.");
ask_newco:;
    await_processed_keyboard();
    if(lir_inkey == 'N')goto stay_newco;
    if(lir_inkey != 'Y')goto ask_newco;
    ui.operator_skil=OPERATOR_SKIL_NORMAL;
    }
stay_newco:;
  if( ui.operator_skil == OPERATOR_SKIL_NORMAL)
    {
    clear_screen();
    lir_text(0,7,"You are now in NORMAL mode.");
    lir_text(0,9," 'Y' to change to EXPERT mode or 'N' to");
    lir_text(0,10,"stay in normal mode.");
ask_normal:;
    await_processed_keyboard();
    if(lir_inkey == 'N')goto stay_normal;
    if(lir_inkey != 'Y')goto ask_normal;
    ui.operator_skil=OPERATOR_SKIL_EXPERT;
    }
stay_normal:;
  }
// *******************************************************
if(wn!=0)
  {
  clear_screen();
  }
if(ui.operator_skil == OPERATOR_SKIL_NEWCOMER)
  {
  ui.font_scale=2;
  }
else
  {
  sprintf(s,"Enter font scale (1 to 5)"); 
fntc:;
  if(wn==0)
    {
    printf("\n%s\n=>",s); 
    await_keyboard();
    if(lir_inkey==0)exit(0);
    }
  else
    {
    lir_text(0,line,s);
    await_keyboard();
    if(kill_all_flag) return;
    line++;
    if(line>=screen_last_line)line=screen_last_line;
    }
  if(lir_inkey < '1' || lir_inkey > '5')goto fntc;
  ui.font_scale=lir_inkey-'0';
  }
if(wn == 0)fgraphics_init();
init_font(ui.font_scale);
if(kill_all_flag)return;
clear_screen();
settextcolor(15);
lir_text(0,2,"Mouse speed reduction factor:");
ui.mouse_speed=lir_get_integer(30,2,3,1,999);
ui.max_blocked_cpus=0;
ui.process_priority=0;
wse.parport=0;
wse.parport_pin=0;
if(ui.operator_skil != OPERATOR_SKIL_NEWCOMER)
  {
  if(no_of_processors > 1)
    {
    clear_screen();
    sprintf(s,"This system has % d processors.",no_of_processors);
    lir_text(0,3,s);
    lir_text(0,4,"How many do you allow Linrad to block?");
    lir_text(0,5,
        "If you run several instances of Linrad on one multiprocessor");
    lir_text(0,6,"platform it may be a bad idea to allow the total number");
    lir_text(0,7,"of blocked CPUs to be more that the total number less one.");        
    lir_text(5,9,"=>");
    ui.max_blocked_cpus=lir_get_integer(8,9,2,0,no_of_processors-1);
    }    
prio:;
  clear_screen();
  sprintf(s,"Set process priority (0=NORMAL to "); 
  if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
    {
    strcat(s,"3=REALTIME)");
    maxprio=3;
    }
  else  
    {
    strcat(s,"2=HIGH)");
    maxprio=2;
    }
  lir_text(0,7,s);
  to_upper_await_keyboard();
  if(lir_inkey >='0' && lir_inkey <= maxprio+'0')
    {
    ui.process_priority=lir_inkey-'0';
    }
  else
    {
    goto prio;
    }
  if(ui.process_priority==2)
    {
    sprintf(s,"Are you sure? (Y/N)");
    clear_screen();
    lir_text(0,3,s);
    to_upper_await_keyboard();
    if(lir_inkey=='Y')goto prio_x;
    goto prio;
    }
  if(ui.process_priority==3)
    {
    sprintf(s,"Hmmm, you are the expert so you know what might happen....");
    sprintf(ss,"Are you really sure? (Y/N)");
    clear_screen();
    lir_text(0,3,s);
    lir_text(0,5,ss);
    to_upper_await_keyboard();
    if(lir_inkey=='Y')goto prio_x;
    goto prio;
    }
prio_x:;
  if(ui.operator_skil == OPERATOR_SKIL_EXPERT)
    {
    get_autostart:;    
    clear_screen();
    lir_text(0,3,
             "Set autostart: Z=none,A=WCW,B=CW,C=MS,D=SSB,E=FM,F=AM,G=QRSS");
    to_upper_await_keyboard();
    }
  if(lir_inkey == 'Z')
    {
    ui.autostart=0;
    }
  else
    {
    if(lir_inkey < 'A' || lir_inkey > 'G')goto get_autostart;
    ui.autostart=lir_inkey;
    }
  }  
if(ui.screen_width_factor <= 0)ui.screen_width_factor=96;
if(ui.screen_height_factor <= 0)ui.screen_height_factor=92;
clear_screen();
lir_text(0,6,
          "Do not forget to save your parameters with 'W' in the main menu");
lir_text(5,8,press_any_key);
await_keyboard();
clear_screen();
settextcolor(7);
}

void flir_fillbox(int x, int y,int w, int h, unsigned char c)
{
int i, j, k;
unsigned short int *ipalette;
k=(x+y*screen_width);
if(k<0 || k+(h-1)*screen_width+w > screen_totpix)
  {
  fillbox_error(x,y,w,h);
  return;
  }
switch (color_depth)
  {
  case 24:
  case 32:
  for(j=0; j<h; j++)
    {
    for(i=0; i<w; i++)
      {
      mempix_char[4*(k+i)  ]=svga_palette[3*c  ];
      mempix_char[4*(k+i)+1]=svga_palette[3*c+1];
      mempix_char[4*(k+i)+2]=svga_palette[3*c+2];
      }
    k+=screen_width;  
    }
  break;
      
  case 16:
  ipalette=(unsigned short int*)(&svga_palette[0]);
  for(j=0; j<h; j++)
    {
    for(i=0; i<w; i++)
      {
      mempix_shi[k+i]=ipalette[c];
      }
    k+=screen_width;  
    }
  break;

  case 8:
  for(j=0; j<h; j++)
    {
    for(i=0; i<w; i++)
      {
      mempix_char[k+i]=c;
      }
    k+=screen_width;  
    }
  break;
  }    
}


void flir_getbox(int x, int y, int w, int h, size_t* dp)
{
unsigned char *savmem;
int i, j, k, m;
unsigned char c,c1,c2,c3;
unsigned short int *ipalette;
k=x+y*screen_width;
savmem=(unsigned char*)dp;
m=0;
c=0;
c1=0;
c2=0;
switch (color_depth)
  {
  case 24:
  case 32:
  for(j=0; j<h; j++)
    {
    for(i=0; i<w; i++)
      {
      if(svga_palette[3*c  ] == mempix_char[4*(k+i)  ] &&
         svga_palette[3*c+1] == mempix_char[4*(k+i)+1] &&
         svga_palette[3*c+2] == mempix_char[4*(k+i)+2])goto store_24;
      if(svga_palette[3*c1  ] == mempix_char[4*(k+i)  ] &&
         svga_palette[3*c1+1] == mempix_char[4*(k+i)+1] &&
         svga_palette[3*c1+2] == mempix_char[4*(k+i)+2])
        {
        c3=c;
        c=c1;
        c1=c;
        goto store_24;
        }
      if(svga_palette[3*c2  ] == mempix_char[4*(k+i)  ] &&
         svga_palette[3*c2+1] == mempix_char[4*(k+i)+1] &&
         svga_palette[3*c2+2] == mempix_char[4*(k+i)+2])
        {
        c3=c;
        c=c2;
        c1=c;
        c2=c3;
        goto store_24;
        }
      c2=c1;
      c1=c;
      while(svga_palette[3*c  ] != mempix_char[4*(k+i)  ] ||
            svga_palette[3*c+1] != mempix_char[4*(k+i)+1] ||
            svga_palette[3*c+2] != mempix_char[4*(k+i)+2])
        {
        c++;
        if(c>=MAX_SVGA_PALETTE)c=0;
        if(c==c1)
          {
//          lirerr(1248);
c=0;
goto store_24;


          return;
          }
        }
store_24:;
      savmem[m]=c;
      m++;
      }
    k+=screen_width;
    }
  break;

  case 16:
  ipalette=(unsigned short int*)(&svga_palette[0]);
  for(j=0; j<h; j++)
    {
    for(i=0; i<w; i++)
      {
      if(ipalette[c] == mempix_shi[(k+i)]) goto store_16;
      if(ipalette[c1] == mempix_shi[(k+i)])
        {
        c3=c;
        c=c1;
        c1=c;
        goto store_16;
        }
      if(ipalette[c2] == mempix_shi[(k+i)])
        {
        c3=c;
        c=c2;
        c1=c;
        c2=c3;
        goto store_16;
        }
      c2=c1;
      c1=c;
      while(ipalette[c] != mempix_shi[(k+i)])
        {
        c++;
        if(c>=MAX_SVGA_PALETTE)c=0;
        if(c==c1)
          {
          lirerr(1248);
          return;
          }
        }
store_16:;
      savmem[m]=c;
      m++;
      }
    k+=screen_width;
    }
  break;

  case 8:
  for(j=0; j<h; j++)
    {
    for(i=0; i<w; i++)
      {
      savmem[m]=mempix_char[k+i];
      m++;
      }
    k+=screen_width;
    }
  break;
  }
}


void flir_putbox(int x, int y, int w, int h, size_t* dp)
{
unsigned char *savmem;
int i, j, k, m;
unsigned short int *ipalette;
k=(x+y*screen_width);
if(k<0 || k+(h-1)*screen_width+w-1 >= screen_totpix)
  {
  putbox_error(x,y,w,h);
  return;
  }
m=0;
savmem=(unsigned char*)dp;
switch (color_depth)
  {
  case 24:
  case 32:
  for(j=0; j<h; j++)
    {  
    for(i=0; i<w; i++)
      {
      mempix_char[4*(k+i)  ]=svga_palette[3*savmem[m]  ];
      mempix_char[4*(k+i)+1]=svga_palette[3*savmem[m]+1];
      mempix_char[4*(k+i)+2]=svga_palette[3*savmem[m]+2];
      m++;
      }
    k+=screen_width;
    }
  break;
  
  case 16:
  ipalette=(unsigned short int*)(&svga_palette[0]);
  for(j=0; j<h; j++)
    {  
    for(i=0; i<w; i++)
      {
      mempix_shi[k+i]=ipalette[savmem[m]];
      m++;
      }
    k+=screen_width;
    }
  break;

  case 8:
  for(j=0; j<h; j++)
    {  
    for(i=0; i<w; i++)
      {
      mempix_char[k+i]=savmem[m];
      m++;
      }
    k+=screen_width;
    }
  }  
}


void flir_hline(int x1, int y, int x2, unsigned char c)
{
int i, ia, ib;
unsigned short int *ipalette;
ia=y*screen_width;
if(x1 <= x2)
  {
  ib=ia+x2;
  ia+=x1;
  }
else
  {
  ib=ia+x1;
  ia+=x2;
  }
if(ia < 0 || ib >= screen_totpix)
  {
  h_line_error(x1, y, x2);
  return;
  }
switch (color_depth)
  {
  case 24:
  case 32:
  for(i=ia; i<=ib; i++)
    {
    mempix_char[4*i  ]=svga_palette[3*c  ];
    mempix_char[4*i+1]=svga_palette[3*c+1];
    mempix_char[4*i+2]=svga_palette[3*c+2];
    }
  break;
  
  case 16:
  ipalette=(unsigned short int*)(&svga_palette[0]);
  for(i=ia; i<=ib; i++)
    {
    mempix_shi[i]=ipalette[c];
    }
  break;

  case 8:
  for(i=ia; i<=ib; i++)
    {
    mempix_char[i]=c;
    }
  break;
  }
}


void flir_line(int x1, int yy1, int x2,int y2, unsigned char c)
{
int ia;
int i,j,k;
int xd, yd;
float t1,t2,delt;
unsigned short int *ipalette;
if(x1 < 0)x1=0;
if(x1 >= screen_width)x1=screen_width-1;
if(x2 < 0)x2=0;
if(x2 >= screen_width)x2=screen_width-1;
if(yy1 < 0)yy1=0;
if(yy1 >= screen_height)yy1=screen_height-1;
if(y2 < 0)y2=0;
if(y2 >= screen_height)y2=screen_height-1;
xd=x2-x1;
yd=y2-yy1;
if(yd==0)
  {
  if(xd==0)
    {
    lir_setpixel(x1,yy1,c);
    }
  else
    {
    lir_hline(x1,yy1,x2,c);
    }
  return;
  }
if(abs(xd)>=abs(yd))
  {
  if(xd>=0)
    {
    k=1;
    }
  else
    {
    k=-1;
    }  
  if(yd >= 0)
    {
    delt=0.5;
    }
  else
    {  
    delt=-0.5;
    }
  t1=yy1;
  t2=(float)(yd)/abs(xd);
  i=x1-k;
  switch (color_depth)
    {
    case 24:
    case 32:
    while(i!=x2)
      {
      i+=k;
      j=t1+delt;
      ia=i+j*(screen_width);
      mempix_char[4*ia  ]=svga_palette[3*c  ];
      mempix_char[4*ia+1]=svga_palette[3*c+1];
      mempix_char[4*ia+2]=svga_palette[3*c+2];
      t1+=t2;
      }
    break;
    
    case 16:
    ipalette=(unsigned short int*)(&svga_palette[0]);
    while(i!=x2)
      {
      i+=k;
      j=t1+delt;
      ia=i+j*(screen_width);
      mempix_shi[ia  ]=ipalette[c  ];
      t1+=t2;
      }
    break;
    
    case 8:
    while(i!=x2)
      {
      i+=k;
      j=t1+delt;
      ia=i+j*(screen_width);
      mempix_char[ia  ]=c;
      t1+=t2;
      }
    break;
        
    }  
  }  
else
  {
  if(yd>=0)
    {
    k=1;
    }
  else
    {
    k=-1;
    } 
  if(xd >= 0)
    {
    delt=0.5;
    }
  else
    {  
    delt=-0.5;
    }
  t1=x1;
  t2=(float)(xd)/abs(yd);
  i=yy1-k;
  switch (color_depth)
    {
    case 24:
    case 32:
    while(i!=y2)
      {
      i+=k;
      j=t1+delt;
      ia=j+i*(screen_width);
      mempix_char[4*ia  ]=svga_palette[3*c  ];
      mempix_char[4*ia+1]=svga_palette[3*c+1];
      mempix_char[4*ia+2]=svga_palette[3*c+2];
      t1+=t2;
      }
    break;
    
    case 16:
    ipalette=(unsigned short int*)(&svga_palette[0]);
    while(i!=y2)
      {
      i+=k;
      j=t1+delt;
      ia=j+i*(screen_width);
      mempix_shi[ia  ]=ipalette[c  ];
      t1+=t2;
      }
    break;

    case 8:
    while(i!=y2)
      {
      i+=k;
      j=t1+delt;
      ia=j+i*(screen_width);
      mempix_char[ia  ]=c;
      t1+=t2;
      }
    break;
    }    
  }  
}

void fclear_screen(void)
{
int i, k;
switch (color_depth)
  {
  case 24:
  case 32:
  k=4*screen_width*screen_height;
  for(i=0; i<k; i+=4)
    {
    mempix_char[i]=0;
    mempix_char[i+1]=0;
    mempix_char[i+2]=0;
    mempix_char[i+3]=255;
    }
  break;

  case 16:
  k=screen_width*screen_height;
  for(i=0; i<k; i++)
    {
    mempix_shi[i]=0;
    }
  break;
  
  case 8:
  k=screen_width*screen_height;
  for(i=0; i<k; i++)
    {
    mempix_char[i]=0;
    }
  }
lir_refresh_screen();
}

void flir_getpalettecolor(int j, int *r, int *g, int *b)
{
int k;
unsigned short int *ipalette;
switch (color_depth)
  {
  case 24:
  case 32:
  b[0]=(int)(svga_palette[3*j  ])>>2;
  g[0]=(int)(svga_palette[3*j+1])>>2;
  r[0]=(int)(svga_palette[3*j+2])>>2;
  break;

  case 16:
  ipalette=(unsigned short int*)(&svga_palette[0]);
  k=ipalette[j];
  k<<=1;
  b[0]=k&0x3f;
  k>>=6;
  g[0]=k&0x3f;
  k>>=5;
  r[0]=k;
  break;

  case 8:
  b[0]=(int)(svga_palette[3*j  ]);
  g[0]=(int)(svga_palette[3*j+1]);
  r[0]=(int)(svga_palette[3*j+2]);
  break;
  }
}

void flir_setpixel(int x, int y, unsigned char c)
{
int ia;
unsigned short int *ipalette;
ia=x+y*screen_width;
if(ia < 0 || ia >= screen_totpix)
  {
  setpixel_error(x, y);;
  return;
  }
switch (color_depth)
  {
  case 24:
  case 32:
  mempix_char[4*ia  ]=svga_palette[3*c  ];
  mempix_char[4*ia+1]=svga_palette[3*c+1];
  mempix_char[4*ia+2]=svga_palette[3*c+2];
  break;
  
  case 16:
  ipalette=(unsigned short int*)(&svga_palette[0]);
  mempix_shi[ia]=ipalette[c];
  break;

  case 8:
  mempix_char[ia]=c;
  break;
  }
}

int flir_getpixel(int x, int y)
{
int ia;
int c;
unsigned short int *ipalette;
c=-1;
ia=x+y*screen_width;
if(ia < 0 || ia >= screen_totpix)
  {
  DEB"\nwrite: x=%d   y=%d",x,y);
  getpixel_error(x, y);;
  return 0;
  }
switch (color_depth)
  {
  case 24:
  case 32:
nxt24:
  c++;  
  if(c >= MAX_SVGA_PALETTE)
    {
lirerr(223232);
    return 0;
    }
  if(mempix_char[4*ia  ]!=svga_palette[3*c  ])goto nxt24;
  if(mempix_char[4*ia+1]!=svga_palette[3*c+1])goto nxt24;
  if(mempix_char[4*ia+2]!=svga_palette[3*c+2])goto nxt24;
  break;
  
  case 16:
  ipalette=(unsigned short int*)(&svga_palette[0]);
nxt16:
  c++;  
  if(c >= MAX_SVGA_PALETTE)
    {
    lirerr(842318);
    return 0;
    }
  if(mempix_shi[ia]!=ipalette[c])goto nxt16;
  break;

  case 8:
  c=mempix_char[ia];
  break;
  }
return (char)c;  
}
