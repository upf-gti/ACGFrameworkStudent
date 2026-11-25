#version 330 core

in vec3 v_position;
in vec3 v_world_position;

uniform vec4 u_color;
uniform vec3 u_background_color;
uniform float u_abs_coefficient;
uniform vec3 u_camera_position;
uniform float u_step;
uniform vec3 u_emitted_light;
uniform vec3 u_camera_origin;
uniform float u_light_change;
uniform float u_light_intensity;
uniform vec3 u_light_position;
uniform vec4 u_light_color;
uniform vec3 u_local_light_position;
uniform float u_scat_coefficient;
uniform sampler3D u_3D_volume;

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

float mod289(float x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 mod289(vec4 x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 perm(vec4 x){return mod289(((x * 34.0) + 1.0) * x);}

float noise(vec3 p, float i){
    vec3 a = floor(p);
    vec3 d = p - a;
    d = d * d * (3.0 - 2.0 * d);

    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = perm(b.xyxy);
    vec4 k2 = perm(k1.xyxy + b.zzww);

    vec4 c = k2 + a.zzzz;
    vec4 k3 = perm(c);
    vec4 k4 = perm(c + 1.0);

    vec4 o1 = fract(k3 * (1.0 / 41.0));
    vec4 o2 = fract(k4 * (1.0 / 41.0));

    vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
    vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);

    return o4.y * d.y*i + o4.x * (1.0 - d.y)*(1-i);
}

// MAKE SURE TO CLAMP
// end of noise function
// Using 3D data instead of random noise
float noise3Ddata(vec3 pos) {
	return texture(u_3D_volume, pos).r;
}

// RAY MARCHING FUNCTIONS

float getAbsorbtion(vec3 pos) {
	//POSITION SHOULD BE IN TEXTURE POSITION
	pos = clamp(pos, vec3(0.0), vec3(1.0));

	return clamp(noise3Ddata(pos), 0.0, 1.0);
}

float computeStep(float inter) {
	return u_step * inter;
}

vec3 getLight(vec3 pos) {
    float x = clamp(noise(pos, u_light_change), 0.0, 1.0);
    return x * u_emitted_light;
}

vec3 light_pathtacer(vec3 p, vec3 boxMin, vec3 boxMax) {

	vec3 to_light = u_local_light_position - p;
	float light_distance = length(to_light);
	if (light_distance <= 0.0001) {
		return u_light_color.rgb * u_light_intensity;
	}

	vec3 wi = to_light / light_distance;
    vec2 entry_exit = intersectAABB(p, wi, boxMin, boxMax);
	float exit_dist = min(entry_exit[1], light_distance);
    float delta = max(max(computeStep(exit_dist), exit_dist / 4.0), 0.01);
    float i = delta * 0.5;
	vec3 pos = wi * i + p;
	float tau = 0.0;

    while(i < exit_dist) {
		float density = getAbsorbtion(pos);
		float mu_t = density * (u_abs_coefficient + u_scat_coefficient);
		tau += mu_t * delta;
		i += delta;
		pos = p + wi * i;
	}
	float T = exp(-tau);

	vec3 L = u_light_color.rgb * T * u_light_intensity
		/ max(light_distance * light_distance, 0.0001);
	
    return L;
}


void main()
{
	
	vec3 rayOrigin = u_camera_origin;
	vec3 rayDir = normalize(v_position - u_camera_origin);
	vec3 boxMin = vec3(-1.0, -1.0, -1.0); //HARDCODED, GET INFO FROM MESH LATER
	vec3 boxMax = vec3(1.0, 1.0, 1.0); //HARDCODED, GET INFO FROM MESH LATER
	vec2 entry_exit = intersectAABB(rayOrigin, rayDir, boxMin, boxMax);

	vec3 B = u_background_color;
	float entry_dist = max(entry_exit[0], 0.0);
	float exit_dist = entry_exit[1];

	float delta = max(computeStep(exit_dist-entry_dist), 0.01);
	float i = delta * 0.5 + entry_dist;
	vec3 pos = rayDir * i + rayOrigin;
	float tau = 0.0;
    vec3 emission = vec3(0.0);
	vec3 scatter = vec3(0.0);

	if(exit_dist <= entry_dist){
		FragColor = vec4(B, 1.0);
		return;
	}

	while(i < exit_dist) {
		float density = getAbsorbtion(pos);
		float sigma_a = density * u_abs_coefficient;
		float sigma_s = density * u_scat_coefficient;
		float mu_t = sigma_a + sigma_s;
        float T = exp(-tau);
        emission += T * getLight(pos) * sigma_a * delta;
        scatter += T * sigma_s * light_pathtacer(pos, boxMin, boxMax) * delta;
        tau += mu_t * delta;
		i += delta;
		pos = rayOrigin + rayDir * i;
	}
	float T = exp(-tau);

	vec3 L = B*T+emission+scatter;

	FragColor = vec4(L, 1.0);
}