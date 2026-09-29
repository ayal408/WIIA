/*
 * Connect Four
 * Classic 2-player drop-a-disc game, in color. Demonstrates using ANSI
 * color escape codes on the Wii's console for a more attractive look, plus
 * simple grid-based win detection (horizontal / vertical / diagonal).
 *
 * Controls (both players share one Wii Remote, turn by turn):
 *   Left / Right -> move the column selector
 *   A            -> drop a disc in the selected column
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

/* ANSI colors supported by libogc's console */
#define COL_RESET  "\x1b[0m"
#define COL_RED    "\x1b[31m"
#define COL_YELLOW "\x1b[33m"
#define COL_CYAN   "\x1b[36m"
#define COL_GREEN  "\x1b[32m"

static int board[ROWS][COLS];

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
	/* horizontal */
	for (int r = 0; r < ROWS; r++)
		for (int c = 0; c <= COLS - 4; c++)
			if (board[r][c] == p && board[r][c+1] == p && board[r][c+2] == p && board[r][c+3] == p)
				return 1;
	/* vertical */
	for (int c = 0; c < COLS; c++)
		for (int r = 0; r <= ROWS - 4; r++)
			if (board[r][c] == p && board[r+1][c] == p && board[r+2][c] == p && board[r+3][c] == p)
				return 1;
	/* diagonal down-right */
	for (int r = 0; r <= ROWS - 4; r++)
		for (int c = 0; c <= COLS - 4; c++)
			if (board[r][c] == p && board[r+1][c+1] == p && board[r+2][c+2] == p && board[r+3][c+3] == p)
				return 1;
	/* diagonal up-right */
	for (int r = 3; r < ROWS; r++)
		for (int c = 0; c <= COLS - 4; c++)
			if (board[r][c] == p && board[r-1][c+1] == p && board[r-2][c+2] == p && board[r-3][c+3] == p)
				return 1;
	return 0;
}

static void draw_board(int selector, int turn, int winner, int draw) {
	printf("\x1b[6;0H");
	printf(COL_CYAN "  1   2   3   4   5   6   7 \n" COL_RESET);

	/* column selector arrow */
	for (int c = 0; c < COLS; c++) {
		if (c == selector) {
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
		printf("%s*** Player %d WINS! ***%s              \n",
		       (winner == P1) ? COL_RED : COL_YELLOW, winner, COL_RESET);
		printf("Press A to play again, HOME to quit.       \n");
	} else if (draw) {
		printf(COL_GREEN "It's a draw!" COL_RESET "                    \n");
		printf("Press A to play again, HOME to quit.       \n");
	} else {
		printf("%sPlayer %d's turn%s - Left/Right to move, A to drop   \n",
		       (turn == P1) ? COL_RED : COL_YELLOW, turn, COL_RESET);
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

	reset_board();
	int selector = COLS / 2;
	int turn = P1;
	int winner = 0;
	int draw = 0;

	printf("\x1b[2;0H");
	printf(COL_CYAN "=== CONNECT FOUR ===" COL_RESET "\n");
	printf("Two players share one Wii Remote. HOME to quit.\n");

	while (1) {
		WPAD_ScanPads();
		u32 pressed = WPAD_ButtonsDown(WPAD_CHAN_0);
		if (pressed & WPAD_BUTTON_HOME) break;

		if (winner || draw) {
			if (pressed & WPAD_BUTTON_A) {
				reset_board();
				selector = COLS / 2;
				turn = P1;
				winner = 0;
				draw = 0;
			}
		} else {
			if (pressed & WPAD_BUTTON_LEFT) {
				selector = (selector - 1 + COLS) % COLS;
			}
			if (pressed & WPAD_BUTTON_RIGHT) {
				selector = (selector + 1) % COLS;
			}
			if (pressed & WPAD_BUTTON_A) {
				if (drop(selector, turn) >= 0) {
					if (check_win(turn)) {
						winner = turn;
					} else if (board_full()) {
						draw = 1;
					} else {
						turn = (turn == P1) ? P2 : P1;
					}
				}
			}
		}

		draw_board(selector, turn, winner, draw);

		VIDEO_WaitVSync();
	}

	return 0;
}
