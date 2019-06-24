#include "Application.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

#include <glm/gtc/random.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <chrono>

#include <imgui.h>
#include <imgui_impl/imgui_impl_glfw.h>
#include <imgui_impl/imgui_impl_opengl3.h>

namespace particles
{
	#pragma region Shader Loaders

	static unsigned int CreateAndLinkProgram(const std::initializer_list<unsigned int>& shaders)
	{
		// Vertex and fragment shaders are successfully compiled.
		// Now time to link them together into a program.
		// Get a program object.
		unsigned int program = glCreateProgram();


		// Attach our shaders to our program
		for (const auto s : shaders)
		{
			glAttachShader(program, s);
		}

		// Link our program
		glLinkProgram(program);

		// Note the different functions here: glGetProgram* instead of glGetShader*.
		GLint isLinked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, (int *)&isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<char> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

			// We don't need the program anymore.
			glDeleteProgram(program);

			// Don't leak shaders either.
			for (const auto s : shaders)
			{
				glDeleteShader(s);
			}

			// Use the infoLog as you see fit.
			ERRO("Program linking failed: {0}", infoLog.data());

			// In this simple program, we'll just leave
			return 0;
		}

		// Don't leak shaders either.

		for (const auto s : shaders)
		{
			glDetachShader(program, s);
		}


