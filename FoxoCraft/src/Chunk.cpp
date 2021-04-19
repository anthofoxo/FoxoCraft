#include "Chunk.h"

#include <unordered_map>

#include "OpenSimplexNoise.h"
#include "Log.h"

#include <GLFW/glfw3.h>

namespace FoxoCraft
{
	namespace Faces
	{
		static constexpr size_t s_NumFaces = 6;
		static constexpr size_t s_NumVerts = 6;
		static constexpr size_t s_Count = 9 * s_NumVerts;

		// px py pz nx ny nz tx ty tz
		static constexpr float data[s_Count * s_NumFaces] =
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

		const float* GetFacePointer(size_t faceIndex)
		{
			return data + faceIndex * s_Count;
		}

		void AppendFace(std::vector<float>& data, size_t faceIndex, glm::ivec3 ws, int textureIndex, int& count)
		{
			const float* facePtr = GetFacePointer(faceIndex);

			for (size_t i = 0; i < s_Count; i += 9)
			{
				data.push_back(facePtr[i + 0] + ws.x);
				data.push_back(facePtr[i + 1] + ws.y);
				data.push_back(facePtr[i + 2] + ws.z);
				data.push_back(facePtr[i + 3]);
				data.push_back(facePtr[i + 4]);
				data.push_back(facePtr[i + 5]);
				data.push_back(facePtr[i + 6]);
				data.push_back(facePtr[i + 7]);
				data.push_back(facePtr[i + 8] + textureIndex);
			}

			count += s_NumVerts;
		}
	}

	BlockFace::BlockFace(size_t index)
		: m_TextureIndex(index)
	{
	}

	Block::Block(BlockFace* top, BlockFace* side, BlockFace* bottom)
		: m_Top(top), m_Side(side), m_Bottom(bottom)
	{
	}

	static std::unordered_map<std::string, BlockFace> s_BlockFaces;
	static std::unordered_map<std::string, Block> s_Blocks;
	static bool s_LockModify = false;

	BlockFace* GetBlockFace(const std::string& id)
	{
		auto result = s_BlockFaces.find(id);

		if (result != s_BlockFaces.end())
			return &result->second;

		return nullptr;
	}

	Block* GetBlock(const std::string& id)
	{
		auto result = s_Blocks.find(id);

		if (result != s_Blocks.end())
			return &result->second;

		return nullptr;
	}

	void RegisterBlockFace(const std::string& id, const BlockFace& face)
	{
		if (s_LockModify)
		{
			FE_LOG_ERROR("Registers have been locked, cannot modify further");
			__debugbreak();
			return;
		}

		s_BlockFaces[id] = face;
	}

	void RegisterBlock(const std::string& id, const Block& block)
	{
		if (s_LockModify)
		{
			FE_LOG_ERROR("Registers have been locked, cannot modify further");
			__debugbreak();
			return;
		}

		s_Blocks[id] = block;
	}

	void LockModify()
	{
		s_LockModify = true;
	}

	Chunk::Chunk(glm::ivec3 pos, World* world)
	{
		m_Pos = pos;
		m_World = world;

		m_Data.fill(nullptr);
	}

	Chunk::~Chunk()
	{
		if (m_Vao != 0) glDeleteVertexArrays(1, &m_Vao);
		if (m_Vbo != 0) glDeleteBuffers(1, &m_Vbo);
	}

	bool Chunk::InBoundsLS(glm::ivec3 ls)
	{
		if (ls.x < 0) return false;
		if (ls.y < 0) return false;
		if (ls.z < 0) return false;
		if (ls.x >= s_ChunkSize) return false;
		if (ls.y >= s_ChunkSize) return false;
		if (ls.z >= s_ChunkSize) return false;

		return true;
	}

	glm::ivec3 Chunk::WSLS(glm::ivec3 ws)
	{
		ws -= m_Pos * static_cast<int>(s_ChunkSize);
		return ws;
	}

	Block* Chunk::GetBlockLSUS(glm::ivec3 ls)
	{
		return m_Data[IndexLS(ls)];
	}

	Block* Chunk::GetBlockLS(glm::ivec3 ls)
	{
		if (!InBoundsLS(ls)) return nullptr;
		return GetBlockLSUS(ls);
	}


	Block* Chunk::GetBlockWSEX(glm::ivec3 ws)
	{
		glm::ivec3 ls = WSLS(ws);

		if (InBoundsLS(ls)) return GetBlockLSUS(ls);
		
		return m_World->GetBlockWS(ws);
	}

	void Chunk::SetBlockLS(glm::ivec3 ls, Block* block)
	{
		if (!InBoundsLS(ls)) return;
		m_Data[IndexLS(ls)] = block;
	}

