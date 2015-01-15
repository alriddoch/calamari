// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2000,2004 Alistair Riddoch

#ifdef WIN32
#include <Windows.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265f
#endif

#include "vector.h"
#include "quaternion.h"

#include <GL/glew.h>

#include <SDL.h>

#include "font.h"

#include <cmath>
#include <limits>

#include <assert.h>

// Constants

static const int screen_width = 600;
static const int screen_height = 400;

// Number of squares in the grid. The number of points is this number +1.
#define grid_width 12
#define grid_height 12

// Number of milliseconds between steps in the game model.
static const int step_time = 1000;

// Types

typedef struct block {
    float x, y, z;
    float scale;
    float diffuse[4];
    int present;
    Quaternion orientation;
    struct block * next;
} Block;

Block * blocks = 0;

// Variables that store the game state

static float scale = 0.1f;
static int next_level = 1;

static float pos_x = 0;
static float pos_y = -2;
static float pos_z = 0;

static float angle = 0;

static Quaternion orientation = { {0, 0, 0}, 1 };

static float velocity[3] = { 0, 0, 0 };

static bool key_lf = false;
static bool key_lb = false;
static bool key_rf = false;
static bool key_rb = false;
static bool key_flip = false;

static const float max_velocity = 3.f;
static const float max_accel = 1.f;
static const float max_decel = 3.f;

// Structure to hold the properties of a single square on the grid.
// If you want to add more information to the grid, add new members here.
typedef struct block_properties {
    bool block;
} BlockProperties;

// Current blocks on the grid, stored as true if there is a block at the
// given localtion.
BlockProperties properties[grid_width][grid_height];

// Flag used to inform the main loop if the program should now terminate.
// Set this to true if its done.
static bool program_finished = false;

// Calculated frames per second to display. Very useful feedback when
// debugging graphics performance problems.
int average_frames_per_second;

// Texture handles for the texture used to handle printing text on the
// screen.
GLuint textTexture;
GLuint textBase;

GLuint gVBO = 0;
GLuint gIBO = 0;

GLuint gProgramID = 0;
GLuint gTextProgID = 0;
GLint gVertexPos2DLocation = -1;

static inline float square(float f)
{
    return f * f;
}

static inline float cube(float f)
{
    return f * f * f;
}

static inline float uniform(float min, float max)
{
    return ((float)rand() / RAND_MAX) * (max - min) + min;
}

static float logarithmic(float min, float max)
{
    assert(min > 0.f);
    assert(max > 0.f);

    float res1 = uniform(log10(min), log10(max));
    float res2 = std::pow(10.f, res1);

    printf("%f %f %f %f %f %f\n", min, max, log10(min), log10(max), res1, res2);
    return res2;
}

void printProgramLog(GLuint program)
{
  //Make sure name is shader
  if(glIsProgram(program))
  {
    //Program log length
    int infoLogLength = 0;
    int maxLength = infoLogLength;

    //Get info string length
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

    //Allocate string
    char* infoLog = new char[ maxLength ];

    //Get info log
    glGetProgramInfoLog(program, maxLength, &infoLogLength, infoLog);
    if(infoLogLength > 0)
    {
      //Print Log
      printf("%s\n", infoLog);
    }

    //Deallocate string
    delete[] infoLog;
  }
  else
  {
    printf("Name %d is not a program\n", program);
  }
}

void printShaderLog(GLuint shader)
{
  //Make sure name is shader
  if(glIsShader(shader))
  {
    //Shader log length
    int infoLogLength = 0;
    int maxLength = infoLogLength;

    //Get info string length
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

    //Allocate string
    char* infoLog = new char[ maxLength ];

    //Get info log
    glGetShaderInfoLog(shader, maxLength, &infoLogLength, infoLog);
    if(infoLogLength > 0)
    {
      //Print Log
      printf("%s\n", infoLog);
    }

    //Deallocate string
    delete[] infoLog;
  }
  else
  {
    printf("Name %d is not a shader\n", shader);
  }
}

void camera_pos();

GLuint create_shader(GLenum type, const GLchar ** shaderSource)
{
  GLuint shader = glCreateShader(type);

  //Set vertex source
  glShaderSource(shader, 1, shaderSource, NULL);

  //Compile vertex source
  glCompileShader(shader);

  //Check vertex shader for errors
  GLint shaderCompiled = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderCompiled);
  if(shaderCompiled != GL_TRUE)
  {
    printShaderLog(shader);
    return GL_ZERO;
  }
  return shader;
}

