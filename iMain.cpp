#include "iGraphics.h"
#include "iFont.h"
#include "iSound.h"

// * Bugs
// TODO: Bug: Player moving to the right on its own.
// TODO: Reckheck pausing mechanism.
// TODO: Resume button cilckable even if it's not shown on the screen.
// TODO: Life bugs.

// * Optimization
// TODO: Optimize asset loading.
// TODO: Free images and sprites.
// TODO: Handle keyboard control only if the current page is GAME_PAGE.

// * Tasks
// TODO: Add player running sprite.
// TODO: Try to make the game full screen.
// TODO: Divide the code into multiple files.
// TODO: Try to use object oriented programming.
// TODO: Extract Position and Size structs.
// TODO: Remove the printf statements after finishing the game.
// TODO: Add loading screen.
// TODO: Make all snake_case variables camelCase.
// TODO: Turn all boolean ints into booleans.
// TODO: Sprite image count marco instead of hardcoding.
// TODO: fclose the files.

// ? How often the iDraw() function is called? Is it constant or device dependent?

#define TITLE "RETRO RACCOON"
#define WIDTH 1280
#define HEIGHT 720
#define COLUMNS 32
#define ROWS 18
#define TILE_SIZE (WIDTH / COLUMNS)

#define LEVEL_COUNT 5
#define MAX_LAYER_COUNT 10
#define MAX_COLLECTABLE_COUNT 30

#define FLIPPED_HORIZONTALLY_FLAG 0x80000000
#define FLIPPED_VERTICALLY_FLAG 0x40000000
#define DOES_COLLIDE_FLAG 0x10000000

#define FLAG_ID 111
#define FULL_LIFE_ID 44
#define NO_LIFE_ID 46
#define TRAP_ID 68
#define COIN_ID 151
#define DIAMOND_ID 67

#define COIN_SCORE 10
#define DIAMOND_SCORE 20

#define PLAYER_INITIAL_X 200
#define PLAYER_INITIAL_Y 300
#define X_ANIMATION_DEL_X 5

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

struct Color
{
    int red;
    int green;
    int blue;
    float alpha;
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
    bool isHovered;
};

struct Player
{
    int x = PLAYER_INITIAL_X;
    double y = PLAYER_INITIAL_Y;
    double width = TILE_SIZE;
    double height = TILE_SIZE;
    Direction direction = RIGHT;
};

// * Game UI management variables
int currentPage = MENU_PAGE;
int currentLevel = 1;
int isResumable = 0;
int isFirstDraw = 1;
char levelCompletionText[50]; // TODO: Find a better way to handle this.
char scoreText[50];
int mouseX = 0;
int mouseY = 0;

// * Asset management variables
Image background_image;
Image lifeImages[2];
Image tileImages[180]; // TODO: Load only the tiles that are needed.
Image coinFrames[2];
Sprite coinSprite;
Image flagFrames[2];
Sprite flagSprite;
Image playerIdleFrames[4];
Sprite playerIdleSprite;
Image playerJumpFrames[5];
Sprite playerJumpSprite;
int loadedLevels[LEVEL_COUNT] = {0};
int layerCount;
int tiles[MAX_LAYER_COUNT][ROWS][COLUMNS][3] = {};
int doesCollideArray[ROWS][COLUMNS] = {};
int coinArray[ROWS][COLUMNS] = {};
int diamondArray[ROWS][COLUMNS] = {};
int lifeArray[ROWS][COLUMNS] = {};
int trapArray[ROWS][COLUMNS] = {};

// * Sound management variables
int backgroundMusicChannel = -1;
int currentBackgroundMusic = -1; // -1: none, 0: menu, 1: game
bool isBackgroundMusicPlaying = false;

