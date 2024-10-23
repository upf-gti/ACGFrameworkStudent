#pragma once

//under windows we need this file to make opengl work
#ifdef _WIN32
	#define NOMINMAX
	#include <windows.h>
#endif

#ifndef APIENTRY
	#define APIENTRY
#endif

#ifdef _WIN32
	#define USE_GLEW
	#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES

#include <GLFW/glfw3.h>

//GLUT
#ifdef _WIN32
	//#include <GL/glext.h>
	#include "GL/GLU.h"
#endif

#include <iostream>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

//used to access opengl extensions
#define REGISTER_GLEXT(RET, FUNCNAME, ...) typedef RET ( * FUNCNAME ## _func)(__VA_ARGS__); FUNCNAME ## _func FUNCNAME = NULL;
#define IMPORT_GLEXT(FUNCNAME) FUNCNAME = (FUNCNAME ## _func) SDL_GL_GetProcAddress(#FUNCNAME); if (FUNCNAME == NULL) { std::cout << "ERROR: This Graphics card doesnt support " << #FUNCNAME << std::endl; }