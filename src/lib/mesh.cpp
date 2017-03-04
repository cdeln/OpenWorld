#include "mesh.hpp"
#include <iostream>
#include <type_traits>
#include "GL_utilities.h"
#include "util.hpp"
#include "debug.hpp"
#include "autogen.hpp"

#define printvec2(X) std::cout << #X <<"("<<X[0]<<","<<X[1]<<")"<<std::endl;
#define printvec3(X) std::cout << #X <<"("<<X[0]<<","<<X[1]<<","<<X[2]<<")"<<std::endl;
#define codePrint(X) std::cout << #X << " = " << X << std::endl;

namespace cd
{

    Mesh::Mesh()
    { 
    };

    void Mesh::insertRawPointerData(
            GLuint num_index,
            GLuint num_vertex,
            GLuint * index_array,
            GLfloat * vertex_array,
            GLfloat * normal_array,
            GLfloat * tangent_array,
            GLfloat * bitangent_array,
            GLfloat * tex_coord_array)
    {
        m_index_array.resize(num_index);
        m_vertex_array.resize(num_vertex);
        m_normal_array.resize(num_vertex);
        m_tangent_array.resize(num_vertex);
        m_bitangent_array.resize(num_vertex);
        m_tex_coord_array.resize(num_vertex);
        if( index_array != NULL )
            for(GLuint i = 0; i < num_index; ++i)
            {
                m_index_array[i] = index_array[i];
            }
        for(GLuint i = 0; i < num_vertex; ++i)
        {
            if( vertex_array != NULL )
                m_vertex_array[i] = glm::vec3(vertex_array[3*i],vertex_array[3*i+1],vertex_array[3*i+2]);
            if( normal_array != NULL )
                m_normal_array[i] = glm::vec3(normal_array[3*i],normal_array[3*i+1],normal_array[3*i+2]);
            if( tangent_array != NULL )
                m_tangent_array[i] = glm::vec3(tangent_array[3*i],tangent_array[3*i+1],tangent_array[3*i+2]);
            if( bitangent_array != NULL )
                m_bitangent_array[i] = glm::vec3(bitangent_array[3*i],bitangent_array[3*i+1],bitangent_array[3*i+2]);
            if( tex_coord_array != NULL )
                m_tex_coord_array[i] = glm::vec2(tex_coord_array[2*i],tex_coord_array[2*i+1]);
        }
    };

    /*
     * Generate, bind and upload all non-empty arrays to GPU buffers
     */
    void Mesh::toGPU()
    {
        glGenVertexArrays(1,&m_vao);
        glBindVertexArray(m_vao);
        if( m_vertex_array.size() > 0 )
        {
            arrayToBuffer(m_vertex_array, GL_ARRAY_BUFFER, &m_vbo[VERTEX]);
        }
        if( m_index_array.size() > 0 )
        {
            arrayToBuffer(m_index_array, GL_ELEMENT_ARRAY_BUFFER, &m_vbo[INDEX]);
        }
        if( m_normal_array.size() > 0 )
        {
            arrayToBuffer(m_normal_array, GL_ARRAY_BUFFER, &m_vbo[NORMAL]);
        }
        if( m_tangent_array.size() > 0 )
        {
            arrayToBuffer(m_tangent_array, GL_ARRAY_BUFFER, &m_vbo[TANGENT]);
        }
        if( m_bitangent_array.size() > 0 )
        {
            arrayToBuffer(m_bitangent_array, GL_ARRAY_BUFFER, &m_vbo[BITANGENT]);
        }
        if( m_tex_coord_array.size() > 0 )
        {
            arrayToBuffer(m_tex_coord_array, GL_ARRAY_BUFFER, &m_vbo[TEX_COORD]);
        }
    };

    /*
     * Bind the shader after uploading data to GPU 
     */
    void Mesh::bindShader( 
            GLuint shader_ID,
            const std::string & vertex_name,
            const std::string & normal_name,
            const std::string & tangent_name,
            const std::string & bitangent_name,
            const std::string & tex_coord_name)
    {
        glBindVertexArray(m_vao);
        if( ! vertex_name.empty() )
            bindBufferAttribArray(m_vbo[VERTEX], shader_ID, vertex_name, 3);
        printError("Mesh::bindShader::vertex");
        if( ! normal_name.empty() )
            bindBufferAttribArray(m_vbo[NORMAL], shader_ID, normal_name, 3);
        printError("Mesh::bindShader::normal");
        if( ! tangent_name.empty() )
            bindBufferAttribArray(m_vbo[TANGENT], shader_ID, tangent_name, 3);
        printError("Mesh::bindShader::tangent");
        if( ! bitangent_name.empty() )
            bindBufferAttribArray(m_vbo[BITANGENT], shader_ID, bitangent_name, 3);
        printError("Mesh::bindShader::bitangent");
        if( ! tex_coord_name.empty() )
            bindBufferAttribArray(m_vbo[TEX_COORD], shader_ID, tex_coord_name, 2);
        printError("Mesh::bindShader::tex_coord");
    }

    /*
     * Draw the mesh
     */
    void Mesh::draw()
    {
        glBindVertexArray(m_vao);
        if( ! m_index_array.empty() )
            glDrawElements(GL_TRIANGLES, m_index_array.size(), GL_UNSIGNED_INT, 0L);
        else
            glDrawArrays(GL_POINTS, 0, m_vertex_array.size()); 
    }

