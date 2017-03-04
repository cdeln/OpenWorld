#version 150

// http://www.geeks3d.com/20130905/exploring-glsl-normal-visualizer-with-geometry-shaders-shader-library/
// https://open.gl/geometry

layout(points) in;
layout(line_strip, max_vertices = 6) out;

/*
in gl_PerVertex
{
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
} gl_in[];
*/

in Vertex
{
    vec4 normal; 
    vec4 tangent;
    vec4 bitangent;
} vertex[];

out vec4 varColor;

uniform mat4 mdlMatrix;
uniform mat4 projMatrix;

void main(void)
{

    vec3 pos = gl_in[0].gl_Position.xyz;
    vec3 normal = vertex[0].normal.xyz;
    vec3 tangent = vertex[0].tangent.xyz;
    vec3 bitangent = vertex[0].bitangent.xyz;

    float dist = length(mdlMatrix * vec4(pos,1.0));
    float falloff = exp( - dist / 100.0);

    varColor = vec4(1,0,0,falloff);
    gl_Position = projMatrix * mdlMatrix * vec4(pos, 1.0);
    EmitVertex(); 
    gl_Position = projMatrix * mdlMatrix * vec4(pos + normal, 1.0);
    EmitVertex();

    varColor = vec4(0,1,0,falloff);
    gl_Position = projMatrix * mdlMatrix * vec4(pos, 1.0);
    EmitVertex(); 
    gl_Position = projMatrix * mdlMatrix * vec4(pos + tangent, 1.0);
    EmitVertex();

    varColor = vec4(0,0,1,falloff);
    gl_Position = projMatrix * mdlMatrix * vec4(pos, 1.0);
    EmitVertex(); 
    gl_Position = projMatrix * mdlMatrix * vec4(pos + bitangent, 1.0);
    EmitVertex();

    EndPrimitive();
}
