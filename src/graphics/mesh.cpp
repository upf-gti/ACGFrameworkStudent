#include "mesh.h"

#include <cassert>
#include <iostream>
#include <limits>
#include <sys/stat.h>

#include "shader.h"
#include "texture.h"
#include "../framework/includes.h"
#include "../framework/utils.h"
#include "../framework/camera.h"

bool Mesh::use_binary = true;			//checks if there is .wbin, it there is one tries to read it instead of the other file
bool Mesh::auto_upload_to_vram = true;	//uploads the mesh to the GPU VRAM to speed up rendering
bool Mesh::interleave_meshes = true;	//places the geometry in an interleaved array

std::map<std::string, Mesh*> Mesh::sMeshesLoaded;
long Mesh::num_meshes_rendered = 0;
long Mesh::num_triangles_rendered = 0;

#define FORMAT_ASE 1
#define FORMAT_OBJ 2
#define FORMAT_MBIN 3
#define FORMAT_MESH 4

const glm::vec3 corners[] = { {1,1,1},  {1,1,-1},  {1,-1,1},  {1,-1,-1},  {-1,1,1},  {-1,1,-1},  {-1,-1,1},  {-1,-1,-1} };

BoundingBox transformBoundingBox(const glm::mat4 m, const BoundingBox& box)
{
	glm::vec3 box_min(10000000.0f, 1000000.0f, 1000000.0f);
	glm::vec3 box_max(-10000000.0f, -1000000.0f, -1000000.0f);

	for (int i = 0; i < 8; ++i)
	{
		glm::vec3 corner = corners[i];
		corner = box.halfsize * corner;
		corner = corner + box.center;
		corner = m * glm::vec4(corner, 1.f);

		//box_min.setMin(corner);
		if (corner.x < box_min.x) box_min.x = corner.x;
		if (corner.y < box_min.y) box_min.y = corner.y;
		if (corner.z < box_min.z) box_min.z = corner.z;

		//box_max.setMax(corner);
		if (corner.x > box_min.x) box_min.x = corner.x;
		if (corner.y > box_min.y) box_min.y = corner.y;
		if (corner.z > box_min.z) box_min.z = corner.z;
	}

	glm::vec3 halfsize = (box_max - box_min) * 0.5f;
	return BoundingBox(box_max - halfsize, halfsize);
}

Mesh::Mesh()
{
	radius = 0;
	vertices_vbo_id = uvs_vbo_id = uvs1_vbo_id = normals_vbo_id = colors_vbo_id = interleaved_vbo_id = indices_vbo_id = bones_vbo_id = weights_vbo_id = 0;
	collision_model = NULL;
	clear();
}

Mesh::~Mesh()
{
	clear();
}

void Mesh::clear()
{
	//Free VBOs
	if (vertices_vbo_id)
		glDeleteBuffersARB(1, &vertices_vbo_id);
	if (uvs_vbo_id)
		glDeleteBuffersARB(1, &uvs_vbo_id);
	if (normals_vbo_id)
		glDeleteBuffersARB(1, &normals_vbo_id);
	if (colors_vbo_id)
		glDeleteBuffersARB(1, &colors_vbo_id);
	if (interleaved_vbo_id)
		glDeleteBuffersARB(1, &interleaved_vbo_id);
	if (indices_vbo_id)
		glDeleteBuffersARB(1, &indices_vbo_id);
	if (bones_vbo_id)
		glDeleteBuffersARB(1, &bones_vbo_id);
	if (weights_vbo_id)
		glDeleteBuffersARB(1, &weights_vbo_id);
	if (uvs1_vbo_id)
		glDeleteBuffersARB(1, &uvs1_vbo_id);

	//VBOs ids
	vertices_vbo_id = uvs_vbo_id = normals_vbo_id = colors_vbo_id = interleaved_vbo_id = indices_vbo_id = weights_vbo_id = bones_vbo_id = uvs1_vbo_id = 0;

	//buffers
	vertices.clear();
	normals.clear();
	uvs.clear();
	colors.clear();
	interleaved.clear();
	indices.clear();
	bones.clear();
	weights.clear();
	uvs1.clear();
}

int vertex_location = -1;
int normal_location = -1;
int uv_location = -1;
int color_location = -1;
int bones_location = -1;
int weights_location = -1;
int uv1_location = -1;

void Mesh::enableBuffers(Shader* sh)
{
	vertex_location = sh->getAttribLocation("a_vertex");
	assert(vertex_location != -1 && "No a_vertex found in shader");

	if (vertex_location == -1)
		return;

	int spacing = 0;
	int offset_normal = 0;
	int offset_uv = 0;

	if (interleaved.size())
	{
		spacing = sizeof(tInterleaved);
		offset_normal = sizeof(glm::vec3);
		offset_uv = sizeof(glm::vec3) + sizeof(glm::vec3);
	}

	glEnableVertexAttribArray(vertex_location);

	if (vertices_vbo_id || interleaved_vbo_id)
	{
		glBindBuffer(GL_ARRAY_BUFFER, interleaved_vbo_id ? interleaved_vbo_id : vertices_vbo_id);
		glVertexAttribPointer(vertex_location, 3, GL_FLOAT, GL_FALSE, spacing, 0);
	}
	else
		glVertexAttribPointer(vertex_location, 3, GL_FLOAT, GL_FALSE, spacing, interleaved.size() ? &interleaved[0].vertex : &vertices[0]);

	normal_location = -1;
	if (normals.size() || spacing)
	{
		normal_location = sh->getAttribLocation("a_normal");
		if (normal_location != -1)
		{
			glEnableVertexAttribArray(normal_location);
			if (normals_vbo_id || interleaved_vbo_id)
			{
				glBindBuffer(GL_ARRAY_BUFFER, interleaved_vbo_id ? interleaved_vbo_id : normals_vbo_id);
				glVertexAttribPointer(normal_location, 3, GL_FLOAT, GL_FALSE, spacing, (void*)offset_normal);
			}
			else
				glVertexAttribPointer(normal_location, 3, GL_FLOAT, GL_FALSE, spacing, interleaved.size() ? &interleaved[0].normal : &normals[0]);
		}
	}

	uv_location = -1;
	if (uvs.size() || spacing)
	{
		uv_location = sh->getAttribLocation("a_uv");
		if (uv_location != -1)
		{
			glEnableVertexAttribArray(uv_location);
			if (uvs_vbo_id || interleaved_vbo_id)
			{
				glBindBuffer(GL_ARRAY_BUFFER, interleaved_vbo_id ? interleaved_vbo_id : uvs_vbo_id);
				glVertexAttribPointer(uv_location, 2, GL_FLOAT, GL_FALSE, spacing, (void*)offset_uv);
			}
			else
				glVertexAttribPointer(uv_location, 2, GL_FLOAT, GL_FALSE, spacing, interleaved.size() ? &interleaved[0].uv : &uvs[0]);
		}
	}

	uv1_location = -1;
	if (uvs1.size())
	{
		uv1_location = sh->getAttribLocation("a_uv1");
		if (uv1_location != -1)
		{
			glEnableVertexAttribArray(uv1_location);
			if (uvs1_vbo_id)
			{
				glBindBuffer(GL_ARRAY_BUFFER, uvs1_vbo_id);
				glVertexAttribPointer(uv1_location, 2, GL_FLOAT, GL_FALSE, spacing, (void*)NULL);
			}
			else
				glVertexAttribPointer(uv1_location, 2, GL_FLOAT, GL_FALSE, spacing, &uvs1[0]);
		}
	}

	color_location = -1;
	if (colors.size())
	{
		color_location = sh->getAttribLocation("a_color");
		if (color_location != -1)
		{
			glEnableVertexAttribArray(color_location);
			if (colors_vbo_id)
			{
				glBindBuffer(GL_ARRAY_BUFFER, colors_vbo_id);
				glVertexAttribPointer(color_location, 4, GL_FLOAT, GL_FALSE, 0, NULL);
			}
			else
				glVertexAttribPointer(color_location, 4, GL_FLOAT, GL_FALSE, 0, &colors[0]);
		}
	}

	bones_location = -1;
	if (bones.size())
	{
		bones_location = sh->getAttribLocation("a_bones");
		if (bones_location != -1)
		{
			glEnableVertexAttribArray(bones_location);
			if (bones_vbo_id)
			{
				glBindBuffer(GL_ARRAY_BUFFER, bones_vbo_id);
				glVertexAttribPointer(bones_location, 4, GL_UNSIGNED_BYTE, GL_FALSE, 0, NULL);
			}
			else
				glVertexAttribPointer(bones_location, 4, GL_UNSIGNED_BYTE, GL_FALSE, 0, &bones[0]);
		}
	}
	weights_location = -1;
	if (weights.size())
	{
		weights_location = sh->getAttribLocation("a_weights");
		if (weights_location != -1)
		{
			glEnableVertexAttribArray(weights_location);
			if (weights_vbo_id)
			{
				glBindBuffer(GL_ARRAY_BUFFER, weights_vbo_id);
				glVertexAttribPointer(weights_location, 4, GL_FLOAT, GL_FALSE, 0, NULL);
			}
			else
				glVertexAttribPointer(weights_location, 4, GL_FLOAT, GL_FALSE, 0, &weights[0]);
		}
	}

}

