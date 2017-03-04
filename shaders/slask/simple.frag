#version 150

in vec3 varNormal;
in vec3 varLight;

out vec4 outColor;

void main(void)
{
    vec3 normal = normalize(varNormal);
    vec3 light = normalize(varLight);
    float shade = clamp(dot(normal,light),0,1);

    outColor = vec4(shade);
}
