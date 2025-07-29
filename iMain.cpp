#include "iGraphics.h"
#include "iFont.h"
#include "iSound.h"

// * Bugs
// TODO: Reckheck pausing mechanism.
// TODO: Player direction bug.

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
#define COIN_ID 151
#define DIAMOND_ID 67

#define COIN_SCORE 10
#define DIAMOND_SCORE 50

#define PLAYER_INITIAL_X 200 // 1200
#define PLAYER_INITIAL_Y 300 // 500
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
    HELP_PAGE,
    CREDITS_PAGE,
};

enum MusicType
{
    MENU_MUSIC,
    GAME_MUSIC,
    NONE
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
    Color bgColor;
    Color textColor;
    char *text;
    int fontSize;
    int xOffset;
    int yOffset;
    Page page;
    void (*onClick)(void);
    bool isHovered;
};

struct Icon
{
    int x;
    int y;
    int width;
    int height;
    Color bgColor;
    Image *image;
    int xOffset;
    int yOffset;
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
Image yellowStarImage;
Image whiteStarImage;
Image audioOffImage;
Image audioOnImage;
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
int layerCount[LEVEL_COUNT] = {0};
int tiles[MAX_LAYER_COUNT][ROWS][COLUMNS][3] = {};
int doesCollideArray[ROWS][COLUMNS] = {};
int coinArray[ROWS][COLUMNS] = {};
int diamondArray[ROWS][COLUMNS] = {};
int lifeArray[ROWS][COLUMNS] = {};
int trapArray[ROWS][COLUMNS] = {};
int trapIds[] = {68, 33, 34, 35, 53, 54, 55, 73, 74, 75};

// * Sound management variables
int backgroundMusicChannel = -1;
MusicType currentMusicType = MENU_MUSIC;
bool isBackgroundMusicPlaying = false;
bool isMusicOn = true;
bool isSoundOn = true;

// * Game state variables
int gameStateUpdateTimer;
int horizontalMovementTimer;
int spriteAnimationTimer; // TODO: Create separate timer for each sprite animation?
Player player;
int animateToX = player.x;
double velocityY = 0;
int score = 0;
int lifeCount = 3;
int starCount = 0;
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
void drawHelpPage();
void drawCreditsPage();
void drawWinPage();
void drawGameOverPage();

void drawScore();
void drawLifeCount();
void drawTile(int layer, int row, int col, Sprite *sprite = NULL);
void drawTiles();
void drawButton(Button &button);

// * Background music management functions
void playBackgroundMusic(MusicType musicType);
void stopBackgroundMusic();
void switchBackgroundMusic(MusicType newMusicType);

bool isTrap(int id);

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
    // if (loadedLevels[level - 1] == 1)
    // {
    //     return;
    // }

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
    fscanf(level_metadata_file, "%d", &layerCount[level - 1]);
    fclose(level_metadata_file);

    for (int layer = 0; layer < layerCount[level - 1]; layer++)
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
                    else if (isTrap(id))
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

    // Load star image
    iLoadImage(&yellowStarImage, "assets/icons/star_yellow.png");
    iLoadImage(&whiteStarImage, "assets/icons/star_white.png");

    // Load audio off/on images
    iLoadImage(&audioOffImage, "assets/icons/audio_off.png");
    iLoadImage(&audioOnImage, "assets/icons/audio_on.png");

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

    playBackgroundMusic(MENU_MUSIC);
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

    initializeGridArrays(doesCollideArray, 0);
    initializeGridArrays(coinArray, 0);
    initializeGridArrays(diamondArray, 0);
    initializeGridArrays(lifeArray, 0);
    initializeGridArrays(trapArray, 0);
}

int collectableCount(int collectableArray[ROWS][COLUMNS])
{
    int count = 0;
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLUMNS; j++)
        {
            if (collectableArray[i][j])
            {
                count++;
            }
        }
    }
    return count;
}

void changeLevel(int level)
{
    resetGame();
    currentPage = GAME_PAGE;
    currentLevel = level;
    loadLevel(level);

    switchBackgroundMusic(GAME_MUSIC);
}

