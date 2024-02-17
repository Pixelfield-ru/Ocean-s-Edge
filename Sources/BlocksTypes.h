//
// Created by ilya on 17.02.24.
//

#ifndef OCEANSEDGE_BLOCKSTYPES_H
#define OCEANSEDGE_BLOCKSTYPES_H

#include <cstddef>
#include <unordered_map>
#include "BlockMeta.h"

namespace OceansEdge
{
    struct BlocksTypes
    {
        friend struct Atlas;
        
        static constexpr const inline size_t OEB_AIR = 0;
        static constexpr const inline size_t OEB_MUD_WITH_GRASS = 1;
        
        static auto& getBlockMeta(const size_t& blockType) noexcept
        {
            return m_blocksMeshesData[blockType];
        }
        
    private:
        static inline std::unordered_map<size_t, BlockMeta> m_blocksMeshesData;
    };
}

#endif //OCEANSEDGE_BLOCKSTYPES_H
