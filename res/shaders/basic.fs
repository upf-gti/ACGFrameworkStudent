#version 410 core

in vec3 v_position;
in vec3 v_world_position;
in vec3 v_normal;

uniform vec3 u_camera_position;

uniform vec4 u_color;
uniform vec4 u_ambient_light;

uniform vec3 u_light_position;
uniform vec4 u_light_color;
uniform float u_light_intensity;
uniform float u_light_shininess;

out vec4 FragColor;

void main()
{
	vec3 N = normalize(v_normal);
	vec3 L = normalize(u_light_position - v_world_position);
	vec3 V = normalize(u_camera_position - v_world_position);
	vec3 R = reflect(-L, N);

	// Phong
	float diff = max(dot(N, L), 0.0);
	vec4 diffuse = diff * u_light_color;

	float spec = pow(max(dot(V, R), 0.0), u_light_shininess);
	vec4 specular = spec * u_light_color;

	vec4 phong_color = (u_ambient_light + diffuse + specular) * u_color;
	FragColor = phong_color * u_light_intensity;
}
