#ifndef __APPLE__
#include "gl_include.h"
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

bool enable_debug_viz = false;

std::vector<GLfloat> height_map;
std::vector<GLfloat> snow_map;
std::vector<GLfloat> water_map;
GLuint map_size;
GLfloat glut_time;
cd::Camera camera;
cd::GLMCamera index_camera;

// References to shader programs
GLuint terrain_shader;
GLuint cloud_shader;
GLuint water_shader;
GLuint index_shader;
GLuint surface_viz_shader;
GLuint normal_viz_shader;
GLuint ball_shader;

int window_width = 600;
int window_height = 600;
GLuint fbo, rbo;
GLuint renderTex;

cd::Mesh terrain_mesh;
cd::Mesh terrain_mesh_dual;
cd::Mesh cloud_mesh;
cd::Mesh water_mesh;
cd::Mesh viz_mesh;

Model * ball_model;

long global_counter;

GLuint terrain_tex_objID[4];
GLuint cloud_tex_objID[1];

enum MESH_NUM {
    TERRAIN,
    CLOUD,
    WATER,
    VIZ,
    BALL,
    INDEX,
    MESH_NUM_END
};

bool enable_draw[MESH_NUM_END] = 
{
    true,
    false,
    false,
    false,
    false,
    true
};

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

void getTriangle(float posX, float posZ, int gridWidth, int * index0, int * index1, int * index2)
{
    int indX0 = static_cast<int>(std::round(posX)); //(int)(posX + 0.5f);
    int indZ0 = static_cast<int>(std::round(posZ)); //(int)(posZ + 0.5f);
    int diffX = indX0 < posX ? 1 : -1;
    int diffZ = indZ0 < posZ ? 1 : -1;

    *index0 = indX0 + indZ0 * gridWidth;
    *index1 = (indX0+diffX) + indZ0 * gridWidth;
    *index2 = indX0 + (indZ0+diffZ) * gridWidth;
}

float calcDist(vec3 a, vec3 b)
{
    float x = a.x - b.x;
    float y = a.y - b.y;
    float z = a.z - b.z;
    return sqrt( x*x + y*y + z*z);
}

// Compute barycentric coordinates (u, v, w) for
// point p with respect to triangle (a, b, c)
// http://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
void barycentric(vec3 p, vec3 a, vec3 b, vec3 c, float * u, float * v, float * w)
{
    vec3 v0 = VectorSub(b,a);
    vec3 v1 = VectorSub(c,a);
    vec3 v2 = VectorSub(p,a);
    float d00 = DotProduct(v0, v0);
    float d01 = DotProduct(v0, v1);
    float d11 = DotProduct(v1, v1);
    float d20 = DotProduct(v2, v0);
    float d21 = DotProduct(v2, v1);
    float denom = d00 * d11 - d01 * d01;
    *v = (d11 * d20 - d01 * d21) / denom;
    *w = (d00 * d21 - d01 * d20) / denom;
    *u = 1.0f - *v - *w;
}

float getHeight(GLfloat * vertexArray, GLfloat * height_map, int size, float posX, float posZ)
{
    int index0, index1, index2;
    getTriangle(posX, posZ, size, &index0, &index1, &index2);

    float vx1 = vertexArray[index0*3 + 0];
    float vx2 = vertexArray[index1*3 + 0];
    float vx3 = vertexArray[index2*3 + 0];
    float vy1 = vertexArray[index0*3 + 1];
    float vy2 = vertexArray[index1*3 + 1];
    float vy3 = vertexArray[index2*3 + 1];
    float vz1 = vertexArray[index0*3 + 2];
    float vz2 = vertexArray[index1*3 + 2];
    float vz3 = vertexArray[index2*3 + 2];

    vec3 v0 = SetVector(posX, 0, posZ);
    vec3 v1 = SetVector(vx1,0,vz1);
    vec3 v2 = SetVector(vx2,0,vz2);
    vec3 v3 = SetVector(vx3,0,vz3);

    float u,v,w;
    barycentric(v0,v1,v2,v3,&u,&v,&w);
    float y = u * vy1 + v * vy2 + w * vy3;
    return y;
}