// * Game state variables
int gameStateUpdateTimer;
int horizontalMovementTimer;
int spriteAnimationTimer; // TODO: Create separate timer for each sprite animation?
Player player;
int animateToX = player.x;
double velocityY = 0;
int score = 0;
int lifeCount = 3;
// TODO: Include these in the player struct.
// TODO: Handle the jump in a better way?
bool isJumping = false; // isJumping tells whether the player just jumped or not.
bool isOnAir = false;
int jumpAnimationFrame = 0;
double gravity = 80;

// Stores the row and column of the collected coins and diamonds.
int collectedCoins[MAX_COLLECTABLE_COUNT][2] = {};
int collectedDiamonds[MAX_COLLECTABLE_COUNT][2] = {};
int collectedLife[MAX_COLLECTABLE_COUNT][2] = {};
int collectedCoinCount = 0;
int collectedDiamondCount = 0;
int collectedLifeCount = 0;

// * These functions acts as UI Widgets.
void drawMenuPage();
void drawLevelsPage();
void drawHighScoresPage();
void drawOptionsPage();
void drawWinPage();
void drawGameOverPage();

void drawScore();
void drawLifeCount();
void drawTile(int layer, int row, int col, Sprite *sprite = NULL);
void drawTiles();
void drawButton(Button &button);

// * Background music management functions
void playBackgroundMusic(int musicType);
void stopBackgroundMusic();
void switchBackgroundMusic(int newMusicType);

// * Initialization functions
void initializeGridArrays(int array[ROWS][COLUMNS], int value)
{
    for (int row = 0; row < ROWS; row++)
    {
        for (int col = 0; col < COLUMNS; col++)
        {
            array[row][col] = value;
        }
    }
}

void loadLevel(int level)
{
    if (loadedLevels[level - 1] == 1)
    {
        return;
    }

    // ? Should I initialize the arrays here?
    initializeGridArrays(doesCollideArray, 0);
    initializeGridArrays(coinArray, 0);
    initializeGridArrays(diamondArray, 0);
    initializeGridArrays(lifeArray, 0);
    initializeGridArrays(trapArray, 0);

    // Initialize the collected collectables.
    for (int i = 0; i < MAX_COLLECTABLE_COUNT; i++)
    {
        collectedCoins[i][0] = -1;
        collectedCoins[i][1] = -1;
        collectedDiamonds[i][0] = -1;
        collectedDiamonds[i][1] = -1;
        collectedLife[i][0] = -1;
        collectedLife[i][1] = -1;
    }

    char level_metadata_filename[100];
    sprintf(level_metadata_filename, "levels/level%d/n.txt", level);
    FILE *level_metadata_file = fopen(level_metadata_filename, "r");
    if (level_metadata_file == NULL)
    {
        // TODO: Bug: Why is this called twice?
        printf("Level metadata file open failed\n");
        return;
    }
    fscanf(level_metadata_file, "%d", &layerCount);
    fclose(level_metadata_file);

    for (int layer = 0; layer < layerCount; layer++)
    {
        char layer_file_name[100];
        sprintf(layer_file_name, "levels/level%d/layer_%d_customized.csv", level, layer);
        FILE *layer_file = fopen(layer_file_name, "r");
        if (layer_file == NULL)
        {
            printf("Layer file open failed\n");
            return;
        }

        char line[1000]; // TODO: Adjust the size
        int row = 0;
        // TODO: Learn how this works in-depth.
        while (fgets(line, sizeof(line), layer_file))
        {
            char *cell;
            cell = strtok(line, ",\n");
            int col = 0;
            while (cell)
            {
                int gid = atoi(cell);
                if (gid == -1)
                {
                    tiles[layer][row][col][0] = -1;
                    tiles[layer][row][col][1] = 0;
                    tiles[layer][row][col][2] = 0;
                }
                else
                {
                    int flip_h = (gid & FLIPPED_HORIZONTALLY_FLAG) != 0;
                    int flip_v = (gid & FLIPPED_VERTICALLY_FLAG) != 0;
                    int does_collide = (gid & DOES_COLLIDE_FLAG) != 0;

                    // Mask out flip flags to get actual GID
                    int id = gid & ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | DOES_COLLIDE_FLAG);
                    tiles[layer][row][col][0] = id;
                    tiles[layer][row][col][1] = flip_h;
                    tiles[layer][row][col][2] = flip_v;
                    if (does_collide == 1)
                    {
                        doesCollideArray[row][col] = 1;
                    }
                    if (id == COIN_ID)
                    {
                        coinArray[row][col] = 1;
                    }
                    else if (id == DIAMOND_ID)
                    {
                        diamondArray[row][col] = 1;
                    }
                    else if (id == TRAP_ID)
                    {
                        trapArray[row][col] = 1;
                    }
                    else if (id == FULL_LIFE_ID)
                    {
                        lifeArray[row][col] = 1;
                    }
                }
                cell = strtok(NULL, ",\n");
                col++;
            }
            row++;
        }
        fclose(layer_file);
    }

    // Mark the level as loaded.
    printf("Level %d loaded\n", level);
    loadedLevels[level - 1] = 1;
}