struct Button buttons[50] = {
    {WIDTH - 400, 620, 380, 80, {0, 0, 0}, {200, 200, 200}, "RESUME", 40, 2, 0, MENU_PAGE, []()
     {
         currentPage = GAME_PAGE;
         switchBackgroundMusic(GAME_MUSIC);
     },
     false},
    {WIDTH - 400, 520, 380, 80, {0, 0, 0}, {200, 200, 200}, "LEVELS", 40, 10, 0, MENU_PAGE, []()
     {
         currentPage = LEVELS_PAGE;
     },
     false},
    {WIDTH - 400, 420, 380, 80, {0, 0, 0}, {200, 200, 200}, "HIGH SCORES", 40, 24, 0, MENU_PAGE, []()
     {
         currentPage = HIGH_SCORES_PAGE;
     },
     false},
    {WIDTH - 400, 320, 380, 80, {0, 0, 0}, {200, 200, 200}, "OPTIONS", 40, 8, 0, MENU_PAGE, []()
     {
         currentPage = OPTIONS_PAGE;
     },
     false},
    {WIDTH - 400, 220, 380, 80, {0, 0, 0}, {200, 200, 200}, "HELP", 40, 8, 0, MENU_PAGE, []()
     {
         currentPage = HELP_PAGE;
     },
     false},
    {WIDTH - 400, 120, 380, 80, {0, 0, 0}, {200, 200, 200}, "CREDITS", 40, 8, 0, MENU_PAGE, []()
     {
         currentPage = CREDITS_PAGE;
     },
     false},
    {WIDTH - 400, 20, 380, 80, {0, 0, 0}, {200, 200, 200}, "EXIT", 40, 6, 0, MENU_PAGE, []()
     { iCloseWindow(); },
     false},

    {WIDTH / 2 - 180, 500, 380, 80, {0, 0, 0}, {200, 200, 200}, "LEVEL 1", 40, 8, 0, LEVELS_PAGE, []()
     { changeLevel(1); },
     false},
    {WIDTH / 2 - 180, 400, 380, 80, {0, 0, 0}, {200, 200, 200}, "LEVEL 2", 40, 8, 0, LEVELS_PAGE, []()
     { changeLevel(2); },
     false},
    {WIDTH / 2 - 180, 300, 380, 80, {0, 0, 0}, {200, 200, 200}, "LEVEL 3", 40, 8, 0, LEVELS_PAGE, []()
     { changeLevel(3); },
     false},
    {WIDTH / 2 - 180, 200, 380, 80, {0, 0, 0}, {200, 200, 200}, "LEVEL 4", 40, 8, 0, LEVELS_PAGE, []()
     { changeLevel(4); },
     false},
    {WIDTH / 2 - 180, 100, 380, 80, {0, 0, 0}, {200, 200, 200}, "LEVEL 5", 40, 8, 0, LEVELS_PAGE, []()
     { changeLevel(5); },
     false},

    {180, 120, 380, 100, {0, 0, 0}, {200, 200, 200}, "MAIN MENU", 40, 6, 0, GAME_OVER_PAGE, []()
     {
         currentPage = MENU_PAGE;
         switchBackgroundMusic(MENU_MUSIC);
     },
     false},
    {WIDTH / 2 + 80, 120, 380, 100, {0, 0, 0}, {200, 200, 200}, "TRY AGAIN", 40, 16, 0, GAME_OVER_PAGE, []()
     {
         currentPage = GAME_PAGE;
         changeLevel(currentLevel);
         switchBackgroundMusic(GAME_MUSIC);
     },
     false},

    // WIN_PAGE
    {80, 80, 320, 100, {0, 0, 0}, {200, 200, 200}, "MAIN MENU", 40, 6, 0, WIN_PAGE, []()
     {
         currentPage = MENU_PAGE;
         switchBackgroundMusic(MENU_MUSIC);
     },
     false},
    {WIDTH / 2 - 158, 80, 320, 100, {0, 0, 0}, {200, 200, 200}, "PLAY AGAIN", 40, 16, 0, WIN_PAGE, []()
     {
         currentPage = GAME_PAGE;
         changeLevel(currentLevel);
         switchBackgroundMusic(GAME_MUSIC);
     },
     false},
    {WIDTH / 2 + 240, 80, 320, 100, {0, 0, 0}, {200, 200, 200}, "", 40, 16, 0, WIN_PAGE, []() {}, false}, // Placeholder for the third button

