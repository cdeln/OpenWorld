#include "autogen.hpp"
#include "VectorUtils3.h"
#include <cmath>
#include <fftw3.h>
#include <random>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <numeric>
#include "debug.hpp"
#include <ctime>

#include <glm/glm.hpp>

namespace cd
{

    /* Calculate min and max value of an array */
    template <typename T>
        void argMinMax(T * array, size_t n, T * amin, T * amax)
        {
            T min = std::numeric_limits<T>::max();
            T max = std::numeric_limits<T>::min();
            for(size_t i = 0; i < n; ++i)
            {
                T x = array[i];
                if( x < min )
                    min = x;
                if( x > max )
                    max = x;
            }
            *amin = min;
            *amax = max;
        }

    /* Normalize array of vec3s */
    void normalizeVectorArray(
            GLuint vector_count,
            GLfloat * in_array,
            GLfloat * out_array)
    {
        for(GLuint i = 0; i < 3 * vector_count; i += 3)
        {  
            GLfloat x = in_array[i];
            GLfloat y = in_array[i+1];
            GLfloat z = in_array[i+2];
            GLfloat norm = std::sqrt( x*x + y*y + z*z );
            out_array[i] = x / norm;
            out_array[i+1] = y / norm;
            out_array[i+2] = z / norm;
        } 
    }

    void normalizeVectorArray(std::vector<glm::vec3> & array)
    {
        for(GLuint k = 0; k < array.size(); ++k)
        {
            array[k] /= glm::length(array[k]);
        }
    }


    /* Generate normals from vertex and index data */
    void generateNormals(
            GLfloat * vertex_array,
            GLuint * index_array,
            GLuint vertex_count,
            GLuint triangle_count,
            GLfloat * normal_array)
    {
        // calculate normals
        for(GLuint i = 0; i  < 3*triangle_count; i += 3)
        {
            GLuint i0 = 3*index_array[i]; 
            GLuint i1 = 3*index_array[i+1]; 
            GLuint i2 = 3*index_array[i+2]; 

            GLfloat vx1 = vertex_array[i0 + 0];
            GLfloat vy1 = vertex_array[i0 + 1];
            GLfloat vz1 = vertex_array[i0 + 2];
            GLfloat vx2 = vertex_array[i1 + 0];
            GLfloat vy2 = vertex_array[i1 + 1];
            GLfloat vz2 = vertex_array[i1 + 2];
            GLfloat vx3 = vertex_array[i2 + 0];
            GLfloat vy3 = vertex_array[i2 + 1];
            GLfloat vz3 = vertex_array[i2 + 2];

            vec3 v1 = SetVector(vx1 - vx2, vy1 - vy2, vz1 - vz2);
            vec3 v2 = SetVector(vx3 - vx2, vy3 - vy2, vz3 - vz2);
            vec3 normal = CrossProduct(v2,v1);
            GLfloat norm = std::sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
            normal.x /= norm;
            normal.y /= norm;
            normal.z /= norm;


            normal_array[i0 + 0] += normal.x;
            normal_array[i0 + 1] += normal.y;
            normal_array[i0 + 2] += normal.z;
            normal_array[i1 + 0] += normal.x;
            normal_array[i1 + 1] += normal.y;
            normal_array[i1 + 2] += normal.z;
            normal_array[i2 + 0] += normal.x;
            normal_array[i2 + 1] += normal.y;
            normal_array[i2 + 2] += normal.z;
        } 
        // normalize normals
        for(GLuint i =  0; i < vertex_count; ++i)
        {
            GLuint index_x = 3*i + 0;
            GLuint index_y = 3*i + 1;
            GLuint index_z = 3*i + 2;

            GLfloat nx = normal_array[index_x];
            GLfloat ny = normal_array[index_y];
            GLfloat nz = normal_array[index_z];

            GLfloat norm = std::sqrt( nx*nx + ny*ny + nz*nz );

            normal_array[index_x] = nx / norm;
            normal_array[index_y] = ny / norm;
            normal_array[index_z] = nz / norm;
        } 
    }

    std::vector<glm::vec3> generateNormals(
            std::vector<glm::vec3> & v,
            std::vector<GLuint> & i)
    {
        std::vector<glm::vec3> n(v.size(), glm::vec3(0,0,0));
        glm::vec3 v1,v2,v3;
        for(GLuint k = 0; k  < i.size(); k += 3)
        {

            v1 = v[i[k]];
            v2 = v[i[k+1]];
            v3 = v[i[k+2]];
            n[i[k]] += glm::cross(v3-v2,v1-v2);
            n[i[k+1]] += glm::cross(v3-v2,v1-v2);
            n[i[k+2]] += glm::cross(v3-v2,v1-v2);
        } 
        normalizeVectorArray(n);
        return n;
    }

    /* Generate tangent and bitangents */
    void computeTangentBasis(
            GLfloat * vertex_array,
            GLfloat * tex_coord_array,
            GLuint * index_array,
            GLuint vertex_count,
            GLuint triangle_count,
            GLfloat * tangent_array,
            GLfloat * bitangent_array)
    { 
        //GLuint index_count = 3 * triangle_count;
        for(GLuint i = 0; i < 3 * triangle_count; i += 3)
        {
            GLuint i0 = index_array[i];
            GLuint i1 = index_array[i+1];
            GLuint i2 = index_array[i+2];

            GLfloat v0x = vertex_array[3*i0 + 0];
            GLfloat v0y = vertex_array[3*i0 + 1];
            GLfloat v0z = vertex_array[3*i0 + 2];
            GLfloat v1x = vertex_array[3*i1 + 0];
            GLfloat v1y = vertex_array[3*i1 + 1];
            GLfloat v1z = vertex_array[3*i1 + 2];
            GLfloat v2x = vertex_array[3*i2 + 0];
            GLfloat v2y = vertex_array[3*i2 + 1];
            GLfloat v2z = vertex_array[3*i2 + 2];

            GLfloat tex0_u = tex_coord_array[2*i0 + 0];
            GLfloat tex0_v = tex_coord_array[2*i0 + 1];
            GLfloat tex1_u = tex_coord_array[2*i1 + 0];
            GLfloat tex1_v = tex_coord_array[2*i1 + 1];
            GLfloat tex2_u = tex_coord_array[2*i2 + 0];
            GLfloat tex2_v = tex_coord_array[2*i2 + 1];

            glm::vec3 v0(v0x,v0y,v0z);
            glm::vec3 v1(v1x,v1y,v1z);
            glm::vec3 v2(v2x,v2y,v2z);

            glm::vec2 uv0(tex0_u,tex0_v);
            glm::vec2 uv1(tex1_u,tex1_v);
            glm::vec2 uv2(tex2_u, tex2_v);

            glm::vec3 v1_delta = v1 - v0;
            glm::vec3 v2_delta = v2 - v0;
            glm::vec2 uv1_delta = uv1 - uv0;
            glm::vec2 uv2_delta = uv2 - uv0;

            GLfloat denom = uv1_delta.x * uv2_delta.y - uv1_delta.y * uv2_delta.x;

            //glm::vec3 normal = glm::cross(v2,v1);
            glm::vec3 tangent = (uv2_delta.y * v1_delta - uv1_delta.y * v2_delta) / denom;
            glm::vec3 bitangent = (uv1_delta.x * v2_delta - uv2_delta.x * v1_delta) / denom;

            /*
               normal_array[i0 + 0] += normal.x;
               normal_array[i0 + 1] += normal.y;
               normal_array[i0 + 2] += normal.z;
               normal_array[i1 + 0] += normal.x;
               normal_array[i1 + 0] += normal.x;
               normal_array[i1 + 1] += normal.y;
               normal_array[i2 + 2] += normal.z;
               normal_array[i2 + 1] += normal.y;
               normal_array[i2 + 2] += normal.z;
               */

            tangent_array[3*i0 + 0] += tangent.x;
            tangent_array[3*i0 + 1] += tangent.y;
            tangent_array[3*i0 + 2] += tangent.z;
            tangent_array[3*i1 + 0] += tangent.x;
            tangent_array[3*i1 + 0] += tangent.x;
            tangent_array[3*i1 + 1] += tangent.y;
            tangent_array[3*i2 + 2] += tangent.z;
            tangent_array[3*i2 + 1] += tangent.y;
            tangent_array[3*i2 + 2] += tangent.z;

            bitangent_array[3*i0 + 0] += bitangent.x;
            bitangent_array[3*i0 + 1] += bitangent.y;
            bitangent_array[3*i0 + 2] += bitangent.z;
            bitangent_array[3*i1 + 0] += bitangent.x;
            bitangent_array[3*i1 + 0] += bitangent.x;
            bitangent_array[3*i1 + 1] += bitangent.y;
            bitangent_array[3*i2 + 2] += bitangent.z;
            bitangent_array[3*i2 + 1] += bitangent.y;
            bitangent_array[3*i2 + 2] += bitangent.z;
        } 

        //normalizeVectorArray(vertex_count, normal_array, normal_array);
        normalizeVectorArray(vertex_count, tangent_array, tangent_array);
        normalizeVectorArray(vertex_count, bitangent_array, bitangent_array);
    }

