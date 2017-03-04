#include <iostream> 
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <fftw3.h>

void printFFTWComplexArray(fftw_complex * array, int n)
{
    for(int i = 0; i < n; ++i)
    {
        std::cout << "(" << array[i][0] << "," << array[i][1] << "), ";
    }
    std::cout << std::endl;
}

int main(int argc, char ** argv)
{

    int n = 5;
    fftw_complex * in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * n);
    for(int i = 0; i < n; ++i)
    {
        in[i][0] = 0;
        in[i][1] = 0;
    }
    in[1][0] = 1;
    fftw_complex * out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * n);
    fftw_plan p = fftw_plan_dft_1d(n, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(p);

    printFFTWComplexArray(in,n);
    printFFTWComplexArray(out,n);

    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);
    return 0;
}
