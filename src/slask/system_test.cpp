#include "gl_include.h"
#include "MicroGlut.h"
#include <string>
#include <iostream>

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);
    glutInitContextVersion(4, 2);
    glutInitWindowSize (100,100);
    glutCreateWindow ("System test");


    const GLubyte * version = glGetString(GL_SHADING_LANGUAGE_VERSION);
    std::cout << "Supported GLSL version is " << version << std::endl; 
    return 0;
}
