#include <gccore.h>
#include <ogcsys.h>

//---------------------------------------------------------------------------------
typedef struct tagKeyInput{
	vs16	up,
			down,
			state,
			oldstate;
	}__attribute__ ((packed)) KeyInput;

static volatile KeyInput Keys[4] = { {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};
static PADStatus pad[4];


//---------------------------------------------------------------------------------
void ScanPads() {
//---------------------------------------------------------------------------------

	PAD_Read(pad);
	int index;

	for ( index = 0; index < 4; index++) {

		Keys[index].oldstate	= Keys[index].state;
		Keys[index].state		= pad[index].button;
		Keys[index].up			= Keys[index].oldstate & ~Keys[index].state;
		Keys[index].down		= ~Keys[index].oldstate & Keys[index].state;
	}			
}

//---------------------------------------------------------------------------------
u16 ButtonsUp(int pad) {
//---------------------------------------------------------------------------------
	if (pad<0 || pad >3) return 0;
	
	return 	Keys[pad].up;
}
//---------------------------------------------------------------------------------
u16 ButtonsDown(int pad) {
//---------------------------------------------------------------------------------
	if (pad<0 || pad >3) return 0;

	return 	Keys[pad].down;
}

//---------------------------------------------------------------------------------
u16 ButtonsHeld(int pad) {
//---------------------------------------------------------------------------------
	if (pad<0 || pad >3) return 0;

	return Keys[pad].state;
}
