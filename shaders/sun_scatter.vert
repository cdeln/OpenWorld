#version 150

#define PI 3.14

float sun(float f)
{
    return 10;// * f*f;
}

float rayleigh(float f)
{
    return 1.0 / pow(f,4);
}

float mie(float f)
{
    return 1.0 / pow(f,2);
}

in vec3 inDir;

out vec3 varDir;
out vec3 varColorRay;
out vec3 varColorMie;

uniform mat4 projMatrix;
uniform mat4 mdlMatrix;
uniform vec3 sunDir;
uniform float skyRadius;
uniform float skyAngle;

#define NUM_WAVELEN 6
uniform float wavelen[NUM_WAVELEN];
uniform vec3 tristimulus[NUM_WAVELEN];
uniform float sunLight[NUM_WAVELEN];

void main(void)
{
    gl_Position = projMatrix * vec4(mat3(mdlMatrix) * inDir, 1);

    // Assume camera in (0,0,0)
    const int numSamples = 2;
    const float step = 1.0 / numSamples;
    const float sunFactor = 70.0;
    float scaleFactor = 32.0/skyRadius;

    //const float n = 1.000293;
    //const float psize = 1e-2; // nm
    //const float Kr = (2.0*pow(PI,5)/3.0)*pow(psize,6)*pow( (n*n-1)/(n*n+2), 2);
    const float Kr = 0.0005;
    const float Km = 0.01;
    const float lambdaDiv = 1000.0;
    vec3 B = inDir;

    float skyRadius2 = pow(skyRadius,2);
    float groundHeight = skyRadius*cos(skyAngle);
    vec3 ground = vec3(0,groundHeight,0);
 
    float responseRay[NUM_WAVELEN];
    float responseMie[NUM_WAVELEN];

    for(int k = 0; k < NUM_WAVELEN; ++k)
    {
        float lambda = wavelen[k] / lambdaDiv;
        float R = Kr * rayleigh(lambda);
        float M = Km * mie(lambda);
        float outerRay = 0;
        float outerMie = 0;
        for(int i = 1; i <= numSamples; ++i)
        {
            vec3 P1 = i * step * B;

            // Calculate intersection with skysdome
            vec3 p = P1 + ground;
            float a = - dot(sunDir,p);
            float b = sqrt(max(0, skyRadius2 - dot(p,p) + pow(dot(p,sunDir),2)));
            float lambda1 = a + b;
            float lambda2 = a - b;
            vec3 D = lambda1*sunDir;
            //vec3 intersection = p + lambda1*sunDir - ground;
            //vec3 D = intersection - P1;

            float innerAP = 0;
            float innerPC = 0;
            for(int j = 1; j <= numSamples; ++j)
            {
                vec3 P2 = j * step * P1;
                float height = length(P2 + ground) - groundHeight;
                innerAP += exp(-scaleFactor*height);
            }
            for(int j = 1; j <= numSamples; ++j)
            {
                vec3 C = P1 + (j * step) * D;
                float height = length(C + ground) - groundHeight;
                innerPC += exp(-scaleFactor*height);
            }
            innerAP *= step * length(P1);
            innerPC *= step * length(D);
            float inner = innerAP + innerPC;
            float innerRay = R * inner;
            float innerMie = M * inner;
            float height = length(p) - groundHeight;
            outerRay += exp(-scaleFactor*height) * exp(-innerRay);
            outerMie += exp(-scaleFactor*height) * exp(-innerMie);
        }
        outerRay *= step * length(B);
        outerMie *= step * length(B);
        responseRay[k] = sunFactor * sunLight[k] * R * outerRay;
        responseMie[k] = sunFactor * sunLight[k] * M * outerMie;
    }    

    varColorRay = vec3(0,0,0);
    varColorMie = vec3(0,0,0);
    for(int k = 0; k < NUM_WAVELEN; ++k)
    {
        varColorRay.r += tristimulus[k].r * responseRay[k];
        varColorRay.g += tristimulus[k].g * responseRay[k];
        varColorRay.b += tristimulus[k].b * responseRay[k];

        varColorMie.r += tristimulus[k].r * responseMie[k];
        varColorMie.g += tristimulus[k].g * responseMie[k];
        varColorMie.b += tristimulus[k].b * responseMie[k];
    }

    varDir = inDir;
}
