#include "Sandbox.h"

#include <filesystem>
#include <limits>
#include <vector>

#include <glad/gl.h>
#include <FoxoCommons/Util.h>

#include "stb_image.h"
#include "Log.h"

void MouseLock::Lock(FoxoCommons::Window& window)
{
	if (s_Locked) return;
	s_Locked = true;

	window.SetInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
}

void MouseLock::Unlock(FoxoCommons::Window& window)
{
	if (!s_Locked) return;
	s_Locked = false;

	window.SetInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
}

Player::Player()
{
	m_Transform.m_Pos = { 5, 60, 5 };
}

void Player::Update(GLFWwindow* window, double deltaTime, glm::vec2 mouseDelta, FoxoCraft::World& world)
{
	if (!MouseLock::IsLocked()) return;

	constexpr float sensitivity = 0.1f;
	constexpr float gravity = -10.f;
	constexpr float walkspeed = 4.f;
	constexpr float runspeed = walkspeed * 2.f;
	constexpr float jump = 5.f;

	if (mouseDelta.x != 0.f)
		m_Transform.Rotate(glm::radians(mouseDelta.x * -sensitivity), glm::vec3(0, 1, 0));

	if (mouseDelta.y != 0.f)
		m_TransformExtra.Rotate(glm::radians(mouseDelta.y * -sensitivity), glm::vec3(1, 0, 0));

	vel += gravity * static_cast<float>(deltaTime);

	
	glm::vec3 movement = glm::vec3(0.0f);

	if (glfwGetKey(window, GLFW_KEY_W)) --movement.z;
	if (glfwGetKey(window, GLFW_KEY_S)) ++movement.z;
	if (glfwGetKey(window, GLFW_KEY_A)) --movement.x;
	if (glfwGetKey(window, GLFW_KEY_D)) ++movement.x;

	if (glfwGetKey(window, GLFW_KEY_SPACE) && canJump)
	{
		vel = jump;
		canJump = false;
	}

	//if (glfwGetKey(window, GLFW_KEY_SPACE)) ++movement.y;
	//if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) --movement.y;

	float speed = walkspeed;
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) speed = runspeed;

	if (glm::length2(movement) > 0)
		movement = glm::normalize(movement) * speed * static_cast<float>(deltaTime);

	glm::mat4 matrix = m_Transform.ToMatrix();
	matrix = glm::translate(matrix, glm::vec3(movement));
	FoxoCommons::Transform nextTransform;
	nextTransform.FromMatrix(matrix);
	nextTransform.m_Pos.y += vel * static_cast<float>(deltaTime);

	glm::vec3& currentPos = m_Transform.m_Pos;
	glm::vec3& nextPos = nextTransform.m_Pos;

	if (!world.GetBlockWS(glm::vec3(nextPos.x, currentPos.y, currentPos.z)))
	{
		currentPos.x = nextPos.x;
	}

	if (!world.GetBlockWS(glm::vec3(currentPos.x, nextPos.y, currentPos.z)))
	{
		currentPos.y = nextPos.y;
	}
	else
	{
		vel = 0;
		canJump = true;
	}

	if (!world.GetBlockWS(glm::vec3(currentPos.x, currentPos.y, nextPos.z)))
	{
		currentPos.z = nextPos.z;
	}
}