		return program;
	}

	static unsigned int LoadShader(unsigned int type, const std::string& source)
	{
		// Create an empty vertex shader handle
		unsigned int shader = glCreateShader(type);

		// Send the vertex shader source code to GL
		// Note that std::string's .c_str is NULL character terminated.
		const GLchar *src = (const GLchar *)source.c_str();
		glShaderSource(shader, 1, &src, 0);

		// Compile the vertex shader
		glCompileShader(shader);

		GLint isCompiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
		if (isCompiled == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<char> infoLog(maxLength);
			glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);

			// We don't need the shader anymore.
			glDeleteShader(shader);

			// Use the infoLog as you see fit.
			ERRO("Shader compilation failed: {0}", infoLog.data());

			// In this simple program, we'll just leave

			return 0;
		}

		return shader;
	}

	#pragma endregion


	#pragma region GLFW Callbacks

	static inline Application* GetApplication(GLFWwindow* window)
	{
		return (Application*)glfwGetWindowUserPointer(window);
	}

	static void GLFW_WindowSize(GLFWwindow* window, int w, int h)
	{
		GetApplication(window)->OnResize(w, h);
	}

	#pragma endregion


	#pragma region Shader source

	static const std::string SH_DRAW = "Draw";
	static const std::string SH_COMPUTE = "Compute";

	static const std::string CS_PARTICLES = R"(
		#version 450

		struct ParticleData {
			vec2 pos;
			vec2 vel;
		};	

		layout(std430, binding = 0) buffer Particles {
			ParticleData particles[];
		};	

		uniform float uDt;
		uniform vec2 uScreenSize;
		uniform vec2 uAttractTo;
		uniform vec2 uRepulseFrom;

		layout( local_size_x = 1024, local_size_y = 1, local_size_z = 1 ) in;
		
		const vec2 GRAVITY = vec2(0, -10);		
		const float ATTRACTION_MAG = 200;
		const float REPULSION_MAG = 800;
	
		void main() {
			uint gid = gl_GlobalInvocationID.x;

			particles[gid].vel += GRAVITY * uDt;

			if(uAttractTo.x > 0 && uAttractTo.y > 0) {
				particles[gid].vel += normalize(uAttractTo - particles[gid].pos) * ATTRACTION_MAG * uDt;
			}

			if(uRepulseFrom.x > 0 && uRepulseFrom.y > 0) {
				particles[gid].vel += normalize(particles[gid].pos- uRepulseFrom) * REPULSION_MAG * uDt;
			}
			
			if((particles[gid].pos.x <= 0 && particles[gid].vel.x < 0) || (particles[gid].pos.x >= uScreenSize.x && particles[gid].vel.x > 0)) {
				particles[gid].vel.x *= -0.5;
			}

			if((particles[gid].pos.y <= 0 && particles[gid].vel.y < 0) || (particles[gid].pos.y >= uScreenSize.y && particles[gid].vel.y > 0)) {
				particles[gid].vel.y *= -0.5;
			}
	
			particles[gid].pos += particles[gid].vel * uDt;
		}

	)";

	static const std::string VS_PARTICLES = R"(
		#version 450

		uniform mat4 uProjection;
		
		layout(location = 0) in vec2 aPosition;	

		out vec4 fsColor;	

		void main() {
			gl_Position = uProjection * vec4(aPosition, 0.0, 1.0);
		}
	)";

	static const std::string FS_PARTICLES = R"(
		#version 450
		
		uniform vec4 uColor;
				
		void main() {
			gl_FragColor = uColor;
		}

	)";

	#pragma endregion



	Application::Application()
	{
	}

	Application::~Application()
	{
	}

	bool Application::Start()
	{

		/* Initialize the library */
		if (!glfwInit())
			return false;

		/* Create a windowed mode window and its OpenGL context */
		m_Window = glfwCreateWindow(1280, 768, "Compute Shader - Particles", NULL, NULL);
		if (!m_Window)
		{
			glfwTerminate();
			return false;
		}

		/* Make the window's context current */
		glfwMakeContextCurrent(m_Window);
		glfwSetWindowUserPointer(m_Window, (void*)this);

		/* Bind glfw callbacks */
		glfwSetWindowSizeCallback(m_Window, &GLFW_WindowSize);

		gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

		/* Initialize ImGui */
				// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		ImGui_ImplOpenGL3_Init(nullptr);
		ImGui_ImplGlfw_InitForOpenGL(m_Window, true);


		/* Initialize particles */
		CreateParticlesVertexArray();
		GeneratePaticles();

		/* Load shaders */
		m_Shaders[SH_DRAW] = CreateAndLinkProgram({ LoadShader(GL_VERTEX_SHADER, VS_PARTICLES), LoadShader(GL_FRAGMENT_SHADER, FS_PARTICLES) });
		m_Shaders[SH_COMPUTE] = CreateAndLinkProgram({ LoadShader(GL_COMPUTE_SHADER, CS_PARTICLES) });

		/* Init GL */
		glClearColor(0, 0, 0, 1);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glPointSize(2.0f);

		auto currTime = std::chrono::high_resolution_clock::now();
		auto prevTime = std::chrono::high_resolution_clock::now();

		/* Loop until the user closes the window */
		while (!glfwWindowShouldClose(m_Window))
		{
			auto[w, h] = GetWindowSize();

			currTime = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> deltaTime = currTime - prevTime;
			prevTime = currTime;

			float dt = deltaTime.count();

			/* Clear the screen */
			glClear(GL_COLOR_BUFFER_BIT);

			/* Update particles */
			{ 
				glm::vec2 attractTo(-1, -1);
				glm::vec2 repulseFrom(-1, -1);

				if (glfwGetMouseButton(m_Window, 0) == GLFW_PRESS)
				{
					double x, y;
					glfwGetCursorPos(m_Window, &x, &y);
					attractTo = glm::vec2(x, h - y);
				} 
				else if (glfwGetMouseButton(m_Window, 1) == GLFW_PRESS)
				{
					double x, y;
					glfwGetCursorPos(m_Window, &x, &y);
					repulseFrom = glm::vec2(x, h - y);
				}


				glBindVertexArray(0);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_Vb);
				glUseProgram(m_Shaders[SH_COMPUTE]);
				GL_CALL(glUniform1f(glGetUniformLocation(m_Shaders[SH_COMPUTE], "uDt"), dt));
				GL_CALL(glUniform2f(glGetUniformLocation(m_Shaders[SH_COMPUTE], "uScreenSize"), w, h));
				GL_CALL(glUniform2fv(glGetUniformLocation(m_Shaders[SH_COMPUTE], "uAttractTo"), 1, glm::value_ptr(attractTo)));
				GL_CALL(glUniform2fv(glGetUniformLocation(m_Shaders[SH_COMPUTE], "uRepulseFrom"), 1, glm::value_ptr(repulseFrom)));
				GL_CALL(glDispatchCompute(GetNumParticles() / 1024, 1, 1));
				GL_CALL(glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT));
			}
			

			/* Render particles */
			{
				glm::mat4 projection = glm::ortho(0.0f, (float)w, 0.0f, (float)h);

				glUseProgram(m_Shaders[SH_DRAW]);

				GL_CALL(glUniformMatrix4fv(glGetUniformLocation(m_Shaders[SH_DRAW], "uProjection"), 1, GL_FALSE, glm::value_ptr(projection)));
				GL_CALL(glUniform4fv(glGetUniformLocation(m_Shaders[SH_DRAW], "uColor"), 1, glm::value_ptr(m_Color)));
				
				GL_CALL(glBindVertexArray(m_Vao));
				GL_CALL(glDrawArrays(GL_POINTS, 0, GetNumParticles()));

			}

			/* ImGui */

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();

			ImGui::NewFrame();
			{

				ImGui::Begin("Compute Shader - Particles");

				ImGui::BulletText("Left click/hold to attract particles");
				ImGui::BulletText("Right click/hold to repulse particles");

				ImGui::SliderInt("Million particles", &m_MillionParticles, MinMillionPartcles, MaxMillionParticles);
				ImGui::ColorEdit4("Color", glm::value_ptr(m_Color));
				ImGui::End();

			}
			ImGui::EndFrame();

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());




			/* Swap front and back buffers */
			glfwSwapBuffers(m_Window);

			/* Poll for and process events */
			glfwPollEvents();
		}

		glfwTerminate();
		return 0;
	}

	std::tuple<int, int> Application::GetWindowSize()
	{
		int w, h;
		glfwGetWindowSize(m_Window, &w, &h);
		return { w, h };
	}

	void Application::OnResize(int w, int h)
	{
		glViewport(0, 0, w, h);
	}

	void Application::CreateParticlesVertexArray()
	{


		glGenVertexArrays(1, &m_Vao);
		glGenBuffers(1, &m_Vb);

		glBindVertexArray(m_Vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_Vb);
	
		GL_CALL(glEnableVertexAttribArray(0));

		GL_CALL(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), 0));


	}
	void Application::GeneratePaticles()
	{
		int n = MaxMillionParticles * 1024 * 1024;
		auto[w, h] = GetWindowSize();
		auto particles = new Particle[n];

		for (size_t i = 0; i < n ; i++)
		{
			particles[i].Position = glm::linearRand(glm::vec2(0, 0), glm::vec2(w, h));
			particles[i].Velocity = glm::linearRand(glm::vec2(-10, -10), glm::vec2(10, 10));
		}

		glBindBuffer(GL_ARRAY_BUFFER, m_Vb);
		GL_CALL(glBufferData(GL_ARRAY_BUFFER, sizeof(Particle) * n, particles, GL_STATIC_DRAW));

		delete[] particles;

	}
}