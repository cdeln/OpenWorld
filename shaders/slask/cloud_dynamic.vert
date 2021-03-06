#version 150

in vec3 inPosition;
in vec2 inTexCoord;

uniform mat4 projMatrix;
uniform mat4 mdlMatrix;
uniform float time;

out vec2 varTexCoord;

void main(void)
{
	gl_Position = projMatrix * vec4(mat3(mdlMatrix) * inPosition, 1.0);
    //gl_Position = projMatrix * mdlMatrix * vec4(inPosition, 1.0);
    const vec2 wind_vel = vec2(0.01,0);
    varTexCoord = inTexCoord + time*wind_vel;
}
