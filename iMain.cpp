#include "iGraphics.h"

// TODO: Add player running sprite.
// TODO: Try to make the game responsive to different screen sizes and full screen.
// TODO: Divide the code into multiple files.
// TODO: Try to use object oriented programming.
// TODO: Try to add Dvorak keyboard support.
// TODO: Remove the printf statements after finishing the game.
// ? How often the iDraw() function is called? Is it constant or device dependent?

// TODO: Bug: Player moving to the right on its own.

// TODO: Handle keyboard control only if the current page is GAME_PAGE.

#define WIDTH 1280
#define HEIGHT 720
#define COLUMNS 32
#define ROWS 18
#define TITLE "Platformer Game"
#define TILE_SIZE (WIDTH / COLUMNS)

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
    OPTIONS_PAGE,
    GAME_OVER_PAGE,
    WIN_PAGE
};

enum TileType
{
    TILE_TOP,
    TILE_BOTTOM,
    TILE_LEFT,
    TILE_RIGHT,
    TILE_TOP_LEFT,
    TILE_TOP_RIGHT,
    TILE_BOTTOM_LEFT,
    TILE_BOTTOM_RIGHT,
    TILE
};

// typedef struct
// {
//     double x;
//     double y;
//     double length;
// } Blocker;

typedef struct
{
    double x = 200;
    double y = 150;
    double width = TILE_SIZE;
    double height = TILE_SIZE;
    Direction direction = RIGHT;
} Player;

// * Game UI management variables
int currentPage = MENU_PAGE;
int currentLevel = 1;
int hoverRectangleYs[5] = {HEIGHT / 2 + 40};
int hoverRectangleY = -1;
int hoverRectangleHeight = 50;
bool firstDraw = true;
char scoreText[50];

// * Asset management variables
Image background_image;
Image tileBrownTop;
Image tileBrownBottom;
Image tileBrownLeft;
Image tileBrownRight;
Image tileBrownTopLeft;
Image tileBrownTopRight;
Image tileBrownBottomLeft;
Image tileBrownBottomRight;
Image tileBrown;

char tiles[ROWS][COLUMNS + 1] = {};
Image coinFrames[5];
Sprite coinSprite;
Image playerIdleFrames[4];
Sprite playerIdleSprite;
// TODO: Check whether jumping needs another set of frames at the bottom.
Image playerJumpFrames[5];
Sprite playerJumpSprite;

// * Game state variables
int gameStateUpdateTimer;
int coinAnimationTimer;
bool blockedSides[4] = {false}; // Left, Right, Up, Down // TODO: Find a better way to handle this.
double velocityX = 0;
double velocityY = 0;
int collectedCoins = 0;
Player player;
// TODO: Include these in the player struct.
bool isJumping = false; // isJumping tells whether the player just jumped or not.
bool isOnAir = false;
int jumpAnimationFrame = 0;

// * These functions acts as UI Widgets.
void drawMenuPage();
void drawLevelsPage();
void drawHighScoresPage();
void drawOptionsPage();
void drawWinPage();
void drawGameOverPage();

void drawCoinCount();
void drawTilesAndCoins();

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

