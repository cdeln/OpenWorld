
#include "tex.hpp"
#include <opencv2/core/core.hpp>
//#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <exception>

/*
 * * * * C1  C2  C3  C4
 CV_8U   0   8   16  24
 CV_8S   1   9   17  25
 CV_16U  2   10  18  26
 CV_16S  3   11  19  27
 CV_32S  4   12  20  28
 CV_32F  5   13  21  29
 CV_64F  6   14  22  30
 */
namespace cd
{

    std::string cvType2Str(int t)
    {
        int c = (t / 8) + 1;
        int r = t % 8;
        std::string st;
        switch(r)
        {
            case 0: st = "8U"; break;
            case 1: st = "8S"; break;
            case 2: st = "16U"; break;
            case 3: st = "16S"; break;
            case 4: st = "32U"; break;
            case 5: st = "32F"; break;
            case 6: st = "64F"; break;
            default: st = "UNKNOWN";
        }

        st = "CV_" + st + "_C" + std::to_string(c);
        return st;
    }

    GLuint loadTexture2D(const std::string & path)
    {
        cv::Mat img = cv::imread(path, -1);    
        std::cout << "Image size = " << img.size()  << ", channels = " << img.channels() << ", type = " << cvType2Str(img.type()) << std::endl;
        GLuint tex_objID;
        glGenTextures(1, &tex_objID);
        glBindTexture(GL_TEXTURE_2D, tex_objID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	// Linear Filtered
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// Linear Filtered

        int image_type = img.type();
        if( image_type == CV_8UC3 )
        { 
            std::cout << "Pushing texture CV_8UC3" << std::endl;
            glTexImage2D(GL_TEXTURE_2D,
                    0, GL_RGBA,
                    img.cols, img.rows,
                    0, GL_BGR,
                    GL_UNSIGNED_BYTE,
                    img.data);
        }
        else if( image_type == CV_16UC3)
        {
            std::cout << "Pushing texture CV_16UC3" << std::endl;
            glTexImage2D(GL_TEXTURE_2D,
                    0, GL_RGBA,
                    img.cols, img.rows,
                    0, GL_BGR,
                    GL_UNSIGNED_SHORT,
                    img.data);
        }
        else if( image_type == CV_8UC4 )
        {
            std::cout << "Pushing texture CV_8UC4" << std::endl;
            glTexImage2D(GL_TEXTURE_2D,
                    0, GL_RGBA,
                    img.cols, img.rows,
                    0, GL_BGRA,
                    GL_UNSIGNED_BYTE,
                    img.data);
        }

        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        return tex_objID;
    }

    GLuint loadTexture3D(std::vector<std::string> & paths)
    {
        std::vector<GLubyte> texels;
        GLuint width, height;
        int type;
        for(auto path = paths.begin(); path != paths.end(); ++path)
        {
            cv::Mat img = cv::imread(*path,-1);
            width = img.cols;
            height = img.rows;
            type = img.type();
            texels.insert(texels.end(), &img.data[0], &img.data[4*width*height]);
        }

        GLuint tex_objID;
        glGenTextures(1, &tex_objID);
        glBindTexture(GL_TEXTURE_3D, tex_objID);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        //glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        //glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        //glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

        if( type == CV_8UC4 ) 
        {
            std::cout << "Pushing texture3D CV_8UC4" << std::endl;
            glTexImage3D(
                    GL_TEXTURE_3D,
                    0, GL_RGBA,
                    width, height, paths.size(),
                    0, GL_RGBA,
                    GL_UNSIGNED_BYTE,
                    &texels[0]);
        }
        else
        {
            std::stringstream ss;
            ss << "loadTexture3D " << "Unsupported file format " << cvType2Str(type);
            throw std::runtime_error(ss.str());
        }


        return tex_objID;
    }

    GLuint loadTexture3D(
            const std::vector<GLfloat> & tex,
            GLuint size_x,
            GLuint size_y,
            GLuint size_z)
    {

        GLuint tex_objID;
        glGenTextures(1, &tex_objID);
        glBindTexture(GL_TEXTURE_3D, tex_objID);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage3D(
                GL_TEXTURE_3D,
                0, GL_RGBA,
                size_x, size_y, size_z, 
                0, GL_RED,
                GL_FLOAT,
                &tex[0]);
        return tex_objID;
    }
}
