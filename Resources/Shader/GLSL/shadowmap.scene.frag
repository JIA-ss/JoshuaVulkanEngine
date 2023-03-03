#version 450

// SET1 CUSTOM5SAMPLER
layout(set = 1, binding = 1) uniform sampler2D diffuse;
layout(set = 1, binding = 2) uniform sampler2D specular;
layout(set = 1, binding = 3) uniform sampler2D ambient;
layout(set = 1, binding = 4) uniform sampler2D emissive;

// SET2 SHADOWMAP
layout(set = 2, binding = 1) uniform sampler2D shadowMap1;
layout(set = 2, binding = 2) uniform sampler2D shadowMap2;
layout(set = 2, binding = 3) uniform sampler2D shadowMap3;
layout(set = 2, binding = 4) uniform sampler2D shadowMap4;
layout(set = 2, binding = 5) uniform sampler2D shadowMap5;


layout(location = 0) in vec4 fragPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 worldNormal;
layout(location = 3) in vec3 camPos;

layout(location = 4) in float lightNum;
layout(location = 5) in vec4 shadowCoord[5];
layout(location = 10) in vec4 lightPosition[5];
layout(location = 15) in vec4 lightColor[5];
layout(location = 20) in vec2 lightNearFar[5];
layout(location = 25) in vec4 lightDirection[5];

layout(location = 30) in vec4 modelColor;

layout(location = 0) out vec4 outColor;

vec4 sampleShadowmap(int lightIdx, vec2 uv)
{
    if (lightIdx == 0)
    {
        return texture(shadowMap1, uv);
    }

    if (lightIdx == 1)
    {
        return texture(shadowMap2, uv);
    }

    if (lightIdx == 2)
    {
        return texture(shadowMap3, uv);
    }

    if (lightIdx == 3)
    {
        return texture(shadowMap4, uv);
    }

    if (lightIdx == 4)
    {
        return texture(shadowMap5, uv);
    }
}

vec4 getShadowMapCoord(int lightIdx)
{
    return shadowCoord[lightIdx] / shadowCoord[lightIdx].w;
}

float calculateDepthRecordedInShadowMap(int lightIdx)
{
    return sampleShadowmap(lightIdx, getShadowMapCoord(lightIdx).xy).r;
}

float isInShadow(int lightIdx)
{
    float shadow = 1.0;
    float depth = calculateDepthRecordedInShadowMap(lightIdx);
    vec3 lightDir = vec3(fragPosition.xyz) / fragPosition.w - lightPosition[lightIdx].xyz;
    if (dot(lightDir, lightDirection[lightIdx].xyz) < 0.0)
    {
        return 0.0;
    }

    // float dist = length(lightDir);
    // if (dist < lightNearFar[lightIdx].x || dist > lightNearFar[lightIdx].y)
    // {
    //     return 0.0;
    // }

    vec4 coord = getShadowMapCoord(lightIdx);
    if (depth < coord.z)
    {
        return 1.0;
    }
    return 0.0;
}

vec3 getColorInshadow(vec3 color, float isInshadow)
{
    if (lightNum == 0)
    {
        return color;
    }

    float coeffi = (0.1/lightNum - 1)*isInshadow + 1;
    return color * coeffi;
}

void main() {

    vec3 diffuseColor = texture(diffuse, fragTexCoord).rgb;
    vec3 specularTex = texture(specular, fragTexCoord).rgb;

    vec3 normal = normalize(worldNormal);
    vec3 viewDir = normalize(camPos - vec3(fragPosition.xyz)/fragPosition.w);

    float inShadowAvg = 0;
    float specularCoeffi = 0.5 / lightNum;
    vec3 specularColor = vec3(0);


    // vec4 coord = getShadowMapCoord(0);
    // float dist = length(fragPosition - lightPosition[0].xyz) / 100.0;
// 
    // outColor = vec4(sampleShadowmap(0, coord.xy).x, coord.z, dist, 1);
    // return;

    for (int lightIdx = 0; lightIdx < lightNum; lightIdx++)
    {
        float inShadow = isInShadow(lightIdx);
        if (inShadow == 0.0)
        {
            vec3 lightDir = normalize(lightPosition[lightIdx].xyz - vec3(fragPosition.xyz) / fragPosition.w);
            vec3 reflectDir = reflect(-lightDir, normal);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            specularColor += specularCoeffi * spec * specularTex;
        }
        inShadowAvg += inShadow;
    }
    inShadowAvg /= lightNum;

    vec3 finalColor = (getColorInshadow(diffuseColor, inShadowAvg) + specularColor);
    vec3 gammaOutput = pow(finalColor, vec3(1.0/2.2)) * modelColor.rgb;


    outColor = vec4(gammaOutput, 1.0);
    // outColor = vec4(inShadowAvg, 0,0,1);
}