#include <gccore.h>
#include <ogcsys.h>

typedef struct tagKeyInput{
	vu16 Up,
		Down,
		Held,
		Last,
		DownRepeat;
	}__attribute__ ((packed)) KeyInput;

static volatile KeyInput Keys = { 0,0,0,0,0 };
PADStatus pad[4];


//---------------------------------------------------------------------------------
u16	KeysDown(void) {
//---------------------------------------------------------------------------------
	u16 tmp = Keys.Down;
	Keys.DownRepeat = 0;
	Keys.Down =0;
	return tmp;
}

//---------------------------------------------------------------------------------
u16	KeysUp(void) {
//---------------------------------------------------------------------------------
	u16 tmp = Keys.Up;
	Keys.Up = 0;

	return tmp;
}

//---------------------------------------------------------------------------------
u16	KeysHeld(void) {
//---------------------------------------------------------------------------------
	return Keys.Held;
}

//---------------------------------------------------------------------------------
void ScanKeys() {
//---------------------------------------------------------------------------------
	PAD_Read(pad);
	Keys.Last = Keys.Held;
	Keys.Held = pad[0].button;

	if(pad[0].button&PAD_BUTTON_START) {
		void (*reload)() = (void(*)())0x80001800;
		reload();
	}

	u16 pressed = Keys.Held & ( ~Keys.Last);

	Keys.DownRepeat	|=	pressed;
	Keys.Down |= pressed;


	u16 released = ((~Keys.Held) & Keys.Last);

	Keys.Up		|=	released;

	Keys.Down	&= ~released;
	Keys.DownRepeat	&= ~released;

	Keys.Up &= ~pressed;

/*	if ( Keys.Last != Keys.Held) count = delay;


	if ( delay != 0)
	{
		count--;
		if (count == 0)
		{
			count = repeat;
			Keys.DownRepeat |= Keys.Held;
		}
	}
*/
}
