#version 330

// input attributes 
layout(location = 0) in vec3 position; 

// input uniforms

uniform mat4 mdvMat;      // modelview matrix 
uniform mat4 projMat;     // projection matrix
uniform mat3 normalMat;   // normal matrix
uniform mat4 mvpDepthMat; //mvp depth matrix
uniform vec3 light;
uniform vec3 motion;
uniform float time;

//We must have water_low < flow_low < flow_high < 0
uniform float water_low; 
uniform float flow_low; 
uniform float flow_high; 

// out variables 

out vec3 normalView;
out vec3 eyeView;
out vec4 shadcoord;
out vec3 p; //Le vecteur position de chaque pixel
out vec3 mpos; 
out float flow_altitude; //hauteur des vagues
out float max_altitude; //hauteur maximum des montagnes
out float depth;

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

  max_altitude = 3.0;
  float h = pnoise(p+motion.xy,max_altitude,0.2,0.5,5);

  //Plat en dessous de l'eau
  h = max(h,water_low);
  flow_altitude = flow_high+sin(10*time+100*p.x*p.y)*abs(flow_low-flow_high);
  return h;
  // version plan
   return 0;
  
  // version sinus statique
  //return 0.2*sin(p.x*30);

  // version sinus animé 
    //return 0.2*cos(p.x+motion.x)*sin((p.x+motion.x)*30);
}


vec3 computeNormal(in vec2 p) {
  const float EPS = 0.01;
  const float SCALE = 2000.;
  
  vec2 g = vec2(computeHeight(p+vec2(EPS,0.))-computeHeight(p-vec2(EPS,0.)),
		computeHeight(p+vec2(0.,EPS))-computeHeight(p-vec2(0.,EPS)))/2.*EPS;
  
  vec3 n1 = vec3(1.,0.,g.x*SCALE);
  vec3 n2 = vec3(0.,1.,-g.y*SCALE);
  vec3 n = normalize(cross(n1,n2));

  return n;
}

void main() {

  mpos = position+motion;
  float h = computeHeight(position.xy);
  vec3  n = computeNormal(position.xy);
  
  p = vec3(position.xy,h);
  
  gl_Position =  projMat*mdvMat*vec4(p,1.0);
  shadcoord   = mvpDepthMat*vec4(p,1.0)*0.5+vec4(0.5);
  normalView  = normalize(normalMat*n);
  eyeView     = normalize((mdvMat*vec4(p,1.0)).xyz);
  depth       = -(mdvMat*vec4(position,1.0)).z/30.0;
}
