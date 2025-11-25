#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#define SCREEN_W 80
#define SCREEN_H 25

// Terminal helpers (ANSI escape sequences)
static struct termios orig_termios;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    // show cursor on exit
    printf("\033[?25h");
    fflush(stdout);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON); // no echo, non-canonical
    raw.c_cc[VMIN] = 0; // non-blocking read
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    // hide cursor
    printf("\033[?25l");
    fflush(stdout);
}

int kbhit() {
    struct timeval tv = {0L, 0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO+1, &fds, NULL, NULL, &tv) > 0;
}

int getch_nonblock() {
    unsigned char c = 0;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n <= 0) return -1;
    return c;
}

void gotoXY(int x, int y) {
    // ANSI: row (y) and column (x) are 1-based
    printf("\033[%d;%dH", y + 1, x + 1);
}

void clearScreen() {
    // clear screen and move cursor to top-left
    printf("\033[2J\033[H");
}

void hideCursor() {
    printf("\033[?25l");
}

void showCursor() {
    printf("\033[?25h");
}

// sprites and visuals
const char *dino_sprite[] = {
    " (^^) ",
    "/||\\ ",
    " /\\   "
};

const char *obst_sprite[] = {
    " | ",
    " | ",
    "/|\\"
};

const char *sky_sun[] = {
    " \\ | / ",
    " --O-- ",
    " / | \\ "
};

const char *sky_moon[] = {
    "  __  ",
    " (  ) ",
    "  ||  "
};

// game state (will be re-used between plays)
int highScore = 0;

// Draw helpers
void drawGround() {
    gotoXY(0, 22);
    for (int i = 0; i < SCREEN_W; i++) putchar('=');
}

void drawSky(int day) {
    int cx = SCREEN_W / 2 - 4;
    const char **art = day ? sky_sun : sky_moon;
    int lines = 3;
    for (int i = 0; i < lines; i++) {
        gotoXY(cx, 2 + i);
        printf("%s", art[i]);
    }
}

// Game play function: returns final score
int playSingleRun() {
    // local game variables
    int dinoBaseY = 19;
    int jump = 0, rising = 1, jumpPos = 0;
    int obstacleX = SCREEN_W - 7;
    int score = 0;
    int gameOver = 0;

    // frame loop
    while (!gameOver) {
        clearScreen();

        int day = (score % 2 == 0);
        drawSky(day);

        gotoXY(0, 0);
        printf("Dino Console Runner");
        gotoXY(0, 1);
        printf("Score: %d   High Score: %d", score, highScore);

        drawGround();

        // Input (non-blocking)
        if (kbhit()) {
            int ch = getch_nonblock();
            if (ch == ' ' && !jump) {
                jump = 1;
                rising = 1;
            } else if (ch == 'x' || ch == 'X') {
                // immediate quit out of the play loop - return partial score
                return score;
            }
        }

        // Move obstacle
        obstacleX--;
        if (obstacleX < -4) { // off-screen
            obstacleX = SCREEN_W - 7;
            score++;
        }

        // Update jump
        if (jump) {
            if (rising) {
                jumpPos++;
                if (jumpPos >= 6) rising = 0;
            } else {
                jumpPos--;
                if (jumpPos <= 0) {
                    jumpPos = 0;
                    jump = 0;
                    rising = 1;
                }
            }
        }

        // Draw dino (3 lines)
        for (int i = 0; i < 3; i++) {
            gotoXY(5, dinoBaseY - i - jumpPos);
            printf("%s", dino_sprite[i]);
        }

        // Draw obstacle (3 lines)
        for (int i = 0; i < 3; i++) {
            int ox = obstacleX;
            // Make sure we don't draw outside left bound
            if (ox >= 0 && ox < SCREEN_W - 1) {
                gotoXY(ox, 19 - i);
                printf("%s", obst_sprite[i]);
            }
        }

        // Simple collision detection: if obstacle is near dino x-range and dino is on ground
        if (obstacleX >= 4 && obstacleX <= 10 && jumpPos == 0) {
            gameOver = 1;
        }

        // frame delay ~55ms
        usleep(55000);
    }

    return score;
}

