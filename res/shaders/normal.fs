#version 330 core

in vec3 v_normal;

out vec4 FragColor;

void main()
{
	// Convert from range [-1,1] to [0,1]
	vec3 normal_color = (v_normal + 1.0) * 0.5;
	
	FragColor = vec4(normal_color, 1.0);
}