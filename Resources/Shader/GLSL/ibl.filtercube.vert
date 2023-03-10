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


layout(push_constant) uniform PushConsts {
	layout (offset = 0) mat4 model;
} consts;


layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 modelPosition;

void main()
{
	modelPosition = inPosition;
	gl_Position = camUbo.proj /* camUbo.view*/ * consts.model * vec4(inPosition.xyz, 1.0);
    gl_Position.y = -gl_Position.y;
}
