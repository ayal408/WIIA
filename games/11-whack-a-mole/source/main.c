/*
 * Whack-a-Mole
 * Moles pop up briefly at random spots on screen; point the Wii Remote and
 * press A on top of one before it ducks back down. Demonstrates IR pointer
 * input combined with a "spawn, live briefly, despawn" timing pattern.
 *
 * Controls:
 *   Point Wii Remote at screen -> move crosshair
 *   A                          -> whack
 *   HOME                       -> quit
 */

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <stdlib.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

#define SCREEN_W 640
#define SCREEN_H 480
#define HOLE_RADIUS 35
#define NUM_HOLES 6
#define UP_FRAMES_MIN 30
#define UP_FRAMES_MAX 70

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

	printf("\x1b[2;0H");
	printf("=== WHACK-A-MOLE ===\n");
	printf("Point at a mole marked '^' and press A before it ducks. HOME to quit.\n\n");

	while (1) {
		WPAD_ScanPads();
		u32 pressed = WPAD_ButtonsDown(WPAD_CHAN_0);
		if (pressed & WPAD_BUTTON_HOME) break;

		struct ir_t ir;
		WPAD_IR(WPAD_CHAN_0, &ir);

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

		if (ir.valid && (pressed & WPAD_BUTTON_A)) {
			for (int i = 0; i < NUM_HOLES; i++) {
				if (!holes[i].up) continue;
				int dx = (int)ir.x - holes[i].x;
				int dy = (int)ir.y - holes[i].y;
				if (dx * dx + dy * dy <= HOLE_RADIUS * HOLE_RADIUS) {
					score++;
					holes[i].up = 0;
					holes[i].timer = 30 + (rand() % 90);
					break;
				}
			}
		}

		printf("\x1b[6;0H");
		if (ir.valid) {
			printf("Crosshair: (%4d, %4d)      \n", (int)ir.x, (int)ir.y);
		} else {
			printf("Crosshair: point Wiimote at the sensor bar...   \n");
		}
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
