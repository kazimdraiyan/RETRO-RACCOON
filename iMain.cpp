#include "iGraphics.h" // v4.0.0
#include "iFont.h"
#include "iSound.h"

// * Optimization
// TODO: Free images and sprites.
// TODO: Use the const keyword where possible.

// * Tasks
// TODO: Make the game full screen.
// TODO: Use Position and Size structs.
// TODO: Add loading screen.

// * Questions
// ? How often the iDraw() function is called? Is it constant or device dependent?

#define TITLE "RETRO RACCOON"
#define WIDTH 1280
#define HEIGHT 720
#define COLUMNS 32
#define ROWS 18
#define TILE_SIZE (WIDTH / COLUMNS)

#define LEVEL_COUNT 5 // TODO: Get the level count from the levels folder.
#define BUTTON_COUNT 19
#define ICON_COUNT 2
#define COIN_SPRITE_COUNT 2
#define FLAG_SPRITE_COUNT 2
#define PLAYER_IDLE_SPRITE_COUNT 4
#define PLAYER_JUMP_SPRITE_COUNT 5

#define MAX_LAYER_COUNT 10
#define MAX_COLLECTABLE_COUNT 30
#define MAX_PLAYER_COUNT 50
#define MAX_PLAYER_NAME_LENGTH 20
#define MAX_FILE_PATH_LENGTH 100

// Bitmasks to encode and decode tile ids.
#define FLIPPED_HORIZONTALLY_FLAG 0x80000000 // 32nd (leftmost) bit
#define FLIPPED_VERTICALLY_FLAG 0x40000000 // 31st bit
#define DOES_COLLIDE_FLAG 0x10000000 // 29th bit

#define FLAG_ID 111
#define COIN_ID 151
#define DIAMOND_ID 67
#define FULL_LIFE_ID 44
#define NO_LIFE_ID 46

#define COIN_SCORE 10
#define DIAMOND_SCORE 50
#define PLAYER_INITIAL_X 200
#define PLAYER_INITIAL_Y 300
#define X_ANIMATION_DEL_X 5 // Number of pixels to move the player in each animation frame.
#define GRAVITY 80
#define JUMP_VELOCITY 150
#define DEL_T 0.08 // Time step for calculating vertical movement.

#define FONT_PATH "assets/fonts/minecraft_ten.ttf"

const int trapIds[] = {68, 33, 34, 35, 53, 54, 55, 73, 74, 75}; // IDs of the tiles that are traps.

enum Page
{
    NAME_INPUT_PAGE,
    MENU_PAGE,
    LEVELS_PAGE,
    HIGH_SCORES_PAGE,
    OPTIONS_PAGE,
    HELP_PAGE,
    CREDITS_PAGE,
    GAME_PAGE,
    WIN_PAGE,
    GAME_OVER_PAGE,
    NONE_PAGE
};

enum MusicType
{
    MENU_MUSIC,
    GAME_MUSIC,
    NONE_MUSIC
};

enum Direction
{
    LEFT,
    RIGHT,
};

enum BackgroundImageColor
{
    BROWN,
    BLUE,
    GREEN
};

struct Color
{
    int red;
    int green;
    int blue;
    float alpha; // TODO: Is used?
};

struct TextButton
{
    int x;
    int y;
    int width;
    int height;
    Color bgColor;
    Color textColor;
    char *text;
    int fontSize;
    // Offset to make the text centered by trial and error.
    int xOffset;
    int yOffset;
    Page page; // Which page the text button is rendered on.
    void (*onClick)(void);
    bool hasBeenHovered;
};

struct IconButton
{
    int x;
    int y;
    int width;
    int height;
    Color bgColor;
    Image *image;
    // Offset to make the icon centered by trial and error.
    int xOffset;
    int yOffset;
    Page page; // Which page the icon button is rendered on.
    void (*onClick)(void);
    bool hasBeenHovered;
};

struct Player
{
    int x; // The current x-position of the player sprite.
    double y;
    double width;
    double height;
    int animateToX; // The x-position to which the player is moving. It is the target x-position. It is a multiple of TILE_SIZE.
    double velocityY;
    bool isJumping; // isJumping tells whether the player just jumped or not. It is different from isOnAir. // TODO: Handle the jump in a better way?
    bool isOnAir;
    Direction direction;
};

// * Asset management variables
Image tileImages[ROWS * COLUMNS];
Image backgroundImage;
Image yellowStarImage;
Image whiteStarImage;
Image audioOnImage;
Image audioOffImage;
Image fullLifeImage;
Image noLifeImage;
Image coinFrames[COIN_SPRITE_COUNT];
Sprite coinSprite;
Image flagFrames[FLAG_SPRITE_COUNT];
Sprite flagSprite;
Image playerIdleFrames[PLAYER_IDLE_SPRITE_COUNT];
Sprite playerIdleSprite;
Image playerJumpFrames[PLAYER_JUMP_SPRITE_COUNT];
Sprite playerJumpSprite;

// * Level management variables
int layerCount = 0;                           // How many layers are in the current level.
int tiles[MAX_LAYER_COUNT][ROWS][COLUMNS][3]; // Each cell has 3 information: tile id, is flipped horizontally, is flipped vertically.
// Grid arrays: contains extra information about each cell.
bool doesCollideArray[ROWS][COLUMNS]; // true if there is a tile in the cell that is a collider, false otherwise.
bool coinArray[ROWS][COLUMNS];
bool diamondArray[ROWS][COLUMNS];
bool lifeArray[ROWS][COLUMNS];
bool trapArray[ROWS][COLUMNS];
// Stores the row and column of the collected coins and diamonds.
int collectedCoins[MAX_COLLECTABLE_COUNT][2];
int collectedDiamonds[MAX_COLLECTABLE_COUNT][2];
int collectedLives[MAX_COLLECTABLE_COUNT][2];
int collectedCoinCount = 0;
int collectedDiamondCount = 0;
int collectedLifeCount = 0;

// * UI management variables
Page currentPage = NONE_PAGE;
int currentLevel = 1;
bool isResumable = false;
char playerNameInput[MAX_PLAYER_NAME_LENGTH + 1] = ""; // TODO: Limit to 50 characters while inputting.
char scoreText[50] = "";
// Current mouse position. Used for detecting hover state of buttons.
int mouseX = 0;
int mouseY = 0;

// * Sound management variables
int backgroundMusicChannel = -1;
bool isMusicPlaying = false;
bool isMusicOn = true;
bool isSoundOn = true;
MusicType currentMusicType = MENU_MUSIC;

