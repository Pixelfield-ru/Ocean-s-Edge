//
// Created by ilya on 16.03.24.
//

#ifndef OCEANSEDGE_CHUNK_H
#define OCEANSEDGE_CHUNK_H

#include <vector>
#include <entt/entity/entity.hpp>

#include <unordered_map>
#include <SGUtils/Math/MathUtils.h>
#include <SGUtils/SingleArrayMultiArray.h>
#include "GameGlobals.h"
#include "BlockData.h"

namespace OceansEdge
{
    struct Chunk
    {
        // using blocks_containter_t = std::unordered_map<lvec3, SGCore::Ref<BlockData>, SGCore::MathUtils::GLMVectorHash<lvec3>>;
        // using blocks_containter_t = std::vector<std::vector<std::vector<BlockData>>>;
        // using blocks_container_t = BlockData***;
        using blocks_container_t = SGCore::SingleArrayMultiArray<BlockData, 3>;
        
        blocks_container_t m_blocks;
        
        // blocks_container_t m_blocks { };
        // std::unordered_map<lvec3, SGCore::entity_t, SGCore::MathUtils::GLMVectorHash<lvec3>> m_blocks;
        
    private:
        bool m_dummy = true;
    };
}

#endif //OCEANSEDGE_CHUNK_H
