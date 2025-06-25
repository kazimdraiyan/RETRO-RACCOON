#include "iGraphics.h"

// TODO: Try to use object oriented programming.
// TODO: Try to add Dvorak keyboard support.

#define WIDTH 1280
#define HEIGHT 720
#define COLUMNS 64
#define ROWS 36
#define TITLE "Platformer Game"

// TODO: Check out how these constants work and be optimized.
#define LEFT 0x0001
#define RIGHT 0x0002
#define UP 0x0003
#define DOWN 0x0004

#define MENU_PAGE 0x0000
#define GAME_PAGE 0x0001
#define LEVELS_PAGE 0x0002
#define HIGH_SCORES_PAGE 0x0003
#define OPTIONS_PAGE 0x0004

int currentPage = MENU_PAGE;
int currentLevel = 0;

void drawMenuPage();
void drawLevelsPage();
void drawCoinCount();

Image background_image;
Image tile;

int hoverRectangleYs[5] = {HEIGHT / 2 + 40};
int hoverRectangleY = -1;
int hoverRectangleHeight = 50;

bool firstDraw = true;
bool blockedSides[4] = {false}; // Left, Right, Up, Down
double t = 0;
int collectedCoins = 0;

// Player represented as an array: {x, y, width, height}
double player[4] = {WIDTH / 9.0, 150, 30, 30};

char tiles[ROWS][COLUMNS + 1] = {};

void loadLevel(int level)
{
    // Loading levels
    FILE *file_pointer;
    char filename[20];
    sprintf(filename, "levels/level%d.txt", level);
    file_pointer = fopen(filename, "r");
    char discard[2];
    if (file_pointer != NULL)
    {
        for (int row = 1; row <= ROWS; row++)
        {
            fgets(tiles[row - 1], COLUMNS + 1, file_pointer); // +1 for null terminator.
            fgets(discard, 1 + 1, file_pointer);              // Read and discard the newline character. +1 for null terminator.
        }

        for (int i = 0; i < ROWS; i++)
        {
            printf("%d: %s\n", i, tiles[i]);
        }
    }
    else
    {
        printf("Failed to load level.");
    }
    fclose(file_pointer);
}

bool doesTouchLeftBoundary(double x)
{
    return x <= 0;
}

bool doesTouchRightBoundary(double x, double width)
{
    return x + width >= WIDTH;
}

bool doesTouchUpBoundary(double y, double height)
{
    return y + height >= HEIGHT;
}

bool doesTouchDownBoundary(double y)
{
    return y <= 0;
}

void touchingBoundaries()
{
    if (doesTouchLeftBoundary(player[0]))
        blockedSides[LEFT - 1] = true;
    if (doesTouchRightBoundary(player[0], player[2]))
        blockedSides[RIGHT - 1] = true;
    if (doesTouchUpBoundary(player[1], player[3]))
        blockedSides[UP - 1] = true;
    if (doesTouchDownBoundary(player[1]))
    {
        blockedSides[DOWN - 1] = true;
        t = 0;
    }
}

// TODO: Fix collisions at the front of a platform.
bool collidesWithTile(double x, double y, double width, double height)
{
    int tileRow = (ROWS - 1) - (int)(y / (HEIGHT / ROWS));
    int tileCol = (int)(x / (WIDTH / COLUMNS));

    if (tileRow < 0 || tileRow >= ROWS || tileCol < 0 || tileCol >= COLUMNS)
        return false; // Out of bounds

    return tiles[tileRow][tileCol] == '#';
}

void updateBlockedSides()
{
    for (int i = 0; i < 4; i++)
    {
        blockedSides[i] = false; // Reset blocked sides
    }
    touchingBoundaries();
}

bool isBlocked(int side)
{
    return blockedSides[side - 1];
}

void goUp(int amount)
{
    if (!isBlocked(UP))
    {
        if (doesTouchUpBoundary(player[1] + amount, player[3]))
        {
            player[1] = HEIGHT - player[3]; // Prevent going out of bounds
        }
        else
            player[1] += amount;
        t = 0;
    }
}

void goDown(int amount)
{
    if (!isBlocked(DOWN))
    {
        if (doesTouchDownBoundary(player[1] - amount))
        {
            player[1] = 0; // Prevent going out of bounds
        }
        else
            player[1] -= amount;
    }
}

