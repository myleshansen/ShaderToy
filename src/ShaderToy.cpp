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

unsigned short indices[] = {
	0, 1, 2,
	0, 2, 3
};

double lastMouseX = 0.0;
double lastMouseY = 0.0;
bool firstMouse = true;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastMouseX = xpos;
		lastMouseY = ypos;
		firstMouse = false;
	}

	double xoffset = xpos - lastMouseX;
	double yoffset = lastMouseY - ypos; // Reversed since y-coordinates go from bottom to top

	lastMouseX = xpos;
	lastMouseY = ypos;
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		// Handle left mouse button press
		// cout << "Left mouse button pressed at (" << lastMouseX << ", " << lastMouseY << ")\n";
	}
}

int main()
{
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

	// Enable opengl debugging output.
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

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

	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(glDebugOutput, 0);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

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

	GLint u_resolution = s.getUniform("iResolution");
	GLint u_time = s.getUniform("iTime");
	GLint u_time_delta = s.getUniform("iTimeDelta");
	GLint u_frame_rate = s.getUniform("iFrameRate");
	GLint u_frame = s.getUniform("iFrame");
	GLint u_mouse = s.getUniform("iMouse");
	GLint u_date = s.getUniform("iDate");


	static int frameCount = 0;

	float timer = 0.0f;
	bool timerActive = false;  // Start paused
	float lastFrameTime = (float)glfwGetTime();

	while (!glfwWindowShouldClose(window))
	{
		frameCount++;
		float currentFrameTime = (float)glfwGetTime();
		float deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;

		if (timerActive) {
			timer += deltaTime;  // Accumulate real time only when playing
		}

		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);

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
			timer = 0.0f;
			timerActive = false; // restart paused
		}

		ImGui::SameLine();
		if (ImGui::Button("Exit")) {
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}

		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		// Shader updates
		s.bind();

		if (u_resolution != -1) glUniform2f(u_resolution, (float)width, (float)height);
		if (u_time != -1) glUniform1f(u_time, timer);
		if (u_time_delta != -1) glUniform1f(u_time_delta, deltaTime);
		if (u_frame_rate != -1) glUniform1f(u_frame_rate, 1.0f / deltaTime);
		if (u_frame != -1) glUniform1i(u_frame, frameCount);
		if (u_mouse != -1)
		{
			float mouseX = (float)lastMouseX;
			float mouseY = (float)(height - lastMouseY); // Invert Y for shader coordinates
			glUniform4f(u_mouse, mouseX, mouseY,
				glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS ? mouseX : 0.0f,
				glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS ? mouseY : 0.0f);
		}
		if (u_date != -1)
		{
			time_t now = time(0);
			tm* ltm = localtime(&now);
			glUniform4f(u_date,
				(float)(ltm->tm_year + 1900),
				(float)(ltm->tm_mon + 1),
				(float)ltm->tm_mday,
				(float)(ltm->tm_hour * 3600 + ltm->tm_min * 60 + ltm->tm_sec));
		}
		

		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

		// ImGui render
		ImGui::Render();
		int display_w, display_h = 0;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);

	glfwTerminate();

	return 0;
}