    /* Compute tangent basis from model data */
    void computeTangentBasis(
            Model * model,
            GLfloat ** tangent_array,
            GLfloat ** bitangent_array)
    {

        *tangent_array = new GLfloat[3 * model->numVertices];
        *bitangent_array = new GLfloat[3 * model->numVertices];
        computeTangentBasis(
                model->vertexArray,
                model->texCoordArray,
                model->indexArray,
                model->numVertices,
                model->numIndices / 3,
                *tangent_array,
                *bitangent_array);
    }

    /* Generate height map */
    std::vector<GLfloat> generateTexture2D(
            GLuint map_size_x,
            GLuint map_size_z,
            GLfloat min_val,
            GLfloat max_val)
    {
        cd::Timer tim;
        size_t map_size = map_size_x * map_size_z;
        std::normal_distribution<double> distribution(0.0, 1.0);
        //std::random_device rnd;
        std::default_random_engine rnd;

        // allocate some fft arrays
        fftw_complex * in = fftw_alloc_complex(map_size); 
        fftw_complex * out = fftw_alloc_complex(map_size);
        //std::vector<fftw_complex> in(map_size);
        //std::vector<fftw_complex> out(map_size);

        // generate random frequencies with amplitude fallof
        tim.tic("generating random frequencies");
        for(GLuint i = 0; i < map_size; ++i)
        {
            in[i][0] = distribution(rnd); 
            in[i][1] = distribution(rnd);
        }
        tim.toc("generating random frequencies");
        tim.tic("applying amplitude falloff");
        for(GLuint z = 0; z < map_size_z; ++z)
        {
            for(GLuint x = 0; x < map_size_x; ++x)
            {   
                double xf = static_cast<double>(x);
                double zf = static_cast<double>(z);
                if( xf > map_size_x / 2 )
                    xf = static_cast<double>(map_size_x) - xf;
                if( zf > map_size_z / 2 )
                    zf = static_cast<double>(map_size_z) - zf;
                double amp = 1.0 / std::sqrt( 0.01 + xf * xf + zf * zf);
                GLuint index = x + z * map_size_x;
                in[index][0] *= amp;
                in[index][1] *= amp;
            }
        }
        tim.toc("applying amplitude falloff");

        tim.tic("fft");
        fftw_plan plan = fftw_plan_dft_2d(map_size_x, map_size_z, in, out, FFTW_BACKWARD, FFTW_ESTIMATE);
        fftw_execute(plan);
        tim.toc("fft");

        // copy to output
        std::cout << "copy to output" << std::endl;
        std::vector<GLfloat> height_map(map_size,0);// = new GLfloat[map_size];
        for(size_t n = 0; n < map_size; ++n)
        {
            height_map[n] = static_cast<GLfloat>(out[n][0]); // take real part
        }
        GLfloat minval, maxval;
        argMinMax(&height_map[0], map_size, &minval, &maxval);
        std::cout << "maxval  = " << maxval << ", minval = " << minval << std::endl;
        GLfloat range_target = max_val - min_val;
        GLfloat range_actual = maxval - minval;
        for(size_t n = 0; n < map_size; ++n)
        {
            height_map[n] = min_val + range_target * (height_map[n] - minval) / range_actual; 
        }

        argMinMax(&height_map[0], map_size, &minval, &maxval);
        std::cout << "maxval  = " << maxval << ", minval = " << minval << std::endl;

        std::cout << "cleanup" << std::endl;
        // cleanup
        fftw_free(in);
        fftw_free(out); 
        fftw_destroy_plan(plan);
        return height_map;
    }

    /* Generate height map, but more simple*/
    std::vector<GLfloat> generateTexture2DSimple(
            GLuint map_size_x,
            GLuint map_size_z)
    {
        cd::Timer tim;
        size_t map_size = map_size_x * map_size_z;
        std::normal_distribution<double> distribution(0.0, 1.0);
        std::random_device rnd;
        //std::default_random_engine rnd;

        // allocate some fft arrays
        fftw_complex * in = fftw_alloc_complex(map_size); 
        fftw_complex * out = fftw_alloc_complex(map_size);

        // generate random frequencies with amplitude fallof
        tim.tic("generating random frequencies");
        for(GLuint i = 0; i < map_size; ++i)
        {
            in[i][0] = distribution(rnd); 
            in[i][1] = distribution(rnd);
        }
        tim.toc("generating random frequencies");
        tim.tic("applying amplitude falloff");
        for(GLuint z = 0; z < map_size_z; ++z)
        {
            for(GLuint x = 0; x < map_size_x; ++x)
            {   
                double xf = static_cast<double>(x);
                double zf = static_cast<double>(z);
                if( xf > map_size_x / 2 )
                    xf = static_cast<double>(map_size_x) - xf;
                if( zf > map_size_z / 2 )
                    zf = static_cast<double>(map_size_z) - zf;
                double amp = 1.0 / ( 1.0 + xf * xf + zf * zf);
                GLuint index = x + z * map_size_x;
                in[index][0] *= amp;
                in[index][1] *= amp;
            }
        }
        tim.toc("applying amplitude falloff");

        tim.tic("fft");
        fftw_plan plan = fftw_plan_dft_2d(map_size_x, map_size_z, in, out, FFTW_BACKWARD, FFTW_ESTIMATE);
        fftw_execute(plan);
        tim.toc("fft");

        // copy to output
        std::cout << "copy to output" << std::endl;
        std::vector<GLfloat> height_map(map_size,0);// = new GLfloat[map_size];
        for(size_t n = 0; n < map_size; ++n)
        {
            height_map[n] = static_cast<GLfloat>(out[n][0]); // take real part
        }

        std::cout << "cleanup" << std::endl;
        // cleanup
        fftw_free(in);
        fftw_free(out); 
        fftw_destroy_plan(plan);
        return height_map;
    }

    /* Generate 3D texture */ 
    std::vector<GLfloat> generateTexture3D(
            GLuint size_x,
            GLuint size_y,
            GLuint size_z)
    {
        cd::Timer tim;
        size_t size = size_x * size_y * size_z;
        std::normal_distribution<double> distribution(0.0, 1.0);
        std::random_device rnd;
        //std::default_random_engine rnd;

        // allocate some fft arrays
        fftw_complex * in = fftw_alloc_complex(size); 
        fftw_complex * out = fftw_alloc_complex(size);

        // generate random frequencies with amplitude fallof
        tim.tic("generating random frequencies");
        for(GLuint i = 0; i < size; ++i)
        {
            in[i][0] = distribution(rnd); 
            in[i][1] = distribution(rnd);
        }
        tim.toc("generating random frequencies");
        tim.tic("applying amplitude falloff");
        for(GLuint z = 0; z < size_z; ++z)
        {
            for(GLuint y = 0; y < size_y; ++y)
            {
                for(GLuint x = 0; x < size_x; ++x)
                {   
                    double xf = static_cast<double>(x);
                    double yf = static_cast<double>(y);
                    double zf = static_cast<double>(z);
                    if( xf > size_x / 2 )
                        xf = static_cast<double>(size_x) - xf;
                    if( yf > size_y / 2 )
                        yf = static_cast<double>(size_y) - yf;
                    if( zf > size_z / 2 )
                        zf = static_cast<double>(size_z) - zf;
                    double amp = 1.0 / ( 1.0 + xf*xf + yf*yf +  zf*zf);
                    GLuint index = x + y * size_x + z * size_x * size_y;
                    in[index][0] *= amp;
                    in[index][1] *= amp;
                }
            }
        }
        tim.toc("applying amplitude falloff");

        tim.tic("fft");
        fftw_plan plan = fftw_plan_dft_3d(size_x, size_y, size_z, in, out, FFTW_BACKWARD, FFTW_ESTIMATE);
        fftw_execute(plan);
        tim.toc("fft");

        // copy to output
        std::cout << "copy to output" << std::endl;
        std::vector<GLfloat> tex(size,0);// = new GLfloat[map_size];
        for(size_t n = 0; n < size; ++n)
        {
            tex[n] = static_cast<GLfloat>(out[n][0]); // take real part
        }

        std::cout << "cleanup" << std::endl;
        // cleanup
        fftw_free(in);
        fftw_free(out); 
        fftw_destroy_plan(plan);
        return tex;
    }

