#pragma once

#include <vector>
#include <map>
#include <string>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/matrix.hpp>
#include <glm/gtx/transform.hpp>

class Shader; //for binding
class Image; //for displace
class Skeleton; //for skinned meshes

//version from 21/01/2024
#define MESH_BIN_VERSION 12 //this is used to regenerate bins if the format changes

#define MAX_SUBMESH_DRAW_CALLS 16

class BoundingBox
{
public:
	glm::vec3 center;
	glm::vec3 halfsize;
	BoundingBox() {};
	BoundingBox(glm::vec3 center, glm::vec3 halfsize) { this->center = center; this->halfsize = halfsize; };
};

//applies a transform to a AABB so it is 
BoundingBox transformBoundingBox(const glm::mat4 m, const BoundingBox& box);

struct BoneInfo
{
	char name[32]; //max 32 chars per bone name
	glm::mat4 bind_pose;
};

struct sSubmeshDrawCallInfo
{

	char material[32];
	size_t start;//in primitive
	size_t length;//in primitive
};

struct sSubmeshInfo
{
	unsigned int num_draw_calls;
	char name[32];
	sSubmeshDrawCallInfo draw_calls[MAX_SUBMESH_DRAW_CALLS];
};

struct sMaterialInfo
{
	glm::vec3 Ka;
	glm::vec3 Kd;
	glm::vec3 Ks;
};

class Mesh
{
public:
	static std::map<std::string, Mesh*> sMeshesLoaded;
	static bool use_binary; //always load the binary version of a mesh when possible
	static bool interleave_meshes; //loaded meshes will me automatically interleaved
	static bool auto_upload_to_vram; //loaded meshes will be stored in the VRAM
	static long num_meshes_rendered;
	static long num_triangles_rendered;

	std::string name;

	std::vector<sSubmeshInfo> submeshes; //contains info about every submesh
	std::map<std::string, sMaterialInfo> materials; //contains info about every material

	std::vector< glm::vec3 > vertices; //here we store the vertices
	std::vector< glm::vec3 > normals;	 //here we store the normals
	std::vector< glm::vec2 > uvs;	 //here we store the texture coordinates
	std::vector< glm::vec2 > uvs1; //secondary sets of uvs
	std::vector< glm::vec4 > colors; //here we store the colors

	struct tInterleaved {
		glm::vec3 vertex;
		glm::vec3 normal;
		glm::vec2 uv;
	};

	std::vector< tInterleaved > interleaved; //to render interleaved

	std::vector< glm::vec3 > indices; //for indexed meshes

	//for animated meshes
	std::vector< glm::vec4 > bones; //tells which bones afect the vertex (4 max)
	std::vector< glm::vec4 > weights; //tells how much affect every bone
	std::vector< BoneInfo > bones_info; //tells 
	glm::mat4 bind_matrix;

	glm::vec3 aabb_min;
	glm::vec3 aabb_max;
	BoundingBox box;

	float radius;

	unsigned int vertices_vbo_id;
	unsigned int uvs_vbo_id;
	unsigned int normals_vbo_id;
	unsigned int colors_vbo_id;

	unsigned int indices_vbo_id;
	unsigned int interleaved_vbo_id;
	unsigned int bones_vbo_id;
	unsigned int weights_vbo_id;
	unsigned int uvs1_vbo_id;

	Mesh();
	~Mesh();

	void clear();

	void render(unsigned int primitive, int submesh_id = -1, int num_instances = 0);
	void renderInstanced(unsigned int primitive, const glm::mat4* instanced_models, int number);
	void renderInstanced(unsigned int primitive, const std::vector<glm::vec3> positions, const char* uniform_name);
	void renderBounding(const glm::mat4& model, bool world_bounding = true);
	void renderFixedPipeline(int primitive); //sloooooooow
	void renderAnimated(unsigned int primitive, Skeleton* sk);

	void enableBuffers(Shader* shader);
	void drawCall(unsigned int primitive, int submesh_id, int draw_call_id, int num_instances);
	void disableBuffers(Shader* shader);

	bool readBin(const char* filename);
	bool writeBin(const char* filename);

	unsigned int getNumSubmeshes() { return (unsigned int)submeshes.size(); }
	unsigned int getNumVertices() { return (unsigned int)interleaved.size() ? (unsigned int)interleaved.size() : (unsigned int)vertices.size(); }

	//collision testing
	void* collision_model;
	//bool createCollisionModel(bool is_static = false); //is_static sets if the inv matrix should be computed after setTransform (true) or before rayCollision (false)
	////help: model is the transform of the mesh, ray origin and direction, a Vector3 where to store the collision if found, a Vector3 where to store the normal if there was a collision, max ray distance in case the ray should go to infintiy, and in_object_space to get the collision point in object space or world space
	//bool testRayCollision(Matrix44 model, Vector3 ray_origin, Vector3 ray_direction, Vector3& collision, Vector3& normal, float max_ray_dist = 3.4e+38F, bool in_object_space = false);
	//bool testSphereCollision(Matrix44 model, Vector3 center, float radius, Vector3& collision, Vector3& normal);

	//loader
	static Mesh* Get(const char* filename);
	void registerMesh(std::string name);

	//create help meshes
	void createQuad(float center_x, float center_y, float w, float h, bool flip_uvs);
	void createPlane(float size);
	void createSubdividedPlane(float size = 1, int subdivisions = 256, bool centered = false);
	void createCube();
	void createWireBox();
	void createGrid(float dist);
	void displace(Image* heightmap, float altitude);
	static Mesh* getQuad(); //get global quad

	void updateBoundingBox();

	//optimize meshes
	void uploadToVRAM();
	bool interleaveBuffers();

private:
	//bool loadASE(const char* filename);
	bool loadOBJ(const char* filename);
	bool parseMTL(const char* filename);
	bool loadMESH(const char* filename); //personal format used for animations
};