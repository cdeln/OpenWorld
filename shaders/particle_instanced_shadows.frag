#version 330

in float varDist;
in float visibility;
in vec2 varTexCoord;

layout(location = 0) out vec4 outColor;

void main(void)
{

    const float halfRange = 25000.0;
    const float cutoffDist = 4*halfRange;
    const float k_a = 0.35;
    const float shadowFactor = 0.8;
    
    if( varDist > cutoffDist )
        discard;
    else
    {
        float shade = exp( - varDist / halfRange);
        shade = clamp(1.0 - dot(varTexCoord,varTexCoord), 0, 1);
        shade /= 2;
        vec4 preColor = vec4(1); 
        outColor = ((1-shadowFactor) + shadowFactor*visibility) * preColor;
        outColor += k_a * preColor;
        outColor.a = shade;
    }
}
