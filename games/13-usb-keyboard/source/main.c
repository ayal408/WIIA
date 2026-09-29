/*
 * USB Keyboard Demo
 * Plug a standard USB keyboard into the Wii's USB port and type - the
 * characters you type are echoed to the screen. Demonstrates reading a
 * real USB keyboard via libogc's wiikeyboard module, which any game in
 * this repo can reuse for text entry (high-score names, chat, etc.)
 * instead of an on-screen button-driven keyboard.
 *
 * Controls:
 *   Any USB keyboard plugged into the Wii -> types characters on screen
 *   ESC (on the USB keyboard) or HOME (on a Wii Remote) -> quit
 */

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <wiikeyboard/keyboard.h>
#include <stdio.h>
#include <string.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

#define LINE_LEN 64

int main(int argc, char **argv) {
	VIDEO_Init();
	WPAD_Init();
	KEYBOARD_Init(NULL);

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

	char line[LINE_LEN + 1];
	int len = 0;
	line[0] = '\0';

	printf("\x1b[2;0H");
	printf("=== USB KEYBOARD DEMO ===\n");
	printf("Plug a USB keyboard into the Wii and start typing.\n");
	printf("Backspace deletes, Enter clears the line.\n");
	printf("ESC on the keyboard, or HOME on a Wii Remote, quits.\n\n");

	int quit = 0;
	while (!quit) {
		WPAD_ScanPads();
		if (WPAD_ButtonsDown(WPAD_CHAN_0) & WPAD_BUTTON_HOME) break;

		keyboard_event event;
		while (KEYBOARD_GetEvent(&event)) {
			if (event.type != KEYBOARD_PRESSED) continue;

			if (event.symbol == KS_Escape) {
				quit = 1;
			} else if (event.symbol == KS_BackSpace) {
				if (len > 0) line[--len] = '\0';
			} else if (event.symbol == KS_Return || event.symbol == KS_KP_Enter) {
				len = 0;
				line[0] = '\0';
			} else if (event.symbol >= 0x20 && event.symbol < 0x7f && len < LINE_LEN) {
				/* printable ASCII */
				line[len++] = (char)event.symbol;
				line[len] = '\0';
			}
		}

		printf("\x1b[8;0H");
		printf("You typed: %-64s\n", line);

		VIDEO_WaitVSync();
	}

	return 0;
}
