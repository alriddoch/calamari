// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2000,2004 Alistair Riddoch

#ifdef WIN32
#include <Windows.h>
#define M_PI 3.14159265f
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <cmath>

// Constants

static const int blocks_wide = 8;
static const int blocks_high = 12;

static const int step_time = 1000;

// Variables that store the game state

static int block_i = 4;
static int block_j = 11;

static bool slots[blocks_wide][blocks_high + 1];

static bool done = false;

void clear()
{
    for(int i = 0; i < blocks_wide; ++i) {
        for(int j = 0; j <= blocks_high; ++j) {
            slots[i][j] = false;
        }
    }
}

void setup()
{
    // Clear the block store
    clear();

}

void draw_one_block()
{
    static const float front_vertices[] = {
        0.f, 0.f, 1.f,
        1.f, 0.f, 1.f,
        1.f, 1.f, 1.f,
        0.f, 1.f, 1.f,
    };
    glVertexPointer(3, GL_FLOAT, 0, front_vertices);
    glDrawArrays(GL_QUADS, 0, 4);

    static const float left_vertices[] = {
        0.f, 0.f, 0.f,
        0.f, 0.f, 1.f,
        0.f, 1.f, 1.f,
        0.f, 1.f, 0.f,
    };
    glVertexPointer(3, GL_FLOAT, 0, left_vertices);
    glDrawArrays(GL_QUADS, 0, 4);

    static const float right_vertices[] = {
        1.f, 0.f, 1.f,
        1.f, 0.f, 0.f,
        1.f, 1.f, 0.f,
        1.f, 1.f, 1.f,
    };
    glVertexPointer(3, GL_FLOAT, 0, right_vertices);
    glDrawArrays(GL_QUADS, 0, 4);

    static const float top_vertices[] = {
        0.f, 1.f, 1.f,
        1.f, 1.f, 1.f,
        1.f, 1.f, 0.f,
        0.f, 1.f, 0.f,
    };
    glVertexPointer(3, GL_FLOAT, 0, top_vertices);
    glDrawArrays(GL_QUADS, 0, 4);

    static const float bottom_vertices[] = {
        0.f, 0.f, 0.f,
        1.f, 0.f, 0.f,
        1.f, 0.f, 1.f,
        0.f, 0.f, 1.f,
    };
    glVertexPointer(3, GL_FLOAT, 0, bottom_vertices);
    glDrawArrays(GL_QUADS, 0, 4);
}

void draw_blocks()
{
    glTranslatef(-(float)blocks_wide/2.0f, -(float)blocks_high/2.0f, 0.0f);
    for(int i = 0; i < blocks_wide; ++i) {
        for(int j = 0; j < blocks_high; ++j) {
            if ((slots[i][j]) || ((i == block_i) && (j == block_j))) {
                draw_one_block();
            }
            glTranslatef(0.0f, 1.0f, 0.0f);
        }
        glTranslatef(1.0f, -blocks_high, 0.0f);
    }

}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up the modelview - camera 20 units from the objects
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -20.0f);

    static float rot = 0.0f;

    // Update the rotation on the camera
    rot += 0.001;
    if (rot > (2 * M_PI)) {
        rot -= (2 * M_PI);
    }

    // Add a little camera movement
    glRotatef(10, sin(rot), cos(rot), 0.0f);

    // Draw the scene
    draw_blocks();

    glutSwapBuffers();
    glutPostRedisplay();
}

void checkrows()
{
    // Go through all the stationary blocks to check if there
    // is a complete solid row
    for(int j = 0; j < blocks_high; ++j) {
        // This flag will become false if this row has a gap
        bool solid = true;
        for(int i = 0; i < blocks_wide; ++i) {
            solid &= slots[i][j];
        }
        // If solid is still true, there were no gaps, so we shift all
        // the rows above down.
        if (solid) {
            for(int k = 0; k < blocks_wide; ++k) {
                for(int l = j + 1; l < blocks_high; ++l) {
                    slots[k][l - 1] = slots[k][l];
                }
                slots[k][blocks_high - 1] = false;
            }
            --j;
        }
    }
}

void step(int)
{
    // Update the current falling block

    // If its at the bottom, or there is a block below it, then it
    // is grounded and stops
    if ((block_j == 0) || (slots[block_i][block_j - 1])) {
        // Store this blick in the array of fixed blocks
        slots[block_i][block_j] = true;
        // Set the moving block back at the top
        block_i = 3; block_j = 12;
    } else {
        // Step the moving block down the screen
        --block_j;
    }

    checkrows();
    glutTimerFunc(step_time, step, 0);
}

void key_pressed(int key, int x, int y)
{
    // We have a keypress
    switch (key) {
        case GLUT_KEY_UP:
            // In tetris up is often used to rotate
            // the object. This is not valid for single blocks
            break;
        case GLUT_KEY_DOWN:
            // Drop the block
            int j;
            for(j = block_j; j > 0; --j) {
                if (slots[block_i][j-1]) {
                    break;
                }
            }
            block_j = j;
            break;
        case GLUT_KEY_LEFT:
            // Move block left
            if ((block_i > 0) && !slots[block_i - 1][block_j]) {
                --block_i;
            }
            break;
        case GLUT_KEY_RIGHT:
            // Move block right
            if ((block_i < (blocks_wide-1)) && !slots[block_i + 1][block_j]) {
                ++block_i;
            }
            break;
        default:
            break;
    }
}

bool initScreen()
{
    const int width = 400;
    const int height = 400;

    int argc = 1;
    char * argv [] = { "bonkers", 0 };
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
    glutInitWindowSize(width, height);
    glutCreateWindow("bonkers");
    glutDisplayFunc(render);
    glutSpecialFunc(key_pressed);
    glutTimerFunc(step_time, step, 0);

    // Setup the viewport transform
    glViewport(0,0,width,height);

    // Enable the depth test
    glEnable(GL_DEPTH_TEST);

    glEnableClientState(GL_VERTEX_ARRAY);

    // Set the colour the screen will be when cleared - black
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glColor3f(0.3, 0.3, 0.3);

    // Set the projection transform
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, width/height, 1.f, 100.f);

    return true;
}

int main()
{
    if (!initScreen()) {
        return 1;
    }

    setup();

    glutMainLoop();
    return 0;
}

#ifdef WIN32

int WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
)
{
    main();
}

#endif
