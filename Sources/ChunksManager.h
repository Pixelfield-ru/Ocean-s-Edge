//
// Created by ilya on 16.03.24.
//

#ifndef OCEANSEDGE_CHUNKSMANAGER_H
#define OCEANSEDGE_CHUNKSMANAGER_H

#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>
#include <entt/entt.hpp>
#include <SGCore/Scene/Scene.h>

#include "GameGlobals.h"

namespace OceansEdge
{
    class ChunksManager
    {
    public:
        static void buildChunksGrid(const SGCore::Ref<SGCore::Scene>& scene, const glm::vec3& playerPosition, const size_t& seed);
        
        static std::unordered_map<ulvec2, entt::entity, SGCore::MathUtils::GLMVectorHash<ulvec2>>& getChunks() noexcept;
    private:
        static inline std::unordered_map<ulvec2, entt::entity, SGCore::MathUtils::GLMVectorHash<ulvec2>> m_chunks;
    };
}

#endif //OCEANSEDGE_CHUNKSMANAGER_H
