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
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBittangent;



layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outColor;
layout (location = 3) out vec4 outWorldPos;
layout (location = 4) out vec3 outTangent;

void main()
{

    outWorldPos = modelUbo.model * vec4(inPosition, 1.0);
    gl_Position = camUbo.proj * camUbo.view * outWorldPos;
	outUV = inTexCoord;


    mat3 mNormal = transpose(inverse(mat3(modelUbo.model)));
	outNormal = mNormal * normalize(inNormal);
	outTangent = mNormal * normalize(inTangent);
	outColor = modelUbo.color.rgb;
}
