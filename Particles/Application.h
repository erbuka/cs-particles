#pragma once

#include <string>
#include <unordered_map>
#include <tuple>
#include <glm/glm.hpp>

#define CRIT(...) { spdlog::critical(__VA_ARGS__); }
#define INFO(...) { spdlog::info(__VA_ARGS__); }
#define ERRO(...) { spdlog::error(__VA_ARGS__); }

#define GL_CALL(x) { x; auto err = glGetError(); if(err != GL_NO_ERROR) ERRO("GL error at {0}:{1} with code: {2}", __FILE__, __LINE__, err); }


struct GLFWwindow;


namespace particles
{

	struct Particle {
		glm::vec2 Position;
		glm::vec2 Velocity;
	};

	class Application
	{
	public:
		Application();
		~Application();

		bool Start();

		std::tuple<int, int> GetWindowSize();

		void OnResize(int w, int h);

		void CreateParticlesVertexArray();
		void GeneratePaticles();

		int GetNumParticles() { return m_MillionParticles * 1024 * 1024; }


	private:

		static constexpr int MinMillionPartcles = 1, MaxMillionParticles = 10;

		int m_MillionParticles = 1;

		glm::vec4 m_Color = { 0.7, 0.2, 0.3, 0.1 };
		unsigned int m_Vao, m_Vb;
		std::unordered_map<std::string, unsigned int> m_Shaders;
		GLFWwindow* m_Window;

	};
}