struct ModLoader
{
	static void Load(FoxoCommons::Texture2DArray& gpuTexture)
	{
		struct TextureInfo
		{
			std::string m_FileName;
			std::string m_Id;

			TextureInfo(std::string_view filename, std::string_view id)
			{
				m_FileName = filename;
				m_Id = id;
			}

			int w = 0, h = 0, c = 0;
			stbi_uc* pixels = nullptr;
		};

		std::vector<TextureInfo> textures;

		// Discover modlist
		std::vector<std::string> mods;
		for (const auto& entry : std::filesystem::directory_iterator("FoxoCraft/mods"))
		{
			mods.push_back(entry.path().filename().u8string());
		}

		// Load textures for each mod
		for (const auto& mod : mods)
		{
			for (const auto& entry : std::filesystem::directory_iterator("FoxoCraft/mods/" + mod + "/textures"))
			{
				std::string texturePath = entry.path().u8string();
				std::string textureName = entry.path().filename().u8string();
				textureName = textureName.substr(0, textureName.find_last_of("."));

				textures.emplace_back(texturePath, mod + '.' + textureName);
			}
		}

		stbi_set_flip_vertically_on_load(true);

		for (auto& texture : textures)
		{
			texture.pixels = stbi_load(texture.m_FileName.data(), &texture.w, &texture.h, &texture.c, 4);
		}

		int largestWidth = 0, largestHeight = 0;

		for (auto& texture : textures)
		{
			if (texture.w > largestWidth) largestWidth = texture.w;
			if (texture.h > largestHeight) largestHeight = texture.h;
		}

		gpuTexture = FoxoCommons::Texture2DArray(GL_LINEAR_MIPMAP_LINEAR, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_RGBA8, largestWidth, largestHeight, textures.size(), true);

		for (size_t i = 0; i < textures.size(); ++i)
		{
			auto& texture = textures[i];

			FoxoCraft::RegisterBlockFace(texture.m_Id, FoxoCraft::BlockFace(i));

			if (texture.pixels)
			{
				gpuTexture.SubImage(0, 0, i, texture.w, texture.h, 1, GL_RGBA, GL_UNSIGNED_BYTE, texture.pixels);
				stbi_image_free(texture.pixels);
			}
		}

		gpuTexture.GenerateMipmaps();
	}
};

namespace FoxoCraft
{
	class GameState final : public FoxoCommons::State
	{
	public:
		GameState() = default;
		virtual ~GameState() = default;

		virtual void Init() override
		{
			int64_t seed = FoxoCommons::GenerateValue(std::numeric_limits<int64_t>::lowest(), std::numeric_limits<int64_t>::max());
			FC_LOG_INFO("Using seed: {}", seed);
			m_World = FoxoCraft::World(seed);

			m_World.StartMeshing();

			

			
		}

