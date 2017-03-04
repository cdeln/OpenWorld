#include "gl_include.h"
#include "MicroGlut.h"
#include "GL_utilities.h"
#include "LoadTGA.h"
#include "camera.hpp"
#include "autogen.hpp"
#include "tex.hpp"
#include "mesh.hpp"
#include "util.hpp"
#include "debug.hpp"
#include "shader.hpp"
#include "light.hpp"
#include <cmath>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <glm/gtx/string_cast.hpp>
#include <cassert>

/* Settings */
GLuint window_width = 1024;
GLuint window_height = 720;
/*
 * map_size_exp :  This controls size, map total size is (2^(2*map_size_exp))
 * map_size_exp = 13 requires just below 32Gbit RAM
 * map_size_exp = 12 : requires less than 16Gbit RAM
 * Both 12,13 takes some while to generate, so be patient
 * map_size_exp = 10 : Good default setting
*/
GLuint map_size_exp = 10; 
GLfloat waterDepth = 50; // Depth of water (before completely black) in meters
GLfloat snow_prob_thresh = 0.75; // Snow should appear at what height? (percentage)

/*
 * Various control variables, accesssible through keyboard buttons
 */ 
bool quit_application = false;
bool fast_cam = false;
bool enable_particles = false;
bool enable_topology_viz = false;
bool enable_cam_fly = true;
bool enable_shadow = true;
bool enable_warp_pointer = true;
bool enable_snow_texture = true;
bool take_screenshot = false;
bool blinnPhong = true;
bool enable_normal_mapping = true;
bool enable_gloss_mapping = true;
bool enable_rayleigh = true;
bool enable_mie = true;
bool enable_clouds = true;
bool enable_cloud_projection = true;
bool enable_cloud_fog = true;
float depth_margin = 0.01f;
float depth_margin_step = 0.01f;
int depth_num_sample = 2;
float water_alpha = 0.2;
float water_alpha_step = 0.05;
float dist_step = 100.0f;
float dist_step_step = 10.0f;
float current_time = 6;
float time_step = 0.1;
bool enable_shadow_map_debug = false;

/*
 * Various global variables, non accessible
 */
cd::Timer tim;
int image_counter = 0;
GLfloat elapsed_time = 0;
GLuint map_size;
GLfloat map_scale;
GLfloat map_size_x;
GLfloat map_size_z;
GLfloat map_height;
GLuint num_particles;

/* Meshes */
std::vector<GLfloat> height_map;
std::vector<GLfloat> snow_map;
std::vector<cd::MeshGridGMM> terrain_grid;
std::vector<std::vector<GLint> > geolevel;
cd::Mesh sun_mesh;
cd::Mesh ground_mesh;
cd::Mesh water_mesh;
cd::Mesh cloud_mesh;
cd::Mesh viz_mesh;

/* Textures */
GLuint terrain_tex_objID[6];
GLuint snow_tex;
GLuint height_tex;
GLuint wind_tex_X;
GLuint wind_tex_Z;
GLuint ground_tex;
GLuint cloud_tex_objID;

/* Camera */
cd::GLMCamera camera;
cd::GLMCamera mirror_camera;
GLfloat camera_speed = 1.0f;
GLfloat camera_speed_fast = 100.0f;

/* Shaders */
GLuint terrain_shader;
GLuint particle_shader;
GLuint particle_transform_shader;
GLuint sun_shader;
GLuint ground_shader;
GLuint water_shader;
GLuint reflection_shader;
GLuint cloud_shader;
GLuint shadow_shader;
GLuint viz_shader;

/* Particle stuff */
std::vector<glm::vec3> particles;
GLint current_buffer;
const GLint buffer_cnt = 2;
GLuint particle_vao[buffer_cnt];
GLuint particle_transform_vao[buffer_cnt];
GLuint particle_vbo[buffer_cnt];

// Water
GLuint reflection_billboard_vao;

// Shadows 
glm::vec3 sun_dir;
cd::GLMCamera shadow_camera;
GLuint fbo;
GLuint fbo_shadow;
GLuint reflection_tex;
GLuint reflection_depth_tex;
GLuint shadow_depth_tex;

/* Functions for updating the off screen rendering textures on toggling fullscreen */
void updateFBOTexReflective()
{
    glBindTexture(GL_TEXTURE_2D, reflection_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); 
    glBindTexture(GL_TEXTURE_2D, reflection_depth_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, window_width, window_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT,NULL);
}
void updateFBOTexShadow()
{
    //glBindTexture(GL_TEXTURE_2D, shadow_tex);
    // * glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, window_width, window_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT,NULL);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); 
    glBindTexture(GL_TEXTURE_2D, shadow_depth_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, window_width, window_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT,NULL);
    glUseProgram(terrain_shader);
    glUniform2f(glGetUniformLocation(terrain_shader, "texSize"), window_width, window_height);
}
void updateFBOTex()
{
    updateFBOTexReflective();
    updateFBOTexShadow();
}

/* User interface functions */
void toggleFullscreen()
{
    glutToggleFullScreen();
    int w,h;
    glutGetWindowSize(&w,&h);
    window_width = static_cast<GLuint>(w);
    window_height = static_cast<GLuint>(h);
    updateFBOTex();
}

