#include <stdio.h>
#include <gccore.h>
#include <math.h>
#include <malloc.h>
#include <string.h>
#include <ogcsys.h>

/*Texture*/
#include "GC_front.h"
#include "GC_front_tpl.h"
#include "GC_body.h"
#include "GC_body_tpl.h"

/*Geometry*/
#include "controller.h"

#define DEFAULT_FIFO_SIZE (256*1024)

typedef struct
{
	guVector pos; 
	guVector rot; 
	guVector sca; 
	
	guVector* vfaces;
	guVector* nfaces;  	
	guVector* ufaces; 
	guVector* vgeo;
	guVector* ngeo; 
	guVector* ugeo; 
	
	int face_count;
}obj_t; 

obj_t bbody;
obj_t fbody;
obj_t astick;  
obj_t cstick; 
obj_t dpad; 
obj_t startb;
obj_t abut;
obj_t bbut; 
obj_t xbut;
obj_t ybut; 
obj_t zbut; 
obj_t rtrig;  
obj_t ltrig;  

GXTexObj GCF_tex;
GXTexObj GCB_tex;
TPLFile GCF_TPL; 
TPLFile GCB_TPL; 

int context; 
u32* xfb[2] = {NULL, NULL};
GXRModeObj* rmode; 
Mtx44 p; 
Mtx v;

static GXColor lightColor[] = {
        { 0xFF, 0xFF, 0xFF, 0xFF }, 
        { 0x20, 0x20, 0x20, 0xFF }, 
};


void set_light()
{    
    GXLightObj MyLight;
    guVector lpos = {0.0f, 0.3f, 3.0f};
	
    //guVecMultiply(v, &lpos, &lpos);
    GX_InitLightPos(&MyLight, lpos.x, lpos.y, lpos.z);
    GX_InitLightColor(&MyLight, lightColor[0]);
    GX_InitLightSpot(&MyLight, 0.0f, GX_SP_OFF);
    GX_InitLightDistAttn(&MyLight, 20.0f, 1.0f, GX_DA_MEDIUM);
    GX_LoadLightObj(&MyLight, GX_LIGHT0);

    GX_SetNumChans(1);
    GX_SetChanCtrl(GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT0, GX_DF_CLAMP, GX_AF_SPOT);
    GX_SetChanAmbColor(GX_COLOR0A0,lightColor[1]);

}

void gcn_init()
{
	f32 yscale; 
	u32 xfbHeight; 
	void* gp_fifo; 
	f32 w; 
	f32 h; 
	
	GXColor bg = {0, 0, 0, 0xff};
	
	VIDEO_Init(); 
	PAD_Init(); 
	
	rmode = VIDEO_GetPreferredMode(NULL);
	
	context = 0; 
	
	gp_fifo = memalign(32, DEFAULT_FIFO_SIZE);
	memset(gp_fifo, 0, DEFAULT_FIFO_SIZE);
	
	xfb[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));  
	xfb[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));  
	
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb[context]);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush(); 
	VIDEO_WaitVSync(); 
	
	if(rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync(); 
	
	GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);
	GX_SetCopyClear(bg, 0x00ffffff);
	GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
	
	yscale = GX_GetYScaleFactor(rmode->efbHeight, rmode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	
	GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
	GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
	GX_SetDispCopyDst(rmode->fbWidth, xfbHeight); 
	GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, 1, rmode->vfilter);
	GX_SetFieldMode(rmode->field_rendering, ((rmode->viHeight == 2 * rmode->xfbHeight)? 1 : 0));
	
	if (rmode->aa) {
		GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	} else {
		GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
	}
	
	//for now
	GX_SetCullMode(GX_CULL_NONE);
	GX_CopyDisp(xfb[context], 1);
	GX_SetDispCopyGamma(GX_GM_1_0);
	
	guVector cam = {0.0f, 0.0f, 0.0f},
			 up = {0.0f, 1.0f, 0.0f}, 
			 look = {0.0f, 0.0f, -1.0f};
			 
	guLookAt(v, &cam, &up, &look);
	
	w = rmode->viWidth; 
	h = rmode->viHeight; 
	
	guPerspective(p, 60, (f32)w/h, 10.0f, 300.0f);
	GX_LoadProjectionMtx(p, GX_PERSPECTIVE);
}