void Mesh::render(unsigned int primitive, int submesh_id, int num_instances)
{
	Shader* shader = Shader::current;
	if (!shader || !shader->compiled)
	{
		assert(0 && "no shader or shader not compiled or enabled");
		return;
	}
	assert((interleaved.size() || vertices.size()) && "No vertices in this mesh");

	//bind buffers to attribute locations
	enableBuffers(shader);

	//draw call
	if (submesh_id == -1 && materials.size() > 0) // if there's mesh mtl
	{
		for (int i = 0; i < submeshes.size(); ++i) {
			sSubmeshInfo& submesh = submeshes[i];
			for (uint32_t j = 0; j < submesh.num_draw_calls; ++j) {
				const sSubmeshDrawCallInfo& dc = submesh.draw_calls[j];
				if (materials.count(dc.material) > 0) {
					shader->setUniform("u_Ka", materials[dc.material].Ka);
					shader->setUniform("u_Kd", materials[dc.material].Kd);
					shader->setUniform("u_Ks", materials[dc.material].Ks);
				}
				drawCall(primitive, i, j, num_instances);
			}
		}
	}
	else {
		drawCall(primitive, submesh_id, 0, num_instances);
	}

	//unbind them
	disableBuffers(shader);
}

void Mesh::drawCall(unsigned int primitive, int submesh_id, int draw_call_id, int num_instances)
{
	size_t start = 0; //in primitives
	size_t size = vertices.size();
	if (indices.size())
		size = indices.size();
	else if (interleaved.size())
		size = interleaved.size();

	if (submesh_id > -1)
	{
		assert(submesh_id < submeshes.size() && "this mesh doesnt have as many submeshes");
		sSubmeshInfo& submesh = submeshes[submesh_id];
		sSubmeshDrawCallInfo& dc = submesh.draw_calls[draw_call_id];
		start = dc.start;
		size = dc.length;
	}

	//DRAW
	if (indices.size())
	{
		if (num_instances > 0)
		{
			assert(indices_vbo_id && "indices must be uploaded to the GPU");
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_vbo_id);
			glDrawElementsInstanced(primitive, size * 3, GL_UNSIGNED_INT, (void*)(start * sizeof(glm::vec3)), num_instances);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
		else
		{
			if (indices_vbo_id)
			{
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_vbo_id);
				glDrawElements(primitive, size * 3, GL_UNSIGNED_INT, (void*)(start * sizeof(glm::vec3)));
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
			else
				glDrawElements(primitive, size * 3, GL_UNSIGNED_INT, (void*)(&indices[0] + start)); //no multiply, its a vector3u pointer)
		}
	}
	else
	{
		if (num_instances > 0)
			glDrawArraysInstanced(primitive, start, size, num_instances);
		else
			glDrawArrays(primitive, start, size);
	}

	num_triangles_rendered += static_cast<long>((size / 3) * (num_instances ? num_instances : 1));
	num_meshes_rendered++;
}

void Mesh::disableBuffers(Shader* shader)
{
	glDisableVertexAttribArray(vertex_location);
	if (normal_location != -1) glDisableVertexAttribArray(normal_location);
	if (uv_location != -1) glDisableVertexAttribArray(uv_location);
	if (uv1_location != -1) glDisableVertexAttribArray(uv1_location);
	if (color_location != -1) glDisableVertexAttribArray(color_location);
	if (bones_location != -1) glDisableVertexAttribArray(bones_location);
	if (weights_location != -1) glDisableVertexAttribArray(weights_location);
	glBindBuffer(GL_ARRAY_BUFFER, 0);    //if crashes here, COMMENT THIS LINE ****************************
}

GLuint instances_buffer_id = 0;

//should be faster but in some system it is slower
void Mesh::renderInstanced(unsigned int primitive, const glm::mat4* instanced_models, int num_instances)
{
	if (!num_instances)
		return;

	Shader* shader = Shader::current;
	assert(shader && "shader must be enabled");

	if (instances_buffer_id == 0)
		glGenBuffersARB(1, &instances_buffer_id);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, instances_buffer_id);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, num_instances * sizeof(glm::mat4), instanced_models, GL_STREAM_DRAW_ARB);

	int attribLocation = shader->getAttribLocation("u_model");
	assert(attribLocation != -1 && "shader must have attribute mat4 u_model (not a uniform)");
	if (attribLocation == -1)
		return; //this shader doesnt support instanced model

	//mat4 count as 4 different attributes of vec4... (thanks opengl...)
	for (int k = 0; k < 4; ++k)
	{
		glEnableVertexAttribArray(attribLocation + k);
		int offset = sizeof(float) * 4 * k;
		const uint8_t* addr = (uint8_t*)offset;
		glVertexAttribPointer(attribLocation + k, 4, GL_FLOAT, false, sizeof(glm::mat4x4), addr);
		glVertexAttribDivisor(attribLocation + k, 1); // This makes it instanced!
	}

	//regular render
	render(primitive, -1, num_instances);

	//disable instanced attribs
	for (int k = 0; k < 4; ++k)
	{
		glDisableVertexAttribArray(attribLocation + k);
		glVertexAttribDivisor(attribLocation + k, 0);
	}
}