void updateKeyDown(unsigned char event)
{
    switch(event)
    {
        case 'p': enable_particles = !enable_particles; break;
        case 'l': blinnPhong = !blinnPhong; break;
        case 'v': enable_topology_viz = !enable_topology_viz; break;
        case 'c': enable_cam_fly = !enable_cam_fly; break;
        case 'm': enable_shadow = !enable_shadow; break;
        case 'z': enable_warp_pointer = !enable_warp_pointer; break;
        case 'k': enable_snow_texture = !enable_snow_texture; break;
        case 13: take_screenshot = true; break; // Return 
        case 'g': enable_gloss_mapping = !enable_gloss_mapping; break;
        case 'n': enable_normal_mapping = !enable_normal_mapping; break;
        case 'r': enable_rayleigh = !enable_rayleigh; break;
        case 't': enable_mie = !enable_mie; break;
        case 'y': enable_clouds = !enable_clouds; break;
        case 'u': enable_cloud_projection = !enable_cloud_projection; break;
        case 'i': enable_cloud_fog = !enable_cloud_fog; break;
        case 49: depth_margin = std::max(0.0f,depth_margin-depth_margin_step); break; // 1
        case 50: depth_margin += depth_margin_step; break; // 2
        case 51: depth_num_sample = std::max(0,depth_num_sample-1); break; // 3
        case 52: depth_num_sample += 1; break; // 4
        case 53: water_alpha = std::max(0.0f,water_alpha - water_alpha_step); break; // 5
        case 54: water_alpha = std::min(1.0f,water_alpha + water_alpha_step); break; // 6
        case 55: dist_step = std::max(0.0f, dist_step - dist_step_step); break; // 7
        case 56: dist_step += dist_step_step; break; // 8
        case 57: current_time -= time_step; break; // 9
        case 48: current_time += time_step; break; // 0
        case 'b': enable_shadow_map_debug = !enable_shadow_map_debug; break;
    }
}

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
        case 'f': toggleFullscreen(); break;
        case 'q': quit_application = true; break;
        case 32: fast_cam = !fast_cam; break; // Space

    }
    if( ! up )
    {
        updateKeyDown(event);
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
    if( enable_warp_pointer )
    {
        int w,h;
        glutGetWindowSize(&w, &h);
        mx -= w/2;
        my -= h/2;
        float wf = ((float)w) / 2.0f;
        float hf = ((float)h) / 2.0f;
        mouseX += ((float)mx) / wf; 
        mouseY += ((float)my) / hf; 
        if( (mx != 0) || (my != 0) )
        {
            glutWarpPointer(wf,hf);
        }
    }
}

