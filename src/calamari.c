// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2000,2001 Alistair Riddoch

#ifdef WIN32
#include <Windows.h>
#define M_PI 3.14159265f
#endif

#include <SDL.h>
#include <GL/gl.h>

#include <cmath>

static const int blocks_wide = 8;
static const int blocks_high = 12;

static int block_i = 4;
static int block_j = 11;

static const int step_time = 1000;

static bool slots[blocks_wide][blocks_high + 1];

static const int width = 400;
static const int height = 400;

static bool done = false;

unsigned long block_list;

SDL_Surface * screen;

bool initScreen()
{
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0) {
        // std::cerr << "Failed to initialise video" << std::endl << std::flush;
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    if ((screen = SDL_SetVideoMode(width, height, 0, SDL_OPENGL)) == NULL) {
        // std::cerr << "Failed to set video mode" << std::endl << std::flush;
        SDL_Quit();
        return false;
    }

    glViewport(0,0,width,height);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearDepth(1.0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-0.5f, 0.5f, -0.5f, 0.5f, 0.65f, 20.0f);

    glMatrixMode(GL_MODELVIEW);

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
    clear();

    block_list = glGenLists(1);
    glNewList(block_list, GL_COMPILE);
    glBegin(GL_QUAD_STRIP);
    glColor3f(0.3, 0.3, 0.3);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f, 0.5f, -0.5f);
    glVertex3f(0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f, 0.5f, -0.5f);
    glVertex3f(0.5f, -0.5f, 0.5f);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glVertex3f(-0.5f, -0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f, 0.5f, -0.5f);
    glEnd();

    glBegin(GL_QUADS);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f, -0.5f, 0.5f);
    glVertex3f(0.5f, -0.5f, 0.5f);

    glVertex3f(-0.5f, 0.5f, -0.5f);
    glVertex3f(0.5f, 0.5f, -0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glEnd();

    glEndList();
}

void draw_blocks()
{
    glTranslatef(-(float)blocks_wide/2.0f, -(float)blocks_high/2.0f, 0.0f);
    for(int i = 0; i < blocks_wide; ++i) {
        for(int j = 0; j < blocks_high; ++j) {
            if ((slots[i][j]) || ((i == block_i) && (j == block_j))) {
                glCallList(block_list);
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

    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -10.0f);

    glRotatef(10, sin(rot), cos(rot), 0.0f);

    glPushMatrix();
    draw_blocks();
    glPopMatrix();

    SDL_GL_SwapBuffers();
}

void checkrows()
{
    for(int j = 0; j < blocks_high; ++j) {
        bool solid = true;
        for(int i = 0; i < blocks_wide; ++i) {
            solid &= slots[i][j];
        }
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
    if ((block_j == 0) || (slots[block_i][block_j - 1])) {
        // Grounded at block_j
        slots[block_i][block_j] = true;
        block_i = 3; block_j = 12;
    } else {
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

    while (!done) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    done = true;
                    break;
                case SDL_KEYDOWN:
                    if ( event.key.keysym.sym == SDLK_ESCAPE ) {
                        // quit
                        done = true;
                    }
                    if ( event.key.keysym.sym == SDLK_UP ) {
                        // rot not valid for single blocks
                    }
                    if ( event.key.keysym.sym == SDLK_DOWN ) {
                        // drop
                        int j;
                        for(j = block_j; j > 0; --j) {
                            if (slots[block_i][j-1]) {
                                break;
                            }
                        }
                        block_j = j;
                    }
                    if ( event.key.keysym.sym == SDLK_LEFT ) {
                        // left
                        if ((block_i > 0) && !slots[block_i - 1][block_j]) {
                            --block_i;
                        }
                    }
                    if ( event.key.keysym.sym == SDLK_RIGHT ) {
                        // right
                        if ((block_i < (blocks_wide-1)) && !slots[block_i + 1][block_j]) {
                            ++block_i;
                        }
                    }
                    break;
                default:
                    break;
            }
        }
        int ticks = SDL_GetTicks();
        ++fps;
        if ((ticks - last_step) > step_time) {
            last_step = ticks;
            fps = 0;
            step();
        }
        float delta = (ticks - elapsed_time) / 1000.0f;
        elapsed_time = ticks;
        rot += delta;
        if (rot > (2 * M_PI)) {
            rot -= (2 * M_PI);
        }
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
