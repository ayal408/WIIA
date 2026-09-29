/*
 * Balance Quest
 * Shift your weight on the Wii Balance Board to match the requested
 * direction (front / back / left / right / center) before the timer runs out.
 * Demonstrates reading the Balance Board expansion via WPAD_Expansion().
 *
 * Controls:
 *   Stand on the Balance Board and shift your weight
 *   HOME on a paired Wii Remote -> quit
 */

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

typedef enum { DIR_CENTER, DIR_FRONT, DIR_BACK, DIR_LEFT, DIR_RIGHT, DIR_COUNT } Direction;

static const char *dir_name(Direction d) {
	switch (d) {
		case DIR_CENTER: return "CENTER (even weight)";
		case DIR_FRONT:  return "LEAN FORWARD";
		case DIR_BACK:   return "LEAN BACK";
		case DIR_LEFT:   return "LEAN LEFT";
		case DIR_RIGHT:  return "LEAN RIGHT";
		default:         return "?";
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

	srand(1337);

	int score = 0;
	int round_frames = 0;
	const int frames_per_round = 180; /* ~3 seconds at 60fps */
	Direction target = (Direction)(rand() % (DIR_COUNT - 1)) + DIR_FRONT; /* skip CENTER on first round for clarity */

	printf("\x1b[2;0H");
	printf("=== BALANCE QUEST ===\n");
	printf("Step onto the Balance Board (sync it like a Wii Remote).\n");
	printf("Match the requested direction before time runs out. HOME on a Wiimote to quit.\n\n");

	while (1) {
		WPAD_ScanPads();

		u32 pressed = WPAD_ButtonsDown(WPAD_CHAN_0);
		if (pressed & WPAD_BUTTON_HOME) break;

		struct expansion_t exp;
		WPAD_Expansion(WPAD_CHAN_0, &exp);

		printf("\x1b[7;0H");

		if (exp.type == WPAD_EXP_BALANCE_BOARD) {
			float tl = exp.wb.tl;
			float tr = exp.wb.tr;
			float bl = exp.wb.bl;
			float br = exp.wb.br;
			float total = tl + tr + bl + br;

			float front = tl + tr;
			float back  = bl + br;
			float left  = tl + bl;
			float right = tr + br;

			printf("Weight  TL:%5.1fkg TR:%5.1fkg\n", tl, tr);
			printf("        BL:%5.1fkg BR:%5.1fkg\n", bl, br);
			printf("Total: %6.1f kg                 \n", total);

			Direction detected = DIR_CENTER;
			float bias_fb = front - back;
			float bias_lr = right - left;
			const float threshold = 5.0f; /* kg imbalance needed to register a lean */

			if (fabsf(bias_fb) > fabsf(bias_lr) && fabsf(bias_fb) > threshold) {
				detected = (bias_fb > 0) ? DIR_FRONT : DIR_BACK;
			} else if (fabsf(bias_lr) > threshold) {
				detected = (bias_lr > 0) ? DIR_RIGHT : DIR_LEFT;
			}

			printf("\nGoal:     %-24s\n", dir_name(target));
			printf("Your lean: %-24s\n", dir_name(detected));
			printf("Score: %-4d   Time left: %2d\n",
			       score, (frames_per_round - round_frames) / 60);

			if (detected == target) {
				score++;
				round_frames = 0;
				target = (Direction)(rand() % DIR_COUNT);
			}
		} else {
			printf("Waiting for a Balance Board... (sync it, then step on)\n");
		}

		round_frames++;
		if (round_frames >= frames_per_round) {
			round_frames = 0;
			target = (Direction)(rand() % DIR_COUNT);
		}

		VIDEO_WaitVSync();
	}

	return 0;
}