GLuint create_program(const GLchar ** vertexShaderSource,
                      const GLchar ** fragmentShaderSource)
{
  GLuint programID = glCreateProgram();

  //Create vertex shader
  GLuint vertexShader = create_shader(GL_VERTEX_SHADER, vertexShaderSource);

  if (vertexShader == GL_ZERO)
  {
    printf("Unable to compile vertex shader %d!\n", vertexShader);
    return GL_ZERO;
  }

  //Attach vertex shader to program
  glAttachShader(programID, vertexShader);

  //Create fragment shader
  GLuint fragmentShader = create_shader(GL_FRAGMENT_SHADER,
                                        fragmentShaderSource);

  if(fragmentShader == GL_ZERO)
  {
    printf("Unable to compile fragment shader %d!\n", fragmentShader);
    return GL_ZERO;
  }

  //Attach fragment shader to program
  glAttachShader(programID, fragmentShader);


  //Link program
  glLinkProgram(programID);

  //Check for errors
  GLint programSuccess = GL_TRUE;
  glGetProgramiv(programID, GL_LINK_STATUS, &programSuccess);
  if(programSuccess != GL_TRUE)
  {
    printf("Error linking program %d!\n", programID);
    printProgramLog(programID);
    return GL_ZERO;
  }

  return programID;
}

// Initialise the graphics subsystem. This is pretty much boiler plate
// code with very little to worry about.
SDL_Window * init_graphics()
{
    // Initialise SDL
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0) {
        // std::cerr << "Failed to initialise video" << std::endl << std::flush;
        return nullptr;
    }

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window * screen;

    // Create the window
    // screen = SDL_SetVideoMode(screen_width, screen_height, 0, SDL_OPENGL);
    screen = SDL_CreateWindow("Calamari Dalasie",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              screen_width, screen_height,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (screen == nullptr) {
        // std::cerr << "Failed to set video mode" << std::endl << std::flush;
        SDL_Quit();
        return nullptr;
    }

    SDL_GLContext context;

    context = SDL_GL_CreateContext(screen);

    GLenum e = glewInit();
    if (e != GLEW_OK)
    {
        fprintf(stderr, "GLEW fail! %s\n", glewGetErrorString(e));
        SDL_Quit();
        return nullptr;
    }

  SDL_GL_SetSwapInterval(0);

  //Get vertex source
  const GLchar* vertexShaderSource[] =
  {
    R"glsl(
#version 120
varying vec3 normal;
varying vec3 vertex_to_light_vector;
void main() {
  vec3 normal, lightDir;
  vec4 diffuse, ambient;
  float NdotL;

  /* first transform the normal into eye space and normalize the result */
  normal = normalize(gl_NormalMatrix * gl_Normal);

  /* now normalize the light's direction. Note that according to the
  OpenGL specification, the light is stored in eye space. Also since
  we're talking about a directional light, the position field is actually
  direction */
  lightDir = normalize(vec3(gl_LightSource[1].position));

  /* compute the cos of the angle between the normal and lights direction.
  The light is directional so the direction is constant for every vertex.
  Since these two are normalized the cosine is the dot product. We also
  need to clamp the result to the [0,1] range. */
  NdotL = max(dot(normal, lightDir), 0.0);

  /* Compute the diffuse term */
  diffuse = gl_FrontMaterial.diffuse * gl_LightSource[1].diffuse;
  ambient = gl_FrontMaterial.ambient * gl_LightSource[1].ambient;
  gl_FrontColor =  NdotL * diffuse + ambient;
  gl_Position = ftransform();
}
)glsl"
  };

  //Get fragment source
  const GLchar* fragmentShaderSource[] =
  {
    R"glsl(
#version 120
void main() {
  gl_FragColor = gl_Color;
}
)glsl"
  };

  gProgramID = create_program(vertexShaderSource, fragmentShaderSource);
  if (gProgramID == GL_ZERO)
  {
    return nullptr;
  }
#if 0
  //Get vertex attribute location
  gVertexPos2DLocation = glGetAttribLocation(gProgramID, "LVertexPos2D");
  if(gVertexPos2DLocation == -1)
  {
    printf("LVertexPos2D is not a valid glsl program variable!\n");
    return nullptr;
  }