void Mesh::renderInstanced(unsigned int primitive, const std::vector<glm::vec3> positions, const char* uniform_name)
{
	if (!positions.size())
		return;
	int num_instances = positions.size();

	Shader* shader = Shader::current;
	assert(shader && "shader must be enabled");

	if (instances_buffer_id == 0)
		glGenBuffersARB(1, &instances_buffer_id);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, instances_buffer_id);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, num_instances * sizeof(glm::vec3), &positions[0], GL_STREAM_DRAW_ARB);

	int attribLocation = shader->getAttribLocation(uniform_name);
	assert(attribLocation != -1 && "shader uniform not found");
	if (attribLocation == -1)
		return; //this shader doesnt have instanced uniform

	glEnableVertexAttribArray(attribLocation);
	glVertexAttribPointer(attribLocation, 3, GL_FLOAT, false, sizeof(glm::vec3), 0);
	glVertexAttribDivisor(attribLocation, 1); // This makes it instanced!

	//regular render
	render(primitive, -1, num_instances);

	//disable instanced attribs
	glDisableVertexAttribArray(attribLocation);
	glVertexAttribDivisor(attribLocation, 0);
}


//super obsolete rendering method, do not use
void Mesh::renderFixedPipeline(int primitive)
{
	assert((vertices.size() || interleaved.size()) && "No vertices in this mesh");

	int interleave_offset = interleaved.size() ? sizeof(tInterleaved) : 0;
	int offset_normal = sizeof(glm::vec3);
	int offset_uv = sizeof(glm::vec3) + sizeof(glm::vec3);

	glEnableClientState(GL_VERTEX_ARRAY);

	if (vertices_vbo_id || interleaved_vbo_id)
	{
		glBindBuffer(GL_ARRAY_BUFFER, interleave_offset ? interleaved_vbo_id : vertices_vbo_id);
		glVertexPointer(3, GL_FLOAT, interleave_offset, 0);
	}
	else
		glVertexPointer(3, GL_FLOAT, interleave_offset, interleave_offset ? &interleaved[0].vertex : &vertices[0]);

	if (normals.size() || interleave_offset)
	{
		glEnableClientState(GL_NORMAL_ARRAY);
		if (normals_vbo_id || interleaved_vbo_id)
		{
			glBindBuffer(GL_ARRAY_BUFFER, interleaved_vbo_id ? interleaved_vbo_id : normals_vbo_id);
			glNormalPointer(GL_FLOAT, interleave_offset, (void*)offset_normal);
		}
		else
			glNormalPointer(GL_FLOAT, interleave_offset, interleave_offset ? &interleaved[0].normal : &normals[0]);
	}

	if (uvs.size() || interleave_offset)
	{
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		if (uvs_vbo_id || interleaved_vbo_id)
		{
			glBindBuffer(GL_ARRAY_BUFFER, interleaved_vbo_id ? interleaved_vbo_id : uvs_vbo_id);
			glTexCoordPointer(2, GL_FLOAT, interleave_offset, (void*)offset_uv);
		}
		else
			glTexCoordPointer(2, GL_FLOAT, interleave_offset, interleave_offset ? &interleaved[0].uv : &uvs[0]);
	}

	if (colors.size())
	{
		glEnableClientState(GL_COLOR_ARRAY);
		if (colors_vbo_id)
		{
			glBindBuffer(GL_ARRAY_BUFFER, colors_vbo_id);
			glColorPointer(4, GL_FLOAT, 0, NULL);
		}
		else
			glColorPointer(4, GL_FLOAT, 0, &colors[0]);
	}

	int size = (int)vertices.size();
	if (!size)
		size = (int)interleaved.size();

	glDrawArrays(primitive, 0, (GLsizei)size);
	glDisableClientState(GL_VERTEX_ARRAY);
	if (normals.size())
		glDisableClientState(GL_NORMAL_ARRAY);
	if (uvs.size())
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if (colors.size())
		glDisableClientState(GL_COLOR_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, 0); //if it crashes, comment this line
}

// TODO
//void Mesh::renderAnimated(unsigned int primitive, Skeleton* skeleton)
//{
//	if (skeleton)
//	{
//		Shader* shader = Shader::current;
//		std::vector<Matrix44> bone_matrices;
//		assert(bones.size());
//		int bones_loc = shader->getUniformLocation("u_bones");
//		if (bones_loc != -1)
//		{
//			skeleton->computeFinalBoneMatrices(bone_matrices, this);
//			shader->setUniform("u_bones", bone_matrices);
//		}
//	}
//
//	render(primitive);
//}

void Mesh::uploadToVRAM()
{
	assert(vertices.size() || interleaved.size());

	if (glGenBuffersARB == 0)
	{
		std::cout << "Error: your graphics cards dont support VBOs. Sorry." << std::endl;
		exit(0);
	}

	if (interleaved.size())
	{
		// Vertex,Normal,UV
		if (interleaved_vbo_id == 0)
			glGenBuffersARB(1, &interleaved_vbo_id);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, interleaved_vbo_id);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, interleaved.size() * sizeof(tInterleaved), &interleaved[0], GL_STATIC_DRAW_ARB);
	}
	else
	{
		// Vertices
		if (vertices_vbo_id == 0)
			glGenBuffersARB(1, &vertices_vbo_id);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, vertices_vbo_id);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW_ARB);

		// UVs
		if (uvs.size())
		{
			if (uvs_vbo_id == 0)
				glGenBuffersARB(1, &uvs_vbo_id);
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, uvs_vbo_id);
			glBufferDataARB(GL_ARRAY_BUFFER_ARB, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW_ARB);
		}

		// Normals
		if (normals.size())
		{
			if (normals_vbo_id == 0)
				glGenBuffersARB(1, &normals_vbo_id);
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, normals_vbo_id);
			glBufferDataARB(GL_ARRAY_BUFFER_ARB, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW_ARB);
		}
	}

	// UVs
	if (uvs1.size())
	{
		if (uvs1_vbo_id == 0)
			glGenBuffersARB(1, &uvs1_vbo_id);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, uvs1_vbo_id);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, uvs1.size() * sizeof(glm::vec2), &uvs1[0], GL_STATIC_DRAW_ARB);
	}

	// Colors
	if (colors.size())
	{
		if (colors_vbo_id == 0)
			glGenBuffersARB(1, &colors_vbo_id);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, colors_vbo_id);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, colors.size() * sizeof(glm::vec4), &colors[0], GL_STATIC_DRAW_ARB);
	}

	if (bones.size())
	{
		if (bones_vbo_id == 0)
			glGenBuffersARB(1, &bones_vbo_id);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, bones_vbo_id);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, bones.size() * sizeof(glm::uvec4), &bones[0], GL_STATIC_DRAW_ARB);
	}
	if (weights.size())
	{
		if (weights_vbo_id == 0)
			glGenBuffersARB(1, &weights_vbo_id);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, weights_vbo_id);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, weights.size() * sizeof(glm::vec4), &weights[0], GL_STATIC_DRAW_ARB);
	}

	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

	// Indices
	if (indices.size())
	{
		if (indices_vbo_id == 0)
			glGenBuffersARB(1, &indices_vbo_id);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, indices_vbo_id);
		glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(glm::uvec3), &indices[0], GL_STATIC_DRAW_ARB);
	}
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);



	checkGLErrors();

	//clear buffers to save memory
}

bool Mesh::interleaveBuffers()
{
	if (!vertices.size() || !normals.size() || !uvs.size())
		return false;

	assert(vertices.size() == normals.size() && normals.size() == uvs.size());

	interleaved.resize(vertices.size());

	for (unsigned int i = 0; i < vertices.size(); ++i)
	{
		interleaved[i].vertex = vertices[i];
		interleaved[i].normal = normals[i];
		interleaved[i].uv = uvs[i];
	}

	vertices.resize(0);
	normals.resize(0);
	uvs.resize(0);

	return true;
}

