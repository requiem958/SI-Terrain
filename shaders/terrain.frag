#version 330

// input uniforms

uniform vec3 light;

//déplacement de la caméra
uniform vec3 motion;



//temps écoulé depuis le début du programme
uniform float time;

//Hauteur la plus basse du terrain
uniform float water_low;

//Haut des vagues
uniform float flow_low;
//Bas des vagues
uniform float flow_high; 

//textures pour le terrain
uniform sampler2D forest;
uniform sampler2D sol;
uniform sampler2D snow;
uniform sampler2DShadow shadowmap;
// in variables 

in vec3  normalView;
in vec3  eyeView;
in vec3  p;
in vec3 mpos;
in float depth;

//Coordonnées pour le shadow mapping
in vec4 shadcoord;

//Altitude des vagues
in float flow_altitude;

//Altitude maximum du terrain
in float max_altitude;


// out buffers 

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outDepth;

// Pour le calcul d'ombres douces
vec2 poissonDisk[16] = vec2[]( 
vec2( -0.94201624, -0.39906216 ), 
vec2( 0.94558609, -0.76890725 ), 
vec2( -0.094184101, -0.92938870 ), 
vec2( 0.34495938, 0.29387760 ), 
vec2( -0.91588581, 0.45771432 ), 
vec2( -0.81544232, -0.87912464 ), 
vec2( -0.38277543, 0.27676845 ), 
vec2( 0.97484398, 0.75648379 ), 
vec2( 0.44323325, -0.97511554 ), 
vec2( 0.53742981, -0.47373420 ), 
vec2( -0.26496911, -0.41893023 ), 
vec2( 0.79197514, 0.19090188 ), 
vec2( -0.24188840, 0.99706507 ), 
vec2( -0.81409955, 0.91437590 ), 
vec2( 0.19984126, 0.78641367 ), 
vec2( 0.14383161, -0.14100790 ) 
);

//Fonction pour calculer à quel point un point est dans l'ombre
float shadow_percentage(){
  float v = 1.0;
  float b = 0.005;

  int ind = 0;
  vec2 coord;

  //On fait du PCF avec des ombres douces
  //Cela fait la superpostion de plusieurs ombres légérement décalées
  for (ind = 0; ind < 20; ind++){
    coord = shadcoord.xy+poissonDisk[ind]/100;
    v -= 0.02*(1.0-texture(shadowmap,vec3(coord.xy,(shadcoord.z-b)/shadcoord.w)));
  }
  return v;
}


void main() {
   vec3 ambient_haut  = texture(snow,mpos.xy).xyz; //rochers
   vec3 ambient_sol = texture(sol,mpos.xy).xyz;// sol près de la mer
   vec3 ambient_forest = texture(forest,mpos.xy).xyz;//forets au milieu
   vec3 ambient_water = vec3(0,0,1); //la mer

  const vec3 diffuse  = vec3(0.3,0.5,0.8);
  const vec3 specular = vec3(0.8,0.2,0.2);
  const float et = 50.0;


  vec3 n = normalize(normalView);
  vec3 e = normalize(eyeView);
  vec3 l = normalize(light);
  float diff = dot(l,n);
  float spec = pow(max(dot(reflect(l,n),e),0.0),et);

  vec3 a = vec3(0);

  float altitude = p.z;

  //Constantes pour savoir comment on divise la hauteur du terrain
  
  const float division = 4.0;
  const float min_foret = 0/division;
  const float max_foret = 1.0/division;

  //Water
  if (altitude <= flow_altitude){
    a = mix(ambient_water,vec3(0,0,0),l.x);
  }
  else {
    altitude = (altitude - flow_high)/(max_altitude);
    if (altitude <= min_foret){
      a = ambient_sol;
    }else if (altitude >= max_foret){
      a = mix(ambient_forest,ambient_haut,(altitude-max_foret)*division);
    }else{
      a = mix(ambient_sol,ambient_forest,(altitude-min_foret)*division);
    }
  }
  vec3 color = a + diff*diffuse + spec*specular;

  float v = shadow_percentage();


  outColor = vec4(v*color,1.0);

  outDepth = vec4(n,depth);
}

