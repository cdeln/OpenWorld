#version 330

in vec2 varTexCoord;
in float varDist;

layout(location = 0) out vec4 outColor;

void main(void)
{

    const float halfRange = 250.0;
    const float cutoffDist = 4*halfRange;

    if( varDist > cutoffDist )
        discard;
    else
    {
        vec2 texCoord = varTexCoord; 
        float falloff = exp( - varDist / halfRange);
        float shade = falloff * clamp(1.0 - dot(texCoord, texCoord),0,1);

        outColor = vec4(1.0, 1.0, 1.0, shade); 
    }
}
