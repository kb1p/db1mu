R"(#version 100
attribute vec2 aOffset;

varying vec2 vTexCoord;

void main()
{
    vec2 t = aOffset;
    vTexCoord = t;

    // [0..1] -> [-1..1]
    gl_Position = vec4(t * 2.0 - vec2(1.0), 1.0, 1.0);
}
)";
