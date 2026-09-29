/*
 * Connect Four - Party Edition
 * Classic 2-player drop-a-disc game, dressed up in a "party game" style:
 * a title screen, a best-of-3 match with a running scoreboard, and
 * Wii Remote rumble feedback on drops and wins. Demonstrates using ANSI
 * color escape codes for a more attractive look, WPAD_Rumble() for
 * physical feedback, and a simple screen/game-state machine.
 *
 * Controls (both players share one Wii Remote, turn by turn):
 *   A            -> start / drop a disc in the selected column
 *   Left / Right -> move the column selector
 *   HOME         -> quit
 */

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

#define COLS 7
#define ROWS 6
#define EMPTY 0
#define P1 1
#define P2 2
#define ROUNDS_TO_WIN_MATCH 2 /* best of 3 */

/* ANSI colors supported by libogc's console */
#define COL_RESET  "\x1b[0m"
#define COL_RED    "\x1b[31m"
#define COL_YELLOW "\x1b[33m"
#define COL_CYAN   "\x1b[36m"
#define COL_GREEN  "\x1b[32m"
#define COL_MAGENTA "\x1b[35m"

static int board[ROWS][COLS];
static int rumble_timer = 0;

static void rumble_start(int frames) {
	WPAD_Rumble(WPAD_CHAN_0, 1);
	rumble_timer = frames;
}

static void rumble_tick(void) {
	if (rumble_timer > 0) {
		rumble_timer--;
		if (rumble_timer == 0) WPAD_Rumble(WPAD_CHAN_0, 0);
	}
}

static void reset_board(void) {
	memset(board, EMPTY, sizeof(board));
}

/* drop a piece into column c for player p; returns the row it landed on, or -1 if full */
static int drop(int c, int p) {
	for (int r = ROWS - 1; r >= 0; r--) {
		if (board[r][c] == EMPTY) {
			board[r][c] = p;
			return r;
		}
	}
	return -1;
}

static int board_full(void) {
	for (int c = 0; c < COLS; c++)
		if (board[0][c] == EMPTY) return 0;
	return 1;
}

static int check_win(int p) {
	for (int r = 0; r < ROWS; r++)
		for (int c = 0; c <= COLS - 4; c++)
			if (board[r][c] == p && board[r][c+1] == p && board[r][c+2] == p && board[r][c+3] == p)
				return 1;
	for (int c = 0; c < COLS; c++)
		for (int r = 0; r <= ROWS - 4; r++)
			if (board[r][c] == p && board[r+1][c] == p && board[r+2][c] == p && board[r+3][c] == p)
				return 1;
	for (int r = 0; r <= ROWS - 4; r++)
		for (int c = 0; c <= COLS - 4; c++)
			if (board[r][c] == p && board[r+1][c+1] == p && board[r+2][c+2] == p && board[r+3][c+3] == p)
				return 1;
	for (int r = 3; r < ROWS; r++)
		for (int c = 0; c <= COLS - 4; c++)
			if (board[r][c] == p && board[r-1][c+1] == p && board[r-2][c+2] == p && board[r-3][c+3] == p)
				return 1;
	return 0;
}

static void draw_scoreboard(int match_wins_p1, int match_wins_p2, int round_num) {
	printf(COL_CYAN "===================================" COL_RESET "\n");
	printf("  Round %d   " COL_RED "P1: %d" COL_RESET "   -   " COL_YELLOW "P2: %d" COL_RESET
	       "   (first to %d wins the match)\n",
	       round_num, match_wins_p1, match_wins_p2, ROUNDS_TO_WIN_MATCH);
	printf(COL_CYAN "===================================" COL_RESET "\n");
}

static void draw_board(int selector, int turn, int winner, int draw) {
	printf(COL_CYAN "  1   2   3   4   5   6   7 \n" COL_RESET);

	for (int c = 0; c < COLS; c++) {
		if (c == selector && !winner && !draw) {
			printf("%s " "v" " " COL_RESET, (turn == P1) ? COL_RED : COL_YELLOW);
		} else {
			printf("    ");
		}
	}
	printf("\n");

	for (int r = 0; r < ROWS; r++) {
		for (int c = 0; c < COLS; c++) {
			char piece;
			const char *color;
			if (board[r][c] == P1) { piece = 'O'; color = COL_RED; }
			else if (board[r][c] == P2) { piece = 'O'; color = COL_YELLOW; }
			else { piece = '.'; color = COL_RESET; }
			printf("%s[%c]%s ", color, piece, COL_RESET);
		}
		printf("\n");
	}
	printf("\n");

	if (winner) {
		printf("%s*** Player %d wins this round! ***%s          \n",
		       (winner == P1) ? COL_RED : COL_YELLOW, winner, COL_RESET);
		printf("Press A to continue.                              \n");
	} else if (draw) {
		printf(COL_GREEN "This round is a draw!" COL_RESET "                \n");
		printf("Press A to continue.                              \n");
	} else {
		printf("%sPlayer %d's turn%s - Left/Right to move, A to drop        \n",
		       (turn == P1) ? COL_RED : COL_YELLOW, turn, COL_RESET);
	}
}

