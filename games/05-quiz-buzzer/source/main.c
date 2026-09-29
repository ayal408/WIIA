/*
 * Quiz Buzzer
 * Up to 4 players, each with their own Wii Remote, race to press A first
 * once the host "opens" the round. Demonstrates reading multiple Wiimotes
 * at once (WPAD_CHAN_0..WPAD_CHAN_3) - the basis for any party/multiplayer game.
 *
 * Controls:
 *   A on any connected Wiimote -> buzz in
 *   1  (on Wiimote in channel 0) -> host opens a new round
 *   HOME -> quit
 */

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <stdlib.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

#define MAX_PLAYERS 4

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

	int score[MAX_PLAYERS] = {0, 0, 0, 0};
	int round_open = 0;
	int winner = -1;

	printf("\x1b[2;0H");
	printf("=== QUIZ BUZZER (up to 4 players) ===\n");
	printf("Sync a Wii Remote per player. Player 1 presses 1 to open a round.\n");
	printf("Everyone presses A to buzz in - fastest wins the round. HOME to quit.\n\n");

	while (1) {
		WPAD_ScanPads();

		u32 pressed0 = WPAD_ButtonsDown(WPAD_CHAN_0);
		if (pressed0 & WPAD_BUTTON_HOME) break;

		if (!round_open && (pressed0 & WPAD_BUTTON_1)) {
			round_open = 1;
			winner = -1;
		}

		if (round_open) {
			for (int chan = 0; chan < MAX_PLAYERS; chan++) {
				u32 pressed = WPAD_ButtonsDown(chan);
				if ((pressed & WPAD_BUTTON_A) && winner == -1) {
					winner = chan;
					score[chan]++;
					round_open = 0;
				}
			}
		}

		printf("\x1b[6;0H");
		printf("Round: %s              \n", round_open ? "OPEN - buzz in!" : "closed (press 1 on P1 to open)");
		if (winner >= 0) {
			printf("Last winner: Player %d          \n", winner + 1);
		} else {
			printf("Last winner: -                  \n");
		}
		printf("\nScores:\n");
		for (int i = 0; i < MAX_PLAYERS; i++) {
			printf("  Player %d: %-4d\n", i + 1, score[i]);
		}

		VIDEO_WaitVSync();
	}

	return 0;
}
