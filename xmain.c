
// Copyright (c) <2012> <Leif Asbrink>
//
// Permission is hereby granted, free of charge, to any person 
// obtaining a copy of this software and associated documentation 
// files (the "Software"), to deal in the Software without restriction, 
// including without limitation the rights to use, copy, modify, 
// merge, publish, distribute, sublicense, and/or sell copies of 
// the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be 
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
// OR OTHER DEALINGS IN THE SOFTWARE.


// For help on X11 give command: "man X Interface"
// Event masks and event definitions are in /usr/X11R6/include/X11/X.h

//#define UNINIT_MEMDATA 0xf7
#define UNINIT_MEMDATA 0
#include "osnum.h"

#if HAVE_SVGALIB == 1
#include <vga.h>
#include <vgagl.h>
#include <vgamouse.h>
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
#include <locale.h>
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
#endif


#include "globdef.h"
#include "thrdef.h"
#include "uidef.h"
#include "screendef.h"
#include "vernr.h"
#include "options.h"
#include "keyboard_def.h"
#include "xdef.h"
#include "lscreen.h"

#if HAVE_SHM == 1
extern XShmSegmentInfo *shminfo;
int ShmMajor,ShmMinor;
Bool ShmPixmaps;
#endif


#if DO_NOT_USE_XKBLIB == TRUE 
#define USE_XKBLIB 0
#else
#if HAVE_XKBLIB == 1
#include <X11/XKBlib.h>
#define USE_XKBLIB 1
#endif
#endif

#if HAVE_X11
extern GC xgc;
extern XImage *ximage;
extern Display *xdis;
extern Window xwin;
extern Colormap lir_colormap;
// We want to know about the user clicking on the close window button
Atom wm_delete_window;
#endif

int newcomer_escflag;

typedef struct {
unsigned short int red;
unsigned short int green;
unsigned short int blue;unsigned int pixel;
short int flag;
float total;
}PIXINFO;

#define LINRAD "Linrad"
char screen_filename[]="par_linux_screen";
char *screen_types[4]={"X11","MIT-SHM","SVGALIB","FBDEV"};

#if CPU == CPU_ARM
void outsb(void){};
#endif


int xmain( char **argv);
int lmain(void);
int fmain(void);

void install_instruct(char *pkg, char *hlp)
{
int i;
#if IA64 == 0
i=32;
#else
i=64;
#endif
printf("\e[1;31m");
printf("\n\nThe development environment for %s is not installed.",pkg);
#if BUILD == 0
printf("\nFor instructions, run ./configure --with-%s-%d",hlp,i);
printf("\n\nAfter the installation, run ./configure again.");
#if IA64 == 0
printf("\nFinally run make xlinrad");
#else
printf("\nFinally run make xlinrad64)"); 
#endif
#else
printf("\nFor instructions, run this command: ../configure --with-%s-%d",hlp,i);
printf("\n\nAfter the installation, run cmake .. ");
printf("\nFinally run make"); 
#endif
printf("\e[1;0m");
printf("\nYou may try another screen\n\n");
exit(10);
}

void firstinfo(void)
{
printf("\n\nYou can open a display for Linrad on another machine by");
printf("\nspecifying hostname:server_number.screen_number as an argument");
printf("\nto Linrad when you are running under X11.");
printf("\nRead about XOpenDisplay for info");
printf("\n\nAlternatively you can specify what graphics to use by giving");
printf("\nthe graphics type as an argument to Linrad.");
printf("\nGraphics types in linrad:\n");
}

int main(int argc, char **argv)
{
int i, j, k, m, i1, i2, bytes;
int info_flag;
int mode_x11;
(void) argc;
char s[80];
setlocale(LC_ALL,"POSIX");
FILE *file;
char *tmp;
info_flag=TRUE;
#if UNINIT_MEMDATA != 0
  {
  char *tmp;
  tmp=(char*)(&uninit_mem_begin);
  k=(char*)&mirics-tmp;
  for(i=0; i<k; i++)tmp[i]=UNINIT_MEMDATA;
  }
#endif  
tmp=(char *)getenv("DISPLAY");
if(tmp == NULL)
  {
  mode_x11=FALSE;
  }
else
  {
  mode_x11=TRUE;
  }  
file = fopen(screen_filename, "rb");
tmp=NULL;
if (file == NULL)
  {
screen_setup:;  
  printf("\n\nWELCOME TO LINRAD.");
  if(mode_x11)
    {
    printf("\nYou are running under X11");
    printf("\n\nA => X11");
    printf("\nB => X11 with MIT SHM");
    printf("\nC => svgalib");
    printf("\nfbdev available in terminal mode only");
    i1=0;
    i2=2;
    }
  else
    {
    printf("\nYou are running in terminal mode");
    printf("\n\nC => svgalib");
    printf("\nD => fbdev");
    i1=2;
    i2=3;
    }
get_screen:;
  printf("\n\nSelect your graphics from %c to %c => ",'A'+i1, 'A'+i2);
  while(fgets(s,8,stdin)==NULL);
  i=toupper(s[0]);
  i-=(int)'A';
  if(i < i1 || i > i2)
    {
    printf("\nNot a valid choice, try again");
    goto get_screen; 
    }
  if(i == 0 || i == 1)
    {
#if HAVE_X11 == 1
    firstinfo();
    info_flag=FALSE;
    for(j=0; j<4; j++)printf("\n%s",screen_types[j]);
    printf("\n\n    press ENTER to continue and save selection.");
    while(fgets(s,8,stdin)==NULL);
#else
    install_instruct("X11", "x11");
    if(info_flag)exit(10);
#endif
    }
  if(i == 1)
    {
#if HAVE_SHM == 1
    if(info_flag)firstinfo();
    info_flag=FALSE;
#else    
    install_instruct("MIT-SHM", "xext");
    if(info_flag)exit(10);
#endif
    }
  if(i == 2 || i == 3)
    {
#if HAVE_SVGALIB == 1
    if(info_flag)firstinfo();
    info_flag=FALSE;
    for(j=0; j<4; j++)printf("\n%s",screen_types[j]);
    printf("\n\n    press ENTER to continue and save selection.");
    while(fgets(s,8,stdin)==NULL);
#else
    install_instruct("svgalib", "svgalib");
    if(info_flag)exit(10);
#endif
    }
  file = fopen(screen_filename, "w");
  fprintf(file,"%s",screen_types[i]);
  fclose(file);
  }
else
  {
  if(tmp==NULL)
    {
    tmp=malloc(4096);
    if(tmp==NULL)
      {
      fclose(file);
      lirerr(1078);
      return 10;
      }
    }
  for(i=0; i<4096; i++) tmp[i]=0;
  bytes=fread(tmp,1,4095,file);
  fclose(file);
  if(bytes == 0)goto screen_setup;
  i=0;
  while(tmp[i] && i<bytes)
    {
    tmp[i]=toupper(tmp[i]);
    i++;
    }
  j=0;
  i=-1;
  while(j < 4 && i == -1)
    {
    if(strcmp((tmp),screen_types[j])==0)i=j;
    j++;
    }
  if(i == -1)goto screen_setup;
  free(tmp);
  }
if(argv[1] != NULL)
  {  
  j=0;
  k=-1;
  while(j < 4 && k == -1)
    {
    m=0;
    while(screen_types[j][m] != 0 || argv[1][m] != 0)
      {
      if(toupper(argv[1][m]) != screen_types[j][m])goto diff;
      m++;
      }
    k=j;
diff:;
    j++;
    }
// If the user provided a different screen type:
  if(k != -1)
    {
    i=k;
    argv[1]=NULL;
    }
  }  
k=1;
while(i!=0)
  {
  i--;
  k*=2;
  }
screen_type=k;
if(mode_x11)
  {
  if(k == 3)
    {
    printf("\n\nYou can not select fbdev under X11!!");
    }
  }
else
  {    
  if(k < 2)
    {
    printf("\n\nYou can not select an X11 display in terminal mode!!");
    exit(0);
    } 
  }
#if (DUMPFILE == TRUE)
  {
  dmp = fopen("dmp", "w");
  dmp1 = fopen("dmp1", "w");
  DEB"\n******************************\n");
  }
