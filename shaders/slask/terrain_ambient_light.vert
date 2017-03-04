#version 150

in vec3 inPosition;
in vec3 inTangent;
in vec3 inBitangent;
in vec3 inNormal;
in vec2 inTexCoord;
in float inSnowLevel;

out vec2 varTexCoord;
out vec3 varView;
out vec3 varNormalGlobal;
out float height;
out float varSnowLevel;
out vec3 varLight[6];

uniform mat4 projMatrix;
uniform mat4 mdlMatrix;
uniform vec3 lightDir[6];

void main(void)
{
	mat3 normalMatrix = mat3(mdlMatrix);

    vec3 tangent = normalMatrix * normalize(inTangent);
    vec3 bitangent = normalMatrix * normalize(inBitangent); 
    vec3 normal = normalMatrix * normalize(inNormal);
    mat3 TBN = transpose(mat3(tangent, bitangent, normal));
    for(int i = 0; i < 6; ++i)
        varLight[i] = TBN * normalMatrix * lightDir[i];
    vec4 worldPos = mdlMatrix * vec4(inPosition, 1.0);
    varView = - TBN * vec3(worldPos / worldPos.w);
    varNormalGlobal = inNormal;
    varSnowLevel = inSnowLevel;

	varTexCoord = inTexCoord;

    height = inPosition.y;

	gl_Position = projMatrix * mdlMatrix * vec4(inPosition, 1.0);
}