void loadAssets()
{
    // Load images
    iLoadImage(&background_image, "assets/backgrounds/background.png");
    iResizeImage(&background_image, WIDTH, HEIGHT);

    // Load tiles
    for (int i = 0; i < 180; i++)
    {
        char filename[100];
        sprintf(filename, "assets/tiles/%d.png", i);
        iLoadImage(&tileImages[i], filename);
        iResizeImage(&tileImages[i], TILE_SIZE, TILE_SIZE);
    }

    // Load life images
    iLoadImage(&lifeImages[0], "assets/special_tiles/no_life.png");
    iLoadImage(&lifeImages[1], "assets/special_tiles/full_life.png");

    // Load coin sprite
    iLoadFramesFromFolder(coinFrames, "assets/sprites/coin/");
    iInitSprite(&coinSprite);
    iChangeSpriteFrames(&coinSprite, coinFrames, 2);
    iResizeSprite(&coinSprite, TILE_SIZE, TILE_SIZE);

    iLoadFramesFromFolder(flagFrames, "assets/sprites/flag/");
    iInitSprite(&flagSprite);
    iChangeSpriteFrames(&flagSprite, flagFrames, 2);
    iResizeSprite(&flagSprite, TILE_SIZE, TILE_SIZE);

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
    iInitializeSound();

    playBackgroundMusic(0); // Menu music
}

void resetGame()
{
    // TODO: Check this rigorously.
    iPauseTimer(gameStateUpdateTimer);
    iPauseTimer(spriteAnimationTimer);
    player.x = PLAYER_INITIAL_X;
    player.y = PLAYER_INITIAL_Y;
    animateToX = player.x;
    velocityY = 0;
    player.direction = RIGHT;
    isJumping = false;
    jumpAnimationFrame = 0;
    isResumable = 0;
    lifeCount = 3;

    sprintf(scoreText, "Score: %d", score);
    score = 0;

    // Reset the collected collectables.
    collectedCoinCount = 0;
    collectedDiamondCount = 0;
    collectedLifeCount = 0;
    for (int i = 0; i < MAX_COLLECTABLE_COUNT; i++)
    {
        collectedCoins[i][0] = -1;
        collectedCoins[i][1] = -1;
        collectedDiamonds[i][0] = -1;
        collectedDiamonds[i][1] = -1;
        collectedLife[i][0] = -1;
        collectedLife[i][1] = -1;
    }
}

void changeLevel(int level)
{
    resetGame();
    currentPage = GAME_PAGE;
    currentLevel = level;
    loadLevel(level);

    switchBackgroundMusic(1); // Game music
}

