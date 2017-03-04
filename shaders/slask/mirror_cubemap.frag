#version 150

in vec3 varPosition;
in vec3 varNormal;

out vec4 outColor;

uniform samplerCube texUnit;
uniform vec3 camPosition;

void main(void)
{
    
    vec3 normal = normalize(varNormal);
    vec3 light = normalize(camPosition - varPosition);
    vec3 reflection = normalize(2 * dot(normal, light) * normal - light);
    float x = reflection.x;
    float y = reflection.y;
    float z = reflection.z;
    float m = max(max(abs(x),abs(y)),abs(z));
    reflection /= m;
    //vec3 reflection = reflect(normalize(varPosition - camPosition), normalize(varNormal));
    outColor = texture(texUnit,reflection);
}