void iDraw()
{
    if (firstDraw)
    {
        // glutFullScreen();
        iLoadImage(&background_image, "assets/images/menu_page_background.png");
        iResizeImage(&background_image, WIDTH, HEIGHT);

        iLoadImage(&tile, "assets/images/tile.png");
        iResizeImage(&tile, WIDTH / COLUMNS, HEIGHT / ROWS);

        currentLevel = 1;
        loadLevel(currentLevel);

        firstDraw = false;
    }

    iClear();

    // Draw background
    iSetColor(255, 255, 255);
    iShowLoadedImage(0, 0, &background_image);

    // Draw tiles
    for (int row = 0; row < ROWS; row++)
    {
        for (int col = 0; col < COLUMNS; col++)
        {
            if (tiles[row][col] == '#')
            {
                iShowLoadedImage(col * (WIDTH / COLUMNS), (ROWS - row - 1) * (HEIGHT / ROWS), &tile);
            }
            else if (tiles[row][col] == 'O')
            {
                // Coin
                iSetColor(255, 215, 0); // Gold color
                iFilledCircle(col * (WIDTH / COLUMNS) + (WIDTH / COLUMNS) / 2, (ROWS - row - 1) * (HEIGHT / ROWS) + (HEIGHT / ROWS) / 2, 10, 100);
            }
        }
    }

    // Player
    iSetColor(40, 75, 30);
    iFilledRectangle(player[0], player[1], player[2], player[3]);

    updateBlockedSides();

    // Collect coins
    for (int row = 0; row < ROWS; row++)  
    {
        for (int col = 0; col < COLUMNS; col++)
        {
            if (tiles[row][col] == 'O')
            {
                double coinX = col * (WIDTH / COLUMNS) + (WIDTH / COLUMNS) / 2;
                double coinY = (ROWS - row - 1) * (HEIGHT / ROWS) + (HEIGHT / ROWS) / 2;
                if (player[0] < coinX + 10 && player[0] + player[2] > coinX - 10 &&
                    player[1] < coinY + 10 && player[1] + player[3] > coinY - 10)
                {
                    collectedCoins++;
                    tiles[row][col] = ' ';
                }
            }
        }
    }

    drawCoinCount();

    // Apply gravity effect
    t++;
    double gravityDeltaY = 0.03 * (t * 2 - 1); // TODO: Store the 0.03 magic number to somewhere.
    if (collidesWithTile(player[0], player[1] - gravityDeltaY, player[2], player[3]))
    {
        // TODO: Go down until the player is on the tile.
        t = 0;
        gravityDeltaY = 0;
    }
    else
    {
        goDown(gravityDeltaY);
    }

    if (currentPage == MENU_PAGE)
    {
        drawMenuPage();
    }
    else if (currentPage == GAME_PAGE)
    {
    }
    else if (currentPage == LEVELS_PAGE)
    {
        drawLevelsPage();
    }
    else if (currentPage == HIGH_SCORES_PAGE)
    {
    }
    else if (currentPage == OPTIONS_PAGE)
    {
    }
}

void iKeyboard(unsigned char key)
{
    updateBlockedSides();
    switch (key)
    {
    // TODO: Instead of moving a certain amount, implement a intial velocity mechanism as well as friction.
    // TODO: Try to handle both left and right movement by a single function.
    case 'w':
        // TODO: Animate going up.
        goUp(100);
        break;
    case 'a':
        if (!isBlocked(LEFT))
        {
            if (doesTouchLeftBoundary(player[0] - 10))
            {
                player[0] = 0; // Prevent going out of bounds
            }
            else
                player[0] -= 25;
        }
        break;
    case 'd':
        if (!isBlocked(RIGHT))
        {
            if (doesTouchRightBoundary(player[0] + 10, player[2]))
            {
                player[0] = WIDTH - player[2]; // Prevent going out of bounds
            }
            else
                player[0] += 25;
        }
        break;
    case 27: // Escape key
        currentPage = MENU_PAGE;
        break;
    default:
        break;
    }
}

void drawCoinCount()
{
    iSetColor(0, 0, 0);
    char coinText[50];
    sprintf(coinText, "Coins: %d", collectedCoins);
    iTextAdvanced(WIDTH - 200, HEIGHT - 50, coinText, 0.3, 2.5, GLUT_STROKE_ROMAN);
}

