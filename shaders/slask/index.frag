#version 150

flat in int instanceID;
flat in int isFaceBack;

//out vec4 outColor;
//out uvec4 outColor;
out int outColor;

void main(void)
{
    if( isFaceBack == 1)
        outColor = 0;
    else
        outColor = gl_PrimitiveID+1; // shift since background = 0
    /*
    outColor = uvec4(
            (gl_PrimitiveID) % 255,
            (2*gl_PrimitiveID) % 255,
            (4*gl_PrimitiveID) % 255,
            255);
            */
    /*
    outColor = vec4(
            (gl_PrimitiveID % 10) / 10.0,
            (gl_PrimitiveID & 100) / 100.0,
            (gl_PrimitiveID & 1000) / 1000.0,
            1); 
    */
}