/* Movement functions */
void getTriangle(float posX, float posZ, int gridWidth, float scale, int * index0, int * index1, int * index2)
{
    int indX0 = static_cast<int>(std::round(posX/scale)); //(int)(posX + 0.5f);
    int indZ0 = static_cast<int>(std::round(posZ/scale)); //(int)(posZ + 0.5f);
    int diffX = indX0 < posX ? 1 : -1;
    int diffZ = indZ0 < posZ ? 1 : -1;

    *index0 = indX0 + indZ0 * gridWidth;
    *index1 = (indX0+diffX) + indZ0 * gridWidth;
    *index2 = indX0 + (indZ0+diffZ) * gridWidth;
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

void barycentricSafe(
        glm::vec3 p,
        glm::vec3 a,
        glm::vec3 b,
        glm::vec3 c,
        float *u,
        float *v,
        float *w)
{
    glm::vec3 v0 = b - a;
    glm::vec3 v1 = c - a;
    glm::vec3 v2 = p - a;
    glm::vec3 c0 = glm::cross(v0,v1);
    glm::vec3 c1 = glm::cross(b - p,c - p);
    glm::vec3 c2 = glm::cross(c - p, a - p); 
    float A0 = std::fabs(c0.y);
    float A1 = std::fabs(c1.y);
    float A2 = std::fabs(c2.y);
    *u = A1 / A0;
    *v = A2 / A0;
    *w = 1.0f - *u - *v;
    assert(*u>=0);
    assert(*v>=0);
    assert(*w>=0);
}

float getHeight(GLfloat * height_map, int size, float scale, float posX, float posZ)
{
    int index0, index1, index2;
    getTriangle(posX, posZ, size, scale, &index0, &index1, &index2);

    float vx1 = static_cast<float>(index0 % size) * scale;
    float vx2 = static_cast<float>(index1 % size) * scale;
    float vx3 = static_cast<float>(index2 % size) * scale;
    float vy1 = height_map[index0];
    float vy2 = height_map[index1];
    float vy3 = height_map[index2];
    float vz1 = static_cast<float>(index0 / size) * scale;
    float vz2 = static_cast<float>(index1 / size) * scale;
    float vz3 = static_cast<float>(index2 / size) * scale;

    glm::vec3 v0(posX, 0, posZ);
    glm::vec3 v1(vx1, 0, vz1);
    glm::vec3 v2(vx2, 0, vz2);
    glm::vec3 v3(vx3, 0, vz3);

    float u,v,w;
    //barycentric(v0,v1,v2,v3,&u,&v,&w);
    barycentricSafe(v0,v1,v2,v3,&u,&v,&w);
    float y = u * vy1 + v * vy2 + w * vy3;
    return y;
}

/* Initial setup */
void init(void)
{
    glEnable(GL_FRAMEBUFFER_SRGB);
    // GL inits
    glClearColor(0,0,0,water_alpha);
    glClearStencil(0xFF);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDepthFunc(GL_LESS);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    printError("GL inits");

    /* Load and compile shaders */
    terrain_shader = loadShaders(
            "shaders/terrain_final.vert",
            "shaders/terrain_final.frag");
    particle_shader = loadShaders(
            "shaders/particle_instanced_shadows.vert",
            "shaders/particle_instanced_shadows.frag");
    sun_shader = loadShaders(
            "shaders/sun_scatter.vert",
            "shaders/sun_scatter.frag");
    ground_shader = loadShaders(
            "shaders/ground.vert",
            "shaders/ground.frag");
    water_shader = loadShaders(
            "shaders/transparent.vert",
            "shaders/transparent.frag");
    reflection_shader = loadShaders(
            "shaders/texture_billboard.vert",
            "shaders/texture_billboard.frag");
    cloud_shader = loadShaders(
            "shaders/cloud_dynamic_horizon_fog_projective.vert",
            "shaders/cloud_dynamic_horizon_fog_projective.frag");
    shadow_shader = loadShaders(
            "shaders/shadow_map.vert",
            "shaders/shadow_map.frag");
    viz_shader = loadShadersG(
            "shaders/topology_viz.vert",
            "shaders/topology_viz.frag",
            "shaders/topology_viz.geom");

    // Load terrain data
    map_size = (1<<map_size_exp);
    GLuint num_height_map_arrays = 1;
    map_scale = 1.0f;
    map_size_x = map_scale * map_size;
    map_size_z = map_scale * map_size;
    GLfloat texture_scale = 1.0f;
    std::cout << "Generate height_map" << std::endl;
    std::vector<std::vector<GLfloat> > height_maps = cd::generateTexture2DArray(map_size, map_size, num_height_map_arrays);
    height_map = cd::quiltTexture2D(height_maps, map_size, map_size);
    map_size *= 1<<(num_height_map_arrays-1);
    map_height = map_scale * (static_cast<GLfloat>(map_size)/8);
    cd::scaleArray(height_map, 0, map_height);
    cd::smoothEdges2Zero(height_map,map_size,map_size);
    cd::scaleArray(height_map, -waterDepth, map_height);
    VERBOSE(height_map.size());
    std::cout << "Generate snow map" << std::endl;
    snow_map = cd::generateTexture2D(height_map, map_size, map_size, 0, map_height, snow_prob_thresh*map_height);

    GLuint num_levels = static_cast<GLuint>(map_size_exp-4);
    GLuint num_groups = (1<<4);
    std::vector<std::vector<GLfloat> > pyra = cd::generateMultiScaleTexture2D(
            height_map,
            map_size, map_size,
            num_levels);

    std::cout << "Generating height map scale pyramid" << std::endl;
    for(GLuint k = 0; k < pyra.size(); ++k)
        std::cout << "pyamid_level["<<k<<"] size = " << pyra[k].size() << std::endl;

    std::cout << "Create geomipmap" << std::endl;
    terrain_grid.resize(pyra.size());
    for(GLuint k = 0; k < terrain_grid.size(); ++k)
    {
        GLuint scale = (1<<k);
        GLuint size = map_size / scale;
        cd::Mesh terrain_mesh = cd::generateTerrainSimple(&pyra[k][0],size,size,texture_scale,map_scale * static_cast<GLfloat>(scale));
        terrain_mesh.recomputeSurfaceVectors();
        if( k == 0 )
        {
            // Topology visualizer
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
        }
        PRINT("At index " << k);
        terrain_grid[k] = cd::splitMeshIntoGridMultiScale(
                terrain_mesh,
                size,size,
                num_groups);
    }

    // WARNING : HARDCODED num_groups-1
    geolevel = std::vector<std::vector<GLint> >(num_groups-1, std::vector<GLint>(num_groups-1,0));

    std::cout << "Transfer to GPU" << std::endl;
    for(GLuint k = 0; k < terrain_grid.size(); ++k)
    {
        for(GLuint y = 0; y < terrain_grid[k].size_y; ++y)
        {
            for(GLuint x = 0; x < terrain_grid[k].size_x; ++x)
            {
                terrain_grid[k].m_grid[y][x].toGPU();
                terrain_grid[k].m_grid[y][x].bindShader(
                        terrain_shader,
                        "inPosition",
                        "inNormal",
                        "inTangent",
                        "inBitangent",
                        "inTexCoord");
                terrain_grid[k].m_grid[y][x].bindShader(
                        shadow_shader, 
                        "inPosition",
                        "", "", "", "");
            }
        }
    }

    /* Setup terrain shader uniforms */
    camera.bindShader(
            terrain_shader,
            "",
            "mdlMatrix",
            "MVPMatrix",
            "normalMatrix"); 
    GLfloat rock_transition_levels[4] = {0,0.2f*map_height,0.4f*map_height,0.6f*map_height};
    glUniform1i(glGetUniformLocation(terrain_shader, "texUnitRock0"), 0);
    glUniform1i(glGetUniformLocation(terrain_shader, "texUnitRock1"), 1);
    glUniform1i(glGetUniformLocation(terrain_shader, "texUnitRock2"), 2);
    glUniform1i(glGetUniformLocation(terrain_shader, "texUnitSnow"), 3);
    glUniform1i(glGetUniformLocation(terrain_shader, "texUnitGloss"), 4);
    glUniform1i(glGetUniformLocation(terrain_shader, "texUnitNormal"), 5);
    glUniform1i(glGetUniformLocation(terrain_shader, "texUnitSnowLevel"), 6);
    glUniform1i(glGetUniformLocation(terrain_shader, "shadowSampler"), 7);
    glUniform1f(glGetUniformLocation(terrain_shader, "mapSizeX"), map_size_x);
    glUniform1f(glGetUniformLocation(terrain_shader, "mapSizeZ"), map_size_z);
    glUniform1f(glGetUniformLocation(terrain_shader, "mapHeight"), map_height);
    glUniform1f(glGetUniformLocation(terrain_shader, "snowProbThresh"), snow_prob_thresh);
    glUniform1fv(glGetUniformLocation(terrain_shader, "rockTransLevel"), 4, rock_transition_levels);
    printError("terrain shader uniforms");

    /* Setup terrain textures */
    terrain_tex_objID[0] = cd::loadTexture2D("data/textures/grass_leaves_0036_01.jpg");
    terrain_tex_objID[1] = cd::loadTexture2D("data/textures/TexturesCom_RockGrassy0051_1_seamless_S.jpg"); 
    terrain_tex_objID[2] = cd::loadTexture2D("data/textures/TexturesCom_RockMossy0033_1_seamless_S.jpg");
    terrain_tex_objID[3] = cd::loadTexture2D("data/textures/snow_albedo.tif");
    terrain_tex_objID[4] = cd::loadTexture2D("data/textures/snow_albedo_gloss.tif");
    terrain_tex_objID[5] = cd::loadTexture2D("data/textures/snow_albedo_normal.tif");
    printError("terrain shader textures");


    /* Setup sun */
    GLfloat sky_radius = 1000.0f;
    GLfloat sky_angle = M_PI/8;
    sun_mesh = cd::createDome(
            20,
            sky_radius,
            sky_angle,
            1.0f);
    std::cout << "Sun to gpu" << std::endl;
    sun_mesh.toGPU();
    sun_mesh.bindShader(sun_shader, "inDir", "", "", "", "");
    glUniform1f(glGetUniformLocation(sun_shader,"skyRadius"), sky_radius);
    glUniform1f(glGetUniformLocation(sun_shader,"skyAngle"), sky_angle);

#define NUM_WAVELEN 6
    std::vector<GLfloat> wavelen = cd::linspace(400,750,NUM_WAVELEN);
    GLfloat tristimulus[NUM_WAVELEN][3];
    GLfloat sun_light[NUM_WAVELEN];
    GLfloat sun_temp = 4000.0f;

    double s = 0;
    for(GLuint w = 0; w < NUM_WAVELEN; ++w)
    {
        double v = cd::lightIntensity(wavelen[w], sun_temp);
        sun_light[w] = static_cast<GLfloat>(v);
        s += v;
    }
    for(GLuint w = 0; w < NUM_WAVELEN; ++w)
    {
        sun_light[w] *= 1.0/s;
        PRINT("sun_light["<<wavelen[w]<<"] = " << sun_light[w]);
    }

    for(GLuint w = 0; w < NUM_WAVELEN; ++w)
    {
        for(GLuint k = 0; k < 3; ++k)
        {
            tristimulus[w][0] = cd::tristimR31(wavelen[w]);
            tristimulus[w][1] = cd::tristimG31(wavelen[w]);
            tristimulus[w][2] = cd::tristimB31(wavelen[w]);
        }
    }

    GLint waveloc = glGetUniformLocation(sun_shader, "wavelen");
    GLint triloc = glGetUniformLocation(sun_shader, "tristimulus");
    GLint sunloc = glGetUniformLocation(sun_shader, "sunLight");
    glUniform1fv(waveloc, NUM_WAVELEN, &wavelen[0]);
    glUniform3fv(triloc, NUM_WAVELEN, &tristimulus[0][0]);
    glUniform1fv(sunloc, NUM_WAVELEN, sun_light);
    VERBOSE(waveloc);
    VERBOSE(triloc);
    VERBOSE(sunloc);
    printError("sun shader");

    /* Create snow texture */
    std::cout << "Create snow texture" << std::endl;
    glGenTextures(1,&snow_tex);
    glBindTexture(GL_TEXTURE_2D, snow_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

    glTexImage2D(GL_TEXTURE_2D,
            0, GL_RGBA32F,
            map_size, map_size,
            0, GL_RED,
            GL_FLOAT,
            &snow_map[0]);
    VERBOSE(snow_tex);
    printError("snow texture");

    std::cout << "Create height texture" << std::endl;
    glGenTextures(1,&height_tex);
    glBindTexture(GL_TEXTURE_2D, height_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexImage2D(GL_TEXTURE_2D,
            0, GL_RGBA32F,
            map_size, map_size,
            0, GL_RED,
            GL_FLOAT,
            &height_map[0]);
    VERBOSE(height_tex);
    printError("height texture");

    /* Load wind texture */
    std::cout << "Load wind data" << std::endl;
    std::vector<std::string> wind_path_X;
    std::vector<std::string> wind_path_Z;
    for(int i = 1; i < 402; ++i)
    {   
        std::stringstream ss; 
        std::string fn_X, fn_Z;
        ss << "data/textures/windX/wind" << i << ".png";
        ss >> fn_X;
        ss.clear();
        ss << "data/textures/windZ/wind" << i << ".png";
        ss >> fn_Z;
        wind_path_X.push_back(fn_X);
        wind_path_Z.push_back(fn_Z);
    }   
    wind_tex_X = cd::loadTexture3D(wind_path_X);
    wind_tex_Z = cd::loadTexture3D(wind_path_Z);
    printError("wind");

    /* Setup particle render shader */
    glUseProgram(particle_shader);
    glUniform1i(glGetUniformLocation(particle_shader, "shadowSampler"), 0);

    /* Setup particle transform shader */
    std::cout << "Creating particle shader" << std::endl;
    char * shader_src = readFile("shaders/snow_transform_simple.vert");
    GLuint shader_tmp = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(shader_tmp, 1, &shader_src, nullptr);
    glCompileShader(shader_tmp);
    checkShaderCompileStatus(shader_tmp);
    printError("Compile shader");

    particle_transform_shader = glCreateProgram();
    glAttachShader(particle_transform_shader, shader_tmp);
    printError("attach shader");

    const char *feedback_var[] = {"outPosition", "outVelocity"};
    std::cout << "Setting feedback varyings" << std::endl;
    glTransformFeedbackVaryings(particle_transform_shader, 2, feedback_var, GL_INTERLEAVED_ATTRIBS);
    std::cout << "transform feedback varyings set!" << std::endl;
    printError("feedback var");

    std::cout << "linking program" << std::endl;
    glLinkProgram(particle_transform_shader);
    checkProgramLinkStatus(particle_transform_shader);
    printError("link program");
    glUseProgram(particle_transform_shader);

    glUniform1i(glGetUniformLocation(particle_transform_shader, "windMapX"), 0);
    glUniform1i(glGetUniformLocation(particle_transform_shader, "windMapZ"), 1);
    glUniform1f(glGetUniformLocation(particle_transform_shader, "mapSize"), map_scale*map_size);
    glUniform1f(glGetUniformLocation(particle_transform_shader, "mapScale"), map_scale);
    glUniform1f(glGetUniformLocation(particle_transform_shader, "maxX"), map_size_x); 
    glUniform1f(glGetUniformLocation(particle_transform_shader, "maxZ"), map_size_z);
    glUniform1f(glGetUniformLocation(particle_transform_shader, "seedY"), 2*map_height);


    /* Generate particle data */
    std::cout << "Generate particles" << std::endl;
    float dt = 1.0f / 60.0f;
    glm::vec3 g(0.0f,-9.82f,0.0f);
    num_particles = map_size*map_size;
    VERBOSE(num_particles);
    particles.resize(2*num_particles);

    for(GLuint i = 0; i < num_particles; ++i)
    {
        // position
        particles[2*i] = glm::vec3(
                (map_scale*map_size-1) * float(std::rand())/RAND_MAX,
                map_height + 2*(map_height)*(0.5f - float(std::rand())/RAND_MAX),
                (map_scale*map_size-1) * float(std::rand())/RAND_MAX);
        // velocity
        particles[2*i+1] = glm::vec3(0,0,0);
    }

    const GLfloat flake_size = 0.1f;
    GLfloat billboard_data[] = {
        -flake_size, -flake_size, 0.0f,
        flake_size, -flake_size, 0.0f,
        -flake_size, flake_size, 0.0f,
        flake_size, flake_size, 0.0f
    };

    GLfloat billboard_tex_coord[] = {
        -1.0f,-1.0f,
        -1.0f,1.0f,
        1.0f,-1.0f,
        1.0f,1.0f
    };

    std::cout << "Upload particles to GPU" << std::endl;
    glGenVertexArrays(buffer_cnt, particle_vao);
    glGenVertexArrays(buffer_cnt, particle_transform_vao);
    glGenBuffers(buffer_cnt, particle_vbo); 
    GLuint billboard_vbo;
    GLuint billboard_tex_vbo;
    for(GLint k = 0; k < buffer_cnt; ++k)
    {
        glBindVertexArray(particle_vao[k]);

        // Instanced attributes
        glGenBuffers(1,&billboard_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, billboard_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(billboard_data), billboard_data, GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)(0));

        glGenBuffers(1,&billboard_tex_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, billboard_tex_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(billboard_tex_coord), billboard_tex_coord, GL_STATIC_DRAW);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, (void*)(0));

        // Transform attributes
        glBindBuffer(GL_ARRAY_BUFFER, particle_vbo[k]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*particles.size(), &particles[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (void*)(3*sizeof(GLfloat)));

        // Divisors
        glVertexAttribDivisor(0,1);
        glVertexAttribDivisor(1,1);
        glVertexAttribDivisor(2,0);
        glVertexAttribDivisor(3,0);
    }

    for(GLint k = 0; k < buffer_cnt; ++k)
    {
        glBindVertexArray(particle_transform_vao[k]);
        // Transform attributes
        glBindBuffer(GL_ARRAY_BUFFER, particle_vbo[k]);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (void*)(3*sizeof(GLfloat)));
        //glVertexAttribDivisor(0,0);
        //glVertexAttribDivisor(1,0);
    }

    current_buffer = 0;
    printError("data to particle buffers");

    /* Setup ground mesh (Used for shadow map only)*/
    std::cout << "Creating base ground" << std::endl;
    GLfloat ground_size = map_scale * static_cast<GLfloat>(map_size);
    ground_mesh = cd::createQuad(-10*ground_size,-10*ground_size,21*ground_size,21*ground_size);
    ground_mesh.toGPU();
    PRINT("ground toGPU");
    ground_mesh.bindShader(
            ground_shader,
            "inPosition",
            "inNormal",
            "",
            "",
            "inTexCoord");
    glUniform1i(glGetUniformLocation(ground_shader,"texUnit"),0);
    glUniform1f(glGetUniformLocation(ground_shader, "texScale"), 4*map_scale*texture_scale*static_cast<GLfloat>(map_size));
    ground_tex = terrain_tex_objID[0]; //cd::loadTexture2D("data/textures/grass_leaves_0036_01.jpg");
    printError("ground mesh");
    PRINT("ground done");

    /* Setup water */ 
    water_mesh = cd::createQuad(-2*ground_size,-2*ground_size,5*ground_size,5*ground_size); 
    water_mesh.toGPU();
    water_mesh.bindShader(
            water_shader,
            "inPosition",
            "inNormal",
            "",
            "",
            "inTexCoord");
    printError("water mesh");

    /* Setup clouds */
    cloud_mesh = cd::createDome(
            100,
            sky_radius,
            sky_angle,
            1.0f);
    //cloud_mesh.translate(map_size/2,0,map_size/2);
    cloud_mesh.toGPU();
    cloud_mesh.bindShader(
            cloud_shader,
            "inPosition",
            "","","",
            "inTexCoord");
    camera.bindShader(
            cloud_shader,
            "projMatrix",
            "mdlMatrix");
    float sky_height = sky_radius * (1.0 - std::cos(sky_angle));
    VERBOSE(sky_height);
    glUniform1i(glGetUniformLocation(cloud_shader, "texUnit"), 0);
    glUniform1f(glGetUniformLocation(cloud_shader, "skyRadius"), sky_radius);
    glUniform1f(glGetUniformLocation(cloud_shader, "skyHeight"), sky_height);

    /* Of some reason becomes wierd */
    /*
    std::cout << "Generating clouds" << std::endl;
    std::vector<GLfloat> cloud_tex = cd::generateTexture3D(200,200,400);
    cd::scaleArray(cloud_tex, 0.0f, 1.0f);
    std::cout << "Clouds generated" << std::endl;
    cloud_tex_objID = cd::loadTexture3D(cloud_tex, 200, 200, 400);
    */

    std::vector<std::string> paths;
    for(int i = 1; i < 402; ++i)
    {
        std::stringstream ss;
        std::string filename;
        ss << "data/textures/clouds/cloud" << i << ".png";
        ss >> filename;
        paths.push_back(filename);
    }
    cloud_tex_objID = cd::loadTexture3D(paths);
    printError("Clouds");

    /* Setup reflections */
    GLfloat reflection_billboard_data[] = {
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f
    };

    GLfloat reflection_billboard_tex_coord[] = {
        0.0f,0.0f,
        0.0f,1.0f,
        1.0f,0.0f,
        1.0f,1.0f
    };

    glGenVertexArrays(1, &reflection_billboard_vao);
    glBindVertexArray(reflection_billboard_vao);
    GLuint reflection_billboard_vbo[2];
    glGenBuffers(2, reflection_billboard_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, reflection_billboard_vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(reflection_billboard_data), reflection_billboard_data, GL_STATIC_DRAW); 
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)(0));
    glBindBuffer(GL_ARRAY_BUFFER, reflection_billboard_vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(billboard_tex_coord), reflection_billboard_tex_coord, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)(0));
    glUseProgram(reflection_shader);
    glUniform1i(glGetUniformLocation(water_shader, "tex"), 0);

    /* Setup camera */
    camera.lookTowards(
            glm::vec3(map_size/2,map_height+10,map_size/2),
            glm::vec3(1,0,1));
    camera.bindShader(terrain_shader, "projMatrix", "mdlMatrix");
    printError("init camera shader");
    camera.bindShader(particle_shader, "projMatrix", "mdlMatrix");
    printError("init camera particle");

    std::cout << "Init complete" << std::endl;

    /* Setup shadow map FBO */
    glGenFramebuffers(1,&fbo);
    printError("glGenFramebuffers");
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    printError("glBindFramebuffer");
    glGenTextures(1, &reflection_tex);
    glBindTexture(GL_TEXTURE_2D, reflection_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);//GL_CLAMP_TO_EDGE); 
    printError("reflection glTexImage2D");

    glGenTextures(1, &reflection_depth_tex);
    glBindTexture(GL_TEXTURE_2D, reflection_depth_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);//GL_CLAMP_TO_EDGE); 
    printError("shadow glTexImage2D");

    updateFBOTexReflective();

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, reflection_tex, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, reflection_depth_tex, 0);
    printError("glFrameBufferTexture render target");
    GLenum draw_buffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, draw_buffers);
    printError("glDrawBuffers");

    std::cout << "FBO = " << fbo << std::endl;
    //std::cout << "cube_map_depth_tex = " << cube_map_depth_tex << std::endl;
    std::cout << "reflection_tex = " << reflection_tex << std::endl;
    GLenum fbo_status = cd::checkFBO();
    if( fbo_status != GL_FRAMEBUFFER_COMPLETE )
        exit(1);

    glGenFramebuffers(1,&fbo_shadow);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_shadow);
    glGenTextures(1, &shadow_depth_tex);
    glBindTexture(GL_TEXTURE_2D, shadow_depth_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 

    updateFBOTexShadow();

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadow_depth_tex, 0);
    glDrawBuffer(GL_NONE);
    printError("shadow fbo");

    fbo_status = cd::checkFBO();
    if( fbo_status != GL_FRAMEBUFFER_COMPLETE )
        exit(1);
}

