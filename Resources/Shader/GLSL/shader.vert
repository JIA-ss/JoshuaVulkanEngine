#version 450

layout(binding = 0) uniform CameraUniformBufferObject {
    mat4 view;
    mat4 proj;
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

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 camPos;
layout(location = 3) out vec3 lightPos;
layout(location = 4) out vec3 worldNormal;

void main() {
    vec4 worldPos = modelUbo.model * vec4(inPosition, 1.0);
    gl_Position = camUbo.proj * camUbo.view * worldPos;

    fragPosition = vec3(worldPos) / worldPos.z;

    fragTexCoord = inTexCoord;

    camPos = camUbo.camPos.xyz;
    lightPos = lightUbo.position[0].xyz;
    worldNormal = mat3(transpose(inverse(modelUbo.model))) * inNormal;
}