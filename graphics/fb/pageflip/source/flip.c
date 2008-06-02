/****************************************************************************
* pflip.c
*
* libogc example for 2D video page flipping without tearing.
* Shows a checkerboard and bouncing square.
****************************************************************************/
#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
/*** 2D Video Globals ***/
GXRModeObj *vmode;        /*** Graphics Mode Object ***/
u32 *xfb[2] = { NULL, NULL };    /*** Framebuffers ***/
int whichfb = 0;        /*** Frame buffer toggle ***/
 
/****************************************************************************
* CvtRGB
*
* This function simply returns two RGB pixels as one Y1CbY2Cr.
*****************************************************************************/
u32
CvtRGB (u8 r1, u8 g1, u8 b1, u8 r2, u8 g2, u8 b2)
{
  int y1, cb1, cr1, y2, cb2, cr2, cb, cr;
 
  y1 = (299 * r1 + 587 * g1 + 114 * b1) / 1000;
  cb1 = (-16874 * r1 - 33126 * g1 + 50000 * b1 + 12800000) / 100000;
  cr1 = (50000 * r1 - 41869 * g1 - 8131 * b1 + 12800000) / 100000;
 
  y2 = (299 * r2 + 587 * g2 + 114 * b2) / 1000;
  cb2 = (-16874 * r2 - 33126 * g2 + 50000 * b2 + 12800000) / 100000;
  cr2 = (50000 * r2 - 41869 * g2 - 8131 * b2 + 12800000) / 100000;
 
  cb = (cb1 + cb2) >> 1;
  cr = (cr1 + cr2) >> 1;
 
  return (y1 << 24) | (cb << 16) | (y2 << 8) | cr;
}
 
/****************************************************************************
* Initialise Video
*
* Before doing anything in libogc, it's recommended to configure a video
* output.
****************************************************************************/
static void
Initialise (void) {

	VIDEO_Init();		/*** ALWAYS CALL FIRST IN ANY LIBOGC PROJECT!
							Not only does it initialise the video 
							subsystem, but also sets up the ogc os
						***/
 
	PAD_Init();			/*** Initialise pads for input ***/
 
	/*** Try to match the current video display mode
		using the higher resolution interlaced.
    
		So NTSC/MPAL gives a display area of 640x480
		PAL display area is 640x528
	***/

	vmode = VIDEO_GetPreferredMode(NULL);
 
	/*** Let libogc configure the mode ***/
	VIDEO_Configure (vmode);
 
	/*** Now configure the framebuffer. 
		Really a framebuffer is just a chunk of memory
		to hold the display line by line.
	***/
 
	xfb[0] = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (vmode));

	/*** I prefer also to have a second buffer for double-buffering.
		This is not needed for the console demo.
	***/
	xfb[1] = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (vmode));
 

	/*** Define a console ***/
	console_init (xfb[0], 20, 64, vmode->fbWidth, vmode->xfbHeight, vmode->fbWidth * 2);
 
	/*** Clear framebuffer to black ***/
	VIDEO_ClearFrameBuffer (vmode, xfb[0], COLOR_BLACK);
	VIDEO_ClearFrameBuffer (vmode, xfb[1], COLOR_BLACK);
 
	/*** Set the framebuffer to be displayed at next VBlank ***/
	VIDEO_SetNextFramebuffer (xfb[0]);
 
	/*** Get the PAD status updated by libogc ***/
	VIDEO_SetBlack (0);
 
	/*** Update the video for next vblank ***/
	VIDEO_Flush ();

	VIDEO_WaitVSync ();        /*** Wait for VBL ***/

	if (vmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync ();
 
}


/****************************************************************************
* Main
*
* Create a checkerboard, and flip it
****************************************************************************/
int main () {
	u32 *checkerboard;
	u32 colour1, colour2, colswap;
	int rows, cols;
	int height;
	int i, j, t, v;
	int pos = 0;
	int startx, starty;
	int directionx, directiony;
	PADStatus pads[4];
 
	/*** Start libOGC ***/
	Initialise ();
 
	/*** Allocate a buffer to hold the checkerboard ***/
	height = vmode->xfbHeight + 64;
	checkerboard = (u32 *) malloc ((704 * height * 2));
	colour1 = CvtRGB (0x00, 0x00, 0x80, 0x00, 0x00, 0x80);
	colour2 = CvtRGB (0x00, 0x00, 0xe0, 0x00, 0x00, 0xe0);
 
	for (i = 0; i < height; i += 32) {
		/*** Draw a line ***/
		colswap = colour1;
		colour1 = colour2;
		colour2 = colswap;
 
		for (v = 0; v < 32; v++)
		{
			for (t = 0; t < 11; t++)
			{
				for (j = 0; j < 16; j++)
					checkerboard[pos++] = colour1;
 
				for (j = 0; j < 16; j++)
					checkerboard[pos++] = colour2;
			}
 
		}
    }
 
	pos = 0;
	colour1 = CvtRGB (0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
	startx = starty = directionx = directiony = 0;
 
	while(1) {

		/*** Flip to off screen xfb ***/
		whichfb ^= 1;
 
		/*** Draw checkerboard ***/
		t = 0;
		v = 0;

		for (j = 0; j < vmode->xfbHeight; j++) {
			memcpy (&xfb[whichfb][v], &checkerboard[pos + t], 1280);
			t += 352;
			v += 320;
		}

		/*** Draw Bouncing Square ***/
		if (directionx)
			startx -= 4;
		else
			startx += 4;
 
		if (directiony)
			starty -= 2;
		else
			starty += 2;
 
		if (startx >= 576) directionx = 1;
 
		if (starty >= (vmode->xfbHeight - 64)) directiony = 1;
 
		if (startx < 0) {
			startx = 0;
			directionx = 0;
		}
 
		if (starty < 0) {
			starty = 0;
			directiony = 0;
		}
 
		v = (starty * 320) + (startx >> 1);

		for (rows = 0; rows < 64; rows++) {
			for (cols = 0; cols < 32; cols++)
				xfb[whichfb][v + cols] = colour1;
 
			v += 320;
		}
 
		/*** Set this as next frame to display ***/
		VIDEO_SetNextFramebuffer (xfb[whichfb]);
		VIDEO_Flush ();
		VIDEO_WaitVSync ();
 
		/*** Move the checkerboard along ***/
		pos += 353;

		if (pos == ((352 * 64) + 64)) pos = 0;

		PAD_ScanPads(0);
		PAD_Read(pads);
		if (pads[0].button & PAD_BUTTON_START) {
			exit(0);
		}

	}

	return 0;
}
