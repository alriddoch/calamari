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

#include "font.h"

#include <cmath>
#include <cstring>

// Constants

static const int screen_width = 600;
static const int screen_height = 400;

static const int grid_width = 12;
static const int grid_height = 12;

static const int step_time = 1000;

// Variables that store the game state

static int block_i = 4;
static int block_j = 11;

static bool slots[grid_width][grid_height + 1];

static bool program_finished = false;

int average_frames_per_second;
GLuint textTexture;
GLuint textBase;

bool init_graphics()
{
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
    screen = SDL_SetVideoMode(screen_width, screen_height, 0, SDL_OPENGL);
    if (screen == NULL) {
        // std::cerr << "Failed to set video mode" << std::endl << std::flush;
        SDL_Quit();
        return false;
    }

    // Setup the viewport transform
    glViewport(0, 0, screen_width, screen_height);

    glEnableClientState(GL_VERTEX_ARRAY);

    // Set the colour the screen will be when cleared - black
    glClearColor(0.0, 0.0, 0.0, 0.0);

    // Initialise the texture used for rendering text
    glGenTextures(1, &textTexture);
    glBindTexture(GL_TEXTURE_2D, textTexture);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexImage2D(GL_TEXTURE_2D, 0, texture_font_internalFormat,
                 texture_font_width, texture_font_height, 0,
                 texture_font_format, GL_UNSIGNED_BYTE, texture_font_pixels);
    if (glGetError() != 0) {
        return false;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    textBase = glGenLists(256);
    float vertices[] = { 0, 0, 16, 0, 16, 16, 0, 16 };
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    for(int loop=0; loop<256; loop++) {
        float cx=(float)(loop%16)/16.0f;      // X Position Of Current Character
        float cy=(float)(loop/16)/16.0f;      // Y Position Of Current Character

        float texcoords[] = { cx, 1-cy-0.0625f,
                              cx+0.0625f, 1-cy-0.0625f,
                              cx+0.0625f, 1-cy,
                              cx, 1-cy };

        glNewList(textBase+loop,GL_COMPILE);   // Start Building A List

        glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
        glDrawArrays(GL_QUADS, 0, 4);

        glTranslated(10,0,0);                  // Move To The Right Of The Character
        glEndList();                           // Done Building The Display List
    }                                        
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    return true;
}

void clear()
{
    for(int i = 0; i < grid_width; ++i) {
        for(int j = 0; j <= grid_height; ++j) {
            slots[i][j] = false;
        }
    }
}

void gl_print(const char * str)
{
    glPushMatrix();
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, textTexture);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glListBase(textBase-32);
    glCallLists(strlen(str),GL_BYTE,str);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glPopMatrix();
}

void setup()
{
    // Clear the block store
    clear();

}

void draw_unit_cube()
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

void draw_grid()
{
    float horizontal_line_vertices[] = {
        0.f, 0.f, 0.f,
        grid_width, 0.f, 0.f,
    };
    float vertical_line_vertices[] = {
        0.f, 0.f, 0.f,
        0.f, grid_height, 0.f,
    };

    // Move to the origin of the grid
    glTranslatef(-(float)grid_width/2.0f, -(float)grid_height/2.0f, 0.0f);
    // Store this position
    glPushMatrix();

    glVertexPointer(3, GL_FLOAT, 0, vertical_line_vertices);
    for (int i = 0; i <= grid_width; ++i) {
        glDrawArrays(GL_LINES, 0, 2);
        glTranslatef(1.0f, 0.0f, 0.0f);
    }

    // Reset to the origin
    glPopMatrix();
    glPushMatrix();

    glVertexPointer(3, GL_FLOAT, 0, horizontal_line_vertices);
    for (int j = 0; j <= grid_height; ++j) {
        glDrawArrays(GL_LINES, 0, 2);
        glTranslatef(0.0f, 1.0f, 0.0f);
    }

    // Reset to the origin
    glPopMatrix();
    glPushMatrix();

    for(int i = 0; i < grid_width; ++i) {
        for(int j = 0; j < grid_height; ++j) {
            if ((slots[i][j]) || ((i == block_i) && (j == block_j))) {
                draw_unit_cube();
            }
            glTranslatef(0.0f, 1.0f, 0.0f);
        }
        glTranslatef(1.0f, -grid_height, 0.0f);
    }

    glPopMatrix();
}