#else
  {  
  dmp=NULL;
  }
#endif
switch (screen_type)
  {
  case SCREEN_TYPE_X11:
#if HAVE_X11 == 1
  i=xmain(argv);
#else
  install_instruct("X11", "x11");
#endif
  break;
  
  case SCREEN_TYPE_X11_SHM:
#if HAVE_X11 == 1
#if HAVE_SHM == 1
  i=xmain(argv);
#else
  install_instruct("MIT-SHM", "xext");
#endif
#else
  install_instruct("X11", "x11");
#endif
  break;
  
  case SCREEN_TYPE_SVGALIB:
#if HAVE_SVGALIB
  i=lmain();
#else
  install_instruct("svgalib", "svgalib");
#endif
  break;

  case SCREEN_TYPE_FBDEV:
#if HAVE_SVGALIB
  i=fmain();
#else
  install_instruct("svgalib", "svgalib");
#endif
  break;
  }
return i;
}

int xmain(char **argv)
{
#if HAVE_X11 == 1
int id,il,kd,kl;
int bitmap_pad;
float t1,t2;
PIXINFO *defpix, *lirpix;
float *pixdiff;
PIXINFO tmppix;
Colormap default_colormap;
char *hostname;
Visual *visual;
int i, k, m, screen_num;
Cursor cross_cursor;
unsigned short int *ipalette;
XColor xco;
XClassHint* classHint;
XInitThreads();
init_xlir();
for(i=0; i<MAX_LIREVENT; i++)
  {
  lir_event_cond[i]=(pthread_cond_t) PTHREAD_COND_INITIALIZER;
  }  
#if SERVER == 1
pthread_create(&thread_identifier_html_server,NULL,
                                           (void*)thread_html_server, NULL);
#endif
expose_event_done=FALSE;
first_mempix=0x7fffffff;
last_mempix=0;
shift_key_status=0;
init_os_independent_globals();
newcomer_escflag=FALSE;
serport=-1;
keyboard_buffer=malloc(KEYBOARD_BUFFER_SIZE*sizeof(int));
if (argv[1] == NULL)
  {
  hostname = NULL;
  }
else
  {
  hostname = argv[1];
  }
xdis = XOpenDisplay(hostname);
if (xdis == NULL)
  {
  fprintf(STDERR, "\nCan't open display: %s\n", hostname);
  return(10);
  }
ui_setup();
#if HAVE_SHM == 1
// test if the X11 server supports MIT-SHM
if(screen_type == SCREEN_TYPE_X11_SHM)
  {
  if(!XShmQueryVersion(xdis,&ShmMajor,&ShmMinor,&ShmPixmaps))
    {
    printf("\nThe parameter shm_mode in par_userint specifies MIT-SHM");
    printf("\nbut the X11 server does not support MIT-SHM \n");
    printf("Check your X11 configuration with the xdpyinfo command \n");
    printf("and try to enable MIT-SHM  by adding following lines\n");
    printf("to the /etc/X11/xorg.conf file :\n\n");
    printf("Section ""Extensions"" \n");
    printf("       Option ""MIT-SHM"" ""enable""  \n");
    printf("EndSection \n\n");
    goto shm_error;
    }
  }
#endif
visual=DefaultVisual(xdis, 0);
screen_num = DefaultScreen(xdis);

if(ui.screen_width_factor > 0)
  {
  screen_width = ui.screen_width_factor*DisplayWidth(xdis, screen_num)/100;
  screen_height = ui.screen_height_factor*DisplayHeight(xdis, screen_num)/100;
  }

else
  {
  screen_width = -ui.screen_width_factor;
  screen_height = -ui.screen_height_factor;
  if(screen_width > DisplayWidth(xdis, screen_num))
    {
    screen_width=DisplayWidth(xdis, screen_num);
    }
  if(screen_height > DisplayHeight(xdis, screen_num))
    {
    screen_height = DisplayHeight(xdis, screen_num);
    }
  }
screen_width &= -4;
screen_totpix=screen_width*(screen_height+1);
// *****************************************************************
// Set the variables Linrad uses to access the screen.


init_font(ui.font_scale);
if(lir_errcod != 0)goto exitmain;
xwin=XCreateSimpleWindow(xdis, 
                         RootWindow(xdis, 0),
                         0, 
                         0, 
                         (unsigned int)screen_width, 
                         (unsigned int)screen_height, 
                         1, 
                         0, 
                         0);
// We want to know about the user clicking on the close window button
classHint=XAllocClassHint();
if(classHint)
  {
  classHint->res_name = LINRAD;
  classHint->res_class = LINRAD;
  XSetClassHint(xdis, xwin, classHint);
  XFree(classHint);
  }
wm_delete_window = XInternAtom(xdis, "WM_DELETE_WINDOW", 0);
XSetWMProtocols(xdis,xwin, &wm_delete_window, 1);
cross_cursor = XCreateFontCursor(xdis, XC_diamond_cross);
// attach the icon cursor to our window
XDefineCursor(xdis, xwin, cross_cursor);
xgc=DefaultGC(xdis, 0);
color_depth = DefaultDepth(xdis, screen_num );
if(visual->class!=TrueColor && color_depth != 8)
  {
  printf("Unknown color type\n");
  exit(1);
  }
bitmap_pad=color_depth;  
switch (color_depth)
  {
  case 24:
  bitmap_pad=32;
  mempix_char=(unsigned char*)malloc((size_t)(screen_totpix+1)*4);
  for(i=0; i<(screen_totpix+1)*4;i++)mempix_char[i]=0;
// ******************************************************************
// Rearrange the palette. It was designed for svgalib under Linux
  for(i=0; i<3*256; i++)
    {
    svga_palette[i]=(unsigned char)(svga_palette[i]<<2);
    if(svga_palette[i] != 0) svga_palette[i]|=3;
    }
  break;
  
  case 16:
  mempix_shi=(unsigned short int*)malloc((size_t)(screen_totpix+1)*sizeof(unsigned short int));
  mempix_char=(unsigned char*)mempix_shi;
  for(i=0; i<screen_totpix+1; i++)mempix_shi[i]=0;
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
  mempix_char=(unsigned char*)malloc((size_t)(screen_totpix+1)+256);
  defpix=(PIXINFO*)malloc(sizeof(PIXINFO)*256);
  lirpix=(PIXINFO*)malloc(sizeof(PIXINFO)*256);
  pixdiff=(float*)malloc(256*256*sizeof(float));
  xpalette=&mempix_char[screen_totpix+1];
  for(i=0; i<(screen_totpix+1)+256; i++)mempix_char[i]=0;
  lir_colormap=XCreateColormap(xdis, xwin, visual, AllocAll);
  default_colormap = DefaultColormap(xdis, screen_num);
// Store the default colormap in defpix
  for(id=0; id<256; id++)
    {
    xco.pixel=(unsigned long)id;
    k=XQueryColor (xdis,default_colormap,&xco);  
    defpix[id].red=xco.red;
    defpix[id].green=xco.green;
    defpix[id].blue=xco.blue;
    defpix[id].flag=0;
    defpix[id].pixel=(unsigned int)id;
    defpix[id].total=(float)defpix[id].red*(float)defpix[id].red+
                     (float)defpix[id].green*(float)defpix[id].green+
                     (float)defpix[id].blue*(float)defpix[id].blue;
    }
// svga_palette uses the six lowest bits for the colour intensities.
// shift left by 10 to move our data to occupy the six highest bits.
// Store the svgalib palette.
  for(il=0; il<MAX_SVGA_PALETTE; il++)
    {
    lirpix[il].red=(short unsigned int)(svga_palette[3*il+2]<<2);
    lirpix[il].green=(short unsigned int)(svga_palette[3*il+1]<<2);
    lirpix[il].blue=(short unsigned int)(svga_palette[3*il  ]<<2);
    if(lirpix[il].red != 0)lirpix[il].red|=3;
    if(lirpix[il].green != 0)lirpix[il].green|=3;
    if(lirpix[il].blue != 0)lirpix[il].blue|=3;
    lirpix[il].red=(short unsigned int)(lirpix[il].red<<8);
    lirpix[il].green=(short unsigned int)(lirpix[il].green<<8);
    lirpix[il].blue=(short unsigned int)(lirpix[il].blue<<8);
    lirpix[il].pixel=(unsigned int)il;
    lirpix[il].flag=1;
    lirpix[il].total=(float)lirpix[il].red*(float)lirpix[il].red+
                     (float)lirpix[il].green*(float)lirpix[il].green+
                     (float)lirpix[il].blue*(float)lirpix[il].blue;
    }
  for(il=MAX_SVGA_PALETTE; il<256; il++)
    {
    lirpix[il].red=0;
    lirpix[il].green=0;
    lirpix[il].blue=0;
    lirpix[il].pixel=(unsigned int)il;
    lirpix[il].flag=0;
    }
#define M 0.00000001
#define N 0x100
// Sort lirpix in order of ascending total intensity.
  for(il=0; il<MAX_SVGA_PALETTE-1; il++)
    {
    t1=0;
    m=il;
    for(kl=il; kl<MAX_SVGA_PALETTE; kl++)
      {
      if(lirpix[kl].total > t1)
        {
        t1=lirpix[kl].total;
        m=kl;
        }
      }  
    tmppix=lirpix[il];
    lirpix[il]=lirpix[m];
    lirpix[m]=tmppix;
    }
// Compute the similarity between lirpix and defpix and store in a matrix.
  for(il=0; il<MAX_SVGA_PALETTE; il++)
    {
    for(id=0; id<256; id++)
      {
      t2=(float)(pow((float)(lirpix[il].red-defpix[id].red),2.0)+
                 pow((float)(lirpix[il].green-defpix[id].green),2.0)+
                 pow((float)(lirpix[il].blue-defpix[id].blue),2.0));
      pixdiff[id+il*256]=t2;
      }
    }  
// Reorder the default colormap for the diagonal elements
// of pixdiff (up to MAX_SVGA_PALETTE-1) to become as small
// as possible when stepping in the ascending order that
// lirpix currently is sorted in.
  for(il=0; il<MAX_SVGA_PALETTE; il++)
    {
    t1=BIGFLOAT;
    kd=0;
    for(id=0; id<256; id++)
      {
      if(pixdiff[id+il*256] < t1)
        {
        t1=pixdiff[id+il*256];
        kd=id;
        }
      }
    tmppix=defpix[il];
    defpix[il]=defpix[kd];
    defpix[kd]=tmppix;
    for(kl=0; kl<MAX_SVGA_PALETTE; kl++)
      {
      t1=pixdiff[kd+kl*256];
      pixdiff[kd+kl*256]=pixdiff[il+kl*256];
      pixdiff[il+kl*256]=t1;
      }
    }
  for(i=0; i<MAX_SVGA_PALETTE; i++)
    {
    xco.pixel=defpix[i].pixel;
    xco.red=lirpix[i].red;
    xco.green=lirpix[i].green;
    xco.blue=lirpix[i].blue;
    xco.flags=DoRed|DoGreen|DoBlue;
    xco.pad=0;
    k=XStoreColor(xdis, lir_colormap, &xco);  
    if(k==0)
      {
      printf("\nPalette failed\n");
      goto exitmain;
      }
    xpalette[lirpix[i].pixel]=(unsigned char)xco.pixel;
    }
  for(i=MAX_SVGA_PALETTE; i<256; i++)
    {
    xco.pixel=defpix[i].pixel;
    xco.red=defpix[i].red;
    xco.green=defpix[i].green;
    xco.blue=defpix[i].blue;
    xco.flags=DoRed|DoGreen|DoBlue;
    xco.pad=0;
    k=XStoreColor(xdis, lir_colormap, &xco);  
    if(k==0)
      {
      printf("\nPalette failed\n");
      goto exitmain;
      }
    xpalette[i]=(unsigned char)lirpix[i].pixel;
    }
  XSetWindowColormap(xdis, xwin, lir_colormap);
  free(defpix);
  free(lirpix);
  free(pixdiff);
  break;
  
  default:
  printf("\nUnknown color depth: %d\n",color_depth);
  goto exitmain;
  } 
if(screen_type != SCREEN_TYPE_X11_SHM)
  {
  ximage=XCreateImage(xdis, visual, 
          (unsigned int)color_depth, 
          ZPixmap, 
          0, 
          (char*)(mempix_char), 
          (unsigned int)screen_width, 
          (unsigned int)screen_height+1, bitmap_pad, 0);
  }
#if HAVE_SHM == 1
else
  { 
  shminfo=(XShmSegmentInfo *)malloc(sizeof(XShmSegmentInfo));
  memset(shminfo,0, sizeof(XShmSegmentInfo));
  ximage =XShmCreateImage(xdis, 
                          visual, 
                          (unsigned int)color_depth, 
                          ZPixmap,
                          NULL, 
                          shminfo, 
                          (unsigned int)screen_width, 
                          (unsigned int)screen_height+1 );
  shminfo->shmid=shmget(IPC_PRIVATE,
		        (size_t)(ximage->bytes_per_line*ximage->height),
		        IPC_CREAT|0777);
  shminfo->shmaddr = ximage->data = shmat (shminfo->shmid, 0, 0);
// replace  address in mempix_char and mempix_shi by shared memory address 
  switch (color_depth)
    {
    case 24:
    free(mempix_char);
    mempix_char=(unsigned char*)(ximage->data);
    break;
  
    case 16:
    free(mempix_char);
    mempix_char=(unsigned char*)(ximage->data);
    mempix_shi=(unsigned short int*)(ximage->data);
    break;

    case 8:
    free(mempix_char);
    mempix_char=(unsigned char *)(ximage->data);
    defpix=(PIXINFO*)malloc(sizeof(PIXINFO)*256);
    lirpix=(PIXINFO*)malloc(sizeof(PIXINFO)*256);
    pixdiff=(float*)malloc(256*256*sizeof(float));
    xpalette=&mempix_char[screen_totpix+1];
    for(i=0; i<(screen_totpix+1)+256; i++)mempix_char[i]=0;
    lir_colormap=XCreateColormap(xdis, xwin, visual, AllocAll);
    default_colormap = DefaultColormap(xdis, screen_num);
// Store the default colormap in defpix
    for(id=0; id<256; id++)
      {
      xco.pixel=(unsigned long)id;
      k=XQueryColor (xdis,default_colormap,&xco);  
      defpix[id].red=xco.red;
      defpix[id].green=xco.green;
      defpix[id].blue=xco.blue;
      defpix[id].flag=0;
      defpix[id].pixel=(unsigned int)id;
      defpix[id].total=(float)(defpix[id].red)*(float)defpix[id].red+
                       (float)(defpix[id].green)*(float)defpix[id].green+
                       (float)(defpix[id].blue)*(float)defpix[id].blue;
      } 
// svga_palette uses the six lowest bits for the colour intensities.
// shift left by 10 to move our data to occupy the six highest bits.
// Store the svgalib palette.
    for(il=0; il<MAX_SVGA_PALETTE; il++)
      {
      lirpix[il].red=(short unsigned int)(svga_palette[3*il+2]<<2);
      lirpix[il].green=(short unsigned int)(svga_palette[3*il+1]<<2);
      lirpix[il].blue=(short unsigned int)(svga_palette[3*il  ]<<2);
      if(lirpix[il].red != 0)lirpix[il].red|=3;
      if(lirpix[il].green != 0)lirpix[il].green|=3;
      if(lirpix[il].blue != 0)lirpix[il].blue|=3;
      lirpix[il].red=(short unsigned int)(lirpix[il].red<<8);
      lirpix[il].green=(short unsigned int)(lirpix[il].green<<8);
      lirpix[il].blue=(short unsigned int)(lirpix[il].blue<<8);
      lirpix[il].pixel=(unsigned int)il;
      lirpix[il].flag=1;
      lirpix[il].total= (float)(lirpix[il].red)*(float)lirpix[il].red+
                        (float)(lirpix[il].green)*(float)lirpix[il].green+
                        (float)(lirpix[il].blue)*(float)lirpix[il].blue;
      }
    for(il=MAX_SVGA_PALETTE; il<256; il++)
      {
      lirpix[il].red=0;
      lirpix[il].green=0;
      lirpix[il].blue=0;
      lirpix[il].pixel=(unsigned int)il;
      lirpix[il].flag=0;
      }
#define M 0.00000001
#define N 0x100
// Sort lirpix in order of ascending total intensity.
    for(il=0; il<MAX_SVGA_PALETTE-1; il++)
      {
      t1=0;
      m=il;
      for(kl=il; kl<MAX_SVGA_PALETTE; kl++)
        {
        if(lirpix[kl].total > t1)
          {
          t1=lirpix[kl].total;
          m=kl;
          }
        }  
      tmppix=lirpix[il];
      lirpix[il]=lirpix[m];
      lirpix[m]=tmppix;
      }
// Compute the similarity between lirpix and defpix and store in a matrix.
    for(il=0; il<MAX_SVGA_PALETTE; il++)
      {
      for(id=0; id<256; id++)
        {
        t2=(float)(pow((double)(lirpix[il].red-defpix[id].red),2.0)+
                   pow((double)((lirpix[il].green-defpix[id].green)),2.0)+
                   pow((double)((lirpix[il].blue-defpix[id].blue)),2.0));
        pixdiff[id+il*256]=t2;
        }
      }  
// Reorder the default colormap for the diagonal elements
// of pixdiff (up to MAX_SVGA_PALETTE-1) to become as small
// as possible when stepping in the ascending order that
// lirpix currently is sorted in.
    for(il=0; il<MAX_SVGA_PALETTE; il++)
      {
      t1=BIGFLOAT;
      kd=0;
      for(id=0; id<256; id++)
        {
        if(pixdiff[id+il*256] < t1)
          {
          t1=pixdiff[id+il*256];
          kd=id;
          }
        }
      tmppix=defpix[il];
      defpix[il]=defpix[kd];
      defpix[kd]=tmppix;
      for(kl=0; kl<MAX_SVGA_PALETTE; kl++)
        {
        t1=pixdiff[kd+kl*256];
        pixdiff[kd+kl*256]=pixdiff[il+kl*256];
        pixdiff[il+kl*256]=t1;
        }
      }
    for(i=0; i<MAX_SVGA_PALETTE; i++)
      {
      xco.pixel=defpix[i].pixel;
      xco.red=lirpix[i].red;
      xco.green=lirpix[i].green;
      xco.blue=lirpix[i].blue;
      xco.flags=DoRed|DoGreen|DoBlue;
      xco.pad=0;
      k=XStoreColor(xdis, lir_colormap, &xco);  
      if(k==0)
        {
        printf("\nPalette failed\n");
        goto exitmain;
        }
      xpalette[lirpix[i].pixel]=(unsigned char)xco.pixel;
      }
    for(i=MAX_SVGA_PALETTE; i<256; i++)
      {
      xco.pixel=defpix[i].pixel;
      xco.red=defpix[i].red;
      xco.green=defpix[i].green;
      xco.blue=defpix[i].blue;
      xco.flags=DoRed|DoGreen|DoBlue;
      xco.pad=0;
      k=XStoreColor(xdis, lir_colormap, &xco);  
      if(k==0)
        {
        printf("\nPalette failed\n");
        goto exitmain;
        }
      xpalette[i]=(unsigned char)lirpix[i].pixel;
      }
    XSetWindowColormap(xdis, xwin, lir_colormap);
    free(defpix);
    free(lirpix);
    free(pixdiff);
 
    break;
    }
  shminfo->readOnly = False;
  XShmAttach(xdis,shminfo);
  }
#endif
XInitImage(ximage);
XSelectInput(xdis, xwin, 
             ButtonPressMask|
             ExposureMask|
             KeyPressMask|
             KeyReleaseMask| 
             ButtonReleaseMask|
             PointerMotionMask|
             StructureNotifyMask);
XMapWindow(xdis, xwin);
// Create a thread that will close the entire process in a controlled way
// in case lirerr() is called or the ESC key is pressed.
lir_init_event(EVENT_KILL_ALL);
pthread_create(&thread_identifier_kill_all,NULL,(void*)thread_kill_all, NULL);
lir_init_event(EVENT_REFRESH_SCREEN);
pthread_create(&thread_identifier_refresh_screen,NULL,
                                          (void*)thread_refresh_screen, NULL);
lir_status=LIR_OK;
lir_init_event(EVENT_KEYBOARD);
lir_init_event(EVENT_SYSCALL);
process_event_flag=TRUE;
pthread_create(&thread_identifier_process_event,NULL,
                                        (void*)thread_process_event, NULL);
while(expose_event_done == FALSE)
  {
  lir_sleep(1000);
  }
lir_init_event(EVENT_MOUSE);
pthread_create(&thread_identifier_mouse,NULL, (void*)wxmouse, NULL);
// Create a thread for the main menu.
lir_init_event(EVENT_MANAGE_EXTIO);
users_open_devices();
if(kill_all_flag) goto skipmenu;
pthread_create(&thread_identifier_main_menu,NULL,
                                              (void*)thread_main_menu, NULL);
while(extio_command_flag != EXTIO_COMMAND_KILL_ALL)
  {
  lir_await_event(EVENT_MANAGE_EXTIO);
  switch(extio_command_flag)
    {
    case EXTIO_COMMAND_LOAD_DLL:
    extio_error=load_extio_library();
    break;
    
    case EXTIO_COMMAND_UNLOAD_DLL:
    unload_extio_library();
    break;
        
    case EXTIO_COMMAND_START:
    start_extio();
    break;
    
    case EXTIO_COMMAND_STOP:
    stop_extio();
    break;

    case EXTIO_COMMAND_KILL_ALL:
    goto kill;
    }
  extio_command_flag=EXTIO_COMMAND_DONE;
  }
kill:;
pthread_join(thread_identifier_main_menu,0);
skipmenu:;
refresh_screen_flag=FALSE;
lir_refresh_screen();
pthread_join(thread_identifier_refresh_screen,0);
lir_set_event(EVENT_KILL_ALL);
pthread_join(thread_identifier_kill_all,0);
lir_remove_mouse_thread();
pthread_join(thread_identifier_mouse,0);
//XSync(xdis,True);    ööööööö does never return in Ubuntu 11.04
pthread_kill(thread_identifier_process_event, 9);
process_event_flag=FALSE;
pthread_join(thread_identifier_process_event,0);
XFreeCursor(xdis, cross_cursor);
if(screen_type != SCREEN_TYPE_X11_SHM)
  {
  free(mempix_char);
  }
#if HAVE_SHM == 1
else
  { 
  XShmDetach(xdis,shminfo);
  shmdt(shminfo->shmaddr);
  shmctl(shminfo->shmid,IPC_RMID,NULL);
  free(shminfo);
  }
#endif 	
exitmain:;
users_close_devices();
lir_close_serport();
free(vga_font);
#if HAVE_SHM == 1
shm_error:;
#endif
XCloseDisplay(xdis);
free(keyboard_buffer);
if(dmp!=NULL)fclose(dmp);
if(dmp1!=NULL)fclose(dmp1);
#if SERVER == 1
pthread_join(thread_identifier_html_server,0);
#endif
lir_close_event(EVENT_KILL_ALL);
lir_close_event(EVENT_SYSCALL);
lir_close_event(EVENT_KEYBOARD);
lir_close_event(EVENT_MANAGE_EXTIO);
lir_close_event(EVENT_MOUSE);
lir_close_event(EVENT_REFRESH_SCREEN);
return lir_errcod;
#else
(void)argv;
return 10;
#endif
}

