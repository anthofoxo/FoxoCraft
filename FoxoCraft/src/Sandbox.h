#pragma once

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <FoxoCommons/Window.h>
#include <FoxoCommons/Transform.h>
#include <FoxoCommons/OpenGL/Shader.h>
#include <FoxoCommons/OpenGL/Texture.h>
#include <FoxoCommons/State.h>
#include <FoxoCommons/Application.h>

#include "Chunk.h"
#include "DebugInfo.h"

namespace MouseLock
{
	static bool s_Locked = false;

	inline bool IsLocked()
	{
		return s_Locked;
	}

	void Lock(FoxoCommons::Window& window);
	void Unlock(FoxoCommons::Window& window);
}

struct Player
{
	FoxoCommons::Transform m_Transform;
	FoxoCommons::Transform m_TransformExtra;
	float vel = 0;
	bool canJump = false;

	Player();

	void Update(GLFWwindow* window, double deltaTime, glm::vec2 mouseDelta, FoxoCraft::World& world);
};

struct Camera final
{
	float m_Fov = 90.f;
	float m_Near = 0.01f;
	float m_Far = 1000.f;
	float m_Aspect = 1.f;

	glm::mat4 Calculate()
	{
		return glm::perspective(glm::radians(m_Fov), m_Aspect, m_Near, m_Far);
	}
};

namespace FoxoCraft
{
	class Sandbox : public FoxoCommons::Application
	{
	public:
		Sandbox() = default;
		virtual ~Sandbox() = default;

		virtual void Init() override;
		virtual void Update() override;
		virtual void Destroy() override;
		virtual double GetTime() override;
	public:
		FoxoCommons::Window m_Window;

		glm::vec2 m_MouseLast = glm::vec2();
		glm::vec2 m_MouseCurrent = glm::vec2();
		glm::vec2 m_MouseDelta = glm::vec2();

		FoxoCommons::StateManager m_StateManger;

		FoxoCommons::Program m_Program = 0;
		FoxoCommons::Texture2DArray m_Texture;
	};
}