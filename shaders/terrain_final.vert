#version 330

in vec3 inPosition;
in vec3 inTangent;
in vec3 inBitangent;
in vec3 inNormal;
in vec2 inTexCoord;
//in float inSnowLevel;

out vec2 varTexCoord;
out vec4 varShadowCoord;
out vec3 varLight;
out vec3 varView;
out vec3 varNormal;
out float height;
out float snowLevel;
//out float varSnowLevel;
out float shadowLevel;

uniform mat4 mdlMatrix;
uniform mat4 MVPMatrix;
uniform mat3 normalMatrix;
uniform mat4 shadowMVP;
uniform vec3 sunDir;
//uniform float spatialScale;
uniform float mapSizeX;
uniform float mapSizeZ;
uniform float mapHeight;
uniform float snowProbThresh;
uniform vec2 texSize;

uniform sampler2D shadowSampler;
uniform bool enableShadow;
uniform float depthMargin;
uniform int depthNumSample; 

uniform bool enableSnowTexture;
uniform bool enableNormalMapping;

//uniform layout(rgba32f) image2D texUnitSnowLevel;
uniform sampler2D texUnitSnowLevel;

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
    //const float depthSensitivity = 0.01;
    //const int depthNumSample = 2;

    vec3 tangent = normalMatrix * inTangent;
    vec3 bitangent = normalMatrix * inBitangent; 
    vec3 normal = normalMatrix * inNormal;
    vec4 worldPos = mdlMatrix * vec4(inPosition, 1.0);
    if( enableNormalMapping )
    {
        mat3 TBN = transpose(mat3(tangent, bitangent, normal));
        varLight = TBN * normalMatrix * sunDir;
        varView = - TBN * vec3(worldPos / worldPos.w);
    }
    else
    {
        varLight = normalMatrix * sunDir;
        varView = -vec3(worldPos / worldPos.w);
    }
    //varPosition = inPosition;
    varNormal = normal; 
    vec2 posTexCoord = vec2(inPosition.x / mapSizeX, inPosition.z / mapSizeZ); 
    if( enableSnowTexture )
    {
        snowLevel = texture(texUnitSnowLevel, posTexCoord).r;
    }
    else
    {
        if( inPosition.y > mapHeight*snowProbThresh )
            snowLevel = 1;
        else
            snowLevel = 0; 
    }

    varTexCoord = inTexCoord;

    height = inPosition.y;

    gl_Position = MVPMatrix * vec4(inPosition, 1.0);

    if( enableShadow )
    {
        vec4 shadowCoord = shadowMVP * vec4(inPosition,1);
        shadowCoord *= 0.5;
        shadowCoord += 0.5*vec4(1,1,1,1);
        shadowLevel = filteredShadow(shadowSampler, shadowCoord, texSize, depthMargin, depthNumSample);
    }
    else {
        shadowLevel = 1;
    }
}
