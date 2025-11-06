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

//float triangleVertices[] = {
//		1.0, 1.0, 0,	1, 0, 0,	// Vertex 1
//	   -1.0, 1.0, 0,	0, 1, 0,	// Vertex 2
//	   -1.0, -1.0, 0,	0, 0, 1,	// Vertex 3
//		1.0, -1.0, 0,	0, 0, 1		// Vertex 4
//};

float triangleVertices[] = {
		0.5, 0.5, 0,	1, 0, 0,	// Vertex 1
	   -0.5, 0.5, 0,	0, 1, 0,	// Vertex 2
	   -0.5, -0.5, 0,	0, 0, 1,	// Vertex 3
		0.5, -0.5, 0,	0, 0, 1		// Vertex 4
};

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

	GLint u_time = s.getUniform("u_time");
	GLint u_color = s.getUniform("u_color");

	while (!glfwWindowShouldClose(window))
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height); // Set the viewport to the window dimensions

		glClear(GL_COLOR_BUFFER_BIT); // Clear the screen

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Window");
		ImGui::Text("Color test");
		static float color[3] = { 0.5,0.5,0.5 };
		ImGui::ColorPicker3("Color:", color);

		if (ImGui::Button("Press me!"))
		{
			std::cout << "The button was pressed\n";
		}

		ImGui::End();

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