#endif

  const GLchar* textVertexSource[] =
  {
    R"glsl(
#version 120
void main() {
  gl_TexCoord[0] = gl_MultiTexCoord0;
  gl_Position = ftransform();
}
)glsl"
  };

  const GLchar* textFragmentSource[] =
  {
    R"glsl(
#version 120
uniform sampler2D tex;
void main() {
  vec4 white = vec4(1.0, 1.0, 1.0, 1.0);
  gl_FragColor = white * texture2D(tex, gl_TexCoord[0].st).a;
}
)glsl"
  };

  gTextProgID = create_program(textVertexSource, textFragmentSource);
  if (gTextProgID == GL_ZERO)
  {
    return nullptr;
  }

    // Setup the viewport transform
    glViewport(0, 0, screen_width, screen_height);

    // Enable vertex arrays
    glEnableClientState(GL_VERTEX_ARRAY);
    // Texture coordinate arrays well need to be enabled _ONLY_ when using
    // texture coordinates from an array, and disabled afterwards.
    // glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    // Set the colour the screen will be when cleared - black
    glClearColor(0.0, 0.0, 0.0, 0.0);

    GLfloat ambient_colour[] = {0.4f, 0.4f, 0.4f, 1.f};
    GLfloat diffuse_colour[] = {1.f, 1.f, 1.00, 1.f};

    glLightfv(GL_LIGHT1, GL_AMBIENT, ambient_colour);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse_colour);

    glEnable(GL_CULL_FACE);

    // Initialise the texture used for rendering text
    glGenTextures(1, &textTexture);
    glBindTexture(GL_TEXTURE_2D, textTexture);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexImage2D(GL_TEXTURE_2D, 0, texture_font_internalFormat,
                 texture_font_width, texture_font_height, 0,
                 texture_font_format, GL_UNSIGNED_BYTE, texture_font_pixels);
    if (glGetError() != 0) {
        return nullptr;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    textBase = glGenLists(256);
    float vertices[] = { 0, 0, 16, 0, 16, 16, 0, 16 };
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    int loop;
    for(loop=0; loop<256; loop++) {
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

    static const float vertexData[] = {
        0.f, 0.f, 0.f,
        1.f, 0.f, 0.f,
        1.f, 1.f, 0.f,
        0.f, 1.f, 0.f,
        0.f, 0.f, 1.f,
        1.f, 0.f, 1.f,
        1.f, 1.f, 1.f,
        0.f, 1.f, 1.f,
    };
    //Create VBO
    glGenBuffers(1, &gVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 3 * 8 * sizeof(GLfloat),
                 vertexData,
                 GL_STATIC_DRAW);

    static const GLuint indexData[] = {
        4, 5, 7, 6,
        0, 3, 1, 2,
        0, 4, 3, 7,
        1, 2, 5, 6,
        2, 3, 6, 7,
        0, 1, 4, 5
    };
    //Create IBO
    glGenBuffers(1, &gIBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 24 * sizeof(GLuint),
                 indexData,
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_ZERO);
    glBindBuffer(GL_ARRAY_BUFFER, GL_ZERO);

    // Set the projection transform
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float s = ((float)screen_width / (float)screen_height) * 3.0f / 8.0f;
    glFrustum(-s, s, -0.375f, 0.375f, 0.65f, 100.f);

    // Set up the modelview
    glMatrixMode(GL_MODELVIEW);
    // Reset the camera
    glLoadIdentity();
    // Set the camera position
    camera_pos();

    GLfloat lightPos[] = {0.f, 0.f, 1.f, 0.f};
    glLightfv(GL_LIGHT1, GL_POSITION, lightPos);

    return screen;
}

// Clear the grid state.
void clear()
{
    int i, j;
    for(i = 0; i < grid_width; ++i) {
        for(j = 0; j < grid_height; ++j) {
            properties[i][j].block = false;
        }
    }
}

// Print a text string on the screen at the current position.
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

void level(float factor)
{
    Block ** p = &blocks;
    for (; *p != 0; p = &((*p)->next));

    int i, j;
    for (i = -grid_width; i < grid_width; ++i) {
        for (j = -grid_height; j < grid_height; ++j) {
            Block * b = new Block;
            b->x = (i / 2.f + uniform(-0.5f, 0.5f)) * factor;
            b->y = (j / 2.f + uniform(-0.5f, 0.5f)) * factor;
            b->z = 0;
            b->diffuse[0] = uniform(0.f, 1.f);
            b->diffuse[1] = uniform(0.f, 1.f);
            b->diffuse[2] = uniform(0.f, 1.f);
            b->diffuse[3] = 1.f;
            b->scale = logarithmic(0.05, 0.5) * factor;
            b->present = 0;
            b->next = nullptr;
            if ((b->x + b->scale) > -factor / 2 && b->x < factor / 2 &&
                (b->y + b->scale) > -factor / 2 && b->y < factor / 2) {
                delete b;
                continue;
            }
            *p = b;
            p = &b->next;
        }
    }
}

void trim()
{
    float min_size = scale / 100.f;
    Block * b = blocks;
    while (b->next != 0 && b->scale < min_size) {
        Block * current = b;
        printf("Deleting %f\n", b->scale);
        b = b->next;
        delete current;
    }
    blocks = b;
}

void setup()
{
    // Clear the block store
    clear();

    quaternion_init(&orientation);

    level(1);
    level(10);
}

#define BUFFER_OFFSET(bytes) ((GLubyte*) nullptr + (bytes) * sizeof(GLuint))

void draw_unit_cube()
{
    glNormal3f(0,0,1);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, BUFFER_OFFSET(0));

    glNormal3f(0,0,-1);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, BUFFER_OFFSET(4));

    glNormal3f(-1,0,0);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, BUFFER_OFFSET(8));

    glNormal3f(1,0,0);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, BUFFER_OFFSET(12));

    glNormal3f(0,1,0);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, BUFFER_OFFSET(16));

    glNormal3f(0,-1,0);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, BUFFER_OFFSET(20));
}

