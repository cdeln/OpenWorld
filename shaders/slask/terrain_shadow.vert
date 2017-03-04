#version 420

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
out vec3 varNormalGlobal;
out vec3 varPosition;
out float height;
out float snowLevel;
//out float varSnowLevel;
out float shadowLevel;

uniform mat4 projMatrix;
uniform mat4 mdlMatrix;
uniform mat4 shadowProjMatrix;
uniform mat4 shadowMdlMatrix;
uniform vec3 sunDir;
uniform float spatialScale;

uniform sampler2D shadowSampler;

uniform layout(rgba32f) image2D texUnitSnowLevel;

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
    const int depthNumSample = 3;

    mat3 normalMatrix = mat3(mdlMatrix);

    vec3 tangent = normalMatrix * normalize(inTangent);
    vec3 bitangent = normalMatrix * normalize(inBitangent); 
    vec3 normal = normalMatrix * normalize(inNormal);
    mat3 TBN = transpose(mat3(tangent, bitangent, normal));
    varLight = TBN * normalMatrix * sunDir;
    vec4 worldPos = mdlMatrix * vec4(inPosition, 1.0);
    varView = - TBN * vec3(worldPos / worldPos.w);
    varNormalGlobal = inNormal;
    varPosition = inPosition;
    ivec2 posTexCoord = ivec2(int(inPosition.x/spatialScale), int(inPosition.z/spatialScale)); 
    vec4 snowLevelVec = imageLoad(texUnitSnowLevel, posTexCoord); 
    snowLevel = snowLevelVec.r;

    varTexCoord = inTexCoord;

    height = inPosition.y;

    gl_Position = projMatrix * mdlMatrix * vec4(inPosition, 1.0);

    vec4 shadowCoord = shadowProjMatrix * shadowMdlMatrix * vec4(inPosition,1);
    shadowCoord *= 0.5;
    shadowCoord += 0.5*vec4(1,1,1,1);
    shadowLevel = filteredShadow(shadowSampler, shadowCoord, vec2(150,150), depthSensitivity, depthNumSample);
}
