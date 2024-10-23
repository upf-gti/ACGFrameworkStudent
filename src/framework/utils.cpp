#include "utils.h"

#ifdef _WIN32
	#include <windows.h>
#else
	#include <sys/time.h>
#endif

#include "includes.h"

#include "../application.h"
#include "camera.h"
#include "../graphics/shader.h"
#include "../graphics/mesh.h"

#include <glm/gtx/transform.hpp>

long getTime()
{
	#ifdef _WIN32
		return GetTickCount();
	#else
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return (int)(tv.tv_sec * 1000 + (tv.tv_usec / 1000));
	#endif
}

float* snapshot()
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	int x = viewport[0];
	int y = viewport[1];
	int width = viewport[2];
	int height = viewport[3];

	float* data = new float[width * height * 4]; // (R, G, B, A)

	if (!data)
		return 0;

	// glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(x, y, width, height, GL_RGBA, GL_FLOAT, data);

	return data;
}

glm::vec3 transformQuat(const glm::vec3& a, const glm::quat& q)
{
	// benchmarks: https://jsperf.com/quaternion-transform-vec3-implementations-fixed
	float qx = q.x, qy = q.y, qz = q.z, qw = q.w;
	float x = a.x, y = a.y, z = a.z;
	// var qvec = [qx, qy, qz];
	// var uv = vec3.cross([], qvec, a);
	float uvx = qy * z - qz * y,
		uvy = qz * x - qx * z,
		uvz = qx * y - qy * x;
	// var uuv = vec3.cross([], qvec, uv);
	float uuvx = qy * uvz - qz * uvy,
		uuvy = qz * uvx - qx * uvz,
		uuvz = qx * uvy - qy * uvx;
	// vec3.scale(uv, uv, 2 * w);
	float w2 = qw * 2;
	uvx *= w2;
	uvy *= w2;
	uvz *= w2;
	// vec3.scale(uuv, uuv, 2);
	uuvx *= 2;
	uuvy *= 2;
	uuvz *= 2;
	// return vec3.add(out, a, vec3.add(out, uv, uuv));

	glm::vec3 out;

	out[0] = x + uvx + uuvx;
	out[1] = y + uvy + uuvy;
	out[2] = z + uvz + uuvz;

	return out;
}

//this function is used to access OpenGL Extensions (special features not supported by all cards)
//void* getGLProcAddress(const char* name)
//{
//	return SDL_GL_GetProcAddress(name);
//}

//Retrieve the current path of the application
#ifdef __APPLE__
#include "CoreFoundation/CoreFoundation.h"
#endif

#ifdef _WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

std::string getPath()
{
	std::string fullpath;
	// ----------------------------------------------------------------------------
	// This makes relative paths work in C++ in Xcode by changing directory to the Resources folder inside the .app bundle
#ifdef __APPLE__
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
	char path[PATH_MAX];
	if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8*)path, PATH_MAX))
	{
		// error!
	}
	CFRelease(resourcesURL);
	chdir(path);
	fullpath = path;
#else
	char cCurrentPath[1024];
	if (!GetCurrentDir(cCurrentPath, sizeof(cCurrentPath)))
		return "";

	cCurrentPath[sizeof(cCurrentPath) - 1] = '\0';
	fullpath = cCurrentPath;

#endif    
	return fullpath;
}

bool readFile(const std::string& filename, std::string& content)
{
	content.clear();

	long count = 0;

	FILE* fp = fopen(filename.c_str(), "rb");
	if (fp == NULL)
	{
		std::cerr << "::readFile: file not found " << filename << std::endl;
		return false;
	}

	fseek(fp, 0, SEEK_END);
	count = ftell(fp);
	rewind(fp);

	content.resize(count);
	if (count > 0)
	{
		count = fread(&content[0], sizeof(char), count, fp);
	}
	fclose(fp);

	return true;
}