void draw_grid()
{
    int i, j;
    float horizontal_line_vertices[] = {
        0.f, 0.f, 0.f,
        grid_width, 0.f, 0.f,
    };
    float vertical_line_vertices[] = {
        0.f, 0.f, 0.f,
        0.f, grid_height, 0.f,
    };

    glColor3f(0.3f, 0.3f, 0.3f);
    glNormal3f(0.f, 0.f, 1.f);

    // Move to the origin of the grid
    glTranslatef(-(float)grid_width/2.0f, -(float)grid_height/2.0f, 0.0f);
    // Store this position
    glPushMatrix();

    // Draw vertical lines
    glVertexPointer(3, GL_FLOAT, 0, vertical_line_vertices);
    for (i = 0; i <= grid_width; ++i) {
        glDrawArrays(GL_LINES, 0, 2);
        glTranslatef(1.0f, 0.0f, 0.0f);
    }

    // Reset to the origin
    glPopMatrix();
    glPushMatrix();

    // Draw horizontal lines
    glVertexPointer(3, GL_FLOAT, 0, horizontal_line_vertices);
    for (j = 0; j <= grid_height; ++j) {
        glDrawArrays(GL_LINES, 0, 2);
        glTranslatef(0.0f, 1.0f, 0.0f);
    }

    // Reset to the origin
    glPopMatrix();
}

float camera_rotation = 0.0f;

void grid_origin()
{
    glTranslatef(0, 0, -1);
    glScalef(1.f/scale, 1.f/scale, 1.f/scale);
    // Add a little camera movement
    // glRotatef(10, sin(camera_rotation), cos(camera_rotation), 0.0f);
    glTranslatef(-pos_x, -pos_y, -pos_z);
}

void camera_pos()
{
    // Move the camera 20 units from the objects
    // and one unit above
    glTranslatef(0.0f, -1.0f, -10.0f);

    // Set the angle so we just can't see the horizon
    glRotatef(-65, 1, 0, 0);
    glRotatef(angle, 0, 0, 1);

}

