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


bool quit_application = false;

std::vector<GLfloat> height_map;
std::vector<GLfloat> snow_map;

cd::Mesh terrain_mesh;
cd::Mesh sun_mesh;
GLuint map_size;
GLfloat map_height;
GLuint terrain_tex_objID[7];

GLuint snow_buffer_cnt = 0;
const GLuint num_snow_buffers = 2;
//GLuint snow_vao[num_snow_buffers];
GLuint snow_vbo[num_snow_buffers];

cd::GLMCamera camera;

// References to shader programs
GLuint terrain_shader;
GLuint particle_shader;
GLuint sun_shader;

std::vector<glm::vec3> particles;
const GLuint num_particle_buffers = 2;
GLuint buffer_cnt;
GLuint particle_vao[num_particle_buffers];
GLuint particle_vbo[num_particle_buffers];

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
        case 'f': glutToggleFullScreen(); break;
        case 'q': quit_application = true; break;
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

float calcDist(glm::vec3 a, glm::vec3 b)
{
    float x = a.x - b.x;
    float y = a.y - b.y;
    float z = a.z - b.z;
    return sqrt( x*x + y*y + z*z);
}

// Compute barycentric coordinates (u, v, w) for
// point p with respect to triangle (a, b, c)
// http://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
void barycentric(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c, float * u, float * v, float * w)
{
    glm::vec3 v0 = b-a;
    glm::vec3 v1 = c-a;
    glm::vec3 v2 = p-a;
    float d00 = glm::dot(v0,v0);
    float d01 = glm::dot(v0,v1);
    float d11 = glm::dot(v1,v1);
    float d20 = glm::dot(v2, v0);
    float d21 = glm::dot(v2, v1);
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

    glm::vec3 v0(posX, 0, posZ);
    glm::vec3 v1(vx1, 0, vz1);
    glm::vec3 v2(vx2, 0, vz2);
    glm::vec3 v3(vx3, 0, vz3);

    float u,v,w;
    barycentric(v0,v1,v2,v3,&u,&v,&w);
    float y = u * vy1 + v * vy2 + w * vy3;
    return y;
}