		virtual void Update() override
		{
			static glm::ivec3 lastChunkCoord = { -1000, -1000, -1000 };

			Sandbox* game = GetStateManager()->GetUserPtr<Sandbox>();

			s_DebugData.playerPos = m_Player.m_Transform.m_Pos;
			s_DebugData.Draw();

			m_Player.Update(game->m_Window.GetHandle(), game->GetDeltaTime(), game->m_MouseDelta, m_World);

			glm::ivec3 chunkCoord;
			chunkCoord.x = FoxoCommons::FastFloor(m_Player.m_Transform.m_Pos.x / s_ChunkSizeF);
			chunkCoord.y = FoxoCommons::FastFloor(m_Player.m_Transform.m_Pos.y / s_ChunkSizeF);
			chunkCoord.z = FoxoCommons::FastFloor(m_Player.m_Transform.m_Pos.z / s_ChunkSizeF);

			if (chunkCoord != lastChunkCoord)
			{
				// Only do this between chunks
				m_World.AddChunksCS(chunkCoord);
			}

			lastChunkCoord = chunkCoord;

			auto [w, h] = game->m_Window.GetSize();
			m_Camera.m_Aspect = game->m_Window.GetAspect();

			glViewport(0, 0, w, h);
			glClearColor(0.7f, 0.8f, 0.9f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			if (s_DebugData.enableWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			glm::mat4 projectionMatrix = m_Camera.Calculate();

			game->m_Texture.Bind(0);
			game->m_Program.Bind();
			game->m_Program.UniformMat4f("u_Projection", projectionMatrix);
			FoxoCommons::Transform t = m_Player.m_Transform;
			t.m_Pos.y += 1.7f;

			glm::mat4 viewMatrix = glm::inverse(t.ToMatrix() * m_Player.m_TransformExtra.ToMatrix());

			game->m_Program.UniformMat4f("u_View", viewMatrix);
			game->m_Program.UniformMat4f("u_Model", glm::mat4(1.0f));
			game->m_Program.Uniform1i("u_Albedo", 0);

			m_World.Render(projectionMatrix * viewMatrix, s_DebugData);

			if (s_DebugData.enableWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		virtual void Destroy() override
		{
			m_World.EndMeshing();
		}
	private:
		Camera m_Camera;
		Player m_Player;
		World m_World = World(0);
		DebugData s_DebugData;
	};

	class MenuState final : public FoxoCommons::State
	{
	public:
		MenuState() = default;
		virtual ~MenuState() = default;

		virtual void Init() override
		{
		}

		virtual void Update() override
		{
			if (ImGui::Begin(__FUNCTION__))
			{
				if (ImGui::Button("Play"))
				{
					FoxoCommons::StateManager* manager = GetStateManager();
					Sandbox* game = manager->GetUserPtr<Sandbox>();

					game->InvokeNextFrame([manager]()
					{
						manager->SetState<GameState>();
					});
				}
			}
			ImGui::End();
		}

		virtual void Destroy() override
		{
		}
	};

	void Sandbox::Init()
	{
		m_Window = FoxoCommons::Window(1280, 720, "FoxoCraft", []()
		{
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
		});

		if (!m_Window.GetHandle())
		{
			FC_LOG_CRITICAL("Failed to create window");
		}

		m_Window.SetUserPointer(this);

		m_Window.MakeContextCurrent();

		if (!gladLoadGL(glfwGetProcAddress))
		{
			FC_LOG_CRITICAL("Failed to load opengl");
		}

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void) io;
		ImGui::StyleColorsDark();
		ImGui_ImplGlfw_InitForOpenGL(m_Window.GetHandle(), true);
		ImGui_ImplOpenGL3_Init("#version 460 core");

		FC_LOG_INFO(glGetString(GL_VENDOR));
		FC_LOG_INFO(glGetString(GL_RENDERER));
		FC_LOG_INFO(glGetString(GL_VERSION));
		FC_LOG_INFO(glGetString(GL_SHADING_LANGUAGE_VERSION));

		ModLoader::Load(m_Texture);

		FoxoCraft::RegisterBlock("core.grass_block", FoxoCraft::Block(FoxoCraft::GetBlockFace("core.grass_block_top"), FoxoCraft::GetBlockFace("core.grass_block_side"), FoxoCraft::GetBlockFace("core.dirt")));
		FoxoCraft::RegisterBlock("core.dirt", FoxoCraft::Block(FoxoCraft::GetBlockFace("core.dirt"), FoxoCraft::GetBlockFace("core.dirt"), FoxoCraft::GetBlockFace("core.dirt")));
		FoxoCraft::RegisterBlock("core.oak_planks", FoxoCraft::Block(FoxoCraft::GetBlockFace("core.oak_planks"), FoxoCraft::GetBlockFace("core.oak_planks"), FoxoCraft::GetBlockFace("core.oak_planks")));
		FoxoCraft::RegisterBlock("core.stone", FoxoCraft::Block(FoxoCraft::GetBlockFace("core.stone"), FoxoCraft::GetBlockFace("core.stone"), FoxoCraft::GetBlockFace("core.stone")));
		FoxoCraft::RegisterBlock("core.oak_log", FoxoCraft::Block(FoxoCraft::GetBlockFace("core.oak_log_top"), FoxoCraft::GetBlockFace("core.oak_log_side"), FoxoCraft::GetBlockFace("core.oak_log_top")));
		FoxoCraft::LockModify(); // prevent further changes to structures

		std::optional<std::string> vertSrc = FoxoCommons::ReadTextFile("res/chunk.vert");
		std::optional<std::string> fragSrc = FoxoCommons::ReadTextFile("res/chunk.frag");

		if (vertSrc && fragSrc)
		{
			std::vector<FoxoCommons::Shader> shaders;
			shaders.reserve(2);
			shaders.emplace_back(GL_VERTEX_SHADER, vertSrc.value());
			shaders.emplace_back(GL_FRAGMENT_SHADER, fragSrc.value());

			m_Program = FoxoCommons::Program(shaders);
		}
		else
		{
			FC_LOG_INFO("Failed to load shaders");
		}

		glfwSetCursorPosCallback(m_Window.GetHandle(), [](GLFWwindow* window, double x, double y)
		{
			auto* ptr = static_cast<Sandbox*>(glfwGetWindowUserPointer(window));
			ptr->m_MouseCurrent = { static_cast<float>(x), static_cast<float>(y) };
		});

		glfwSetWindowCloseCallback(m_Window.GetHandle(), [](GLFWwindow* window)
		{
			auto* ptr = static_cast<Sandbox*>(glfwGetWindowUserPointer(window));
			ptr->Stop();
		});

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);
		glEnable(GL_DEPTH_TEST);

		m_StateManger.SetUserPtr(this);
		m_StateManger.SetState<MenuState>();
	}

	void Sandbox::Update()
	{
		m_MouseLast = m_MouseCurrent;
		glfwPollEvents();
		m_MouseDelta = m_MouseCurrent - m_MouseLast;

		// Handle mouse lock/unlock
		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) MouseLock::Lock(m_Window);
		else if (glfwGetKey(m_Window.GetHandle(), GLFW_KEY_ESCAPE)) MouseLock::Unlock(m_Window);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		m_StateManger.Update();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		m_Window.SwapBuffers();
	}

