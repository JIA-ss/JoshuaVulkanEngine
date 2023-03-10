#version 450

layout(set = 1, binding = 1) uniform sampler2D diffuse;
layout(set = 1, binding = 2) uniform sampler2D specular;
layout(set = 1, binding = 3) uniform sampler2D ambient;
layout(set = 1, binding = 4) uniform sampler2D emissive;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 camPos;
layout(location = 3) in vec3 lightPos;
layout(location = 4) in vec3 worldNormal;
layout(location = 5) in vec4 modelColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 diffuseColor = texture(diffuse, fragTexCoord).rgb;
    vec3 specularTex = texture(specular, fragTexCoord).rgb;

    vec3 normal = normalize(worldNormal);
    vec3 lightDir = normalize(lightPos - fragPosition);
    float diffuseCoeffi = max(dot(normal, lightDir), 0.0);

    //specular
    float specularCoeffi = 0.5;
    vec3 viewDir = normalize(camPos - fragPosition);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specularColor = specularCoeffi * spec * specularTex;

    vec3 finalColor = (diffuseColor + specularColor);
    // finalColor = pow(finalColor, vec3(1.0/2.2)) * modelColor.rgb;

    outColor = vec4(finalColor, 1.0);
    // outColor = vec4(fragColor, 1.0);
}