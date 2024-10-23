#include "camera.h"

#include "includes.h"
#include <iostream>

Camera* Camera::current = nullptr;

Camera::Camera()
{
	this->current = this;

	this->view_matrix = glm::mat4(1.f);
	this->projection_matrix = glm::mat4(1.f);
	this->viewprojection_matrix = glm::mat4(1.f);

	setOrthographic(-1, 1, 1, -1, -1, 1);
}

glm::vec3 Camera::getLocalVector(const glm::vec3& v)
{
	glm::mat4 iV = glm::inverse(view_matrix);

	glm::vec3 result = iV * glm::vec4(v, 0.0f);
	return result;
}

glm::vec3 Camera::projectVector(glm::vec3 pos, bool& negZ)
{
	glm::vec4 result = viewprojection_matrix * glm::vec4(pos, 1.0);
	negZ = result.z < 0;
	if (type == ORTHOGRAPHIC)
		return glm::vec3(result.x, result.y, result.z);
	else
		return glm::vec3(result.x, result.y, result.z) / result.w;
}

void Camera::rotate(float angle, const glm::vec3& axis)
{
	glm::vec3 front = center - eye;
	//normalize(front);
	glm::quat rotation = glm::angleAxis(angle, axis);

	glm::vec3 newFront = rotation * front;
	center = newFront + eye;
	updateViewMatrix();
}

glm::vec3 Camera::transformQuat(const glm::vec3& a, const glm::quat& q)
{
	float qx = q.x, qy = q.y, qz = q.z, qw = q.w;
	float x = a.x, y = a.y, z = a.z;

	// var qvec = [qx, qy, qz];
	// var uv = vec3.cross([], qvec, a);
	float uvx = qy * z - qz * y,
		uvy = qz * x - qx * z,
		uvz = qx * y - qy * x;
	// var uuv = vec3.cross([], qvec, uv);
	float uuvx = qy * uvz - qz * uvy,
		uuvy = qz * uvx - qx * uvz,
		uuvz = qx * uvy - qy * uvx;
	// vec3.scale(uv, uv, 2 * w);
	float w2 = qw * 2;
	uvx *= w2;
	uvy *= w2;
	uvz *= w2;
	// vec3.scale(uuv, uuv, 2);
	uuvx *= 2;
	uuvy *= 2;
	uuvz *= 2;
	// return vec3.add(out, a, vec3.add(out, uv, uuv));

	glm::vec3 out;

	out[0] = x + uvx + uuvx;
	out[1] = y + uvy + uuvy;
	out[2] = z + uvz + uuvz;

	return out;
}

glm::quat Camera::setAxisAngle(const glm::vec3& axis, const float angle)
{
	glm::quat q;
	float s;
	s = sinf(angle * 0.5f);

	q.x = axis.x * s;
	q.y = axis.y * s;
	q.z = axis.z * s;
	q.w = cosf(angle * 0.5f);

	return q;
}


void Camera::orbit(float yaw, float pitch)
{
	glm::vec3 front = glm::normalize(center - eye);
	float problem_angle = glm::dot(front, up);

	glm::vec4 right_aux = glm::inverse(view_matrix) * glm::vec4(1.f, 0.f, 0.f, 0.f);
	glm::vec3 right = glm::vec3(right_aux.x, right_aux.y, right_aux.z);
	glm::vec3 dist = eye - center;

	// yaw
	glm::quat R = glm::angleAxis(-yaw, up);
	dist = transformQuat(dist, R);

	if (!(problem_angle > 0.99 && pitch > 0 || problem_angle < -0.99 && pitch < 0)) {
		// pitch
		R = setAxisAngle(right, pitch);
	}

	dist = transformQuat(dist, R);
	eye = dist + center;

	updateViewMatrix();
}

void Camera::move(glm::vec3 delta)
{
	glm::vec3 localDelta = getLocalVector(delta);
	eye = eye - localDelta;
	center = center - localDelta;
	updateViewMatrix();
}

void Camera::setOrthographic(float left, float right, float top, float bottom, float near_plane, float far_plane)
{
	type = ORTHOGRAPHIC;

	this->left = left;
	this->right = right;
	this->top = top;
	this->bottom = bottom;
	this->near_plane = near_plane;
	this->far_plane = far_plane;

	updateProjectionMatrix();
}