    // HELP_PAGE
    {WIDTH - 400, 20, 380, 80, {0, 0, 0}, {200, 200, 200}, "BACK", 40, 6, 0, LEVELS_PAGE, []()
     {
         currentPage = MENU_PAGE;
     },
     false},
}; // TODO: Add extra dimension for pages?

struct Icon icons[2] = {
    // Music on/off
    {WIDTH / 2 - 100, HEIGHT / 2, 60, 60, {0, 0, 0}, NULL, 0, -3, OPTIONS_PAGE, []()
     {
         isMusicOn = !isMusicOn;
         if (isMusicOn)
         {
             playBackgroundMusic(MENU_MUSIC);
         }
         else
         {
             stopBackgroundMusic();
         }
     },
     false},
    // Sound on/off
    {WIDTH / 2 - 100, HEIGHT / 2 - 100, 60, 60, {0, 0, 0}, NULL, 0, -3, OPTIONS_PAGE, []()
     {
         isSoundOn = !isSoundOn;
     },
     false},
};

// * Background music management functions
void playBackgroundMusic(MusicType musicType)
{
    // Stop any currently playing background music
    if (isBackgroundMusicPlaying)
    {
        iStopSound(backgroundMusicChannel);
        isBackgroundMusicPlaying = false;
    }

    if (isMusicOn)
    {
        const char *musicFile;
        if (musicType == MENU_MUSIC) // Menu music
        {
            musicFile = "assets/sounds/menu_bg.wav";
            currentMusicType = MENU_MUSIC;
        }
        else if (musicType == GAME_MUSIC) // Game music
        {
            musicFile = "assets/sounds/game_bg.wav";
            currentMusicType = GAME_MUSIC;
        }
        else
        {
            return; // Invalid music type
        }

        backgroundMusicChannel = iPlaySound(musicFile, true, musicType == MENU_MUSIC ? 50 : 25); // Game music volume is lower than the menu music volume.
        isBackgroundMusicPlaying = true;
    }
}

void stopBackgroundMusic()
{
    if (isBackgroundMusicPlaying)
    {
        iStopSound(backgroundMusicChannel);
        isBackgroundMusicPlaying = false;
        currentMusicType = NONE;
    }
}

void switchBackgroundMusic(MusicType newMusicType)
{
    if (currentMusicType != newMusicType)
    {
        playBackgroundMusic(newMusicType);
    }
}

// * Game logic functions
bool isTrap(int id)
{
    for (int i = 0; i < sizeof(trapIds) / sizeof(trapIds[0]); i++)
    {
        if (id == trapIds[i])
            return true;
    }
    return false;
}

