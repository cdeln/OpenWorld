#version 150

in vec2 varTexCoord;

out vec4 outColor;

uniform sampler2D texUnit;
uniform vec2 camTexCoord;

void main(void)
{
    //vec2 texCoord = vec2(1.0 - varTexCoord.t, 1.0 - varTexCoord.s);
    const vec2 offset = vec2(0.5,0.5);
    vec2 texCoord = varTexCoord - camTexCoord + offset;
    outColor = texture(texUnit,texCoord);
}
