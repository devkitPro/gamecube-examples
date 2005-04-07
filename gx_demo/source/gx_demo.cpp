#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <gcmodplay.h>
#include "texture.h"
#include "spaceship.h"
#include "main.h"

#define STACKSIZE			16384
#define DEFAULT_FIFO_SIZE	(256*1024)
#define MAX_SCOORD			0x4000  // for 16b, 2.14 fixed point format
#define MAX_TCOORD			0x4000  // for 16b, 2.14 fixed point format
#define STEP_SCOORD			8
#define STEP_TCOORD			24

static GXTexObj texture;
static void *xfb = NULL;
static u32 do_copy = GX_FALSE;
static lwp_t hmain_game = NULL;
static u32 main_gamestack[STACKSIZE/sizeof(u32)];

static void copy_to_xfb(u32);
static void* main_game(void*);
static void DrawInit(SceneCtrl *);
static void SetCamera(ViewObj *);
static void SetLight(LitObj *);

extern u8 music_start[],music_end[];

s16 TopVerts[] ATTRIBUTE_ALIGN(32) =
{
    -40,  40, -40, // 0
    -40,  40,  40, // 1
    -40, -40,  40, // 2
    -40, -40, -40, // 3
     40,  40, -40, // 4
     40, -40, -40, // 5
     40, -40,  40, // 6
     40,  40,  40  // 7
};

f32 NrmArray[] ATTRIBUTE_ALIGN(32) =
{
	-1.0F, 0.0F, 0.0F,
     1.0F, 0.0F, 0.0F,
     0.0F, -1.0F, 0.0F,
     0.0F, 1.0F, 0.0F,
     0.0F, 0.0F, -1.0F,
     0.0F, 0.0F, 1.0F
};

u8 Colors[] ATTRIBUTE_ALIGN(32) =
{
    0x80, 0x00, 0x00, 0x80,
    0x00, 0x80, 0x00, 0x80,
    0x00, 0x00, 0x80, 0x80,
    0x80, 0x80, 0x00, 0x80,
    0x80, 0x00, 0x80, 0x80,
    0x00, 0x80, 0x80, 0x80
};

static GXColor AmbientColor  = { 0x40, 0x40, 0x40, 0x00 };
static GXColor MaterialColor = { 0x80, 0xD8, 0xFF, 0x00 };
static GXColor LightColor = { 0xC0, 0xC0, 0xC0, 0x00 };
static Vector LightPos = { 150.0F, 150.0F, 300.0F };

static CamCfg DefaultCamera =
{
    {  11.3859, 6.674, 20.09155  }, // location
    { -0.351414, 0.902297, -0.249737    }, // up
    {  -3.19744e-014, -5.50671e-014, 1.42109e-014 }, // target
	0.736262, //fovy
	1.33333,  //aspect
    -320.0F,  // left
    240.0F,   // top
    0.1F,   // near
    1000.0F  // far
};

static MODPlay play;
GXRModeObj *rmode;

int main()
{
	AUDIO_Init(NULL);

	//DecoderInit(mpegfile_start,(mpegfile_end-mpegfile_start));
	VIDEO_Init();
	PAD_Init();
	
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

	
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
		
	VIDEO_Configure(rmode);
		
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	VIDEO_SetPreRetraceCallback(PAD_ScanPads);
	VIDEO_SetNextFramebuffer(xfb);

	VIDEO_SetPostRetraceCallback(copy_to_xfb);

	MODPlay_Init(&play);
	MODPlay_SetMOD(&play,music_start);
	MODPlay_Start(&play);

	LWP_CreateThread(&hmain_game,main_game,NULL,main_gamestack,STACKSIZE,50);
	LWP_SetThreadPriority(0,0);

	while(1);

	free(xfb);
	return 1;
}

static void* main_game(void *arg)
{
	SceneCtrl cntrl;
	Mtx mry,mrx,ms,mv,mvi;


	cntrl.rmode = &TVPal574IntDfScale;
	DrawInit(&cntrl);

	while(1) {
		//VIDEO_ClearFrameBuffer((u32)fb,(640*576*2),COLOR_BLACK);

		GX_SetViewport(0,0,cntrl.rmode->fbWidth,cntrl.rmode->efbHeight,0,1);

		GX_InvVtxCache();
		GX_InvalidateTexAll();

		GX_SetTevOp(GX_TEVSTAGE0,cntrl.tevmode);
		GX_SetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD0,GX_TEXMAP0,GX_COLOR0A0);
		SetLight(&cntrl.litobj);

		GX_LoadTexObj(&texture,GX_TEXMAP0);

		if(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) cntrl.scale += 0.025f;
		else if(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) cntrl.scale -= 0.025f;
		CLAMP(cntrl.scale,0.1f,2.0f);


		guMtxRotDeg(mry,'x',-(PAD_StickY(0)/24));
		guMtxRotDeg(mrx,'y',(PAD_StickX(0)/24));
		guMtxConcat(mry,cntrl.model,cntrl.model);
		guMtxConcat(mrx,cntrl.model,cntrl.model);

		guMtxScale(ms,cntrl.scale,cntrl.scale,cntrl.scale);
		guMtxConcat(cntrl.viewobj.view,ms,mv);
		guMtxConcat(mv,cntrl.model,mv);
		GX_LoadPosMtxImm(mv,GX_PNMTX0);

		guMtxInverse(mv,mvi);
		guMtxTranspose(mvi,mv);
		GX_LoadNrmMtxImm(mv,GX_PNMTX0);

		pCube1();

		GX_DrawDone();
		do_copy = GX_TRUE;
		VIDEO_WaitVSync();

		if(PAD_ButtonsHeld(0) & PAD_BUTTON_START) {
			MODPlay_Stop(&play);
			void (*reload)() = (void(*)())0x80001800;
			reload();
		}
	}
}

