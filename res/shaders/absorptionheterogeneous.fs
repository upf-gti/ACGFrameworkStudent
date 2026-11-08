#version 330 core

in vec3 v_position;
in vec3 v_world_position;

uniform vec4 u_color;
uniform vec3 u_background_color;
uniform float u_abs_coefficient;
uniform vec3 u_camera_position;
uniform float u_step;

out vec4 FragColor; 

vec2 intersectAABB(vec3 rayOrigin, vec3 rayDir, vec3 boxMin, vec3 boxMax) {
    vec3 tMin = (boxMin - rayOrigin) / rayDir;
    vec3 tMax = (boxMax - rayOrigin) / rayDir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);	
    float tFar = min(min(t2.x, t2.y), t2.z);
    return vec2(tNear, tFar);
}

// Noise function:

/* discontinuous pseudorandom uniformly distributed in [-0.5, +0.5]^3 */
vec3 random3(vec3 c) {
	float j = 4096.0*sin(dot(c,vec3(17.0, 59.4, 15.0)));
	vec3 r;
	r.z = fract(512.0*j);
	j *= .125;
	r.x = fract(512.0*j);
	j *= .125;
	r.y = fract(512.0*j);
	return r-0.5;
}

const float F3 =  0.3333333;
const float G3 =  0.1666667;
float snoise(vec3 p) {

	vec3 s = floor(p + dot(p, vec3(F3)));
	vec3 x = p - s + dot(s, vec3(G3));
	 
	vec3 e = step(vec3(0.0), x - x.yzx);
	vec3 i1 = e*(1.0 - e.zxy);
	vec3 i2 = 1.0 - e.zxy*(1.0 - e);
	 	
	vec3 x1 = x - i1 + G3;
	vec3 x2 = x - i2 + 2.0*G3;
	vec3 x3 = x - 1.0 + 3.0*G3;
	 
	vec4 w, d;
	 
	w.x = dot(x, x);
	w.y = dot(x1, x1);
	w.z = dot(x2, x2);
	w.w = dot(x3, x3);
	 
	w = max(0.6 - w, 0.0);
	 
	d.x = dot(random3(s), x);
	d.y = dot(random3(s + i1), x1);
	d.z = dot(random3(s + i2), x2);
	d.w = dot(random3(s + 1.0), x3);
	 
	w *= w;
	w *= w;
	d *= w;
	 
	return dot(d, vec4(52.0));
}

float snoiseFractal(vec3 m) {
	return   0.5333333* snoise(m)
				+0.2666667* snoise(2.0*m)
				+0.1333333* snoise(4.0*m)
				+0.0666667* snoise(8.0*m);
}

// MAKE SURE TO CLAMP
// end of noise function


// RAY MARCHING FUNCTIONS

float getAbsorbtion(vec3 pos) {
	return clamp(snoiseFractal(pos), 0.0, 1.0);
}

float computeStep(float inter) {
	return u_step * inter;
}



void main()
{
	
	vec3 rayOrigin = u_camera_position;
	vec3 rayDir = normalize(v_position - u_camera_position);
	vec3 boxMin = vec3(-1.0, -1.0, -1.0); //HARDCODED, GET INFO FROM MESH LATER
	vec3 boxMax = vec3(1.0, 1.0, 1.0); //HARDCODED, GET INFO FROM MESH LATER
	vec2 entry_exit = intersectAABB(rayOrigin, rayDir, boxMin, boxMax);

	vec3 B = u_background_color;
	float entry_dist = max(entry_exit[0], 0.0);
	float exit_dist = entry_exit[1];

	float delta = max(computeStep(exit_dist-entry_dist), 0.1);
	float i = delta * 0.5 + entry_dist;
	vec3 pos = rayDir * i + rayOrigin;
	float tau = 0.0;

	if(exit_dist <= entry_dist){
		FragColor = vec4(B, 1.0);
		return;
	}

	while(i < exit_dist) {
		float mu_a = getAbsorbtion(pos)*u_abs_coefficient;
		tau += mu_a * delta;
		i += delta;
		pos = rayOrigin + rayDir * i;
	}

	float T = exp(-tau);
	vec3 L = B*T;

	FragColor = vec4(L, 1.0);
}