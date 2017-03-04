#ifndef __AUTOGEN__
#define __AUTOGEN__

#include <vector>
#include "loadobj.h"
#include "mesh.hpp"

namespace cd
{
    void normalizeVectorArray(
            GLuint vector_count,
            GLfloat * in_array,
            GLfloat * out_array);

    void generateNormals(
            GLfloat * vertex_array,
            GLuint * index_array,
            GLuint vertex_count,
            GLuint triangle_count,
            GLfloat * normal_array); 

    std::vector<glm::vec3> generateNormals(
            std::vector<glm::vec3> & vertex_array,
            std::vector<GLuint> & index_array);

    void computeTangentBasis(
            GLfloat * vertex_array,
            GLfloat * tex_coord_array,
            GLuint * index_array,
            GLuint vertex_count,
            GLuint triangle_count,
            GLfloat * tangent_array,
            GLfloat * bitangent_array);

    void computeTangentBasis(
            Model * model,
            GLfloat ** tangent_array,
            GLfloat ** bitangent_array);

    std::vector<GLfloat> generateTexture2D(
            GLuint size_x,
            GLuint size_y,
            GLfloat min_val,
            GLfloat max_val);

    std::vector<GLfloat> generateTexture2D(
            std::vector<GLfloat> & map,
            GLuint size_x,
            GLuint size_y,
            GLfloat min_val,
            GLfloat max_val,
            GLfloat threshold);

    std::vector<std::vector<GLfloat> > generateTexture2DArray(
            GLuint size_x,
            GLuint size_y,
            GLuint num_arrays);

    std::vector<GLfloat> generateTexture3D(
            GLuint size_x,
            GLuint size_y,
            GLuint size_z);

    void scaleArray(
            std::vector<GLfloat> & tex,
            GLfloat min_val,
            GLfloat max_val);

    std::vector<GLfloat> quiltTexture2D(
            std::vector<std::vector<GLfloat> > & tex_array,
            GLuint size_x,
            GLuint size_y);

    void smoothEdges2Zero(
            std::vector<GLfloat> & tex,
            GLuint size_x,
            GLuint size_y);

    std::vector<std::vector<GLfloat> > generateMultiScaleTexture2D(
            std::vector<GLfloat> & texture,
            GLuint size_x,
            GLuint size_y,
            GLuint num_level);
    
    std::vector<GLfloat> generateWaterMap(
            std::vector<GLfloat> & map,
            GLuint size_x,
            GLuint size_y,
            GLfloat water_height);

    Mesh generateTerrain(
            GLfloat * height_map,
            GLuint map_size_x,
            GLuint map_size_z,
            GLfloat texture_scale = 1.0f,
            GLfloat spatial_scale = 1.0f);

    Mesh generateTerrainSimple(
            GLfloat * height_map,
            GLuint map_size_x,
            GLuint map_size_z,
            GLfloat texture_scale = 1.0f,
            GLfloat spatial_scale = 1.0f);

    MeshGrid splitMeshIntoGrid(
            Mesh & mesh,
            GLuint size_x,
            GLuint size_y,
            GLuint num_groups);

    MeshGridGMM splitMeshIntoGridMultiScale(
            Mesh & mesh,
            GLuint size_x,
            GLuint size_y,
            GLuint num_groups);
    
    void constrainMeshGridEdges(
            Mesh & mesh,
            Mesh & sub_mesh,
            GLint size_x,
            GLint size_y,
            GLint sub_size_x,
            GLint sub_size_y);

    /*   
    MeshGrid generateTerrainGrouped(
            std::vector<GLfloat> & height_map,
            GLuint map_size_x,
            GLuint map_size_y,
            GLuint group_size,
            GLfloat texture_scale = 1.0f,
            GLfloat spatial_scale = 1.0f);
    */
    /*
    std::vector<MeshGrid> generateTerrainMultiScale(
            std::vector<std::vector<GLfloat> > & height_map_multi_scale,
            Gluint size_x,
            GLuint size_y,
            GLuint group_size,
            GLuint num_levels,
            GLfloat texture_scale = 1.0f,
            GLfloat spatial_scale = 1.0f);
            */
    std::vector<GLfloat> linspace(GLfloat from, GLfloat to, GLuint num);

    Mesh createDome(
            GLuint resolution,
            GLfloat size,
            GLfloat angle,
            GLfloat tex_scale);

    Mesh createQuad(
            GLfloat x,
            GLfloat y,
            GLfloat width,
            GLfloat height);

    // TODO
    void generatedTexture3D(
        GLuint size_1,
        GLuint size_2,
        GLuint size_3,
        GLfloat * texture);

}

#endif
