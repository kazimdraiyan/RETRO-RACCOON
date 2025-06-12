#include "iGraphics.h"

// TODO: Try to use object oriented programming.
// TODO: Try to add Dvorak keyboard support.

#define WIDTH 1920
#define HEIGHT 1080
#define TITLE "Platformer Game"

// TODO: Check out how these constants work and be optimized.
#define LEFT 0x0001
#define RIGHT 0x0002
#define UP 0x0003
#define DOWN 0x0004

bool firstDraw = true;


// Player represented as an array: {x, y, width, height}
double player[4] = {
    WIDTH / 9.0, 150, 50, 50};

// Platforms represented as an array of arrays: {x, y, width, height}
double platforms[][4] = {
    {WIDTH / 9.0, 100, WIDTH / 3.0, 50},    // Platform 1
    {WIDTH * 5.0 / 9, 100, WIDTH / 3.0, 50} // Platform 2
};

int touchingBoundary()
{
    if (player[0] <= 0)
        return LEFT;
    else if (player[0] + player[2] >= WIDTH)
        return RIGHT;
    else if (player[1] <= 0)
        return DOWN;
    else if (player[1] + player[3] >= HEIGHT)
        return UP;
    return 0; // Not touching any boundary
}

int blockedSide()
{
    return touchingBoundary();
}

void iDraw()
{
    if (firstDraw)
    {
        glutFullScreen();
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
}

void iKeyboard(unsigned char key)
{
    switch (key)
    {
    // TODO: Implement the logic where the player is currently not blocked, but will be blocked if it moves, then it will move only the available space.
    case 'a':
        if (blockedSide() != LEFT)
        {
            player[0] -= 10;
        }
        break;
    case 'd':
        if (blockedSide() != RIGHT)
        {
            player[0] += 10;
        }
        break;
    case 'w':
        if (blockedSide() != UP)
        {
            player[1] += 10;
        }
        break;
    case 's':
        if (blockedSide() != DOWN)
        {
            player[1] -= 10;
        }
        break;
    default:
        break;
    }
}

/* GLUT_KEY_F1, GLUT_KEY_F2, GLUT_KEY_F3, GLUT_KEY_F4, GLUT_KEY_F5, GLUT_KEY_F6,
GLUT_KEY_F7, GLUT_KEY_F8, GLUT_KEY_F9, GLUT_KEY_F10, GLUT_KEY_F11,
GLUT_KEY_F12, GLUT_KEY_LEFT, GLUT_KEY_UP, GLUT_KEY_RIGHT, GLUT_KEY_DOWN,
GLUT_KEY_PAGE_UP, GLUT_KEY_PAGE_DOWN, GLUT_KEY_HOME, GLUT_KEY_END,
GLUT_KEY_INSERT */
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
}

void iMouseDrag(int mx, int my)
{
}

void iMouse(int button, int state, int mx, int my)
{
}

void iMouseWheel(int dir, int mx, int my)
{
}

int main(int argc, char *argv[])
{
    glutInit(&argc, argv);

    iInitialize(WIDTH, HEIGHT, TITLE);

    return 0;
}