#version 450

// SET1 CUSTOM5SAMPLER
layout(set = 1, binding = 1) uniform sampler2D albedoTex;
layout(set = 1, binding = 2) uniform sampler2D metallicTex;
layout(set = 1, binding = 3) uniform sampler2D roughnessTex;
layout(set = 1, binding = 4) uniform sampler2D normalTex;
layout(set = 1, binding = 5) uniform sampler2D aoTex;

// SET2 IBL
layout(set = 2, binding = 1) uniform samplerCube irradianceTex;
layout(set = 2, binding = 2) uniform samplerCube preFilterTex;
layout(set = 2, binding = 3) uniform sampler2D brdfLUT;


layout(location = 12) in vec4 fragPosition;
layout(location = 13) in vec2 fragTexCoord;
layout(location = 14) in vec3 modelNormal;
layout(location = 15) in vec3 camPos;

layout(location = 16) in float lightNum;
layout(location = 17) in vec4 lightPosition[5];
layout(location = 22) in vec4 lightColor[5];

layout (location = 0) out vec4 outColor;




layout(push_constant) uniform PushConsts {
	layout (offset = 0) float useIbl;
} consts;
/*

Energy:                 Q
Radiant Flux(Power):    Phi = dQ/dt
Intensity:              I = dPhi/dw
Irrandiance:            E = dPhi/(dA · cos(theta))
Radiance:               L = dE/dw = d2Phi/dwdAcos(theta)

Render Equation:
Lo(p,wo) = integral( BRDF(p,wi,wo) * Li(p,wi) * n·wi dwi )

BRDF:
BRDF(wi,wo) = dLo(wo) / dE(wi)

Cook-Torrance BRDF:
f = k_d * f_lambert + k_s * f_cook-torrance
> k_d = 1 - k_s
> k_s = Fresnel

f_lambert = c / pi
> c: albedo
> pi: c * integral( Li * cos_theta dw ) = Lo <= Li ==> c <= 1/pi

f_cook-torrance = F(l,h) * G(l,h) * D(h) / 4(n·l)(n·v)
> l: 入射方向
> h: 微平面法向
> v: 观察方向
> n: 宏观法向
*/


const float PI = 3.14159265359;


// Normal Distribution function --------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

// Geometric Shadowing function --------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    // float k_ibl = roughness * roughness / 2
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}



// Fresnel function ----------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
vec3 fresnelSchlickR(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}


vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalTex, fragTexCoord).xyz * 2.0 - 1.0;
    vec3 WorldPos = fragPosition.xyz / fragPosition.w;
    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(fragTexCoord);
    vec2 st2 = dFdy(fragTexCoord);

    vec3 N   = normalize(modelNormal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

vec3 prefilteredReflection(vec3 R, float roughness)
{
	const float MAX_REFLECTION_LOD = 4.0; // todo: param/const
	float lod = roughness * MAX_REFLECTION_LOD;
	float lodf = floor(lod);
	float lodc = ceil(lod);
	vec3 a = textureLod(preFilterTex, R, lodf).rgb;
	vec3 b = textureLod(preFilterTex, R, lodc).rgb;
	return mix(a, b, lod - lodf);
}


// From http://filmicgames.com/archives/75
vec3 Uncharted2Tonemap(vec3 x)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main() {
    vec3 albedo = texture(albedoTex, fragTexCoord).rgb;
    // albedo = pow(albedo, vec3(2.2));
    float metallic = texture(metallicTex, fragTexCoord).r;
    float roughness = texture(roughnessTex, fragTexCoord).r;
    float ao = texture(aoTex, fragTexCoord).r;

    vec3 N = getNormalFromMap();
    vec3 V = normalize(camPos - fragPosition.xyz/fragPosition.w);
    vec3 R = reflect(-V, N);

    vec3 Lo = vec3(0.0);
    vec3 F = vec3(0.0);
    vec3 specularColor = vec3(0.0);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);


    for (int i = 0; i < lightNum; i++)
    {
        // int i = 0;
        vec3 lightDir = lightPosition[i].xyz - fragPosition.xyz / fragPosition.w;
        vec3 L = normalize(lightDir);
        vec3 H = normalize (V + L);

        float distance = length(lightDir);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = attenuation * lightColor[i].rgb;


        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        F    = fresnelSchlick(max(dot(H, V), 0.0), F0);


        vec3 k_d = (vec3(1.0) - F) * (1 - metallic);

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        specularColor  = numerator / denominator;

        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specularColor) * /*radiance * */ NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

    vec3 samplerDir = normalize(modelNormal);


    vec3 ambient = vec3(0.03) * albedo * ao;

    if (consts.useIbl == 1.0)
    {
        vec3 iblIrradianceColor = texture(irradianceTex, samplerDir).rgb;
        vec3 iblPrefilterEnvColor = prefilteredReflection(R, roughness).rgb;
        vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;

        vec3 iblDiffuse = iblIrradianceColor * albedo;
        vec3 iblF = fresnelSchlickR(max(dot(N, V), 0.0), F0, roughness);
        vec3 iblSpecular = iblPrefilterEnvColor * (iblF * brdf.x + brdf.y);
        vec3 iblKd = 1.0 - iblF;
        // iblKd *= 1.0 - metallic;
        vec3 iblAmbient = (iblKd * iblDiffuse + iblSpecular);
        ambient = iblAmbient * ao;
    }

    vec3 color = ambient + Lo;

	// Tone mapping
	// color = Uncharted2Tonemap(color * 0.2);
	// color = color * (1.0f / Uncharted2Tonemap(vec3(11.2)));

    color = vec3(1.0) - exp(-color * 1.0);
	// Gamma correction
	color = pow(color, vec3(1.0 / 2.2));

	outColor = vec4(color, 1.0);



    // color = color / (color + vec3(1.0));
    // color = pow(color, vec3(1.0/2.2));

    outColor = vec4(color,1.0);
    // outColor = vec4(prefilteredReflection(R, roughness), 1.0);
}