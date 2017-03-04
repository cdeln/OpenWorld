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
#include "debug.hpp"
#include "shader.hpp"
#include <cmath>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <glm/gtx/string_cast.hpp>

bool quit_application = false;
GLfloat elapsed_time;

std::vector<GLfloat> height_map;
std::vector<GLfloat> snow_map;

cd::Mesh terrain_mesh;

std::vector<cd::MeshGrid> terrain_grid;
cd::Mesh sun_mesh;
GLuint map_size;
GLfloat map_height;
GLuint terrain_tex_objID[7];
GLuint snow_tex;
GLuint height_tex;
GLuint wind_tex_X;
GLuint wind_tex_Z;
GLuint num_particles;

cd::GLMCamera camera;

// References to shader programs
GLuint terrain_shader;
GLuint particle_shader;
GLuint particle_transform_shader;
GLuint sun_shader;

std::vector<glm::vec3> particles;

GLint current_buffer;
const GLint buffer_cnt = 2;
GLuint particle_vao[buffer_cnt];
GLuint particle_vbo[buffer_cnt];

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
			"shaders/terrain_snow_image.vert",
			"shaders/terrain_snow_image.frag");
	particle_shader = loadShadersG(
			"shaders/particle.vert",
			"shaders/particle.frag",
			"shaders/particle.geom");
	sun_shader = loadShaders(
			"shaders/sun.vert",
			"shaders/sun.frag");

	// Load terrain data
	map_size = (1<<10);
	map_height = static_cast<GLfloat>(map_size)/8;
	std::cout << "Generate height_map" << std::endl;
	height_map = cd::generateTexture2D(map_size, map_size, -map_height, map_height);
	std::cout << "Generate snow map" << std::endl;
	snow_map = cd::generateTexture2D(height_map, map_size, map_size, -map_height, map_height, map_height/2);

	GLuint num_levels = 6;
	std::vector<std::vector<GLfloat> > pyra = cd::generateMultiScaleTexture2D(
			height_map,
			map_size, map_size,
			num_levels);

	std::cout << "Generate terrain_mesh" << std::endl;
	terrain_mesh = cd::generateTerrain(&height_map[0], map_size, map_size, 0.1f, 1.0f);

	for(GLuint k = 0; k < pyra.size(); ++k)
		std::cout << "pyamid_level["<<k<<"] size = " << pyra[k].size() << std::endl;


	std::cout << "Create geomipmap" << std::endl;
	GLuint num_groups = 8;
	terrain_grid.resize(pyra.size());
	for(GLuint k = 0; k < terrain_grid.size(); ++k)
	{
		GLuint scale = (1<<k);
		GLuint size = map_size / scale;
		cd::Mesh terrain_mesh = cd::generateTerrain(&pyra[k][0],size,size,0.1f,scale);
		terrain_grid[k] = cd::splitMeshIntoGrid(
				terrain_mesh,
				size,size,
				num_groups);
	}

	/* Does not work
	   std::cout << "Constraining geomipmap" << std::endl;
	   for(GLint k = 0; k < terrain_grid.size(); ++k)
	   {
	   for(GLuint y = 0; y < terrain_grid[k].size_y; ++y)
	   {
	   for(GLuint x = 0; x < terrain_grid[k].size_x; ++x)
	   {
	   if( k > 0 )
	   {
	   cd::constrainMeshGridEdges(
	   terrain_grid[k-1].m_grid[y][x],
	   terrain_grid[k].m_grid[y][x],
	   terrain_grid[k-1].patch_size_x,
	   terrain_grid[k-1].patch_size_y,
	   terrain_grid[k].patch_size_x,
	   terrain_grid[k].patch_size_y);
	   }
	   }
	   }
	   }
	   */

	// TODO : Gives edge artifacts
	std::cout << "Generate surface vectors" << std::endl;
	for(GLuint k = 0; k < terrain_grid.size(); ++k)
	{
		for(GLuint y = 0; y < terrain_grid[k].size_y; ++y)
		{
			for(GLuint x = 0; x < terrain_grid[k].size_x; ++x)
			{
				terrain_grid[k].m_grid[y][x].recomputeSurfaceVectors();
			}
		}
	}

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
				camera.bindShader(terrain_shader, "projMatrix", "mdlMatrix");
			}
		}
	}

	terrain_mesh.toGPU();
	terrain_mesh.bindShader(
			terrain_shader,
			"inPosition",
			"inNormal",
			"inTangent",
			"inBitangent",
			"inTexCoord"); 
	camera.bindShader(terrain_shader, "projMatrix", "mdlMatrix");

	glUniform1i(glGetUniformLocation(terrain_shader, "texUnitRock0"), 0);
	glUniform1i(glGetUniformLocation(terrain_shader, "texUnitRock1"), 1);
	glUniform1i(glGetUniformLocation(terrain_shader, "texUnitRock2"), 2);
	glUniform1i(glGetUniformLocation(terrain_shader, "texUnitRockSlope"), 3);
	glUniform1i(glGetUniformLocation(terrain_shader, "texUnitSnow"), 4);
	glUniform1i(glGetUniformLocation(terrain_shader, "texUnitGloss"), 5);
	glUniform1i(glGetUniformLocation(terrain_shader, "texUnitNormal"), 6);
	glUniform1i(glGetUniformLocation(terrain_shader, "texUnitSnowLevel"), 0);
	printError("uniforms");

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
	std::cout << "Sun to gpu" << std::endl;
	sun_mesh.toGPU();
	sun_mesh.bindShader(sun_shader, "inPosition", "", "", "", "inTexCoord");
	std::cout << "Sun bind shader" << std::endl;
	printError("sub_shader");

	// Snow
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

	// Wind
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


	// Particle
	std::cout << "Creating particle shader" << std::endl;
	// Shader program 
	char * shader_src = readFile("shaders/snow_map_transform.vert");
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

	glUniform1i(glGetUniformLocation(particle_transform_shader, "snowMap"), 0);
	glUniform1i(glGetUniformLocation(particle_transform_shader, "heightMap"), 1);
	glUniform1i(glGetUniformLocation(particle_transform_shader, "windMapX"), 0);
	glUniform1i(glGetUniformLocation(particle_transform_shader, "windMapZ"), 1);
	glUniform1f(glGetUniformLocation(particle_transform_shader, "mapSize"), map_size);
	glUniform1f(glGetUniformLocation(particle_transform_shader, "maxX"), map_size);
	glUniform1f(glGetUniformLocation(particle_transform_shader, "maxZ"), map_size);
	glUniform1f(glGetUniformLocation(particle_transform_shader, "seedY"), 2*map_height);

	// Data
	std::cout << "Generate particles" << std::endl;
	float dt = 1.0f / 60.0f;
	glm::vec3 g(0.0f,-9.82f,0.0f);
    num_particles = map_size*map_size;
	particles.resize(2*num_particles);

	for(GLuint i = 0; i < num_particles; ++i)
	{
		// position
		particles[2*i] = glm::vec3(
				(map_size-1) * float(std::rand())/RAND_MAX,
				map_height + (map_height)*(0.5f - float(std::rand())/RAND_MAX),
				(map_size-1) * float(std::rand())/RAND_MAX);
		particles[2*i+1] = glm::vec3(0,0,0);
	}

	std::cout << "Upload particles to GPU" << std::endl;
	glGenVertexArrays(buffer_cnt, particle_vao);
	glGenBuffers(buffer_cnt, particle_vbo); 
	for(GLint k = 0; k < buffer_cnt; ++k)
	{
		glBindVertexArray(particle_vao[k]);
		glBindBuffer(GL_ARRAY_BUFFER, particle_vbo[k]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*particles.size(), &particles[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), (void*)(3*sizeof(GLfloat)));
	}
	current_buffer = 0;
	printError("data to particle buffers");

	// Camera
	camera.lookTowards(
			glm::vec3(map_size/2,map_size/8,map_size/2),
			glm::vec3(1,-1,0));
	camera.bindShader(terrain_shader, "projMatrix", "mdlMatrix");
	printError("init camera shader");
	camera.bindShader(particle_shader, "projMatrix", "mdlMatrix");
	printError("init camera particle");

	std::cout << "Init complete" << std::endl;

    // Time
    elapsed_time = 0;
}

