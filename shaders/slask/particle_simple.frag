#version 330

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
        float shade = exp( - varDist / halfRange);
        outColor = vec4(1.0, 1.0, 1.0, shade); 
    }
}