struct sMeshInfo
{
	int version = 0;
	int header_bytes = 0;
	size_t size = 0;
	size_t num_indices = 0;
	glm::vec3 aabb_min;
	glm::vec3 aabb_max;
	glm::vec3 center;
	glm::vec3 halfsize;
	float radius = 0.0;
	size_t num_bones = 0;
	size_t num_submeshes = 0;
	glm::mat4 bind_matrix;
	char streams[8]; //Vertex/Interlaved|Normal|Uvs|Color|Indices|Bones|Weights|Extra|Uvs1
	char extra[32]; //unused
};

bool Mesh::readBin(const char* filename)
{
	FILE* f;
	assert(filename);

	struct stat stbuffer;

	stat(filename, &stbuffer);
	f = fopen(filename, "rb");
	if (f == NULL)
		return false;

	unsigned int size = (unsigned int)stbuffer.st_size;
	char* data = new char[size];
	fread(data, size, 1, f);
	fclose(f);

	//watermark
	if (memcmp(data, "MBIN", 4) != 0)
	{
		std::cout << "[ERROR] loading BIN: invalid content: " << filename << std::endl;
		return false;
	}

	char* pos = data + 4;
	sMeshInfo info;
	memcpy(&info, pos, sizeof(sMeshInfo));
	pos += sizeof(sMeshInfo);

	if (info.version != MESH_BIN_VERSION || info.header_bytes != sizeof(sMeshInfo))
	{
		std::cout << "[WARN] loading BIN: old version: " << filename << std::endl;
		return false;
	}

	if (info.streams[0] == 'I')
	{
		interleaved.resize(info.size);
		memcpy((void*)&interleaved[0], pos, sizeof(tInterleaved) * info.size);
		pos += sizeof(tInterleaved) * info.size;
	}
	else if (info.streams[0] == 'V')
	{
		vertices.resize(info.size);
		memcpy((void*)&vertices[0], pos, sizeof(glm::vec3) * info.size);
		pos += sizeof(glm::vec3) * info.size;
	}

	if (info.streams[1] == 'N')
	{
		normals.resize(info.size);
		memcpy((void*)&normals[0], pos, sizeof(glm::vec3) * info.size);
		pos += sizeof(glm::vec3) * info.size;
	}

	if (info.streams[2] == 'U')
	{
		uvs.resize(info.size);
		memcpy((void*)&uvs[0], pos, sizeof(glm::vec2) * info.size);
		pos += sizeof(glm::vec2) * info.size;
	}

	if (info.streams[3] == 'C')
	{
		colors.resize(info.size);
		memcpy((void*)&colors[0], pos, sizeof(glm::vec4) * info.size);
		pos += sizeof(glm::vec4) * info.size;
	}

	if (info.streams[4] == 'I')
	{
		indices.resize(info.num_indices);
		memcpy((void*)&indices[0], pos, sizeof(glm::vec3) * info.num_indices);
		pos += sizeof(glm::vec3) * info.num_indices;
	}

	if (info.streams[5] == 'B')
	{
		bones.resize(info.size);
		memcpy((void*)&bones[0], pos, sizeof(glm::vec4) * info.size);
		pos += sizeof(glm::vec4) * info.size;
	}

	if (info.streams[6] == 'W')
	{
		weights.resize(info.size);
		memcpy((void*)&weights[0], pos, sizeof(glm::vec4) * info.size);
		pos += sizeof(glm::vec4) * info.size;
	}

	if (info.streams[7] == 'u')
	{
		uvs1.resize(info.size);
		memcpy((void*)&uvs1[0], pos, sizeof(glm::vec2) * info.size);
		pos += sizeof(glm::vec2) * info.size;
	}

	if (info.num_bones)
	{
		bones_info.resize(info.num_bones);
		memcpy((void*)&bones_info[0], pos, sizeof(BoneInfo) * info.num_bones);
		pos += sizeof(BoneInfo) * info.num_bones;
	}

	aabb_max = info.aabb_max;
	aabb_min = info.aabb_min;
	box.center = info.center;
	box.halfsize = info.halfsize;
	radius = info.radius;
	bind_matrix = info.bind_matrix;

	submeshes.resize(info.num_submeshes);
	if (info.num_submeshes)
	{
		memcpy(&submeshes[0], pos, sizeof(sSubmeshInfo) * info.num_submeshes);
		pos += sizeof(sSubmeshInfo) * info.num_submeshes;
	}

	// if the mtl is not specified in the obj but it's needed
	if (!materials.size()) {
		std::string mesh_name = filename;
		mesh_name = mesh_name.substr(0, mesh_name.size() - 5);

		std::string ext = mesh_name.substr(mesh_name.find_last_of(".") + 1);
		if (ext == "obj" || ext == "OBJ") {
			replace(mesh_name, ".obj", ".mtl");
			if (!parseMTL(mesh_name.c_str()))
				std::cerr << "MTL file not found: " << mesh_name.c_str() << std::endl;
		}
	}

	//createCollisionModel();
	return true;
}

bool Mesh::writeBin(const char* filename)
{
	assert(vertices.size() || interleaved.size());
	std::string s_filename = filename;
	s_filename += ".mbin";

	FILE* f = fopen(s_filename.c_str(), "wb");
	if (f == NULL)
	{
		std::cout << "[ERROR] cannot write mesh BIN: " << s_filename.c_str() << std::endl;
		return false;
	}

	//watermark
	fwrite("MBIN", sizeof(char), 4, f);

	sMeshInfo info;
	memset(&info, 0, sizeof(info));
	info.version = MESH_BIN_VERSION;
	info.header_bytes = sizeof(sMeshInfo);
	info.size = interleaved.size() ? interleaved.size() : vertices.size();
	info.num_indices = indices.size();
	info.aabb_max = aabb_max;
	info.aabb_min = aabb_min;
	info.center = box.center;
	info.halfsize = box.halfsize;
	info.radius = radius;
	info.num_bones = bones_info.size();
	info.bind_matrix = bind_matrix;
	info.num_submeshes = submeshes.size();

	info.streams[0] = interleaved.size() ? 'I' : 'V';
	info.streams[1] = normals.size() ? 'N' : ' ';
	info.streams[2] = uvs.size() ? 'U' : ' ';
	info.streams[3] = colors.size() ? 'C' : ' ';
	info.streams[4] = indices.size() ? 'I' : ' ';
	info.streams[5] = bones.size() ? 'B' : ' ';
	info.streams[6] = weights.size() ? 'W' : ' ';
	info.streams[7] = uvs1.size() ? 'u' : ' ';

	//write info
	fwrite((void*)&info, sizeof(sMeshInfo), 1, f);

	//write streams
	if (interleaved.size())
		fwrite((void*)&interleaved[0], interleaved.size() * sizeof(tInterleaved), 1, f);
	else
	{
		fwrite((void*)&vertices[0], vertices.size() * sizeof(glm::vec3), 1, f);
		if (normals.size())
			fwrite((void*)&normals[0], normals.size() * sizeof(glm::vec3), 1, f);
		if (uvs.size())
			fwrite((void*)&uvs[0], uvs.size() * sizeof(glm::vec2), 1, f);
	}

	if (colors.size())
		fwrite((void*)&colors[0], colors.size() * sizeof(glm::vec4), 1, f);

	if (indices.size())
		fwrite((void*)&indices[0], indices.size() * sizeof(glm::vec3), 1, f);

	if (bones.size())
		fwrite((void*)&bones[0], bones.size() * sizeof(glm::vec4), 1, f);
	if (weights.size())
		fwrite((void*)&weights[0], weights.size() * sizeof(glm::vec4), 1, f);
	if (bones_info.size())
		fwrite((void*)&bones_info[0], bones_info.size() * sizeof(BoneInfo), 1, f);
	if (uvs1.size())
		fwrite((void*)&uvs1[0], uvs1.size() * sizeof(glm::vec2), 1, f);

	if (submeshes.size())
		fwrite((void*)&submeshes[0], submeshes.size() * sizeof(sSubmeshInfo), 1, f);

	fclose(f);
	return true;
}

