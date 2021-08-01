R"(#version 100
precision mediump float;

uniform sampler2D uTexture;

varying vec2 vTexCoord;

void main()
{
    vec2 tc = vTexCoord;
    gl_FragColor = texture2D(uTexture, tc);
}
)";