struct Button buttons[50] = {
    {WIDTH - 400, 420, 380, 80, {0, 0, 0}, {200, 200, 200}, "RESUME", 40, 2, MENU_PAGE, []()
     {
         currentPage = GAME_PAGE;
         switchBackgroundMusic(1); // Game music
     },
     false},
    {WIDTH - 400, 320, 380, 80, {0, 0, 0}, {200, 200, 200}, "LEVELS", 40, 10, MENU_PAGE, []()
     {
         currentPage = LEVELS_PAGE;
         switchBackgroundMusic(0); // Menu music
     },
     false},
    {WIDTH - 400, 220, 380, 80, {0, 0, 0}, {200, 200, 200}, "HIGH SCORES", 40, 24, MENU_PAGE, []()
     {
         currentPage = HIGH_SCORES_PAGE;
         switchBackgroundMusic(0); // Menu music
     },
     false},
    {WIDTH - 400, 120, 380, 80, {0, 0, 0}, {200, 200, 200}, "OPTIONS", 40, 8, MENU_PAGE, []()
     {
         currentPage = OPTIONS_PAGE;
         switchBackgroundMusic(0); // Menu music
     },
     false},
    {WIDTH - 400, 20, 380, 80, {0, 0, 0}, {200, 200, 200}, "EXIT", 40, 6, MENU_PAGE, []()
     { iCloseWindow(); },
     false},

    {WIDTH / 2 - 180, 520, 380, 80, {0, 0, 0}, {200, 200, 200}, "LEVEL 1", 40, 8, LEVELS_PAGE, []()
     { changeLevel(1); },
     false},
    {WIDTH / 2 - 180, 420, 380, 80, {0, 0, 0}, {200, 200, 200}, "LEVEL 2", 40, 8, LEVELS_PAGE, []()
     { changeLevel(2); },
     false},
    {WIDTH / 2 - 180, 320, 380, 80, {0, 0, 0}, {200, 200, 200}, "LEVEL 3", 40, 8, LEVELS_PAGE, []()
     { changeLevel(3); },
     false},
    {WIDTH / 2 - 180, 220, 380, 80, {0, 0, 0}, {200, 200, 200}, "LEVEL 4", 40, 8, LEVELS_PAGE, []()
     { changeLevel(4); },
     false},
    {WIDTH / 2 - 180, 120, 380, 80, {0, 0, 0}, {200, 200, 200}, "LEVEL 5", 40, 8, LEVELS_PAGE, []()
     { changeLevel(5); },
     false},
}; // TODO: Add extra dimension for pages?

// * Game logic functions
// TODO: Clean the if-elses.
// TODO: Add horizontal collision detection.
// TODO: Invert velocity for bouncing.
void moveVerticallyTillCollision(double delY)
{
    if (player.y + delY < 0) // Collision with the bottom of the screen.
    {
        player.y = 0;
        velocityY = 0;
        isOnAir = false;
    }
    else if (player.y + player.height + delY > HEIGHT) // Collision with the top of the screen.
    {
        player.y = HEIGHT - player.height;
        velocityY = 0;
    }
    else
    {
        int row = (int)((player.y + delY) / TILE_SIZE);
        int col = (int)(animateToX / TILE_SIZE);
        if (doesCollideArray[ROWS - row - 1][col]) // Collision with the tile below the player.
        {
            player.y = (row + 1) * TILE_SIZE;
            velocityY = 0;
            isOnAir = false;
        }
        else if (doesCollideArray[ROWS - row - 2][col]) // Collision with the tile above the player.
        {
            player.y = row * TILE_SIZE;
            velocityY = 0;
        }
        else
        {
            player.y += delY;
        }
    }
}

void gameStateUpdate()
{
    // ? Store these as macros?
    double delT = 0.08;

    velocityY -= gravity * delT; // Apply gravity
    double delY = velocityY * delT;

    moveVerticallyTillCollision(delY);

    if (player.x + player.width > WIDTH)
    {
        sprintf(levelCompletionText, "Level %d Completed", currentLevel);
        resetGame();

        stopBackgroundMusic();
        iPlaySound("assets/sounds/level_complete.wav", 0, 80);

        currentPage = WIN_PAGE;
        isResumable = 0;
    }
}

