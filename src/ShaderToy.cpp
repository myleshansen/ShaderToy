// ShaderToy.cpp : Defines the entry point for the application.
//

#include "ShaderToy.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <openglDebug.h>

using namespace std;

float triangleVertices[] = {
	-0.5f, -0.5f, 0.0f,
	 0.5f, -0.5f, 0.0f,
	 0.0f,  0.5f, 0.0f
};

int main()
{
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

#pragma region report opengl errors to std
	//enable opengl debugging output.
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#pragma endregion

	GLFWwindow* window = glfwCreateWindow(640, 480, "ShaderToy", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		cout << "Failed to initialize GLAD" << endl;
		return -1;
	}

#pragma region report opengl errors to std
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(glDebugOutput, 0);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#pragma endregion

	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo = 0;
	glGenBuffers(1, &vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	glBindVertexArray(0);

	while (!glfwWindowShouldClose(window))
	{
		int w = 0, h = 0;
		glfwGetWindowSize(window, &w, &h);
		glViewport(0, 0, w, h); // Set the viewport to the window dimensions

		glClear(GL_COLOR_BUFFER_BIT); // Clear the screen

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		
		glfwSwapBuffers(window); // Swap front and back buffers
		glfwPollEvents();
	}

	glfwDestroyWindow(window); // Clean up and exit

	glfwTerminate(); // Terminate GLFW

	return 0;
}
