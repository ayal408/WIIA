/*
 * Island Adventure
 * A longer, multi-world game that combines the Wii Balance Board and the
 * Wii Remote together: lean left/right on the Balance Board to dodge lane
 * obstacles, press A on the Wiimote to jump low obstacles, and stand
 * centered on the board to open the gate to the next world. Three worlds,
 * each faster and busier than the last. Demonstrates combining
 * WPAD_Expansion() (Balance Board) and WPAD_ButtonsDown() (Wiimote) in one
 * game, plus a simple level/world state machine.
 *
 * Controls:
 *   Lean left / right on the Balance Board -> change lane
 *   Stand centered on the Balance Board     -> hold to open a world gate
 *   A (Wii Remote)                          -> jump low obstacles
 *   HOME                                    -> quit
 */

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

#define NUM_LANES 3
#define TRACK_LEN 40
#define NUM_WORLDS 3
#define OBSTACLES_PER_WORLD 12
#define GATE_HOLD_FRAMES 90 /* ~1.5s centered to open the gate */
#define LEAN_THRESHOLD 5.0f /* kg imbalance needed to register a lean */

typedef enum { OBST_LANE, OBST_JUMP } ObstacleKind;

typedef struct {
	int active;
	int lane;       /* which lane (0..NUM_LANES-1); JUMP obstacles use current lane at spawn time */
	int x;          /* distance from player, counts down to 0 */
	ObstacleKind kind;
} Obstacle;

#define MAX_OBSTACLES 5

typedef enum { PLAYING, GATE, WORLD_CLEAR, GAMEOVER, WIN } GameState;

