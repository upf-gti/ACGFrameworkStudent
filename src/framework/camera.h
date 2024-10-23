#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/matrix.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

class Camera
{
public:
	static Camera* current;

	// Types of cameras available
	enum { PERSPECTIVE, ORTHOGRAPHIC };
	char type;

	// Vectors to define the orientation of the camera
	glm::vec3 eye;		// Where is the camera
	glm::vec3 center;	// Where is it pointing
	glm::vec3 up;		// The up pointing up

	// Properties of the projection of the camera
	float fov;			// View angle in degrees (1/zoom)
	float min_fov;
	float max_fov;
	float aspect;		// Aspect ratio (width/height)
	float near_plane;	// Near plane
	float far_plane;	// Far plane

	// For orthogonal projection
	float left, right, top, bottom;

	// Matrices
	glm::mat4 view_matrix;
	glm::mat4 projection_matrix;
	glm::mat4 viewprojection_matrix;

	Camera();

	// Setters
	void setAspectRatio(float aspect) { this->aspect = aspect; };

	// Translate and rotate the camera
	void move(glm::vec3 delta);
	void rotate(float angle, const glm::vec3& axis);
	glm::vec3 transformQuat(const glm::vec3& a, const glm::quat& q); // check glm implmentation
	glm::quat setAxisAngle(const glm::vec3& axis, const float angle); // check glm implmentation
	void orbit(float yaw, float pitch);

	// Transform a local camera vector to world coordinates
	glm::vec3 getLocalVector(const glm::vec3& v);

	// Project 3D Vectors to 2D Homogeneous Space
	// If negZ is true, the projected point IS NOT inside the frustum, 
	// so it does not have to be rendered!
	glm::vec3 projectVector(glm::vec3 pos, bool& negZ);

	// Set the info for each projection
	void setPerspective(float fov, float aspect, float near_plane, float far_plane);
	void setOrthographic(float left, float right, float top, float bottom, float near_plane, float far_plane);
	void lookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);

	// Compute the matrices
	void updateViewMatrix();
	void updateProjectionMatrix();
	void updateViewProjectionMatrix();

	glm::mat4 getViewProjectionMatrix();

	void renderInMenu();
};