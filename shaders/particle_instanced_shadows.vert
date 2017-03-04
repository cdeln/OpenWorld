#version 330

layout(location = 0) in vec3 inPosition;
layout(location = 2) in vec3 inVertex;
layout(location = 3) in vec2 inTexCoord;

out vec2 varTexCoord;
out float varDist;
out float visibility;

uniform mat4 projMatrix;
uniform mat4 mdlMatrix;
uniform mat4 shadowMVP;
uniform sampler2D shadowSampler;
uniform bool enableShadow;

float smoothStep(float low, float high, float val)
{
    return clamp((val-low)/(high-low),0,1);
}

float smoothShadow(sampler2D depthSampler, vec2 depthCoord, float reference, float depthBias)
{
    float depth = texture(depthSampler, depthCoord).r;
    return 1.0 - smoothStep(depth, depth + depthBias, reference);
}

float filteredShadow(sampler2D depthSampler, vec4 depthCoord, vec2 texSize, float depthBias, int range)
{
    float shadowValue = 0;
    for(int i = -range; i <= range; ++i)
    {
        for(int j = -range; j <= range; ++j)
        {
            shadowValue += smoothShadow(depthSampler, depthCoord.xy + vec2(i,j)/texSize, depthCoord.z, depthBias);
        }
    }
    return shadowValue / pow(2*range+1,2);
} 

void main(void)
{

    const float depthSensitivity = 0.01;

    vec4 pos = mdlMatrix * vec4(inPosition,1);
    vec4 posShift = pos + vec4(inVertex,0);
    gl_Position = projMatrix * posShift;
    varTexCoord = inTexCoord;
    varDist = gl_Position.w;

    if( enableShadow )
    {
        vec4 shadowCoord = shadowMVP * vec4(inPosition,1);
        shadowCoord *= 0.5;
        shadowCoord += 0.5*vec4(1,1,1,1);
        visibility = smoothShadow(shadowSampler, shadowCoord.xy, shadowCoord.z, depthSensitivity); 
    }
    else
    {
        visibility = 1;
    }
}