void drawMenuPage()
{
    iClear();
    iSetColor(255, 255, 255);

    iShowLoadedImage(0, 0, &background_image);

    iSetColor(0, 0, 0);
    iTextAdvanced(WIDTH / 2 - 250, HEIGHT / 2 + 200, "Platformer Game", 0.5, 5.0, GLUT_STROKE_ROMAN);

    iTextAdvanced(WIDTH / 2 - 100, HEIGHT / 2 + 50, "Resume", 0.3, 2.5, GLUT_STROKE_ROMAN);
    iTextAdvanced(WIDTH / 2 - 100, HEIGHT / 2 + 0, "Levels", 0.3, 2.5, GLUT_STROKE_ROMAN);
    iTextAdvanced(WIDTH / 2 - 100, HEIGHT / 2 - 50, "High Scores", 0.3, 2.5, GLUT_STROKE_ROMAN);
    iTextAdvanced(WIDTH / 2 - 100, HEIGHT / 2 - 100, "Options", 0.3, 2.5, GLUT_STROKE_ROMAN);
    iTextAdvanced(WIDTH / 2 - 100, HEIGHT / 2 - 150, "Exit", 0.3, 2.5, GLUT_STROKE_ROMAN);

    // Draw hover effect rectangles
    if (hoverRectangleY != -1)
    {
        iSetTransparentColor(100, 100, 100, 0.25); // TODO: Change the hover color, the current one looks bad.
        iFilledRectangle(0, hoverRectangleY, WIDTH, hoverRectangleHeight);
    }
}

void drawLevelsPage()
{
    iClear();
    iSetColor(255, 255, 255);

    iShowLoadedImage(0, 0, &background_image);

    iSetColor(0, 0, 0);
    iTextAdvanced(WIDTH / 2 - 100, HEIGHT / 2 + 200, "Levels", 0.5, 5.0, GLUT_STROKE_ROMAN);
    iTextAdvanced(WIDTH / 2 - 100, HEIGHT / 2 + 50, "Level 1", 0.3, 2.5, GLUT_STROKE_ROMAN);
    iTextAdvanced(WIDTH / 2 - 100, HEIGHT / 2 + 0, "Level 2", 0.3, 2.5, GLUT_STROKE_ROMAN);
    iTextAdvanced(WIDTH / 2 - 100, HEIGHT / 2 - 50, "Level 3", 0.3, 2.5, GLUT_STROKE_ROMAN);
    iTextAdvanced(WIDTH / 2 - 100, HEIGHT / 2 - 100, "Level 4", 0.3, 2.5, GLUT_STROKE_ROMAN);
    iTextAdvanced(WIDTH / 2 - 100, HEIGHT / 2 - 150, "Level 5", 0.3, 2.5, GLUT_STROKE_ROMAN);

    // Draw hover effect rectangles
    if (hoverRectangleY != -1)
    {
        iSetTransparentColor(100, 100, 100, 0.25); // TODO: Store the colors somewhere instead of hardcoding them.
        iFilledRectangle(0, hoverRectangleY, WIDTH, hoverRectangleHeight);
    }
}

// GLUT_KEY_F1, GLUT_KEY_F2, GLUT_KEY_F3, GLUT_KEY_F4, GLUT_KEY_F5, GLUT_KEY_F6, GLUT_KEY_F7, GLUT_KEY_F8, GLUT_KEY_F9, GLUT_KEY_F10, GLUT_KEY_F11, GLUT_KEY_F12, GLUT_KEY_LEFT, GLUT_KEY_UP, GLUT_KEY_RIGHT, GLUT_KEY_DOWN, GLUT_KEY_PAGE_UP, GLUT_KEY_PAGE_DOWN, GLUT_KEY_HOME, GLUT_KEY_END, GLUT_KEY_INSERT
void iSpecialKeyboard(unsigned char key)
{
    switch (key)
    {
    default:
        break;
    }
}

