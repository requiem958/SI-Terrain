#version 330

// input uniforms

uniform vec3 light;
uniform vec3 motion;
uniform float time;
uniform float water_low; 
uniform float flow_low; 
uniform float flow_high; 

uniform sampler2D forest;
uniform sampler2D sol;
uniform sampler2D snow;
//uniform sampler2D water;
uniform sampler2DShadow shadowmap;
// in variables 

in vec3  normalView;
in vec3  eyeView;
in vec3  p;
in vec3 mpos;
in vec4 shadcoord;
in float flow_altitude;
in float max_altitude;
// out buffers 

layout(location = 0) out vec4 outColor;

void main() {
   vec3 ambient_haut  = texture(snow,mpos.xy).xyz;//vec3(1,1,1); //snow
   vec3 ambient_sol = texture(sol,mpos.xy).xyz;//vec3(0.36, 0.2, 0.09); //brown
   vec3 ambient_forest = texture(forest,mpos.xy).xyz;//vec3(0,1,0); //green
   vec3 ambient_water = vec3(0,0,1); //blue //texture(water,mpos.xy).xyz;

  const vec3 diffuse  = vec3(0.3,0.5,0.8);
  const vec3 specular = vec3(0.8,0.2,0.2);
  const float et = 50.0;


  vec3 n = normalize(normalView);
  vec3 e = normalize(eyeView);
  vec3 l = normalize(light);
  l.x += sin(2*time);
  l = normalize(l);
  float diff = dot(l,n);
  float spec = pow(max(dot(reflect(l,n),e),0.0),et);

  vec3 a = vec3(0);;

  float altitude = p.z;
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

  float v = 1.0;
  float b = 0.05;

  v -= 0.8*(1.0-texture(shadowmap,vec3(shadcoord.xy,(shadcoord.z-b)/shadcoord.w)));
  
  //if ( texture( shadowmap,shadcoord.xy ).z  <shadcoord.z - b){
  //  v = 0.5;
  //}

  outColor = vec4(v*color,1.0);
}
