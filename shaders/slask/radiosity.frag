#version 150

in vec3 varNormal;
in vec3 varLight;

out vec4 outColor;

void main(void)
{
    vec3 normal = normalize(varNormal);
    float shade = clamp(dot(normal,varLight),0,1);

    outColor = vec4(shade);
}