float getHeight(TextureData * heightMap, float scaleFactor, float posX, float posZ) {
    if( posX < 0 || posZ < 0 )
        return 0;
    else if( posX > heightMap->width || posZ > heightMap->height )
        return 0;

    int indX = (int)posX;
    int indZ = (int)posZ;
    int ind = ((indX + heightMap->width * indZ) * heightMap->bpp) / 8;
    return scaleFactor * heightMap->imageData[ ind ];
}

Model * createObject(char * modelPath)
{
    Model * m = LoadModel(modelPath);

    Model* model = LoadDataToModel(
            m->vertexArray,
            m->normalArray,
            m->texCoordArray,
            NULL,
            m->indexArray,
            m->numVertices,
            m->numIndices);

    return model;
}

void mouseCallback(int mx, int my) {
    mouseX = mx;
    mouseY = my;
    int w,h;
    glutGetWindowSize(&w, &h);
    float wf = ((float)w) / 2.0f;
    float hf = ((float)h) / 2.0f;
    mouseX = ((float)mouseX - wf) / wf;
    mouseY = ((float)mouseY - hf) / hf;
    //glutWarpPointer(wf,hf);
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
    //glClearColor(0.2,0.2,0.5,0);
    glClearColor(0,0,0,0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    //glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    printError("GL inits");

    // Load and compile shader s
    terrain_shader = loadShaders("shaders/terrain.vert", "shaders/terrain.frag");
    cloud_shader = loadShaders("shaders/cloud_dynamic.vert", "shaders/cloud_dynamic.frag");
    water_shader = loadShaders("shaders/water.vert", "shaders/water.frag");
    index_shader = loadShaders("shaders/index.vert", "shaders/index.frag");
    surface_viz_shader = loadShadersG(
            "shaders/topology_viz.vert",
            "shaders/topology_viz.frag",
            "shaders/topology_viz.geom");
    normal_viz_shader = loadShadersG(
            "shaders/normal_viz.vert",
            "shaders/normal_viz.frag",
            "shaders/normal_viz.geom");
    ball_shader = loadShaders(
            "shaders/ball.vert",
            "shaders/ball.frag");

    // Load terrain data
    map_size = 256;
    std::cout << "Generate height_map" << std::endl;
    height_map = cd::generateTexture2D(map_size, map_size, -50, 50);
    std::cout << "Generate snow_map" << std::endl;
    snow_map = cd::generateTexture2D(height_map, map_size, map_size, -5, 5, 2);
    //std::cout << "Generate water_map" << std::endl;
    //water_map = cd::generateWaterMap(height_map, map_size, map_size, 10.0);
    std::cout << "Generate terrain_mesh" << std::endl;
    terrain_mesh = cd::generateTerrain(&height_map[0], map_size, map_size, 0.1f);
    //terrain_mesh.scale(1.0, 100.0, 1.0);
    terrain_mesh.toGPU();
    if( ! enable_draw[INDEX] )
    {
        terrain_mesh.bindShader(
                terrain_shader,
                "inPosition",
                "inNormal",
                "inTangent",
                "inBitangent",
                "inTexCoord");
    }
    else
    {
        terrain_mesh.bindShader(
                index_shader,
                "inPosition",
                "", "", "", "");
    }
    //glUseProgram(index_shader);
    //glUniformMatrix4fv(glGetUniformLocation(terrain_shader, "projMatrix"), 1,
    //        GL_TRUE, camera.projection.m);
    index_camera.bindShader(
            index_shader,
            "projMatrix",
            "");
    printError("terrain bindShader");

    glUseProgram(terrain_shader);
    glUniformMatrix4fv(glGetUniformLocation(terrain_shader, "projMatrix"), 1,
            GL_TRUE, camera.projection.m);
    glUniform1i(glGetUniformLocation(terrain_shader, "texUnit0"), 0);
    glUniform1i(glGetUniformLocation(terrain_shader, "texUnit1"), 1);
    glUniform1i(glGetUniformLocation(terrain_shader, "texUnitGloss"), 2);
    glUniform1i(glGetUniformLocation(terrain_shader, "texUnitNormal"), 3);
    LoadTGATextureSimple("data/textures/grass.tga", &terrain_tex_objID[0]);
    terrain_tex_objID[1] = cd::loadTexture2D("data/textures/snow_albedo.tif");
    terrain_tex_objID[2] =
        cd::loadTexture2D("data/textures/snow_albedo_gloss.tif");
    terrain_tex_objID[3] =
        cd::loadTexture2D("data/textures/snow_albedo_normal.tif");
    printError("terrain_shader");

    GLuint snow_vbo;
    cd::arrayToBuffer(snow_map, GL_ARRAY_BUFFER, &snow_vbo);
    cd::bindBufferAttribArray(snow_vbo, terrain_shader, "inSnowLevel",1);
    printError("terrain snow");

    std::cout << "Generate water_mesh" << std::endl;
    water_mesh = cd::createQuad(0,0,map_size,map_size); 
    //water_mesh = cd::generateTerrain(&water_map[0], map_size, map_size, -1);
    std::cout << "toGPU" << std::endl;
    water_mesh.toGPU();
    std::cout << "bindShader" << std::endl;
    water_mesh.bindShader(
            water_shader,
            "inPosition",
            "inNormal",
            "",
            "",
            "");
    glUseProgram(water_shader);
    glUniform1i(glGetUniformLocation(water_shader, "cloudSampler"), 0);
    glUniform1f(glGetUniformLocation(water_shader, "domeRadius"), 3000);
    glUniform1f(glGetUniformLocation(water_shader, "domeAngle"), M_PI/8);
    glUniform1f(glGetUniformLocation(water_shader, "domeTexScale"), 4);
    glUniformMatrix4fv(glGetUniformLocation(water_shader, "projMatrix"), 1, GL_TRUE, camera.projection.m);
    printError("water mesh");

    // Clouds
    cloud_mesh = cd::createDome(100,3000,M_PI/8,4);
    cloud_mesh.translate(map_size/2,0,map_size/2);
    cloud_mesh.toGPU();
    cloud_mesh.bindShader(
            cloud_shader,
            "inPosition",
            "","","",
            "inTexCoord");
    glUniformMatrix4fv(glGetUniformLocation(cloud_shader, "projMatrix"), 1, GL_TRUE, camera.projection.m);
    glUniform1i(glGetUniformLocation(cloud_shader, "texUnit"), 0);
    //cloud_tex_objID[0] = cd::loadTexture2D("data/textures/clouds.png");
    std::vector<std::string> paths;
    for(GLuint i = 1; i < 402; ++i)
    {
        std::stringstream ss;
        std::string filename;
        ss << "data/textures/cloud/cloud" << i << ".png";
        ss >> filename;
        paths.push_back(filename);
    }
    cloud_tex_objID[0] = cd::loadTexture3D(paths);
    printError("Clouds");


    // Surface and Normal visualizer
    glUseProgram(surface_viz_shader);
    glUniformMatrix4fv(glGetUniformLocation(surface_viz_shader, "projMatrix"), 1, GL_TRUE, camera.projection.m);
    glUseProgram(normal_viz_shader);
    glUniformMatrix4fv(glGetUniformLocation(normal_viz_shader, "projMatrix"), 1, GL_TRUE, camera.projection.m);

    viz_mesh.insertRawPointerData(
            0,
            terrain_mesh.m_vertex_array.size(),
            NULL,
            (GLfloat*)&terrain_mesh.m_vertex_array[0],
            (GLfloat*)&terrain_mesh.m_normal_array[0],
            (GLfloat*)&terrain_mesh.m_tangent_array[0],
            (GLfloat*)&terrain_mesh.m_bitangent_array[0],
            NULL);

    viz_mesh.toGPU();
    viz_mesh.bindShader(
            surface_viz_shader,
            "inPosition",
            "inNormal",
            "inTangent",
            "inBitangent",
            "");
    printError("viz_mesh");

    std::cout << "Creating terrain dual mesh" << std::endl;
    terrain_mesh_dual = terrain_mesh.dual();
    global_counter = 0L;
    terrain_mesh_dual.toGPU();
    terrain_mesh_dual.bindShader(
            normal_viz_shader,
            "inPosition",
            "inNormal",
            "", "", "");
    printError("terrain_mesh_dual");

    // BALL
    //ball_model = LoadModel("data/models/groundsphere.obj");
    ball_model = createObject("data/models/groundsphere.obj");
    CenterModel(ball_model);
    //ScaleModel(ball_model, 5, 5, 5);
    ReloadModelData(ball_model);
    glUseProgram(ball_shader);
    glUniformMatrix4fv(glGetUniformLocation(ball_shader, "projMatrix"), 1, GL_TRUE, camera.projection.m);

    // FBOs
    if( enable_draw[INDEX] )
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

        std::cout << "FBO = " << fbo << std::endl;
        std::cout << "renderTex = " << renderTex << std::endl;

        GLenum fbo_status = checkFBO();
        if( fbo_status != GL_FRAMEBUFFER_COMPLETE )
            exit(1);
    }

    // Various stuff
    // Init camera
    camera.pos = SetVector(map_size/2,5,map_size/2); camera.yaw = M_PI / 4;
    camera.speed = 50.0; // OBS
    printError("init shader");
    glut_time = (GLfloat)glutGet(GLUT_ELAPSED_TIME);    

    // Collect radiosity data
    if( enable_draw[INDEX] )
    { 
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER,fbo);
        terrain_mesh.bindShader(
                index_shader, 
                "inPosition",
                "inNormal",
                "",
                "",
                "");
        index_camera.bindShader(
                index_shader,
                "projMatrix",
                "");

        std::vector<std::int32_t> data(window_width*window_height,0);
        std::vector<glm::vec3> view_dirs(5);
        view_dirs[0] = glm::vec3(0,0,1);
        view_dirs[1] = glm::vec3(0,0,-1);
        view_dirs[2] = glm::vec3(0,1,0);
        view_dirs[3] = glm::vec3(1,0,0);
        view_dirs[4] = glm::vec3(-1,0,0);

        int num_faces = terrain_mesh_dual.m_vertex_array.size();
        Eigen::SparseMatrix<std::uint32_t> RadiosityFactorMatrix(num_faces, num_faces);

        /*
           std::vector<std::vector<std::int32_t> > histogram(
           terrain_mesh_dual.m_vertex_array.size(),
           std::vector<std::int32_t> (terrain_mesh_dual.m_vertex_array.size(), 0));
           std::cout << "histogram size = " << histogram.size() << std::endl;
           std::cout << "histogram[0] size = " << histogram[0].size() << std::endl;
           */

        //std::vector<std::int32_t> histogram(terrain_mesh_dual.m_vertex_array.size());
        std::vector<Eigen::Triplet<std::int32_t> > triplet_list; 

        for(GLuint i = 0; i < terrain_mesh_dual.m_vertex_array.size(); ++i)
        {
            for(GLuint j = 0; j < view_dirs.size(); ++j)
            {
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glm::vec3 v = terrain_mesh_dual.m_vertex_array[i];
                //glm::vec3 n = terrain_mesh_dual.m_normal_array[i];
                index_camera.lookTowards(v,view_dirs[0]); // TODO : Ska vara view_dirs[j]
                index_camera.bindShader(
                        index_shader,
                        "",
                        "mdlMatrix");
                glDisable(GL_CULL_FACE);
                terrain_mesh.draw();
                //glFinish();

                glReadBuffer(GL_COLOR_ATTACHMENT0);
                printError("glReadBuffer");
                glReadPixels(0,0,window_width,window_height,GL_RED_INTEGER,GL_INT,&data[0]);
                //glFinish(); 
                printError("glReadPixels");

                int num_unique_inds = 0;
                //std::fill(histogram.begin(), histogram.end(), 0);
                for(GLuint k = 0; k < data.size(); ++k)
                {
                    /*
                    if( data[k] != 0)
                        triplet_list.insert(i,k,1);
                    */
                    if( RadiosityFactorMatrix.coeff(i,data[k]) == 0 )
                    {
                        num_unique_inds += 1;
                        RadiosityFactorMatrix.insert(i,data[k]) = 1;
                    }
                    else
                        RadiosityFactorMatrix.coeffRef(i,data[k]) += 1;
                    
                    //histogram[data[i]] += 1;
                }

                std::cout << "num_unqiue_inds = " << num_unique_inds << " / " << num_faces << std::endl;

                cv::Mat tmp(window_height,window_width,CV_32SC1,&data[0]);
                cv::flip(tmp,tmp,0);
                tmp.convertTo(tmp, CV_32FC1);
                double min,max;
                cv::Point locmin,locmax;
                cv::minMaxLoc(tmp, &min, &max, &locmin, &locmax);
                //std::cout << "max = " << max << std::endl;
                tmp = tmp / max;
                cv::imshow("readback",tmp);
                cv::waitKey(1);
                //glutSwapBuffers();
            }
        }
    }
}


