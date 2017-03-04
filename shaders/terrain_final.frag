#version 330

in vec2 varTexCoord;
in vec4 varShadowCoord;
in vec3 varLight;
in vec3 varView;
in vec3 varNormal;
in float height;
in float snowLevel;
in float shadowLevel;

out vec4 outColor;

uniform sampler2D texUnitRock0;
uniform sampler2D texUnitRock1;
uniform sampler2D texUnitRock2;
//uniform sampler2D texUnitRockSlope;
uniform sampler2D texUnitSnow;
uniform sampler2D texUnitGloss;
uniform sampler2D texUnitNormal;

uniform float rockTransLevel[4];
uniform vec3 sunDir;
uniform bool isFlipped;
uniform bool blinnPhong;

uniform bool enableNormalMapping;
uniform bool enableGlossMapping;

#define PI 3.14

void Ramp(in float val, in float beg, in float end, out float ret)
{
    if( val <= beg )
        ret = 0;
    else if( val <= end )
        ret = (val-beg)/(end-beg);
    else
        ret = 1;
}

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

float beckmann(vec3 halfVector, vec3 normal)
{
    float alpha = acos(dot(halfVector,normal));
    return exp(-pow(tan(alpha),2)) / (PI * pow(cos(alpha),4)); 
}

float cookTorrance(vec3 view, vec3 light, vec3 normal)
{
    vec3 halfVector = normalize(view + light);
    float NH = dot(normal, halfVector);
    float NV = dot(normal, view);
    float NL = dot(normal, light);
    float VH = dot(view, halfVector);
    float G = min(1, min( 2*NH*NV/VH, 2*NH*NL/VH  ));
    float D = beckmann(halfVector,  normal);
    float F = 1;
    return F*D*G*(4*NV*NL);
}

void main(void)
{

    const float k_a = 0.1;
    const float k_d = 0.75;
    const float k_s = 0.75;
    const float k_g = 0;
    const float waterHalfRange = 5.0f;
    const float specExp = 4;

    vec3 normal;
    if( enableNormalMapping )
    {
        normal = normalize(texture(texUnitNormal, varTexCoord).rgb * 2.0 - 1.0);
    }
    else
    {
        normal = varNormal;
    }
    vec3 view = normalize(varView);
    vec3 light = normalize(varLight);

    vec3 reflection = 2 * dot(normal, light) * normal - light;

    float shadeDiffuse = clamp(dot(normal, light), 0, 1);
    float shadeSpecular = 0;
    if(blinnPhong)
    {
        vec3 halfVector = normalize(view + light);
        shadeSpecular = pow(clamp(dot(normal,halfVector),0,1), specExp); 
        //float sign = dot(normal,light);
        //if(sign < 0)
        //    shadeSpecular = 0;
        shadeSpecular *= clamp(dot(normal,light),0,1);
    }
    else
    {
        shadeSpecular = pow(clamp(dot(view, reflection),0,1), specExp/4); 
    }

    vec4 texColorRock0 = texture(texUnitRock0, varTexCoord);
    vec4 texColorRock1 = texture(texUnitRock1, varTexCoord);
    vec4 texColorRock2 = texture(texUnitRock2, varTexCoord);
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

    float glossFactor = snowLevel; 
    if( enableGlossMapping )
    {
        glossFactor *= (k_g + (1 - k_g)*texGloss.r);
    }

    float shade = k_a + k_d * shadeDiffuse + k_s * glossFactor * shadeSpecular;
    shade *= clamp(cos(acos(sunDir.y)/2),0,1); // light intensity

    vec4 texColorRock = rockLevel0*texColorRock0 + rockLevel1*texColorRock1 + rockLevel2*texColorRock2;
    //texColorRock = rockSlopeLevel * texColorRockSlope + (1-rockSlopeLevel)*texColorRock;
    vec4 texColor = shade * ((1-snowLevel)*texColorRock + snowLevel*texColorSnow);
    outColor = clamp(texColor, 0, 1);
    if( height < 0 )
    {
        if( isFlipped )
            discard;
        else
        {
            outColor = clamp(exp(log(2.0)*height/waterHalfRange),0,1) * outColor; // Exponential model
            //outColor = clamp(1.0 + height / (2*waterHalfRange), 0, 1) *  outColor; // Linear model
        }
    }

    vec4 tmp = outColor;
    outColor *= shadowLevel;
    outColor += k_a * tmp;
    outColor.a = 1;
}