void iMouseMove(int mx, int my)
{
    if (currentPage == MENU_PAGE)
    {
        // Hover effect
        if (my >= hoverRectangleYs[0] && my <= hoverRectangleYs[0] + hoverRectangleHeight)
        {
            // Resume button
            hoverRectangleY = hoverRectangleYs[0];
        }
        else if (my >= hoverRectangleYs[1] && my <= hoverRectangleYs[1] + hoverRectangleHeight)
        {
            // Levels button
            hoverRectangleY = hoverRectangleYs[1];
        }
        else if (my >= hoverRectangleYs[2] && my <= hoverRectangleYs[2] + hoverRectangleHeight)
        {
            // High Scores button
            hoverRectangleY = hoverRectangleYs[2];
        }
        else if (my >= hoverRectangleYs[3] && my <= hoverRectangleYs[3] + hoverRectangleHeight)
        {
            // Options button
            hoverRectangleY = hoverRectangleYs[3];
        }
        else if (my >= hoverRectangleYs[4] && my <= hoverRectangleYs[4] + hoverRectangleHeight)
        {
            // Exit button
            hoverRectangleY = hoverRectangleYs[4];
        }
        else
        {
            hoverRectangleY = -1;
        }
    }
    else if (currentPage == LEVELS_PAGE)
    {
        // Hover effect for levels
        if (my >= hoverRectangleYs[0] && my <= hoverRectangleYs[0] + hoverRectangleHeight)
        {
            // Level 1 button
            hoverRectangleY = hoverRectangleYs[0];
        }
        else if (my >= hoverRectangleYs[1] && my <= hoverRectangleYs[1] + hoverRectangleHeight)
        {
            // Level 2 button
            hoverRectangleY = hoverRectangleYs[1];
        }
        else if (my >= hoverRectangleYs[2] && my <= hoverRectangleYs[2] + hoverRectangleHeight)
        {
            // Level 3 button
            hoverRectangleY = hoverRectangleYs[2];
        }
        else if (my >= hoverRectangleYs[3] && my <= hoverRectangleYs[3] + hoverRectangleHeight)
        {
            // Level 4 button
            hoverRectangleY = hoverRectangleYs[3];
        }
        else if (my >= hoverRectangleYs[4] && my <= hoverRectangleYs[4] + hoverRectangleHeight)
        {
            // Level 5 button
            hoverRectangleY = hoverRectangleYs[4];
        }
        else
        {
            hoverRectangleY = -1;
        }
    }
}

void iMouseDrag(int mx, int my)
{
}

void iMouse(int button, int state, int mx, int my)
{
    // TODO: Add click effect?
    // Button click handling
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && currentPage == MENU_PAGE)
    {
        if (my >= hoverRectangleYs[0] && my <= hoverRectangleYs[0] + hoverRectangleHeight)
        {
            printf("Resume button clicked\n");
            currentPage = GAME_PAGE;
        }
        else if (my >= hoverRectangleYs[1] && my <= hoverRectangleYs[1] + hoverRectangleHeight)
        {
            printf("Levels button clicked\n");
            currentPage = LEVELS_PAGE;
        }
        else if (my >= hoverRectangleYs[2] && my <= hoverRectangleYs[2] + hoverRectangleHeight)
        {
            printf("High Scores button clicked\n");
            currentPage = HIGH_SCORES_PAGE;
        }
        else if (my >= hoverRectangleYs[3] && my <= hoverRectangleYs[3] + hoverRectangleHeight)
        {
            printf("Options button clicked\n");
            currentPage = OPTIONS_PAGE;
        }
        else if (my >= hoverRectangleYs[4] && my <= hoverRectangleYs[4] + hoverRectangleHeight)
        {
            printf("Exit button clicked\n");
            exit(0);
        }
    }
    else if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && currentPage == LEVELS_PAGE)
    {
        if (my >= hoverRectangleYs[0] && my <= hoverRectangleYs[0] + hoverRectangleHeight)
        {
            printf("Level 1 button clicked\n");
            currentLevel = 1;
            currentPage = GAME_PAGE;
        }
        else if (my >= hoverRectangleYs[1] && my <= hoverRectangleYs[1] + hoverRectangleHeight)
        {
            printf("Level 2 button clicked\n");
            currentLevel = 2;
            currentPage = GAME_PAGE;
        }
        else if (my >= hoverRectangleYs[2] && my <= hoverRectangleYs[2] + hoverRectangleHeight)
        {
            printf("Level 3 button clicked\n");
            currentLevel = 3;
            currentPage = GAME_PAGE;
        }
        else if (my >= hoverRectangleYs[3] && my <= hoverRectangleYs[3] + hoverRectangleHeight)
        {
            printf("Level 4 button clicked\n");
            currentLevel = 4;
            currentPage = GAME_PAGE;
        }
        else if (my >= hoverRectangleYs[4] && my <= hoverRectangleYs[4] + hoverRectangleHeight)
        {
            printf("Level 5 button clicked\n");
            currentLevel = 5;
            currentPage = GAME_PAGE;
        }
        collectedCoins = 0;
        loadLevel(currentLevel);
    }
    else
    {
    }
}

void iMouseWheel(int dir, int mx, int my)
{
}

int main(int argc, char *argv[])
{
    for (int i = 1; i < 5; i++)
    {
        hoverRectangleYs[i] = hoverRectangleYs[i - 1] - hoverRectangleHeight;
    }

    glutInit(&argc, argv);
    iInitialize(WIDTH, HEIGHT, TITLE);

    return 0;
}