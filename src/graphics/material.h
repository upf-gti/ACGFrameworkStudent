#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/matrix.hpp>

#include "../framework/camera.h"
#include "mesh.h"
#include "texture.h"
#include "shader.h"
#include "utils.h"

#include "openvdbReader.h"

class Material {
public:

	Shader* shader = NULL;
	Texture* texture = NULL;
	glm::vec4 color;

	int index; //Used for selections
	bool abs_slider = false;
	bool step_slider = false;

	virtual void setUniforms(Camera* camera, glm::mat4 model) = 0;
	virtual void render(Mesh* mesh, glm::mat4 model, Camera* camera) = 0;
	virtual void renderInMenu() = 0; //=0, thus should be overriden by child classes
};

class FlatMaterial : public Material {
public:
	FlatMaterial(glm::vec4 color = glm::vec4(1.f));
	~FlatMaterial();

	void setUniforms(Camera* camera, glm::mat4 model);
	void render(Mesh* mesh, glm::mat4 model, Camera* camera);
	void renderInMenu();
};

class WireframeMaterial : public FlatMaterial {
public:

	WireframeMaterial();
	~WireframeMaterial();

	void render(Mesh* mesh, glm::mat4 model, Camera* camera);
};

class StandardMaterial : public Material {
public:

	bool first_pass = false;

	bool show_normals = false;
	Shader* base_shader = NULL;
	Shader* normal_shader = NULL;

	StandardMaterial(glm::vec4 color = glm::vec4(1.f));
	~StandardMaterial();

	void setUniforms(Camera* camera, glm::mat4 model);
	void render(Mesh* mesh, glm::mat4 model, Camera* camera);
	void renderInMenu();
};


class VolumeMaterial : public Material {
public:
	float abs_coefficient;
	Shader* base_shader = NULL;
	float step;
	glm::vec3 Le = glm::vec3(0.0);


	VolumeMaterial(float abs_coefficient = 1, float step = 0.01);
	~VolumeMaterial();
	void setUniforms(Camera* camera, glm::mat4 model);
	void loadVDB(std::string file_path);
	void estimate3DTexture(easyVDB::OpenVDBReader* vdbReader);
	void render(Mesh* mesh, glm::mat4 model, Camera* camera);
	void renderInMenu();
	//Render in menu then checks for current shader to show sliders	
};