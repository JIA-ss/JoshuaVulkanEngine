#version 450


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

layout(location = 0) out vec2 fragTexCoord;
void main() {
    fragTexCoord = inTexCoord;
    vec4 p = modelUbo.model * vec4(inPosition, 1.0);
    p = p / p.w;
    gl_Position = p;
}