void render_scene()
{
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Enable the depth test
    glEnable(GL_DEPTH_TEST);

    // Set the projection transform
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float s = ((float)screen_width / (float)screen_height) * 3.0f / 8.0f;
    glFrustum(-s, s, -0.375f, 0.375f, 0.65f, 100.f);

    // Set up the modelview
    glMatrixMode(GL_MODELVIEW);
    // Reset the camera
    glLoadIdentity();
    // Set the camera position
    camera_pos();

    glPushMatrix();

    GLfloat matrix[16];
    quaternion_rotmatrix(&orientation, matrix);
    glMultMatrixf(matrix);

    glUseProgram(gProgramID);

    glBindBuffer(GL_ARRAY_BUFFER, gVBO);
    glVertexPointer(3, GL_FLOAT, 0, nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);

    glPushMatrix();
    glScalef(.2f/scale, .2f/scale, .2f/scale);
    glTranslatef(-0.5f, -0.5f, -0.5f);
    draw_unit_cube();
    glPopMatrix();

    Block * b;
    for (b = blocks; b != nullptr; b = b->next) {
        if (b->present != 1) {
            continue;
        }
        glPushMatrix();
        quaternion_rotmatrix(&b->orientation, matrix);
        glMultMatrixf(matrix);
        glScalef(1/scale, 1/scale, 1/scale);
        glTranslatef(b->x, b->y, b->z);
        glScalef(b->scale, b->scale, b->scale);
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, b->diffuse);
        draw_unit_cube();
        glPopMatrix();
        
    }

    glPopMatrix();

    grid_origin();


    for (b = blocks; b != nullptr; b = b->next) {
        if (b->present != 0) {
            continue;
        }
        glPushMatrix();
        glTranslatef(b->x, b->y, 0);
        glScalef(b->scale, b->scale, b->scale);
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, b->diffuse);
        draw_unit_cube();
        glPopMatrix();
        
    }

    glBindBuffer(GL_ARRAY_BUFFER, GL_ZERO);

    static float white[] = { 1.f, 1.f, 1.f, 1.f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, white);

    // Draw the scene
    draw_grid();

}

// Draw any text output and other screen oriented user interface
// If you want any kind of text or other information overlayed on top
// of the 3d view, put it here.
void render_interface()
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

    glUseProgram(gTextProgID);

    // Print the number of frames per second. This is essential performance
    // information when developing 3D graphics.

    glPushMatrix();
    // Use glTranslatef to go to the screen coordinates where we want the
    // text. The origin is the bottom left by default in OpenGL.
    glTranslatef(5.f, 5.f, 0);
    sprintf(buf, "FPS: %d", average_frames_per_second);
    gl_print(buf);
    glPopMatrix();

    glTranslatef(5.f, screen_height - 16 - 5, 0);
    int metres = floor(scale);
    int centimetres = floor(fmod(scale, 1) * 100.f);
    int milimetres = floor(fmod(scale, .01) * 1000.f);
    sprintf(buf, "%dm %dcm %dmm", metres, centimetres, milimetres);
    gl_print(buf);
}

// Handle a mouse click. Call this function with the screen coordinates where
// the mouse was clicked, in OpenGL format with the origin in the bottom left.
// This function exactly mimics rendering a scene, and then detects which
// grid location of the scene were under the mouse pointer when the click
// occured.
void mouse_click(unsigned int x, unsigned int y)
{
// This code needs to be ported to fix dependency on glu
#if 0
    int i, j;
    GLuint selectBuf[512];
    GLfloat square_vertices[] = { 0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
                                  1.f, 1.f, 0.f, 0.f, 1.f, 0.f };

    // Put OpenGL into Selection mode rather than Render mode. This disables
    // rendering to the window.
    glSelectBuffer(512,selectBuf);
    glRenderMode(GL_SELECT);

    
    // This is just like setting up the projection for normal rendering
    // except we use gluPickMatrix to specify the portion of the screen
    // we are interested in - the 1 pixel square under the mouse pointer.
    {
        // Set the projection transform
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT,viewport);
        gluPickMatrix(x, y, 1, 1, viewport);
        gluPerspective(45, (float)screen_width/screen_height, 1.f, 100.f);

        // Set the camera position
        camera_pos();
        grid_origin();
    }

    // Each thing we render will have a numerical name that we must later use
    // to identify which thing we rendered was under the mouse pointer.
    // The names are pushed onto a stack, so a thing can have more than
    // one. We will just have one name per thing, so we start by pushing a
    // token name onto the stack.
    glInitNames();
    glPushName(0);

    // Here we will draw the grid exactly as it was drawn when rendering to
    // the screen, except we draw a quad on each square rather than just
    // drawing lines. If you want to detect clicking on other things, you
    // will need to modify the code here.
    glTranslatef(-(float)grid_width/2.0f, -(float)grid_height/2.0f, 0.0f);

    glVertexPointer(3, GL_FLOAT, 0, square_vertices);

    for(i = 0; i < grid_width; ++i) {
        for(j = 0; j < grid_height; ++j) {
            // Before we render each grid square, we specify the name by
            // loading it into the top of the name stack. The name
            // is derived from the coordinates of the square, so that later
            // when we get a name back, we can tell which square it corresponds
            // to.
            glLoadName(i + j * grid_width);
            glDrawArrays(GL_QUADS, 0, 4);
            glTranslatef(0.0f, 1.0f, 0.0f);
        }
        glTranslatef(1.0f, -grid_height, 0.0f);
    }

    // We have now finished with the name stack, so we empty it.
    glPopName();

    // Put OpenGL back into rendering mode. This has the side effect of
    // returning how many hits we got in the selection code above.
    int hits = glRenderMode(GL_RENDER);

    // std::cout << "Got " << hits << " hits" << std::endl << std::flush;

    // If we got no hits, the user clicked on empty space.
    if (hits == 0) {
        return;
    }

    // This code searches through the results buffer to find what hits we
    // got.

    // ptr points to the beginning of the results
    GLuint * ptr = &selectBuf[0];
    GLuint minDepth = std::numeric_limits<GLuint>::max(), noNames = 0;
    GLuint * namePtr = 0;
    for (i = 0; i < hits; i++) {
        int names = *(ptr++);
        // std::cout << "{" << *ptr << "}";
        // Check if this hit is closer to the viewer than the last one we
        // processed. If this test passes, then this is the closest hit
        // so far, so we record the necessary details.
        if (*ptr < minDepth) {
            noNames = names;
            minDepth = *ptr;
            namePtr = ptr + 2;
        }
        ptr += (names + 2);
    }
    // Once this loop is complete, we have the details of the nearest hit
    // stored, which is the thing we rendered that was closest to the viewer
    // under the mouse pointer. This should be the thing the player was aiming
    // to click on when they clicked.

    // noNames = Number of names our hit has
    // namePtr = Pointer to list of names

    // Extract the X and Y coords of the clicked square from the hit name
    GLuint hitName = *namePtr;
    int hit_x = hitName % grid_width;
    int hit_y = hitName / grid_width;

    // Place or remove a block on the square the user clicked.
    if (hit_x < grid_width && hit_y < grid_height) {
        properties[hit_x][hit_y].block = !properties[hit_x][hit_y].block;
    }
