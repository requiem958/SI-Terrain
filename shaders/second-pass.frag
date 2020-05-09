#version 330

out vec4 bufferColor;

uniform vec3      light;
uniform sampler2D normalmap;
uniform sampler2D colormap;

in vec2 texcoord;

vec4 shade(in vec2 coord) {
  vec4  nd = texture(normalmap,coord);
  vec3  c  = texture(colormap ,coord).xyz;

  //return vec4(c,1); à décommenter pour tester sans fog
    // on recupere ce qui se trouve dans le canal alpha (i.e. la profondeur)
  // on peut le modifier eventuellement, puis on clampe les valeurs entre 0 et 1
  float d = clamp(nd.w*1.5,0.0,1.0);
  // couleur du brouillard
  vec4 fogColor = vec4(0.8,0.75,0.76,1.0);
  // on modifie la couleur avec la couleur du brouillard
  // en fonction de la profondeur (simple interpolation lineaire ici)
  return mix(vec4(c,1),fogColor,d);
}

void main() {
  vec4 color = shade(texcoord);
  
  bufferColor = vec4(color);
}
