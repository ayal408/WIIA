/*
 * Pointer Shooter
 * Point the Wii Remote at the screen and press A when the crosshair
 * is inside the target zone to score. Demonstrates WPAD IR (pointer) input,
 * with an automatic fallback to a D-pad/Nunchuk-driven cursor when no
 * sensor bar is available - so this plays fine either way.
 *
 * Controls:
 *   Point Wii Remote at screen (or D-pad / Nunchuk if no sensor bar) -> move crosshair
 *   A                           -> shoot
 *   HOME                        -> quit
 */

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

#define SCREEN_W 640
#define SCREEN_H 480
#define TARGET_RADIUS 40
#define DPAD_SPEED 6
#define NUNCHUK_MAX_SPEED 10

typedef struct {
	int x, y;
	int alive;
} Target;

static void new_target(Target *t) {
	t->x = TARGET_RADIUS + (rand() % (SCREEN_W - 2 * TARGET_RADIUS));
	t->y = TARGET_RADIUS + (rand() % (SCREEN_H - 2 * TARGET_RADIUS));
	t->alive = 1;
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

	/* We want buttons + IR (pointer) data from the Wii Remote */
	WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);
	WPAD_SetVRes(WPAD_CHAN_0, SCREEN_W, SCREEN_H);

	srand(0xC0FFEE);

	Target target;
	new_target(&target);
	int score = 0;
	int misses = 0;

	/* Fallback cursor position, used whenever the IR pointer isn't valid
	   (no sensor bar in view). Starts centered. */
	float cx = SCREEN_W / 2.0f;
	float cy = SCREEN_H / 2.0f;

	printf("\x1b[2;0H");
	printf("=== POINTER SHOOTER ===\n");
	printf("Point the Wiimote at the TV (or use D-pad/Nunchuk if no sensor bar). A to shoot, HOME to quit.\n\n");

	while (1) {
		WPAD_ScanPads();

		u32 pressed = WPAD_ButtonsDown(WPAD_CHAN_0);
		u32 held = WPAD_ButtonsHeld(WPAD_CHAN_0);
		if (pressed & WPAD_BUTTON_HOME) break;

		struct ir_t ir;
		WPAD_IR(WPAD_CHAN_0, &ir);

		int crosshair_x, crosshair_y;
		int using_ir = ir.valid;

		if (using_ir) {
			crosshair_x = (int)ir.x;
			crosshair_y = (int)ir.y;
			cx = (float)crosshair_x;
			cy = (float)crosshair_y;
		} else {
			/* No sensor bar in view - drive the cursor with the D-pad,
			   and faster with a Nunchuk analog stick if one is attached. */
			if (held & WPAD_BUTTON_LEFT)  cx -= DPAD_SPEED;
			if (held & WPAD_BUTTON_RIGHT) cx += DPAD_SPEED;
			if (held & WPAD_BUTTON_UP)    cy -= DPAD_SPEED;
			if (held & WPAD_BUTTON_DOWN)  cy += DPAD_SPEED;

			struct expansion_t exp;
			WPAD_Expansion(WPAD_CHAN_0, &exp);
			if (exp.type == WPAD_EXP_NUNCHUK) {
				float mag = exp.nunchuk.js.mag;
				float ang = exp.nunchuk.js.ang;
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

			crosshair_x = (int)cx;
			crosshair_y = (int)cy;
		}

		printf("\x1b[6;0H");
		printf("Crosshair: (%4d, %4d)  [%s]        \n", crosshair_x, crosshair_y,
		       using_ir ? "IR pointer" : "D-pad/Nunchuk");
		printf("Target:    (%4d, %4d)  radius %d\n", target.x, target.y, TARGET_RADIUS);
		printf("Score: %-4d   Misses: %-4d\n", score, misses);

		if (pressed & WPAD_BUTTON_A) {
			int dx = crosshair_x - target.x;
			int dy = crosshair_y - target.y;
			if (dx * dx + dy * dy <= TARGET_RADIUS * TARGET_RADIUS) {
				score++;
				new_target(&target);
			} else {
				misses++;
			}
		}

		VIDEO_WaitVSync();
	}

	return 0;
}
