#version 450

layout(binding = 0) uniform CameraUniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 model;
    vec4 camPos;
} camUbo;

layout(binding = 1) uniform LightInforUniformBufferObject {
    mat4 viewProjMatrix[5];
    vec4 direction[5];
    vec4 position[5];
    vec4 color[5];
    vec4 nearFar[5];
    float lightNum;
} lightUbo;

layout(binding = 2) uniform ModelUniformBufferObject
{
    mat4 model;
    vec4 color;
} modelUbo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBittangent;

layout(location = 0) out vec4 fragPosition;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 worldNormal;
layout(location = 3) out vec3 camPos;

layout(location = 4) out float lightNum;
layout(location = 5) out vec4 shadowCoord[5];
layout(location = 10) out vec4 lightPosition[5];
layout(location = 15) out vec4 lightColor[5];
layout(location = 20) out vec2 lightNearFar[5];
layout(location = 25) out vec4 lightDirection[5];

layout(location = 30) out vec4 modelColor;

layout(location = 31) out mat3 TBNMatrix;

// ????
const mat4 biasMat = mat4(
    // 1 column
	0.5, 0.0, 0.0, 0.0,
    // 2 column
	0.0, 0.5, 0.0, 0.0,
    // 3 column
	0.0, 0.0, 1.0, 0.0,
    // 4 column
	0.5, 0.5, 0.0, 1.0
);



void main() {
    vec4 worldPos = modelUbo.model * vec4(inPosition, 1.0);
    gl_Position = camUbo.proj * camUbo.view * worldPos;
    fragPosition = worldPos;
    fragTexCoord = inTexCoord;

    camPos = camUbo.camPos.xyz;
    lightNum = lightUbo.lightNum;


    mat3 normalMatrix = transpose(inverse(mat3(modelUbo.model)));
    vec3 T = normalize(normalMatrix * inTangent);
    vec3 B = normalize(normalMatrix * inBittangent);
    vec3 N = normalize(normalMatrix * inNormal);
    TBNMatrix = transpose(mat3(T, B, N));

    for (int lightIdx = 0; lightIdx < lightNum; lightIdx++)
    {
        shadowCoord[lightIdx] = biasMat * lightUbo.viewProjMatrix[lightIdx] * worldPos;
        lightNearFar[lightIdx] = lightUbo.nearFar[lightIdx].xy;
        lightPosition[lightIdx] = lightUbo.position[lightIdx];
        lightColor[lightIdx] = lightUbo.color[lightIdx];
        lightDirection[lightIdx] = lightUbo.direction[lightIdx];
    }

    worldNormal = N;
    modelColor = modelUbo.color;
}