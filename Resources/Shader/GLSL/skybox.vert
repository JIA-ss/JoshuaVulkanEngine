#version 450

layout(binding = 0) uniform CameraUniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 model;
    vec4 camPos;
} camUbo;

layout(binding = 2) uniform ModelUniformBufferObject
{
    mat4 model;
    vec4 color;
} modelUbo;

layout(location = 0) in vec3 inPosition;


layout (location = 0) out vec3 outUVW;

void main() {
	outUVW = inPosition;
	// Convert cubemap coordinates into Vulkan coordinate space
	// outUVW.y *= -1.0;

    gl_Position = camUbo.proj * vec4( mat3(camUbo.view) * inPosition, 1.0) ;
}
