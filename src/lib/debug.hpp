#ifndef __DEBUG__
#define __DEBUG__

#include "gl_include.h"
#include <iostream>
#include <map>
#include <string>
#include <ctime>

#define PRINT(X) std::cout << X << std::endl;
#define PRINTFUNC(X) std::cout << __func__ << " :: " << X << std::endl;
#define VERBOSE(X) std::cout << #X " = " << X << std::endl;

#define FBO_PRINT(X) std::cout << "FBOError :: " << #X << std::endl;

namespace cd
{
    GLenum checkFBO();

    class Timer
    {
        private:
            std::map<std::string,double> TIMEMAP;

        public:
            void tic(const std::string & name)
            {
                auto it = TIMEMAP.find(name);
                if( it != TIMEMAP.end() )
                {
                    it->second = static_cast<double>(std::clock());
                }
                else
                {
                    TIMEMAP.insert(it, std::make_pair(name, static_cast<double>(std::clock())));
                }
            }

            void toc(const std::string & name)
            {
                auto it = TIMEMAP.find(name);
                if( it != TIMEMAP.end() )
                {
                    double time_elapsed = static_cast<double>(std::clock() - it->second) / CLOCKS_PER_SEC;
                    std::cout << "TIME["<<name<<"] = " << time_elapsed << std::endl;
                }
                else
                {
                    std::cout << "TIME["<<name<<"] = UNKNOWN" << std::endl;
                }
            }
    };
}

#endif