    /*
     * Generate array of random 2d textures
     */
    std::vector<std::vector<GLfloat> > generateTexture2DArray(
            GLuint size_x,
            GLuint size_y,
            GLuint num_arrays)
    {
        std::vector<std::vector<GLfloat> > tex_array(num_arrays);
        for(GLuint k = 0; k < num_arrays; ++k)
        {
            tex_array[k] = generateTexture2DSimple(size_x,size_y);
        }
        return tex_array;
    }

    /*
     * Flips a 2d texture in all directions and puts together
     * The resulting texture is twice as big in all dimensions
     */
    cv::Mat flipPatch(cv::Mat & from)
    {
        //cv::Mat from(size_y,size_x,CV_32FC1,&tex[0]);
        GLuint size_x = from.cols;
        GLuint size_y = from.rows;
        cv::Mat to(2*size_y,2*size_x,CV_32FC1);

        from.copyTo(to(cv::Rect(0,0,size_x,size_y)));
        cv::flip(from, to(cv::Rect(0,size_y,size_x,size_y)), 0);
        cv::flip(from, to(cv::Rect(size_x,0,size_x,size_y)), 1);
        cv::flip(to(cv::Rect(size_x,0,size_x,size_y)),
                to(cv::Rect(size_x,size_y,size_x,size_y)), 0);
        return to;
        /*
           GLfloat* ptr = reinterpret_cast<GLfloat*>(to.data);
           std::vector<GLfloat> retvec(ptr,ptr+4*size_x*size_y);
           return retvec;
           */
    }

    /*
     * Scale a texture to lie between min and max
     */
    void scaleArray(
            std::vector<GLfloat> & tex,
            GLfloat min_val,
            GLfloat max_val)
    {
        GLfloat minval, maxval;
        argMinMax(&tex[0], tex.size(), &minval, &maxval);
        GLfloat range_target = max_val - min_val;
        GLfloat range_actual = maxval - minval;
        for(size_t n = 0; n < tex.size(); ++n)
        {
            tex[n] = min_val + range_target * (tex[n] - minval) / range_actual; 
        }
    }

    /*
     * "Quilt" a texture, size of the texture grows exponentially with factor
     */
    std::vector<GLfloat> quiltTexture2D(
            std::vector<std::vector<GLfloat> > & tex_array,
            GLuint size_x,
            GLuint size_y)
    {
        if( tex_array.empty() )
            return std::vector<GLfloat>();

        PRINT("quilting");
        cv::Mat tex = cv::Mat(size_y,size_x,CV_32FC1,&tex_array[0][0]);

        GLuint cur_size_x = size_x;
        GLuint cur_size_y = size_y;
        for(GLuint k = 1; k < tex_array.size(); ++k)
        {
            GLuint scale = 1<<k;
            cv::Mat next(size_y,size_x,CV_32FC1, &tex_array[k][0]);
            cv::resize(next,next,cv::Size(scale*size_x,scale*size_y));
            tex = flipPatch(tex) + scale*scale*next; 
        }

        // Copy to output
        GLuint scale = 1<<(tex_array.size()-1);
        GLfloat* ptr = reinterpret_cast<GLfloat*>(tex.data);
        std::vector<GLfloat> retvec(ptr,ptr+scale*scale*size_x*size_y);
        return retvec;
    }

    /*
     * Force edges to go to zero
     */
    void smoothEdges2Zero(
            std::vector<GLfloat> & tex,
            GLuint size_x,
            GLuint size_y)
    {
        GLfloat mid_x = static_cast<GLfloat>(size_x-1)/2.0f;
        GLfloat mid_y = static_cast<GLfloat>(size_y-1)/2.0f;
        GLfloat div_factor = static_cast<GLfloat>(mid_x*mid_y);
        for(GLuint y = 0; y < size_y; ++y)
        {
            for(GLuint x = 0; x < size_x; ++x)
            {
                GLuint index = y*size_x + x;
                GLfloat xf = (static_cast<GLfloat>(x) - mid_x)/mid_x;
                GLfloat yf = (static_cast<GLfloat>(y) - mid_y)/mid_y;
                GLfloat r = 1.0 - std::max(std::fabs(xf),std::fabs(yf));
                if( x == 0 || y == 0 || x == (size_x-1) || y == (size_y-1) )
                    r = 0;
                tex[index] *= r;
            }
        }
    }

    /*
     * Generate 2D texture from probabilites drawn from another texture
     */
    std::vector<GLfloat> generateTexture2D(
            std::vector<GLfloat> & map,
            GLuint size_x,
            GLuint size_y,
            GLfloat min_val,
            GLfloat max_val,
            GLfloat threshold)
    {
        //GLfloat range = max_val - min_val;
        std::vector<GLfloat> tex(map.size(),0);
        std::uniform_real_distribution<double> distribution(min_val, max_val);
        std::random_device rnd;
        //std::default_random_engine rnd;
        for(GLuint i = 0; i < map.size(); ++i)
        {
            if( map[i] > threshold )
            {
                float x = distribution(rnd);
                if( x < map[i] )
                    tex[i] = 1.0;//(x - threshold) / range;
            }
        };

        cv::Mat mat(size_y,size_x,CV_32FC1,&tex[0]);
        cv::dilate(mat,mat, cv::Mat(), cv::Point(-1, -1), 11);
        return tex;
    }

    /*
     * Generate scale pyramid, num_levels_exclusive (can index including num_level_excl)
     */
    std::vector<std::vector<GLfloat> > generateMultiScaleTexture2D(
            std::vector<GLfloat> & texture,
            GLuint size_x,
            GLuint size_y,
            GLuint num_level_excl)
    {
        GLuint num_level = num_level_excl + 1;
        std::vector<std::vector<GLfloat> > pyramid(num_level);
        pyramid[0] = texture;
        cv::Mat texmat(size_y, size_x, CV_32FC1, &texture[0]);
        for(GLuint k = 1; k < num_level; ++k)
        {
            GLuint scale = 1<<k;
            GLuint sub_size_x = size_x / scale;
            GLuint sub_size_y = size_y / scale;
            cv::Size subsize = cv::Size(sub_size_x, sub_size_y);
            cv::Mat subtexmat;
            cv::resize(texmat, subtexmat, subsize, 0, 0, cv::INTER_NEAREST);

            GLfloat * ptr = reinterpret_cast<GLfloat*>(subtexmat.data);
            GLuint subnum = sub_size_x * sub_size_y;
            pyramid[k] = std::vector<GLfloat>(ptr, ptr + subnum);
        }
        /*
           GLuint num_level = num_level_excl + 1;
           GLuint size = size_x * size_y;
           std::cout << "size = " << size << ", tex size = " << texture.size() << std::endl;
           std::vector<std::vector<GLfloat> > pyramid(num_level);
           pyramid[0] = texture;
           for(GLuint k = 1; k < num_level; ++k)
           {
           GLuint scale_factor = 1 << k;
           std::cout << "scale_factor["<<k<<"] = " << scale_factor << std::endl;
           GLuint local_size_x = size_x / scale_factor;
           GLuint local_size_y = size_y / scale_factor;
           GLuint local_size = local_size_x * local_size_y;
           pyramid[k].resize(local_size);

           for(GLuint y = 0; y < local_size_y; ++y)
           {
           for(GLuint x = 0; x < local_size_x; ++x)
           {
        // jump 2 in x and y dir
        GLuint local_index = y*local_size_x + x;
        GLuint prev_index = (2*y)*(2*local_size_x) + 2*x;
        pyramid[k][local_index] = pyramid[k-1][prev_index];
        }
        }
        }
        */

        return pyramid;
    }   

