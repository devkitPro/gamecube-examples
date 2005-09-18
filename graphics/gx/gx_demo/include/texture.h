#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <gccore.h>

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

extern u8 textureI8[];
extern u8 textureRGBA8[];
extern u8 textureRGB5A3[];
extern u16 textureRGB5A3LOD[];

extern u32 textureI8sz;
extern u32 textureRGBA8sz;
extern u32 textureRGB5A3sz;
extern u32 ttextureRGB5A3LODsz;

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif
