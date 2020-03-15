R"(
precision highp int;
precision mediump float;

varying vec2 vOffset;

uniform int uSpriteData[64];

int imod(int x, int y)
{
    return x - x / y * y;
}

// [0.0, 1.0] -> [0, 7]
int point(float x)
{
    return int(clamp(floor(x * 8.0), 0.0, 7.0));
}

vec4 color256(int x)
{
    const float st = 1.0 / 31.0;
    vec4 clr = vec4(0.0);
    clr.b = st * float(imod(x, 32));
    clr.g = st * float(imod(x / 32, 32));
    clr.r = st * float(imod(x / 1024, 32));

    // if highmost bit is zero, this is transparent pixel
    clr.a = float(x / 32768);
    return clr;
}

void main()
{
    int c = uSpriteData[point(vOffset.y) * 8 + point(vOffset.x)];
    gl_FragColor = color256(c);
}
)";
