/*---------------------------------------------------------------------------------

	a simple rotating cube demo by tkcne

---------------------------------------------------------------------------------*/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <gcmodplay.h>
#include <debug.h>
#include <ogcsys.h>
 
#define DEFAULT_FIFO_SIZE	(256*1024)		//GX_FIFO_MINSIZE
 
//---------------------------------------------------------------------------------
// cube vertex data
//---------------------------------------------------------------------------------
s16 cube[] ATTRIBUTE_ALIGN(32) = {
	// x y z
	-30,  30, -30, 	// 0
	 30,  30, -30, 	// 1
	 30,  30,  30, 	// 2
	-30,  30,  30, 	// 3
	 30, -30, -30, 	// 4
	 30, -30,  30, 	// 5
	-30, -30,  30,  // 6
	-30, -30, -30,  // 7
};
 

//---------------------------------------------------------------------------------
// color data
//---------------------------------------------------------------------------------
u8 colors[] ATTRIBUTE_ALIGN(32) = {
	// r, g, b, a
	100,  10, 100, 255, // 0 purple
	240,   0,   0, 255,	// 1 red
	255, 180,   0, 255,	// 2 orange
	255, 255,   0, 255, // 3 yellow
	 10, 120,  40, 255, // 4 green
	  0,  20, 100, 255  // 5 blue
};
 
 
static u32 curr_fb = 0;
static u32 first_frame = 1;
static void *xfb[2] = {NULL,NULL};
GXRModeObj *rmode;
 
void draw_init();
void draw_cube(Mtx v);
 
