#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>
#include <time.h>

static void *xfb = NULL;

u32 first_frame = 1;
GXRModeObj *rmode;

int main()
{
	PADStatus pad[4];
	
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
	
	xfb = MEM_K0_TO_K1(memalign(VIDEO_PadFramebufferWidth(rmode->fbWidth)*rmode->xfbHeight*VI_DISPLAY_PIX_SZ,32));
		
	VIDEO_Configure(rmode);
		
	VIDEO_SetNextFramebuffer(xfb);
	
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	
	console_init(xfb,20,64,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*2);



	time_t gc_time;
	gc_time = time(NULL);

	srand(gc_time);

	printf("testing console\n");
	printf("random number is %08x\n",rand());
	printf("RTC time is %s",ctime(&gc_time));

	while(1)
	{

		VIDEO_SetNextFramebuffer(xfb);
		if(first_frame)
		{
			first_frame = 0;
			VIDEO_SetBlack(FALSE);
		}
	
		VIDEO_Flush();
		VIDEO_WaitVSync();

		PAD_Read(pad);

		if(pad[0].button&PAD_BUTTON_START) {
			void (*reload)() = (void(*)())0x80001800;
			reload();
		}
		if(pad[0].button &PAD_BUTTON_A) {

			free ((void *)42); // crrrrrash :)
		}
	};

}
