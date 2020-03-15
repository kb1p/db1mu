R"(
precision mediump float;
precision mediump int;

attribute vec2 aOffset;

uniform ivec2 uPosition;

varying vec2 vOffset;

void main()
{
    vec2 off = aOffset;
    vOffset = off;

    // Convert NES screen coordinates [0..255]x[0..239] to [0..1.0]x[0..1.0]
    const float PXW = 1.0 / 256.0;
    const float PXH = 1.0 / 240.0;
    vec2 nPos = vec2(float(uPosition.x) * PXW, 1.0 - (8.0 + float(uPosition.y)) * PXH);

    // Add offset within character rectangle, each character is 8x8 pixels
    nPos.x += off.x * 8.0 * PXW;
    nPos.y += (1.0 - off.y) * 8.0 * PXH;

    // Convert [0..1]^2 coordinates to normalized GLES [-1..1]^2
    vec4 pos = vec4(vec2(nPos.xy * 2.0 - vec2(1.0)), 1.0, 1.0);
    gl_Position = pos;
}
)";