// TODO: Invert velocity for bouncing?
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
        if (lifeCount == 3 && collectedCoinCount == collectableCount(coinArray) && collectedDiamondCount == collectableCount(diamondArray))
        {
            // If the player has 3 lives, and collected all coins and diamonds.
            starCount = 3;
        }
        else if (lifeCount == 3 || (collectedCoinCount == collectableCount(coinArray) && collectedDiamondCount == collectableCount(diamondArray)))
        {
            // If the player has 3 lives, xor collected all coins and diamonds.
            starCount = 2;
        }
        else
        {
            // If the player has less than 3 lives, and not collected all coins and diamonds.
            starCount = 1;
        }

        char highScoreFilePath[100];
        sprintf(highScoreFilePath, "levels/level%d/high_score.txt", currentLevel);
        FILE *highScoreFile = fopen(highScoreFilePath, "r");
        if (highScoreFile == NULL)
        {
            highScoreFile = fopen(highScoreFilePath, "w");
            fprintf(highScoreFile, "%d %d", starCount, score);
            fclose(highScoreFile);
        }
        else
        {
            int highScoreStars, highScoreScore;
            fscanf(highScoreFile, "%d %d", &highScoreStars, &highScoreScore);
            printf("High Score: %d %d\n", highScoreStars, highScoreScore);
            if (starCount > highScoreStars)
            {
                fclose(highScoreFile);
                highScoreFile = fopen(highScoreFilePath, "w");
                fprintf(highScoreFile, "%d %d", starCount, score);
            }
            else if (starCount == highScoreStars && score > highScoreScore)
            {
                fclose(highScoreFile);
                highScoreFile = fopen(highScoreFilePath, "w");
                fprintf(highScoreFile, "%d %d", highScoreStars, score);
            }
            fclose(highScoreFile);
        }

        printf("Stars: %d\n", starCount);

        sprintf(levelCompletionText, "Level %d", currentLevel);
        resetGame();

        stopBackgroundMusic();
        if (isSoundOn)
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

            if (soundPath != NULL && isSoundOn)
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

        if (lifeCount > 0 && isSoundOn)
            iPlaySound("assets/sounds/hurt.wav", 0, 50);

        if (lifeCount == 0)
        {
            resetGame();
            stopBackgroundMusic();
            if (isSoundOn)
                iPlaySound("assets/sounds/game_over.wav", 0, 80);
            currentPage = GAME_OVER_PAGE;
        }
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
    else if (currentPage == HELP_PAGE)
    {
        drawHelpPage();
    }
    else if (currentPage == CREDITS_PAGE)
    {
        drawCreditsPage();
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
    for (int layer = 0; layer < layerCount[currentLevel - 1]; layer++)
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
                        if (!checkIfAlreadyCollected(row, col, collectedLife, &collectedLifeCount))
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
        if (!button.isHovered && isSoundOn)
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

    iSetTransparentColor(button.bgColor.red, button.bgColor.green, button.bgColor.blue, alpha);
    iFilledRectangle(button.x, button.y, button.width, button.height);

    // Approximate text width and height for centering
    int textLen = strlen(button.text);
    int textWidth = (int)(textLen * button.fontSize * 0.6);
    int textHeight = button.fontSize * 0.82;
    int textX = button.x + (button.width - textWidth) / 2 + button.xOffset;
    int textY = button.y + (button.height - textHeight) / 2;

    iSetColor(button.textColor.red, button.textColor.green, button.textColor.blue);
    iShowText(textX, textY, button.text, "assets/fonts/minecraft_ten.ttf", button.fontSize);
}

void drawIcon(Icon &icon)
{
    float alpha;
    bool isHovering = mouseX >= icon.x && mouseX <= icon.x + icon.width && mouseY >= icon.y && mouseY <= icon.y + icon.height;

    if (isHovering)
    {
        alpha = 0.7; // Hover effect
        if (!icon.isHovered && isSoundOn)
        {
            iPlaySound("assets/sounds/menu_hover.wav", 0, 5);
        }
        icon.isHovered = true;
    }
    else
    {
        alpha = 0.9;
        icon.isHovered = false;
    }

    iSetTransparentColor(icon.bgColor.red, icon.bgColor.green, icon.bgColor.blue, alpha);
    iFilledRectangle(icon.x, icon.y, icon.width, icon.height);
    iShowLoadedImage(icon.x + icon.xOffset, icon.y + icon.yOffset, icon.image);
}

// * UI Widget: Page function definitions
void drawMenuPage()
{
    iClear();
    iShowLoadedImage(0, 0, &background_image);

    iSetColor(0, 0, 0);
    iShowText(40, 170, "RETRO", "assets/fonts/minecraft_ten.ttf", 167);
    iShowText(40, 40, "RACCOON", "assets/fonts/minecraft_ten.ttf", 120);

    for (int i = 0; i < 7; i++)
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

    iSetColor(0, 0, 0);
    iShowText(WIDTH / 2 - 120, HEIGHT - 100, "Levels", "assets/fonts/minecraft_ten.ttf", 80);

    for (int i = 7; i < 12; i++)
    {
        drawButton(buttons[i]);
    }

    // Back button
    buttons[17].page = LEVELS_PAGE;
    drawButton(buttons[17]);
}

void drawHighScoresPage()
{
    // TODO: Implement high scores page.
}

