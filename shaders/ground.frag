#version 150

in vec2 varTexCoord;
in float varLight;

out vec4 outColor;

uniform sampler2D texUnit;
uniform float texScale;

void main(void)
{
    outColor = max(1,varLight) * texture(texUnit, texScale*varTexCoord);
}
