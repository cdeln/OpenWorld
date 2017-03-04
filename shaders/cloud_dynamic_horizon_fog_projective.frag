#version 150

in vec2 varTexCoord;
in float fogLevel;

uniform sampler3D texUnit;
uniform float time;

out vec4 outColor;

void main(void)
{
    const float speed = 0.02;
    const vec4 horizonColor = vec4(1,1,1,0.1);
    const float horizonFactor = 0.75;
    vec3 texCoord3D = vec3(varTexCoord, speed * time);
    outColor = texture(texUnit, texCoord3D);
    outColor = mix(outColor, horizonColor, horizonFactor*fogLevel*fogLevel);
}
