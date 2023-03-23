#version 450


layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec2 fragTexCoord;
void main() {
    fragTexCoord = inPosition.xy * 0.5 + vec2(0.5, 0.5);
    gl_Position = vec4(inPosition, 1.0);
}