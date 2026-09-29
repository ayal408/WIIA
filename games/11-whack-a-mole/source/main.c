/*
 * Whack-a-Mole
 * Moles pop up briefly at random spots on screen; point the Wii Remote and
 * press A on top of one before it ducks back down. Demonstrates IR pointer
 * input combined with a "spawn, live briefly, despawn" timing pattern, with
 * an automatic fallback to a D-pad/Nunchuk-driven cursor when no sensor
 * bar is available.
 *
 * Controls:
 *   Point Wii Remote at screen (or D-pad / Nunchuk if no sensor bar) -> move crosshair
 *   A                          -> whack
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
#define HOLE_RADIUS 35
#define NUM_HOLES 6
#define UP_FRAMES_MIN 30
#define UP_FRAMES_MAX 70
#define DPAD_SPEED 6
#define NUNCHUK_MAX_SPEED 10

typedef struct {
	int x, y;
	int up;          /* 1 while the mole is poking up and whackable */
	int timer;       /* frames left in the current state */
	int next_wait;   /* frames to wait before popping up again */
} Hole;

static void schedule_pop(Hole *h) {
	h->up = 0;
	h->timer = 20 + (rand() % 90);
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

	srand(2468);

	Hole holes[NUM_HOLES];
	int cols = 3, rows = 2;
	for (int i = 0; i < NUM_HOLES; i++) {
		int cx = (i % cols);
		int cy = (i / cols);
		holes[i].x = (SCREEN_W / cols) * cx + (SCREEN_W / cols) / 2;
		holes[i].y = (SCREEN_H / rows) * cy + (SCREEN_H / rows) / 2;
		schedule_pop(&holes[i]);
	}

	int score = 0;
	int missed_time = 0; /* moles that ducked back down unwhacked */

	float cx = SCREEN_W / 2.0f;
	float cy = SCREEN_H / 2.0f;

	printf("\x1b[2;0H");
	printf("=== WHACK-A-MOLE ===\n");
	printf("Point at a mole marked '^' (or use D-pad/Nunchuk) and press A before it ducks. HOME to quit.\n\n");

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

		for (int i = 0; i < NUM_HOLES; i++) {
			if (--holes[i].timer <= 0) {
				if (holes[i].up) {
					holes[i].up = 0;
					missed_time++;
					holes[i].timer = 30 + (rand() % 90);
				} else {
					holes[i].up = 1;
					holes[i].timer = UP_FRAMES_MIN + (rand() % (UP_FRAMES_MAX - UP_FRAMES_MIN));
				}
			}
		}

		if (pressed & WPAD_BUTTON_A) {
			for (int i = 0; i < NUM_HOLES; i++) {
				if (!holes[i].up) continue;
				int dx = crosshair_x - holes[i].x;
				int dy = crosshair_y - holes[i].y;
				if (dx * dx + dy * dy <= HOLE_RADIUS * HOLE_RADIUS) {
					score++;
					holes[i].up = 0;
					holes[i].timer = 30 + (rand() % 90);
					break;
				}
			}
		}

		printf("\x1b[6;0H");
		printf("Crosshair: (%4d, %4d)  [%s]        \n", crosshair_x, crosshair_y,
		       using_ir ? "IR pointer" : "D-pad/Nunchuk");
		printf("\n");
		for (int i = 0; i < NUM_HOLES; i++) {
			printf("Hole %d: (%3d,%3d) %s     \n", i + 1, holes[i].x, holes[i].y,
			       holes[i].up ? "^ UP! whack it" : ". down");
		}
		printf("\nScore: %-5d   Missed: %-5d\n", score, missed_time);

		VIDEO_WaitVSync();
	}

	return 0;
}