/* Various update functions, called each loop */
void updateTime()
{
    elapsed_time += 0.02f; // Approximate
    glUseProgram(particle_transform_shader);
    glUniform1f(glGetUniformLocation(particle_transform_shader,"time"),elapsed_time);
    glUseProgram(cloud_shader);
    glUniform1f(glGetUniformLocation(cloud_shader, "time"), elapsed_time);
    GLfloat th = M_PI + (current_time / 24) * 2 * M_PI;
    GLfloat ph = 0; 
    sun_dir = glm::vec3(sin(th)*cos(ph),cos(th),sin(th)*sin(ph));
    sun_dir = glm::normalize(sun_dir);
}

void updateCamera() {

    int w,h;
    glutGetWindowSize(&w, &h);
    camera.setAspect(w,h);
    float rot_vel = 1.0;
    float dt = 0.02f;
    float lin_vel = camera_speed * dt;
    if( fast_cam )
        lin_vel = camera_speed_fast * dt;
    camera.rotateLocal(rot_vel * mouseY, rot_vel * mouseX);
    camera.translateLocal(lin_vel * moveX, 0, lin_vel * moveZ);      
    if( ! enable_cam_fly )
    {
        glm::vec3 pos = camera.getPos();
        float y = getHeight(&height_map[0], map_size, map_scale, pos.x, pos.z);
        camera.setPosY(1.8+y);
    }
    mouseX = 0;
    mouseY = 0;
    float bound = 1.1f*(map_scale * map_size / 2.0f);

    // reflection camera
    mirror_camera = camera;
    mirror_camera.setScale(1,-1,1);

    // shadow camera
    shadow_camera.setOrtho(-bound,bound,-bound,bound,-bound,bound);
    shadow_camera.lookTowards(glm::vec3(map_size*map_scale/2,0,map_size*map_scale/2), -sun_dir);
}

