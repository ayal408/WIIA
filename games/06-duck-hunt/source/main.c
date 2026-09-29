/*
 * Duck Hunt (original)
 * Several targets move around the screen; point the Wii Remote and press B
 * (the "trigger" button, under the Wiimote like a real light gun) to shoot
 * whichever target the pointer is over. Demonstrates IR pointer input
 * combined with simple moving-target logic, with an automatic fallback to
 * a D-pad/Nunchuk-driven cursor when no sensor bar is available.
 *
 * Controls:
 *   Point Wii Remote at screen (or D-pad / Nunchuk if no sensor bar) -> move crosshair
 *   B                          -> shoot
 *   HOME                       -> quit
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
#define TARGET_RADIUS 30
#define NUM_TARGETS 3
#define DPAD_SPEED 6
#define NUNCHUK_MAX_SPEED 10

typedef struct {
	int x, y;
	int vx, vy;
} Duck;

static void respawn(Duck *d) {
	d->x = TARGET_RADIUS + (rand() % (SCREEN_W - 2 * TARGET_RADIUS));
	d->y = TARGET_RADIUS + (rand() % (SCREEN_H - 2 * TARGET_RADIUS));
	d->vx = (rand() % 5) - 2;
	d->vy = (rand() % 5) - 2;
	if (d->vx == 0 && d->vy == 0) d->vx = 1;
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

	WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);
	WPAD_SetVRes(WPAD_CHAN_0, SCREEN_W, SCREEN_H);

	srand(99);

	Duck ducks[NUM_TARGETS];
	for (int i = 0; i < NUM_TARGETS; i++) respawn(&ducks[i]);

	int score = 0;
	int shots = 0;

	float cx = SCREEN_W / 2.0f;
	float cy = SCREEN_H / 2.0f;

	printf("\x1b[2;0H");
	printf("=== DUCK HUNT ===\n");
	printf("Point at a moving target (or use D-pad/Nunchuk) and press B to shoot. HOME to quit.\n\n");

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

		for (int i = 0; i < NUM_TARGETS; i++) {
			ducks[i].x += ducks[i].vx;
			ducks[i].y += ducks[i].vy;
			if (ducks[i].x < TARGET_RADIUS || ducks[i].x > SCREEN_W - TARGET_RADIUS) ducks[i].vx = -ducks[i].vx;
			if (ducks[i].y < TARGET_RADIUS || ducks[i].y > SCREEN_H - TARGET_RADIUS) ducks[i].vy = -ducks[i].vy;
		}

		if (pressed & WPAD_BUTTON_B) {
			shots++;
			for (int i = 0; i < NUM_TARGETS; i++) {
				int dx = crosshair_x - ducks[i].x;
				int dy = crosshair_y - ducks[i].y;
				if (dx * dx + dy * dy <= TARGET_RADIUS * TARGET_RADIUS) {
					score++;
					respawn(&ducks[i]);
					break;
				}
			}
		}

		printf("\x1b[6;0H");
		printf("Crosshair: (%4d, %4d)  [%s]        \n", crosshair_x, crosshair_y,
		       using_ir ? "IR pointer" : "D-pad/Nunchuk");
		for (int i = 0; i < NUM_TARGETS; i++) {
			printf("Target %d: (%4d, %4d)   \n", i + 1, ducks[i].x, ducks[i].y);
		}
		printf("\nHits: %-4d   Shots: %-4d   Accuracy: %3d%%\n",
		       score, shots, shots ? (score * 100 / shots) : 0);

		VIDEO_WaitVSync();
	}

	return 0;
}
