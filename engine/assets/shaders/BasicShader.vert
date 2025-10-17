#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 v_worldPos;

void main()
{
    int x = gl_InstanceIndex / 100;
    int y = (gl_InstanceIndex / 10) % 10;
    int z = gl_InstanceIndex % 10;
    v_worldPos = (ubo.model *  vec4(inPosition, 1.0) + 0.5 * vec4(x, y, z, 0.0)).rgb;
    gl_Position = ubo.proj * ubo.view * (ubo.model *  vec4(inPosition, 1.0) + 0.5 * vec4(x, y, z, 0.0));
    fragColor = inColor;
}