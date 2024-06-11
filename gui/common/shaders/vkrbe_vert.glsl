#version 450

layout (location = 0) out vec2 fragUV;

const vec2 pos[6] = vec2[](vec2(0.0f, 0.0f),
                           vec2(1.0f, 0.0f),
                           vec2(1.0f, 1.0f),
                           vec2(1.0f, 1.0f),
                           vec2(0.0f, 1.0f),
                           vec2(0.0f, 0.0f));

void main()
{
    vec2 c = pos[gl_VertexIndex];
    fragUV = c;

    // Vulkan has positive Y axis direction pointing downwards (unlike OpenGL),
    // need to invert it otherwise triangles will be culled out.
    gl_Position = vec4(c * vec2(2.0, -2.0) - vec2(1.0, -1.0), 0.0, 1.0);
}
