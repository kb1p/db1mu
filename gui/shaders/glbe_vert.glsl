R"(
attribute vec2 aOffset;

uniform vec2 uPosition;

varying vec2 vOffset;

void main()
{
    vec2 off = aOffset;
    vOffset = off;

    gl_Position = uPosition + off;
}
)"
