#include <glad/gl.h>
#include <spdlog/spdlog.h>
#include <unordered_map>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <FoxoCommons/Window.h>
#include <FoxoCommons/Shader.h>
#include <FoxoCommons/Transform.h>
#include <FoxoCommons/Util.h>
#include <FoxoCommons/Texture.h>

#include "Log.h"
#include "OpenSimplexNoise.h"
#include "stb_image.h"

struct BlockFace
{
	BlockFace(size_t index)
		: m_TextureIndex(index)
	{
	}

	size_t m_TextureIndex;
};

struct Block
{
	Block(std::shared_ptr<BlockFace> top, std::shared_ptr<BlockFace> side, std::shared_ptr<BlockFace> bottom)
		: m_Top(top), m_Side(side), m_Bottom(bottom)
	{
	}

	std::shared_ptr<BlockFace> m_Top;
	std::shared_ptr<BlockFace> m_Side;
	std::shared_ptr<BlockFace> m_Bottom;
};

std::unordered_map<std::string, std::shared_ptr<BlockFace>> s_BlockFaces;
std::unordered_map<std::string, std::shared_ptr<Block>> s_Blocks;

std::shared_ptr<BlockFace> GetBlockFace(const std::string& id)
{
	auto result = s_BlockFaces.find(id);

	if (result != s_BlockFaces.end())
		return result->second;

	return nullptr;
}

std::shared_ptr<Block> GetBlock(const std::string& id)
{
	auto result = s_Blocks.find(id);

	if (result != s_Blocks.end())
		return result->second;

	return nullptr;
}

namespace Faces
{
	static constexpr size_t s_NumFaces = 6;
	static constexpr size_t s_NumVerts = 6;
	static constexpr size_t s_Count = 9 * s_NumVerts;

	// px py pz nx ny nz tx ty tz
	static float data[s_Count * s_NumFaces] =
	{
		// left
		0, 0, 0, -1, 0, 0, 0, 0, 0,
		0, 0, 1, -1, 0, 0, 1, 0, 0,
		0, 1, 0, -1, 0, 0, 0, 1, 0,
		0, 1, 1, -1, 0, 0, 1, 1, 0,
		0, 1, 0, -1, 0, 0, 0, 1, 0,
		0, 0, 1, -1, 0, 0, 1, 0, 0,
		// right
		1, 0, 1, 1, 0, 0, 0, 0, 0,
		1, 0, 0, 1, 0, 0, 1, 0, 0,
		1, 1, 1, 1, 0, 0, 0, 1, 0,
		1, 1, 0, 1, 0, 0, 1, 1, 0,
		1, 1, 1, 1, 0, 0, 0, 1, 0,
		1, 0, 0, 1, 0, 0, 1, 0, 0,
		// bottom
		0, 0, 0, 0, -1, 0, 0, 0, 0,
		1, 0, 0, 0, -1, 0, 1, 0, 0,
		0, 0, 1, 0, -1, 0, 0, 1, 0,
		1, 0, 1, 0, -1, 0, 1, 1, 0,
		0, 0, 1, 0, -1, 0, 0, 1, 0,
		1, 0, 0, 0, -1, 0, 1, 0, 0,
		// top
		0, 1, 1, 0, 1, 0, 0, 0, 0,
		1, 1, 1, 0, 1, 0, 1, 0, 0,
		0, 1, 0, 0, 1, 0, 0, 1, 0,
		1, 1, 0, 0, 1, 0, 1, 1, 0,
		0, 1, 0, 0, 1, 0, 0, 1, 0,
		1, 1, 1, 0, 1, 0, 1, 0, 0,
		// back
		1, 0, 0, 0, 0, -1, 0, 0, 0,
		0, 0, 0, 0, 0, -1, 1, 0, 0,
		1, 1, 0, 0, 0, -1, 0, 1, 0,
		0, 1, 0, 0, 0, -1, 1, 1, 0,
		1, 1, 0, 0, 0, -1, 0, 1, 0,
		0, 0, 0, 0, 0, -1, 1, 0, 0,
		// front
		0, 0, 1, 0, 0, 1, 0, 0, 0,
		1, 0, 1, 0, 0, 1, 1, 0, 0,
		0, 1, 1, 0, 0, 1, 0, 1, 0,
		1, 1, 1, 0, 0, 1, 1, 1, 0,
		0, 1, 1, 0, 0, 1, 0, 1, 0,
		1, 0, 1, 0, 0, 1, 1, 0, 0
	};

