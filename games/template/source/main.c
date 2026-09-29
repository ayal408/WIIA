/*
 * Template
 * Minimal starting point for a new Wii homebrew game.
 * Copy the whole "template" folder, rename it, and start editing here.
 *
 * Controls:
 *   A     -> example action (prints a message)
 *   HOME  -> quit
 */

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <stdlib.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

int main(int argc, char **argv) {
	VIDEO_Init();
	WPAD_Init();

	rmode = VIDEO_GetPreferredMode(NULL);
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight,
	             rmode->fbWidth * VI_DISPLAY_PIX_SZ);

	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

	/* Ask for buttons + accelerometer + IR pointer data.
	   Swap/extend this if your game also needs a Nunchuk, Classic
	   Controller, or Balance Board - see the other example games. */
	WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);
	WPAD_SetVRes(WPAD_CHAN_0, rmode->fbWidth, rmode->xfbHeight);

	printf("\x1b[2;0H");
	printf("=== NEW GAME TEMPLATE ===\n");
	printf("Press A to test input. HOME to quit.\n\n");

	int presses = 0;

	while (1) {
		WPAD_ScanPads();

		u32 pressed = WPAD_ButtonsDown(WPAD_CHAN_0);
		if (pressed & WPAD_BUTTON_HOME) break;

		if (pressed & WPAD_BUTTON_A) {
			presses++;
		}

		printf("\x1b[6;0H");
		printf("A pressed %d times so far.   \n", presses);

		VIDEO_WaitVSync();
	}

	return 0;
}
