// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2000,2004 Alistair Riddoch

#ifdef WIN32
#include <Windows.h>
#define M_PI 3.14159265f
#endif

#include <SDL/SDL.h>

#include <GL/gl.h>
#include <GL/glu.h>

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

bool initScreen()
{
    const int width = 400;
    const int height = 400;

    // Initialise SDL
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0) {
        // std::cerr << "Failed to initialise video" << std::endl << std::flush;
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Surface * screen;

    // Create the window
    if ((screen = SDL_SetVideoMode(width, height, 0, SDL_OPENGL)) == NULL) {
        // std::cerr << "Failed to set video mode" << std::endl << std::flush;
        SDL_Quit();
        return false;
    }

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

float rot = 0.0f;

void render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up the modelview - camera 20 units from the objects
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -20.0f);

    // Add a little camera movement
    glRotatef(10, sin(rot), cos(rot), 0.0f);

    // Draw the scene
    draw_blocks();

    SDL_GL_SwapBuffers();
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

void step()
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
}

void loop()
{
    SDL_Event event;
    int elapsed_time = SDL_GetTicks();
    int last_step = elapsed_time;
    int fps = 0;

    // This is the main program loop. It will run until something sets
    // the flag to indicate we are done.
    while (!done) {
        // Check for events
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    // The user closed the window
                    done = true;
                    break;
                case SDL_KEYDOWN:
                    // We have a keypress
                    if ( event.key.keysym.sym == SDLK_ESCAPE ) {
                        // quit
                        done = true;
                    }
                    if ( event.key.keysym.sym == SDLK_UP ) {
                        // In tetris up is often used to rotate
                        // the object. This is not valid for single blocks
                    }
                    if ( event.key.keysym.sym == SDLK_DOWN ) {
                        // Drop the block
                        int j;
                        for(j = block_j; j > 0; --j) {
                            if (slots[block_i][j-1]) {
                                break;
                            }
                        }
                        block_j = j;
                    }
                    if ( event.key.keysym.sym == SDLK_LEFT ) {
                        // Move block left
                        if ((block_i > 0) && !slots[block_i - 1][block_j]) {
                            --block_i;
                        }
                    }
                    if ( event.key.keysym.sym == SDLK_RIGHT ) {
                        // Move block right
                        if ((block_i < (blocks_wide-1)) && !slots[block_i + 1][block_j]) {
                            ++block_i;
                        }
                    }
                    break;
                default:
                    break;
            }
        }
        // Get the time and check if enough time has elapsed for
        // the moving block to move
        int ticks = SDL_GetTicks();
        ++fps;
        if ((ticks - last_step) > step_time) {
            last_step = ticks;
            fps = 0;
            step();
        }
        float delta = (ticks - elapsed_time) / 1000.0f;
        elapsed_time = ticks;

        // Update the rotation on the camera
        rot += delta;
        if (rot > (2 * M_PI)) {
            rot -= (2 * M_PI);
        }

        // Render the screen
        render();
    }
}

int main()
{
    if (!initScreen()) {
        return 1;
    }

    setup();

    loop();
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
