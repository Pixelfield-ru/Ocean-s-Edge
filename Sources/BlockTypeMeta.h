//
// Created by ilya on 17.02.24.
//

#ifndef OCEANSEDGE_BLOCKTYPEMETA_H
#define OCEANSEDGE_BLOCKTYPEMETA_H

#include <SGCore/Main/CoreGlobals.h>
#include <SGCore/ImportedScenesArch/IMeshData.h>
#include "AtlasPointMeta.h"

namespace OceansEdge
{
    struct BlockTypeMeta
    {
        SGCore::Ref<SGCore::IMeshData> m_meshData;
    };
}

#endif //OCEANSEDGE_BLOCKTYPEMETA_H
