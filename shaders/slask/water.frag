#version 150

in vec3 varNormal;
in vec3 varLight;
in vec3 varPos;

uniform float domeRadius;
uniform float domeAngle;
uniform float domeTexScale;
uniform sampler3D cloudSampler;
uniform mat4 mdlMatrix;
uniform float time;

out vec4 outColor;

#define M_PI 3.1415926535897932384626433832795

void main(void)
{

    const vec2 windVel = vec2(0.01,0);
    const float cloudChangeSpeed = 0.02;

    vec3 normal = normalize(varNormal);
    vec3 light = normalize(varLight);
    float shadeAmbient = 0.5;
    float shadeDiffuse = clamp(dot(normal, light), 0, 1);

    vec3 reflection = 2 * dot(normal, light) * normal - light;
    vec3 myPosDirection = - normalize(varPos);
    float shadeSpec = pow(clamp(dot(myPosDirection, reflection), 0, 1), 50.0);
    
    vec3 viewReflection = transpose(mat3(mdlMatrix)) * normalize(reflect(varPos, normal));
    float th = domeAngle * acos(viewReflection.y) / M_PI;
    float ph = atan(viewReflection.x, viewReflection.z);
    float domeTexX = sin(th) * cos(ph);
    float domeTexZ = sin(th) * sin(ph);
    vec3 cloudTexCoord = vec3(domeTexScale * domeTexX, domeTexScale * domeTexZ, 0); 
    cloudTexCoord += vec3(time * windVel, time * cloudChangeSpeed);
    vec4 cloudTexColor = texture(cloudSampler, cloudTexCoord);

    float shade = clamp(shadeAmbient + 0.5*shadeDiffuse + 0.5*shadeSpec,0,1);
    outColor = cloudTexColor + shade * vec4(133.0/255.0, 200.0/255.0, 242.0/255.0, 0.75);
}
