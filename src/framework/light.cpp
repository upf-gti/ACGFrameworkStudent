#include "light.h"

#include "ImGuizmo.h"

Light::Light(glm::vec3 position, eLightType type, float intensity, glm::vec4 color)
{
	this->type = NODE_LIGHT;
	this->light_type = type;

	this->name = std::string("Light" + std::to_string(this->lastNameId));
	this->model = glm::translate(this->model, position);
	
	this->color = color;
	this->intensity = intensity;

	this->max_distance;
	this->cast_shadows;

	// create a debug sphere mesh
	this->mesh = Mesh::Get("res/meshes/sphere.obj");
	this->model = glm::scale(this->model, glm::vec3(0.1f));
	this->material = new FlatMaterial();
}

void Light::setUniforms(Shader* shader, const glm::mat4& model)
{
	glm::vec3 position = glm::vec3(this->model[3][0], this->model[3][1], this->model[3][2]);
	glm::vec3 front = glm::vec3(this->model[2][0], this->model[2][1], this->model[2][2]);

	// compute camera position in local coordinates
	glm::mat4 inverseModel = glm::inverse(model);
	glm::vec4 temp = glm::vec4(position, 1.0);
	temp = inverseModel * temp;
	glm::vec3 local_pos = glm::vec3(temp.x / temp.w, temp.y / temp.w, temp.z / temp.w);

	shader->setUniform("u_light_type", this->light_type);
	shader->setUniform("u_light_intensity", this->intensity);
	shader->setUniform("u_light_shininess", this->shininess);
	shader->setUniform("u_light_color", this->color);
	shader->setUniform("u_light_direction", front);
	shader->setUniform("u_light_position", position);
	shader->setUniform("u_local_light_position", local_pos);
}

void Light::renderInMenu()
{
	glm::vec3 front = glm::vec3(model[2][0], model[2][1], model[2][2]);

	if (ImGui::Combo("Light Type", (int*)&this->light_type, "DIRECTIONAL\0POINT\0SPOT", 3))
	{
		// do something
	}

	float matrixTranslation[3], matrixRotation[3], matrixScale[3];
	ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(this->model), matrixTranslation, matrixRotation, matrixScale);
	ImGui::DragFloat3("Position", matrixTranslation, 0.1f);
	ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, glm::value_ptr(this->model));

	ImGui::SliderFloat("Intensity", (float*)&this->intensity, 0.f, 50.f);
	ImGui::SliderFloat("Shininess", (float*)&this->shininess, 0.f, 30.f);
	ImGui::ColorEdit3("Color", (float*)&this->color);

	ImGui::SliderFloat3("Direction", (float*)&front.x, -0.99f, 0.99f);
	ImGui::SliderFloat("Max Distance", (float*)&this->max_distance, 0.f, 1000.f);

	// update the front vector with the new values ?
}