bool Mesh::parseMTL(const char* filename)
{
	struct stat stbuffer;

	FILE* f = fopen(filename, "rb");
	if (f == NULL)
		return false;

	stat(filename, &stbuffer);

	unsigned int size = stbuffer.st_size;
	char* data = new char[size + 1];
	fread(data, size, 1, f);
	fclose(f);
	data[size] = 0;

	char* pos = data;
	char line[255];
	int i = 0;

	bool parsingMaterial = false;
	std::string material_name;

	sMaterialInfo info;

	//parse file
	while (*pos != 0)
	{
		if (*pos == '\n') pos++;
		if (*pos == '\r') pos++;

		//read one line
		i = 0;
		while (i < 255 && pos[i] != '\n' && pos[i] != '\r' && pos[i] != 0) i++;
		memcpy(line, pos, i);
		line[i] = 0;
		pos = pos + i;

		//std::cout << "Line: \"" << line << "\"" << std::endl;
		if (*line == '#' || *line == 0) continue; //comment

		//tokenize line
		std::vector<std::string> tokens = tokenize(line, " ");

		if (tokens.empty()) continue;

		if (tokens[0] == "Ka")
		{
			info.Ka = glm::vec3((float)atof(tokens[1].c_str()), (float)atof(tokens[2].c_str()), (float)atof(tokens[3].c_str()));
		}
		else if (tokens[0] == "Kd")
		{
			info.Kd = glm::vec3((float)atof(tokens[1].c_str()), (float)atof(tokens[2].c_str()), (float)atof(tokens[3].c_str()));
		}
		else if (tokens[0] == "Ks")
		{
			info.Ks = glm::vec3((float)atof(tokens[1].c_str()), (float)atof(tokens[2].c_str()), (float)atof(tokens[3].c_str()));
		}
		else if (tokens[0] == "newmtl") //material file
		{
			if (parsingMaterial) {
				materials[material_name] = info;
			}
			parsingMaterial = true;
			material_name = tokens[1];
		}
	}

	materials[material_name] = info;

	std::cout << "[MTL] ";

	return true;
}

void parseFromText(glm::vec3& v, const char* text, const char separator)
{
	int pos = 0;
	char num[255];
	const char* start = text;
	const char* current = text;

	while (1)
	{
		if (*current == separator || (*current == '\0' && current != text))
		{
			strncpy(num, start, current - start);
			num[current - start] = '\0';
			start = current + 1;
			if (num[0] != 'x')
				switch (pos)
				{
				case 0: v.x = (float)atof(num); break;
				case 1: v.y = (float)atof(num); break;
				case 2: v.z = (float)atof(num); break;
				default: return; break;
				}

			++pos;
			if (*current == '\0')
				break;
		}

		++current;
	}
};

