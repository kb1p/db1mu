R"(
precision lowp int;
precision mediump float;

varying vec2 vOffset;

uniform int uSpriteData[64];

// ${PALETTE}

// [0.0, 1.0] -> [0, 7]
int point(float x)
{
    return clamp(int(floor(x * 8)), 0, 7);
}

vec4 color256(int x)
{
    return PALETTE[x];
}

void main()
{
    int c = uSpriteData[point(vOffset.y) * 8 + point(vOffset.x)];
    gl_FragColor = color256(c);
}
)"
