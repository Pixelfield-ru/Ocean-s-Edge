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

namespace OceansEdge
{
    class World
    {
    public:
        using chunks_container_t = std::unordered_map<lvec2, SGCore::Ref<Chunk>, SGCore::MathUtils::GLMVectorHash<lvec2>>;
        
        void render(const SGCore::Ref<SGCore::Scene>& scene) noexcept;
        
        void prepareGrid(const SGCore::Ref<SGCore::Scene>& scene) noexcept;
        
        void buildChunksGrid(const SGCore::Ref<SGCore::Scene>& scene, const glm::vec3& playerPosition, const size_t& seed);
    private:
        chunks_container_t m_chunks;
        
        std::unordered_set<SGCore::Ref<Chunk>> m_freeChunksEntities;
        std::unordered_set<SGCore::Ref<Chunk>> m_occupiedChunksEntities;
        std::unordered_map<lvec2, SGCore::Ref<Chunk>, SGCore::MathUtils::GLMVectorHash<lvec2>> m_lastOccupiedIndices;
        
        void addBlockTopSideVertices(const glm::vec3& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept;
        void addBlockBottomSideVertices(const glm::vec3& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept;
        void addBlockFaceSideVertices(const glm::vec3& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept;
        void addBlockBackSideVertices(const glm::vec3& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept;
        void addBlockLeftSideVertices(const glm::vec3& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept;
        void addBlockRightSideVertices(const glm::vec3& blockPos, const SGCore::Ref<Chunk>& chunk) noexcept;
        // static inline std::unordered_set<entt::entity> m_chunksEntities;
    };
}

#endif //OCEANSEDGE_WORLD_H