#if HAVE_X11
void thread_process_event(void)
{
int i, k, chr;
int cc, mx, my;
XEvent ev;
char key_buff[16];
int count, m;
KeySym ks;
int mbutton_state;
mbutton_state=0;
expose_time=current_time();
while(process_event_flag)
  {
  XNextEvent(xdis, &ev);
  switch(ev.type)
    {
// We want to know about the user clicking on the close window button
    case ClientMessage:
    if ( ev.xclient.data.l[0] == (int)(wm_delete_window) )
      {
      printf ( "Shutting down now!!!\n" );
      chr = X_ESC_SYM;
      lir_set_event(EVENT_KILL_ALL);
      store_in_kbdbuf(0);
      return;
      } 
    break;

    case Expose:
    expose_time=recent_time;
    if (ev.xexpose.count == 256)
      {
      if( thread_status_flag[THREAD_SCREEN] == THRFLAG_ACTIVE)goto expo;
      }
    if (ev.xexpose.count != 0)break;
    if( thread_status_flag[THREAD_SCREEN] != THRFLAG_SEM_WAIT &&
        thread_status_flag[THREAD_SCREEN] != THRFLAG_ACTIVE &&
        thread_status_flag[THREAD_SCREEN] != THRFLAG_IDLE)
      {
expo:;
      if(screen_type != SCREEN_TYPE_X11_SHM)
        {
        XPutImage(xdis, xwin, xgc, ximage,0,0,0,0, 
                (unsigned int)screen_width, (unsigned int)screen_height);
        }
#if HAVE_SHM == 1
      else
        { 
        XShmPutImage (xdis, xwin, xgc, ximage, 0, 0, 0, 0, 
          (unsigned int)screen_width, (unsigned int)screen_height, FALSE);
        }
#endif
      }
    if(color_depth==8)XInstallColormap(xdis, lir_colormap);
    expose_event_done=TRUE;
    break;

    case ButtonPress:
    if ( (ev.xbutton.button == Button1) != 0)
      {
      new_lbutton_state=1;
      goto mousepost;
      }
    if ( (ev.xbutton.button == Button3) != 0) 
      {
      new_rbutton_state=1;
      goto mousepost;
      }
    if ( (ev.xbutton.button == Button2) != 0)
      {
      mbutton_state=1;
      }
    if(mbutton_state==0)
      {  
      if ( (ev.xbutton.button == Button5) != 0)
        {
        step_rx_frequency(1);
        }
      if ( (ev.xbutton.button == Button4) != 0)
        {
        step_rx_frequency(-1);
        }
      }
    else
      {
      m=bg.wheel_stepn;
      if ( (ev.xbutton.button == Button5) != 0)
        {
        m++;
        if(m>30)m=30;
        }
      if ( (ev.xbutton.button == Button4) != 0)
        {
        m=bg.wheel_stepn;
        m--;
        if(m<-30)m=-30;
        if((genparm[AFC_ENABLE]==0 || genparm[AFC_LOCK_RANGE] == 0) && m<0)m=0;
        }
      bg.wheel_stepn=m;
      sc[SC_SHOW_WHEEL]++;
      make_modepar_file(GRAPHTYPE_BG);
      }
    break;

    case ButtonRelease:
    if ( (ev.xbutton.button == Button1) != 0)
      {
      new_lbutton_state=0;
      goto mousepost;
      }
    if ( (ev.xbutton.button == Button3) != 0) 
      {
      new_rbutton_state=0;
      goto mousepost;
      }
    if ( (ev.xbutton.button == Button2) != 0)
      {
      mbutton_state=0;
      }
    break;

    case MotionNotify:
    mx=new_mouse_x;
    my=new_mouse_y;
    new_mouse_x= ev.xbutton.x;
    if(new_mouse_x < 0)new_mouse_x=0;
    if(new_mouse_x >= screen_width)new_mouse_x=screen_width-1;
    new_mouse_y= ev.xbutton.y;
    if(new_mouse_y < 0)new_mouse_y=0;
    if(new_mouse_y >= screen_height)new_mouse_y=screen_height-1;
    if(  mx == new_mouse_x &&   my==new_mouse_y)break;
    if( (mx == new_mouse_x && new_mouse_x == screen_width-1) || 
        (my == new_mouse_y && new_mouse_y == screen_height-1)|| 
        (mx == new_mouse_x && new_mouse_x == 0) || 
        (my == new_mouse_y && new_mouse_y == 0) )break;
mousepost:;
    lir_set_event(EVENT_MOUSE);
    break;
  
    case KeyPress:
#if USE_XKBLIB == 1
    chr=(int)XkbKeycodeToKeysym(xdis, (KeyCode)ev.xkey.keycode, 0, 0);
#else      
    chr=(int)XKeycodeToKeysym(xdis, (KeyCode)ev.xkey.keycode, 0);
#endif
    if(newcomer_escflag == TRUE)
      {
      cc=toupper(chr);
      if(cc=='Y')goto escexit;
      if(cc!='N')break;
      newcomer_escpress(1);
      newcomer_escflag=FALSE;
      break;
      }
    if(chr == X_ESC_SYM)
      {
// The ESC key was pressed.
      if(ui.operator_skil == OPERATOR_SKIL_NEWCOMER)
        {
        newcomer_escpress(0);
        newcomer_escflag=TRUE;
        break;
        }
escexit:;
      lir_set_event(EVENT_KILL_ALL);
check_threads:;
      k=0; 
      for(i=0; i<THREAD_MAX; i++)    
        {
        if(i != THREAD_SYSCALL && 
           thread_command_flag[i] != THRFLAG_NOT_ACTIVE &&
           thread_waitsem[i] == -2)
          {
          k++;
          }   
        }
      if(k != 0)
        {
        lir_sleep(30000);
        goto check_threads;
        }  
      store_in_kbdbuf(0);
      return;
      }    
    if(chr == X_SHIFT_SYM_L || chr == X_SHIFT_SYM_R)
      {
      shift_key_status=1;
      break;
      }
    if(chr == X_ALT_SYM)
      {
      alt_key_status=1;
      break;
      }
// Get ASCII codes from Dec 32 to Dec 127~
    count = XLookupString((XKeyEvent *)&ev, key_buff, 2, &ks,NULL);
    if ((count == 1) && ((int)key_buff[0]>31) )
      {
      i=(int)key_buff[0];
      if(alt_key_status == 1)i=toupper(i)-'A'+ALT_A_KEY;
      store_in_kbdbuf(i);
      break;
      }
    cc=0;
    if(chr == X_NUM_ENTER_SYM)
      {
      store_in_kbdbuf(10);
      break;
      }
    if(chr >= X_F1_SYM && chr <= X_F12_SYM)
      {
      if(chr <= X_F10_SYM)
        {
        if(shift_key_status == 0)
          {
          cc=F1_KEY+chr-X_F1_SYM;
          }
        else  
          {
          cc=SHIFT_F1_KEY+chr-X_F1_SYM;
          if(chr > X_F2_SYM)cc++;
          if(chr >= X_F9_SYM)cc=0;
          }
        }
      else
        {
        if(shift_key_status == 0)
          {
          cc=F11_KEY+chr-X_F11_SYM;
          }
        }  
      if(cc != 0)store_in_kbdbuf(cc);        
      break;
      }
    switch (chr)
      {
      case X_UP_SYM:
      cc=ARROW_UP_KEY;
      break;

      case X_DWN_SYM:
      cc=ARROW_DOWN_KEY;
      break;

      case X_RIGHT_SYM:
      cc=ARROW_RIGHT_KEY;
      break;

      case  X_BACKDEL_SYM:
      case X_LEFT_SYM:
      cc=ARROW_LEFT_KEY;
      break;
      
      case X_HOME_SYM:
      cc=HOME_KEY;
      break;
      
      case X_INSERT_SYM:
      cc=INSERT_KEY;
      break;
      
      case X_DELETE_SYM:
      cc=DELETE_KEY;
      break;
      
      case X_END_SYM:
      cc=END_KEY;
      break;
      
      case X_PGUP_SYM:
      cc=PAGE_UP_KEY;
      break;

      case X_PGDN_SYM:
      cc=PAGE_DOWN_KEY;
      break;
      
      case X_PAUSE_SYM:
      cc=PAUSE_KEY;
      break;
      
      case X_ENTER_SYM:
      cc=10;
      break;

      }
    if(cc != 0)store_in_kbdbuf(cc);        
    break;

    case KeyRelease:
#if USE_XKBLIB == 1
    chr=(int)XkbKeycodeToKeysym(xdis, (KeyCode)ev.xkey.keycode, 0, 0);
#else      
    chr=(int)XKeycodeToKeysym(xdis, (KeyCode)ev.xkey.keycode, 0);
#endif
    if(chr == X_ALT_SYM)
      {
      alt_key_status=0;
      break;
      }
    if(chr == X_SHIFT_SYM_L || chr == X_SHIFT_SYM_R)
      {
      shift_key_status=0;
      }
    break;        

    case ConfigureNotify:
    if(ev.xconfigure.width != screen_width ||
       ev.xconfigure.height != screen_height)
      {
      fprintf( stderr,"\n\nSCREEN RESIZE NOT SUPPORTED");
      fprintf( stderr,
      "\nIf you want a different screen size you must set the new size");
      fprintf( stderr,"\nwith S=Global parms set up on the main menu.");
      fprintf( stderr,"\nYou can also edit par_userint.");
      fprintf( stderr,"\nIn both cases exit and restart Linrad.");
      } 
      break;
    }
  }
}