void init(void)
{
    // GL inits
    glClearColor(0.2,0.2,0.5,0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDepthFunc(GL_LESS);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    printError("GL inits");

    // Load and compile shader s
    terrain_shader = loadShaders(
            "shaders/terrain_ambient_light.vert",
            "shaders/terrain_ambient_light.frag");
    particle_shader = loadShadersG(
            "shaders/particle.vert",
            "shaders/particle.frag",
            "shaders/particle.geom");
    sun_shader = loadShaders(
            "shaders/sun.vert",
            "shaders/sun.frag");

    // Load terrain data
    map_size = 256;
    map_height = 50; //static_cast<GLfloat>(map_size)/8;
    std::cout << "Generate height_map" << std::endl;
    height_map = cd::generateTexture2D(map_size, map_size, -map_height, map_height);
    std::cout << "Generate snow map" << std::endl;
    snow_map = cd::generateTexture2D(height_map, map_size, map_size, -map_height, map_height, map_height/2);
    std::cout << "Generate terrain_mesh" << std::endl;
    terrain_mesh = cd::generateTerrain(&height_map[0], map_size, map_size, 0.1f);
    terrain_mesh.toGPU();
    terrain_mesh.bindShader(
            terrain_shader,
            "inPosition",
            "inNormal",
            "inTangent",
            "inBitangent",
            "inTexCoord"); 
    glUseProgram(terrain_shader);
    camera.bindShader(terrain_shader, "projMatrix", "mdlMatrix");

    std::vector<glm::vec3> light_dir(6);
    std::vector<GLfloat> light_ampl(6,0);
    std::vector<glm::vec3> axis(3);
    axis[0] = glm::vec3(1,0,0);
    axis[1] = glm::vec3(0,1,0);
    axis[2] = glm::vec3(0,0,1);

    light_dir[0] = glm::vec3(1,0,0);

    /*
    for(int i = 0; i < 6; ++i)
    {
        light_dir[i] = glm::vec3(
                (std::rand()-0.5f) / RAND_MAX,
                (std::rand()-0.5f) / RAND_MAX,
                (std::rand()-0.5f) / RAND_MAX);
        light_ampl[i] = ((float)std::rand()) / RAND_MAX;
        light_ampl_sum += light_ampl[i];
        std::cout << light_ampl[i] << std::endl;
    }
    for(int i = 0; i < 6; ++i)
        light_ampl[i] /= light_ampl_sum;
    */

    glUniform3fv(glGetUniformLocation(terrain_shader, "lightDir"), light_dir.size(), (GLfloat*)&light_dir[0]);
    glUniform1fv(glGetUniformLocation(terrain_shader, "lightAmpl"), light_ampl.size(), &light_ampl[0]);
    glUniform1i(glGetUniformLocation(terrain_shader, "texUnitRock0"), 0);
    glUniform1i(glGetUniformLocation(terrain_shader, "texUnitRock1"), 1);
    glUniform1i(glGetUniformLocation(terrain_shader, "texUnitRock2"), 2);
    glUniform1i(glGetUniformLocation(terrain_shader, "texUnitRockSlope"), 3);
    glUniform1i(glGetUniformLocation(terrain_shader, "texUnitSnow"), 4);
    glUniform1i(glGetUniformLocation(terrain_shader, "texUnitGloss"), 5);
    glUniform1i(glGetUniformLocation(terrain_shader, "texUnitNormal"), 6);
    terrain_tex_objID[0] = cd::loadTexture2D("data/textures/grass_leaves_0036_01.jpg");
    terrain_tex_objID[1] = cd::loadTexture2D("data/textures/TexturesCom_RockGrassy0051_1_seamless_S.jpg"); 
    terrain_tex_objID[2] = cd::loadTexture2D("data/textures/TexturesCom_RockMossy0033_1_seamless_S.jpg");
    terrain_tex_objID[3] = cd::loadTexture2D("data/textures/TexturesCom_RockSediment0122_S.jpg");
    terrain_tex_objID[4] = cd::loadTexture2D("data/textures/snow_albedo.tif");
    terrain_tex_objID[5] = cd::loadTexture2D("data/textures/snow_albedo_gloss.tif");
    terrain_tex_objID[6] = cd::loadTexture2D("data/textures/snow_albedo_normal.tif");
    printError("terrain_shader");

    // Sun
    sun_mesh = cd::createDome(
            100,
            0.5,
            M_PI/2,
            1.0f);
    std::cout << "sun to gpu" << std::endl;
    sun_mesh.toGPU();
    std::cout << "sub gpu compl" << std::endl;
    sun_mesh.bindShader(sun_shader, "inPosition", "", "", "", "inTexCoord");
    std::cout << "sun bind shader" << std::endl;
    printError("sub_shader");

    // Snow
    glBindVertexArray(terrain_mesh.m_vao);    
    glGenBuffers(num_snow_buffers, snow_vbo);
    for(GLuint i = 0; i < num_snow_buffers; ++i)
    {
        glBindBuffer(GL_ARRAY_BUFFER, snow_vbo[i]);
        std::cout << "snow_vbo["<<i<<"] = " << snow_vbo[i] << std::endl;
        printError("snow vbo");
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*snow_map.size(), &snow_map[0], GL_DYNAMIC_DRAW);
        printError("terrain snow data");
        cd::bindBufferAttribArray(snow_vbo[i], terrain_shader, "inSnowLevel", 1);
        printError("terrain snow bindBufferAttrib");
    }
    //cd::arrayToBuffer(snow_map, GL_ARRAY_BUFFER, &snow_vbo);
    //cd::bindBufferAttribArray(snow_vbo, terrain_shader, "inSnowLevel",1);
    printError("terrain snow");

    // Particles
    float dt = 1.0f / 60.0f;
    glm::vec3 g(0.0f,-9.82f,0.0f);
    particles.resize(128*1024);

    for(GLuint i = 0; i < particles.size(); ++i)
    {
        particles[i] = glm::vec3(
                (map_size-1) * float(std::rand())/RAND_MAX,
                map_height + (map_height)*(0.5f - float(std::rand())/RAND_MAX),
                (map_size-1) * float(std::rand())/RAND_MAX);
    }

    buffer_cnt = 0;
    glGenVertexArrays(num_particle_buffers, particle_vao);
    glGenBuffers(num_particle_buffers, particle_vbo); 
    for(GLuint i = 0; i < num_particle_buffers; ++i)
    {
        std::cout << "vao[" <<i<<"] = " << particle_vao[i] << std::endl;
        glBindVertexArray(particle_vao[i]);
        glBindBuffer(GL_ARRAY_BUFFER, particle_vbo[i]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*particles.size(), &particles[0], GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), nullptr);
    }
    printError("data to particle buffers");

    camera.lookTowards(
            glm::vec3(map_size/2,map_size/8,map_size/2),
            glm::vec3(1,-1,0));
    camera.bindShader(terrain_shader, "projMatrix", "mdlMatrix");
    printError("init camera shader");
    camera.bindShader(particle_shader, "projMatrix", "mdlMatrix");
    printError("init camera particle");
}

