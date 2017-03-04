#ifndef __MESH__
#define __MESH__

#include "gl_include.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>

#include "loadobj.h"
#include "camera.hpp"

namespace cd
{

    enum VBO_ID : GLuint
    {
        INDEX,
        VERTEX,
        NORMAL,
        TANGENT,
        BITANGENT,
        TEX_COORD,
        NUM_VBO
    };
    
    struct Mesh
    {
        // fields 
        GLuint m_vao;
        GLuint m_vbo[NUM_VBO];
        std::vector<GLuint> m_index_array; 
        std::vector<glm::vec3> m_vertex_array;
        std::vector<glm::vec3> m_normal_array;
        std::vector<glm::vec3> m_tangent_array;
        std::vector<glm::vec3> m_bitangent_array;
        std::vector<glm::vec2> m_tex_coord_array;

        //std::vector<glm::vec4> m_bounding_box;
        glm::vec4 m_bounding_box[8];
        glm::vec3 m_centroid;

        GLenum draw_mode;

        // members
        Mesh();
        void insertRawPointerData(
                GLuint num_index,
                GLuint num_vertex,
                GLuint * index_array,
                GLfloat * vertex_array,
                GLfloat * normal_array,
                GLfloat * tangent_array,
                GLfloat * bitangent_array,
                GLfloat * tex_coord_array);
        void generateBoundingBox();
        void toGPU();
        void bindShader(
                GLuint shader_ID,
                const std::string & vertex_name,
                const std::string & normal_name,
                const std::string & tangent_name,
                const std::string & bitangent_name,
                const std::string & tex_coord_name);
        void draw();
        void translate(GLfloat x, GLfloat y, GLfloat z);
        void scale(GLfloat x, GLfloat y, GLfloat z);
        void scaleTex(GLfloat su, GLfloat sv);
        void recomputeSurfaceVectors();
        Mesh dual();
    };    

    enum SIDE
    {
        TOP = 0,
        BOTTOM = 1,
        LEFT = 2,
        RIGHT = 3,
        NUM_SIDE
    };

    struct MeshGMM : public Mesh
    {
        std::vector<GLuint> m_index_array_edge[2][NUM_SIDE];
        GLuint m_vbo_edge[2][NUM_SIDE];
        void drawEdge(GLuint level, SIDE side);
        void draw();
        void toGPU();
    };

    struct MeshGrid
    {
        std::vector<std::vector<Mesh> > m_grid;
        GLuint size_x;
        GLuint size_y;
        GLuint patch_size_x;
        GLuint patch_size_y;
    };

    struct MeshGridGMM
    {
        std::vector<std::vector<MeshGMM> > m_grid;
        GLuint size_x;
        GLuint size_y;
        GLuint patch_size_x;
        GLuint patch_size_y;
    };
}

#endif