char const* gl_error_string(GLenum const err) noexcept
{
	switch (err)
	{
		// opengl 2 errors (8)
	case GL_NO_ERROR:
		return "GL_NO_ERROR";

	case GL_INVALID_ENUM:
		return "GL_INVALID_ENUM";

	case GL_INVALID_VALUE:
		return "GL_INVALID_VALUE";

	case GL_INVALID_OPERATION:
		return "GL_INVALID_OPERATION";

	case GL_STACK_OVERFLOW:
		return "GL_STACK_OVERFLOW";

	case GL_STACK_UNDERFLOW:
		return "GL_STACK_UNDERFLOW";

	case GL_OUT_OF_MEMORY:
		return "GL_OUT_OF_MEMORY";

	case GL_TABLE_TOO_LARGE:
		return "GL_TABLE_TOO_LARGE";

		// opengl 3 errors (1)
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		return "GL_INVALID_FRAMEBUFFER_OPERATION";

		// gles 2, 3 and gl 4 error are handled by the switch above
	default:
		assert(!"unknown error");
		return nullptr;
	}
}

bool checkGLErrors()
{
	#ifdef _DEBUG
		GLenum errCode;
		const char* errString;

		if ((errCode = glGetError()) != GL_NO_ERROR) {
			errString = gl_error_string(errCode);
			std::cerr << "OpenGL Error: " << errString << std::endl;
			return false;
		}
	#endif

	return true;
}

