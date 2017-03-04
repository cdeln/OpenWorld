#include <iostream> 
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "autogen.hpp"

int main(int argc, char ** argv)
{
    GLuint map_size_x = 640;
    GLuint map_size_z = 640;

    while(true)
    {
        std::vector<GLfloat> height_map = cd::generateTexture2D(map_size_x, map_size_z, 1.0, 2.0);
        std::vector<GLfloat> snow_map = cd::generateTexture2D(height_map, map_size_x, map_size_z, 1.0, 2.0, 1.5);
		//std::vector<GLfloat> water_map = cd::generateWaterMap(height_map, map_size_x, map_size_z,10.0);
        cv::Mat mat = cv::Mat(cv::Size(map_size_x, map_size_z), CV_32F, &height_map[0]);
        cv::Mat snow = cv::Mat(cv::Size(map_size_x, map_size_z), CV_32F, &snow_map[0]);
		//cv::Mat water = cv::Mat(cv::Size(map_size_x, map_size_z), CV_32F, &water_map[0]);

        double min_val, max_val;
        cv::Point min_loc, max_loc;
        cv::minMaxLoc(mat, &min_val, &max_val, &min_loc, &max_loc);
        std::cout << "min_val = " << min_val << std::endl;
        std::cout << "max_val = " << max_val << std::endl;
        std::cout << "min_loc = " << min_loc << std::endl;
        std::cout << "max_loc = " << max_loc << std::endl;

        mat -= min_val;
        mat /= (max_val - min_val);
        std::cout << "imshow" << std::endl;
        cv::imshow("height",mat);
        cv::imshow("snow",snow);
		//cv::imshow("water",water);
        std::cout << "imshow success" << std::endl;
        char q = cv::waitKey(0);
        if( q == 'q')
            break;
    }

    return 0;
}
