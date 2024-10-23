#pragma once

#include <glm/matrix.hpp>

#include "camera.h"
#include "../graphics/shader.h"
#include "../graphics/mesh.h"
#include "../graphics/material.h"
#include "framework/utils.h"

class Light;
enum eType { NODE_BASE, NODE_VOLUME, NODE_LIGHT };

class SceneNode {
public:

	eType type;
	static unsigned int lastNameId;
	std::string name;

	glm::mat4 model = glm::mat4(1.f);
	Mesh* mesh = NULL;
	Material* material = NULL;

	bool visible = true;

	SceneNode();
	SceneNode(const char* name);
	~SceneNode();

	virtual void render(Camera* camera);
	virtual void renderWireframe(Camera* camera);
	virtual void renderInMenu();
};