static const char *world_name(int w) {
	switch (w) {
		case 0: return "Meadow Path";
		case 1: return "Desert Dunes";
		case 2: return "Volcano Ridge";
		default: return "?";
	}
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

	srand(555);

	int world = 0;
	int lives = 3;
	int score = 0;
	int lane = 1; /* start in the middle lane */
	int jump_frames_left = 0;
	int obstacles_cleared = 0;
	int gate_progress = 0;
	int spawn_timer = 20;
	Obstacle obstacles[MAX_OBSTACLES];
	memset(obstacles, 0, sizeof(obstacles));

	GameState state = PLAYING;
	int message_timer = 0;

	printf("\x1b[2;0H");
	printf("=== ISLAND ADVENTURE ===\n");
	printf("Balance Board: lean left/right to change lane.\n");
	printf("Wii Remote: press A to jump. Stand centered at a gate to open it.\n");
	printf("HOME to quit.\n\n");

	while (1) {
		WPAD_ScanPads();
		u32 pressed = WPAD_ButtonsDown(WPAD_CHAN_0);
		if (pressed & WPAD_BUTTON_HOME) break;

		struct expansion_t exp;
		WPAD_Expansion(WPAD_CHAN_0, &exp);
		int board_connected = (exp.type == WPAD_EXP_BALANCE_BOARD);

		int detected_lane_dir = 0; /* -1 left, 0 center, +1 right */
		if (board_connected) {
			float left = exp.wb.tl + exp.wb.bl;
			float right = exp.wb.tr + exp.wb.br;
			float bias_lr = right - left;
			if (bias_lr > LEAN_THRESHOLD) detected_lane_dir = 1;
			else if (bias_lr < -LEAN_THRESHOLD) detected_lane_dir = -1;
		}

		printf("\x1b[9;0H");
		printf("World %d/%d: %-16s   Lives: %-2d   Score: %-6d      \n",
		       world + 1, NUM_WORLDS, world_name(world), lives, score);

		if (!board_connected) {
			printf("\nWaiting for a Balance Board... (sync it, then step on)         \n");
			VIDEO_WaitVSync();
			continue;
		}

		if (state == GAMEOVER) {
			printf("\nGAME OVER - final score: %-6d\n", score);
			printf("Press A to try again, HOME to quit.               \n");
			if (pressed & WPAD_BUTTON_A) {
				world = 0; lives = 3; score = 0; lane = 1;
				jump_frames_left = 0; obstacles_cleared = 0; gate_progress = 0;
				spawn_timer = 20;
				memset(obstacles, 0, sizeof(obstacles));
				state = PLAYING;
			}
			VIDEO_WaitVSync();
			continue;
		}

		if (state == WIN) {
			printf("\nYOU FINISHED THE ADVENTURE! Final score: %-6d\n", score);
			printf("Press A to play again, HOME to quit.              \n");
			if (pressed & WPAD_BUTTON_A) {
				world = 0; lives = 3; score = 0; lane = 1;
				jump_frames_left = 0; obstacles_cleared = 0; gate_progress = 0;
				spawn_timer = 20;
				memset(obstacles, 0, sizeof(obstacles));
				state = PLAYING;
			}
			VIDEO_WaitVSync();
			continue;
		}

		if (state == WORLD_CLEAR) {
			printf("\n%s cleared! Advancing...                          \n", world_name(world));
			if (--message_timer <= 0) {
				world++;
				if (world >= NUM_WORLDS) {
					state = WIN;
				} else {
					obstacles_cleared = 0;
					gate_progress = 0;
					spawn_timer = 20;
					memset(obstacles, 0, sizeof(obstacles));
					state = PLAYING;
				}
			}
			VIDEO_WaitVSync();
			continue;
		}

		if (state == GATE) {
			printf("\nGate! Stand CENTERED on the board to open it... (%d%%)      \n",
			       gate_progress * 100 / GATE_HOLD_FRAMES);
			if (detected_lane_dir == 0) {
				gate_progress++;
				if (gate_progress >= GATE_HOLD_FRAMES) {
					state = WORLD_CLEAR;
					message_timer = 90;
				}
			} else {
				if (gate_progress > 0) gate_progress--;
			}
			VIDEO_WaitVSync();
			continue;
		}

		/* state == PLAYING */
		lane += detected_lane_dir;
		if (lane < 0) lane = 0;
		if (lane > NUM_LANES - 1) lane = NUM_LANES - 1;

		if ((pressed & WPAD_BUTTON_A) && jump_frames_left == 0) {
			jump_frames_left = 20;
		}
		int jumping = jump_frames_left > 0;
		if (jumping) jump_frames_left--;

		int base_spacing = 22 - world * 4;
		if (base_spacing < 8) base_spacing = 8;

		if (--spawn_timer <= 0) {
			for (int i = 0; i < MAX_OBSTACLES; i++) {
				if (!obstacles[i].active) {
					obstacles[i].active = 1;
					obstacles[i].x = TRACK_LEN - 1;
					obstacles[i].kind = (rand() % 2 == 0) ? OBST_LANE : OBST_JUMP;
					obstacles[i].lane = (obstacles[i].kind == OBST_LANE) ? (rand() % NUM_LANES) : lane;
					break;
				}
			}
			spawn_timer = base_spacing + (rand() % 6);
		}

		for (int i = 0; i < MAX_OBSTACLES; i++) {
			if (!obstacles[i].active) continue;
			obstacles[i].x--;

			if (obstacles[i].x == 0) {
				int hit;
				if (obstacles[i].kind == OBST_LANE) {
					hit = (obstacles[i].lane == lane);
				} else {
					hit = (obstacles[i].lane == lane) && !jumping;
				}
				if (hit) {
					lives--;
					if (lives <= 0) state = GAMEOVER;
				} else {
					score += 5;
				}
			}
			if (obstacles[i].x < 0) {
				obstacles[i].active = 0;
				obstacles_cleared++;
			}
		}

		if (state == PLAYING && obstacles_cleared >= OBSTACLES_PER_WORLD) {
			state = GATE;
			gate_progress = 0;
		}

		/* draw 3 lanes */
		char lanes_buf[NUM_LANES][TRACK_LEN + 1];
		for (int l = 0; l < NUM_LANES; l++) {
			memset(lanes_buf[l], '.', TRACK_LEN);
			lanes_buf[l][TRACK_LEN] = '\0';
		}
		for (int i = 0; i < MAX_OBSTACLES; i++) {
			if (obstacles[i].active && obstacles[i].x >= 0 && obstacles[i].x < TRACK_LEN) {
				lanes_buf[obstacles[i].lane][obstacles[i].x] = (obstacles[i].kind == OBST_LANE) ? '#' : '^';
			}
		}
		lanes_buf[lane][0] = jumping ? 'o' : 'O';

		printf("\nObstacles cleared: %2d / %-2d   (lean to dodge '#', 'A' to jump '^')\n",
		       obstacles_cleared, OBSTACLES_PER_WORLD);
		for (int l = 0; l < NUM_LANES; l++) {
			printf("%s\n", lanes_buf[l]);
		}

		VIDEO_WaitVSync();
	}

	return 0;
}