void draw_init()
{
	GX_ClearVtxDesc();
	
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_NRM, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);  
	
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	
	GX_SetNumChans(1);
	GX_SetNumTexGens(1);
	
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
	
	GX_InvVtxCache(); 
	GX_InvalidateTexAll(); 
	
	TPL_OpenTPLFromMemory(&GCF_TPL, (void *)GC_front_tpl,GC_front_tpl_size);
	TPL_GetTexture(&GCF_TPL,GCF,&GCF_tex);
	
	TPL_OpenTPLFromMemory(&GCB_TPL, (void *)GC_body_tpl,GC_body_tpl_size);
	TPL_GetTexture(&GCB_TPL,GCB,&GCB_tex);

}

void draw_obj(obj_t* o)
{
	Mtx m, mv;
	Mtx mrx, mry, mrz;

	guVector xr = {1, 0, 0};
	guVector yr = {0, 1, 0};
	guVector zr = {0, 0, 1};
	
	guMtxIdentity(m);
	guMtxRotAxisDeg(mrx, &xr, o->rot.x);
	guMtxRotAxisDeg(mry, &yr, o->rot.y);
	guMtxRotAxisDeg(mrz, &zr, o->rot.z);
	guMtxConcat(mrx, mry, mry);
	guMtxConcat(mry, mrz, mrz);
	guMtxConcat(mrz, m, m);
	guMtxTransApply(m, m, o->pos.x, o->pos.y, o->pos.z);
	guMtxConcat(v, m, mv);
	
	GX_LoadPosMtxImm(mv, GX_PNMTX0);
	GX_LoadNrmMtxImm(mv, GX_PNMTX0);

	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, o->face_count*3);
	for(int i = 0; i < o->face_count; ++i)
	{
			GX_Position3f32(o->vgeo[(int)o->vfaces[i].x].x, 
							o->vgeo[(int)o->vfaces[i].x].y,
							o->vgeo[(int)o->vfaces[i].x].z); 
							
			GX_Normal3f32(o->ngeo[(int)o->nfaces[i].x].x, 
						  o->ngeo[(int)o->nfaces[i].x].y,
					      o->ngeo[(int)o->nfaces[i].x].z); 
					      
			GX_TexCoord2f32(o->ugeo[(int)o->ufaces[i].x].x,
			                o->ugeo[(int)o->ufaces[i].x].y); 
			                
			GX_Position3f32(o->vgeo[(int)o->vfaces[i].y].x, 
							o->vgeo[(int)o->vfaces[i].y].y,
							o->vgeo[(int)o->vfaces[i].y].z); 
			
			GX_Normal3f32(o->ngeo[(int)o->nfaces[i].y].x, 
						  o->ngeo[(int)o->nfaces[i].y].y,
					      o->ngeo[(int)o->nfaces[i].y].z);
					       				
			GX_TexCoord2f32(o->ugeo[(int)o->ufaces[i].y].x,
			                o->ugeo[(int)o->ufaces[i].y].y);
			                
			GX_Position3f32(o->vgeo[(int)o->vfaces[i].z].x, 
							o->vgeo[(int)o->vfaces[i].z].y,
							o->vgeo[(int)o->vfaces[i].z].z); 
							
			GX_Normal3f32(o->ngeo[(int)o->nfaces[i].z].x, 
						  o->ngeo[(int)o->nfaces[i].z].y,
					      o->ngeo[(int)o->nfaces[i].z].z); 
					      				
			GX_TexCoord2f32(o->ugeo[(int)o->ufaces[i].z].x,
			                o->ugeo[(int)o->ufaces[i].z].y); 	}		                 			                
   GX_End(); 
}