	float* GetFacePointer(size_t faceIndex)
	{
		return data + faceIndex * s_Count;
	}

	void AppendFace(std::vector<float>& data, size_t faceIndex, int x, int y, int z, int textureIndex, int& count)
	{
		float* facePtr = GetFacePointer(faceIndex);

		for (size_t i = 0; i < s_Count; i += 9)
		{
			data.push_back(facePtr[i + 0] + x);
			data.push_back(facePtr[i + 1] + y);
			data.push_back(facePtr[i + 2] + z);
			data.push_back(facePtr[i + 3]);
			data.push_back(facePtr[i + 4]);
			data.push_back(facePtr[i + 5]);
			data.push_back(facePtr[i + 6]);
			data.push_back(facePtr[i + 7]);
			data.push_back(facePtr[i + 8] + textureIndex);
		}

		count += s_NumVerts;
	}
};

struct Camera
{
	FoxoCommons::Transform transform;

	void Update(GLFWwindow* window, double deltaTime, float dx, float dy)
	{
		if (dx != 0.0f || dy != 0.0f)
		{
			glm::mat4 matrix = transform.Recompose();
			matrix = glm::rotate(matrix, glm::radians(dx * -0.1f), glm::vec3(glm::inverse(matrix) * glm::vec4(0, 1, 0, 0)));
			matrix = glm::rotate(matrix, glm::radians(dy * -0.1f), glm::vec3(1, 0, 0));
			transform.Decompose(matrix);
		}

		float speed = 10;

		glm::vec4 movement = glm::vec4(0.0f);

		if (glfwGetKey(window, GLFW_KEY_W)) --movement.z;
		if (glfwGetKey(window, GLFW_KEY_S)) ++movement.z;
		if (glfwGetKey(window, GLFW_KEY_A)) --movement.x;
		if (glfwGetKey(window, GLFW_KEY_D)) ++movement.x;
		if (glfwGetKey(window, GLFW_KEY_Q)) --movement.y;
		if (glfwGetKey(window, GLFW_KEY_E)) ++movement.y;
		if (glfwGetKey(window, GLFW_KEY_SPACE)) ++movement.w;
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) --movement.w;

		if (glm::length2(movement) > 0)
		{
			glm::mat4 matrix = transform.Recompose();

			movement = glm::normalize(movement) * speed * static_cast<float>(deltaTime);

			matrix = glm::translate(matrix, glm::vec3(movement));
			matrix = glm::translate(matrix, glm::vec3(glm::inverse(matrix) * glm::vec4(0, 1, 0, 0)) * movement.w);
			transform.Decompose(matrix);
		}
	}
};

struct Storage
{
	glm::vec2 mouseLast = glm::vec2();
	glm::vec2 mouseCurrent = glm::vec2();
	glm::vec2 mouseDelta = glm::vec2();
} s_Storage;

static constexpr size_t s_ChunkSize = 32;
static constexpr size_t s_ChunkSize2 = s_ChunkSize * s_ChunkSize;
static constexpr size_t s_ChunkSize3 = s_ChunkSize * s_ChunkSize * s_ChunkSize;

struct ChunkData
{
	int m_X, m_Y, m_Z;

	std::array<std::shared_ptr<Block>, s_ChunkSize3> blocks;

	ChunkData(int x, int y, int z)
	{
		m_X = x;
		m_Y = y;
		m_Z = z;

		for (size_t i = 0; i < blocks.size(); ++i)
		{
			blocks[i] = nullptr;
		}
	}

	~ChunkData()
	{
		if (m_Vao != 0)
			glDeleteVertexArrays(1, &m_Vao);

		if (m_Vbo != 0)
			glDeleteBuffers(1, &m_Vbo);
	}

