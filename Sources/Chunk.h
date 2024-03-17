//
// Created by ilya on 16.03.24.
//

#ifndef OCEANSEDGE_CHUNK_H
#define OCEANSEDGE_CHUNK_H

#include <vector>
#include <entt/entity/entity.hpp>

#include <unordered_map>
#include <SGUtils/Math/MathUtils.h>
#include "GameGlobals.h"

namespace OceansEdge
{
    struct Chunk
    {
        std::unordered_map<lvec2, entt::entity, SGCore::MathUtils::GLMVectorHash<lvec2>> m_blocks;
        
    private:
        bool m_dummy = true;
    };
}

#endif //OCEANSEDGE_CHUNK_H