void ui_setup(void)
{
size_t bytes;
FILE *file;
int i, j, k;
int xxprint;
char s[10];
char chr;
int *uiparm;
char *parinfo;
uiparm=(int*)(&ui);
parinfo=NULL;
#if DARWIN == 0
xxprint=investigate_cpu();
#else
xxprint=0;
simd_present=0;
mmx_present=0;
no_of_processors=1;
#endif
file = fopen(userint_filename, "rb");
if (file == NULL)
  {
  printf("\n\nWELCOME TO LINRAD");
  printf("\nThis message is not an error, but an indication that setup");
  printf("\nhas not yet been done.");
  print_procerr(xxprint);
  printf("\n\nSetup file %s missing.",userint_filename);
full_setup:;
  for(i=0; i<MAX_UIPARM; i++) uiparm[i]=0;
  welcome_msg();
  while(fgets(s,8,stdin)==NULL);
  chr=(char)toupper((int)s[0]);
  if(chr != 'S' && chr != 'N' && chr != 'E') exit(0);
  if(chr == 'N')
    {
    ui.operator_skil=OPERATOR_SKIL_NEWCOMER;
    }
  else
    {
    if(chr == 'S')
      {
      ui.operator_skil=OPERATOR_SKIL_NORMAL;
      }
    else
      {
      ui.operator_skil=OPERATOR_SKIL_EXPERT;
      }
    }
  x_global_uiparms(0);
  ui.rx_input_mode=-1;
  ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
  ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
  ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
  ui.max_dma_rate=DEFAULT_MAX_DMA_RATE;
  ui.min_dma_rate=DEFAULT_MIN_DMA_RATE;
  ui.rx_soundcard_radio=RX_SOUNDCARD_RADIO_UNDEFINED;
  ui.converter_mhz=0;
  ui.converter_hz=0;
  uiparm_change_flag=TRUE;
  }
else
  {
  if(parinfo==NULL)
    {
    parinfo=malloc(4096);
    if(parinfo==NULL)
      {
      fclose(file);
      lirerr(1078);
      return;
      }
    }
  for(i=0; i<4096; i++) parinfo[i]=0;
  bytes=fread(parinfo,1,4095,file);
  fclose(file);
  file=NULL;
  if(bytes >= 4095)
    {
    goto go_full_setup;
    }
  k=0;
  for(i=0; i<MAX_UIPARM; i++)
    {
    while(parinfo[k]==' ' ||
          parinfo[k]== '\n' )k++;
    j=0;
    while(parinfo[k]== uiparm_text[i][j] && k<4096)
      {
      k++;
      j++;
      } 
    if(uiparm_text[i][j] != 0)goto go_full_setup;
    while(parinfo[k]!='[' && k<4096)k++;
    sscanf(&parinfo[k],"[%d]",&uiparm[i]);
    while(parinfo[k]!='\n')k++;
    }
  if(ui.screen_width_factor*ui.screen_height_factor <=0)goto go_full_setup;
  if(ui.screen_width_factor > 0)
    {
    if(ui.screen_width_factor < 25 || 
      ui.screen_width_factor > 100 ||
      ui.screen_height_factor <25 || 
      ui.screen_height_factor > 100)goto go_full_setup;
    }
  else
    {
    if(ui.screen_width_factor < -10000 || 
      ui.screen_width_factor > -500 ||
      ui.screen_height_factor <-10000 || 
      ui.screen_height_factor > -400)goto go_full_setup;
    }
  if( ui.font_scale < 1 || 
      ui.font_scale > 5 ||
      ui.check != UI_VERNR ||
      ui.operator_skil < OPERATOR_SKIL_NEWCOMER ||
      ui.operator_skil > OPERATOR_SKIL_EXPERT ||
      ui.rx_soundcard_radio < RX_SOUNDCARD_RADIO_UNDEFINED ||
      ui.rx_soundcard_radio >= MAX_RX_SOUNDCARD_RADIO)
    {
go_full_setup:;
    printf("\n\nSetup file %s has errors",userint_filename);
    goto full_setup;
    }
  uiparm_change_flag=FALSE;
  free(parinfo);
  }
for(i=0;i<3; i++)svga_palette[i]=(char)3*ui.bkg_color;  
}
#endif