	size_t Index(int x, int y, int z)
	{
		return z * s_ChunkSize2 + y * s_ChunkSize + x;
	}

	bool InBounds(int x, int y, int z)
	{
		if (x < 0) return false;
		if (y < 0) return false;
		if (z < 0) return false;
		if (x >= s_ChunkSize) return false;
		if (y >= s_ChunkSize) return false;
		if (z >= s_ChunkSize) return false;

		return true;
	}

	std::shared_ptr<Block> GetBlock(int x, int y, int z)
	{
		if (!InBounds(x, y, z)) return nullptr;
		return blocks[Index(x, y, z)];
	}

	void SetBlock(int x, int y, int z, std::shared_ptr<Block> block)
	{
		if (!InBounds(x, y, z)) return;
		blocks[Index(x, y, z)] = block;
	}

	void Generate()
	{
		OpenSimplexNoise noise = OpenSimplexNoise(0);

		std::shared_ptr<Block> grass = ::GetBlock("core.grass");
		std::shared_ptr<Block> dirt = ::GetBlock("core.dirt");
		std::shared_ptr<Block> wood = ::GetBlock("core.wood");
		std::shared_ptr<Block> stone = ::GetBlock("core.stone");

		for (int z = 0; z < s_ChunkSize; ++z)
		{
			int wz = z + m_Z * s_ChunkSize;

			for (int x = 0; x < s_ChunkSize; ++x)
			{
				int wx = x + m_X * s_ChunkSize;

				double height1 = noise.Evaluate(wx / 64.f, wz / 64.f) * 32.0f;
				double height2 = noise.Evaluate(wx / 32.f, wz / 32.f) * 16.0f;
				double height3 = noise.Evaluate(wx / 16.f, wz / 16.f) * 8.0f;
				double height4 = noise.Evaluate(wx / 8.f, wz / 8.f) * 4.0f;

				int height = static_cast<int>(height1 + height2 + height3 + height4);

				for (int y = 0; y < s_ChunkSize; ++y)
				{
					int wy = y + m_Y * s_ChunkSize;
					
					if (wy < height)
					{
						if (wy < height - 3) SetBlock(x, y, z, stone);
						else SetBlock(x, y, z, dirt);
					}

					if (wy == height)
						SetBlock(x, y, z, grass);

					if (((wx == 0 || wz == 0) && wy == 0) || ((wy == 0 || wz == 0) && wx == 0))
						SetBlock(x, y, z, wood);
				}
			}
		}
	}

