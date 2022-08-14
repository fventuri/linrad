/* Program to test the svgalib mouse functions. */
/* Updated to use middle button and rx axis (for wheel mice)
   by Brion Vibber <brion@pobox.com>, 5 July 1998 */
// Part of the svgalib package.
// Included in Linrad with a return value from main
// to compile without warnings or errors.



#include <stdlib.h>

#include <stdlib.h>
#include <stdio.h>
#include <vga.h>
#include <vgagl.h>
#include <vgamouse.h>
#include <vgakeyboard.h>

/* Manually open and close mouse? */
#define MANUALLY_SETUP_MOUSE_NOT

#if CPU == CPU_ARM
void outsb(void){};
#endif


static int newcolor(void)
{
    if (BYTESPERPIXEL == 1)
	return random() % 15 + 1;
    return gl_rgbcolor(random() & 255, random() & 255, random() & 255);
}


int main(void)
{
    int vgamode, color, leftpressed, middlepressed;
    int x, y, rx, ox, oy, boxsize, button, wheel;
    struct MouseCaps caps;

    vga_init();
    vgamode = vga_getdefaultmode();
    if (vgamode == -1)
	vgamode = G320x200x256;

    if (!vga_hasmode(vgamode)) {
	printf("Mode not available.\n");
	exit(-1);
    }
#ifndef MANUALLY_SETUP_MOUSE
    /* Enable automatic mouse setup at mode set. */
    vga_setmousesupport(1);
#endif
    vga_setmode(vgamode);
    /* Disable wrapping (default). */
    /* mouse_setwrap(MOUSE_NOWRAP); */
    gl_setcontextvga(vgamode);
    gl_enableclipping();

#ifdef MANUALLY_SETUP_MOUSE
    mouse_init("/dev/mouse", MOUSE_MICROSOFT, MOUSE_DEFAULTSAMPLERATE);
    mouse_setxrange(0, WIDTH - 1);
    mouse_setyrange(0, HEIGHT - 1);
    mouse_setwrap(MOUSE_NOWRAP);
#endif

    /* Check the mouse capabilities */
    if(mouse_getcaps(&caps)) {
        /* Failed! Old library version... Check the mouse type. */
        switch(vga_getmousetype() & MOUSE_TYPE_MASK) {
        case MOUSE_INTELLIMOUSE:
        case MOUSE_IMPS2:
            wheel = 1;
            break;
        default:
            wheel = 0;
        }
    } else {
        /* If this is a wheel mouse, interpret rx as a wheel */
        wheel = ((caps.info & MOUSE_INFO_WHEEL) != 0);
    }

    /* To be able to test fake mouse events... */
    if (keyboard_init()) {
	printf("Could not initialize keyboard.\n");
	exit(1);
    }

    /* Set the range for the wheel */
    if(wheel)
        mouse_setrange_6d(0,0, 0,0, 0, 0, -180,180, 0,0, 0,0, MOUSE_RXDIM);

    color = newcolor();
    leftpressed = middlepressed = x = y = rx = ox = oy = 0;
    boxsize = 5;
    
    for (;;) {
	keyboard_update();
	gl_fillbox(x, y, boxsize, boxsize, color);
        mouse_update();

        /* The RX axis represents the wheel on an wheel mouse */
        mouse_getposition_6d(&x, &y, NULL, &rx, NULL, NULL);

        if(wheel && rx) {
            /* For clarity - wipe the old location out
             so we can redraw with the new box size */
            gl_fillbox(ox, oy, boxsize, boxsize, 0);

            /* Interpret wheel turns; we care only about direction,
               not amount, for our purposes */
            boxsize += (rx / abs(rx));
            (boxsize < 1)?(boxsize = 1):((boxsize > 10)?(boxsize = 10):0);

            /* Zero the wheel position */
            mouse_setposition_6d(0,0,0, 0,0,0, MOUSE_RXDIM);
        }

        ox = x; oy = y;

	button = mouse_getbutton();
	if (button & MOUSE_LEFTBUTTON) {
	    if (!leftpressed) {
		color = newcolor();
		leftpressed = 1;
	    }
	} else
            leftpressed = 0;

        if (button & MOUSE_MIDDLEBUTTON) {
	    if (!middlepressed) {
                /* Move the cursor to a random location */
                mouse_setposition_6d(random() % WIDTH, random() % HEIGHT,0,
                                     0,0,0,
                                     MOUSE_2DIM);
                middlepressed = 1;
            }
        } else
            middlepressed = 0;

	if (button & MOUSE_RIGHTBUTTON)
	    break;
    }

#ifdef MANUALLY_SETUP_MOUSE
    mouse_close();
#endif
    vga_setmode(TEXT);
return 0;
}
