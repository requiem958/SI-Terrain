#version 330

// input attributes 
layout(location = 0) in vec3 position; 

// input uniforms
uniform mat4 mvpMat;
uniform vec3 motion;

//We must have water_low < flow_low < flow_high < 0
uniform float water_low;

// fonctions utiles pour créer des terrains en général
vec2 hash(vec2 p) {
  p = vec2( dot(p,vec2(127.1,311.7)),
	    dot(p,vec2(269.5,183.3)) );  
  return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

float gnoise(in vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
	
  vec2 u = f*f*(3.0-2.0*f);
  
  return mix(mix(dot(hash(i+vec2(0.0,0.0)),f-vec2(0.0,0.0)), 
		 dot(hash(i+vec2(1.0,0.0)),f-vec2(1.0,0.0)),u.x),
	     mix(dot(hash(i+vec2(0.0,1.0)),f-vec2(0.0,1.0)), 
		 dot(hash(i+vec2(1.0,1.0)),f-vec2(1.0,1.0)),u.x),u.y);
}

float pnoise(in vec2 p,in float amplitude,in float frequency,in float persistence, in int nboctaves) {
  float a = amplitude;
  float f = frequency;
  float n = 0.0;
  
  for(int i=0;i<nboctaves;++i) {
    n = n+a*gnoise(p*f);
    f = f*2.;
    a = a*persistence;
  }
  
  return n;
}


float computeHeight(in vec2 p) {
  //perlin basique

  float max_altitude = 3.0;
  float h = pnoise(p+motion.xy,max_altitude,0.2,0.5,5);

  //Plat en dessous de l'eau
  h = max(h,water_low);
  return h;
  // version plan
   return 0;
  
  // version sinus statique
  //return 0.2*sin(p.x*30);

  // version sinus animé 
    //return 0.2*cos(p.x+motion.x)*sin((p.x+motion.x)*30);
}

void main() {
  float h = computeHeight(position.xy);
  
  vec3 p = vec3(position.xy,h);
  gl_Position =  mvpMat*vec4(p,1);
}
