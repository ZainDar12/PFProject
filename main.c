#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include <time.h>

#define SCREEN_W 80
#define SCREEN_H 25

// console helpers
void gotoXY(int x, int y) {
    COORD c = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

void hideCursor() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    GetConsoleCursorInfo(h, &info);
    info.bVisible = 0;
    SetConsoleCursorInfo(h, &info);
}

void clearToTopLeft() {
    gotoXY(0, 0);
}

// sprites and visuals
const char *dino_sprite[] = {
    " (^^) ",
    "/||\\\\ ",
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
        clearToTopLeft();

        int day = (score % 2 == 0);
        drawSky(day);

        gotoXY(0, 0);
        printf("Dino Console Runner");
        gotoXY(0, 1);
        printf("Score: %d   High Score: %d", score, highScore);

        drawGround();

        // Input (non-blocking)
        if (_kbhit()) {
            int ch = _getch();
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

        Sleep(55);
    }

    return score;
}

// Display main menu and return choice: 'p' play, 'i' instructions, 'e' exit
char mainMenu() {
    while (1) {
        system("cls");
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

        int ch = _getch();
        if (ch == '1' || ch == 'p' || ch == 'P') return 'p';
        if (ch == '2' || ch == 'i' || ch == 'I') return 'i';
        if (ch == '3' || ch == 'e' || ch == 'E') return 'e';
    }
}

// Instructions screen (press any key to return)
void showInstructions() {
    system("cls");
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
    _getch();
}

// Restart menu shown after game over. Returns 'r' replay or 'e' exit to main menu
char restartMenu(int lastScore, int high) {
    while (1) {
        system("cls");
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

        int ch = _getch();
        if (ch == '1' || ch == 'r' || ch == 'R') return 'r';
        if (ch == '2' || ch == 'm' || ch == 'M') return 'm';
        if (ch == '3' || ch == 'e' || ch == 'E') return 'e';
    }
}

int main() {
    // initial setup
    hideCursor();
    srand((unsigned int)time(NULL));
    system("cls");

    while (1) {
        char choice = mainMenu();

        if (choice == 'e') {
            system("cls");
            gotoXY(SCREEN_W/2 - 6, SCREEN_H/2);
            printf("Goodbye!");
            Sleep(700);
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
                system("cls");
                gotoXY(SCREEN_W/2 - 6, SCREEN_H/2);
                printf("Goodbye!");
                Sleep(700);
                break;
            }
        }
    }

    return 0;
}