void drawOptionsPage()
{
    iClear();
    iShowLoadedImage(0, 0, &background_image);

    iSetColor(0, 0, 0);
    iShowText(WIDTH / 2 - 140, HEIGHT - 100, "Options", "assets/fonts/minecraft_ten.ttf", 80);

    // Music off/on
    // drawIcon(WIDTH / 2 - 100, HEIGHT / 2, isMusicOn ? &audioOnImage : &audioOffImage, false, 0, -3);
    icons[0].image = isMusicOn ? &audioOnImage : &audioOffImage;
    drawIcon(icons[0]);
    iShowText(WIDTH / 2 - 20, HEIGHT / 2 + 8, "Music", "assets/fonts/minecraft_ten.ttf", 60);

    // Sound off/on
    // drawIcon(WIDTH / 2 - 100, HEIGHT / 2 - 100, isSoundOn ? &audioOnImage : &audioOffImage, false, 0, -3);
    icons[1].image = isSoundOn ? &audioOnImage : &audioOffImage;
    drawIcon(icons[1]);
    iShowText(WIDTH / 2 - 20, HEIGHT / 2 - 92, "Sound", "assets/fonts/minecraft_ten.ttf", 60);

    // Back button
    buttons[17].page = OPTIONS_PAGE;
    drawButton(buttons[17]);
}

void drawHelpPage()
{
    iClear();
    iShowLoadedImage(0, 0, &background_image);

    iSetColor(0, 0, 0);
    iShowText(WIDTH / 2 - 100, HEIGHT - 100, "Help", "assets/fonts/minecraft_ten.ttf", 80);

    // Section 1: Controls
    iShowText(80, HEIGHT - 200, "Controls", "assets/fonts/minecraft_ten.ttf", 60);
    iShowText(80, HEIGHT - 250, "Left and Right Arrow - Move", "assets/fonts/minecraft_ten.ttf", 30);
    iShowText(80, HEIGHT - 300, "Up Arrow - Jump", "assets/fonts/minecraft_ten.ttf", 30);
    iShowText(80, HEIGHT - 350, "Esc - Pause", "assets/fonts/minecraft_ten.ttf", 30);

    // Section 2: Objectives
    iShowText(80, HEIGHT - 450, "Objective", "assets/fonts/minecraft_ten.ttf", 60);
    iShowText(80, HEIGHT - 500, "Reach the flag on the right", "assets/fonts/minecraft_ten.ttf", 30);
    iShowText(80, HEIGHT - 550, "Avoid traps", "assets/fonts/minecraft_ten.ttf", 30);
    iShowText(80, HEIGHT - 600, "Collect coins and diamonds", "assets/fonts/minecraft_ten.ttf", 30);

    // Section 3: Scoring
    iShowText(600, HEIGHT - 200, "Scoring", "assets/fonts/minecraft_ten.ttf", 60);
    iShowText(600, HEIGHT - 250, "Coin - 10 points", "assets/fonts/minecraft_ten.ttf", 30);
    iShowText(600, HEIGHT - 300, "Diamond - 50 points", "assets/fonts/minecraft_ten.ttf", 30);

    // Section 4: Stars
    iShowText(600, HEIGHT - 400, "Stars", "assets/fonts/minecraft_ten.ttf", 60);
    iShowText(600, HEIGHT - 450, "3 Stars - 3 lives + all coins & diamonds", "assets/fonts/minecraft_ten.ttf", 30);
    iShowText(600, HEIGHT - 500, "2 Stars - 3 lives or all coins & diamonds", "assets/fonts/minecraft_ten.ttf", 30);
    iShowText(600, HEIGHT - 550, "1 Star - Finished, but missed both", "assets/fonts/minecraft_ten.ttf", 30);

    // Back button
    buttons[17].page = HELP_PAGE;
    drawButton(buttons[17]);
}

