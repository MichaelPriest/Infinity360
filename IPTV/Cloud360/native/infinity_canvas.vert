#version 450
layout(location=0) in vec3 position;
// XSDK GLSL vertex input mapping: 0=POSITION, 1=COLOR, 2=TEXCOORD0.
layout(location=2) in vec2 uv;
layout(location=0) out vec2 texcoord;
void main(){gl_Position=vec4(position,1.0);texcoord=uv;}
