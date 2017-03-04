#version 150

in vec3 varNormal;
in vec3 varDirection;

out vec4 outColor;

void main(void)
{
    vec3 normal = normalize(varNormal);
    vec3 dir = normalize(varDirection);
    float shade = clamp(dot(normal,dir),0,1);
    shade = pow(shade,25);
    outColor = vec4(shade,0,shade,1.0);
}