#endif
}

// This function is called every step_time milliseconds. In many games you
// will want to use this function to update the game state. For example in
// a Tetris game, this would be the place where the falling blocks were
// moved down the grid.
void step()
{
}

void update(float delta)
{
    static bool flipped = false;
    bool vel_changed = false;
    bool braking = false;
    float ang_rad = (angle / 180) * M_PI;
    // Direction camera is facing
    float forwards[] = { std::sin(ang_rad),   std::cos(ang_rad) };
    float sideways[] = { std::cos(ang_rad), - std::sin(ang_rad) };
    // Speed in the camera direction
    float speed = vector2_dot(velocity, forwards);
    float drift = vector2_dot(velocity, sideways);
    static float support = 0;

    // printf("Velocity (%f,%f), Direction (%f,%f), Speed %f, Forward %f\n",
           // velocity[0], velocity[1], forwards[0], forwards[1], speed,
           // vector2_dot(velocity, forwards));

    // Velociy should be a vector, so speed in facing direction can
    // be determined by the dot product of velocity in the camera direction
    // and this value used to determine what constraints should be placed on
    // changes in speed, such as whether accelerating or braking, and maximums.
    // Once vel is a vector, similar techniques can be used to handle sideways
    // velocity, then finall deflection can be handled.

    if (key_lf) {
        if (key_rf) {
            if (!(key_lb || key_rb)) {
                // accelerate forwards
                if (speed > 0) {
                    speed += delta * max_accel;
                } else {
                    speed += delta * max_decel;
                    braking = true;
                }
                vel_changed = true;
            }
        } else {
            if (key_lb) {
                printf("Roll left\n");
                drift = -1;
            } else {
                if (key_rb) {
                    // rotate right
                    angle += delta * 50;
                } else {
                    // coast right
                    angle += delta * 20;
                    if (speed > 0) {
                        speed += delta * max_accel / 2;
                    } else {
                        speed += delta * max_decel / 2;
                    }
                    vel_changed = true;
                }
            }
        }
    } else {
        if (key_rf) {
            if (key_rb) {
                printf("Roll right\n");
                drift = 1;
            } else {
                if (key_lb) {
                    // rotate left
                    angle -= delta * 50;
                } else {
                    // coast left
                    angle -= delta * 20;
                    if (speed > 0) {
                        speed += delta * max_accel / 2;
                    } else {
                        speed += delta * max_decel / 2;
                    }
                    vel_changed = true;
                }
            }
        } else {
            if (key_lb) {
                if (key_rb) {
                    // reverse
                    if (speed < 0) {
                        speed -= delta * max_accel;
                    } else {
                        speed -= delta * max_decel;
                        braking = true;
                    }
                    vel_changed = true;
                } else {
                    // reverse coast left
                    angle -= delta * 20;
                    if (speed < 0) {
                        speed -= delta * max_accel / 2;
                    } else {
                        speed -= delta * max_decel / 2;
                    }
                    vel_changed = true;
                }
            } else if (key_rb) {
                if (!key_lb) {
                    // reverse coast right
                    angle += delta * 20;
                    if (speed < 0) {
                        speed -= delta * max_accel / 2;
                    } else {
                        speed -= delta * max_decel / 2;
                    }
                    vel_changed = true;
                }
            }
        }
    }
    if (key_flip) {
        if (!flipped) {
            angle += 180;
            speed = -speed;
            vel_changed = true;
            flipped = true;
        }
    } else {
        flipped = false;
    }

    if (vel_changed) {
        // If the controls have had an effect on velocity, clamp it to
        // the valid range
        speed = fmaxf(speed, -max_velocity);
        speed = fminf(speed, max_velocity);
    } else {
        // Otherwise coast gently to a stop
        if (speed < 0.f) {
            speed += delta;
            speed = fminf(speed, 0.f);
        } else {
            speed -= delta;
            speed = fmaxf(speed, 0.f);
        }
    }
    if (drift < 0.f) {
        drift += delta;
        drift = fminf(drift, 0.f);
    } else {
        drift -= delta;
        drift = fmaxf(drift, 0.f);
    }

    float new_ang_rad = (angle / 180) * M_PI;

    velocity[0] = sin(new_ang_rad) * speed + cos(ang_rad) * drift;
    velocity[1] = cos(new_ang_rad) * speed - sin(ang_rad) * drift;

    // float axis[] = { -1, 0, 0 };

    pos_x += velocity[0] * delta * scale;
    pos_y += velocity[1] * delta * scale;
    pos_z += velocity[2] * delta * scale;

    // For a unit sphere, distance rolled is equal to angle rolled in
    // radians
    if (!braking) {
        float mag = hypotf(velocity[0], velocity[1]);
        if (mag > 0.f) {
            float axis[3];

            axis[0] =   velocity[1] / mag;
            axis[1] = - velocity[0] / mag;
            axis[2] = 0;
            printf("(%f,%f) %f\n", axis[0], axis[1], mag);
            orientation = quaternion_rotate(&orientation, axis, -mag * delta);
        }
    }

    // scale *= (1 + (delta * 0.01f));
    bool climbing = false;
    support = 0;

    Block * b;
    for (b = blocks; b != nullptr; b = b->next) {
        bool collision = false;
        if (b->present != 0) {
            continue;
        }
        float bx = b->x + b->scale / 2.f;
        float by = b->y + b->scale / 2.f;
        if (pos_x < (b->x + b->scale + scale) &&
            pos_x > (b->x - scale) &&
            pos_y < (b->y + b->scale + scale) &&
            pos_y > (b->y - scale)) {
            support = fmaxf(support, b->scale);
            if (pos_z < b->scale) {
                printf("%f, %f\n", pos_z, b->scale);
                if (pos_y < (b->y + b->scale) &&
                    pos_y > (b->y)) {
                    collision = true;
                    printf("COLY\n");
                }
                if (pos_x < (b->x + b->scale) &&
                    pos_x > (b->x)) {
                    collision = true;
                    printf("COLX\n");
                }
            }
        }
        if (sqrt(square(pos_x - (b->x + b->scale / 2)) +
                 square(pos_y - (b->y + b->scale / 2)) +
                 square(pos_z + scale - (b->scale / 2))) < (scale + b->scale / 2)) {
            collision = true;
            printf("COLS\n");
        }
        if (!collision) {
            continue;
        }
        if (b->scale > scale) {
            // printf("TOO BIG!\n");
            // FIXME collide
            if ((pos_z + scale / 8) >= b->scale) {
                // on top
            } else if (fabsf(pos_x - bx) < fabsf(pos_y - by)) {
                // bouncing y
                if (pos_y > by) {
                    if (velocity[1] < 0) {
                        printf("Bounce +x\n");
                        velocity[1] = -velocity[1];
                        if (velocity[1] < 0.2) {
                            climbing = true;
                        }
                    }
                } else {
                    if (velocity[1] > 0) {
                        printf("Bounce -x\n");
                        velocity[1] = -velocity[1];
                        if (velocity[1] > -0.2) {
                            climbing = true;
                        }
                    }
                }
            } else {
                // bouncing x
                if (pos_x > bx) {
                    if (velocity[0] < 0) {
                        printf("Bounce +y\n");
                        velocity[0] = -velocity[0];
                        if (velocity[0] < 0.2) {
                            climbing = true;
                        }
                    }
                } else {
                    if (velocity[0] > 0) {
                        printf("Bounce -y\n");
                        velocity[0] = -velocity[0];
                        if (velocity[0] > -0.2) {
                            climbing = true;
                        }
                    }
                }
            }
            printf("Climbing %d\n", climbing);
            continue;
        }
        b->orientation = orientation;
        quaternion_invert(&b->orientation);
        b->x = b->x-pos_x;
        b->y = b->y-pos_y;
        b->z = -(pos_z + scale);
        b->present = 1;
        // scale === ball_radius
        printf("B %f\n", scale);
        scale = powf(cube(scale) + cube(b->scale) / (M_PI * 4.f / 3.f), 1.f/3.f);
        printf("A %f\n", scale);
    }
    printf("P %f %f\n", pos_z, support);
    if (climbing) {
        if (pos_z < support) {
            velocity[2] = 1;
        }
    } else {
        if (pos_z > support) {
            // If we are above solid surface, fall towards it
            velocity[2] -= 9.8 * delta;
        } else if (velocity[2] < 0) {
            // If we are not above, but still falling, bounce
            velocity[2] = -0.7 * velocity[2];
        } else {
            // Otherwise we are below and rising, in which case we must
            // behave under gravity, but velocity must not go negative
            velocity[2] -= 9.8 * delta;
            if (velocity[2] < 0.f) {
                velocity[2] = 0;
            }
        }
        printf("V %f %f\n", velocity[2], 9.8 * delta);
    }

    if (scale > next_level) {
        level(next_level * 100);
        trim();
        next_level *= 10;
    }
    // printf("%f %f\n", scale, log10(scale));
}

