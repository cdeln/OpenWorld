#version 150

uniform bool enableRayleigh;
uniform bool enableMie;

#define PI 3.14

float phase(vec3 a, vec3 b, float g)
{
    float c = dot(a,b);
    float gFactor = 3.0*(1.0 - g*g) / (2.0*(2.0 + g*g));
    return gFactor * (1.0 + c*c) / pow(1 + g*g - 2*g*c, 3.0/2.0);
}

float phaseRay(vec3 a, vec3 b, float g)
{
    float c = dot(a,b);
    return (1.0 + c*c)/2.0;
}

float phaseMie(vec3 a, vec3 b, float g)
{
    float c = dot(a,b);
    return (1-g*g)/(4*PI*pow(1 + g*g - 2*g*c, 1.5));
}

in vec3 varDir;
in vec3 varColorRay;
in vec3 varColorMie;

out vec4 outColor;

uniform vec3 sunDir;

void main(void)
{
    const float gRay = 0.05;
    const float gMie = 0.75;
    const float gamma = 0.5;
    vec3 B = normalize(varDir);
    vec3 D = normalize(sunDir);
    vec3 colorRay = phaseRay(B,D,gRay) * varColorRay;
    vec3 colorMie = phaseMie(B,D,gMie) * varColorMie;

    //float powDiff = length(colorMie)/length(colorRay);
    /*
    for(int k = 0; k < 3; ++k)
    {
        colorRay[k] = pow(colorRay[k],gamma);
        //colorMie[k] = pow(colorMie[k],gamma);
    }
    */
    //outColor = vec4((colorRay + powDiff*colorMie)/2.0, 1.0);

    //float ray = 0;
    //float mie = 0;
    float alphaRay = colorRay.b;
    float alphaMie = colorMie.r;
    if( ! enableRayleigh )
    {
        alphaRay = 0.0;
    }
    if( ! enableMie )
    {
        alphaMie = 0.0;
    }
    vec3 colorBlend = (alphaRay*colorRay + alphaMie*colorMie)/(alphaRay+alphaMie);
    outColor = vec4(colorBlend,1);
    //outColor = vec4(colorRay,1.0);

    //if( dot(B,D) > 0.99 )
    //    outColor = vec4(1,1,0,1);
    outColor.a = 1;//outColor.b;
}
