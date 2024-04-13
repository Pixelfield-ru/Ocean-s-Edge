//
// Created by ilya on 17.02.24.
//

#ifndef OCEANSEDGE_BLOCKSTYPES_H
#define OCEANSEDGE_BLOCKSTYPES_H

#include <cstddef>
#include <unordered_map>
#include "BlockTypeMeta.h"

namespace OceansEdge
{
    struct BlocksTypes
    {
        friend struct Atlas;
        
        static constexpr const inline std::uint16_t OEB_AIR = 0;
        static constexpr const inline std::uint16_t OEB_MUD_WITH_GRASS = 1;
        static constexpr const inline std::uint16_t OEB_BRICKS = 2;
        static constexpr const inline std::uint16_t OEB_STONE = 3;
        static constexpr const inline std::uint16_t OEB_UNKNOWN = std::numeric_limits<std::uint16_t>::max();
        
        static auto& getBlockTypeMeta(const size_t& blockType) noexcept
        {
            return m_blocksTypesMeta[blockType];
        }
        
    private:
        static inline std::unordered_map<size_t, BlockTypeMeta> m_blocksTypesMeta;
    };
}

#endif //OCEANSEDGE_BLOCKSTYPES_H