// * Timer variables for animation
int gameStateUpdateTimer;
int horizontalMovementTimer;
int spriteAnimationTimer;
int jumpAnimationFrame = 0; // TODO: Is this needed?

// * Game state variables
char playerName[MAX_PLAYER_NAME_LENGTH + 1] = "";                    // Current player name.
int playerCount = 0;                                                 // Number of players in the high scores.
char playerNames[MAX_PLAYER_COUNT][MAX_PLAYER_NAME_LENGTH + 1] = {}; // Max MAX_PLAYER_COUNT players, 20 characters per player name.
int highScores[MAX_PLAYER_COUNT][LEVEL_COUNT][2] = {};               // Max MAX_PLAYER_COUNT players, LEVEL_COUNT levels, 2 scores (stars and score).
Player player;
int score = 0;
int lifeCount = 3;
int starCount = 0;

// * These functions acts as UI Widgets.
// Page rendering functions.
void drawNameInputPage();
void drawMenuPage();
void drawLevelsPage();
void drawHighScoresPage();
void drawOptionsPage();
void drawHelpPage();
void drawCreditsPage();
void drawGamePage();
void drawWinPage();
void drawGameOverPage();
// UI rendering functions.
void drawScore();
void drawLifeCount();
void drawTiles();
void drawTile(int layer, int row, int col, Sprite *sprite = NULL);
void drawTextButton(TextButton &button);
void drawIconButton(IconButton &icon);

// * Background music management functions
void playBackgroundMusic(MusicType musicType);
void stopBackgroundMusic();
void switchBackgroundMusic(MusicType newMusicType);

// * Helper functions
void toLowerString(char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        str[i] = tolower(str[i]);
    }
}

bool isStringWhitespace(char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        if (!isspace(str[i]))
            return false;
    }
    return true;
}

bool isNewPlayer()
{
    for (int i = 0; i < playerCount; i++)
    {
        if (strcmp(playerNames[i], playerName) == 0)
            return false;
    }
    return true;
}

bool isTrap(int id)
{
    for (int i = 0; i < sizeof(trapIds) / sizeof(trapIds[0]); i++)
    {
        if (id == trapIds[i])
            return true;
    }
    return false;
}

bool isAlreadyCollected(int row, int col, int collectedCollectableArray[][2], int *collectedCollectableCount)
{
    for (int i = 0; i < *collectedCollectableCount; i++)
    {
        if (collectedCollectableArray[i][0] == row && collectedCollectableArray[i][1] == col)
            return true;
    }
    return false;
}

void mirrorTile(int tileId, MirrorState mirrorState, Sprite *sprite = NULL)
{
    if (sprite == NULL)
        iMirrorImage(&tileImages[tileId], mirrorState);
    else
        iMirrorSprite(sprite, mirrorState);
}

// Checks the number of a collectable type in the grid.
int collectableCount(bool collectableArray[ROWS][COLUMNS])
{
    int count = 0;
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLUMNS; j++)
        {
            if (collectableArray[i][j])
                count++;
        }
    }
    return count;
}

// * Initialization functions
void initializeGridArray(bool array[ROWS][COLUMNS], bool value)
{
    for (int row = 0; row < ROWS; row++)
    {
        for (int col = 0; col < COLUMNS; col++)
        {
            array[row][col] = value;
        }
    }
}

void initializeCollectedCollectables(int collectedCollectableArray[MAX_COLLECTABLE_COUNT][2])
{
    for (int i = 0; i < MAX_COLLECTABLE_COUNT; i++)
    {
        collectedCollectableArray[i][0] = -1;
        collectedCollectableArray[i][1] = -1;
    }
}

void initializeHighScores()
{
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
    {
        for (int j = 0; j < LEVEL_COUNT; j++)
        {
            highScores[i][j][0] = 0;
            highScores[i][j][1] = 0;
        }
    }
}

void initializePlayer()
{
    player.x = PLAYER_INITIAL_X;
    player.y = PLAYER_INITIAL_Y;
    player.width = TILE_SIZE;
    player.height = TILE_SIZE;
    player.animateToX = PLAYER_INITIAL_X;
    player.velocityY = 0;
    player.isJumping = false;
    player.isOnAir = false;
    if (player.direction == LEFT)
    {
        player.direction = RIGHT;
        iMirrorSprite(&playerIdleSprite, HORIZONTAL);
        iMirrorSprite(&playerJumpSprite, HORIZONTAL);
    }
    jumpAnimationFrame = 0;
}

// * Loading functions
void loadAssets()
{
    // * iResizeImage is not used, as using it makes the tiles blurry for some reason. Instead, the image assests are pre-resized.

    // Load tiles
    for (int i = 0; i < 180; i++)
    {
        // TODO: Optimize this by not loading the tiles that are not used in any level.
        char filePath[MAX_FILE_PATH_LENGTH];
        sprintf(filePath, "assets/tiles/%d.png", i);
        iLoadImage(&tileImages[i], filePath);
    }

    // Load star image
    iLoadImage(&yellowStarImage, "assets/icons/star_yellow.png");
    iLoadImage(&whiteStarImage, "assets/icons/star_white.png");

    // Load audio on/off images
    iLoadImage(&audioOnImage, "assets/icons/audio_on.png");
    iLoadImage(&audioOffImage, "assets/icons/audio_off.png");

    // Load life images
    iLoadImage(&fullLifeImage, "assets/special_tiles/full_life.png");
    iLoadImage(&noLifeImage, "assets/special_tiles/no_life.png");

    // Load coin sprite
    iLoadFramesFromFolder(coinFrames, "assets/sprites/coin/");
    iInitSprite(&coinSprite);
    iChangeSpriteFrames(&coinSprite, coinFrames, COIN_SPRITE_COUNT);
    iResizeSprite(&coinSprite, TILE_SIZE, TILE_SIZE);

    // Load flag sprite
    iLoadFramesFromFolder(flagFrames, "assets/sprites/flag/");
    iInitSprite(&flagSprite);
    iChangeSpriteFrames(&flagSprite, flagFrames, FLAG_SPRITE_COUNT);
    iResizeSprite(&flagSprite, TILE_SIZE, TILE_SIZE);

    // Load player idle sprite
    iLoadFramesFromFolder(playerIdleFrames, "assets/sprites/player/idle/");
    iInitSprite(&playerIdleSprite);
    iChangeSpriteFrames(&playerIdleSprite, playerIdleFrames, PLAYER_IDLE_SPRITE_COUNT);
    iResizeSprite(&playerIdleSprite, TILE_SIZE, TILE_SIZE);

    // Load player jump sprite
    iLoadFramesFromFolder(playerJumpFrames, "assets/sprites/player/jump/");
    iInitSprite(&playerJumpSprite);
    iChangeSpriteFrames(&playerJumpSprite, playerJumpFrames, PLAYER_JUMP_SPRITE_COUNT);
    iResizeSprite(&playerJumpSprite, TILE_SIZE, TILE_SIZE);
}

