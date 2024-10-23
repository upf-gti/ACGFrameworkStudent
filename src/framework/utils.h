/*  by Javi Agenjo 2013 UPF  javi.agenjo@gmail.com
	This contains several functions that can be useful when programming your game.
*/

#pragma once

#include <string>
#include <sstream>
#include <vector>

#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "includes.h"

//General functions **************
long getTime();
float* snapshot();
bool readFile(const std::string& filename, std::string& content);

//generic purposes fuctions
void drawGrid();
glm::vec3 transformQuat(const glm::vec3& a, const glm::quat& q);

//check opengl errors
bool checkGLErrors();

std::string getPath();

//Vector2 getDesktopSize(int display_index = 0);

std::vector<std::string> tokenize(const std::string& source, const char* delimiters, bool process_strings = false);
std::vector<std::string>& split(const std::string& s, char delim, std::vector<std::string>& elems);
std::vector<std::string> split(const std::string& s, char delim);
bool replace(std::string& str, const std::string& from, const std::string& to);

//std::string getGPUStats();

//Used in the MESH and ANIM parsers
char* fetchWord(char* data, char* word);
char* fetchFloat(char* data, float& f);
char* fetchMatrix44(char* data, glm::mat4& m);
char* fetchEndLine(char* data);
char* fetchBufferFloat(char* data, std::vector<float>& vector, int num = 0);
char* fetchBufferVec3(char* data, std::vector<glm::vec3>& vector);
char* fetchBufferVec2(char* data, std::vector<glm::vec2>& vector);
char* fetchBufferVec3u(char* data, std::vector<glm::vec3>& vector);
char* fetchBufferVec4ub(char* data, std::vector<glm::vec4>& vector);
char* fetchBufferVec4(char* data, std::vector<glm::vec4>& vector);
