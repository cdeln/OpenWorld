#ifndef __LIGHT_HPP__
#define __LIGHT_HPP__

#include <cmath>

namespace cd
{

    const double hc2 = 1.1916*(1e-16);
    const double hck = 0.014391304347826;
    const double nm = 1e-9;
    const double XYZ2RGB[3][3] = {
        {0.418, -0.158, -0.082},
        {-0.091, 0.252, 0.015},
        {0.000, -0.002, 0.178}
    };

    // wavelength in nanometers
    double lightIntensity(double wavelen, double temp)
    {
        return hc2 / (std::pow(nm*wavelen,5) * (std::exp(hck /(nm*wavelen * temp)) - 1));
    }

    double tristimX31(double wavelen)
    {
        return 1.065*std::exp(-0.5*std::pow((wavelen-595.8)/33.33,2))
            + 0.366*std::exp(-0.5*std::pow((wavelen-444.8)/19.44, 2));
    }
    double tristimY31(double wavelen)
    {
        return 1.014*std::exp(-0.5*std::pow((std::log(wavelen)-std::log(556.3))/0.075, 2)); 
    }
    double tristimZ31(double wavelen)
    {
        return 1.839*std::exp(-0.5*std::pow((std::log(wavelen)-std::log(449.8))/(0.051),2));
    }
    double tristimR31(double wavelen)
    {
        return XYZ2RGB[0][0]*tristimX31(wavelen)
            + XYZ2RGB[0][1]*tristimY31(wavelen)
            + XYZ2RGB[0][2]*tristimZ31(wavelen);
    }
    double tristimG31(double wavelen)
    {
        return XYZ2RGB[1][0]*tristimX31(wavelen)
            + XYZ2RGB[1][1]*tristimY31(wavelen)
            + XYZ2RGB[1][2]*tristimZ31(wavelen);
    }
    double tristimB31(double wavelen)
    {
        return XYZ2RGB[2][0]*tristimX31(wavelen)
            + XYZ2RGB[2][1]*tristimY31(wavelen)
            + XYZ2RGB[2][2]*tristimZ31(wavelen);
    }
}
#endif