	void Chunk::Generate()
	{
		OpenSimplexNoise noise = OpenSimplexNoise(0);

		Block* grass = GetBlock("core.grass");
		Block* dirt = GetBlock("core.dirt");
		Block* wood = GetBlock("core.wood");
		Block* stone = GetBlock("core.stone");

		glm::ivec3 ws;
		glm::ivec3 ls;

		for (ls.z = 0; ls.z < s_ChunkSize; ++ls.z)
		{
			ws.z = ls.z + m_Pos.z * s_ChunkSize;

			for (ls.x = 0; ls.x < s_ChunkSize; ++ls.x)
			{
				ws.x = ls.x + m_Pos.x * s_ChunkSize;

				double height1 = noise.Evaluate(ws.x / 64.f, ws.z / 64.f) * 32.0f;
				double height2 = noise.Evaluate(ws.x / 32.f, ws.z / 32.f) * 16.0f;
				double height3 = noise.Evaluate(ws.x / 16.f, ws.z / 16.f) * 8.0f;
				double height4 = noise.Evaluate(ws.x / 8.f, ws.z / 8.f) * 4.0f;

				int height = static_cast<int>(height1 + height2 + height3 + height4);

				for (ls.y = 0; ls.y < s_ChunkSize; ++ls.y)
				{
					ws.y = ls.y + m_Pos.y * s_ChunkSize;

					if (ws.y < height)
					{
						if (ws.y < height - 3) SetBlockLS(ls, stone);
						else SetBlockLS(ls, dirt);
					}

					if (ws.y == height)
						SetBlockLS(ls, grass);

					if (((ws.x == 0 || ws.z == 0) && ws.y == 0) || ((ws.y == 0 || ws.z == 0) && ws.x == 0))
						SetBlockLS(ls, wood);
				}
			}
		}
	}

	void Chunk::BuildMeshV2()
	{
		std::vector<float> data;
		m_Count = 0;

		glm::ivec3 ws;
		glm::ivec3 ls;

		for (ls.z = 0; ls.z < s_ChunkSize; ++ls.z)
		{
			ws.z = ls.z + m_Pos.z * s_ChunkSize;
			for (ls.y = 0; ls.y < s_ChunkSize; ++ls.y)
			{
				ws.y = ls.y + m_Pos.y * s_ChunkSize;
				for (ls.x = 0; ls.x < s_ChunkSize; ++ls.x)
				{
					ws.x = ls.x + m_Pos.x * s_ChunkSize;

					Block* block = GetBlockLS(ls);
					if (!block) continue;

					if (!GetBlockWSEX(ws + glm::ivec3(-1, 0, 0))) Faces::AppendFace(data, 0, ws, block->m_Side->m_TextureIndex, m_Count);
					if (!GetBlockWSEX(ws + glm::ivec3(1, 0, 0))) Faces::AppendFace(data, 1, ws, block->m_Side->m_TextureIndex, m_Count);
					if (!GetBlockWSEX(ws + glm::ivec3(0, -1, 0))) Faces::AppendFace(data, 2, ws, block->m_Bottom->m_TextureIndex, m_Count);
					if (!GetBlockWSEX(ws + glm::ivec3(0, 1, 0))) Faces::AppendFace(data, 3, ws, block->m_Top->m_TextureIndex, m_Count);
					if (!GetBlockWSEX(ws + glm::ivec3(0, 0, -1))) Faces::AppendFace(data, 4, ws, block->m_Side->m_TextureIndex, m_Count);
					if (!GetBlockWSEX(ws + glm::ivec3(0, 0, 1))) Faces::AppendFace(data, 5, ws, block->m_Side->m_TextureIndex, m_Count);
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

	void Chunk::Render()
	{
		if (m_Vao == 0 || m_Vbo == 0) return;

		glBindVertexArray(m_Vao);
		glDrawArrays(GL_TRIANGLES, 0, m_Count);
	}

	void World::AddChunks()
	{
		int radius = 3;

		glm::ivec3 cs;

		double timer = glfwGetTime();

		for (cs.z = -radius; cs.z <= radius; ++cs.z)
		{
			for (cs.y = -radius; cs.y <= radius; ++cs.y)
			{
				for (cs.x = -radius; cs.x <= radius; ++cs.x)
				{
					std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>(cs, this);
					chunk->Generate();
					m_Chunks.push_back(chunk);
				}
			}
		}

		timer = glfwGetTime() - timer;
		FE_LOG_INFO("World gen took {}s", timer);

		timer = glfwGetTime();

		for (auto& chunk : m_Chunks)
		{
			chunk->BuildMeshV2();
		}

		timer = glfwGetTime() - timer;
		FE_LOG_INFO("World build took {}s", timer);
	}

	Block* World::GetBlockWS(glm::ivec3 ws)
	{
		glm::ivec3 cs;
		cs.x = static_cast<int>(glm::floor(static_cast<float>(ws.x) / static_cast<float>(s_ChunkSize)));
		cs.y = static_cast<int>(glm::floor(static_cast<float>(ws.y) / static_cast<float>(s_ChunkSize)));
		cs.z = static_cast<int>(glm::floor(static_cast<float>(ws.z) / static_cast<float>(s_ChunkSize)));

		std::shared_ptr<Chunk> foundchunk = nullptr;

		for (std::shared_ptr<Chunk>& chunk : m_Chunks)
		{
			if (chunk->m_Pos == cs)
			{
				foundchunk = chunk;
				break;
			}
		}

		if (!foundchunk) return nullptr;

		glm::ivec3 ls = ws - cs * static_cast<int>(s_ChunkSize);

		return foundchunk->GetBlockLS(ls);
	}

	void World::Render()
	{
		for (auto chunk : m_Chunks)
		{
			chunk->Render();
		}
	}
}