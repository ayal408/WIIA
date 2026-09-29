/*
 * Pointer Shooter
 * Point the Wii Remote at the screen and press A when the crosshair
 * is inside the target zone to score. Demonstrates WPAD IR (pointer) input.
 *
 * Controls:
 *   Point Wii Remote at screen  -> move crosshair
 *   A                           -> shoot
 *   HOME                        -> quit
 */

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

#define SCREEN_W 640
#define SCREEN_H 480
#define TARGET_RADIUS 40

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

	printf("\x1b[2;0H");
	printf("=== POINTER SHOOTER ===\n");
	printf("Point the Wiimote at the TV. Press A over the target. HOME to quit.\n\n");

	while (1) {
		WPAD_ScanPads();

		u32 pressed = WPAD_ButtonsDown(WPAD_CHAN_0);
		if (pressed & WPAD_BUTTON_HOME) break;

		struct ir_t ir;
		WPAD_IR(WPAD_CHAN_0, &ir);

		printf("\x1b[6;0H");
		if (ir.valid) {
			printf("Crosshair: (%4d, %4d)      \n", (int)ir.x, (int)ir.y);
		} else {
			printf("Crosshair: point Wiimote at the sensor bar...   \n");
		}
		printf("Target:    (%4d, %4d)  radius %d\n", target.x, target.y, TARGET_RADIUS);
		printf("Score: %-4d   Misses: %-4d\n", score, misses);

		if (ir.valid && (pressed & WPAD_BUTTON_A)) {
			int dx = (int)ir.x - target.x;
			int dy = (int)ir.y - target.y;
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
