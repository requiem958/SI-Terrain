#version 330

out vec4 bufferColor;

uniform vec3      light;
uniform sampler2D normalmap;
uniform sampler2D colormap;

//Variable permettant d'activer ou désactiver le brouillard
uniform int fog;

in vec2 texcoord;

vec4 shade(in vec2 coord) {
  vec4  nd = texture(normalmap,coord);
  vec3  c  = texture(colormap ,coord).xyz;

  //Calcul d'un brouillard exponentiel, avec le tutoriel Ozone
  float density = 3.95;
  const float LOG2 = 1.442695;
  float z = nd.w/1.2;
  float fogFactor = exp2( -density * 
			  density *
			  z *
			  z *
			  LOG2 );
  fogFactor = clamp(fogFactor, 0.0, 1.0);
  float d = 1-fogFactor;
  
  // couleur du brouillard
  vec4 fogColor = vec4(0.5,0.6,0.9,1.0);

  //Modification de la couleur avec celle du brouillard
  return mix(vec4(c,1),fogColor,d*fog);
}

void main() {
  vec4 color = shade(texcoord);
  
  bufferColor = vec4(color);
}