void loadLevel(int level)
{
    // Grid arrays should be initialized every time a level is loaded.
    initializeGridArray(doesCollideArray, false);
    initializeGridArray(coinArray, false);
    initializeGridArray(diamondArray, false);
    initializeGridArray(lifeArray, false);
    initializeGridArray(trapArray, false);

    char levelMetadataFilePath[MAX_FILE_PATH_LENGTH];
    sprintf(levelMetadataFilePath, "levels/level%d/metadata.txt", level);
    FILE *levelMetadataFile = fopen(levelMetadataFilePath, "r");
    if (levelMetadataFile == NULL)
    {
        printf("levels/level%d/metadata.txt file not found\n", level);
        return;
    }
    char levelBackgroundFileName[50];
    fscanf(levelMetadataFile, "%d %s", &layerCount, levelBackgroundFileName);
    char levelBackgroundFilePath[MAX_FILE_PATH_LENGTH];
    sprintf(levelBackgroundFilePath, "assets/backgrounds/%s", levelBackgroundFileName);
    fclose(levelMetadataFile);

    // TODO: Optimize this by not loading the background if it was loaded once.
    iLoadImage(&backgroundImage, levelBackgroundFilePath);

    for (int layer = 0; layer < layerCount; layer++)
    {
        char layerFilePath[MAX_FILE_PATH_LENGTH];
        sprintf(layerFilePath, "levels/level%d/layer_%d_customized.csv", level, layer);
        FILE *layerFile = fopen(layerFilePath, "r");
        if (layerFile == NULL)
        {
            printf("levels/level%d/layer_%d_customized.csv file not found\n", level, layer);
            return;
        }

        char line[500];
        int row = 0;
        while (fgets(line, 500, layerFile))
        {
            char *cell;
            cell = strtok(line, ",\n");
            int col = 0;
            while (cell)
            {
                int encodedId = atoi(cell);
                if (encodedId == -1)
                {
                    tiles[layer][row][col][0] = -1;
                    tiles[layer][row][col][1] = false;
                    tiles[layer][row][col][2] = false;
                }
                else
                {
                    bool isFlippedHorizontally = (encodedId & FLIPPED_HORIZONTALLY_FLAG) != 0;
                    bool isFlippedVertically = (encodedId & FLIPPED_VERTICALLY_FLAG) != 0;
                    bool doesCollide = (encodedId & DOES_COLLIDE_FLAG) != 0;

                    // Mask out the flags to get the decoded ID.
                    int id = encodedId & ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | DOES_COLLIDE_FLAG);
                    tiles[layer][row][col][0] = id;
                    tiles[layer][row][col][1] = isFlippedHorizontally;
                    tiles[layer][row][col][2] = isFlippedVertically;
                    if (doesCollide)
                        doesCollideArray[row][col] = true;

                    if (id == COIN_ID)
                        coinArray[row][col] = true;
                    else if (id == DIAMOND_ID)
                        diamondArray[row][col] = true;
                    else if (isTrap(id))
                        trapArray[row][col] = true;
                    else if (id == FULL_LIFE_ID)
                        lifeArray[row][col] = true;
                }
                cell = strtok(NULL, ",\n"); // Get the next cell.
                col++;
            }
            row++;
        }
        fclose(layerFile);
    }
}

void loadPlayerName()
{
    FILE *playerNameFile = fopen("saves/current_player.txt", "r");
    if (playerNameFile == NULL)
    {
        printf("saves/current_player.txt file not found\n");
        return;
    }

    fgets(playerName, MAX_PLAYER_NAME_LENGTH + 1, playerNameFile);
    playerName[strcspn(playerName, "\n")] = '\0'; // Remove the newline character.
    toLowerString(playerName);

    fclose(playerNameFile);
}

void loadOptions()
{
    FILE *optionsFile = fopen("saves/options.txt", "r");
    if (optionsFile == NULL)
    {
        printf("saves/options.txt file not found\n");
        return;
    }

    fscanf(optionsFile, "%d %d", &isMusicOn, &isSoundOn);
    fclose(optionsFile);
}

void loadHighScores()
{
    FILE *highScoresFile = fopen("saves/high_scores.txt", "r");
    if (highScoresFile == NULL)
    {
        printf("saves/high_scores.txt file not found\n");
        return;
    }

    fscanf(highScoresFile, "%d", &playerCount);
    for (int i = 0; i < playerCount; i++)
    {
        fgetc(highScoresFile); // Skip the newline character.
        fgets(playerNames[i], MAX_PLAYER_NAME_LENGTH + 1, highScoresFile);
        playerNames[i][strcspn(playerNames[i], "\n")] = '\0'; // Remove the newline character.
        toLowerString(playerNames[i]);
        for (int j = 0; j < LEVEL_COUNT; j++)
        {
            fscanf(highScoresFile, "%d,%d", &highScores[i][j][0], &highScores[i][j][1]);
        }
    }
    fclose(highScoresFile);
}

// * Saving functions
void saveHighScores()
{
    FILE *highScoresFile = fopen("saves/high_scores.txt", "w");
    if (highScoresFile == NULL)
    {
        printf("Error creating file: saves/high_scores.txt\n");
        return;
    }
    fprintf(highScoresFile, "%d\n", playerCount);
    for (int i = 0; i < playerCount; i++)
    {
        toLowerString(playerNames[i]);
        fprintf(highScoresFile, "%s\n", playerNames[i]);
        for (int j = 0; j < LEVEL_COUNT; j++)
        {
            fprintf(highScoresFile, "%d,%d\n", highScores[i][j][0], highScores[i][j][1]);
        }
    }
    fclose(highScoresFile);
}

void saveOptions()
{
    FILE *optionsFile = fopen("saves/options.txt", "w");
    if (optionsFile == NULL)
    {
        printf("Error creating file: saves/options.txt\n");
        return;
    }
    fprintf(optionsFile, "%d %d", isMusicOn, isSoundOn);
    fclose(optionsFile);
}

// * Background music management functions
void stopBackgroundMusic()
{
    if (isMusicPlaying)
    {
        iStopSound(backgroundMusicChannel);
        currentMusicType = NONE_MUSIC;
        isMusicPlaying = false;
    }
}