void loadAssets()
{
    // Load images
    iLoadImage(&background_image, "assets/images/menu_page_background.png");
    iResizeImage(&background_image, WIDTH, HEIGHT);

    // Load tiles
    iLoadImage(&tileBrownTop, "assets/images/tiles/tile_b_t.png");
    iResizeImage(&tileBrownTop, TILE_SIZE, TILE_SIZE);
    iLoadImage(&tileBrownBottom, "assets/images/tiles/tile_b_b.png");
    iResizeImage(&tileBrownBottom, TILE_SIZE, TILE_SIZE);
    iLoadImage(&tileBrownLeft, "assets/images/tiles/tile_b_l.png");
    iResizeImage(&tileBrownLeft, TILE_SIZE, TILE_SIZE);
    iLoadImage(&tileBrownRight, "assets/images/tiles/tile_b_r.png");
    iResizeImage(&tileBrownRight, TILE_SIZE, TILE_SIZE);
    iLoadImage(&tileBrownTopLeft, "assets/images/tiles/tile_b_tl.png");
    iResizeImage(&tileBrownTopLeft, TILE_SIZE, TILE_SIZE);
    iLoadImage(&tileBrownTopRight, "assets/images/tiles/tile_b_tr.png");
    iResizeImage(&tileBrownTopRight, TILE_SIZE, TILE_SIZE);
    iLoadImage(&tileBrownBottomLeft, "assets/images/tiles/tile_b_bl.png");
    iResizeImage(&tileBrownBottomLeft, TILE_SIZE, TILE_SIZE);
    iLoadImage(&tileBrownBottomRight, "assets/images/tiles/tile_b_br.png");
    iResizeImage(&tileBrownBottomRight, TILE_SIZE, TILE_SIZE);
    iLoadImage(&tileBrown, "assets/images/tiles/tile_b.png");
    iResizeImage(&tileBrown, TILE_SIZE, TILE_SIZE);

    // Load coin sprite
    iLoadFramesFromFolder(coinFrames, "assets/sprites/coin/");
    iInitSprite(&coinSprite);
    iChangeSpriteFrames(&coinSprite, coinFrames, 5);
    iResizeSprite(&coinSprite, (int)(TILE_SIZE / 1.5), (int)(TILE_SIZE / 1.5));

    // Load player sprite
    iLoadFramesFromFolder(playerIdleFrames, "assets/sprites/player/idle/");
    iInitSprite(&playerIdleSprite);
    iChangeSpriteFrames(&playerIdleSprite, playerIdleFrames, 4);
    iResizeSprite(&playerIdleSprite, TILE_SIZE, TILE_SIZE);

    iLoadFramesFromFolder(playerJumpFrames, "assets/sprites/player/jump/");
    iInitSprite(&playerJumpSprite);
    iChangeSpriteFrames(&playerJumpSprite, playerJumpFrames, 5);
    iResizeSprite(&playerJumpSprite, TILE_SIZE, TILE_SIZE);
}

void initializeHoverRectangleYs()
{
    for (int i = 1; i < 5; i++)
    {
        hoverRectangleYs[i] = hoverRectangleYs[i - 1] - hoverRectangleHeight;
    }
}

void resetGame()
{
    iPauseTimer(gameStateUpdateTimer);
    iPauseTimer(coinAnimationTimer);
    player.x = 200;
    player.y = 150;
    player.direction = RIGHT;
    velocityX = 0;
    velocityY = 0;
    isOnAir = false;
    isJumping = false;
    jumpAnimationFrame = 0;
    firstDraw = true;

    sprintf(scoreText, "Score: %d", collectedCoins);
    collectedCoins = 0;
}

// * Game logic functions
// TODO: Clean the if-elses.
// TODO: Add horizontal collision detection.
// TODO: Invert velocity for bouncing.
void checkCollisionWithTilesAndMoveAccordingly(double delX = 0, double delY = 0)
{
    int tileRow = (ROWS - 1) - (int)((player.y + delY) / TILE_SIZE);
    int tileCol = (int)((player.x + delX) / TILE_SIZE);

    if (tileRow < 0 || tileRow >= ROWS || tileCol < 0 || tileCol >= COLUMNS)
    {
        printf("OUT OF BOUNDS\n");
        return;
    }
    else if (tiles[tileRow][tileCol] == '#' || (tileCol < COLUMNS - 1 && tiles[tileRow][tileCol + 1] == '#'))
    {
        double tileY = (ROWS - tileRow - 1) * TILE_SIZE;
        if (player.y + delY < tileY + TILE_SIZE)
        {
            // TODO: Clean this mess
            // printf("%d\n", (tileCol + 1) * TILE_SIZE);
            // printf("%f\n", (player.x));
            // Blocked by a tile below.
            if (tiles[tileRow][tileCol] != '#' && (tileCol + 1) * TILE_SIZE - (player.x + delX + player.width) < 0.5)
            {
                player.x = tileCol * TILE_SIZE;
                velocityX = 0;
            }
            else if (tiles[tileRow][tileCol + 1] != '#' && player.x + delX - (tileCol + 1) * TILE_SIZE < 0.5)
            {
                player.x = (tileCol + 1) * TILE_SIZE;
                velocityX = 0;
            }
            else
            {
                player.y = tileY + TILE_SIZE;
                velocityY = 0;
                isOnAir = false;
                return;
            }
        }
    }
    else if (tileRow > 0 && (tiles[tileRow - 1][tileCol] == '#' || (tileCol < COLUMNS - 1 && tiles[tileRow - 1][tileCol + 1] == '#')))
    {
        double tileY = (ROWS - tileRow - 1) * TILE_SIZE;
        if (player.y + delY > tileY)
        {
            // Blocked by a tile above.
            player.y = tileY;
            velocityY = 0;
            return;
        }
    }
    player.x += delX;
    player.y += delY;
}

