#include "mesh.hpp"
#include "autogen.hpp"
#include <iostream>
#include "debug.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <random>

int main(int argc, char ** argv)
{

    GLuint size = 6;
    GLuint sub_size = 3;
    GLuint sub_sub_size = 2;
    cd::Mesh m0, m1,m2;
    m0.m_vertex_array.resize(size*size);
    m1.m_vertex_array.resize(sub_size*sub_size);
    for(int i = 0; i < sub_size; ++i)
    {
        for(int j = 0; j < sub_size; ++j)
        {
            m1.m_vertex_array[i*sub_size+j] = glm::vec3(
                    2*i,
                    (float)std::rand() / RAND_MAX,
                    2*j);
        }
    }
    m2.m_vertex_array.resize(sub_sub_size*sub_sub_size);
    //m0.m_index_array.resize(size);
    //m1.m_index_array.resize(sub_size);
    //

    std::cout << "mesh0-->mesh1" << std::endl;
    cd::constrainMeshGridEdges(
            m0,m1,
            size,size,
            sub_size,sub_size);

    for(int i = 0; i < size; ++i)
    {
        for(int j = 0; j < size; ++j)
        {
            PRINT("m0["<<i<<","<<j<<"] = " << glm::to_string(m0.m_vertex_array[i*size+j]));
        }
    }
    for(int i = 0; i < sub_size; ++i)
    {
        for(int j = 0; j < sub_size; ++j)
        {
            PRINT("m1["<<i<<","<<j<<"] = " << glm::to_string(m1.m_vertex_array[i*sub_size+j]));
        }
    }

    /*
       std::cout << "mesh1-->mesh2" << std::endl;
       cd::constrainMeshGridEdges(
       m1,m2,
       sub_size,sub_size,
       sub_sub_size,sub_sub_size);
       */



    return 0;
}