    /* Generate random terrain */
    Mesh generateTerrain(
            GLfloat * height_map,
            GLuint map_size_x,
            GLuint map_size_z,
            GLfloat texture_scale,
            GLfloat spatial_scale)
    {

        GLuint vertex_count_x = map_size_x;
        GLuint vertex_count_z = map_size_z;
        GLuint vertex_count = vertex_count_x * vertex_count_z;

        GLuint triangle_count = 2 * (vertex_count_x - 1) * (vertex_count_z - 1); 

        GLfloat * vertex_array = new GLfloat[3 * vertex_count]();
        GLfloat * normal_array = new GLfloat[3 * vertex_count]();
        GLfloat * tex_coord_array = new GLfloat[2 * vertex_count]();
        GLuint * index_array = new GLuint[3 * triangle_count]();

        // generate vertices and texture coordinates
        for(GLuint z = 0; z < vertex_count_z; ++z)
        {
            for(GLuint x = 0; x < vertex_count_x; ++x)
            {
                GLuint index_x = (x + z * vertex_count_x)*3 + 0;
                GLuint index_y = (x + z * vertex_count_x)*3 + 1;
                GLuint index_z = (x + z * vertex_count_x)*3 + 2;
                GLfloat height = height_map[x + z * vertex_count_x];
                vertex_array[index_x] = spatial_scale * static_cast<GLfloat>(x); 
                vertex_array[index_y] = static_cast<GLfloat>(height);
                vertex_array[index_z] = spatial_scale * static_cast<GLfloat>(z);

                // texture coordinates
                GLuint tex_index_x = (x + z * vertex_count_x) * 2 + 0;
                GLuint tex_index_z = (x + z * vertex_count_x) * 2 + 1;
                tex_coord_array[tex_index_x] = spatial_scale * texture_scale * static_cast<GLfloat>(x);
                tex_coord_array[tex_index_z] = spatial_scale * texture_scale * static_cast<GLfloat>(z);
            }
        }

        // generate indices
        std::cout << "Generating indices" << std::endl;
        for(GLuint z = 0; z < vertex_count_z - 1; ++z)
        {
            for(GLuint x = 0; x < vertex_count_x - 1; ++x)
            {
                GLuint index = x + z * (vertex_count_x-1);
                index_array[index * 6 + 0] = x + z * vertex_count_x;
                index_array[index * 6 + 1] = x + (z+1) * vertex_count_x;
                index_array[index * 6 + 2] = x+1 + z * vertex_count_x;
                index_array[index * 6 + 3] = x+1 + z * vertex_count_x;
                index_array[index * 6 + 4] = x + (z+1) * vertex_count_x;
                index_array[index * 6 + 5] = x+1 + (z+1) * vertex_count_x;
            }
        }

        // generate normals
        std::cout << "Generate normals" << std::endl;
        generateNormals(vertex_array, index_array, vertex_count, triangle_count, normal_array);

        // generate tangent basis
        std::cout << "Generate tangent basis" << std::endl;
        GLfloat *tangent_array = new GLfloat[3 * vertex_count];
        GLfloat *bitangent_array = new GLfloat[3 * vertex_count];
        computeTangentBasis(
                vertex_array,
                tex_coord_array,
                index_array,
                vertex_count,
                triangle_count,
                tangent_array,
                bitangent_array);


        // put into mesh, return
        Mesh mesh;
        mesh.insertRawPointerData(
                3*triangle_count,
                vertex_count,
                index_array,
                vertex_array,
                normal_array,
                tangent_array,
                bitangent_array,
                tex_coord_array);
        return mesh;
    }

    /* Generate random terrain, but simple*/
    Mesh generateTerrainSimple(
            GLfloat * height_map,
            GLuint map_size_x,
            GLuint map_size_z,
            GLfloat texture_scale,
            GLfloat spatial_scale)
    {

        cd::Timer tim;
        GLuint vertex_count_x = map_size_x;
        GLuint vertex_count_z = map_size_z;
        GLuint vertex_count = vertex_count_x * vertex_count_z;

        GLuint triangle_count = 2 * (vertex_count_x - 1) * (vertex_count_z - 1); 

        GLfloat * vertex_array = new GLfloat[3 * vertex_count]();
        GLfloat * tex_coord_array = new GLfloat[2 * vertex_count]();
        GLuint * index_array = new GLuint[3 * triangle_count]();

        // generate vertices and texture coordinates
        for(GLuint z = 0; z < vertex_count_z; ++z)
        {
            for(GLuint x = 0; x < vertex_count_x; ++x)
            {
                GLuint index_x = (x + z * vertex_count_x)*3 + 0;
                GLuint index_y = (x + z * vertex_count_x)*3 + 1;
                GLuint index_z = (x + z * vertex_count_x)*3 + 2;
                GLfloat height = height_map[x + z * vertex_count_x];
                vertex_array[index_x] = spatial_scale * static_cast<GLfloat>(x); 
                vertex_array[index_y] = static_cast<GLfloat>(height);
                vertex_array[index_z] = spatial_scale * static_cast<GLfloat>(z);

                // texture coordinates
                GLuint tex_index_x = (x + z * vertex_count_x) * 2 + 0;
                GLuint tex_index_z = (x + z * vertex_count_x) * 2 + 1;
                tex_coord_array[tex_index_x] = spatial_scale * texture_scale * static_cast<GLfloat>(x);
                tex_coord_array[tex_index_z] = spatial_scale * texture_scale * static_cast<GLfloat>(z);
            }
        }

        // generate indices
        PRINTFUNC("Generate indices");
        tim.tic("Generate indices");
        for(GLuint z = 0; z < vertex_count_z - 1; ++z)
        {
            for(GLuint x = 0; x < vertex_count_x - 1; ++x)
            {
                GLuint index = x + z * (vertex_count_x-1);
                index_array[index * 6 + 0] = x + z * vertex_count_x;
                index_array[index * 6 + 1] = x + (z+1) * vertex_count_x;
                index_array[index * 6 + 2] = x+1 + z * vertex_count_x;
                index_array[index * 6 + 3] = x+1 + z * vertex_count_x;
                index_array[index * 6 + 4] = x + (z+1) * vertex_count_x;
                index_array[index * 6 + 5] = x+1 + (z+1) * vertex_count_x;
            }
        }
        tim.toc("Generate indices");
        // put into mesh, return
        Mesh mesh;
        mesh.insertRawPointerData(
                3*triangle_count,
                vertex_count,
                index_array,
                vertex_array,
                NULL,
                NULL,
                NULL,
                tex_coord_array);
        return mesh;
    }

    /*
       std::vector<Mesh> generateGeoMipMap(
       Mesh * mesh,
       GLuint size_x,
       GLuint size_y,
       GLuint num_level)
       {
       std::vector<Mesh> mipmap(num_level);
       for(GLuint k = 0; k < num_level; ++k)
       {

       }
       }
       */

    void generateIndices(
            GLuint vertex_count_z,
            GLuint vertex_count_x,
            GLuint * index_array)
    {
        for(GLuint z = 0; z < vertex_count_z - 1; ++z)
        {
            for(GLuint x = 0; x < vertex_count_x - 1; ++x)
            {
                GLuint index = x + z * (vertex_count_x-1);
                index_array[index * 6 + 0] = x + z * vertex_count_x;
                index_array[index * 6 + 1] = x + (z+1) * vertex_count_x;
                index_array[index * 6 + 2] = x+1 + z * vertex_count_x;
                index_array[index * 6 + 3] = x+1 + z * vertex_count_x;
                index_array[index * 6 + 4] = x + (z+1) * vertex_count_x;
                index_array[index * 6 + 5] = x+1 + (z+1) * vertex_count_x;
            }
        }
    }

