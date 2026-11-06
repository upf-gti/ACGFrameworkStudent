#version 330 core

uniform vec4 u_color;
uniform vec3 u_background_color;

out vec4 FragColor; //Can be controlled through GUI

void main()
{
	vec3 B = u_background_color;
	// entry_dist
	// exit_dist
	// abs_coeff
	float optical_thickness (exitt_dist - entry_dist)*abs_coeff;
	float T = exp(-optical_thickness);
	vec3 L = B^+T;

	FragColor = vec4(B, 1.0);
}