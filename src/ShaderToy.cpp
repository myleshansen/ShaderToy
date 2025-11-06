// ShaderToy.cpp : Defines the entry point for the application.
//

#include "ShaderToy.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace std;

int main()
{
	if (!glfwInit())
	{
		return -1;
	}

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

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	cout << "Hello CMake." << endl;
	return 0;
}
