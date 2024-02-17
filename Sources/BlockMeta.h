//
// Created by ilya on 17.02.24.
//

#ifndef OCEANSEDGE_BLOCKMETA_H
#define OCEANSEDGE_BLOCKMETA_H

#include <SGCore/Main/CoreGlobals.h>
#include <SGCore/ImportedScenesArch/IMeshData.h>
#include "AtlasPointMeta.h"

namespace OceansEdge
{
    struct BlockMeta
    {
        SGCore::Ref<SGCore::IMeshData> m_meshData;
    };
}

#endif //OCEANSEDGE_BLOCKMETA_H
