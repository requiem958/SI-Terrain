#version 330

layout (location = 0) out vec4 outColor;

uniform sampler2D shadowmap;

void main() {
  outColor = texelFetch(shadowmap,ivec2(gl_FragCoord.xy),0);
}