float camera_rotation = 0.0f;

void render_scene()
{
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set the projection transform
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, (float)screen_width/screen_height, 1.f, 100.f);

    // Set up the modelview
    glMatrixMode(GL_MODELVIEW);
    // Reset the camera
    glLoadIdentity();
    // Move the camera 20 units from the objects
    glTranslatef(0.0f, 0.0f, -20.0f);

    // Enable the depth test
    glEnable(GL_DEPTH_TEST);

    glColor3f(0.3, 0.3, 0.3);

    // Add a little camera movement
    glRotatef(10, sin(camera_rotation), cos(camera_rotation), 0.0f);

    // Draw the scene
    draw_grid();
}

// Draw any text output and other screen oriented user interface
void render_ui()
{
    char buf[256];

    // Set the projection to a transform that allows us to use pixel
    // coordinates.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, screen_width, 0, screen_height, -800.0f, 800.0f);

    // Set up the modelview
    glMatrixMode(GL_MODELVIEW);
    // Reset the camera
    glLoadIdentity();

    // Disable the depth test, as its not useful when rendering text
    glDisable(GL_DEPTH_TEST);

    // 
    glTranslatef(5.f, 5.f, 0);
    glColor3f(1.f, 1.f, 1.f);
    sprintf(buf, "FPS: %d", average_frames_per_second);
    gl_print(buf);
}

// Handle a mouse click. Coordinates are given in OpenGL style coordinates
// with the origin in the bottom left.
void mouse_click(unsigned int x, unsigned int y)
{
}

void check_rows()
{
    // Go through all the stationary blocks to check if there
    // is a complete solid row
    for(int j = 0; j < grid_height; ++j) {
        // This flag will become false if this row has a gap
        bool solid = true;
        for(int i = 0; i < grid_width; ++i) {
            solid &= slots[i][j];
        }
        // If solid is still true, there were no gaps, so we shift all
        // the rows above down.
        if (solid) {
            for(int k = 0; k < grid_width; ++k) {
                for(int l = j + 1; l < grid_height; ++l) {
                    slots[k][l - 1] = slots[k][l];
                }
                slots[k][grid_height - 1] = false;
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
        block_i = 4; block_j = 12;
    } else {
        // Step the moving block down the screen
        --block_j;
    }

    check_rows();
}

void loop()
{
    SDL_Event event;
    int elapsed_time = SDL_GetTicks();
    int last_step = elapsed_time;
    int frame_count = 0;

    // This is the main program loop. It will run until something sets
    // the flag to indicate we are done.
    while (!program_finished) {
        // Check for events
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    // The user closed the window
                    program_finished = true;
                    break;
                case SDL_KEYDOWN:
                    // We have a keypress
                    if ( event.key.keysym.sym == SDLK_ESCAPE ) {
                        // quit
                        program_finished = true;
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
                        if ((block_i < (grid_width-1)) && !slots[block_i + 1][block_j]) {
                            ++block_i;
                        }
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        mouse_click(event.button.x,
                                    screen_height - event.button.y);
                    }
                    break;
                default:
                    break;
            }
        }
        // Get the time and check if enough time has elapsed for
        // the moving block to move
        int ticks = SDL_GetTicks();
        ++frame_count;
        if ((ticks - last_step) > step_time) {
            last_step = ticks;
            average_frames_per_second = frame_count;
            frame_count = 0;
            step();
        }
        float delta = (ticks - elapsed_time) / 1000.0f;
        elapsed_time = ticks;

        // Update the rotation on the camera
        camera_rotation += delta;
        if (camera_rotation > (2 * M_PI)) {
            camera_rotation -= (2 * M_PI);
        }

        // Render the screen
        render_scene();
        render_ui();

        SDL_GL_SwapBuffers();
    }
}

int main()
{
    if (!init_graphics()) {
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
