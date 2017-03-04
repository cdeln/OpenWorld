#ifndef __APPLE__

#include "gl_include.h"
#include "MicroGlut.h"
#include "GL_utilities.h"
#include <iostream>
#include <vector>
//#include <glm/vec4.hpp>
//#include <glm/mat4x4.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

GLuint compute_program;
GLuint draw_program;
GLuint vao;
int num_particles = 128;

void display()
{

	printError("predisplay");

	glUseProgram(compute_program);
	glDispatchCompute(num_particles, 1, 1);
	printError("dispatch");

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	glm::mat4 Projection = glm::perspective(90.0f, 4.0f / 3.0f, 0.1f, 100.f);
	glm::mat4 View = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -30.0f));
	View = glm::rotate(View, 30.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(View));
	glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(Projection));
	printError("uniform");

	glBindVertexArray(vao);
	glDrawArrays(GL_POINTS, 0, num_particles);

	printError("display");

	glutSwapBuffers();
}

void timer(int i)
{
	glutTimerFunc(20, &timer, i);
	glutPostRedisplay();
}

int main(int argc, char ** argv)
{

	glutInit(&argc, argv);
	//glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
	glutInitContextVersion(4,5);
	glutInitWindowSize (600,600);
	glutCreateWindow ("ComputeTest");
	glutDisplayFunc(display);

	char * shader_source = readFile("shaders/compute_test.comp");

	GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(shader, 1, &shader_source, NULL);
	glCompileShader(shader); 
	printShaderInfoLog(shader, "shader");
	printError("shader compile");

	compute_program = glCreateProgram();
	glAttachShader(compute_program, shader);
	glLinkProgram(compute_program);

	std::vector<glm::vec4> pos_data(num_particles);
	std::vector<glm::vec4> vel_data(num_particles);
	for(int i = 0; i < num_particles; ++i)
	{
		pos_data[i] = glm::gaussRand(glm::vec4(0,0,0,1), glm::vec4(1,0.2,1,0));
		vel_data[i] = glm::vec4(0);
	}

	glGenVertexArrays(1,&vao);
	glBindVertexArray(vao);

	GLuint pos_vbo, vel_vbo;
	glGenBuffers(1,&pos_vbo);
	glGenBuffers(1,&vel_vbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, vel_vbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4)*num_particles, &vel_data[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, pos_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4)*num_particles, &pos_data[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (char*)0 + 0*sizeof(GLfloat));

	const GLuint ssbos[] = {pos_vbo, vel_vbo};
	glBindBuffersBase(GL_SHADER_STORAGE_BUFFER, 0, 2, ssbos);

	float dt = 1.0f / 60.0f;
	glUseProgram(compute_program);
	glUniform1f(0,dt);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	// run
	glutTimerFunc(20, &timer, 0);
	glutMainLoop();
	exit(0);
}

/*
#define NUM_PARTICLES 1024*1024
#define WORK_GROUP_SIZE 128

struct pos
{
float x, y, z, w;
};

int main(int argc, char ** argv)
{
// need to do the following for both position, velocity, and colors of the particles:
GLuint  posSSbo;
glGenBuffers(1, &posSSbo);
glBindBuffer(GL_SHADER_STORAGE_BUFFER, posSSbo);
glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(struct pos), NULL, GL_STATIC_DRAW);
GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;


// the invalidate makes a big difference when re-writing
pos *points = (pos *) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, NUM_PARTICLES * sizeof(pos), bufMask);
for( int i = 0; i < NUM_PARTICLES; i++ )
{
points[ i ].x = Ranf( XMIN, XMAX );
points[ i ].y = Ranf( YMIN, YMAX );
points[ i ].z = Ranf( ZMIN, ZMAX );
points[ i ].w = 1.;
}
glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );

glBindBufferBase( GL_SHADER_STORAGE_BUFFER,  4,  posSSbo );

char * shader_source = readFile("shaders/compute_test.comp");
std::cout << shader_source << std::endl;
GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
glShaderSource(shader, 1, &shader_source, NULL);
glCompileShader(shader);

GLuint program = glCreateProgram();
glAttachShader(program,shader);
glUseProgram(program);
glDispatchCompute( NUM_PARTICLES  / WORK_GROUP_SIZE, 1,  1 );
glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
}
*/

#else

#include <iostream>
int main(int argc, char ** argv)
{
    std::cout << "Compute shader not supported on this MAC" << std::endl;
    return 1;
}

#endif