//---------------------------------------------------------------------------------
int main( int argc, char **argv ){
//---------------------------------------------------------------------------------
	f32 yscale;
	u32 xfbHeight;
	Mtx v,p; // view and perspective matrices
	GXColor background = {0, 0, 0, 0xff};
 
	// init the vi. setup frame buffer and set the retrace callback
	// to copy the efb to xfb
	VIDEO_Init();
	PAD_Init();
 
	rmode = VIDEO_GetPreferredMode(NULL);
 
 
	curr_fb = 0;
	first_frame = 0;
 
	// setup the fifo and then init the flipper
	void *gp_fifo = NULL;
	gp_fifo = memalign(32,DEFAULT_FIFO_SIZE);
	memset(gp_fifo,0,DEFAULT_FIFO_SIZE);
 
	xfb[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	xfb[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb[curr_fb]);
	if(!first_frame) VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	console_init(xfb[curr_fb],20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
 
	//curr_fb ^= 1;
 
	GX_Init(gp_fifo,DEFAULT_FIFO_SIZE);
 
	// clears the bg to color and clears the z buffer
	GX_SetCopyClear(background, 0x00ffffff);
 
	// other gx setup
	GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
	yscale = GX_GetYScaleFactor(rmode->efbHeight,rmode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetScissor(0,0,rmode->fbWidth,rmode->efbHeight);
	GX_SetDispCopySrc(0,0,rmode->fbWidth,rmode->efbHeight);
	GX_SetDispCopyDst(rmode->fbWidth,xfbHeight);
	GX_SetCopyFilter(rmode->aa,rmode->sample_pattern,GX_TRUE,rmode->vfilter);
	GX_SetFieldMode(rmode->field_rendering,((rmode->viHeight==2*rmode->xfbHeight)?GX_ENABLE:GX_DISABLE));
 
	// cull none because other values produce weird results
	GX_SetCullMode(GX_CULL_NONE);
	GX_CopyDisp(xfb[curr_fb],GX_TRUE);
	GX_SetDispCopyGamma(GX_GM_1_0);
 
	// setup our camera at the origin
	// looking down the -z axis with y up
	guVector cam = {0.0F, 0.0F, 0.0F},
			up = {0.0F, 1.0F, 0.0F},
		  look = {0.0F, 0.0F, -1.0F};
	guLookAt(v, &cam, &up, &look);
 
	// setup our projection matrix
	// this creates a perspective matrix with a view angle of 60,
	// an aspect ratio of 4/3 (i'm not sure if that's the right
	// way to do it but i just went by what made a square on my screen)
	// and z near and far distances
    f32 w = rmode->viWidth;
    f32 h = rmode->viHeight;
	guPerspective(p, 60, (f32)w/h, 10.0F, 300.0F);
	GX_LoadProjectionMtx(p, GX_PERSPECTIVE);
 
	// setup vertexes
	draw_init();
 
	// main loop
	while(1) {

		PAD_ScanPads();

		if (PAD_ButtonsDown(0) & PAD_BUTTON_START) {
			void (*reload)() = (void(*)())0x90000020;
			reload();
		}

		// do this before drawing
		GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
		GX_InvVtxCache();
		GX_InvalidateTexAll();
 
		// draw our cube
		draw_cube(v);
 
		GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
		GX_SetColorUpdate(GX_TRUE);
		GX_CopyDisp(xfb[curr_fb],GX_TRUE);
 
		// do this stuff after drawing
		GX_DrawDone();
 
		VIDEO_SetNextFramebuffer(xfb[curr_fb]);
 
		if(first_frame) {
			first_frame = 0;
			VIDEO_SetBlack(FALSE);
		}	
		VIDEO_Flush();
 
		VIDEO_WaitVSync();
		curr_fb ^= 1;
 
	}
	return 0;
}
 
 
//---------------------------------------------------------------------------------
void draw_init() {
//---------------------------------------------------------------------------------
	// setup the vertex descriptor
	// tells the flipper to expect 8bit indexes for position
	// and color data. could also be set to direct.
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_INDEX8);
	GX_SetVtxDesc(GX_VA_CLR0, GX_INDEX8);
 
	// setup the vertex attribute table
	// describes the data
	// args: vat location 0-7, type of data, data format, size, scale
	// so for ex. in the first call we are sending position data with
	// 3 values X,Y,Z of size S16. scale sets the number of fractional
	// bits for non float data.
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
 
	// tells gx where our position and color data is
	// args: type of data, pointer, array stride
	GX_SetArray(GX_VA_POS, cube, 3*sizeof(s16));
	GX_SetArray(GX_VA_CLR0, colors, 4*sizeof(u8));
	DCFlushRange(cube,sizeof(cube));
	DCFlushRange(colors,sizeof(colors));
 
	// no idea...sets to no textures
	// i don't know anything about textures or lighting yet :|
	GX_SetNumChans(1);
	GX_SetNumTexGens(0);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
}
 
 
//---------------------------------------------------------------------------------
// draws a quad from 4 vertex idx and one color idx
//---------------------------------------------------------------------------------
void draw_quad(u8 v0, u8 v1, u8 v2, u8 v3, u8 c) {
//---------------------------------------------------------------------------------
	// one 8bit position idx
	GX_Position1x8(v0);
	// one 8bit color idx
	GX_Color1x8(c);
	GX_Position1x8(v1);
	GX_Color1x8(c);
	GX_Position1x8(v2);
	GX_Color1x8(c);
	GX_Position1x8(v3);
	GX_Color1x8(c);
}
 
 
//---------------------------------------------------------------------------------
void draw_cube(Mtx v) {
//---------------------------------------------------------------------------------
	Mtx m; // model matrix.
	Mtx mv; // modelview matrix.
	guVector axis = {-1,1,0};
	static float rotateby = 0;
 
	rotateby ++;
 
	// move the cube out in front of us and rotate it
	guMtxIdentity(m);
	guMtxRotAxisDeg(m, &axis, rotateby);
	guMtxTransApply(m, m, 0.0F, 0.0F, -200.0F);
	guMtxConcat(v,m,mv);
	// load the modelview matrix into matrix memory
	GX_LoadPosMtxImm(mv, GX_PNMTX0);
 
	// drawing begins!
	// tells the flipper what type of primitive we will be drawing
	// which descriptor in the VAT to use and the number of vertices
	// to expect. 24 since we will draw 6 quads with 4 verts each.
	GX_Begin(GX_QUADS, GX_VTXFMT0, 24);
 
		draw_quad(0, 3, 2, 1, 0);
		draw_quad(0, 7, 6, 3, 1);
		draw_quad(0, 1, 4, 7, 2);
		draw_quad(1, 2, 5, 4, 3);
		draw_quad(2, 3, 6, 5, 4);
		draw_quad(4, 7, 6, 5, 5);
 
	GX_End();
}
