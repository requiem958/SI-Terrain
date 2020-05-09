#version 330

out vec4 bufferColor;

uniform vec3      light;
uniform sampler2D normalmap;
uniform sampler2D colormap;

uniform int fog;

in vec2 texcoord;

vec4 shade(in vec2 coord) {
  vec4  nd = texture(normalmap,coord);
  vec3  c  = texture(colormap ,coord).xyz;

  //return vec4(c,1); à décommenter pour tester sans fog
  
  float density = 3.95;
  const float LOG2 = 1.442695;
  float z = nd.w;
  float fogFactor = exp2( -density * 
			  density *
			  z *
			  z *
			  LOG2 );
  fogFactor = clamp(fogFactor, 0.0, 1.0);
  float d = 1-fogFactor;
  // couleur du brouillard
  vec4 fogColor = vec4(0.8,0.8,0.8,1.0);
  // on modifie la couleur avec la couleur du brouillard
  // en fonction de la profondeur (simple interpolation lineaire ici)
  return mix(vec4(c,1),fogColor,d*fog);
}

void main() {
  vec4 color = shade(texcoord);
  
  bufferColor = vec4(color);
}
