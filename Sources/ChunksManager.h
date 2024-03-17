//
// Created by ilya on 16.03.24.
//

#ifndef OCEANSEDGE_CHUNKSMANAGER_H
#define OCEANSEDGE_CHUNKSMANAGER_H

#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>
#include <entt/entt.hpp>
#include <SGCore/Scene/Scene.h>
#include <bitset>
#include <bit>

#include "GameGlobals.h"

namespace OceansEdge
{
    class ChunksManager
    {
    public:
        static void prepareGrid(const SGCore::Ref<SGCore::Scene>& scene) noexcept;
        
        static void buildChunksGrid(const SGCore::Ref<SGCore::Scene>& scene, const glm::vec3& playerPosition, const size_t& seed);
        
        static std::unordered_map<lvec2, entt::entity, SGCore::MathUtils::GLMVectorHash<lvec2>>& getChunks() noexcept;
    private:
        static inline std::unordered_set<entt::entity> m_freeChunksEntities;
        static inline std::unordered_set<entt::entity> m_occupiedChunksEntities;
        static inline std::unordered_map<lvec2, entt::entity, SGCore::MathUtils::GLMVectorHash<lvec2>> m_lastOccupiedIndices;
        static inline std::vector<entt::entity> m_chunksEntities;
        
        static inline std::unordered_map<lvec2, entt::entity, SGCore::MathUtils::GLMVectorHash<lvec2>> m_chunks;
        // static inline std::unordered_set<entt::entity> m_chunksEntities;
    };
}

#endif //OCEANSEDGE_CHUNKSMANAGER_H
