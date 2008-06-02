/*---------------------------------------------------------------------------------

 libogc - DVD Demo

 Demonstrates reading a sector from DVD

---------------------------------------------------------------------------------*/

#include <stdio.h>
#include <gccore.h>        // Wrapper to include common libogc headers
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
// 2D Video Globals

GXRModeObj *vmode;		// Graphics Mode Object
u32 *xfb = NULL;		// Framebuffer  
int dvdstatus = 0;

static u8 dvdbuffer[2048] ATTRIBUTE_ALIGN (32);    // One Sector
dvdcmdblk cmdblk;
 
/*---------------------------------------------------------------------------------
 Initialise Video

 Before doing anything in libogc, it's recommended to configure a video
 output.
---------------------------------------------------------------------------------*/
static void Initialise () {
//---------------------------------------------------------------------------------

	VIDEO_Init ();	/*	ALWAYS CALL FIRST IN ANY LIBOGC PROJECT!
						Not only does it initialise the video 
						subsystem, but also sets up the ogc os
					*/
 
	PAD_Init ();	// Initialise pads for input
 
	
	vmode = VIDEO_GetPreferredMode(NULL);
 
	// Let libogc configure the mode
	VIDEO_Configure (vmode);
 

	/*
	
	Now configure the framebuffer. 
	Really a framebuffer is just a chunk of memory
	to hold the display line by line.
	
	*/

	xfb = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (vmode));


	// Define a console
	console_init (xfb, 20, 64, vmode->fbWidth, vmode->xfbHeight,
	vmode->fbWidth * 2);
 
    // Clear framebuffer to black
	VIDEO_ClearFrameBuffer (vmode, xfb, COLOR_BLACK);
	VIDEO_ClearFrameBuffer (vmode, xfb, COLOR_BLACK);

	/*** Set the framebuffer to be displayed at next VBlank ***/
	VIDEO_SetNextFramebuffer (xfb);
 
	VIDEO_SetBlack (0);
 
	// Update the video for next vblank 
	VIDEO_Flush ();
  
	VIDEO_WaitVSync ();        // Wait for next Vertical Blank

	if (vmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync ();
 
}
 
//---------------------------------------------------------------------------
int main () {
//--------------------------------------------------------------------------
	int ret = 0;
	int mounted = 0;
	int *p;
	int j, i;
 
	Initialise ();		// Start video etc
	DVD_Init ();		// And the DVD subsystem

	printf ("libOGC DVD Example\n");
	printf ("Mounting Disc\n");
 
	// This will mount pretty much any disc, original or ISO
	mounted = DVD_Mount ();
	printf ("OK\n");
 
	// Read Sector 0
	ret = DVD_ReadPrio (&cmdblk, dvdbuffer, 2048, 0, 2);

	if (ret <= 0) {
		printf ("Error during read sector 0\n");
		while (1);
	}
 
	printf ("Read %d bytes\n", ret);
	p = (int *) dvdbuffer;
 
	// Identify disc type
	j = 0;

	for (i = 0; i < 512; i++)
		j += p[i];
 
	if (j == 0) {
		printf ("Disc is most probably ISO-9660\n");
	} else {

		if (memcmp (dvdbuffer + 32, "GAMECUBE \"EL TORITO\" BOOTLOADER", 31) == 0) {
			printf ("Disc is gc-linux bootable\n");
		} else {

			if (memcmp (dvdbuffer, "COBRAMB1", 8) == 0) {
				printf ("Disc is multigame image\n");
			} else {
				printf ("Disc is gcm\n%s\n", (char *) dvdbuffer + 32);
			}
		}
	}
 
	while(1) {

		VIDEO_WaitVSync();
		PAD_ScanPads();

		int buttonsDown = PAD_ButtonsDown(0);
		

		if (buttonsDown & PAD_BUTTON_START) {
			void (*reload)() = (void(*)())0x80001800;
			reload();
		}
	}
 
	return 0;
}