	void Sandbox::Destroy()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	double Sandbox::GetTime()
	{
		return glfwGetTime();
	}
}

#if 0
struct Deprecated_Camera
{
	FoxoCommons::Transform transform;

	Deprecated_Camera()
	{
		transform.m_Pos.y = 80;
	}

	void Update(GLFWwindow* window, double deltaTime, float dx, float dy, bool mouseLocked, FoxoCraft::World& world)
	{
		if (!mouseLocked) return;

		if (dx != 0.0f || dy != 0.0f)
		{
			glm::mat4 matrix = transform.Recompose();
			matrix = glm::rotate(matrix, glm::radians(dx * -0.1f), glm::vec3(glm::inverse(matrix) * glm::vec4(0, 1, 0, 0)));
			matrix = glm::rotate(matrix, glm::radians(dy * -0.1f), glm::vec3(1, 0, 0));
			transform.Decompose(matrix);
		}

		constexpr float defaultMovementSpeed = 10;
		constexpr float accelerMovementSpeed = 50;

		float speed = defaultMovementSpeed;

		glm::vec4 movement = glm::vec4(0.0f);

		if (glfwGetKey(window, GLFW_KEY_W)) --movement.z;
		if (glfwGetKey(window, GLFW_KEY_S)) ++movement.z;
		if (glfwGetKey(window, GLFW_KEY_A)) --movement.x;
		if (glfwGetKey(window, GLFW_KEY_D)) ++movement.x;
		if (glfwGetKey(window, GLFW_KEY_Q)) --movement.y;
		if (glfwGetKey(window, GLFW_KEY_E)) ++movement.y;
		if (glfwGetKey(window, GLFW_KEY_SPACE)) ++movement.w;
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) --movement.w;
		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) speed = accelerMovementSpeed;

		if (glm::length2(movement) > 0)
		{
			glm::mat4 matrix = transform.Recompose();

			movement = glm::normalize(movement) * speed * static_cast<float>(deltaTime);

			matrix = glm::translate(matrix, glm::vec3(movement));
			matrix = glm::translate(matrix, glm::vec3(glm::inverse(matrix) * glm::vec4(0, 1, 0, 0)) * movement.w);

			FoxoCommons::Transform newTransform;
			newTransform.Decompose(matrix);

			FoxoCraft::Block* block = world.GetBlockWS(newTransform.m_Pos);

			if (!block)
			{
				transform = newTransform;
			}
		}
	}
};
#endif