// Display main menu and return choice: 'p' play, 'i' instructions, 'e' exit
char mainMenu() {
    while (1) {
        clearScreen();
        hideCursor();
        gotoXY(SCREEN_W/2 - 10, 4);
        printf("=== DINO CONSOLE RUNNER ===");
        gotoXY(SCREEN_W/2 - 8, 7);
        printf("1. Play");
        gotoXY(SCREEN_W/2 - 8, 9);
        printf("2. Instructions");
        gotoXY(SCREEN_W/2 - 8, 11);
        printf("3. Exit");
        gotoXY(SCREEN_W/2 - 18, 14);
        printf("Press 1/2/3 to choose (or P/I/E).");

        // blocking wait for a key
        int ch = -1;
        while (ch == -1) ch = getch_nonblock();

        if (ch == '1' || ch == 'p' || ch == 'P') return 'p';
        if (ch == '2' || ch == 'i' || ch == 'I') return 'i';
        if (ch == '3' || ch == 'e' || ch == 'E') return 'e';
    }
}

// Instructions screen (press any key to return)
void showInstructions() {
    clearScreen();
    hideCursor();
    gotoXY(8, 4);
    printf("INSTRUCTIONS:");
    gotoXY(8, 6);
    printf("- Press SPACE to jump.");
    gotoXY(8, 7);
    printf("- Avoid the cactus obstacles.");
    gotoXY(8, 8);
    printf("- Press X during play to quit early.");
    gotoXY(8, 10);
    printf("- Score increases when you pass an obstacle.");
    gotoXY(8, 12);
    printf("High score is kept while program runs (no file).");
    gotoXY(8, 14);
    printf("Press any key to return to main menu...");

    // wait for a keypress
    while (!kbhit()) usleep(10000);
    // consume key
    getch_nonblock();
}

// Restart menu shown after game over. Returns 'r' replay or 'e' exit to main menu
char restartMenu(int lastScore, int high) {
    while (1) {
        clearScreen();
        hideCursor();
        gotoXY(SCREEN_W/2 - 10, 6);
        printf("=== GAME OVER ===");
        gotoXY(SCREEN_W/2 - 12, 8);
        printf("Your Score : %d", lastScore);
        gotoXY(SCREEN_W/2 - 12, 9);
        printf("High Score : %d", high);
        gotoXY(SCREEN_W/2 - 12, 12);
        printf("1. Play Again");
        gotoXY(SCREEN_W/2 - 12, 14);
        printf("2. Main Menu");
        gotoXY(SCREEN_W/2 - 12, 16);
        printf("3. Exit");
        gotoXY(SCREEN_W/2 - 28, 18);
        printf("Press 1/2/3 (or R/M/E).");

        int ch = -1;
        while (ch == -1) ch = getch_nonblock();

        if (ch == '1' || ch == 'r' || ch == 'R') return 'r';
        if (ch == '2' || ch == 'm' || ch == 'M') return 'm';
        if (ch == '3' || ch == 'e' || ch == 'E') return 'e';
    }
}

int main() {
    // initial setup
    enableRawMode();
    srand((unsigned int)time(NULL));
    clearScreen();

    while (1) {
        char choice = mainMenu();

        if (choice == 'e') {
            clearScreen();
            gotoXY(SCREEN_W/2 - 6, SCREEN_H/2);
            printf("Goodbye!");
            fflush(stdout);
            usleep(700000);
            break;
        } else if (choice == 'i') {
            showInstructions();
            continue;
        } else if (choice == 'p') {
            // Play loop: allow replaying from restart menu without returning to main menu unless chosen
            int finishedScore = playSingleRun();

            // update in-memory high score (no file)
            if (finishedScore > highScore) highScore = finishedScore;

            // show restart menu
            char post = restartMenu(finishedScore, highScore);
            if (post == 'r') {
                // loop will just play again
                continue;
            } else if (post == 'm') {
                // return to main menu (outer loop)
                continue;
            } else if (post == 'e') {
                clearScreen();
                gotoXY(SCREEN_W/2 - 6, SCREEN_H/2);
                printf("Goodbye!");
                fflush(stdout);
                usleep(700000);
                break;
            }
        }
    }

    return 0;
}