static void draw_title_screen(int frame) {
	const char *colors[] = { COL_RED, COL_YELLOW, COL_GREEN, COL_CYAN, COL_MAGENTA };
	const char *c = colors[(frame / 10) % 5];
	printf("\x1b[2;0H");
	printf("\n");
	printf("%s   ######  ####  ##   ##  ##   ##  #####  #####  ######\n" COL_RESET, c);
	printf("%s  ##      ##  ## ###  ## ###  ## ##      ##       ##  \n" COL_RESET, c);
	printf("%s  ##      ##  ## ## # ## ## # ## ##      ##       ##  \n" COL_RESET, c);
	printf("%s  ##      ##  ## ##  ### ##  ### ##      ##       ##  \n" COL_RESET, c);
	printf("%s   ######  ####  ##   ##  ##   ##  #####  #####    ##  \n" COL_RESET, c);
	printf("\n");
	printf("                    F O U R  -  P A R T Y   E D I T I O N\n\n");
	printf("            Two players, one Wii Remote, best of 3 rounds.\n\n\n");
	if ((frame / 20) % 2 == 0) {
		printf(COL_GREEN "                         >>> Press A to start <<<" COL_RESET "        \n");
	} else {
		printf("                                                             \n");
	}
	printf("\n                              HOME to quit\n");
}

static void draw_match_winner(int winner, int frame) {
	const char *color = (winner == P1) ? COL_RED : COL_YELLOW;
	const char *confetti[] = { "* . * . * . * . * . * . * . *", ". * . * . * . * . * . * . * .", };
	printf("\x1b[2;0H");
	printf("%s%s%s\n" , COL_MAGENTA, confetti[frame % 2], COL_RESET);
	printf("\n");
	printf("%s          #     #     #     #######  #######  #     # \n" COL_RESET, color);
	printf("%s          #  #  #     #        #      #     # #     # \n" COL_RESET, color);
	printf("%s          #  #  #     #        #      #     # ####### \n" COL_RESET, color);
	printf("%s           ## ##      #        #      #     #     #   \n" COL_RESET, color);
	printf("%s            #  #      #        #      #######      #  \n" COL_RESET, color);
	printf("\n");
	printf("%s              PLAYER %d WINS THE MATCH!%s              \n", color, winner, COL_RESET);
	printf("\n%s%s%s\n", COL_MAGENTA, confetti[(frame + 1) % 2], COL_RESET);
	printf("\n                Press A for a new match, HOME to quit.\n");
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

	enum { TITLE, PLAYING, MATCH_WIN } screen = TITLE;
	int frame = 0;

	reset_board();
	int selector = COLS / 2;
	int turn = P1;
	int winner = 0;
	int draw = 0;
	int round_num = 1;
	int match_wins_p1 = 0, match_wins_p2 = 0;
	int match_winner = 0;

	while (1) {
		WPAD_ScanPads();
		u32 pressed = WPAD_ButtonsDown(WPAD_CHAN_0);
		if (pressed & WPAD_BUTTON_HOME) break;
		rumble_tick();
		frame++;

		printf("\x1b[2;0H");

		if (screen == TITLE) {
			draw_title_screen(frame);
			if (pressed & WPAD_BUTTON_A) {
				reset_board();
				selector = COLS / 2;
				turn = P1;
				winner = 0; draw = 0;
				round_num = 1;
				match_wins_p1 = 0; match_wins_p2 = 0;
				screen = PLAYING;
			}
		} else if (screen == MATCH_WIN) {
			draw_match_winner(match_winner, frame / 15);
			if (pressed & WPAD_BUTTON_A) {
				screen = TITLE;
			}
		} else { /* PLAYING */
			draw_scoreboard(match_wins_p1, match_wins_p2, round_num);

			if (winner || draw) {
				if (pressed & WPAD_BUTTON_A) {
					if (winner == P1) match_wins_p1++;
					if (winner == P2) match_wins_p2++;

					if (match_wins_p1 >= ROUNDS_TO_WIN_MATCH || match_wins_p2 >= ROUNDS_TO_WIN_MATCH) {
						match_winner = (match_wins_p1 > match_wins_p2) ? P1 : P2;
						rumble_start(60);
						screen = MATCH_WIN;
					} else {
						reset_board();
						selector = COLS / 2;
						turn = (winner == P1) ? P2 : P1; /* loser (or alternate on draw) goes first */
						winner = 0; draw = 0;
						round_num++;
					}
				}
			} else {
				if (pressed & WPAD_BUTTON_LEFT) selector = (selector - 1 + COLS) % COLS;
				if (pressed & WPAD_BUTTON_RIGHT) selector = (selector + 1) % COLS;
				if (pressed & WPAD_BUTTON_A) {
					if (drop(selector, turn) >= 0) {
						rumble_start(8);
						if (check_win(turn)) {
							winner = turn;
							rumble_start(30);
						} else if (board_full()) {
							draw = 1;
						} else {
							turn = (turn == P1) ? P2 : P1;
						}
					}
				}
			}

			draw_board(selector, turn, winner, draw);
		}

		VIDEO_WaitVSync();
	}

	WPAD_Rumble(WPAD_CHAN_0, 0);
	return 0;
}