void Camera::setPerspective(float fov, float aspect, float near_plane, float far_plane)
{
	type = PERSPECTIVE;

	this->fov = fov;
	this->aspect = aspect;
	this->near_plane = near_plane;
	this->far_plane = far_plane;

	this->min_fov = 10;
	this->max_fov = 110;

	updateProjectionMatrix();
}

void Camera::lookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up)
{
	this->eye = eye;
	this->center = center;
	this->up = up;

	updateViewMatrix();
}

void Camera::updateViewMatrix()
{
	// Reset Matrix (Identity)
	view_matrix = glm::mat4(1.f);

	// Front/Forward vector
	glm::vec3 f = glm::normalize(center - eye) * -1.0f;

	// Right/Side vector
	glm::vec3 r = glm::cross(up, f); // Right handed
	if (r == glm::vec3(0, 0, 0)) {
		std::cout << "Error: View and up vectors are parallel\n";
		return; // Error
	}
	r = glm::normalize(r);

	// Uo/Top vector
	glm::vec3 u = glm::normalize(glm::cross(f, r)); // Right handed
	glm::vec3 t = glm::vec3(
		-dot(r, eye),
		-dot(u, eye),
		-dot(f, eye)
	);
	view_matrix = glm::mat4(
		// Transpose upper 3x3 matrix to invert it
		r.x, u.x, f.x, 0,
		r.y, u.y, f.y, 0,
		r.z, u.z, f.z, 0,
		t.x, t.y, t.z, 1
	);

	updateViewProjectionMatrix();
}

// Create a projection matrix
void Camera::updateProjectionMatrix()
{
	// Reset Matrix (Identity)
	projection_matrix = glm::mat4(1.f);

	if (left == right || top == bottom || near_plane == far_plane) {
		std::cout << "Error: Invalid frustum\n";
		return; // Error
	}

	if (type == PERSPECTIVE) {
		float ymax = near_plane * tanf(fov * 3.14159265359f / 360.0f);
		float xmax = ymax * aspect;
		left = -xmax; right = xmax; bottom = -ymax; top = ymax;
		projection_matrix = glm::mat4(
			(2.0f * near_plane) / (right - left), 0, 0, 0,
			0, (2.0f * near_plane) / (top - bottom), 0, 0,
			(right + left) / (right - left), (top + bottom) / (top - bottom), (-(far_plane + near_plane)) / (far_plane - near_plane), -1,
			0, 0, (-2 * far_plane * near_plane) / (far_plane - near_plane), 0
		);
	}
	else if (type == ORTHOGRAPHIC) {
		projection_matrix = glm::mat4(
			2.0f / (right - left), 0, 0, 0,
			0, 2.0f / (top - bottom), 0, 0,
			0, 0, -2.0f / (far_plane - near_plane), 0,
			-((right + left) / (right - left)), -((top + bottom) / (top - bottom)), -((far_plane + near_plane) / (far_plane - near_plane)), 1
		);
	}

	updateViewProjectionMatrix();
}

void Camera::updateViewProjectionMatrix()
{
	viewprojection_matrix = projection_matrix * view_matrix;
}

glm::mat4 Camera::getViewProjectionMatrix()
{
	updateViewMatrix();
	updateProjectionMatrix();

	return viewprojection_matrix;
}

void Camera::renderInMenu()
{
	bool changed = false;
	changed |= ImGui::DragFloat3("Eye", &this->eye.x, 1, -100, 100);
	changed |= ImGui::DragFloat3("Center", &this->center.x, 1, -100, 100);
	changed |= ImGui::DragFloat3("Up", &this->up.x, 1, -100, 100);
	changed |= ImGui::SliderFloat("FOV", &this->fov, this->min_fov, this->max_fov);
	changed |= ImGui::SliderFloat("Near", &this->near_plane, 0.01f, this->far_plane);
	changed |= ImGui::SliderFloat("Far", &this->far_plane, this->near_plane, 1000.f);

	if (changed) {
		this->updateViewMatrix();
		this->updateProjectionMatrix();
	}
}