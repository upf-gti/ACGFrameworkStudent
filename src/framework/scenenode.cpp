#include "scenenode.h"

#include "application.h"
#include "utils.h"

#include "ImGuizmo.h"

#include <istream>
#include <fstream>
#include <algorithm>

unsigned int SceneNode::lastNameId = 0;
unsigned int mesh_selected = 0;

SceneNode::SceneNode()
{
	this->type = NODE_BASE;
	this->name = std::string("Node" + std::to_string(this->lastNameId++));
}

SceneNode::SceneNode(const char* name)
{
	this->type = NODE_BASE;
	this->name = name;
}

SceneNode::~SceneNode() { }

void SceneNode::render(Camera* camera)
{
	if (this->material && this->visible)
		this->material->render(this->mesh, this->model, camera);
}

void SceneNode::renderWireframe(Camera* camera)
{
	WireframeMaterial mat = WireframeMaterial();
	mat.render(this->mesh, this->model, camera);
}

void SceneNode::renderInMenu()
{
	// Model edit
	if (ImGui::TreeNode("Model")) 
	{
		float matrixTranslation[3], matrixRotation[3], matrixScale[3];
		ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(this->model), matrixTranslation, matrixRotation, matrixScale);
		ImGui::DragFloat3("Position", matrixTranslation, 0.1f);
		ImGui::DragFloat3("Rotation", matrixRotation, 0.1f);
		ImGui::DragFloat3("Scale", matrixScale, 0.1f);
		ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, glm::value_ptr(this->model));
		
		ImGui::TreePop();
	}

	// Material
	if (this->material && ImGui::TreeNode("Material"))
	{
		material->renderInMenu();
		ImGui::TreePop();
	}
}