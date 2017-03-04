#version 150

in vec3 inPosition;
in vec3 inNormal;
in vec3 inTangent;
in vec3 inBitangent;

uniform mat4 mdlMatrix;
uniform mat4 projMatrix;

out Vertex
{
    vec4 normal;
    vec4 tangent;
    vec4 bitangent;
} vertex;

void main(void)
{
    gl_Position = vec4(inPosition, 1.0);
    vertex.normal = vec4(inNormal, 1.0);
    vertex.tangent = vec4(inTangent, 1.0);
    vertex.bitangent = vec4(inBitangent, 1.0);
}