void playBackgroundMusic(MusicType musicType)
{
    stopBackgroundMusic();

    if (isMusicOn)
    {
        const char *musicFile;
        if (musicType == MENU_MUSIC)
        {
            backgroundMusicChannel = iPlaySound("assets/sounds/menu_bg.wav", true, 50);
            currentMusicType = MENU_MUSIC;
        }
        else if (musicType == GAME_MUSIC)
        {
            // Game music volume is lower than the menu music volume.
            backgroundMusicChannel = iPlaySound("assets/sounds/game_bg.wav", true, 30);
            currentMusicType = GAME_MUSIC;
        }
        isMusicPlaying = true;
    }
}

void switchBackgroundMusic(MusicType newMusicType)
{
    if (newMusicType != currentMusicType)
        playBackgroundMusic(newMusicType);
}

// * Game management functions
void pauseGame()
{
    currentPage = MENU_PAGE;
    switchBackgroundMusic(MENU_MUSIC);

    iPauseTimer(gameStateUpdateTimer);
    iPauseTimer(horizontalMovementTimer);
    iPauseTimer(spriteAnimationTimer);
}

void resumeGame()
{
    currentPage = GAME_PAGE;
    switchBackgroundMusic(GAME_MUSIC);

    iResumeTimer(gameStateUpdateTimer);
    iResumeTimer(horizontalMovementTimer);
    iResumeTimer(spriteAnimationTimer);
}

// Called when playing a new game instead of resuming.
void resetGame()
{
    isResumable = false;

    iPauseTimer(gameStateUpdateTimer);
    iPauseTimer(horizontalMovementTimer);
    iPauseTimer(spriteAnimationTimer);

    initializePlayer();

    // Collected collectables should be initialized every time the game is reset.
    initializeCollectedCollectables(collectedCoins);
    initializeCollectedCollectables(collectedDiamonds);
    initializeCollectedCollectables(collectedLives);

    // Reset the collected collectables.
    collectedCoinCount = 0;
    collectedDiamondCount = 0;
    collectedLifeCount = 0;

    score = 0;
    lifeCount = 3;
}

void changeLevel(int level)
{
    resetGame();

    loadLevel(level);
    currentLevel = level;

    resumeGame();
    isResumable = true;
}

void checkAndUpdateHighScores()
{
    for (int i = 0; i < playerCount; i++)
    {
        if (strcmp(playerNames[i], playerName) == 0)
        {
            if (starCount > highScores[i][currentLevel - 1][0])
            {
                highScores[i][currentLevel - 1][0] = starCount;
                highScores[i][currentLevel - 1][1] = score;
                saveHighScores();
            }
            else if (starCount == highScores[i][currentLevel - 1][0] && score > highScores[i][currentLevel - 1][1])
            {
                highScores[i][currentLevel - 1][1] = score;
                saveHighScores();
            }
            break;
        }
    }
}

// * Buttons
// TODO: Make the array 2D, where rows will represent pages and columns will represent buttons in the respective page.
struct TextButton buttons[] = {
    // * MENU_PAGE (0 - 6)
    {WIDTH - 400, 620, 380, 80, {0, 0, 0}, {200, 200, 200}, "RESUME", 40, 2, 0, MENU_PAGE, resumeGame, false},
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
    {WIDTH - 400, 20, 380, 80, {0, 0, 0}, {200, 200, 200}, "EXIT", 40, 6, 0, MENU_PAGE, iCloseWindow, false},

    // * LEVELS_PAGE (7 - 11)
    // TODO: Add a function to initialize the level buttons.
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

    // * GAME_OVER_PAGE (12 - 13)
    {180, 120, 380, 100, {0, 0, 0}, {200, 200, 200}, "MAIN MENU", 40, 6, 0, GAME_OVER_PAGE, []()
     {
         currentPage = MENU_PAGE;
         switchBackgroundMusic(MENU_MUSIC);
     },
     false},
    {WIDTH / 2 + 80, 120, 380, 100, {0, 0, 0}, {200, 200, 200}, "TRY AGAIN", 40, 16, 0, GAME_OVER_PAGE, []()
     {
         changeLevel(currentLevel);
     },
     false},

    // * WIN_PAGE (14 - 16)
    {80, 80, 320, 100, {0, 0, 0}, {200, 200, 200}, "MAIN MENU", 40, 6, 0, WIN_PAGE, []()
     {
         currentPage = MENU_PAGE;
         switchBackgroundMusic(MENU_MUSIC);
     },
     false},
    {WIDTH / 2 - 158, 80, 320, 100, {0, 0, 0}, {200, 200, 200}, "PLAY AGAIN", 40, 16, 0, WIN_PAGE, []()
     {
         changeLevel(currentLevel);
     },
     false},
    {WIDTH / 2 + 240, 80, 320, 100, {0, 0, 0}, {200, 200, 200}, "", 40, 16, 0, WIN_PAGE, []() {}, false}, // Placeholder for the third button. Can be "Next Level" or "Exit".

    // * OPTIONS_PAGE (17)
    {40, 20, 380, 80, {0, 0, 0}, {200, 200, 200}, "Edit Name", 40, 6, 0, OPTIONS_PAGE, []()
     {
         strcpy(playerNameInput, playerName);
         currentPage = NAME_INPUT_PAGE;
     },
     false},

    // * Back Button (18)
    {WIDTH - 400, 20, 380, 80, {0, 0, 0}, {200, 200, 200}, "BACK", 40, 6, 0, NONE_PAGE, []()
     {
         currentPage = MENU_PAGE;
         switchBackgroundMusic(MENU_MUSIC);
     },
     false}, // Placeholder for the back button. Can be on any page.

};

struct IconButton icons[2] = {
    // * OPTIONS_PAGE (0 - 1)
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
         saveOptions();
     },
     false},
    // Sound on/off
    {WIDTH / 2 - 100, HEIGHT / 2 - 100, 60, 60, {0, 0, 0}, NULL, 0, -3, OPTIONS_PAGE, []()
     {
         isSoundOn = !isSoundOn;
         saveOptions();
     },
     false},
};

// * Animation functions
void animateHorizontalMovement()
{
    if (player.x < player.animateToX)
        player.x += X_ANIMATION_DEL_X;
    else if (player.x > player.animateToX)
        player.x -= X_ANIMATION_DEL_X;
    else
        iPauseTimer(horizontalMovementTimer);
}

void animateSprites()
{
    iAnimateSprite(&coinSprite);
    iAnimateSprite(&flagSprite);

    if (player.isJumping)
    {
        iAnimateSprite(&playerJumpSprite);
        jumpAnimationFrame++;
        if (jumpAnimationFrame >= PLAYER_JUMP_SPRITE_COUNT)
        {
            player.isJumping = false;
            jumpAnimationFrame = 0;
        }
    }
    else
        iAnimateSprite(&playerIdleSprite);
}

