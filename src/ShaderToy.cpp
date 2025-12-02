// ShaderToy.cpp : Defines the entry point for the application.
//

#include "ShaderToy.h"

#include "TextEditor.h"

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

static TextEditor editor;
static bool shaderDirty = false;
static std::string userShaderCode;

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


std::filesystem::file_time_type getFileLastWriteTime(const std::string& filePath)
{
	try
	{
		return std::filesystem::last_write_time(filePath);
	}
	catch (const std::filesystem::filesystem_error& e)
	{
		std::cerr << "Error getting last write time for file " << filePath << ": " << e.what() << std::endl;
		return std::filesystem::file_time_type::min();
	}
}

std::string loadUserShaderSection(const std::string& path) {
	std::ifstream file(path);
	std::string line;
	std::string content;

	bool inside = false;

	while (std::getline(file, line)) {
		if (line.find("// BEGIN_USER_CODE") != std::string::npos) {
			inside = true;
			continue;
		}
		if (line.find("// END_USER_CODE") != std::string::npos) {
			inside = false;
			continue;
		}
		if (inside)
			content += line + "\n";
	}

	return content;
}

std::string buildFullFragmentShader(const std::string& userCode) {
	return
		R"(#version 460 core

layout(location = 0) out vec4 fragColor;
in vec2 fragCoord;

uniform float iTime;
uniform vec4 iDate;
uniform vec3 u_color;
uniform vec2 iResolution;

// BEGIN_USER_CODE
)" + userCode +
R"(// END_USER_CODE

void main()
{
    vec2 uv = gl_FragCoord.xy / iResolution.xy;
    vec3 col = userColor(uv);
    fragColor = vec4(col, 1.0);
}
)";
}

GLuint loadShaderFromString(GLenum type, const std::string& source)
{
	GLuint shader = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	// Check compiler errors
	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success) {
		GLint logLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

		std::string log(logLength, ' ');
		glGetShaderInfoLog(shader, logLength, nullptr, &log[0]);

		std::cerr << "[Shader Error] Compile failed:\n" << log << std::endl;

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader)
{
	GLuint program = glCreateProgram();
	if (!program) {
		std::cerr << "Error: Failed to create GL program\n";
		return 0;
	}

	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	glLinkProgram(program);

	// Check linking errors
	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (!success) {
		GLint logLength = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

		std::string log(logLength, ' ');
		glGetProgramInfoLog(program, logLength, nullptr, log.data());

		std::cerr << "[Program Link Error]\n" << log << std::endl;

		glDeleteProgram(program);
		return 0;
	}

	// Optional: detach shaders after linking
	glDetachShader(program, vertexShader);
	glDetachShader(program, fragmentShader);

	return program;
}

bool compileShaderWithErrors(GLenum type, const std::string& src, GLuint& outShader, std::string& outErrors)
{
	outErrors.clear();
	outShader = glCreateShader(type);

	const char* csrc = src.c_str();
	glShaderSource(outShader, 1, &csrc, nullptr);
	glCompileShader(outShader);

	GLint success = 0;
	glGetShaderiv(outShader, GL_COMPILE_STATUS, &success);

	if (!success) {
		GLint logSize = 0;
		glGetShaderiv(outShader, GL_INFO_LOG_LENGTH, &logSize);
		outErrors.resize(logSize);
		glGetShaderInfoLog(outShader, logSize, nullptr, &outErrors[0]);

		glDeleteShader(outShader);
		outShader = 0;
		return false;
	}

	return true;
}

