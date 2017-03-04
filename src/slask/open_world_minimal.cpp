// Lab 4, terrain generation
#ifdef __APPLE__
#include <OpenGL/gl3.h>
// Linking hint for Lightweight IDE
// uses framework Cocoa
#endif
#include "MicroGlut.h"
#include "GL_utilities.h"
#include "LoadTGA.h"
#include "camera.hpp"
#include "autogen.hpp"
#include "tex.hpp"
#include "mesh.hpp"
#include "util.hpp"
#include <cmath>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

std::vector<GLfloat> height_map;
cd::Mesh terrain_mesh;
GLuint map_size;

cd::GLMCamera camera;

// References to shader programs
GLuint shader;

int window_width = 600;
int window_height = 600;

float mouseX, mouseY;
float moveZ, moveX;
void updateKey(unsigned char event, int up)
{ 
    switch(event)
    {
        case 'w': moveZ = up ? 0 : -1; break;
        case 'a': moveX = up ? 0 : -1; break; 
        case 's': moveZ = up ? 0 : 1; break;
        case 'd': moveX = up ? 0 : 1; break; 
        case 'q': glutToggleFullScreen(); break;
    }
}
void keyboardCallback(unsigned char event, int x, int y)
{
    updateKey(event, 0);
}
void keyboardCallbackRelease(unsigned char event, int x, int y)
{
    updateKey(event, 1);
}

void mouseCallback(int mx, int my) {
    int w,h;
    glutGetWindowSize(&w, &h);
    mx -= w/2;
    my -= h/2;
    float wf = ((float)w) / 2.0f;
    float hf = ((float)h) / 2.0f;
    mouseX += ((float)mx) / wf; 
    mouseY += ((float)my) / hf; 
    if( (mx != 0) || (my != 0) )
        glutWarpPointer(wf,hf);
}

void init(void)
{
    // GL inits
    glClearColor(0,0,0,0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDepthFunc(GL_LESS);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    printError("GL inits");

    // Load and compile shader s
    shader = loadShaders("shaders/simple.vert","shaders/simple.frag");

    // Load terrain data
    map_size = 128;
    std::cout << "Generate height_map" << std::endl;
    height_map = cd::generateTexture2D(map_size, map_size, -static_cast<GLfloat>(map_size)/8, static_cast<GLfloat>(map_size)/8);
    std::cout << "Generate terrain_mesh" << std::endl;
    terrain_mesh = cd::generateTerrain(&height_map[0], map_size, map_size, 0.1f);
    terrain_mesh.toGPU();
    terrain_mesh.bindShader(
            shader,
            "inPosition",
            "", "", "", "");
    printError("terrain bindShader");

    camera.lookTowards(
            glm::vec3(map_size/2,map_size/2,map_size/2),
            glm::vec3(1,-1,0));
    camera.bindShader(shader, "projMatrix", "mdlMatrix");
    camera.bindShader(shader, "projMatrix", "mdlMatrix");
}

void updateCamera() {

    float rot_vel = 1.0;
    camera.rotateLocal(rot_vel * mouseY, rot_vel * mouseX);
    camera.translateLocal(moveX, 0, moveZ);      
    mouseX = 0;
    mouseY = 0;
}

void display(void)
{
    printError("pre display");

    updateCamera();
    printError("updateCamera");

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_CULL_FACE);
    terrain_mesh.bindShader(
            shader, 
            "inPosition",
            "inNormal",
            "",
            "",
            "");
    camera.bindShader(
            shader,
            "projMatrix",
            "mdlMatrix");

    terrain_mesh.draw();
    printError("display terrain");

    glutSwapBuffers();
}

void timer(int i)
{
    glutTimerFunc(20, &timer, i);
    glutPostRedisplay();
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitContextVersion(3, 2);
    glutInitWindowSize (window_width, window_height);
    glutCreateWindow ("TSBK07 Lab 4");
    glutDisplayFunc(display);
    init ();
    glutTimerFunc(20, &timer, 0);

    glutKeyboardFunc(keyboardCallback);
    glutKeyboardUpFunc(keyboardCallbackRelease);
    glutPassiveMotionFunc(mouseCallback);

    glutMainLoop();
    exit(0);
}
