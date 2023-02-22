#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 camPos;
    vec3 lightPos;
} ubo;

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
    vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;

    fragPosition = vec3(worldPos) / worldPos.z;

    fragTexCoord = inTexCoord;

    camPos = ubo.camPos;
    lightPos = ubo.lightPos;
    worldNormal = mat3(transpose(inverse(ubo.model))) * inNormal;
}