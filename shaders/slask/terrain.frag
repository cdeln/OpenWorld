#version 150

in vec2 varTexCoord;
in vec3 varLight;
in vec3 varView;
in float height;
in float varSnowLevel;

out vec4 outColor;

uniform sampler2D texUnit0;
uniform sampler2D texUnit1;
uniform sampler2D texUnitGloss;
uniform sampler2D texUnitNormal;
//uniform sampler2D texUnitDirt;

void main(void)
{

    const float k_a = 0.2;
    const float k_d = 0.4;
    const float k_s = 0.8;
    const float k_g = 0.5;
    const float shadeDivFactor = k_a + k_d + (k_g + 1.0)*k_s;
    const float shadeAmplMultFactor = 5.0 / shadeDivFactor;
    const float shadeAmplDivFactor = log(1.0 + shadeAmplMultFactor);
    const float levelFalloff = 10.0;

    float dist = length(varView);
    float fog = log(1.0 + dist);

    vec3 normal = normalize(texture(texUnitNormal, varTexCoord).rgb * 2.0 - 1.0);
    vec3 view = normalize(varView);
    vec3 light = normalize(varLight);

    vec3 reflection = 2 * dot(normal, light) * normal - light;

    float shadeDiffuse = k_d * clamp(dot(normal, light), 0, 1);
    float shadeSpecular = k_s * pow(clamp(dot(view, reflection),0,1), 2);

    vec4 texColor0 = texture(texUnit0, varTexCoord);
    vec4 texColor1 = texture(texUnit1, varTexCoord);
    vec4 gloss = texture(texUnitGloss, varTexCoord);
    float grassLevel = 1.0 - varSnowLevel; //clamp((40 - height)/levelFalloff, 0, 1);
    float snowLevel = varSnowLevel; //clamp((height - 20)/levelFalloff, 0, 1);
    float glossFactor = k_g + varSnowLevel * gloss.x;

    float shade = k_a + shadeDiffuse + glossFactor*shadeSpecular;
    shade = shadeAmplMultFactor * shade; 
    outColor = shade * (grassLevel * texColor0 + snowLevel * texColor1);
    outColor = clamp(log(1.0 + outColor) / shadeAmplDivFactor, 0, 1);
}