// * Game logic functions
void jump()
{
    if (!player.isOnAir)
    {
        if (isSoundOn)
            iPlaySound("assets/sounds/jump.wav", 0, 35);
        player.velocityY = JUMP_VELOCITY;
        player.isJumping = true;
        jumpAnimationFrame = 0;
        player.isOnAir = true;
    }
}

void moveVerticallyTillCollision(double delY)
{
    if (player.y + delY < 0) // Collision with the bottom of the screen.
    {
        player.y = 0;
        player.velocityY = 0;
        player.isOnAir = false;
    }
    else if (player.y + player.height + delY > HEIGHT) // Collision with the top of the screen.
    {
        player.y = HEIGHT - player.height;
        player.velocityY = 0;
    }
    else
    {
        int playerRow = (int)((player.y + delY) / TILE_SIZE);
        int playerCol = (int)(player.animateToX / TILE_SIZE);
        if (doesCollideArray[ROWS - playerRow - 1][playerCol]) // Collision with the tile below the player.
        {
            player.y = (playerRow + 1) * TILE_SIZE;
            player.velocityY = 0;
            player.isOnAir = false;
        }
        else if (doesCollideArray[ROWS - playerRow - 2][playerCol]) // Collision with the tile above the player.
        {
            player.y = playerRow * TILE_SIZE;
            player.velocityY = 0;
        }
        else
        {
            player.y += delY;
        }
    }
}

void checkCollisionWithTraps()
{
    int row = ROWS - (int)(player.y / TILE_SIZE) - 1;
    int col = (int)(player.animateToX / TILE_SIZE);

    if (trapArray[row][col] && player.x == player.animateToX)
    {
        initializePlayer();
        lifeCount--;

        if (lifeCount > 0 && isSoundOn)
            iPlaySound("assets/sounds/hurt.wav", 0, 50);
        // * Game over condition
        else if (lifeCount == 0)
        {
            resetGame();
            currentPage = GAME_OVER_PAGE;

            stopBackgroundMusic();
            if (isSoundOn)
                iPlaySound("assets/sounds/game_over.wav", 0, 80);
        }
    }
}