void update_view()
{
	static guVector up = {0, 1, 0};
	static guVector right = {1, 0, 0};
	Mtx xr, yr; 
	guMtxTransApply(v, v, 0, 0, 200);
	guMtxRotAxisDeg(xr, &right, astick.rot.x*0.1f);
	guMtxRotAxisDeg(yr, &up, astick.rot.y*0.1f); 
	guMtxConcat(xr, yr, xr);
	guMtxConcat(xr, v, xr);
	guMtxTransApply(xr, v, 0, 0, -200);
}

void render()
{
	static int tevs = 0; 
	if (PAD_ButtonsDown(0) & PAD_TRIGGER_Z)
		tevs = (tevs+1)%3; 
		
	set_light();

	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	
	
	switch(tevs)
	{
		case 0:
			GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
			break;
		case 1:
			GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
			break;
		case 2:
			GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
			break;
	}
	
	update_view();
	
	GX_LoadTexObj(&GCB_tex, GX_TEXMAP0);
	draw_obj(&bbody);
	 
	GX_LoadTexObj(&GCF_tex, GX_TEXMAP0);
	draw_obj(&zbut);
	draw_obj(&rtrig);
	draw_obj(&ltrig);
	draw_obj(&astick); 
	draw_obj(&cstick);
	draw_obj(&dpad);
	draw_obj(&startb);
	draw_obj(&abut); 
	draw_obj(&bbut); 
	draw_obj(&xbut);
	draw_obj(&ybut); 
	
	GX_SetZMode(1, GX_LEQUAL, 1);
	GX_SetColorUpdate(1);
	GX_CopyDisp(xfb[context], 1);
	
	GX_DrawDone();
	GX_InvalidateTexAll();
	
	VIDEO_SetNextFramebuffer(xfb[context]); 
	
	VIDEO_Flush(); 
	VIDEO_WaitVSync();
	context ^= 1; 
}

