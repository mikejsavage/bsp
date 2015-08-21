#ifndef _KEYS_H_
#define _KEYS_H_

#include "intrinsics.h"

// TODO: decide whether enum class is too annoying
// TODO: decide this layout properly
enum KeyNames {
	KEY_NONE,

	// KEY_A,
	// KEY_B,
	// KEY_C,
	// KEY_D,
	// KEY_E,
	// KEY_F,
	// KEY_G,
	// KEY_H,
	// KEY_I,
	// KEY_J,
	// KEY_K,
	// KEY_L,
	// KEY_M,
	// KEY_N,
	// KEY_O,
	// KEY_P,
	// KEY_Q,
	// KEY_R,
	// KEY_S,
	// KEY_T,
	// KEY_U,
	// KEY_V,
	// KEY_W,
	// KEY_X,
	// KEY_Y,
	// KEY_Z,
        //
	// KEY_1,
	// KEY_2,
	// KEY_3,
	// KEY_4,
	// KEY_5,
	// KEY_6,
	// KEY_7,
	// KEY_8,
	// KEY_9,
	// KEY_0,

	KEY_KP1 = 127,
	KEY_KP2,
	KEY_KP3,
	KEY_KP4,
	KEY_KP5,
	KEY_KP6,
	KEY_KP7,
	KEY_KP8,
	KEY_KP9,
	KEY_KP0,

	KEY_KP_PLUS,
	KEY_KP_MINUS,
	KEY_KP_STAR,
	KEY_KP_SLASH,
	KEY_KP_DOT,
	KEY_KP_ENTER,

	KEY_UPARROW,
	KEY_DOWNARROW,
	KEY_LEFTARROW,
	KEY_RIGHTARROW,

	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,

	KEY_LEFTCTRL,
	KEY_RIGHTCTRL,
	KEY_LEFTSHIFT,
	KEY_RIGHTSHIFT,
	KEY_LEFTALT,
	KEY_RIGHTALT,

	KEY_SPACE,
	KEY_BACKSPACE,
	KEY_RETURN,

	KEY_PAGEUP,
	KEY_PAGEDOWN,
	KEY_INSERT,
	KEY_DELETE,
	KEY_HOME,
	KEY_END,

	KEY_COUNT,
};

#endif // _KEYS_H_