void updateCamera() {
    GLfloat new_time = (GLfloat)glutGet(GLUT_ELAPSED_TIME);
    GLfloat dt = (new_time - glut_time) / 1000.0f;
    glut_time = new_time;
    camera.setRotateVel(mouseY, mouseX);
    camera.setMoveVel(dt* moveX, 0, dt * moveZ);
    //float currentHeight = getHeight((GLfloat*)&terrain_mesh.m_vertex_array[0], &height_map[0], map_size, camera.pos.x, camera.pos.z);
    //camera.setTargetY(1.8f + currentHeight);
    camera.update();

    global_counter += 1;
    long index_counter = (global_counter) % terrain_mesh_dual.m_vertex_array.size();
    glm::vec3 v = terrain_mesh_dual.m_vertex_array[index_counter];
    glm::vec3 n = terrain_mesh_dual.m_normal_array[index_counter];
    /*
       camera.pos = SetVector(v.x,v.y,v.z);
       float th = std::acos(n.y);
       float ph = std::atan2(n.z,n.x);
       camera.pitch = th; //M_PI;
       camera.yaw = ph;//M_PI/2;
       std::cout << "th = " << th << ", ph = " << ph << std::endl;
       camera.update();
       */

    // GLMCamera
    index_camera.lookTowards(v+n,glm::vec3(0,0,1));
}