void obj_init()
{
	bbody.pos.x = 0; bbody.pos.y = 0; bbody.pos.z = -200.0;
	bbody.rot.x = 0; bbody.rot.y = 0; bbody.rot.z = 0;
	bbody.sca.x = 0; bbody.sca.x = 0; bbody.sca.x = 0;	
	bbody.vfaces = bbody_points;
	bbody.nfaces = bbody_normpoints; 
	bbody.ufaces = bbody_uvpoints;
	bbody.vgeo   = bbody_vert;
	bbody.ngeo   = bbody_norm;
	bbody.ugeo   = bbody_uv;
	bbody.face_count = BBODY_POINT_COUNT;
	
	astick.pos.x = -58; astick.pos.y = 15; astick.pos.z = -172;
	astick.rot.x = 0; astick.rot.y = 0; astick.rot.z = 0;
	astick.sca.x = 0; astick.sca.x = 0; astick.sca.x = 0;	
	astick.vfaces = astick_points;
	astick.nfaces = astick_normpoints; 
	astick.ufaces = astick_uvpoints;
	astick.vgeo   = astick_vert;
	astick.ngeo   = astick_norm;
	astick.ugeo   = astick_uv;
	astick.face_count = ASTICK_POINT_COUNT;
	
	cstick.pos.x = 30; cstick.pos.y = -25; cstick.pos.z = -177;
	cstick.rot.x = 0; cstick.rot.y = 0; cstick.rot.z = 0;
	cstick.sca.x = 0; cstick.sca.x = 0; cstick.sca.x = 0;	
	cstick.vfaces = cstick_points;
	cstick.nfaces = cstick_normpoints;
	cstick.ufaces = cstick_uvpoints;
	cstick.vgeo   = cstick_vert;
	cstick.ngeo   = cstick_norm;
	cstick.ugeo   = cstick_uv;
	cstick.face_count = CSTICK_POINT_COUNT;
	
	dpad.pos.x = -30; dpad.pos.y = -25; dpad.pos.z = -174;
	dpad.rot.x = 0; dpad.rot.y = 0; dpad.rot.z = 0;
	dpad.sca.x = 0; dpad.sca.x = 0; dpad.sca.x = 0;	
	dpad.vfaces = dpad_points;
	dpad.nfaces = dpad_normpoints;
	dpad.ufaces = dpad_uvpoints;
	dpad.vgeo   = dpad_vert;
	dpad.ngeo   = dpad_norm;
	dpad.ugeo   = dpad_uv;
	dpad.face_count = DPAD_POINT_COUNT;
	
	startb.pos.x = 0; startb.pos.y = 20; startb.pos.z = -176;
	startb.rot.x = 0; startb.rot.y = 0; startb.rot.z = 0;
	startb.sca.x = 0; startb.sca.x = 0; startb.sca.x = 0;	
	startb.vfaces = start_points;
	startb.nfaces = start_normpoints;
	startb.ufaces = start_uvpoints;
	startb.vgeo   = start_vert;
	startb.ngeo   = start_norm;
	startb.ugeo   = start_uv;
	startb.face_count = START_POINT_COUNT;
	
	abut.pos.x = 56; abut.pos.y = 15; abut.pos.z = -168;
	abut.rot.x = 0; abut.rot.y = 0; abut.rot.z = 0;
	abut.sca.x = 0; abut.sca.x = 0; abut.sca.x = 0;	
	abut.vfaces = a_points;
	abut.nfaces = a_normpoints;
	abut.ufaces = a_uvpoints;
	abut.vgeo   = a_vert;
	abut.ngeo   = a_norm;
	abut.ugeo   = a_uv;
	abut.face_count = A_POINT_COUNT;
	
	bbut.pos.x = 36; bbut.pos.y = 4; bbut.pos.z = -170;
	bbut.rot.x = 0; bbut.rot.y = 0; bbut.rot.z = 0;
	bbut.sca.x = 0; bbut.sca.x = 0; bbut.sca.x = 0;	
	bbut.vfaces = b_points;
	bbut.nfaces = b_normpoints;
	bbut.ufaces = b_uvpoints;
	bbut.vgeo   = b_vert;
	bbut.ngeo   = b_norm;
	bbut.ugeo   = b_uv;
	bbut.face_count = B_POINT_COUNT;
	
	xbut.pos.x = 75; xbut.pos.y = 20; xbut.pos.z = -170;
	xbut.rot.x = 0; xbut.rot.y = 0; xbut.rot.z = 0;
	xbut.sca.x = 0; xbut.sca.x = 0; xbut.sca.x = 0;	
	xbut.vfaces = x_points;
	xbut.nfaces = x_normpoints;
	xbut.ufaces = x_uvpoints;
	xbut.vgeo   = x_vert;
	xbut.ngeo   = x_norm;
	xbut.ugeo   = x_uv;
	xbut.face_count = X_POINT_COUNT;
	
	ybut.pos.x = 50; ybut.pos.y = 33; ybut.pos.z = -170;
	ybut.rot.x = 0; ybut.rot.y = 0; ybut.rot.z = 0;
	ybut.sca.x = 0; ybut.sca.x = 0; ybut.sca.x = 0;	
	ybut.vfaces = y_points;
	ybut.nfaces = y_normpoints;
	ybut.ufaces = y_uvpoints;
	ybut.vgeo   = y_vert;
	ybut.ngeo   = y_norm;
	ybut.ugeo   = y_uv;
	ybut.face_count = Y_POINT_COUNT;
	
	zbut.pos.x = 50; zbut.pos.y = 59; zbut.pos.z = -178;
	zbut.rot.x = 0; zbut.rot.y = 0; zbut.rot.z = 0;
	zbut.sca.x = 0; zbut.sca.x = 0; zbut.sca.x = 0;	
	zbut.vfaces = z_points;
	zbut.nfaces = z_normpoints;
	zbut.ufaces = z_uvpoints;
	zbut.vgeo   = z_vert;
	zbut.ngeo   = z_norm;
	zbut.ugeo   = z_uv;
	zbut.face_count = Z_POINT_COUNT;
	
	rtrig.pos.x = 50; rtrig.pos.y = 60; rtrig.pos.z = -200;
	rtrig.rot.x = 0; rtrig.rot.y = 0; rtrig.rot.z = 0;
	rtrig.sca.x = 0; rtrig.sca.x = 0; rtrig.sca.x = 0;	
	rtrig.vfaces = r_points;
	rtrig.nfaces = r_normpoints;
	rtrig.ufaces = r_uvpoints;
	rtrig.vgeo   = r_vert;
	rtrig.ngeo   = r_norm;
	rtrig.ugeo   = r_uv;
	rtrig.face_count = R_POINT_COUNT;
	
	ltrig.pos.x = -50; ltrig.pos.y = 60; ltrig.pos.z = -200;
	ltrig.rot.x = 0; ltrig.rot.y = 0; ltrig.rot.z = 0;
	ltrig.sca.x = 0; ltrig.sca.x = 0; ltrig.sca.x = 0;	
	ltrig.vfaces = l_points;
	ltrig.nfaces = l_normpoints;
	ltrig.ufaces = l_uvpoints;
	ltrig.vgeo   = l_vert;
	ltrig.ngeo   = l_norm;
	ltrig.ugeo   = l_uv;
	ltrig.face_count = L_POINT_COUNT;
}

