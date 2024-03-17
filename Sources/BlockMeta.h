//
// Created by ilya on 16.03.24.
//

#ifndef OCEANSEDGE_BLOCKMETA_H
#define OCEANSEDGE_BLOCKMETA_H

#include <cstddef>
#include "BlocksTypes.h"

namespace OceansEdge
{
    struct BlockMeta
    {
        size_t m_type = BlocksTypes::OEB_AIR;
    };
}

#endif //OCEANSEDGE_BLOCKMETA_H