void updateCamera() {

    float rot_vel = 1.0;
    camera.rotateLocal(rot_vel * mouseY, rot_vel * mouseX);
    camera.translateLocal(moveX, 0, moveZ);      
    mouseX = 0;
    mouseY = 0;
}

void updateParticles()
{
    
    float snow_flake_amount = 0.1;

    for(GLuint i = 0; i < particles.size(); ++i)
    {
        float x = particles[i].x;
        float y = particles[i].y;
        float z = particles[i].z;
        float height  = getHeight(
                (GLfloat*)&terrain_mesh.m_vertex_array[0],
                &height_map[0],
                map_size,
                x,z);

        if( y <= height ) 
        {
            particles[i].y = map_height + (map_height)*(0.5f - float(std::rand())/RAND_MAX);
            GLuint index = static_cast<GLuint>(z * map_size + x);
            if( index >= snow_map.size() )
                std::cout << "index = " << index << "/ " << snow_map.size() << std::endl;
            snow_map[index] = std::min(1.0f, snow_map[index] + snow_flake_amount);
        }
        particles[i].y -= 0.1;
    };

    // update particle buffer
    buffer_cnt = (buffer_cnt+1) % num_particle_buffers;
    glBindBuffer(GL_ARRAY_BUFFER, particle_vbo[buffer_cnt]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*particles.size(), 0, GL_DYNAMIC_DRAW);

    glm::vec3 * mapped_array = 
        reinterpret_cast<glm::vec3*>(
                glMapBufferRange(GL_ARRAY_BUFFER, 0, 
                    sizeof(glm::vec3)*particles.size(),
                    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT
                    )
                );
    std::copy(particles.begin(), particles.end(), mapped_array);
    glUnmapBuffer(GL_ARRAY_BUFFER);

    // update snowmap
    glBindVertexArray(terrain_mesh.m_vao);
    snow_buffer_cnt = (snow_buffer_cnt+1) % num_snow_buffers;
    glBindBuffer(GL_ARRAY_BUFFER, snow_vbo[snow_buffer_cnt]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*snow_map.size(), 0, GL_DYNAMIC_DRAW);
    GLfloat * snow_mapped_array = 
        reinterpret_cast<GLfloat*>(
                glMapBufferRange(GL_ARRAY_BUFFER, 0,
                    sizeof(GLfloat) * snow_map.size(),
                    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT
                    )
                );
    std::copy(snow_map.begin(), snow_map.end(), snow_mapped_array);
    glUnmapBuffer(GL_ARRAY_BUFFER);
}

void display(void)
{
    printError("pre display");

    updateCamera();
    printError("updateCamera");

    //updateParticles();
    //printError("updateParticles");

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw sky
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    sun_mesh.bindShader(
            sun_shader,
            "inPosition",
            "","","",
            "inTexCoord");
    camera.bindShader(
            sun_shader,
            "projMatrix",
            "mdlMatrix");
    glm::vec3 sun_dir(1,1,0);
    glUniform3fv(glGetUniformLocation(sun_shader,"sunDir"), 1, &sun_dir[0]);
    sun_mesh.draw();

    // Draw terrain
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDepthFunc(GL_LESS);
    terrain_mesh.bindShader(
            terrain_shader, 
            "inPosition",
            "inNormal",
            "inTangent",
            "inBitangent",
            "inTexCoord");
    camera.bindShader(
            terrain_shader,
            "projMatrix",
            "mdlMatrix");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, terrain_tex_objID[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, terrain_tex_objID[1]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, terrain_tex_objID[2]);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, terrain_tex_objID[3]);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, terrain_tex_objID[4]);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, terrain_tex_objID[5]);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, terrain_tex_objID[6]);
    terrain_mesh.draw();
    printError("display terrain");

    // Draw particles
    /*
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(particle_shader);
    camera.bindShader(particle_shader, "projMatrix", "mdlMatrix");
    glBindVertexArray(particle_vao[buffer_cnt]);
    glDrawArrays(GL_POINTS, 0, particles.size());
    printError("draw particles");
    */


    glutSwapBuffers();

    if( quit_application )
        exit(0);
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
