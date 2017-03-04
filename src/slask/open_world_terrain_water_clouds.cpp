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

bool enable_debug_viz = false;

std::vector<GLfloat> height_map;
std::vector<GLfloat> snow_map;
std::vector<GLfloat> water_map;
GLuint map_size;
GLfloat glut_time;
cd::Camera camera;

// References to shader programs
GLuint terrain_shader;
GLuint cloud_shader;
GLuint water_shader;
GLuint index_shader;
GLuint viz_shader;

cd::Mesh terrain_mesh;
cd::Mesh cloud_mesh;
cd::Mesh water_mesh;
cd::Mesh viz_mesh;

GLuint terrain_tex_objID[4];
GLuint cloud_tex_objID[1];

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


void init(void)
{
    // GL inits
    glClearColor(0.2,0.2,0.5,0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    printError("GL inits");

    // Load and compile shader s
    terrain_shader = loadShaders("shaders/terrain.vert", "shaders/terrain.frag");
    cloud_shader = loadShaders("shaders/cloud_dynamic.vert", "shaders/cloud_dynamic.frag");
    water_shader = loadShaders("shaders/water.vert", "shaders/water.frag");
    index_shader = loadShaders("shaders/index.vert", "shaders/index.frag");
    viz_shader = loadShadersG(
            "shaders/topology_viz.vert",
            "shaders/topology_viz.frag",
            "shaders/topology_viz.geom");

    // Load terrain data
    map_size = 256;
    std::cout << "Generate height_map" << std::endl;
    height_map = cd::generateTexture2D(map_size, map_size, -50, 50);
    std::cout << "Generate snow_map" << std::endl;
    snow_map = cd::generateTexture2D(height_map, map_size, map_size, -50, 50, 25);
    //std::cout << "Generate water_map" << std::endl;
    //water_map = cd::generateWaterMap(height_map, map_size, map_size, 10.0);
    std::cout << "Generate terrain_mesh" << std::endl;
    terrain_mesh = cd::generateTerrain(&height_map[0], map_size, map_size, 0.1f);
    //terrain_mesh.scale(1.0, 100.0, 1.0);
    terrain_mesh.toGPU();
    terrain_mesh.bindShader(
            terrain_shader,
            "inPosition",
            "inNormal",
            "inTangent",
            "inBitangent",
            "inTexCoord");
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
    for(int i = 1; i < 402; ++i)
    {
        std::stringstream ss;
        std::string filename;
        ss << "data/textures/cloud/cloud" << i << ".png";
        ss >> filename;
        paths.push_back(filename);
    }
    cloud_tex_objID[0] = cd::loadTexture3D(paths);
    printError("Clouds");


    // Normal visualizer
    glUseProgram(viz_shader);
    glUniformMatrix4fv(glGetUniformLocation(viz_shader, "projMatrix"), 1, GL_TRUE, camera.projection.m);

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
            viz_shader,
            "inPosition",
            "inNormal",
            "inTangent",
            "inBitangent",
            "");
    printError("viz_mesh");

    //

    // Various stuff
    // Init camera
    camera.pos = SetVector(map_size/2,5,map_size/2); camera.yaw = M_PI / 4;
    camera.speed = 50.0; // OBS
    printError("init shader");
    glut_time = (GLfloat)glutGet(GLUT_ELAPSED_TIME);    
}


void updateCamera() {
    GLfloat new_time = (GLfloat)glutGet(GLUT_ELAPSED_TIME);
    GLfloat dt = (new_time - glut_time) / 1000.0f;
    glut_time = new_time;
    camera.setRotateVel(mouseY, mouseX);
    camera.setMoveVel(dt* moveX, 0, dt * moveZ);
    float currentHeight = getHeight((GLfloat*)&terrain_mesh.m_vertex_array[0], &height_map[0], map_size, camera.pos.x, camera.pos.z);
    camera.setTargetY(1.8f + currentHeight);
    camera.update();
}

void display(void)
{
    // clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    updateCamera();

    printError("pre display");
    // Draw clouds
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

    // Draw terrain
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
    terrain_mesh.draw();
    printError("display terrain");

    // Draw water
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

    // Draw normals
    if( enable_debug_viz )
    {
        glUseProgram(viz_shader);
        glUniformMatrix4fv(glGetUniformLocation(viz_shader, "mdlMatrix"), 1, GL_TRUE, camera.transform.m);
        glEnable(GL_BLEND);
        viz_mesh.draw();
        glDisable(GL_BLEND);
        printError("display topology");
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
    glutInitWindowSize (600, 600);
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