static void DrawInit(SceneCtrl *control)
{
	void *gp_fifo = NULL;
	GXColor backcol = {0x00,0x00,0x00,0xff};
	GXRModeObj *rmode = control->rmode;

	gp_fifo = MEM_K0_TO_K1(memalign(32,DEFAULT_FIFO_SIZE));
	memset(gp_fifo,0,DEFAULT_FIFO_SIZE);

	GX_Init(gp_fifo,DEFAULT_FIFO_SIZE);

	GX_SetCopyClear(backcol,0x00ffffff);

	GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
	GX_SetDispCopyYScale((f32)rmode->xfbHeight/(f32)rmode->efbHeight);
	GX_SetScissor(0,0,rmode->fbWidth,rmode->efbHeight);
	GX_SetDispCopySrc(0,0,rmode->fbWidth,rmode->efbHeight);
	GX_SetDispCopyDst(rmode->fbWidth,rmode->xfbHeight);
	GX_SetCopyFilter(rmode->aa,rmode->sample_pattern,GX_TRUE,rmode->vfilter);
	GX_SetFieldMode(rmode->field_rendering,((rmode->viHeight==2*rmode->xfbHeight)?GX_ENABLE:GX_DISABLE));

	if (rmode->aa)
        GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
    else
        GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

	GX_SetCullMode(GX_CULL_FRONT);
	GX_CopyDisp(xfb,GX_TRUE);
	GX_SetDispCopyGamma(GX_GM_1_0);

	GX_SetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
	GX_SetVtxAttrFmt(GX_VTXFMT0,GX_VA_TEX0,GX_TEX_ST,GX_F32,0);
	GX_SetVtxAttrFmt(GX_VTXFMT0,GX_VA_NRM,GX_NRM_XYZ,GX_F32,0);

	GX_InvVtxCache();
	GX_SetArray(GX_VA_POS,(void*)VertexData,3*sizeof(f32));
	GX_SetArray(GX_VA_NRM,(void*)NormalData,3*sizeof(f32));
	GX_SetArray(GX_VA_TEX0,(void*)TexCoordData,2*sizeof(f32));
	DCFlushRange((void*)VertexData,VertexDataSize);
	DCFlushRange((void*)NormalData,NormalDataSize);
	DCFlushRange((void*)TexCoordData,TexCoordDataSize);

	GX_SetNumTexGens(1);
	GX_SetTexCoordGen(GX_TEXCOORD0,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY);

	GX_InvalidateTexAll();
	GX_InitTexObj(&texture,textureI8,32,32,GX_TF_I8,GX_REPEAT,GX_REPEAT,GX_FALSE);
	//GX_InitTexObjLOD(&texture,GX_NEAR,GX_NEAR,0,0,0,GX_FALSE,GX_DISABLE,GX_ANISO_1);
	DCFlushRange(textureI8,sizeof(textureI8sz));

	control->viewobj.cfg = DefaultCamera;
	SetCamera(&control->viewobj);

	control->litobj.theta = 35;
	control->litobj.phi = 25;
	control->litobj.enable = 1;
	control->litobj.litcol = LightColor;
	control->litobj.pos = LightPos;

	control->scale = 1.0f;
	control->tevmode = GX_MODULATE;
	guMtxScale(control->model,control->scale,control->scale,control->scale);
}

static void SetCamera(ViewObj *obj)
{
	guLookAt(obj->view,&obj->cfg.campos,&obj->cfg.camup,&obj->cfg.target);
	guPerspective(obj->proj,obj->cfg.fovy,obj->cfg.aspect,obj->cfg.znear,obj->cfg.zfar);
	//guFrustum(obj->proj,obj->cfg.top,-(obj->cfg.top),obj->cfg.left,-(obj->cfg.left),obj->cfg.znear,obj->cfg.zfar);
	GX_LoadProjectionMtx(obj->proj,GX_PERSPECTIVE);
}

static void SetLight(LitObj *obj)
{
	u8 rgbMask;
	Vector lpos;
	f32 theta,phi,dx,dy,dz;

	theta = (f32)obj->theta*M_PI/180.0f;
	phi = (f32)obj->phi*M_PI/180.0f;
	dx = cosf(phi)*sinf(theta);
	dy = sinf(phi);
	dz = -cosf(phi)*cosf(theta);

	lpos = obj->pos;
	GX_InitLightDir(&obj->lit,dx,dy,dz);
	GX_InitLightPos(&obj->lit,lpos.x,lpos.y,lpos.z);
	GX_InitLightColor(&obj->lit,obj->litcol);
	GX_InitLightAttn(&obj->lit,-39.0f,40.0f,0.0f,1.0f,0.0f,0.0f);

	GX_LoadLightObj(&obj->lit,GX_LIGHT0);

	rgbMask = (obj->enable)?GX_LIGHT0:GX_LIGHTNULL;

	GX_SetNumChans(1);
	GX_SetChanCtrl(GX_COLOR0,GX_ENABLE,GX_SRC_REG,GX_SRC_REG,rgbMask,GX_DF_CLAMP,GX_AF_NONE);
	GX_SetChanAmbColor(GX_COLOR0A0,AmbientColor);
	GX_SetChanMatColor(GX_COLOR0A0,MaterialColor);
}

static void copy_to_xfb(u32 count)
{
	if(do_copy==GX_TRUE) {
		GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
		GX_SetColorUpdate(GX_TRUE);
		GX_CopyDisp(xfb,GX_TRUE);
		GX_Flush();
		do_copy = GX_FALSE;
	}
}
