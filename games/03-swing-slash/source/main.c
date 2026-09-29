/*
 * Swing Slash
 * Swing the Wii Remote like a sword when an enemy appears to slash it
 * before time runs out. Demonstrates reading the Wii Remote accelerometer.
 *
 * Controls:
 *   Swing the Wii Remote hard  -> slash
 *   HOME                       -> quit
 */

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

#define SWING_THRESHOLD 1.6f   /* g-force delta considered a "swing" */
#define ENEMY_WINDOW_FRAMES 90 /* ~1.5s to react at 60fps */

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

	srand(42);

	int score = 0;
	int misses = 0;
	int enemy_active = 0;
	int enemy_timer = 0;
	int spawn_cooldown = 60;

	printf("\x1b[2;0H");
	printf("=== SWING SLASH ===\n");
	printf("An enemy will appear - swing the Wiimote hard to slash it! HOME to quit.\n\n");

	while (1) {
		WPAD_ScanPads();

		u32 pressed = WPAD_ButtonsDown(WPAD_CHAN_0);
		if (pressed & WPAD_BUTTON_HOME) break;

		WPADData *data = WPAD_Data(WPAD_CHAN_0);
		float ax = data->accel.x / 100.0f; /* rough g-normalized values */
		float ay = data->accel.y / 100.0f;
		float az = data->accel.z / 100.0f;
		float magnitude = sqrtf(ax * ax + ay * ay + az * az);
		float delta = fabsf(magnitude - 1.0f); /* 1g at rest */

		printf("\x1b[6;0H");
		printf("Accel magnitude: %.2f (swing delta %.2f)      \n", magnitude, delta);

		if (!enemy_active) {
			if (--spawn_cooldown <= 0) {
				enemy_active = 1;
				enemy_timer = ENEMY_WINDOW_FRAMES;
				spawn_cooldown = 60 + (rand() % 90);
			}
			printf("\n           ...waiting...            \n");
		} else {
			printf("\n           >>> ENEMY! SLASH NOW! <<<   (%2d)\n", enemy_timer / 60 + 1);

			if (delta > SWING_THRESHOLD) {
				score++;
				enemy_active = 0;
			} else if (--enemy_timer <= 0) {
				misses++;
				enemy_active = 0;
			}
		}

		printf("\nScore: %-4d   Missed: %-4d\n", score, misses);

		VIDEO_WaitVSync();
	}

	return 0;
}