void updateShaders()
{
    glUseProgram(terrain_shader);
    glUniform1i(glGetUniformLocation(terrain_shader, "blinnPhong"), blinnPhong);
    glUniform1i(glGetUniformLocation(terrain_shader, "enableShadow"), enable_shadow);
    glUniform1i(glGetUniformLocation(terrain_shader, "enableSnowTexture"), enable_snow_texture);
    glUniform1i(glGetUniformLocation(terrain_shader, "enableNormalMapping"), enable_normal_mapping);
    glUniform1i(glGetUniformLocation(terrain_shader, "enableGlossMapping"), enable_gloss_mapping);
    glUniform1f(glGetUniformLocation(terrain_shader, "depthMargin"), depth_margin); 
    glUniform1i(glGetUniformLocation(terrain_shader, "depthNumSample"), depth_num_sample); 

    glUseProgram(sun_shader);
    glUniform1i(glGetUniformLocation(sun_shader, "enableRayleigh"), enable_rayleigh);
    glUniform1i(glGetUniformLocation(sun_shader, "enableMie"), enable_mie);

    shadow_camera.bindShader(
            particle_shader,
            "",
            "",
            "shadowMVP",
            "");
    glUniform1i(glGetUniformLocation(particle_shader, "enableShadow"), enable_shadow);
    camera.bindShader(
            viz_shader,
            "projMatrix",
            "mdlMatrix");
}

