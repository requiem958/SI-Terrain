#version 330

layout (location=0) out vec4 outColor;

uniform sampler2D colormap;

in vec2 texcoord;

vec4 shade(in vec2 coord) {
  return texture(colormap,coord);
}

void main() {  
  outColor = shade(texcoord);
}
