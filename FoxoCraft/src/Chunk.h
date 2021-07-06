#pragma once

#include <array>
#include <vector>
#include <memory>
#include <string>
#include <deque>
#include <unordered_map>
#include <mutex>

#include <FoxoCommons/OpenGL/Buffer.h>

#include <glm/glm.hpp>
#include <glad/gl.h>

#include <FoxoCommons/OpenSimplexNoise.h>
#include "DebugInfo.h"

namespace FoxoCraft
{
	struct IVec3
	{
		int x, y, z;

		IVec3() = default;
		inline IVec3(float xx, float yy, float zz)
			: x(xx), y(yy), z(zz)
		{
		}
		inline IVec3(glm::ivec3 val)
			: x(val.x), y(val.y), z(val.z)
		{
		}

		
	};

	inline bool operator==(IVec3 lhs, IVec3 rhs)
	{
		return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
	}

	inline constexpr size_t s_ChunkSize = 16;
	inline constexpr float s_ChunkSizeF = static_cast<float>(s_ChunkSize);
	inline constexpr size_t s_ChunkSize2 = s_ChunkSize * s_ChunkSize;
	inline constexpr size_t s_ChunkSize3 = s_ChunkSize * s_ChunkSize * s_ChunkSize;

	namespace Faces
	{
		const float* GetFacePointer(size_t faceIndex);
		void AppendFace(std::vector<float>& data, size_t faceIndex, glm::ivec3 ws, int textureIndex, int& count);
	};

	struct BlockFace
	{
		BlockFace() = default;
		BlockFace(size_t index);

		size_t m_TextureIndex = -1;
	};

	struct Block
	{
		Block() = default;
		Block(BlockFace* top, BlockFace* side, BlockFace* bottom);

		BlockFace* m_Top = nullptr;
		BlockFace* m_Side = nullptr;
		BlockFace* m_Bottom = nullptr;
	};

	BlockFace* GetBlockFace(const std::string& id);
	Block* GetBlock(const std::string& id);

	void RegisterBlockFace(const std::string& id, const BlockFace& face);
	void RegisterBlock(const std::string& id, const Block& block);
	void LockModify();

	struct World;

	struct Chunk final
	{
		glm::ivec3 m_Pos = glm::ivec3(0, 0, 0);
		World* m_World = nullptr;
		std::array<Block*, s_ChunkSize3> m_Data;
		GLint m_Count = 0;
		FoxoCommons::VertexArray m_Vao = 0;
		FoxoCommons::BufferObject m_Vbo = 0;
		std::vector<float> data;

		Chunk(glm::ivec3 pos, World* world);

		inline size_t IndexLS(glm::ivec3 ls)
		{
			return ls.z * s_ChunkSize2 + ls.y * s_ChunkSize + ls.x;
		}

		bool InBoundsLS(glm::ivec3 ls);

		glm::ivec3 WSLS(glm::ivec3 ws);

		Block* GetBlockLSUS(glm::ivec3 ls);

		Block* GetBlockLS(glm::ivec3 ls);

		Block* GetBlockWSEX(glm::ivec3 ws);

		void SetBlockLS(glm::ivec3 ls, Block* block);

		void Generate();

		void UploadMesh();

		void BuildMeshV2();

		bool IsAvailable();

		void Render();
	};

	struct KeyHash
	{
		size_t operator()(const IVec3& k) const
		{
			return std::hash<int>()(k.x) ^ std::hash<int>()(k.y) ^ std::hash<int>()(k.z);
		}
	};

	struct WorldGenerator
	{
		int64_t m_Seed;
		OpenSimplexNoise m_Generator;

		WorldGenerator();
		WorldGenerator(int64_t seed);
	};

	struct World
	{
		WorldGenerator m_Generator;

		World(int64_t seed);

		void StartMeshing();
		void EndMeshing();

		

		void AddChunksCS(const glm::ivec3& cs);
		void AddChunksWS(const glm::vec3& ws);

		std::shared_ptr<Chunk> GetChunk(glm::ivec3 cs);
		Block* GetBlockWS(glm::ivec3 ws);
		Block* GetBlockWS(glm::vec3 ws);

		void Render(const glm::mat4& projView, DebugData& data);
	private:
		std::unordered_map<IVec3, std::shared_ptr<Chunk>, KeyHash> m_Chunks;
	};
}