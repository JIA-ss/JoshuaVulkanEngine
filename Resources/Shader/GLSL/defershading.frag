#version 450

layout (set = 1, binding = 1) uniform sampler2D samplerposition;
layout (set = 1, binding = 2) uniform sampler2D samplerNormal;
layout (set = 1, binding = 3) uniform sampler2D samplerAlbedo;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragcolor;

layout(binding = 3) uniform CustomLargeLightUBO
{
	vec4 header;
	mat4 viewProjMatrix[100];
	vec4 position_near[100];
	vec4 color_far[100];
	vec4 direction[100];
} lightUbo;



layout(push_constant) uniform PushConsts {
	layout (offset = 0) int showDebugTarget;
} consts;

void main()
{
	// Get G-Buffer values
	vec4 fragPos4 = texture(samplerposition, inUV).rgba;
	vec3 fragPos = fragPos4.xyz / fragPos4.w;
	vec3 normal = texture(samplerNormal, inUV).rgb;
	vec4 albedo = texture(samplerAlbedo, inUV);

	// Debug display
	if (consts.showDebugTarget > 0) {
		switch (consts.showDebugTarget) {
			case 1:
				outFragcolor.rgb = fragPos;
				break;
			case 2:
				outFragcolor.rgb = normal;
				break;
			case 3:
				outFragcolor.rgb = albedo.rgb;
				break;
			case 4:
				outFragcolor.rgb = albedo.aaa;
				break;
		}
		outFragcolor.a = 1.0;
		return;
	}

	// Render-target composition

	float lightCount = lightUbo.header.x;
	#define ambient 0.1

	// Ambient part
	vec3 fragcolor  = albedo.rgb * ambient;

	for(int i = 0; i < lightCount; ++i)
	{
		// Vector to light
		vec3 L = lightUbo.position_near[i].xyz - fragPos;
		// Distance from light to fragment position
		float dist = length(L) / 3.0;

		// Viewer to fragment
		vec3 V = lightUbo.viewProjMatrix[i][3].xyz - fragPos;
		V = normalize(V);

		//if(dist < ubo.lights[i].radius)
		{
			// Light to fragment
			L = normalize(L);

			// Attenuation
			float atten = lightUbo.color_far[i].w / (pow(dist, 2.0) + 1.0);
			//atten = 1.0;

			// Diffuse part
			vec3 N = normalize(normal);
			float NdotL = max(0.0, dot(N, L));
			vec3 diff = lightUbo.color_far[i].rgb * albedo.rgb * NdotL * atten;

			// Specular part
			// Specular map values are stored in alpha of albedo mrt
			vec3 R = reflect(-L, N);
			float NdotR = max(0.0, dot(R, V));
			vec3 spec = lightUbo.color_far[i].rgb * albedo.a * pow(NdotR, 16.0) * atten;

			fragcolor += diff + spec;
		}
	}

    outFragcolor = vec4(fragcolor, 1.0);
}