bool Mesh::loadOBJ(const char* filename)
{
	struct stat stbuffer;

	FILE* f = fopen(filename, "rb");
	if (f == NULL)
	{
		std::cerr << "File not found: " << filename << std::endl;
		return false;
	}

	stat(filename, &stbuffer);

	unsigned int size = stbuffer.st_size;
	char* data = new char[size + 1];
	fread(data, size, 1, f);
	fclose(f);
	data[size] = 0;

	char* pos = data;
	char line[255];
	int i = 0;

	std::vector<glm::vec3> indexed_positions;
	std::vector<glm::vec4> indexed_colors;
	std::vector<glm::vec3> indexed_normals;
	std::vector<glm::vec2> indexed_uvs;

	const float max_float = 10000000;
	const float min_float = -10000000;
	aabb_min = glm::vec3(max_float, max_float, max_float);
	aabb_max = glm::vec3(min_float, min_float, min_float);

	unsigned int vertex_i = 0;
	unsigned int submesh_draw_calls = 0;

	sSubmeshInfo submesh_info;
	memset(&submesh_info, 0, sizeof(submesh_info));

	sSubmeshDrawCallInfo submesh_dc_info;
	memset(&submesh_dc_info, 0, sizeof(submesh_dc_info));
	submesh_dc_info.start = 0;
	size_t last_submesh_vertex = 0;

	//parse file
	while (*pos != 0)
	{
		if (*pos == '\n') pos++;
		if (*pos == '\r') pos++;

		//read one line
		i = 0;
		while (i < 255 && pos[i] != '\n' && pos[i] != '\r' && pos[i] != 0) i++;
		memcpy(line, pos, i);
		line[i] = 0;
		pos = pos + i;

		//std::cout << "Line: \"" << line << "\"" << std::endl;
		if (*line == '#' || *line == 0) continue; //comment

		//tokenize line
		std::vector<std::string> tokens = tokenize(line, " ");

		if (tokens.empty()) continue;

		if (tokens[0] == "mtllib") //material file
		{
			std::string mesh_path = filename;
			size_t lastPath = mesh_path.find_last_of('/');
			std::string path = mesh_path.substr(0, lastPath) + '/' + tokens[1];
			if (!parseMTL(path.c_str()))
				std::cerr << "MTL file not found: " << path.c_str() << std::endl;
		}
		else if (tokens[0] == "v")
		{
			glm::vec3 v((float)atof(tokens[1].c_str()), (float)atof(tokens[2].c_str()), (float)atof(tokens[3].c_str()));
			indexed_positions.push_back(v);

			//aabb_min.setMin(v);
			if (v.x < aabb_min.x) aabb_min.x = v.x;
			if (v.y < aabb_min.y) aabb_min.y = v.y;
			if (v.z < aabb_min.z) aabb_min.z = v.z;

			//aabb_max.setMax(v);
			if (v.x > aabb_max.x) aabb_max.x = v.x;
			if (v.y > aabb_max.y) aabb_max.y = v.y;
			if (v.z > aabb_max.z) aabb_max.z = v.z;

			if (tokens.size() > 4) {
				glm::vec4 color((float)atof(tokens[4].c_str()), (float)atof(tokens[5].c_str()), (float)atof(tokens[6].c_str()), 1.0);
				indexed_colors.push_back(color);
			}
		}
		else if (tokens[0] == "vt" && tokens.size() >= 3)
		{
			glm::vec2 v((float)atof(tokens[1].c_str()), (float)atof(tokens[2].c_str()));
			indexed_uvs.push_back(v);
		}
		else if (tokens[0] == "vn" && tokens.size() == 4)
		{
			glm::vec3 v((float)atof(tokens[1].c_str()), (float)atof(tokens[2].c_str()), (float)atof(tokens[3].c_str()));
			indexed_normals.push_back(v);
		}
		else if (tokens[0] == "o") // submesh
		{
			if (submesh_draw_calls > 0)
			{
				// Store last submesh drawcall
				submesh_dc_info.length = vertices.size() - submesh_dc_info.start;
				last_submesh_vertex = vertices.size();
				submesh_info.draw_calls[submesh_draw_calls] = submesh_dc_info;
				submesh_dc_info.start = last_submesh_vertex;

				// Store submesh
				submesh_info.num_draw_calls = submesh_draw_calls + 1;
				submeshes.push_back(submesh_info);

				// New submesh
				memset(&submesh_info, 0, sizeof(submesh_info));
				strcpy(submesh_info.name, tokens[1].c_str());
				submesh_draw_calls = 0;
			}
			else
				strcpy(submesh_info.name, tokens[1].c_str());
		}
		else if (tokens[0] == "usemtl") //surface? it appears one time before the faces
		{
			if (last_submesh_vertex != vertices.size())
			{
				// Store draw call
				submesh_dc_info.length = vertices.size() - submesh_dc_info.start;
				last_submesh_vertex = vertices.size();
				submesh_info.draw_calls[submesh_draw_calls] = submesh_dc_info;
				submesh_draw_calls++;

				// New draw call
				memset(&submesh_dc_info, 0, sizeof(submesh_dc_info));
				strcpy(submesh_dc_info.material, tokens[1].c_str());
				submesh_dc_info.start = last_submesh_vertex;
			}
			else
				strcpy(submesh_dc_info.material, tokens[1].c_str());
		}
		else if (tokens[0] == "g") //surface? it appears one time before the faces
		{

		}
		else if (tokens[0] == "f" && tokens.size() >= 4)
		{
			glm::vec3 v1, v2, v3;
			parseFromText(v1, tokens[1].c_str(), '/');

			for (unsigned int iPoly = 2; iPoly < tokens.size() - 1; iPoly++)
			{
				parseFromText(v2, tokens[iPoly].c_str(), '/');
				parseFromText(v3, tokens[iPoly + 1].c_str(), '/');

				vertices.push_back(indexed_positions[(unsigned int)(v1.x) - 1]);
				vertices.push_back(indexed_positions[(unsigned int)(v2.x) - 1]);
				vertices.push_back(indexed_positions[(unsigned int)(v3.x) - 1]);

				if (!indexed_colors.empty()) {
					colors.push_back(indexed_colors[(unsigned int)(v1.x) - 1]);
					colors.push_back(indexed_colors[(unsigned int)(v2.x) - 1]);
					colors.push_back(indexed_colors[(unsigned int)(v3.x) - 1]);
				}

				//triangles.push_back( VECTOR_INDICES_TYPE(vertex_i, vertex_i+1, vertex_i+2) ); //not needed
				vertex_i += 3;

				if (indexed_uvs.size() > 0)
				{
					uvs.push_back(indexed_uvs[(unsigned int)(v1.y) - 1]);
					uvs.push_back(indexed_uvs[(unsigned int)(v2.y) - 1]);
					uvs.push_back(indexed_uvs[(unsigned int)(v3.y) - 1]);
				}

				if (indexed_normals.size() > 0)
				{
					normals.push_back(indexed_normals[(unsigned int)(v1.z) - 1]);
					normals.push_back(indexed_normals[(unsigned int)(v2.z) - 1]);
					normals.push_back(indexed_normals[(unsigned int)(v3.z) - 1]);
				}
			}
		}
	}

	// if the mtl is not specified in the obj but it's needed
	if (!materials.size()) {
		std::string mesh_name = filename;
		replace(mesh_name, ".obj", ".mtl");
		if (!parseMTL(mesh_name.c_str()))
			std::cerr << "MTL file not found: " << mesh_name.c_str() << std::endl;
	}

	box.center = (aabb_max + aabb_min) * 0.5f;
	box.halfsize = (aabb_max - box.center);
	radius = (float)fmax(aabb_max.length(), aabb_min.length());

	submesh_dc_info.length = vertices.size() - last_submesh_vertex;
	submesh_info.draw_calls[submesh_draw_calls] = submesh_dc_info;
	submesh_info.num_draw_calls = submesh_draw_calls + 1;
	submeshes.push_back(submesh_info);
	return true;
}

bool Mesh::loadMESH(const char* filename)
{
	struct stat stbuffer;

	FILE* f = fopen(filename, "rb");
	if (f == NULL)
	{
		std::cerr << "File not found: " << filename << std::endl;
		return false;
	}
	stat(filename, &stbuffer);

	unsigned int size = stbuffer.st_size;
	char* data = new char[size + 1];
	fread(data, size, 1, f);
	fclose(f);
	data[size] = 0;
	char* pos = data;
	char word[255];

	while (*pos)
	{
		char type = *pos;
		pos++;
		if (type == '-') //buffer
		{
			pos = fetchWord(pos, word);
			std::string str(word);
			if (str == "vertices")
				pos = fetchBufferVec3(pos, vertices);
			else if (str == "normals")
				pos = fetchBufferVec3(pos, normals);
			else if (str == "coords")
				pos = fetchBufferVec2(pos, uvs);
			else if (str == "colors")
				pos = fetchBufferVec4(pos, colors);
			else if (str == "bone_indices")
				pos = fetchBufferVec4ub(pos, bones);
			else if (str == "weights")
				pos = fetchBufferVec4(pos, weights);
			else
				pos = fetchEndLine(pos);
		}
		else if (type == '*') //buffer
		{
			pos = fetchWord(pos, word);
			pos = fetchBufferVec3u(pos, indices);
		}
		else if (type == '@') //info
		{
			pos = fetchWord(pos, word);
			std::string str(word);
			if (str == "bones")
			{
				pos = fetchWord(pos, word);
				bones_info.resize(static_cast<size_t>(atof(word)));
				for (int j = 0; j < bones_info.size(); ++j)
				{
					pos = fetchWord(pos, word);
					strcpy(bones_info[j].name, word);
					pos = fetchMatrix44(pos, bones_info[j].bind_pose);
				}
			}
			else if (str == "bind_matrix")
				pos = fetchMatrix44(pos, bind_matrix);
			else
				pos = fetchEndLine(pos);
		}
		else
			pos = fetchEndLine(pos);
	}

	delete[] data;

	return true;
}

void Mesh::createCube()
{
	const float _verts[] = { -1, 1, -1, -1, -1, +1, -1, 1, 1,    -1, 1, -1, -1, -1, -1, -1, -1, +1,     1, 1, -1,  1, 1, 1,  1, -1, +1,     1, 1, -1,   1, -1, +1,   1, -1, -1,    -1, 1, 1,  1, -1, 1,  1, 1, 1,    -1, 1, 1, -1,-1,1,  1, -1, 1,    -1,1,-1, 1,1,-1,  1,-1,-1,   -1,1,-1, 1,-1,-1, -1,-1,-1,   -1,1,-1, 1,1,1, 1,1,-1,    -1,1,-1, -1,1,1, 1,1,1,    -1,-1,-1, 1,-1,-1, 1,-1,1,   -1,-1,-1, 1,-1,1, -1,-1,1 };
	const float _uvs[] = { 0,  1, 1, 0, 1, 1,			 	     0, 1,       0,  0,      1,  0,        0, 1,      1, 1,      1, 0,         0, 1,        1, 0,        0, 0,          0, 1, 1, 0, 1, 1,               0, 1,  0, 0,  1,  0,              0,1,  1,1, 1,0,              0,1,    1,0,    0,0,           0,0, 1,1, 1,0,           0,0,    0,1,   1,1,        0,0, 1,0, 1,1,              0,0, 1,1, 0,1 };

	vertices.resize(6 * 2 * 3);
	uvs.resize(6 * 2 * 3);
	memcpy(&vertices[0], _verts, sizeof(glm::vec3) * vertices.size());
	memcpy(&uvs[0], _uvs, sizeof(glm::vec2) * uvs.size());

	box.center = glm::vec3(0, 0, 0);
	box.halfsize = glm::vec3(1, 1, 1);
	radius = (float)box.halfsize.length();

	updateBoundingBox();
}

