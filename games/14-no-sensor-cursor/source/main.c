/*
 * No-Sensor Cursor
 * A "mouse" cursor you can move and click WITHOUT the IR sensor bar -
 * steered entirely with the Wii Remote's D-pad (and sped up further with
 * a Nunchuk analog stick, if one is plugged in). Useful when you don't
 * have a sensor bar handy, or want menu/pointer-style control that still
 * works if the Wiimote isn't pointed at the TV. The same technique (a
 * plain x/y position moved by WPAD_ButtonsHeld) can replace WPAD_IR in
 * any of the pointer-based games in this repo (01, 06, 11).
 *
 * Controls:
 *   D-pad             -> move the cursor
 *   Nunchuk stick      -> move the cursor faster, if a Nunchuk is attached
 *   A                  -> click (try to hit the target)
 *   HOME               -> quit
 */

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

#define SCREEN_W 640
#define SCREEN_H 480
#define TARGET_RADIUS 35
#define DPAD_SPEED 6
#define NUNCHUK_MAX_SPEED 10

typedef struct { int x, y; } Target;

static void respawn(Target *t) {
	t->x = TARGET_RADIUS + (rand() % (SCREEN_W - 2 * TARGET_RADIUS));
	t->y = TARGET_RADIUS + (rand() % (SCREEN_H - 2 * TARGET_RADIUS));
}

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

	/* No WPAD_FMT_..._IR here on purpose - this demo never touches the
	   sensor bar / IR camera at all. */
	WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC);

	srand(31415);

	float cx = SCREEN_W / 2.0f;
	float cy = SCREEN_H / 2.0f;
	Target target;
	respawn(&target);
	int score = 0;

	printf("\x1b[2;0H");
	printf("=== NO-SENSOR CURSOR ===\n");
	printf("Move the cursor with the D-pad (or a Nunchuk stick), A to click. HOME to quit.\n\n");

	while (1) {
		WPAD_ScanPads();
		u32 held = WPAD_ButtonsHeld(WPAD_CHAN_0);
		u32 pressed = WPAD_ButtonsDown(WPAD_CHAN_0);
		if (pressed & WPAD_BUTTON_HOME) break;

		if (held & WPAD_BUTTON_LEFT)  cx -= DPAD_SPEED;
		if (held & WPAD_BUTTON_RIGHT) cx += DPAD_SPEED;
		if (held & WPAD_BUTTON_UP)    cy -= DPAD_SPEED;
		if (held & WPAD_BUTTON_DOWN)  cy += DPAD_SPEED;

		struct expansion_t exp;
		WPAD_Expansion(WPAD_CHAN_0, &exp);
		if (exp.type == WPAD_EXP_NUNCHUK) {
			float mag = exp.nunchuk.js.mag;   /* 0.0 .. ~1.0 */
			float ang = exp.nunchuk.js.ang;   /* degrees, 0 = up, clockwise */
			if (mag > 0.15f) {
				float rad = ang * (M_PI / 180.0f);
				cx += sinf(rad) * mag * NUNCHUK_MAX_SPEED;
				cy -= cosf(rad) * mag * NUNCHUK_MAX_SPEED;
			}
		}

		if (cx < 0) cx = 0;
		if (cx > SCREEN_W - 1) cx = SCREEN_W - 1;
		if (cy < 0) cy = 0;
		if (cy > SCREEN_H - 1) cy = SCREEN_H - 1;

		if (pressed & WPAD_BUTTON_A) {
			int dx = (int)cx - target.x;
			int dy = (int)cy - target.y;
			if (dx * dx + dy * dy <= TARGET_RADIUS * TARGET_RADIUS) {
				score++;
				respawn(&target);
			}
		}

		printf("\x1b[6;0H");
		printf("Cursor: (%4d, %4d)               \n", (int)cx, (int)cy);
		printf("Target: (%4d, %4d)  radius %d\n", target.x, target.y, TARGET_RADIUS);
		printf("Nunchuk: %-20s\n", (exp.type == WPAD_EXP_NUNCHUK) ? "connected" : "not connected (D-pad only)");
		printf("Score: %-5d\n", score);

		VIDEO_WaitVSync();
	}

	return 0;
}
