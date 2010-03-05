#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <debug.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

/* 
	Network settings for debugging over tcp with BBA
	change these as appropriate for your network

	External C linkage required
*/

#ifdef __cplusplus
	extern "C" {
#endif

const char *tcp_localip = "192.168.1.10";
const char *tcp_netmask = "255.255.255.0";
const char *tcp_gateway = "192.168.1.1";

#ifdef __cplusplus
	}
#endif

int main() {

	VIDEO_Init();
	PAD_Init();
	
	rmode = VIDEO_GetPreferredMode(NULL);

	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
		
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	/* Configure for use with TCP on default port ( 2828 ) */
	DEBUG_Init(GDBSTUB_DEVICE_TCP,GDBSTUB_DEF_TCPPORT);


	printf("Waiting for debugger ...\n");
	/* This function call enters the debug stub for the first time */
	/* It's needed to call this if one wants to start debugging. */
	_break();

	printf("debugger connected ...\n");
     
	while(1)
	{
		VIDEO_WaitVSync();
		PAD_ScanPads();

		int buttons = PAD_ButtonsDown(0);

		if(buttons & PAD_BUTTON_A) {
			printf("Button A pressed.\n");
		}

		if (buttons & PAD_BUTTON_START) {
			break;
		}
	}

	return 0;
}
