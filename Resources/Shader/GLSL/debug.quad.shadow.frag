#version 450

// SET2 SHADOWMAP
layout(set = 2, binding = 1) uniform sampler2D shadowMap1;
layout(set = 2, binding = 2) uniform sampler2D shadowMap2;
layout(set = 2, binding = 3) uniform sampler2D shadowMap3;
layout(set = 2, binding = 4) uniform sampler2D shadowMap4;
layout(set = 2, binding = 5) uniform sampler2D shadowMap5;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {


    outColor = texture(shadowMap1, fragTexCoord);
    // outColor = vec4(inShadowAvg, 0,0,1);
}