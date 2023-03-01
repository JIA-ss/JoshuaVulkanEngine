#version 450

layout(binding = 0) uniform CameraUniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 camPos;
} camUbo;

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


void main() {
    vec4 worldPos = modelUbo.model * vec4(inPosition, 1.0);
    gl_Position = camUbo.proj * camUbo.view * worldPos;
}