//
// Created by ilya on 22.03.24.
//

#ifndef OCEANSEDGE_BLOCKDATA_H
#define OCEANSEDGE_BLOCKDATA_H

#include <glm/vec3.hpp>
#include "BlocksTypes.h"

namespace OceansEdge
{
    struct BlockData
    {
        // size_t m_d;
        // bool f;
        // glm::vec3 m_position { };
        // size_t m_data = 0;
        // 4 color bytes, 2 bytes for texture id,
        // size_t m_data = 0;
        std::uint16_t m_type = BlocksTypes::OEB_AIR;
        ivec3_16 m_indices = { 0, 0, 0 };
        // size_t m_type = BlocksTypes::OEB_AIR;
    };
}

#endif //OCEANSEDGE_BLOCKDATA_H
