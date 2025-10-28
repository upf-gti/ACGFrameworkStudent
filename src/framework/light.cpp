#include "light.h"

#include "ImGuizmo.h"

Light::Light(glm::vec3 position, float intensity, glm::vec4 color)
{
	this->type = NODE_LIGHT;

	this->name = std::string("Light" + std::to_string(this->lastNameId));
	this->model = glm::translate(this->model, position);
	
	this->color = color;
	this->intensity = intensity;

	this->cast_shadows;

	// Create a debug sphere mesh
	this->mesh = Mesh::Get("res/meshes/sphere.obj");
	this->model = glm::scale(this->model, glm::vec3(0.1f));
	this->material = new FlatMaterial();
}

void Light::setUniforms(Shader* shader, const glm::mat4& model)
{
	glm::vec3 position = glm::vec3(this->model[3][0], this->model[3][1], this->model[3][2]);

	// Compute light position in local coordinates
	glm::mat4 inverseModel = glm::inverse(model);
	glm::vec4 temp = glm::vec4(position, 1.0);
	temp = inverseModel * temp;
	glm::vec3 local_pos = glm::vec3(temp.x / temp.w, temp.y / temp.w, temp.z / temp.w);

	shader->setUniform("u_light_intensity", this->intensity);
	shader->setUniform("u_light_color", this->color);
	shader->setUniform("u_light_position", position);
	shader->setUniform("u_local_light_position", local_pos);
}

void Light::renderInMenu()
{
	float matrixTranslation[3], matrixRotation[3], matrixScale[3];
	ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(this->model), matrixTranslation, matrixRotation, matrixScale);
	ImGui::DragFloat3("Position", matrixTranslation, 0.1f);
	ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, glm::value_ptr(this->model));

	ImGui::SliderFloat("Intensity", (float*)&this->intensity, 0.f, 50.f);
	ImGui::ColorEdit3("Color", (float*)&this->color);
}