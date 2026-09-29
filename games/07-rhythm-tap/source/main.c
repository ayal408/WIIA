/*
 * Rhythm Tap
 * A beat marker sweeps across a bar; press A exactly when it's inside the
 * hit zone to score, like a simple rhythm game. Demonstrates frame-accurate
 * timing windows driven by Wii Remote button input.
 *
 * Controls:
 *   A     -> tap on the beat
 *   HOME  -> quit
 */

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

#define BAR_WIDTH 40
#define HIT_ZONE_CENTER (BAR_WIDTH / 2)
#define HIT_ZONE_RADIUS 2

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

	int pos = 0;
	int dir = 1;
	int speed = 1; /* bar cells moved per frame group */
	int frame_div = 2; /* moves once every frame_div frames -> slower/faster */
	int frame_count = 0;

	int score = 0;
	int combo = 0;
	int best_combo = 0;
	int misses = 0;

	printf("\x1b[2;0H");
	printf("=== RHYTHM TAP ===\n");
	printf("Press A when the marker is in the [ ] zone. HOME to quit.\n\n");

	while (1) {
		WPAD_ScanPads();

		u32 pressed = WPAD_ButtonsDown(WPAD_CHAN_0);
		if (pressed & WPAD_BUTTON_HOME) break;

		if (++frame_count >= frame_div) {
			frame_count = 0;
			pos += dir * speed;
			if (pos <= 0) { pos = 0; dir = 1; }
			if (pos >= BAR_WIDTH - 1) { pos = BAR_WIDTH - 1; dir = -1; }
		}

		if (pressed & WPAD_BUTTON_A) {
			int dist = pos - HIT_ZONE_CENTER;
			if (dist < 0) dist = -dist;
			if (dist <= HIT_ZONE_RADIUS) {
				score += (HIT_ZONE_RADIUS - dist + 1) * 10;
				combo++;
				if (combo > best_combo) best_combo = combo;
				if (combo > 0 && combo % 10 == 0 && frame_div > 1) frame_div--; /* speed up over time */
			} else {
				misses++;
				combo = 0;
			}
		}

		char bar[BAR_WIDTH + 1];
		memset(bar, '-', BAR_WIDTH);
		bar[BAR_WIDTH] = '\0';
		bar[HIT_ZONE_CENTER - HIT_ZONE_RADIUS] = '[';
		bar[HIT_ZONE_CENTER + HIT_ZONE_RADIUS] = ']';
		bar[pos] = 'O';

		printf("\x1b[6;0H");
		printf("%s\n\n", bar);
		printf("Score: %-6d Combo: %-4d Best combo: %-4d Misses: %-4d\n",
		       score, combo, best_combo, misses);

		VIDEO_WaitVSync();
	}

	return 0;
}
