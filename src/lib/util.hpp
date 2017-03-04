#ifndef __UTIL__
#define __UTIL__

#include "gl_include.h"
#include <vector>
#include <string>

namespace cd
{

    /*
     * Template for binding and uploading array to GPU buffer
     */
    template <typename T>
        void arrayToBuffer(
                std::vector<T> & array,
                GLenum buffer_type,
                GLuint * vbo)
        {
            glGenBuffers(1, vbo);
            glBindBuffer(buffer_type, *vbo);
            glBufferData(buffer_type, sizeof(T)*array.size(), &array[0], GL_STATIC_DRAW);
        }

    void bindBufferAttribArray(
            GLuint buffer_obj_ID,
            GLuint shader_ID,
            const std::string attrib_name,
            GLuint attrib_size);
}

#endif
