/* From VGAlib, changed for svgalib */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */
// Part of the svgalib package.
// Included in Linrad with stdlib and a return value from main
// to compile without warnings or errors.



#include <vga.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static unsigned char line[2048 * 3];
static int lirflag;

#if CPU == CPU_ARM
void outsb(void){};
#endif


static void testmode(int mode)
{
    int xmax, ymax, i, x, y, yw, ys, c;
    vga_modeinfo *modeinfo;

    vga_setmode(mode);

    modeinfo = vga_getmodeinfo(mode);

    printf("Width: %d  Height: %d  Colors: %d\n",
	   modeinfo->width,
	   modeinfo->height,
	   modeinfo->colors);
    printf("DisplayStartRange: %xh  Maxpixels: %d  Blit: %s\n",
	   modeinfo->startaddressrange,
	   modeinfo->maxpixels,
	   modeinfo->haveblit ? "YES" : "NO");

#ifdef TEST_MODEX
    if (modeinfo->colors == 256)
	printf("Switching to ModeX ... %s\n",
	       (vga_setmodeX()? "done" : "failed"));
#endif

    vga_screenoff();

    xmax = vga_getxdim() - 1;
    ymax = vga_getydim() - 1;

    vga_setcolor(vga_white());
    vga_drawline(0, 0, xmax, 0);
    vga_drawline(xmax, 0, xmax, ymax);
    vga_drawline(xmax, ymax, 0, ymax);
    vga_drawline(0, ymax, 0, 0);

    for (i = 0; i <= 15; i++) {
	vga_setegacolor(i);
	vga_drawline(10 + i * 5, 10, 90 + i * 5, 90);
    }
    for (i = 0; i <= 15; i++) {
	vga_setegacolor(i);
	vga_drawline(90 + i * 5, 10, 10 + i * 5, 90);
    }

    vga_screenon();

    ys = 100;
    yw = (ymax - 100) / 4;
    switch (vga_getcolors()) {
    case 256:
	for (i = 0; i < 60; ++i) {
	    c = (i * 64) / 60;
	    vga_setpalette(i + 16, c, c, c);
	    vga_setpalette(i + 16 + 60, c, 0, 0);
	    vga_setpalette(i + 16 + (2 * 60), 0, c, 0);
	    vga_setpalette(i + 16 + (3 * 60), 0, 0, c);
	}
	line[0] = line[xmax] = 15;
	line[1] = line[xmax - 1] = 0;
	for (x = 2; x < xmax - 1; ++x)
	    line[x] = (((x - 2) * 60) / (xmax - 3)) + 16;
	for (y = ys; y < ys + yw; ++y)	/* gray */
	    vga_drawscanline(y, line);
	for (x = 2; x < xmax - 1; ++x)
	    line[x] += 60;
	ys += yw;
	for (y = ys; y < ys + yw; ++y)	/* red */
	    vga_drawscanline(y, line);
	for (x = 2; x < xmax - 1; ++x)
	    line[x] += 60;
	ys += yw;
	for (y = ys; y < ys + yw; ++y)	/* green */
	    vga_drawscanline(y, line);
	for (x = 2; x < xmax - 1; ++x)
	    line[x] += 60;
	ys += yw;
	for (y = ys; y < ys + yw; ++y)	/* blue */
	    vga_drawscanline(y, line);
	break;

    case 1 << 15:
    case 1 << 16:
    case 1 << 24:
	for (x = 2; x < xmax - 1; ++x) {
	    c = ((x - 2) * 256) / (xmax - 3);
	    y = ys;
	    vga_setrgbcolor(c, c, c);
	    vga_drawline(x, y, x, y + yw - 1);
	    y += yw;
	    vga_setrgbcolor(c, 0, 0);
	    vga_drawline(x, y, x, y + yw - 1);
	    y += yw;
	    vga_setrgbcolor(0, c, 0);
	    vga_drawline(x, y, x, y + yw - 1);
	    y += yw;
	    vga_setrgbcolor(0, 0, c);
	    vga_drawline(x, y, x, y + yw - 1);
	}
	for (x = 0; x < 64; x++) {
	    for (y = 0; y < 64; y++) {
		vga_setrgbcolor(x * 4 + 3, y * 4 + 3, 0);
		vga_drawpixel(xmax / 2 - 160 + x, y + ymax / 2 - 80);
		vga_setrgbcolor(x * 4 + 3, 0, y * 4 + 3);
		vga_drawpixel(xmax / 2 - 32 + x, y + ymax / 2 - 80);
		vga_setrgbcolor(0, x * 4 + 3, y * 4 + 3);
		vga_drawpixel(xmax / 2 + 160 - 64 + x, y + ymax / 2 - 80);

		vga_setrgbcolor(x * 4 + 3, y * 4 + 3, 255);
		vga_drawpixel(xmax / 2 - 160 + x, y + ymax / 2 + 16);
		vga_setrgbcolor(x * 4 + 3, 255, y * 4 + 3);
		vga_drawpixel(xmax / 2 - 32 + x, y + ymax / 2 + 16);
		vga_setrgbcolor(255, x * 4 + 3, y * 4 + 3);
		vga_drawpixel(xmax / 2 + 160 - 64 + x, y + ymax / 2 + 16);
	    }
	}
	break;
    default:
	if (vga_getcolors() == 16) {
	    for (i = 0; i < xmax - 1; i++)
		line[i] = (i + 2) % 16;
	    line[0] = line[xmax] = 15;
	    line[1] = line[xmax - 1] = 0;
	}
	if (vga_getcolors() == 2) {
	    for (i = 0; i <= xmax; i++)
		line[i] = 0x11;
	    line[0] = 0x91;
	}
	for (i = 100; i < ymax - 1; i++)
	    vga_drawscanline(i, line);
	break;

    }

    if (getchar() == 'd')
	vga_dumpregs();

    vga_setmode(TEXT);
}

