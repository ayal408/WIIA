/*
 * Hop & Run
 * An original side-scrolling endless runner/platformer. Your character
 * auto-runs; jump over incoming obstacles to survive as long as possible.
 * Demonstrates a simple game loop, timing, and collision detection driven
 * by Wii Remote button input - a template for building a full platformer.
 *
 * Controls:
 *   A     -> jump
 *   HOME  -> quit
 */

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

#define LANE_WIDTH 40
#define GROUND_ROW 0
#define JUMP_ROW 1
#define AIR_FRAMES 24       /* how long a jump keeps you in the air */
#define OBSTACLE_SPACING 14 /* frames between spawns, shrinks over time */

typedef struct {
	int active;
	int x; /* distance from the player, counts down to 0 */
} Obstacle;

#define MAX_OBSTACLES 4

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

	srand(7);

	Obstacle obstacles[MAX_OBSTACLES];
	memset(obstacles, 0, sizeof(obstacles));

	int jump_frames_left = 0;
	int score = 0;
	int alive = 1;
	int spawn_timer = OBSTACLE_SPACING;
	int speed_level = 0;

	printf("\x1b[2;0H");
	printf("=== HOP & RUN ===\n");
	printf("Press A to jump over obstacles. HOME to quit.\n\n");

	while (1) {
		WPAD_ScanPads();

		u32 pressed = WPAD_ButtonsDown(WPAD_CHAN_0);
		if (pressed & WPAD_BUTTON_HOME) break;

		if (!alive) {
			printf("\x1b[10;0H");
			printf("GAME OVER - final score: %d\n", score);
			printf("Press A to play again, HOME to quit.\n");
			if (pressed & WPAD_BUTTON_A) {
				memset(obstacles, 0, sizeof(obstacles));
				jump_frames_left = 0;
				score = 0;
				alive = 1;
				spawn_timer = OBSTACLE_SPACING;
				speed_level = 0;
			}
			VIDEO_WaitVSync();
			continue;
		}

		if ((pressed & WPAD_BUTTON_A) && jump_frames_left == 0) {
			jump_frames_left = AIR_FRAMES;
		}
		int player_row = (jump_frames_left > 0) ? JUMP_ROW : GROUND_ROW;
		if (jump_frames_left > 0) jump_frames_left--;

		/* spawn obstacles */
		if (--spawn_timer <= 0) {
			for (int i = 0; i < MAX_OBSTACLES; i++) {
				if (!obstacles[i].active) {
					obstacles[i].active = 1;
					obstacles[i].x = LANE_WIDTH - 1;
					break;
				}
			}
			int min_spacing = OBSTACLE_SPACING - speed_level;
			if (min_spacing < 6) min_spacing = 6;
			spawn_timer = min_spacing + (rand() % 8);
		}

		/* advance obstacles, check collision */
		for (int i = 0; i < MAX_OBSTACLES; i++) {
			if (!obstacles[i].active) continue;
			obstacles[i].x--;
			if (obstacles[i].x == 0 && player_row == GROUND_ROW) {
				alive = 0;
			}
			if (obstacles[i].x < 0) {
				obstacles[i].active = 0;
				score++;
				if (score % 5 == 0) speed_level++;
			}
		}

		/* draw */
		char air_line[LANE_WIDTH + 1];
		char ground_line[LANE_WIDTH + 1];
		memset(air_line, ' ', LANE_WIDTH);
		memset(ground_line, ' ', LANE_WIDTH);
		air_line[LANE_WIDTH] = '\0';
		ground_line[LANE_WIDTH] = '\0';

		for (int i = 0; i < MAX_OBSTACLES; i++) {
			if (obstacles[i].active && obstacles[i].x >= 0 && obstacles[i].x < LANE_WIDTH) {
				ground_line[obstacles[i].x] = '^';
			}
		}
		if (player_row == JUMP_ROW) {
			air_line[0] = 'O';
		} else {
			ground_line[0] = 'O';
		}

		printf("\x1b[6;0H");
		printf("%s\n", air_line);
		printf("%s\n", ground_line);
		printf("________________________________________\n");
		printf("Score: %-5d  Speed level: %-3d          \n", score, speed_level);

		VIDEO_WaitVSync();
	}

	return 0;
}
