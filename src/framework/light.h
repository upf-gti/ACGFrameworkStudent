#pragma once

#include "scenenode.h"

enum eLightType { LIGHT_DIRECTIONAL, LIGHT_POINT, LIGHT_SPOT };

class Light : public SceneNode {
public:

	eLightType light_type;
	float intensity;
	glm::vec4 color;

	float shininess = 10.f;
	float max_distance = 100.f;
	bool cast_shadows = false;

	Light(glm::vec3 position = glm::vec3(0.f), eLightType type = LIGHT_DIRECTIONAL, float intensity = 1.f, glm::vec4 color = glm::vec4(1.f));

	void setUniforms(Shader* shader, const glm::mat4& model);
	void renderInMenu();
};