    void Mesh::translate(GLfloat x, GLfloat y, GLfloat z)
    {
        for(GLuint i = 0; i < m_vertex_array.size(); ++i)
        {
            m_vertex_array[i].x += x;
            m_vertex_array[i].y += y;
            m_vertex_array[i].z += z;
        }
    }
    void Mesh::scale(GLfloat x, GLfloat y, GLfloat z)
    {
        for(GLuint i = 0; i < m_vertex_array.size(); ++i)
        {
            m_vertex_array[i].x *= x;
            m_vertex_array[i].y *= y;
            m_vertex_array[i].z *= z;
        }
        recomputeSurfaceVectors();
    }
    void Mesh::scaleTex(GLfloat su, GLfloat sv)
    {
        glm::vec2 scale(su,sv);
        for(GLuint i = 0; i < m_tex_coord_array.size(); ++i)
        {
            m_tex_coord_array[i] *= scale;
        }
    }
    void Mesh::recomputeSurfaceVectors()
    {
        cd::Timer tim;
        if( ! m_vertex_array.empty() && ! m_index_array.empty()  )
        {
            m_normal_array.resize(m_vertex_array.size());
            PRINTFUNC("Generate normals");
            tim.tic("Generate normals");
            generateNormals(
                    (GLfloat*)&m_vertex_array[0],
                    (GLuint*)&m_index_array[0],
                    m_vertex_array.size(),
                    m_index_array.size() / 3,
                    (GLfloat*)&m_normal_array[0]);
            //m_normal_array = generateNormals(m_vertex_array, m_index_array);
            tim.toc("Generate normals");
            if( ! m_tex_coord_array.empty() )
            {
                m_tangent_array.resize(m_vertex_array.size());
                m_bitangent_array.resize(m_vertex_array.size());
                PRINTFUNC("Generate tangent basis");
                tim.tic("Generate tangent basis");
                computeTangentBasis(
                        (GLfloat*)&m_vertex_array[0],
                        (GLfloat*)&m_tex_coord_array[0],
                        (GLuint*)&m_index_array[0],
                        m_vertex_array.size(),
                        m_index_array.size() / 3,
                        (GLfloat*)&m_tangent_array[0],
                        (GLfloat*)&m_bitangent_array[0]);
                tim.toc("Generate tangent basis");
            }
        }
    }

    void Mesh::generateBoundingBox()
    {
        GLfloat min_x, min_y, min_z;
        GLfloat max_x, max_y, max_z;
        min_x = min_y = min_z = std::numeric_limits<GLfloat>::max();
        max_x = max_y = max_z = std::numeric_limits<GLfloat>::min();

        glm::vec3 c;
        for(GLuint i = 0; i < m_vertex_array.size(); ++i)
        {
            glm::vec3 v = m_vertex_array[i];
            c += v;
            if( v.x < min_x )
                min_x = v.x;
            if( v.y < min_y )
                min_y = v.y;
            if( v.z < min_z )
                min_z = v.z;
            if( v.x > max_x )
                max_x = v.x;
            if( v.y > max_y )
                max_y = v.y;
            if( v.z > max_z )
                max_z = v.z;
        }

        m_centroid = c / static_cast<GLfloat>(m_vertex_array.size());

        //m_bounding_box.resize(8);
        m_bounding_box[0] = glm::vec4(min_x,min_y,min_z,1);
        m_bounding_box[1] = glm::vec4(min_x,min_y,max_z,1);
        m_bounding_box[2] = glm::vec4(min_x,max_y,min_z,1);
        m_bounding_box[3] = glm::vec4(min_x,max_y,max_z,1);
        m_bounding_box[4] = glm::vec4(max_x,min_y,min_z,1);
        m_bounding_box[5] = glm::vec4(max_x,min_y,max_z,1);
        m_bounding_box[6] = glm::vec4(max_x,max_y,min_z,1);
        m_bounding_box[7] = glm::vec4(max_x,max_y,max_z,1);
    }

    Mesh Mesh::dual()
    {
        Mesh dual_mesh;
        GLuint num_triangles = m_index_array.size() / 3;
        dual_mesh.m_vertex_array.resize(num_triangles);
        dual_mesh.m_normal_array.resize(num_triangles);
        for(GLuint k = 0; k < num_triangles; ++k)
        {
            glm::vec3 v1 = m_vertex_array[m_index_array[3*k]];
            glm::vec3 v2 = m_vertex_array[m_index_array[3*k+1]];
            glm::vec3 v3 = m_vertex_array[m_index_array[3*k+2]];
            glm::vec3 face_midpoint= (v1+v2+v3)/3.0f;
            glm::vec3 face_normal = glm::cross(v2-v1, v3-v1);
            face_normal /= glm::length(face_normal);
            dual_mesh.m_vertex_array[k] = face_midpoint;
            dual_mesh.m_normal_array[k] = face_normal; 
        }

        return dual_mesh;
    };

    // MeshGMM
    void MeshGMM::toGPU()
    {
        Mesh::toGPU();

        for(GLuint k = 0; k < 2; ++k)
            for(GLuint j = 0; j < NUM_SIDE; ++j)
                arrayToBuffer(m_index_array_edge[k][j], GL_ELEMENT_ARRAY_BUFFER, &m_vbo_edge[k][j]);
    };

    /*
     * Draw the mesh edges
     */
    void MeshGMM::drawEdge(GLuint level, SIDE side)
    {
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vbo_edge[level][side]);
        glDrawElements(GL_TRIANGLES, m_index_array_edge[level][side].size(), GL_UNSIGNED_INT, 0L);
    }
    void MeshGMM::draw()
    {
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vbo[INDEX]);
        if( ! m_index_array.empty() )
            glDrawElements(GL_TRIANGLES, m_index_array.size(), GL_UNSIGNED_INT, 0L);
        else
            glDrawArrays(GL_POINTS, 0, m_vertex_array.size()); 
    }

}
