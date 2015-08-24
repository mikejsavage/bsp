#ifndef _ASSETS_H_
#define _ASSETS_H_

#include "platform_opengl.h"

enum AssetType {
	AT_BITMAP,
	AT_SOUND,
};

enum AssetHandle {
	// bitmaps
	ASSET_FONT,

	// sounds

	ASSET_COUNT,
};

struct Bitmap {
	u32 width;
	u32 height;
	u8 * data;
};

struct Sound {
	u32 num_samples;
	u8 * data;
};

struct Asset {
	AssetType type;
	union {
		struct {
			Bitmap bitmap;
			GLuint texture;
		};
		Sound sound;
	};
};

#endif // _ASSETS_H_