    MeshGrid splitMeshIntoGrid(
            Mesh & mesh,
            GLuint size_x,
            GLuint size_y,
            GLuint num_groups)
    {
        GLuint group_size_x = size_x / num_groups;
        GLuint group_size_y = size_y / num_groups;
        GLuint sub_mesh_size_x = group_size_x+1;
        GLuint sub_mesh_size_y = group_size_y+1;
        GLuint sub_mesh_size = (sub_mesh_size_x)*(sub_mesh_size_y);
        num_groups -= 1; // to be sure not seqfaulting

        MeshGrid mesh_grid;
        mesh_grid.m_grid.reserve(num_groups);

        GLuint num_index = 6*(sub_mesh_size_x-1)*(sub_mesh_size_y-1);
        std::vector<GLuint> index_array_template(num_index);
        generateIndices(sub_mesh_size_x, sub_mesh_size_x, &index_array_template[0]);

        for(GLuint y = 0; y < num_groups; ++y)
        {
            std::vector<Mesh> mesh_row;
            mesh_row.reserve(num_groups);
            for(GLuint x = 0; x < num_groups; ++x)
            {
                GLuint group_index = num_groups * y + x;
                Mesh sub_mesh;
                sub_mesh.m_vertex_array.resize(sub_mesh_size);
                sub_mesh.m_normal_array.resize(sub_mesh_size);
                sub_mesh.m_tangent_array.resize(sub_mesh_size);
                sub_mesh.m_bitangent_array.resize(sub_mesh_size);
                sub_mesh.m_tex_coord_array.resize(sub_mesh_size);

                for(GLuint i = 0; i < sub_mesh_size_y; ++i)
                {
                    for(GLuint j = 0; j < sub_mesh_size_x; ++j)
                    {
                        GLuint sub_mesh_index = i*sub_mesh_size_x + j;
                        GLuint mesh_index = (y*group_size_y + i)*size_x + x*group_size_x + j;

                        // Copy vertices
                        sub_mesh.m_vertex_array[sub_mesh_index]
                            = mesh.m_vertex_array[mesh_index];
                        // Recalculate surface vectors afterwards
                        sub_mesh.m_normal_array[sub_mesh_index]
                            = mesh.m_normal_array[mesh_index];
                        sub_mesh.m_tangent_array[sub_mesh_index]
                            = mesh.m_tangent_array[mesh_index];
                        sub_mesh.m_bitangent_array[sub_mesh_index]
                            = mesh.m_bitangent_array[mesh_index];
                        // Tex coords
                        sub_mesh.m_tex_coord_array[sub_mesh_index]
                            = mesh.m_tex_coord_array[mesh_index];
                    }
                }

                sub_mesh.m_index_array = index_array_template; 
                sub_mesh.generateBoundingBox();
                mesh_row.push_back(sub_mesh);
            }
            mesh_grid.m_grid.push_back(mesh_row);
        }
        mesh_grid.size_x = num_groups;
        mesh_grid.size_y = num_groups;
        mesh_grid.patch_size_x = sub_mesh_size_x; 
        mesh_grid.patch_size_y = sub_mesh_size_y; 
        return mesh_grid;
    }

    /*
     * Split mesh into grid with mutli scale edge contraints
     */

    void copyAttribs(
            Mesh & from,
            Mesh & to,
            GLuint size_x,
            GLuint size_y,
            GLuint yfrom,
            GLuint yto,
            GLuint xfrom,
            GLuint xto)
    {
        GLuint numvtx = to.m_vertex_array.size();
        GLuint numattr = (yto-yfrom)*(xto-xfrom);
        to.m_vertex_array.reserve(numvtx+numattr);
        for(GLuint y = yfrom; y < yto; ++y)
        {
            for(GLuint x = xfrom; x < xto; ++x)
            {
                GLuint index = y*size_x + x;
                to.m_vertex_array.push_back(
                        from.m_vertex_array[index]); 
                to.m_normal_array.push_back(
                        from.m_normal_array[index]);
                to.m_tangent_array.push_back(
                        from.m_tangent_array[index]);
                to.m_bitangent_array.push_back(
                        from.m_bitangent_array[index]);
                to.m_tex_coord_array.push_back(
                        from.m_tex_coord_array[index]);
            }
        }
    }

    void generateIndicesGMM(
            GLuint vertex_count_z,
            GLuint vertex_count_x,
            std::vector<GLuint> & index_array)
    {
        GLuint num_index = 6*(vertex_count_x-3)*(vertex_count_z-3);
        index_array.resize(num_index);
        for(GLuint z = 1; z < vertex_count_z-2; ++z)
        {
            for(GLuint x = 1; x < vertex_count_x-2; ++x)
            {
                GLuint index = (x-1) + (z-1) * (vertex_count_x-3);
                index_array[index * 6 + 0] = x + z * vertex_count_x;
                index_array[index * 6 + 1] = x + (z+1) * vertex_count_x;
                index_array[index * 6 + 2] = x+1 + z * vertex_count_x;
                index_array[index * 6 + 3] = x+1 + z * vertex_count_x;
                index_array[index * 6 + 4] = x + (z+1) * vertex_count_x;
                index_array[index * 6 + 5] = x+1 + (z+1) * vertex_count_x;
            }
        }
    }

    template <typename T>
        void map2Dto1D(
                std::vector<std::pair<T,T> > & map,
                std::vector<T> & flatmap,
                GLuint size_x,
                GLuint size_y)
        {
            flatmap.reserve(map.size());
            for(auto it = map.begin(); it != map.end(); ++it)
            {
                T x = it->first;
                T y = it->second;
                flatmap.push_back(y*size_x+x);
            }
        }

    typedef std::vector<std::pair<GLuint,GLuint> > ind2Dmap;