void updateParticles()
{
    glUseProgram(particle_transform_shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, wind_tex_X);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, wind_tex_Z);

    glBindVertexArray(particle_transform_vao[(current_buffer+1)%buffer_cnt]);
    glEnable(GL_RASTERIZER_DISCARD);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, particle_vbo[current_buffer]);
    printError("bindBufferBase");
    glBeginTransformFeedback(GL_POINTS);
    printError("begin transform");
    glDrawArrays(GL_POINTS, 0, num_particles);
    printError("draw arrays");
    glEndTransformFeedback();
    glDisable(GL_RASTERIZER_DISCARD);
    glFlush();
    printError("transform feedback");
}

/* Used to check if an object is in the camera frustm */
bool frustumCheck(cd::GLMCamera & camera, cd::Mesh & mesh)
{
    return camera.canSee(mesh.m_bounding_box);
}   

/* Main draw function */
void draw(GLuint fbo, cd::GLMCamera & camera, GLbitfield flags = (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT), GLenum cullMode = GL_BACK)
{

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClear(flags);

    // Draw sky
    tim.tic("skydome");
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    sun_mesh.bindShader(
            sun_shader,
            "inDir",
            "","","",
            "");
    camera.bindShader(
            sun_shader,
            "projMatrix",
            "mdlMatrix");
    glUniform3fv(glGetUniformLocation(sun_shader,"sunDir"), 1, &sun_dir[0]);
    sun_mesh.draw();
    //glFinish();
    tim.toc("skydome");

    // Draw clouds
    if( enable_clouds )
    {
        tim.tic("clouds");
        camera.bindShader(
                cloud_shader,
                "projMatrix",
                "mdlMatrix");
        glUniform1i(glGetUniformLocation(cloud_shader, "enableProj"), enable_cloud_projection);
        glUniform1i(glGetUniformLocation(cloud_shader, "enableFog"), enable_cloud_fog);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, cloud_tex_objID);
        cloud_mesh.draw();
        printError("display clouds");
        tim.toc("clouds");
    }

    // Draw terrain
    glEnable(GL_CULL_FACE);
    glCullFace(cullMode);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    camera.bindShader(
            terrain_shader,
            "",
            "mdlMatrix",
            "MVPMatrix",
            "normalMatrix");
    shadow_camera.bindShader(
            terrain_shader,
            "",
            "",
            "shadowMVP",
            "");
    glUniform3fv(glGetUniformLocation(terrain_shader,"sunDir"), 1, &sun_dir[0]);

    tim.tic("frustum_cull");
    int numCulled = 0;
    for(GLuint y = 0; y < terrain_grid[0].size_y; ++y)
    {
        for(GLuint x = 0; x < terrain_grid[0].size_x; ++x)
        {

            // Frustum check
            if( ! frustumCheck(camera,terrain_grid[0].m_grid[y][x]) )
            {
                geolevel[y][x] = terrain_grid.size()-1;
                numCulled += 1;
                continue;
            }

            glm::vec3 pos_diff = terrain_grid[0].m_grid[y][x].m_centroid - camera.getPos();
            GLfloat dist = glm::length(pos_diff);
            GLuint geo_level = terrain_grid.size()-1;
            for(GLuint k = 0; k < terrain_grid.size(); ++k)
            {
                if( dist < (1<<k)*dist_step )
                {
                    geo_level = k;
                    break;
                }
            }

            geolevel[y][x] = geo_level;
        }
    }
    //std::cout << "percentage culled = " << 100.0f * static_cast<float>(numCulled) / static_cast<float>(terrain_grid[0].size_x * terrain_grid[0].size_y) << std::endl;
    tim.toc("frustum_cull");

    tim.tic("draw_terrain");
    GLint size_x = static_cast<GLint>(terrain_grid[0].size_x);
    GLint size_y = static_cast<GLint>(terrain_grid[0].size_y);
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
    glBindTexture(GL_TEXTURE_2D, snow_tex); 
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, shadow_depth_tex);

    printError("bindImageTex");
    for(GLuint y = 0; y < size_y; ++y)
    {
        for(GLuint x = 0; x < size_x; ++x)
        {
            if(geolevel[y][x] == -1 )
                continue;

            terrain_grid[geolevel[y][x]].m_grid[y][x].draw();

            if( (x>0) and geolevel[y][x-1] > geolevel[y][x] )
                terrain_grid[geolevel[y][x]].m_grid[y][x].drawEdge(1,cd::LEFT);
            else
                terrain_grid[geolevel[y][x]].m_grid[y][x].drawEdge(0,cd::LEFT);

            if( (y>0) and geolevel[y-1][x] > geolevel[y][x] )
                terrain_grid[geolevel[y][x]].m_grid[y][x].drawEdge(1,cd::TOP);
            else
                terrain_grid[geolevel[y][x]].m_grid[y][x].drawEdge(0,cd::TOP);

            if( (x<(size_x-1)) and geolevel[y][x+1] > geolevel[y][x] )
                terrain_grid[geolevel[y][x]].m_grid[y][x].drawEdge(1,cd::RIGHT);
            else
                terrain_grid[geolevel[y][x]].m_grid[y][x].drawEdge(0,cd::RIGHT);

            if( (y<(size_y-1)) and geolevel[y+1][x] > geolevel[y][x] )
                terrain_grid[geolevel[y][x]].m_grid[y][x].drawEdge(1,cd::BOTTOM);
            else
                terrain_grid[geolevel[y][x]].m_grid[y][x].drawEdge(0,cd::BOTTOM);
        }
    }
    printError("display terrain");
    tim.toc("draw_terrain");
}

