#version 330 core

in vec3 v_position;
in vec3 v_world_position;
uniform vec4 u_color;
uniform vec3 u_background_color;
uniform float u_abs_coefficient;

out vec4 FragColor; //Can be controlled through GUI

// ray origin: u_camera_position
vec2 intersectAABB(vec3 rayOrigin, vec3 rayDir, vec3 boxMin, vec3 boxMax) {
    vec3 tMin = (boxMin - rayOrigin) / rayDir;
    vec3 tMax = (boxMax - rayOrigin) / rayDir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return vec2(tNear, tFar);
}

void main()
{
	//MOVE ALL THIS TO ABSORPTION SHADER
	/* 
	vec3 rayOrigin = u_camera_position;
	vec3 rayDir = normalize(u_camera_position-v_position)
	vec3 boxMin = vec3(-1, -1, -1); //HARDCODED, GET INFO FROM MESH LATER
	vec3 boxMax = vec3(1, 1, 1); //HARDCODED, GET INFO FROM MESH LATER
	vec2 entry_exit = intersectAABB(rayOrigin, rayDir, boxMin, boxMax)

	vec3 B = u_background_color;
	// entry_dist
	// exit_dist
	// abs_coeff
	float optical_thickness (exitt_dist - entry_dist)*abs_coeff;
	float T = exp(-optical_thickness);
	vec3 L = B^+T;

	//FragColor = vec4(B, 1.0);
	*/

	FragColor = vec4(1.0);
}