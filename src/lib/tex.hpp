#ifndef __TEX__
#define __TEX__

#include "gl_include.h" 
#include <string>
#include <vector>

namespace cd
{
    std::string cvType2Str(int cv_type);
    GLuint loadTexture2D(const std::string & path);
    GLuint loadTexture3D(std::vector<std::string> & paths);
    GLuint loadTexture3D(
            const std::vector<GLfloat> & tex,
            GLuint size_x,
            GLuint size_y,
            GLuint size_z);
}

#endif
