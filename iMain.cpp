#include "iGraphics.h"

// TODO: Try to make the game responsive to different screen sizes and full screen.
// TODO: Divide the code into multiple files.
// TODO: Try to use object oriented programming.
// TODO: Try to add Dvorak keyboard support.
// TODO: Remove the printf statements after finishing the game.
// TODO: Use iSetTimer to handle the game loop instead of using t variable.
// ? How often the iDraw() function is called? Is it constant or device dependent?

#define WIDTH 1280
#define HEIGHT 720
#define COLUMNS 64
#define ROWS 36
#define TITLE "Platformer Game"

// TODO: Learn more about enum.
enum Direction
{
    LEFT,
    RIGHT,
    UP,
    DOWN
};

enum Page
{
    MENU_PAGE,
    GAME_PAGE,
    LEVELS_PAGE,
    HIGH_SCORES_PAGE,
    OPTIONS_PAGE
};

// * Game UI management variables
int currentPage = MENU_PAGE;
int currentLevel = 0;
int hoverRectangleYs[5] = {HEIGHT / 2 + 40};
int hoverRectangleY = -1;
int hoverRectangleHeight = 50;
bool firstDraw = true;

// * Asset management variables
Image background_image;
Image tile;
char tiles[ROWS][COLUMNS + 1] = {};

// * Game state variables
bool blockedSides[4] = {false}; // Left, Right, Up, Down // TODO: Find a better way to handle this.
double t = 0;
int collectedCoins = 0;
// TODO: Add Player struct
// Player represented as an array: {x, y, width, height}
double player[4] = {WIDTH / 9.0, 150, 30, 30};

// * These functions acts as UI Widgets.
void drawMenuPage();
void drawLevelsPage();
void drawHighScoresPage();
void drawOptionsPage();

void drawCoinCount();

// * Initialization functions
void loadLevel(int level)
{
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
            // ? Is input buffer handling needed?
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

void initializeHoverRectangleYs()
{
    for (int i = 1; i < 5; i++)
    {
        hoverRectangleYs[i] = hoverRectangleYs[i - 1] - hoverRectangleHeight;
    }
}

// * Game logic functions
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

        // Load images
        iLoadImage(&background_image, "assets/images/menu_page_background.png");
        iResizeImage(&background_image, WIDTH, HEIGHT);
        iLoadImage(&tile, "assets/images/tile.png");
        iResizeImage(&tile, WIDTH / COLUMNS, HEIGHT / ROWS);

        initializeHoverRectangleYs();

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
                // Tile
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

    // * Page rendering
    if (currentPage == MENU_PAGE)
    {
        drawMenuPage();
    }
    else if (currentPage == GAME_PAGE)
    {
        // TODO: Add a mechanism to pause the game when going to other pages.
        // TODO: Add a mechanism to resume the game when coming back to the game page.
    }
    else if (currentPage == LEVELS_PAGE)
    {
        drawLevelsPage();
    }
    else if (currentPage == HIGH_SCORES_PAGE)
    {
        drawHighScoresPage();
    }
    else if (currentPage == OPTIONS_PAGE)
    {
        drawOptionsPage();
    }
    else
    {
        // TODO: Edge case handling.
    }
}

// * UI Widget: Small widget function definitions
void drawCoinCount()
{
    iSetColor(0, 0, 0);
    char coinText[50];
    sprintf(coinText, "Coins: %d", collectedCoins);
    iTextAdvanced(WIDTH - 200, HEIGHT - 50, coinText, 0.3, 2.5, GLUT_STROKE_ROMAN);
}

// * UI Widget: Page function definitions
void drawMenuPage()
{
    iClear();
    iSetColor(255, 255, 255);

    iShowLoadedImage(0, 0, &background_image);

    iSetColor(0, 0, 0);
    iTextAdvanced(WIDTH / 2 - 250, HEIGHT / 2 + 200, "Platformer Game", 0.5, 5.0, GLUT_STROKE_ROMAN);

    // TODO: Make a widget for buttons
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
    // TODO: Make a widget for buttons
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

void drawHighScoresPage()
{
    // TODO: Implement high scores page.
}

void drawOptionsPage()
{
    // TODO: Implement options page.
}

// * Keyboard functions
void iKeyboard(unsigned char key)
{
    // TODO: Add enter key handling to go to the hovered page when in the menu page. Also selecting / hovering buttons by the Up and Down arrow keys.
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

// GLUT_KEY_F1, GLUT_KEY_F2, GLUT_KEY_F3, GLUT_KEY_F4, GLUT_KEY_F5, GLUT_KEY_F6, GLUT_KEY_F7, GLUT_KEY_F8, GLUT_KEY_F9, GLUT_KEY_F10, GLUT_KEY_F11, GLUT_KEY_F12, GLUT_KEY_LEFT, GLUT_KEY_UP, GLUT_KEY_RIGHT, GLUT_KEY_DOWN, GLUT_KEY_PAGE_UP, GLUT_KEY_PAGE_DOWN, GLUT_KEY_HOME, GLUT_KEY_END, GLUT_KEY_INSERT
void iSpecialKeyboard(unsigned char key)
{
    switch (key)
    {
    default:
        break;
    }
}

// * Mouse functions
// TODO: Cleaner button click handling.
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

// TODO: Cleaner hover effect handling.
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

// * Unused functions
void iMouseWheel(int dir, int mx, int my)
{
}

void iMouseDrag(int mx, int my)
{
}

int main(int argc, char *argv[])
{
    glutInit(&argc, argv); // argc and argv are used for command line arguments.
    iInitialize(WIDTH, HEIGHT, TITLE);

    return 0;
}