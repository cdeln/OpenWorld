#version 420

in vec2 varTexCoord;
in vec3 varLight;
in vec3 varView;
in vec3 varNormalGlobal;
in vec3 varPosition;
in float height;
in float snowLevel;

out vec4 outColor;

uniform sampler2D texUnitRock0;
uniform sampler2D texUnitRock1;
uniform sampler2D texUnitRock2;
uniform sampler2D texUnitRockSlope;
uniform sampler2D texUnitSnow;
uniform sampler2D texUnitGloss;
uniform sampler2D texUnitNormal;

uniform float rockTransLevel[4];
uniform vec3 sunDir;

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

    const float k_a = 0.1;
    const float k_d = 0.9;
    const float k_s = 1.0;
    const float k_g = 0.1;
    const float shadeDivFactor = k_a + k_d + (k_g + 1.0)*k_s;
    const float shadeAmplMultFactor = 5.0 / shadeDivFactor;
    const float shadeAmplDivFactor = log(1.0 + shadeAmplMultFactor);

    vec3 normal = normalize(texture(texUnitNormal, varTexCoord).rgb * 2.0 - 1.0);
    vec3 view = normalize(varView);
    vec3 light = normalize(varLight);

    vec3 reflection = 2 * dot(normal, light) * normal - light;

    float shadeDiffuse = k_d * clamp(dot(normal, light), 0, 1);
    float shadeSpecular = k_s * pow(clamp(dot(view, reflection),0,1), 2);

    vec4 texColorRock0 = texture(texUnitRock0, varTexCoord);
    vec4 texColorRock1 = texture(texUnitRock1, varTexCoord);
    vec4 texColorRock2 = texture(texUnitRock2, varTexCoord);
    vec4 texColorRockSlope = texture(texUnitRockSlope, varTexCoord);
    vec4 texColorSnow = texture(texUnitSnow, varTexCoord);
    vec4 texGloss = texture(texUnitGloss, varTexCoord);

    float rockLevel0;
    float rockLevel1;
    float rockLevel2;
    Ramp(height, rockTransLevel[0], rockTransLevel[1], rockLevel0);
    rockLevel0 = 1 - rockLevel0;
    Plat(height, rockTransLevel[0], rockTransLevel[1], rockTransLevel[2], rockTransLevel[3], rockLevel1);
    Ramp(height, rockTransLevel[2], rockTransLevel[3], rockLevel2);
    float rockLevelSum = rockLevel0 + rockLevel1 + rockLevel2;
    rockLevel0 /= rockLevelSum;
    rockLevel1 /= rockLevelSum;
    rockLevel2 /= rockLevelSum;

    //float rockSlopeLevel = pow(max(0, acos(varNormalGlobal.y) / (PI)), 2);
    float rockSlopeLevel = 0;
    //ivec2 posTexCoord = ivec2(int(varPosition.x), int(varPosition.z)); 
    //vec4 snowLevelVec = imageLoad(texUnitSnowLevel, posTexCoord); //texture(texUnitSnowLevel, posTexCoord);
    //float snowLevel = snowLevelVec.r;
    float glossFactor = snowLevel * (k_g + texGloss.r);

    float shade = k_a + shadeDiffuse + glossFactor*shadeSpecular;
    shade *= clamp(cos(acos(sunDir.y)/2),0,1);

    //shade = shadeAmplMultFactor * shade; 
    vec4 texColorRock = rockLevel0*texColorRock0 + rockLevel1*texColorRock1 + rockLevel2*texColorRock2;
    texColorRock = rockSlopeLevel * texColorRockSlope + (1-rockSlopeLevel)*texColorRock;
    vec4 texColor = shade * ((1-snowLevel)*texColorRock + snowLevel*texColorSnow);
    outColor = clamp(texColor, 0, 1);
    //vec4 fogColor = vec4(1.0); 
    //outColor = clamp(log(1.0 + outColor) / shadeAmplDivFactor, 0, 1);
}
