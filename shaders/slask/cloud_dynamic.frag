#version 150

in vec2 varTexCoord;

uniform sampler3D texUnit;
uniform float time;

out vec4 outColor;

void main(void)
{
    const float speed = 0.02;
    vec3 texCoord3D = vec3(varTexCoord, speed * time);
    outColor = texture(texUnit, texCoord3D);
}
