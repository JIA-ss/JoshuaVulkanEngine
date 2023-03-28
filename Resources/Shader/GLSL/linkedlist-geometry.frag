#version 450

layout (early_fragment_tests) in;

struct Node
{
    vec4 color;
    float depth;
    uint next;
};

layout (set = 2, binding = 0) buffer MetaSBO
{
    uint count;
    uint maxNodeCount;
};

layout (set = 2, binding = 1) buffer LinkedListSBO
{
    Node nodes[];
};

layout (set = 2, binding = 2, r32ui) uniform coherent uimage2D headIndexImage;

layout(set = 1, binding = 1) uniform sampler2D diffuse;

layout(location = 0) in vec4 inModelColor;
layout(location = 1) in vec2 inTexCoord;

void main()
{
    // Increase the node count
    uint nodeIdx = atomicAdd(count, 1);

    // Check LinkedListSBO is full
    if (nodeIdx < maxNodeCount)
    {
        // Exchange new head index and previous head index
        uint prevHeadIdx = imageAtomicExchange(headIndexImage, ivec2(gl_FragCoord.xy), nodeIdx);

        // Store node data
        vec4 modelColor = vec4(texture(diffuse, inTexCoord).rgb, 1) * inModelColor;
        nodes[nodeIdx].color = modelColor;
        nodes[nodeIdx].depth = gl_FragCoord.z;
        nodes[nodeIdx].next = prevHeadIdx;
    }
}