std::vector<std::string>& split(const std::string& s, char delim, std::vector<std::string>& elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

std::vector<std::string> split(const std::string& s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

bool replace(std::string& str, const std::string& from, const std::string& to) {
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}

//Vector2 getDesktopSize(int display_index)
//{
//	SDL_DisplayMode current;
//	// Get current display mode of all displays.
//	int should_be_zero = SDL_GetCurrentDisplayMode(display_index, &current);
//	return Vector2((float)current.w, (float)current.h);
//}

std::vector<std::string> tokenize(const std::string& source, const char* delimiters, bool process_strings)
{
	std::vector<std::string> tokens;

	std::string str;
	size_t del_size = strlen(delimiters);
	const char* pos = source.c_str();
	char in_string = 0;
	unsigned int i = 0;
	while (*pos != 0)
	{
		bool split = false;

		if (!process_strings || (process_strings && in_string == 0))
		{
			for (i = 0; i < del_size && *pos != delimiters[i]; i++);
			if (i != del_size) split = true;
		}

		if (process_strings && (*pos == '\"' || *pos == '\''))
		{
			if (!str.empty() && in_string == 0) //some chars remaining
			{
				tokens.push_back(str);
				str.clear();
			}

			in_string = (in_string != 0 ? 0 : *pos);
			if (in_string == 0)
			{
				str += *pos;
				split = true;
			}
		}

		if (split)
		{
			if (!str.empty())
			{
				tokens.push_back(str);
				str.clear();
			}
		}
		else
			str += *pos;
		pos++;
	}
	if (!str.empty())
		tokens.push_back(str);
	return tokens;
}

#define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX 0x9048
#define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX 0x9049

//std::string getGPUStats()
//{
//	GLint nTotalMemoryInKB = 0;
//	glGetIntegerv(GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX, &nTotalMemoryInKB);
//	GLint nCurAvailMemoryInKB = 0;
//	glGetIntegerv(GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX, &nCurAvailMemoryInKB);
//	if (glGetError() != GL_NO_ERROR) //unsupported feature by driver
//	{
//		nTotalMemoryInKB = 0;
//		nCurAvailMemoryInKB = 0;
//	}
//
//	std::string str = "FPS: " + std::to_string(Application::instance->fps) + " DCS: " + std::to_string(Mesh::num_meshes_rendered) + " Tris: " + std::to_string(long(Mesh::num_triangles_rendered * 0.001)) + "Ks  VRAM: " + std::to_string(int((nTotalMemoryInKB - nCurAvailMemoryInKB) * 0.001)) + "MBs / " + std::to_string(int(nTotalMemoryInKB * 0.001)) + "MBs";
//	Mesh::num_meshes_rendered = 0;
//	Mesh::num_triangles_rendered = 0;
//	return str;
//}

Mesh* grid = NULL;

void drawGrid()
{
	if (!grid)
	{
		grid = new Mesh();
		grid->createGrid(10);
	}

	glLineWidth(1);
	glEnable(GL_BLEND);
	glDepthMask(false);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Shader* grid_shader = Shader::getDefaultShader("grid");
	grid_shader->enable();
	glm::mat4 m = glm::mat4(1.f);
	m[0][3] = floor(Camera::current->eye.x / 100.0) * 100.0f;
	m[1][3] = 0.0f;
	m[2][3] = floor(Camera::current->eye.z / 100.0f) * 100.0f; // glm::translate(glm::vec3(floor(Camera::current->eye.x / 100.0) * 100.0f, 0.0f, floor(Camera::current->eye.z / 100.0f) * 100.0f));
	grid_shader->setUniform("u_color", glm::vec4(0.7, 0.7, 0.7, 0.7));
	grid_shader->setUniform("u_model", m);
	grid_shader->setUniform("u_camera_position", Camera::current->eye);
	grid_shader->setUniform("u_viewprojection", Camera::current->viewprojection_matrix);
	grid->render(GL_LINES); //background grid
	glDisable(GL_BLEND);
	glDepthMask(true);
	grid_shader->disable();
}


char* fetchWord(char* data, char* word)
{
	int pos = 0;
	while (*data != ',' && *data != '\n' && pos < 254) { word[pos++] = *data; data++; }
	word[pos] = 0;
	if (pos < 254)
		data++; //skip ',' or '\n'
	return data;
}

char* fetchFloat(char* data, float& v)
{
	char w[255];
	data = fetchWord(data, w);
	v = atof(w);
	return data;
}

char* fetchMatrix44(char* data, glm::mat4& m)
{
	char word[255];
	for (int i = 0; i < 16; ++i)
	{
		data = fetchWord(data, word);
		//m.m[i] = atof(word);
		float* mValues = (float*)glm::value_ptr(m);
		mValues[i] = atof(word);
	}
	return data;
}

char* fetchEndLine(char* data)
{
	while (*data && *data != '\n') { data++; }
	if (*data == '\n')
		data++;
	return data;
}

char* fetchBufferFloat(char* data, std::vector<float>& vector, int num)
{
	int pos = 0;
	char word[255];
	if (num)
		vector.resize(num);
	else //read size with the first number
	{
		data = fetchWord(data, word);
		float v = atof(word);
		assert(v);
		vector.resize(v);
	}

	int index = 0;
	while (*data != 0) {
		if (*data == ',' || *data == '\n')
		{
			if (pos == 0)
			{
				data++;
				continue;
			}
			word[pos] = 0;
			float v = atof(word);
			vector[index++] = v;
			if (*data == '\n' || *data == 0)
			{
				if (*data == '\n')
					data++;
				return data;
			}
			data++;
			if (index >= vector.size())
				return data;
			pos = 0;
		}
		else
		{
			word[pos++] = *data;
			data++;
		}
	}

	return data;
}

char* fetchBufferVec3(char* data, std::vector<glm::vec3>& vector)
{
	int pos = 0;
	std::vector<float> floats;
	data = fetchBufferFloat(data, floats);
	vector.resize(floats.size() / 3);
	memcpy(&vector[0], &floats[0], sizeof(float) * floats.size());
	return data;
}

char* fetchBufferVec2(char* data, std::vector<glm::vec2>& vector)
{
	int pos = 0;
	std::vector<float> floats;
	data = fetchBufferFloat(data, floats);
	vector.resize(floats.size() / 2);
	memcpy(&vector[0], &floats[0], sizeof(float) * floats.size());
	return data;
}

char* fetchBufferVec3u(char* data, std::vector<glm::vec3>& vector)
{
	int pos = 0;
	std::vector<float> floats;
	data = fetchBufferFloat(data, floats);
	vector.resize(floats.size() / 3);
	for (int i = 0; i < floats.size(); i += 3)
		vector[i / 3] = glm::vec3(floats[i], floats[i + 1], floats[i + 2]);
	return data;
}

char* fetchBufferVec4ub(char* data, std::vector<glm::vec4>& vector)
{
	int pos = 0;
	std::vector<float> floats;
	data = fetchBufferFloat(data, floats);
	vector.resize(floats.size() / 4);
	for (int i = 0; i < floats.size(); i += 4)
		vector[i / 4] = glm::vec4(floats[i], floats[i + 1], floats[i + 2], floats[i + 3]);
	return data;
}

char* fetchBufferVec4(char* data, std::vector<glm::vec4>& vector)
{
	int pos = 0;
	std::vector<float> floats;
	data = fetchBufferFloat(data, floats);
	vector.resize(floats.size() / 4);
	memcpy(&vector[0], &floats[0], sizeof(float) * floats.size());
	return data;
}