    void generateEdgeIndicesLevel0(
            GLuint size_x,
            GLuint size_y,
            std::vector<GLuint> index_array[2][NUM_SIDE])
    {
        index_array[0][TOP].reserve(2*size_x);
        index_array[0][BOTTOM].reserve(2*size_x);
        index_array[0][LEFT].reserve(2*size_y);
        index_array[0][RIGHT].reserve(2*size_y);
        ind2Dmap inds;
        // Top level 0
        inds.push_back(std::make_pair(1,0));
        inds.push_back(std::make_pair(0,0));
        inds.push_back(std::make_pair(1,1));
        inds.push_back(std::make_pair(size_x-1,0));
        inds.push_back(std::make_pair(size_x-2,0));
        inds.push_back(std::make_pair(size_x-2,1));
        map2Dto1D(inds,index_array[0][TOP],size_x,size_y);
        for(GLuint x = 1; x < size_x-2; ++x)
        {
            GLuint i0 = x; 
            GLuint i1 = x+size_x; 
            GLuint i2 = x+1; 
            GLuint i3 = x+1;
            GLuint i4 = x+size_x;
            GLuint i5 = x+1+size_x;
            index_array[0][TOP].push_back(i0);
            index_array[0][TOP].push_back(i1);
            index_array[0][TOP].push_back(i2);
            index_array[0][TOP].push_back(i3);
            index_array[0][TOP].push_back(i4);
            index_array[0][TOP].push_back(i5);
        }
        // Bottom level 0
        inds.clear();
        inds.push_back(std::make_pair(0,size_y-1));
        inds.push_back(std::make_pair(1,size_y-1));
        inds.push_back(std::make_pair(1,size_y-2));
        inds.push_back(std::make_pair(size_x-2,size_y-2));
        inds.push_back(std::make_pair(size_x-2,size_y-1));
        inds.push_back(std::make_pair(size_x-1,size_y-1));
        map2Dto1D(inds,index_array[0][BOTTOM],size_x,size_y);
        for(GLuint x = 1; x < size_x-2; ++x)
        {
            GLuint i0 = x+(size_y-2)*size_x; 
            GLuint i1 = x+(size_y-1)*size_x; 
            GLuint i2 = x+(size_y-2)*size_x+1; 
            GLuint i3 = x+(size_y-2)*size_x+1;
            GLuint i4 = x+(size_y-1)*size_x;
            GLuint i5 = x+(size_y-1)*size_x+1;
            index_array[0][BOTTOM].push_back(i0);
            index_array[0][BOTTOM].push_back(i1);
            index_array[0][BOTTOM].push_back(i2);
            index_array[0][BOTTOM].push_back(i3);
            index_array[0][BOTTOM].push_back(i4);
            index_array[0][BOTTOM].push_back(i5);
        }
        // Left level 0
        inds.clear();
        inds.push_back(std::make_pair(0,0));
        inds.push_back(std::make_pair(0,1));
        inds.push_back(std::make_pair(1,1));
        inds.push_back(std::make_pair(0,size_y-1));
        inds.push_back(std::make_pair(1,size_y-2));
        inds.push_back(std::make_pair(0,size_y-2));
        map2Dto1D(inds,index_array[0][LEFT],size_x,size_y);
        for(GLuint y = 1; y < size_y-2; ++y)
        {
            GLuint i0 = y*size_x; 
            GLuint i1 = (y+1)*size_x; 
            GLuint i2 = y*size_x+1; 
            GLuint i3 = i2;
            GLuint i4 = i1; 
            GLuint i5 = (y+1)*size_x+1;
            index_array[0][LEFT].push_back(i0);
            index_array[0][LEFT].push_back(i1);
            index_array[0][LEFT].push_back(i2);
            index_array[0][LEFT].push_back(i3);
            index_array[0][LEFT].push_back(i4);
            index_array[0][LEFT].push_back(i5);
        }

        // Right level 0
        inds.clear();
        inds.push_back(std::make_pair(size_x-1,0));
        inds.push_back(std::make_pair(size_x-2,1));
        inds.push_back(std::make_pair(size_x-1,1));
        inds.push_back(std::make_pair(size_x-1,size_y-2));
        inds.push_back(std::make_pair(size_x-2,size_y-2));
        inds.push_back(std::make_pair(size_x-1,size_y-1));
        map2Dto1D(inds,index_array[0][RIGHT],size_x,size_y);
        for(GLuint y = 1; y < size_y-2; ++y)
        {
            GLuint i0 = y*size_x+(size_x-2); 
            GLuint i1 = (y+1)*size_x+(size_x-2); 
            GLuint i2 = y*size_x+1+(size_x-2); 
            GLuint i3 = i2;
            GLuint i4 = i1; 
            GLuint i5 = (y+1)*size_x+1+(size_x-2);
            index_array[0][RIGHT].push_back(i0);
            index_array[0][RIGHT].push_back(i1);
            index_array[0][RIGHT].push_back(i2);
            index_array[0][RIGHT].push_back(i3);
            index_array[0][RIGHT].push_back(i4);
            index_array[0][RIGHT].push_back(i5);
        }
    }

    void generateEdgeIndicesLevel1(
            GLuint size_x,
            GLuint size_y,
            std::vector<GLuint> index_array[2][NUM_SIDE])
    {
        index_array[1][TOP].reserve(2*size_x);
        index_array[1][BOTTOM].reserve(2*size_x);
        index_array[1][LEFT].reserve(2*size_y);
        index_array[1][RIGHT].reserve(2*size_y);
        ind2Dmap inds;
        // Top level 1
        // Outer
        for(GLuint x = 0; x < size_x-2; x += 2)
        {
            inds.push_back(std::make_pair(x,0));
            inds.push_back(std::make_pair(x+1,1));
            inds.push_back(std::make_pair(x+2,0));
        }
        // Inner
        for(GLuint x = 1; x < size_x-2; x += 2)
        {
            inds.push_back(std::make_pair(x,1));
            inds.push_back(std::make_pair(x+1,1));
            inds.push_back(std::make_pair(x+1,0));
            inds.push_back(std::make_pair(x+1,1));
            inds.push_back(std::make_pair(x+2,1));
            inds.push_back(std::make_pair(x+1,0));
        }
        map2Dto1D(inds,index_array[1][TOP],size_x,size_y);

        // Bottom level 1
        inds.clear();
        // outer
        for(GLuint x = 0; x < size_x-2; x += 2)
        {
            inds.push_back(std::make_pair(x,size_x-1));
            inds.push_back(std::make_pair(x+2,size_x-1));
            inds.push_back(std::make_pair(x+1,size_x-2));
        }
        // inner
        for(GLuint x = 1; x < size_x-2; x += 2)
        {
            inds.push_back(std::make_pair(x,size_x-2));
            inds.push_back(std::make_pair(x+1,size_x-1));
            inds.push_back(std::make_pair(x+1,size_x-2));
            inds.push_back(std::make_pair(x+1,size_x-2));
            inds.push_back(std::make_pair(x+1,size_x-1));
            inds.push_back(std::make_pair(x+2,size_x-2));
        }
        map2Dto1D(inds,index_array[1][BOTTOM],size_x,size_y);

        // LEFT level 1
        inds.clear();
        // outer
        for(GLuint y = 0; y < size_y-2; y += 2)
        {
            inds.push_back(std::make_pair(0,y));
            inds.push_back(std::make_pair(0,y+2));
            inds.push_back(std::make_pair(1,y+1));
        }
        // inner
        for(GLuint y = 1; y < size_y-2; y += 2)
        {
            inds.push_back(std::make_pair(1,y));
            inds.push_back(std::make_pair(0,y+1));
            inds.push_back(std::make_pair(1,y+1));
            inds.push_back(std::make_pair(1,y+1));
            inds.push_back(std::make_pair(0,y+1));
            inds.push_back(std::make_pair(1,y+2));
        }
        map2Dto1D(inds,index_array[1][LEFT],size_x,size_y);

        // RIGHT level 1
        inds.clear();
        // outer
        for(GLuint y = 0; y < size_y-2; y += 2)
        {
            inds.push_back(std::make_pair(size_x-1,y));
            inds.push_back(std::make_pair(size_x-2,y+1));
            inds.push_back(std::make_pair(size_x-1,y+2));
        }
        // inner
        for(GLuint y = 1; y < size_y-2; y += 2)
        {
            inds.push_back(std::make_pair(size_x-2,y));
            inds.push_back(std::make_pair(size_x-2,y+1));
            inds.push_back(std::make_pair(size_x-1,y+1));
            inds.push_back(std::make_pair(size_x-1,y+1));
            inds.push_back(std::make_pair(size_x-2,y+1));
            inds.push_back(std::make_pair(size_x-2,y+2));
        }
        map2Dto1D(inds,index_array[1][RIGHT],size_x,size_y);
    }

    MeshGridGMM splitMeshIntoGridMultiScale(
            Mesh & mesh,
            GLuint size_x,
            GLuint size_y,
            GLuint num_groups)
    {
        GLuint group_size_x = size_x / num_groups;
        GLuint group_size_y = size_y / num_groups;
        GLuint sub_mesh_size_x = group_size_x+1;
        GLuint sub_mesh_size_y = group_size_y+1;
        GLuint sub_mesh_size = (sub_mesh_size_x)*(sub_mesh_size_y);
        num_groups -= 1; // to be sure not seqfaulting

        MeshGridGMM mesh_grid;
        mesh_grid.m_grid.reserve(num_groups);

        GLuint num_index = 6*(sub_mesh_size_x-1)*(sub_mesh_size_y-1);
        std::vector<GLuint> index_array_template;
        generateIndicesGMM(sub_mesh_size_x, sub_mesh_size_y, index_array_template);

        std::vector<GLuint> index_array_edge[2][NUM_SIDE];
        generateEdgeIndicesLevel0(sub_mesh_size_x, sub_mesh_size_y, index_array_edge);
        generateEdgeIndicesLevel1(sub_mesh_size_x, sub_mesh_size_y, index_array_edge);

        for(GLuint y = 0; y < num_groups; ++y)
        {
            std::vector<MeshGMM> mesh_row;
            mesh_row.reserve(num_groups);
            for(GLuint x = 0; x < num_groups; ++x)
            {
                GLuint group_index = num_groups * y + x;
                MeshGMM sub_mesh;
                sub_mesh.m_vertex_array.resize(sub_mesh_size);
                sub_mesh.m_normal_array.resize(sub_mesh_size);
                sub_mesh.m_tangent_array.resize(sub_mesh_size);
                sub_mesh.m_bitangent_array.resize(sub_mesh_size);
                sub_mesh.m_tex_coord_array.resize(sub_mesh_size);

                // Body
                for(GLuint i = 0; i < sub_mesh_size_y; ++i)
                {
                    for(GLuint j = 0; j < sub_mesh_size_x; ++j)
                    {
                        GLuint sub_mesh_index = i*sub_mesh_size_x + j;
                        GLuint mesh_index = (y*group_size_y + i)*size_x + x*group_size_x + j;

                        // Copy vertices
                        sub_mesh.m_vertex_array[sub_mesh_index]
                            = mesh.m_vertex_array[mesh_index];
                        sub_mesh.m_normal_array[sub_mesh_index]
                            = mesh.m_normal_array[mesh_index];
                        sub_mesh.m_tangent_array[sub_mesh_index]
                            = mesh.m_tangent_array[mesh_index];
                        sub_mesh.m_bitangent_array[sub_mesh_index]
                            = mesh.m_bitangent_array[mesh_index];
                        // Tex coords
                        sub_mesh.m_tex_coord_array[sub_mesh_index]
                            = mesh.m_tex_coord_array[mesh_index];
                    }
                }

                sub_mesh.m_index_array = index_array_template; 
                for(GLuint k = 0; k < 2; ++k)
                    for(GLuint j = 0; j < NUM_SIDE; ++j) 
                        sub_mesh.m_index_array_edge[k][j] = index_array_edge[k][j];
                sub_mesh.generateBoundingBox();
                mesh_row.push_back(sub_mesh);
            }
            mesh_grid.m_grid.push_back(mesh_row);
        }
        mesh_grid.size_x = num_groups;
        mesh_grid.size_y = num_groups;
        mesh_grid.patch_size_x = sub_mesh_size_x; 
        mesh_grid.patch_size_y = sub_mesh_size_y; 
        return mesh_grid;
    }

