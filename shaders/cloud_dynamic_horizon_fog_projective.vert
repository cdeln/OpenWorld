#version 150

in vec3 inPosition;
in vec2 inTexCoord;

uniform mat4 projMatrix;
uniform mat4 mdlMatrix;
uniform float time;
uniform float skyRadius;
uniform float skyHeight;
uniform bool enableProj;
uniform bool enableFog;

out vec2 varTexCoord;
out float fogLevel;

void main(void)
{
    gl_Position = projMatrix * vec4(mat3(mdlMatrix) * inPosition, 1.0);
    const vec2 wind_vel = vec2(0.02,0);
    if( enableProj )
    {
        varTexCoord = vec2(inPosition.x,inPosition.z)/(inPosition.y * skyRadius / skyHeight);
    }
    else
    {
        varTexCoord = inTexCoord;
    }
    varTexCoord += time*wind_vel;


    vec3 posNorm = normalize(inPosition);
    if( enableFog )
    {
        fogLevel = 1.0 - posNorm.y;
    }
    else
    {
        fogLevel = 0;
    }
}