void obj_update()
{
	PAD_ScanPads(); 
	
	astick.rot.x = -26.0f*((f32)PAD_StickY(0)/101.0f);
	astick.rot.y = 26.0f*((f32)PAD_StickX(0)/101.0f); 
	
	cstick.rot.x = -26.0f*((f32)PAD_SubStickY(0)/101.0f);
	cstick.rot.y = 26.0f*((f32)PAD_SubStickX(0)/101.0f);
	
	rtrig.pos.y = 55 + ((f32)PAD_TriggerR(0)/255) * (48 - 55); 
	ltrig.pos.y = 55 + ((f32)PAD_TriggerL(0)/255) * (48 - 55); 
		
	if      (PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT)  { dpad.rot.y = -6; }
	else if (PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) { dpad.rot.y =  6; }
	else                                            { dpad.rot.y =  0; }
		
	if      (PAD_ButtonsHeld(0) & PAD_BUTTON_UP)   { dpad.rot.x = -6; }
	else if (PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) { dpad.rot.x =  6; }
	else                                           { dpad.rot.x =  0; }

	if (PAD_ButtonsHeld(0) & PAD_BUTTON_START) { startb.pos.z = -178; }
	else                                       { startb.pos.z = -176; }
		
	if (PAD_ButtonsHeld(0) & PAD_BUTTON_A) { abut.pos.z = -170; }
	else                                   { abut.pos.z = -168; }
		
	if (PAD_ButtonsHeld(0) & PAD_BUTTON_B) { bbut.pos.z = -172; }
	else                                   { bbut.pos.z = -170; }
		
	if (PAD_ButtonsHeld(0) & PAD_BUTTON_X) { xbut.pos.z = -172; }
	else                                   { xbut.pos.z = -170; }
		
	if (PAD_ButtonsHeld(0) & PAD_BUTTON_Y) { ybut.pos.z = -172; }
	else                                   { ybut.pos.z = -170; }
	
	if (PAD_ButtonsHeld(0) & PAD_TRIGGER_Z) { zbut.pos.y = 57; }
	else                                    { zbut.pos.y = 59; }
}

int main(int argc, char** argv)
{
	gcn_init(); 
	draw_init();
	obj_init(); 
	
	while(1)
	{
		PAD_ScanPads(); 
		
		/*Hold up and press start to end program*/
		if(PAD_ButtonsDown(0) & PAD_BUTTON_START && 
		   PAD_ButtonsHeld(0) & PAD_BUTTON_UP)
		{
			break;
		}
		
		obj_update();
		render(); 
	}
	 
	return 0; 
}
