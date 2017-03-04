// Lab 4, terrain generation
#ifdef __APPLE__
#include <OpenGL/gl3.h>
// Linking hint for Lightweight IDE
// uses framework Cocoa
#endif
#include "MicroGlut.h"
#include "GL_utilities.h"
#include "VectorUtils3.h"
#include "loadobj.h"
#include "LoadTGA.h"
#include "camera.hpp"
#include "autogen.hpp"
#include "tex.hpp"
#include "mesh.hpp"
#include <cmath>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <sstream>
#include "util.hpp"
#include <Eigen/SparseCore>

std::vector<GLfloat> height_map;
GLuint map_size;

cd::GLMCamera camera;

// References to shader programs
GLuint radiosity_shader;
GLuint index_shader;

int window_width = 600;
int window_height = 600;
GLuint fbo, rbo;
GLuint renderTex;

cd::Mesh terrain_mesh;
cd::Mesh terrain_mesh_dual;

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

#define FBO_PRINT(X) std::cout << "FBOError :: " << #X << std::endl;
GLenum checkFBO()
{
    GLenum status;
    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch(status)
    {
        case GL_FRAMEBUFFER_COMPLETE: std::cout << "FBO ok" << std::endl; break;
        case GL_FRAMEBUFFER_UNDEFINED: FBO_PRINT(GL_FRAME_BUFFER_UNDEFINED); break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: FBO_PRINT(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: FBO_PRINT(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT); break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: FBO_PRINT(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER); break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: FBO_PRINT(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER); break;
        case GL_FRAMEBUFFER_UNSUPPORTED: FBO_PRINT(GL_FRAMEBUFFER_UNSUPPORTED); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: FBO_PRINT(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE); break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS : FBO_PRINT(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS); break;
        default: FBO_PRINT(Unknown error);
    }
    std::cout << "FBO status = " << status << std::endl;
    return status;
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
    radiosity_shader = loadShaders("shaders/radiosity.vert", "shaders/radiosity.frag");
    index_shader = loadShaders("shaders/index.vert", "shaders/index.frag");

    // Load terrain data
    map_size = 16;
    std::cout << "Generate height_map" << std::endl;
    height_map = cd::generateTexture2D(map_size, map_size, -static_cast<GLfloat>(map_size)/8, static_cast<GLfloat>(map_size)/8);
    std::cout << "Generate terrain_mesh" << std::endl;
    terrain_mesh = cd::generateTerrain(&height_map[0], map_size, map_size, 0.1f);
    terrain_mesh.toGPU();
    terrain_mesh.bindShader(
            radiosity_shader,
            "inPosition",
            "", "", "", "");
    printError("terrain bindShader");

    std::cout << "Creating terrain dual mesh" << std::endl;
    terrain_mesh_dual = terrain_mesh.dual();
    printError("terrain_mesh_dual");

    // Collect radiosity data
    terrain_mesh.bindShader(
            index_shader, 
            "inPosition",
            "inNormal",
            "",
            "",
            "");

    camera.lookTowards(
            //glm::vec3(map_size/2,map_size/2,-map_size/2),
            glm::vec3(map_size/2,map_size/2,map_size/2),
            glm::vec3(1,-1,0));
    camera.bindShader(radiosity_shader, "projMatrix", "mdlMatrix");
    camera.bindShader(index_shader, "projMatrix", "mdlMatrix");

    if( true )
    {
        glGenFramebuffers(1,&fbo);
        printError("glGenFramebuffers");
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        printError("glBindFramebuffer");
        glGenTextures(1, &renderTex);
        glBindTexture(GL_TEXTURE_2D, renderTex);
        glTexImage2D(GL_TEXTURE_2D,0,GL_R32I,window_width, window_height,0,GL_RED_INTEGER,GL_INT,NULL);
        printError("glTexImage2D");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glGenRenderbuffers(1,&rbo);
        printError("glGenRenderbuffers");
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        printError("glBindRenderbuffer");
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window_width, window_height);
        printError("glRenderbufferStorage");
        glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
        printError("glFramebufferRenderbuffer");

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTex, 0);
        printError("glFrameBufferTexture2D");

        GLenum draw_buffers[1] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, draw_buffers);
        printError("glDrawBuffers");

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER,fbo);

        std::cout << "FBO = " << fbo << std::endl;
        std::cout << "renderTex = " << renderTex << std::endl;
        GLenum fbo_status = checkFBO();
        if( fbo_status != GL_FRAMEBUFFER_COMPLETE )
            exit(1);

        GLuint num_faces = terrain_mesh_dual.m_vertex_array.size();
        std::vector<std::int32_t> data(window_width*window_height,0);
        std::vector<bool> face_seen(num_faces);
        std::vector<std::vector<std::uint32_t> > histogram(num_faces, std::vector<std::uint32_t>(num_faces,0));
        std::vector<glm::vec3> view_dirs(4);
        view_dirs[0] = glm::vec3(0,0,1);
        view_dirs[1] = glm::vec3(0,0,-1);
        view_dirs[2] = glm::vec3(1,0,0);
        view_dirs[3] = glm::vec3(-1,0,0);

        Eigen::SparseMatrix<std::uint32_t> RadiosityFactorMatrix(num_faces, num_faces);

        std::vector<Eigen::Triplet<std::int32_t> > triplet_list; 

        std::cout << "histogram size = " << histogram.size() << ", histogram[0].size() = " << histogram[0].size() << std::endl;

        for(GLuint i = 0; i < num_faces; ++i)
        {
            for(GLuint j = 0; j < view_dirs.size(); ++j)
            {
                std::cout << 100*((float)i)/num_faces << std::endl;
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glm::vec3 v = terrain_mesh_dual.m_vertex_array[i];
                //glm::vec3 n = terrain_mesh_dual.m_normal_array[i];
                camera.lookTowards(v,view_dirs[j]);
                camera.bindShader(
                        index_shader,
                        "",
                        "mdlMatrix");
                glDisable(GL_CULL_FACE);
                glEnable(GL_DEPTH_TEST);
                terrain_mesh.draw();
                //glFinish();

                glReadBuffer(GL_COLOR_ATTACHMENT0);
                printError("glReadBuffer");
                glReadPixels(0,0,window_width,window_height,GL_RED_INTEGER,GL_INT,&data[0]);
                //glFinish(); 
                printError("glReadPixels");

                //int num_unique_inds = 0;
                std::fill(face_seen.begin(), face_seen.end(), false);
                for(GLuint k = 0; k < data.size(); ++k)
                {

                    std::int32_t face_index = data[k]-1; // -1 since all indices are shifted
                    if( face_index < 0 )
                        continue;

                    histogram[i][face_index] += 1;
                    /*
                    if( ! face_seen[face_index] )
                    {
                        num_unique_inds += 1;
                        face_seen[face_index] = true;
                    }
                    */
                }

                /*
                std::cout << "num_unqiue_inds = " << num_unique_inds << " / " << num_faces << std::endl;

                cv::Mat tmp(window_height,window_width,CV_32SC1,&data[0]);
                cv::flip(tmp,tmp,0);
                tmp.convertTo(tmp, CV_32FC1);
                double min,max;
                cv::Point locmin,locmax;
                cv::minMaxLoc(tmp, &min, &max, &locmin, &locmax);
                tmp = tmp / max;
                cv::imshow("readback",tmp);
                cv::waitKey(1);

                std::cout << "HISTOGRAM["<<i<<"]" << std::endl;
                for(int l = 0; l < num_faces; ++l)
                {
                    std::cout << histogram[i][l] << ", ";
                }
                std::cout << std::endl;
                //glutSwapBuffers();
                */
            }
        }
    }
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
    // clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    updateCamera();
    printError("updateCamera");

    glEnable(GL_CULL_FACE);
    terrain_mesh.bindShader(
            radiosity_shader, 
            "inPosition",
            "inNormal",
            "",
            "",
            "");
    camera.bindShader(
            radiosity_shader,
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
