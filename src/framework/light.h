#pragma once

#include "scenenode.h"

class Light : public SceneNode {
public:

	float intensity;

	Light(glm::vec3 position = glm::vec3(0.f), float intensity = 1.f, glm::vec4 color = glm::vec4(1.f));

	void setUniforms(Shader* shader, const glm::mat4& model);
	void renderInMenu();
};