void Mesh::createWireBox()
{
	const float _verts[] = { -1,-1,-1,  1,-1,-1,  -1,1,-1,  1,1,-1, -1,-1,1,  1,-1,1, -1,1,1,  1,1,1,    -1,-1,-1, -1,1,-1, 1,-1,-1, 1,1,-1, -1,-1,1, -1,1,1, 1,-1,1, 1,1,1,   -1,-1,-1, -1,-1,1, 1,-1,-1, 1,-1,1, -1,1,-1, -1,1,1, 1,1,-1, 1,1,1 };
	vertices.resize(24);
	memcpy(&vertices[0], _verts, sizeof(glm::vec3) * vertices.size());

	box.center = glm::vec3(0, 0, 0);
	box.halfsize = glm::vec3(1, 1, 1);
	radius = (float)box.halfsize.length();
}

void Mesh::createQuad(float center_x, float center_y, float w, float h, bool flip_uvs)
{
	vertices.clear();
	normals.clear();
	uvs.clear();
	colors.clear();

	//create six vertices (3 for upperleft triangle and 3 for lowerright)

	vertices.push_back(glm::vec3(center_x + w * 0.5f, center_y + h * 0.5f, 0.0f));
	vertices.push_back(glm::vec3(center_x - w * 0.5f, center_y - h * 0.5f, 0.0f));
	vertices.push_back(glm::vec3(center_x + w * 0.5f, center_y - h * 0.5f, 0.0f));
	vertices.push_back(glm::vec3(center_x - w * 0.5f, center_y + h * 0.5f, 0.0f));
	vertices.push_back(glm::vec3(center_x - w * 0.5f, center_y - h * 0.5f, 0.0f));
	vertices.push_back(glm::vec3(center_x + w * 0.5f, center_y + h * 0.5f, 0.0f));

	//texture coordinates
	uvs.push_back(glm::vec2(1.0f, flip_uvs ? 0.0f : 1.0f));
	uvs.push_back(glm::vec2(0.0f, flip_uvs ? 1.0f : 0.0f));
	uvs.push_back(glm::vec2(1.0f, flip_uvs ? 1.0f : 0.0f));
	uvs.push_back(glm::vec2(0.0f, flip_uvs ? 0.0f : 1.0f));
	uvs.push_back(glm::vec2(0.0f, flip_uvs ? 1.0f : 0.0f));
	uvs.push_back(glm::vec2(1.0f, flip_uvs ? 0.0f : 1.0f));

	//all of them have the same normal
	normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
	normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
	normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
	normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
	normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
	normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
}


void Mesh::createPlane(float size)
{
	vertices.clear();
	normals.clear();
	uvs.clear();
	colors.clear();

	//create six vertices (3 for upperleft triangle and 3 for lowerright)

	vertices.push_back(glm::vec3(size, 0, size));
	vertices.push_back(glm::vec3(size, 0, -size));
	vertices.push_back(glm::vec3(-size, 0, -size));
	vertices.push_back(glm::vec3(-size, 0, size));
	vertices.push_back(glm::vec3(size, 0, size));
	vertices.push_back(glm::vec3(-size, 0, -size));

	//all of them have the same normal
	normals.push_back(glm::vec3(0, 1, 0));
	normals.push_back(glm::vec3(0, 1, 0));
	normals.push_back(glm::vec3(0, 1, 0));
	normals.push_back(glm::vec3(0, 1, 0));
	normals.push_back(glm::vec3(0, 1, 0));
	normals.push_back(glm::vec3(0, 1, 0));

	//texture coordinates
	uvs.push_back(glm::vec2(1, 1));
	uvs.push_back(glm::vec2(1, 0));
	uvs.push_back(glm::vec2(0, 0));
	uvs.push_back(glm::vec2(0, 1));
	uvs.push_back(glm::vec2(1, 1));
	uvs.push_back(glm::vec2(0, 0));

	box.center = glm::vec3(0, 0, 0);
	box.halfsize = glm::vec3(size, 0, size);
	radius = (float)box.halfsize.length();
}

void Mesh::createSubdividedPlane(float size, int subdivisions, bool centered)
{
	float isize = static_cast<float>(size / (double)(subdivisions));
	//float hsize = centered ? size * -0.5f : 0.0f;
	float iuv = static_cast<float>(1 / (double)(subdivisions * size));
	float sub_size = 1.0f / subdivisions;
	vertices.clear();

	for (int x = 0; x < subdivisions; ++x)
	{
		for (int z = 0; z < subdivisions; ++z)
		{

			glm::vec2 offset(sub_size * z, sub_size * x);
			glm::vec3 offset2(isize * x, 0.0f, isize * z);

			vertices.push_back(glm::vec3(isize, 0.0f, isize) + offset2);
			vertices.push_back(glm::vec3(isize, 0.0f, 0.0f) + offset2);
			vertices.push_back(glm::vec3(0.0f, 0.0f, 0.0f) + offset2);

			uvs.push_back(glm::vec2(sub_size, sub_size) + offset);
			uvs.push_back(glm::vec2(0.0f, sub_size) + offset);
			uvs.push_back(glm::vec2(0.0f, 0.0f) + offset);

			vertices.push_back(glm::vec3(isize, 0.0f, isize) + offset2);
			vertices.push_back(glm::vec3(0.0f, 0.0f, 0.0f) + offset2);
			vertices.push_back(glm::vec3(0.0f, 0.0f, isize) + offset2);

			uvs.push_back(glm::vec2(sub_size, sub_size) + offset);
			uvs.push_back(glm::vec2(0.0f, 0.0f) + offset);
			uvs.push_back(glm::vec2(sub_size, 0.0f) + offset);
		}
	}
	if (centered)
		box.center = glm::vec3(0.0f, 0.0f, 0.0f);
	else
		box.center = glm::vec3(size * 0.5f, 0.0f, size * 0.5f);

	box.halfsize = glm::vec3(size * 0.5f, 0.0f, size * 0.5f);
	radius = static_cast<float>(box.halfsize.length());
}

void Mesh::displace(Image* heightmap, float altitude)
{
	assert(heightmap && heightmap->data && "image without data");
	assert(uvs.size() && "cannot displace without uvs");

	bool is_interleaved = interleaved.size() != 0;
	int num = is_interleaved ? interleaved.size() : vertices.size();
	assert(num && "no vertices found");

	for (int i = 0; i < num; ++i)
	{
		glm::vec2& uv = uvs[i];
		glm::vec4 c = heightmap->getPixelInterpolated(uv.x * heightmap->width, uv.y * heightmap->height);
		if (is_interleaved)
			interleaved[i].vertex.y = (c.x / 255.0f) * altitude;
		else
			vertices[i].y = (c.x / 255.0f) * altitude;
	}
	box.center.y += altitude * 0.5f;
	box.halfsize.y += altitude * 0.5f;
	radius = static_cast<float>(box.halfsize.length());
}


