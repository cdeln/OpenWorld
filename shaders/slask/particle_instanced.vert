#version 330

layout(location = 0) in vec3 inPosition;
layout(location = 2) in vec3 inVertex;
layout(location = 3) in vec2 inTexCoord;

out vec2 varTexCoord;
out float varDist;

uniform mat4 projMatrix;
uniform mat4 mdlMatrix;

void main(void)
{

    const vec2 offset = vec2(-0.5,-0.5);

    vec4 pos = mdlMatrix * vec4(inPosition,1);
    gl_Position = projMatrix * (pos + vec4(inVertex,0));
    varTexCoord = inTexCoord; //2*(vec2(gl_VertexID/2,gl_VertexID%2) + offset);
    varDist = gl_Position.w;
}
