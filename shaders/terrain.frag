#version 330

// input uniforms 
uniform vec3 light;
uniform vec3 motion;
uniform float time;

// in variables 
in vec3  normalView;
in vec3  eyeView;
in vec3  p;
// out buffers 
layout(location = 0) out vec4 outColor;

void main() {
  const vec3 ambient_haut  = vec3(1,1,1); //snow
  const vec3 ambient_sol = vec3(0.36, 0.2, 0.09); //brown
  const vec3 ambient_forest = vec3(0,1,0); //green
  const vec3 ambient_water = vec3(0,0,1); //blue

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

  vec3 a = ambient_sol;

  float altitude = p.z;

  if (altitude <= -0.2){
    a = ambient_water;
    }
  else if (altitude <= 1.0 / 3.0){
     a = ambient_sol;
  }else if (altitude >= 2.5/3){
    a = mix(ambient_forest,ambient_haut,(p.z-2.5/3.0)*3.0);
  }else{
     a = mix(ambient_sol,ambient_forest,(p.z-1.0/3.0)*3);
  }
  
  vec3 color = a + diff*diffuse + spec*specular;

  outColor = vec4(color,1.0);
}