void checkAndCollect(bool collectableArray[ROWS][COLUMNS], int collectableId, int collectableScore, int collectedCollectableArray[][2], int *collectedCollectableCount, int isLife = 0, const char *soundPath = NULL, int volume = 100)
{
    int row = ROWS - (int)(player.y / TILE_SIZE) - 1;
    int col = (int)(player.animateToX / TILE_SIZE);

    if (collectableArray[row][col]) // Collision with collectables tested rigorously.
    {
        if (!isAlreadyCollected(row, col, collectedCollectableArray, collectedCollectableCount))
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

void checkCollisionWithAllCollectables()
{
    checkAndCollect(coinArray, COIN_ID, COIN_SCORE, collectedCoins, &collectedCoinCount, 0, "assets/sounds/coin.wav", 70);
    checkAndCollect(diamondArray, DIAMOND_ID, DIAMOND_SCORE, collectedDiamonds, &collectedDiamondCount, 0, "assets/sounds/diamond.wav", 100);
    checkAndCollect(lifeArray, FULL_LIFE_ID, 0, collectedLives, &collectedLifeCount, 1, "assets/sounds/life.wav", 70);
}

void gameStateUpdate()
{
    player.velocityY -= GRAVITY * DEL_T;
    double delY = player.velocityY * DEL_T;

    moveVerticallyTillCollision(delY);
    checkCollisionWithTraps();
    checkCollisionWithAllCollectables();

    // * Win condition
    if (player.x + player.width > WIDTH)
    {
        if (lifeCount == 3 && collectedCoinCount == collectableCount(coinArray) && collectedDiamondCount == collectableCount(diamondArray))
            starCount = 3;
        else if (lifeCount == 3 || (collectedCoinCount == collectableCount(coinArray) && collectedDiamondCount == collectableCount(diamondArray)))
            starCount = 2;
        else
            starCount = 1;

        checkAndUpdateHighScores();

        resetGame();
        currentPage = WIN_PAGE;

        stopBackgroundMusic();
        if (isSoundOn)
            iPlaySound("assets/sounds/level_complete.wav", 0, 80);
    }
}

void iDraw()
{
    switch (currentPage)
    {
    case NAME_INPUT_PAGE:
        drawNameInputPage();
        break;
    case MENU_PAGE:
        drawMenuPage();
        break;
    case LEVELS_PAGE:
        drawLevelsPage();
        break;
    case HIGH_SCORES_PAGE:
        drawHighScoresPage();
        break;
    case OPTIONS_PAGE:
        drawOptionsPage();
        break;
    case HELP_PAGE:
        drawHelpPage();
        break;
    case CREDITS_PAGE:
        drawCreditsPage();
        break;
    case GAME_PAGE:
        drawGamePage();
        break;
    case WIN_PAGE:
        drawWinPage();
        break;
    case GAME_OVER_PAGE:
        drawGameOverPage();
        break;
    }
}

// * UI Widget: Small widget function definitions
void drawScore()
{
    if (currentPage == GAME_PAGE)
        sprintf(scoreText, "Score: %d", score);

    iSetColor(0, 0, 0);
    iShowText(30, HEIGHT - 60, scoreText, FONT_PATH, 48);
}

void drawLifeCount()
{
    iShowLoadedImage(WIDTH - 190, HEIGHT - 72, lifeCount > 2 ? &fullLifeImage : &noLifeImage); // Leftmost life
    iShowLoadedImage(WIDTH - 135, HEIGHT - 72, lifeCount > 1 ? &fullLifeImage : &noLifeImage); // Middle life
    iShowLoadedImage(WIDTH - 80, HEIGHT - 72, lifeCount > 0 ? &fullLifeImage : &noLifeImage);  // Rightmost life
}

void drawTiles()
{
    for (int layer = 0; layer < layerCount; layer++)
    {
        for (int row = 0; row < ROWS; row++)
        {
            for (int col = 0; col < COLUMNS; col++)
            {
                switch (tiles[layer][row][col][0])
                {
                case -1: // Empty tile
                    break;
                case FLAG_ID:
                    drawTile(layer, row, col, &flagSprite);
                    break;
                case COIN_ID:
                    if (!isAlreadyCollected(row, col, collectedCoins, &collectedCoinCount))
                        drawTile(layer, row, col, &coinSprite);
                    break;
                case DIAMOND_ID:
                    if (!isAlreadyCollected(row, col, collectedDiamonds, &collectedDiamondCount))
                        drawTile(layer, row, col);
                    break;
                case FULL_LIFE_ID:
                    if (!isAlreadyCollected(row, col, collectedLives, &collectedLifeCount))
                        drawTile(layer, row, col);
                    break;
                default:
                    drawTile(layer, row, col);
                    break;
                }
            }
        }
    }
}

void drawTile(int layer, int row, int col, Sprite *sprite)
{
    int tileId = tiles[layer][row][col][0];
    bool isFlippedHorizontally = tiles[layer][row][col][1];
    bool isFlippedVertically = tiles[layer][row][col][2];
    int x = col * TILE_SIZE;
    int y = (ROWS - row - 1) * TILE_SIZE;

    if (isFlippedHorizontally)
        mirrorTile(tileId, HORIZONTAL, sprite);
    if (isFlippedVertically)
        mirrorTile(tileId, VERTICAL, sprite);

    if (sprite == NULL)
        iShowLoadedImage(x, y, &tileImages[tileId]);
    else
    {
        iSetSpritePosition(sprite, x, y);
        iShowSprite(sprite);
    }

    // Mirror the tile back to its original state.
    if (isFlippedHorizontally)
        mirrorTile(tileId, HORIZONTAL, sprite);
    if (isFlippedVertically)
        mirrorTile(tileId, VERTICAL, sprite);
}

void drawTextButton(TextButton &button)
{
    float alpha;
    bool isBeingHovered = mouseX >= button.x && mouseX <= button.x + button.width && mouseY >= button.y && mouseY <= button.y + button.height;

    if (isBeingHovered)
    {
        alpha = 0.7; // Hover effect
        if (!button.hasBeenHovered && isSoundOn)
            iPlaySound("assets/sounds/menu_hover.wav", 0, 5);
        button.hasBeenHovered = true;
    }
    else
    {
        alpha = 0.9;
        button.hasBeenHovered = false;
    }

    iSetTransparentColor(button.bgColor.red, button.bgColor.green, button.bgColor.blue, alpha);
    iFilledRectangle(button.x, button.y, button.width, button.height);

    // Approximate text width and height for centering
    int textLen = strlen(button.text);
    int textWidth = (int)(textLen * button.fontSize * 0.6);
    int textHeight = button.fontSize * 0.82;
    int textX = button.x + (button.width - textWidth) / 2 + button.xOffset;
    int textY = button.y + (button.height - textHeight) / 2 + button.yOffset;

    iSetColor(button.textColor.red, button.textColor.green, button.textColor.blue);
    iShowText(textX, textY, button.text, FONT_PATH, button.fontSize);
}

void drawIconButton(IconButton &icon)
{
    float alpha;
    bool isBeingHovered = mouseX >= icon.x && mouseX <= icon.x + icon.width && mouseY >= icon.y && mouseY <= icon.y + icon.height;

    if (isBeingHovered)
    {
        alpha = 0.7; // Hover effect
        if (!icon.hasBeenHovered && isSoundOn)
            iPlaySound("assets/sounds/menu_hover.wav", 0, 5);
        icon.hasBeenHovered = true;
    }
    else
    {
        alpha = 0.9;
        icon.hasBeenHovered = false;
    }

    iSetTransparentColor(icon.bgColor.red, icon.bgColor.green, icon.bgColor.blue, alpha);
    iFilledRectangle(icon.x, icon.y, icon.width, icon.height);
    iShowLoadedImage(icon.x + icon.xOffset, icon.y + icon.yOffset, icon.image);
}

void drawNameInputPage()
{
    iClear();
    iShowLoadedImage(0, 0, &backgroundImage);

    iSetColor(0, 0, 0);

    iShowText(WIDTH / 2 - 320, HEIGHT / 2 + 100, "Enter your name", FONT_PATH, 80);

    // Text field
    iShowText(WIDTH / 2 - 315, HEIGHT / 2, playerNameInput, FONT_PATH, 36);
    iFilledRectangle(WIDTH / 2 - 315, HEIGHT / 2 - 26, 660, 4);

    iShowText(WIDTH / 2 - 180, 48, "Press Enter to continue", FONT_PATH, 30);

    if (strlen(playerName) > 0)
    {
        // Back button
        buttons[18].page = NAME_INPUT_PAGE;
        drawTextButton(buttons[18]);
    }
}

// * UI Widget: Page function definitions
void drawMenuPage()
{
    iClear();
    iShowLoadedImage(0, 0, &backgroundImage);

    iSetColor(0, 0, 0);

    // Player name
    char playerNameText[80];
    sprintf(playerNameText, "Player: %s", playerName);
    iShowText(40, HEIGHT - 60, playerNameText, FONT_PATH, 36);

    // Logo
    iShowText(40, 170, "RETRO", FONT_PATH, 167);
    iShowText(40, 40, "RACCOON", FONT_PATH, 120);

    // Menu buttons
    for (int i = 0; i < 7; i++)
    {
        if (i == 0 && !isResumable)
            continue;
        drawTextButton(buttons[i]);
    }
}

void drawLevelsPage()
{
    iClear();
    iShowLoadedImage(0, 0, &backgroundImage);

    iSetColor(0, 0, 0);
    iShowText(WIDTH / 2 - 120, HEIGHT - 100, "Levels", FONT_PATH, 80);

    // Level buttons
    for (int i = 7; i < 12; i++)
    {
        drawTextButton(buttons[i]);
    }

    // Back button
    buttons[18].page = LEVELS_PAGE;
    drawTextButton(buttons[18]);
}

void drawHighScoresPage()
{
    iClear();
    iShowLoadedImage(0, 0, &backgroundImage);

    iSetColor(0, 0, 0);
    iShowText(WIDTH / 2 - 210, HEIGHT - 100, "High Scores", FONT_PATH, 80);

    // Table header row
    for (int i = 0; i < LEVEL_COUNT; i++)
    {
        char levelText[50];
        sprintf(levelText, "Level %d", i + 1);
        iShowText(500 + i * 150, HEIGHT - 200, levelText, FONT_PATH, 30);
    }

    // Table body
    for (int i = 0; i < playerCount; i++)
    {
        // Each row of the table
        iShowText(60, HEIGHT - 250 - i * 50, playerNames[i], FONT_PATH, 30);
        for (int j = 0; j < LEVEL_COUNT; j++)
        {
            char scoreText[50];
            if (highScores[i][j][0] == 0)
                sprintf(scoreText, "N/A"); // Player has not played this level yet.
            else
                sprintf(scoreText, "%d - %d", highScores[i][j][0], highScores[i][j][1]);
            iShowText(500 + j * 150, HEIGHT - 250 - i * 50, scoreText, FONT_PATH, 28);
        }
    }

    // Back button
    buttons[18].page = HIGH_SCORES_PAGE;
    drawTextButton(buttons[18]);
}

void drawOptionsPage()
{
    iClear();
    iShowLoadedImage(0, 0, &backgroundImage);

    iSetColor(0, 0, 0);
    iShowText(WIDTH / 2 - 140, HEIGHT - 100, "Options", FONT_PATH, 80);

    // Music off/on
    icons[0].image = isMusicOn ? &audioOnImage : &audioOffImage;
    drawIconButton(icons[0]);
    iShowText(WIDTH / 2 - 20, HEIGHT / 2 + 8, "Music", FONT_PATH, 60);

    // Sound off/on
    icons[1].image = isSoundOn ? &audioOnImage : &audioOffImage;
    drawIconButton(icons[1]);
    iShowText(WIDTH / 2 - 20, HEIGHT / 2 - 92, "Sound", FONT_PATH, 60);

    // Edit Name button
    drawTextButton(buttons[17]);

    // Back button
    buttons[18].page = OPTIONS_PAGE;
    drawTextButton(buttons[18]);
}

void drawHelpPage()
{
    iClear();
    iShowLoadedImage(0, 0, &backgroundImage);

    iSetColor(0, 0, 0);
    iShowText(WIDTH / 2 - 100, HEIGHT - 100, "Help", FONT_PATH, 80);

    // Section 1: Controls
    iShowText(80, HEIGHT - 200, "Controls", FONT_PATH, 60);
    iShowText(80, HEIGHT - 250, "Left and Right Arrow - Move", FONT_PATH, 30);
    iShowText(80, HEIGHT - 300, "Up Arrow or Space - Jump", FONT_PATH, 30);
    iShowText(80, HEIGHT - 350, "Esc - Pause", FONT_PATH, 30);

    // Section 2: Objectives
    iShowText(80, HEIGHT - 450, "Objective", FONT_PATH, 60);
    iShowText(80, HEIGHT - 500, "Reach the flag on the right", FONT_PATH, 30);
    iShowText(80, HEIGHT - 550, "Avoid traps", FONT_PATH, 30);
    iShowText(80, HEIGHT - 600, "Collect coins and diamonds", FONT_PATH, 30);

    // Section 3: Scoring
    iShowText(600, HEIGHT - 200, "Scoring", FONT_PATH, 60);
    iShowText(600, HEIGHT - 250, "Coin - 10 points", FONT_PATH, 30);
    iShowText(600, HEIGHT - 300, "Diamond - 50 points", FONT_PATH, 30);

    // Section 4: Stars
    iShowText(600, HEIGHT - 400, "Stars", FONT_PATH, 60);
    iShowText(600, HEIGHT - 450, "3 Stars - 3 lives + all coins & diamonds", FONT_PATH, 30);
    iShowText(600, HEIGHT - 500, "2 Stars - 3 lives or all coins & diamonds", FONT_PATH, 30);
    iShowText(600, HEIGHT - 550, "1 Star - Finished, but missed both", FONT_PATH, 30);

    // Back button
    buttons[18].page = HELP_PAGE;
    drawTextButton(buttons[18]);
}

void drawCreditsPage()
{
    iClear();
    iShowLoadedImage(0, 0, &backgroundImage);

    iSetColor(0, 0, 0);
    iShowText(WIDTH / 2 - 130, HEIGHT - 100, "Credits", FONT_PATH, 80);

    // Section 1: Contributors
    iShowText(80, HEIGHT - 200, "Contributors", FONT_PATH, 60);
    iShowText(80, HEIGHT - 250, "2405102 - Arif Awasaf Wriddho", FONT_PATH, 30);
    iShowText(80, HEIGHT - 300, "2405103 - Kazi Md. Raiyan", FONT_PATH, 30);

    // Section 2: Tools
    iShowText(80, HEIGHT - 400, "Tools", FONT_PATH, 60);
    iShowText(80, HEIGHT - 450, "Modern iGraphics v0.4.0 by Mahir Labib Dihan", FONT_PATH, 30);
    iShowText(80, HEIGHT - 500, "Tiled - for level design", FONT_PATH, 30);

    // Section 3: Assets
    iShowText(80, HEIGHT - 600, "Assets", FONT_PATH, 60);
    iShowText(80, HEIGHT - 650, "Pixel Platformer by Kenney", FONT_PATH, 30);

    // Section 4: Supervisor
    iShowText(700, HEIGHT - 200, "Supervisor", FONT_PATH, 60);
    iShowText(700, HEIGHT - 250, "Sumaiya Sultana (SSA)", FONT_PATH, 30);

    // Back button
    buttons[18].page = CREDITS_PAGE;
    drawTextButton(buttons[18]);
}

void drawGamePage()
{
    iClear();
    iShowLoadedImage(0, 0, &backgroundImage);

    drawTiles();

    // Draw player
    if (player.velocityY > 0)
    {
        iSetSpritePosition(&playerJumpSprite, player.x, player.y);
        iShowSprite(&playerJumpSprite);
    }
    else
    {
        iSetSpritePosition(&playerIdleSprite, player.x, player.y);
        iShowSprite(&playerIdleSprite);
    }

    drawScore();
    drawLifeCount();
}

void drawWinPage()
{
    iClear();
    iShowLoadedImage(0, 0, &backgroundImage);

    char levelText[50];
    sprintf(levelText, "Level %d", currentLevel);

    iSetColor(0, 0, 0);
    iShowText(WIDTH / 2 - 140, HEIGHT - 120, levelText, FONT_PATH, 80);
    iShowText(WIDTH / 2 - 130, HEIGHT / 2 - 70, scoreText, FONT_PATH, 60);

    // Stars
    iShowLoadedImage(WIDTH / 2 - 240, HEIGHT / 2 + 50, &yellowStarImage);
    iShowLoadedImage(WIDTH / 2 - 60, HEIGHT / 2 + 50, starCount > 1 ? &yellowStarImage : &whiteStarImage);
    iShowLoadedImage(WIDTH / 2 + 120, HEIGHT / 2 + 50, starCount > 2 ? &yellowStarImage : &whiteStarImage);

    // Main Menu and Play Again buttons
    drawTextButton(buttons[14]);
    drawTextButton(buttons[15]);

    // Next Level or Exit button
    if (currentLevel < LEVEL_COUNT)
    {
        buttons[16].text = "NEXT LEVEL";
        buttons[16].onClick = []()
        {
            changeLevel(currentLevel + 1);
        };
    }
    else
    {
        buttons[16].text = "EXIT";
        buttons[16].onClick = []()
        {
            iCloseWindow();
        };
    }
    drawTextButton(buttons[16]);
}

void drawGameOverPage()
{
    iClear();
    iShowLoadedImage(0, 0, &backgroundImage);

    iSetColor(0, 0, 0);
    iShowText(WIDTH / 2 - 245, HEIGHT / 2 + 100, "Game Over", FONT_PATH, 100);
    iShowText(WIDTH / 2 - 126, HEIGHT / 2, scoreText, FONT_PATH, 60);

    // Main Menu and Try Again buttons
    drawTextButton(buttons[12]);
    drawTextButton(buttons[13]);
}

// * Keyboard functions
void iKeyboard(unsigned char key, int state)
{
    // TODO: Add enter key handling to go to the hovered page when in the menu page. Also, selecting buttons by the Up and Down arrow keys.
    switch (key)
    {
    case 27:                                                              // Escape key
        if (!(currentPage == NAME_INPUT_PAGE && strlen(playerName) == 0)) // No going back from the name input page when there is no player name.
            pauseGame();
        break;
    case ' ': // Space key
        if (currentPage == GAME_PAGE)
            jump();
        break;
    default:
        break;
    }

    if (currentPage == NAME_INPUT_PAGE && state == GLUT_DOWN)
    {
        int len = strlen(playerNameInput);
        if (key == '\b' && len > 0)
        {
            // Backspace key pressed.
            playerNameInput[len - 1] = '\0';
        }
        else if (key == '\r' && len > 0 && !isStringWhitespace(playerNameInput))
        {
            // Enter key pressed.
            currentPage = MENU_PAGE;
            toLowerString(playerNameInput);
            strcpy(playerName, playerNameInput);

            if (isNewPlayer())
            {
                resetGame();
                strcpy(playerNames[playerCount], playerName);
                playerCount++;
                saveHighScores();
            }

            FILE *playerNameFile = fopen("saves/current_player.txt", "w");
            fprintf(playerNameFile, "%s", playerNameInput);
            fclose(playerNameFile);
        }
        else if (key >= 32 && key <= 126 && len < MAX_PLAYER_NAME_LENGTH)
        {
            playerNameInput[len] = key;
            playerNameInput[len + 1] = '\0';
        }
    }
}

// GLUT_KEY_F1, GLUT_KEY_F2, GLUT_KEY_F3, GLUT_KEY_F4, GLUT_KEY_F5, GLUT_KEY_F6, GLUT_KEY_F7, GLUT_KEY_F8, GLUT_KEY_F9, GLUT_KEY_F10, GLUT_KEY_F11, GLUT_KEY_F12, GLUT_KEY_LEFT, GLUT_KEY_UP, GLUT_KEY_RIGHT, GLUT_KEY_DOWN, GLUT_KEY_PAGE_UP, GLUT_KEY_PAGE_DOWN, GLUT_KEY_HOME, GLUT_KEY_END, GLUT_KEY_INSERT
void iSpecialKeyboard(unsigned char key, int state)
{
    if (currentPage != GAME_PAGE)
        return;

    switch (key)
    {
    case GLUT_KEY_UP:
        jump();
        break;
    // TODO: Extract function
    case GLUT_KEY_LEFT:
        if (state == GLUT_DOWN && !doesCollideArray[ROWS - (int)(player.y / TILE_SIZE) - 1][(player.x / TILE_SIZE) - 1] && player.animateToX >= TILE_SIZE)
        {
            player.animateToX -= TILE_SIZE;
            iResumeTimer(horizontalMovementTimer);
        }
        if (player.direction == RIGHT)
        {
            iMirrorSprite(&playerIdleSprite, HORIZONTAL);
            iMirrorSprite(&playerJumpSprite, HORIZONTAL);
            player.direction = LEFT;
        }
        break;
    case GLUT_KEY_RIGHT:
        if (state == GLUT_DOWN && !doesCollideArray[ROWS - (int)(player.y / TILE_SIZE) - 1][(player.x / TILE_SIZE) + 1])
        {
            player.animateToX += TILE_SIZE;
            iResumeTimer(horizontalMovementTimer);
        }
        if (player.direction == LEFT)
        {
            iMirrorSprite(&playerIdleSprite, HORIZONTAL);
            iMirrorSprite(&playerJumpSprite, HORIZONTAL);
            player.direction = RIGHT;
        }
        break;
    default:
        break;
    }
}

// * Mouse functions
void iMouse(int button, int state, int mx, int my)
{
    // TODO: Add click effect
    // Button click handling
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        for (int i = 0; i < BUTTON_COUNT; i++)
        {
            if (buttons[i].page == currentPage && mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].width && my >= buttons[i].y && my <= buttons[i].y + buttons[i].height)
            {
                if (isSoundOn)
                    iPlaySound("assets/sounds/menu_click.wav", 0, 30);
                if (i == 0 && !isResumable)
                    continue;
                if (i == 18 && strlen(playerName) == 0) // No back button for the name input page when there is no player name.
                    continue;
                buttons[i].onClick();
            }
        }
        for (int i = 0; i < ICON_COUNT; i++)
        {
            if (icons[i].page == currentPage && mx >= icons[i].x && mx <= icons[i].x + icons[i].width && my >= icons[i].y && my <= icons[i].y + icons[i].height)
            {
                if (isSoundOn)
                    iPlaySound("assets/sounds/menu_click.wav", 0, 30);
                icons[i].onClick();
            }
        }
    }
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
}

int main(int argc, char *argv[])
{
    initializePlayer();
    initializeCollectedCollectables(collectedCoins);
    initializeCollectedCollectables(collectedDiamonds);
    initializeCollectedCollectables(collectedLives);

    loadAssets();
    loadLevel(currentLevel);
    loadPlayerName();
    loadHighScores();
    loadOptions();

    if (strlen(playerName) == 0)
        currentPage = NAME_INPUT_PAGE;
    else
        currentPage = MENU_PAGE;

    glutInit(&argc, argv); // argc and argv are used for command line arguments.

    gameStateUpdateTimer = iSetTimer(10, gameStateUpdate);
    horizontalMovementTimer = iSetTimer(10, animateHorizontalMovement);
    spriteAnimationTimer = iSetTimer(200, animateSprites);
    iPauseTimer(gameStateUpdateTimer);
    iPauseTimer(horizontalMovementTimer);
    iPauseTimer(spriteAnimationTimer);

    iInitializeFont();
    iInitializeSound();
    playBackgroundMusic(MENU_MUSIC);

    iOpenWindow(WIDTH, HEIGHT, TITLE);

    return 0;
}