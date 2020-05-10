#version 330

// input attributes 
layout(location = 0) in vec3 position;// position of the vertex in world space

//coordonnées dans les textures
out vec2 texcoord;

void main() {
  gl_Position = vec4(position,1.0);

  //redimensionnement entre [0,1]
  texcoord = position.xy*0.5+0.5;
}