void updateTime()
{
    glUseProgram(particle_transform_shader);
    glUniform1f(glGetUniformLocation(particle_transform_shader,"time"),elapsed_time);
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
	glUseProgram(particle_transform_shader);
	glBindImageTexture(0, snow_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(1, height_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, wind_tex_X);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, wind_tex_Z);

	glBindVertexArray(particle_vao[(current_buffer+1)%buffer_cnt]);
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

bool frustumCheck(cd::Mesh & mesh)
{
	//return camera.boxCutsFrustum(mesh.m_bounding_box);
	return camera.canSee(mesh.m_bounding_box);
}   

void display(void)
{
	printError("pre display");

    updateTime();
    printError("updateTime");

	updateCamera();
	printError("updateCamera");

	updateParticles();
	printError("updateParticles");

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

	glBindImageTexture(0, snow_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	for(GLuint y = 0; y < terrain_grid[0].size_y; ++y)
		for(GLuint x = 0; x < terrain_grid[0].size_x; ++x)
		{

			// Frustum check
			if( ! frustumCheck(terrain_grid[0].m_grid[y][x]) )
			{
				continue;
			}

			glm::vec3 pos_diff = terrain_grid[0].m_grid[y][x].m_centroid - camera.getPos();
			GLfloat dist = glm::length(pos_diff);
			GLuint geo_level = terrain_grid.size()-1;
			GLfloat dist_step = 100.0f;
			for(GLuint k = 0; k < terrain_grid.size(); ++k)
			{
				if( dist < (1<<k)*dist_step )
				{
					geo_level = k;
					break;
				}
			}

			glBindVertexArray(terrain_grid[geo_level].m_grid[y][x].m_vao);
			terrain_grid[geo_level].m_grid[y][x].bindShader(
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

			printError("bindImageTex");

			terrain_grid[geo_level].m_grid[y][x].draw();
		}
	//terrain_mesh.draw();
	printError("display terrain");

	// Draw particles
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(particle_shader);
	camera.bindShader(particle_shader, "projMatrix", "mdlMatrix");
	glBindVertexArray(particle_vao[current_buffer]);
	glDrawArrays(GL_POINTS, 0, num_particles);
	printError("draw particles");


	glutSwapBuffers();

	current_buffer = (current_buffer+1) % buffer_cnt;

	if( quit_application )
		exit(0);
}

void timer(int i)
{
	glutTimerFunc(20, &timer, i);
	glutPostRedisplay();
    elapsed_time += 0.02f;
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
	glutInitContextVersion(4, 2);
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
