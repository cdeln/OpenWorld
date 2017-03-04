#version 150

out vec4 outColor;

uniform float alpha;

void main(void)
{
    outColor = vec4(0, 0, 0, alpha);
}
