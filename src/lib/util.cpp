
#include "util.hpp"

namespace cd
{

    /*
     * Binds a buffer to an attribute in a shader 
     */
    void bindBufferAttribArray(
            GLuint buffer_obj_ID,
            GLuint shader_ID,
            const std::string attrib_name,
            GLuint attrib_size)
    {
        glUseProgram(shader_ID);
        glBindBuffer(GL_ARRAY_BUFFER, buffer_obj_ID);
        GLint attrib_loc = glGetAttribLocation(shader_ID, attrib_name.c_str());
        glVertexAttribPointer(attrib_loc, attrib_size, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(attrib_loc);
    }
}