/* Loop callback function */
void display()
{
    tim.tic("all");
    printError("pre display");

    updateTime();
    printError("updateTime");

    updateCamera();
    printError("updateCamera");

    updateShaders();
    printError("updateShaders");

    if( enable_particles )
    {
        tim.tic("update_particles");
        updateParticles();
        //glFinish();
        tim.toc("update_particles");
        printError("updateParticles");
    }

    if( enable_shadow )
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_shadow);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDisable(GL_CULL_FACE);

        // Draw ground 
        tim.tic("ground_level");
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        shadow_camera.bindShader(
                ground_shader,
                "projMatrix",
                "mdlMatrix");
        ground_mesh.draw();
        tim.toc("ground_level");

        shadow_camera.bindShader(
                shadow_shader, 
                "",
                "",
                "MVP",
                "");
        tim.tic("shadow_geomipmap");
        for(GLuint y = 0; y < terrain_grid[0].size_y; ++y)
        {
            for(GLuint x = 0; x < terrain_grid[0].size_x; ++x)
            {

                glm::vec3 pos_diff = terrain_grid[0].m_grid[y][x].m_centroid - camera.getPos();
                GLfloat dist = glm::length(pos_diff);
                GLuint geo_level = terrain_grid.size()-1;
                for(GLuint k = 0; k < terrain_grid.size(); ++k)
                {
                    if( dist < (1<<k)*dist_step )
                    {
                        geo_level = k;
                        break;
                    }
                }
                geolevel[y][x] = geo_level;
            }
        }
        tim.toc("shadow_geomipmap");
        GLint size_x = static_cast<GLint>(terrain_grid[0].size_x);
        GLint size_y = static_cast<GLint>(terrain_grid[0].size_y);
        tim.tic("shadow_draw");
        for(GLuint y = 0; y < size_y; ++y)
        {
            for(GLuint x = 0; x < size_x; ++x)
            {
                if(geolevel[y][x] == -1 )
                    continue;

                terrain_grid[geolevel[y][x]].m_grid[y][x].draw();

                if( (x>0) and geolevel[y][x-1] > geolevel[y][x] )
                    terrain_grid[geolevel[y][x]].m_grid[y][x].drawEdge(1,cd::LEFT);
                else
                    terrain_grid[geolevel[y][x]].m_grid[y][x].drawEdge(0,cd::LEFT);

                if( (y>0) and geolevel[y-1][x] > geolevel[y][x] )
                    terrain_grid[geolevel[y][x]].m_grid[y][x].drawEdge(1,cd::TOP);
                else
                    terrain_grid[geolevel[y][x]].m_grid[y][x].drawEdge(0,cd::TOP);

                if( (x<(size_x-1)) and geolevel[y][x+1] > geolevel[y][x] )
                    terrain_grid[geolevel[y][x]].m_grid[y][x].drawEdge(1,cd::RIGHT);
                else
                    terrain_grid[geolevel[y][x]].m_grid[y][x].drawEdge(0,cd::RIGHT);

                if( (y<(size_y-1)) and geolevel[y+1][x] > geolevel[y][x] )
                    terrain_grid[geolevel[y][x]].m_grid[y][x].drawEdge(1,cd::BOTTOM);
                else
                    terrain_grid[geolevel[y][x]].m_grid[y][x].drawEdge(0,cd::BOTTOM);
            }
        }
        tim.toc("shadow_draw");
    }

    if( enable_shadow_map_debug ) 
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glUseProgram(reflection_shader); 
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, shadow_depth_tex);
        glBindVertexArray(reflection_billboard_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glutSwapBuffers();
        current_buffer = (current_buffer+1) % buffer_cnt;
        tim.toc("all");
        if( quit_application )
            exit(0);
        return;
    }  

    // Draw original scene 
    tim.tic("scene_org");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0,0,0,water_alpha);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glUseProgram(terrain_shader);
    glUniform1i(glGetUniformLocation(terrain_shader, "isFlipped"),0);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    draw(0,camera,GL_DEPTH_BUFFER_BIT);
    tim.toc("scene_org");

    // Draw water mask
    tim.tic("water_mask");
    glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 0xFF, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    camera.bindShader(
            water_shader,
            "projMatrix",
            "mdlMatrix");
    glUniform1f(glGetUniformLocation(water_shader, "alpha"), water_alpha);
    water_mesh.draw();
    glBlendEquation(GL_FUNC_ADD);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    tim.toc("water_mask");

    // Draw reflections into fbo ( texture )
    tim.tic("scene_mirror");
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glUseProgram(terrain_shader);
    glUniform1i(glGetUniformLocation(terrain_shader, "isFlipped"), 1);
    draw(fbo,mirror_camera, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_FRONT);
    tim.toc("scene_mirror");

    // Draw reflection onto screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 0xFF, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glUseProgram(reflection_shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, reflection_tex);
    glBindVertexArray(reflection_billboard_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDepthMask(GL_TRUE);

    // Draw topology visualization
    if( enable_topology_viz )
    {
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);
        glUseProgram(viz_shader);
        viz_mesh.draw();
        glDisable(GL_BLEND);
        printError("display topology");
    }

    // Draw particles
    if( enable_particles )
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        tim.tic("draw_particles");
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_STENCIL_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);

        glUseProgram(particle_shader);
        camera.bindShader(particle_shader, "projMatrix", "mdlMatrix");
        glBindVertexArray(particle_vao[current_buffer]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, shadow_depth_tex);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP,0,4,num_particles);
        printError("draw particles");
        //glFinish();
        tim.toc("draw_particles");
    }

    glBindFramebuffer(GL_FRAMEBUFFER,0);

    if( take_screenshot )
    {
        cv::Mat image(window_height, window_width, CV_8UC3);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glReadPixels(0, 0, window_width, window_height, GL_BGR, GL_UNSIGNED_BYTE, image.data);
        cv::flip(image,image,0);
        std::stringstream ss;
        ss << "data/record/image" << image_counter++ << ".png" << std::endl;
        cv::imwrite(ss.str(), image);
        take_screenshot = false;
    }

    glutSwapBuffers();

    // Swap particle transform buffers
    current_buffer = (current_buffer+1) % buffer_cnt;

    tim.toc("all");

    VERBOSE(current_time);

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
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);
    glutInitContextVersion(4, 2);
    glutInitWindowSize (window_width, window_height);
    glutCreateWindow ("OpenWorld");
    glutDisplayFunc(display);
    GLint maxTexUnits;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTexUnits);
    VERBOSE(maxTexUnits);
    init ();
    glutTimerFunc(20, &timer, 0);

    glutKeyboardFunc(keyboardCallback);
    glutKeyboardUpFunc(keyboardCallbackRelease);
    glutPassiveMotionFunc(mouseCallback);

    glutMainLoop();
    exit(0);
}
