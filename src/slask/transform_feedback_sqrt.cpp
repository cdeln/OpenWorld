
#include "gl_include.h"
#include "GL_utilities.h"
#include "MicroGlut.h"
#include <iostream>

int main(int argc, char ** argv)
{

    // load and compile shader
    //const GLchar* shader_src = "shaders/transform.vert";
    //
    glutInit(&argc, argv);
    //glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitContextVersion(3, 2);
    glutInitWindowSize (1,1);
    glutCreateWindow ("TransformFeedbackTest");
    //glutInitContextVersion(3, 2);
    
    GLint max_transform_components[1];
    glGetIntegerv(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, max_transform_components);
    std::cout << "max transform = " << max_transform_components[0] << std::endl;

    const GLchar* shader_src = R"glsl(
        in float inValue;
        varying out float outValue;

        void main()
        {
            outValue = sqrt(inValue);
            gl_Position = vec4(0,0,0,1);
        }
        )glsl";

    GLuint shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(shader, 1, &shader_src, nullptr);
    glCompileShader(shader);
    std::cout << "shader = " << shader << std::endl;
    printError("compile");

    // attach shader to a proram
    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    std::cout << "program = " << program << std::endl;
    printError("attach");

    // configure program with transform feedback
    const GLchar* feedback_varyings[] = { "outValue" };
    glTransformFeedbackVaryings(program, 1, feedback_varyings, GL_INTERLEAVED_ATTRIBS);
    printError("feedback varyings");

    // link program
    GLint log_len;
    glLinkProgram(program);
    glUseProgram(program);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);
    std::cout << "loglen = " << log_len << std::endl;
    if( log_len > 2)
    {
        GLchar * info_log = new GLchar[log_len];
        glGetProgramInfoLog(program, 1000, &log_len, info_log);
        std::cout << info_log << std::endl;
        delete [] info_log;
    }
    printError("link");

    // vao
    GLuint vao;
    glGenVertexArrays(1,&vao);
    glBindVertexArray(vao);
    std::cout << "vao = " << vao << std::endl;

    // input vbo
    GLfloat data[5] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f }; 
    std::cout << "sizeof(data) = " << sizeof(data) << std::endl;
    for(GLuint i = 0; i < sizeof(data) / sizeof(GLfloat); ++i)
        std::cout << data[i] << ", ";
    std::cout << std::endl;

    GLuint vbo;
    glGenBuffers(1,&vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
    std::cout << "vbo = " << vbo << std::endl;

    GLint attrib_loc = glGetAttribLocation(program, "inValue");
    glEnableVertexAttribArray(attrib_loc);
    glVertexAttribPointer(attrib_loc, 1, GL_FLOAT, GL_FALSE, 0, 0);
    std::cout << "attrib_loc = " << attrib_loc << std::endl;
    printError("input vbo");

    // transform feedback output vbo
    GLuint tbo;
    glGenBuffers(1, &tbo);
    glBindBuffer(GL_ARRAY_BUFFER, tbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(data), nullptr, GL_STATIC_READ);
    std::cout << "tbo = " << tbo << std::endl;
    printError("tbo");

    glEnable(GL_RASTERIZER_DISCARD);
    printError("rasterize");
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, tbo); 
    printError("buffer base");

    // draw
    glBeginTransformFeedback(GL_POINTS);
    glDrawArrays(GL_POINTS, 0, 5);
    glEndTransformFeedback();
    glFlush();
    printError("draw");

    // read data
    GLfloat feedback[5];
    glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(feedback), feedback);
    printError("get buffer");

    std::cout << "sizeof(feedback) = " << sizeof(feedback) << std::endl;
    for(GLuint i = 0; i < sizeof(feedback) / sizeof(GLfloat); ++i)
        std::cout << feedback[i] << ", ";
};
