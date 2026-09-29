/*
 * Simon Says
 * The game plays back a growing sequence of the four "color" buttons
 * (1, 2, A, B). Repeat it correctly to advance a round. Demonstrates
 * reading several distinct buttons and building simple game-state machines
 * (show sequence -> wait for input -> check -> grow sequence).
 *
 * Controls:
 *   1 / 2 / A / B -> the four "colors"
 *   HOME          -> quit
 */

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <stdlib.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

#define MAX_SEQUENCE 64

static const char *color_name(int c) {
	switch (c) {
		case 0: return "1 (Red)";
		case 1: return "2 (Blue)";
		case 2: return "A (Green)";
		case 3: return "B (Yellow)";
		default: return "?";
	}
}

static u32 color_button(int c) {
	switch (c) {
		case 0: return WPAD_BUTTON_1;
		case 1: return WPAD_BUTTON_2;
		case 2: return WPAD_BUTTON_A;
		case 3: return WPAD_BUTTON_B;
		default: return 0;
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

	srand(2024);

	int sequence[MAX_SEQUENCE];
	int length = 1;
	sequence[0] = rand() % 4;

	enum { SHOWING, WAITING, GAMEOVER } state = SHOWING;
	int show_index = 0;
	int show_timer = 0;
	int input_index = 0;
	int round = 1;

	printf("\x1b[2;0H");
	printf("=== SIMON SAYS ===\n");
	printf("Watch the sequence, then repeat it with 1 / 2 / A / B. HOME to quit.\n\n");

	while (1) {
		WPAD_ScanPads();
		u32 pressed = WPAD_ButtonsDown(WPAD_CHAN_0);
		if (pressed & WPAD_BUTTON_HOME) break;

		printf("\x1b[6;0H");
		printf("Round: %-4d                                   \n", round);

		if (state == SHOWING) {
			printf("Watch:  %-16s                     \n", color_name(sequence[show_index]));
			printf("                                               \n");
			if (++show_timer >= 40) {
				show_timer = 0;
				show_index++;
				if (show_index >= length) {
					state = WAITING;
					input_index = 0;
				}
			}
		} else if (state == WAITING) {
			printf("Your turn! Repeat the sequence (%d/%d)          \n", input_index, length);
			printf("                                               \n");

			for (int c = 0; c < 4; c++) {
				if (pressed & color_button(c)) {
					if (c == sequence[input_index]) {
						input_index++;
						if (input_index >= length) {
							/* round complete */
							round++;
							if (length < MAX_SEQUENCE) {
								sequence[length] = rand() % 4;
								length++;
							}
							state = SHOWING;
							show_index = 0;
							show_timer = 0;
						}
					} else {
						state = GAMEOVER;
					}
					break;
				}
			}
		} else { /* GAMEOVER */
			printf("GAME OVER - you reached round %-4d              \n", round);
			printf("Press A to try again, HOME to quit.             \n");
			if (pressed & WPAD_BUTTON_A) {
				length = 1;
				sequence[0] = rand() % 4;
				round = 1;
				state = SHOWING;
				show_index = 0;
				show_timer = 0;
			}
		}

		VIDEO_WaitVSync();
	}

	return 0;
}
