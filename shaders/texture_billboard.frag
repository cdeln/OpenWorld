#version 150

in vec2 varTexCoord;

out vec4 outColor;

uniform sampler2D tex;

void main(void)
{
    outColor = texture(tex, varTexCoord);
}