void display(void)
{

    // TODO : Bind correct fbo
    if( enable_draw[INDEX] )
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER,fbo);

    // clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    updateCamera();

    printError("pre display");
    // Draw clouds
    if( enable_draw[CLOUD] )
    {
        glUseProgram(cloud_shader);
        glUniformMatrix4fv(glGetUniformLocation(cloud_shader, "mdlMatrix"), 1, GL_TRUE, camera.transform.m);
        glUniform1f(glGetUniformLocation(cloud_shader, "time"), glut_time / 1000.0f);
        glEnable(GL_BLEND);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, cloud_tex_objID[0]); //cloud_tex_objID[0]);
        cloud_mesh.draw();
        glDisable(GL_BLEND);
        glClear(GL_DEPTH_BUFFER_BIT);
        printError("display clouds");
    }

    // Draw terrain
    if( enable_draw[TERRAIN] )
    {
        if( ! enable_draw[INDEX] )
        {
            glUseProgram(terrain_shader);	
            glUniformMatrix4fv(glGetUniformLocation(terrain_shader, "mdlMatrix"), 1, GL_TRUE, camera.transform.m);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, terrain_tex_objID[0]);		// Bind Our Texture tex1
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, terrain_tex_objID[1]);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, terrain_tex_objID[2]);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, terrain_tex_objID[3]);
        }
        else {

            terrain_mesh.bindShader(
                    index_shader, 
                    "inPosition",
                    "",
                    "",
                    "",
                    "");
            glUniformMatrix4fv(glGetUniformLocation(index_shader, "mdlMatrix"), 1, GL_TRUE, camera.transform.m);
            glDisable(GL_CULL_FACE);
            /*
               index_camera.bindShader(
               index_shader,
               "",
               "mdlMatrix");
               */
        }

        terrain_mesh.draw();
        printError("display terrain");
    }

    // Draw water
    if( enable_draw[WATER] )
    {
        glUseProgram(water_shader);
        printError("use program");
        glEnable(GL_BLEND);
        glUniform1f(glGetUniformLocation(water_shader, "time"), glut_time / 1000.0f);
        glUniformMatrix4fv(glGetUniformLocation(water_shader, "mdlMatrix"), 1, GL_TRUE, camera.transform.m);
        printError("uniform");
        glActiveTexture(GL_TEXTURE0);
        printError("active tex");
        glBindTexture(GL_TEXTURE_3D, cloud_tex_objID[0]);
        printError("bind tex");
        water_mesh.draw();
        printError("water mesh draw");
        glDisable(GL_BLEND);
        printError("display water");
    }

    // Draw normals
    if( enable_draw[VIZ] )
    {
        glEnable(GL_BLEND);
        glUseProgram(surface_viz_shader);
        glUniformMatrix4fv(glGetUniformLocation(surface_viz_shader, "mdlMatrix"), 1, GL_TRUE, camera.transform.m);
        viz_mesh.draw();
        glUseProgram(normal_viz_shader); 
        glUniformMatrix4fv(glGetUniformLocation(normal_viz_shader, "mdlMatrix"), 1, GL_TRUE, camera.transform.m);
        terrain_mesh_dual.draw();
        glDisable(GL_BLEND);
        printError("display topology");
    }
    // draw ball
    /*
       glUseProgram(ball);
       float x = camera.pos.x + sin(camera.yaw)*10;
       float z = camera.pos.z - cos(camera.yaw)*10;
       float y = getHeight(tm, &height_map[0], map_size, x,z);
       mat4 ball_transform = Mult(T(x,y,z), S(2,2,2));
       ball_transform = Mult(camera.transform, ball_transform); // Mult(camMatrix, modelView);
       glUniformMatrix4fv(glGetUniformLocation(terrain_shader, "mdlMatrix"), 1, GL_TRUE, ball_transform.m);
       DrawModel(sphere,ball,"inPosition", "inNormal", "inTexCoord");
       */
    if( enable_draw[BALL] )
    {
        glDisable(GL_CULL_FACE);
        glUseProgram(ball_shader);
        glUniformMatrix4fv(glGetUniformLocation(ball_shader, "mdlMatrix"), 1, GL_TRUE, camera.transform.m);
        long ball_index = map_size*map_size / 2 + (global_counter / 10);
        ball_index = ball_index % terrain_mesh_dual.m_vertex_array.size();
        glm::vec3 v = terrain_mesh_dual.m_vertex_array[ball_index];
        glm::vec3 n = terrain_mesh_dual.m_normal_array[ball_index];
        //std::cout << "v = (" << v.x << ", " << v.y <<  ", " << v.z << ")" << std::endl;
        //std::cout << "n = (" << n.x << ", " << n.y <<  ", " << n.z << ")" << std::endl;
        GLint pos_loc = glGetUniformLocation(ball_shader, "position");
        GLint dir_loc = glGetUniformLocation(ball_shader, "direction");
        glUniform3fv(pos_loc, 1, &v[0]); 
        printError("ball position");
        glUniform3fv(dir_loc, 1, &n[0]); 
        printError("ball direction");
        DrawModel(ball_model, ball_shader, "inPosition", "inNormal", NULL);
        printError("Draw ball");
        glEnable(GL_CULL_FACE);
    }

    if( enable_draw[INDEX] )
    {
        std::vector<std::int32_t> data(window_width*window_height,0);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        printError("glReadBuffer");
        glReadPixels(0,0,window_width,window_height,GL_RED_INTEGER,GL_INT,&data[0]);
        printError("glReadPixels");
        cv::Mat tmp(window_height,window_width,CV_32SC1,&data[0]);
        cv::flip(tmp,tmp,0);
        tmp.convertTo(tmp, CV_32FC1);
        double min,max;
        cv::Point locmin,locmax;
        cv::minMaxLoc(tmp, &min, &max, &locmin, &locmax);
        std::cout << "max = " << max << std::endl;
        tmp = tmp / max;
        cv::imshow("readback",tmp);
        cv::waitKey(1);
    }

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

#else
#include <iostream>
int main(int argc, char **argv)
{
    std::cout << "This executable is not tested on Mac, aborting" << std::endl;
    return 1;
}

#endif
