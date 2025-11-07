// ShaderToy.cpp : Defines the entry point for the application.
//

#include "ShaderToy.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <openglDebug.h>
#include <shaderLoader.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using namespace std;

float triangleVertices[] = {
		1.0, 1.0, 0,	1, 0, 0,	// Vertex 1
	   -1.0, 1.0, 0,	0, 1, 0,	// Vertex 2
	   -1.0, -1.0, 0,	0, 0, 1,	// Vertex 3
		1.0, -1.0, 0,	0, 0, 1		// Vertex 4
};

//float triangleVertices[] = {
//		0.5, 0.5, 0,	1, 0, 0,	// Vertex 1
//	   -0.5, 0.5, 0,	0, 1, 0,	// Vertex 2
//	   -0.5, -0.5, 0,	0, 0, 1,	// Vertex 3
//		0.5, -0.5, 0,	0, 0, 1		// Vertex 4
//};

unsigned short indices[] = {
	0, 1, 2,
	0, 2, 3
};

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main()
{
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

#pragma region report opengl errors to std
	// Enable opengl debugging output.
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#pragma endregion

	GLFWwindow* window = glfwCreateWindow(640, 480, "ShaderToy", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(window, key_callback);

	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		cout << "Failed to initialize GLAD" << endl;
		return -1;
	}
	glfwSwapInterval(1);

#pragma region report opengl errors to std
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(glDebugOutput, 0);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#pragma endregion

	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");

	// Vertex Array Object
	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Vertex Buffer Object
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

	// Index Buffer Object
	GLuint ibo = 0;
	glGenBuffers(1, &ibo);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);


	// Unbind VAO
	glBindVertexArray(0);

	Shader s;
	s.loadShaderProgramFromFile(RESOURCES_PATH "vertex.vert", RESOURCES_PATH "fragment.frag");
	s.bind();

	GLint u_time = s.getUniform("iTime");
	GLint u_color = s.getUniform("u_color");
	GLint u_resolution = s.getUniform("iResolution");
	GLint iTimeDelta = s.getUniform("iTimeDelta");

	float startTime = (float)clock() / 750.f;
	float timer = 0.0f;
	bool timerActive = true;

	while (!glfwWindowShouldClose(window))
	{
		timer += timerActive ? 0.016f : 0.0f;
		// cout << "Time: " << timer << "\n";

		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height); // Set the viewport to the window dimensions
		glClear(GL_COLOR_BUFFER_BIT); // Clear the screen

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		float barHeight = 40.0f;

		ImVec2 barPos = ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - barHeight);
		ImVec2 barSize = ImVec2(viewport->Size.x, barHeight);

		ImGui::SetNextWindowPos(barPos);
		ImGui::SetNextWindowSize(barSize);

		ImGuiWindowFlags barFlags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoCollapse;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 5));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(30, 30, 30, 255));

		ImGui::Begin("BottomBar", nullptr, barFlags);

		if (ImGui::Button(timerActive ? "Pause" : "Play")) {
			timerActive = !timerActive;
		}
		ImGui::SameLine();
		ImGui::Text("Time: %.2f", timer);
		ImGui::SameLine();
		ImGui::Text("| Resolution: %d x %d", width, height);

		float rightAlign = ImGui::GetContentRegionAvail().x - 120;
		ImGui::SameLine(rightAlign);

		if (ImGui::Button("Restart"))
		{
			// reset host-side state
			timer = 0.0f;
			timerActive = true;
			startTime = (float)clock() / 750.f;

			// --- Delete previous GL objects ---
			// Unbind before deleting
			glBindVertexArray(0);
			if (ibo) { glDeleteBuffers(1, &ibo); ibo = 0; }
			if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
			if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }

			// If your Shader class exposes the program ID, delete it.
			// Add a getProgramID() method to Shader if you don't have one.
			#if 1
			// If Shader::getProgramID() exists:
						GLuint prog = 0;
						// try-catch like access; if your Shader doesn't have this, remove these two lines
						// prog = s.getProgramID();
						// if (prog) { glDeleteProgram(prog); }
			#endif

			// --- Recreate VAO/VBO/IBO exactly like initialization ---
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			glGenBuffers(1, &vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);

			// position attribute
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

			// color attribute (re-upload ensures initial vertex colors are present)
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

			glGenBuffers(1, &ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

			// --- Reload shader from file and bind ---
			s.loadShaderProgramFromFile(RESOURCES_PATH "vertex.vert", RESOURCES_PATH "fragment.frag");
			s.bind();

			// --- Refresh uniform locations (program changed) ---
			u_time = s.getUniform("iTime");
			u_color = s.getUniform("u_color");
			u_resolution = s.getUniform("iResolution");
			iTimeDelta = s.getUniform("iTimeDelta");

			// --- Set initial uniform values to match first-run ---
			if (u_color != -1) {
				// If your shader has u_color and you want it at some default:
				glUniform3f(u_color, 0.0f, 0.0f, 0.0f); // adjust default as needed
			}
			if (u_time != -1) {
				glUniform1f(u_time, 0.0f);
			}
			// update resolution
			int w, h; glfwGetFramebufferSize(window, &w, &h);
			if (u_resolution != -1) glUniform2f(u_resolution, (float)w, (float)h);

			// --- Clear framebuffer so there's no leftover accumulated image ---
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glFinish();
		}

		ImGui::SameLine();
		if (ImGui::Button("Exit")) {
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}

		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		s.bind();

		float currentTime = (float)clock() / 750.f;
		if (u_time != -1) glUniform1f(u_time, currentTime - startTime);
		if (u_resolution != -1) glUniform2f(u_resolution, (float)width, (float)height);

		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

		ImGui::Render();
		int display_w, display_h = 0;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		
		glfwSwapBuffers(window); // Swap front and back buffers
		glfwPollEvents();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window); // Clean up and exit

	glfwTerminate(); // Terminate GLFW

	return 0;
}