void animateHorizontalMovement()
{
    if (player.x < animateToX)
    {
        player.x += X_ANIMATION_DEL_X;
    }
    else if (player.x > animateToX)
    {
        player.x -= X_ANIMATION_DEL_X;
    }
    else
    {
        iPauseTimer(horizontalMovementTimer);
    }
}

void animateSprites()
{
    iAnimateSprite(&coinSprite);
    iAnimateSprite(&flagSprite);
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

int checkIfAlreadyCollected(int row, int col, int collectedCollectableArray[][2], int *collectedCollectableCount)
{
    for (int i = 0; i < *collectedCollectableCount; i++)
    {
        if (collectedCollectableArray[i][0] == row && collectedCollectableArray[i][1] == col)
        {
            return 1;
        }
    }
    return 0;
}

void checkAndCollect(int collectableArray[ROWS][COLUMNS], int collectableId, int collectableScore, int collectedCollectableArray[][2], int *collectedCollectableCount, int isLife = 0, const char *soundPath = NULL, int volume = 100)
{
    int row = ROWS - (int)(player.y / TILE_SIZE) - 1;
    int col = (int)(animateToX / TILE_SIZE);

    if (collectableArray[row][col]) // Collision tested rigorously.
    {
        if (!checkIfAlreadyCollected(row, col, collectedCollectableArray, collectedCollectableCount))
        {
            score += collectableScore;
            collectedCollectableArray[*collectedCollectableCount][0] = row;
            collectedCollectableArray[*collectedCollectableCount][1] = col;
            (*collectedCollectableCount)++;
            if (isLife && lifeCount < 3) // If the player has 3 lives, collect the life but don't increment the count.
                lifeCount++;

            if (soundPath != NULL)
                iPlaySound(soundPath, 0, volume);
        }
    }
}

void updateAllCollectables()
{
    checkAndCollect(coinArray, COIN_ID, COIN_SCORE, collectedCoins, &collectedCoinCount, 0, "assets/sounds/coin.wav", 70);
    checkAndCollect(diamondArray, DIAMOND_ID, DIAMOND_SCORE, collectedDiamonds, &collectedDiamondCount, 0, "assets/sounds/diamond.wav", 100);
    checkAndCollect(lifeArray, FULL_LIFE_ID, 0, collectedLife, &collectedLifeCount, 1, "assets/sounds/life.wav", 70);
}

void checkCollisionWithTraps()
{
    int row = ROWS - (int)(player.y / TILE_SIZE) - 1;
    int col = (int)(animateToX / TILE_SIZE);

    if (trapArray[row][col] && player.x == animateToX)
    {
        // TODO: Add an animation for the trap.
        // TODO: Check if this is the correct way to reset the player.
        player.x = PLAYER_INITIAL_X;
        player.y = PLAYER_INITIAL_Y;
        animateToX = player.x;
        velocityY = 0;
        if (player.direction == LEFT)
        {
            iMirrorSprite(&playerIdleSprite, HORIZONTAL);
            iMirrorSprite(&playerJumpSprite, HORIZONTAL);
        }
        player.direction = RIGHT;
        isJumping = false;
        jumpAnimationFrame = 0;
        lifeCount--;

        if (lifeCount > 0)
            iPlaySound("assets/sounds/hurt.wav", 0, 50);

        if (lifeCount == 0)
        {
            resetGame();
            stopBackgroundMusic();
            iPlaySound("assets/sounds/game_over.wav", 0, 80);
            currentPage = GAME_OVER_PAGE;
        }
    }
}

// * Background music management functions
void playBackgroundMusic(int musicType)
{
    // Stop any currently playing background music
    if (isBackgroundMusicPlaying)
    {
        iStopSound(backgroundMusicChannel);
        isBackgroundMusicPlaying = false;
    }

    const char *musicFile;
    if (musicType == 0) // Menu music
    {
        musicFile = "assets/sounds/menu_bg.wav";
        currentBackgroundMusic = 0;
    }
    else if (musicType == 1) // Game music
    {
        musicFile = "assets/sounds/game_bg.wav";
        currentBackgroundMusic = 1;
    }
    else
    {
        return; // Invalid music type
    }

    backgroundMusicChannel = iPlaySound(musicFile, true, musicType == 0 ? 50 : 25); // Game music volume is lower than the menu music volume.
    isBackgroundMusicPlaying = true;
}

void stopBackgroundMusic()
{
    if (isBackgroundMusicPlaying)
    {
        iStopSound(backgroundMusicChannel);
        isBackgroundMusicPlaying = false;
        currentBackgroundMusic = -1;
    }
}

void switchBackgroundMusic(int newMusicType)
{
    if (currentBackgroundMusic != newMusicType)
    {
        playBackgroundMusic(newMusicType);
    }
}

void iDraw()
{
    if (isFirstDraw)
    {
        loadAssets();
        loadLevel(currentLevel);
    }

    iClear();

    // Draw background
    iSetColor(255, 255, 255);
    iShowLoadedImage(0, 0, &background_image);

    // Draw tiles and coins
    drawTiles();

    // Player
    // iFilledRectangle(player.x, player.y, player.width, player.height);
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
    updateAllCollectables();
    checkCollisionWithTraps();
    drawScore();
    drawLifeCount();

    // * Page rendering
    if (currentPage == MENU_PAGE)
    {
        drawMenuPage();
    }
    else if (currentPage == GAME_PAGE)
    {
        // TODO: Don't resume every time iDraw() is called.
        iResumeTimer(gameStateUpdateTimer);
        iResumeTimer(spriteAnimationTimer);
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

    isFirstDraw = false;
}

// * UI Widget: Small widget function definitions
void drawScore()
{
    iSetColor(0, 0, 0);
    if (currentPage == GAME_PAGE)
    {
        sprintf(scoreText, "Score: %d", score);
    }
    iShowText(30, HEIGHT - 60, scoreText, "assets/fonts/minecraft_ten.ttf", 48);
}

void drawLifeCount()
{
    iShowLoadedImage(WIDTH - 80, HEIGHT - 72, &lifeImages[lifeCount > 0 ? 1 : 0]);
    iShowLoadedImage(WIDTH - 135, HEIGHT - 72, &lifeImages[lifeCount > 1 ? 1 : 0]);
    iShowLoadedImage(WIDTH - 190, HEIGHT - 72, &lifeImages[lifeCount > 2 ? 1 : 0]);
}

void drawTiles()
{
    for (int layer = 0; layer < layerCount; layer++)
    {
        for (int row = 0; row < ROWS; row++)
        {
            for (int col = 0; col < COLUMNS; col++)
            {
                if (tiles[layer][row][col][0] != -1)
                {
                    if (tiles[layer][row][col][0] == FLAG_ID)
                    {
                        drawTile(layer, row, col, &flagSprite);
                    }
                    else if (tiles[layer][row][col][0] == COIN_ID)
                    {
                        if (!checkIfAlreadyCollected(row, col, collectedCoins, &collectedCoinCount))
                            drawTile(layer, row, col, &coinSprite);
                    }
                    else if (tiles[layer][row][col][0] == DIAMOND_ID)
                    {
                        if (!checkIfAlreadyCollected(row, col, collectedDiamonds, &collectedDiamondCount))
                            drawTile(layer, row, col);
                    }
                    else if (tiles[layer][row][col][0] == FULL_LIFE_ID)
                    {
                        if (!checkIfAlreadyCollected(row, col, collectedLife, &lifeCount))
                            drawTile(layer, row, col);
                    }
                    else
                    {
                        drawTile(layer, row, col);
                    }
                }
            }
        }
    }
}

void mirrorTile(int tileId, MirrorState mirrorState, Sprite *sprite = NULL)
{
    if (sprite == NULL)
        iMirrorImage(&tileImages[tileId], mirrorState);
    else
        iMirrorSprite(sprite, mirrorState);
}

void drawTile(int layer, int row, int col, Sprite *sprite)
{
    int tileId = tiles[layer][row][col][0];
    int flipH = tiles[layer][row][col][1];
    int flipV = tiles[layer][row][col][2];
    int x = col * TILE_SIZE;
    int y = (ROWS - row - 1) * TILE_SIZE;
    if (flipH)
        mirrorTile(tileId, HORIZONTAL, sprite);
    if (flipV)
        mirrorTile(tileId, VERTICAL, sprite);
    if (sprite == NULL)
    {
        iShowLoadedImage(x, y, &tileImages[tileId]);
    }
    else
    {
        iSetSpritePosition(sprite, x, y);
        iShowSprite(sprite);
    }
    // Mirror the tile back to its original state.
    if (flipH)
        mirrorTile(tileId, HORIZONTAL, sprite);
    if (flipV)
        mirrorTile(tileId, VERTICAL, sprite);
}

void drawButton(Button &button)
{
    float alpha;
    bool isHovering = mouseX >= button.x && mouseX <= button.x + button.width && mouseY >= button.y && mouseY <= button.y + button.height;

    if (isHovering)
    {
        alpha = 0.7; // Hover effect
        if (!button.isHovered)
        {
            iPlaySound("assets/sounds/menu_hover.wav", 0, 5);
        }
        button.isHovered = true;
    }
    else
    {
        alpha = 0.9;
        button.isHovered = false;
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
    iShowText(200, HEIGHT / 2 + 50, levelCompletionText, "assets/fonts/minecraft_ten.ttf", 100);
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
        switchBackgroundMusic(0); // Switch to menu music
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
        if (!isOnAir)
        {
            iPlaySound("assets/sounds/jump.wav", 0, 50);
            velocityY = 150;
            isJumping = true;
            jumpAnimationFrame = 0;
            isOnAir = true;
        }
        break;
    }
    case GLUT_KEY_LEFT:
    {
        if (state == GLUT_DOWN && !doesCollideArray[ROWS - (int)(player.y / TILE_SIZE) - 1][(player.x / TILE_SIZE) - 1] && animateToX >= TILE_SIZE)
        {
            iResumeTimer(horizontalMovementTimer);
            animateToX -= TILE_SIZE;
        }
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
        // setHorizontalVelocity(80);
        if (state == GLUT_DOWN && !doesCollideArray[ROWS - (int)(player.y / TILE_SIZE) - 1][(player.x / TILE_SIZE) + 1])
        {
            iResumeTimer(horizontalMovementTimer);
            animateToX += TILE_SIZE;
        }
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
                printf("Clicked button: %s\n", buttons[i].text);
                iPlaySound("assets/sounds/menu_click.wav", 0, 30);
                buttons[i].onClick();
            }
        }
    }
    // // For debugging purposes.
    // if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
    // {
    //     gravity *= -1;
    // }
}

void iMouseMove(int mx, int my)
{
    // For hover effect.
    mouseX = mx;
    mouseY = my;
}

// * Unused functions
void iMouseWheel(int dir, int mx, int my)
{
}

void iMouseDrag(int mx, int my)
{
    // // For debugging purposes.
    // player.x = mx;
    // player.y = my;
}

int main(int argc, char *argv[])
{
    glutInit(&argc, argv); // argc and argv are used for command line arguments.

    gameStateUpdateTimer = iSetTimer(10, gameStateUpdate);
    horizontalMovementTimer = iSetTimer(10, animateHorizontalMovement);
    spriteAnimationTimer = iSetTimer(200, animateSprites);
    iPauseTimer(gameStateUpdateTimer);
    iPauseTimer(horizontalMovementTimer);
    iPauseTimer(spriteAnimationTimer);

    iOpenWindow(WIDTH, HEIGHT, TITLE);

    return 0;
}