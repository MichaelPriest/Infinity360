#version 450
layout(location=0) in vec2 texcoord;
layout(location=0) out vec4 color;
layout(set=0,binding=0) uniform sampler2D canvas;
void main(){color=texture(canvas,texcoord);}
