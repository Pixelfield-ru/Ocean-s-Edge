//
// Created by ilya on 17.02.24.
//

#ifndef OCEANSEDGE_ATLASPOINTMETA_H
#define OCEANSEDGE_ATLASPOINTMETA_H

#include <cstddef>

namespace OceansEdge
{
    struct AtlasPointMeta
    {
        friend struct Atlas;
        
        size_t m_blockType = 0;
        size_t m_meshVertexID = 0;
        
        float m_widthInAtlas = 0.0f;
        float m_heightInAtlas = 0.0f;
        
        float m_xPositionInAtlas = 0.0f;
        float m_yPositionInAtlas = 0.0f;
        
        bool operator==(const AtlasPointMeta& other) const noexcept
        {
            return m_blockType == other.m_blockType && m_meshVertexID == other.m_meshVertexID;
        }
        
        bool operator!=(const AtlasPointMeta& other) const noexcept
        {
            return !(*this == other);
        }
    };
}

#endif //OCEANSEDGE_ATLASPOINTMETA_H