void setVerticalVelocity(double amount)
{
    velocityY = amount;
}

void setHorizontalVelocity(double amount)
{
    velocityX = amount;
}

void moveVerticallyIfPossible(double delY)
{
    if (player.y + delY < 0)
    {
        player.y = 0;
        velocityY = 0;
        isOnAir = false;
    }
    else if (player.y + player.height + delY > HEIGHT)
    {
        player.y = HEIGHT - player.height;
        velocityY = 0;
        isOnAir = false;
    }
    else
    {
        checkCollisionWithTilesAndMoveAccordingly(0, delY);
    }
}

void moveHorizontallyIfPossible(double delX)
{
    if (player.x + delX < 0)
    {
        player.x = 0;
        velocityX = 0;
    }
    else if (player.x + player.width + delX > WIDTH)
    {
        player.x = WIDTH - player.width;
        velocityX = 0;
    }
    else
    {
        checkCollisionWithTilesAndMoveAccordingly(delX, 0);
    }
}

void gameStateUpdate()
{
    // ? Store these as macros?
    double delT = 0.08;
    double horizontalResistance;
    if (isOnAir)
    {
        horizontalResistance = 15; // Air resistance
    }
    else
    {
        horizontalResistance = 22; // Friction
    }
    double gravity = 40;

    int velocityXDir = velocityX == 0 ? 0 : (velocityX > 0 ? 1 : -1);
    velocityX += -1 * velocityXDir * horizontalResistance * delT; // Apply air resistance
    double delX = velocityX * delT;
    moveHorizontallyIfPossible(delX);

    velocityY -= gravity * delT; // Apply gravity
    double delY = velocityY * delT;
    moveVerticallyIfPossible(delY);

    if (player.y <= 0)
    {
        resetGame();
        currentPage = GAME_OVER_PAGE;
    }
    if (player.x + player.width > WIDTH)
    {
        resetGame();
        currentPage = WIN_PAGE;
        if (currentLevel < 5)
        currentLevel++;
    }
}

void animateSprites()
{
    iAnimateSprite(&coinSprite);
    // TODO: Recheck the logic.
    if (velocityY == 0)
    {
        // If the player's jump animation is over, but still in air (velocityY != 0), then nothing is animated.
        iAnimateSprite(&playerIdleSprite);
    }
    else if (isJumping)
    {
        iAnimateSprite(&playerJumpSprite);
        jumpAnimationFrame++;
        if (jumpAnimationFrame >= 5)
        {
            isJumping = false;
            jumpAnimationFrame = 0;
        }
    }
}

// TODO: Revise this function.
void checkCollisionWithCoins()
{
    for (int row = 0; row < ROWS; row++)
    {
        for (int col = 0; col < COLUMNS; col++)
        {
            if (tiles[row][col] == 'O')
            {
                // ? Corner problem?
                double coinX = col * (TILE_SIZE) + (TILE_SIZE) / 2;
                double coinY = (ROWS - row - 1) * (TILE_SIZE) + (TILE_SIZE) / 2;
                if (player.x < coinX + 10 && player.x + player.width > coinX - 10 &&
                    player.y < coinY + 10 && player.y + player.height > coinY - 10)
                {
                    collectedCoins++;
                    tiles[row][col] = ' ';
                }
            }
        }
    }
}

