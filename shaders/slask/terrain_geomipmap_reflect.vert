#version 420

in vec3 inPosition;
in vec3 inTangent;
in vec3 inBitangent;
in vec3 inNormal;
in vec2 inTexCoord;
//in float inSnowLevel;

out vec2 varTexCoord;
out vec3 varLight;
out vec3 varView;
out vec3 varNormalGlobal;
out vec3 varPosition;
out float height;
out float snowLevel;
//out float varSnowLevel;

uniform mat4 projMatrix;
uniform mat4 mdlMatrix;
uniform vec3 sunDir;
uniform float spatialScale;

uniform layout(rgba32f) image2D texUnitSnowLevel;

void main(void)
{
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
}
