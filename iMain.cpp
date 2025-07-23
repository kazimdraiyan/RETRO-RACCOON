#include "iGraphics.h"
#include "iFont.h"

// TODO: Add player running sprite.
// TODO: Try to make the game responsive to different screen sizes and full screen.
// TODO: Divide the code into multiple files.
// TODO: Try to use object oriented programming.
// TODO: Remove the printf statements after finishing the game.
// ? How often the iDraw() function is called? Is it constant or device dependent?

// TODO: Bug: Player moving to the right on its own.

// TODO: Handle keyboard control only if the current page is GAME_PAGE.
// TODO: Extract Position and Size structs.

#define WIDTH 1280
#define HEIGHT 720
#define COLUMNS 32
#define ROWS 18
#define TITLE "RETRO RACCOON"
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
    WIN_PAGE,
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

struct Color
{
    int red;
    int green;
    int blue;
    float alpha;
};

struct Player
{
    double x = 200;
    double y = 150;
    double width = TILE_SIZE;
    double height = TILE_SIZE;
    Direction direction = RIGHT;
};

struct Button
{
    int x;
    int y;
    int width;
    int height;
    Color bg_color;
    Color text_color;
    char *text;
    int fontSize;
    int x_offset;
    Page page;
    void (*onClick)(void);
};

// * Game UI management variables
int currentPage = MENU_PAGE;
int isResumable = 0;
int currentLevel = 1;
char levelText[50];
bool firstDraw = true;
char scoreText[50];
int mouseX = 0;
int mouseY = 0;

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
void drawButton(Button button);

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

    iInitializeFont();
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
    isResumable = 0;

    sprintf(scoreText, "Score: %d", collectedCoins);
    collectedCoins = 0;
}

void changeLevel(int level)
{
    resetGame();
    currentPage = GAME_PAGE;
    currentLevel = level;
    loadLevel(level);
}

struct Button buttons[50] = {
    {WIDTH - 400, 420, 380, 80, {0, 0, 0}, {200, 200, 200}, "RESUME", 40, 2, MENU_PAGE, []()
     { currentPage = GAME_PAGE; }},
    {WIDTH - 400, 320, 380, 80, {0, 0, 0}, {200, 200, 200}, "LEVELS", 40, 10, MENU_PAGE, []()
     { currentPage = LEVELS_PAGE; }},
    {WIDTH - 400, 220, 380, 80, {0, 0, 0}, {200, 200, 200}, "HIGH SCORES", 40, 24, MENU_PAGE, []()
     { currentPage = HIGH_SCORES_PAGE; }},
    {WIDTH - 400, 120, 380, 80, {0, 0, 0}, {200, 200, 200}, "OPTIONS", 40, 8, MENU_PAGE, []()
     { currentPage = OPTIONS_PAGE; }},
    {WIDTH - 400, 20, 380, 80, {0, 0, 0}, {200, 200, 200}, "EXIT", 40, 6, MENU_PAGE, []()
     { iCloseWindow(); }},

    {WIDTH / 2 - 180, 520, 380, 80, {0, 0, 0}, {200, 200, 200}, "LEVEL 1", 40, 8, LEVELS_PAGE, []()
     { changeLevel(1); }},
    {WIDTH / 2 - 180, 420, 380, 80, {0, 0, 0}, {200, 200, 200}, "LEVEL 2", 40, 8, LEVELS_PAGE, []()
     { changeLevel(2); }},
    {WIDTH / 2 - 180, 320, 380, 80, {0, 0, 0}, {200, 200, 200}, "LEVEL 3", 40, 8, LEVELS_PAGE, []()
     { changeLevel(3); }},
    {WIDTH / 2 - 180, 220, 380, 80, {0, 0, 0}, {200, 200, 200}, "LEVEL 4", 40, 8, LEVELS_PAGE, []()
     { changeLevel(4); }},
    {WIDTH / 2 - 180, 120, 380, 80, {0, 0, 0}, {200, 200, 200}, "LEVEL 5", 40, 8, LEVELS_PAGE, []()
     { changeLevel(5); }},
};

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