void drawCreditsPage()
{
    iClear();
    iShowLoadedImage(0, 0, &background_image);

    iSetColor(0, 0, 0);
    iShowText(WIDTH / 2 - 130, HEIGHT - 100, "Credits", "assets/fonts/minecraft_ten.ttf", 80);

    // Section 1: Contributors
    iShowText(80, HEIGHT - 200, "Contributors", "assets/fonts/minecraft_ten.ttf", 60);
    iShowText(80, HEIGHT - 250, "2405102 - Arif Awasaf Wriddho", "assets/fonts/minecraft_ten.ttf", 30);
    iShowText(80, HEIGHT - 300, "2405103 - Kazi Md. Raiyan", "assets/fonts/minecraft_ten.ttf", 30);

    // Section 2: Tools
    iShowText(80, HEIGHT - 400, "Tools", "assets/fonts/minecraft_ten.ttf", 60);
    iShowText(80, HEIGHT - 450, "Modern iGraphics v0.4.0 by Mahir Labib Dihan", "assets/fonts/minecraft_ten.ttf", 30);
    iShowText(80, HEIGHT - 500, "Tiled - for level design", "assets/fonts/minecraft_ten.ttf", 30);

    // Section 3: Assets
    iShowText(80, HEIGHT - 600, "Assets", "assets/fonts/minecraft_ten.ttf", 60);
    iShowText(80, HEIGHT - 650, "Pixel Platformer by Kenney", "assets/fonts/minecraft_ten.ttf", 30);

    // Back button
    buttons[17].page = CREDITS_PAGE;
    drawButton(buttons[17]);
}

void drawGameOverPage()
{
    iClear();

    iShowLoadedImage(0, 0, &background_image);

    iSetColor(0, 0, 0);
    iShowText(WIDTH / 2 - 245, HEIGHT / 2 + 100, "Game Over", "assets/fonts/minecraft_ten.ttf", 100);
    iShowText(WIDTH / 2 - 126, HEIGHT / 2, scoreText, "assets/fonts/minecraft_ten.ttf", 60);

    // Main Menu and Try Again buttons
    drawButton(buttons[12]);
    drawButton(buttons[13]);
}

void drawWinPage()
{
    iClear();

    iShowLoadedImage(0, 0, &background_image);

    iSetColor(0, 0, 0);
    iShowText(WIDTH / 2 - 140, HEIGHT - 120, levelCompletionText, "assets/fonts/minecraft_ten.ttf", 80);
    iShowText(WIDTH / 2 - 130, HEIGHT / 2 - 70, scoreText, "assets/fonts/minecraft_ten.ttf", 60);

    // 3 stars
    iShowLoadedImage(WIDTH / 2 - 240, HEIGHT / 2 + 50, &yellowStarImage);
    iShowLoadedImage(WIDTH / 2 - 60, HEIGHT / 2 + 50, starCount > 1 ? &yellowStarImage : &whiteStarImage);
    iShowLoadedImage(WIDTH / 2 + 120, HEIGHT / 2 + 50, starCount > 2 ? &yellowStarImage : &whiteStarImage);

    // Main Menu and Play Again buttons
    drawButton(buttons[14]);
    drawButton(buttons[15]);
    // Next Level or Exit button
    if (currentLevel < LEVEL_COUNT)
    {
        buttons[16].text = "NEXT LEVEL";
        buttons[16].onClick = []()
        {
            currentPage = GAME_PAGE;
            changeLevel(currentLevel + 1);
            switchBackgroundMusic(GAME_MUSIC);
        };
        drawButton(buttons[16]);
    }
    else
    {
        buttons[16].text = "EXIT";
        buttons[16].onClick = []()
        {
            iCloseWindow();
        };
        drawButton(buttons[16]);
    }
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
        switchBackgroundMusic(MENU_MUSIC);
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
            if (isSoundOn)
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
        for (int i = 0; i < 18; i++) // TODO: Extract button count to a variable.
        {
            if (buttons[i].page == currentPage && mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].width && my >= buttons[i].y && my <= buttons[i].y + buttons[i].height)
            {
                if (isSoundOn)
                    iPlaySound("assets/sounds/menu_click.wav", 0, 30);
                if (i == 0 && !isResumable)
                    continue;
                buttons[i].onClick();
            }
        }
        for (int i = 0; i < 2; i++)
        {
            if (icons[i].page == currentPage && mx >= icons[i].x && mx <= icons[i].x + icons[i].width && my >= icons[i].y && my <= icons[i].y + icons[i].height)
            {
                icons[i].onClick();
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