    /*
     * Constraint mesh to sub_mesh edges
     */
    void constrainMeshGridEdges(
            Mesh & mesh,
            Mesh & sub_mesh,
            GLint size_x,
            GLint size_y,
            GLint sub_size_x,
            GLint sub_size_y)
    {

        GLint index, sub_index0, sub_index1;

        // Top 
        for(GLint j = 0; j < size_x; ++j)
        {
            index = j;
            sub_index0 = j/2;
            if( j % 2 == 0 )
                sub_index1 = sub_index0;
            else
                sub_index1 = sub_index0 + 1;

            mesh.m_vertex_array[index] = 
                (sub_mesh.m_vertex_array[sub_index0]
                 + sub_mesh.m_vertex_array[sub_index1]) / 2.0f;

            //std::cout<<index<<" ---> "<<sub_index0<<" + "<<sub_index1<<std::endl;
        }
        // Bottom
        for(GLint j = 0; j < size_x; ++j)
        {
            index = (size_y-1)*size_x + j;
            sub_index0 = (sub_size_y-1)*sub_size_x + j/2;
            if( j % 2 == 0 )
                sub_index1 = sub_index0;
            else
                sub_index1 = sub_index0 + 1;

            mesh.m_vertex_array[index] = 
                (sub_mesh.m_vertex_array[sub_index0]
                 + sub_mesh.m_vertex_array[sub_index1]) / 2.0f;

            //std::cout<<index<<" ---> "<<sub_index0<<" + "<<sub_index1<<std::endl;
        }

        // Left
        for(GLint i = 0; i < size_y; ++i)
        {
            index = i*size_x;
            sub_index0 = (i/2)*sub_size_x;
            if( i % 2 == 0 )
                sub_index1 = sub_index0;
            else
                sub_index1 = sub_index0 + sub_size_x;

            mesh.m_vertex_array[index] = 
                (sub_mesh.m_vertex_array[sub_index0]
                 + sub_mesh.m_vertex_array[sub_index1]) / 2.0f;
            //std::cout<<index<<" ---> "<<sub_index0<<" + "<<sub_index1<<std::endl;
        }
        // Right
        for(GLint i = 0; i < size_y; ++i)
        {
            index = i*size_x + size_x - 1;
            sub_index0 = (i/2)*sub_size_x + sub_size_x - 1;
            if( i % 2 == 0 )
                sub_index1 = sub_index0;
            else
                sub_index1 = sub_index0 + sub_size_x;

            mesh.m_vertex_array[index] = 
                (sub_mesh.m_vertex_array[sub_index0]
                 + sub_mesh.m_vertex_array[sub_index1]) / 2.0f;
            //std::cout<<index<<" ---> "<<sub_index0<<" + "<<sub_index1<<std::endl;
        }

        // Constain sub_mesh to mesh
        /*
           GLint sub_index, org_index;
           for(GLint j = 0; j < sub_size_x; ++j)
           {
           sub_index = j;
           org_index = 2*j;
           sub_mesh.m_vertex_array[sub_index] = 
           mesh.m_vertex_array[org_index];

           sub_index = (sub_size_y-1)*sub_size_x + j;
           org_index = (size_y-1)*size_x + 2*j;
           sub_mesh.m_vertex_array[sub_index] = 
           mesh.m_vertex_array[org_index];
           }

           for(GLint i = 0; i < sub_size_y; ++i)
           {
           sub_index = i*sub_size_x;
           org_index = 2*i*size_x;
           sub_mesh.m_vertex_array[sub_index] = 
           mesh.m_vertex_array[org_index];

           sub_index = i*sub_size_x + sub_size_x - 1;
           org_index = 2*i*size_x + size_x - 1;
           sub_mesh.m_vertex_array[sub_index] = 
           mesh.m_vertex_array[org_index];
           }
           */
    }
    /*   
         MeshGrid generateTerrainGrouped(
         std::vector<GLfloat> & height_map,
         GLuint map_size_x,
         GLuint map_size_y,
         GLuint group_size,
         GLfloat texture_scale = 1.0f,
         GLfloat spatial_scale = 1.0f);

         std::vector<MeshGrid> generateTerrainMultiScale(
         std::vector<std::vector<GLfloat> > & height_map_multi_scale,
         Gluint size_x,
         GLuint size_y,
         GLuint group_size,
         GLuint num_levels,
         GLfloat texture_scale = 1.0f,
         GLfloat spatial_scale = 1.0f);
         */

    template<typename T>
        void getNeighborhood(
                std::vector<T> & img,
                int size_x,
                int size_y,
                int x,
                int y,
                T n[8])
        {
            n[0] = img[(y-1)*size_x + x - 1]; 
            n[1] = img[(y-1)*size_x + x];
            n[2] = img[(y-1)*size_x + x + 1];
            n[3] = img[y*size_x + x - 1];
            n[4] = img[y*size_x + x + 1];
            n[5] = img[(y+1)*size_x + x - 1];
            n[6] = img[(y+1)*size_x + x];
            n[7] = img[(y+1)*size_x + x + 1];
        }

    int getPixelNeighborhoodID(
            std::vector<int> & pixel_ids,
            int size_x,
            int size_y,
            int x,
            int y)
    {
        int n[8];
        getNeighborhood(
                pixel_ids,
                size_x, size_y,
                x, y, n);
        int id = pixel_ids[y*size_x+x];
        for(int i = 0; i < 8; ++i)
        {
            id = std::max(id,n[i]);
            /*
               int new_id = n[i];
               if( (n[i] > 0) && (new_id != id) )
               {   
               if( id == 0 )
               id = new_id;
               else
               id = -1;
               } 
               */
        }
        return id;
    }

    bool isPixelRidge(
            std::vector<GLfloat> & height_map,
            std::vector<int> & pixel_ids,
            int size_x, int size_y,
            int x, int y)
    {
        int id_sur[8];
        GLfloat val_sur[8];
        getNeighborhood(
                pixel_ids,
                size_x, size_y,
                x, y, id_sur);
        getNeighborhood(
                height_map,
                size_x, size_y,
                x, y, val_sur);

        GLfloat val = height_map[y*size_x + x];
        bool is_ridge = true;
        for(int i = 0; i < 8; ++i)
        {
            if( id_sur[i] == 0)
            {
                if( val >= val_sur[i] )
                {
                    is_ridge = false;
                    break;
                }   
            }
        }
        return is_ridge;
    }