	void BuildMeshV2()
	{
		std::vector<float> data;
		m_Count = 0;

		for (int z = 0; z < s_ChunkSize; ++z)
		{
			int wz = z + m_Z * s_ChunkSize;

			for (int y = 0; y < s_ChunkSize; ++y)
			{
				int wy = y + m_Y * s_ChunkSize;

				for (int x = 0; x < s_ChunkSize; ++x)
				{
					int wx = x + m_X * s_ChunkSize;

					std::shared_ptr<Block> block = GetBlock(x, y, z);
					if (!block) continue;

					if (!GetBlock(x - 1, y, z)) Faces::AppendFace(data, 0, wx, wy, wz, block->m_Side->m_TextureIndex, m_Count);
					if (!GetBlock(x + 1, y, z)) Faces::AppendFace(data, 1, wx, wy, wz, block->m_Side->m_TextureIndex, m_Count);
					if (!GetBlock(x, y - 1, z)) Faces::AppendFace(data, 2, wx, wy, wz, block->m_Bottom->m_TextureIndex, m_Count);
					if (!GetBlock(x, y + 1, z)) Faces::AppendFace(data, 3, wx, wy, wz, block->m_Top->m_TextureIndex, m_Count);
					if (!GetBlock(x, y, z - 1)) Faces::AppendFace(data, 4, wx, wy, wz, block->m_Side->m_TextureIndex, m_Count);
					if (!GetBlock(x, y, z + 1)) Faces::AppendFace(data, 5, wx, wy, wz, block->m_Side->m_TextureIndex, m_Count);
				}
			}
		}

		if (m_Vao != 0)
		{
			glDeleteVertexArrays(1, &m_Vao);
			m_Vao = 0;
		}

		if (m_Vbo != 0)
		{
			glDeleteBuffers(1, &m_Vbo);
			m_Vbo = 0;
		}

		// no data was in the chunk, dont create gpu information
		if (data.size() != 0)
		{
			glCreateBuffers(1, &m_Vbo);
			glNamedBufferStorage(m_Vbo, data.size() * sizeof(float), data.data(), GL_NONE);

			glCreateVertexArrays(1, &m_Vao);
			glVertexArrayVertexBuffer(m_Vao, 0, m_Vbo, 0, 9 * sizeof(float));
			glEnableVertexArrayAttrib(m_Vao, 0);
			glEnableVertexArrayAttrib(m_Vao, 1);
			glEnableVertexArrayAttrib(m_Vao, 2);
			glVertexArrayAttribFormat(m_Vao, 0, 3, GL_FLOAT, GL_FALSE, 0 * sizeof(float));
			glVertexArrayAttribFormat(m_Vao, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
			glVertexArrayAttribFormat(m_Vao, 2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
			glVertexArrayAttribBinding(m_Vao, 0, 0);
			glVertexArrayAttribBinding(m_Vao, 1, 0);
			glVertexArrayAttribBinding(m_Vao, 2, 0);
		}
	}

	int m_Count = 0;
	GLuint m_Vao = 0;
	GLuint m_Vbo = 0;

	void Render()
	{
		if (m_Vao == 0 || m_Vbo == 0) return;

		glBindVertexArray(m_Vao);
		glDrawArrays(GL_TRIANGLES, 0, m_Count);
	}
};

struct World
{
	std::vector<std::shared_ptr<ChunkData>> m_Chunks;

	World()
	{
		int radius = 3;

		for (int z = -radius; z <= radius; ++z)
		{
			for (int y = -radius; y <= radius; ++y)
			{
				for (int x = -radius; x <= radius; ++x)
				{
					std::shared_ptr<ChunkData> chunk = std::make_shared<ChunkData>(x, y, z);
					chunk->Generate();
					chunk->BuildMeshV2();
					m_Chunks.push_back(chunk);
				}
			}
		}
	}

	void Render()
	{
		for (auto chunk : m_Chunks)
		{
			chunk->Render();
		}
	}
};

static int Run()
{
	FoxoCommons::Window window = FoxoCommons::Window(1280, 720, "FoxoCraft", []()
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	});

	if (!window.GetHandle()) return -1;

	window.MakeContextCurrent();
	window.SetInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGL(glfwGetProcAddress))
	{
		spdlog::error("Failed to load opengl");
		return -1;
	}