void Mesh::createGrid(float dist)
{
	int num_lines = 2000;
	glm::vec4 color(0.5f, 0.5f, 0.5f, 1.0f);

	for (float i = num_lines * -0.5f; i <= num_lines * 0.5f; ++i)
	{
		vertices.push_back(glm::vec3(i * dist, 0.0f, dist * num_lines * -0.5f));
		vertices.push_back(glm::vec3(i * dist, 0.0f, dist * num_lines * +0.5f));
		vertices.push_back(glm::vec3(dist * num_lines * 0.5f, 0.0f, i * dist));
		vertices.push_back(glm::vec3(dist * num_lines * -0.5f, 0.0f, i * dist));

		glm::vec4 color = int(i) % 10 == 0 ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) : glm::vec4(0.75f, 0.75f, 0.75f, 0.5f);
		colors.push_back(color);
		colors.push_back(color);
		colors.push_back(color);
		colors.push_back(color);
	}
}

void Mesh::updateBoundingBox()
{
	if (vertices.size())
	{
		aabb_max = aabb_min = vertices[0];
		for (int i = 1; i < vertices.size(); ++i)
		{
			//aabb_min.setMin(vertices[i]);
			if (vertices[i].x < aabb_min.x) aabb_min.x = vertices[i].x;
			if (vertices[i].y < aabb_min.y) aabb_min.y = vertices[i].y;
			if (vertices[i].z < aabb_min.z) aabb_min.z = vertices[i].z;

			//aabb_max.setMax(vertices[i]);
			if (vertices[i].x > aabb_max.x) aabb_max.x = vertices[i].x;
			if (vertices[i].y > aabb_max.y) aabb_max.y = vertices[i].y;
			if (vertices[i].z > aabb_max.z) aabb_max.z = vertices[i].z;
		}
	}
	else if (interleaved.size())
	{
		aabb_max = aabb_min = interleaved[0].vertex;
		for (int i = 1; i < interleaved.size(); ++i)
		{
			//aabb_min.setMin(interleaved[i].vertex);
			if (interleaved[i].vertex.x < aabb_min.x) aabb_min.x = interleaved[i].vertex.x;
			if (interleaved[i].vertex.y < aabb_min.y) aabb_min.y = interleaved[i].vertex.y;
			if (interleaved[i].vertex.z < aabb_min.z) aabb_min.z = interleaved[i].vertex.z;

			//aabb_max.setMax(interleaved[i].vertex);
			if (interleaved[i].vertex.x > aabb_max.x) aabb_max.x = interleaved[i].vertex.x;
			if (interleaved[i].vertex.y > aabb_max.y) aabb_max.y = interleaved[i].vertex.y;
			if (interleaved[i].vertex.z > aabb_max.z) aabb_max.z = interleaved[i].vertex.z;
		}
	}
	box.center = (aabb_max + aabb_min) * 0.5f;
	box.halfsize = aabb_max - box.center;
}

Mesh* wire_box = NULL;

void Mesh::renderBounding(const glm::mat4& model, bool world_bounding)
{
	if (!wire_box)
	{
		wire_box = new Mesh();
		wire_box->createWireBox();
		wire_box->uploadToVRAM();
	}

	Shader* sh = Shader::getDefaultShader("flat");
	sh->enable();
	sh->setUniform("u_viewprojection", Camera::current->viewprojection_matrix);

	glm::mat4 matrix;
	matrix = glm::translate(matrix, box.center);
	matrix = glm::scale(matrix, box.halfsize);

	sh->setUniform("u_color", glm::vec4(1, 1, 0, 1));
	sh->setUniform("u_model", matrix * model);
	wire_box->render(GL_LINES);

	if (world_bounding)
	{
		BoundingBox AABB = transformBoundingBox(model, box);
		matrix = glm::mat4(1.f);
		matrix = glm::translate(matrix, AABB.center);
		matrix = glm::scale(matrix, AABB.halfsize);
		sh->setUniform("u_model", matrix);
		sh->setUniform("u_color", glm::vec4(0, 1, 1, 1));
		wire_box->render(GL_LINES);
	}

	sh->disable();
}



Mesh* Mesh::getQuad()
{
	static Mesh* quad = NULL;
	if (!quad)
	{
		quad = new Mesh();
		quad->createQuad(0, 0, 2, 2, false);
		quad->uploadToVRAM();
	}
	return quad;
}

Mesh* Mesh::Get(const char* filename)
{
	assert(filename);
	std::map<std::string, Mesh*>::iterator it = sMeshesLoaded.find(filename);
	if (it != sMeshesLoaded.end())
		return it->second;

	Mesh* m = new Mesh();
	std::string name = filename;

	//detect format
	char file_format = 0;
	std::string ext = name.substr(name.find_last_of(".") + 1);
	if (ext == "ase" || ext == "ASE")
		file_format = FORMAT_ASE;
	else if (ext == "obj" || ext == "OBJ")
		file_format = FORMAT_OBJ;
	else if (ext == "mbin" || ext == "MBIN")
		file_format = FORMAT_MBIN;
	else if (ext == "mesh" || ext == "MESH")
		file_format = FORMAT_MESH;
	else
	{
		std::cerr << "Unknown mesh format: " << filename << std::endl;
		return NULL;
	}

	//stats
	long time = getTime();
	std::cout << " + Mesh loading: " << filename << " ... ";
	std::string binfilename = filename;

	if (file_format != FORMAT_MBIN)
		binfilename = binfilename + ".mbin";

	//try loading the binary version
	if (use_binary && m->readBin(binfilename.c_str()))
	{
		if (interleave_meshes && m->interleaved.size() == 0)
		{
			std::cout << "[INTERL] ";
			m->interleaveBuffers();
		}

		if (auto_upload_to_vram)
		{
			std::cout << "[VRAM] ";
			m->uploadToVRAM();
		}

		std::cout << "[OK BIN]  Faces: " << (m->interleaved.size() ? m->interleaved.size() : m->vertices.size()) / 3 << " Time: " << (getTime() - time) * 0.001 << "sec" << std::endl;
		m->registerMesh(filename);
		return m;
	}

	//load the ascii version
	bool loaded = false;
	if (file_format == FORMAT_OBJ)
		loaded = m->loadOBJ(filename);
	/*else if (file_format == FORMAT_ASE)
		loaded = m->loadASE(filename);*/
	else if (file_format == FORMAT_MESH)
		loaded = m->loadMESH(filename);

	if (!loaded)
	{
		delete m;
		std::cout << "[ERROR]: Mesh not found" << std::endl;
		return NULL;
	}

	//to optimize, interleave the meshes
	if (interleave_meshes)
	{
		std::cout << "[INTERL] ";
		m->interleaveBuffers();
	}

	//and upload them to VRAM
	if (auto_upload_to_vram)
	{
		std::cout << "[VRAM] ";
		m->uploadToVRAM();
	}

	std::cout << "[OK]  Faces: " << m->vertices.size() / 3 << " Time: " << (getTime() - time) * 0.001 << "sec" << std::endl;
	if (use_binary)
	{
		std::cout << "\t\t Writing .BIN ... ";
		m->writeBin(filename);
		std::cout << "[OK]" << std::endl;
	}

	m->registerMesh(name);
	return m;
}

void Mesh::registerMesh(std::string name)
{
	this->name = name;
	sMeshesLoaded[name] = this;
}