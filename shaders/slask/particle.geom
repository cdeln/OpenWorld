#version 330

uniform mat4 projMatrix;
uniform mat4 mdlMatrix;

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;
out vec2 varTexCoord;
out float varDist;

void main(void)
{

    const float radius = 0.1;
    vec4 pos = mdlMatrix * gl_in[0].gl_Position;
    varDist = length(pos);

    varTexCoord = vec2(-1,-1);
    gl_Position = projMatrix * (pos + radius * vec4(varTexCoord,0,0));
    EmitVertex();

    varTexCoord = vec2(-1,1);
    gl_Position = projMatrix * (pos + radius * vec4(varTexCoord,0,0));
    EmitVertex();

    varTexCoord = vec2(1,-1);
    gl_Position = projMatrix * (pos + radius * vec4(varTexCoord,0,0));
    EmitVertex();

    varTexCoord = vec2(1,1);
    gl_Position = projMatrix * (pos + radius * vec4(varTexCoord,0,0));
    EmitVertex();

    EndPrimitive();
}