	spdlog::info(glGetString(GL_VENDOR));
	spdlog::info(glGetString(GL_RENDERER));
	spdlog::info(glGetString(GL_VERSION));
	spdlog::info(glGetString(GL_SHADING_LANGUAGE_VERSION));

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_DEPTH_TEST);

	FoxoCommons::Texture2DArray texture;
	{
		struct BlockFaceBuilder
		{
			std::string m_FileName;
			std::string m_Id;

			BlockFaceBuilder(std::string_view filename, std::string_view id)
			{
				m_FileName = filename;
				m_Id = id;
			}

			int w = 0, h = 0, c = 0;
			stbi_uc* pixels = nullptr;
		};

		std::vector<BlockFaceBuilder> faces;
		faces.reserve(5);
		faces.emplace_back("res/textures/grass.png", "core.grass");
		faces.emplace_back("res/textures/stone.png", "core.stone");
		faces.emplace_back("res/textures/grass_side.png", "core.grass_side");
		faces.emplace_back("res/textures/dirt.png", "core.dirt");
		faces.emplace_back("res/textures/wood.png", "core.wood");

		stbi_set_flip_vertically_on_load(true);
		
		for (auto& face : faces)
		{
			face.pixels = stbi_load(face.m_FileName.data(), &face.w, &face.h, &face.c, 4);	
		}

		int largestWidth = 0, largestHeight = 0;

		for (auto& face : faces)
		{
			if (face.w > largestWidth) largestWidth = face.w;
			if (face.h > largestHeight) largestHeight = face.h;
		}

		texture = FoxoCommons::Texture2DArray(GL_LINEAR_MIPMAP_LINEAR, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_RGBA8, largestWidth, largestHeight, faces.size(), true);

		for (size_t i = 0; i < faces.size(); ++i)
		{
			auto& face = faces[i];

			s_BlockFaces[face.m_Id] = std::make_shared<BlockFace>(i);

			if (face.pixels)
			{
				texture.SubImage(0, 0, i, face.w, face.h, 1, GL_RGBA, GL_UNSIGNED_BYTE, face.pixels);
				stbi_image_free(face.pixels);
			}
		}

		texture.GenerateMipmaps();
	}

	{
		s_Blocks["core.grass"] = std::make_shared<Block>(GetBlockFace("core.grass"), GetBlockFace("core.grass_side"), GetBlockFace("core.dirt"));
		s_Blocks["core.dirt"] = std::make_shared<Block>(GetBlockFace("core.dirt"), GetBlockFace("core.dirt"), GetBlockFace("core.dirt"));
		s_Blocks["core.wood"] = std::make_shared<Block>(GetBlockFace("core.wood"), GetBlockFace("core.wood"), GetBlockFace("core.wood"));
		s_Blocks["core.stone"] = std::make_shared<Block>(GetBlockFace("core.stone"), GetBlockFace("core.stone"), GetBlockFace("core.stone"));
	}

	World world;

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	FoxoCommons::Program program;

	{
		std::optional<std::string> vertSrc = FoxoCommons::ReadTextFile("res/chunk.vert");
		std::optional<std::string> fragSrc = FoxoCommons::ReadTextFile("res/chunk.frag");

		if (vertSrc && fragSrc)
		{
			std::vector<FoxoCommons::Shader> shaders;
			shaders.reserve(2);
			shaders.emplace_back(GL_VERTEX_SHADER, vertSrc.value());
			shaders.emplace_back(GL_FRAGMENT_SHADER, fragSrc.value());

			program = FoxoCommons::Program(shaders);
		}
		else
		{
			FE_LOG_ERROR("Failed to load shaders");
		}
	}

	window.SetUserPointer(&s_Storage);

	glfwSetCursorPosCallback(window.GetHandle(), [](GLFWwindow* win, double x, double y)
	{
		Storage* ptr = static_cast<Storage*>(glfwGetWindowUserPointer(win));
		ptr->mouseCurrent.x = static_cast<float>(x);
		ptr->mouseCurrent.y = static_cast<float>(y);
	});

	double lastTime, currentTime, deltaTime;

	lastTime = glfwGetTime();
	deltaTime = 1.;

	Camera camera;

	while (!window.ShouldClose())
	{
		currentTime = glfwGetTime();
		deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		s_Storage.mouseLast = s_Storage.mouseCurrent;
		glfwPollEvents();
		s_Storage.mouseDelta = s_Storage.mouseCurrent - s_Storage.mouseLast;

		camera.Update(window.GetHandle(), deltaTime, s_Storage.mouseDelta.x, s_Storage.mouseDelta.y);

		auto [w, h] = window.GetSize();

		glViewport(0, 0, w, h);
		glClearColor(0.7f, 0.8f, 0.9f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		texture.Bind(0);
		program.Bind();
		program.UniformMat4f("u_Projection", glm::infinitePerspective(glm::radians(70.f), window.GetAspect(), 0.01f));
		program.UniformMat4f("u_View", glm::inverse(camera.transform.Recompose()));
		program.UniformMat4f("u_Model", glm::mat4(1.0f));
		program.Uniform1i("u_Albedo", 0);

		world.Render();

		window.SwapBuffers();
	}

	//chunk0 = nullptr;
	//chunk1 = nullptr;

	return 0;
}

int main()
{
	FoxoEngine::CreateLogger();
	int status = Run();
	FoxoEngine::DestroyLogger();
	return status;
}