int main(void)
{
int mode;
int i, high;
vga_init();			/* Initialize. */
mode=-1;
showall:;
printf("Choose one of the following video modes: \n");
high = 0;
for (i = 1; i <= vga_lastmodenumber(); i++)
  {
  if (vga_hasmode(i)) 
    {
    vga_modeinfo *info;
    char xxpl[100];
    char *cols = NULL;
    lirflag=0;

    *xxpl = '\0';
    info = vga_getmodeinfo(i);
    switch (info->colors) 
      {

      case 2:
      cols = "2";
      strcpy(xxpl, "1 bitplane, monochrome");
      break;

      case 16:
      cols = "16";
      strcpy(xxpl, "4 bitplanes");
      break;

      case 256:
      lirflag=1;
      if (i == G320x200x256)
        {
        strcpy(xxpl, "packed-pixel");
        }
      else 
        {
        if (i == G320x240x256 || i == G320x400x256 || i == G360x480x256)
          {
          strcpy(xxpl, "Mode X");
          }
        else
          {
          strcpy(xxpl,"packed-pixel, banked");
          }
        }
      break;

      case 1 << 15:
      cols = "32K";
      strcpy(xxpl, "5-5-5 RGB, blue at LSB, banked");
      break;

      case 1 << 16:
      cols = "64K";
      strcpy(xxpl, "5-6-5 RGB, blue at LSB, banked");
      break;

      case 1 << 24:
      cols = "16M";
      if (info->bytesperpixel == 3) 
        {
        if (info->flags & RGB_MISORDERED)
          {
	  strcpy(xxpl, "8-8-8 BGR, red byte first, banked");
          }
        else
          {
          strcpy(xxpl, "8-8-8 RGB, blue byte first, banked");
          }
        } 
      else 
        {
        if (info->flags & RGB_MISORDERED)
          {
          strcpy(xxpl, "8-8-8 RGBX, 32-bit pixels, X byte first, banked");
          }
        else
          {
          strcpy(xxpl, "8-8-8 XRGB, 32-bit pixels, blue byte first, banked");
	  }
        }	
      break;
      }
    if (info->flags & IS_INTERLACED) 
      {
      if (*xxpl != '\0')
        {
        strcat(xxpl, ", ");
        }
      strcat(xxpl, "interlaced");
      }
    if (info->flags & IS_DYNAMICMODE) 
      {
      if (*xxpl != '\0')
        {
        strcat(xxpl, ", ");
        }
      strcat(xxpl, "dynamically loaded");
      }
    high = i;
    if(info->width < 640 || info->height < 480)lirflag=0;
    if(mode == 0 || lirflag !=0)
      {
      printf("%5d: %dx%d, ",i, info->width, info->height);
      if (cols == NULL)
        {
        printf("%d", info->colors);
        }
      else
        {
        printf("%s", cols);
        }
      printf(" colors ");
      if (*xxpl != '\0')
        {
        printf("(%s)", xxpl);
        }
      printf("\n");
      }  
    }
  }
if(mode == -1)
  {
  printf("These modes are OK for Linrad. Enter mode number (1-%d) to",high);
  printf("\ntest mode. Enter 0 to get a list of all modes:");
  }  
else
  {    
  printf("Enter mode number (1-%d): ", high);
  }
scanf("%d", &mode);
getchar();
printf("\n");
if(mode == 0)goto showall;
if (mode < 1 || mode > GLASTMODE) 
  {
  printf("Error: Mode number out of range \n");
  exit(-1);
  }
if (vga_hasmode(mode))
  {
  testmode(mode);
  }
else 
  {
  printf("Error: Video mode not supported by driver\n");
  exit(-1);
  }
exit(0);
return 0;
}