// TODO: Are these functions needed?
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
        sprintf(levelText, "Level %d Completed", currentLevel);
        resetGame();
        currentPage = WIN_PAGE;
        isResumable = 0;
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
        isResumable = 1;
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
    // else if (currentPage == TEST_PAGE)
    // {
    //     drawTestPage();
    // }
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
    if (currentPage == GAME_PAGE)
    {
        sprintf(scoreText, "Score: %d", collectedCoins);
    }
    iShowText(WIDTH - 210, HEIGHT - 78, scoreText, "assets/fonts/minecraft_ten.ttf", 40);
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

void drawButton(Button button)
{
    float alpha;
    if (mouseX >= button.x && mouseX <= button.x + button.width && mouseY >= button.y && mouseY <= button.y + button.height)
    {
        alpha = 0.7; // Hover effect
    }
    else
    {
        alpha = 0.9;
    }
    iSetTransparentColor(button.bg_color.red, button.bg_color.green, button.bg_color.blue, alpha);
    iFilledRectangle(button.x, button.y, button.width, button.height);

    // Approximate text width and height for centering
    int textLen = strlen(button.text);
    int textWidth = (int)(textLen * button.fontSize * 0.6);
    int textHeight = button.fontSize * 0.82;
    int textX = button.x + (button.width - textWidth) / 2 + button.x_offset;
    int textY = button.y + (button.height - textHeight) / 2;

    iSetColor(button.text_color.red, button.text_color.green, button.text_color.blue);
    iShowText(textX, textY, button.text, "assets/fonts/minecraft_ten.ttf", button.fontSize);
}

// * UI Widget: Page function definitions
void drawMenuPage()
{
    iClear();
    iShowLoadedImage(0, 0, &background_image);

    iSetColor(0, 0, 0);
    iShowText(40, 170, "RETRO", "assets/fonts/minecraft_ten.ttf", 167);
    iShowText(40, 40, "RACCOON", "assets/fonts/minecraft_ten.ttf", 120);

    for (int i = 0; i < 5; i++)
    {
        if (i == 0 && !isResumable)
        {
            continue;
        }
        drawButton(buttons[i]);
    }
}

void drawLevelsPage()
{
    iClear();
    iShowLoadedImage(0, 0, &background_image);

    for (int i = 5; i < 10; i++)
    {
        drawButton(buttons[i]);
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
    iShowText(WIDTH / 2 - 245, HEIGHT / 2 + 50, "Game Over", "assets/fonts/minecraft_ten.ttf", 100);
    iShowText(WIDTH / 2 - 110, HEIGHT / 2 - 50, scoreText, "assets/fonts/minecraft_ten.ttf", 60);
}

void drawWinPage()
{
    iClear();

    iShowLoadedImage(0, 0, &background_image);

    iSetColor(0, 0, 0);
    iShowText(200, HEIGHT / 2 + 50, levelText, "assets/fonts/minecraft_ten.ttf", 100);
    iShowText(WIDTH / 2 - 110, HEIGHT / 2 - 50, scoreText, "assets/fonts/minecraft_ten.ttf", 60);
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
void iMouse(int button, int state, int mx, int my)
{
    // TODO: Add click effect?
    // Button click handling
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        for (int i = 0; i < 10; i++)
        {
            if (buttons[i].page == currentPage && mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].width && my >= buttons[i].y && my <= buttons[i].y + buttons[i].height)
            {
                buttons[i].onClick();
            }
        }
    }
}

// TODO: Cleaner hover effect handling.
void iMouseMove(int mx, int my)
{
    mouseX = mx;
    mouseY = my;
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
    iPauseTimer(coinAnimationTimer);

    iOpenWindow(WIDTH, HEIGHT, TITLE);

    return 0;
}