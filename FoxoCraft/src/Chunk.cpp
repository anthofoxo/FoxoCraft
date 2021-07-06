#include "Chunk.h"

#include <unordered_map>

#include <FoxoCommons/DebugTrap.h>
#include <FoxoCommons/Util.h>

#include "Log.h"
#include <FoxoCommons/FrustumCull.h>

#include <GLFW/glfw3.h>

#include <iostream>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "EasyConditionVariable.h"

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
			FC_LOG_ERROR("Registers have been locked, cannot modify further");
			psnip_trap();
			return;
		}

		s_BlockFaces[id] = face;
	}

	void RegisterBlock(const std::string& id, const Block& block)
	{
		if (s_LockModify)
		{
			FC_LOG_ERROR("Registers have been locked, cannot modify further");
			psnip_trap();
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

	// US = unsafe
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
		if (GetBlockLSUS(ls) == block) return;

		m_Data[IndexLS(ls)] = block;
	}

	void Chunk::Generate()
	{
		constexpr const std::array<glm::dvec2, 4> s_MapOffsets =
		{
			glm::dvec2(9134542, 312781),
			glm::dvec2(3320191, -554605),
			glm::dvec2(-9743106, 761011),
			glm::dvec2(-4211348, -812416)
		};

		OpenSimplexNoise& noise = m_World->m_Generator.m_Generator;

		Block* grass = GetBlock("core.grass_block");
		Block* dirt = GetBlock("core.dirt");
		Block* stone = GetBlock("core.stone");

		glm::ivec3 ws;
		glm::ivec3 ls;

		for (ls.z = 0; ls.z < s_ChunkSize; ++ls.z)
		{
			ws.z = ls.z + m_Pos.z * s_ChunkSize;

			for (ls.x = 0; ls.x < s_ChunkSize; ++ls.x)
			{
				ws.x = ls.x + m_Pos.x * s_ChunkSize;

				double dheight = 0.;

				double factor = 128.0f;
				double factor2 = 64.0f;
				for(size_t i = 0; i < s_MapOffsets.size(); ++i)
				{
					dheight += noise.Evaluate((ws.x + s_MapOffsets[i].x) / factor, (ws.z + s_MapOffsets[i].y) / factor) * factor2;
					factor *= 0.5f;
					factor2 *= 0.5f;
				}

				int height = static_cast<int>(dheight);

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
				}
			}
		}
	}

	void Chunk::UploadMesh()
	{
		m_Vao = 0;
		m_Vbo = 0;

		// no data was in the chunk, dont create gpu information
		if (data.size() != 0)
		{
			m_Vbo = FoxoCommons::BufferObject();
			m_Vbo.Storage(data.size() * sizeof(float), data.data(), GL_NONE);

			m_Vao = FoxoCommons::VertexArray();
			m_Vao.VertexBuffer(0, m_Vbo, 0, 9 * sizeof(float));
			m_Vao.EnableAttrib(0);
			m_Vao.EnableAttrib(1);
			m_Vao.EnableAttrib(2);
			m_Vao.AttribFormat(0, 3, GL_FLOAT, false, 0 * sizeof(float));
			m_Vao.AttribFormat(1, 3, GL_FLOAT, false, 3 * sizeof(float));
			m_Vao.AttribFormat(2, 3, GL_FLOAT, false, 6 * sizeof(float));
			m_Vao.AttribBinding(0, 0);
			m_Vao.AttribBinding(1, 0);
			m_Vao.AttribBinding(2, 0);
		}

		data.clear();
		data.shrink_to_fit();
	}

	void Chunk::BuildMeshV2()
	{
		// W is the side, 0 is top, 1 is side, 2 is bottom
		constexpr const std::array<glm::ivec4, 6> faceDirections =
		{
			glm::ivec4(-1, 0, 0, 1),
			glm::ivec4(1, 0, 0, 1),
			glm::ivec4(0, -1, 0, 2),
			glm::ivec4(0, 1, 0, 0),
			glm::ivec4(0, 0, -1, 1),
			glm::ivec4(0, 0, 1, 1)
		};

		data.clear();
		// data.reserve(Faces::s_Count * 3 * s_ChunkSize);
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

					for (size_t i = 0; i < 6; ++i)
					{
						size_t textureIndex = 0;

						switch (faceDirections[i].w)
						{
							case 0:
								textureIndex = block->m_Top->m_TextureIndex;
								break;
							case 1:
								textureIndex = block->m_Side->m_TextureIndex;
								break;
							case 2:
								textureIndex = block->m_Bottom->m_TextureIndex;
								break;
						}

						if (!GetBlockWSEX(ws + glm::ivec3(faceDirections[i]))) Faces::AppendFace(data, i, ws, textureIndex, m_Count);
					}
				}
			}
		}

		
	}

	bool Chunk::IsAvailable()
	{
		return m_Vao.GetHandle() != 0 && m_Vbo.GetHandle() != 0;
	}

	void Chunk::Render()
	{
		m_Vao.Bind();
		glDrawArrays(GL_TRIANGLES, 0, m_Count);
	}

	WorldGenerator::WorldGenerator()
		: m_Seed(0)
	{
		m_Generator = OpenSimplexNoise(0);
	}

	WorldGenerator::WorldGenerator(int64_t seed)
		: m_Seed(seed)
	{
		m_Generator = OpenSimplexNoise(seed);
	}

	class ThreadQueue
	{
	public:
		void Push(std::shared_ptr<Chunk> chunk)
		{
			std::lock_guard<std::mutex> lck(m_Mutex);
			m_Queue.push_back(chunk);

			m_Cv.notify_one();
		}

		[[nodiscard]] std::shared_ptr<Chunk> Pop()
		{
			std::lock_guard<std::mutex> lck(m_Mutex);

			if (m_Queue.empty()) return nullptr;

			std::shared_ptr<Chunk> back = m_Queue.back();
			m_Queue.pop_back();

			if (m_Queue.empty()) m_Queue.shrink_to_fit();

			return back;
		}

		bool IsEmpty()
		{
			std::lock_guard<std::mutex> lck(m_Mutex);
			return m_Queue.empty();
		}

		void WaitForData()
		{
			std::unique_lock<std::mutex> lck(m_Mutex);
			
			if (!m_Queue.empty() && m_Rdy) return;

			while(m_Queue.empty() && m_Rdy) m_Cv.wait(lck);
		}

		void NotifyAll()
		{
			m_Rdy = false;
			m_Cv.notify_all();
			
		}
	private:
		std::vector<std::shared_ptr<Chunk>> m_Queue;
		std::mutex m_Mutex = std::mutex();
		std::condition_variable m_Cv;
		std::atomic_bool m_Rdy = true;
	};

	static ThreadQueue s_MeshQueue;
	static ThreadQueue s_UploadQueue;

	// Meshing Threading
	std::atomic<bool> m_RunMeshing = false;
	std::vector<std::thread> m_MeshingThreads;
	

	World::World(int64_t seed)
		: m_Generator(seed)
	{
	}

	void World::StartMeshing()
	{
		if (m_RunMeshing) return;

		m_RunMeshing = true;

		unsigned int processorCount = std::thread::hardware_concurrency();
		if (processorCount == 0) processorCount = 4;

		std::cout << "Using " << processorCount << " generator threads\n";

		m_MeshingThreads.reserve(processorCount);

		for (size_t i = 0; i < processorCount; ++i)
		{
			m_MeshingThreads.push_back(std::thread([]()
			{
				while (m_RunMeshing)
				{
					s_MeshQueue.WaitForData();

					if (!s_MeshQueue.IsEmpty())
					{
						// Exception thrown at 0x00007FF65CBB91EE in FoxoCraft.exe: 0xC0000005: Access violation reading location 0xFFFFFFFFFFFFFFF8.
						std::shared_ptr<Chunk> chunk = s_MeshQueue.Pop();

						if (chunk)
						{
							chunk->BuildMeshV2();
							s_UploadQueue.Push(chunk);
						}

						
					}
				}
			}));
		}
	}

	void World::EndMeshing()
	{
		if (!m_RunMeshing) return;

		m_RunMeshing = false;

		s_MeshQueue.NotifyAll();

		for (auto& thread : m_MeshingThreads)
		{
			thread.join();
		}

		m_MeshingThreads.clear();
	}

	std::shared_ptr<Chunk> World::GetChunk(glm::ivec3 cs)
	{

		auto result = m_Chunks.find(cs);

		if (result == m_Chunks.end()) return nullptr;

		return result->second;
	}

	void World::AddChunksCS(const glm::ivec3& rel)
	{
		// Setup initial spawn area
		//int radius = 8;
		int radius = 8;

		std::vector<std::shared_ptr<Chunk>> chunks;

		glm::ivec3 cs;

		for (cs.z = -radius; cs.z <= radius; ++cs.z)
		{
			for (cs.y = -radius; cs.y <= radius; ++cs.y)
			{
				for (cs.x = -radius; cs.x <= radius; ++cs.x)
				{
					glm::ivec3 ccs = cs + rel;

					// Continue if this chunk never been generated
					if (!GetChunk(ccs))
					{
						// TODO: queue neighbor chunks to be remeshed
						std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>(ccs, this);
						chunks.push_back(chunk);
						chunk->Generate();
						m_Chunks[ccs] = chunk;
					}
				}
			}
		}

		// Set the new chunks to generate

		for (auto& chunk : chunks)
		{
			s_MeshQueue.Push(chunk);
		}
	}

	void World::AddChunksWS(const glm::vec3& ws)
	{
		glm::ivec3 cs;
		cs.x = FoxoCommons::FastFloor(ws.x / s_ChunkSizeF);
		cs.y = FoxoCommons::FastFloor(ws.y / s_ChunkSizeF);
		cs.z = FoxoCommons::FastFloor(ws.z / s_ChunkSizeF);

		AddChunksCS(cs);
	}

	
	Block* World::GetBlockWS(glm::vec3 ws)
	{
		return GetBlockWS(glm::ivec3(glm::floor(ws)));
	}

	Block* World::GetBlockWS(glm::ivec3 ws)
	{
		glm::ivec3 cs;
		cs.x = FoxoCommons::FastFloor(ws.x / s_ChunkSizeF);
		cs.y = FoxoCommons::FastFloor(ws.y / s_ChunkSizeF);
		cs.z = FoxoCommons::FastFloor(ws.z / s_ChunkSizeF);

		std::shared_ptr<Chunk> chunk = GetChunk(cs);

		if (!chunk) return nullptr;

		glm::ivec3 ls = ws - cs * static_cast<int>(s_ChunkSize);

		return chunk->GetBlockLS(ls);
	}

	void World::Render(const glm::mat4& projView, DebugData& data)
	{
		{
			// The maximum amount of uploads per frame
			size_t maxCount = 0;

			while (!s_UploadQueue.IsEmpty())
			{
				std::shared_ptr<Chunk> chunk = s_UploadQueue.Pop();
				
				chunk->UploadMesh();

				++maxCount;
				if (maxCount >= 1024) break;
			}

			if(maxCount) std::cout << "Uploaded " << maxCount << '\n';
		}

		// Check if we moved between chunks

		

		// Render chunks

		Frustum f(projView);


		data.chunksTotal = m_Chunks.size();
		data.chunksRendered = 0;

		for (auto& [k, v] : m_Chunks)
		{
			glm::vec3 chunkMin = v->m_Pos;
			chunkMin *= static_cast<float>(s_ChunkSize);
			glm::vec3 chunkMax = chunkMin + static_cast<float>(s_ChunkSize);

			if (v->IsAvailable() && f.IsBoxVisible(chunkMin, chunkMax))
			{
				++data.chunksRendered;
				v->Render();
			}
		}
	}
}