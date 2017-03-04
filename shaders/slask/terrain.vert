#version 150

in vec3 inPosition;
in vec3 inTangent;
in vec3 inBitangent;
in vec3 inNormal;
in vec2 inTexCoord;
in float inSnowLevel;

out vec2 varTexCoord;
out vec3 varLight;
out vec3 varView;
out float height;
out float varSnowLevel;

uniform mat4 projMatrix;
uniform mat4 mdlMatrix;

void main(void)
{
    const vec3 lightSource = normalize(vec3(10,1,0));
	mat3 normalMatrix = mat3(mdlMatrix);

    vec3 tangent = normalMatrix * normalize(inTangent);
    vec3 bitangent = normalMatrix * normalize(inBitangent); 
    vec3 normal = normalMatrix * normalize(inNormal);
    mat3 TBN = transpose(mat3(tangent, bitangent, normal));
    varLight = TBN * normalMatrix * lightSource;
    vec4 worldPos = mdlMatrix * vec4(inPosition, 1.0);
    varView = - TBN * vec3(worldPos / worldPos.w);
    varSnowLevel = inSnowLevel;

	varTexCoord = inTexCoord;

    height = inPosition.y;

	gl_Position = projMatrix * mdlMatrix * vec4(inPosition, 1.0);
}
