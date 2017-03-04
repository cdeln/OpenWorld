#version 330

layout(location = 0) out float depth;
//layout(location = 0) out vec4 depth;

void main(void)
{
    //depth = vec4(gl_FragCoord.z);
    depth = gl_FragCoord.z; 
}
