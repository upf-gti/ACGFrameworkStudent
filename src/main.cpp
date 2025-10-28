#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream> // to output

// IMGUI
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "ImGuizmo.h"

#include "application.h"

// Globals
Application* app;
Application* Application::instance = new Application();

void renderGUI(GLFWwindow* window, Application* app) 
{
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
    
	assert(window);

	{
		ImGui::Begin("Controls");

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		if (ImGui::TreeNodeEx("Debugger", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Checkbox("View wireframe", &app->flag_wireframe);
			ImGui::Checkbox("View grid", &app->flag_grid);
			if (ImGui::IsMousePosValid())
				ImGui::Text("Mouse pos: (%g, %g)", xpos, ypos);
			else
				ImGui::Text("Mouse pos: <INVALID>");
			ImGui::Text("Mouse down:");
			for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++) if (ImGui::IsMouseDown(i)) { ImGui::SameLine(); ImGui::Text("b%d (%.02f secs)", i, io.MouseDownDuration[i]); }
			ImGui::TreePop();
		}

		app->renderGUI();

		ImGui::End();
	}

	// Rendering
	ImGui::Render();
	int display_w, display_h;
	glfwGetFramebufferSize(window, &display_w, &display_h);
	glViewport(0, 0, display_w, display_h);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void onKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (app != 0) {
		// Forward input data to ImGui
		ImGuiIO& io = ImGui::GetIO();

		// Do only if is not interacting with the ImGui
		if (!io.WantCaptureKeyboard) {
			if (action == GLFW_PRESS) {
				app->onKeyDown(key, scancode);
			}
			if (action == GLFW_RELEASE) {
				app->onKeyUp(key, scancode);
			}
		}
	}
}

void onMouseEvent(GLFWwindow* window, int button, int action, int mods)
{
	if (app != 0) {
		// Forward input data to ImGui
		ImGuiIO& io = ImGui::GetIO();

		// Do only if is not interacting with the ImGui
		if (!io.WantCaptureMouse) {
			if (button == GLFW_MOUSE_BUTTON_RIGHT) {
				if (action == GLFW_PRESS)
					app->onRightMouseDown();
				else if (action == GLFW_RELEASE) {
					app->onRightMouseUp();
				}
			}
			if (button == GLFW_MOUSE_BUTTON_LEFT) {
				if (action == GLFW_PRESS)
					app->onLeftMouseDown();
				else if (action == GLFW_RELEASE) {
					app->onLeftMouseUp();
				}
			}
			if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
				if (action == GLFW_PRESS)
					app->onMiddleMouseDown();
				else if (action == GLFW_RELEASE) {
					app->onMiddleMouseUp();
				}
			}
		}
	}
}

void onCursorEvent(GLFWwindow* window, double xpos, double ypos)
{
	if (app != 0) {
		// Forward input data to ImGui
		ImGuiIO& io = ImGui::GetIO();
		app->onMousePosition(xpos, ypos);
	}
}


void onScrollEvent(GLFWwindow* window, double xoffset, double yoffset)
{
	if (app != 0) {
		// Forward input data to ImGui
		ImGuiIO& io = ImGui::GetIO();

		// Do only if is not interacting with the ImGui
		if (!io.WantCaptureMouse) {
			app->onScroll(xoffset, yoffset);
		}
	}
}

void onWindowSizeEvent(GLFWwindow* window, int width, int height)
{
	if (app != 0) {
		app->onWindowSize(width, height);
	}
}

void mainLoop(GLFWwindow* window) 
{
	int32_t width, height;
	glfwGetFramebufferSize(window, &width, &height);
	double prev_frame_time = 0.0;
	double xpos, ypos; // mouse position vars

	// Loop until the user closes the window
	while (!glfwWindowShouldClose(window))
	{
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);

		// Poll for and process events
		glfwPollEvents();
		glfwGetCursorPos(window, &xpos, &ypos);
		app->mousePosition.x = static_cast<float>(xpos);
		app->mousePosition.y = static_cast<float>(ypos);

		double curr_time = glfwGetTime();
		double delta_time = curr_time - prev_frame_time;
		prev_frame_time = curr_time;
		app->update((float)delta_time);

		if (app->close) break;

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGuizmo::SetOrthographic(false);
		ImGuizmo::BeginFrame();

		//ImGui::ShowDemoWindow();

		app->render();

		renderGUI(window, app);
		
		// Swap front and back buffers
		glfwSwapBuffers(window);

		ImGui::EndFrame();
	}
}

int main(void) 
{
	// Glfw (Window API)
	if (!glfwInit())
		return -1;

	// Create a windowed mode window and its OpenGL context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	GLFWwindow* window = glfwCreateWindow(1600, 900, "Advanced Computer Graphics 25-26", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	// Make the window's context current
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // enable vsync

	// Glew (OpenGL API)
	if (glewInit() != GLEW_OK)
		std::cout << "[Error] GLEW not initialized" << std::endl;
	fprintf(stdout, "[Info] Using GLEW %s\n", glewGetString(GLEW_VERSION));

	// Get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // renderer as a string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("\n[Info] Renderer %s", renderer);
	printf("\n[Info] OpenGL version supported %s\n\n", version);
	fflush(stdout);

	// Bind event callbacks
	glfwSetKeyCallback(window, onKeyEvent);
	glfwSetMouseButtonCallback(window, onMouseEvent);
	glfwSetCursorPosCallback(window, onCursorEvent);
	glfwSetScrollCallback(window, onScrollEvent);
	glfwSetWindowSizeCallback(window, onWindowSizeEvent);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // enable keyboard controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // enable gamepad controls
	// Setup Dear ImGui style
	ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	const char* glsl_version = "#version 330";
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	app = new Application();
	app->init(window);

	// Main loop, application gets inside here till user closes it
	mainLoop(window);

	// Free memory
	delete app;

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}