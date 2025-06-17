#include "iGraphics.h"

// TODO: Try to use object oriented programming.
// TODO: Try to add Dvorak keyboard support.

#define WIDTH 1560
#define HEIGHT 720
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

void drawMenuPage();

Image menu_background_image;

int hoverRectangleYs[5] = {HEIGHT / 2 + 40};
int hoverRectangleY = -1;
int hoverRectangleHeight = 50;

bool firstDraw = true;
bool blockedSides[4] = {false}; // Left, Right, Up, Down
double t = 0;

// Player represented as an array: {x, y, width, height}
double player[4] = {WIDTH / 9.0, 150, 50, 50};

// Platforms represented as an array of arrays: {x, y, width, height}
double platforms[][4] = {
    {WIDTH / 9.0, 100, WIDTH / 3.0, 50},    // Platform 1
    {WIDTH * 5.0 / 9, 100, WIDTH / 3.0, 50} // Platform 2
};

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
        iLoadImage(&menu_background_image, "assets/images/menu_page_background.png");
        iResizeImage(&menu_background_image, WIDTH, HEIGHT);
        firstDraw = false;
    }

    iClear();

    iSetColor(50, 50, 50);
    iFilledRectangle(0, 0, WIDTH, HEIGHT); // Background

    // Two platforms
    iSetColor(0, 0, 0);
    iFilledRectangle(platforms[0][0], platforms[0][1], platforms[0][2], platforms[0][3]); // Platform 1
    iFilledRectangle(platforms[1][0], platforms[1][1], platforms[1][2], platforms[1][3]); // Platform 2

    // Player
    iSetColor(250, 0, 0);
    iFilledRectangle(player[0], player[1], player[2], player[3]);

    updateBlockedSides();

    // Apply gravity effect
    t++;
    double gravityDeltaY = 0.03 * (t * 2 - 1); // TODO: Store the 0.03 magic number to somewhere.
    goDown(gravityDeltaY);

    if (currentPage == MENU_PAGE)
    {
        drawMenuPage();
    }
    else if (currentPage == GAME_PAGE)
    {
    }
    else if (currentPage == LEVELS_PAGE)
    {
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
                player[0] -= 10;
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
                player[0] += 10;
        }
        break;
    case 27: // Escape key
        currentPage = MENU_PAGE;
        break;
    default:
        break;
    }
}

void drawMenuPage()
{
    iClear();
    iSetColor(255, 255, 255);

    iShowLoadedImage(0, 0, &menu_background_image);

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
    if (currentPage != MENU_PAGE)
        return;
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

void iMouseDrag(int mx, int my)
{
}

void iMouse(int button, int state, int mx, int my)
{
    if (currentPage != MENU_PAGE)
        return;
    // Button click handling
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
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
        // hoverRectangleY = -1;
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