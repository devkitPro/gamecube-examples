#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>

#include "input.h"

static void *xfb = NULL;

static u8 SysArea[CARD_WORKAREA] ATTRIBUTE_ALIGN(32);

u32 first_frame = 1;
GXRModeObj *rmode;

/*---------------------------------------------------------------------------------
	This function is called if a card is physically removed
---------------------------------------------------------------------------------*/
void card_removed(s32 chn,s32 result) {
//---------------------------------------------------------------------------------
	printf("card was removed from slot %c\n",(chn==0)?'A':'B');
	CARD_Unmount(chn);
}

//---------------------------------------------------------------------------------
int main() {
//---------------------------------------------------------------------------------
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
	
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
		
	VIDEO_Configure(rmode);
		
	VIDEO_SetNextFramebuffer(xfb);
	
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	VIDEO_SetPreRetraceCallback(ScanKeys);
	console_init(xfb,20,64,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*2);

	printf("Memory Card Demo\n\n");

	while (1) {
		printf("Insert A card in slot B and press A\n");


		while ( !(KeysDown()& PAD_BUTTON_A)) {
			VIDEO_WaitVSync();
		}

		printf("Mounting card ...\n");

		CARD_Init("\xffDEMO","\xff00");
		int SlotB_error = CARD_Mount(CARD_SLOTB, SysArea, card_removed);
	
		printf("slot B code %d\n",SlotB_error);

		if (SlotB_error > 0) {
			printf("Starting directory\n");
			card_dir CardDir;
		
			int CardError = CARD_FindFirst(CARD_SLOTB, &CardDir);
		
			while ( CARD_ERROR_NOFILE != CardError ) {
				printf("%s  %s  %s\n",CardDir.filename,CardDir.gamecode,CardDir.company);
				CardError = CARD_FindNext(&CardDir);
			};

			printf("Finished directory\n\n");
			
			printf("writing test file ...\n");
			
			CARD_Unmount(CARD_SLOTB);
		}
	}

}
