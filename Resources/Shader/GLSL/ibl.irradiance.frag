// Generates an irradiance cube from an environment map using convolution

#version 450

layout (location = 0) in vec3 modelPosition;

layout (location = 0) out vec4 outColor;

layout (set = 1, binding = 1) uniform samplerCube skyboxCubeMap;


#define PI                  3.1415926535897932384626433832795

layout(push_constant) uniform PushConsts {
	layout (offset = 64) float deltaPhi;
    layout (offset = 68) float deltaTheta;
} consts;


void main()
{
	vec3 worldNormal = normalize(modelPosition);
	vec3 worldUp = vec3(0.0, 1.0, 0.0);
	vec3 worldRight = normalize(cross(worldUp, worldNormal));
	worldUp = cross(worldNormal, worldRight);

	const float TWO_PI = PI * 2.0;
	const float HALF_PI = PI * 0.5;

	vec3 irradiance = vec3(0.0);
	uint sampleCount = 0u;

    // phi from 0 to 2PI
    for (float phi = 0.0; phi < TWO_PI; phi += consts.deltaPhi)
    {
        // theta from 0 to 0.5PI
		for (float theta = 0.0; theta < HALF_PI; theta += consts.deltaTheta)
        {
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);

            // from sphere coord to cartesian coordinate system (!!! Tangent Space !!!)
            vec3 tangentSpaceCoord = vec3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);

            // tangent space to world space
            vec3 sampleVec = tangentSpaceCoord.x * worldRight + tangentSpaceCoord.y * worldUp + tangentSpaceCoord.z * worldNormal;

			irradiance += texture(skyboxCubeMap, sampleVec).rgb * cosTheta * sinTheta;
			sampleCount++;
		}
	}

	outColor = vec4(PI * irradiance / float(sampleCount), 1.0);

	outColor = texture(skyboxCubeMap, modelPosition);
}