// The main program loop function. This does not return until the program
// has finished.
void loop(SDL_Window * screen)
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
                    }
                    if ( event.key.keysym.sym == SDLK_DOWN ) {
                    }
                    if ( event.key.keysym.sym == SDLK_LEFT ) {
                    }
                    if ( event.key.keysym.sym == SDLK_RIGHT ) {
                    }
                    if ( event.key.keysym.sym == SDLK_d ) {
                        key_lf = true;
                    }
                    if ( event.key.keysym.sym == SDLK_c ) {
                        key_lb = true;
                    }
                    if ( event.key.keysym.sym == SDLK_k ) {
                        key_rf = true;
                    }
                    if ( event.key.keysym.sym == SDLK_m ) {
                        key_rb = true;
                    }
                    if ( event.key.keysym.sym == SDLK_SPACE ) {
                        key_flip = true;
                    }
                    break;
                case SDL_KEYUP:
                    if ( event.key.keysym.sym == SDLK_d ) {
                        key_lf = false;
                    }
                    if ( event.key.keysym.sym == SDLK_c ) {
                        key_lb = false;
                    }
                    if ( event.key.keysym.sym == SDLK_k ) {
                        key_rf = false;
                    }
                    if ( event.key.keysym.sym == SDLK_m ) {
                        key_rb = false;
                    }
                    if ( event.key.keysym.sym == SDLK_SPACE ) {
                        key_flip = false;
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

        ++frame_count;

        // Get the time and check if a complete time step has passed.
        // For step based games like Tetris, this is used to update the
        // the game state
        const int ticks = SDL_GetTicks();
        if ((ticks - last_step) > step_time) {
            last_step = ticks;
            average_frames_per_second = frame_count;
            frame_count = 0;
            step();
        }

        // Calculate the time in seconds since the last frame
        // For a real time program this would be used to update the game state
        int frame_ticks = (ticks - elapsed_time);
        if (frame_ticks > 0) {
            float delta = frame_ticks / 1000.0f;
            update(delta);
            elapsed_time = ticks;

            // Update the rotation on the camera
            camera_rotation += delta;
            if (camera_rotation > (2 * M_PI)) {
                camera_rotation -= (2 * M_PI);
            }
        }

        // Render the screen
        render_scene();
        render_interface();

        SDL_GL_SwapWindow(screen);
    }
}

int main()
{
    // Initialise the graphics
    SDL_Window * screen = init_graphics();
    if (screen == nullptr) {
        return 1;
    }

    // Intialise the game state
    setup();

    // Run the game
    loop(screen);
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
