#version 150

in vec2 varTexCoord;
in vec3 varView;
in vec3 varNormalGlobal;
in float height;
in float varSnowLevel;
in vec3 varLight[6];

out vec4 outColor;

uniform sampler2D texUnitRock0;
uniform sampler2D texUnitRock1;
uniform sampler2D texUnitRock2;
uniform sampler2D texUnitRockSlope;
uniform sampler2D texUnitSnow;
uniform sampler2D texUnitGloss;
uniform sampler2D texUnitNormal;

uniform float lightAmpl[6];

#define PI 3.14

void Ramp(in float val, in float beg, in float end, out float ret)
{
    if( val <= beg )
        ret = 0;
    else if( val <= end )
        ret = (val-beg)/(end-beg);
    else
        ret = 1;
};

void Plat(in float val, in float beg1, in float end1, in float beg2, in float end2, out float ret)
{
    if( val <= beg1 )
        ret = 0;
    else if( val <= end1 )
        ret = (val-beg1)/(end1-beg1);
    else if( end1 < val && val <= beg2 )
        ret = 1;
    else if( val <= end2 )
        ret = (end2-val)/(end2-beg2);
    else 
        ret = 0;
}

void main(void)
{

    const float k_a = 0;
    const float k_d = 1;
    const float k_s = 1;
    const float k_g = 0.1;
    const float shadeDivFactor = k_a + k_d + (k_g + 1.0)*k_s;
    const float shadeAmplMultFactor = 5.0 / shadeDivFactor;
    const float shadeAmplDivFactor = log(1.0 + shadeAmplMultFactor);
    const float levelFalloff = 10.0;
    const float fogFalloff = 300.0;
    const float rockTransLevel0 = -100;
    const float rockTransLevel1 = -50;
    const float rockTransLevel2 = 0;
    const float rockTransLevel3 = 50;

    float dist = length(varView);
    float fogLevel = min(dist / fogFalloff, 1);

    vec3 normal = normalize(texture(texUnitNormal, varTexCoord).rgb * 2.0 - 1.0);
    vec3 view = normalize(varView);

    vec3 light[6];
    for(int i = 0; i < 6; ++i)
        light[i] = normalize(varLight[i]);

    /*
    float shadeDiffuse = 0;
    float shadeSpecular = 0;
    for(int i = 0; i < 6; ++i)
    {
        vec3 reflection = 2 * dot(normal, light[i]) * normal - light[i];
        shadeDiffuse += lightAmpl[i] * k_d * clamp(dot(normal, light[i]), 0, 1);
        shadeSpecular += lightAmpl[i] * k_s * pow(clamp(dot(view, reflection),0,1), 2);
    }
    shadeDiffuse /= 1;
    shadeSpecular /= 1;
    */

    float angle = acos(dot(normal, light[0]));
    float shadeDiffuse = k_d * cos(angle/2) * cos(angle/2);
    vec3 reflection = 2 * dot(normal, light[0]) * normal - light[0];
    float shadeSpecular = k_s * pow(clamp(dot(view, reflection),0,1), 2);

    vec4 texColorRock0 = texture(texUnitRock0, varTexCoord);
    vec4 texColorRock1 = texture(texUnitRock1, varTexCoord);
    vec4 texColorRock2 = texture(texUnitRock2, varTexCoord);
    vec4 texColorRockSlope = texture(texUnitRockSlope, varTexCoord);
    vec4 texColorSnow = texture(texUnitSnow, varTexCoord);
    vec4 texGloss = texture(texUnitGloss, varTexCoord);

    float rockLevel0;;
    float rockLevel1;;
    float rockLevel2;;
    Ramp(height, rockTransLevel0, rockTransLevel1, rockLevel0);
    rockLevel0 = 1 - rockLevel0;
    Plat(height, rockTransLevel0, rockTransLevel1, rockTransLevel2, rockTransLevel3, rockLevel1);
    Ramp(height, rockTransLevel2, rockTransLevel3, rockLevel2);
    float rockLevelSum = rockLevel0 + rockLevel1 + rockLevel2;
    rockLevel0 /= rockLevelSum;
    rockLevel1 /= rockLevelSum;
    rockLevel2 /= rockLevelSum;

    //float rockSlopeLevel = pow(max(0, acos(varNormalGlobal.y) / (PI)), 2);
    float rockSlopeLevel = 0;
    float snowLevel = varSnowLevel;
    float glossFactor = varSnowLevel * (k_g + texGloss.r);

    float shade = k_a + shadeDiffuse + glossFactor*shadeSpecular;

    //shade = shadeAmplMultFactor * shade; 
    vec4 texColorRock = rockLevel0*texColorRock0 + rockLevel1*texColorRock1 + rockLevel2*texColorRock2;
    texColorRock = rockSlopeLevel * texColorRockSlope + (1-rockSlopeLevel)*texColorRock;
    vec4 texColor = shade * ((1-snowLevel)*texColorRock + snowLevel*texColorSnow);
    outColor = clamp(texColor, 0, 1);
    //vec4 fogColor = vec4(1.0); 
    //outColor = clamp(log(1.0 + outColor) / shadeAmplDivFactor, 0, 1);
}
