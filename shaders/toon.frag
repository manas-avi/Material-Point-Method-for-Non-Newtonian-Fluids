//Fragment shader contour detection
//Blinn-Phong with same color for RGB
#version 330
layout(location = 0) out vec4 out_color;

uniform vec3 light_position;
uniform vec3 eye_position;

uniform int material_shininess;
uniform float material_kd;
uniform float material_ks;

in vec3 world_pos;
in vec3 world_normal;

void main()
{
 vec3 L = normalize( light_position - world_pos);
 vec3 V = normalize( eye_position - world_pos);
 vec3 H = normalize(L + V );

 float diffuse = material_kd * max(0, dot(L,world_normal));
 float specular = 0;

 if( dot(L,world_normal) > 0.0)
 {
 specular = material_ks * pow( max(0, dot( H, world_normal)), material_shininess);
 }

 //Black color if dot product is smaller than 0.3
 //else keep the same colors
 float edgeDetection = (dot(V, world_normal) > 0.3) ? 1 : 0;

 float light = edgeDetection * (diffuse + specular);
 vec3 color = vec3(light,light,light);

 out_color = vec4(color,1);
}