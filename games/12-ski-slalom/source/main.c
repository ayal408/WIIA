/*
 * Ski Slalom
 * Continuously lean left/right on the Balance Board to steer between
 * incoming gate poles as they scroll toward you; leaning forward (more
 * weight on the front sensors) skis faster for a higher score, but gives
 * less time to react. Demonstrates continuous (not just threshold) use of
 * Balance Board weight for analog-style steering and speed control.
 *
 * Controls:
 *   Lean left / right on the Balance Board -> steer
 *   Lean forward on the Balance Board      -> ski faster (higher risk/reward)
 *   HOME (on a paired Wii Remote)          -> quit
 */

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

#define TRACK_WIDTH 41
#define TRACK_CENTER (TRACK_WIDTH / 2)
#define GATE_GAP 9
#define TRACK_DEPTH 12

typedef struct {
	int active;
	int gate_left;  /* left edge (in track columns) of the safe gap */
	int y;          /* row distance from player, counts down to 0 */
} Gate;

#define MAX_GATES 3

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

	srand(9001);

	float player_x = TRACK_CENTER; /* fractional position for smooth steering */
	int score = 0;
	int crashes = 0;
	int lives = 3;
	Gate gates[MAX_GATES];
	memset(gates, 0, sizeof(gates));
	int spawn_timer = 8;

	printf("\x1b[2;0H");
	printf("=== SKI SLALOM ===\n");
	printf("Lean left/right to steer through the gates. Lean forward to go faster.\n");
	printf("HOME to quit.\n\n");

	while (1) {
		WPAD_ScanPads();
		u32 pressed = WPAD_ButtonsDown(WPAD_CHAN_0);
		if (pressed & WPAD_BUTTON_HOME) break;

		struct expansion_t exp;
		WPAD_Expansion(WPAD_CHAN_0, &exp);
		int board_connected = (exp.type == WPAD_BALANCE_BOARD);

		printf("\x1b[8;0H");
		if (!board_connected) {
			printf("Waiting for a Balance Board... (sync it, then step on)      \n");
			VIDEO_WaitVSync();
			continue;
		}

		if (lives <= 0) {
			printf("CRASHED OUT - final score: %-6d                  \n", score);
			printf("Press A on a Wiimote to try again, HOME to quit.  \n");
			if (pressed & WPAD_BUTTON_A) {
				player_x = TRACK_CENTER;
				score = 0; crashes = 0; lives = 3;
				memset(gates, 0, sizeof(gates));
				spawn_timer = 8;
			}
			VIDEO_WaitVSync();
			continue;
		}

		float left = exp.wb.tl + exp.wb.bl;
		float right = exp.wb.tr + exp.wb.br;
		float front = exp.wb.tl + exp.wb.tr;
		float back = exp.wb.bl + exp.wb.br;
		float total = left + right;

		float steer = 0.0f;
		if (total > 5.0f) { /* avoid divide-by-noise when barely anyone is on the board */
			steer = (right - left) / total; /* -1..1 roughly */
		}
		float speed_lean = 0.0f;
		if (total > 5.0f) {
			speed_lean = (front - back) / total; /* >0 = leaning forward */
		}

		player_x += steer * 1.6f;
		if (player_x < 1) player_x = 1;
		if (player_x > TRACK_WIDTH - 2) player_x = TRACK_WIDTH - 2;

		int speed = 1 + (speed_lean > 0.15f ? 1 : 0); /* faster scrolling while leaning forward */

		if (--spawn_timer <= 0) {
			for (int i = 0; i < MAX_GATES; i++) {
				if (!gates[i].active) {
					gates[i].active = 1;
					gates[i].y = TRACK_DEPTH - 1;
					int max_left = TRACK_WIDTH - GATE_GAP - 1;
					gates[i].gate_left = 1 + (rand() % (max_left > 1 ? max_left : 1));
					break;
				}
			}
			spawn_timer = 6 + (rand() % 4);
		}

		int player_col = (int)(player_x + 0.5f);

		for (int i = 0; i < MAX_GATES; i++) {
			if (!gates[i].active) continue;
			gates[i].y -= speed;
			if (gates[i].y == 0) {
				int in_gap = (player_col >= gates[i].gate_left &&
				              player_col < gates[i].gate_left + GATE_GAP);
				if (in_gap) {
					score += 10;
				} else {
					crashes++;
					lives--;
				}
			}
			if (gates[i].y < 0) {
				gates[i].active = 0;
			}
		}

		/* draw the track, nearest row last */
		char rows_buf[TRACK_DEPTH][TRACK_WIDTH + 1];
		for (int r = 0; r < TRACK_DEPTH; r++) {
			memset(rows_buf[r], ' ', TRACK_WIDTH);
			rows_buf[r][TRACK_WIDTH] = '\0';
		}
		for (int i = 0; i < MAX_GATES; i++) {
			if (!gates[i].active) continue;
			int r = gates[i].y;
			if (r < 0 || r >= TRACK_DEPTH) continue;
			for (int c = 0; c < TRACK_WIDTH; c++) {
				if (c < gates[i].gate_left || c >= gates[i].gate_left + GATE_GAP) {
					rows_buf[r][c] = '|';
				}
			}
		}
		if (player_col >= 0 && player_col < TRACK_WIDTH) rows_buf[0][player_col] = 'A';

		printf("Lives: %-2d  Score: %-6d  Speed: %s          \n",
		       lives, score, (speed > 1) ? "FAST (leaning forward)" : "normal");
		for (int r = 0; r < TRACK_DEPTH; r++) {
			printf("%s\n", rows_buf[r]);
		}

		VIDEO_WaitVSync();
	}

	return 0;
}
