#version 150

in vec2 varTexCoord;
in float sunAngle;

out vec4 outColor;

#define PI 3.14

void main(void)
{
    float t = clamp(0.5*sunAngle/PI,0,1);
    t = 1 - pow(1-t,6);
    float v0 = clamp(pow(1-t,5),0,1);
    float v1 = clamp(5*pow(t,1)*pow(1-t,4),0,1);
    float v2 = clamp(10*pow(t,2)*pow(1-t,3),0,1);
    float v3 = clamp(10*pow(t,3)*pow(1-t,2),0,1);
    float v4 = clamp(5*pow(t,4)*pow(1-t,1),0,1);
    float v5 = clamp(pow(t,5),0,1);

    vec3 c0 = vec3(1,1,1);
    vec3 c1 = vec3(1,1,0);
    vec3 c2 = vec3(126,192,238)/255; // sky blue
    vec3 c3 = c2; 
    vec3 c4 = c3;
    vec3 c5 = vec3(0.2,0.2,0.8);
    vec3 col = v0*c0 + v1*c1 + v2*c2 + v3*c3 + v4*c4 + v5*c5;
    outColor = vec4(col,1);
}
