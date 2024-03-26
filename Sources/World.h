//
// Created by ilya on 16.03.24.
//

#ifndef OCEANSEDGE_WORLD_H
#define OCEANSEDGE_WORLD_H

#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>
#include <entt/entt.hpp>
#include <SGCore/Scene/Scene.h>
#include <bitset>
#include <bit>

#include "GameGlobals.h"
#include "Chunk.h"
#include "SGUtils/Noise/PerlinNoise.hpp"

namespace OceansEdge
{
    class World
    {
    public:
        siv::PerlinNoise::seed_type m_seed = 123456u;
        
        using chunks_container_t = std::unordered_map<lvec2, SGCore::Ref<Chunk>, SGCore::MathUtils::GLMVectorHash<lvec2>>;
        
        void render(const SGCore::Ref<SGCore::Scene>& scene) noexcept;
        
        void prepareGrid(const SGCore::Ref<SGCore::Scene>& scene) noexcept;
        
        void buildChunksGrid(const SGCore::Ref<SGCore::Scene>& scene, const glm::vec3& playerPosition, const size_t& seed);
    private:
        std::uniform_int_distribution<std::mt19937::result_type> m_yDirDistribution;
        std::mt19937 m_yDirDistributionRange;
        float m_yDir = 0;
        
        chunks_container_t m_chunks;
        
        flat_array<std::uint16_t, 3> m_chunkTmpBlocks;
        flat_array<long, 2> m_chunkTmpYMaximums;
        
        siv::PerlinNoise m_perlinNoise { m_seed };
        
        std::unordered_set<SGCore::Ref<Chunk>> m_freeChunksEntities;
        std::unordered_set<SGCore::Ref<Chunk>> m_occupiedChunksEntities;
        std::unordered_map<lvec2, SGCore::Ref<Chunk>, SGCore::MathUtils::GLMVectorHash<lvec2>> m_lastOccupiedIndices;
        
        void addBlockTopSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept;
        void addBlockBottomSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept;
        void addBlockFaceSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept;
        void addBlockBackSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept;
        void addBlockLeftSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept;
        void addBlockRightSideVertices(const ivec3_32& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept;
        // static inline std::unordered_set<entt::entity> m_chunksEntities;
    };
}

#endif //OCEANSEDGE_WORLD_H
