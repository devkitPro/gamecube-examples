#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>
#include <time.h>

static void *xfb = NULL;

u32 first_frame = 1;
GXRModeObj *rmode;
vu16 oldstate;
vu16 keystate;
vu16 keydown;
vu16 keyup;
PADStatus pad[4];

void ScanPads() {

		PAD_Read(pad);

		oldstate = keystate;
		keystate = pad[0].button;

		keydown = (~oldstate) & keystate;
		keyup = oldstate & (~keystate);
}

u16 ButtonsUp() {
	return keyup;
}
u16 ButtonsDown() {
	return keydown;
}

u16 ButtonsHeld() {
	return keystate;
}

int main()
{
	
	VIDEO_Init();
	
	switch(VIDEO_GetCurrentTvMode())
	{
		case VI_NTSC:
			rmode = &TVNtsc480IntDf;
			break;
		case VI_PAL:
			rmode = &TVPal528IntDf;
			break;
		case VI_MPAL:
			rmode = &TVMpal480IntDf;
			break;
		default:
			rmode = &TVNtsc480IntDf;
			break;
	}
	
	
	PAD_Init();

	oldstate = 0;
	keystate = 0;
	keydown = 0;
	keyup = 0;
	
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
		
	VIDEO_Configure(rmode);
		
	VIDEO_SetNextFramebuffer(xfb);
	
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	
	console_init(xfb,20,64,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*2);
	VIDEO_SetPreRetraceCallback(ScanPads);



	time_t gc_time;
	gc_time = time(NULL);

	srand(gc_time);

	printf("testing console\n");
	printf("random number is %08x\n",rand());
	printf("RTC time is %s",ctime(&gc_time));

	while(1) {

		VIDEO_SetNextFramebuffer(xfb);
		if(first_frame) {
			first_frame = 0;
			VIDEO_SetBlack(FALSE);
		}
	
		VIDEO_WaitVSync();


		if ( ButtonsDown() || ButtonsUp()) {
			printf( "%04x %04x %04x %04x %04x\n", pad[0].button, oldstate, keystate, keydown, keyup );
		}

		if(ButtonsDown()&PAD_BUTTON_START) {
			void (*reload)() = (void(*)())0x80001800;
			reload();
		}
	};

}
