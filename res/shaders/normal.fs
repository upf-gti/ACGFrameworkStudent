#version 450 core

in vec3 v_normal;

out vec4 FragColor;

void main()
{
	FragColor = vec4(v_normal, 1.0);
}