void iDraw()
{
    if (firstDraw)
    {
        loadAssets();

        initializeHoverRectangleYs();

        loadLevel(currentLevel);
    }

    iClear();

    // Draw background
    iSetColor(255, 255, 255);
    iShowLoadedImage(0, 0, &background_image);

    // Draw tiles and coins
    drawTilesAndCoins();

    // Player
    if (velocityY > 0)
    {
        iSetSpritePosition(&playerJumpSprite, player.x, player.y);
        iShowSprite(&playerJumpSprite);
    }
    else
    {
        iSetSpritePosition(&playerIdleSprite, player.x, player.y);
        iShowSprite(&playerIdleSprite);
    }

    // TODO: Optimize by redrawing the widgets only when they're changed.
    checkCollisionWithCoins();
    drawCoinCount();

    // * Page rendering
    if (currentPage == MENU_PAGE)
    {
        drawMenuPage();
    }
    else if (currentPage == GAME_PAGE)
    {
        // TODO: Don't resume every time iDraw() is called.
        iResumeTimer(gameStateUpdateTimer);
        iResumeTimer(coinAnimationTimer);
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
    else if (currentPage == GAME_OVER_PAGE)
    {
        drawGameOverPage();
    }
    else if (currentPage == WIN_PAGE)
    {
        drawWinPage();
    }
    else
    {
        // TODO: Edge case handling.
    }

    firstDraw = false;
}

// * UI Widget: Small widget function definitions
void drawCoinCount()
{
    iSetColor(0, 0, 0);
    if (currentPage == GAME_PAGE) {
        sprintf(scoreText, "Score: %d", collectedCoins);
    }
    iTextAdvanced(WIDTH - 200, HEIGHT - 50, scoreText, 0.3, 2.5, GLUT_STROKE_ROMAN);
}

void drawTilesAndCoins()
{
    for (int row = 0; row < ROWS; row++)
    {
        for (int col = 0; col < COLUMNS; col++)
        {
            if (tiles[row][col] == '#')
            {
                // Tile
                double tileX = col * TILE_SIZE;
                double tileY = (ROWS - row - 1) * TILE_SIZE;
                TileType tileType;
                if (row == 0 || tiles[row - 1][col] != '#')
                {
                    if (col == 0 || tiles[row][col - 1] != '#')
                    {
                        tileType = TILE_TOP_LEFT;
                        iShowLoadedImage(tileX, tileY, &tileBrownTopLeft);
                    }
                    else if (col == COLUMNS - 1 || tiles[row][col + 1] != '#')
                    {
                        tileType = TILE_TOP_RIGHT;
                        iShowLoadedImage(tileX, tileY, &tileBrownTopRight);
                    }
                    else
                    {
                        tileType = TILE_TOP;
                        iShowLoadedImage(tileX, tileY, &tileBrownTop);
                    }
                }
                else if (row == ROWS - 1 || tiles[row + 1][col] != '#')
                {
                    if (col == 0 || tiles[row][col - 1] != '#')
                    {
                        tileType = TILE_BOTTOM_LEFT;
                        iShowLoadedImage(tileX, tileY, &tileBrownBottomLeft);
                    }
                    else if (col == COLUMNS - 1 || tiles[row][col + 1] != '#')
                    {
                        tileType = TILE_BOTTOM_RIGHT;
                        iShowLoadedImage(tileX, tileY, &tileBrownBottomRight);
                    }
                    else
                    {
                        tileType = TILE_BOTTOM;
                        iShowLoadedImage(tileX, tileY, &tileBrownBottom);
                    }
                }
                else if (col == 0 || tiles[row][col - 1] != '#')
                {
                    tileType = TILE_LEFT;
                    iShowLoadedImage(tileX, tileY, &tileBrownLeft);
                }
                else if (col == COLUMNS - 1 || tiles[row][col + 1] != '#')
                {
                    tileType = TILE_RIGHT;
                    iShowLoadedImage(tileX, tileY, &tileBrownRight);
                }
                else
                {
                    tileType = TILE;
                    iShowLoadedImage(tileX, tileY, &tileBrown);
                }
                // iShowLoadedImage(tileX, tileY, &tile);
            }
            else if (tiles[row][col] == 'O')
            {
                // Coin
                // TODO: Resizing makes the coin sprite blurry.
                // iScaleSprite(&coinSprite, 0.1);
                iSetSpritePosition(&coinSprite, col * (TILE_SIZE) + (TILE_SIZE) / 2, (ROWS - row - 1) * (TILE_SIZE) + (TILE_SIZE) / 2);
                iShowSprite(&coinSprite);
            }
        }
    }
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

void drawGameOverPage()
{
    iClear();

    iShowLoadedImage(0, 0, &background_image);

    iSetColor(0, 0, 0);
    iTextAdvanced(WIDTH / 2 - 180, HEIGHT / 2 + 50, "Game Over", 0.5, 5.0, GLUT_STROKE_ROMAN);
    iTextAdvanced(WIDTH / 2 - 90, HEIGHT / 2 - 50, scoreText, 0.3, 2.5, GLUT_STROKE_ROMAN);
}

void drawWinPage()
{
    iClear();

    iShowLoadedImage(0, 0, &background_image);

    char levelText[50];
    sprintf(levelText, "Level %d Completed", currentLevel - 1);

    iSetColor(0, 0, 0);
    iTextAdvanced(WIDTH / 2 - 280, HEIGHT / 2 + 50, levelText, 0.5, 5.0, GLUT_STROKE_ROMAN);
    iTextAdvanced(WIDTH / 2 - 90, HEIGHT / 2 - 50, scoreText, 0.3, 2.5, GLUT_STROKE_ROMAN);
}

// * Keyboard functions
void iKeyboard(unsigned char key, int state)
{
    // TODO: Add enter key handling to go to the hovered page when in the menu page. Also selecting / hovering buttons by the Up and Down arrow keys.
    switch (key)
    {
    case 27: // Escape key
        currentPage = MENU_PAGE;
        iPauseTimer(gameStateUpdateTimer);
        break;
    default:
        break;
    }
}

// GLUT_KEY_F1, GLUT_KEY_F2, GLUT_KEY_F3, GLUT_KEY_F4, GLUT_KEY_F5, GLUT_KEY_F6, GLUT_KEY_F7, GLUT_KEY_F8, GLUT_KEY_F9, GLUT_KEY_F10, GLUT_KEY_F11, GLUT_KEY_F12, GLUT_KEY_LEFT, GLUT_KEY_UP, GLUT_KEY_RIGHT, GLUT_KEY_DOWN, GLUT_KEY_PAGE_UP, GLUT_KEY_PAGE_DOWN, GLUT_KEY_HOME, GLUT_KEY_END, GLUT_KEY_INSERT
void iSpecialKeyboard(unsigned char key, int state)
{
    // TODO: Extract functions?
    if (currentPage != GAME_PAGE)
    {
        return;
    }
    switch (key)
    {
    case GLUT_KEY_UP:
    {
        setVerticalVelocity(100);
        isJumping = true;
        isOnAir = true;
        jumpAnimationFrame = 0;
        break;
    }
    case GLUT_KEY_LEFT:
    {
        setHorizontalVelocity(-80);
        if (player.direction == RIGHT)
        {
            iMirrorSprite(&playerIdleSprite, HORIZONTAL);
            iMirrorSprite(&playerJumpSprite, HORIZONTAL);
            player.direction = LEFT;
        }
        break;
    }
    case GLUT_KEY_RIGHT:
    {
        setHorizontalVelocity(80);
        if (player.direction == LEFT)
        {
            iMirrorSprite(&playerIdleSprite, HORIZONTAL);
            iMirrorSprite(&playerJumpSprite, HORIZONTAL);
            player.direction = RIGHT;
        }
        break;
    }
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
            iCloseWindow();
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
    // For debugging:
    // else if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && currentPage == GAME_PAGE)
    // {
    //     player.x = mx;
    //     player.y = my;
    //     checkCollisionWithTilesAndMoveAccordingly();
    // }
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

    gameStateUpdateTimer = iSetTimer(10, gameStateUpdate);
    coinAnimationTimer = iSetTimer(100, animateSprites);
    iPauseTimer(gameStateUpdateTimer);

    iOpenWindow(WIDTH, HEIGHT, TITLE);

    return 0;
}