TextEditor::ErrorMarkers buildErrorMarkers(const std::string& log)
{
	TextEditor::ErrorMarkers markers;

	std::istringstream iss(log);
	std::string line;

	while (std::getline(iss, line)) {
		// typical GLSL formats:
		// 0(17) : error C1008: ...
		// ERROR: 0:17: 'xxx' : error
		int lineNum = -1;

		// Pattern 1: 0(17)
		size_t open = line.find('(');
		size_t close = line.find(')');
		if (open != std::string::npos && close != std::string::npos && close > open) {
			lineNum = std::stoi(line.substr(open + 1, close - open - 1));
		}

		// Pattern 2: ERROR: 0:17:
		size_t colon1 = line.find(':');
		size_t colon2 = line.find(':', colon1 + 1);
		if (colon1 != std::string::npos && colon2 != std::string::npos) {
			try {
				lineNum = std::stoi(line.substr(colon1 + 1, colon2 - colon1 - 1));
			}
			catch (...) {}
		}

		if (lineNum >= 0) {
			markers[lineNum] = line;
		}
	}

	return markers;
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

	std::string fragmentShaderPath = RESOURCES_PATH "fragment.frag";
	auto lastWriteTime = getFileLastWriteTime(fragmentShaderPath);

	userShaderCode = loadUserShaderSection(fragmentShaderPath);

	if (userShaderCode.empty()) {
		userShaderCode =
				R"(vec3 userColor(vec2 uv)
					{
						return 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0, 2, 4));
					}
				)";
	}

	editor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
	editor.SetText(userShaderCode);

	static bool compileShaderFromEditor = false;

	while (!glfwWindowShouldClose(window))
	{
		frameCount++;
		float currentFrameTime = (float)glfwGetTime();
		float deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;

		auto currentWriteTime = getFileLastWriteTime(fragmentShaderPath);

		if (currentWriteTime != lastWriteTime)
		{
			std::cout << "Detected change in fragment shader. Reloading..." << std::endl;
			if (s.loadShaderProgramFromFile(RESOURCES_PATH "vertex.vert", RESOURCES_PATH "fragment.frag"))
			{
				timer = 0.0f;
				timerActive = false; // restart paused
				std::cout << "Shader reloaded successfully." << std::endl;
			}
			else
			{
				std::cout << "Failed to reload shader." << std::endl;
			}
			lastWriteTime = currentWriteTime;
		}

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

		ImGui::Begin("Fragment Shader Editor");

		ImGui::Text("Editing: %s", fragmentShaderPath.c_str());
		ImGui::Separator();

		// Calculate size for the editor that leaves some room for buttons
		ImVec2 avail = ImGui::GetContentRegionAvail();

		// Leave ~2 button rows of height + some padding
		float buttonRowHeight = ImGui::GetFrameHeightWithSpacing() * 2.0f;
		avail.y -= buttonRowHeight;
		if (avail.y < 100.0f) avail.y = 100.0f; // clamp to something reasonable

		bool ctrl = ImGui::GetIO().KeyCtrl;

		if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
			// Ctrl+S -> Save file
			std::ofstream out(fragmentShaderPath);
			out << buildFullFragmentShader(editor.GetText());
			out.close();
			std::cout << "[Hotkey] Saved shader.\n";
		}

		if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Enter)) {
			// Ctrl+Enter -> Compile shader
			compileShaderFromEditor = true;
		}

		// Editor widget
		editor.Render("ShaderEditor", avail);

		// Track changes
		if (editor.IsTextChanged()) {
			shaderDirty = true;
		}

		// Buttons
		if (ImGui::Button("Compile Shader") || compileShaderFromEditor)
		{
			compileShaderFromEditor = false;

			std::string editedCode = editor.GetText();
			std::string fullShader = buildFullFragmentShader(editedCode);

			// 1. Write file back to disk
			std::ofstream out(fragmentShaderPath);
			out << fullShader;
			out.close();

			// 2. Compile fragment shader with error capture
			GLuint newFrag;
			std::string errors;

			if (!compileShaderWithErrors(GL_FRAGMENT_SHADER, fullShader, newFrag, errors))
			{
				std::cout << "[Shader] Compile FAILED:\n" << errors << std::endl;

				// Build ImGui error markers
				editor.SetErrorMarkers(buildErrorMarkers(errors));
				continue;
			}

			// Clear error markers if successful
			editor.SetErrorMarkers(TextEditor::ErrorMarkers());

			// 3. Link using your Shader class
			if (s.loadShaderProgramFromFile(RESOURCES_PATH "vertex.vert", fragmentShaderPath.c_str()))
			{
				std::cout << "[Shader] Compilation + link successful.\n";
				s.bind();
				lastWriteTime = getFileLastWriteTime(fragmentShaderPath);
			}
		}

		ImGui::End();

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