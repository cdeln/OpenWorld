#version 150

in vec2 varTexCoord;

uniform sampler2D texUnit;

out vec4 outColor;

void main(void)
{
    outColor = texture(texUnit, varTexCoord);
}