    std::vector<GLfloat> generateWaterMap(
            std::vector<GLfloat> & height_map,
            GLuint size_x,
            GLuint size_y,
            GLfloat water_height)
    {

        // Find local minimas in heightmap
        std::vector<GLfloat> marker_map(size_x*size_y, 0);
        for(GLuint y = 1; y < size_y-1; ++y)
        {
            for(GLuint x = 1; x < size_x-1; ++x)
            {
                float v0 = height_map[y*size_x + x];
                float v[8];
                v[0] = height_map[(y-1)*size_x + x - 1]; 
                v[1] = height_map[(y-1)*size_x + x];
                v[2] = height_map[(y-1)*size_x + x + 1];
                v[3] = height_map[y*size_x + x - 1];
                v[4] = height_map[y*size_x + x + 1];
                v[5] = height_map[(y+1)*size_x + x - 1];
                v[6] = height_map[(y+1)*size_x + x];
                v[7] = height_map[(y+1)*size_x + x + 1];
                bool is_local_min = true;
                for(GLuint i = 0; i < 8; ++i)
                {
                    if( v[i] <= v0 )
                    {
                        is_local_min = false;
                        break;
                    }
                }
                if( is_local_min )
                    marker_map[y*size_x + x] = 1.0;
            }
        }

        // Create index image
        std::vector<int> pixel_ids(marker_map.size(),0);
        int id_cnt = 0;
        for(GLuint y = 0; y < size_y; ++y)
        {
            for(GLuint x = 0; x < size_x; ++x)
            {
                int index = y*size_x + x;
                if( marker_map[index] )
                {
                    id_cnt += 1;
                    pixel_ids[index] = id_cnt;
                }
            }
        }

        // Dilate pixel ids according to height map
        std::vector<bool> id_complete(id_cnt,false);
        std::vector<GLfloat> id_heights(id_cnt,100000);
        std::vector<int> next_pixel_ids(pixel_ids.size(),0);
        int num_iter = 10;
        for(int iter = 0; iter < num_iter; ++iter)
        {
            cv::Mat id_img(size_y, size_x, CV_32SC1, &pixel_ids[0]);
            id_img.convertTo(id_img,CV_8UC3);
            cv::imshow("ids",id_img);
            cv::waitKey(0);
            for(GLuint y = 1; y < size_y-1; ++y)
            {
                for(GLuint x = 1; x < size_x-1; ++x)
                {
                    int index = y*size_x + x;
                    int id = getPixelNeighborhoodID(pixel_ids, size_x, size_y, x, y);
                    bool is_ridge = isPixelRidge(height_map, pixel_ids, size_x, size_y, x, y);
                    std::cout << "index = " << index << "/" << size_x*size_y << ", id = " << id << ", ridge = " << is_ridge << std::endl;
                    if( (id > 0) && is_ridge )
                    {
                        next_pixel_ids[index] = id;
                        //id_heights[id] = std::max(id_heights[id], height_map[index]);
                    }
                    else
                    {
                        next_pixel_ids[index] = -1;
                        int n[8];
                        getNeighborhood(pixel_ids, size_x, size_y, x, y, n);
                        for(int i = 0; i < 8; ++i)
                        {
                            if( n[i] > 0 )
                            {
                                id_heights[n[i]] = std::min(id_heights[n[i]],height_map[index]);
                            }
                        }
                    }
                }
            }
            pixel_ids.swap(next_pixel_ids);
        }

        std::vector<GLfloat> water_map(size_x*size_y,0);
        for(GLuint y = 0; y < size_y; ++y)
        {
            for(GLuint x = 0; x < size_x; ++x)
            {
                GLuint index = y*size_x + x;
                if( pixel_ids[index] > 0 ) {
                    water_map[index] = id_heights[pixel_ids[index]];
                }
            }
        }

        std::cout << "RETURN" << std::endl;
        return water_map;
    }

    /*
     * Generate random 3D texture
     */
    void generatedTexture3D(
            GLuint size_1,
            GLuint size_2,
            GLuint size_3,
            GLfloat * texture)
    {
        // TODO 
    }

    std::vector<GLfloat> linspace(GLfloat from, GLfloat to, GLuint num)
    {
        GLfloat step = (to - from) / (num-1);
        std::vector<GLfloat> space(num);
        for(GLuint i = 0; i < num; ++i)
        {
            space[i] = from + step*i;
        }
        return space;
    }

    Mesh createDome(
            GLuint resolution,
            GLfloat size,
            GLfloat angle,
            GLfloat tex_scale)
    {

        GLfloat height = size * std::cos(angle);
        std::vector<GLfloat> Th = linspace(0, angle, resolution);
        std::vector<GLfloat> Ph = linspace(0, 2*M_PI, 4 * resolution);
        GLuint num_vertex = Th.size() * Ph.size();
        GLuint num_index = 6 * (Th.size()-1) * (Ph.size()-1);
        Mesh mesh;
        mesh.m_index_array.resize(num_index);
        mesh.m_vertex_array.resize(num_vertex);
        mesh.m_tex_coord_array.resize(num_vertex);
        for(GLuint i = 0; i < Th.size(); ++i)
        {
            for(GLuint j = 0; j < Ph.size(); ++j)
            {
                GLfloat th = Th[i];
                GLfloat ph = Ph[j];
                GLfloat x = std::sin(th) * std::cos(ph) * size;
                GLfloat z = std::sin(th) * std::sin(ph) * size;
                GLfloat y = std::cos(th) * size - height;
                GLfloat u = tex_scale * (x / size); //static_cast<GLfloat>(j) / Ph.size();
                GLfloat v = tex_scale * (z / size); //static_cast<GLfloat>(i) / Th.size();

                GLuint index = i * Ph.size() + j;
                mesh.m_vertex_array[index] = glm::vec3(x,y,z);
                mesh.m_tex_coord_array[index] = glm::vec2(u,v);
            }
        }

        for(GLuint i = 0; i < Th.size() - 1; ++i)
        {
            for(GLuint j = 0; j < Ph.size() - 1; ++j)
            { 
                int index = j + i * (Ph.size()-1);
                mesh.m_index_array[index * 6 + 0] = j + i * Ph.size();
                mesh.m_index_array[index * 6 + 1] = j + (i+1) * Ph.size();
                mesh.m_index_array[index * 6 + 2] = j+1 + i * Ph.size();
                mesh.m_index_array[index * 6 + 3] = j+1 + i * Ph.size();
                mesh.m_index_array[index * 6 + 4] = j + (i+1) * Ph.size();
                mesh.m_index_array[index * 6 + 5] = j+1 + (i+1) * Ph.size();
            }
        }
        return mesh;
    };  

    Mesh createQuad(
            GLfloat x,
            GLfloat y,
            GLfloat width,
            GLfloat height)
    {
        Mesh mesh;
        mesh.m_index_array.reserve(6);        
        mesh.m_vertex_array.reserve(4);        
        mesh.m_tex_coord_array.reserve(4);
        mesh.m_vertex_array.push_back(glm::vec3(x,0,y));
        mesh.m_vertex_array.push_back(glm::vec3(x,0,y+height));
        mesh.m_vertex_array.push_back(glm::vec3(x+width,0,y));
        mesh.m_vertex_array.push_back(glm::vec3(x+width,0,y+height));
        mesh.m_tex_coord_array.push_back(glm::vec2(0,0));
        mesh.m_tex_coord_array.push_back(glm::vec2(0,1));
        mesh.m_tex_coord_array.push_back(glm::vec2(1,0));
        mesh.m_tex_coord_array.push_back(glm::vec2(1,1));
        mesh.m_index_array.push_back(0);
        mesh.m_index_array.push_back(1);
        mesh.m_index_array.push_back(2);
        mesh.m_index_array.push_back(3);
        mesh.m_index_array.push_back(2);
        mesh.m_index_array.push_back(1);

        mesh.m_normal_array.resize(4);
        generateNormals(
                (GLfloat*)&mesh.m_vertex_array[0],
                (GLuint*)&mesh.m_index_array[0],
                mesh.m_vertex_array.size(),
                mesh.m_index_array.size() / 3,
                (GLfloat*)&mesh.m_normal_array[0]);
        return mesh;
    };
}