int lmain(void)
{
#if HAVE_SVGALIB == 1
struct MouseCaps caps;
int i,ir,j,k;
char *parinfo;
int xxprint;
FILE *file;
int *uiparm;
init_llir();
// Init svgalib and set it to text mode.
vga_init();
// Permissions to write to the hardware have to be set
// before any new threads are started. Reason unknown........
init_os_independent_globals();
serport=-1;
// Allocate buffers for keyboard and mouse, then start their threads.
keyboard_buffer=malloc(KEYBOARD_BUFFER_SIZE*sizeof(int)+
                     4*(MAX_MOUSE_CURSIZE+1)*sizeof(char));
behind_mouse=(char*)keyboard_buffer+KEYBOARD_BUFFER_SIZE*sizeof(int);
// Set the flags that tell whether daughter processes are running.
#ifdef THREAD_TIMERS
for(i=0; i<THREAD_MAX; i++)
  {
  thread_workload[i]=0;
  thread_tottim1[i]=0;
  thread_cputim1[i]=0;
  thread_tottim2[i]=0;
  thread_cputim2[i]=0;
  }
#endif
lir_init_event(EVENT_KEYBOARD);
lir_init_event(EVENT_SYSCALL);
pthread_create(&thread_identifier_keyboard,NULL,(void*)thread_keyboard, NULL);
pthread_create(&thread_identifier_mouse,NULL,(void*)lthread_mouse, NULL);
// Create a thread that will close the entire process in a controlled way
// in case lirerr() is called or the ESC key is pressed.
lir_init_event(EVENT_KILL_ALL);
pthread_create(&thread_identifier_kill_all,NULL,(void*)thread_kill_all, NULL);
uiparm=(int*)(&ui);
screen_type=SCREEN_TYPE_SVGALIB;
#if DARWIN == 0
xxprint=investigate_cpu();
#else
xxprint=0;
simd_present=0;
mmx_present=0;
no_of_processors=1;
#endif
file = fopen(userint_filename, "rb");
if (file == NULL)
  {
  printf("\n\nWELCOME TO LINRAD");
  printf("\nThis message is not an error, but an indication that setup");
  printf("\nhas not yet been done.");
  print_procerr(xxprint);
  printf("\nSetup file %s missing.",userint_filename);
full_setup:;
  for(i=0; i<MAX_UIPARM; i++) uiparm[i]=0;
  welcome_msg();
  to_upper_await_keyboard();
  if(lir_inkey != 'S' && lir_inkey != 'N' && lir_inkey != 'E') goto skip;
  if(lir_inkey == 'N')
    {
    ui.operator_skil=OPERATOR_SKIL_NEWCOMER;
    }
  else
    {
    if(lir_inkey == 'S')
      {
      ui.operator_skil=OPERATOR_SKIL_NORMAL;
      }
    else
      {
      ui.operator_skil=OPERATOR_SKIL_EXPERT;
      }
    }
  llin_global_uiparms(0);
  if(kill_all_flag) goto text_exitmain;
  uiparm_change_flag=TRUE;
  ui.rx_input_mode=-1;
  ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
  ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
  ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
  ui.max_dma_rate=DEFAULT_MAX_DMA_RATE;
  ui.min_dma_rate=DEFAULT_MIN_DMA_RATE;
  ui.rx_soundcard_radio=RX_SOUNDCARD_RADIO_UNDEFINED;
  ui.converter_mhz=0;
  ui.converter_hz=0;
  }
else
  {
  parinfo=malloc(4096);
  if(parinfo == NULL)
    {
    lir_errcod=1078;
    goto text_exitmain;
    }
  for(i=0; i<4096; i++) parinfo[i]=0;
  ir=(int)fread(parinfo,1,4095,file);
  fclose(file);
  file=NULL;
  if(ir >= 4095)
    {    
    goto go_full_setup;
    }
  k=0;
  for(i=0; i<MAX_UIPARM; i++)
    {
    while(parinfo[k]==' ' ||
          parinfo[k]== '\n' )k++;
    j=0;
    while(parinfo[k]== uiparm_text[i][j])
      {
      k++;
      j++;
      } 
    if(uiparm_text[i][j] != 0)goto go_full_setup;
    while(parinfo[k]!='[' && k< ir)k++;
    sscanf(&parinfo[k],"[%d]",&uiparm[i]);
    while(parinfo[k]!='\n' && k< ir)k++;
    }
  if( ui.check != UI_VERNR ||
      ui.operator_skil < OPERATOR_SKIL_NEWCOMER ||
      ui.operator_skil > OPERATOR_SKIL_EXPERT ||
      ui.rx_soundcard_radio < RX_SOUNDCARD_RADIO_UNDEFINED ||
      ui.rx_soundcard_radio >= MAX_RX_SOUNDCARD_RADIO)
    {
go_full_setup:;    
    vga_setmode(TEXT);
    printf("\n\nSetup file %s has errors",userint_filename);
    parinfo=chk_free(parinfo);
    goto full_setup;
    }
  if (vga_hasmode(ui.vga_mode)==0)goto go_full_setup;
  lgraphics_init();
  if(lir_errcod != 0)goto text_exitmain;
  uiparm_change_flag=FALSE;
  }
vga_setmousesupport(1);
mouse_setxrange(0, WIDTH-1);
mouse_setyrange(0, HEIGHT-1);
mouse_setwrap(MOUSE_NOWRAP);
lir_status=LIR_OK;
// Set up mouse and system parameters in case it has not been done already.
mouse_cursize=HEIGHT/80;
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
//extio_error=load_extio_library();  
lir_init_event(EVENT_MANAGE_EXTIO);
users_open_devices();
if(kill_all_flag) goto exitmain;
pthread_create(&thread_identifier_main_menu,NULL,
                                              (void*)thread_main_menu, NULL);
usleep(50000);
pthread_join(thread_identifier_main_menu,0);
exitmain:;
//unload_extio_library();
clear_keyboard();
text_exitmain:;
show_errmsg(0);
vga_setmode(TEXT);
// Set text mode again. Helps lap top computers that otherwise
// fail to restore text mode.
vga_setmode(TEXT);
users_close_devices();
lir_close_serport();
pthread_cancel(thread_identifier_mouse);
pthread_join(thread_identifier_mouse,0);
lir_sleep(20000);
pthread_cancel(thread_identifier_keyboard);
pthread_join(thread_identifier_keyboard,0);
skip:;
printf("\n\nLeaving %s\n",PROGRAM_NAME);
if(dmp!=NULL)fclose(dmp);
lir_close_event(EVENT_KILL_ALL);
lir_close_event(EVENT_SYSCALL);
lir_close_event(EVENT_KEYBOARD);
lir_close_event(EVENT_MANAGE_EXTIO);
return lir_errcod;
#else
return 10;
#endif
}

