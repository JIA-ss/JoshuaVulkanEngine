#version 450

layout (set = 1, binding = 1) uniform samplerCube samplerCubeMap;

// SET2 IBL
layout(set = 2, binding = 1) uniform samplerCube irradianceTex;
layout(set = 2, binding = 2) uniform samplerCube preFilterTex;
layout(set = 2, binding = 3) uniform samplerCube brdfLUT;


layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outFragColor;


layout(push_constant) uniform PushConsts {
	layout (offset = 0) float showIrradiance;
} consts;


void main()
{
    vec3 color;

    if (consts.showIrradiance == 1.0)
    {
        color = texture(irradianceTex, inUVW).rgb;
    }
    else
    {
        color = texture(samplerCubeMap, inUVW).rgb;
    }
    // color = pow(color, vec3(1.0/2.2));
	outFragColor = vec4(color, 1);
}