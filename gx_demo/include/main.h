#ifndef __MAIN_H__
#define __MAIN_H__

#include <gccore.h>

typedef struct _camcfg {
	Vector campos;
	Vector camup;
	Vector target;
	f32 fovy,aspect;
	f32 left,top;
	f32 znear,zfar;
} CamCfg;

typedef struct _viewobj {
	CamCfg cfg;
	Mtx44 proj;
	Mtx view;
} ViewObj;

typedef struct _litobj {
	GXLightObj lit;
	Vector pos;
	Vector dir;
	GXColor litcol;
	s32 theta,phi;
	u8 enable;
} LitObj;

typedef struct _scenectrl {
	ViewObj viewobj;
	LitObj litobj;
	GXRModeObj *rmode;
	Mtx model;
	f32 scale;
	u8 tevmode;
} SceneCtrl;

#define CLAMP(val,min,max) \
	((val) = (((val) < (min)) ? (min) : ((val) > (max)) ? (max) : (val)))

#endif