int fmain(void)
{
#if HAVE_SVGALIB == 1
int i,ir,j,k;
int xxprint;
char *parinfo;
FILE *file;
int *uiparm;
init_flir();
i=system("reset");
terminal_flag=FALSE;
fb_palette=NULL;
gpm_fd=-1;
parinfo=(char *)getenv("TERM");
if(strncmp(parinfo,"xterm",5) == 0)
  {
  printf("par_linux_screen specifies FBDEV.\n");
  printf("Change or specify X11 or MIT-SHM on the command line.\n\n");
  exit(0);
  }
// put something nonzero in scratch memory to make sure we not rely on 
// it being zero when starting (this may help us detect bugs).
// Init svgalib and set it to text mode.
vga_simple_init();
// Permissions to write to the hardware have to be set
// before any new threads are started. Reason unknown........
init_os_independent_globals();
serport=-1;
// Allocate buffers for keyboard and mouse, then start their threads.
keyboard_buffer=malloc(KEYBOARD_BUFFER_SIZE*sizeof(int)+
                     4*(MAX_MOUSE_CURSIZE+1)*sizeof(char));
behind_mouse=(char*)keyboard_buffer+KEYBOARD_BUFFER_SIZE*sizeof(int);
// Set the flags that tell whether daughter processes are running.
#ifdef THREAD_TIMERS
for(i=0; i<THREAD_MAX; i++)
  {
  thread_workload[i]=0;
  thread_tottim1[i]=0;
  thread_cputim1[i]=0;
  thread_tottim2[i]=0;
  thread_cputim2[i]=0;
  }
#endif
lir_init_event(EVENT_KEYBOARD);
lir_init_event(EVENT_SYSCALL);
pthread_create(&thread_identifier_keyboard,NULL,(void*)thread_keyboard, NULL);
pthread_create(&thread_identifier_mouse,NULL,(void*)lthread_mouse, NULL);
// Create a thread that will close the entire process in a controlled way
// in case lirerr() is called or the ESC key is pressed.
lir_init_event(EVENT_KILL_ALL);
pthread_create(&thread_identifier_kill_all,NULL,(void*)thread_kill_all, NULL);
uiparm=(int*)(&ui);
#if DARWIN == 0
xxprint=investigate_cpu();
#else
xxprint=0;
simd_present=0;
mmx_present=0;
no_of_processors=1;
#endif
file = fopen(userint_filename, "rb");
if (file == NULL)
  {
  printf("\n\nWELCOME TO LINRAD");
  printf("\nThis message is not an error, but an indication that setup");
  printf("\nhas not yet been done.");
  print_procerr(xxprint);
  printf("\nSetup file %s missing.",userint_filename);
full_setup:;
  for(i=0; i<MAX_UIPARM; i++) uiparm[i]=0;
  welcome_msg();
  to_upper_await_keyboard();
  if(lir_inkey != 'S' && lir_inkey != 'N' && lir_inkey != 'E') goto skip;
  if(lir_inkey == 'N')
    {
    ui.operator_skil=OPERATOR_SKIL_NEWCOMER;
    }
  else
    {
    if(lir_inkey == 'S')
      {
      ui.operator_skil=OPERATOR_SKIL_NORMAL;
      }
    else
      {
      ui.operator_skil=OPERATOR_SKIL_EXPERT;
      }
    }
  flin_global_uiparms(0);
  if(kill_all_flag) goto exitmain;
  uiparm_change_flag=TRUE;
  ui.rx_input_mode=-1;
  ui.tx_dadev_no=UNDEFINED_DEVICE_CODE;
  ui.rx_dadev_no=UNDEFINED_DEVICE_CODE;
  ui.rx_addev_no=UNDEFINED_DEVICE_CODE;
  ui.max_dma_rate=DEFAULT_MAX_DMA_RATE;
  ui.min_dma_rate=DEFAULT_MIN_DMA_RATE;
  ui.rx_soundcard_radio=RX_SOUNDCARD_RADIO_UNDEFINED;
  ui.converter_mhz=0;
  ui.converter_hz=0;
  }
else
  {
  parinfo=malloc(4096);
  if(parinfo == NULL)
    {
    lir_errcod=1078;
    goto exitmain;
    }
  for(i=0; i<4096; i++) parinfo[i]=0;
  ir=(int)fread(parinfo,1,4095,file);
  fclose(file);
  file=NULL;
  if(ir >= 4095)
    {    
    goto go_full_setup;
    }
  k=0;
  for(i=0; i<MAX_UIPARM; i++)
    {
    while(parinfo[k]==' ' ||
          parinfo[k]== '\n' )k++;
    j=0;
    while(parinfo[k]== uiparm_text[i][j])
      {
      k++;
      j++;
      } 
    if(uiparm_text[i][j] != 0)goto go_full_setup;
    while(parinfo[k]!='[' && k< ir)k++;
    sscanf(&parinfo[k],"[%d]",&uiparm[i]);
    while(parinfo[k]!='\n' && k< ir)k++;
    }
  if( ui.check != UI_VERNR ||
      ui.operator_skil < OPERATOR_SKIL_NEWCOMER ||
      ui.operator_skil > OPERATOR_SKIL_EXPERT ||
      ui.rx_soundcard_radio < RX_SOUNDCARD_RADIO_UNDEFINED ||
      ui.rx_soundcard_radio >= MAX_RX_SOUNDCARD_RADIO)
    {
go_full_setup:;    
    printf("\n\nSetup file %s has errors",userint_filename);
    parinfo=chk_free(parinfo);
    goto full_setup;
    }
  fgraphics_init();
  init_font(ui.font_scale);
  if(lir_errcod != 0)goto exitmain;
  uiparm_change_flag=FALSE;
  }
//extio_error=load_extio_library();  
lir_init_event(EVENT_MANAGE_EXTIO);
users_open_devices();
if(kill_all_flag) goto exitmain;
pthread_create(&thread_identifier_main_menu,NULL,
                                              (void*)thread_main_menu, NULL);
file=freopen( "stderr.log", "w", stderr );
usleep(50000);

lir_inkey=0;

pthread_join(thread_identifier_main_menu,0);
exitmain:;
//unload_extio_library();
if(terminal_flag)
  {
  clear_keyboard();
  tcsetattr(0, TCSANOW, &termios_pp);
  }
if(gpm_fd>=0) close(gpm_fd);
printf( "\e[?25h" );	// restore cursor visibility 
users_close_devices();
lir_close_serport();
pthread_cancel(thread_identifier_mouse);
pthread_join(thread_identifier_mouse,0);
lir_sleep(20000);
pthread_cancel(thread_identifier_keyboard);
pthread_join(thread_identifier_keyboard,0);
show_errmsg(0);
skip:;
printf("\nLeaving %s\n",PROGRAM_NAME);
if(dmp!=NULL)fclose(dmp);
munmap(mempix_char, framebuffer_screensize);
if(framebuffer_handle)close(framebuffer_handle);
if(fb_palette)free(fb_palette);
lir_close_event(EVENT_KILL_ALL);
lir_close_event(EVENT_SYSCALL);
lir_close_event(EVENT_KEYBOARD);
lir_close_event(EVENT_MANAGE_EXTIO);
return lir_errcod;
#else
return 10;
#endif
}

